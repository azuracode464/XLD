; Em: kernel/src/cpu/idt/isr.asm

section .text
bits 64

; Importa o nosso handler C++
extern generic_interrupt_handler

; Macro para criar um stub de ISR que NÃO tem um código de erro empurrado pelo CPU.
; Nós empurramos um 0 para manter a estrutura da pilha consistente.
%macro isr_no_err_stub 1
global isr%1
isr%1:
    cli          ; Desabilita outras interrupções
    push 0       ; Empurra um código de erro falso
    push %1      ; Empurra o número da interrupção
    jmp isr_common_stub
%endmacro

; Macro para criar um stub de ISR que TEM um código de erro empurrado pelo CPU.
%macro isr_err_stub 1
global isr%1
isr%1:
    cli
    ; O código de erro já está na pilha
    push %1      ; Empurra o número da interrupção
    jmp isr_common_stub
%endmacro

; O código comum para todos os stubs
isr_common_stub:
    ; Salva todos os registradores de propósito geral
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

    ; O ponteiro para o nosso InterruptFrame agora está em RSP.
    ; Passamos ele como primeiro argumento (em RDI) para a função C++.
    mov rdi, rsp

    ; Chama o handler C++
    call generic_interrupt_handler

    ; Restaura todos os registradores (em ordem inversa)
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

    ; Limpa o número da interrupção e o código de erro da pilha
    add rsp, 16

    sti          ; Reabilita interrupções
    iretq        ; Retorna da interrupção

; Agora, criamos os 256 stubs. Vamos focar no do timer (IRQ0 -> int 32)
; (As primeiras 32 interrupções são reservadas para exceções da CPU)
isr_no_err_stub 32  ; Este será nosso timer!

