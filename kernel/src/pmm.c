// pmm.c - Physical Memory Manager (bitmap-based) - versão otimizada
//
// Melhorias e garantias:
// - Usa a request do Limine definida em limine_requests.c (extern memmap_request).
// - Reserva um espaço contíguo para bitmap + pequeno cache (frames_stack) dentro
//   de uma região USABLE do memmap para evitar usar pmalloc antes do init.
// - Implementa um pequeno cache (stack) de frames livres para acelerar pmalloc(1)/pfree(1).
// - Scans do bitmap são acelerados pulando palavras 64-bit que estão todas usadas.
// - Logs seguros usando %llx / %llu (assume que klog suporta isso).
// - PHYS_TO_VIRT / VIRT_TO_PHYS configuráveis via KERNEL_VIRT_OFFSET.
// - Checagens e probes para evitar triple fault por acesso a memória não mapeada.
//
// Nota importante:
// - Ajuste KERNEL_VIRT_OFFSET se o kernel já estiver mapeado higher-half.
//   Por padrão deixamos 0 (identity) para early-boot; defina o valor correto
//   no seu build se você tem higher-half mapping já ativo.

#include "pmm.h"
#include "string.h"
#include <limine.h>
#include <stdint.h>
#include <stdbool.h>
#include "spinlock.h"

extern volatile struct limine_memmap_request memmap_request; // de limine_requests.c
extern void klog(int level, const char *fmt, ...);
extern void panic(const char *msg);

// Ajuste: se seu kernel estiver higher-half mapped já, sobrescreva este valor no build.
#ifndef KERNEL_VIRT_OFFSET
#define KERNEL_VIRT_OFFSET 0ULL
#endif

#define PHYS_TO_VIRT(x) ((void *)((uint64_t)(x) + (uint64_t)KERNEL_VIRT_OFFSET))
#define VIRT_TO_PHYS(x) ((uint64_t)(x) - (uint64_t)KERNEL_VIRT_OFFSET)

// Parâmetros do cache de frames
#define FRAMES_STACK_ENTRIES 1024  // 1024 entradas = 8KB (2 páginas) por padrão
#define FRAMES_STACK_MIN_PAGES 2   // páginas reservadas para o stack cache

// Níveis de log (compatível com o restante do projeto)
#define KLOG_INFO 0
#define KLOG_WARN 1
#define KLOG_ERROR 2
#define KLOG_DEBUG 3

static pmm_manager_t pmm;
static spinlock_t pmm_lock = SPINLOCK_INIT;

/* Helpers de bitmap */
static inline void bitmap_set(uint64_t page, bool value) {
    uint64_t byte = page >> 3;           // /8
    uint8_t bit = page & 7;              // %8

    if (value)
        pmm.bitmap[byte] |= (1 << bit);
    else
        pmm.bitmap[byte] &= ~(1 << bit);
}

static inline bool bitmap_get(uint64_t page) {
    uint64_t byte = page >> 3;
    uint8_t bit = page & 7;
    return (pmm.bitmap[byte] >> bit) & 1;
}

/* Helper para testar se uma palavra de 64 bits do bitmap está toda ocupada (todas as 8*8 bits = 64 bits) */
static inline bool bitmap_word_full(uint64_t word) {
    return word == UINT64_MAX;
}

/* Busca uma sequência livre de 'pages' páginas.
   Retorna índice da primeira página (física page index) ou UINT64_MAX se não encontrar.
   Algoritmo: percorre bitmap em palavras de 64 bits pulando palavras cheias; quando acha
   uma palavra não cheia procura o primeiro bit zero e faz verificação sequencial até encontrar
   a run desejada. */
static uint64_t find_free_run(uint64_t pages) {
    if (pages == 0) return UINT64_MAX;

    uint64_t total_bits = pmm.total_pages;
    uint64_t total_words = (total_bits + 63) / 64;
    uint64_t *bm64 = (uint64_t *)pmm.bitmap;

    uint64_t bit_index = 0;

    for (uint64_t w = 0; w < total_words; w++) {
        uint64_t word = bm64[w];

        if (bitmap_word_full(word)) {
            bit_index += 64;
            continue;
        }

        // há pelo menos um zero neste word; examine bits
        for (int b = 0; b < 64; b++) {
            uint64_t idx = bit_index + b;
            if (idx >= total_bits) return UINT64_MAX;

            if (!((word >> b) & 1)) {
                // candidate start at idx
                uint64_t run = 1;
                uint64_t check_idx = idx + 1;
                // checar bits seguintes (pode cruzar palavras)
                while (run < pages && check_idx < total_bits) {
                    if (bitmap_get(check_idx)) break;
                    run++;
                    check_idx++;
                }

                if (run == pages) {
                    return idx;
                }

                // Pular para check_idx (aumenta b/word de forma eficiente)
                b += (run); // avançar b (loop fará +1 extra)
            }
        }

        bit_index += 64;
    }

    return UINT64_MAX;
}

/* Fast cache (stack) helpers para alloc/free de 1 página */
static inline uint64_t stack_pop(void) {
    if (pmm.stack_top == 0) return UINT64_MAX;
    return pmm.frames_stack[--pmm.stack_top];
}

static inline void stack_push(uint64_t frame) {
    if (pmm.stack_top >= pmm.bitmap_pages * PMM_PAGE_SIZE) return; // segurança
    if (pmm.stack_top >= FRAMES_STACK_ENTRIES) return;
    pmm.frames_stack[pmm.stack_top++] = frame;
}

/* Inicializa PMM: constrói bitmap e stack cache.
 * Processo:
 *  - Lê memmap_request.response
 *  - Calcula total_pages e tamanho do bitmap
 *  - Procura uma região USABLE capaz de armazenar (bitmap + frames_stack)
 *  - Reserva essas páginas (ajusta a entrada do memmap localmente)
 *  - Inicializa bitmap (tudo 1) e depois marca regiões USABLE como 0
 *  - Marca o próprio bitmap e stack como usados
 *  - Marca o kernel reservado (se configurado) como usado
 */
void pmm_init(void) {
    if (memmap_request.response == NULL) {
        panic("PMM: Limine memmap response is NULL");
    }

    struct limine_memmap_response *mmap = memmap_request.response;
    klog(KLOG_INFO, "PMM: memmap entries: %llu", (unsigned long long)mmap->entry_count);

    // 1) calcular memória total
    uint64_t highest = 0;
    for (size_t i = 0; i < mmap->entry_count; i++) {
        struct limine_memmap_entry *e = mmap->entries[i];
        if (e->type == LIMINE_MEMMAP_USABLE ||
            e->type == LIMINE_MEMMAP_ACPI_RECLAIMABLE ||
            e->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE) {
            uint64_t end = e->base + e->length;
            if (end > highest) highest = end;
        }
    }

    pmm.total_memory = highest;
    pmm.total_pages = highest / PMM_PAGE_SIZE;

    // 2) tamanho do bitmap em bytes / páginas
    uint64_t bitmap_size_bytes = (pmm.total_pages + 7) / 8;
    pmm.bitmap_pages = (bitmap_size_bytes + PMM_PAGE_SIZE - 1) / PMM_PAGE_SIZE;

    klog(KLOG_INFO, "PMM: total mem=%llu MB, pages=%llu, bitmap=%llu bytes (%llu pages)",
         (unsigned long long)(pmm.total_memory / 1024 / 1024),
         (unsigned long long)pmm.total_pages,
         (unsigned long long)bitmap_size_bytes,
         (unsigned long long)pmm.bitmap_pages);

    // 3) determinar tamanho do frames_stack reservado (em páginas)
    size_t frames_stack_bytes = FRAMES_STACK_ENTRIES * sizeof(uint64_t);
    uint64_t frames_stack_pages = (frames_stack_bytes + PMM_PAGE_SIZE - 1) / PMM_PAGE_SIZE;
    if (frames_stack_pages < FRAMES_STACK_MIN_PAGES) frames_stack_pages = FRAMES_STACK_MIN_PAGES;

    // 4) encontrar região USABLE que comporte bitmap + frames_stack (contíguo)
    uint64_t needed_bytes = bitmap_size_bytes + frames_stack_pages * PMM_PAGE_SIZE;
    uint64_t chosen_phys = 0;
    size_t chosen_idx = (size_t)-1;

    for (size_t i = 0; i < mmap->entry_count; i++) {
        struct limine_memmap_entry *e = mmap->entries[i];
        if (e->type == LIMINE_MEMMAP_USABLE && e->length >= needed_bytes) {
            // alinhar ao tamanho de página (já está)
            chosen_phys = e->base;
            chosen_idx = i;
            // reduzir localmente a entry (reservamos do começo)
            e->base += needed_bytes;
            e->length -= needed_bytes;
            break;
        }
    }

    if (chosen_phys == 0) {
        // fallback: se não encontrou contíguo, procure para bitmap apenas e frames_stack depois (menos ideal)
        // aqui optamos por panic para evitar comportamento indefinido
        panic("PMM: cannot find contiguous area for bitmap+stack");
    }

    pmm.bitmap_phys = chosen_phys;
    pmm.bitmap = (uint8_t *)PHYS_TO_VIRT(pmm.bitmap_phys);

    // frames_stack_phys logo após o bitmap
    uint64_t frames_stack_phys = chosen_phys + bitmap_size_bytes;
    pmm.frames_stack = (uint64_t *)PHYS_TO_VIRT(frames_stack_phys);
    pmm.stack_top = 0;

    klog(KLOG_DEBUG, "PMM: bitmap at phys=0x%llx virt=0x%llx (%llu bytes)",
         (unsigned long long)pmm.bitmap_phys,
         (unsigned long long)(uint64_t)pmm.bitmap,
         (unsigned long long)bitmap_size_bytes);
    klog(KLOG_DEBUG, "PMM: frames_stack at phys=0x%llx virt=0x%llx (%llu entries)",
         (unsigned long long)frames_stack_phys,
         (unsigned long long)(uint64_t)pmm.frames_stack,
         (unsigned long long)FRAMES_STACK_ENTRIES);

    // 5) probe rápido para garantir que a memória é acessível (evita triple fault imediata)
    volatile uint8_t *probe = (volatile uint8_t *)pmm.bitmap;
    uint8_t saved = probe[0];
    probe[0] = saved ^ 0xAA;
    probe[0] = saved;
    klog(KLOG_DEBUG, "PMM: bitmap probe OK");

    // 6) inicializar bitmap como tudo usado (1)
    memset(pmm.bitmap, 0xFF, (size_t)bitmap_size_bytes);

    // 7) marcar regiões USABLE como livres (0)
    pmm.free_pages = 0;
    for (size_t i = 0; i < mmap->entry_count; i++) {
        struct limine_memmap_entry *e = mmap->entries[i];
        if (e->type == LIMINE_MEMMAP_USABLE) {
            uint64_t start_page = e->base / PMM_PAGE_SIZE;
            uint64_t end_page = (e->base + e->length) / PMM_PAGE_SIZE;
            for (uint64_t pg = start_page; pg < end_page; pg++) {
                bitmap_set(pg, false);
                pmm.free_pages++;
            }
            // log mínimo para evitar muitos prints
            klog(KLOG_DEBUG, "PMM: region 0x%llx-0x%llx free (%llu pages)",
                 (unsigned long long)e->base,
                 (unsigned long long)(e->base + e->length),
                 (unsigned long long)(end_page - start_page));
        }
    }

    // 8) marcar bitmap e frames_stack como usados
    uint64_t bm_start_page = pmm.bitmap_phys / PMM_PAGE_SIZE;
    uint64_t bm_end_page = (pmm.bitmap_phys + bitmap_size_bytes + PMM_PAGE_SIZE - 1) / PMM_PAGE_SIZE;
    for (uint64_t pg = bm_start_page; pg < bm_end_page; pg++) {
        if (!bitmap_get(pg)) {
            bitmap_set(pg, true);
            pmm.free_pages--;
        }
    }

    uint64_t fs_start_page = frames_stack_phys / PMM_PAGE_SIZE;
    uint64_t fs_end_page = (frames_stack_phys + frames_stack_pages * PMM_PAGE_SIZE) / PMM_PAGE_SIZE;
    for (uint64_t pg = fs_start_page; pg < fs_end_page; pg++) {
        if (!bitmap_get(pg)) {
            bitmap_set(pg, true);
            pmm.free_pages--;
        }
    }

    // 9) marcar kernel reservado (ajuste conforme necessário)
    // Se o seu kernel não estiver exatamente neste range, ajuste kernel_start/kernel_end.
    // Alternativamente, recolha essa informação do linker script.
    uint64_t kernel_start = 0x100000ULL;   // físico
    uint64_t kernel_end   = 0x200000ULL;   // físico - ajustar conforme seu kernel
    uint64_t ks = kernel_start / PMM_PAGE_SIZE;
    uint64_t ke = (kernel_end + PMM_PAGE_SIZE - 1) / PMM_PAGE_SIZE;
    for (uint64_t pg = ks; pg < ke; pg++) {
        if (!bitmap_get(pg)) {
            bitmap_set(pg, true);
            pmm.free_pages--;
        }
    }

    klog(KLOG_INFO, "PMM: ready. free pages=%llu (%llu MB)",
         (unsigned long long)pmm.free_pages,
         (unsigned long long)((pmm.free_pages * PMM_PAGE_SIZE) / 1024 / 1024));
}

/* pmalloc: retorna endereço VIRTUAL (PHYS_TO_VIRT) */
void *pmalloc(size_t pages) {
    if (pages == 0) return NULL;

    spinlock_lock(&pmm_lock);

    // Fast path: single page from stack cache
    if (pages == 1 && pmm.stack_top > 0) {
        uint64_t phys = stack_pop();
        if (phys != UINT64_MAX) {
            pmm.free_pages--;
            void *v = PHYS_TO_VIRT(phys * PMM_PAGE_SIZE);
            spinlock_unlock(&pmm_lock);
            return v;
        }
    }

    // Procurar run livre
    uint64_t start_page = find_free_run(pages);
    if (start_page == UINT64_MAX) {
        spinlock_unlock(&pmm_lock);
        klog(KLOG_ERROR, "PMM: out of memory requesting %llu pages", (unsigned long long)pages);
        return NULL;
    }

    // Marcar como usado
    for (uint64_t i = start_page; i < start_page + pages; i++) {
        bitmap_set(i, true);
    }
    pmm.free_pages -= pages;

    uint64_t phys_addr = start_page * PMM_PAGE_SIZE;
    void *virt = PHYS_TO_VIRT(phys_addr);

    // Se alocou blocos individuais, preencha cache com alguns frames próximos (opcional)
    if (pages == 1) {
        // nada adicional já que devolvemos uma
    }

    spinlock_unlock(&pmm_lock);
    return virt;
}

void *pmalloc_aligned(size_t pages, size_t alignment) {
    if (pages == 0) return NULL;
    if (alignment % PMM_PAGE_SIZE != 0) return NULL;

    spinlock_lock(&pmm_lock);

    uint64_t align_pages = alignment / PMM_PAGE_SIZE;

    // Simples scan alinhado: procurar start i tal que i % align_pages == 0
    uint64_t total = pmm.total_pages;
    for (uint64_t i = 0; i + pages <= total; i = ((i / align_pages) + 1) * align_pages) {
        bool ok = true;
        for (uint64_t j = 0; j < pages; j++) {
            if (bitmap_get(i + j)) { ok = false; break; }
        }
        if (ok) {
            for (uint64_t j = 0; j < pages; j++) bitmap_set(i + j, true);
            pmm.free_pages -= pages;
            uint64_t phys_addr = i * PMM_PAGE_SIZE;
            void *virt = PHYS_TO_VIRT(phys_addr);
            spinlock_unlock(&pmm_lock);
            return virt;
        }
    }

    spinlock_unlock(&pmm_lock);
    return NULL;
}

void pfree(void *ptr, size_t pages) {
    if (!ptr || pages == 0) return;

    spinlock_lock(&pmm_lock);

    uint64_t phys = VIRT_TO_PHYS(ptr);
    uint64_t start = phys / PMM_PAGE_SIZE;
    if (start >= pmm.total_pages) {
        klog(KLOG_ERROR, "PMM: free invalid phys=0x%llx", (unsigned long long)phys);
        spinlock_unlock(&pmm_lock);
        return;
    }

    // Verificar e liberar
    for (uint64_t i = 0; i < pages; i++) {
        uint64_t pg = start + i;
        if (!bitmap_get(pg)) {
            klog(KLOG_WARN, "PMM: double free or corruption at page %llu", (unsigned long long)pg);
            // ainda assim continuamos e garantimos que fique livre
        }
        bitmap_set(pg, false);

        // Se for single page, tentar empurrar para cache
        if (pages == 1) {
            // armazenamos o número da página física
            if (pmm.stack_top < FRAMES_STACK_ENTRIES) {
                pmm.frames_stack[pmm.stack_top++] = pg;
                // não decrementar free_pages aqui porque iremos incrementar abaixo
            }
        }
    }

    pmm.free_pages += pages;

    spinlock_unlock(&pmm_lock);
}

uint64_t pmm_get_free(void) {
    return pmm.free_pages * PMM_PAGE_SIZE;
}

uint64_t pmm_get_total(void) {
    return pmm.total_memory;
}

/* Marca uma região (base físico, size bytes) como used/free.
   base e size são físicos. */
void pmm_set_region(uint64_t base, uint64_t size, bool used) {
    uint64_t start = base / PMM_PAGE_SIZE;
    uint64_t end = (base + size + PMM_PAGE_SIZE - 1) / PMM_PAGE_SIZE;

    spinlock_lock(&pmm_lock);

    for (uint64_t pg = start; pg < end; pg++) {
        bool cur = bitmap_get(pg);
        if (cur != used) {
            bitmap_set(pg, used);
            if (used) {
                if (pmm.free_pages > 0) pmm.free_pages--;
            } else {
                pmm.free_pages++;
            }
        }
    }

    spinlock_unlock(&pmm_lock);
}

void pmm_dump(void) {
    klog(KLOG_INFO, "=== PMM DUMP ===");
    klog(KLOG_INFO, "Total memory: %llu MB", (unsigned long long)(pmm.total_memory / 1024 / 1024));
    klog(KLOG_INFO, "Free memory : %llu MB", (unsigned long long)(pmm_get_free() / 1024 / 1024));
    klog(KLOG_INFO, "Total pages : %llu, Free pages: %llu", (unsigned long long)pmm.total_pages, (unsigned long long)pmm.free_pages);
}
