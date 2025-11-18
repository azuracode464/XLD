// kernel/src/pic/pic.c

#include "pic.h"
#include "../io/io.h" // Precisaremos de funções para ler/escrever em portos (outb/inb)

// Função para inicializar e remapear o PIC 8259A.
void pic_init(void) {
    // Salva as máscaras atuais dos PICs
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);

    // ICW1: Inicia a sequência de inicialização em modo cascata.
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();

    // ICW2: Remapeia os vetores da IDT.
    // PIC1 (Master) começa no vetor 32 (0x20).
    // PIC2 (Slave) começa no vetor 40 (0x28).
    outb(PIC1_DATA, 0x20);
    io_wait();
    outb(PIC2_DATA, 0x28);
    io_wait();

    // ICW3: Informa ao Master que há um Slave na linha IRQ2 (0x04).
    outb(PIC1_DATA, 4);
    io_wait();
    // Informa ao Slave sua identidade em cascata (0x02).
    outb(PIC2_DATA, 2);
    io_wait();

    // ICW4: Define o modo 8086.
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

// Envia o sinal de Fim de Interrupção (EOI) para o(s) PIC(s).
void pic_send_eoi(uint8_t irq) {
    // Se a interrupção (IRQ) veio do PIC Slave (IRQs 8-15),
    // nós precisamos enviar um EOI para o Slave primeiro.
    if (irq >= 8) {
        outb(PIC2_COMMAND, 0x20); // Envia EOI para o PIC Slave
    }

    // Em todos os casos, nós SEMPRE enviamos um EOI para o PIC Master.
    outb(PIC1_COMMAND, 0x20); // Envia EOI para o PIC Master
}
