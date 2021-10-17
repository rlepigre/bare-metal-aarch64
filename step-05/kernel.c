#include <macros.h>
#include <types.h>
#include <bcm2837/uart1.h>

// Entry point for the kernel, allocated in the ".text" segment.
// This function never returns.
void kernel_entry(void *dtb, u64 x1, u64 x2, u64 x3){
  // Silence GDB warnings about unused parameters.
  UNUSED(dtb); UNUSED(x1); UNUSED(x2); UNUSED(x3);

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
