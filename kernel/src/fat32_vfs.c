/* fat32_vfs.c - Adapter FAT32 para VFS
 * 
 * ATUALIZADO: Agora o FAT32 espera paths relativos ao /mnt
 * O VFS já faz a tradução / -> /mnt antes de chamar
 */

#include "vfs.h"
#include "string.h"
#include "kmalloc.h"
#include "fat32.h"

extern void klog(int level, const char *fmt, ...);

#define KLOG_INFO  0
#define KLOG_WARN  1
#define KLOG_ERROR 2
#define KLOG_DEBUG 3

/* ===================== ESTRUTURAS ===================== */

typedef struct {
    fat32_fs_t *fatfs;
} fat32_vfs_context_t;

typedef struct {
    fat32_file_t *fat_file;
    uint8_t is_dir;
    uint32_t dir_cluster;
    uint32_t dir_index;
} fat32_vfs_handle_t;

/* ===================== OPEN/CLOSE ===================== */

static int fat32_vfs_open(struct vfs_mount *mnt, const char *path, int flags, void **handle) {
    fat32_vfs_context_t *ctx = (fat32_vfs_context_t *)mnt->private_data;
    if (!ctx || !ctx->fatfs) {
        return VFS_ERR_GENERIC;
    }
    
    /* Path já vem relativo ao /mnt do VFS
     * Exemplos recebidos aqui:
     *   "root/hello.txt"  (se usuário chamou /root/hello.txt)
     *   ""               (se usuário chamou /)
     *   "documentos/x"   (se usuário chamou /documentos/x)
     */
    
    /* FAT32 espera path começando com / */
    char fat_path[256];
    fat_path[0] = '/';
    
    if (path && *path) {
        strncpy(fat_path + 1, path, 254);
        fat_path[255] = '\0';
    } else {
        fat_path[1] = '\0';  /* root */
    }
    
    klog(KLOG_DEBUG, "[FAT32-VFS] open: rel='%s' -> fat_path='%s'", 
         path ? path : "(empty)", fat_path);
    
    /* Converte flags */
    const char *mode = (flags & VFS_WRITE) ? "w" : "r";
    
    /* Chama driver FAT32 */
    fat32_file_t *fat_file = fat32_open(ctx->fatfs, fat_path, mode);
    if (!fat_file) {
        klog(KLOG_DEBUG, "[FAT32-VFS] fat32_open failed for '%s'", fat_path);
        return VFS_ERR_NOTFOUND;
    }
    
    /* Aloca handle */
    fat32_vfs_handle_t *h = kmalloc(sizeof(fat32_vfs_handle_t));
    if (!h) {
        fat32_close(fat_file);
        return VFS_ERR_NOMEM;
    }
    
    memset(h, 0, sizeof(fat32_vfs_handle_t));
    h->fat_file = fat_file;
    h->is_dir = 0;
    
    *handle = h;
    return VFS_OK;
}

static int fat32_vfs_close(void *handle) {
    if (!handle) return VFS_ERR_GENERIC;
    
    fat32_vfs_handle_t *h = (fat32_vfs_handle_t *)handle;
    
    if (h->fat_file) {
        fat32_close(h->fat_file);
    }
    
    kfree(h);
    return VFS_OK;
}

/* ===================== READ/WRITE ===================== */

static int fat32_vfs_read(void *handle, void *buf, size_t count) {
    if (!handle || !buf) return VFS_ERR_GENERIC;
    
    fat32_vfs_handle_t *h = (fat32_vfs_handle_t *)handle;
    
    if (!h->fat_file) {
        return VFS_ERR_GENERIC;
    }
    
    int ret = fat32_read(h->fat_file, buf, (int)count);
    
    return ret;
}

static int fat32_vfs_write(void *handle, const void *buf, size_t count) {
    if (!handle || !buf) return VFS_ERR_GENERIC;
    
    fat32_vfs_handle_t *h = (fat32_vfs_handle_t *)handle;
    
    if (!h->fat_file) {
        return VFS_ERR_GENERIC;
    }
    
    int ret = fat32_write(h->fat_file, buf, (int)count);
    
    return ret;
}

/* ===================== SEEK/TELL ===================== */

static int fat32_vfs_seek(void *handle, int64_t offset, int whence) {
    if (!handle) return VFS_ERR_GENERIC;
    
    fat32_vfs_handle_t *h = (fat32_vfs_handle_t *)handle;
    if (!h->fat_file) return VFS_ERR_GENERIC;
    
    /* Seek simplificado - apenas SEEK_SET para início */
    if (whence == SEEK_SET) {
        h->fat_file->curr_cluster = h->fat_file->first_cluster;
        h->fat_file->curr_offset = 0;
        h->fat_file->file_pos = 0;
        
        if (offset > 0) {
            /* Avança lendo */
            char dummy[512];
            int remaining = (int)offset;
            while (remaining > 0) {
                int chunk = remaining > 512 ? 512 : remaining;
                int r = fat32_read(h->fat_file, dummy, chunk);
                if (r <= 0) break;
                remaining -= r;
            }
        }
        return VFS_OK;
    }
    
    return VFS_ERR_NOTSUPP;
}

static int64_t fat32_vfs_tell(void *handle) {
    if (!handle) return VFS_ERR_GENERIC;
    
    fat32_vfs_handle_t *h = (fat32_vfs_handle_t *)handle;
    if (!h->fat_file) return VFS_ERR_GENERIC;
    
    return (int64_t)h->fat_file->file_pos;
}

static int fat32_vfs_truncate(void *handle, size_t length) {
    return VFS_ERR_NOTSUPP;
}

static int fat32_vfs_sync(void *handle) {
    if (!handle) return VFS_ERR_GENERIC;
    
    fat32_vfs_handle_t *h = (fat32_vfs_handle_t *)handle;
    if (!h->fat_file || !h->fat_file->fs) return VFS_ERR_GENERIC;
    
    if (h->fat_file->fs->dev->flush) {
        return h->fat_file->fs->dev->flush();
    }
    
    return VFS_OK;
}

/* ===================== DIRETÓRIOS ===================== */

static int fat32_vfs_mkdir(struct vfs_mount *mnt, const char *path) {
    fat32_vfs_context_t *ctx = (fat32_vfs_context_t *)mnt->private_data;
    if (!ctx || !ctx->fatfs) return VFS_ERR_GENERIC;
    
    /* Prepara path para FAT32 */
    char fat_path[256];
    fat_path[0] = '/';
    
    if (path && *path) {
        strncpy(fat_path + 1, path, 254);
        fat_path[255] = '\0';
    } else {
        return VFS_ERR_GENERIC;
    }
    
    int ret = fat32_mkdir(ctx->fatfs, fat_path);
    
    return (ret == 0) ? VFS_OK : VFS_ERR_GENERIC;
}

static int fat32_vfs_rmdir(struct vfs_mount *mnt, const char *path) {
    return VFS_ERR_NOTSUPP;
}

static int fat32_vfs_unlink(struct vfs_mount *mnt, const char *path) {
    return VFS_ERR_NOTSUPP;
}

static int fat32_vfs_rename(struct vfs_mount *mnt, const char *oldpath, const char *newpath) {
    return VFS_ERR_NOTSUPP;
}

static int fat32_vfs_stat(struct vfs_mount *mnt, const char *path, struct vfs_stat *st) {
    fat32_vfs_context_t *ctx = (fat32_vfs_context_t *)mnt->private_data;
    if (!ctx || !ctx->fatfs || !st) return VFS_ERR_GENERIC;
    
    memset(st, 0, sizeof(struct vfs_stat));
    
    /* Prepara path para FAT32 */
    char fat_path[256];
    fat_path[0] = '/';
    
    if (path && *path) {
        strncpy(fat_path + 1, path, 254);
        fat_path[255] = '\0';
    } else {
        /* Root */
        st->st_mode = VFS_TYPE_DIR;
        st->st_size = 0;
        return VFS_OK;
    }
    
    /* Resolve no FAT32 */
    uint32_t parent;
    char name[11];
    
    if (fat32_resolve(ctx->fatfs, fat_path, &parent, name) < 0) {
        return VFS_ERR_NOTFOUND;
    }
    
    if (name[0] == '\0') {
        st->st_mode = VFS_TYPE_DIR;
        st->st_size = 0;
        return VFS_OK;
    }
    
    /* Procura entrada */
    struct fat32_dirent entry;
    if (fat32_find(ctx->fatfs, parent, name, &entry, NULL, NULL) < 0) {
        return VFS_ERR_NOTFOUND;
    }
    
    if (entry.attr & FAT32_ATTR_DIRECTORY) {
        st->st_mode = VFS_TYPE_DIR;
        st->st_size = 0;
    } else {
        st->st_mode = VFS_TYPE_FILE;
        st->st_size = entry.size;
    }
    
    return VFS_OK;
}

/* ===================== ITERAÇÃO ===================== */

static int fat32_vfs_opendir(struct vfs_mount *mnt, const char *path, void **handle) {
    fat32_vfs_context_t *ctx = (fat32_vfs_context_t *)mnt->private_data;
    if (!ctx || !ctx->fatfs) return VFS_ERR_GENERIC;
    
    /* Prepara path */
    char fat_path[256];
    fat_path[0] = '/';
    
    if (path && *path) {
        strncpy(fat_path + 1, path, 254);
        fat_path[255] = '\0';
    } else {
        fat_path[1] = '\0';
    }
    
    /* Resolve */
    uint32_t parent;
    char name[11];
    
    if (fat32_resolve(ctx->fatfs, fat_path, &parent, name) < 0) {
        return VFS_ERR_NOTFOUND;
    }
    
    uint32_t dir_cluster;
    
    if (name[0] == '\0') {
        dir_cluster = parent;
    } else {
        struct fat32_dirent entry;
        if (fat32_find(ctx->fatfs, parent, name, &entry, NULL, NULL) < 0) {
            return VFS_ERR_NOTFOUND;
        }
        
        if (!(entry.attr & FAT32_ATTR_DIRECTORY)) {
            return VFS_ERR_NOTDIR;
        }
        
        dir_cluster = entry.cluster_lo | ((uint32_t)entry.cluster_hi << 16);
    }
    
    /* Aloca handle */
    fat32_vfs_handle_t *h = kmalloc(sizeof(fat32_vfs_handle_t));
    if (!h) return VFS_ERR_NOMEM;
    
    memset(h, 0, sizeof(fat32_vfs_handle_t));
    h->is_dir = 1;
    h->dir_cluster = dir_cluster;
    h->dir_index = 0;
    
    *handle = h;
    return VFS_OK;
}

static int fat32_vfs_readdir(void *handle, struct vfs_dirent *dirent) {
    /* TODO: Implementar corretamente */
    return VFS_ERR_NOTSUPP;
}

static int fat32_vfs_closedir(void *handle) {
    if (!handle) return VFS_ERR_GENERIC;
    kfree(handle);
    return VFS_OK;
}

/* ===================== FILESYSTEM ===================== */

static int fat32_vfs_sync_fs(struct vfs_mount *mnt) {
    fat32_vfs_context_t *ctx = (fat32_vfs_context_t *)mnt->private_data;
    if (!ctx || !ctx->fatfs) return VFS_ERR_GENERIC;
    
    if (ctx->fatfs->dev->flush) {
        return ctx->fatfs->dev->flush();
    }
    
    return VFS_OK;
}

static int fat32_vfs_statfs(struct vfs_mount *mnt, uint64_t *total, uint64_t *free) {
    fat32_vfs_context_t *ctx = (fat32_vfs_context_t *)mnt->private_data;
    if (!ctx || !ctx->fatfs) return VFS_ERR_GENERIC;
    
    uint64_t total_clusters = ctx->fatfs->bpb.total_sectors_32 / 
                              ctx->fatfs->bpb.sectors_per_cluster;
    
    if (total) {
        *total = total_clusters * ctx->fatfs->bytes_per_cluster;
    }
    
    if (free) {
        *free = total_clusters * ctx->fatfs->bytes_per_cluster / 2;
    }
    
    return VFS_OK;
}

/* ===================== TABELA DE OPERAÇÕES ===================== */

static struct vfs_fs_ops fat32_vfs_ops = {
    .open     = fat32_vfs_open,
    .close    = fat32_vfs_close,
    .read     = fat32_vfs_read,
    .write    = fat32_vfs_write,
    .seek     = fat32_vfs_seek,
    .tell     = fat32_vfs_tell,
    .truncate = fat32_vfs_truncate,
    .sync     = fat32_vfs_sync,
    
    .mkdir    = fat32_vfs_mkdir,
    .rmdir    = fat32_vfs_rmdir,
    .unlink   = fat32_vfs_unlink,
    .rename   = fat32_vfs_rename,
    .stat     = fat32_vfs_stat,
    
    .opendir  = fat32_vfs_opendir,
    .readdir  = fat32_vfs_readdir,
    .closedir = fat32_vfs_closedir,
    
    .sync_fs  = fat32_vfs_sync_fs,
    .statfs   = fat32_vfs_statfs,
};

/* ===================== API PÚBLICA ===================== */

struct vfs_fs_ops* vfs_get_fat32_ops(void) {
    return &fat32_vfs_ops;
}

void* vfs_create_fat32_context(fat32_fs_t *fatfs) {
    if (!fatfs) {
        klog(KLOG_ERROR, "[FAT32-VFS] create_context: NULL fatfs");
        return NULL;
    }
    
    fat32_vfs_context_t *ctx = kmalloc(sizeof(fat32_vfs_context_t));
    if (!ctx) {
        klog(KLOG_ERROR, "[FAT32-VFS] create_context: Out of memory");
        return NULL;
    }
    
    ctx->fatfs = fatfs;
    
    klog(KLOG_INFO, "[FAT32-VFS] Context created");
    return ctx;
}

void vfs_destroy_fat32_context(void *context) {
    if (context) {
        kfree(context);
    }
}
