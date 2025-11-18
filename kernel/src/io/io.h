// kernel/src/io/io.h
#include <stdint.h>
#ifndef IO_H
#define IO_H
// Escreve um byte em um porto de I/O.
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

// Lê um byte de um porto de I/O.
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Uma pequena pausa para garantir que os portos de I/O tenham tempo de processar.
static inline void io_wait(void) {
    outb(0x80, 0);
}

#endif // IO_H

