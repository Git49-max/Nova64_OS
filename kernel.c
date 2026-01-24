/* Basic kernel for Nova64 OS. Originally wrote by Saulo Henrique in Thursday, January 22nd, 2026.
Last update: Saturday, January 24th, 2026, at 11:00 GMT-3 (Horário de Brasília)

kernel.c */

#include "videodriver.h"
#include "rtcdriver.h"
#include "kbdriver.h"
#include "io.h"
void main() {
    clear_screen();
    
    while (inb(0x64) & 0x01) { inb(0x60); }
    keyboard_init();

    print("Nova64 OS - Keyboard Diagnostic", 0x02, 0, 0);

    while(1) {
        if (inb(0x64) & 0x01) {
            unsigned char scancode = inb(0x60);
            
            print("Raw Code: ", 0x0F, 0, 3);
            print_int(scancode, 0x0E, 10, 3);

            if (!(scancode & 0x80)) {
                print("Mapped Char: ", 0x0F, 0, 5);
                if (scancode < 128) {
                    putc(keyboard_map[scancode], 0x0A, 13, 5);
                }
            }
        }
    }
}
