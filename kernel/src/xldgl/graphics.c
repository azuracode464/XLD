// kernel/src/xldgl/graphics.c

#include "graphics.h"
#include "font.h"

// Variáveis estáticas (privadas) para a biblioteca.
static uint32_t* fb_ptr = NULL;
static uint64_t fb_width = 0;
static uint64_t fb_height = 0;
static uint64_t fb_pitch = 0;

// Função auxiliar para valor absoluto de um inteiro.
static inline int abs(int n) {
    return (n < 0) ? -n : n;
}

// --- INICIALIZAÇÃO E INFO ---
void graphics_init(struct limine_framebuffer *fb) {
    fb_ptr = fb->address;
    fb_width = fb->width;
    fb_height = fb->height;
    fb_pitch = fb->pitch;
}

void graphics_get_dimensions(uint64_t *width, uint64_t *height) {
    *width = fb_width;
    *height = fb_height;
}

// --- DESENHO DE PRIMITIVAS ---
void put_pixel(uint64_t x, uint64_t y, uint32_t color) {
    if (fb_ptr == NULL || x >= fb_width || y >= fb_height) return;
    fb_ptr[y * (fb_pitch / 4) + x] = color;
}

void draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    int e2;

    for (;;) {
        put_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

// --- DESENHO DE FORMAS ---
void fill_rect(uint64_t x, uint64_t y, uint64_t width, uint64_t height, uint32_t color) {
    for (uint64_t j = 0; j < height; j++) {
        for (uint64_t i = 0; i < width; i++) {
            put_pixel(x + i, y + j, color);
        }
    }
}

void draw_rect(uint64_t x, uint64_t y, uint64_t width, uint64_t height, uint32_t color) {
    draw_line(x, y, x + width - 1, y, color);
    draw_line(x, y, x, y + height - 1, color);
    draw_line(x + width - 1, y, x + width - 1, y + height - 1, color);
    draw_line(x, y + height - 1, x + width - 1, y + height - 1, color);
}

void draw_circle(int xc, int yc, int r, uint32_t color) {
    int x = r, y = 0;
    int err = 0;

    while (x >= y) {
        put_pixel(xc + x, yc + y, color);
        put_pixel(xc + y, yc + x, color);
        put_pixel(xc - y, yc + x, color);
        put_pixel(xc - x, yc + y, color);
        put_pixel(xc - x, yc - y, color);
        put_pixel(xc - y, yc - x, color);
        put_pixel(xc + y, yc - x, color);
        put_pixel(xc + x, yc - y, color);

        if (err <= 0) {
            y += 1;
            err += 2 * y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

void fill_circle(int xc, int yc, int r, uint32_t color) {
    int x = r, y = 0;
    int err = 0;

    while (x >= y) {
        draw_line(xc - x, yc + y, xc + x, yc + y, color);
        draw_line(xc - y, yc + x, xc + y, yc + x, color);
        draw_line(xc - x, yc - y, xc + x, yc - y, color);
        draw_line(xc - y, yc - x, xc + y, yc - x, color);

        if (err <= 0) {
            y += 1;
            err += 2 * y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

void draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3, uint32_t color) {
    draw_line(x1, y1, x2, y2, color);
    draw_line(x2, y2, x3, y3, color);
    draw_line(x3, y3, x1, y1, color);
}

// --- RENDERIZAÇÃO DE TEXTO ---
void draw_char(uint64_t x, uint64_t y, char c, uint32_t color) {
    unsigned char uc = c;
    
    // ===================================================================
    // AQUI ESTÁ A CORREÇÃO
    // Trata 'font' como um ponteiro de byte único para a aritmética.
    // ===================================================================
    uint8_t *glyph = (uint8_t *)font + uc * FONT_HEIGHT;

    for (int i = 0; i < FONT_HEIGHT; i++) {
        for (int j = 0; j < FONT_WIDTH; j++) {
            if ((glyph[i] >> (FONT_WIDTH - 1 - j)) & 1) {
                put_pixel(x + j, y + i, color);
            }
        }
    }
}

void draw_string(uint64_t x, uint64_t y, const char *str, uint32_t color) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        draw_char(x + i * FONT_WIDTH, y, str[i], color);
    }
}

uint32_t get_pixel(uint64_t x, uint64_t y) {
    if (fb_ptr == NULL || x >= fb_width || y >= fb_height) return 0; // Retorna preto se fora dos limites
    return fb_ptr[y * (fb_pitch / 4) + x];
}
