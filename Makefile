CC = gcc
AS = nasm
LD = ld

# Diretórios
INC_DIR = include
SRC_DIR = src
UTIL_DIR = utils
BOOT_DIR = $(SRC_DIR)/boot

CFLAGS = -m64 -ffreestanding -fno-stack-protector -fno-pie -fno-pic \
         -mno-red-zone -Iinclude -c \
         -I$(INC_DIR) -I$(UTIL_DIR) -I$(INC_DIR)/utils -I$(INC_DIR)/timer \
         -I$(INC_DIR)/VGA -I$(INC_DIR)/keyboard -I$(INC_DIR)/RTC -I$(INC_DIR)/shell \
         -I. -c

# Removemos o --oformat binary porque o GRUB precisa do arquivo ELF completo
LDFLAGS = -m elf_x86_64 -T linker.ld -z max-page-size=0x1000

# Adicionamos os novos objetos de boot do GRUB
KERNEL_OBJS = multiboot_header.o boot_64.o kernel.o videodriver.o kbdriver.o \
              rtcdriver.o pit.o io.o string.o shell.o config.o animations.o \
              idt.o idt_asm.o irq_stubs.o irq1_stubs.o stellar.o pmm.o malloc.o \
			  fat32.o ata.o stellar_compiler.o stellar_errors.o

all: nova64.iso

# Cria a ISO bootável
nova64.iso: kernel.bin
	mkdir -p isodir/boot/grub
	cp kernel.bin isodir/boot/kernel.bin
	echo 'set timeout=0' > isodir/boot/grub/grub.cfg
	echo 'set default=0' >> isodir/boot/grub/grub.cfg
	echo 'menuentry "Nova64 OS" {' >> isodir/boot/grub/grub.cfg
	echo '    multiboot2 /boot/kernel.bin' >> isodir/boot/grub/grub.cfg
	echo '    boot' >> isodir/boot/grub/grub.cfg
	echo '}' >> isodir/boot/grub/grub.cfg
	grub-mkrescue -o nova64.iso isodir

kernel.bin: $(KERNEL_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(KERNEL_OBJS)

# --- Objetos de Boot (NOVOS) ---
multiboot_header.o: $(BOOT_DIR)/multiboot_header.asm
	$(AS) -f elf64 $< -o $@

boot_64.o: $(BOOT_DIR)/boot_64.asm
	$(AS) -f elf64 $< -o $@

# --- Objetos do Kernel e Utils ---
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

ata.o: $(SRC_DIR)/drivers/disk/ata.c
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

irq_stubs.o: utils/irq_stubs.asm
	$(AS) -f elf64 $< -o $@

irq1_stubs.o: utils/irq1_stubs.asm
	$(AS) -f elf64 $< -o $@

stellar.o: $(SRC_DIR)/stellar/stellar.c
	$(CC) $(CFLAGS) $< -o $@

stellar_compiler.o: $(SRC_DIR)/stellar/stellar_compiler.c
	$(CC) $(CFLAGS) $< -o $@

stellar_errors.o: $(SRC_DIR)/stellar/stellar_errors.c
	$(CC) $(CFLAGS) $< -o $@

pmm.o: $(SRC_DIR)/utils/pmm.c
	$(CC) $(CFLAGS) $< -o $@

malloc.o: $(SRC_DIR)/utils/malloc.c
	$(CC) $(CFLAGS) $< -o $@

fat32.o: $(SRC_DIR)/fs/fat32.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *.bin *.o isodir

run: all
	qemu-system-x86_64 -cdrom nova64.iso -vga std -d int,cpu_reset -D qemu.log -hda disk.img -boot d

.PHONY: stellar
stellar:
	gcc -DSTELLAR_HOST -fno-builtin -I./include src/tools/host_main.c -o stellar