#ifndef KHEAP_H
#define KHEAP_H

#include <stddef.h>

// Inicializa o gerenciador de heap.
void kheap_init(void);

// Aloca um bloco de memória.
void* kmalloc(size_t size);

// Libera um bloco de memória previamente alocado.
void kfree(void* ptr);

#endif // KHEAP_H

