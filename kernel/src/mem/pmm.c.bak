#include "pmm.h"
#include "../xldui/xldui.h"
#include "../lib/string.h"
#include <stdbool.h>

static uint8_t* pmm_bitmap = NULL;
static uint64_t pmm_total_pages = 0;
static uint64_t last_checked_page = 0;

static void bitmap_set(uint64_t page) {
    if (page >= pmm_total_pages) return;
    pmm_bitmap[page / 8] |= (1 << (page % 8));
}

static void bitmap_clear(uint64_t page) {
    if (page >= pmm_total_pages) return;
    pmm_bitmap[page / 8] &= ~(1 << (page % 8));
}

static bool bitmap_test(uint64_t page) {
    if (page >= pmm_total_pages) return true;
    return (pmm_bitmap[page / 8] & (1 << (page % 8))) != 0;
}

void pmm_init(volatile struct limine_memmap_request *memmap_request) {
    struct limine_memmap_entry **mmap = memmap_request->response->entries;
    uint64_t mmap_entry_count = memmap_request->response->entry_count;
    uint64_t highest_addr = 0;

    for (uint64_t i = 0; i < mmap_entry_count; i++) {
        uint64_t top = mmap[i]->base + mmap[i]->length;
        if (top > highest_addr) {
            highest_addr = top;
        }
    }
    pmm_total_pages = highest_addr / PAGE_SIZE;
    uint64_t bitmap_size = pmm_total_pages / 8 + 1;

    for (uint64_t i = 0; i < mmap_entry_count; i++) {
        if (mmap[i]->type == LIMINE_MEMMAP_USABLE && mmap[i]->length >= bitmap_size) {
            pmm_bitmap = (uint8_t*)mmap[i]->base;
            goto bitmap_found;
        }
    }
    console_set_color(0xFF0000);
    console_print("FATAL: Could not find space for PMM bitmap!\n");
    for(;;) asm("hlt");

bitmap_found:
    memset(pmm_bitmap, 0xFF, bitmap_size);

    for (uint64_t i = 0; i < mmap_entry_count; i++) {
        if (mmap[i]->type == LIMINE_MEMMAP_USABLE) {
            for (uint64_t j = 0; j < mmap[i]->length; j += PAGE_SIZE) {
                bitmap_clear((mmap[i]->base + j) / PAGE_SIZE);
            }
        }
    }
}

void* pmm_alloc_page(void) {
    for (uint64_t page = last_checked_page; page < pmm_total_pages; page++) {
        if (!bitmap_test(page)) {
            bitmap_set(page);
            last_checked_page = page + 1;
            void* addr = (void*)(page * PAGE_SIZE);
            memset(addr, 0, PAGE_SIZE);
            return addr;
        }
    }
    for (uint64_t page = 0; page < last_checked_page; page++) {
        if (!bitmap_test(page)) {
            bitmap_set(page);
            last_checked_page = page + 1;
            void* addr = (void*)(page * PAGE_SIZE);
            memset(addr, 0, PAGE_SIZE);
            return addr;
        }
    }
    return NULL;
}

void pmm_free_page(void* page_addr) {
    uint64_t page = (uint64_t)page_addr / PAGE_SIZE;
    if (page >= pmm_total_pages) return;
    bitmap_clear(page);
}

void* pmm_alloc_pages(size_t num_pages) {
    if (num_pages == 0) return NULL;
    if (num_pages == 1) return pmm_alloc_page();

    size_t consecutive = 0;
    for (uint64_t page = 0; page < pmm_total_pages; page++) {
        if (!bitmap_test(page)) {
            consecutive++;
        } else {
            consecutive = 0;
        }

        if (consecutive >= num_pages) {
            uint64_t start_page = page - num_pages + 1;
            for (size_t i = 0; i < num_pages; i++) {
                bitmap_set(start_page + i);
            }
            void* addr = (void*)(start_page * PAGE_SIZE);
            memset(addr, 0, num_pages * PAGE_SIZE);
            return addr;
        }
    }
    return NULL;
}

void pmm_free_pages(void* start_addr, size_t num_pages) {
    uint64_t start_page = (uint64_t)start_addr / PAGE_SIZE;
    for (size_t i = 0; i < num_pages; i++) {
        bitmap_clear(start_page + i);
    }
}

