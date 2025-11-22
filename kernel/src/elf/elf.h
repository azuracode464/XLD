#ifndef ELF_H
#define ELF_H

#include <stdint.h>
#include <stddef.h>
#include "../process/process.h" // Para pid_t

// --- Estruturas Padrão do Formato ELF64 ---

#define EI_NIDENT 16

// Cabeçalho principal do arquivo ELF
typedef struct {
    unsigned char e_ident[EI_NIDENT]; // Deve começar com 0x7F, 'E', 'L', 'F'
    uint16_t      e_type;             // Tipo do arquivo (ex: executável)
    uint16_t      e_machine;          // Arquitetura (ex: x86-64)
    uint32_t      e_version;
    uint64_t      e_entry;            // Ponto de entrada virtual do programa
    uint64_t      e_phoff;            // Offset para os cabeçalhos de programa
    uint64_t      e_shoff;
    uint32_t      e_flags;
    uint16_t      e_ehsize;
    uint16_t      e_phentsize;        // Tamanho de uma entrada no cabeçalho de programa
    uint16_t      e_phnum;            // Número de cabeçalhos de programa
    uint16_t      e_shentsize;
    uint16_t      e_shnum;
    uint16_t      e_shstrndx;
} __attribute__((packed)) Elf64_Ehdr;

// Cabeçalho de Programa (Program Header) - descreve um segmento
typedef struct {
    uint32_t p_type;    // Tipo do segmento (ex: PT_LOAD)
    uint32_t p_flags;   // Flags de permissão (Read, Write, Execute)
    uint64_t p_offset;  // Offset do segmento no arquivo ELF
    uint64_t p_vaddr;   // Endereço virtual onde o segmento deve ser carregado
    uint64_t p_paddr;   // Endereço físico (ignorado na maioria dos sistemas)
    uint64_t p_filesz;  // Tamanho do segmento no arquivo
    uint64_t p_memsz;   // Tamanho do segmento na memória (pode ser maior que filesz para .bss)
    uint64_t p_align;
} __attribute__((packed)) Elf64_Phdr;

// --- Constantes ELF ---

// Valores para e_ident
#define EI_MAG0 0 // 0x7F
#define EI_MAG1 1 // 'E'
#define EI_MAG2 2 // 'L'
#define EI_MAG3 3 // 'F'

// Valores para p_type (tipo de segmento)
#define PT_LOAD 1 // Segmento carregável (código, dados, etc.)

// --- Protótipo da Função do Carregador ---

/**
 * @brief Carrega um programa no formato ELF a partir de um buffer de memória
 *        e cria um novo processo para executá-lo.
 * 
 * @param elf_data Ponteiro para o início dos dados do arquivo ELF na memória.
 * @return O PID do novo processo, ou -1 em caso de falha.
 */
pid_t elf_load_process(void* elf_data);

#endif // ELF_H

