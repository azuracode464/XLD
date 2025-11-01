#include "idt.hpp"
#include "xldgl/graphics.hpp"

// --- Variáveis Globais ---
__attribute__((aligned(0x10)))
static IDTEntry idt[256];
static IDTPointer idt_ptr;
Graphics* g_gfx = nullptr;

// --- Declarações e Definições dos Stubs ---

extern "C" void generic_interrupt_handler(InterruptFrame* frame);

// Macro para DEFINIR um stub de interrupção.
#define INTERRUPT_STUB_DEFINE(vector) \
asm ( \
    ".global interrupt_stub_" #vector "\n" \
    "interrupt_stub_" #vector ":\n" \
    ".if " #vector " != 8 && (" #vector " < 10 || " #vector " > 14) && " #vector " != 17 && " #vector " != 21 && " #vector " != 29 && " #vector " != 30 \n" \
    "    pushq $0\n" \
    ".endif\n" \
    "    pushq $" #vector "\n" \
    "    jmp common_interrupt_stub\n" \
);

// Stub comum para todos os vetores.
asm (
    ".global common_interrupt_stub\n"
    "common_interrupt_stub:\n"
    "    pushq %rax; pushq %rbx; pushq %rcx; pushq %rdx; pushq %rsi; pushq %rdi; pushq %rbp;\n"
    "    pushq %r8;  pushq %r9;  pushq %r10; pushq %r11; pushq %r12; pushq %r13; pushq %r14; pushq %r15;\n"
    "    movq %rsp, %rdi\n"
    "    call generic_interrupt_handler\n"
    "    popq %r15; popq %r14; popq %r13; popq %r12; popq %r11; popq %r10; popq %r9;  popq %r8;\n"
    "    popq %rbp; popq %rdi; popq %rsi; popq %rdx; popq %rcx; popq %rbx; popq %rax;\n"
    "    addq $16, %rsp\n"
    "    iretq\n"
);

// Macro para DECLARAR um stub de interrupção.
#define INTERRUPT_STUB_DECLARE(vector) extern "C" void interrupt_stub_##vector()

// DECLARA todas as funções stub no escopo global.
INTERRUPT_STUB_DECLARE(0);  INTERRUPT_STUB_DECLARE(1);  INTERRUPT_STUB_DECLARE(2);  INTERRUPT_STUB_DECLARE(3);
INTERRUPT_STUB_DECLARE(4);  INTERRUPT_STUB_DECLARE(5);  INTERRUPT_STUB_DECLARE(6);  INTERRUPT_STUB_DECLARE(7);
INTERRUPT_STUB_DECLARE(8);  INTERRUPT_STUB_DECLARE(9);  INTERRUPT_STUB_DECLARE(10); INTERRUPT_STUB_DECLARE(11);
INTERRUPT_STUB_DECLARE(12); INTERRUPT_STUB_DECLARE(13); INTERRUPT_STUB_DECLARE(14); INTERRUPT_STUB_DECLARE(15);
INTERRUPT_STUB_DECLARE(16); INTERRUPT_STUB_DECLARE(17); INTERRUPT_STUB_DECLARE(18); INTERRUPT_STUB_DECLARE(19);
INTERRUPT_STUB_DECLARE(20); INTERRUPT_STUB_DECLARE(21); INTERRUPT_STUB_DECLARE(22); INTERRUPT_STUB_DECLARE(23);
INTERRUPT_STUB_DECLARE(24); INTERRUPT_STUB_DECLARE(25); INTERRUPT_STUB_DECLARE(26); INTERRUPT_STUB_DECLARE(27);
INTERRUPT_STUB_DECLARE(28); INTERRUPT_STUB_DECLARE(29); INTERRUPT_STUB_DECLARE(30); INTERRUPT_STUB_DECLARE(31);
INTERRUPT_STUB_DECLARE(33);

// DEFINE o corpo de todos os stubs.
INTERRUPT_STUB_DEFINE(0);  INTERRUPT_STUB_DEFINE(1);  INTERRUPT_STUB_DEFINE(2);  INTERRUPT_STUB_DEFINE(3);
INTERRUPT_STUB_DEFINE(4);  INTERRUPT_STUB_DEFINE(5);  INTERRUPT_STUB_DEFINE(6);  INTERRUPT_STUB_DEFINE(7);
INTERRUPT_STUB_DEFINE(8);  INTERRUPT_STUB_DEFINE(9);  INTERRUPT_STUB_DEFINE(10); INTERRUPT_STUB_DEFINE(11);
INTERRUPT_STUB_DEFINE(12); INTERRUPT_STUB_DEFINE(13); INTERRUPT_STUB_DEFINE(14); INTERRUPT_STUB_DEFINE(15);
INTERRUPT_STUB_DEFINE(16); INTERRUPT_STUB_DEFINE(17); INTERRUPT_STUB_DEFINE(18); INTERRUPT_STUB_DEFINE(19);
INTERRUPT_STUB_DEFINE(20); INTERRUPT_STUB_DEFINE(21); INTERRUPT_STUB_DEFINE(22); INTERRUPT_STUB_DEFINE(23);
INTERRUPT_STUB_DEFINE(24); INTERRUPT_STUB_DEFINE(25); INTERRUPT_STUB_DEFINE(26); INTERRUPT_STUB_DEFINE(27);
INTERRUPT_STUB_DEFINE(28); INTERRUPT_STUB_DEFINE(29); INTERRUPT_STUB_DEFINE(30); INTERRUPT_STUB_DEFINE(31);
INTERRUPT_STUB_DEFINE(33);


// --- Funções Auxiliares e Handlers ---

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

void generic_interrupt_handler(InterruptFrame* frame) {
    if (frame->vector_number == 33) {
        uint8_t scancode = inb(0x60);
        (void)scancode;
        if (g_gfx) {
            g_gfx->draw_char('K', 400, 200, 0xFFFF00);
        }
    } else {
        if (g_gfx) {
            g_gfx->draw_text("CPU EXCEPTION!", 10, 200, 0xFF0000);
        }
    }
    if (frame->vector_number >= 40) {
        outb(0xA0, 0x20);
    }
    outb(0x20, 0x20);
}

static void set_idt_gate(int vector, uint64_t offset, uint16_t selector, uint8_t attributes) {
    idt[vector].offset_low = offset & 0xFFFF;
    idt[vector].offset_mid = (offset >> 16) & 0xFFFF;
    idt[vector].offset_high = (offset >> 32) & 0xFFFFFFFF;
    idt[vector].selector = selector;
    idt[vector].attributes = attributes;
    idt[vector].ist = 0;
    idt[vector].zero = 0;
}

static void remap_pic() {
    uint8_t a1 = inb(0x21);
    uint8_t a2 = inb(0xA1);
    outb(0x20, 0x11); outb(0xA0, 0x11);
    outb(0x21, 32); outb(0xA1, 40);
    outb(0x21, 4); outb(0xA1, 2);
    outb(0x21, 1); outb(0xA1, 1);
    outb(0x21, a1); outb(0xA1, a2);
}

// A função que o kmain vai chamar
void initialize_interrupts() {
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = (uint64_t)&idt;

    // Macro para REGISTRAR um stub (agora sem 'extern')
    #define IDT_REGISTER(vector) \
        set_idt_gate(vector, (uint64_t)interrupt_stub_##vector, 0x08, 0x8E);

    // REGISTRA todos os stubs na IDT.
    IDT_REGISTER(0);  IDT_REGISTER(1);  IDT_REGISTER(2);  IDT_REGISTER(3);
    IDT_REGISTER(4);  IDT_REGISTER(5);  IDT_REGISTER(6);  IDT_REGISTER(7);
    IDT_REGISTER(8);  IDT_REGISTER(9);  IDT_REGISTER(10); IDT_REGISTER(11);
    IDT_REGISTER(12); IDT_REGISTER(13); IDT_REGISTER(14); IDT_REGISTER(15);
    IDT_REGISTER(16); IDT_REGISTER(17); IDT_REGISTER(18); IDT_REGISTER(19);
    IDT_REGISTER(20); IDT_REGISTER(21); IDT_REGISTER(22); IDT_REGISTER(23);
    IDT_REGISTER(24); IDT_REGISTER(25); IDT_REGISTER(26); IDT_REGISTER(27);
    IDT_REGISTER(28); IDT_REGISTER(29); IDT_REGISTER(30); IDT_REGISTER(31);
    IDT_REGISTER(33);

    remap_pic();
    asm volatile ("lidt %0" : : "m"(idt_ptr));
    asm volatile ("sti");
}

