#include <cstdint>
#include <cstddef>
#include <limine.h>

// --- NOSSOS MÓDULOS PRINCIPAIS ---
#include "xldgl/graphics.hpp"
#include "drivers/keyboard/keyboard.hpp"
#include "apps/terminal/terminal.hpp"

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
// (O código de baixo nível que não precisamos mexer mais)
extern "C" {
    void *memcpy(void *d, const void *s, size_t n) { for(size_t i=0;i<n;i++) ((char*)d)[i]=((char*)s)[i]; return d; }
    void *memset(void *s, int c, size_t n) { for(size_t i=0;i<n;i++) ((char*)s)[i]=(char)c; return s; }
    int __cxa_atexit(void (*)(void *), void *, void *) { return 0; }
    void __cxa_pure_virtual() { for(;;); }
    void *__dso_handle;
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
    Terminal::init();

    // 3. Entrega o controle para o Terminal
    // A partir daqui, o Terminal é o dono do sistema.
    Terminal::run();

    // O código nunca deve chegar aqui.
    hcf();
}

