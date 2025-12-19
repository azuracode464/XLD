#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#define FONT_WIDTH 8
#define FONT_HEIGHT 16

extern void graphics_putchar(char c, uint32_t fg, uint32_t bg);
extern void graphics_set_cursor(uint32_t x, uint32_t y);
extern uint32_t graphics_get_cursor_x(void);
extern uint32_t graphics_get_cursor_y(void);

static uint32_t log_y = 0;

void klog_init(void) {
    log_y = 0;
    graphics_set_cursor(0, 0);
}

static void kputchar(char c) {
    graphics_putchar(c, 0xFFFFFF, 0x000000);
}

static void kputs(const char *str) {
    while (*str) {
        kputchar(*str++);
    }
}

static void kputnum(uint64_t num, int base) {
    char buf[32];
    int i = 0;
    
    if (num == 0) {
        kputchar('0');
        return;
    }
    
    while (num > 0) {
        int digit = num % base;
        buf[i++] = (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
        num /= base;
    }
    
    while (i > 0) {
        kputchar(buf[--i]);
    }
}

void printk(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
                case 'd': {
                    int val = va_arg(args, int);
                    if (val < 0) {
                        kputchar('-');
                        val = -val;
                    }
                    kputnum(val, 10);
                    break;
                }
                case 'u': {
                    unsigned int val = va_arg(args, unsigned int);
                    kputnum(val, 10);
                    break;
                }
                case 'x': {
                    unsigned int val = va_arg(args, unsigned int);
                    kputs("0x");
                    kputnum(val, 16);
                    break;
                }
                case 's': {
                    const char *str = va_arg(args, const char *);
                    kputs(str);
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    kputchar(c);
                    break;
                }
                case '%':
                    kputchar('%');
                    break;
            }
        } else {
            kputchar(*fmt);
        }
        fmt++;
    }
    
    va_end(args);
}

void klog(int level, const char *fmt, ...) {
    const char *prefix;
    
    switch (level) {
        case 0: prefix = "[INFO] "; break;
        case 1: prefix = "[WARN] "; break;
        case 2: prefix = "[ERROR] "; break;
        default: prefix = "[LOG] "; break;
    }
    
    kputs(prefix);
    
    va_list args;
    va_start(args, fmt);
    
    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
                case 'd': {
                    int val = va_arg(args, int);
                    if (val < 0) {
                        kputchar('-');
                        val = -val;
                    }
                    kputnum(val, 10);
                    break;
                }
                case 'u': {
                    unsigned int val = va_arg(args, unsigned int);
                    kputnum(val, 10);
                    break;
                }
                case 'x': {
                    unsigned int val = va_arg(args, unsigned int);
                    kputs("0x");
                    kputnum(val, 16);
                    break;
                }
                case 's': {
                    const char *str = va_arg(args, const char *);
                    kputs(str);
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    kputchar(c);
                    break;
                }
            }
        } else {
            kputchar(*fmt);
        }
        fmt++;
    }
    
    kputchar('\n');
    va_end(args);
}
