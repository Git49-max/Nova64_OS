void outb(unsigned short port, unsigned char val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

unsigned char inb(unsigned short port) {
    unsigned char ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}


void outw(unsigned short port, unsigned short val) {
    asm volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

unsigned short inw(unsigned short port) {
    unsigned short ret;
    asm volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}


void reboot() {
    unsigned char good = 0x02;
    while (good & 0x02)
        good = inb(0x64);
    outb(0x64, 0xFE); 
}

void shutdown() {
    outw(0x604, 0x2000);
    outw(0x4004, 0x3400);
}