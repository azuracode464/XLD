#include "pit.hpp"
#include "io.hpp"

#define PIT_CMD_PORT   0x43
#define PIT_DATA_PORT0 0x40

namespace PIT {

void set_frequency(uint32_t frequency) {
    uint32_t divisor = 1193182 / frequency; // Frequência base do PIT

    // Envia o comando para o PIT: Canal 0, modo de acesso Lo/Hi, modo 2 (gerador de taxa)
    outb(PIT_CMD_PORT, 0x34);

    // Envia os bytes do divisor (primeiro o baixo, depois o alto)
    outb(PIT_DATA_PORT0, (uint8_t)(divisor & 0xFF));
    outb(PIT_DATA_PORT0, (uint8_t)((divisor >> 8) & 0xFF));
}

} // namespace PIT

