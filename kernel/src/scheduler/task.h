#ifndef TASK_H
#define TASK_H

#include <stdint.h>

// Estrutura que representa o estado completo da CPU salvo na pilha
typedef struct {
    // Registradores salvos pelo nosso stub
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    // Registradores salvos pela CPU na interrupção
    uint64_t rip, cs, rflags, rsp, ss;
} cpu_state_t;

typedef struct task {
    cpu_state_t *cpu_state; // Ponteiro para o estado salvo na pilha da tarefa
    int id;
    void *stack; // Ponteiro para a base da pilha (para desalocação futura)
    struct task *next;
} task_t;

void tasking_init(void);
task_t* create_task(void (*entry_point)(void));

// Nova assinatura: recebe o ponteiro do estado atual e retorna o do próximo
cpu_state_t* schedule(cpu_state_t *current_state);

#endif // TASK_H

