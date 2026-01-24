/* Keyboard driver. Originally wrote by Saulo Henrique in Friday, January 23rd, 2026.

Last update: Saturday, January 24th, 2026, at 11:51 GMT-3 (Horário de Brasília)

kbdriver.c*/



#include "io.h"

unsigned char keyboard_map[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
  '9', '0', '-', '=', '\b',	/* Backspace */
  '\t',			/* Tab */
  'q', 'w', 'e', 'r',	/* 19 */
  't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* Enter */
    0,			/* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 39 */
 '\'', '`',   0,		/* Left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n',			/* 49 */
  'm', ',', '.', '/',   0,				/* Right shift */
  '*',
    0,	/* Alt */
  ' ',	/* Space bar */
    0,	/* Caps lock */
    0,	/* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,	/* < ... F10 */
    0,	/* 69 - Num lock*/
    0,	/* Scroll Lock */
    0,	/* Home key */
    0,	/* Up Arrow */
    0,	/* Page Up */
  '-',
    0,	/* Left Arrow */
    0,
    0,	/* Right Arrow */
  '+',
    0,	/* 79 - End key*/
    0,	/* Down Arrow */
    0,	/* Page Down */
    0,	/* Insert Key */
    0,	/* Delete Key */
    0,   0,   0,
    0,	/* F11 Key */
    0,	/* F12 Key */
    0,	/* All other keys are undefined */
};

char get_key(){
  unsigned char status = inb(0x64);
  if(status & 0x01){
     unsigned char scancode = inb(0x60);
      if(scancode < 128){
          return keyboard_map[scancode];
    }
      return inb(0x60);
  }
return 0;
}
void keyboard_init() {
    while (inb(0x64) & 0x02); 
    outb(0x60, 0xF4);
    
    while (inb(0x64) & 0x02);
    outb(0x64, 0x20);
    while (!(inb(0x64) & 0x01));
    unsigned char cb = inb(0x60);
    cb |= 0x40;
    while (inb(0x64) & 0x02);
    outb(0x64, 0x60);
    while (inb(0x64) & 0x02);
    outb(0x60, cb);
}    0,
    '+',
    0,
    0,
    0,
    0,
    0,
    0,   0,   0,
    0,
    0,
    0,
};

void keyboard_init() {
    while (inb(0x64) & 0x02);
    outb(0x64, 0xA7);

    while (inb(0x64) & 0x02);
    outb(0x64, 0xAE);

    while (inb(0x64) & 0x02);
    outb(0x60, 0xF4);
    while (!(inb(0x64) & 0x01));
    inb(0x60);

    while (inb(0x64) & 0x02);
    outb(0x64, 0x60);
    while (inb(0x64) & 0x02);
    outb(0x60, 0x41);
}
