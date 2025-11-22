#ifndef GDT_H
#define GDT_H

#include <stdint.h>

// Estrutura de uma entrada na GDT
typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  limit_high_flags;
    uint8_t  base_high;
} __attribute__((packed)) gdt_entry_t;

// Estrutura de uma entrada de sistema (para a TSS)
typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  type_attr;
    uint8_t  limit_high_flags;
    uint8_t  base_high;
    uint32_t base_upper;
    uint32_t reserved;
} __attribute__((packed)) gdt_system_entry_t;

// Estrutura da Task State Segment (TSS) para x86-64
typedef struct {
    uint32_t reserved0;
    uint64_t rsp0;       // A ESTRELA DO SHOW: Ponteiro da pilha para Ring 0
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist[7];
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopb_offset;
} __attribute__((packed)) tss_t;

// Função de inicialização
void gdt_init(void);

#endif // GDT_H

