#ifndef CONFIG_H
#define CONFIG_H

extern int config_status;

void start_config();
void update_tz_by_name(char* input_name);
void update_user(char* input_name);

#endif