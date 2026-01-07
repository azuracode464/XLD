#pragma once
#include <stdint.h>

/* Estrutura de uma entrada na IDT (x86_64) */
struct idt_entry {
    uint16_t offset_low;    // Offset bits 0..15
    uint16_t selector;      // Code segment selector in GDT
    uint8_t  ist;           // Bits 0..2 hold Interrupt Stack Table offset, rest of bits zero.
    uint8_t  type_attr;     // Gate type, dpl, and p fields
    uint16_t offset_middle; // Offset bits 16..31
    uint32_t offset_high;   // Offset bits 32..63
    uint32_t reserved;      // Reservado (deve ser 0)
} __attribute__((packed));

/* Estrutura para carregar a IDT (LIDT) */
struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

/* Inicializa a IDT */
void idt_init(void);

/*
 * Define uma entrada na IDT.
 * num:      Número do vetor (0-255)
 * base:     Endereço da função handler (Assembly)
 * selector: Segmento de código (geralmente 0x08)
 * flags:    Flags de tipo e privilégio (ex: 0x8E para Kernel, 0xEE para User)
 */
void idt_set_gate(int num, uint64_t base, uint16_t selector, uint8_t flags);

