#include "graphics.hpp"
#include <cstddef>
#include "math.hpp"
#include <algorithm>

Graphics::Graphics(volatile uint32_t* framebuffer, int pitch)
    : fb_(framebuffer), pitch_(pitch) {}

// Pixel
void Graphics::put_pixel(int x, int y, uint32_t color) {
    fb_[y * (pitch_ / 4) + x] = color;
}

// Retângulo
void Graphics::draw_rect(int x, int y, int width, int height, uint32_t color) {
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            put_pixel(x + i, y + j, color);
        }
    }
}

// Círculo (Midpoint circle algorithm simples)
void Graphics::draw_circle(int cx, int cy, int r, uint32_t color) {
    for (int y = -r; y <= r; y++) {
        for (int x = -r; x <= r; x++) {
            if (x*x + y*y <= r*r) put_pixel(cx + x, cy + y, color);
        }
    }
}

// Triângulo (fill simples usando bounding box)
void Graphics::draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color) {
    int minX = std::min({x0, x1, x2});
    int maxX = std::max({x0, x1, x2});
    int minY = std::min({y0, y1, y2});
    int maxY = std::max({y0, y1, y2});

    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            int w0 = (x1 - x0)*(y - y0) - (x - x0)*(y1 - y0);
            int w1 = (x2 - x1)*(y - y1) - (x - x1)*(y2 - y1);
            int w2 = (x0 - x2)*(y - y2) - (x - x2)*(y0 - y2);
            if ((w0 >= 0 && w1 >= 0 && w2 >= 0) || (w0 <=0 && w1 <=0 && w2 <=0)) {
                put_pixel(x, y, color);
            }
        }
    }
}

// Desenha caractere usando font.hpp
void Graphics::draw_char(char c, int x, int y, uint32_t color) {
    if (c < 32) return;
    const uint8_t* glyph = large_font[c];
    for (int row = 0; row < FONT_HEIGHT; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < FONT_WIDTH; col++) {
            if (bits & (1 << (7 - col))) {
                put_pixel(x + col, y + row, color);
            }
        }
    }
}

// Desenha string
void Graphics::draw_text(const char* str, int x, int y, uint32_t color) {
    while (*str) {
        draw_char(*str, x, y, color);
        x += FONT_WIDTH;
        str++;
    }
}
