#include <stdint.h>
#include <stddef.h>
#include <limine.h>

static struct limine_framebuffer *fb_ptr = NULL;
static uint32_t *fb_addr = NULL;
static uint32_t fb_width = 0;
static uint32_t fb_height = 0;
static uint32_t fb_pitch = 0;

void fb_init(struct limine_framebuffer *fb) {
    fb_ptr = fb;
    fb_addr = (uint32_t *)fb->address;
    fb_width = fb->width;
    fb_height = fb->height;
    fb_pitch = fb->pitch;
}

void fb_putpixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x >= fb_width || y >= fb_height) return;
    
    uint32_t *pixel = (uint32_t *)((uint8_t *)fb_addr + y * fb_pitch + x * 4);
    *pixel = color;
}

void fb_clear(uint32_t color) {
    for (uint32_t y = 0; y < fb_height; y++) {
        for (uint32_t x = 0; x < fb_width; x++) {
            fb_putpixel(x, y, color);
        }
    }
}

uint32_t fb_get_width(void) {
    return fb_width;
}

uint32_t fb_get_height(void) {
    return fb_height;
}
