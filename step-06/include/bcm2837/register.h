#pragma once
#include <types.h>

// The documentation for the BCM2837 gives peripheral bus addresses, which are
// not directly mapped to physical addresses. Physical addresses 0x3f000000 to
// 0x3fffffff, used for peripheral MMIO, are mapped starting at the peripheral
// bus addresses range starting at 0x7e000000 (and ending at 0x7effffff).
//
// Example: bus address 0x7e00beef corresponds to physical address 0x3f00beef.

// Convert a bus address into a physical address.
static inline uintptr_t bus_to_phys(u64 addr){
  return (uintptr_t) (addr - 0x3f000000ULL);
}

// Convert a bus address into a 32-bit register pointer.
#define bus_to_reg32(addr) ((volatile u32 *) bus_to_phys(addr))
