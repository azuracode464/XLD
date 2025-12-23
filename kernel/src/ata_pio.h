// kernel/src/ata_pio.h
#pragma once
#include <stdint.h>
#include <stddef.h>

/* Registradores ATA PIO (Primary Master) */
#define ATA_DATA_PORT       0x1F0
#define ATA_ERROR_PORT      0x1F1
#define ATA_FEATURES_PORT   0x1F1 // Mesmo registrador que ERROR, mas para escrita
#define ATA_SECCOUNT_PORT   0x1F2
#define ATA_LBA0_PORT       0x1F3
#define ATA_LBA1_PORT       0x1F4
#define ATA_LBA2_PORT       0x1F5
#define ATA_DRIVE_PORT      0x1F6
#define ATA_COMMAND_PORT    0x1F7
#define ATA_STATUS_PORT     0x1F7 // Mesmo registrador que COMMAND, mas para leitura
#define ATA_CONTROL_PORT    0x3F6 /* Usado para reset e desabilitar interrupções */

/* Bits de Status (STATUS_PORT) */
#define ATA_STATUS_BSY      0x80 /* Busy */
#define ATA_STATUS_DRDY     0x40 /* Drive Ready */
#define ATA_STATUS_DF       0x20 /* Drive Fault */
#define ATA_STATUS_DSC      0x10 /* Device Seek Complete */
#define ATA_STATUS_DRQ      0x08 /* Data Request */
#define ATA_STATUS_CORR     0x04 /* Corrected Data */
#define ATA_STATUS_IDX      0x02 /* Index */
#define ATA_STATUS_ERR      0x01 /* Error */

/* Bits de Controle (CONTROL_PORT) */
#define ATA_CONTROL_SRST    0x04 /* Software Reset */
#define ATA_CONTROL_nIEN    0x02 /* Interrupt Disable (1 = desabilita) */

/* Comandos ATA */
#define ATA_CMD_READ_PIO    0x20
#define ATA_CMD_READ_DMA    0xC8
#define ATA_CMD_WRITE_PIO   0x30
#define ATA_CMD_WRITE_DMA   0xCA
#define ATA_CMD_IDENTIFY    0xEC
#define ATA_CMD_FLUSH_CACHE 0xE7

#define ATA_SECTOR_SIZE     512

/* Inicializa driver ATA PIO */
int ata_pio_init(void);

/* Lê setores */
int ata_pio_read(uint32_t lba, uint32_t count, void *buf);

/* Escreve setores */
int ata_pio_write(uint32_t lba, uint32_t count, const void *buf);

/* Flush cache */
int ata_pio_flush(void);

// Funções de E/S privadas (não exportadas globalmente, mas usadas internamente)
static inline void ata_outb(uint16_t port, uint8_t val);
static inline void ata_outw(uint16_t port, uint16_t val);
static inline void ata_outl(uint16_t port, uint32_t val);
static inline uint8_t ata_inb(uint16_t port);
static inline uint16_t ata_inw(uint16_t port);
static inline uint32_t ata_inl(uint16_t port);
static inline void ata_outsw(uint16_t port, const void* data, size_t count);
static inline void ata_insw(uint16_t port, void* data, size_t count);
static inline void io_wait(void);
