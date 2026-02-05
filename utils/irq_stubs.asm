[bits 64]
global irq0_stub
extern timer_handler

irq0_stub:
    ; CPU já entra com IF=0, não precisa cli

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

    ; ---- chama handler em C ----
    call timer_handler

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
