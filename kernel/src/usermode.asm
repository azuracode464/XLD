BITS 64

global jump_to_usermode

; void jump_to_usermode(uint64_t rip, uint64_t rsp);
; RDI = RIP (Instruction Pointer)
; RSI = RSP (Stack Pointer)
jump_to_usermode:
    cli                 ; Interrupções desligadas durante a troca

    ; Carrega Segment Selectors de Usuário (Index 3 na GDT = 0x18)
    ; O RPL (Request Privilege Level) deve ser 3.
    ; 0x18 | 3 = 0x1B
    mov ax, 0x1B
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Prepara o stack frame para IRETQ
    ; O processador espera a seguinte ordem na pilha para IRETQ em 64-bit:
    ; SS (User)
    ; RSP (User)
    ; RFLAGS
    ; CS (User)
    ; RIP (User)

    push 0x1B           ; SS (User Data + RPL 3)
    push rsi            ; RSP de usuário
    push 0x202          ; RFLAGS (Interrupts Enabled (bit 9) + Reserved (bit 1))
    push 0x23           ; CS (User Code: Index 4 = 0x20 | 3 = 0x23)
    push rdi            ; RIP (Entry point)

    iretq               ; Retorna da "interrupção", entrando em Ring 3

