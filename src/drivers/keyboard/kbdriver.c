/* Keyboard driver. Originally wrote by Saulo Henrique in Friday, January 23rd, 2026.

Last update: Sunday, January 25th, 2026, at 15:38 GMT-3 (Horário de Brasília)

kbdriver.c*/

#include "io.h"
#include "VGA/videodriver.h"

extern int cursor_x;
extern int cursor_y;

int shift_pressed = 0;

unsigned char keyboard_map[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

unsigned char keyboard_map_shift[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~', 0, '|',
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' '
};

void keyboard_handler() {
    unsigned char scancode = inb(0x60);

    if (scancode & 0x80) {
        scancode &= ~0x80;
        if (scancode == 0x2A || scancode == 0x36) shift_pressed = 0;
        return;
    }

    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = 1;
        return;
    }

    if (scancode == 0x0E) {
        if (cursor_x > 1) { 
            cursor_x--;
            putc(' ', 0x02, cursor_x, cursor_y); 
        }
        return;
    }

    // Caracteres normais
    char c = shift_pressed ? keyboard_map_shift[scancode] : keyboard_map[scancode];
    
    if (c > 0 && c != '\b') {
        putc(c, 0x02, cursor_x, cursor_y);
        cursor_x++;
        if (cursor_x >= 80) {
            cursor_x = 0;
            cursor_y++;
        }
    }
}

void keyboard_init() {
    while (inb(0x64) & 0x01) inb(0x60);
}




