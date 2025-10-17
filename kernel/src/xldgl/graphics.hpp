#pragma once
#include <cstdint>
#include "font.hpp"

class Graphics {
public:
    Graphics(volatile uint32_t* framebuffer, int pitch);

    // Primitivas
    void put_pixel(int x, int y, uint32_t color);
    void draw_rect(int x, int y, int width, int height, uint32_t color);
    void draw_circle(int x, int y, int radius, uint32_t color);
    void draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color);

    // Texto
    void draw_char(char c, int x, int y, uint32_t color);
    void draw_text(const char* str, int x, int y, uint32_t color);

private:
    volatile uint32_t* fb_;
    int pitch_;
};
