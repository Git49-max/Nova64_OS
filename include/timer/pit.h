#ifndef PIT_H
#define PIT_H
#include "types.h"

void init_timer(uint32_t frequency);
void sleep(uint32_t ms);
#endif