#include <stdint.h>
#include "idt.h"
#include "string.h" // para memset
#include "klog.h"   // para debug

// =============================================================
// IMPORTANTE: O array DEVE ter 256 entradas para suportar 
// exceções (0-31), IRQs (32-47) e Syscall (128/0x80).
// =============================================================
struct idt_entry idt[256]; 
struct idt_ptr idtp;

extern void isr_syscall(void); // Definido em syscall.asm
// Supondo que você tenha um array de handlers de exceção definido em assembly
extern uint64_t exception_handlers[32]; 
extern void keyboard_handler(void);

// Função auxiliar para definir entradas de forma limpa
void idt_set_gate(int num, uint64_t base, uint16_t selector, uint8_t flags) {
    idt[num].offset_low = base & 0xFFFF;
    idt[num].selector = selector;
    idt[num].ist = 0;
    idt[num].type_attr = flags;
    idt[num].offset_middle = (base >> 16) & 0xFFFF;
    idt[num].offset_high = (base >> 32) & 0xFFFFFFFF;
    idt[num].reserved = 0;
}

void idt_init(void) {
    // 1. Limpa tudo
    memset(&idt, 0, sizeof(idt));
    
    // 2. Ponteiro da IDT
    // sizeof(idt) aqui será 256 * 16 = 4096 bytes.
    // O limite é tamanho - 1.
    idtp.limit = sizeof(idt) - 1;
    idtp.base = (uint64_t)&idt;

    // 3. Exceções (0-31) -> Ring 0 (0x8E = Interrupt Gate, Kernel)
    for (int i = 0; i < 32; i++) {
        idt_set_gate(i, exception_handlers[i], 0x08, 0x8E);
    }

    // 4. IRQ 1 (Teclado) -> Vetor 33 -> Ring 0
    idt_set_gate(33, (uint64_t)keyboard_handler, 0x08, 0x8E);

    // 5. SYSCALL (0x80) -> Vetor 128 -> Ring 3 ACESSÍVEL
    // 0xEE = 11101110b 
    //   Present(1) | DPL(11=3 User) | Storage(0) | Type(1110=Int Gate 64)
    idt_set_gate(0x80, (uint64_t)isr_syscall, 0x08, 0xEE);

    // 6. Carrega na CPU
    asm volatile("lidt %0" : : "m"(idtp));
    
    klog(0, "IDT: Initialized (Limit: %d, Base: 0x%llx)", idtp.limit, idtp.base);
}

