[global idt_load]
[global irq1]
[extern idtp]
[extern keyboard_handler]

idt_load:
    lidt [idtp]
    ret

irq1:
    pusha
    call keyboard_handler
    popa
    iretd
