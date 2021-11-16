/* emulate rpi3 firmware by putting dtb/kernel addr in proper places
 */

.globl _start
_start:
// overwrite magic number at 0xF0 with 0x0000
    mov x0, 0xf0
// write DTB pointer to 0xF8
    str wzr, [x0]
    ldr x0, =0xf8
    ldr x1, =0x01000000
    str w1, [x0]
// write Kernel pointer to 0xFC
    ldr x0, =0xfc
    ldr x1, =0x02000000
    str w1, [x0]
// jump to armstub8.bin
    ldr x0, =0x4
    br x0