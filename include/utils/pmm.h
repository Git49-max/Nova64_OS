#ifndef PMM_H
#define PMM_H

#include "types.h"

extern uint64_t free_pages;

#define PAGE_SIZE 4096
#define BITMAP_SIZE (32768 / 8) // Para 128MB de RAM (32768 páginas)

void pmm_init(uint64_t kernel_end);
void* pmm_alloc_page();
void pmm_free_page(void* ptr);

#endif