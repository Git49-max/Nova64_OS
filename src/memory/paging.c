#include "paging.h"

void setup_paging64() {
    for(int i = 0; i < 512; i++) {
        pml4[i] = 0;
        pdpt[i] = 0;
        pd[i] = 0;
        pt[i] = 0;
    }

    pml4[0] = (uint64_t)pdpt | PAGE_PRESENT | PAGE_WRITE;
    pdpt[0] = (uint64_t)pd | PAGE_PRESENT | PAGE_WRITE;
    pd[0] = (uint64_t)pt | PAGE_PRESENT | PAGE_WRITE;

    for (uint64_t i = 0; i < 512; i++) {
        pt[i] = (i * 0x1000) | PAGE_PRESENT | PAGE_WRITE;
    }
}