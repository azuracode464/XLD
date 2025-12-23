/* fat32.c - Versão compatível com SEU kernel */
#include "fat32.h"
#include "string.h"
#include "kmalloc.h"
#include "spinlock.h"

extern void klog(int level, const char *fmt, ...);

/* Seus defines de log - ADAPTE! */
#ifndef KLOG_INFO
#define KLOG_INFO  0
#define KLOG_WARN  1
#define KLOG_ERROR 2
#define KLOG_DEBUG 3
#endif

/* ===================== CONSTANTES ===================== */
#define SECTOR_SIZE          512
#define DIR_ENTRIES_PER_SEC  16
#define DIR_ENTRY_SIZE       32
#define FAT32_EOC_MIN        0x0FFFFFF8
#define FAT32_BAD_CLUSTER    0x0FFFFFF7
#define FAT32_FREE_CLUSTER   0x00000000

/* CORRIGIDO: String de 11 chars exatos */
static const char DOT_NAME[11]      = ".         ";
static const char DOTDOT_NAME[11]   = "..        ";

/* ===================== ESTRUTURAS LOCAIS ===================== */
typedef struct {
    fat32_fs_t *fs;
    uint8_t sector_buf[SECTOR_SIZE];
    spinlock_t lock;
} fat32_cache_t;

/* ===================== VARIÁVEIS GLOBAIS ===================== */
static fat32_file_t open_files[MAX_OPEN_FILES];
static fat32_cache_t cache;

/* ===================== FUNÇÕES AUXILIARES ===================== */
static inline void spinlock_init(spinlock_t *lock) {
    lock->lock = 0;
    lock->cpu_id = 0;
}

static int validate_fs(fat32_fs_t *fs) {
    if (!fs || !fs->dev) return -1;
    if (fs->bpb.bytes_per_sector != SECTOR_SIZE) return -1;
    if (fs->bpb.sectors_per_cluster == 0) return -1;
    if (fs->bpb.num_fats == 0) return -1;
    if (fs->bpb.root_cluster < 2) return -1;
    return 0;
}

/* ===================== CACHE THREAD-SAFE ===================== */
static int cache_read_sector(fat32_fs_t *fs, uint32_t sector, uint8_t *buf) {
    spinlock_lock(&cache.lock);
    int ret = fs->dev->read(sector, 1, buf);
    spinlock_unlock(&cache.lock);
    return ret;
}

static int cache_write_sector(fat32_fs_t *fs, uint32_t sector, uint8_t *buf) {
    spinlock_lock(&cache.lock);
    int ret = fs->dev->write(sector, 1, buf);
    spinlock_unlock(&cache.lock);
    return ret;
}

/* ===================== FAT OPERATIONS ===================== */
static uint32_t read_fat_entry(fat32_fs_t *fs, uint32_t cluster) {
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = fs->fat_start + (fat_offset / SECTOR_SIZE);
    uint32_t entry_offset = fat_offset % SECTOR_SIZE;
    
    uint8_t sector[SECTOR_SIZE];
    if (cache_read_sector(fs, fat_sector, sector) != 0) {
        return FAT32_BAD_CLUSTER;
    }
    
    uint32_t entry = *(uint32_t*)(sector + entry_offset);
    return entry & 0x0FFFFFFF;
}

static int write_fat_entry(fat32_fs_t *fs, uint32_t cluster, uint32_t value) {
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = fs->fat_start + (fat_offset / SECTOR_SIZE);
    uint32_t entry_offset = fat_offset % SECTOR_SIZE;
    
    uint8_t sector[SECTOR_SIZE];
    if (cache_read_sector(fs, fat_sector, sector) != 0) {
        return -1;
    }
    
    uint32_t *entry = (uint32_t*)(sector + entry_offset);
    uint32_t old = *entry;
    *entry = (value & 0x0FFFFFFF) | (old & 0xF0000000);
    
    return cache_write_sector(fs, fat_sector, sector);
}

uint32_t fat32_next_cluster(fat32_fs_t *fs, uint32_t cluster) {
    uint32_t next = read_fat_entry(fs, cluster);
    
    if (next >= FAT32_EOC_MIN) {
        return 0xFFFFFFFF;
    }
    if (next == FAT32_BAD_CLUSTER) {
        return 0xFFFFFFFF;
    }
    
    return next;
}

static uint32_t allocate_cluster(fat32_fs_t *fs, uint32_t prev_cluster) {
    for (uint32_t cluster = 2; cluster < 0x0FFFFFF0; cluster++) {
        if (read_fat_entry(fs, cluster) == FAT32_FREE_CLUSTER) {
            if (write_fat_entry(fs, cluster, 0x0FFFFFFF) < 0) {
                return 0;
            }
            
            if (prev_cluster != 0) {
                if (write_fat_entry(fs, prev_cluster, cluster) < 0) {
                    write_fat_entry(fs, cluster, FAT32_FREE_CLUSTER);
                    return 0;
                }
            }
            
            return cluster;
        }
    }
    return 0;
}

/* ===================== 8.3 CONVERSION ===================== */
static void to_8_3(const char *src, char dst[11]) {
    char name[9] = "        ";
    char ext[4] = "   ";
    
    const char *dot = strchr(src, '.');
    
    if (dot && dot != src) {
        int name_len = dot - src;
        if (name_len > 8) name_len = 8;
        
        int ext_len = strlen(dot + 1);
        if (ext_len > 3) ext_len = 3;
        
        for (int i = 0; i < name_len; i++) {
            char c = src[i];
            if (c >= 'a' && c <= 'z') c -= 32;
            if (c == ' ') c = '_';
            name[i] = c;
        }
        
        for (int i = 0; i < ext_len; i++) {
            char c = dot[i + 1];
            if (c >= 'a' && c <= 'z') c -= 32;
            if (c == ' ') c = '_';
            ext[i] = c;
        }
    } else {
        int len = strlen(src);
        if (len > 8) len = 8;
        
        for (int i = 0; i < len; i++) {
            char c = src[i];
            if (c >= 'a' && c <= 'z') c -= 32;
            if (c == ' ') c = '_';
            name[i] = c;
        }
    }
    
    memcpy(dst, name, 8);
    memcpy(dst + 8, ext, 3);
}

static void from_8_3(const struct fat32_dirent *entry, char dst[11]) {
    memcpy(dst, entry->name, 8);
    memcpy(dst + 8, entry->ext, 3);
}

static int compare_8_3(const char a[11], const char b[11]) {
    return memcmp(a, b, 11) == 0;
}

/* ===================== DIRECTORY OPERATIONS ===================== */
int fat32_find(fat32_fs_t *fs, uint32_t dir_cluster, 
               const char target[11], struct fat32_dirent *result,
               uint32_t *found_sector, uint8_t *found_entry) {
    
    uint32_t current = dir_cluster;
    
    while (current != 0xFFFFFFFF) {
        uint32_t first_sector = fs->data_start + 
                               (current - 2) * fs->bpb.sectors_per_cluster;
        
        for (uint32_t sec = 0; sec < fs->bpb.sectors_per_cluster; sec++) {
            uint8_t sector[SECTOR_SIZE];
            if (cache_read_sector(fs, first_sector + sec, sector) != 0) {
                return -1;
            }
            
            for (int ent = 0; ent < DIR_ENTRIES_PER_SEC; ent++) {
                struct fat32_dirent *de = 
                    (struct fat32_dirent*)(sector + ent * DIR_ENTRY_SIZE);
                
                if (de->name[0] == 0x00) {
                    return -1;
                }
                
                if (de->name[0] == 0xE5 || de->attr == 0x0F) {
                    continue;
                }
                
                char current_name[11];
                from_8_3(de, current_name);
                
                if (compare_8_3(current_name, target)) {
                    if (result) memcpy(result, de, DIR_ENTRY_SIZE);
                    if (found_sector) *found_sector = first_sector + sec;
                    if (found_entry) *found_entry = ent;
                    return 0;
                }
            }
        }
        
        current = fat32_next_cluster(fs, current);
    }
    
    return -1;
}

static int find_free_dirent_slot(fat32_fs_t *fs, uint32_t dir_cluster,
                                 uint32_t *sector_out, uint8_t *entry_out) {
    
    uint32_t current = dir_cluster;
    
    while (current != 0xFFFFFFFF) {
        uint32_t first_sector = fs->data_start + 
                               (current - 2) * fs->bpb.sectors_per_cluster;
        
        for (uint32_t sec = 0; sec < fs->bpb.sectors_per_cluster; sec++) {
            uint8_t sector[SECTOR_SIZE];
            if (cache_read_sector(fs, first_sector + sec, sector) != 0) {
                return -1;
            }
            
            for (int ent = 0; ent < DIR_ENTRIES_PER_SEC; ent++) {
                struct fat32_dirent *de = 
                    (struct fat32_dirent*)(sector + ent * DIR_ENTRY_SIZE);
                
                if (de->name[0] == 0x00 || de->name[0] == 0xE5) {
                    if (sector_out) *sector_out = first_sector + sec;
                    if (entry_out) *entry_out = ent;
                    return 0;
                }
            }
        }
        
        current = fat32_next_cluster(fs, current);
    }
    
    return -1;
}

/* ===================== PATH RESOLUTION ===================== */
int fat32_resolve(fat32_fs_t *fs, const char *path, 
                  uint32_t *parent_cluster, char name[11]) {
    
    if (!fs || !path || !parent_cluster || !name) {
        return -1;
    }
    
    if (path[0] == '\0' || (path[0] == '/' && path[1] == '\0')) {
        name[0] = '\0';
        *parent_cluster = fs->bpb.root_cluster;
        return 0;
    }
    
    const char *current = path;
    if (current[0] == '/') {
        current++;
    }
    
    char tmp[256];
    size_t len = strlen(current);
    if (len >= sizeof(tmp)) return -1;
    memcpy(tmp, current, len);
    tmp[len] = '\0';
    
    uint32_t cur_cluster = fs->bpb.root_cluster;
    char *last_component = NULL;
    char *start = tmp;
    
    while (*start) {
        char *slash = strchr(start, '/');
        if (slash) *slash = '\0';
        
        if (*start) {
            if (slash) {
                char component[11];
                to_8_3(start, component);
                
                struct fat32_dirent de;
                if (fat32_find(fs, cur_cluster, component, &de, NULL, NULL) < 0) {
                    return -1;
                }
                
                if (!(de.attr & FAT32_ATTR_DIRECTORY)) {
                    return -1;
                }
                
                cur_cluster = de.cluster_lo | ((uint32_t)de.cluster_hi << 16);
            } else {
                last_component = start;
            }
        }
        
        if (slash) {
            start = slash + 1;
        } else {
            break;
        }
    }
    
    if (last_component) {
        to_8_3(last_component, name);
        *parent_cluster = cur_cluster;
        return 0;
    } else {
        name[0] = '\0';
        *parent_cluster = cur_cluster;
        return 0;
    }
}

/* ===================== PUBLIC API ===================== */
fat32_fs_t *fat32_mount(blockdev_t *dev) {
    if (!dev || !dev->read) {
        return NULL;
    }
    
    uint8_t sector[512];
    uint32_t fat32_lba = 0;
    /* 1. PRIMEIRO: Tenta ler como disco FAT32 bruto (setor 0) */
    if (dev->read(0, 1, sector) != 0) {
        klog(KLOG_ERROR, "[FAT32] Failed to read sector 0");
        return NULL;
    }
    
    struct fat32_bpb *bpb = (struct fat32_bpb *)sector;
    
    /* 2. VERIFICA SE É FAT32 DIRETO */
    int is_fat32_direct = 0;
    
    /* Check 1: Bytes por setor (512) */
    if (bpb->bytes_per_sector == 512) {
        /* Check 2: Tem "FAT" ou "FAT32" na assinatura */
        if (memcmp(bpb->fs_type, "FAT32", 5) == 0 || 
            memcmp(bpb->fs_type, "FAT", 3) == 0 ||
            memcmp(bpb->fs_type, "MSDOS", 5) == 0) {
            is_fat32_direct = 1;
            klog(KLOG_INFO, "[FAT32] Detected raw FAT32 filesystem");
        }
        /* Check 3: Tem boot jump (EB 58 90 ou EB 3C 90) */
        else if ((bpb->jmp[0] == 0xEB && bpb->jmp[2] == 0x90) ||
                 (bpb->jmp[0] == 0xE9) ||
                 (bpb->jmp[0] == 0xE8)) {
            /* Check 4: Sectors per cluster válido (1,2,4,8,16,32,64,128) */
            uint8_t spc = bpb->sectors_per_cluster;
            if (spc == 1 || spc == 2 || spc == 4 || spc == 8 || 
                spc == 16 || spc == 32 || spc == 64 || spc == 128) {
                is_fat32_direct = 1;
                klog(KLOG_INFO, "[FAT32] Detected FAT-like structure");
            }
        }
    }
    
    /* 3. SE NÃO FOR FAT32 DIRETO, PROCURA MBR */
    if (!is_fat32_direct) {
        klog(KLOG_INFO, "[FAT32] Checking for MBR partition table...");
        
        /* Tenta ler MBR (já temos setor 0) */
        if (sector[510] == 0x55 && sector[511] == 0xAA) {
            klog(KLOG_INFO, "[FAT32] Valid MBR signature found");
            
            /* Procura partição FAT32 (type 0x0B, 0x0C, 0x1C, 0x0E) */
            uint32_t fat32_lba = 0;
            
            for (int i = 0; i < 4; i++) {
                uint8_t *part = sector + 0x1BE + (i * 16);
                uint8_t type = part[4];
                
                klog(KLOG_DEBUG, "[FAT32] Partition %d: type=0x%02x", i, type);
                
                /* Tipos de partição FAT32 */
                if (type == 0x0B || type == 0x0C || 
                    type == 0x1B || type == 0x1C || 
                    type == 0x0E || type == 0x1E) {
                    
                    fat32_lba = *(uint32_t*)(part + 8);
                    klog(KLOG_INFO, "[FAT32] Found FAT32 partition at LBA %u", fat32_lba);
                    
                    /* Lê boot sector da partição */
                    if (dev->read(fat32_lba, 1, sector) != 0) {
                        klog(KLOG_ERROR, "[FAT32] Failed to read partition boot sector");
                        return NULL;
                    }
                    
                    bpb = (struct fat32_bpb *)sector;
                    break;
                }
            }
            
            if (fat32_lba == 0) {
                klog(KLOG_ERROR, "[FAT32] No FAT32 partition found in MBR");
                return NULL;
            }
        } else {
            klog(KLOG_ERROR, "[FAT32] Not a valid FAT32 or MBR disk");
            return NULL;
        }
    }
    
    /* 4. VALIDA BPB */
    if (bpb->bytes_per_sector != 512) {
        klog(KLOG_ERROR, "[FAT32] Invalid sector size: %u", bpb->bytes_per_sector);
        return NULL;
    }
    
    if (bpb->sectors_per_cluster == 0) {
        klog(KLOG_ERROR, "[FAT32] Invalid sectors per cluster: 0");
        return NULL;
    }
    
    if (bpb->num_fats == 0) {
        klog(KLOG_ERROR, "[FAT32] Invalid number of FATs: 0");
        return NULL;
    }
    
    /* 5. ALOCA ESTRUTURA FS */
    fat32_fs_t *fs = kmalloc(sizeof(fat32_fs_t));
    if (!fs) {
        klog(KLOG_ERROR, "[FAT32] Out of memory");
        return NULL;
    }
    
    memset(fs, 0, sizeof(fat32_fs_t));
    memcpy(&fs->bpb, bpb, sizeof(struct fat32_bpb));
    fs->dev = dev;
    
    /* 6. CALCULA OFFSETS */
    fs->fat_start = bpb->reserved_sectors;
    if (is_fat32_direct && fat32_lba > 0) {
        fs->fat_start += fat32_lba;  /* Adiciona offset da partição */
    }
    
    fs->data_start = fs->fat_start + (bpb->num_fats * bpb->sectors_per_fat);
    fs->bytes_per_cluster = bpb->bytes_per_sector * bpb->sectors_per_cluster;
    
    /* 7. LOG DE INFO */
    klog(KLOG_INFO, "[FAT32] Mounted: clusters=%u, cluster_size=%u bytes",
         bpb->total_sectors_32 / bpb->sectors_per_cluster,
         fs->bytes_per_cluster);
    
    klog(KLOG_INFO, "[FAT32] FAT start: sector %u, Data start: sector %u",
         fs->fat_start, fs->data_start);
    
    klog(KLOG_INFO, "[FAT32] Root cluster: %u", bpb->root_cluster);
    
    return fs;
}
fat32_file_t *fat32_open(fat32_fs_t *fs, const char *path, const char *mode) {
    if (validate_fs(fs) < 0) return NULL;
    
    uint32_t parent;
    char target[11];
    
    if (fat32_resolve(fs, path, &parent, target) < 0) {
        return NULL;
    }
    
    if (target[0] == '\0') {
        return NULL;
    }
    
    struct fat32_dirent entry;
    uint32_t dir_sector;
    uint8_t dir_entry;
    
    int exists = (fat32_find(fs, parent, target, &entry, &dir_sector, &dir_entry) == 0);
    
    fat32_file_t *file = NULL;
    
    if (!exists) {
        if (mode[0] != 'w' && mode[0] != 'a') {
            return NULL;
        }
        
        memset(&entry, 0, sizeof(entry));
        memcpy(entry.name, target, 8);
        memcpy(entry.ext, target + 8, 3);
        entry.attr = FAT32_ATTR_ARCHIVE;
        
        uint32_t first_cluster = allocate_cluster(fs, 0);
        if (first_cluster == 0) {
            return NULL;
        }
        
        entry.cluster_lo = first_cluster & 0xFFFF;
        entry.cluster_hi = (first_cluster >> 16) & 0xFFFF;
        
        if (find_free_dirent_slot(fs, parent, &dir_sector, &dir_entry) < 0) {
            write_fat_entry(fs, first_cluster, FAT32_FREE_CLUSTER);
            return NULL;
        }
        
        uint8_t sector[SECTOR_SIZE];
        if (cache_read_sector(fs, dir_sector, sector) != 0) {
            write_fat_entry(fs, first_cluster, FAT32_FREE_CLUSTER);
            return NULL;
        }
        
        struct fat32_dirent *slot = 
            (struct fat32_dirent*)(sector + dir_entry * DIR_ENTRY_SIZE);
        memcpy(slot, &entry, DIR_ENTRY_SIZE);
        
        if (cache_write_sector(fs, dir_sector, sector) != 0) {
            write_fat_entry(fs, first_cluster, FAT32_FREE_CLUSTER);
            return NULL;
        }
    }
    
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (!open_files[i].used) {
            file = &open_files[i];
            break;
        }
    }
    
    if (!file) {
        return NULL;
    }
    
    memset(file, 0, sizeof(fat32_file_t));
    file->used = 1;
    file->fs = fs;
    
    uint32_t first_cluster = entry.cluster_lo | ((uint32_t)entry.cluster_hi << 16);
    file->first_cluster = first_cluster;
    file->curr_cluster = first_cluster;  /* COMPATÍVEL COM SEU CÓDIGO! */
    file->curr_offset = 0;
    file->file_size = entry.size;
    file->file_pos = 0;
    
    if (mode[0] == 'w') {
        file->mode = 1;  /* WRITE - COMPATÍVEL! */
        file->file_size = 0;
        file->file_pos = 0;
    } else if (mode[0] == 'a') {
        file->mode = 1;
        file->file_pos = entry.size;
    } else {
        file->mode = 0;  /* READ - COMPATÍVEL! */
    }
    
    return file;
}

int fat32_read(fat32_file_t *file, void *buffer, int size) {
    if (!file || !file->used || file->mode != 0) {  /* 0 = READ */
        return -1;
    }
    
    if (size <= 0) return 0;
    if (file->file_pos >= file->file_size) return 0;
    
    if (file->file_pos + size > file->file_size) {
        size = file->file_size - file->file_pos;
    }
    
    uint8_t *dst = (uint8_t*)buffer;
    int bytes_read = 0;
    
    while (size > 0) {
        uint32_t cluster_offset = file->curr_offset / file->fs->bytes_per_cluster;
        uint32_t sector_in_cluster = (file->curr_offset % file->fs->bytes_per_cluster) / SECTOR_SIZE;
        
        uint32_t target_cluster = file->curr_cluster;
        for (uint32_t i = 0; i < cluster_offset; i++) {
            target_cluster = fat32_next_cluster(file->fs, target_cluster);
            if (target_cluster == 0xFFFFFFFF) {
                goto done;
            }
        }
        
        uint32_t sector = file->fs->data_start + 
                         (target_cluster - 2) * file->fs->bpb.sectors_per_cluster + 
                         sector_in_cluster;
        
        uint8_t sector_buf[SECTOR_SIZE];
        if (cache_read_sector(file->fs, sector, sector_buf) != 0) {
            break;
        }
        
        uint32_t offset_in_sector = file->curr_offset % SECTOR_SIZE;
        uint32_t bytes_in_sector = SECTOR_SIZE - offset_in_sector;
        uint32_t to_copy = (size < bytes_in_sector) ? size : bytes_in_sector;
        
        memcpy(dst, sector_buf + offset_in_sector, to_copy);
        
        dst += to_copy;
        bytes_read += to_copy;
        file->file_pos += to_copy;
        file->curr_offset += to_copy;
        size -= to_copy;
        
        if (file->curr_offset >= file->fs->bytes_per_cluster) {
            file->curr_offset = 0;
            file->curr_cluster = fat32_next_cluster(file->fs, file->curr_cluster);
            
            if (file->curr_cluster == 0xFFFFFFFF) {
                break;
            }
        }
    }
    
done:
    return bytes_read;
}

int fat32_write(fat32_file_t *file, const void *buffer, int size) {
    if (!file || !file->used || file->mode != 1) {  /* 1 = WRITE */
        return -1;
    }
    
    if (size <= 0) return 0;
    
    const uint8_t *src = (const uint8_t*)buffer;
    int bytes_written = 0;
    
    while (size > 0) {
        if (file->curr_cluster == 0) {
            file->first_cluster = allocate_cluster(file->fs, 0);
            if (file->first_cluster == 0) {
                break;
            }
            file->curr_cluster = file->first_cluster;
            file->curr_offset = 0;
        }
        
        uint32_t sector_in_cluster = file->curr_offset / SECTOR_SIZE;
        uint32_t offset_in_sector = file->curr_offset % SECTOR_SIZE;
        
        uint32_t sector = file->fs->data_start + 
                         (file->curr_cluster - 2) * file->fs->bpb.sectors_per_cluster + 
                         sector_in_cluster;
        
        uint8_t sector_buf[SECTOR_SIZE];
        if (cache_read_sector(file->fs, sector, sector_buf) != 0) {
            memset(sector_buf, 0, SECTOR_SIZE);
        }
        
        uint32_t bytes_in_sector = SECTOR_SIZE - offset_in_sector;
        uint32_t to_copy = (size < bytes_in_sector) ? size : bytes_in_sector;
        
        memcpy(sector_buf + offset_in_sector, src, to_copy);
        
        if (cache_write_sector(file->fs, sector, sector_buf) != 0) {
            break;
        }
        
        src += to_copy;
        bytes_written += to_copy;
        file->file_pos += to_copy;
        file->curr_offset += to_copy;
        size -= to_copy;
        
        if (file->file_pos > file->file_size) {
            file->file_size = file->file_pos;
        }
        
        if (file->curr_offset >= file->fs->bytes_per_cluster) {
            uint32_t next = fat32_next_cluster(file->fs, file->curr_cluster);
            
            if (next == 0xFFFFFFFF) {
                next = allocate_cluster(file->fs, file->curr_cluster);
                if (next == 0) {
                    break;
                }
            }
            
            file->curr_cluster = next;
            file->curr_offset = 0;
        }
    }
    
    return bytes_written;
}

void fat32_close(fat32_file_t *file) {
    if (file && file->used) {
        /* TODO: Atualizar tamanho no diretório se em modo escrita */
        file->used = 0;
    }
}

int fat32_mkdir(fat32_fs_t *fs, const char *path) {
    if (validate_fs(fs) < 0) return -1;
    
    uint32_t parent;
    char name[11];
    
    if (fat32_resolve(fs, path, &parent, name) < 0) {
        return -1;
    }
    
    if (name[0] == '\0') return -1;
    
    struct fat32_dirent existing;
    if (fat32_find(fs, parent, name, &existing, NULL, NULL) == 0) {
        return -1;
    }
    
    uint32_t new_cluster = allocate_cluster(fs, 0);
    if (new_cluster == 0) {
        return -1;
    }
    
    uint32_t first_sector = fs->data_start + (new_cluster - 2) * fs->bpb.sectors_per_cluster;
    uint8_t zero_sector[SECTOR_SIZE];
    memset(zero_sector, 0, SECTOR_SIZE);
    
    for (uint32_t i = 0; i < fs->bpb.sectors_per_cluster; i++) {
        if (cache_write_sector(fs, first_sector + i, zero_sector) != 0) {
            write_fat_entry(fs, new_cluster, FAT32_FREE_CLUSTER);
            return -1;
        }
    }
    
    uint8_t sector[SECTOR_SIZE];
    if (cache_read_sector(fs, first_sector, sector) != 0) {
        write_fat_entry(fs, new_cluster, FAT32_FREE_CLUSTER);
        return -1;
    }
    
    struct fat32_dirent *entries = (struct fat32_dirent*)sector;
    
    memset(&entries[0], 0, DIR_ENTRY_SIZE);
    memcpy(entries[0].name, DOT_NAME, 11);
    entries[0].attr = FAT32_ATTR_DIRECTORY;
    entries[0].cluster_lo = new_cluster & 0xFFFF;
    entries[0].cluster_hi = (new_cluster >> 16) & 0xFFFF;
    
    memset(&entries[1], 0, DIR_ENTRY_SIZE);
    memcpy(entries[1].name, DOTDOT_NAME, 11);
    entries[1].attr = FAT32_ATTR_DIRECTORY;
    entries[1].cluster_lo = parent & 0xFFFF;
    entries[1].cluster_hi = (parent >> 16) & 0xFFFF;
    
    if (cache_write_sector(fs, first_sector, sector) != 0) {
        write_fat_entry(fs, new_cluster, FAT32_FREE_CLUSTER);
        return -1;
    }
    
    uint32_t dir_sector;
    uint8_t dir_entry;
    
    if (find_free_dirent_slot(fs, parent, &dir_sector, &dir_entry) < 0) {
        write_fat_entry(fs, new_cluster, FAT32_FREE_CLUSTER);
        return -1;
    }
    
    if (cache_read_sector(fs, dir_sector, sector) != 0) {
        write_fat_entry(fs, new_cluster, FAT32_FREE_CLUSTER);
        return -1;
    }
    
    struct fat32_dirent *new_entry = 
        (struct fat32_dirent*)(sector + dir_entry * DIR_ENTRY_SIZE);
    
    memset(new_entry, 0, DIR_ENTRY_SIZE);
    memcpy(new_entry->name, name, 11);
    new_entry->attr = FAT32_ATTR_DIRECTORY;
    new_entry->cluster_lo = new_cluster & 0xFFFF;
    new_entry->cluster_hi = (new_cluster >> 16) & 0xFFFF;
    
    if (cache_write_sector(fs, dir_sector, sector) != 0) {
        write_fat_entry(fs, new_cluster, FAT32_FREE_CLUSTER);
        return -1;
    }
    
    return 0;
}
