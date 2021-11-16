#include <macros.h>
#include <types.h>
#include <bcm2837/uart1.h>
#include <limits.h>
#include <string.h>
#include <util.h>
#include <fdt.h>
#include <kernel/shell.h>

// Entry point for the kernel, allocated in the ".text" segment.
// This function never returns.
void kernel_entry(void *dtb, u64 x1, u64 x2, u64 x3, u64 x4, u64 x5, u64 x6){
  // Initialise the UART, and print a first message.
  uart1_init();
  uart1_puts("********************************************\n");
  uart1_puts("*              Hello, World!!              *\n");
  uart1_puts("********************************************\n");

  // Print information about the environment.
  uart1_printf("Initial value of x1:     0x%w.\n", x1);
  uart1_printf("Initial value of x2:     0x%w.\n", x2);
  uart1_printf("Initial value of x3:     0x%w.\n", x3);
  uart1_printf("Initial entry point:     0x%w.\n", x4);
  uart1_printf("Initial exception level: EL%i.\n", (int) x5);
  uart1_printf("Current exception level: EL%i.\n", (int) x6);
  uart1_printf("Address of the DTB:      ");
  if(dtb){
    uart1_printf("0x%w.\n", (u64) dtb);
    dtb_print_bootargs(dtb);
  } else {
    uart1_printf("n/a\n");
  }

  // Enter the (infinite) shell loop.
  uart1_puts("Entering the interactive mode.\n");
  shell_main(); // Never returns.
}
