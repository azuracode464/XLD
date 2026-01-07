#pragma once
#include <stdint.h>
#include "vmm.h"

typedef struct process {
    int pid;
    pml4_t pml4;        // Tabela de páginas física/virtual
    void *kernel_stack; // Pilha do kernel (RSP0) para syscalls
    uint64_t rip;       // Entry point
    uint64_t rsp;       // User stack pointer
} process_t;

/* Carrega um binário SBF e cria o processo */
process_t* process_create_from_sbf(const char *path);

/* Pula para execução do processo */
void process_run(process_t *proc);

