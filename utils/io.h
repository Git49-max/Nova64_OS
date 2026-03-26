/* I/O header. Originally wrote by Saulo Henrique in Friday, January 23rd, 2026.

Last update: Friday, January 23rd, 2026, at 16:52 GMT-3 (Horário de Brasília)

io.h*/

#ifndef IO_H
#define IO_H
void outb(unsigned short port, unsigned char val);
unsigned char inb(unsigned short port);
void outw(unsigned short port, unsigned short val);
unsigned short inw(unsigned short port);
void reboot();
void shutdown();
#endif
