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
  // Check that x0 has a valid DTB pointer, otherwise set it to 0.
  ldr w5, [x0]          // Load a half-word from the address in x0 into w5.
  ldr w6, =0xedfe0dd0   // Put the (reversed) DTB magic number in w6.
  cmp w5, w6            // Compare the two.
  b.eq done_with_dtb    // If they are equal, x0 is a valid DTB pointer.
  mov x0, xzr           // If not we change the value of x0 to 0.
done_with_dtb:

  // Put the current exception level in x5.
  mrs x5, CurrentEL     // Move the CurrentEL system register into x5.
  ubfx x5, x5, #2, #2   // Extract the relevant bitfield (bits 3:2).

  // Set the SPSel register so that SP_EL0 is the stack pointer at all EL.
  mrs x6, SPSel         // Move the current SPSel system register into x6.
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

  // Install an exception vector.
  ldr x6, =el2_exception_vector
  msr vbar_el2, x6

  // Move to EL1.
  mov x6, (1 << 31)     // Hypervisor configuration: aarch64 mode for EL1.
  msr hcr_el2, x6       // (Written to the HCR_EL2 system register.)
  mov x6, 0x3c4         // Configure EL1 state (use SP_EL0, no interupts).
  msr spsr_el2, x6      // (Written to the SPSR_EL2 system register.)
  ldr x6, =enter_el1    // Specify the "return from exception" address.
  msr elr_el2, x6       // (Written to the ELR_EL2 system register.)
  eret                  // Simulate an "exception return" to move to EL1.
enter_el1:

  // Put the current exception level in x6 (as we did for x5 above).
  mrs x6, CurrentEL
  ubfx x6, x6, #2, #2

  // Call the "kernel_entry" C function with the following arguments:
  // - the (untouched) pointer to the DTB from register x0,
  // - the (untouched) value initially in x1,
  // - the (untouched) value initially in x2,
  // - the (untouched) value initially in x3,
  // - the (untouched) value initially in x4 (0x80000, not reliable),
  // - the value of register x5 (exception level upon entry).
  // - the value of register x6 (exception level at kernel entry).
  // (This call should never return.)
  bl kernel_entry

  // Hang forever in a loop (just in case).
hang_forever:
  wfe                   // Allow the CPU to go to low-power mode.
  b hang_forever

// Our exception vector for EL2.
.align 11
el2_exception_vector:
  // Synchronous - Current EL with SP0.
  .align 7
  b .
  // IRQ - Current EL with SP0.
  .align 7
  b .
  // FIQ - Current EL with SP0.
  .align 7
  b .
  // SError - Current EL with SP0.
  .align 7
  b .
  // Synchronous - Current EL with SPx.
  .align 7
  b .
  // IRQ - Current EL with SPx.
  .align 7
  b .
  // FIQ - Current EL with SPx.
  .align 7
  b .
  // SError - Current EL with SPx.
  .align 7
  b .
  // Synchronous - Lower EL with AArch64.
  .align 7
  b hvc_handler
  // IRQ - Lower EL with AArch64.
  .align 7
  b .
  // FIQ - Lower EL with AArch64.
  .align 7
  b .
  // SError - Lower EL with AArch64.
  .align 7
  b .
  // Synchronous - Lower EL with AArch32.
  .align 7
  b .
  // IRQ - Lower EL with AArch32.
  .align 7
  b .
  // FIQ - Lower EL with AArch32.
  .align 7
  b .
  // SError - Lower EL with AArch32.
  .align 7
  b .

hvc_handler:
  // Configure SPSel to use the SP_EL0.
  msr SPSel, #0

  // Save the registers we will need to the stack.
  sub sp, sp, #0x10
  stp x0, x1, [sp]

  // The parameter given to the "hvc" instruction is in bits 15-0 of ESR_EL2.
  mrs x0, ESR_EL2
  ubfx x0, x0, #0, #15

  // If x0 is 42: increment operation.
  cmp x0, #42
  b.eq hvc_handle_increment

  // If x0 is 73: read operation
  cmp x0, #73
  b.eq hvc_handle_get

  // No other supported hypercall, just loop.
  b .

hvc_handle_increment:
  ldr x0, =secret_counter // Put the address of the secret counter in x0.
  ldr x1, [x0]            // Load the value of the counter to x1.
  add x1, x1, 0x1         // Increment the counter in x1.
  str x1, [x0]            // Write the new counter value to memory.

  // Restore the saved registers from the stack.
  ldp x0, x1, [sp]
  add sp, sp, #0x10

  // Return from the exception.
  eret

hvc_handle_get:
  ldr x0, =secret_counter // Put the address of the secret counter in x0.
  ldr x1, [x0]            // Load the value of the counter to x1.

  // We use our stack space to return the value.
  mov x0, xzr
  stp x1, x0, [sp]

  // Return from the exception.
  eret

secret_counter:
  .quad 0x0
