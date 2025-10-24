#include "pic.hpp"
#include "io.hpp" // O arquivo que você já criou!

namespace PIC {

void remap_and_disable() {
    // Salva as máscaras atuais
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);

    // Inicia a sequência de inicialização (em modo cascata)
    outb(PIC1_CMD, 0x11);
    io_wait();
    outb(PIC2_CMD, 0x11);
    io_wait();

    // Define os offsets dos vetores da IDT para o PIC.
    // IRQ 0-7  -> Interrupções 32-39
    // IRQ 8-15 -> Interrupções 40-47
    outb(PIC1_DATA, 32);
    io_wait();
    outb(PIC2_DATA, 40);
    io_wait();

    // Configura a cascata entre os dois PICs (PIC2 está na IRQ2 do PIC1)
    outb(PIC1_DATA, 4);
    io_wait();
    outb(PIC2_DATA, 2);
    io_wait();

    // Define o modo 8086
    outb(PIC1_DATA, 0x01);
    io_wait();
    outb(PIC2_DATA, 0x01);
    io_wait();

    // Restaura as máscaras salvas. Inicialmente, todas as IRQs estão desabilitadas.
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

void send_eoi(int irq) {
    if (irq >= 8) {
        outb(PIC2_CMD, 0x20); // Envia EOI para o PIC escravo
    }
    outb(PIC1_CMD, 0x20); // Envia EOI para o PIC mestre
}

} // namespace PIC

