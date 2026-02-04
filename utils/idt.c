#include "io.h"
#include "utils/types.h"

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

// Alocamos a IDT em 1MB
struct idt_entry *idt = (struct idt_entry *)0x100000;

extern void idt_load_raw(uint64_t addr);
extern void irq0();
extern void irq1();
extern void irq_common_stub();

void irq0_handler() {
    // Chame sua lógica de timer aqui
    outb(0x20, 0x20); 
}

void irq1_handler() {
    // Chame sua lógica de teclado aqui
    outb(0x20, 0x20);
}

void common_irq_handler() {
    outb(0x20, 0x20);
    outb(0xA0, 0x20);
}

void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low = (uint16_t)(base & 0xFFFF);
    idt[num].sel = sel;
    idt[num].ist = 0;
    idt[num].flags = flags;
    idt[num].base_mid = (uint16_t)((base >> 16) & 0xFFFF);
    idt[num].base_high = (uint32_t)((base >> 32) & 0xFFFFFFFF);
    idt[num].reserved = 0;
}

void idt_init() {
    // 1. Remapeia o PIC (Obrigatório)
    outb(0x20, 0x11); outb(0xA0, 0x11);
    outb(0x21, 0x20); outb(0xA1, 0x28);
    outb(0x21, 0x04); outb(0xA1, 0x02);
    outb(0x21, 0x01); outb(0xA1, 0x01);

    // 2. Limpa a IDT com o stub comum
    for(int i = 0; i < 256; i++) {
        idt_set_gate(i, (uint64_t)irq_common_stub, 0x08, 0x8E);
    }

    // 3. Portas específicas
    idt_set_gate(32, (uint64_t)irq0, 0x08, 0x8E);
    idt_set_gate(33, (uint64_t)irq1, 0x08, 0x8E);

    // 4. A ESTRUTURA LOCAL (Isso corrige o IDT Limit=0)
    // Usamos static para garantir que ela não suma da pilha antes da hora
    static struct idt_ptr ptr; 
    ptr.limit = (uint16_t)(sizeof(struct idt_entry) * 256) - 1;
    ptr.base = (uint64_t)0x100000;

    // 5. Passa o endereço da estrutura para o ASM
    idt_load_raw((uint64_t)&ptr);

    // 6. Habilita interrupções no PIC
    outb(0x21, 0xFC); 
    outb(0xA1, 0xFF);

    __asm__ volatile("sti");
}