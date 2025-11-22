#include "syscall.h"
#include "../idt/idt.h"
#include "../xldui/xldui.h"
#include "../lib/string.h"
#include "../process/process.h"
#include "../drivers/keyboard.h"

// --- Protótipos das funções de syscall ---

/**
 * @brief Syscall SYS_READ (3): Lê dados de um descritor de arquivo.
 */
static void sys_read(cpu_state_t *state) {
    int fd = (int)state->rdi;
    char *buf = (char *)state->rsi;
    size_t count = (size_t)state->rdx;
    size_t bytes_read = 0;

    if (fd == 0) { // stdin
        for (size_t i = 0; i < count; i++) {
            buf[i] = getchar();
            bytes_read++;
            if (buf[i] == '\n') {
                break;
            }
        }
    } else {
        bytes_read = (size_t)-1;
    }

    state->rax = bytes_read;
}

/**
 * @brief Syscall SYS_WRITE (4): Escreve dados em um descritor de arquivo.
 */
static void sys_write(cpu_state_t *state) {
    int fd = (int)state->rdi;
    const char* user_buf = (const char*)state->rsi;
    size_t count = (size_t)state->rdx;

    if (fd == 1) { // stdout
        char kernel_buf[1024];
        size_t bytes_to_copy = count < (sizeof(kernel_buf) - 1) ? count : (sizeof(kernel_buf) - 1);
        
        // Copia do buffer do usuário para o buffer do kernel.
        // Se isso falhar, um Page Fault será gerado, e agora o handler será chamado.
        memcpy(kernel_buf, user_buf, bytes_to_copy);
        kernel_buf[bytes_to_copy] = '\0';

        console_print(kernel_buf);

        state->rax = bytes_to_copy;
    } else {
        state->rax = (uint64_t)-1;
    }
}

/**
 * @brief Syscall SYS_EXIT (1): Termina o processo atual.
 */
static void sys_exit(cpu_state_t *state) {
    (void)state; // status não é usado
    process_exit();
}

/**
 * @brief Syscall SYS_SPAWN_PROCESS (6): Cria um novo processo.
 */
static void sys_spawn(cpu_state_t *state) {
    void (*entry)(void) = (void (*)(void))state->rdi;
    pid_t pid = create_process(entry, TASK_USER);
    state->rax = (uint64_t)pid;
}

/**
 * @brief Syscall SYS_CLEAR (5): Limpa a tela.
 */
static void sys_clear(cpu_state_t *state) {
    uint32_t color = (uint32_t)state->rdi;
    console_clear(color);
    state->rax = 0;
}


// --- O Handler Principal de Syscalls ---

void syscall_handler(cpu_state_t *state) {
/*    uint64_t syscall_num = state->rax;

    // Mensagem de diagnóstico (opcional, mas útil)
    // Como as interrupções já estão desabilitadas aqui, é seguro imprimir.
    console_set_color(0x00AAAA);
    if (syscall_num == SYS_WRITE) {
        console_print("[W]");
    } else if (syscall_num == SYS_EXIT) {
        console_print("[E]");
    } else {
        console_print("[?]");
    }
    console_set_color(0xFFFF00);

    switch (syscall_num) {
        case SYS_READ:
            sys_read(state);
            break;
        case SYS_WRITE:
            sys_write(state);
            break;
        case SYS_EXIT:
            sys_exit(state);
            break;
        case SYS_SPAWN_PROCESS:
            sys_spawn(state);
            break;
        case SYS_CLEAR:
            sys_clear(state);
            break;
        default:
            state->rax = (uint64_t)-1;
            break;
    }*/
return;
}

