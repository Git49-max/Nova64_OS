/* Basic kernel for Nova64 OS. Originally wrote by Saulo Henrique in Thursday, January 22nd, 2026.
Last update: Saturday, January 24th, 2026, at 11:27 GMT-3 (Horário de Brasília)

kernel.c */

#include "videodriver.h"
#include "kbdriver.h"
#include "io.h"

void main() {
    clear_screen();
    
    while (inb(0x64) & 0x01) { inb(0x60); }
    keyboard_init();

    print("Nova64 OS - Diagnostic Mode", 0x02, 0, 0);

    while(1) {
        if (inb(0x64) & 0x01) {
            unsigned char sc = inb(0x60);
            
            print("Raw: ", 0x0F, 0, 3);
            print_int(sc, 0x0E, 5, 3);

            if (!(sc & 0x80) && sc < 128) {
                char c = keyboard_map[sc];
                print("Char: ", 0x0F, 0, 5);
                putc(c, 0x0A, 6, 5);
            }
        }
    }
}
