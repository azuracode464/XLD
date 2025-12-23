/* fat32_ls.c - Listagem recursiva do FAT32
 * 
 * Adicione ao seu fat32.c ou crie arquivo separado
 */

#include "fat32.h"
#include "string.h"

extern void klog(int level, const char *fmt, ...);
extern void printk(const char *fmt, ...);

#define KLOG_INFO 0
#define KLOG_DEBUG 3

/* Helper para converter 8.3 para string legível */
static void fat32_name_to_string(const struct fat32_dirent *entry, char *out) {
    /* Nome */
    int i = 0;
    for (i = 0; i < 8 && entry->name[i] != ' '; i++) {
        out[i] = entry->name[i];
    }
    
    /* Extensão */
    if (entry->ext[0] != ' ') {
        out[i++] = '.';
        for (int j = 0; j < 3 && entry->ext[j] != ' '; j++) {
            out[i++] = entry->ext[j];
        }
    }
    
    out[i] = '\0';
}

/* Lista um diretório (não recursivo) */
static void fat32_list_dir_flat(fat32_fs_t *fs, uint32_t dir_cluster, 
                                 const char *prefix, int *file_count, 
                                 int *dir_count) {
    uint32_t current = dir_cluster;
    
    while (current != 0xFFFFFFFF && current >= 2) {
        uint32_t first_sector = fs->data_start + 
                               (current - 2) * fs->bpb.sectors_per_cluster;
        
        for (uint32_t sec = 0; sec < fs->bpb.sectors_per_cluster; sec++) {
            uint8_t sector[512];
            if (fs->dev->read(first_sector + sec, 1, sector) != 0) {
                return;
            }
            
            for (int ent = 0; ent < 16; ent++) {
                struct fat32_dirent *de = 
                    (struct fat32_dirent*)(sector + ent * 32);
                
                /* Fim do diretório */
                if (de->name[0] == 0x00) {
                    return;
                }
                
                /* Entrada deletada */
                if (de->name[0] == 0xE5) {
                    continue;
                }
                
                /* LFN ou Volume ID */
                if (de->attr == 0x0F || de->attr & 0x08) {
                    continue;
                }
                
                /* Ignora . e .. */
                if (de->name[0] == '.') {
                    continue;
                }
                
                /* Converte nome */
                char name[13];
                fat32_name_to_string(de, name);
                
                /* Imprime entrada */
                if (de->attr & FAT32_ATTR_DIRECTORY) {
                    printk("%s[DIR]  %s\n", prefix, name);
                    (*dir_count)++;
                } else {
                    printk("%s[FILE] %s (%u bytes)\n", prefix, name, de->size);
                    (*file_count)++;
                }
            }
        }
        
        current = fat32_next_cluster(fs, current);
    }
}

/* Lista recursivamente */
static void fat32_list_dir_recursive(fat32_fs_t *fs, uint32_t dir_cluster, 
                                      const char *path, int depth, 
                                      int *file_count, int *dir_count) {
    /* Proteção contra recursão infinita */
    if (depth > 10) {
        printk("%s  [... depth limit reached]\n", path);
        return;
    }
    
    /* Cria prefix para indentação */
    char prefix[256];
    for (int i = 0; i < depth * 2; i++) {
        prefix[i] = ' ';
    }
    prefix[depth * 2] = '\0';
    
    uint32_t current = dir_cluster;
    
    while (current != 0xFFFFFFFF && current >= 2) {
        uint32_t first_sector = fs->data_start + 
                               (current - 2) * fs->bpb.sectors_per_cluster;
        
        for (uint32_t sec = 0; sec < fs->bpb.sectors_per_cluster; sec++) {
            uint8_t sector[512];
            if (fs->dev->read(first_sector + sec, 1, sector) != 0) {
                return;
            }
            
            for (int ent = 0; ent < 16; ent++) {
                struct fat32_dirent *de = 
                    (struct fat32_dirent*)(sector + ent * 32);
                
                if (de->name[0] == 0x00) {
                    return;
                }
                
                if (de->name[0] == 0xE5) {
                    continue;
                }
                
                if (de->attr == 0x0F || de->attr & 0x08) {
                    continue;
                }
                
                if (de->name[0] == '.') {
                    continue;
                }
                
                char name[13];
                fat32_name_to_string(de, name);
                
                if (de->attr & FAT32_ATTR_DIRECTORY) {
                    printk("%s[DIR]  %s/\n", prefix, name);
                    (*dir_count)++;
                    
                    /* Recursão */
                    uint32_t sub_cluster = de->cluster_lo | 
                                          ((uint32_t)de->cluster_hi << 16);
                    
                    char subpath[256];
                    int len = strlen(path);
                    if (len > 0 && len < 240) {
                        strcpy(subpath, path);
                        strcat(subpath, "/");
                        strcat(subpath, name);
                    } else {
                        strcpy(subpath, name);
                    }
                    
                    fat32_list_dir_recursive(fs, sub_cluster, subpath, 
                                            depth + 1, file_count, dir_count);
                } else {
                    printk("%s[FILE] %s", prefix, name);
                    
                    /* Mostra tamanho formatado */
                    if (de->size < 1024) {
                        printk(" (%u B)\n", de->size);
                    } else if (de->size < 1024*1024) {
                        printk(" (%u KB)\n", de->size / 1024);
                    } else {
                        printk(" (%u MB)\n", de->size / (1024*1024));
                    }
                    
                    (*file_count)++;
                }
            }
        }
        
        current = fat32_next_cluster(fs, current);
    }
}

/* API Pública - Lista tudo recursivamente */
void fat32_ls_recursive(fat32_fs_t *fs) {
    if (!fs) {
        klog(KLOG_INFO, "[FAT32] ls: Invalid filesystem");
        return;
    }
    
    int file_count = 0;
    int dir_count = 0;
    
    printk("\n");
    printk("==========================================\n");
    printk(" FAT32 Disk Contents (Recursive)\n");
    printk("==========================================\n");
    printk("\n");
    printk("/\n");
    
    fat32_list_dir_recursive(fs, fs->bpb.root_cluster, "", 0, 
                            &file_count, &dir_count);
    
    printk("\n");
    printk("------------------------------------------\n");
    printk("Total: %d files, %d directories\n", file_count, dir_count);
    printk("==========================================\n");
    printk("\n");
}

/* API Pública - Lista apenas um diretório */
void fat32_ls(fat32_fs_t *fs, const char *path) {
    if (!fs) {
        klog(KLOG_INFO, "[FAT32] ls: Invalid filesystem");
        return;
    }
    
    int file_count = 0;
    int dir_count = 0;
    
    printk("\n");
    printk("Listing: %s\n", path ? path : "/");
    printk("------------------------------------------\n");
    
    /* TODO: Resolve path para cluster */
    /* Por enquanto, só lista raiz */
    uint32_t cluster = fs->bpb.root_cluster;
    
    fat32_list_dir_flat(fs, cluster, "", &file_count, &dir_count);
    
    printk("------------------------------------------\n");
    printk("Total: %d files, %d directories\n", file_count, dir_count);
    printk("\n");
}
