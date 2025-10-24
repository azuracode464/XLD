#pragma once
#include <cstdint>

// Estrutura de cabeçalho padrão para todas as tabelas ACPI
struct ACPI_SDTHeader {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__((packed));

namespace ACPI {
    // Função para encontrar uma tabela específica (como a "APIC")
    // a partir da tabela raiz RSDT.
    void* find_table(const char* signature);
}

