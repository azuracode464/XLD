BITS 64

%macro EXCEPTION_NOERRCODE 1
global exception_handler_%1
exception_handler_%1:
    push 0                     ; erro dummy
    push %1                    ; número da exceção
    jmp common_exception_handler
%endmacro

%macro EXCEPTION_ERRCODE 1
global exception_handler_%1
exception_handler_%1:
    push %1                    ; número da exceção
    jmp common_exception_handler
%endmacro

EXCEPTION_NOERRCODE 0
EXCEPTION_NOERRCODE 1
EXCEPTION_NOERRCODE 2
EXCEPTION_NOERRCODE 3
EXCEPTION_NOERRCODE 4
EXCEPTION_NOERRCODE 5
EXCEPTION_NOERRCODE 6
EXCEPTION_NOERRCODE 7
EXCEPTION_ERRCODE   8
EXCEPTION_NOERRCODE 9
EXCEPTION_ERRCODE   10
EXCEPTION_ERRCODE   11
EXCEPTION_ERRCODE   12
EXCEPTION_ERRCODE   13
EXCEPTION_ERRCODE   14
EXCEPTION_NOERRCODE 15
EXCEPTION_NOERRCODE 16
EXCEPTION_NOERRCODE 17
EXCEPTION_NOERRCODE 18
EXCEPTION_NOERRCODE 19
EXCEPTION_NOERRCODE 20
EXCEPTION_NOERRCODE 21
EXCEPTION_NOERRCODE 22
EXCEPTION_NOERRCODE 23
EXCEPTION_NOERRCODE 24
EXCEPTION_NOERRCODE 25
EXCEPTION_NOERRCODE 26
EXCEPTION_NOERRCODE 27
EXCEPTION_NOERRCODE 28
EXCEPTION_NOERRCODE 29
EXCEPTION_NOERRCODE 30
EXCEPTION_NOERRCODE 31

extern exception_handler

common_exception_handler:
    ; Salva registradores (simplificado)
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi

    ; Chama handler C: exception_handler(vector, error_code)
    mov rdi, [rsp+56]   ; vector
    mov rsi, [rsp+64]   ; error_code
    call exception_handler

    ; Nunca retorna aqui
    cli
    hlt
