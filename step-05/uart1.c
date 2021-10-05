#include <macros.h>
#include <bits.h>
#include <types.h>
#include <bcm2837/auxiliaries.h>
#include <bcm2837/gpio.h>
#include <bcm2837/uart1.h>

// Wait for at least n CPU cycles.
void wait_cycles(u32 n){
  while(n--){
    asm volatile("nop");
  }
}

void uart1_init(){
  // We first need to map UART1 to the GPIO pins.
  // We need to set the function of pins 14 and 15 to alternative 5.
  // (Both are configured via alternate function select register 0: GPFSEL1.)
  u32 r = *GPFSEL1;       // Get the current value of the register.
  r &= ~MASK_U32(12, 3);  // Clear the config for pin 14.
  r &= ~MASK_U32(15, 3);  // Clear the config for pin 15.
  r |= GPFSEL_ALT5 << 12; // Select alternative function 5 for pin 14.
  r |= GPFSEL_ALT5 << 15; // Select alternative function 5 for pin 15.
  *GPFSEL1 = r;           // Write the new configuration.

  // We also need to disable pull-up/down for pins 14 and 15.
  *GPPUD = GPPUD_OFF;     // Set the configuration we want to write.
  wait_cycles(150);       // Wait for 150 cycles.
  *GPPUDCLK0 = BIT_U32(14) | BIT_U32(15); // Assert clock on pins 14 and 15.
  wait_cycles(150);       // Wait for 150 cycles again.
  *GPPUD = GPPUD_OFF;     // Write to GPPUD (is it necessary?).
  *GPPUDCLK0 = 0;         // Remove the clock on pins 14 and 15.

  // We can now initialise UART1 (mini UART), which we must first enable.
  *AUX_ENABLES |= AUX_ENABLES_BIT_UART1;

  // Configuration of the mini UART.
  *AUX_MU_CNTL_REG = 0;   // Clear the control register (disables Tx and Rx).
  *AUX_MU_LCR_REG = 3;    // Use the 8-bit mode.
  *AUX_MU_MCR_REG = 0;    // Set the UART1_RTS line high (why?).
  *AUX_MU_IER_REG = 0;    // Do not generate receive/transmit interrupts.
  *AUX_MU_IIR_REG = 6;    // Clear receive FIFO and clear transmit FIFO.
  *AUX_MU_BAUD_REG = 270; // Set the baud rate to 115200 (if 250MHz clock).

  // Finally, enable both Tx and Rx.
  *AUX_MU_CNTL_REG = AUX_MU_CNTL_RX_ENABLE | AUX_MU_CNTL_TX_ENABLE;
}

void uart1_putc(char c){
  // Wait until the FIFO can accept at least one byte.
  while(!(*AUX_MU_LSR_REG & AUX_MU_LSR_TX_EMPTY)) {
    asm volatile("nop");
  }

  // Write to the buffer.
  *AUX_MU_IO_REG = (u32) c;
}

void uart1_puts(const char *s){
  while(*s){
    if(*s == '\n') uart1_putc('\r');
    uart1_putc(*s);
    s++;
  }
}

char uart1_getc() {
  // Wait until the FIFO hold at least one byte.
  while(!(*AUX_MU_LSR_REG & AUX_MU_LSR_DATA_READY)){
    asm volatile("nop");
  }

  // Read a character.
  char c = (char) *AUX_MU_IO_REG;

  // Convert `\r` into `\n`.
  if(c == '\r') c = '\n';
  return c;
}

size_t uart1_getline(char *lineptr, size_t n){
  size_t nb_read = 0;

  // Read at most n-1 characters.
  while(nb_read < n - 1){
    char c = uart1_getc();

    lineptr[nb_read] = c;
    nb_read++;

    // Stop on newline.
    if(c == '\n') break;
  }

  // Add a null character.
  lineptr[nb_read] = '\0';

  return nb_read;
}
