#pragma once
#include <cstdint>

// --- Estruturas da IDT ---

// Estrutura de uma entrada na IDT (Gate Descriptor de 64 bits)
struct IDTEntry {
    uint16_t offset_low;    // Bits 0-15 do endereço do handler
    uint16_t selector;      // Seletor de segmento do GDT (deve ser o seletor de código do kernel)
    uint8_t  ist;           // Interrupt Stack Table index (usaremos 0)
    uint8_t  attributes;    // Tipo e atributos do gate
    uint16_t offset_mid;    // Bits 16-31 do endereço
    uint32_t offset_high;   // Bits 32-63 do endereço
    uint32_t zero;          // Reservado
} __attribute__((packed));

// Estrutura do ponteiro para a IDT (usado pela instrução 'lidt')
struct IDTPointer {
    uint16_t limit;         // Tamanho da IDT em bytes - 1
    uint64_t base;          // Endereço do início da IDT
} __attribute__((packed));


// --- Estrutura do Frame de Interrupção ---

// Esta estrutura espelha o que o stub de interrupção empilha na pilha.
// Permite que o handler em C++ acesse os registradores de forma segura.
struct InterruptFrame {
    // Registradores salvos pelo nosso stub
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;

    // Informações que nós adicionamos à pilha
    uint64_t vector_number;
    uint64_t error_code;

    // Informações empilhadas pela CPU automaticamente
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};


// --- Funções de Inicialização ---

// A função principal que orquestra toda a inicialização de interrupções.
void initialize_interrupts();


