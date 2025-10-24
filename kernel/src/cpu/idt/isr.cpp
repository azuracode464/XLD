#include "isr.hpp"
#include "graphics.hpp"
#include "apic.hpp" // <-- MUDOU DE pic.hpp PARA apic.hpp

extern Graphics* g_gfx;
volatile uint64_t system_ticks = 0;

extern "C" void generic_interrupt_handler(InterruptFrame* frame) {
    if (frame->int_no == 32) { // Timer
        system_ticks++;

        if (g_gfx != nullptr) {
            if ((system_ticks % 100) < 50) {
                g_gfx->draw_rect(0, 0, 20, 20, 0xFF00FF); // Magenta
            } else {
                g_gfx->draw_rect(0, 0, 20, 20, 0x00FFFF); // Ciano
            }
        }
    }
    // else if (frame->int_no == 39) { /* Tratar interrupção espúria */ }

    // Envia o EOI para o APIC, não mais para o PIC.
    APIC::send_eoi();
}

