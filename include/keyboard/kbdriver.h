/* Keyboard driver header. Originally wrote by Saulo Henrique in Friday, January 23rd, 2026.

Last update: Saturday, January 24th, 2026, at 10:38 GMT-3 (Horário de Brasília)

kbdriver.h*/

#ifndef KBDRIVER_H
#define KBDRIVER_H
#include "utils/string.h"

extern String key_buffer;

void keyboard_init();
char get_input();
void keyboard_handler();

#endif
