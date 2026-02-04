#include "types.h"

#define PAGE_PRESENT    (1 << 0)
#define PAGE_WRITE      (1 << 1)
#define PAGE_USER       (1 << 2)

typedef uint64_t pml4_entry_t;
typedef uint64_t pdpt_entry_t;
typedef uint64_t pd_entry_t;
typedef uint64_t pt_entry_t;

pml4_entry_t pml4[512] __attribute__((aligned(4096)));
pdpt_entry_t pdpt[512] __attribute__((aligned(4096)));
pd_entry_t   pd[512]   __attribute__((aligned(4096)));
pt_entry_t   pt[512]   __attribute__((aligned(4096)));