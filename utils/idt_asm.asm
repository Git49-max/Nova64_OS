[bits 64]
[global idt_load_raw]
[global irq0]
[global irq1]
[global irq_common_stub]
[extern irq0_handler]
[extern irq1_handler]
[extern common_irq_handler]

idt_load_raw:
    lidt [rdi]
    ret

%macro pushall 0
    push rax
    push rbx
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
%endmacro

%macro popall 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rdx
    pop rcx
    pop rbx
    pop rax
%endmacro

irq0:
    pushall
    call irq0_handler
    popall
    iretq

irq1:
    pushall
    call irq1_handler
    popall
    iretq

irq_common_stub:
    pushall
    call common_irq_handler
    popall
    iretq