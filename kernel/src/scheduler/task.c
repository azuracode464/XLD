#include "task.h"
#include <stddef.h>

#define MAX_TASKS 10
#define STACK_SIZE 4096

volatile task_t *current_task;
volatile task_t *task_list_head;
int next_task_id = 0;

task_t task_pool[MAX_TASKS];
char stack_pool[MAX_TASKS][STACK_SIZE];
int tasks_created = 0;

cpu_state_t* schedule(cpu_state_t *current_state) {
    if (!current_task) return current_state;

    // 1. Salva o ponteiro do estado atual na estrutura da tarefa
    current_task->cpu_state = current_state;

    // 2. Escolhe a próxima tarefa
    current_task = current_task->next;
    if (!current_task) {
        current_task = task_list_head;
    }

    // 3. Retorna o ponteiro do estado da próxima tarefa
    return current_task->cpu_state;
}

void tasking_init() {
    asm volatile("cli");

    task_t *kernel_task = &task_pool[tasks_created++];
    kernel_task->id = next_task_id++;
    kernel_task->stack = NULL;
    kernel_task->cpu_state = NULL; // O estado do kernel será salvo na primeira interrupção
    kernel_task->next = kernel_task;

    task_list_head = kernel_task;
    current_task = kernel_task;

    asm volatile("sti");
}

task_t* create_task(void (*entry_point)(void)) {
    if (tasks_created >= MAX_TASKS) return NULL;

    asm volatile("cli");

    task_t *new_task = &task_pool[tasks_created];
    new_task->id = next_task_id++;
    new_task->stack = &stack_pool[tasks_created][0];
    
    // Aponta para o topo da pilha
    cpu_state_t *state = (cpu_state_t*)((uint64_t)new_task->stack + STACK_SIZE - sizeof(cpu_state_t));

    // Zera todos os registradores para um estado limpo
    for (int i = 0; i < sizeof(cpu_state_t) / sizeof(uint64_t); i++) {
        ((uint64_t*)state)[i] = 0;
    }

    // Configura os registradores essenciais para o iretq
    state->rip = (uint64_t)entry_point;
    state->cs = 0x28;
    state->rflags = 0x202; // IF = 1 (habilita interrupções)
    state->rsp = (uint64_t)state; // A pilha da tarefa começa aqui
    state->ss = 0x30;

    new_task->cpu_state = state;

    // Adiciona à lista de tarefas
    new_task->next = (struct task*)task_list_head;
    task_t *tail = (task_t*)task_list_head;
    while(tail->next != task_list_head) {
        tail = tail->next;
    }
    tail->next = new_task;

    tasks_created++;
    asm volatile("sti");
    return new_task;
}

