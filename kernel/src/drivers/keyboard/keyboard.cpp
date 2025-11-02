// Em kernel/src/drivers/keyboard.cpp

#include "keyboard.hpp"
#include <cstdint> // Para uint8_t

// --- Funções de baixo nível para I/O ---
// A gente precisa delas para conversar com o hardware do teclado

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// --- Tabela de Scancodes (PS/2 Set 1) ---
// Este é o nosso "dicionário" para traduzir os códigos do teclado

namespace {
    // Array que mapeia scancode para caractere. 0 significa não imprimível.
    const char SCANCODE_MAP[128] = {
        0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', // Backspace
        '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', // Enter
        0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
        'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', // Space
        // ... o resto a gente pode ignorar por enquanto
    };
}

namespace Keyboard {

void init() {
    // Por enquanto, não precisamos fazer nada aqui.
    // O bootloader geralmente já deixa o teclado em um estado usável.
}

KeyEvent wait_for_key() {
    while (true) {
        // 1. Checa o status do controlador do teclado (porta 0x64)
        // O bit 0 (Output Buffer Status) nos diz se tem um dado esperando
        if (inb(0x64) & 1) {
            // 2. Se tem dado, lê o scancode da porta de dados (porta 0x60)
            uint8_t scancode = inb(0x60);

            // 3. Ignora teclas sendo soltas (scancodes > 128)
            if (scancode < 128) {
                char character = SCANCODE_MAP[scancode];
                SpecialKey special = SpecialKey::None;

                if (character == '\n') {
                    special = SpecialKey::Enter;
                    character = 0; // Não é um caractere imprimível
                } else if (character == '\b') {
                    special = SpecialKey::Backspace;
                    character = 0;
                }
                // TODO: Adicionar casos para as setas, etc.

                // Se for uma tecla que a gente conhece, retorna ela!
                if (character != 0 || special != SpecialKey::None) {
                    return {character, special};
                }
            }
        }
    }
}

} // namespace Keyboard

