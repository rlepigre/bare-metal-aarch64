#pragma once

// Macro to use on unused function parameters to silence GDB warnings.
#define UNUSED(x) (void)(x)

// Absolute value for integer types.
#define ABS(x) ((x) < 0 ? -(x) : (x))
