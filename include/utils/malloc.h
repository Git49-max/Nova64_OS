#ifndef MALLOC_H
#define MALLOC_H

#include "types.h"

void  kheap_init();
void* kmalloc(uint64_t size);
void  kfree(void* ptr);

#endif