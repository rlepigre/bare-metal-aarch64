#pragma once
#include <types.h>

// Produce a u32 whose n least significant bits are 1, and all others are 0.
#define ONES_U32(n) (UINT32_MAX >> (32 - (n)))

// Produce a u32 representing a mask of n bits starting at offset off.
#define MASK_U32(off, n) (ONES_U32((n)) << (off))

// Produce a u32 whose i-th bit is 1, and all other bits are 0.
#define BIT_U32(i) (((u32) 1) << (i))
