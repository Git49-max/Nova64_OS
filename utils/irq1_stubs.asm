[bits 64]
global irq1_stub
extern keyboard_handler

irq1_stub:
    ; ---- salva registradores ----
    push rax
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; ---- chama handler do teclado ----
    call keyboard_handler

    ; ---- EOI no PIC MASTER ----
    mov al, 0x20
    out 0x20, al

    ; ---- restaura registradores ----
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rax

    iretq
