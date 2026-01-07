#include "vmm.h"
#include "pmm.h"
#include "string.h"
#include "klog.h"
#include <limine.h>

/* Importa a requisição HHDM definida em limine_requests.c */
extern volatile struct limine_hhdm_request hhdm_request;

/* Variável global para armazenar o offset */
uint64_t g_hhdm_offset = 0;

/* O PML4 atual do kernel (acessível via Virtual HHDM) */
static pml4_t kernel_pml4 = NULL;

/* Macros de índices */
#define PML4_IDX(v) (((v) >> 39) & 0x1FF)
#define PDPT_IDX(v) (((v) >> 30) & 0x1FF)
#define PD_IDX(v)   (((v) >> 21) & 0x1FF)
#define PT_IDX(v)   (((v) >> 12) & 0x1FF)

/* Função auxiliar para navegar nas tabelas */
static uint64_t *get_next_level(uint64_t *curr_table, uint16_t idx, bool allocate) {
    if (curr_table[idx] & PTE_PRESENT) {
        // === CORREÇÃO CRÍTICA AQUI ===
        // Se a tabela já existe (ex: criada pelo Kernel/Limine), 
        // ela pode estar marcada como "Supervisor Only" (sem PTE_USER).
        // Precisamos adicionar PTE_USER | PTE_RW para que o Ring 3 
        // consiga "atravessar" esse diretório e chegar na página final.
        curr_table[idx] |= (PTE_USER);
        // ==============================

        uint64_t phys = curr_table[idx] & ~0xFFF;
        return (uint64_t *)PHYS_TO_VIRT(phys);
    }

    if (!allocate) return NULL;

    uint64_t new_table_phys = (uint64_t)pmalloc(1);
    uint64_t *new_table_virt = (uint64_t*)PHYS_TO_VIRT(new_table_phys);
    
    if (!new_table_virt) return NULL;

    memset(new_table_virt, 0, 4096);

    // Cria nova tabela já com permissão de usuário
    curr_table[idx] = new_table_phys | PTE_PRESENT | PTE_RW | PTE_USER;

    return new_table_virt;
}

void vmm_init(void) {
    // 1. Configura o Offset HHDM
    if (hhdm_request.response != NULL) {
        g_hhdm_offset = hhdm_request.response->offset;
        klog(0, "VMM: HHDM Offset set to 0x%llx", g_hhdm_offset);
    } else {
        klog(2, "VMM: HHDM Failed! Forcing default Limine offset.");
        g_hhdm_offset = 0xffff800000000000ULL;
    }

    // 2. Pega o CR3 atual (Endereço Físico do PML4 do Kernel)
    uint64_t cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    
    // Converte para virtual para podermos ler/copiar
    kernel_pml4 = (pml4_t)PHYS_TO_VIRT(cr3);
    
    klog(0, "VMM: Initialized. Kernel PML4 phys=0x%llx", cr3);
}

pml4_t vmm_new_address_space(void) {
    // Aloca físico para o novo PML4
    uint64_t pml4_phys = (uint64_t)pmalloc(1);
    
    // Converte para virtual para escrever
    pml4_t new_pml4 = (pml4_t)PHYS_TO_VIRT(pml4_phys);
    
    if (!new_pml4) return NULL;
    
    memset(new_pml4, 0, 4096);

    // CORREÇÃO: Copia TODO o mapa do kernel (0 a 512)
    // Isso garante que o Identity Map (Memória Baixa) e o Kernel (Memória Alta)
    // estejam acessíveis no novo processo.
    for (int i = 0; i < 512; i++) {
        if (kernel_pml4[i] & PTE_PRESENT) {
            new_pml4[i] = kernel_pml4[i];
        }
    }

    return new_pml4;
}

void vmm_map_page(pml4_t pml4, uint64_t virt, uint64_t phys, uint64_t flags) {
    // pml4 recebido já é um ponteiro virtual (HHDM)
    uint64_t *pml4_virt = (uint64_t*)pml4;
    
    uint64_t *pdpt = get_next_level(pml4_virt, PML4_IDX(virt), true);
    if (!pdpt) return;

    uint64_t *pd = get_next_level(pdpt, PDPT_IDX(virt), true);
    if (!pd) return;

    uint64_t *pt = get_next_level(pd, PD_IDX(virt), true);
    if (!pt) return;

    // Grava a entrada na Page Table (Nível 1)
    pt[PT_IDX(virt)] = (phys & ~0xFFF) | flags;
    
    // Invalida TLB
    asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

void vmm_switch_pml4(pml4_t pml4) {
    // pml4 é virtual (HHDM), precisamos converter para FÍSICO para o CR3
    uint64_t phys = VIRT_TO_PHYS(pml4);
    asm volatile("mov %0, %%cr3" : : "r"(phys) : "memory");
}

uint64_t vmm_virt_to_phys(pml4_t pml4, uint64_t virt) {
    uint64_t *table = (uint64_t*)pml4;
    
    uint16_t idx = PML4_IDX(virt);
    if (!(table[idx] & PTE_PRESENT)) return 0;
    table = (uint64_t*)PHYS_TO_VIRT(table[idx] & ~0xFFF);

    idx = PDPT_IDX(virt);
    if (!(table[idx] & PTE_PRESENT)) return 0;
    table = (uint64_t*)PHYS_TO_VIRT(table[idx] & ~0xFFF);

    idx = PD_IDX(virt);
    if (!(table[idx] & PTE_PRESENT)) return 0;
    table = (uint64_t*)PHYS_TO_VIRT(table[idx] & ~0xFFF);

    idx = PT_IDX(virt);
    if (!(table[idx] & PTE_PRESENT)) return 0;
    
    return (table[idx] & ~0xFFF) + (virt & 0xFFF);
}

