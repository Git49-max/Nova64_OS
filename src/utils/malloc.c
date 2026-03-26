#include "utils/malloc.h"
#include "utils/pmm.h"

#define HEAP_START 0x1000000 
uint64_t heap_current = HEAP_START;
uint64_t heap_mapped_limit = HEAP_START; // Até onde já avisamos o PMM

void* kmalloc(uint64_t size) {
    // 1. Alinhamento de 8 bytes
    size = (size + 7) & ~7;

    void* ptr = (void*)heap_current;
    heap_current += size;

    while (heap_current > heap_mapped_limit) {
        pmm_alloc_page(); // Isso diminui o free_pages do Neofetch!
        heap_mapped_limit += 4096; // Avança de 4KB em 4KB
    }

    return ptr;
}