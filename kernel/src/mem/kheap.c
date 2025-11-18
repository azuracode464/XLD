#include "kheap.h"
#include "pmm.h"
#include <stddef.h>
#include <stdbool.h>

typedef struct free_block {
    struct free_block *next;
} free_block_t;

typedef struct page_header {
    int bucket_index;
    size_t size;
} page_header_t;

#define BUCKET_COUNT 8
const size_t bucket_sizes[BUCKET_COUNT] = {16, 32, 64, 128, 256, 512, 1024, 2048};
static free_block_t* free_lists[BUCKET_COUNT];

#define MAX_BUCKET_SIZE bucket_sizes[BUCKET_COUNT - 1]

void kheap_init(void) {
    for (int i = 0; i < BUCKET_COUNT; i++) {
        free_lists[i] = NULL;
    }
}

static void expand_bucket(int bucket_index) {
    uint8_t* page = pmm_alloc_page();
    if (page == NULL) {
        return;
    }

    page_header_t* header = (page_header_t*)page;
    header->bucket_index = bucket_index;

    size_t block_size = bucket_sizes[bucket_index];
    int block_count = (PAGE_SIZE - sizeof(page_header_t)) / block_size;
    uint8_t* first_block = page + sizeof(page_header_t);

    for (int i = 0; i < block_count; i++) {
        free_block_t* block = (free_block_t*)(first_block + i * block_size);
        block->next = free_lists[bucket_index];
        free_lists[bucket_index] = block;
    }
}

void* kmalloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    if (size > MAX_BUCKET_SIZE) {
        size_t num_pages = (size + sizeof(page_header_t) + PAGE_SIZE - 1) / PAGE_SIZE;
        uint8_t* page = pmm_alloc_pages(num_pages);
        if (page == NULL) {
            return NULL;
        }
        page_header_t* header = (page_header_t*)page;
        header->bucket_index = -1;
        header->size = num_pages;
        return (void*)(page + sizeof(page_header_t));
    }

    int bucket_index = 0;
    while (bucket_sizes[bucket_index] < size) {
        bucket_index++;
    }

    if (free_lists[bucket_index] == NULL) {
        expand_bucket(bucket_index);
        if (free_lists[bucket_index] == NULL) {
            return NULL;
        }
    }

    free_block_t* block = free_lists[bucket_index];
    free_lists[bucket_index] = block->next;
    return (void*)block;
}

void kfree(void* ptr) {
    if (ptr == NULL) {
        return;
    }

    uint64_t page_addr = (uint64_t)ptr & ~(PAGE_SIZE - 1);
    page_header_t* header = (page_header_t*)page_addr;

    if (header->bucket_index != -1) {
        int bucket_index = header->bucket_index;
        free_block_t* block = (free_block_t*)ptr;
        block->next = free_lists[bucket_index];
        free_lists[bucket_index] = block;
    } else {
        size_t num_pages = header->size;
        pmm_free_pages((void*)page_addr, num_pages);
    }
}

