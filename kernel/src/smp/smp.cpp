#include "smp.hpp"

// Nosso banco de dados de CPUs, onde guardaremos o resultado do censo.
static SMP::CPU_Info cpu_list[MAX_CPU_COUNT];
static size_t cpu_count = 0;

namespace SMP {

    // Por enquanto, a inicialização não faz nada.
    // Ela será chamada pelo ACPI quando ele encontrar as informações.
    void initialize() {
        // Esta função será preenchida depois.
    }

    size_t get_cpu_count() {
        return cpu_count;
    }

    CPU_Info* get_cpu_list() {
        return cpu_list;
    }
}

