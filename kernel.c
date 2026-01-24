/* Basic kernel for Nova64 OS. Originally wrote by Saulo Henrique in Thursday, January 22nd, 2026.
Last update: Saturday, January 24th, 2026, at 12:12 GMT-3 (Horário de Brasília)

kernel.c */
#include "videodriver.h"
#include "rtcdriver.h"
#include "kbdriver.h"
#include "io.h"

extern void idt_init();

void main() {
    clear_screen();
    int h, m, s;

    while (inb(0x64) & 0x01) { inb(0x60); }
    
    idt_init();
    keyboard_init();

    print("Nova64 OS - IDT Keyboard Active", 0x02, 0, 0);
    putc('>', 0x02, 0, 1);

    while(1) {
        get_time(&h, &m, &s);
        
        if (h < 10) {
            putc('0', 0x02, 71, 24);
            print_int(h, 0x02, 72, 24);
        } else {
            print_int(h, 0x02, 71, 24);
        }

        putc(':', 0x02, 73, 24);

        if (m < 10) {
            putc('0', 0x02, 74, 24);
            print_int(m, 0x02, 75, 24);
        } else {
            print_int(m, 0x02, 74, 24);
        }

        putc(':', 0x02, 76, 24);

        if (s < 10) {
            putc('0', 0x02, 77, 24);
            print_int(s, 0x02, 78, 24);
        } else {
            print_int(s, 0x02, 77, 24);
        }
    }
}

