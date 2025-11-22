#include "process.h"
#include "../mem/kheap.h"
#include "../lib/string.h"
#include "../scheduler/task.h"
#include "../xldui/xldui.h"
#include "../mem/vmm.h"
#include "../mem/pmm.h"
#include "../mem/mem.h"

// Não precisamos mais do hhdm_offset aqui.
// extern uint64_t hhdm_offset;

#define MAX_PROCESSES 64
process_t process_table[MAX_PROCESSES];
pid_t next_pid = 1;

extern volatile task_t* current_task;

void process_init(void) {
    memset(process_table, 0, sizeof(process_table));
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_table[i].state = PROCESS_STATE_FREE;
        process_table[i].pml4 = NULL;
    }
}

process_t* find_process_by_task(task_t* task) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state != PROCESS_STATE_FREE && process_table[i].task == task) {
            return &process_table[i];
        }
    }
    return NULL;
}

// Função auxiliar para converter uint64_t para hex string
static void u64_to_hex(uint64_t value, char* str) {
    const char* hex_chars = "0123456789ABCDEF";
    str[0] = '0';
    str[1] = 'x';
    str[18] = '\0'; // Adiciona o terminador nulo
    for (int i = 15; i >= 0; i--) {
        str[2 + (15 - i)] = hex_chars[(value >> (i * 4)) & 0xF];
    }
}

pid_t create_process(void (*entry_point)(void), task_type_t type) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROCESS_STATE_FREE) {
            process_t* p = &process_table[i];
            
            p->pid = next_pid++;
            p->state = PROCESS_STATE_RUNNING;

            p->pml4 = vmm_create_map();
            if (p->pml4 == NULL) {
                p->state = PROCESS_STATE_FREE;
                return -1;
            }

            // =================== INÍCIO DA CORREÇÃO ===================
            // O cálculo do endereço físico estava incorreto.
            // O endereço virtual do kernel (entry_point) não está no HHDM.
            // A conversão correta usa a base virtual e física do próprio kernel.
            
            // Base virtual onde o código do kernel é linkado.
            // Em kernel/src/process/process.c

// ...
uint64_t kernel_virt_base = 0xFFFFFFFF80100000; // Corrigido para corresponder ao linker
uint64_t kernel_phys_base = 0x100000;
uint64_t entry_phys = ((uint64_t)entry_point - kernel_virt_base) + kernel_phys_base;
// ...

            // Onde o código do usuário será mapeado no espaço virtual do processo.
            uint64_t code_virt_start = 0x400000;

            // ==================== FIM DA CORREÇÃO =====================
            
            console_print("Creating process - Entry point: ");
            char addr_str[20];
            u64_to_hex((uint64_t)entry_point, addr_str);
            console_print(addr_str);
            
            console_print(", Physical: ");
            u64_to_hex(entry_phys, addr_str);
            console_print(addr_str);
            console_print("\n");
            
            // Alinha o endereço físico e virtual para o início da página.
            uint64_t phys_page_start = entry_phys & ~(PAGE_SIZE - 1);
            uint64_t page_offset = entry_phys & (PAGE_SIZE - 1);
            uint64_t virt_page_start = code_virt_start; // Mapeia no início da região de código.
            
            console_print("Mapping physical ");
            u64_to_hex(phys_page_start, addr_str);
            console_print(addr_str);
            console_print(" to virtual ");
            u64_to_hex(virt_page_start, addr_str);
            console_print(addr_str);
            console_print("\n");
            
            // Mapeia a página física que contém a função para o espaço virtual do processo.
            if (map_page(p->pml4, virt_page_start, phys_page_start, PTE_PRESENT | PTE_USER) != 0) {
                console_print("FAILED to map code page!\n");
                vmm_destroy_map(p->pml4);
                p->state = PROCESS_STATE_FREE;
                return -1;
            }
            
            // O ponto de entrada no espaço virtual do usuário é a base virtual + o offset dentro da página.
            uint64_t user_entry = virt_page_start + page_offset;

            // Mapeamento da pilha do usuário.
            void* stack_phys = pmm_alloc_page();
            if (stack_phys == NULL) {
                vmm_destroy_map(p->pml4);
                p->state = PROCESS_STATE_FREE;
                return -1;
            }
            
            uint64_t stack_virt_top = 0x700000000000;
            if (map_page(p->pml4, stack_virt_top - PAGE_SIZE, (uint64_t)stack_phys, 
                        PTE_PRESENT | PTE_WRITE | PTE_USER) != 0) {
                pmm_free_page(stack_phys);
                vmm_destroy_map(p->pml4);
                p->state = PROCESS_STATE_FREE;
                return -1;
            }

            // Cria a tarefa do kernel associada, apontando para o endereço de entrada do *usuário*.
            task_t* t = create_task((void (*)(void))user_entry, type);
            if (t == NULL) {
                pmm_free_page(stack_phys);
                vmm_destroy_map(p->pml4);
                p->state = PROCESS_STATE_FREE;
                return -1;
            }
            
            p->task = t;

            // Define o ponteiro da pilha (RSP) para o topo da pilha do usuário.
            if (type == TASK_USER) {
                t->cpu_state->rsp = stack_virt_top;
                
                console_print("User task created - RSP: ");
                u64_to_hex(stack_virt_top, addr_str);
                console_print(addr_str);
                console_print(", RIP: ");
                u64_to_hex(user_entry, addr_str);
                console_print(addr_str);
                console_print("\n");
            }

            return p->pid;
        }
    }
    return -1; // Nenhum slot de processo livre.
}

extern volatile task_t* task_list_head; // Precisamos disso

// ... outras funções ...

void process_exit(void) {
    asm("cli"); // Desabilita interrupções para evitar condições de corrida

    process_t* p = find_process_by_task((task_t*)current_task);
    if (p == NULL) {
        // Tarefa do kernel tentando sair? Ou erro? Pare tudo.
        console_set_color(0xFF0000);
        console_print("\nKERNEL PANIC: exit() called on non-process task!\n");
        for(;;) asm("cli; hlt");
    }

    // 1. Libere os recursos do processo (mapa de memória, etc.)
    if (p->pml4) {
        vmm_destroy_map(p->pml4);
    }

    // 2. Remova a tarefa da lista do escalonador
    remove_task(p->task);

    // 3. Libere a memória da tarefa
    kfree(p->task->stack); // Libera a pilha do kernel da tarefa
    kfree(p->task);       // Libera a estrutura da tarefa

    // 4. Marque o slot do processo como livre
    p->task = NULL;
    p->pml4 = NULL;
    p->state = PROCESS_STATE_FREE;

    // 5. NÃO RETORNE! Force uma troca de contexto para a próxima tarefa disponível.
    // A função schedule NUNCA será chamada com o estado da tarefa morta.
    
    // Pega a próxima tarefa (que agora é a cabeça da lista ou outra)
    cpu_state_t* next_state = current_task->cpu_state;
    
    // Carrega o mapa de memória do próximo processo (se for um processo)
    process_t* next_process = find_process_by_task((task_t*)current_task);
    switch_map(next_process ? next_process->pml4 : NULL);

    // Pula manualmente para o código assembly que restaura o estado da PRÓXIMA tarefa.
    // Isso é como um 'longjmp' do kernel.
    asm volatile (
        // Restaura todos os registradores de propósito geral
        "mov %0, %%rsp\n\t"
        "popq %%rax\n\t"
        "popq %%rbx\n\t"
        "popq %%rcx\n\t"
        "popq %%rdx\n\t"
        "popq %%rsi\n\t"
        "popq %%rdi\n\t"
        "popq %%rbp\n\t"
        "popq %%r8\n\t"
        "popq %%r9\n\t"
        "popq %%r10\n\t"
        "popq %%r11\n\t"
        "popq %%r12\n\t"
        "popq %%r13\n\t"
        "popq %%r14\n\t"
        "popq %%r15\n\t"

        // Envia EOI (End of Interrupt) para o PIC Master
        "movb $0x20, %%al\n\t"  // Coloca o valor 0x20 no registrador AL (8-bit)
        "outb %%al, $0x20\n\t"  // Envia o byte de AL para a porta 0x20

        // Salta para a próxima tarefa
        "iretq"
        : 
        : "r"(next_state) // Entrada: o ponteiro para o estado da próxima tarefa
        : "memory", "rax" // Clobbers: memória e o registrador 'rax' (por causa do movb)
    );
    // Este código nunca é alcançado
    for(;;);
}
void get_process_info(int index, pid_t* pid, process_state_t* state) {
    if (index < 0 || index >= MAX_PROCESSES) {
        *pid = -1;
        return;
    }
    *pid = process_table[index].pid;
    *state = process_table[index].state;
}

