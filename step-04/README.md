Step 4: setting up a C environment
==================================

Our plan is now to set up an UART so that we can print debugging messages from
our kernel (running on the real Raspberry Pi 3). Although it would be possible
to do that in assembly, this would be quite painful. As a consequence, we will
rather implement our debugging console in C.


Minimal C entry point
---------------------

We will start with the following, minimal C code, in a new file `kernel.c`.
```c
// Entry point for the kernel, allocated in the ".text" segment.
// This function never returns.
void kernel_entry(){
  // Go in an infinite loop of "wfe" instructions.
  while(1){
    asm("wfe");
  }
}
```
It defines the function `kernel_entry` that will be called from `boot.S`. This
function will be the C code entry point, and it will never return. However, we
will still be able to use inline assembly to test specific instructions. Here,
we see an example with the `wfe` instruction.

To compile our new `kernel.c` file, we extend the `Makefile` as follows.
```make
# Flags passed to GCC.
GCC_FLAGS = -ffreestanding -Wall -Wextra -Werror -pedantic -O0

kernel.o: kernel.c
	${BINROOT}gcc ${GCC_FLAGS} -c $< -o $@
```
The `-ffreestanding` flag tells the compiler that there is no standard library
in our environment, and that the program will not necessarily start at `main`.
We also turn on more aggressive warnings, and disable optimisations with `-O0`
to make the generated assembly easier to relate to the C source code.

After having compiled `kernel.c` using `make kernel.o`, we can now inspect the
generated assembly using the command
```
./compiler/prefix/bin/aarch64-elf-objdump -d kernel.o
```
which produces the following output (irrelevant header removed).
```
Disassembly of section .text:

0000000000000000 <kernel_entry>:
   0:	d503205f 	wfe
   4:	17ffffff 	b	0 <kernel_entry>
```
Unsurprisingly, the code looks very similar to what we wrote in `boot.S`. Note
that GCC places code in the `.text` section, which is not yet accounted for in
our linker script `kernel8.ld`. We thus extend
```
  .text : {
    /* The ".text" segment itself starts with the code from "boot.S". */
    /* The "_start" symbol (at the beginning of "boot.S") is at 0x80000. */
    *(.text.boot)
  }
```
into the following.
```
  .text : {
    /* The ".text" segment itself starts with the code from "boot.S". */
    /* The "_start" symbol (at the beginning of "boot.S") is at 0x80000. */
    *(.text.boot)
    /* It then continues with the rest of the ".text" segment. */
    *(.text)
  }
```

We are now two small steps away to making our first version of the C code run.
First, we need to add `kernel.o` as a dependency to build `kernel8.elf` in our
`Makefile`. For this, we need to change the line
```
kernel8.elf: kernel8.ld boot.o
```
into the following.
```
kernel8.elf: kernel8.ld boot.o kernel.o
```
Note that the command do not need to be changed, since it already used all the
correct automatic variables.

Finally, we last need to modify `boot.S` so that it calls our C function. This
can be done using the following instruction, which we will insert right before
the `hang_forever` label.
```gas
  // Call the "kernel_entry" C function.
  // (This call should never return.)
  bl kernel_entry
```
The `bl` (branch with link) instruction is used to perform a function call. It
uses register `x30` to store the return address (for us, it corresponds to the
`hang_forever` label), at which computation will resume when the function uses
the `ret` (return from subroutine) instruction. In our case, `kernel_entry` is
not going to return, but we will still keep the `hang_forever` loop in case we
mess things up and it ends up returning.


Setting up the stack
--------------------

The above minimal example works as we expect (which we can check in QEMU), but
this does not mean that we did a good job setting up the environment for C. In
fact, we did not even initialise the `sp` (stack pointer) register, so we were
just lucky that our C code did not need to use the stack, otherwise things may
have gone pretty wrong.

To see the problem, let us change our `kernel_entry` function so that it takes
as arguments the contents of registers `x0` (containing a pointer to the DTB),
`x1`, `x2`, and `x3`. In other words, we will transmit the contents of all the
registers that were initialised by the firmware. We hence modify `kernel.c` to
contain the following.
```c
#include <stdint.h>

#define UNUSED(x) (void)(x)

// Short name for the type of unsigned, 64-bits integers.
typedef uint64_t u64;

// Entry point for the kernel, allocated in the ".text" segment.
// This function never returns.
void kernel_entry(void *dtb_p, u64 x1, u64 x2, u64 x3){
  // Silence GDB warnings about unused parameters.
  UNUSED(dtb_p); UNUSED(x1); UNUSED(x2); UNUSED(x3);

  // Go in an infinite loop of "wfe" instructions.
  while(1){
    asm("wfe");
  }
}
```
For now, we are not doing anything with the function's parameters (and we must
rely on the `UNUSED` macro to silence GCC warnings), but they still need to be
accounted for in the generated assembly. Here is the output of `objdump`, with
some irrelevant header removed.
```
Disassembly of section .text:

0000000000000000 <kernel_entry>:
   0:	d10083ff 	sub	sp, sp, #0x20
   4:	f9000fe0 	str	x0, [sp, #24]
   8:	f9000be1 	str	x1, [sp, #16]
   c:	f90007e2 	str	x2, [sp, #8]
  10:	f90003e3 	str	x3, [sp]
  14:	d503205f 	wfe
  18:	17ffffff 	b	14 <kernel_entry+0x14>
```
As we can see, the function starts by pushing its arguments to the stack, with
the first five instructions. The first instruction subtracts 32 from the stack
pointer `sp`, to reserve space for the arguments (which use 8 bytes each). The
values of registers `r0` to `r3` (containing the arguments) are the written to
the reserved slots using the `str` instruction.

**Note:** since `kernel_entry` does not return, the stack pointer is never put
back to its previous value (this would be done using `add sp, sp, #0x20`).

Now, since it is likely (at least in QEMU) that the initial value of `sp` is 0
on startup, things will likely go wrong if we run our code. And they indeed do
go wrong: the stack pointer ends up underflowing, and an exception is produced
by the subsequent `str` instruction. However, no exception vector has yet been
set up, so we just end up jumping at address `0x200`, which is where a handler
would be if the exception vector was placed at `0x0` (which it definitely must
not since there is firmware code at that address).

So, let us set up the stack before the `bl` instruction then.
```gas
  // Set the SPSel register so that SP_EL0 is the stack pointer at all EL.
  mrs x6, SPSel         // Move the current SPSel  system register into x6.
  and x6, x6, ~1        // Clear the 0 bit of x6.
  msr SPSel, x6         // Set the value of SPSel to x6.

  // Set up the stack below our code (it grows downwards).
  // This should be plenty big enough: only the first 4KB of memory are used.
  ldr x6, =_start
  mov sp, x6
```
The first thing we do, which is not absolutely necessary, is ensure that every
exception level relies on the same stack (that of `EL0`). To this aim, we need
to clear the last bit of the `SPSel` system register, which requires using the
`mrs` and `msr` instructions.

We then set the stack to be below our `_start` symbol (remember that the stack
grows downwards): its top will hence be at address `0x80000`, and we will have
no problem as long as it does not grow too big (i.e., bigger than ~508KB). The
`ldr x6, =_start` instruction can be a bit confusing: it loads a word, but not
from the `_start` address (which would be weird). Instead, the load is done at
a fresh address, denoted by `=_start` (note the `=`), which store the value of
the `_start` label.

With this fix, we can now run our code in QEMU, and things goes as expected.


Read-only, data and BDD segments
--------------------------------

Although our C code can now use the stack, and thus rely on function calls, we
still still need to do some work to support global variables correctly. Global
variables are allocated in specific segments of memory called `.rodata` (which
stands for read-only data), `.data`, and `.bss` (block starting symbol). Every
global variable that is initialised is either allocated in `.rodata`, if it is
marked as `const`, or in `.data` otherwise. Uninitialised global variables are
allocated in the `.bss` section. To observe this, let us change our `kernel.c`
file to the following.
```c
#include <stdint.h>

#define UNUSED(x) (void)(x)

// Short name for the type of unsigned, 64-bits integers.
typedef uint64_t u64;

const int x = 42; // Allocated in the ".rodata" segment.
int y = 73;       // Allocated in the ".data" segment.
int z;            // Allocated in the ".bss" segment.

// Entry point for the kernel, allocated in the ".text" segment.
// This function never returns.
void kernel_entry(void *dtb_p, u64 x1, u64 x2, u64 x3){
  // Silence GDB warnings about unused parameters.
  UNUSED(dtb_p); UNUSED(x1); UNUSED(x2); UNUSED(x3);

  // Just a bit of arithmetic for testing.
  y = y + z;
  z = x * 2;

  // Go in an infinite loop of "wfe" instructions.
  while(1){
    asm("wfe");
  }
}
```
After compiling this code, we can use the command
```
./compiler/prefix/bin/aarch64-elf-objdump -D kernel.o
```
(note the `-D` instead of `-d`) to disassemble all sections of `kernel.o`. The
output is the following (irrelevant header and ".comment" section removed: the
latter only encode the version information string "GCC: (GNU) 11.2").
```
Disassembly of section .text:

0000000000000000 <kernel_entry>:
   0:	d10083ff 	sub	sp, sp, #0x20
   4:	f9000fe0 	str	x0, [sp, #24]
   8:	f9000be1 	str	x1, [sp, #16]
   c:	f90007e2 	str	x2, [sp, #8]
  10:	f90003e3 	str	x3, [sp]
  14:	90000000 	adrp	x0, 0 <kernel_entry>
  18:	91000000 	add	x0, x0, #0x0
  1c:	b9400001 	ldr	w1, [x0]
  20:	90000000 	adrp	x0, 0 <kernel_entry>
  24:	91000000 	add	x0, x0, #0x0
  28:	b9400000 	ldr	w0, [x0]
  2c:	0b000021 	add	w1, w1, w0
  30:	90000000 	adrp	x0, 0 <kernel_entry>
  34:	91000000 	add	x0, x0, #0x0
  38:	b9000001 	str	w1, [x0]
  3c:	52800540 	mov	w0, #0x2a                  	// #42
  40:	531f7801 	lsl	w1, w0, #1
  44:	90000000 	adrp	x0, 0 <kernel_entry>
  48:	91000000 	add	x0, x0, #0x0
  4c:	b9000001 	str	w1, [x0]
  50:	d503205f 	wfe
  54:	17ffffff 	b	50 <kernel_entry+0x50>

Disassembly of section .data:

0000000000000000 <y>:
   0:	00000049 	udf	#73

Disassembly of section .bss:

0000000000000000 <z>:
   0:	00000000 	udf	#0

Disassembly of section .rodata:

0000000000000000 <x>:
   0:	0000002a 	udf	#42
```

As we can see in the above, variables in the `.bss` segment are initialised to
0, as mandated by the C standard. But, unlike `.rodata` and `.data`, `.bss` is
not included in the generated binary file `kernel8.img`. The reason is that it
comes after the other sections (by default), and its value is known to be zero
everywhere, so this saves space. However, this means that we must zero-out the
corresponding memory ourself before entering C code!

### Controlling the placement of sections

As it is now, our kernel will likely work as expected, at least in QEMU, as the
uninitialised bits of memory generally are zeroed. However, this is definitely
not reliable, so we do need to zero-out the `.bss` section. But how? The first
step is to modify our linker script (`kernel8.ld`) to decide where exactly the
section (including `.bss`) are placed, and define symbols giving the start and
end address of each of the sections. (So far we only need this for `.bss`, but
we will define such symbols to each sections for uniformity.) Additionally, we
will also pad each section to the next page boundary (assuming 4KB page size),
which is not strictly necessary, but might come in handy when we start playing
with page tables. Nonetheless, we need the start of the `.bss` section to have
a word-aligned address (and page-aligned implies word-aligned), since we shall
make use of that assumption when we write the initialisation (zeroing) code.

Our `kernel8.ld` file now contains the following.
```
/* Entry point defined in file "boot.S". */
ENTRY(_start)
 
SECTIONS {
  /* Our kernel image will be placed at address 0x80000. */
  . = 0x80000;
  /* It starts with the ".text" segment. */
  __start = .;
  __text_start = .;
  .text : {
    /* The ".text" segment itself starts with the code from "boot.S". */
    /* The "_start" symbol (at the beginning of "boot.S") is at 0x80000. */
    *(.text.boot)
    /* It then continues with the rest of the ".text" segment. */
    *(.text)
  }
  . = ALIGN(4096); /* Add padding to the next page boundary. */
  __text_end = .;

  /* Read-only data segment (for initialised const C global variables). */
  __rodata_start = .;
  .rodata : {
    *(.rodata)
  }
  . = ALIGN(4096); /* Add padding to the next page boundary. */
  __rodata_end = .;
 
  /* Data segment (for initialised, non-const C global variables). */
  __data_start = .;
  .data : {
    *(.data)
  }
  . = ALIGN(4096); /* Add padding to the next page boundary. */
  __data_end = .;
 
  /* BSS segment (for uninitialised C global variables). */
  /* BSS stands for "block starting symbol". */
  /* The BSS segment must be zeroed prior to entering C code. */
  __bss_start = .;
  .bss : {
    *(.bss)
  }
  . = ALIGN(4096); /* Add padding to the next page boundary. */
  __bss_end = .;
  __end = .;
}
```
Note that we can now use the symbols `__bss_start` and `__bss_end` to refer to
the start address of the `.bss` section and the address one past its end.

### Clearing the BSS segment

The only thing left to do is to actually write the assembly code to initialise
the BSS segment to zero. We will do this as follows.
```gas
  // Clear the BSS segment (assumes __bss_start and __bss_end word-aligned).
  ldr x6, =__bss_start  // Current word of the BSS (initially at start).
  ldr x7, =__bss_end    // Address one past the BSS segment.
bss_clear_loop:
  cmp x6, x7            // If we have reached the end ...
  b.ge bss_clear_done   // ... exit the loop.
  str xzr, [x6]         // Otherwise, zero-out the word at address x6,
  add x6, x6, #8        // increment x6 by a word (8 bytes),
  b bss_clear_loop      // and continue to loop.
bss_clear_done:
```
This is a straight-forward loop: we first initialise `x6` to `__bss_start` and
`x7` to `__bss_end`, and while `x6 <= x7` we then write a zero at address `x6`
and increment `x6` by the size of a word.

**Note:** since we zero-out memory one word (i.e., 8 bytes) at a time, we need
the `__bss_start` and `__bss_end` address to be word-aligned (which they are).
Thanks to this assumption, all accesses are aligned, and the size of `.bss` is
a multiple of the word size so we do not have to deal with padding bytes.


More GDB tricks
---------------

Since our code now loop through the whole `.bss` segment (which require in the
order of a few hundred interations for now), using the `si` (step instruction)
command in GDB. An alternative approach is to set a breakpoint. After starting
GDB, you can for example do the following `bss_clear_done`.
```
(gdb) b bss_clear_done
Breakpoint 1 at 0x80038
(gdb) c
Continuing.

Thread 1 hit Breakpoint 1, 0x0000000000080038 in bss_clear_done ()
```
This first creates a breakpoint at label `bss_clear_done`, which is at the end
ouf our `.bss` initialisation loop, and we can then run up to that label using
the `c` (or `continue`) command.

**Note:** to set a breakpoint at an address that does not have a corresponding
lable, say `0x80004`, you can use `b *0x80004`.

**Note:** the command `layout asm` can be used in GDB to enable a pannel where
the next few instructions to execute are shown, which can be convenient.
