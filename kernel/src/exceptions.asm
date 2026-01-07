BITS 64

; Funções C externas
extern exception_handler

; Símbolos globais exportados
global exception_handlers
global isr_stub_table

section .text

; Macro para exceções SEM código de erro (push 0 para alinhar)
%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    push 0                  ; Empurra erro dummy (0)
    push %1                 ; Empurra número da exceção
    jmp isr_common_stub
%endmacro

; Macro para exceções COM código de erro (a CPU já empurrou o erro)
%macro ISR_ERRCODE 1
global isr%1
isr%1:
    ; O código de erro já está na pilha
    push %1                 ; Empurra número da exceção
    jmp isr_common_stub
%endmacro

; --- Definição das 32 Exceções ---
ISR_NOERRCODE 0   ; Divide by Zero
ISR_NOERRCODE 1   ; Debug
ISR_NOERRCODE 2   ; NMI
ISR_NOERRCODE 3   ; Breakpoint
ISR_NOERRCODE 4   ; Overflow
ISR_NOERRCODE 5   ; Bound Range
ISR_NOERRCODE 6   ; Invalid Opcode
ISR_NOERRCODE 7   ; Device Not Available
ISR_ERRCODE   8   ; Double Fault
ISR_NOERRCODE 9   ; Coprocessor Segment Overrun
ISR_ERRCODE   10  ; Invalid TSS
ISR_ERRCODE   11  ; Segment Not Present
ISR_ERRCODE   12  ; Stack-Segment Fault
ISR_ERRCODE   13  ; General Protection Fault
ISR_ERRCODE   14  ; Page Fault
ISR_NOERRCODE 15  ; Reserved
ISR_NOERRCODE 16  ; x87 Floating Point Exception
ISR_ERRCODE   17  ; Alignment Check
ISR_NOERRCODE 18  ; Machine Check
ISR_NOERRCODE 19  ; SIMD Floating Point Exception
ISR_NOERRCODE 20  ; Virtualization Exception
ISR_NOERRCODE 21  ; Reserved
ISR_NOERRCODE 22  ; Reserved
ISR_NOERRCODE 23  ; Reserved
ISR_NOERRCODE 24  ; Reserved
ISR_NOERRCODE 25  ; Reserved
ISR_NOERRCODE 26  ; Reserved
ISR_NOERRCODE 27  ; Reserved
ISR_NOERRCODE 28  ; Reserved
ISR_NOERRCODE 29  ; Reserved
ISR_ERRCODE   30  ; Security Exception
ISR_NOERRCODE 31  ; Reserved

; --- Stub Comum ---
isr_common_stub:
    ; Salva todos os registradores (Contexto)
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rdi
    push rsi
    push rdx
    push rcx
    push rbx
    push rax
    push rbp

    ; Passa o ponteiro da pilha (struct trapframe*) como argumento para C
    mov rdi, rsp

    ; Chama o handler em C
    call exception_handler

    ; Restaura registradores
    pop rbp
    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15

    ; Limpa código de erro e número da interrupção da pilha (16 bytes)
    add rsp, 16

    iretq

; --- A Tabela que o Linker está procurando ---
section .data
align 8

exception_handlers:
    dq isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7
    dq isr8, isr9, isr10, isr11, isr12, isr13, isr14, isr15
    dq isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23
    dq isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31

