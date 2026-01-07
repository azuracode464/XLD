#include <stdint.h>
#include "klog.h"

/* Estrutura que reflete o que empurramos na pilha no ASM */
typedef struct {
    // Registradores salvos pelo pushall
    uint64_t rbp, rax, rbx, rcx, rdx, rsi, rdi, r8, r9, r10, r11, r12, r13, r14, r15;
    
    // Empurrado manualmente pelos stubs ISR
    uint64_t int_no;
    uint64_t err_code;
    
    // Empurrado automaticamente pela CPU
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed)) trapframe_t;

/* Mensagens de erro padrão */
static const char *exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
    "Security Exception",
    "Reserved"
};

void exception_handler(trapframe_t *tf) {
    klog(2, "!!! CPU EXCEPTION %d !!!", tf->int_no);
    
    if (tf->int_no < 32) {
        klog(2, "Description: %s", exception_messages[tf->int_no]);
    }

    klog(2, "RIP: 0x%llx  CS: 0x%llx  RFLAGS: 0x%llx", tf->rip, tf->cs, tf->rflags);
    klog(2, "RSP: 0x%llx  SS: 0x%llx", tf->rsp, tf->ss);
    klog(2, "Error code: 0x%x", tf->err_code);
    
    // Se for Page Fault (14), o endereço ruim fica no CR2
    if (tf->int_no == 14) {
        uint64_t cr2;
        asm volatile("mov %%cr2, %0" : "=r"(cr2));
        klog(2, "CR2 (Bad Addr): 0x%llx", cr2);
    }

    klog(2, "Kernel halted.");
    
    // Trava tudo
    asm volatile("cli");
    for (;;) asm volatile("hlt");
}

