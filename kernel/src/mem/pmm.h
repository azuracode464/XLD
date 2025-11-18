#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "mem.h"

void pmm_init(volatile struct limine_memmap_request *memmap_request);
void* pmm_alloc_page(void);
void pmm_free_page(void* page_addr);
void* pmm_alloc_pages(size_t num_pages);
void pmm_free_pages(void* start_addr, size_t num_pages);

#endif // PMM_H

