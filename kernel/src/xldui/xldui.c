// kernel/src/xldui/xldui.c

#include "xldui.h"
#include "../xldgl/graphics.h"

// ==========================================================================
// MÓDULO DE DESENHO DE FORMAS (UI - User Interface)
// ==========================================================================

void ui_draw_rect(int x, int y, int width, int height, uint32_t color, int mode) {
    if (mode & MODE_OUTLINE_ONLY) {
        if (mode & MODE_DASHED) {
            for (int i = 0; i < width; i += 8) {
                draw_line(x + i, y, x + i + 4, y, color);
                draw_line(x + i, y + height - 1, x + i + 4, y + height - 1, color);
            }
            for (int i = 0; i < height; i += 8) {
                draw_line(x, y + i, x, y + i + 4, color);
                draw_line(x + width - 1, y + i, x + width - 1, y + i + 4, color);
            }
        } else {
            draw_rect(x, y, width, height, color);
        }
    } else {
        fill_rect(x, y, width, height, color);
    }
}

void ui_draw_circle(int xc, int yc, int radius, uint32_t color, int mode) {
    if (mode & MODE_OUTLINE_ONLY) {
        draw_circle(xc, yc, radius, color);
    } else {
        fill_circle(xc, yc, radius, color);
    }
}

// ==========================================================================
// MÓDULO DE CONSOLE DE TEXTO (Terminal)
// ==========================================================================

static int cursor_x = 0;
static int cursor_y = 0;
static uint32_t text_color = 0xFFFFFF;
static int screen_width_chars = 0;
static int screen_height_chars = 0;

#define CHAR_WIDTH  8
#define CHAR_HEIGHT 16

void console_init(uint32_t bg_color, uint32_t fg_color) {
    cursor_x = 0;
    cursor_y = 0;
    text_color = fg_color;

    uint64_t screen_w, screen_h;
    graphics_get_dimensions(&screen_w, &screen_h);

    screen_width_chars = screen_w / CHAR_WIDTH;
    screen_height_chars = screen_h / CHAR_HEIGHT;

    console_clear(bg_color);
}

void console_clear(uint32_t color) {
    uint64_t screen_w, screen_h;
    graphics_get_dimensions(&screen_w, &screen_h);
    fill_rect(0, 0, screen_w, screen_h, color);
    cursor_x = 0;
    cursor_y = 0;
}

void console_set_color(uint32_t color) {
    text_color = color;
}

void console_print(const char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        char c = str[i];

        if (c == '\n') {
            cursor_x = 0;
            cursor_y++;
        } else {
            draw_char(cursor_x * CHAR_WIDTH, cursor_y * CHAR_HEIGHT, c, text_color);
            cursor_x++;
        }

        if (cursor_x >= screen_width_chars) {
            cursor_x = 0;
            cursor_y++;
        }

        if (cursor_y >= screen_height_chars) {
            cursor_y = 0;
            cursor_x = 0;
        }
    }
}

void console_get_cursor(int *x, int *y) {
    *x = cursor_x;
    *y = cursor_y;
}

void console_move_cursor_back(int *x, int *y) {
    if (cursor_x > 0) {
        cursor_x--;
    } else {
        if (cursor_y > 0) {
            cursor_y--;
            cursor_x = screen_width_chars - 1;
        }
    }
    *x = cursor_x;
    *y = cursor_y;
}
