#include "VGA/videodriver.h"
#include "RTC/rtcdriver.h"
#include "keyboard/kbdriver.h"
#include "shell/shell.h"
#include "io.h"
#include "utils/string.h"
#include "utils/types.h"
#include "timer/pit.h"
#include "animations/animations.h"
#include "stellar/stellar.h"
#include "utils/pmm.h"
#include "fs/fat32.h"

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

uint64_t stellar_stack[8192];
void fpu_init();

__attribute__((section(".text.entry")))
int main() {
    
    shell_active = 0;   
    config_active = 0;  
    cursor_x = 0;
    cursor_y = 0;
    
    clear_screen();
    extern uint64_t _kernel_end;
    pmm_init((uint64_t)&_kernel_end);
    fat32_init();
    fpu_init();
    
    int h, m, s;

    
    idt_init();
    init_timer(100);
    __asm__ volatile ("sti");

    user = string_create("UNDEFINED USER");
    time_zone = string_create("GMT +0");

    // ----------------------------------

    welcome_animation();
    clear_screen();
    keyboard_init();

    print("Nova64 OS", 0x0B, 0, 0);
    print("> [1] Start Configuration", 0x0F, 0, 1);
    putc('>', 0x02, 0, 2);
    
    cursor_x = 2;
    cursor_y = 2;

    int cursor_visible = 0;
    uint32_t last_blink = 0;
    
    void* ptr = pmm_alloc_page();

    while(1) {
        get_time(&h, &m, &s);
        print_formatted_time(h, m, s, 72, 24);

        if (timer_ticks - last_blink >= 50) {
            cursor_visible = !cursor_visible;
            draw_cursor(cursor_x, cursor_y, cursor_visible);
            last_blink = timer_ticks;
        }

        __asm__ volatile("hlt");
    }

    return 0;
}
void fpu_init() {
    uint64_t cr0; // Usando o uint64_t que já está no seu types.h
    asm volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1 << 2); // Limpa o bit EM (Emulação)
    cr0 |= (1 << 1);  // Define o bit MP (Monitor Coprocessador)
    asm volatile ("mov %0, %%cr0" : : "r"(cr0));

    uint64_t cr4;
    asm volatile ("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (3 << 9);  // Define OSFXSR e OSXMMEXCPT (Ativa SSE/FPU)
    asm volatile ("mov %0, %%cr4" : : "r"(cr4));
}