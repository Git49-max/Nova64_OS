/* Real Time Clock driver for Nova64 OS. Originally wrote by Saulo Henrique in Friday, January 23rd, 2026.

Last update: Sunday, January 25th, 2026, at 18:36 GMT-3 (Horário de Brasília)

rtcdriver.c*/

#include "io.h"

extern int current_tz_offset; 

unsigned char get_rtc_register(int reg) {
    outb(0x70, reg);
    return inb(0x71);
}

int is_update_in_progress() {
    outb(0x70, 0x0A);
    return (inb(0x71) & 0x80);
}

void get_time(int *h, int *m, int *s) {
    while (is_update_in_progress());

    unsigned char rs = get_rtc_register(0x00);
    unsigned char rm = get_rtc_register(0x02);
    unsigned char rh = get_rtc_register(0x04);

    *s = (rs & 0x0F) + ((rs / 16) * 10);
    *m = (rm & 0x0F) + ((rm / 16) * 10);
    
    int raw_h = (rh & 0x0F) + ((rh / 16) * 10);

    int adjusted_h = raw_h + current_tz_offset;

    if (adjusted_h >= 24) {
        adjusted_h -= 24;
    } else if (adjusted_h < 0) {
        adjusted_h += 24;
    }

    *h = adjusted_h;
}
