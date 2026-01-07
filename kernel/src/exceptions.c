/* exceptions.c - versão minimalista */
#include <stdint.h>

extern void printk(const char *fmt, ...);

void exception_handler(uint64_t vector, uint64_t error_code) {
    /* Usando printk direto */
    printk("\n!!! CPU EXCEPTION %d !!!\n", (int)vector);
    printk("Error code: 0x%x\n", (unsigned int)error_code);
    printk("Kernel halted.\n");
    
    for (;;) {
        asm volatile("cli; hlt");
    }
}
