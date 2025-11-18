// kernel/src/idt/idt.h

#ifndef IDT_H
#define IDT_H

#include <stdint.h>

// Estrutura de uma entrada na IDT (Interrupt Gate)
struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  types_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} __attribute__((packed));

// Estrutura do ponteiro da IDT (IDTR)
struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

// Função principal de inicialização
void idt_init(void);

#endif // IDT_H

