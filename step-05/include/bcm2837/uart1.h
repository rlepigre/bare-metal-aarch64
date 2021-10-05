#pragma once
#include <stddef.h>

// Initialise UART1: must be called before using other "uart1_*" function.
// Note: after initialisation, GPIO ports 14 and 15 reserved for UART1.
void uart1_init();

// Important note: the following operations are blocking if the UART1 internal
// (input or output) buffer is full.

// Write character c to UART1.
void uart1_putc(char c);

// Write the null-terminated string s to UART1.
// Note: the character "\n" is written as the sequence "\r\n".
void uart1_puts(const char *s);

// Read a character from UART1.
// Note: the character "\r" is converted into "\n".
char uart1_getc();

// Read characters from UART1 into the string buffer buf, whose capacity is n.
// We stop reading if either:
// - n-1 characters have been read (we reserve space for the null terminator),
// - a '\r' character is found (it is written to the buffer as well).
// Note: the character "\r" is converted into "\n".
size_t uart1_getline(char *buf, size_t n);
