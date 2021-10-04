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
