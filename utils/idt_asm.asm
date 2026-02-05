[bits 64]

global idt_load_raw
global isr_exception_stub
global isr_irq_stub

idt_load_raw:
    lidt [rdi]
    ret


; ================= EXCEÇÕES (TEM ERROR CODE) =================
isr_exception_stub:
    push rbp
    mov rbp, rsp

    add rsp, 8       ; descarta error code
    iretq


; ================= IRQs (SEM ERROR CODE) =================
isr_irq_stub:
    push rbp
    mov rbp, rsp
    iretq
