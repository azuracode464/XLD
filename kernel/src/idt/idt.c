#include "idt.h"
#include "../xldui/xldui.h"
#include <stddef.h>

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

void isr_handler(uint8_t isr_num) {
    console_set_color(0xFF0000);
    console_print("\n\n================ KERNEL PANIC ================\n");
    console_print("  UNHANDLED EXCEPTION: ");
    
    if (isr_num < 32) {
        console_print(exception_messages[isr_num]);
    } else {
        console_print("Unknown Interrupt");
    }
    
    console_print("\n  SYSTEM HALTED.\n");
    console_print("============================================\n");

    for (;;) {
        asm("cli; hlt");
    }
}

void idt_init(void) {
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base = (uint64_t)&idt[0];

    for (size_t i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }

    idt_set_gate(0, (uint64_t)isr0, 0x28, 0x8E);
    idt_set_gate(1, (uint64_t)isr1, 0x28, 0x8E);
    idt_set_gate(2, (uint64_t)isr2, 0x28, 0x8E);
    idt_set_gate(3, (uint64_t)isr3, 0x28, 0x8E);
    idt_set_gate(4, (uint64_t)isr4, 0x28, 0x8E);
    idt_set_gate(5, (uint64_t)isr5, 0x28, 0x8E);
    idt_set_gate(6, (uint64_t)isr6, 0x28, 0x8E);
    idt_set_gate(7, (uint64_t)isr7, 0x28, 0x8E);
    idt_set_gate(8, (uint64_t)isr8, 0x28, 0x8E);
    idt_set_gate(9, (uint64_t)isr9, 0x28, 0x8E);
    idt_set_gate(10, (uint64_t)isr10, 0x28, 0x8E);
    idt_set_gate(11, (uint64_t)isr11, 0x28, 0x8E);
    idt_set_gate(12, (uint64_t)isr12, 0x28, 0x8E);
    idt_set_gate(13, (uint64_t)isr13, 0x28, 0x8E);
    idt_set_gate(14, (uint64_t)isr14, 0x28, 0x8E);
    idt_set_gate(15, (uint64_t)isr15, 0x28, 0x8E);
    idt_set_gate(16, (uint64_t)isr16, 0x28, 0x8E);
    idt_set_gate(17, (uint64_t)isr17, 0x28, 0x8E);
    idt_set_gate(18, (uint64_t)isr18, 0x28, 0x8E);
    idt_set_gate(19, (uint64_t)isr19, 0x28, 0x8E);
    idt_set_gate(20, (uint64_t)isr20, 0x28, 0x8E);
    idt_set_gate(21, (uint64_t)isr21, 0x28, 0x8E);
    idt_set_gate(22, (uint64_t)isr22, 0x28, 0x8E);
    idt_set_gate(23, (uint64_t)isr23, 0x28, 0x8E);
    idt_set_gate(24, (uint64_t)isr24, 0x28, 0x8E);
    idt_set_gate(25, (uint64_t)isr25, 0x28, 0x8E);
    idt_set_gate(26, (uint64_t)isr26, 0x28, 0x8E);
    idt_set_gate(27, (uint64_t)isr27, 0x28, 0x8E);
    idt_set_gate(28, (uint64_t)isr28, 0x28, 0x8E);
    idt_set_gate(29, (uint64_t)isr29, 0x28, 0x8E);
    idt_set_gate(30, (uint64_t)isr30, 0x28, 0x8E);
    idt_set_gate(31, (uint64_t)isr31, 0x28, 0x8E);

    idt_set_gate(32, (uint64_t)irq0, 0x28, 0x8E);
    idt_set_gate(33, (uint64_t)irq1, 0x28, 0x8E);

    idt_load(&idtp);
}

