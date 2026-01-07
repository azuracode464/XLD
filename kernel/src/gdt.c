#include <stdint.h>

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

struct gdt_entry gdt[3];
struct gdt_ptr gp;

static void gdt_set_gate(int num, uint64_t base, uint32_t limit, 
                         uint8_t access, uint8_t gran) {
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;
    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[num].access = access;
}

void gdt_init(void) {
    gp.limit = (sizeof(struct gdt_entry) * 3) - 1;
    gp.base = (uint64_t)&gdt;
    
    /* Null descriptor */
    gdt_set_gate(0, 0, 0, 0, 0);
    
    /* Code segment (64-bit): base=0, limit=0, 
       access=0x9A (present, ring0, code, readable), 
       granularity: G=1, L=1, D/B=0 -> 0xA0 */
    gdt_set_gate(1, 0, 0, 0x9A, 0xA0);
    
    /* Data segment: base=0, limit=0,
       access=0x92 (present, ring0, data, writable),
       granularity: G=1, L=0, D/B=0 -> 0xA0 (L is ignored for data) */
    gdt_set_gate(2, 0, 0, 0x92, 0xA0);
    
    /* Carrega GDT */
    asm volatile("lgdt %0" : : "m"(gp));
    
    /* Recarrega segmentos - maneira CORRETA para 64-bit */
    asm volatile(
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        "pushq $0x08\n"           /* Seletor de código (CS) - pushq para garantir 64-bit push */
        "lea 1f(%%rip), %%rax\n" /* Endereço de retorno */
        "pushq %%rax\n"
        ".byte 0x48, 0xcb\n"     /* RETFQ (REX.W + RETF) */
        "1:\n"
        : : : "rax", "memory");
}
