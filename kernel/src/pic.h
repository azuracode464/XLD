#pragma once

#include <stdint.h>

// Funções do PIC
void pic_remap(void);
void pic_disable(void);
void pic_send_eoi(uint8_t irq);
void enable_irq(uint8_t irq);
void disable_irq(uint8_t irq);

// Funções de I/O inline
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void io_wait(void) {
    outb(0x80, 0);
}
