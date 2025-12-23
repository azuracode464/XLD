/* vfs.h - Virtual Filesystem Layer com Bind Automático
 * 
 * Sistema de bind transparente:
 * - Filesystem monta em /mnt
 * - VFS mapeia automaticamente / -> /mnt
 * - Usuário acessa /root/file.txt, VFS traduz para /mnt/root/file.txt
 */

#ifndef VFS_H
#define VFS_H

#include <stdint.h>
#include <stddef.h>

/* ===================== CONSTANTES ===================== */

/* Flags para vfs_open() */
#define VFS_READ    0x01
#define VFS_WRITE   0x02
#define VFS_APPEND  0x04
#define VFS_CREATE  0x08
#define VFS_TRUNC   0x10

/* Tipos de arquivo */
#define VFS_TYPE_FILE    0x01
#define VFS_TYPE_DIR     0x02
#define VFS_TYPE_LINK    0x04
#define VFS_TYPE_CHARDEV 0x08
#define VFS_TYPE_BLKDEV  0x10

/* Seek whence */
#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

/* Códigos de erro */
#define VFS_OK           0
#define VFS_ERR_GENERIC -1
#define VFS_ERR_NOMEM   -2
#define VFS_ERR_NOTFOUND -3
#define VFS_ERR_EXISTS  -4
#define VFS_ERR_NOTDIR  -5
#define VFS_ERR_ISDIR   -6
#define VFS_ERR_PERM    -7
#define VFS_ERR_NOSPACE -8
#define VFS_ERR_NOTSUPP -9

/* ===================== ESTRUTURAS PÚBLICAS ===================== */

struct vfs_stat {
    uint32_t st_mode;
    uint32_t st_size;
    uint32_t st_blocks;
    uint32_t st_atime;
    uint32_t st_mtime;
    uint32_t st_ctime;
};

struct vfs_dirent {
    uint32_t d_ino;
    uint8_t  d_type;
    char     d_name[256];
};

typedef struct vfs_file vfs_file_t;

/* ===================== DRIVER INTERFACE ===================== */

struct vfs_mount;

struct vfs_fs_ops {
    /* Arquivos */
    int (*open)(struct vfs_mount *mnt, const char *path, int flags, void **handle);
    int (*close)(void *handle);
    int (*read)(void *handle, void *buf, size_t count);
    int (*write)(void *handle, const void *buf, size_t count);
    int (*seek)(void *handle, int64_t offset, int whence);
    int64_t (*tell)(void *handle);
    int (*truncate)(void *handle, size_t length);
    int (*sync)(void *handle);
    
    /* Diretórios */
    int (*mkdir)(struct vfs_mount *mnt, const char *path);
    int (*rmdir)(struct vfs_mount *mnt, const char *path);
    int (*unlink)(struct vfs_mount *mnt, const char *path);
    int (*rename)(struct vfs_mount *mnt, const char *oldpath, const char *newpath);
    int (*stat)(struct vfs_mount *mnt, const char *path, struct vfs_stat *st);
    
    /* Iteração */
    int (*opendir)(struct vfs_mount *mnt, const char *path, void **handle);
    int (*readdir)(void *handle, struct vfs_dirent *dirent);
    int (*closedir)(void *handle);
    
    /* Filesystem */
    int (*sync_fs)(struct vfs_mount *mnt);
    int (*statfs)(struct vfs_mount *mnt, uint64_t *total, uint64_t *free);
};

struct vfs_mount {
    char *path;
    struct vfs_fs_ops *ops;
    void *private_data;
    struct vfs_mount *next;
};

/* ===================== API PÚBLICA ===================== */

/* Inicialização */
void vfs_init(void);

/* Montagem 
 * NOTA: O filesystem DEVE ser montado em /mnt
 * O VFS automaticamente fará bind de / -> /mnt
 */
int vfs_mount(const char *path, struct vfs_fs_ops *ops, void *private_data);
int vfs_umount(const char *path);

/* Arquivos */
int vfs_open(const char *path, int flags, vfs_file_t **file);
int vfs_close(vfs_file_t *file);
int vfs_read(vfs_file_t *file, void *buf, size_t count);
int vfs_write(vfs_file_t *file, const void *buf, size_t count);
int vfs_seek(vfs_file_t *file, int64_t offset, int whence);
int64_t vfs_tell(vfs_file_t *file);
int vfs_truncate(vfs_file_t *file, size_t length);
int vfs_sync(vfs_file_t *file);

/* Diretórios */
int vfs_mkdir(const char *path);
int vfs_rmdir(const char *path);
int vfs_unlink(const char *path);
int vfs_rename(const char *oldpath, const char *newpath);
int vfs_stat(const char *path, struct vfs_stat *st);

/* Iteração */
int vfs_opendir(const char *path, vfs_file_t **dir);
int vfs_readdir(vfs_file_t *dir, struct vfs_dirent *dirent);
int vfs_closedir(vfs_file_t *dir);

/* Utilitários */
int vfs_exists(const char *path);
int vfs_is_dir(const char *path);
int vfs_is_file(const char *path);
uint32_t vfs_get_size(const char *path);
int vfs_sync_all(void);

/* Debug */
void vfs_dump_mounts(void);

#endif /* VFS_H */
