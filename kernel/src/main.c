#include <stddef.h>
#include <stdint.h>
#include <limine.h>

// Headers do Kernel
#include "xldui/xldui.h"
#include "xldgl/graphics.h"
#include "gdt/gdt.h"
#include "idt/idt.h"
#include "mem/pmm.h"
#include "mem/vmm.h"
#include "mem/kheap.h"
#include "mem/mem.h"
#include "process/process.h"
#include "scheduler/task.h"
#include "timer/pit.h"
#include "elf/elf.h"
#include "lib/string.h"
#include "drivers/serial.h"
#include "lib/util.h"

// --- Requisições Limine ---
// Pede ao bootloader informações essenciais e para carregar nosso shell.

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

__attribute__((section(".requests"), used))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0
};

__attribute__((section(".requests"), used))
static volatile struct limine_module_request module_request = {
    .id = LIMINE_MODULE_REQUEST_ID,
    .revision = 0
};

// --- Variável Global ---
uint64_t hhdm_offset;

// --- Ponto de Entrada Principal do Kernel (kmain) ---
void kmain(void) {
    // --- 1. Verificações Iniciais e Inicialização Gráfica ---
    if (hhdm_request.response == NULL
     || framebuffer_request.response == NULL
     || framebuffer_request.response->framebuffer_count < 1) {
        for (;;) { asm ("hlt"); }
    }

    hhdm_offset = hhdm_request.response->offset;
    struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];

    graphics_init(framebuffer);
    console_init(0x000033, 0xFFFF00);
    console_print("XLD-OS Kernel Booting...\n\n");

    // --- 2. Inicialização dos Subsistemas do Kernel (ORDEM CORRIGIDA) ---

    // Passo 1: Memória Física (PMM)
    // Precisa vir primeiro, pois outros subsistemas (como a GDT) precisam alocar memória.
    pmm_init(&memmap_request);
    console_print("[OK] Physical Memory Manager (PMM) Initialized.\n");

    // Passo 2: Tabela de Descritores Globais (GDT)
    // Agora pode chamar pmm_alloc_pages() com segurança para a pilha da TSS.
    gdt_init();
    console_print("[OK] GDT and TSS Initialized.\n");

    // Passo 3: Tabela de Descritores de Interrupção (IDT)
    // Configura como a CPU deve lidar com exceções e interrupções de hardware.
    idt_init();
    console_print("[OK] IDT and Interrupts Initialized.\n");

    // Passo 4: Memória Virtual (VMM) e Heap do Kernel (kmalloc)
    // Prepara o gerenciamento de memória virtual e o alocador dinâmico do kernel.
    vmm_init();
    console_print("[OK] Virtual Memory Manager (VMM) Initialized.\n");
    kheap_init();
    console_print("[OK] Kernel Heap (kmalloc) Initialized.\n");

    // Passo 5: Processos e Escalonamento
    // Com a memória funcionando, podemos criar processos e preparar o multitarefa.
    process_init();
    console_print("[OK] Process Manager Initialized.\n");
    tasking_init();
    console_print("[OK] Tasking System Initialized.\n");
    pit_init(100); // Configura o timer para o escalonador (100 Hz).
    console_print("[OK] PIT Initialized for Scheduling (100 Hz).\n\n");

    // --- 3. Carregar o Processo do Shell ---
    console_print("Searching for shell.elf module...\n");

    if (module_request.response == NULL || module_request.response->module_count == 0) {
        console_set_color(0xFF0000);
        console_print("FATAL: 'shell.elf' not loaded by Limine!\n");
        console_print("       Please check your limine.cfg file.\n");
        for (;;) { asm("hlt"); }
    }

    struct limine_file* shell_elf_file = module_request.response->modules[0];
    console_print("Found module! Loading ELF process...\n");

    pid_t shell_pid = elf_load_process(shell_elf_file->address);
    // Em kmain.c, depois de elf_load_process

console_print("\n--- Task List Check ---\n");
extern volatile task_t *task_list_head; // Declare as variáveis externas
extern volatile task_t *current_task;

char buf[20];
u64_to_hex((uint64_t)task_list_head, buf);
console_print("Head: "); console_print(buf); console_print("\n");

u64_to_hex((uint64_t)task_list_head->next, buf);
console_print("Head->Next: "); console_print(buf); console_print("\n");

u64_to_hex((uint64_t)task_list_head->next->next, buf);
console_print("Head->Next->Next: "); console_print(buf); console_print("\n");

console_print("-----------------------\n");

    if (shell_pid < 0) {
        console_set_color(0xFF0000);
        console_print("FATAL: Failed to load shell process from ELF file!\n");
        for (;;) { asm("hlt"); }
    }

    console_print("\n[SUCCESS] Shell process created with PID 1.\n");

    // --- 4. Iniciar o Multitarefa ---
    console_print("Starting scheduler. Kernel will now idle.\n");
    
    asm volatile ("sti");

    // O kmain agora se torna a tarefa ociosa do sistema.
    for (;;) {
        asm ("hlt");
    }
}

