#ifndef _UNISTD_H
#define _UNISTD_H

#include <stdint.h>
#include <stddef.h>
#include "../process/process.h"

// =================== CORREÇÃO 1 ===================
// Definir ssize_t, pois não está nos nossos headers padrão.
// Em x86-64, size_t é 'unsigned long', então ssize_t é 'long'.
typedef long ssize_t;
// ==================================================

// Números das Syscalls
#define SYS_EXIT            1
#define SYS_READ            3
#define SYS_WRITE           4
#define SYS_CLEAR           5
#define SYS_SPAWN_PROCESS   6 // Renomeado para evitar conflito

// Funções padrão POSIX
void _exit(int status);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);

// Funções customizadas
void clear(uint32_t color);
// =================== CORREÇÃO 2 ===================
// Renomeado para evitar conflito com a função do kernel.
pid_t spawn_process(void (*entry)(void));
// ==================================================

#endif // _UNISTD_H

