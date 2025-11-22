#include "vmm.h"
#include "pmm.h"
#include "../lib/string.h"
#include <limine.h>

// --- Variáveis Globais ---

// O mapa de memória do kernel (PML4). Será nosso mapa base.
static pml4_t* kernel_pml4 = NULL;

// O Limine nos fornece o offset do HHDM (Higher Half Direct Map).
extern uint64_t hhdm_offset;

// --- Funções Auxiliares ---

/**
 * @brief Carrega um novo PML4 no registrador CR3.
 * Isso efetivamente troca o espaço de endereço virtual ativo.
 */
void switch_map(pml4_t* pml4) {
    // O endereço passado para o CR3 deve ser o endereço FÍSICO do PML4.
    // Se pml4 for NULL, recarregamos o mapa do kernel.
    uint64_t pml4_phys = pml4 ? ((uint64_t)pml4 - hhdm_offset) : ((uint64_t)kernel_pml4 - hhdm_offset);
    asm volatile("mov %0, %%cr3" : : "r"(pml4_phys));
}

/**
 * @brief Zera uma tabela de páginas (4KB).
 */
static void clear_page_table(void* table) {
    memset(table, 0, PAGE_SIZE);
}

/**
 * @brief Extrai o endereço físico de uma entrada de tabela (PTE).
 */
static uintptr_t pte_get_addr(pte_t pte) {
    return pte & 0x000FFFFFFFFFF000;
}

// --- Implementação do VMM ---

/**
 * @brief Mapeia uma página virtual para uma página física em um dado mapa de memória.
 * 
 * @param pml4_virt O ponteiro VIRTUAL para o PML4 a ser modificado.
 * @param virt_addr O endereço virtual a ser mapeado.
 * @param phys_addr O endereço físico para o qual o virtual deve apontar.
 * @param flags As flags de permissão (PTE_PRESENT, PTE_WRITE, PTE_USER, PTE_NX).
 * @return 0 em caso de sucesso, -1 em caso de falha (falta de memória).
 */
int map_page(pml4_t* pml4_virt, uintptr_t virt_addr, uintptr_t phys_addr, uint64_t flags) {
    uint64_t pml4_index = (virt_addr >> 39) & 0x1FF;
    uint64_t pdpt_index = (virt_addr >> 30) & 0x1FF;
    uint64_t pd_index   = (virt_addr >> 21) & 0x1FF;
    uint64_t pt_index   = (virt_addr >> 12) & 0x1FF;

    pdpt_t* pdpt;
    if (!(pml4_virt->entries[pml4_index] & PTE_PRESENT)) {
        uint64_t pdpt_phys = (uint64_t)pmm_alloc_page();
        if (!pdpt_phys) return -1;
        pdpt = (pdpt_t*)(pdpt_phys + hhdm_offset);
        clear_page_table(pdpt);
        pml4_virt->entries[pml4_index] = pdpt_phys | PTE_PRESENT | PTE_WRITE | PTE_USER;
    } else {
        if (flags & PTE_USER) {
            pml4_virt->entries[pml4_index] |= PTE_USER;
        }
        pdpt = (pdpt_t*)(pte_get_addr(pml4_virt->entries[pml4_index]) + hhdm_offset);
    }

    pd_t* pd;
    if (!(pdpt->entries[pdpt_index] & PTE_PRESENT)) {
        uint64_t pd_phys = (uint64_t)pmm_alloc_page();
        if (!pd_phys) return -1;
        pd = (pd_t*)(pd_phys + hhdm_offset);
        clear_page_table(pd);
        pdpt->entries[pdpt_index] = pd_phys | PTE_PRESENT | PTE_WRITE | PTE_USER;
    } else {
        if (flags & PTE_USER) {
            pdpt->entries[pdpt_index] |= PTE_USER;
        }
        pd = (pd_t*)(pte_get_addr(pdpt->entries[pdpt_index]) + hhdm_offset);
    }

    pt_t* pt;
    if (!(pd->entries[pd_index] & PTE_PRESENT)) {
        uint64_t pt_phys = (uint64_t)pmm_alloc_page();
        if (!pt_phys) return -1;
        pt = (pt_t*)(pt_phys + hhdm_offset);
        clear_page_table(pt);
        pd->entries[pd_index] = pt_phys | PTE_PRESENT | PTE_WRITE | PTE_USER;
    } else {
        if (flags & PTE_USER) {
            pd->entries[pd_index] |= PTE_USER;
        }
        pt = (pt_t*)(pte_get_addr(pd->entries[pd_index]) + hhdm_offset);
    }

    pt->entries[pt_index] = phys_addr | flags;

    return 0;
}

/**
 * @brief Cria um novo espaço de endereço (mapa de memória).
 */
pml4_t* vmm_create_map(void) {
    uint64_t new_pml4_phys = (uint64_t)pmm_alloc_page();
    if (!new_pml4_phys) return NULL;
    pml4_t* new_pml4_virt = (pml4_t*)(new_pml4_phys + hhdm_offset);
    clear_page_table(new_pml4_virt);

    // Copia os mapeamentos do kernel (higher-half) para o novo mapa.
    // Isso garante que o kernel continue acessível em qualquer espaço de endereço.
    for (int i = 256; i < 512; i++) {
        new_pml4_virt->entries[i] = kernel_pml4->entries[i];
    }

    return new_pml4_virt;
}

/**
 * @brief Inicializa o Gerenciador de Memória Virtual (VMM).
 */
void vmm_init(void) {
    uint64_t current_pml4_phys;
    asm volatile("mov %%cr3, %0" : "=r"(current_pml4_phys));
    kernel_pml4 = (pml4_t*)(current_pml4_phys + hhdm_offset);
}

/**
 * @brief Desmapeia uma página virtual.
 */
void unmap_page(pml4_t* pml4_virt, uintptr_t virt_addr) {
    uint64_t pml4_index = (virt_addr >> 39) & 0x1FF;
    uint64_t pdpt_index = (virt_addr >> 30) & 0x1FF;
    uint64_t pd_index   = (virt_addr >> 21) & 0x1FF;
    uint64_t pt_index   = (virt_addr >> 12) & 0x1FF;

    if (!(pml4_virt->entries[pml4_index] & PTE_PRESENT)) return;
    pdpt_t* pdpt = (pdpt_t*)(pte_get_addr(pml4_virt->entries[pml4_index]) + hhdm_offset);

    if (!(pdpt->entries[pdpt_index] & PTE_PRESENT)) return;
    pd_t* pd = (pd_t*)(pte_get_addr(pdpt->entries[pdpt_index]) + hhdm_offset);

    if (!(pd->entries[pd_index] & PTE_PRESENT)) return;
    pt_t* pt = (pt_t*)(pte_get_addr(pd->entries[pd_index]) + hhdm_offset);

    if (pt->entries[pt_index] & PTE_PRESENT) {
        pt->entries[pt_index] = 0;
        // Invalida a página no TLB (Translation Lookaside Buffer)
        asm volatile("invlpg (%0)" : : "b"(virt_addr) : "memory");
    }
}

/**
 * @brief Destrói um espaço de endereço, liberando todas as suas páginas e tabelas.
 */
void vmm_destroy_map(pml4_t* pml4_virt) {
    // Percorre as entradas do PML4 que não são do kernel (0 a 255)
    for (int i = 0; i < 256; i++) {
        if (pml4_virt->entries[i] & PTE_PRESENT) {
            pdpt_t* pdpt = (pdpt_t*)(pte_get_addr(pml4_virt->entries[i]) + hhdm_offset);
            for (int j = 0; j < 512; j++) {
                if (pdpt->entries[j] & PTE_PRESENT) {
                    pd_t* pd = (pd_t*)(pte_get_addr(pdpt->entries[j]) + hhdm_offset);
                    for (int k = 0; k < 512; k++) {
                        if (pd->entries[k] & PTE_PRESENT) {
                            pt_t* pt = (pt_t*)(pte_get_addr(pd->entries[k]) + hhdm_offset);
                            for (int l = 0; l < 512; l++) {
                                if ((pt->entries[l] & PTE_PRESENT) && (pt->entries[l] & PTE_USER)) {
                                    // Libera a página de dados/pilha do usuário
                                    pmm_free_page((void*)pte_get_addr(pt->entries[l]));
                                }
                            }
                            // Libera a própria tabela de páginas (PT)
                            pmm_free_page((void*)((uint64_t)pt - hhdm_offset));
                        }
                    }
                    // Libera o diretório de páginas (PD)
                    pmm_free_page((void*)((uint64_t)pd - hhdm_offset));
                }
            }
            // Libera a tabela de ponteiros de diretório de página (PDPT)
            pmm_free_page((void*)((uint64_t)pdpt - hhdm_offset));
        }
    }
    // Finalmente, libera o próprio PML4
    pmm_free_page((void*)((uint64_t)pml4_virt - hhdm_offset));
}

