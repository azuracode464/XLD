#pragma once
#include <stdint.h>
#include <stdbool.h>

/* Flags de Paginação (Mantenha as que já existem) */
#define PTE_PRESENT   (1ULL << 0)
#define PTE_RW        (1ULL << 1)
#define PTE_USER      (1ULL << 2)
#define PTE_NX        (1ULL << 63)

/* Definição de HHDM - O valor real será lido da resposta do Limine */
extern uint64_t g_hhdm_offset; // Variável global que criaremos

/* Macros de conversão */
#define PHYS_TO_VIRT(phys) ((void*)((uint64_t)(phys) + g_hhdm_offset))
#define VIRT_TO_PHYS(virt) ((uint64_t)(virt) - g_hhdm_offset)

typedef uint64_t *pml4_t;

void vmm_init(void);
pml4_t vmm_new_address_space(void);
void vmm_map_page(pml4_t pml4, uint64_t virt, uint64_t phys, uint64_t flags);
void vmm_switch_pml4(pml4_t pml4);
uint64_t vmm_virt_to_phys(pml4_t pml4, uint64_t virt);

