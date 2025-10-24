#pragma once
#include <stdint.h>
#include <stddef.h> // Para size_t

#define MAX_CPU_COUNT 256 // Um limite generoso para o número de CPUs

namespace SMP {
    // Estrutura para guardar as informações que nos interessam
    struct CPU_Info {
        uint32_t processor_id;
        uint32_t lapic_id;
    };

    // Função para iniciar o censo de processadores
    void initialize();

    // Função para obter o número de processadores encontrados
    size_t get_cpu_count();

    // Função para obter a lista de processadores
    CPU_Info* get_cpu_list();
}

