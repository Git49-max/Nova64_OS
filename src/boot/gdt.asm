; gdt.asm
gdt_start:
    dq 0x0                         ; Descritor Nulo (Obrigatório)

gdt_code:                          ; Seletor 0x08
    ; Flags explicadas:
    ; Bit 44: 1 (Descriptor type - Code/Data)
    ; Bit 47: 1 (Present)
    ; Bit 53: 1 (L - Long Mode - Ativa 64 bits)
    ; Bit 54: 0 (D - Default size - Deve ser 0 em 64 bits)
    dq 0x00209A0000000000 

gdt_data:                          ; Seletor 0x10
    dq 0x0000920000000000 

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1     ; Tamanho da GDT
    dq gdt_start                   ; Endereço da GDT

CODE_SEG equ gdt_code - gdt_start  ; Resulta em 0x08
DATA_SEG equ gdt_data - gdt_start  ; Resulta em 0x10