/*Video driver header for Nova64 OS (Text mode). Originally wrote by Saulo Henrique in Thursday, January 22nd, 2026.

Last update: Thursday, January 22nd, 2026, at 21:07 GMT-3 (Horário de Brasília)

videodriver.h*/


#ifndef VIDEODRIVER_H
#define VIDEODRIVER_H
void putc(char letter, unsigned char color, int x, int y);
void print(char *str, unsigned char color, int x, int y);
void clear_screen();
#endif
