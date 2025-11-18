// kernel/src/pic/pic.h

#ifndef PIC_H
#define PIC_H

#include <stdint.h>

// --- CONSTANTES PARA OS PORTOS DO PIC ---
#define PIC1_COMMAND    0x20
#define PIC1_DATA       0x21
#define PIC2_COMMAND    0xA0
#define PIC2_DATA       0xA1

// --- CONSTANTES PARA COMANDOS DE INICIALIZAÇÃO (ICWs) ---
#define ICW1_INIT       0x10
#define ICW1_ICW4       0x01
#define ICW4_8086       0x01

// --- FUNÇÕES PÚBLICAS ---

// Inicializa e remapeia os PICs.
void pic_init(void);

// Envia o comando de Fim de Interrupção (End of Interrupt - EOI).
void pic_send_eoi(uint8_t irq);

#endif // PIC_H

