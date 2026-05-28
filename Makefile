CC = i686-elf-gcc
LD = i686-elf-gcc
ASM = nasm

CFLAGS = -ffreestanding -m32 -nostdlib -Wall -Wextra
LDFLAGS = -ffreestanding -O2 -nostdlib

OBJS = boot.o kernel.o vga.o gdt.o gdt_asm.o keyboard.o

all: limos.bin

boot.o: boot.asm
	$(ASM) -f elf32 boot.asm -o boot.o

kernel.o: kernel.c
	$(CC) $(CFLAGS) -c kernel.c -o kernel.o

vga.o: drivers/vga.c drivers/io.h
	$(CC) $(CFLAGS) -c drivers/vga.c -o vga.o

keyboard.o: drivers/keyboard.c drivers/io.h
	$(CC) $(CFLAGS) -c drivers/keyboard.c -o keyboard.o

gdt.o: kernel/gdt.c
	$(CC) $(CFLAGS) -c kernel/gdt.c -o gdt.o

gdt_asm.o: kernel/gdt.asm
	$(ASM) -f elf32 kernel/gdt.asm -o gdt_asm.o

limos.bin: $(OBJS)
	$(LD) $(LDFLAGS) -T linker.ld -o $@ $(OBJS)

run: limos.bin
	qemu-system-i386 -kernel limos.bin

clean:
	rm -f *.o *.bin