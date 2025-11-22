#include "gdt.h"
#include "../mem/pmm.h"
#include "../lib/string.h"
#include "../mem/mem.h"
// NÃO precisamos mais de vmm.h aqui
// #include "../mem/vmm.h" 

extern uint64_t hhdm_offset;

// Estrutura do ponteiro da GDT
struct gdt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

// Funções externas de gdt_asm.asm
extern void gdt_load(struct gdt_ptr *gdtp);
extern void tss_load(uint16_t selector);

// Estruturas da GDT e TSS
static struct gdt_structure {
    gdt_entry_t null;
    gdt_entry_t kernel_code;
    gdt_entry_t kernel_data;
    gdt_entry_t user_data;
    gdt_entry_t user_code;
    gdt_system_entry_t tss;
} __attribute__((packed)) gdt;

static tss_t tss;

void gdt_init(void) {
    // 1. Alocar a pilha para o RSP0 (pilha do kernel para transições)
    size_t stack_pages = 2; // 8KB é um tamanho seguro
    void* rsp0_stack_phys = pmm_alloc_pages(stack_pages);

    if (rsp0_stack_phys == NULL) {
        // Se não conseguirmos alocar a pilha, não há como continuar.
        // (Em um kernel real, aqui teríamos um kernel panic)
        for(;;);
    }

    // 2. Configurar a TSS
    memset(&tss, 0, sizeof(tss_t));

    // O HHDM do Limine garante que todo endereço físico `p` é acessível
    // no endereço virtual `p + hhdm_offset`.
    // Portanto, o topo da nossa pilha no espaço VIRTUAL é:
    uint64_t rsp0_top_virt = (uint64_t)rsp0_stack_phys + (stack_pages * PAGE_SIZE) + hhdm_offset;
    
    // A CPU usará este endereço virtual quando mudar para Ring 0.
    // Como o HHDM já mapeou a memória física subjacente, não ocorrerá Page Fault.
    tss.rsp0 = rsp0_top_virt;

    // 3. Preencher as entradas da GDT
    gdt.null = (gdt_entry_t){0, 0, 0, 0, 0, 0};
    gdt.kernel_code = (gdt_entry_t){ .access = 0b10011010, .limit_high_flags = 0b00100000 };
    gdt.kernel_data = (gdt_entry_t){ .access = 0b10010010 };
    gdt.user_data = (gdt_entry_t){ .access = 0b11110010 };
    gdt.user_code = (gdt_entry_t){ .access = 0b11111010, .limit_high_flags = 0b00100000 };
    
    uint64_t tss_base = (uint64_t)&tss;
    gdt.tss.limit_low = sizeof(tss_t);
    gdt.tss.base_low = tss_base & 0xFFFF;
    gdt.tss.base_mid = (tss_base >> 16) & 0xFF;
    gdt.tss.type_attr = 0b10001001; // Present, Ring 0, 64-bit TSS
    gdt.tss.limit_high_flags = 0;
    gdt.tss.base_high = (tss_base >> 24) & 0xFF;
    gdt.tss.base_upper = tss_base >> 32;
    gdt.tss.reserved = 0;

    // 4. Carregar a GDT e a TSS
    struct gdt_ptr gdtp = { .limit = sizeof(gdt) - 1, .base = (uint64_t)&gdt };
    gdt_load(&gdtp);
    tss_load(0x28); // 0x28 é o offset da entrada da TSS na nossa GDT
}

