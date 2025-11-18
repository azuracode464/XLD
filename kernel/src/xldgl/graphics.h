// kernel/src/xldgl/graphics.h

#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>
#include <stddef.h>
#include <limine.h>

// --- INICIALIZAÇÃO E INFO ---
void graphics_init(struct limine_framebuffer *fb);
void graphics_get_dimensions(uint64_t *width, uint64_t *height);

// --- DESENHO DE PRIMITIVAS ---
void put_pixel(uint64_t x, uint64_t y, uint32_t color);
void draw_line(int x0, int y0, int x1, int y1, uint32_t color);
uint32_t get_pixel(uint64_t x, uint64_t y);
// --- DESENHO DE FORMAS ---
void draw_rect(uint64_t x, uint64_t y, uint64_t width, uint64_t height, uint32_t color);
void fill_rect(uint64_t x, uint64_t y, uint64_t width, uint64_t height, uint32_t color);
void draw_circle(int xc, int yc, int r, uint32_t color);
void fill_circle(int xc, int yc, int r, uint32_t color);
void draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3, uint32_t color);

// --- RENDERIZAÇÃO DE TEXTO ---
void draw_char(uint64_t x, uint64_t y, char c, uint32_t color);
void draw_string(uint64_t x, uint64_t y, const char *str, uint32_t color);

#endif // GRAPHICS_H

