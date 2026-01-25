[bits 32]
[global idt_load]
[global irq1]
[global irq_common_stub]
[extern idtp]
[extern keyboard_handler]

idt_load:
    lidt [idtp]
    ret

irq1:
    pusha
    call keyboard_handler
    mov al, 0x20
    out 0x20, al
    popa
    iretd

irq_common_stub:
    pusha
    mov al, 0x20
    out 0x20, al
    popa
    iretd