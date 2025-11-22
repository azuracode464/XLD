
#include "task.h"
#include "../mem/kheap.h"
#include "../lib/string.h"
#include "../mem/vmm.h"
#include "../process/process.h"
#include "../xldui/xldui.h"
#include "../lib/util.h"   // <--- ADICIONE ESTA LINHA
// Defina o tamanho da pilha do kernel para cada tarefa. 4KB é um bom começo.
#define STACK_SIZE 4096

// --- Variáveis Globais do Escalonador ---
volatile task_t *current_task;
volatile task_t *task_list_head;
int next_task_id = 0;

// Ponteiro para o processo atualmente em execução.
static process_t* current_process = NULL;

// --- Funções do Escalonador ---

/**
 * @brief Inicializa o sistema de multitarefa.
 * Cria a tarefa inicial do kernel (idle task).
 */
void tasking_init() {
    asm volatile("cli");

    // Cria a tarefa do kernel (representando o kmain/idle loop).
    task_t *kernel_task = kmalloc(sizeof(task_t));
    kernel_task->id = next_task_id++;
    kernel_task->stack = NULL;      // A pilha do kernel já existe, não alocamos uma nova.
    kernel_task->cpu_state = NULL;  // O estado será salvo na primeira troca de contexto.
    kernel_task->type = TASK_KERNEL;
    kernel_task->next = kernel_task; // Lista circular com apenas um elemento.

    task_list_head = kernel_task;
    current_task = kernel_task;

    asm volatile("sti");
}

/**
 * @brief Cria uma nova tarefa (contexto de CPU e pilha do kernel).
 * Esta é a versão padrão que usa kmalloc.
 */
task_t* create_task(void (*entry_point)(void), task_type_t type) {
    asm volatile("cli");

    task_t *new_task = kmalloc(sizeof(task_t));
    if (new_task == NULL) {
        asm volatile("sti");
        return NULL;
    }

    new_task->id = next_task_id++;
    new_task->stack = kmalloc(STACK_SIZE); // Pilha do KERNEL para esta tarefa
    if (new_task->stack == NULL) {
        kfree(new_task);
        asm volatile("sti");
        return NULL;
    }
    
    uint64_t stack_top = (uint64_t)new_task->stack + STACK_SIZE;
    cpu_state_t *state = (cpu_state_t*)(stack_top - sizeof(cpu_state_t));
    memset(state, 0, sizeof(cpu_state_t));

    // Configura o estado inicial da CPU para a nova tarefa.
    state->rip = (uint64_t)entry_point;
    state->rflags = 0x202; // Habilita interrupções

    if (type == TASK_USER) {
        state->cs = 0x23; // Seletor de Código de Usuário
        state->ss = 0x1B; // Seletor de Dados de Usuário
    } else {
        state->cs = 0x08; // Seletor de Código do Kernel
        state->ss = 0x10; // Seletor de Dados do Kernel
        state->rsp = stack_top;
    }

    new_task->cpu_state = state;
    new_task->type = type;

    // Adiciona a nova tarefa à lista circular.
    if (task_list_head == NULL) {
        task_list_head = new_task;
        new_task->next = new_task;
    } else {
        task_t *tail = (task_t*)task_list_head;
        while(tail->next != task_list_head) {
            tail = tail->next;
        }
        tail->next = new_task;
        new_task->next = (struct task*)task_list_head;
    }

    asm volatile("sti");
    return new_task;
}

/**
 * @brief O coração do multitarefa. Salva o estado atual, escolhe a próxima tarefa
 * e troca o espaço de endereço virtual, se necessário.
 */
__attribute__((optimize("O0")))
cpu_state_t* schedule(cpu_state_t *current_state) {
    if (!current_task) return current_state;

    // 1. Salva o estado da tarefa atual.
    current_task->cpu_state = current_state;

    // 2. Encontra a próxima tarefa na lista circular.
    current_task = current_task->next;
    if (!current_task) {
        current_task = task_list_head;
    }

    // 3. Encontra o processo correspondente à nova tarefa.
    process_t* next_process = find_process_by_task((task_t*)current_task);

    // 4. Verifica se o mapa de memória precisa ser trocado.
    if (next_process != current_process) {
        // Se next_process for NULL (tarefa do kernel), switch_map carregará o mapa do kernel.
        switch_map(next_process ? next_process->pml4 : NULL);
        current_process = next_process;
    }
    // Em schedule()
// ... antes de return current_task->cpu_state;

if (current_task->type == TASK_USER) {
    console_print("\n-- Switching to User Task --\n");
    char buf[20];
    u64_to_hex(current_task->cpu_state->rip, buf); console_print("RIP: "); console_print(buf);
    u64_to_hex(current_task->cpu_state->cs, buf); console_print(" CS: "); console_print(buf);
    u64_to_hex(current_task->cpu_state->ss, buf); console_print(" SS: "); console_print(buf);
    u64_to_hex(current_task->cpu_state->rsp, buf); console_print(" RSP: "); console_print(buf);
    console_print("\n--------------------------\n");
}

return current_task->cpu_state;


}

/**
 * @brief Remove uma tarefa da lista de execução.
 */
void remove_task(task_t* task_to_remove) {
    if (task_to_remove == NULL || task_list_head == NULL) {
        return;
    }

    // Encontra a tarefa anterior (prev)
    task_t* prev = (task_t*)task_list_head;
    while (prev->next != task_list_head && prev->next != task_to_remove) {
        prev = prev->next;
    }

    // Se a tarefa a ser removida for encontrada
    if (prev->next == task_to_remove) {
        // Se a tarefa a ser removida for a cabeça da lista
        if (task_to_remove == task_list_head) {
            // Se for a única tarefa na lista
            if (task_list_head->next == task_list_head) {
                task_list_head = NULL;
                current_task = NULL; // Não há mais tarefas
                return;
            }
            // Avança a cabeça da lista
            task_list_head = task_list_head->next;
        }
        
        // Remove o elo da corrente
        prev->next = task_to_remove->next;

        // Se estávamos executando a tarefa que acabamos de remover,
        // defina a tarefa atual para a próxima tarefa válida na lista.
        if (current_task == task_to_remove) {
            current_task = prev->next;
        }
    }
}
