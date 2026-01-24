/* Keyboard driver. Originally wrote by Saulo Henrique in Friday, January 23rd, 2026.

Last update: Saturday, January 24th, 2026, at 11:57 GMT-3 (Horário de Brasília)

kbdriver.c*/



#include "io.h"
#include "videodriver.h"

extern unsigned char keyboard_map[128];
int cursor_x = 0;
int cursor_y = 2;

void keyboard_handler() {
    unsigned char scancode = inb(0x60);
    
    if (!(scancode & 0x80)) {
        if (scancode < 128) {
            char key = keyboard_map[scancode];
            putc(key, 0x02, cursor_x, cursor_y);
            cursor_x++;
            if (cursor_x > 79) { cursor_x = 0; cursor_y++; }
        }
    }
    outb(0x20, 0x20);
}

void keyboard_init() {
    while (inb(0x64) & 0x02);
    outb(0x60, 0xF4);
}
