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
  // Put the current exception level in x5.
  mrs x5, CurrentEL     // Move the CurrentEL system register into x5.
  ubfx x5, x5, #2, #2   // Extract the relevant bitfield (bits 3:2).

  // Set the SPSel register so that SP_EL0 is the stack pointer at all EL.
  mrs x6, SPSel         // Move the current SPSel  system register into x6.
  and x6, x6, ~1        // Clear the 0 bit of x6.
  msr SPSel, x6         // Set the value of SPSel to x6.

  // Set up the stack below our code (it grows downwards).
  // This should be plenty big enough: only the first 4KB of memory are used.
  ldr x6, =_start
  mov sp, x6

  // Clear the BSS segment (assumes __bss_start and __bss_end word-aligned).
  ldr x6, =__bss_start  // Current word of the BSS (initially at start).
  ldr x7, =__bss_end    // Address one past the BSS segment.
bss_clear_loop:
  cmp x6, x7            // If we have reached the end ...
  b.ge bss_clear_done   // ... exit the loop.
  str xzr, [x6]         // Otherwise, zero-out the word at address x6,
  add x6, x6, #8        // increment x6 by a word (8 bytes),
  b bss_clear_loop      // and continue to loop.
bss_clear_done:

  // Call the "kernel_entry" C function with the following arguments:
  // - the (untouched) pointer to the DTB from register x0,
  // - the (untouched) value initially in x1,
  // - the (untouched) value initially in x2,
  // - the (untouched) value initially in x3.
  // (This call should never return.)
  bl kernel_entry

  // Hang forever in a loop (just in case).
hang_forever:
  wfe                   // Allow the CPU to go to low-power mode.
  b hang_forever
