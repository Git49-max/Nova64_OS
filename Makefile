#Makefile for Nova64 OS. Originally wrote by Saulo Henrique in Thursday, January 22nd, 2026.

#Last update: Saturday, January 24th, 2026, at 12:15 GMT-3 (Horário de Brasília)

#Makefile

AS = nasm
CC = gcc
LD = ld

ASFLAGS = -f bin
AS_ELF_FLAGS = -f elf32
CFLAGS = -m32 -ffreestanding -fno-pic -fno-pie -c
LDFLAGS = -m elf_i386 -T linker.ld --oformat binary

BOOT_SRC = boot.asm
BOOT_BIN = boot.bin
KERNEL_BIN = kernel.bin
IMAGE = nova64.img

KERNEL_OBJS = kernel.o videodriver.o rtcdriver.o io.o kbdriver.o idt.o idt_asm.o

all: $(IMAGE)

$(IMAGE): $(BOOT_BIN) $(KERNEL_BIN)
	cat $(BOOT_BIN) $(KERNEL_BIN) > $(IMAGE)

$(BOOT_BIN): $(BOOT_SRC)
	$(AS) $(ASFLAGS) $(BOOT_SRC) -o $(BOOT_BIN)

idt_asm.o: idt_asm.asm
	$(AS) $(AS_ELF_FLAGS) idt_asm.asm -o idt_asm.o

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

$(KERNEL_BIN): $(KERNEL_OBJS)
	$(LD) $(LDFLAGS) -o $(KERNEL_BIN) $(KERNEL_OBJS)

clean:
	rm -f *.o *.bin *.img

run: $(IMAGE)
	qemu-system-i386 -drive format=raw,file=$(IMAGE)

