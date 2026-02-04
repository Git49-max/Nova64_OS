#include "VGA/videodriver.h"
#include "RTC/rtcdriver.h"
#include "utils/string.h"

int config_status = 1; 

typedef struct {
    char name[10];
    int offset;
} TimeZone;

TimeZone tz_table[] = {
    {"GMT-12", -12}, {"GMT-11", -11}, {"GMT-10", -10}, {"GMT-9", -9},
    {"GMT-8", -8},   {"GMT-7", -7},   {"GMT-6", -6},   {"GMT-5", -5},
    {"GMT-4", -4},   {"GMT-3", -3},   {"GMT-2", -2},   {"GMT-1", -1},
    {"GMT+0", 0},    {"GMT+1", 1},    {"GMT+2", 2},    {"GMT+3", 3},
    {"GMT+4", 4},    {"GMT+5", 5},    {"GMT+6", 6},    {"GMT+7", 7},
    {"GMT+8", 8},    {"GMT+9", 9},    {"GMT+10", 10},  {"GMT+11", 11},
    {"GMT+12", 12}
};

int current_tz_offset = 0;
extern String time_zone;
extern String user;
extern String key_buffer;

void update_tz_by_name(char* input_name) {
    for (int i = 0; i < 25; i++) {
        if (strcmp(input_name, tz_table[i].name) == 0) {
            current_tz_offset = tz_table[i].offset;
            // Atualiza a string global do fuso para o neofetch/shell
            strcpy(time_zone.data, tz_table[i].name);
            time_zone.length = strlen(tz_table[i].name);
            
            config_status = 3;
            return;
        }
    }
}

void update_user(char* input_name){
    strcpy(user.data, input_name);
    user.length = strlen(input_name);
    config_status = 2;
}

void start_config(){
    clear_screen();
    print("Nova64 Initial Setup", 0x0B, 30, 0);
    
    if (config_status == 1) {
        print("Choose your username: ", 0x09, 0, 1);
        cursor_x = 22;
        cursor_y = 1;
    } 
    else if (config_status == 2) {
        print("Choose your time zone (GMT FORMAT!): ", 0x09, 0, 1);
        cursor_x = 38;
        cursor_y = 1;
    } 
    else if (config_status == 3) {
        print("Setup Complete!", 0x0A, 0, 1);
        print("User: ", 0x0B, 0, 2);
        print(user.data, 0x07, 6, 2);
        print("TZ:   ", 0x0B, 0, 3);
        print(time_zone.data, 0x07, 6, 3);
        
        print("To exit, type 'exit': ", 0x0E, 0, 5);
        cursor_x = 22;
        cursor_y = 5;
    }
}

