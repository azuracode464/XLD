// src/kmalloc.c
#include "kmalloc.h"
#include "pmm.h"
#include "string.h"    // sua implementação
#include "spinlock.h"

// Se klog não estiver definido, defina aqui também
#ifndef KLOG_INFO
#define KLOG_INFO 0
#define KLOG_WARN 1
#define KLOG_ERROR 2
#define KLOG_DEBUG 3
#endif

// Declaração externa das funções de log que já existem
extern void klog(int level, const char *fmt, ...);
extern void printk(const char *fmt, ...);

// DEFINA BLOCK_MAGIC aqui no .c (não no .h!)
const uint8_t BLOCK_MAGIC[3] = {'K', 'M', 'B'};

#define KMALLOC_POOL_PAGES 4  // 16KB por pool
#define POOL_COUNT 4          // 4 pools = 64KB heap inicial

static spinlock_t kmalloc_lock = SPINLOCK_INIT;
static kmalloc_block_t *free_list = NULL;
static uint64_t heap_start = 0;
static uint64_t heap_end = 0;
// Inicializar um novo pool de memória
static void add_pool(void) {
    void *pool = pmalloc(KMALLOC_POOL_PAGES);
    if (!pool) {
        klog(KLOG_ERROR, "kmalloc: Failed to allocate new pool");
        return;
    }
    
    uint64_t pool_addr = (uint64_t)pool;
    size_t pool_size = KMALLOC_POOL_PAGES * PMM_PAGE_SIZE;
    
    if (heap_start == 0) {
        heap_start = pool_addr;
        heap_end = pool_addr + pool_size;
    } else {
        heap_end += pool_size;
    }
    
    // Criar bloco livre inicial
    kmalloc_block_t *block = (kmalloc_block_t *)pool_addr;
    block->size = pool_size - BLOCK_HEADER_SIZE;
    block->next = free_list;
    block->used = 0;
    memcpy(block->magic, BLOCK_MAGIC, 3);
    
    free_list = block;
    
    klog(KLOG_DEBUG, "kmalloc: Added pool at 0x%x (%d KB)", 
         pool_addr, (int)(pool_size / 1024));
}

static void split_block(kmalloc_block_t *block, size_t size) {
    if (block->size < size + BLOCK_HEADER_SIZE + KMALLOC_MIN_SIZE)
        return;
    
    kmalloc_block_t *new_block = (kmalloc_block_t *)
        ((uint8_t *)block + BLOCK_HEADER_SIZE + size);
    
    new_block->size = block->size - size - BLOCK_HEADER_SIZE;
    new_block->next = block->next;
    new_block->used = 0;
    memcpy(new_block->magic, BLOCK_MAGIC, 3);
    
    block->size = size;
    block->next = new_block;
}

void kmalloc_init(void) {
    // Alocar pools iniciais
    for (int i = 0; i < POOL_COUNT; i++) {
        add_pool();
    }
    
    klog(KLOG_INFO, "kmalloc: Heap initialized at 0x%x-0x%x", 
         heap_start, heap_end);
}

void *kmalloc(size_t size) {
    if (size == 0) return NULL;
    
    // Alinhar para 16 bytes
    size = (size + KMALLOC_ALIGN - 1) & ~(KMALLOC_ALIGN - 1);
    if (size < KMALLOC_MIN_SIZE) size = KMALLOC_MIN_SIZE;
    
    spinlock_lock(&kmalloc_lock);
    
    // Procurar bloco livre (first-fit)
    kmalloc_block_t *prev = NULL;
    kmalloc_block_t *curr = free_list;
    
    while (curr) {
        if (!curr->used && curr->size >= size) {
            // Verificar magic
            if (memcmp(curr->magic, BLOCK_MAGIC, 3) != 0) {
                klog(KLOG_ERROR, "kmalloc: Block magic corrupted at 0x%x", curr);
                spinlock_unlock(&kmalloc_lock);
                return NULL;
            }
            
            // Dividir bloco se for muito grande
            split_block(curr, size);
            
            // Marcar como usado
            curr->used = 1;
            
            // Remover da lista livre
            if (prev) {
                prev->next = curr->next;
            } else {
                free_list = curr->next;
            }
            
            spinlock_unlock(&kmalloc_lock);
            
            void *ptr = (void *)((uint8_t *)curr + BLOCK_HEADER_SIZE);
            
            klog(KLOG_DEBUG, "kmalloc: Allocated %d bytes at 0x%x", 
                 (int)size, (uint64_t)ptr);
            
            return ptr;
        }
        
        prev = curr;
        curr = curr->next;
    }
    
    // Nenhum bloco livre encontrado, alocar novo pool
    spinlock_unlock(&kmalloc_lock);
    add_pool();
    
    // Tentar novamente
    return kmalloc(size);
}

void *kcalloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    void *ptr = kmalloc(total);
    
    if (ptr) {
        memset(ptr, 0, total);
    }
    
    return ptr;
}

void *krealloc(void *ptr, size_t size) {
    if (!ptr) return kmalloc(size);
    if (size == 0) {
        kfree(ptr);
        return NULL;
    }
    
    kmalloc_block_t *block = (kmalloc_block_t *)
        ((uint8_t *)ptr - BLOCK_HEADER_SIZE);
    
    // Verificar magic
    if (memcmp(block->magic, BLOCK_MAGIC, 3) != 0) {
        klog(KLOG_ERROR, "krealloc: Invalid block at 0x%x", ptr);
        return NULL;
    }
    
    // Se o bloco atual já é grande o suficiente
    if (block->size >= size) {
        return ptr;
    }
    
    // Alocar novo, copiar dados, liberar antigo
    void *new_ptr = kmalloc(size);
    if (!new_ptr) return NULL;
    
    memcpy(new_ptr, ptr, block->size);  // Copia menos que o tamanho original
    kfree(ptr);
    
    return new_ptr;
}

void kfree(void *ptr) {
    if (!ptr) return;
    
    kmalloc_block_t *block = (kmalloc_block_t *)
        ((uint8_t *)ptr - BLOCK_HEADER_SIZE);
    
    // Verificar magic
    if (memcmp(block->magic, BLOCK_MAGIC, 3) != 0) {
        klog(KLOG_ERROR, "kfree: Invalid block at 0x%x (double free?)", ptr);
        return;
    }
    
    spinlock_lock(&kmalloc_lock);
    
    if (!block->used) {
        klog(KLOG_WARN, "kfree: Double free detected at 0x%x", ptr);
        spinlock_unlock(&kmalloc_lock);
        return;
    }
    
    block->used = 0;
    
    // Coalescing: fundir com blocos adjacentes livres
    kmalloc_block_t *curr = free_list;
    kmalloc_block_t *prev = NULL;
    
    // Inserir ordenado por endereço
    while (curr && curr < block) {
        prev = curr;
        curr = curr->next;
    }
    
    // Inserir na posição correta
    block->next = curr;
    if (prev) {
        prev->next = block;
    } else {
        free_list = block;
    }
    
    // Coalesce com próximo
    if (block->next && 
        (uint8_t *)block + BLOCK_HEADER_SIZE + block->size == 
        (uint8_t *)block->next) {
        
        block->size += BLOCK_HEADER_SIZE + block->next->size;
        block->next = block->next->next;
    }
    
    // Coalesce com anterior
    if (prev && 
        (uint8_t *)prev + BLOCK_HEADER_SIZE + prev->size == 
        (uint8_t *)block) {
        
        prev->size += BLOCK_HEADER_SIZE + block->size;
        prev->next = block->next;
    }
    
    spinlock_unlock(&kmalloc_lock);
    
    klog(KLOG_DEBUG, "kfree: Freed block at 0x%x (%d bytes)", 
         (uint64_t)ptr, (int)block->size);
}

size_t kmalloc_usable_size(void *ptr) {
    if (!ptr) return 0;
    
    kmalloc_block_t *block = (kmalloc_block_t *)
        ((uint8_t *)ptr - BLOCK_HEADER_SIZE);
    
    if (memcmp(block->magic, BLOCK_MAGIC, 3) != 0) {
        return 0;
    }
    
    return block->size;
}

void kmalloc_dump(void) {
    spinlock_lock(&kmalloc_lock);
    
    klog(KLOG_INFO, "=== KMALLOC DUMP ===");
    klog(KLOG_INFO, "Heap: 0x%x-0x%x", heap_start, heap_end);
    
    kmalloc_block_t *curr = free_list;
    int free_count = 0;
    size_t free_total = 0;
    
    while (curr) {
        if (!curr->used) {
            free_count++;
            free_total += curr->size;
        }
        curr = curr->next;
    }
    
    klog(KLOG_INFO, "Free blocks: %d, Free memory: %d bytes", 
         free_count, (int)free_total);
    
    // Opcional: listar todos os blocos
    #ifdef KMALLOC_DEBUG
    curr = (kmalloc_block_t *)heap_start;
    while ((uint64_t)curr < heap_end) {
        klog(KLOG_INFO, "  Block 0x%x: size=%d, used=%d", 
             (uint64_t)curr, (int)curr->size, curr->used);
        curr = (kmalloc_block_t *)((uint8_t *)curr + BLOCK_HEADER_SIZE + curr->size);
    }
    #endif
    
    spinlock_unlock(&kmalloc_lock);
}
