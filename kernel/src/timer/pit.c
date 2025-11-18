#include "pit.h"
#include "../io/io.h"

#define PIT_CMD_PORT    0x43
#define PIT_DATA_PORT_0 0x40

void pit_init(uint32_t frequency) {
    uint32_t divisor = 1193180 / frequency;

    // Envia o comando para configurar o canal 0
    // Modo 3 (square wave) e acesso low/high byte
    outb(PIT_CMD_PORT, 0x36);

    // Envia o byte baixo do divisor
    uint8_t l = (uint8_t)(divisor & 0xFF);
    outb(PIT_DATA_PORT_0, l);

    // Envia o byte alto do divisor
    uint8_t h = (uint8_t)((divisor >> 8) & 0xFF);
    outb(PIT_DATA_PORT_0, h);
}
