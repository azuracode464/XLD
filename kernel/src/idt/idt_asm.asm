section .text
bits 64

; --- EXPORTS ---
global idt_load
global isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7, isr8, isr9, isr10
global isr11, isr12, isr13, isr14, isr15, isr16, isr17, isr18, isr19, isr20
global isr21, isr22, isr23, isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31
global irq0, irq1

; --- IMPORTS ---
extern isr_handler
extern keyboard_handler
extern schedule

; --- FUNÇÕES ---

idt_load:
    lidt [rdi]
    ret

; -----------------------------------------------------------------------------
; STUB DO ESCALONADOR (IRQ 0 - TIMER) - VERSÃO SIMPLIFICADA E ROBUSTA
; -----------------------------------------------------------------------------
irq0:
    ; 1. Salva os registradores que a CPU não salva automaticamente.
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; 2. Passa o ponteiro da pilha (que agora aponta para o estado completo) para a função C.
    mov rdi, rsp
    call schedule

    ; 3. A função C 'schedule' retorna no registrador RAX o ponteiro para o estado da PRÓXIMA tarefa.
    ;    Agora, vamos usar esse ponteiro para restaurar tudo.
    mov rsp, rax

    ; 4. Restaura todos os registradores a partir do novo ponteiro de pilha.
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    
    ; 5. Envia o sinal de Fim de Interrupção para o PIC.
    mov al, 0x20
    out 0x20, al

    ; 6. Retorna da interrupção. iretq usará os valores (RIP, CS, RFLAGS, etc.)
    ;    que estão agora no topo da nova pilha.
    iretq

; -----------------------------------------------------------------------------
; MACROS E OUTROS STUBS (permanecem os mesmos)
; -----------------------------------------------------------------------------
%macro ISR_NO_ERR_STUB 1
isr%1:
    push rbp
    mov rbp, rsp
    push rdi
    push rsi
    push rdx
    push rcx
    push r8
    push r9
    push rax
    mov rdi, %1
    call isr_handler
    pop rax
    pop r9
    pop r8
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    mov rsp, rbp
    pop rbp
    iretq
%endmacro

%macro IRQ_STUB 2
irq%1:
    push rax
    call %2
    mov al, 0x20
    out 0x20, al
    pop rax
    iretq
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
ISR_NO_ERR_STUB 8
ISR_NO_ERR_STUB 9
ISR_NO_ERR_STUB 10
ISR_NO_ERR_STUB 11
ISR_NO_ERR_STUB 12
ISR_NO_ERR_STUB 13
ISR_NO_ERR_STUB 14
ISR_NO_ERR_STUB 15
ISR_NO_ERR_STUB 16
ISR_NO_ERR_STUB 17
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
ISR_NO_ERR_STUB 30
ISR_NO_ERR_STUB 31

; O stub do teclado agora também é simplificado. O handler C fará mais trabalho.
irq1:
    push rax
    call keyboard_handler
    mov al, 0x20
    out 0xA0, al ; EOI para o slave (se necessário, o handler decide)
    mov al, 0x20
    out 0x20, al ; EOI para o master
    pop rax
    iretq

