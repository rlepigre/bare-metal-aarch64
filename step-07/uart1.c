#include <stdarg.h>
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

void uart1_printf(const char *format, ...){
  const char *s = format;
  unsigned char b; // Used for the "%b" format string directive.
  u64 w; // Used for the "%w" format string directive.
  u32 h; // Used for the "%h" format string directive.
  int i, d, q, r;

  // We need a character buffer for printing decimal representations.
  char buf[20]; // 64-bit integers have at most 19 decimal digits.
  int pos;      // Position in the bufer.

  va_list ap;
  va_start(ap, format);

  while(*s){
    switch(*s){
    case '\n':
      uart1_putc('\r');
      uart1_putc('\n');
      s++;
      break;
    case '%':
      s++;
      switch(*s){
      case '\0':
        uart1_puts("<MISSING MARKER>");
        return;
      case '%':
        // Escpade '%' character.
        uart1_putc(*s);
        s++;
        break;
      case 's':
        // String.
        uart1_puts(va_arg(ap, const char *));
        s++;
        break;
      case 'c':
        // Character.
        uart1_putc((char) va_arg(ap, int));
        s++;
        break;
      case 'b':
        // Fixed-width hexadecimal representation of a character.
        b = (unsigned char) va_arg(ap, int);
        for(i = 4; i >= 0; i -= 4){
          d = (b >> i) & 0xf;
          if(d <= 0x9){
            uart1_putc('0' + (char) d);
          } else {
            uart1_putc('a' + (char) (d - 10));
          }
        }
        s++;
        break;
      case 'w':
        // Fixed-width hexadecimal representation for a word (u64).
        w = va_arg(ap, u64);
        for(i = 60; i >= 0; i -= 4){
          d = (w >> i) & 0xf;
          if(d <= 0x9){
            uart1_putc('0' + (char) d);
          } else {
            uart1_putc('a' + (char) (d - 10));
          }
        }
        s++;
        break;
      case 'h':
        // Fixed-width hexadecimal representation for a half-word (u32).
        h = va_arg(ap, u32);
        for(i = 28; i >= 0; i -= 4){
          d = (h >> i) & 0xf;
          if(d <= 0x9){
            uart1_putc('0' + (char) d);
          } else {
            uart1_putc('a' + (char) (d - 10));
          }
        }
        s++;
        break;
      case 'i':
        // Decimal representation for an integer (int).
        i = va_arg(ap, int);
        if(i < 0) uart1_putc('-');
        q = ABS(i / 10);
        r = ABS(i % 10);
        pos = 19;
        buf[pos--] = '\0';
        do {
          buf[pos--] = (char) ('0' + r);
          r = q % 10;
          q = q / 10;
        } while(q != 0 || r != 0);
        uart1_puts(&(buf[pos+1]));
        s++;
        break;
      default:
        uart1_puts("<BAD MARKER \""); uart1_putc(*s); uart1_puts("\">");
        s++;
      }
      break;
    default:
      uart1_putc(*s);
      s++;
    }
  }

  va_end(ap);
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

  // Print the character back so the users know what they are typing.
  uart1_putc(c);
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
