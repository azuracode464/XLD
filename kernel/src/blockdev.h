#pragma once
#include <stdint.h>
#include <stddef.h>

typedef struct blockdev {
    int (*read)(uint32_t lba, uint32_t count, void *buf);
    int (*write)(uint32_t lba, uint32_t count, const void *buf);
    int (*flush)(void);
    uint32_t sector_size;
} blockdev_t;

extern blockdev_t *g_root_blockdev;
