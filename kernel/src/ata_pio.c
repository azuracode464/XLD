/*
ata_pio.c - Driver ATA PIO com API compatível
*/
#include <stdint.h>
#include <stddef.h>

#define ATA_DATA 0x00
#define ATA_ERROR 0x01
#define ATA_FEATURES 0x01
#define ATA_SECTOR_CNT 0x02
#define ATA_SECTOR_NUM 0x03
#define ATA_CYL_LOW 0x04
#define ATA_CYL_HIGH 0x05
#define ATA_DRIVE_HEAD 0x06
#define ATA_STATUS 0x07
#define ATA_COMMAND 0x07
#define ATA_DEV_CTL 0x206

#define ATA_STATUS_ERR 0x01
#define ATA_STATUS_DRQ 0x08
#define ATA_STATUS_RDY 0x40
#define ATA_STATUS_BSY 0x80

#define ATA_CMD_IDENTIFY 0xEC
#define ATA_CMD_READ_PIO 0x20
#define ATA_CMD_WRITE_PIO 0x30
#define ATA_CMD_FLUSH_CACHE 0xE7

static inline void io_wait(void) { asm volatile("outb %%al, $0x80" : : "a"(0)); }
static inline uint8_t inb(uint16_t port) { uint8_t ret; asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port)); return ret; }
static inline void outb(uint16_t port, uint8_t val) { asm volatile("outb %0, %1" : : "a"(val), "Nd"(port)); }
static inline uint16_t inw(uint16_t port) { uint16_t ret; asm volatile("inw %1, %0" : "=a"(ret) : "Nd"(port)); return ret; }
static inline void outw(uint16_t port, uint16_t val) { asm volatile("outw %0, %1" : : "a"(val), "Nd"(port)); }

typedef struct {
    uint16_t base;
    uint8_t slave;
    uint32_t sectors;
    int present;
    char model[41];
} ata_drive_t;

static ata_drive_t drive;

static int ata_wait(uint16_t base, uint8_t mask, uint8_t val, int timeout) {
    for(int i=0;i<timeout;i++){
        uint8_t s = inb(base + ATA_STATUS);
        if(s & ATA_STATUS_ERR) return -1;
        if((s & mask) == val) return 0;
        for(int j=0;j<1000;j++) asm volatile("nop");
    }
    return -1;
}

static void ata_reset(uint16_t base) {
    outb(base + ATA_DEV_CTL, 0x04);
    for(int i=0;i<500;i++) io_wait();
    outb(base + ATA_DEV_CTL, 0x00);
    for(int i=0;i<20000;i++) io_wait();
    ata_wait(base, ATA_STATUS_BSY, 0, 30000);
}

static int ata_detect(uint16_t base, uint8_t slave) {
    outb(base + ATA_DRIVE_HEAD, 0xA0 | (slave << 4));
    for(int i=0;i<1000;i++) io_wait();
    
    outb(base + ATA_SECTOR_CNT, 0);
    outb(base + ATA_SECTOR_NUM, 0);
    outb(base + ATA_CYL_LOW, 0);
    outb(base + ATA_CYL_HIGH, 0);
    
    outb(base + ATA_COMMAND, ATA_CMD_IDENTIFY);
    
    if(inb(base + ATA_STATUS) == 0) return 0;
    if(ata_wait(base, ATA_STATUS_BSY, 0, 10000) < 0) return 0;
    
    uint8_t mid = inb(base + ATA_CYL_LOW);
    uint8_t high = inb(base + ATA_CYL_HIGH);
    if(mid == 0x14 && high == 0xEB) return 0;
    
    uint8_t s = inb(base + ATA_STATUS);
    if(!(s & ATA_STATUS_DRQ) && !(s & ATA_STATUS_ERR)) return 0;
    if(s & ATA_STATUS_ERR) return 0;
    
    return 1;
}

static int ata_identify(uint16_t base, uint8_t slave) {
    outb(base + ATA_DRIVE_HEAD, 0xA0 | (slave << 4));
    if(ata_wait(base, ATA_STATUS_BSY|ATA_STATUS_RDY, ATA_STATUS_RDY, 30000)<0) return -1;
    
    outb(base + ATA_COMMAND, ATA_CMD_IDENTIFY);
    if(ata_wait(base, ATA_STATUS_DRQ, ATA_STATUS_DRQ, 10000)<0) return -1;
    
    uint16_t buf[256];
    for(int i=0;i<256;i++) buf[i] = inw(base + ATA_DATA);
    
    for(int i=0;i<20;i++){
        drive.model[i*2] = (buf[27+i]>>8)&0xFF;
        drive.model[i*2+1] = buf[27+i]&0xFF;
    }
    drive.model[40]=0;
    for(int i=39;i>=0&&drive.model[i]==' ';i--) drive.model[i]=0;
    
    if(buf[49] & 0x0200) drive.sectors = *(uint32_t*)&buf[60];
    
    drive.base = base;
    drive.slave = slave;
    drive.present = 1;
    
    return 0;
}

/* API PÚBLICA - NOMES QUE O SEU KERNEL ESPERA */

void ata_pio_init(void) {
    uint16_t bases[] = {0x1F0, 0x170};
    
    for(int b=0;b<2;b++){
        for(int s=0;s<2;s++){
            uint16_t base = bases[b];
            ata_reset(base);
            if(ata_detect(base, s)){
                if(ata_identify(base, s)==0){
                    return;
                }
            }
        }
    }
    drive.present = 0;
}

int ata_pio_read(uint32_t lba, uint16_t count, void *buffer) {
    if(!drive.present) return -1;
    if(count==0||count>256) return -1;
    
    uint16_t base = drive.base;
    
    outb(base + ATA_DRIVE_HEAD, 0xE0 | (drive.slave<<4) | ((lba>>24)&0x0F));
    outb(base + ATA_SECTOR_CNT, count & 0xFF);
    outb(base + ATA_SECTOR_NUM, lba & 0xFF);
    outb(base + ATA_CYL_LOW, (lba>>8) & 0xFF);
    outb(base + ATA_CYL_HIGH, (lba>>16) & 0xFF);
    
    outb(base + ATA_COMMAND, ATA_CMD_READ_PIO);
    
    uint16_t *buf = (uint16_t*)buffer;
    for(int sector=0;sector<count;sector++){
        if(ata_wait(base, ATA_STATUS_DRQ, ATA_STATUS_DRQ, 10000)<0) return -1;
        for(int i=0;i<256;i++) buf[i] = inw(base + ATA_DATA);
        buf += 256;
    }
    
    if(ata_wait(base, ATA_STATUS_BSY, 0, 30000)<0) return -1;
    return 0;
}

int ata_pio_write(uint32_t lba, uint16_t count, void *buffer) {
    if(!drive.present) return -1;
    if(count==0||count>256) return -1;
    
    uint16_t base = drive.base;
    
    outb(base + ATA_DRIVE_HEAD, 0xE0 | (drive.slave<<4) | ((lba>>24)&0x0F));
    outb(base + ATA_SECTOR_CNT, count & 0xFF);
    outb(base + ATA_SECTOR_NUM, lba & 0xFF);
    outb(base + ATA_CYL_LOW, (lba>>8) & 0xFF);
    outb(base + ATA_CYL_HIGH, (lba>>16) & 0xFF);
    
    outb(base + ATA_COMMAND, ATA_CMD_WRITE_PIO);
    
    uint16_t *buf = (uint16_t*)buffer;
    for(int sector=0;sector<count;sector++){
        if(ata_wait(base, ATA_STATUS_DRQ, ATA_STATUS_DRQ, 10000)<0) return -1;
        for(int i=0;i<256;i++) outw(base + ATA_DATA, buf[i]);
        buf += 256;
        if(ata_wait(base, ATA_STATUS_BSY, 0, 10000)<0) return -1;
    }
    
    return 0;
}

int ata_pio_flush(void) {
    if(!drive.present) return -1;
    
    uint16_t base = drive.base;
    outb(base + ATA_COMMAND, ATA_CMD_FLUSH_CACHE);
    if(ata_wait(base, ATA_STATUS_BSY, 0, 30000)<0) return -1;
    return 0;
}

/* Funções auxiliares extras */

int ata_pio_get_sectors(void) {
    return drive.present ? drive.sectors : 0;
}

const char *ata_pio_get_model(void) {
    return drive.present ? drive.model : "No drive";
}

int ata_pio_present(void) {
    return drive.present;
}
