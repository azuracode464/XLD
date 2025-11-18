#include <stddef.h>
#include <stdint.h>
#include <limine.h>

#include "lib/string.h"
#include "xldgl/graphics.h"
#include "xldui/xldui.h"
#include "idt/idt.h"
#include "pic/pic.h"
#include "io/io.h"
#include "scheduler/task.h"
#include "timer/pit.h"
#include "mem/pmm.h"
#include "mem/kheap.h"

__attribute__((section(".requests"), used))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0
};

__attribute__((section(".requests"), used))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0
};

void yield() {
    asm volatile("int $32");
}

void outra_tarefa() {
    int x = 50;
    int y = 10;
    uint32_t bg_color = 0x000033;
    uint32_t fg_color = 0x00FF00;

    for(;;) {
        draw_char(x * 8, y * 8, 'B', fg_color);
        yield();
        yield();
        for (volatile int i = 0; i < 5000000; i++);

        draw_char(x * 8, y * 8, 'B', bg_color);
        yield();
        yield();
        for (volatile int i = 0; i < 5000000; i++);
    }
}

void kmain(void) {
    if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count < 1) {
        for (;;) { asm ("hlt"); }
    }
    struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];

    graphics_init(framebuffer);
    console_init(0x000033, 0xFFFF00);

    idt_init();
    console_print(" - IDT initialized successfully.\n");

    if (memmap_request.response == NULL) {
        console_set_color(0xFF0000);
        console_print("FATAL: Limine memmap request FAILED! System halted.\n");
        for (;;) { asm("hlt"); }
    }
    
    pmm_init(&memmap_request);
    console_print(" - PMM initialized.\n");

    kheap_init();
    console_print(" - KHeap initialized.\n");

    pic_init();
    console_print(" - PIC remapped successfully.\n");

    pit_init(100);
    console_print(" - PIT initialized at 100 Hz.\n");

    tasking_init();
    console_print(" - Multitasking initialized. Kernel is task 0.\n");

    create_task(outra_tarefa);
    console_print(" - Task 1 (outra_tarefa) created.\n");

    asm("sti");
    console_print(" - Interrupts enabled (STI).\n");

    uint8_t current_mask = inb(PIC1_DATA);
    outb(PIC1_DATA, current_mask & ~0x01);
    outb(PIC1_DATA, current_mask & ~0x02);
    console_print(" - Timer (IRQ0) and Keyboard (IRQ1) unmasked.\n");

    console_set_color(0x00FFFF);
    console_print("========================================\n");
    console_print("Welcome to the XLD Operating System!\n");
    console_print("========================================\n\n");

    int k_x = 50;
    int k_y = 12;
    uint32_t k_bg_color = 0x000033;
    uint32_t k_fg_color = 0xFFFF00;

    for (;;) {
        draw_char(k_x * 8, k_y * 8, 'A', k_fg_color);
        yield();
        yield();
        for (volatile int i = 0; i < 5000000; i++);
        
        draw_char(k_x * 8, k_y * 8, 'A', k_bg_color);
        yield();
        yield();
        for (volatile int i = 0; i < 5000000; i++);
    }
}

