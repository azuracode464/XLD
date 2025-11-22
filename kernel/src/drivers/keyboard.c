#include <stdint.h>
#include <stdbool.h>
#include "../io/io.h"
#include "../pic/pic.h"
#include "scancodes.h"

#define KBD_BUFFER_SIZE 256
static char kbd_buffer[KBD_BUFFER_SIZE];
static volatile int kbd_buffer_head = 0;
static volatile int kbd_buffer_tail = 0;

void keyboard_handler(void) {
    uint8_t scancode = inb(0x60);
    pic_send_eoi(1);

    static bool shift_pressed = false;
    static bool caps_lock_on = false;

    if (scancode & KEY_RELEASE_OFFSET) {
        scancode &= ~KEY_RELEASE_OFFSET;
        if (scancode == KEY_LSHIFT || scancode == KEY_RSHIFT) {
            shift_pressed = false;
        }
        return;
    }

    switch (scancode) {
        case KEY_LSHIFT:
        case KEY_RSHIFT:
            shift_pressed = true;
            return;
        case KEY_CAPS:
            caps_lock_on = !caps_lock_on;
            return;
    }

    char ascii_char;
    if (shift_pressed) {
        ascii_char = scancode_to_ascii_shift[scancode];
    } else {
        ascii_char = scancode_to_ascii[scancode];
    }

    bool is_alpha = (ascii_char >= 'a' && ascii_char <= 'z');
    if (caps_lock_on && is_alpha) {
        if (!shift_pressed) {
            ascii_char -= 32;
        }
    }
    
    if (ascii_char != 0) {
        int next_head = (kbd_buffer_head + 1) % KBD_BUFFER_SIZE;
        if (next_head != kbd_buffer_tail) {
            kbd_buffer[kbd_buffer_head] = ascii_char;
            kbd_buffer_head = next_head;
        }
    }
}

char getchar() {
    while (kbd_buffer_tail == kbd_buffer_head) {
        asm volatile ("hlt");
    }
    char c = kbd_buffer[kbd_buffer_tail];
    kbd_buffer_tail = (kbd_buffer_tail + 1) % KBD_BUFFER_SIZE;
    return c;
}

