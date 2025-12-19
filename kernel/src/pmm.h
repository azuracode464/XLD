// pmm.h
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define PMM_PAGE_SIZE       4096
#define PMM_BITS_PER_BYTE   8
#define PMM_BITMAP_ALIGN    8

// Estrutura do gerenciador
typedef struct {
    uint8_t *bitmap;
    uint64_t bitmap_pages;    // Tamanho do bitmap em páginas
    uint64_t total_pages;
    uint64_t free_pages;
    uint64_t total_memory;
    uint64_t bitmap_phys;     // Endereço físico do bitmap
    uint64_t *frames_stack;   // Stack de frames livres (otimização)
    uint64_t stack_top;
} pmm_manager_t;

// Interface pública
void pmm_init(void);
void *pmalloc(size_t pages);          // Aloca páginas (múltiplas de 4K)
void *pmalloc_aligned(size_t pages, size_t alignment);
void pfree(void *ptr, size_t pages);
void pmm_set_region(uint64_t base, uint64_t size, bool used);
uint64_t pmm_get_free(void);
uint64_t pmm_get_total(void);
void pmm_dump(void);                  // Debug


