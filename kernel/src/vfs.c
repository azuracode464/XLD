/* vfs.c - Virtual Filesystem com Bind Automático / -> /mnt
 * 
 * Sistema de bind transparente:
 * 1. Driver monta FAT32 em /mnt
 * 2. VFS cria bind automático / -> /mnt
 * 3. Acesso a /root/file.txt é traduzido para /mnt/root/file.txt
 * 4. Usuário não vê /mnt, apenas /
 */

#include "vfs.h"
#include "string.h"
#include "kmalloc.h"

extern void klog(int level, const char *fmt, ...);

#define KLOG_INFO  0
#define KLOG_WARN  1
#define KLOG_ERROR 2
#define KLOG_DEBUG 3

/* ===================== ESTRUTURAS INTERNAS ===================== */

struct vfs_file {
    struct vfs_mount *mount;
    void *fs_handle;
    int flags;
    int64_t pos;
    uint8_t is_dir;
};

/* ===================== VARIÁVEIS GLOBAIS ===================== */

static struct vfs_mount *mounts = NULL;
static struct vfs_mount *root_bind = NULL;  /* Bind de / para /mnt */
static int vfs_initialized = 0;

/* ===================== PATH TRANSLATION ===================== */

/* Normaliza path removendo barras duplas e espaços */
static int normalize_path(char *path) {
    if (!path || !*path) return -1;
    
    /* Garante que começa com / */
    if (path[0] != '/') return -1;
    
    char *src = path;
    char *dst = path;
    int last_slash = 0;
    
    while (*src) {
        if (*src == '/') {
            if (!last_slash) {
                *dst++ = '/';
                last_slash = 1;
            }
            src++;
        } else if (*src >= 32 && *src <= 126) {  /* 
        } Char imprimível */
            *dst++ = *src++;
            last_slash = 0;
        } else {
            src++;  /* Pula char inválido */
        }
    }
    
    /* Remove barra final (exceto para "/") */
    if (dst > path + 1 && dst[-1] == '/') {
        dst--;
    }
    
    *dst = '\0';
    
    /* Garante que sempre tem pelo menos "/" */
    if (path[0] == '\0') {
        path[0] = '/';
        path[1] = '\0';
    }
    
    return 0;
}
/* Traduz path do usuário (/) para path real (/mnt)
 * 
 * Exemplos:
 *   /root/hello.txt  -> /mnt/root/hello.txt
 *   /                -> /mnt
 *   /documentos/x.txt -> /mnt/documentos/x.txt
 */
static int translate_path(const char *user_path, char *real_path, size_t size) {
    if (!user_path || !real_path || size < 2) {
        return -1;
    }
    
    /* Valida que começa com / */
    if (user_path[0] != '/') {
        klog(KLOG_ERROR, "[VFS] Path must start with /: '%s'", user_path);
        return -1;
    }
    
    /* Se não há bind, copia direto */
    if (!root_bind) {
        strncpy(real_path, user_path, size - 1);
        real_path[size - 1] = '\0';
        return normalize_path(real_path);
    }
    
    /* Constrói /mnt + path_do_usuario */
    size_t mnt_len = strlen(root_bind->path);
    size_t user_len = strlen(user_path);
    
    /* Caso especial: / -> /mnt */
    if (user_len == 1 && user_path[0] == '/') {
        strncpy(real_path, root_bind->path, size - 1);
        real_path[size - 1] = '\0';
        return 0;
    }
    
    /* Caso geral: /algo -> /mnt/algo */
    if (mnt_len + user_len >= size) {
        klog(KLOG_ERROR, "[VFS] Path too long after translation");
        return -1;
    }
    
    strcpy(real_path, root_bind->path);
    strcat(real_path, user_path);
    
    return normalize_path(real_path);
}

/* ===================== MOUNT POINT LOOKUP ===================== */

/* Encontra mount point para um path REAL (já traduzido) */
static struct vfs_mount* find_mount_for_real_path(const char *real_path, 
                                                   const char **rel_path) {
    if (!real_path || !*real_path) return NULL;
    
    /* Busca mount point com maior match */
    struct vfs_mount *best = NULL;
    size_t best_len = 0;
    
    for (struct vfs_mount *m = mounts; m; m = m->next) {
        size_t mlen = strlen(m->path);
        
        /* Checa se path começa com mount point */
        if (strncmp(real_path, m->path, mlen) == 0) {
            /* Valida que é boundary correto (/mnt/x não deve match /mnt/xyz) */
            if (real_path[mlen] == '/' || real_path[mlen] == '\0') {
                if (mlen > best_len) {
                    best = m;
                    best_len = mlen;
                }
            }
        }
    }
    
    if (best && rel_path) {
        /* Retorna path relativo ao mount point */
        const char *rel = real_path + best_len;
        
        /* Pula barra inicial se houver */
        while (*rel == '/') rel++;
        
        *rel_path = rel;
    }
    
    return best;
}

/* ===================== INICIALIZAÇÃO ===================== */

void vfs_init(void) {
    if (vfs_initialized) {
        klog(KLOG_WARN, "[VFS] Already initialized");
        return;
    }
    
    mounts = NULL;
    root_bind = NULL;
    vfs_initialized = 1;
    
    klog(KLOG_INFO, "[VFS] Initialized");
}

/* ===================== MONTAGEM ===================== */

int vfs_mount(const char *path, struct vfs_fs_ops *ops, void *private_data) {
    if (!vfs_initialized) vfs_init();
    
    if (!path || !ops) {
        klog(KLOG_ERROR, "[VFS] mount: Invalid arguments");
        return VFS_ERR_GENERIC;
    }
    
    /* Aloca e normaliza path */
    char norm_path[256];
    strncpy(norm_path, path, 255);
    norm_path[255] = '\0';
    
    if (normalize_path(norm_path) < 0) {
        klog(KLOG_ERROR, "[VFS] mount: Invalid path '%s'", path);
        return VFS_ERR_GENERIC;
    }
    
    /* Verifica se já existe */
    for (struct vfs_mount *m = mounts; m; m = m->next) {
        if (strcmp(m->path, norm_path) == 0) {
            klog(KLOG_ERROR, "[VFS] mount: Path '%s' already mounted", norm_path);
            return VFS_ERR_EXISTS;
        }
    }
    
    /* Aloca mount point */
    struct vfs_mount *m = kmalloc(sizeof(struct vfs_mount));
    if (!m) {
        klog(KLOG_ERROR, "[VFS] mount: Out of memory");
        return VFS_ERR_NOMEM;
    }
    
    m->path = kmalloc(strlen(norm_path) + 1);
    if (!m->path) {
        kfree(m);
        return VFS_ERR_NOMEM;
    }
    
    strcpy(m->path, norm_path);
    m->ops = ops;
    m->private_data = private_data;
    m->next = mounts;
    mounts = m;
    
    klog(KLOG_INFO, "[VFS] Mounted at '%s'", m->path);
    
    /* Se montou em /mnt, cria bind automático / -> /mnt */
    if (strcmp(norm_path, "/mnt") == 0) {
        root_bind = m;
        klog(KLOG_INFO, "[VFS] Auto-bind: / -> /mnt");
        klog(KLOG_INFO, "[VFS] User paths like /root/file.txt map to /mnt/root/file.txt");
    }
    
    return VFS_OK;
}

int vfs_umount(const char *path) {
    if (!path) return VFS_ERR_GENERIC;
    
    char norm_path[256];
    strncpy(norm_path, path, 255);
    norm_path[255] = '\0';
    normalize_path(norm_path);
    
    struct vfs_mount **prev = &mounts;
    
    for (struct vfs_mount *m = mounts; m; m = m->next) {
        if (strcmp(m->path, norm_path) == 0) {
            *prev = m->next;
            
            /* Remove bind se era o /mnt */
            if (m == root_bind) {
                root_bind = NULL;
                klog(KLOG_INFO, "[VFS] Removed auto-bind / -> /mnt");
            }
            
            klog(KLOG_INFO, "[VFS] Unmounted '%s'", m->path);
            
            kfree(m->path);
            kfree(m);
            return VFS_OK;
        }
        prev = &m->next;
    }
    
    klog(KLOG_ERROR, "[VFS] umount: Path '%s' not mounted", norm_path);
    return VFS_ERR_NOTFOUND;
}

/* ===================== ARQUIVOS ===================== */

int vfs_open(const char *path, int flags, vfs_file_t **file) {
    if (!file) return VFS_ERR_GENERIC;
    *file = NULL;
    
    if (!path || path[0] != '/') {
        klog(KLOG_ERROR, "[VFS] open: Invalid path");
        return VFS_ERR_GENERIC;
    }
    
    /* Traduz / -> /mnt */
    char real_path[256];
    if (translate_path(path, real_path, sizeof(real_path)) < 0) {
        klog(KLOG_ERROR, "[VFS] open: Path translation failed for '%s'", path);
        return VFS_ERR_GENERIC;
    }
    
    klog(KLOG_DEBUG, "[VFS] open: '%s' -> '%s'", path, real_path);
    
    /* Encontra mount point */
    const char *rel_path;
    struct vfs_mount *m = find_mount_for_real_path(real_path, &rel_path);
    
    if (!m) {
        klog(KLOG_ERROR, "[VFS] open: No mount for '%s' (real: '%s')", 
             path, real_path);
        return VFS_ERR_NOTFOUND;
    }
    
    if (!m->ops->open) {
        return VFS_ERR_NOTSUPP;
    }
    
    /* Aloca handle */
    vfs_file_t *f = kmalloc(sizeof(vfs_file_t));
    if (!f) return VFS_ERR_NOMEM;
    
    memset(f, 0, sizeof(vfs_file_t));
    f->mount = m;
    f->flags = flags;
    f->pos = 0;
    f->is_dir = 0;
    
    klog(KLOG_DEBUG, "[VFS] open: Calling driver with rel_path='%s'", rel_path);
    
    /* Chama driver */
    int ret = m->ops->open(m, rel_path, flags, &f->fs_handle);
    
    if (ret < 0) {
        kfree(f);
        klog(KLOG_ERROR, "[VFS] open: Driver error %d for '%s'", ret, path);
        return ret;
    }
    
    *file = f;
    klog(KLOG_INFO, "[VFS] Opened '%s' successfully", path);
    return VFS_OK;
}

int vfs_close(vfs_file_t *file) {
    if (!file) return VFS_ERR_GENERIC;
    
    int ret = VFS_OK;
    
    if (file->mount->ops->close && file->fs_handle) {
        ret = file->mount->ops->close(file->fs_handle);
    }
    
    kfree(file);
    return ret;
}

int vfs_read(vfs_file_t *file, void *buf, size_t count) {
    if (!file || !buf) return VFS_ERR_GENERIC;
    
    if (!(file->flags & VFS_READ)) {
        return VFS_ERR_PERM;
    }
    
    if (!file->mount->ops->read) {
        return VFS_ERR_NOTSUPP;
    }
    
    if (count == 0) return 0;
    
    int ret = file->mount->ops->read(file->fs_handle, buf, count);
    
    if (ret > 0) {
        file->pos += ret;
    }
    
    return ret;
}

int vfs_write(vfs_file_t *file, const void *buf, size_t count) {
    if (!file || !buf) return VFS_ERR_GENERIC;
    
    if (!(file->flags & VFS_WRITE)) {
        return VFS_ERR_PERM;
    }
    
    if (!file->mount->ops->write) {
        return VFS_ERR_NOTSUPP;
    }
    
    if (count == 0) return 0;
    
    int ret = file->mount->ops->write(file->fs_handle, buf, count);
    
    if (ret > 0) {
        file->pos += ret;
    }
    
    return ret;
}

int vfs_seek(vfs_file_t *file, int64_t offset, int whence) {
    if (!file) return VFS_ERR_GENERIC;
    
    if (file->mount->ops->seek) {
        int ret = file->mount->ops->seek(file->fs_handle, offset, whence);
        
        if (ret == 0 && file->mount->ops->tell) {
            file->pos = file->mount->ops->tell(file->fs_handle);
        }
        return ret;
    }
    
    /* Fallback simples */
    if (whence == SEEK_SET) {
        file->pos = offset;
    } else if (whence == SEEK_CUR) {
        file->pos += offset;
    }
    
    return VFS_OK;
}

int64_t vfs_tell(vfs_file_t *file) {
    if (!file) return VFS_ERR_GENERIC;
    
    if (file->mount->ops->tell) {
        return file->mount->ops->tell(file->fs_handle);
    }
    
    return file->pos;
}

int vfs_truncate(vfs_file_t *file, size_t length) {
    if (!file) return VFS_ERR_GENERIC;
    
    if (!(file->flags & VFS_WRITE)) {
        return VFS_ERR_PERM;
    }
    
    if (!file->mount->ops->truncate) {
        return VFS_ERR_NOTSUPP;
    }
    
    return file->mount->ops->truncate(file->fs_handle, length);
}

int vfs_sync(vfs_file_t *file) {
    if (!file) return VFS_ERR_GENERIC;
    
    if (file->mount->ops->sync) {
        return file->mount->ops->sync(file->fs_handle);
    }
    
    return VFS_OK;
}

/* ===================== DIRETÓRIOS ===================== */

int vfs_mkdir(const char *path) {
    if (!path || path[0] != '/') return VFS_ERR_GENERIC;
    
    char real_path[256];
    if (translate_path(path, real_path, sizeof(real_path)) < 0) {
        return VFS_ERR_GENERIC;
    }
    
    const char *rel_path;
    struct vfs_mount *m = find_mount_for_real_path(real_path, &rel_path);
    
    if (!m || !m->ops->mkdir) return VFS_ERR_NOTSUPP;
    
    return m->ops->mkdir(m, rel_path);
}

int vfs_rmdir(const char *path) {
    if (!path || path[0] != '/') return VFS_ERR_GENERIC;
    
    char real_path[256];
    if (translate_path(path, real_path, sizeof(real_path)) < 0) {
        return VFS_ERR_GENERIC;
    }
    
    const char *rel_path;
    struct vfs_mount *m = find_mount_for_real_path(real_path, &rel_path);
    
    if (!m || !m->ops->rmdir) return VFS_ERR_NOTSUPP;
    
    return m->ops->rmdir(m, rel_path);
}

int vfs_unlink(const char *path) {
    if (!path || path[0] != '/') return VFS_ERR_GENERIC;
    
    char real_path[256];
    if (translate_path(path, real_path, sizeof(real_path)) < 0) {
        return VFS_ERR_GENERIC;
    }
    
    const char *rel_path;
    struct vfs_mount *m = find_mount_for_real_path(real_path, &rel_path);
    
    if (!m || !m->ops->unlink) return VFS_ERR_NOTSUPP;
    
    return m->ops->unlink(m, rel_path);
}

int vfs_rename(const char *oldpath, const char *newpath) {
    if (!oldpath || !newpath) return VFS_ERR_GENERIC;
    
    char old_real[256], new_real[256];
    
    if (translate_path(oldpath, old_real, sizeof(old_real)) < 0 ||
        translate_path(newpath, new_real, sizeof(new_real)) < 0) {
        return VFS_ERR_GENERIC;
    }
    
    const char *old_rel, *new_rel;
    struct vfs_mount *old_m = find_mount_for_real_path(old_real, &old_rel);
    struct vfs_mount *new_m = find_mount_for_real_path(new_real, &new_rel);
    
    if (old_m != new_m || !old_m || !old_m->ops->rename) {
        return VFS_ERR_NOTSUPP;
    }
    
    return old_m->ops->rename(old_m, old_rel, new_rel);
}

int vfs_stat(const char *path, struct vfs_stat *st) {
    if (!st || !path || path[0] != '/') return VFS_ERR_GENERIC;
    
    char real_path[256];
    if (translate_path(path, real_path, sizeof(real_path)) < 0) {
        return VFS_ERR_GENERIC;
    }
    
    const char *rel_path;
    struct vfs_mount *m = find_mount_for_real_path(real_path, &rel_path);
    
    if (!m || !m->ops->stat) return VFS_ERR_NOTSUPP;
    
    return m->ops->stat(m, rel_path, st);
}

/* ===================== ITERAÇÃO ===================== */

int vfs_opendir(const char *path, vfs_file_t **dir) {
    if (!dir || !path || path[0] != '/') return VFS_ERR_GENERIC;
    *dir = NULL;
    
    char real_path[256];
    if (translate_path(path, real_path, sizeof(real_path)) < 0) {
        return VFS_ERR_GENERIC;
    }
    
    const char *rel_path;
    struct vfs_mount *m = find_mount_for_real_path(real_path, &rel_path);
    
    if (!m || !m->ops->opendir) return VFS_ERR_NOTSUPP;
    
    vfs_file_t *d = kmalloc(sizeof(vfs_file_t));
    if (!d) return VFS_ERR_NOMEM;
    
    memset(d, 0, sizeof(vfs_file_t));
    d->mount = m;
    d->is_dir = 1;
    d->pos = 0;
    
    int ret = m->ops->opendir(m, rel_path, &d->fs_handle);
    
    if (ret < 0) {
        kfree(d);
        return ret;
    }
    
    *dir = d;
    return VFS_OK;
}

int vfs_readdir(vfs_file_t *dir, struct vfs_dirent *dirent) {
    if (!dir || !dir->is_dir || !dirent) return VFS_ERR_GENERIC;
    
    if (!dir->mount->ops->readdir) return VFS_ERR_NOTSUPP;
    
    int ret = dir->mount->ops->readdir(dir->fs_handle, dirent);
    
    if (ret == 0) {
        dir->pos++;
    }
    
    return ret;
}

int vfs_closedir(vfs_file_t *dir) {
    if (!dir || !dir->is_dir) return VFS_ERR_GENERIC;
    
    int ret = VFS_OK;
    
    if (dir->mount->ops->closedir && dir->fs_handle) {
        ret = dir->mount->ops->closedir(dir->fs_handle);
    }
    
    kfree(dir);
    return ret;
}

/* ===================== UTILITÁRIOS ===================== */

int vfs_exists(const char *path) {
    struct vfs_stat st;
    return vfs_stat(path, &st) == 0 ? 1 : 0;
}

int vfs_is_dir(const char *path) {
    struct vfs_stat st;
    if (vfs_stat(path, &st) < 0) return 0;
    return (st.st_mode & VFS_TYPE_DIR) ? 1 : 0;
}

int vfs_is_file(const char *path) {
    struct vfs_stat st;
    if (vfs_stat(path, &st) < 0) return 0;
    return (st.st_mode & VFS_TYPE_FILE) ? 1 : 0;
}

uint32_t vfs_get_size(const char *path) {
    struct vfs_stat st;
    if (vfs_stat(path, &st) < 0) return 0;
    return st.st_size;
}

int vfs_sync_all(void) {
    int errors = 0;
    
    for (struct vfs_mount *m = mounts; m; m = m->next) {
        if (m->ops->sync_fs) {
            if (m->ops->sync_fs(m) < 0) {
                errors++;
            }
        }
    }
    
    return errors ? VFS_ERR_GENERIC : VFS_OK;
}

void vfs_dump_mounts(void) {
    klog(KLOG_INFO, "=== VFS Mount Points ===");
    
    if (!mounts) {
        klog(KLOG_INFO, "(No mounts)");
        return;
    }
    
    for (struct vfs_mount *m = mounts; m; m = m->next) {
        klog(KLOG_INFO, "  %s%s", 
             m->path,
             (m == root_bind) ? " [BIND: / -> /mnt]" : "");
    }
}
