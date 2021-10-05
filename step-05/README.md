Step 5: input-outputs (in C) via an UART
========================================

Now that we are dune setting up our C environment, we can finally focus on the
handling of input-outputs via an UART. As a consequence, we need to learn more
about the peripherals accessible to the Raspberry Pi 3. This computer is build
around the BCM2837 SoC, which provides, among other things, two UARTs. We will
here target UART1, which is part of the "auxiliary peripherals" (together with
two SPI masters), since it is the easiest to set up.

**Note:** [this file](include/bcm2837/README.md) gives more information on the
BCM2837 SoC together with links to relevant documentation. Surprisingly, there
does not seem to be any available document giving the peripheral specification
for the BCM2837, so we have to rely on the documentation of the BCM2835 (which
is said to not be significantly different).


Bus address / physical address
------------------------------

The BCM2837 documentation refers to peripheral registers in terms of their bus
addresses, which do not directly correspond to physical address. Thus, we rely
on the following function (found in `include/bcm2837/register.h` together with
more explanations) to translate a bus address into a physical address.
```c
// Convert a bus address into a physical address.
static inline uintptr_t bus_to_phys(u64 addr){
  return (uintptr_t) (addr - 0x3f000000ULL);
}

// Convert a bus address into a 32-bit register pointer.
#define bus_to_reg32(addr) ((volatile u32 *) bus_to_phys(addr))
```
We also provide a macro `bus_to_reg32(addr)` which, given a bus address, gives
a pointer to a register (with type `u32`). Note that the register type is made
`volatile`, because its value can (in general) be modified by the hardware.

**Note:** as you may have noticed, an `include` directory was created to store
various header files, including an `include/bcm2837` sub-directory with header
files related to peripherals. Some definition previously in `kernel.c` are now
found in header files like `include/types.h` or `include/macros.h`.


Definitions for auxiliary peripherals and the GPIO
--------------------------------------------------

The BCM2837 documentation specifies quite a lot of registers and configuration
values, which we carefully turn into macro definitions in header files. As our
goal is to get UART1 working, there are two sets of registers of interest:
- the auxiliary peripheral registers (see `include/bcm2837/auxiliaries.h`),
- and the GPIO registers (see `include/bcm2837/gpio.h`).

As an example, the former file defines the register `AUX_ENABLES`.
```c
#define AUX_ENABLES bus_to_reg32(0x7e215004ULL)

// Bit fields of the AUX_ENABLES register.
#define AUX_ENABLES_BIT_UART1 BIT_U32(0)
#define AUX_ENABLES_BIT_SPI1  BIT_U32(1)
#define AUX_ENABLES_BIT_SPI2  BIT_U32(2)
```
According to the specification, this registers can be accessed for reading and
writing, and it controls whether the auxiliary peripherals modules are enabled
via its three least significant bits.

**Note:** we use the actual register names (from the documentation) for naming
the corresponding macros.


UART1 interface and client code
-------------------------------

The interface to UART1 is defined in `include/bcm2837/uart1.h`.
```c
#pragma once
#include <stddef.h>
#include <types.h>

// Initialise UART1: must be called before using other "uart1_*" function.
// Note: after initialisation, GPIO ports 14 and 15 reserved for UART1.
void uart1_init();

// Important note: the following operations are blocking if the UART1 internal
// (input or output) buffer is full.

// Write character c to UART1.
void uart1_putc(char c);

// Write the null-terminated string s to UART1.
// Note: the character "\n" is written as the sequence "\r\n".
void uart1_puts(char *s);

// Read a character from UART1.
// Note: the character "\r" is converted into "\n".
char uart1_getc();

// Read characters from UART1 into the string buffer buf, whose capacity is n.
// We stop reading if either:
// - n-1 characters have been read (we reserve space for the null terminator),
// - a '\r' character is found (it is written to the buffer as well).
// Note: the character "\r" is converted into "\n".
size_t uart1_getline(char *buf, size_t n);
```
It provides prototypes for five functions together with their documentation in
comments. Overall, this interface is pretty straight-forward.

To test this interface, we will change our `kernel.c` file as follows.
```c
#include <macros.h>
#include <types.h>
#include <bcm2837/uart1.h>

// Entry point for the kernel, allocated in the ".text" segment.
// This function never returns.
void kernel_entry(void *dtb_p, u64 x1, u64 x2, u64 x3){
  // Silence GDB warnings about unused parameters.
  UNUSED(dtb_p); UNUSED(x1); UNUSED(x2); UNUSED(x3);

  // Initialise the UART, and print a first message.
  uart1_init();
  uart1_puts("Hello, world!\n");

  // Go in an infinite loop.
  while(1){
    // Buffer for the input.
    char buffer[20];

    // Ask the user for input, and read a "line" of input on the UART.
    uart1_puts("Send me something!\n");
    uart1_getline(buffer, 20);

    // Print the line back to the UART.
    uart1_puts("I received: \"");
    uart1_puts(buffer);
    uart1_puts("\".\n");
  }
}
```
What this code does is fairly simple: it initialises the UART, uses it to show
a traditional "Hello, world!" message, and then enters an infinite loop during
which the user can interact with the kernel using a minimal console interface,
in which any text entered by the user is simply printed back (line by line).


UART1 implementation
--------------------

The hard part is actually writing the implementation of the UART functions (in
particular, that of the initialisation function). It is hard to get right, and
requires reading the documentation carefully. We will not explain details here
since file `uart1.c` contains many explanations in comments.

As an example, let us look at the implementation of `uart1_putc`.
```c
void uart1_putc(char c){
  // Wait until we can send.
  while(!(*AUX_MU_LSR_REG & AUX_MU_LSR_TX_EMPTY)) {
    asm volatile("nop");
  }

  // Write to the buffer.
  *AUX_MU_IO_REG = (u32) c;
}
```
What this function mainly does is writes the character `c` it gets as argument
to the `AUX_MU_IO_REG` register. This register can be written to to add a byte
to the UART's TX buffer (and also to read a byte from its RX buffer). However,
we can only write a byte to `AUX_MU_IO_REG` if the TX buffer is not full. This
is why the function starts by waiting for a "transmitter empty" bit to be set.

**Note:** all our input-output routines are blocking.


Makefile modifications
----------------------

The first thing we need to do is add `include` to the list of directories that
are searched for header files by GCC (via the `-I` option).
```make
# Flags passed to GCC.
GCC_FLAGS = -ffreestanding -Wall -Wextra -Werror -pedantic -O0 -I ./include
```

We then need to give a rule to build file `uart1.o` from `uart1.c`. Instead of
adding a specific rule, we instead generalise the rule we used for `kernel.o`.
```make
# All header files.
C_HDR = $(wildcard include/*.h) $(wildcard include/bcm2837/*.h)

%.o: %.c ${C_HDR}
	@echo "[gcc]     $@"
	${Q}${BINROOT}gcc ${GCC_FLAGS} -c $< -o $@
```
Note that we need to give all header files as dependencies to enforce that the
compilation units are recompiled if header files are modified. Of course, some
object files might not depend on all header files, but this over-approximation
is good enough (doing better would be both pretty useless and a pain).

Finally, we also need to make sure that all object files produced from C files
are added to the dependency list in the rule for building `kernel8.elf`.
```make
# All C source files, and corresponding object files.
C_SRC = $(wildcard *.c)
C_OBJ = $(C_SRC:.c=.o)

kernel8.elf: kernel8.ld boot.o ${C_OBJ}
	@echo "[LD]      $@"
	${Q}${BINROOT}ld -T $< -o $@ $(filter-out $<,$^)
```
Since this is done generically, we should not have to modify the `Makefile` if
we add in more C files (at the root of our working directory).


Testing with QEMU
-----------------

Now, if we simply call `make run`, QEMU will start but we won't be able to see
the messages sent by the kernel, nor send messages to the kernel.

In fact, if QEMU is called with the `-nographic` option (which it is), then it
is automatically set to read/write serial data from stdin/stdout. However, the
problem is that QEMU does that for UART0, not UART1. Hence, we need to use the
flags `-serial null -serial mon:stdio` (in order): discarding UART0 and having
UART1 be connected to the monitor's standard input-outputs (i.e., in the shell
form which we call QEMU).
```make
# Flags passed to QEMU.
QEMU_FLAGS = -M raspi3 -nographic -serial null -serial mon:stdio

.PHONY: run
run: kernel8.img
	@echo "[QEMU]    running with $<"
	@echo "(Press Ctrl-A X to exit QEMU.)"
	${Q}qemu-system-aarch64 ${QEMU_FLAGS} -kernel $<

.PHONY: run-gdb
run-gdb: kernel8.img
	@echo "[QEMU]    running with $< (waiting for GDB)"
	@echo "(Press Ctrl-A X to exit QEMU.)"
	${Q}qemu-system-aarch64 ${QEMU_FLAGS} -s -S -kernel $<
```
You should now be able to interact with the kernel by typing messages into the
console, after running `make run` (or `make run-gdb`).
