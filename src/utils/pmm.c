#include "utils/pmm.h"

uint8_t* bitmap;
uint64_t total_pages = 32768; // 128MB / 4KB
uint64_t free_pages = 0;      // <--- O Shell precisa desta variável!

void pmm_init(uint64_t kernel_end) {
    bitmap = (uint8_t*)kernel_end;
    free_pages = total_pages; // Começa com tudo livre

    // Zera o bitmap
    for (int i = 0; i < BITMAP_SIZE; i++) bitmap[i] = 0;

    // Reserva as páginas do Kernel (de 1MB até o fim do bitmap)
    uint64_t used_size = (uint64_t)kernel_end + BITMAP_SIZE - 0x100000;
    uint64_t used_pages = (used_size / PAGE_SIZE) + 1;

    for (uint64_t i = 0; i < used_pages; i++) {
        bitmap[i / 8] |= (1 << (i % 8));
        free_pages--; // <--- Diminui o contador para refletir no Shell
    }
}

void* pmm_alloc_page() {
    for (uint64_t i = 0; i < total_pages; i++) {
        if (!(bitmap[i / 8] & (1 << (i % 8)))) {
            bitmap[i / 8] |= (1 << (i % 8));
            free_pages--; // <--- Atualiza o contador global
            return (void*)(i * PAGE_SIZE);
        }
    }
    return 0;
}