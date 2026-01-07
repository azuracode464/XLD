#include "process.h"
#include "vfs.h"
#include "kmalloc.h"
#include "klog.h"
#include "string.h"
#include "pmm.h"
#include "gdt.h" 
#include "vmm.h" 

#define SBF_MAGIC 0x00464253 // "SBF\0"

typedef struct {
    uint8_t magic[4];
    uint8_t bits; 
    uint8_t endian;
    uint16_t flags;
    uint64_t entry_point;
    uint32_t text_offset;
    uint32_t text_size;
    uint32_t data_offset;
    uint32_t data_size;
    uint32_t bss_size;
} __attribute__((packed)) sbf_header_t;

extern void jump_to_usermode(uint64_t rip, uint64_t rsp);

/* Funções de I/O para mascarar Timer */
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

/* Copia para memória física usando HHDM */
static void copy_to_phys(uint64_t phys_addr, void *src, size_t size) {
    void *dest = PHYS_TO_VIRT(phys_addr);
    memcpy(dest, src, size);
}

process_t* process_create_from_sbf(const char *path) {
    vfs_file_t *f;
    if (vfs_open(path, VFS_READ, &f) != 0) {
        klog(2, "PROC: Failed to open %s", path);
        return NULL;
    }

    sbf_header_t hdr;
    if (vfs_read(f, &hdr, sizeof(hdr)) != sizeof(hdr)) {
        vfs_close(f);
        return NULL;
    }

    if (*(uint32_t*)hdr.magic != SBF_MAGIC) {
        klog(2, "PROC: Invalid SBF magic");
        vfs_close(f);
        return NULL;
    }

    process_t *proc = kmalloc(sizeof(process_t));
    proc->pid = 1; 
    proc->pml4 = vmm_new_address_space();
    
    // Stack do Kernel (para syscalls/interrupts)
    proc->kernel_stack = kmalloc(4096); 
    proc->kernel_stack = (void*)((uint64_t)proc->kernel_stack + 4096);

    klog(0, "PROC: Loading Text (%d bytes)", hdr.text_size);

    // Buffer temporário
    uint8_t *text_buf = kmalloc(hdr.text_size);
    vfs_seek(f, hdr.text_offset, SEEK_SET);
    vfs_read(f, text_buf, hdr.text_size);

    uint64_t virt_text = 0x400000; 
    uint64_t pages_text = (hdr.text_size + 4095) / 4096;
    
    for (uint64_t i = 0; i < pages_text; i++) {
        // CORREÇÃO: pmalloc retorna FÍSICO. Não usamos VIRT_TO_PHYS aqui.
        uint64_t phys = (uint64_t)pmalloc(1);
        
        // Mapeia
        vmm_map_page(proc->pml4, virt_text + i*4096, phys, PTE_PRESENT | PTE_USER | PTE_RW);
        
        // Copia dados
        uint64_t offset = i * 4096;
        uint64_t chunk = (hdr.text_size - offset > 4096) ? 4096 : hdr.text_size - offset;
        copy_to_phys(phys, text_buf + offset, chunk);
        
        // Debug
        if (i == 0) {
            uint8_t *check = (uint8_t*)PHYS_TO_VIRT(phys);
            klog(0, "[DEBUG] Phys: 0x%llx | Byte[0]: %d", phys, check[0]);
        }
    }
    kfree(text_buf);

    // --- DATA ---
    if (hdr.data_size > 0) {
        uint8_t *data_buf = kmalloc(hdr.data_size);
        vfs_seek(f, hdr.data_offset, SEEK_SET);
        vfs_read(f, data_buf, hdr.data_size);

        uint64_t virt_data = virt_text + pages_text * 4096;
        uint64_t pages_data = (hdr.data_size + 4095) / 4096;

        for (uint64_t i = 0; i < pages_data; i++) {
            uint64_t phys = (uint64_t)pmalloc(1); // Físico
            vmm_map_page(proc->pml4, virt_data + i*4096, phys, PTE_PRESENT | PTE_USER | PTE_RW);
            
            uint64_t offset = i * 4096;
            uint64_t chunk = (hdr.data_size - offset > 4096) ? 4096 : hdr.data_size - offset;
            copy_to_phys(phys, data_buf + offset, chunk);
        }
        kfree(data_buf);
    }

    // --- USER STACK ---
    uint64_t virt_stack = 0xF0000000; 
    for (int i = 0; i < 4; i++) { 
        uint64_t phys = (uint64_t)pmalloc(1); // Físico
        vmm_map_page(proc->pml4, virt_stack - (i+1)*4096, phys, PTE_PRESENT | PTE_USER | PTE_RW);
    }

    vfs_close(f);
    proc->rip = hdr.entry_point; 
    proc->rsp = virt_stack;

    return proc;
}

void process_run(process_t *proc) {
    if (!proc) return;

    klog(0, "PROC: Switching to Ring 3...");
    klog(0, "      RIP: 0x%llx | RSP: 0x%llx", proc->rip, proc->rsp);
    
    // Define pilha de Kernel para quando ocorrer interrupção
    tss_set_rsp0((uint64_t)proc->kernel_stack);

    // Mascara Timer para evitar crash (até ter handler)
    outb(0x21, inb(0x21) | 0x01);

    // Troca tabelas de página
    vmm_switch_pml4(proc->pml4);

    // Salta para User Space
    jump_to_usermode(proc->rip, proc->rsp);
}
