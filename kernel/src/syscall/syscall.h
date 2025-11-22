#ifndef SYSCALL_H
#define SYSCALL_H

#include "../scheduler/task.h"

// Números das Syscalls
#define SYS_EXIT            1
#define SYS_READ            3
#define SYS_WRITE           4
#define SYS_CLEAR           5
#define SYS_SPAWN_PROCESS   6 // Renomeado para consistência

void syscall_handler(cpu_state_t *state);

#endif // SYSCALL_H

