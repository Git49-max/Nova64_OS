/* Keyboard driver. Originally wrote by Saulo Henrique in Friday, January 23rd, 2026.

Last update: Saturday, January 24th, 2026, at 14:06 GMT-3 (Horário de Brasília)

kbdriver.c*/

#include "io.h"
#include "videodriver.h"

int cursor_x = 0;
int cursor_y = 2;

unsigned char keyboard_map[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',
    '9', '0', '-', '=', '\b', '\t', 'q', 'w', 'e', 'r',
    't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
    '\'', '`', 0, '\\', 'z', 'x', 'c', 'v', 'b', 'n',
    'm', ',', '.', '/', 0, '*', 0, ' ', 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '-', 0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

void keyboard_handler() {
    unsigned char scancode = inb(0x60);
    if (!(scancode & 0x80)) {
        if (scancode < 128) {
            char key = keyboard_map[scancode];
            if (key != 0) {
                putc(key, 0x02, cursor_x, cursor_y);
                cursor_x++;
                if (cursor_x > 79) { cursor_x = 0; cursor_y++; }
            }
        }
    }
    outb(0x20, 0x20);
}



