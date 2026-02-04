#include "keyboard/kbdriver.h"
#include "VGA/videodriver.h"
#include "utils/string.h"
#include "RTC/rtcdriver.h"
#include "kernel/kernel.h"
#include "utils/io.h"

extern int cursor_x;
extern int cursor_y;
extern int shell_active;
extern int config_status;

int h, m, s;

void display_neofetch() {
    cursor_y++;
    print("  _   _                  ", 0x0B, 0, cursor_y++);
    print(" | \\ | |               ", 0x0B, 0, cursor_y++);
    print(" |  \\| | _____   ____  ", 0x0B, 0, cursor_y++);
    print(" | . ` |/ _ \\ \\ / / _`| ", 0x0B, 0, cursor_y++);
    print(" | |\\  | (_) \\ V / (_|| ", 0x0B, 0, cursor_y++);
    print(" |_| \\_|\\___/ \\_/ \\__, ", 0x0B, 0, cursor_y++);
    
    cursor_y++;
    
    print(" OS: ", 0x0B, 0, cursor_y); 
    print("Nova64 OS devV0.1 Alpha", 0x07, 5, cursor_y++);
    
    print(" KERNEL: ", 0x0B, 0, cursor_y);
    print("x86_32 Monolithic", 0x07, 9, cursor_y++);
    
    print(" SHELL: ", 0x0B, 0, cursor_y);
    print("NovaBash (shell32)", 0x07, 8, cursor_y++);
    
    print(" ARCH: ", 0x0B, 0, cursor_y);
    print("x86 (i386)", 0x07, 7, cursor_y++);

    print(" MEMORY: ", 0x0B, 0, cursor_y);
    print("640 KB (Base Memory)", 0x07, 9, cursor_y++);
}

void print_formatted_time(int hh, int mm, int ss, int x, int y) {
    if(hh < 10) { putc('0', 0xF0, x++, y); }
    print_int(hh, 0xF0, x, y);
    x += (hh < 10) ? 1 : 2;
    putc(':', 0xF0, x++, y);
    if(mm < 10) { putc('0', 0xF0, x++, y); }
    print_int(mm, 0xF0, x, y);
    x += (mm < 10) ? 1 : 2;
    putc(':', 0xF0, x++, y);
    if(ss < 10) { putc('0', 0xF0, x++, y); }
    print_int(ss, 0xF0, x, y);
}

void bashinit(){
    clear_screen();
    print("Nova64 Bash", 0x0B, 0, 0);
    cursor_y = 1;
    putc('>', 0x0F, 0, cursor_y);
    putc('$', 0x0F, 0, cursor_y);
    print(user.data, 0x0F, 2, cursor_y);
    putc('>', 0x0F, user.length + 2, cursor_y); 
    cursor_x = user.length + 3;
}

void exeCommand(String key_buffer){
    cursor_y++; 
    cursor_x = 0;

    if(strcmp(key_buffer.data, "time") == 0){
        get_time(&h, &m, &s);
        print(time_zone.data, 0x02, 0, cursor_y);
        print_formatted_time(h, m, s, 14, cursor_y);
    } 
    else if(strcmp(key_buffer.data, "cls") == 0){
        clear_screen();
        bashinit();
    }
    else if(strcmp(key_buffer.data, "exit") == 0){
        shell_active = 0;
        clear_screen();
        print("Nova64 OS", 0x0B, 0, 0);
        if(config_status != 3 ){
        print("> [1] Start Configuration", 0x0F, 0, 1);
        }
        putc('>', 0x02, 0, 2);
        cursor_x = 2;
        cursor_y = 2;
    } else if(strcmp(key_buffer.data, "neofetch") == 0){
        display_neofetch();
    } else if(strcmp(key_buffer.data, "power --restart") == 0){
        reboot();
    } else if(strcmp(key_buffer.data, "power --off") == 0){
        shutdown();
    }
    else {
        print("COMMAND NOT FOUND! err404", 0x0C, 0, cursor_y);
    }
}