/* I/O file. Originally wrote by Saulo Henrique in Friday, January 23rd, 2026.

Last update: Friday, January 23rd, 2026, at 16:54 GMT-3 (Horário de Brasília)

io.c*/

void outb(unsigned short port, unsigned char val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}
unsigned char inb(unsigned short port) {
    unsigned char ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
