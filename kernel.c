/* Basic kernel for Nova64 OS. Originally wrote by Saulo Henrique in Thursday, January 22nd, 2026.
Last update: Saturday, January 24th, 2026, at 10:41 GMT-3 (Horário de Brasília)

kernel.c */

#include "videodriver.h"
#include "rtcdriver.h"
#include "kbdriver.h"
#include "io.h"

void main() {
    clear_screen();
    
    while (inb(0x64) & 0x01) { inb(0x60); }
    keyboard_init();
    while (inb(0x64) & 0x01) { inb(0x60); }

    int h, m, s;
    int cursor_x = 0;
    int cursor_y = 2;
    char last_key = 0;

    print("Nova64 is here!", 0x02, 0, 0);
    putc('>', 0x02, 0, 1);
    print(" System ready to further development", 0x02, 1, 1);

    while(1) {
        char key = get_key();
        
        if (key != 0 && key != last_key) {
            putc(key, 0x02, cursor_x, cursor_y);
            cursor_x++;
            last_key = key;
            
            if (cursor_x > 79) {
                cursor_x = 0;
                cursor_y++;
            }
        } else if (key == 0) {
            last_key = 0;
        }

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
