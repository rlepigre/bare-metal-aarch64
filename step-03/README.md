Step 3: determining the exception level
=======================================

Now that we can run our kernel in QEMU and inspect the state of registers (and
memory) in GDB, the first thing we will do is determine the exception level at
which the processor is initially running.


Exception levels
----------------

The Arm64 architecture provides four exception levels (EL for short):
- `EL0` for normal user applications,
- `EL1` for the operating system's kernel,
- `EL2` for a possible hypervisor,
- `EL3` for low-level firmware / a possible secure monitor.

Obviously, the higher the exception level, the higher the privileges. It is of
course possible to transition between exception levels. For example, when user
applications (running at `EL1`) do not have enough privileges, a "system call"
mechanism can be used to ask the OS kernel (running at `EL1`) for a service. A
specific instruction `svc` (supervisor call) can be used for that. In much the
same way, the kernel can then ask a service from the hypervisor (which runs at
`EL2`) using the `hvc` (hypervisor call) instruction. We will learn how to use
these mechanisms in a later step.


Reading the `CurrentEL` system register
---------------------------------------

To read the current exception level of the main core, we will need to read the
`CurrentEL` (current exception level) system register. To access it, the first
idea that comes to mind is to use the `info register CurrentEL` in GDB, but it
does not work. Perhaps GDB has a different name for that register?

In any case, we do not just want to read the value of `CurrentEL`: only two of
its bits encode the exception level (from the range 3:2). As a consequence, it
is easier to adapt our `boot.S` to put the value of the exception level in one
of our general purpose registers, say `x5`.
```gas
_start:
  // Put the current exception level in x5.
  mrs x5, CurrentEL     // Move the CurrentEL system register into x5.
  ubfx x5, x5, #2, #2   // Extract the relevant bitfield (bits 3:2).

  // Hang forever in a loop.
hang_forever:
  wfe                   // Allow the CPU to go to low-power mode.
  b hang_forever
```
This is done in two steps, by first using `mrs` (move system register) to copy
the value of `CurrentEL` into `x5`, and then accessing the bitfield via `ubfx`
(unsigned bitfield extract). Note that `CurrentEL`, like all system registers,
cannot be used as operand to most instructions, hence the two steps. Note also
that we changed the looping code at the end (previously `b .`) to use `wfe` at
each step of the loop. This will let the CPU go into low-power mode instead of
just eagerly looping (but this only really matters on real hardware).

**Note:** the operands of the `ubfx` instructions are (in order): the register
where to put the result, the register where the initial value is, the index of
the least significant bit of the field we want to extract, the field size as a
number of bits. For range 3:2, the index of the least significant bit is 2 and
there are 2 bits in the range.

We can now use GDB to access the value of `x5` to see what the current EL is.
```
(gdb) si 10
0x0000000000080008 in hang_forever ()
(gdb) info register x5
x5             0x2                 2
```
We first run 10 instructions using `si 10`, which seems to be enough since the
main core is now executing the `hang_forever` loop. We can then read `x5` with
`info register x5` (which can be abbreviated `i r x5`). We are at `EL2`!


What about the actual Raspberry Pi 3?
-------------------------------------

According to the information I could find, the actual Raspberry Pi 3 starts at
`EL2` by default, and can be configured to start at `EL3`. It is possible that
QEMU can be also configured to start at `EL3`, but I could not find how.

**Note:** some online resources wrongly claim that the emulated Raspberry Pi 3
in QEMU starts at `EL1`. In fact, an initial version of this step contained an
error in the operands of the `ubfx` instruction, which lead me to believe that
the initial exception level was `EL1` as well (this did not surpise me since I
read somewhere that this was supposed to be the case).

Since the plan is to eventually run our experiments on the actual hardware, we
will need to run our kernel on the Raspberry Pi 3, and check its behaviour. To
do this, we will first need to set up asynchronous serial communication via an
UART, that we will use to interact with the computer. Unfortunately, we cannot
use GDB with the real hardware, only when we run it in QEMU (which is going to
still be very useful).
