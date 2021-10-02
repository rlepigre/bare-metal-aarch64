Step 2: inspecting the machine state with GDB
=============================================

We are now going to see how GDB can be used to inspect the QEMU machine state.
GDB will let us inspect registers and memory, decompile the code that is being
run by each of the four CPU cores, and step through instructions.


Interfacing QEMU with GDB
-------------------------

QEMU provides an interface to which GDB can connect. To enable it, we must run
QEMU with the `-s` option, which runs a GDB server (on TCP port 1234). We also
use `-S` option to tell QEMU not to start the CPU: we can do that from GDB. To
make things more convenient, we can use the following `Makefile` target.
```make
.PHONY: run-gdb
run-gdb: kernel8.img
	@echo "[QEMU]    running with $<"
	@echo "(Press Ctrl-A X to exit QEMU.)"
	${Q}qemu-system-aarch64 -s -S -M raspi3 -nographic -kernel $<
```
We will hence just need to run `make run-gdb` to launch QEMU, and have it wait
for GDB to connect.

Now, connecting to QEMU from GDB is simple: just run GDB with `kernel8.elf` as
argument, and run the command `target remote :1234`. We can again use a target
in our `Makefile` to do that for us.
```make
.PHONY: gdb
gdb: kernel8.elf
	@echo "[GDB]     running with $<"
	${Q}${BINROOT}gdb -ex "target remote :1234" $<
```
After having launched QEMU with `make run-gdb`, run `make gdb` in use an other
terminal to start a GDB session in which you are alredy connected to QEMU. You
should obtain something like the following.
```
Remote debugging using :1234
0x0000000000000000 in ?? ()
(gdb)
```
You are now ready to interact with the virtual machine through GDB.


Initialisation of the main core
-------------------------------

Something that is a bit surprising in the line that is printed right above the
GDB prompt, is that the program counter of the running CPU seems to have value
`0x0`, whereas we expected the entry point to be at `0x80000`. But perhaps are
we not running the main CPU? Let's find out.
```
(gdb) info threads
  Id   Target Id                    Frame
* 1    Thread 1.1 (CPU#0 [running]) 0x0000000000000000 in ?? ()
  2    Thread 1.2 (CPU#1 [running]) 0x0000000000000300 in ?? ()
  3    Thread 1.3 (CPU#2 [running]) 0x0000000000000300 in ?? ()
  4    Thread 1.4 (CPU#3 [running]) 0x0000000000000300 in ?? ()
```
The GDB command `info threads` shows the status of each running threads. Here,
we can see that there are four threads (numbered from 1 to 4), and that we are
currently focused on the first one (since it is marked with the `*`).

After all, it seems that the currently active thread (number 1) corresponds to
the main core, since the other three threads have their program counter set to
the same address `0x300`. To understand what is happening on our main core, we
can ask GDB to disassemble the contents of the memory starting from `0x0`.
```
(gdb) disas/r 0x0, +(6 * 4)
Dump of assembler code from 0x0 to 0x18:
=> 0x0000000000000000:	c0 00 00 58	ldr	x0, 0x18
   0x0000000000000004:	e1 03 1f aa	mov	x1, xzr
   0x0000000000000008:	e2 03 1f aa	mov	x2, xzr
   0x000000000000000c:	e3 03 1f aa	mov	x3, xzr
   0x0000000000000010:	84 00 00 58	ldr	x4, 0x20
   0x0000000000000014:	80 00 1f d6	br	x4
End of assembler dump.
```
Note that we asked to disassemble 6 instruction (each taking 4 bytes) starting
from address `0x0`: no need to go further as the last one will unconditionally
branch to some other code (as we will see).

So, what does this code do? First, the instruction `ldr x0, 0x18` loads a word
(4 bytes) from memory at address `0x18`, and stores it in register `x0`. Then,
the next three instructions set the registers `x1`, `x2` and `x3` to zero, via
copies from the `xzr` (zero) register. Then, the instruction `ldr x4, 0x20` is
used to load a word from `0x20`. And finally, `br x4` unconditionally branches
to the address stored in `x4`, which was just loaded from address `0x20`. What
is that mystery address then?
```
(gdb) x/w 0x20
0x20:	0x00080000
```
As we can see, it corresponds to our actual entry point `0x80000`. So it seems
that some initialisation (firmware) code is run prior to branching to the code
of our kernel.

**Note:** we use the GDB command `x/w 0x20` to load a word (hence the `/w`) of
memory from address `0x20`.


Linux main core register settings and DTB address
-------------------------------------------------

In fact, the above initialisation of `x0`, `x1`, `x2`, and `x3` corresponds to
the expected primary CPU general-purpose register settings when booting Linux.
[This document](https://www.kernel.org/doc/html/v5.7/arm64/booting.html) gives
more details, but essentially, `x1`, `x2` and `x3` should have value `0` (they
are reserved for future use), and `x0` should give the physical address of the
DTB (Device Tree Blob), which describes hardware. So, in our case, the address
of the DTB should initially be stored at `0x18`. Let's check.
```
(gdb) x/w 0x18
0x18:	0x00000100
```
After inspecting the memory at address `0x18`, we can see that the loaded word
is `0x100`, which is where the DTB should be in memory. However, that does not
seem to be the case: the DTB must start with the magic number `0xd00dfeed` (in
big-endian order) and we have a different value at address `0x100`.
```
(gdb) x/4b 0x100
0x100:	5	0	0	0
```

**Note:** we use the GDB command `x/4b 0x100` to print 4 memory bytes starting
at address `0x100`. We cannot use `x/w 0x100` here since it would construct an
integer value assuming little-endian encoding, but the magic number is encoded
in big-endian.

The fact that we have no valid DTB pointer is not really a problem for now, as
we do not plan to interact much with the (virtual) hardware for now. Note that
it is possible to provide a DTB as a parameter to QEMU: by running with option
`-dtb bcm2710-rpi-3-b.dtb`, the value stored at `0x18` changes from `0x100` to
`0x8000000`, and the file's contents is made accessible at that address. There
are DTB files available [here](https://github.com/raspberrypi/firmware/).
```
(gdb) # The following assumes QEMU is run with "-dtb bcm2710-rpi-3-b.dtb".
(gdb) x 0x18
0x18:	0x08000000
(gdb) x/4b 0x08000000
0x8000000:	0xd0	0x0d	0xfe	0xed
(gdb)
```


What are the other cores doing?
-------------------------------

Now that we know what the main core is doing on startup, let us have a look at
the code being executed by the other cores.
```
(gdb) disas/r 0x300,+0x30
Dump of assembler code from 0x300 to 0x330:
   0x0000000000000300:	05 1b 80 d2	mov	x5, #0xd8                  	// #216
   0x0000000000000304:	a6 00 38 d5	mrs	x6, mpidr_el1
   0x0000000000000308:	c6 04 40 92	and	x6, x6, #0x3
   0x000000000000030c:	5f 20 03 d5	wfe
   0x0000000000000310:	a4 78 66 f8	ldr	x4, [x5, x6, lsl #3]
   0x0000000000000314:	c4 ff ff b4	cbz	x4, 0x30c
   0x0000000000000318:	00 00 80 d2	mov	x0, #0x0                   	// #0
   0x000000000000031c:	01 00 80 d2	mov	x1, #0x0                   	// #0
   0x0000000000000320:	02 00 80 d2	mov	x2, #0x0                   	// #0
   0x0000000000000324:	03 00 80 d2	mov	x3, #0x0                   	// #0
   0x0000000000000328:	80 00 1f d6	br	x4
End of assembler dump.
```
The first three instructions do some setting up: register `x5` is assigned the
constant `0xd8`, and register `x6` is set to a two bits value corresponding to
the two least significant bits of the `mpidr_el1` system register. Note that a
`mrs` instruction is used to copy the system register in `x6`, and an `and` is
then used to mask all bits but the least significant two. Here, the bits taken
from the `mpidr_el1` system register are used as core identifier: after having
executed the `and` instruction, register `x6` gets value `1` on core 2, `2` on
core 3, and `3` on core 4.

After the setting up is done, a loop is starts at the `wfe` instruction (found
at address `0x30c`). The `wfe` (wait for event) instruction lets the core know
that it can enter a low-power mode (until some event occurs). Then, the second
instruction in the loop, `ldr x4, [x5, x6, lsl #3]`, loads a value from memory
to register `x4`. The second operand, `[x5, x6, lsl #3]`, specifies an address
to load from using a base, and offset, and a shift for the offset. Simply put,
the loading is `x5 + (x6 << 3)`. Note that the loading address depends on `x6`
(and hence the core identifier): core 2 loads from address `0xe0`, core 3 from
`0xe8`, and core 4 from `0xf0`. Finally, instruction `cbz x4, 0x30c` completes
the loop by branching back to the `wfe` instruction if the word loaded to `x4`
equals zero.

In the case where `x4` is non-zero, then it is assumed to represent an address
and the loop is exited. Then, registers `x0`, `x1`, `x2`, and `x3` are zeroed,
and a branch instruction is used to jump to the address stored in `x4`.

### So what does this code do?

When the machine is started, the words stored at the special addresses `0xe0`,
`0xe8` and `0xf0` are all zero. As a consequence, all three cores are stuck in
a loop, which means that they are basically asleep. However, they can be woken
up by writing an address to jump to at these addresses (and by using the `sev`
instruction to generate an event and force the cores to exit low-power mode if
they entered it.)


Stepping through instruction
----------------------------

While in GDB you can step through instruction by running command `stepi` (`si`
in its abbreviated form). You can also inspect the value of registers by using
the `info register x0`, replacing `x0` with any register you'd like to inspect
(including system registers). To show the state of all registers, you can rely
on command `info registers` (it only shows registers of the current thread).

To switch focus to another thread, say, thread 2, use command `thread 2`. Note
that you can use the command `info threads` to check which thread is in focus.
