PREFIX = aarch64-linux-gnu-
CC = $(PREFIX)gcc -I . -DDEBUG
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump
LD = $(PREFIX)ld
ROOTDIR =
Q = @

all: qemu_stub.bin

%.o: %.S
	@echo "[AS]      $(ROOTDIR)$@"
	$(Q)$(CC) -c $< -o $@

%.elf: %.o
	@echo "[LD]      $(ROOTDIR)$@"
	$(Q)$(LD) --section-start=.text=0 -o $@ $^
	@echo "[OBJDUMP] $(ROOTDIR)$@.S"
	$(Q)$(OBJDUMP) -D $@ > $@.S

%.bin: %.elf
	@echo "[OBJCOPY] $(ROOTDIR)$@"
	$(Q)$(OBJCOPY) $< -O binary $@

clean:
	rm -f qemu_stub.o qemu_stub.elf qemu_stub.elf.S qemu_stub.bin