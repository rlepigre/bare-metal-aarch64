/* Entry point defined in file "boot.S". */
ENTRY(_start)
 
SECTIONS {
  /* Our kernel image will be placed at address 0x80000. */
  . = 0x80000;
  /* It starts with the ".text" segment. */
  .text : {
    /* The ".text" segment itself starts with the code from "boot.S". */
    /* The "_start" symbol (at the beginning of "boot.S") is at 0x80000. */
    *(.text.boot)
  }
}
