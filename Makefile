# Variáveis de Ferramentas
AS = nasm
CC = gcc
LD = ld

# Flags
ASFLAGS = -f bin
CFLAGS = -m32 -ffreestanding -fno-pic -fno-pie -c
LDFLAGS = -m elf_i386 -T linker.ld --oformat binary

# Arquivos
BOOT_SRC = boot.asm
BOOT_BIN = boot.bin
KERNEL_SRC = kernel.c videodriver.c
KERNEL_OBJS = kernel.o videodriver.o
KERNEL_BIN = kernel.bin
IMAGE = nova64.img

# Regra principal (Default)
all: $(IMAGE)

# 1. Cria a imagem final
$(IMAGE): $(BOOT_BIN) $(KERNEL_BIN)
	cat $(BOOT_BIN) $(KERNEL_BIN) > $(IMAGE)
	@echo "Build concluído: $(IMAGE)"

# 2. Compila o Bootloader
$(BOOT_BIN): $(BOOT_SRC)
	$(AS) $(ASFLAGS) $(BOOT_SRC) -o $(BOOT_BIN)

# 3. Compila os arquivos C em objetos (.o)
# A regra %.o: %.c automatiza para qualquer arquivo .c
%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

# 4. Linka os objetos no binário do kernel
$(KERNEL_BIN): $(KERNEL_OBJS)
	$(LD) $(LDFLAGS) -o $(KERNEL_BIN) $(KERNEL_OBJS)

# Limpeza dos arquivos gerados
clean:
	rm -f *.o *.bin *.img

# Comando para rodar no QEMU (opcional, mas ajuda muito)
run: $(IMAGE)
	qemu-system-i386 -drive format=raw,file=$(IMAGE)
