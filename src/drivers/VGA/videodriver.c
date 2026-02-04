#include "types.h"

#define vidAdr 0xB8000

int cursor_x = 0;
int cursor_y = 2;

void itoa(int n, char *str) {
    int i = 0;
    if (n == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    while (n > 0) {
        str[i++] = (n % 10) + '0';
        n = n / 10;
    }
    str[i] = '\0';

    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

void putc(char letter, unsigned char color, int x, int y) {
    volatile uint8_t *vram = (uint8_t*) (uint64_t) vidAdr;
    uint64_t offset = (y * 80 + x) * 2;
    vram[offset] = letter;
    vram[offset + 1] = color;
}

void print(char *str, unsigned char color, int x, int y) {
    int i = 0;
    while (str[i] != '\0') {
        int cur_x = x + i;
        int cur_y = y;
        if (cur_x >= 80) {
            cur_x = cur_x % 80;
            cur_y++;
        }
        putc(str[i], color, cur_x, cur_y);
        i++;
    }
}

void print_int(int n, unsigned char color, int x, int y) {
    char buffer[20];
    itoa(n, buffer);
    print(buffer, color, x, y);
}

void clear_screen() {
    volatile uint8_t *vram = (uint8_t*) (uint64_t) vidAdr;
    for (uint64_t i = 0; i < 80 * 25 * 2; i += 2) {
        vram[i] = ' ';      
        vram[i+1] = 0x07;   
    }
}

void draw_cursor(int x, int y, int show) {
    if (show) {
        putc('|', 0x0F, x, y);
    } else {
        putc(' ', 0x0F, x, y);
    }
}