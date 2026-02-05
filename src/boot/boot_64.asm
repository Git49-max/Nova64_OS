[bits 32]
global start
extern main

section .text
start:
    cli

    ; ---------- stack 32-bit ----------
    mov esp, stack_top

    ; ---------- SEGMENTOS (FIX CRÍTICO) ----------
    mov ax, 0x10
    mov ds, ax
    mov es, ax

    ; ---------- PAGINAÇÃO ----------
    mov edi, 0x1000
    mov cr3, edi

    xor eax, eax
    mov ecx, 4096
    rep stosd

    mov edi, cr3
    mov dword [edi], 0x2003
    mov dword [edi + 0x1000], 0x3003
    mov dword [edi + 0x2000], 0x00000083   ; 2MB page

    ; ---------- ATIVAR PAE ----------
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; ---------- ATIVAR LONG MODE ----------
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; ---------- ATIVAR PAGINAÇÃO ----------
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    lgdt [gdt64.pointer]
    jmp 0x08:long_mode_start


; ==================================================
; ================= 64 BIT ==========================
; ==================================================

[bits 64]
global stack_top

long_mode_start:

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov rsp, stack_top
    and rsp, -16
    mov rbp, rsp

    call main

.hang:
    hlt
    jmp .hang


section .rodata
gdt64:
    dq 0

.code:
    dq (1<<43) | (1<<44) | (1<<47) | (1<<53)

.data:
    dq (1<<44) | (1<<47) | (1<<41)

.pointer:
    dw $ - gdt64 - 1
    dq gdt64


section .bss
align 16
stack_bottom:
    resb 16384
stack_top:
