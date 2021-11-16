# Relative path to the prefix where the compiler was installed.
COMPILER_PREFIX = ../compiler/prefix

# Prefix to use before all binutils, gcc and gdb commands.
BINROOT = ${COMPILER_PREFIX}/bin/aarch64-elf-

# Variable used to control the printing of commands.
# Printing is disabled by default (due to the "@").
# To enable command printing run "make Q= ..." instead of "make ...".
Q = @

# Flags passed to GCC.
GCC_FLAGS = \
	-ffreestanding -nostdlib -fomit-frame-pointer -fno-pic -fno-pie \
	-Wall -Wextra -Werror -pedantic \
	-O0 \
	-I ./include \
	-mgeneral-regs-only

LDFLAGS = -nostdlib -static -pie

# Flags passed to QEMU.
# the first line are options to tell it to use the raspi3 architecture and pipe the serial to stdin/stdout
# the second line loads the EL3 non-secure firmware package at 0x0 like the raspi3 has
# the third line places the qemu_stub that writes the data that the actual raspi3 bootloader would
# the fourth line loads the kernel image at the load address
# the fifth line sets the PC to start at the correct stub
# now QEMU will run like it really does on the RPi3 (at least somewhat...)
QEMU_FLAGS_FIRMWARE = \
	-machine raspi3 -display none -serial null -serial mon:stdio \
	-device loader,file=firmware/prebuilt/armstub8.bin,addr=0x0 \
	-device loader,file=firmware/prebuilt/qemu_stub.bin,addr=0x00a00000 \
	-device loader,file=kernel8.img,addr=0x02000000 \
	-device loader,addr=0x00a00000,cpu-num=0

# otherwise just use the default...
QEMU_FLAGS = \
	-machine raspi3 -display none -serial null -serial mon:stdio \
	-kernel kernel8.img

.PHONY: all
all: kernel8.img

boot.o: boot.S
	@echo "[AS]      $@"
	${Q}${BINROOT}as -c $< -o $@

# All header files.
C_HDR = $(wildcard include/*.h) $(wildcard include/bcm2837/*.h)

%.o: %.c ${C_HDR}
	@echo "[GCC]     $@"
	${Q}${BINROOT}gcc ${GCC_FLAGS} -c $< -o $@

# All C source files, and corresponding object files.
C_SRC = $(wildcard *.c)
C_OBJ = $(C_SRC:.c=.o)

kernel8.elf: kernel8.ld boot.o ${C_OBJ}
	@echo "[LD]      $@"
	${Q}${BINROOT}ld ${LDFLAGS} -T $< -o $@ $(filter-out $<,$^)

kernel8.elf.S: kernel8.elf
	@echo "[OBJDUMP] $@"
	${Q}${BINROOT}objdump -D $< > $@

kernel8.img: kernel8.elf kernel8.elf.S
	@echo "[OBJCOPY] $@"
	${Q}${BINROOT}objcopy -O binary $< $@

.PHONY: run
run: kernel8.img
	@echo "[QEMU]    running with $<"
	@echo "(Press Ctrl-A X to exit QEMU.)"
	${Q}qemu-system-aarch64 ${QEMU_FLAGS}

.PHONY: run-gdb
run-gdb: kernel8.img
	@echo "[QEMU]    running with $< (waiting for GDB)"
	@echo "(Press Ctrl-A X to exit QEMU.)"
	${Q}qemu-system-aarch64 ${QEMU_FLAGS} -s -S

.PHONY: gdb
gdb: kernel8.elf
	@echo "[GDB]     running with $<"
	${Q}${BINROOT}gdb -ex "target remote :1234" $<

.PHONY: clean
clean:
	@rm -f *.o
	@rm -f kernel8.elf
	@rm -f kernel8.elf.S
	@rm -f kernel8.img
	@$(MAKE) -s -C firmware clean
