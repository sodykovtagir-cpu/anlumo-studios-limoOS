# (c) 2026 Anlumo Studios
CC = i686-elf-gcc
LD = i686-elf-gcc
ASM = nasm

CFLAGS = -ffreestanding -m32 -nostdlib -Wall -Wextra
LDFLAGS = -ffreestanding -O2 -nostdlib

OBJS = boot.o kernel.o vga.o gdt.o gdt_asm.o keyboard.o vfs.o

ISO_DIR = iso
ISO_BOOT = $(ISO_DIR)/boot
ISO_GRUB = $(ISO_BOOT)/grub

all: limos.bin

boot.o: boot.asm
	$(ASM) -f elf32 boot.asm -o boot.o

kernel.o: kernel.c
	$(CC) $(CFLAGS) -c kernel.c -o kernel.o

vga.o: drivers/vga.c drivers/io.h
	$(CC) $(CFLAGS) -c drivers/vga.c -o vga.o

keyboard.o: drivers/keyboard.c drivers/io.h
	$(CC) $(CFLAGS) -c drivers/keyboard.c -o keyboard.o

vfs.o: drivers/vfs.c drivers/vfs.h
	$(CC) $(CFLAGS) -c drivers/vfs.c -o vfs.o

gdt.o: kernel/gdt.c
	$(CC) $(CFLAGS) -c kernel/gdt.c -o gdt.o

gdt_asm.o: kernel/gdt.asm
	$(ASM) -f elf32 kernel/gdt.asm -o gdt_asm.o

limos.bin: $(OBJS)
	$(LD) $(LDFLAGS) -T linker.ld -o $@ $(OBJS)

run: limos.bin
	qemu-system-i386 -kernel limos.bin

iso: limos.bin
	@mkdir -p $(ISO_GRUB)
	@cp limos.bin $(ISO_BOOT)/
	@echo "set timeout=5" > $(ISO_GRUB)/grub.cfg
	@echo "set default=0" >> $(ISO_GRUB)/grub.cfg
	@echo "" >> $(ISO_GRUB)/grub.cfg
	@echo "menuentry \"LimoOS v0.2\" {" >> $(ISO_GRUB)/grub.cfg
	@echo "    multiboot /boot/limos.bin" >> $(ISO_GRUB)/grub.cfg
	@echo "    boot" >> $(ISO_GRUB)/grub.cfg
	@echo "}" >> $(ISO_GRUB)/grub.cfg
	@grub-mkrescue -o limos.iso $(ISO_DIR)
	@echo "ISO created: limos.iso"

clean:
	rm -f *.o *.bin
	rm -rf $(ISO_DIR) limos.iso