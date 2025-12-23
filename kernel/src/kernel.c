#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "idt.h"
#include "pic.h"
#include "string.h"
#include "pmm.h"
#include "kmalloc.h"
#include "ata_pio.h"
#include "fat32.h"
#include "vfs.h"

/* Protótipos de função */
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
extern void fat32_ls_recursive(fat32_fs_t *fs);
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

/* Definição do dispositivo de bloco (ATA Wrapper) */
static blockdev_t ata_drive = {
    .read = ata_pio_read,
    .write = ata_pio_write,
    .flush = ata_pio_flush,
    .sector_size = 512
};

/* Delay simples */
static void microdelay(uint32_t ms) {
    for (volatile uint32_t i = 0; i < ms * 10000; i++) {
        asm volatile("nop");
    }
}

/* Pânico */
void panic(const char *msg) {
    klog(KLOG_ERROR, "KERNEL PANIC: %s", msg);
    klog(KLOG_ERROR, "System halted.");
    for (;;) {
        asm volatile("cli; hlt");
    }
}

/* Entrada principal do kernel */
void kmain(void) {
    /* Desabilita interrupções durante o setup inicial */
    asm volatile("cli");

    /* ===== FASE 1: Inicialização Gráfica ===== */
    if (framebuffer_request.response == NULL || 
        framebuffer_request.response->framebuffer_count < 1) {
        for(;;) asm volatile("hlt");
    }

    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
    fb_init(fb);
    graphics_init();
    klog_init();

    fb_clear(0x000000);

    klog(KLOG_INFO, "========================================");
    klog(KLOG_INFO, "XLD v0.03 - File Reader Test");
    klog(KLOG_INFO, "Built: %s %s", __DATE__, __TIME__);
    klog(KLOG_INFO, "========================================");
    klog(KLOG_INFO, "");

    /* ===== FASE 2: Gerenciamento de Memória ===== */
    klog(KLOG_INFO, "Initializing Memory Manager...");
    
    pmm_init();
    uint64_t total_mem = pmm_get_total();
    uint64_t free_mem = pmm_get_free();
    klog(KLOG_INFO, "  [OK] PMM: %d MB free / %d MB total", 
         (int)(free_mem/1024/1024), (int)(total_mem/1024/1024));

    kmalloc_init();
    klog(KLOG_INFO, "  [OK] Kernel Heap Initialized");
    
    microdelay(20);

    /* ===== FASE 3: Hardware e Interrupções ===== */
    klog(KLOG_INFO, "Initializing Hardware...");

    gdt_init();
    klog(KLOG_INFO, "  [OK] GDT Loaded");

    pic_remap();
    klog(KLOG_INFO, "  [OK] PIC Remapped");

    idt_init();
    klog(KLOG_INFO, "  [OK] IDT Loaded");

    ps2_init();
    klog(KLOG_INFO, "  [OK] PS/2 Controller");
    
    enable_irq(1);
    
    asm volatile("sti");
    klog(KLOG_INFO, "  [OK] Interrupts Enabled");

    /* ===== FASE 4: Armazenamento e VFS ===== */
    klog(KLOG_INFO, "Initializing Storage & Filesystem...");

    if (ata_pio_init() == 0) {
        klog(KLOG_INFO, "  [OK] ATA PIO Drive 0");
    } else {
        klog(KLOG_ERROR, "  [FAIL] ATA PIO Init");
    }

    fat32_fs_t *fs = fat32_mount(&ata_drive);
    
    if (fs) {
        klog(KLOG_INFO, "  [OK] FAT32 Detected & Parsed");

        struct vfs_fs_ops *fat_ops = vfs_get_fat32_ops();
        void *fs_ctx = vfs_create_fat32_context(fs);

        if (vfs_mount("/mnt", fat_ops, fs_ctx) == 0) {
            klog(KLOG_INFO, "  [OK] VFS mounted at '/mnt'");
            fat32_ls_recursive(fs);
            /* DEBUG: Verifica se o mount está correto */
            extern void vfs_dump_mounts(void);
            vfs_dump_mounts();
            
            klog(KLOG_INFO, "");

            /* ===== FASE 5: LEITURA DO ARQUIVO ===== */
            klog(KLOG_WARN, "--- READING /root/hello.txt ---");
            klog(KLOG_INFO, "");

            vfs_file_t *f;
            
            if (vfs_open("/root/hello.txt", VFS_READ, &f) == 0) {
                char buffer[256];
                memset(buffer, 0, 256);
                
                int bytes_read = vfs_read(f, buffer, 255);
                vfs_close(f);
                
                if (bytes_read > 0) {
                    klog(KLOG_INFO, "File content (%d bytes):", bytes_read);
                    klog(KLOG_INFO, "");
                    
                    /* Exibe o conteúdo linha por linha */
                    printk(">>> ");
                    for (int i = 0; i < bytes_read; i++) {
                        if (buffer[i] == '\n') {
                            printk("\n>>> ");
                        } else {
                            printk("%c", buffer[i]);
                        }
                    }
                    printk("\n");
                    
                    klog(KLOG_INFO, "");
                    klog(KLOG_INFO, "[SUCCESS] File read successfully!");
                } else {
                    klog(KLOG_ERROR, "Failed to read file (returned %d)", bytes_read);
                }
            } else {
                klog(KLOG_ERROR, "Failed to open /root/hello.txt");
                klog(KLOG_WARN, "Make sure the file exists on disk!");
            }
            
            klog(KLOG_WARN, "--- TEST COMPLETE ---");

        } else {
            klog(KLOG_ERROR, "Failed to mount VFS");
        }
    } else {
        klog(KLOG_ERROR, "FAT32 Mount Failed (Check disk image)");
    }

    klog(KLOG_INFO, "");
    klog(KLOG_INFO, "System ready. Press any key to test PS/2...");

    /* ===== FASE 6: Loop Infinito ===== */
    while (1) {
        asm volatile("hlt");
    }
}
