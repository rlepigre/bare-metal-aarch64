/* Entry point defined in file "boot.S". */
ENTRY(_start)

SECTIONS {
  /* Our kernel image will be placed at address 0x80000. */
  . = 0x80000;
  /* It starts with the ".text" segment. */
  __start = .;
  __text_start = .;
  .text : {
    /* The ".text" segment itself starts with the code from "boot.S". */
    /* The "_start" symbol (at the beginning of "boot.S") is at 0x80000. */
    *(.text.boot)
    /* It then continues with the rest of the ".text" segment. */
    *(.text)
  }
  . = ALIGN(4096); /* Add padding to the next page boundary. */
  __text_end = .;

  /* Read-only data segment (for initialised const C global variables). */
  __rodata_start = .;
  .rodata : {
    *(.rodata)
  }
  . = ALIGN(4096); /* Add padding to the next page boundary. */
  __rodata_end = .;

  /* Data segment (for initialised, non-const C global variables). */
  __data_start = .;
  .data : {
    *(.data)
  }
  . = ALIGN(4096); /* Add padding to the next page boundary. */
  __data_end = .;

  /* BSS segment (for uninitialised C global variables). */
  /* BSS stands for "block starting symbol". */
  /* The BSS segment must be zeroed prior to entering C code. */
  __bss_start = .;
  .bss : {
    *(.bss)
  }
  . = ALIGN(4096); /* Add padding to the next page boundary. */
  __bss_end = .;
  __end = .;
}
