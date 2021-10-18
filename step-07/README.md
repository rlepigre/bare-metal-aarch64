Step 7: running our kernel at EL1
=================================

As we discovered in a previous step, the default behaviour of the Raspberry Pi
3 (observed in QEMU) is to boot at exception level 2 (i.e., EL2). Hence, our C
code runs at EL2, and we will now make it so that our C code runs at EL1.


Exception levels
----------------

What is the difference between these exception levels anyway? The standard use
of the different exception levels is the following:
- EL2 is the exception level at which an hypervisor might run,
- EL1 is the exception level where operating systems run, and
- EL0 is the exception level at which user applications run.

Having different exception levels allows one to control what operations can be
performed at each level. For example, an hypervisor (running at EL2) will have
more privileges than an OS (running at EL1). Consequently, an OS typically has
to request services from the hypervisor (i.e., make hypervisor calls) whenever
it needs to perform certain privileged tasks.


Making the switch to EL1
------------------------

To understand how we can switch from EL2 to EL1, we first need to understand a
few things about exceptions and how they are handled. A change in EL typically
happens when code running at a lower EL needs a service. For code that runs at
EL1 (i.e., OS code), this is done via the `hvc` instruction (hypervisor call):
when executed, it triggers an exception that is handled at EL2.

An hypervisor call is generally similar to a function call: when the exception
has been handled, the execution returns to the caller (i.e., execution resumes
at the instruction following the `hvc`). The return address for the hypervisor
call is written to register `ELR_EL2` (exception link register for EL2) at the
time of the `hvc`, and it is jumped to whenever the exception handler uses the
`eret` (exception return) instruction.

Now, how do we force our kernel's code to switch to EL1? We are at EL2, so the
situation is kind of the same as if we were executing an exception handler. So
what if we just use `eret`? Doing so will indeed let us move to EL1, but where
are we gonna jump? There is a good change that `ELR_EL2` contains garbage, and
we do not want to jump to a random place. Luckily, we can just write any value
we want to `ELR_EL2`, so we can control where execution will continue. We also
need to set the `SPSR_EL2` (saved program status register for EL2) register to
configure the process state for EL1.

Putting everything together, this leads to the following piece of code, placed
right before the call to the `kernel_entry` C function.
```gas
  // Move to EL1.
  mov x6, (1 << 31)     // Hypervisor configuration: aarch64 mode for EL1.
  msr hcr_el2, x6       // (Written to the HCR_EL2 system register.)
  mov x6, 0x3c4         // Configure EL1 state (use SP_EL0, no interupts).
  msr spsr_el2, x6      // (Written to the SPSR_EL2 system register.)
  ldr x6, =enter_el1    // Specify the "return from exception" address.
  msr elr_el2, x6       // (Written to the ELR_EL2 system register.)
  eret                  // Simulate an "exception return" to move to EL1.
enter_el1:
```
The first two instructions tell the processor that the execution state for EL1
is aarch64 (and not aarch32). This is configured by setting bit 31 of register
`HCR_EL2` (hypervisor configuration register), which is a system register used
to configure virtualisation stuff (e.g., trapping given register accesses). It
should be fine to set other bits to 0 for now, we can revisit that later if we
need to.

The next two instructions write to the `SPSR_EL2` system register to configure
the EL1 state. Value `0x3c4` gives the following configuration.
```
0    .... 0    3    c    4
0000 .... 0000 0011 1100 0100
                 || ||   ---- → EL1t stack configuration (use SP_EL0)
                 || |-        → FIQ interrupt mask
                 || -         → IRQ interrupt mask
                 |-           → SError interrupt mask
                 -            → Debug exception mask
```
In summary: EL1 is going to use the EL0 stack (`SP_EL0` register) which we are
currently using at EL2, and we mask a bunch of interrupts since we will not be
using them.

Finally, the last three instructions set the return address in `ELR_EL2` to be
a new label `enter_el1`: the EL1 entry point, placed right after our code. The
switch to EL1 is then triggered by using the `eret` instruction.


Is there a problem?
-------------------

Now, running our kernel with `make run` should still work as before, and there
should be no difference in the UART output on startup. However, there seems to
be a problem: the execution gets stuck after having shown "Hello, World!!".
```
$> make run
[QEMU]    running with kernel8.img
(Press Ctrl-A X to exit QEMU.)
********************************************
*              Hello, World!!              *
********************************************
```
And nothing seems to happen from there: we can only terminate the execution of
QEMU. That is unexpected.

So what is the problem? What is surprising is that we have not changed much of
the code since we last ran `make run`: we only added some printing code. There
seems to be no problem around the call to `kernel_entry` either. Given what we
see on the screen, the problem seems to come from function `uart1_printf`. Let
us use GDB to see what is happening.
```
(gdb) b kernel_entry 
Breakpoint 1 at 0x8049c
(gdb) c
Continuing.

Thread 1 hit Breakpoint 1, 0x000000000008049c in kernel_entry ()
```
After having set a breakpoint at the start of the `kernel_entry` function, the
command `c` is used to continue execution up to that point.

We can then use command `layout asm` to have GDB show us the next instructions
that are going to be executed. Using this, we can figure out what where to set
our next breakpoint: at the first call to `uart1_printf`.
```
(gdb) b *0x804c4
Breakpoint 2 at 0x804c4
(gdb) c
Continuing.

Thread 1 hit Breakpoint 2, 0x00000000000804c4 in kernel_entry ()
```
We can then use the `si` command to step through instruction until we find the
problem, which occurs when we try to execute instruction `str q0, [sp, #128]`,
which is meant to store the contents of register `q0` to the stack. If soon as
we try to evaluate the instruction, execution jumps to address `0x0`.

There are two questions that come to mind here:
- Why are we jumption to `0x0`?
- What is this `q0` register we have never seen before?

The answer to the first question is rather simple: what happened is that there
was a trap on our instruction (probably due to a restricted access to register
`q0`), and the corresponding exception is not being handled at EL2, but rather
at an exception level for which we have not created an exception vector table.
In fact, we can check the `ELR_ELx` system registers to see which one has been
set to the address of the trapped instruction! And the winner is `ELR_EL1`.

So, running `str q0, [sp, #128]` triggers an exception that is handled at EL1.
But why does this happen? And how do we fix this? As it turns out `q0` and its
sibling are "advanced SIMD registers", whose access is restricted at EL1. This
is not the case at EL2, which is why we did not have this problem until now. A
simple solution to circumvent the problem is to ask GCC to never use registers
like `q0`: this is done via option `-mgeneral-regs-only`. We can thus add this
option to our `Makefile`, and the problem goes away.

**Note:** you may need to run `make clean` to force all targets being rebuilt:
file `Makefile` does not appear in the dependencies of our targets.

**Note:** another solution would be to figure out what particular flag must be
set in the system registers so that EL1 can use `q0` and its siblings.


Checking that we are at EL1
---------------------------

Having fixed the problem, let us change our code a little bit to check that we
are indeed running at EL1. To do this we will simply read `CurrentEL`, as done
already at the start of our code. Hopefully, we will get a different value!

To keep things simple, we will read the exception level into register `x6`.
```gas
  // Put the current exception level in x6 (as we did for x5 above).
  mrs x6, CurrentEL
  ubfx x6, x6, #2, #2
```
In other words, we will feed the value we just computed as seventh argument to
the `kernel_entry` function. Indeed, recall that the calling convention states
that the first 8 arguments of a function are read from registers `x0` to `x7`.
We can then change our `kernel_entry` function as follows.
```c
void kernel_entry(void *dtb, u64 x1, u64 x2, u64 x3, u64 x4, u64 x5, u64 x6){
  ... // Same as before.
  uart1_printf("Initial exception level: EL%i.\n", (int) x5);
  uart1_printf("Current exception level: EL%i.\n", (int) x6);
  ... // Same as before.
}
```

Running the kernel now produces the expected output.
```
********************************************
*              Hello, World!!              *
********************************************
Initial value of x1:     0x0000000000000000.
Initial value of x2:     0x0000000000000000.
Initial value of x3:     0x0000000000000000.
Initial entry point:     0x0000000000080000.
Initial exception level: EL2.
Current exception level: EL1.
Address of the DTB:      n/a
Entering the interactive mode.
>
```
