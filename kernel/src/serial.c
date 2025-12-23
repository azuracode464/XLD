#include "serial.h"

/* COM1 base */
#define SERIAL_PORT 0x3F8

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Line Status Register */
#define SERIAL_LSR 5
#define SERIAL_THR_EMPTY 0x20

static int serial_is_transmit_empty(void) {
    return inb(SERIAL_PORT + SERIAL_LSR) & SERIAL_THR_EMPTY;
}

void serial_init(void) {
    outb(SERIAL_PORT + 1, 0x00);    // Disable interrupts
    outb(SERIAL_PORT + 3, 0x80);    // Enable DLAB
    outb(SERIAL_PORT + 0, 0x03);    // Baud rate low  (38400)
    outb(SERIAL_PORT + 1, 0x00);    // Baud rate high
    outb(SERIAL_PORT + 3, 0x03);    // 8 bits, no parity, one stop
    outb(SERIAL_PORT + 2, 0xC7);    // Enable FIFO, clear, 14-byte threshold
    outb(SERIAL_PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

void serial_write_char(char c) {
    while (!serial_is_transmit_empty());
    outb(SERIAL_PORT, c);
}

void serial_write(const char *s) {
    while (*s) {
        if (*s == '\n')
            serial_write_char('\r');
        serial_write_char(*s++);
    }
}
