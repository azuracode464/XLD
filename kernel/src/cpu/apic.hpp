#pragma once
#include <cstdint>

namespace APIC {
    void initialize();
    void send_eoi(); // O novo EOI do APIC
}

