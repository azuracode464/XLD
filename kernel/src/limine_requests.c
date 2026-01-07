#include <limine.h>

/* Memmap request - garantimos que fique em .limine.request e não seja otimizado */
volatile struct limine_memmap_request memmap_request
    __attribute__((section(".limine.request"), used)) = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0
};
