#include "gdt.h"
#include "string.h"
#include "klog.h"

/* Estruturas internas */
struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

struct tss_entry_struct {
    uint32_t reserved1;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved2;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved3;
    uint16_t reserved4;
    uint16_t iomap_base;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

// GDT com 7 entradas e TSS
struct gdt_entry gdt[7];
struct gdt_ptr gp;
struct tss_entry_struct tss;

// Função auxiliar interna
static void gdt_set_gate(int num, uint64_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;

    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[num].access = access;
}

void tss_set_rsp0(uint64_t rsp0) {
    tss.rsp0 = rsp0;
}

void gdt_init(void) {
    // 1. Zerar tudo para evitar lixo
    memset(&gdt, 0, sizeof(gdt));
    memset(&tss, 0, sizeof(tss));

    gp.limit = (sizeof(struct gdt_entry) * 7) - 1;
    gp.base = (uint64_t)&gdt;

    // 2. Configurar TSS
    tss.rsp0 = 0; // Será definido pelo process.c depois
    tss.iomap_base = sizeof(tss); // Desabilita IO Map

    // 3. Configurar GDT
    
    // 0: Null
    gdt_set_gate(0, 0, 0, 0, 0);

    // 1: Kernel Code (0x08)
    gdt_set_gate(1, 0, 0, 0x9A, 0xA0); // Present, Ring0, Code, Read

    // 2: Kernel Data (0x10)
    gdt_set_gate(2, 0, 0, 0x92, 0xA0); // Present, Ring0, Data, Write

    // 3: User Data (0x18) -> RPL 3 = 0x1B
    gdt_set_gate(3, 0, 0, 0xF2, 0xA0); // Present, Ring3, Data, Write

    // 4: User Code (0x20) -> RPL 3 = 0x23
    gdt_set_gate(4, 0, 0, 0xFA, 0xA0); // Present, Ring3, Code, Read

    // 5 & 6: TSS (System Segment) - 16 BYTES em 64-bit!
    uint64_t tss_base = (uint64_t)&tss;
    uint32_t tss_limit = sizeof(tss) - 1;

    // Parte Baixa (Indice 5)
    // Access byte 0x89 = Present(1), DPL(0), Type(9=Available TSS 64-bit)
    gdt_set_gate(5, tss_base, tss_limit, 0x89, 0x00);

    // Parte Alta (Indice 6) - Estrutura diferente
    // A função gdt_set_gate não serve para a parte alta do TSS, fazemos manual:
    gdt[6].limit_low = (uint16_t)((tss_base >> 32) & 0xFFFF);
    gdt[6].base_low =  (uint16_t)((tss_base >> 48) & 0xFFFF);
    gdt[6].base_middle = 0;
    gdt[6].access = 0; // Deve ser 0
    gdt[6].granularity = 0; // Deve ser 0
    gdt[6].base_high = (uint8_t)((tss_base >> 56) & 0xFF);

    // 4. Carrega GDT
    asm volatile("lgdt %0" : : "m"(gp));

    // 5. Recarrega Segmentos
    asm volatile(
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        "pushq $0x08\n"
        "lea 1f(%%rip), %%rax\n"
        "pushq %%rax\n"
        ".byte 0x48, 0xcb\n" // retfq
        "1:\n"
        : : : "rax", "memory"
    );

    // 6. Carrega TSS (LTR)
    // O índice é 5. O seletor é 5 * 8 = 0x28 (40 decimal)
    asm volatile("mov $0x28, %%ax; ltr %%ax" : : : "rax");
    
    klog(0, "GDT: Loaded. TSS at 0x%llx (Size: %d)", tss_base, sizeof(tss));
}

