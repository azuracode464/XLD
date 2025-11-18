// kernel/src/drivers/keyboard.c

#include <stdint.h>
#include <stdbool.h>
#include "../xldui/xldui.h"   // Para console_print e obter o cursor
#include "../xldgl/graphics.h" // Para get_pixel e put_pixel
#include "../io/io.h"
#include "../pic/pic.h"
#include "scancodes.h"

// --- Constantes e Variáveis de Estado ---
#define CHAR_WIDTH  8
#define CHAR_HEIGHT 16

static bool shift_pressed = false;
static bool ctrl_pressed = false;
static bool caps_lock_on = false;

// Buffer para salvar os pixels do último caractere, como você sugeriu.
static uint32_t last_char_bg[CHAR_WIDTH * CHAR_HEIGHT];

// Função auxiliar para imprimir um caractere no console
static void print_char(char c) {
    // Obtém a posição atual do cursor (precisamos de uma forma de fazer isso)
    // Vamos assumir que podemos obter externamente ou modificar o console para isso.
    // Por enquanto, vamos criar uma função para obter o cursor.
    int cursor_x, cursor_y;
    console_get_cursor(&cursor_x, &cursor_y);

    // Salva o fundo ANTES de desenhar
    for (int y = 0; y < CHAR_HEIGHT; y++) {
        for (int x = 0; x < CHAR_WIDTH; x++) {
            last_char_bg[y * CHAR_WIDTH + x] = get_pixel(
                (cursor_x * CHAR_WIDTH) + x,
                (cursor_y * CHAR_HEIGHT) + y
            );
        }
    }

    // Imprime o caractere
    char str[2] = {c, '\0'};
    console_print(str);
}

// Função auxiliar para verificar se um caractere é uma letra
static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z');
}

// --- Handler Principal do Teclado ---
void keyboard_handler(void) {
    uint8_t scancode = inb(0x60);
    pic_send_eoi(1);

    // Lógica de liberação de tecla
    if (scancode & KEY_RELEASE_OFFSET) {
        scancode &= ~KEY_RELEASE_OFFSET;
        switch (scancode) {
            case KEY_LSHIFT:
            case KEY_RSHIFT:
                shift_pressed = false;
                break;
            case KEY_LCTRL:
                ctrl_pressed = false;
                break;
        }
        return;
    }

    // Lógica de pressionamento de tecla
    switch (scancode) {
        case KEY_LSHIFT:
        case KEY_RSHIFT:
            shift_pressed = true;
            return;
        case KEY_LCTRL:
            ctrl_pressed = true;
            return;
        case KEY_CAPS:
            caps_lock_on = !caps_lock_on;
            return;

        case KEY_BKSP: {
            // --- LÓGICA DO BACKSPACE DENTRO DO DRIVER ---
            int cursor_x, cursor_y;
            console_move_cursor_back(&cursor_x, &cursor_y); // Pede ao console para mover o cursor

            // Restaura os pixels na nova posição do cursor
            for (int y = 0; y < CHAR_HEIGHT; y++) {
                for (int x = 0; x < CHAR_WIDTH; x++) {
                    put_pixel(
                        (cursor_x * CHAR_WIDTH) + x,
                        (cursor_y * CHAR_HEIGHT) + y,
                        last_char_bg[y * CHAR_WIDTH + x]
                    );
                }
            }
            return;
        }
    }

    // Traduzir e imprimir o caractere
    char ascii_char;
    if (shift_pressed) {
        ascii_char = scancode_to_ascii_shift[scancode];
    } else {
        ascii_char = scancode_to_ascii[scancode];
    }

    if (caps_lock_on && is_alpha(ascii_char)) {
        if (!shift_pressed) {
            ascii_char -= 32;
        }
    }
    
    if (ascii_char != 0) {
        if (ctrl_pressed) {
            print_char('^');
        }
        print_char(ascii_char);
    }
}

