// Em: kernel/src/cpu/idt/idt.hpp

#pragma once
#include <cstdint>

// Estrutura para uma entrada na Tabela de Descritores de Interrupção (IDT)
// Isso descreve um "Interrupt Gate" de 64 bits.
struct IDTEntry {
    uint16_t offset_low;    // Bits 0-15 do endereço do handler
    uint16_t selector;      // Seletor do segmento de código (da GDT)
    uint8_t  ist;           // Interrupt Stack Table offset
    uint8_t  attributes;    // Tipo e atributos do gate
    uint16_t offset_mid;    // Bits 16-31 do endereço do handler
    uint32_t offset_high;   // Bits 32-63 do endereço do handler
    uint32_t zero;          // Reservado

} __attribute__((packed)); // '__attribute__((packed))' garante que o compilador não adicione preenchimento

// Estrutura para o ponteiro da IDT (IDTR), que será carregado no processador
struct IDTPointer {
    uint16_t limit; // Tamanho da IDT - 1
    uint64_t base;  // Endereço base da IDT

} __attribute__((packed));
class IDTManager {
public:
    static void initialize();
    static void set_descriptor(uint8_t vector, void* isr_address, uint8_t flags);

private:
    static IDTEntry idt[256];
    static IDTPointer idt_ptr;
};

