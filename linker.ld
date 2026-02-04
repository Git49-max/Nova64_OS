ENTRY(main)

SECTIONS
{
    
    . = 0x8000;

    .text : AT(0x8000)
    {
        *(.text.entry)     
        *(.text)
    }

    .rodata : ALIGN(4K) { *(.rodata) }
    .data   : ALIGN(4K) { *(.data) }
    .bss    : ALIGN(4K) { *(.bss) *(COMMON) }

    /DISCARD/ :
    {
        *(.note.gnu.property)
        *(.comment)
        *(.eh_frame)
    }
}