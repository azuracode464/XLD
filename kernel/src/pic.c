#include "pic.h"

void pic_remap(void) {
    uint8_t a1, a2;
    
    // Salva máscaras atuais
    a1 = inb(0x21);
    a2 = inb(0xA1);
    
    // Inicializa PICs
    outb(0x20, 0x11); io_wait();
    outb(0xA0, 0x11); io_wait();
    
    // Mapeia IRQ0-7 para 0x20-0x27, IRQ8-15 para 0x28-0x2F
    outb(0x21, 0x20); io_wait();
    outb(0xA1, 0x28); io_wait();
    
    // Configura cascade
    outb(0x21, 0x04); io_wait();
    outb(0xA1, 0x02); io_wait();
    
    // Modo 8086
    outb(0x21, 0x01); io_wait();
    outb(0xA1, 0x01); io_wait();
    
    // Restaura máscaras (desabilita todas IRQs inicialmente)
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);
}

void pic_disable(void) {
    outb(0xA1, 0xFF);
    outb(0x21, 0xFF);
}

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) {
        outb(0xA0, 0x20);
    }
    outb(0x20, 0x20);
}

void enable_irq(uint8_t irq) {
    uint16_t port;
    uint8_t value;
    
    if (irq < 8) {
        port = 0x21;
    } else {
        port = 0xA1;
        irq -= 8;
    }
    value = inb(port) & ~(1 << irq);
    outb(port, value);
}

void disable_irq(uint8_t irq) {
    uint16_t port;
    uint8_t value;
    
    if (irq < 8) {
        port = 0x21;
    } else {
        port = 0xA1;
        irq -= 8;
    }
    value = inb(port) | (1 << irq);
    outb(port, value);
}
