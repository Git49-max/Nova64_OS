;Bootloader for Nova64 OS. Originally wrote by Saulo Henrique in Thursday, January 22nd, 2026.

;Last update: Thursday, January 22nd, 2026, at 20:28 GMT-3 (Horário de Brasília)

;boot.asm

[bits 16]
[org 0x7c00]

KERNEL_OFFSET equ 0x1000

mov [BOOT_DRIVE], dl 

; Configura a pilha
mov bp, 0x9000
mov sp, bp

call load_kernel     
call switch_to_pm    
jmp $

%include "gdt.asm"

[bits 16]
load_kernel:
    mov ah, 0x00        ; Reset disco
    int 0x13
    
    mov ax, 0x0000
    mov es, ax
    mov bx, KERNEL_OFFSET 

    mov ah, 0x02        ; Ler setores
    mov al, 15          ; Quantidade de setores do kernel
    mov ch, 0x00
    mov dh, 0x00
    mov cl, 0x02        ; Começa no setor 2
    mov dl, [BOOT_DRIVE]
    int 0x13
    ret

[bits 16]
switch_to_pm:
    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    jmp CODE_SEG:init_pm ; "Far jump" para limpar o cache 16-bit

[bits 32]
init_pm:
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call KERNEL_OFFSET   ; Pula para o C
    jmp $

BOOT_DRIVE db 0
times 510-($-$$) db 0
dw 0xaa55
