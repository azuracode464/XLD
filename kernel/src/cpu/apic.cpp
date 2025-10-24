#include "apic.hpp"
#include "../acpi/acpi.hpp"      // A única fonte de informação de hardware que precisamos
#include "../xldgl/graphics.hpp" // Para os "sinais de fumaça" visuais

// Declaração de que g_gfx existe em outro lugar (main.cpp).
extern Graphics* g_gfx;

// Endereços dos registradores do APIC local.
constexpr uint32_t LAPIC_EOI_REG        = 0x00B0;
constexpr uint32_t LAPIC_SPURIOUS_REG   = 0x00F0;
constexpr uint32_t LAPIC_LVT_TIMER_REG  = 0x0320;
constexpr uint32_t LAPIC_TIMER_INIT_REG = 0x0380;
constexpr uint32_t LAPIC_TIMER_DIV_REG  = 0x03E0;

// Ponteiro global para o endereço base do APIC local.
static volatile uint32_t* g_lapic_regs = nullptr;

// Estrutura do cabeçalho da tabela MADT ("APIC").
struct MADTHeader {
    ACPI_SDTHeader header;
    uint32_t local_apic_address;
    uint32_t flags;
} __attribute__((packed));

// Funções auxiliares para ler e escrever nos registradores.
namespace {
    void write_reg(uint32_t reg, uint32_t value) {
        if (g_lapic_regs) {
            g_lapic_regs[reg / 4] = value;
        }
    }
    uint32_t read_reg(uint32_t reg) {
        if (g_lapic_regs) {
            return g_lapic_regs[reg / 4];
        }
        return 0;
    }
}

namespace APIC {

    void initialize() {
        // Sinal 1: Entramos na função.
        if (g_gfx) g_gfx->draw_rect(100, 0, 20, 20, 0xFFFFFF); // Quadrado BRANCO

        // Passo 1: Encontrar a tabela MADT usando nosso parser ACPI.
        MADTHeader* madt = (MADTHeader*)ACPI::find_table("APIC");

        if (madt == nullptr) {
            if (g_gfx) g_gfx->draw_rect(100, 0, 20, 20, 0xFF0000); // Quadrado VERMELHO: Tabela MADT não encontrada!
            for (;;);
        }

        // Sinal 2: Tabela MADT encontrada.
        if (g_gfx) g_gfx->draw_rect(100, 0, 20, 20, 0xFFFF00); // Quadrado AMARELO

        // Passo 2: Extrair o endereço base do APIC local DIRETAMENTE da tabela.
        g_lapic_regs = (volatile uint32_t*)(uintptr_t)madt->local_apic_address;

        // Sinal 3: Endereço do APIC obtido.
        if (g_gfx) g_gfx->draw_rect(100, 0, 20, 20, 0x00FF00); // Quadrado VERDE

        // Passo 3: O GRANDE TESTE - Primeira escrita no hardware.
        write_reg(LAPIC_SPURIOUS_REG, read_reg(LAPIC_SPURIOUS_REG) | 0x100 | 39);

        // Sinal 4: Sobrevivemos à escrita!
        if (g_gfx) g_gfx->draw_rect(100, 0, 20, 20, 0x0000FF); // Quadrado AZUL

        // Passo 4: Configurar o timer do APIC.
        write_reg(LAPIC_TIMER_DIV_REG, 0x3);
        write_reg(LAPIC_LVT_TIMER_REG, 32 | (1 << 17));
        write_reg(LAPIC_TIMER_INIT_REG, 10000000);
    }

    void send_eoi() {
        write_reg(LAPIC_EOI_REG, 0);
    }

} // namespace APIC

