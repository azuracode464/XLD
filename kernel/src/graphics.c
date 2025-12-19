#include <stdint.h>
#include <stddef.h>

#define FONT8x16_IMPLEMENTATION
#include "font.h"

extern void fb_putpixel(uint32_t x, uint32_t y, uint32_t color);
extern uint32_t fb_get_width(void);
extern uint32_t fb_get_height(void);

static uint32_t cursor_x = 0;
static uint32_t cursor_y = 0;

void graphics_init(void) {
    cursor_x = 0;
    cursor_y = 0;
}

void graphics_draw_char(char c, uint32_t x, uint32_t y, uint32_t fg, uint32_t bg) {
    unsigned char uc = (unsigned char)c;
    if (uc > 127) uc = 0;

    unsigned char *glyph = font[uc];

    for (int row = 0; row < FONT_HEIGHT; row++) {
        unsigned char line = glyph[row];
        for (int col = 0; col < FONT_WIDTH; col++) {
            uint32_t color = (line & (0x80 >> col)) ? fg : bg;
            fb_putpixel(x + col, y + row, color);
        }
    }
}

void graphics_draw_string(const char *str, uint32_t x, uint32_t y, uint32_t fg, uint32_t bg) {
    uint32_t orig_x = x;

    while (*str) {
        if (*str == '\n') {
            y += FONT_HEIGHT;
            x = orig_x;
        } else {
            graphics_draw_char(*str, x, y, fg, bg);
            x += FONT_WIDTH;
        }
        str++;
    }
}

uint32_t graphics_get_cursor_x(void) {
    return cursor_x;
}

uint32_t graphics_get_cursor_y(void) {
    return cursor_y;
}

void graphics_set_cursor(uint32_t x, uint32_t y) {
    cursor_x = x;
    cursor_y = y;
}

void graphics_putchar(char c, uint32_t fg, uint32_t bg) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y += FONT_HEIGHT;
    } else {
        graphics_draw_char(c, cursor_x, cursor_y, fg, bg);
        cursor_x += FONT_WIDTH;

        if (cursor_x >= fb_get_width()) {
            cursor_x = 0;
            cursor_y += FONT_HEIGHT;
        }
    }

    if (cursor_y >= fb_get_height()) {
        cursor_y = 0;
    }
}
