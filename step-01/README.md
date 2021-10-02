Step 1: building and running a minimal image
============================================

We are now going to build our very first kernel image. It will not do anything
fancy, just run the simplest possible infinite loop.


Running a kernel image on the Raspberry Pi 3
--------------------------------------------

Before we can construct our kernel image (and run it with QEMU), we first need
to understand what the (virtual) hardware expects. This is actually simple: we
just need to construct a binary file `kernel8.img`. On the real hardware, this
file would simply be written to a specific place on a SD card (for details see
[here](https://wiki.osdev.org/Raspberry_Pi_Bare_Bones)), but here we will rely
on QEMU, so we will just need to give a file path on the command line.

When the (virtual) machine is started, the contents of `kernel8.img` is copied
to the memory region starting at address `0x80000`. The `PC` (program counter)
of the main core (but not the other three) is set to `0x80000`, and, hence, it
starts executing instructions "from the start of our image file".

Note that the other three cores are initially running in a loop, waiting to be
woken up. We will have a closer look at what they are doing in a later step.


Our kernel's code
-----------------

Our very simple kernel will be constructed from two files: a file `boot.S` and
a file `kernel8.ld`. The former will contain our assembly code, and the latter
will contain a linker script used to put everything together. At this point, a
single file would be enough (we could do everything in `boot.S`), but having a
separate linker script will be convenient for later stages.

### `boot.S`

The `boot.S` file we will start with contains the following assembly code.
```gas
// The entry point is in its own section ".text.boot".
.section ".text.boot"

// The symbol "_start" is the kernel's entry point, make it global.
.globl _start

// Entry point for the main core.
_start:
  b .
```
Once compiled, this code is inteded to be put at the start of our kernel image
file, which implies that label `_start` corresponds to address `0x80000` where
the main core fetches its first instruction after initialisation. In the above
code that first instruction is `b .`: an (unconditional) branch to the address
of the current instruction (denoted by `.`). In other words, this is a trivial
loop: the `b .` instruction is executed over and over.

**Note:** instruction `b <label>` causes an unconditional branch to `<label>`.
The operand of the actual instruction is a `PC`-relative offset, but using the
sugared notation above is almost always more convenient.

### `kernel8.ld`

Our initial `kernel8.ld` file contains the following linker script.
```ld
/* Entry point defined in file "boot.S". */
ENTRY(_start)
 
SECTIONS {
  /* Our kernel image will be placed at address 0x80000. */
  . = 0x80000;
  /* It starts with the ".text" segment. */
  .text : {
    /* The ".text" segment itself starts with the code from "boot.S". */
    /* The "_start" symbol (at the beginning of "boot.S") is at 0x80000. */
    *(.text.boot)
  }
}
```
The main thing that the linker script does is specify that the `.text` segment
starts at address `0x80000`, and that our `.text.boot` segment (containing the
code of file `boot.S`) is to be placed right at the start.


Compilation of the kernel
-------------------------

To compile our Kernel, we must follow the following three steps.
- compile `boot.S` using the assembler to produce an object file `boot.o`,
- run the linker on `boot.o`, with `kernel8.ld`, producing an ELF-format file,
- transform the ELF file into a binary image.

To do this, we need to add the following targets to our `Makefile` (and remove
the dummy target we added in the previous step).
```make
.PHONY: all
all: kernel8.img

boot.o: boot.S
	${BINROOT}as -c $< -o $@

kernel8.elf: kernel8.ld boot.o
	${BINROOT}ld -T $< -o $@ $(filter-out $<,$^)

kernel8.img: kernel8.elf
	${BINROOT}objcopy -O binary $< $@

.PHONY: clean
clean:
	@rm -f boot.o
	@rm -f kernel8.elf
	@rm -f kernel8.img
```
Running `make` should then produce the binary file `kernel8.img`.

**Note:** the `Makefile` from the repo uses a few tricks to make the output of
`make` less verbose by default. A variable `Q` (whose default value is `@`) is
used to disable the printing of commands, and `echo` displays a short message.
For example, the rule producing target `kernel8.img` becomes the following.
```make
kernel8.img: kernel8.elf
	@echo "[OBJCOPY] $@"
	${Q}${BINROOT}objcopy -O binary $< $@
```
With this, running `make kernel8.img` only prints `[OBJCOPY] kernel8.img`, but
running `make Q= kernel8.img` shows both that and the invoked command.


Checking the generated objects and binary
-----------------------------------------

Everything looks great so far: running `make` now builds our kernel image. But
how can we make sure that we did not mess anything up? To check that the files
we generated make sense, we can rely `objdump` (for the ELF-format files), and
`xxd` for binary files.

### Checking `boot.o`

Let us start by running
```sh
./compiler/prefix/bin/aarch64-elf-objdump -d boot.o
```
which produces the following output (irrelevant header removed).
```
Disassembly of section .text.boot:

0000000000000000 <_start>:
   0:	14000000 	b	0 <_start>
```
As expected, `bool.o` contains a section `.text.boot` containing our loop. The
address of `_start` is `0` at this point, but that is expected. This code will
be (hopefully) moved to start at address `0x80000` by the linker.

### Checking `kernel8.elf`

We now check the generated ELF file by running
```sh
./compiler/prefix/bin/aarch64-elf-objdump -d kernel8.elf
```
which produces the following output (irrelevant header removed).
```
Disassembly of section .text:

0000000000080000 <_start>:
   80000:	14000000 	b	80000 <_start>
```
As specified by the linker script, the `.text` section starts at `0x80000`. It
is headed by the code from the `.text.boot` section, as expected.

### Checking `kernel8.img`

We now expect the binary image `kernel8.img` to contain our single instruction
(whose opcode is `14000000` in hexadecimal). We can check that by running
```sh
xxd kernel8.img
```
which yields the following output.
```
00000000: 0000 0014                                ....
```
The file indeed contains our instruction (with little-endian byte order).


Running the kernel with QEMU
----------------------------

To run our kernel image with QEMU we need `qemu-system-aarch64`. To check that
it is installed on your system you can run the following command.
```sh
qemu-system-aarch64 --version
```
If you have version 6.1.0 or greater then everything looks good.

The command we will use to run QEMU is the following.
```sh
qemu-system-aarch64 -M raspi3 -nographic -kernel kernel8.img
```
The option `-M raspi3` specifies that we want our code to run on the Raspberry
Pi 3. The option `-nographic` disables graphical output since we won't use the
computer's screen (the plan is to communicate with the virtual machine using a
serial port, or with GDB). The last option `-kernel kernel8.img` specifies the
path to our kernel image.

**Note:** to stop QEMU, you need to press Ctrl-A, and then X. For convenience,
we add the following target to our `Makefile`.
```make
.PHONY: run
run: kernel8.img
	@echo "[QEMU]    running with $<"
	@echo "(Press Ctrl-A X to exit QEMU.)"
	${Q}qemu-system-aarch64 -M raspi3 -nographic -kernel $<
```
You can then run our kernel image using `make run`.

**Note:** since our kernel only runs an infinite loop, running `make run` will
not produce any output at all, and we won't be able to observe anything.
