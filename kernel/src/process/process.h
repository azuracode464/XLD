#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include "../scheduler/task.h" // Incluído para task_type_t
#include "../mem/vmm.h"      // Incluído para pml4_t

// Enum para o estado de um processo
typedef enum {
    PROCESS_STATE_FREE,     // O slot na tabela de processos está livre
    PROCESS_STATE_RUNNING,  // O processo está pronto para ser escalonado ou está rodando
    PROCESS_STATE_SLEEPING, // O processo está esperando por um evento (não implementado)
    PROCESS_STATE_ZOMBIE    // O processo terminou, mas ainda não foi limpo (não implementado)
} process_state_t;

// Define o tipo para o ID do processo (Process ID)
typedef int pid_t;

// A estrutura principal que representa um processo no sistema.
// Ela agrupa todos os recursos que pertencem a um processo.
typedef struct {
    pid_t pid;              // ID único do processo.
    process_state_t state;  // Estado atual do processo (livre, rodando, etc.).
    task_t* task;           // Ponteiro para a tarefa do kernel associada (contexto da CPU, pilha).
    
    // =================== A GRANDE MUDANÇA ===================
    // Ponteiro para o mapa de memória de nível 4 (PML4) deste processo.
    // Cada processo tem seu próprio universo de memória virtual.
    pml4_t* pml4;
    // ========================================================

    // Futuramente, podemos adicionar mais campos aqui, como:
    // - Lista de descritores de arquivo abertos.
    // - Diretório de trabalho atual.
    // - Informações sobre o processo pai.
    // - etc.
} process_t;

// --- Protótipos das Funções de Gerenciamento de Processos ---

// Inicializa a tabela de processos.
void process_init(void);

// Cria um novo processo.
pid_t create_process(void (*entry_point)(void), task_type_t type);

// Termina o processo atual.
void process_exit(void);

// Constante que define o número máximo de processos no sistema.
#define MAX_PROCESSES 64

// Obtém informações sobre um processo (usado para depuração, etc.).
void get_process_info(int index, pid_t* pid, process_state_t* state);
process_t* find_process_by_task(task_t* task);
#endif // PROCESS_H

