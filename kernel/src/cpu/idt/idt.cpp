// Em: kernel/src/cpu/idt/idt.cpp

#include "idt.hpp"

// Declaração das nossas variáveis estáticas
IDTEntry IDTManager::idt[256];
IDTPointer IDTManager::idt_ptr;

// Função externa em Assembly para carregar a IDT
extern "C" void idt_load(IDTPointer* idt_ptr);

void IDTManager::set_descriptor(uint8_t vector, void* isr_address, uint8_t flags) {
    IDTEntry* descriptor = &idt[vector];
    uint64_t address = (uint64_t)isr_address;

    descriptor->offset_low  = address & 0xFFFF;
    descriptor->selector    = 0x28; // Seletor de código do kernel (definido pelo Limine)
    descriptor->ist         = 0;
    descriptor->attributes  = flags;
    descriptor->offset_mid  = (address >> 16) & 0xFFFF;
    descriptor->offset_high = (address >> 32) & 0xFFFFFFFF;
    descriptor->zero        = 0;
}

void IDTManager::initialize() {
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = (uint64_t)&idt;

    // (Aqui vamos adicionar chamadas para set_descriptor para cada interrupção)

    // Carrega a IDT no processador
    idt_load(&idt_ptr);
}

