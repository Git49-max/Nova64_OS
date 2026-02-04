#include "VGA/videodriver.h"
#include "timer/pit.h"


void welcome_animation() {
    char *msg = "Hello!";
    for (int i = 0; i < 6; i++) {    
        print(msg, 0x09 + i, 37, 10);
        sleep(200);
    }
    clear_screen();
    putc('H', 0x09, 37, 10);
    sleep(200);
    putc('e', 0x0A, 38, 10);
    sleep(200);
    putc('l', 0x0B, 39, 10);
    sleep(200);
    putc('l', 0x0C, 40, 10);
    sleep(200);
    putc('o', 0x0D, 41, 10);
    sleep(200);
    putc('!', 0x0E, 42, 10);
    sleep(200);
}