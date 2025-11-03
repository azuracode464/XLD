// ARQUIVO CORRIGIDO: kernel/src/drivers/ata/ata.cpp

#include "ata.hpp"

// As funções de I/O são seguras para serem globais, pois são 'inline'
// e só serão compiladas no corpo de quem as chama.
static inline void outb(uint16_t port, uint8_t data) {
    asm volatile("outb %0, %1" : : "a"(data), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void insw(uint16_t port, void* addr, int count) {
    asm volatile("rep insw" : "+D"(addr), "+c"(count) : "d"(port) : "memory");
}

static inline void outsw(uint16_t port, const void* addr, int count) {
    asm volatile("rep outsw" : "+S"(addr), "+c"(count) : "d"(port));
}


namespace ATA {

    // --- MUDANÇA CRUCIAL: Mover todas as constantes e funções de hardware para dentro do namespace ---
    // Isso evita que qualquer código seja executado em tempo de inicialização global.
    // Tudo agora só pode ser acessado através do namespace ATA.

    // --- Portas do Controlador ATA Primário ---
    constexpr uint16_t PORT_DATA          = 0x1F0;
    constexpr uint16_t PORT_ERROR         = 0x1F1;
    constexpr uint16_t PORT_SECTOR_COUNT  = 0x1F2;
    constexpr uint16_t PORT_LBA_LOW       = 0x1F3;
    constexpr uint16_t PORT_LBA_MID       = 0x1F4;
    constexpr uint16_t PORT_LBA_HIGH      = 0x1F5;
    constexpr uint16_t PORT_DRIVE_HEAD    = 0x1F6;
    constexpr uint16_t PORT_STATUS        = 0x1F7;
    constexpr uint16_t PORT_COMMAND       = 0x1F7;

    // --- Comandos ATA ---
    constexpr uint8_t CMD_READ_PIO        = 0x20;
    constexpr uint8_t CMD_WRITE_PIO       = 0x30;

    // --- Bits da Porta de Status ---
    constexpr uint8_t STATUS_BSY          = 0x80; // Busy
    constexpr uint8_t STATUS_DRQ          = 0x08; // Data Request
    constexpr uint8_t STATUS_ERR          = 0x01; // Error

    // Função que espera o disco parar de ficar ocupado.
    static void wait_bsy() {
        // Espera até que o bit BSY (7) seja 0.
        while (inb(PORT_STATUS) & STATUS_BSY);
    }

    // Função que espera o disco estar pronto para transferência de dados.
    static void wait_drq() {
        // Espera até que o bit DRQ (3) seja 1.
        while (!(inb(PORT_STATUS) & STATUS_DRQ));
    }

    void init() {
        // Esta função pode ser usada no futuro para detectar os discos.
        // Por enquanto, ela ser chamada já garante que o driver está "pronto".
    }

    bool read_sectors(uint32_t lba, uint8_t count, uint8_t* buffer) {
        wait_bsy();
        outb(PORT_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
        outb(PORT_SECTOR_COUNT, count);
        outb(PORT_LBA_LOW,  (uint8_t)lba);
        outb(PORT_LBA_MID,  (uint8_t)(lba >> 8));
        outb(PORT_LBA_HIGH, (uint8_t)(lba >> 16));
        outb(PORT_COMMAND, CMD_READ_PIO);

        for (int i = 0; i < count; i++) {
            wait_bsy();
            wait_drq();

            if (inb(PORT_STATUS) & STATUS_ERR) {
                return false;
            }
            insw(PORT_DATA, (uint16_t*)buffer + (i * 256), 256);
        }
        return true;
    }

    bool write_sectors(uint32_t lba, uint8_t count, const uint8_t* buffer) {
        wait_bsy();
        outb(PORT_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
        outb(PORT_SECTOR_COUNT, count);
        outb(PORT_LBA_LOW,  (uint8_t)lba);
        outb(PORT_LBA_MID,  (uint8_t)(lba >> 8));
        outb(PORT_LBA_HIGH, (uint8_t)(lba >> 16));
        outb(PORT_COMMAND, CMD_WRITE_PIO);

        for (int i = 0; i < count; i++) {
            wait_bsy();
            wait_drq();

            if (inb(PORT_STATUS) & STATUS_ERR) {
                return false;
            }
            outsw(PORT_DATA, (const uint16_t*)buffer + (i * 256), 256);
        }
        return true;
    }

} // namespace ATA

