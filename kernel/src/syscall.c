#include <stdint.h>
#include "klog.h"

// Lista de syscalls
#define SYS_WRITE 1
#define SYS_EXIT  2

#include <stdint.h>
#include "klog.h"

// Lista de syscalls
#define SYS_PRINT 1
#define SYS_EXIT  2

void syscall_handler(uint64_t sys_num, uint64_t arg1) {
    // DEBUG: Mostra que a syscall foi recebida e quais argumentos vieram
    klog(0, "[SYSCALL] Recebida INT 0x80!"); 
    klog(0, "          Num: %d | Arg1 (Ptr/Val): 0x%llx", sys_num, arg1);

    switch (sys_num) {
        case SYS_PRINT:
            // Vamos tentar ler o primeiro caractere manualmente para testar
            char *msg = (char*)arg1;
            char c = msg[0]; 
            klog(0, "          Char[0]: '%c' (Hex: 0x%x)", c, c);

            if (c == 0) {
                klog(1, "          [AVISO] String vazia ou nula!");
            } else {
                klog(0, "[USER SAYS]: %s", msg);
            }
            break;
            
        case SYS_EXIT:
            klog(0, "[USER EXIT] O programa terminou (Arg: %d).", arg1);
            klog(0, "            Travando CPU em loop infinito.");
            for(;;) asm volatile("hlt");
            break;
            
        default:
            klog(1, "[SYSCALL] Desconhecida: %d", sys_num);
            break;
    }
}

