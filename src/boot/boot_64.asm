[bits 32]
global start
extern main

section .text
start:
    cli
    mov esp, stack_top

    ; Limpar tabelas de página
    mov edi, 0x1000    ; PML4
    mov cr3, edi
    xor eax, eax
    mov ecx, 4096
    rep stosd

    ; Configurar Hierarquia de Paginação
    mov edi, 0x1000
    mov dword [edi], 0x2003          ; PML4 -> PDPT (0x2000)
    mov dword [edi + 0x1000], 0x3003 ; PDPT -> PD (0x3000)

    ; Mapear 1GB usando 512 entradas de 2MB (Huge Pages)
    mov edi, 0x3000
    mov eax, 0x00000083 ; Present + Writable + Huge
    mov ecx, 512
.map_pages:
    mov [edi], eax
    add eax, 0x200000   ; Próximos 2MB
    add edi, 8          ; Próxima entrada (64-bit)
    loop .map_pages

    ; Ativar PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Ativar Long Mode no EFER
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; Ativar Paginação
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    lgdt [gdt64.pointer]
    jmp 0x08:long_mode_start

[bits 64]
long_mode_start:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov rsp, stack_top
    call main
.hang: hlt
    jmp .hang

section .rodata
gdt64:
    dq 0 ; entry 0
.code: equ $ - gdt64
    dq (1<<43) | (1<<44) | (1<<47) | (1<<53) ; code segment
.data: equ $ - gdt64
    dq (1<<44) | (1<<47) | (1<<41) ; data segment
.pointer:
    dw $ - gdt64 - 1
    dq gdt64

section .bss
align 16
stack_bottom: resb 16384
stack_top: