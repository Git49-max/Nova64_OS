#Makefile for Nova64 OS. Originally wrote by Saulo Henrique in Thursday, January 22nd, 2026.

#Last update: Friday, January 23rd, 2026, at 19:24 GMT-3 (Horário de Brasília)

#Makefile



AS = nasm
CC = gcc
LD = ld

ASFLAGS = -f bin
CFLAGS = -m32 -ffreestanding -fno-pic -fno-pie -c
LDFLAGS = -m elf_i386 -T linker.ld --oformat binary

BOOT_SRC = boot.asm
BOOT_BIN = boot.bin
KERNEL_BIN = kernel.bin
IMAGE = nova64.img

KERNEL_OBJS = kernel.o videodriver.o rtcdriver.o io.o

all: $(IMAGE)

$(IMAGE): $(BOOT_BIN) $(KERNEL_BIN)
	cat $(BOOT_BIN) $(KERNEL_BIN) > $(IMAGE)

$(BOOT_BIN): $(BOOT_SRC)
	$(AS) $(ASFLAGS) $(BOOT_SRC) -o $(BOOT_BIN)

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

$(KERNEL_BIN): $(KERNEL_OBJS)
	$(LD) $(LDFLAGS) -o $(KERNEL_BIN) $(KERNEL_OBJS)

clean:
	rm -f *.o *.bin *.img

run: $(IMAGE)
	qemu-system-i386 -drive format=raw,file=$(IMAGE)
