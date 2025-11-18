#ifndef _XLD_STRING_H
#define _XLD_STRING_H

#include <stddef.h>

static inline size_t strlen(const char* s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

static inline int strcmp(const char* a, const char* b) {
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return *(unsigned char*)a - *(unsigned char*)b;
}

static inline int strncmp(const char* a, const char* b, size_t n) {
    while (n-- && *a && (*a == *b)) {
        a++;
        b++;
    }
    if ((int)n < 0) return 0;
    return *(unsigned char*)a - *(unsigned char*)b;
}

static inline char* strcpy(char* dest, const char* src) {
    char* r = dest;
    while ((*dest++ = *src++));
    return r;
}

static inline char* strncpy(char* dest, const char* src, size_t n) {
    char* r = dest;
    while (n && (*dest++ = *src++)) n--;
    while (n--) *dest++ = '\0';
    return r;
}

static inline void* memcpy(void* dest, const void* src, size_t n) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    while (n--) *d++ = *s++;
    return dest;
}

static inline void* memset(void* dest, int value, size_t n) {
    unsigned char* d = (unsigned char*)dest;
    while (n--) *d++ = (unsigned char)value;
    return dest;
}

static inline int memcmp(const void* a, const void* b, size_t n) {
    const unsigned char* x = (const unsigned char*)a;
    const unsigned char* y = (const unsigned char*)b;
    while (n--) {
        if (*x != *y) return *x - *y;
        x++; y++;
    }
    return 0;
}

#endif
