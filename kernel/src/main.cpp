#include <cstdint>
#include <cstddef>
#include <limine.h>

// Nossos cabeçalhos do kernel
#include "graphics.hpp"
<<<<<<< Updated upstream
#include "idt.hpp"
#include "pic.hpp"
#include "io.hpp"
#include "pit.hpp"
#include "acpi.hpp" // Inclui nosso novo parser ACPI
#include "smp.hpp"  // Inclui nosso novo gerenciador SMP

// =============================================================================
// REQUISIÇÕES E CONFIGURAÇÕES DO BOOTLOADER LIMINE
// =============================================================================

=======
// ... includes ...
#include "idt/idt.hpp" // Inclui nosso novo header
// Set the base revision to 3, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.
extern Graphics* g_gfx;
>>>>>>> Stashed changes
namespace {
    // Requisição para o bootloader usar a revisão 3 do protocolo.
    __attribute__((used, section(".limine_requests")))
    volatile LIMINE_BASE_REVISION(3);

    // Requisição para o bootloader nos fornecer um framebuffer (modo gráfico).
    __attribute__((used, section(".limine_requests")))
    volatile limine_framebuffer_request framebuffer_request = {
        .id = LIMINE_FRAMEBUFFER_REQUEST,
        .revision = 0,
        .response = nullptr
    };
}

// =============================================================================
// FUNÇÕES BÁSICAS DE MEMÓRIA (memcpy, memset, etc.)
// =============================================================================

extern "C" {
    // ... (Suas funções memcpy, memset, etc. continuam aqui, sem alterações) ...
    void *memcpy(void *__restrict dest, const void *__restrict src, std::size_t n) {
        auto *pdest = static_cast<std::uint8_t *__restrict>(dest);
        const auto *psrc = static_cast<const std::uint8_t *__restrict>(src);
        for (std::size_t i = 0; i < n; i++) { pdest[i] = psrc[i]; }
        return dest;
    }
    void *memset(void *s, int c, std::size_t n) {
        auto *p = static_cast<std::uint8_t *>(s);
        for (std::size_t i = 0; i < n; i++) { p[i] = static_cast<uint8_t>(c); }
        return s;
    }
    void *memmove(void *dest, const void *src, std::size_t n) {
        auto *pdest = static_cast<std::uint8_t *>(dest);
        const auto *psrc = static_cast<const std::uint8_t *>(src);
        if (src > dest) { for (std::size_t i = 0; i < n; i++) { pdest[i] = psrc[i]; } }
        else if (src < dest) { for (std::size_t i = n; i > 0; i--) { pdest[i-1] = psrc[i-1]; } }
        return dest;
    }
    int memcmp(const void *s1, const void *s2, std::size_t n) {
        const auto *p1 = static_cast<const std::uint8_t *>(s1);
        const auto *p2 = static_cast<const std::uint8_t *>(s2);
        for (std::size_t i = 0; i < n; i++) { if (p1[i] != p2[i]) { return p1[i] < p2[i] ? -1 : 1; } }
        return 0;
    }
}

// =============================================================================
// FUNÇÃO PARA PARAR O PROCESSADOR (Halt and Catch Fire)
// =============================================================================

namespace {
    void hcf() {
        asm ("cli"); // Desabilita interrupções antes de parar
        for (;;) {
            asm ("hlt");
        }
    }
}

// =============================================================================
// STUBS DA ABI C++ E DECLARAÇÕES EXTERNAS
// =============================================================================

extern "C" {
    int __cxa_atexit(void (*)(void *), void *, void *) { return 0; }
    void __cxa_pure_virtual() { hcf(); }
    void *__dso_handle;
    extern void (*__init_array[])();
    extern void (*__init_array_end[])();
}

// =============================================================================
// VARIÁVEIS GLOBAIS
// =============================================================================

Graphics* g_gfx = nullptr;

// =============================================================================
// PONTO DE ENTRADA DO KERNEL (kmain)
// =============================================================================

extern "C" void kmain() {
    // 1. Validação do Bootloader
    if (LIMINE_BASE_REVISION_SUPPORTED == false) { hcf(); }

    // 2. Chama Construtores Globais
    for (std::size_t i = 0; &__init_array[i] != __init_array_end; i++) {
        __init_array[i]();
    }

    // 3. Inicialização Gráfica
    if (framebuffer_request.response == nullptr || framebuffer_request.response->framebuffer_count < 1) {
        hcf();
    }
    limine_framebuffer* fb = framebuffer_request.response->framebuffers[0];
<<<<<<< Updated upstream
    Graphics gfx_instance((volatile uint32_t*)fb->address, fb->pitch);
    g_gfx = &gfx_instance;

    g_gfx->draw_text("XLD-OS :: Conducting Imperial Census (ACPI)...", 10, 10, 0xFFFF00); // Amarelo

    // --- ATO 1: A DESCOBERTA ---
    // Procura a tabela MADT ("APIC") usando nosso novo parser.
    void* madt_ptr = ACPI::find_table("APIC");
    if (madt_ptr != nullptr) {
        // Se encontramos, realizamos o censo dos processadores!
        ACPI::parse_madt((MADTHeader*)madt_ptr);
    }

    // Exibe o resultado do censo na tela para verificação.
    char cpu_count_str[] = "CPUs Found:  ";
    size_t count = SMP::get_cpu_count();
    if (count > 9) { // Lida com mais de 9 CPUs de forma simples
        cpu_count_str[12] = '?';
    } else {
        cpu_count_str[12] = '0' + count;
    }
    g_gfx->draw_text(cpu_count_str, 10, 30, 0x00FF00); // Verde

    // Por enquanto, paramos aqui para celebrar nossa primeira vitória.
    // O próximo passo seria inicializar a IDT e o APIC usando as informações coletadas.
=======
    volatile uint32_t* fb_ptr = (volatile uint32_t*)fb->address;
    // Note: we assume the framebuffer model is RGB with 32-bit pixels.
    Graphics gfx(fb_ptr, fb->pitch);
    g_gfx = &gfx;
    initialize_interrupts();
    gfx.draw_rect(10,10,50,50,0xFF0000);     // Retângulo vermelho
    gfx.draw_circle(100,50,30,0x00FF00);     // Círculo verde
    gfx.draw_triangle(150,10,180,60,120,60,0x0000FF); // Triângulo azul
    gfx.draw_text("HELLO XLD", 10, 100, 0xFFFFFF);    // Texto branco
    // We're done, just hang...
>>>>>>> Stashed changes
    hcf();
}

