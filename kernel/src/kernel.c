#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "idt.h"
#include "pic.h"

/* Protótipos de função (implementados em outros módulos) */
extern void fb_init(struct limine_framebuffer *fb);
extern void fb_clear(uint32_t color);
extern uint32_t fb_get_width(void);
extern uint32_t fb_get_height(void);
extern void gdt_init(void);
extern void graphics_init(void);
extern void klog_init(void);
extern void klog(int level, const char *fmt, ...);
extern void printk(const char *fmt, ...);

extern void ps2_init(void);
extern void enable_irq(uint8_t irq);
extern void pic_remap(void);
extern void idt_init(void);

extern uint64_t get_total_memory(void);
extern uint64_t get_used_memory(void);

/* Níveis de log */
#define KLOG_INFO 0
#define KLOG_WARN 1
#define KLOG_ERROR 2
#define KLOG_DEBUG 3

/* Requisição do Limine para framebuffer */
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0
};

/* Delay simples (para animação/espera) */
static void microdelay(uint32_t ms) {
    for (volatile uint32_t i = 0; i < ms * 1000; i++) {
        asm volatile("nop");
    }
}

/* Pânico */
static void panic(const char *msg) {
    klog(KLOG_ERROR, "KERNEL PANIC: %s", msg);
    klog(KLOG_ERROR, "System halted.");
    for (;;) {
        asm volatile("cli; hlt");
    }
}

/* Halt (bloqueio) */
static void halt(void) {
    asm volatile("cli");
    for (;;) {
        asm volatile("hlt");
    }
}

/* Implementações simuladas temporárias de memória (substituir por real) */
static uint64_t get_total_memory_simulated(void) {
    return 128 * 1024 * 1024;  /* 128 MB simulados */
}

static uint64_t get_used_memory_simulated(void) {
    return 4 * 1024 * 1024;    /* 4 MB usados simulados */
}

/* Função auxiliar para ler porta (se necessário para debug) */
static inline uint8_t inb_port(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Entrada principal do kernel */
void kmain(void) {
    gdt_init();
    /* ===== FASE 1: Verificações básicas ===== */
    if (framebuffer_request.response == NULL || 
        framebuffer_request.response->framebuffer_count < 1) {
        panic("No framebuffer available");
    }

    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];

    /* ===== FASE 2: Inicialização de hardware básico ===== */
    fb_init(fb);
    graphics_init();
    klog_init();

    /* Limpa tela com fundo preto */
    fb_clear(0x000000);

    /* ===== FASE 3: Banner e informações do sistema ===== */
    klog(KLOG_INFO, "========================================");
    klog(KLOG_INFO, "XLD v0.01 - Experimental Build");
    klog(KLOG_INFO, "Built: %s %s", __DATE__, __TIME__);
    klog(KLOG_INFO, "========================================");
    klog(KLOG_INFO, "");

    /* Informações de memória (simuladas por enquanto) */
    uint64_t mem_total = get_total_memory_simulated();
    uint64_t mem_used = get_used_memory_simulated();
    uint64_t mem_free = mem_total - mem_used;

    klog(KLOG_INFO, "System information:");
    klog(KLOG_INFO, "  Memory: %d MB total, %d MB free", 
         (int)(mem_total / 1024 / 1024),
         (int)(mem_free / 1024 / 1024));
    klog(KLOG_INFO, "  Framebuffer: %dx%d @ 32bpp", 
         fb_get_width(), fb_get_height());
    klog(KLOG_INFO, "");

    /* ===== FASE 4: Inicialização de subsistemas ===== */
    klog(KLOG_INFO, "Initializing kernel subsystems:");
    microdelay(50);

    /* 4.1: PIC (Programmable Interrupt Controller) - remapear UMA vez */
    pic_remap();
    klog(KLOG_INFO, "  [OK] PIC controller (remapped)");
    microdelay(30);

    /* 4.2: Carrega IDT (idt_init NÃO remapeia o PIC e NÃO executa sti) */
    idt_init();
    klog(KLOG_INFO, "  [OK] Interrupt descriptor table (loaded)");
    microdelay(30);

    /* 4.3: Inicializa controlador PS/2 */
    ps2_init();
    klog(KLOG_INFO, "  [OK] PS/2 keyboard controller");
    microdelay(30);

    /* 4.4: Habilita IRQ do teclado no PIC (unmask) */
    enable_irq(1);  /* Habilita IRQ1 (teclado) */
    klog(KLOG_INFO, "  [OK] Keyboard IRQ enabled (PIC)");
    microdelay(30);

    /* Debug: exibir máscaras do PIC para confirmar IRQ1 desmascarada */
    {
        uint8_t mask_master = inb_port(0x21);
        uint8_t mask_slave = inb_port(0xA1);
        klog(KLOG_INFO, "[test] PIC masks: master=0x%x slave=0x%x", (unsigned)mask_master, (unsigned)mask_slave);
    }

    /* Debug: inspecionar IDT[33] (vetor da IRQ1) */
    {
        extern struct idt_entry idt[]; /* definida em idt.c */
        struct idt_entry *e = &idt[33];
        uint32_t off_lo = e->offset_low;
        uint32_t off_mid = e->offset_middle;
        uint32_t off_hi = e->offset_high;
        klog(KLOG_INFO, "[test] IDT[33] selector=0x%x type=0x%x off_lo=0x%x off_mid=0x%x off_hi=0x%x",
             (unsigned)e->selector, (unsigned)e->type_attr, off_lo, off_mid, off_hi);
    }

    /* ===== Habilita interrupções globais APÓS toda a configuração ===== */
    asm volatile("sti");
    klog(KLOG_INFO, "  [OK] CPU interrupts enabled (sti)");
    microdelay(30);

    klog(KLOG_WARN, "");
    klog(KLOG_WARN, "System initialization complete.");
    klog(KLOG_WARN, "");
    klog(KLOG_WARN, "XLD is ready.");
    klog(KLOG_INFO, "Type anything to test keyboard input...");
    klog(KLOG_INFO, "");

    /* ===== FASE 6: Loop principal ===== */
    uint64_t idle_cycles = 0;

    while (1) {
        /* Loop idle - interrupts devem tratar o input */
        idle_cycles++;

        /* DEBUG opcional: log de vida (comentado por padrão) */
        if (idle_cycles % 1000000 == 0) {
            /* klog(KLOG_DEBUG, "Idle cycle: %llu", idle_cycles); */
        }

        /* Entrar em hlt até a próxima interrupção para economizar CPU */
        asm volatile("hlt");
    }

    /* Nunca deve chegar aqui */
    panic("Kernel main loop exited unexpectedly");
}
