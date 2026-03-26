#include "io.h"
#include "utils/types.h"


/* -------- Estruturas da IDT -------- */

struct idt_entry {
    uint16_t base_low;
    uint16_t sel;
    uint8_t  ist;
    uint8_t  flags;
    uint16_t base_mid;
    uint32_t base_high;
    uint32_t reserved;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct idt_entry idt[256] __attribute__((aligned(16)));
static struct idt_ptr ptr;

/* -------- Stubs ASM -------- */

extern void idt_load_raw(uint64_t);
extern void irq0_stub();
extern void irq1_stub();

/* -------- Função utilitária -------- */

static void idt_set_gate(
    uint8_t num,
    uint64_t base,
    uint16_t sel,
    uint8_t flags
) {
    idt[num].base_low  = base & 0xFFFF;
    idt[num].sel       = sel;
    idt[num].ist       = 0;        // sem IST por enquanto
    idt[num].flags     = flags;
    idt[num].base_mid  = (base >> 16) & 0xFFFF;
    idt[num].base_high = (base >> 32) & 0xFFFFFFFF;
    idt[num].reserved  = 0;
}

/* -------- Inicialização da IDT -------- */

void idt_init()
{
    /* -------- Remap do PIC -------- */
    outb(0x20, 0x11);
    outb(0xA0, 0x11);

    outb(0x21, 0x20);   // IRQ0 -> INT 32
    outb(0xA1, 0x28);   // IRQ8 -> INT 40

    outb(0x21, 0x04);
    outb(0xA1, 0x02);

    outb(0x21, 0x01);
    outb(0xA1, 0x01);

    
    outb(0x21, 0xFC);   // libera IRQ0 e IRQ1
    outb(0xA1, 0xFF);


    /* -------- Zera IDT inteira -------- */
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }

    
    idt_set_gate(32, (uint64_t)irq0_stub, 0x08, 0x8E);
    idt_set_gate(33, (uint64_t)irq1_stub, 0x08, 0x8E);


    /* -------- Carrega IDT -------- */
    ptr.limit = sizeof(idt) - 1;
    ptr.base  = (uint64_t)&idt;

    idt_load_raw((uint64_t)&ptr);

    // sti NÃO é aqui
}
