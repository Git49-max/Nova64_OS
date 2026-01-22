;Global Descriptor Table Nova64 OS. Originally wrote by Saulo Henrique in Thursday, January 22nd, 2026.

;Last update: Thursday, January 22nd, ;026, at 17:16 GMT-3 (Horário de Brasília)

;gdt.asm

gdt_start:
    dq 0x0
gdt_code:
    dw 0xffff, 0x0
    db 0x0, 10011010b, 11001111b, 0x0
gdt_data:
    dw 0xffff, 0x0
    db 0x0, 10010010b, 11001111b, 0x0
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start
