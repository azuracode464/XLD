#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>
#include "../io/io.h" // ✅ Usa o io.h que já existe no seu projeto

// Porta COM1 padrão
#define SERIAL_PORT 0x3F8

// Inicializa a porta serial
static inline void serial_init() {
    outb(SERIAL_PORT + 1, 0x00); // Desativa interrupções
    outb(SERIAL_PORT + 3, 0x80); // Habilita DLAB (para configurar baud rate)
    outb(SERIAL_PORT + 0, 0x03); // Define divisor para 3 (low byte) -> 38400 bps
    outb(SERIAL_PORT + 1, 0x00); // (high byte)
    outb(SERIAL_PORT + 3, 0x03); // 8 bits, sem paridade, 1 stop bit
    outb(SERIAL_PORT + 2, 0xC7); // Habilita FIFO, limpa com buffer de 14 bytes
    outb(SERIAL_PORT + 4, 0x0B); // Habilita IRQs, seta RTS/DSR
}

// Verifica se o buffer de transmissão está vazio
static inline int serial_is_transmit_empty() {
   return inb(SERIAL_PORT + 5) & 0x20;
}

// Envia um único caractere
static inline void serial_write_char(char c) {
    while (serial_is_transmit_empty() == 0); // Espera até que possa transmitir
    outb(SERIAL_PORT, c);
}

// Envia uma string (terminada em nulo)
static inline void serial_write(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        // Adiciona um retorno de carro antes de cada nova linha para compatibilidade
        if (str[i] == '\n') {
            serial_write_char('\r');
        }
        serial_write_char(str[i]);
    }
}

#endif // SERIAL_H

