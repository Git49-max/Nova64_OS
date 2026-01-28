#include "io.h"
#include "VGA/videodriver.h"
#include "utils/string.h"
#include "shell/shell.h"
#include "utils/config.h"

extern int cursor_x;
extern int cursor_y;
extern int config_status;

int shift_pressed = 0;
int shell_active = 0;
int config_active = 0;
String key_buffer;

unsigned char keyboard_map[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

unsigned char keyboard_map_shift[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~', 0, '|',
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' '
};

void keyboard_handler() {
    unsigned char scancode = inb(0x60);

    if (scancode & 0x80) {
        scancode &= ~0x80;
        if (scancode == 0x2A || scancode == 0x36) shift_pressed = 0;
        return;
    }

    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = 1;
        return;
    }

    char c = shift_pressed ? keyboard_map_shift[scancode] : keyboard_map[scancode];

    if (c == '\n') {
        if (strcmp(key_buffer.data, "shell32.start") == 0 && config_active == 0) {
            shell_active = 1;
            bashinit();
            string_clear(&key_buffer);
            return;
        }

        if (strcmp(key_buffer.data, "1") == 0 && shell_active == 0 && config_active == 0){
            start_config();
            config_active = 1;
            string_clear(&key_buffer);
            return;
        }

        if (config_active == 1) {
            if (config_status == 1) {
                update_user(key_buffer.data);
                string_clear(&key_buffer);
                start_config();
                return;
            } 
            else if (config_status == 2) {
                update_tz_by_name(key_buffer.data);
                string_clear(&key_buffer);
                start_config();
                return;
            } 
            else if (config_status == 3) {
                if (strcmp(key_buffer.data, "exit") == 0) {
                    config_active = 0;
                    clear_screen();
                    print("Nova64 OS", 0x0B, 0, 0);
                    cursor_x = 0;
                    cursor_y = 1;
                    string_clear(&key_buffer);
                    putc('>', 0x0F, cursor_x, cursor_y);
                    cursor_x++;
                    return;
                }
            }
        }

        if (shell_active && config_active == 0) {
            exeCommand(key_buffer);
        } else if (config_active == 0) {
            cursor_y++;
            cursor_x = 0;
            print("Type 'shell32.start' to use the terminal.", 0x07, cursor_x, cursor_y);
        }

        string_clear(&key_buffer);
        cursor_x = 0;
        cursor_y++;
        putc('>', 0x02, cursor_x, cursor_y);
        cursor_x++;
    } 
    else if (c == '\b') {
        if (key_buffer.length > 0) {
            key_buffer.length--;
            key_buffer.data[key_buffer.length] = '\0';
            if (cursor_x > 0) {
                cursor_x--;
                putc(' ', 0x02, cursor_x, cursor_y);
            }
        }
    } 
    else if (c > 0) {
        string_append(&key_buffer, c);
        putc(c, 0x02, cursor_x, cursor_y);
        cursor_x++;
        if (cursor_x >= 80) {
            cursor_x = 0;
            cursor_y++;
        }
    }
}

void keyboard_init() {
    string_clear(&key_buffer);
    while (inb(0x64) & 0x01) inb(0x60);
}