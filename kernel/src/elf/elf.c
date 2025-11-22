// ==================================================================
// INCLUDES CORRIGIDOS
// ==================================================================
#include "elf.h"
#include "../process/process.h"
#include "../mem/vmm.h"
#include "../mem/pmm.h"
#include "../mem/mem.h"      // Para PAGE_SIZE
#include "../lib/string.h"
#include "../xldui/xldui.h"  // Para console_print

// Variáveis globais do gerenciador de processos, que precisamos acessar
extern process_t process_table[MAX_PROCESSES];
extern pid_t next_pid;

// Constantes
#define USER_STACK_PAGES 16 // 64KB de pilha para o processo de usuário

#define PF_X 1  // Segmento é executável
#define PF_W 2  // Segmento é gravável
#define PF_R 4  // Segmento é legível (geralmente implícito)
#define PTE_PRESENT   (1ULL << 0)
#define PTE_WRITE     (1ULL << 1)
#define PTE_USER      (1ULL << 2)
// ✅ ADICIONE ESTA LINHA
#define PTE_NX        (1ULL << 63) // No-Execute bit (requer suporte da CPU)
// Função auxiliar para converter flags de segmento ELF para flags de tabela de páginas
static inline uint64_t elf_flags_to_pte(uint32_t flags) {
    uint64_t pte_flags = PTE_PRESENT | PTE_USER;
    if (flags & PF_W) {
        pte_flags |= PTE_WRITE;
    }
    if (!(flags & PF_X)) {
        pte_flags |= PTE_NX;
    }
    return pte_flags;
}
pid_t elf_load_process(void* elf_data) {
    Elf64_Ehdr* hdr = (Elf64_Ehdr*)elf_data;

    // 1. Validação: É um arquivo ELF64 válido?
    if (memcmp(hdr->e_ident, "\x7F" "ELF", 4) != 0) {
        console_print("ELF Loader: Invalid ELF magic number!\n");
        return -1;
    }

    // 2. Encontrar um slot de processo livre
    int slot = -1;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROCESS_STATE_FREE) {
            slot = i;
            break;
        }
    }
    if (slot < 0) {
        console_print("ELF Loader: No free process slots available!\n");
        return -1;
    }

    process_t* p = &process_table[slot];
    p->pid = next_pid++;
    p->state = PROCESS_STATE_RUNNING;

    // 3. Criar um novo espaço de endereço virtual (PML4)
    p->pml4 = vmm_create_map();
    if (!p->pml4) {
        console_print("ELF Loader: Failed to create VMM map (PML4)!\n");
        p->state = PROCESS_STATE_FREE;
        return -1;
    }

    // 4. Iterar sobre os Program Headers para carregar os segmentos na memória
    Elf64_Phdr* phdr = (Elf64_Phdr*)((uint64_t)hdr + hdr->e_phoff);
    for (int i = 0; i < hdr->e_phnum; i++) {
        // Processa apenas segmentos carregáveis (PT_LOAD)
        if (phdr[i].p_type != PT_LOAD) {
            continue;
        }

        uint64_t vaddr = phdr[i].p_vaddr;
        uint64_t memsz = phdr[i].p_memsz;
        uint64_t filesz = phdr[i].p_filesz;
        uint64_t file_offset = phdr[i].p_offset;
        uint64_t pte_flags = elf_flags_to_pte(phdr[i].p_flags);

        // Itera sobre cada página que o segmento ocupa
        for (uint64_t offset_in_segment = 0; offset_in_segment < memsz; offset_in_segment += PAGE_SIZE) {
            uint64_t current_vaddr = vaddr + offset_in_segment;
            
            void* phys_page = pmm_alloc_page();
            if (!phys_page) {
                console_print("ELF Loader: Out of physical memory for segment!\n");
                // TODO: Limpeza completa do processo (liberar páginas já alocadas)
                vmm_destroy_map(p->pml4);
                p->state = PROCESS_STATE_FREE;
                return -1;
            }

            // Mapeia a página física no espaço virtual do processo
            map_page(p->pml4, current_vaddr & ~(PAGE_SIZE - 1), (uint64_t)phys_page, pte_flags);

            // Calcula quanto copiar para esta página
            uint64_t bytes_to_copy = 0;
            if (offset_in_segment < filesz) {
                bytes_to_copy = filesz - offset_in_segment;
                if (bytes_to_copy > PAGE_SIZE) {
                    bytes_to_copy = PAGE_SIZE;
                }
                memcpy(phys_page, (uint8_t*)hdr + file_offset + offset_in_segment, bytes_to_copy);
            }

            // Zera o restante da página (para o .bss ou para o final de uma página parcial)
            if (bytes_to_copy < PAGE_SIZE) {
                memset((uint8_t*)phys_page + bytes_to_copy, 0, PAGE_SIZE - bytes_to_copy);
            }
        }
    }

    // 5. Alocar e mapear a pilha do usuário
    uint64_t stack_top = 0x800000000000ULL;
    for (int i = 0; i < USER_STACK_PAGES; i++) {
        void* phys_stack_page = pmm_alloc_page();
        if (!phys_stack_page) {
            console_print("ELF Loader: Out of memory for stack!\n");
            // TODO: Limpeza
            return -1;
        }
        uint64_t stack_page_vaddr = stack_top - ((i + 1) * PAGE_SIZE);
        map_page(p->pml4, stack_page_vaddr, (uint64_t)phys_stack_page, PTE_PRESENT | PTE_WRITE | PTE_USER);
    }

    // 6. Criar a tarefa do kernel associada
    task_t* t = create_task((void (*)(void))hdr->e_entry, TASK_USER);
    if (!t) {
        console_print("ELF Loader: Failed to create task!\n");
        // TODO: Limpeza
        return -1;
    }

    t->cpu_state->rsp = stack_top;
    p->task = t;
    // A associação do PML4 à tarefa será feita pelo scheduler quando ele
    // encontrar o processo correspondente à tarefa.

    console_print("ELF process loaded successfully.\n");
    return p->pid;
}

