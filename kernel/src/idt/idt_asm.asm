section .text
bits 64

; --- EXPORTS ---
global idt_load
global isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7, isr8, isr9, isr10
global isr11, isr12, isr13, isr14, isr15, isr16, isr17, isr18, isr19, isr20
global isr21, isr22, isr23, isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31
global irq0, irq1
global isr128 ; Novo: exporta o syscall handler

; --- IMPORTS ---
extern isr_handler
extern keyboard_handler
extern schedule
extern syscall_handler ; Novo: importa o handler C de syscalls

; --- FUNÇÕES ---
idt_load:
    lidt [rdi]
    ret

; -----------------------------------------------------------------------------
; STUB DO ESCALONADOR (IRQ 0 - TIMER)
; -----------------------------------------------------------------------------
irq0:
    push r15; push r14; push r13; push r12; push r11; push r10; push r9; push r8;
    push rbp; push rdi; push rsi; push rdx; push rcx; push rbx; push rax;
    
    mov rdi, rsp
    call schedule
    mov rsp, rax

    pop rax; pop rbx; pop rcx; pop rdx; pop rsi; pop rdi; pop rbp;
    pop r8; pop r9; pop r10; pop r11; pop r12; pop r13; pop r14; pop r15;
    
    mov al, 0x20
    out 0x20, al
    iretq

; -----------------------------------------------------------------------------
; STUB DO TECLADO (IRQ 1)
; -----------------------------------------------------------------------------
irq1:
    push rax
    call keyboard_handler
    pop rax
    mov al, 0x20
    out 0xA0, al
    mov al, 0x20
    out 0x20, al
    iretq

; =================== INÍCIO DA MODIFICAÇÃO ===================
; -----------------------------------------------------------------------------
; NOVO: STUB PARA SYSCALLS (INT 0x80 / ISR 128)
; -----------------------------------------------------------------------------
isr128:
    ; Salva todos os registradores para passar o estado completo para o handler
    push r15; push r14; push r13; push r12; push r11; push r10; push r9; push r8;
    push rbp; push rdi; push rsi; push rdx; push rcx; push rbx; push rax;

    mov rdi, rsp ; Passa o ponteiro para cpu_state_t como primeiro argumento
    call syscall_handler
    ; O handler pode modificar o estado, então restauramos tudo
    
    pop rax; pop rbx; pop rcx; pop rdx; pop rsi; pop rdi; pop rbp;
    pop r8; pop r9; pop r10; pop r11; pop r12; pop r13; pop r14; pop r15;
    
    iretq

; -----------------------------------------------------------------------------
; MACROS E STUBS DE EXCEÇÃO (MODIFICADOS)
; -----------------------------------------------------------------------------
%macro ISR_NO_ERR_STUB 1
isr%1:
    ; Salva todos os registradores para passar o estado completo
    push r15; push r14; push r13; push r12; push r11; push r10; push r9; push r8;
    push rbp; push rdi; push rsi; push rdx; push rcx; push rbx; push rax;
    
    mov rdi, rsp  ; Arg1: cpu_state_t*
    mov rsi, %1   ; Arg2: número da interrupção
    call isr_handler
    
    ; O isr_handler nunca retorna, então não precisamos de pop
    hlt
%endmacro

; Stubs de exceção com código de erro (a CPU empilha um valor extra)
%macro ISR_ERR_STUB 1
isr%1:
    ; Salva todos os registradores, exceto o código de erro que já está na pilha
    push r15; push r14; push r13; push r12; push r11; push r10; push r9; push r8;
    push rbp; push rdi; push rsi; push rdx; push rcx; push rbx; push rax;

    mov rdi, rsp
    mov rsi, %1
    call isr_handler
    
    hlt
%endmacro

; Geração dos stubs
ISR_NO_ERR_STUB 0
ISR_NO_ERR_STUB 1
ISR_NO_ERR_STUB 2
ISR_NO_ERR_STUB 3
ISR_NO_ERR_STUB 4
ISR_NO_ERR_STUB 5
ISR_NO_ERR_STUB 6
ISR_NO_ERR_STUB 7
ISR_ERR_STUB 8
ISR_NO_ERR_STUB 9
ISR_ERR_STUB 10
ISR_ERR_STUB 11
ISR_ERR_STUB 12
ISR_ERR_STUB 13
ISR_ERR_STUB 14
ISR_NO_ERR_STUB 15
ISR_ERR_STUB 16
ISR_ERR_STUB 17
ISR_NO_ERR_STUB 18
ISR_NO_ERR_STUB 19
ISR_NO_ERR_STUB 20
ISR_NO_ERR_STUB 21
ISR_NO_ERR_STUB 22
ISR_NO_ERR_STUB 23
ISR_NO_ERR_STUB 24
ISR_NO_ERR_STUB 25
ISR_NO_ERR_STUB 26
ISR_NO_ERR_STUB 27
ISR_NO_ERR_STUB 28
ISR_NO_ERR_STUB 29
ISR_ERR_STUB 30
ISR_NO_ERR_STUB 31
; ==================== FIM DA MODIFICAÇÃO =====================

