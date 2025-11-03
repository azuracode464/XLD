// ARQUIVO MODIFICADO: kernel/src/main.cpp

#include <cstdint>
#include <cstddef>
#include <limine.h>

// --- NOSSOS MÓDULOS PRINCIPAIS ---
#include "xldgl/graphics.hpp"
#include "drivers/keyboard/keyboard.hpp"
#include "apps/terminal/terminal.hpp"
#include "drivers/ata/ata.hpp"
#include "drivers/fs/exfat.hpp" // <<< PASSO 1: Incluir o header do exFAT

// --- CONFIGURAÇÃO DO BOOTLOADER LIMINE ---
namespace {
    // ... (sem mudanças aqui) ...
}
// --- CONFIGURAÇÃO DO BOOTLOADER LIMINE ---
namespace {
    __attribute__((used, section(".limine_requests")))
    volatile LIMINE_BASE_REVISION(4);

    __attribute__((used, section(".limine_requests")))
    volatile limine_framebuffer_request framebuffer_request = {
        .id = LIMINE_FRAMEBUFFER_REQUEST,
        .revision = 0,
        .response = nullptr
    };
}

// --- FUNÇÕES BÁSICAS DE C E STUBS DO C++ ABI ---
extern "C" {
    // ... (sem mudanças aqui) ...
}

// --- FUNÇÃO DE PÂNICO ---
namespace {
    void hcf() { asm ("cli; hlt"); }
}

// --- PONTO DE ENTRADA DO KERNEL (kmain) ---
extern "C" void kmain() {
    // 1. Validação do Bootloader
    if (LIMINE_BASE_REVISION_SUPPORTED == false) hcf();
    if (framebuffer_request.response == nullptr || framebuffer_request.response->framebuffer_count < 1) hcf();
    limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];

    // 2. Inicializa os Módulos em Ordem
    GFX::init(framebuffer);
    Keyboard::init();
    ATA::init();
    
    // <<< PASSO 2: Chamar a inicialização do exFAT >>>
    // Colocamos dentro de um if para podermos tratar um erro, se ocorrer.
    if (!exFAT::initialize_volume()) {
        // Se não conseguir inicializar o volume, podemos imprimir um erro no futuro.
        // Por enquanto, apenas continuamos. O terminal ainda deve funcionar.
    }

    Terminal::init();

    // 3. Entrega o controle para o Terminal
    Terminal::run();

    hcf();
}

