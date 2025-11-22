#include "unistd.h"

void _exit(int status) {
    (void)status;
    asm volatile("mov %0, %%rax; int $0x80" : : "i"(SYS_EXIT) : "rax");
    for(;;);
}

ssize_t read(int fd, void *buf, size_t count) {
    ssize_t bytes_read;
    asm volatile(
        "mov %1, %%rax\n\t"
        "mov %2, %%rdi\n\t"
        "mov %3, %%rsi\n\t"
        "mov %4, %%rdx\n\t"
        "int $0x80"
        : "=a"(bytes_read)
        : "i"(SYS_READ), "D"((long)fd), "S"(buf), "d"(count)
        : "rcx", "r11"
    );
    return bytes_read;
}

ssize_t write(int fd, const void *buf, size_t count) {
    ssize_t bytes_written;
    asm volatile(
        "mov %1, %%rax\n\t"
        "mov %2, %%rdi\n\t"
        "mov %3, %%rsi\n\t"
        "mov %4, %%rdx\n\t"
        "int $0x80"
        : "=a"(bytes_written)
        : "i"(SYS_WRITE), "D"((long)fd), "S"(buf), "d"(count)
        : "rcx", "r11"
    );
    return bytes_written;
}

void clear(uint32_t color) {
    // =================== CORREÇÃO 3 ===================
    // Removido "rax" e "rdi" da lista de clobber.
    asm volatile(
        "mov %0, %%rax\n\t"
        "mov %1, %%rdi\n\t"
        "int $0x80"
        :
        : "i"(SYS_CLEAR), "D"((uint64_t)color)
        : "rcx", "r11"
    );
    // ==================================================
}

// =================== CORREÇÃO 2 ===================
// Renomeado para spawn_process
pid_t spawn_process(void (*entry)(void)) {
    uint64_t pid;
    // Removido "rdi" da lista de clobber
    asm volatile(
        "mov %1, %%rax\n\t"
        "mov %2, %%rdi\n\t"
        "int $0x80"
        : "=a"(pid)
        : "i"(SYS_SPAWN_PROCESS), "D"(entry)
        : "rcx", "r11"
    );
    return (pid_t)pid;
}
// ==================================================

