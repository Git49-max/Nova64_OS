/*Video driver header for Nova64 OS (Text mode). Originally wrote by Saulo Henrique in Thursday, January 22nd, 2026.

Last update: Friday, January 23rd, 2026, at 18:59 GMT-3 (Horário de Brasília)

videodriver.h*/


#ifndef VIDEODRIVER_H
#define VIDEODRIVER_H

extern int cursor_x;
extern int cursor_y;

void putc(char letter, unsigned char color, int x, int y);
void print(char *str, unsigned char color, int x, int y);
void print_int(int n, unsigned char color, int x, int y);
void clear_screen();
#endif
