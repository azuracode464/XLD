#ifndef TASK_H
#define TASK_H

#include <stdint.h>

// Estrutura que representa o estado completo da CPU salvo na pilha
typedef struct {
    // Registradores salvos pelo nosso stub
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    // Registradores salvos pela CPU na interrupção (ou simulados por nós)
    uint64_t rip, cs, rflags, rsp, ss;
} cpu_state_t;

// Enum para o tipo de tarefa
typedef enum {
    TASK_KERNEL,
    TASK_USER
} task_type_t;

// Estrutura da tarefa
typedef struct task {
    cpu_state_t *cpu_state;
    int id;
    void *stack;
    task_type_t type; // Novo campo para diferenciar as tarefas
    struct task *next;
} task_t;

// Protótipos de função
void tasking_init(void);
task_t* create_task(void (*entry_point)(void), task_type_t type);
void remove_task(task_t* task_to_remove);
cpu_state_t* schedule(cpu_state_t *current_state);

#endif // TASK_H

