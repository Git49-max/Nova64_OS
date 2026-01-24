#Makefile for Nova64 OS. Originally wrote by Saulo Henrique in Thursday, January 22nd, 2026.

#Last update: Saturday, January 24th, 2026, at 12:15 GMT-3 (Horário de Brasília)

#Makefile

CC = gcc
AS = nasm
LD = ld

# Adicionei idt.o e idt_asm.o aqui
KERNEL_OBJS = kernel.o idt_asm.o idt.o videodriver.o kbdriver.o rtcdriver.o io.o

CFLAGS = -m32 -ffreestanding -fno-stack-protector -fno-pie -c
LDFLAGS = -m elf_i386 -T linker.ld

all: os-image.bin

os-image.bin: boot.bin kernel.bin
	cat boot.bin kernel.bin > os-image.bin

boot.bin: boot.asm
	$(AS) -f bin boot.asm -o boot.bin

kernel.bin: $(KERNEL_OBJS)
	$(LD) $(LDFLAGS) -o kernel.bin $(KERNEL_OBJS) --oformat binary

# Regra para o assembly da IDT
idt_asm.o: idt_asm.asm
	$(AS) -f elf32 idt_asm.asm -o idt_asm.o

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.bin *.o


