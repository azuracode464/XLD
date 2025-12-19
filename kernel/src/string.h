/* 
 * string.h - Implementações seguras para kernel
 * Otimizadas para x86_64 com cuidado com alinhamento
 */

#ifndef KERNEL_STRING_H
#define KERNEL_STRING_H

#include <stddef.h>
#include <stdint.h>

/* ===== MEMORY OPERATIONS ===== */

/* memset - Preenche memória com um byte */
static inline void *memset(void *dest, int c, size_t n) {
    unsigned char *p = dest;
    
    /* Pequenas cópias: faz byte a byte */
    if (n < 16) {
        while (n--) *p++ = (unsigned char)c;
        return dest;
    }
    
    /* Alinha destino para 8 bytes */
    size_t align = (uintptr_t)p & 7;
    if (align) {
        size_t prefix = 8 - align;
        n -= prefix;
        while (prefix--) *p++ = (unsigned char)c;
    }
    
    /* Preenche com palavra de 64 bits */
    uint64_t pattern = 0;
    for (int i = 0; i < 8; i++) {
        pattern |= ((uint64_t)(unsigned char)c << (8 * i));
    }
    
    uint64_t *p64 = (uint64_t *)p;
    while (n >= 8) {
        *p64++ = pattern;
        n -= 8;
    }
    
    /* Bytes finais */
    p = (unsigned char *)p64;
    while (n--) *p++ = (unsigned char)c;
    
    return dest;
}

/* memcpy - Copia memória (não lida com overlap) */
static inline void *memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    
    /* Cópia pequena: byte a byte */
    if (n < 16) {
        while (n--) *d++ = *s++;
        return dest;
    }
    
    /* Verifica se ambos estão alinhados igualmente */
    if ((((uintptr_t)d ^ (uintptr_t)s) & 7) == 0) {
        /* Alinha destino para 8 bytes */
        size_t align = (uintptr_t)d & 7;
        if (align) {
            size_t prefix = 8 - align;
            n -= prefix;
            while (prefix--) *d++ = *s++;
        }
        
        /* Copia em palavras de 64 bits */
        uint64_t *d64 = (uint64_t *)d;
        const uint64_t *s64 = (const uint64_t *)s;
        while (n >= 8) {
            *d64++ = *s64++;
            n -= 8;
        }
        
        d = (unsigned char *)d64;
        s = (const unsigned char *)s64;
    }
    
    /* Bytes finais */
    while (n--) *d++ = *s++;
    
    return dest;
}

/* memmove - Copia memória com overlap seguro */
static inline void *memmove(void *dest, const void *src, size_t n) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    
    if (d == s) return dest;
    
    /* Se destino está depois da fonte, copia de trás para frente */
    if (d > s && d < s + n) {
        d += n;
        s += n;
        while (n--) *(--d) = *(--s);
    } else {
        memcpy(dest, src, n);
    }
    
    return dest;
}

/* memcmp - Compara memória */
static inline int memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *p1 = s1, *p2 = s2;
    
    while (n--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++; p2++;
    }
    return 0;
}

/* ===== STRING OPERATIONS ===== */

/* strlen - Tamanho de string */
static inline size_t strlen(const char *s) {
    const char *p = s;
    while (*p) p++;
    return p - s;
}

/* strcpy - Copia string */
static inline char *strcpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++));
    return dest;
}

/* strncpy - Copia string com limite */
static inline char *strncpy(char *dest, const char *src, size_t n) {
    char *d = dest;
    while (n && (*d++ = *src++)) n--;
    while (n--) *d++ = '\0';
    return dest;
}

/* strcat - Concatena strings */
static inline char *strcat(char *dest, const char *src) {
    char *d = dest;
    while (*d) d++;
    while ((*d++ = *src++));
    return dest;
}

/* strcmp - Compara strings */
static inline int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++; s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

/* strncmp - Compara strings com limite */
static inline int strncmp(const char *s1, const char *s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++; s2++; n--;
    }
    return n ? *(unsigned char *)s1 - *(unsigned char *)s2 : 0;
}

/* strchr - Encontra caractere em string */
static inline char *strchr(const char *s, int c) {
    while (*s && *s != (char)c) s++;
    return (*s == (char)c) ? (char *)s : NULL;
}

/* strrchr - Encontra última ocorrência de caractere */
static inline char *strrchr(const char *s, int c) {
    const char *last = NULL;
    while (*s) {
        if (*s == (char)c) last = s;
        s++;
    }
    return (char *)last;
}

/* strstr - Encontra substring (implementação simples) */
static inline char *strstr(const char *haystack, const char *needle) {
    if (!*needle) return (char *)haystack;
    
    for (; *haystack; haystack++) {
        const char *h = haystack, *n = needle;
        while (*h && *n && *h == *n) {
            h++; n++;
        }
        if (!*n) return (char *)haystack;
    }
    return NULL;
}

/* ===== UTILITIES ===== */

/* bzero - Zera memória (legacy, use memset) */
static inline void bzero(void *s, size_t n) {
    memset(s, 0, n);
}

/* memchr - Encontra caractere na memória */
static inline void *memchr(const void *s, int c, size_t n) {
    const unsigned char *p = s;
    while (n--) {
        if (*p == (unsigned char)c) return (void *)p;
        p++;
    }
    return NULL;
}

#endif /* KERNEL_STRING_H */
