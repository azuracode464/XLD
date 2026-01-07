#include <limine.h>

/* Memmap request - garantimos que fique em .limine.request e não seja otimizado */
volatile struct limine_memmap_request memmap_request
    __attribute__((section(".limine.request"), used)) = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0
};
/* Requisição HHDM (Higher Half Direct Map) */
volatile struct limine_hhdm_request hhdm_request 
    __attribute__((section(".limine.request"), used)) = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0
};
