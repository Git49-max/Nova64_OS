/* Basic kernel for Nova64 OS. Originally wrote by Saulo Henrique in Thursday, January 22nd, 2026.
Last update: Friday, January 23rd, 2026, at 22:45 GMT-3 (Horário de Brasília)

kernel.c */

#include "videodriver.h"
#include "rtcdriver.h"
#include "kbdriver.h"

void main() {
    clear_screen();
    int h, m, s;
    int cursor_x = 0;
    int cursor_y = 2;

    print("Nova64 is here!", 0x02, 0, 0);
    putc('>', 0x02, 0, 1);
    print(" System ready to further development", 0x02, 1, 1);

    while(1) {
        char tecla = get_key();
        
        if (tecla != 0) {
            putc(tecla, 0x02, cursor_x, cursor_y);
            cursor_x++;
            
            if (cursor_x > 79) {
                cursor_x = 0;
                cursor_y++;
            }
        }

        get_time(&h, &m, &s);

        if (h < 10) {
            putc('0', 0x02, 71, 0);
            print_int(h, 0x02, 72, 0);
        } else {
            print_int(h, 0x02, 71, 0);
        }

        putc(':', 0x02, 73, 0);

        if (m < 10) {
            putc('0', 0x02, 74, 0);
            print_int(m, 0x02, 75, 0);
        } else {
            print_int(m, 0x02, 74, 0);
        }

        putc(':', 0x02, 76, 0);

        if (s < 10) {
            putc('0', 0x02, 77, 0);
            print_int(s, 0x02, 78, 0);
        } else {
            print_int(s, 0x02, 77, 0);
        }
    }
}
