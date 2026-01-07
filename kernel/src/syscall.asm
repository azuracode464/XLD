BITS 64
extern syscall_handler

global isr_syscall
isr_syscall:
    ; O usuário chama via INT 0x80
    ; Argumentos: RDI, RSI, RDX... (Convenção System V)
    ; RAX = Número da syscall

    ; Salva contexto
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

    ; Chama handler em C
    ; RDI já contém o primeiro argumento (se o usuário usou a conv. correta)
    ; Mas vamos passar o número da syscall (RAX) como primeiro argumento
    mov rdi, rax    ; Arg1: syscall number
    mov rsi, rbx    ; Arg2: ponteiro string (exemplo) ou valor
                    ; Ajuste conforme a convenção usada no seu userlib

    call syscall_handler

    ; RAX contém o retorno. Restauramos na pilha onde RAX foi salvo
    mov [rsp], rax

    ; Restaura
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

