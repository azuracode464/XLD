#include "idt.h"
#include "../xldui/xldui.h"
#include "../scheduler/task.h"
#include <stddef.h>

// =================== INÍCIO DA CORREÇÃO ===================
// Mover a definição para o escopo do arquivo (topo)
#define KERNEL_CODE_SELECTOR 0x08
// ==================== FIM DA CORREÇÃO =====================

// Protótipos das funções em idt_asm.asm
extern void idt_load(struct idt_ptr*);
extern void isr0(); extern void isr1(); extern void isr2(); extern void isr3();
extern void isr4(); extern void isr5(); extern void isr6(); extern void isr7();
extern void isr8(); extern void isr9(); extern void isr10(); extern void isr11();
extern void isr12(); extern void isr13(); extern void isr14(); extern void isr15();
extern void isr16(); extern void isr17(); extern void isr18(); extern void isr19();
extern void isr20(); extern void isr21(); extern void isr22(); extern void isr23();
extern void isr24(); extern void isr25(); extern void isr26(); extern void isr27();
extern void isr28(); extern void isr29(); extern void isr30(); extern void isr31();
extern void irq0();
extern void irq1();
extern void isr128();

static struct idt_entry idt[256];
static struct idt_ptr idtp;

static const char *exception_messages[] = {
    "Division By Zero", "Debug", "Non Maskable Interrupt", "Breakpoint",
    "Into Detected Overflow", "Out of Bounds", "Invalid Opcode", "No Coprocessor",
    "Double Fault", "Coprocessor Segment Overrun", "Bad TSS", "Segment Not Present",
    "Stack Fault", "General Protection Fault", "Page Fault", "Unknown Interrupt",
    "Coprocessor Fault", "Alignment Check", "Machine Check", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved", "Reserved"
};

static void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags) {
    idt[num].offset_low = (base & 0xFFFF);
    idt[num].offset_mid = (base >> 16) & 0xFFFF;
    idt[num].offset_high = (base >> 32) & 0xFFFFFFFF;
    idt[num].selector = sel;
    idt[num].ist = 0;
    idt[num].types_attr = flags;
    idt[num].zero = 0;
}

static void u64_to_hex(uint64_t value, char* str) {
    const char* hex_chars = "0123456789ABCDEF";
    str[0] = '0';
    str[1] = 'x';
    str[18] = '\0';
    for (int i = 15; i >= 0; i--) {
        str[2 + (15 - i)] = hex_chars[(value >> (i * 4)) & 0xF];
    }
}

// =================== INÍCIO DA CORREÇÃO ===================
// Silenciando o aviso de parâmetro não utilizado.
void isr_handler(cpu_state_t *state, uint64_t isr_num) {
    (void)state; // Informa ao compilador que 'state' é intencionalmente não utilizado.
// ==================== FIM DA CORREÇÃO =====================
    console_set_color(0xFF0000);
    console_print("\n\n================ KERNEL PANIC ================\n");
    console_print("  UNHANDLED EXCEPTION: ");
    
    if (isr_num < 32) {
        console_print(exception_messages[isr_num]);
    } else {
        console_print("Unknown Interrupt");
    }
    console_print("\n");

    if (isr_num == 14) { // Page Fault
        uint64_t fault_addr;
        asm volatile("mov %%cr2, %0" : "=r"(fault_addr));
        char addr_str[20];
        u64_to_hex(fault_addr, addr_str);
        console_print("  Faulting Address: ");
        console_print(addr_str);
        console_print("\n");
    }
    
    console_print("  SYSTEM HALTED.\n");
    console_print("============================================\n");

    for (;;) {
        asm("cli; hlt");
    }
}

void idt_init(void) {
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base = (uint64_t)&idt[0];

    void (*isrs[])() = {
        isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7, isr8, isr9, isr10,
        isr11, isr12, isr13, isr14, isr15, isr16, isr17, isr18, isr19, isr20,
        isr21, isr22, isr23, isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31
    };

    for (uint8_t i = 0; i < 32; i++) {
        idt_set_gate(i, (uint64_t)isrs[i], KERNEL_CODE_SELECTOR, 0x8E);
    }

    idt_set_gate(32, (uint64_t)irq0, KERNEL_CODE_SELECTOR, 0x8E); // Timer
    idt_set_gate(33, (uint64_t)irq1, KERNEL_CODE_SELECTOR, 0x8E); // Teclado

    idt_set_gate(128, (uint64_t)isr128, KERNEL_CODE_SELECTOR, 0xEE);

    idt_load(&idtp);
}

