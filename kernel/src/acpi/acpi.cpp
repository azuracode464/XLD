#include "acpi.hpp"
#include "../smp/smp.hpp" // Incluímos o SMP para podermos preencher a lista de CPUs
#include <limine.h>
#include <cstddef>

// =============================================================================
// PEDIDO AO BOOTLOADER E FUNÇÕES AUXILIARES
// =============================================================================

// Pedido ao Limine para nos dar o endereço da tabela raiz do ACPI (RSDP).
namespace {
    __attribute__((used, section(".limine_requests")))
    volatile limine_rsdp_request rsdp_request = {
        .id = LIMINE_RSDP_REQUEST,
        .revision = 0,
        .response = nullptr
    };
}

// Função auxiliar para comparar as assinaturas de 4 bytes das tabelas ACPI.
static bool sig_cmp(const char* sig1, const char* sig2, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (sig1[i] != sig2[i]) return false;
    }
    return true;
}

// =============================================================================
// ESTRUTURAS DA TABELA MADT (Multiple APIC Description Table)
// =============================================================================

// Cabeçalho principal da tabela MADT (assinatura "APIC").
struct MADTHeader {
    ACPI_SDTHeader header;
    uint32_t local_apic_address;
    uint32_t flags;
} __attribute__((packed));

// Cabeçalho genérico para cada entrada *dentro* da MADT.
struct MADTEntryHeader {
    uint8_t type;
    uint8_t length;
} __attribute__((packed));

// Estrutura específica para uma entrada do tipo 0: um processador (Local APIC).
struct MADTEntry_LocalAPIC {
    MADTEntryHeader header;
    uint8_t acpi_processor_id;
    uint8_t apic_id;
    uint32_t flags;
} __attribute__((packed));

// =============================================================================
// COMUNICAÇÃO COM O MÓDULO SMP
// =============================================================================

// Para preencher a lista de CPUs, precisamos de acesso às variáveis globais do smp.cpp.
// Usamos 'extern' para dizer ao compilador que elas existem em outro lugar.
extern SMP::CPU_Info cpu_list[MAX_CPU_COUNT];
extern size_t cpu_count;

// =============================================================================
// LÓGICA PRINCIPAL DO MÓDULO ACPI
// =============================================================================

namespace ACPI {

    /**
     * @brief Analisa a tabela MADT e preenche a lista de processadores do sistema.
     * @param madt Ponteiro para o cabeçalho da tabela MADT.
     */
    void parse_madt(MADTHeader* madt) {
        // O ponteiro para a primeira entrada está logo após o cabeçalho da MADT.
        uintptr_t current_ptr = (uintptr_t)madt + sizeof(MADTHeader);
        uintptr_t end_ptr = (uintptr_t)madt + madt->header.length;

        cpu_count = 0; // Reinicia nosso censo de CPUs.

        // Percorre todas as entradas de hardware listadas na tabela.
        while (current_ptr < end_ptr) {
            MADTEntryHeader* entry_header = (MADTEntryHeader*)current_ptr;

            // Verificamos se a entrada é do tipo 0, que significa "Local APIC" (um núcleo de CPU).
            if (entry_header->type == 0) {
                MADTEntry_LocalAPIC* lapic_entry = (MADTEntry_LocalAPIC*)current_ptr;

                // A flag 1 indica que o processador está habilitado e pronto para ser usado.
                if (lapic_entry->flags & 1) {
                    if (cpu_count < MAX_CPU_COUNT) {
                        // Registra o processador em nosso banco de dados imperial.
                        cpu_list[cpu_count].processor_id = lapic_entry->acpi_processor_id;
                        cpu_list[cpu_count].lapic_id = lapic_entry->apic_id;
                        cpu_count++;
                    }
                }
            }

            // Pula para a próxima entrada na tabela, usando o tamanho informado no cabeçalho da entrada.
            current_ptr += entry_header->length;
        }
    }

    /**
     * @brief Encontra uma tabela ACPI específica pela sua assinatura de 4 letras.
     * @param signature A assinatura da tabela a ser encontrada (ex: "APIC").
     * @return Um ponteiro para o cabeçalho da tabela, ou nullptr se não for encontrada.
     */
    void* find_table(const char* signature) {
        if (rsdp_request.response == nullptr || rsdp_request.response->address == 0) {
            return nullptr; // O Limine não nos deu o mapa.
        }

        void* rsdp_ptr = reinterpret_cast<void*>(rsdp_request.response->address);
        uint8_t revision = *(uint8_t*)((uintptr_t)rsdp_ptr + 15);
        ACPI_SDTHeader* sdt_root;

        if (revision >= 2) { // ACPI 2.0+ usa a tabela XSDT (64-bit).
            uint64_t xsdt_address = *(uint64_t*)((uintptr_t)rsdp_ptr + 24);
            sdt_root = reinterpret_cast<ACPI_SDTHeader*>(xsdt_address);
        } else { // ACPI 1.0 usa a tabela RSDT (32-bit).
            uint32_t rsdt_address = *(uint32_t*)((uintptr_t)rsdp_ptr + 16);
            sdt_root = reinterpret_cast<ACPI_SDTHeader*>((uintptr_t)rsdt_address);
        }
        
        size_t entry_size = (revision >= 2) ? 8 : 4;
        int entries = (sdt_root->length - sizeof(ACPI_SDTHeader)) / entry_size;
        uintptr_t entry_array_start = (uintptr_t)sdt_root + sizeof(ACPI_SDTHeader);

        for (int i = 0; i < entries; i++) {
            uintptr_t table_ptr_addr = entry_array_start + (i * entry_size);
            uintptr_t table_addr = (entry_size == 8) ? *(uint64_t*)table_ptr_addr : *(uint32_t*)table_ptr_addr;

            ACPI_SDTHeader* h = reinterpret_cast<ACPI_SDTHeader*>(table_addr);
            if (sig_cmp(h->signature, signature, 4)) {
                return (void*)h; // Encontramos!
            }
        }

        return nullptr; // Não encontramos a tabela.
    }

} // namespace ACPI

