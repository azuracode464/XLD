// src/kmalloc.h
#ifndef KMALLOC_H
#define KMALLOC_H

#include <stdint.h>
#include <stddef.h>

#define KMALLOC_ALIGN 16
#define KMALLOC_MIN_SIZE 16
#define KMALLOC_MAX_SMALL 2048

// Cabeçalho do bloco
typedef struct kmalloc_block {
    size_t size;
    struct kmalloc_block *next;
    uint8_t used;
    uint8_t magic[3];  // "KMB"
} kmalloc_block_t;

#define BLOCK_HEADER_SIZE sizeof(kmalloc_block_t)

// Declare BLOCK_MAGIC como extern
extern const uint8_t BLOCK_MAGIC[3];

// Interface pública
void kmalloc_init(void);
void *kmalloc(size_t size);
void *kcalloc(size_t nmemb, size_t size);
void *krealloc(void *ptr, size_t size);
void kfree(void *ptr);
size_t kmalloc_usable_size(void *ptr);
void kmalloc_dump(void);

#endif // KMALLOC_H
