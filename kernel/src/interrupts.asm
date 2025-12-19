; Interrupt handler para teclado (NASM, x86_64)
BITS 64

global keyboard_handler
extern ps2_handler
extern pic_send_eoi

section .text

keyboard_handler:
    ; Prolog - salva registradores
    push rbp
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

    ; Alinha pilha para chamadas C (16 bytes)
    sub rsp, 8

    ; Chama handler C do PS/2
    call ps2_handler

    add rsp, 8

    ; Envia EOI para o PIC (IRQ1 = teclado)
    mov edi, 1        ; escreve 32 bits em EDI (zero-extends RDI)
    sub rsp, 8
    call pic_send_eoi
    add rsp, 8

    ; Epilog - restaura registradores
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
    pop rbp

    iretq
