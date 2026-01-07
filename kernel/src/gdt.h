#ifndef GDT_H
#define GDT_H

#include <stdint.h>

// Estrutura da TSS em x86_64 (Hardware)
// Essencial para o processador saber onde está a pilha do Kernel (RSP0)
// quando ocorre uma interrupção vinda do Ring 3.
struct tss_entry {
    uint32_t reserved1;
    uint64_t rsp0;       // Stack Pointer para Ring 0 (CRITICO)
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved2;
    uint64_t ist[7];     // Interrupt Stack Table
    uint64_t reserved3;
    uint16_t reserved4;
    uint16_t iomap_base;
} __attribute__((packed));

void gdt_init(void);

// Função chamada pelo scheduler para atualizar a pilha do kernel
void tss_set_stack(uint64_t stack); 
void tss_set_rsp0(uint64_t rsp0);
#endif

