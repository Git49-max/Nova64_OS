#include "VGA/videodriver.h"
#include "RTC/rtcdriver.h"
#include "keyboard/kbdriver.h"
#include "shell/shell.h"
#include "io.h"
#include "utils/string.h"

extern void idt_init();
extern void print_formatted_time(int hh, int mm, int ss, int x, int y);

String user;
String time_zone;

void main() {
    clear_screen();
    user = string_create("UNDEFINED USER");
    time_zone = string_create("GMT +0");
    int h, m, s;

    keyboard_init();
    idt_init();

    print("Nova64 OS", 0x0B, 0, 0);
    print("> [1] Start Configuration", 0x0F, 0, 1);
    putc('>', 0x02, 0, 2);
    cursor_x = 2;
    cursor_y = 2;

    while(1) {
        get_time(&h, &m, &s);
        print_formatted_time(h, m, s, 71, 24);
        
        __asm__("hlt");
    }
}