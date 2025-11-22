// kernel/src/lib/util.h

#ifndef LIB_UTIL_H
#define LIB_UTIL_H

#include <stdint.h>

// Converte um inteiro de 64 bits para uma string hexadecimal.
// A string de saída deve ter pelo menos 19 bytes de tamanho (0x + 16 dígitos + nulo).
static inline void u64_to_hex(uint64_t value, char* str) {
    const char* hex_chars = "0123456789ABCDEF";
    str[0] = '0';
    str[1] = 'x';
    str[18] = '\0'; // Adiciona o terminador nulo.

    // Preenche a string da direita para a esquerda.
    for (int i = 15; i >= 0; i--) {
        // Pega os 4 bits correspondentes (um dígito hexadecimal).
        uint8_t nibble = (value >> (i * 4)) & 0xF;
        str[2 + (15 - i)] = hex_chars[nibble];
    }
}

#endif // LIB_UTIL_H

