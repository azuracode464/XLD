// Em: kernel/src/cpu/pic.hpp

#pragma once

// Definições das portas do PIC
#define PIC1_CMD  0x20
#define PIC1_DATA 0x21
#define PIC2_CMD  0xA0
#define PIC2_DATA 0xA1

namespace PIC {
    void remap_and_disable();
    void send_eoi(int irq);
}

