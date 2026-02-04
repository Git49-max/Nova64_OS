#include "VGA/videodriver.h"
#include "RTC/rtcdriver.h"
#include "keyboard/kbdriver.h"
#include "shell/shell.h"
#include "io.h"
#include "utils/string.h"
#include "utils/types.h"
#include "timer/pit.h"
#include "animations/animations.h"

// THE COLOR TABLE!
/*
Hex value,  Color
0,          Black
1,          Blue
2,          Green
3,          Cyan
4,          Red
5,          Magenta
6,          Brown
7,          Light Gray
8,          Dark Gray
9,          Light Blue
A,          Light Green
B,          Light Cyan
C,          Light Red
D,          Light Magenta
E,          Yellow
F,          White
*/

extern void idt_init();
extern void print_formatted_time(int hh, int mm, int ss, int x, int y);
extern int timer_ticks;
extern int key_pressed_now;
extern int shell_active;
extern int config_active;

String user;
String time_zone;

__attribute__((section(".text.entry")))
int main() {
    shell_active = 0;   
    config_active = 0;  
    cursor_x = 0;
    cursor_y = 0;
    
    clear_screen();
    
    user = string_create("UNDEFINED USER");
    time_zone = string_create("GMT +0");
    
    int h, m, s;

    keyboard_init();
    clear_screen(); // Vermelho brilhante
    print("TESTE DE COMPILACAO", 0x4F, 0, 0);
    
    idt_init();
    init_timer(100);

    welcome_animation();
    clear_screen();

    print("Nova64 OS", 0x0B, 0, 0);
    print("> [1] Start Configuration", 0x0F, 0, 1);
    putc('>', 0x02, 0, 2);
    
    cursor_x = 2;
    cursor_y = 2;

    int cursor_visible = 0;
    uint32_t last_blink = 0;

    while(1) {
        get_time(&h, &m, &s);
        print_formatted_time(h, m, s, 72, 0);

        if (timer_ticks - last_blink >= 50) {
            cursor_visible = !cursor_visible;
            draw_cursor(cursor_x, cursor_y, cursor_visible);
            last_blink = timer_ticks;
        }

        __asm__ volatile("hlt");
    }

    return 0;
}