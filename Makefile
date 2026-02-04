#Makefile for Nova64 OS. Originally wrote by Saulo Henrique in Thursday, January 22nd, 2026.

#Last update: Sunday, January 25th, 2026, at 16:05 GMT-3 (Horário de Brasília)

#Makefile

CC = gcc
AS = nasm
LD = ld

INC_DIR = include
SRC_DIR = src
UTIL_DIR = utils


CFLAGS = -m32 -ffreestanding -fno-stack-protector -fno-pie \
         -I$(INC_DIR) -I$(UTIL_DIR) -I$(INC_DIR)/utils -I. -c

LDFLAGS = -m elf_i386 -T linker.ld

KERNEL_OBJS = kernel.o idt_asm.o idt.o videodriver.o kbdriver.o rtcdriver.o pit.o io.o string.o shell.o config.o animations.o

all: nova64.img

nova64.img: boot.bin kernel.bin
	cat boot.bin kernel.bin > nova64.img

boot.bin: $(SRC_DIR)/boot/boot.asm
	$(AS) -f bin $< -o $@ -i$(SRC_DIR)/boot/

kernel.bin: $(KERNEL_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(KERNEL_OBJS) --oformat binary

kernel.o: $(SRC_DIR)/kernel/kernel.c
	$(CC) $(CFLAGS) $< -o $@

videodriver.o: $(SRC_DIR)/drivers/VGA/videodriver.c
	$(CC) $(CFLAGS) $< -o $@

kbdriver.o: $(SRC_DIR)/drivers/keyboard/kbdriver.c
	$(CC) $(CFLAGS) $< -o $@

rtcdriver.o: $(SRC_DIR)/drivers/RTC/rtcdriver.c
	$(CC) $(CFLAGS) $< -o $@

pit.o: $(SRC_DIR)/drivers/timer/pit.c
	$(CC) $(CFLAGS) $< -o $@

animations.o: $(SRC_DIR)/animations/animations.c
	$(CC) $(CFLAGS) $< -o $@

idt.o: $(UTIL_DIR)/idt.c
	$(CC) $(CFLAGS) $< -o $@

idt_asm.o: $(UTIL_DIR)/idt_asm.asm
	$(AS) -f elf32 $< -o $@

io.o: $(UTIL_DIR)/io.c
	$(CC) $(CFLAGS) $< -o $@

shell.o: $(SRC_DIR)/shell/shell.c
	$(CC) $(CFLAGS) $< -o $@

string.o: $(SRC_DIR)/utils/string.c
	$(CC) $(CFLAGS) $< -o $@

config.o: $(SRC_DIR)/utils/config.c
	$(CC) $(CFLAGS) $< -o $@
	

clean:
	rm -f *.bin *.o

run: all
	qemu-system-i386 -drive format=raw,file=nova64.img -vga std



