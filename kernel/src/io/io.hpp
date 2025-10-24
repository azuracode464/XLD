#pragma once
#include <stdint.h>

// =========================
// Leitura e escrita 8-bit
// =========================
inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

// =========================
// Leitura e escrita 16-bit
// =========================
inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

inline void outw(uint16_t port, uint16_t val) {
    asm volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

// =========================
// Leitura e escrita 32-bit
// =========================
inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    asm volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

inline void outl(uint16_t port, uint32_t val) {
    asm volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}

// =========================
// Delay rápido (usado após writes em portas)
// =========================
inline void io_wait() {
    asm volatile ("outb %%al, $0x80" : : "a"(0));
}
// Adicionar no final de io/io.hpp

inline uint64_t rdmsr(uint32_t msr_id) {
    uint32_t low, high;
    asm volatile (
        "rdmsr"
        : "=a"(low), "=d"(high)
        : "c"(msr_id)
    );
    return ((uint64_t)high << 32) | low;
}

