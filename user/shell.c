#include <stdint.h>
#include <stddef.h>

// user/shell.c

// Para syscalls, embora não as usaremos neste teste de loop infinito
long write(int fd, const void *buf, size_t count);
void _exit(int status);

// user/shell.c
void _start() {
    const char *message = "Hello from User Mode ELF!\n";
    write(1, message, 26);
    _exit(0);
}
// ... (resto do arquivo com as definições de write e _exit)

// Implementações stub para as syscalls (serão substituídas pelo kernel)
// Elas precisam estar aqui para o compilador não reclamar, mas o kernel
// interceptará as chamadas 'int $0x80'.
long write(int fd, const void *buf, size_t count) {
    long ret;
    __asm__ volatile(
        "mov %1, %%rax\n\t"
        "mov %2, %%rdi\n\t"
        "mov %3, %%rsi\n\t"
        "mov %4, %%rdx\n\t"
        "int $0x80"
        : "=a"(ret)
        : "i"(4), "D"((long)fd), "S"(buf), "d"(count)
        : "rcx", "r11"
    );
    return ret;
}

void _exit(int status) {
    __asm__ volatile(
        "mov %0, %%rax\n\t"
        "mov %1, %%rdi\n\t"
        "int $0x80"
        : /* Sem saídas */
        : "i"(1), "D"((long)status)
        : "rcx", "r11"
    );
    for(;;); // Garante que a função não retorne
}

