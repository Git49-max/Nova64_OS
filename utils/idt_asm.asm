[bits 32]
[global idt_load]
[global irq0]
[global irq1]
[global irq_common_stub]
[extern idtp]
[extern keyboard_handler]
[extern timer_handler]

idt_load:
    lidt [idtp]
    ret

irq0:
    pusha          
    push ds
    push es
    push fs
    push gs

    call timer_handler

    pop gs
    pop fs
    pop es
    pop ds
    mov al, 0x20
    out 0x20, al
    popa           
    iretd
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