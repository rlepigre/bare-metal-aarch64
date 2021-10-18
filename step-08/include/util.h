#include <types.h>

// Behaves similarly to standard function strtoull, but:
// - does not accept a leading '-',
// - requires base to be non-zero,
// - does not accept leading "0x" in base 16.
u64 strtou64(const char *nptr, char **endptr, int base);
