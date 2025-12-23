#pragma once
#include <stdint.h>
#include <stddef.h>
#include "blockdev.h"

/* Forward declaration para evitar erro de compilação sem incluir vfs.h aqui */
struct vfs_fs_ops;

/* === ESTRUTURAS DE DISCO (PACKED) === */

/* Boot Sector FAT32 */
struct fat32_bpb {
    uint8_t  jmp[3];
    char     oem[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  num_fats;
    uint16_t root_entries;
    uint16_t total_sectors_16;
    uint8_t  media_descriptor;
    uint16_t sectors_per_fat_16;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint32_t sectors_per_fat; /* FAT32 */
    uint16_t ext_flags;
    uint16_t fs_version;
    uint32_t root_cluster;
    uint16_t fsinfo_sector;
    uint16_t boot_backup_sector;
    uint8_t  reserved[12];
    uint8_t  drive_number;
    uint8_t  ntres;
    uint8_t  boot_signature;
    uint32_t volume_serial;
    char     volume_label[11];
    char     fs_type[8];
    uint8_t  boot_code[420];
    uint16_t boot_signature_2;
} __attribute__((packed));

/* Directory Entry FAT32 */
struct fat32_dirent {
    unsigned char name[8];
    unsigned char ext[3];
    uint8_t  attr;
    uint8_t  ntres;
    uint8_t  ctime_tenth;
    uint16_t ctime;
    uint16_t cdate;
    uint16_t adate;
    uint16_t cluster_hi;
    uint16_t mtime;
    uint16_t mdate;
    uint16_t cluster_lo;
    uint32_t size;
} __attribute__((packed));

#define FAT32_ATTR_READONLY    0x01
#define FAT32_ATTR_HIDDEN      0x02
#define FAT32_ATTR_SYSTEM      0x04
#define FAT32_ATTR_VOLUME_ID   0x08
#define FAT32_ATTR_DIRECTORY   0x10
#define FAT32_ATTR_ARCHIVE     0x20
#define FAT32_ATTR_LFN         0x0F

/* === ESTRUTURAS DE MEMÓRIA (HANDLES) === */

/* Estrutura do Sistema de Arquivos (FS Handle) */
typedef struct fat32_fs {
    struct fat32_bpb bpb;
    blockdev_t *dev;
    uint32_t fat_start;
    uint32_t data_start;
    uint32_t bytes_per_cluster;
} fat32_fs_t;

/* Estrutura de Arquivo Aberto (File Handle) */
/* === PROTÓTIPOS DE FUNÇÕES GERAIS === */
/* Adicione isto ao seu fat32.h existente: */

/* Constantes para o código novo */
#define MAX_OPEN_FILES       64
#define FAT32_MODE_READ      0
#define FAT32_MODE_WRITE     1

/* ATUALIZE sua struct fat32_file (no header!): */
typedef struct fat32_file {
    uint8_t used;
    uint8_t mode;           /* 0=read, 1=write - MANTENHA COMPATÍVEL! */
    fat32_fs_t *fs;
    uint32_t first_cluster;
    
    /* RENOMEIE para compatibilidade: */
    uint32_t curr_cluster;  /* Mantém seu nome original */
    uint32_t curr_offset;   /* Mantém seu nome original */
    uint32_t file_size;     /* MANTENHA file_size (não size) */
    uint32_t file_pos;      /* MANTENHA file_pos (não position) */
} fat32_file_t;

/* Adicione protótipos para funções que faltam: */
int fat32_resolve(fat32_fs_t *fs, const char *path, uint32_t *parent, char name[11]);
int fat32_find(fat32_fs_t *fs, uint32_t dir_cluster, const char name[11], 
               struct fat32_dirent *result, uint32_t *sector, uint8_t *entry);
uint32_t fat32_next_cluster(fat32_fs_t *fs, uint32_t cluster);
fat32_fs_t *fat32_mount(blockdev_t *dev);
fat32_file_t* fat32_open(fat32_fs_t *fs, const char *path, const char *mode);
int fat32_read(fat32_file_t *fp, void *buf, int sz);
int fat32_write(fat32_file_t *fp, const void *buf, int sz);
void fat32_close(fat32_file_t *fp);
int fat32_mkdir(fat32_fs_t *fs, const char *path);
void fat32_ls(fat32_fs_t *fs, const char *path);
/* Adicione no fat32.h: */

/* Lista conteúdo do disco recursivamente */
void fat32_ls_recursive(fat32_fs_t *fs);

/* Lista apenas um diretório */
void fat32_ls(fat32_fs_t *fs, const char *path);
/* === PROTÓTIPOS DE INTEGRAÇÃO VFS (DEFINIDOS EM FAT32_VFS.C) === */
struct vfs_fs_ops* vfs_get_fat32_ops(void);
void* vfs_create_fat32_context(fat32_fs_t *fatfs);
void vfs_destroy_fat32_context(void *context);

#pragma once

