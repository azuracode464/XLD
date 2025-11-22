#include "kheap.h"
#include "pmm.h"
#include "mem.h"
#include "../lib/string.h"
#include <stdbool.h>

// O HHDM offset, definido em main.c
extern uint64_t hhdm_offset;

// --- Estruturas e Variáveis Globais ---

// O cabeçalho que precede cada bloco de memória (seja ele usado ou livre).
typedef struct header {
    bool is_free;      // Este bloco está livre?
    size_t size;       // Tamanho da área de DADOS (não inclui este cabeçalho).
    struct header *next; // Próximo bloco na lista ligada.
    struct header *prev; // Bloco anterior na lista ligada.
} header_t;

// Ponteiros para o início e o fim da nossa lista de blocos de memória.
static header_t *head = NULL;
static header_t *tail = NULL;

// --- Funções Auxiliares ---

/**
 * @brief Expande o heap pedindo mais memória ao PMM.
 * @param min_size O tamanho mínimo necessário para a expansão.
 */
static void kheap_expand(size_t min_size) {
    // Pede pelo menos min_size, mas arredondado para cima para um múltiplo de PAGE_SIZE.
    size_t size_to_alloc = (min_size + sizeof(header_t) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    size_t num_pages = size_to_alloc / PAGE_SIZE;

    void* new_mem_phys = pmm_alloc_pages(num_pages);
    if (!new_mem_phys) {
        // Falha catastrófica: PMM não tem mais memória.
        // Em um kernel real, isso geraria um pânico ou um erro OOM (Out Of Memory).
        return;
    }

    header_t* new_block = (header_t*)((uint64_t)new_mem_phys + hhdm_offset);
    new_block->is_free = true;
    new_block->size = size_to_alloc - sizeof(header_t);
    new_block->next = NULL;

    if (head == NULL) {
        // A lista está vazia, este é o primeiro bloco.
        head = new_block;
        tail = new_block;
        new_block->prev = NULL;
    } else {
        // Adiciona o novo bloco ao final da lista.
        tail->next = new_block;
        new_block->prev = tail;
        tail = new_block;
    }
}

/**
 * @brief Tenta fundir um bloco livre com seus vizinhos (se eles também estiverem livres).
 * @param block O bloco que acabou de ser liberado.
 */
static void kheap_merge(header_t* block) {
    // Tenta fundir com o próximo bloco.
    if (block->next && block->next->is_free) {
        block->size += block->next->size + sizeof(header_t);
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        } else {
            // Se o próximo bloco era o 'tail', agora 'block' é o novo 'tail'.
            tail = block;
        }
    }

    // Tenta fundir com o bloco anterior.
    if (block->prev && block->prev->is_free) {
        block->prev->size += block->size + sizeof(header_t);
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        } else {
            // Se 'block' era o 'tail', agora o anterior é o novo 'tail'.
            tail = block->prev;
        }
    }
}

// --- Funções Públicas ---

/**
 * @brief Inicializa o gerenciador de heap.
 */
void kheap_init(void) {
    head = NULL;
    tail = NULL;
    // Começa com um heap inicial de 256KB.
    kheap_expand(256 * 1024);
}

/**
 * @brief Aloca um bloco de memória.
 * @param size O número de bytes a serem alocados.
 * @return Ponteiro para a memória alocada, ou NULL se não houver memória.
 */
void* kmalloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    // Percorre a lista de blocos procurando por um bloco livre que seja grande o suficiente.
    header_t *current = head;
    while (current) {
        if (current->is_free && current->size >= size) {
            // Bloco encontrado! Agora, vamos ver se vale a pena dividi-lo.
            // A condição de divisão: o espaço restante deve ser grande o suficiente
            // para conter um cabeçalho e pelo menos 16 bytes de dados.
            if (current->size > size + sizeof(header_t) + 16) {
                // Vamos dividir o bloco.
                header_t *new_free_block = (header_t*)((char*)current + sizeof(header_t) + size);
                new_free_block->is_free = true;
                new_free_block->size = current->size - size - sizeof(header_t);
                new_free_block->next = current->next;
                new_free_block->prev = current;

                if (current->next) {
                    current->next->prev = new_free_block;
                } else {
                    tail = new_free_block; // O novo bloco é o último da lista.
                }

                current->size = size;
                current->next = new_free_block;
            }

            // Marca o bloco como usado e retorna o ponteiro para a área de dados.
            current->is_free = false;
            return (void*)((char*)current + sizeof(header_t));
        }
        current = current->next;
    }

    // Nenhum bloco livre encontrado. Precisamos expandir o heap.
    kheap_expand(size);
    // Tenta alocar novamente (recursivamente).
    // Isso é seguro porque kheap_expand adiciona um bloco grande o suficiente.
    return kmalloc(size);
}

/**
 * @brief Libera um bloco de memória previamente alocado.
 * @param ptr Ponteiro para a memória a ser liberada.
 */
void kfree(void* ptr) {
    if (ptr == NULL) {
        return;
    }

    // Obtém o cabeçalho do bloco a partir do ponteiro de dados.
    header_t* block_header = (header_t*)((char*)ptr - sizeof(header_t));
    
    // Verifica se o bloco já não está livre (double free).
    if (block_header->is_free) {
        // Poderíamos gerar um pânico aqui. Por enquanto, apenas ignoramos.
        return;
    }

    block_header->is_free = true;

    // Tenta fundir o bloco com seus vizinhos para combater a fragmentação.
    kheap_merge(block_header);
}

