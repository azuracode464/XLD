#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>

/* Headers do Sistema */
#include "string.h"
#include "klog.h"
#include "serial.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"

/* Headers de Memória */
#include "pmm.h"
#include "kmalloc.h"
#include "vmm.h"      // Novo gerenciador de paginação

/* Headers de Armazenamento e VFS */
#include "ata_pio.h"
#include "blockdev.h" // Definição de blockdev_t
#include "fat32.h"
#include "vfs.h"

/* Header de Processos (User Space) */
#include "process.h"

/* ==========================================================================
   REQUSIÇÕES LIMINE
   ========================================================================== */
/* Requisita um framebuffer para saída de vídeo */
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0
};

/* O PMM usa uma requisição de mapa de memória definida externamente em limine_requests.c
   ou pmm.c, garantindo que não duplicamos a requisição. */

/* ==========================================================================
   DEFINIÇÕES GLOBAIS
   ========================================================================== */

/* Definição do driver de disco (ATA PIO Wrapper) */
static blockdev_t ata_drive_dev = {
    .read = ata_pio_read,
    .write = ata_pio_write,
    .flush = ata_pio_flush,
    .sector_size = 512
};

/* Protótipo do Framebuffer Init (definido em framebuffer.c) */
extern void fb_init(struct limine_framebuffer *fb);
extern void fb_clear(uint32_t color);

/* ==========================================================================
   FUNÇÕES AUXILIARES
   ========================================================================== */

/* Trava o sistema em caso de erro fatal */
void panic(const char *msg) {
    klog(2, "KERNEL PANIC: %s", msg);
    klog(2, "System Halted.");
    asm volatile("cli");
    for (;;) {
        asm volatile("hlt");
    }
}

/* Loop infinito simples */
static void hang(void) {
    for (;;) {
        asm volatile("hlt");
    }
}

/* ==========================================================================
   KERNEL MAIN
   ========================================================================== */

void kmain(void) {
    /* 1. Desabilitar interrupções para inicialização crítica */
    asm volatile("cli");

    /* 2. Inicialização de Vídeo e Serial (Logs) */
    if (framebuffer_request.response == NULL || 
        framebuffer_request.response->framebuffer_count < 1) {
        // Sem vídeo, tenta serial e trava (assumindo serial padrão 0x3F8)
        serial_init();
        serial_write("FATAL: No framebuffer found.\n");
        hang();
    }

    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
    fb_init(fb);       // Configura ponteiros do framebuffer
    serial_init();     // Inicializa porta serial COM1
    klog_init();       // Inicializa sistema de log (desenha na tela e serial)

    fb_clear(0x000000); // Limpa tela (preto)

    klog(0, "==========================================");
    klog(0, "XLD Kernel v0.1 - Paging & User Mode");
    klog(0, "Built: %s %s", __DATE__, __TIME__);
    klog(0, "==========================================");

    /* 3. Descritores de Segmento e Interrupção */
    klog(0, "[INIT] Loading GDT and TSS...");
    gdt_init(); // Configura segmentos de Kernel, User e TSS (para Ring 3)

    klog(0, "[INIT] Loading IDT...");
    idt_init(); // Configura tratamento de exceções e Syscall (0x80)

    /* 4. Gerenciamento de Memória */
    klog(0, "[INIT] Initializing Physical Memory (PMM)...");
    pmm_init(); // Lê mapa de memória e cria bitmap de frames livres

    klog(0, "[INIT] Initializing Kernel Heap...");
    kmalloc_init(); // Inicializa alocador dinâmico do kernel

    klog(0, "[INIT] Initializing Virtual Memory (VMM)...");
    vmm_init(); // Configura paginação baseada no mapa atual do Limine (Higher Half)

    /* 5. Hardware e Armazenamento */
    klog(0, "[INIT] Initializing ATA Driver...");
    ata_pio_init(); // Detecta e reseta discos ATA

    klog(0, "[INIT] Mounting Filesystems...");
    // Tenta montar a partição FAT32 do disco mestre
    fat32_fs_t *fat_fs = fat32_mount(&ata_drive_dev);
    
    if (fat_fs) {
        // Cria contexto VFS e monta em /mnt
        struct vfs_fs_ops *fat_ops = vfs_get_fat32_ops();
        void *fs_ctx = vfs_create_fat32_context(fat_fs);

        if (vfs_mount("/mnt", fat_ops, fs_ctx) == VFS_OK) {
            klog(0, "  [OK] FAT32 mounted at /mnt (auto-bound to /)");
        } else {
            panic("Failed to mount VFS at /mnt");
        }
    } else {
        panic("Failed to initialize FAT32 filesystem (Check disk image)");
    }

    /* 6. Habilitar Interrupções */
    klog(0, "[INIT] Remapping PIC and Enabling Interrupts...");
    pic_remap();   // Remapeia IRQs para não conflitar com exceções da CPU
    // enable_irq(1); // Opcional: Habilita teclado se ps2.c estiver pronto
    asm volatile("sti");

    /* 7. Carregar e Executar User Space */
    klog(0, "------------------------------------------");
    klog(0, "Kernel initialization complete.");
    klog(0, "Attempting to load user process: /root/program.sbf");

    // Tenta carregar o executável SBF
    // Certifique-se de ter convertido seu ASM/C user-space para .sbf 
    // e copiado para a imagem de disco em /root/program.sbf
    process_t *proc = process_create_from_sbf("/root/program.sbf");

    if (proc) {
        klog(0, "[EXEC] Process loaded (PID %d). Jumping to Ring 3...", proc->pid);
           // DESLIGA O TIMER (IRQ 0) NO PIC
    // Lê a máscara atual da porta 0x21 (PIC Master Data)
    uint8_t mask = inb(0x21); 
    // Liga o bit 0 (Mascara IRQ 0)
    outb(0x21, mask | 0x01);  

    // Agora sim, pode rodar
        // Esta função NÃO retorna se tiver sucesso.
        // Ela troca o CR3 (PML4) e faz IRETQ para o User Mode.
        process_run(proc);
    } else {
        klog(2, "[FAIL] Could not load /root/program.sbf");
        klog(1, "Ensure the file exists and is in valid SBF format.");
    }

    /* Se chegamos aqui, algo falhou no carregamento do processo */
    klog(1, "System halted.");
    hang();
}

