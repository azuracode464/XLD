// spinlock.h
#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t lock;
    uint32_t cpu_id;  // Para debugging
} spinlock_t;

#define SPINLOCK_INIT {0, 0}

static inline void spinlock_lock(spinlock_t *lock) {
    asm volatile (
        "1:\n"
        "lock bts $0, %0\n"   // Test and set bit 0
        "jc 1b\n"             // Jump if already locked
        : "+m"(lock->lock)
        :
        : "memory", "cc"
    );
}

static inline void spinlock_unlock(spinlock_t *lock) {
    asm volatile (
        "movb $0, %0\n"
        : "+m"(lock->lock)
        :
        : "memory"
    );
}

static inline bool spinlock_trylock(spinlock_t *lock) {
    bool result;
    asm volatile (
        "lock bts $0, %1\n"
        "setnc %0\n"
        : "=r"(result), "+m"(lock->lock)
        :
        : "memory", "cc"
    );
    return result;
}
