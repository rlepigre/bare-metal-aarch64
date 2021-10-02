# Relative path to the prefix where the compiler was installed.
COMPILER_PREFIX = ../compiler/prefix

# Prefix to use before all binutils, gcc and gdb commands.
BINROOT = ${COMPILER_PREFIX}/bin/aarch64-elf-

# Variable used to control the printing of commands.
# Printing is disabled by default (due to the "@").
# To enable command printing run "make Q= ..." instead of "make ...".
Q = @

.PHONY: all
all: kernel8.img

boot.o: boot.S
	@echo "[AS]      $@"
	${Q}${BINROOT}as -c $< -o $@

kernel8.elf: kernel8.ld boot.o
	@echo "[LD]      $@"
	${Q}${BINROOT}ld -T $< -o $@ $(filter-out $<,$^)

kernel8.img: kernel8.elf
	@echo "[OBJCOPY] $@"
	${Q}${BINROOT}objcopy -O binary $< $@

.PHONY: run
run: kernel8.img
	@echo "[QEMU]    running with $<"
	@echo "(Press Ctrl-A X to exit QEMU.)"
	${Q}qemu-system-aarch64 -M raspi3 -nographic -kernel $<

.PHONY: clean
clean:
	@rm -f boot.o
	@rm -f kernel8.elf
	@rm -f kernel8.img
