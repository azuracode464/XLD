#ifndef VMM_H
#define VMM_H

#include <stdint.h>
#include <stddef.h>

// --- Constantes da Arquitetura ---

// Uma tabela de páginas em qualquer nível (PML4, PDPT, PD, PT) contém 512 entradas.
#define PAGE_TABLE_ENTRIES 512

// --- Flags de Permissão para Entradas da Tabela de Páginas (PTEs) ---
// Estas flags são combinadas usando o operador OR bit a bit.

#define PTE_PRESENT   (1ULL << 0)  // A página (ou tabela) está presente na memória.
#define PTE_WRITE     (1ULL << 1)  // A página é gravável. Se não, é somente leitura.
#define PTE_USER      (1ULL << 2)  // A página é acessível pelo modo usuário (Ring 3). Se não, só pelo kernel.
#define PTE_NO_EXEC   (1ULL << 63) // A página não pode conter código executável (requer suporte da CPU).

// --- Estruturas das Tabelas de Páginas ---
// Cada tabela é um array de 512 entradas de 64 bits, alinhado em 4KB.

typedef uint64_t pte_t; // Page Table Entry (a unidade fundamental)

// Definimos os tipos para cada nível para clareza semântica.
typedef pte_t pd_entry_t; // Page Directory Entry
typedef pte_t pdpt_entry_t; // Page Directory Pointer Table Entry
typedef pte_t pml4_entry_t; // Page Map Level 4 Entry

// As tabelas em si são arrays alinhados.
typedef struct {
    pte_t entries[PAGE_TABLE_ENTRIES];
} __attribute__((aligned(4096))) pt_t;

typedef struct {
    pd_entry_t entries[PAGE_TABLE_ENTRIES];
} __attribute__((aligned(4096))) pd_t;

typedef struct {
    pdpt_entry_t entries[PAGE_TABLE_ENTRIES];
} __attribute__((aligned(4096))) pdpt_t;

typedef struct {
    pml4_entry_t entries[PAGE_TABLE_ENTRIES];
} __attribute__((aligned(4096))) pml4_t;


// --- Protótipos das Funções do VMM ---

/**
 * @brief Inicializa o VMM, salvando o mapa de memória do kernel.
 */
void vmm_init(void);

/**
 * @brief Cria um novo espaço de endereço (um novo PML4) para um processo.
 * @return Ponteiro VIRTUAL para o novo PML4, ou NULL em caso de falha.
 */
pml4_t* vmm_create_map(void);

/**
 * @brief Mapeia uma página virtual para uma página física em um dado espaço de endereço.
 * @param pml4_virt O ponteiro VIRTUAL para o PML4 a ser modificado.
 * @param virt_addr O endereço virtual a ser mapeado.
 * @param phys_addr O endereço físico para o qual o virtual deve apontar.
 * @param flags As flags de permissão (PTE_PRESENT, PTE_WRITE, PTE_USER).
 * @return 0 em caso de sucesso, -1 em caso de falha.
 */
int map_page(pml4_t* pml4_virt, uintptr_t virt_addr, uintptr_t phys_addr, uint64_t flags);

/**
 * @brief Desmapeia uma página virtual, invalidando sua entrada na tabela de páginas.
 * @param pml4_virt O ponteiro VIRTUAL para o PML4 do processo.
 * @param virt_addr O endereço virtual a ser desmapeado.
 */
void unmap_page(pml4_t* pml4_virt, uintptr_t virt_addr);

/**
 * @brief Destrói um espaço de endereço, liberando todas as suas páginas e tabelas.
 * @param pml4_virt O ponteiro VIRTUAL para o PML4 a ser destruído.
 */
void vmm_destroy_map(pml4_t* pml4_virt);

/**
 * @brief Troca o espaço de endereço atual, carregando um novo PML4 no registrador CR3.
 * @param pml4 O ponteiro VIRTUAL para o mapa de memória a ser ativado.
 */
void switch_map(pml4_t* pml4);

#endif // VMM_H

