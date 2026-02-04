CC = gcc
AS = nasm
LD = ld

# Diretórios
INC_DIR = include
SRC_DIR = src
UTIL_DIR = utils

# Adicionei caminhos específicos para evitar o erro de "No such file or directory"
# O -I. permite que ele ache arquivos na raiz do projeto (como types.h se estiver lá)
CFLAGS = -m64 -ffreestanding -fno-stack-protector -fno-pie -fno-pic \
         -mno-red-zone -mcmodel=large \
         -I$(INC_DIR) -I$(UTIL_DIR) -I$(INC_DIR)/utils -I$(INC_DIR)/timer \
         -I$(INC_DIR)/VGA -I$(INC_DIR)/keyboard -I$(INC_DIR)/RTC -I$(INC_DIR)/shell \
         -I. -c

LDFLAGS = -m elf_x86_64 -T linker.ld -z max-page-size=0x1000

KERNEL_OBJS = kernel.o videodriver.o kbdriver.o rtcdriver.o pit.o io.o string.o shell.o config.o animations.o idt.o idt_asm.o

all: nova64.img

nova64.img: boot.bin kernel.bin
	cat boot.bin kernel.bin > nova64.img
	truncate -s +64K nova64.img 

boot.bin: $(SRC_DIR)/boot/boot.asm
	$(AS) -f bin $< -o $@ -i$(SRC_DIR)/boot/

kernel.bin: $(KERNEL_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(KERNEL_OBJS) --oformat binary

# Objetos do Kernel e Utils
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
	$(AS) -f elf64 $< -o $@

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
	qemu-system-x86_64 -drive format=raw,file=nova64.img -vga std -d int,cpu_reset -D qemu.log -no-reboot -no-shutdown