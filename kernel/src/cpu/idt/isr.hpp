// Em: kernel/src/cpu/idt/isr.hpp

#pragma once
#include <cstdint>

// Esta estrutura representa o estado dos registradores que salvamos na pilha.
// A ordem é importante e deve corresponder ao que o Assembly faz.
struct InterruptFrame {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_no, err_code; // Nosso stub vai empurrar o número da interrupção e um código de erro.
    // O hardware empurra automaticamente o seguinte:
    uint64_t rip, cs, rflags, rsp, ss;
};

// Nosso handler genérico em C++. Ele recebe um ponteiro para o frame.
extern "C" void generic_interrupt_handler(InterruptFrame* frame);

