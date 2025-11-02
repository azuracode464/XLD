#include "reboot.hpp"

// Função para ler um byte de uma porta de I/O
static inline unsigned char inb(unsigned short port) {
    unsigned char data;
    asm volatile("inb %1, %0" : "=a"(data) : "d"(port));
    return data;
}

// Função para escrever um byte em uma porta de I/O
static inline void outb(unsigned short port, unsigned char data) {
    asm volatile("outb %0, %1" : : "a"(data), "d"(port));
}

namespace Command::Reboot {

int execute(int argc, char* argv[]) {
    // Esta é a "maneira 8042" de reiniciar um PC.
    // A gente manda um comando para o controlador de teclado para pulsar a linha de reset.
    unsigned char temp;
    
    // Espera o buffer de entrada do teclado ficar vazio
    do {
        temp = inb(0x64);
    } while ((temp & 2) != 0);
    
    // Envia o comando de reset para a CPU
    outb(0x64, 0xFE);
    
    // Se chegou aqui, algo deu errado. O PC já deveria ter reiniciado.
    // A gente trava o sistema por segurança.
    asm volatile("cli; hlt");

    return -1; // Nunca deve chegar aqui
}

} // namespace Command::Reboot
