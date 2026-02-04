[bits 16]
[org 0x7c00]

KERNEL_OFFSET equ 0x8000 

start:
    mov [BOOT_DRIVE], dl
    mov bp, 0x9000
    mov sp, bp

    ; Carregar Kernel (64 setores)
    mov ah, 0x02
    mov al, 64          
    mov ch, 0x00
    mov dh, 0x00
    mov cl, 0x02
    mov dl, [BOOT_DRIVE]
    mov bx, KERNEL_OFFSET
    int 0x13

    ; --- Paginação Identity Mapping 2MB ---
    mov edi, 0x1000
    xor eax, eax
    mov ecx, 4096
    rep stosd

    mov dword [0x1000], 0x2003 ; PML4
    mov dword [0x2000], 0x3003 ; PDPT
    mov dword [0x3000], 0x4003 ; PD

    mov edi, 0x4000
    mov eax, 0x00000003
    mov ecx, 512
.map_pt:
    mov [edi], eax
    mov dword [edi+4], 0
    add eax, 0x1000
    add edi, 8
    loop .map_pt

    mov eax, 0x1000
    mov cr3, eax

    cli
    lgdt [gdt_descriptor]

    mov eax, cr4
    or eax, 1 << 5    ; PAE
    mov cr4, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8    ; LME
    wrmsr

    mov eax, cr0
    or eax, 0x80000001
    mov cr0, eax

    jmp 0x08:init_64bit

[bits 64]
init_64bit:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    ; CONFIGURAÇÃO DA PILHA REAL
    mov rsp, 0x90000
    and rsp, -16      ; Alinhamento de 16 bytes para System V ABI
    mov rbp, rsp

    call KERNEL_OFFSET
    jmp $

%include "src/boot/gdt.asm"
BOOT_DRIVE db 0
times 510-($-$$) db 0
dw 0xaa55