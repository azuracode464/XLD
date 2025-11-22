// kernel/src/pic/pic.c
#include "pic.h"
#include "../io/io.h"

void pic_init(void) {
    // O código de inicialização (ICW1-ICW4) está perfeito. Não mexa nele.
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC1_DATA, 0x20);
    io_wait();
    outb(PIC2_DATA, 0x28);
    io_wait();
    outb(PIC1_DATA, 4);
    io_wait();
    outb(PIC2_DATA, 2);
    io_wait();
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    // SUBSTITUA AS DUAS LINHAS ANTIGAS POR ESTAS DUAS:
    // Um bit '1' DESABILITA a IRQ. Um bit '0' HABILITA.
    // Habilita o Timer (IRQ 0) e o Teclado (IRQ 1).
    // A linha de cascata (IRQ 2) também precisa estar habilitada.
    // Bit:   7 6 5 4 3 2 1 0
    // Valor: 1 1 1 1 1 0 0 0 -> Desabilita 7,6,5,4,3. Habilita 2,1,0.
    outb(PIC1_DATA, 0b11111000);

    // Desabilita todas as interrupções do PIC slave por enquanto.
    outb(PIC2_DATA, 0xFF);
}

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) {
        outb(PIC2_COMMAND, 0x20);
    }
    outb(PIC1_COMMAND, 0x20);
}

