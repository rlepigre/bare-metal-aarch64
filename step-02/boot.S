// The entry point is in its own section ".text.boot".
.section ".text.boot"

// The symbol "_start" is the kernel's entry point, make it global.
.globl _start

// The auxiliary cores are initially asleep.
// They can be woken up (independently) by following the following steps.
// 1) Writing an address to jump to at:
//    - 0xe0 for core 2,
//    - 0xe8 for core 3,
//    - 0xf0 for core 4.
// 2) Issuing an "sev" instruction to force the core out of low-power mode.

// Entry point for the main core.
// We have the following guarantees on general-purpose registers:
// - x0 contains the physical address of the device tree blob (DTB) in RAM,
// - x1 has value 0 (reserved for future use),
// - x2 has value 0 (reserved for future use),
// - x3 has value 0 (reserved for future use),
// - x4 has value 0x80000 (probably firmware-specific and unreliable).
_start:
  b .
