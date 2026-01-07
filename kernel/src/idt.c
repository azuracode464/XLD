#include "idt.h"
#include <stdint.h>
#include <string.h>  // para memset

// Declarações dos handlers externos
extern void keyboard_handler(void);  // do seu interrupts.asm

// Handlers de exceção genéricos (precisa criar em exceptions.asm)
extern void exception_handler_0(void);
extern void exception_handler_1(void);
extern void exception_handler_2(void);
extern void exception_handler_3(void);
extern void exception_handler_4(void);
extern void exception_handler_5(void);
extern void exception_handler_6(void);
extern void exception_handler_7(void);
extern void exception_handler_8(void);
extern void exception_handler_9(void);
extern void exception_handler_10(void);
extern void exception_handler_11(void);
extern void exception_handler_12(void);
extern void exception_handler_13(void);
extern void exception_handler_14(void);
extern void exception_handler_15(void);
extern void exception_handler_16(void);
extern void exception_handler_17(void);
extern void exception_handler_18(void);
extern void exception_handler_19(void);
extern void exception_handler_20(void);
extern void exception_handler_21(void);
extern void exception_handler_22(void);
extern void exception_handler_23(void);
extern void exception_handler_24(void);
extern void exception_handler_25(void);
extern void exception_handler_26(void);
extern void exception_handler_27(void);
extern void exception_handler_28(void);
extern void exception_handler_29(void);
extern void exception_handler_30(void);
extern void exception_handler_31(void);

struct idt_entry idt[256];
struct idt_ptr idtp;

// Array de ponteiros para os handlers de exceção (para facilitar)
static uint64_t exception_handlers[32] = {
    (uint64_t)&exception_handler_0,
    (uint64_t)&exception_handler_1,
    (uint64_t)&exception_handler_2,
    (uint64_t)&exception_handler_3,
    (uint64_t)&exception_handler_4,
    (uint64_t)&exception_handler_5,
    (uint64_t)&exception_handler_6,
    (uint64_t)&exception_handler_7,
    (uint64_t)&exception_handler_8,
    (uint64_t)&exception_handler_9,
    (uint64_t)&exception_handler_10,
    (uint64_t)&exception_handler_11,
    (uint64_t)&exception_handler_12,
    (uint64_t)&exception_handler_13,
    (uint64_t)&exception_handler_14,
    (uint64_t)&exception_handler_15,
    (uint64_t)&exception_handler_16,
    (uint64_t)&exception_handler_17,
    (uint64_t)&exception_handler_18,
    (uint64_t)&exception_handler_19,
    (uint64_t)&exception_handler_20,
    (uint64_t)&exception_handler_21,
    (uint64_t)&exception_handler_22,
    (uint64_t)&exception_handler_23,
    (uint64_t)&exception_handler_24,
    (uint64_t)&exception_handler_25,
    (uint64_t)&exception_handler_26,
    (uint64_t)&exception_handler_27,
    (uint64_t)&exception_handler_28,
    (uint64_t)&exception_handler_29,
    (uint64_t)&exception_handler_30,
    (uint64_t)&exception_handler_31,
};

void idt_set_gate(uint8_t num, uint64_t handler) {
    idt[num].offset_low = handler & 0xFFFF;
    idt[num].selector = 0x08;  // Seletor do segmento de código do kernel
    idt[num].ist = 0;          // Sem IST
    idt[num].type_attr = 0x8E; // Interrupt gate 64-bit, presente, ring 0
    idt[num].offset_middle = (handler >> 16) & 0xFFFF;
    idt[num].offset_high = (handler >> 32) & 0xFFFFFFFF;
    idt[num].zero = 0;
}

void idt_init(void) {
    // Zera toda a IDT
    memset(&idt, 0, sizeof(idt));
    
    // 1. Configura handlers de exceção (0-31)
    for (int i = 0; i < 32; i++) {
        idt_set_gate(i, exception_handlers[i]);
    }
    
    // 2. Configura IRQs (32-47) - por enquanto só o teclado
    // IRQ0 (Timer) - se não tiver handler, mantenha zerado por enquanto
    // IRQ1 (Teclado)
    idt_set_gate(32 + 1, (uint64_t)keyboard_handler);  // IRQ1 = vetor 33
    
    // 3. Para as outras entradas (incluindo spurious interrupts)
    // pode deixar zeradas por enquanto
    
    // Configura o ponteiro da IDT
    idtp.limit = sizeof(idt) - 1;
    idtp.base = (uint64_t)&idt;
    
    // Carrega a IDT
    asm volatile("lidt %0" : : "m"(idtp));
    
    // NOTA: NÃO habilita interrupts aqui! Só depois no kmain.
}
