BITS 64
global _start

section .text
_start:
    ; --- Limpeza ---
    ; Nada de 'ud2'
    ; Nada de 'syscall'
    
    ; --- Syscall 1: Magic Number ---
    mov rax, 1              ; Syscall Number
    mov rbx, 0xCAFEBABE     ; Argumento (Magic Number)
    int 0x80                ; <--- USE ISTO, NÃO USE 'syscall'

    ; --- Syscall 2: Exit ---
    mov rax, 2
    mov rbx, 0
    int 0x80

    jmp $

