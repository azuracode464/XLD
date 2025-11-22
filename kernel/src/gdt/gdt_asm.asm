section .text
bits 64

global gdt_load
global tss_load

; Carrega a GDT e recarrega os seletores de segmento do kernel
gdt_load:
    lgdt [rdi]      ; Carrega o ponteiro da GDT (GDTR)
    
    ; Recarrega os seletores de segmento para garantir consistência
    push 0x08       ; Seletor de Código do Kernel
    lea rax, [rel .reload_cs]
    push rax
    retfq           ; Salto longo para recarregar CS
.reload_cs:
    mov ax, 0x10    ; Seletor de Dados do Kernel
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ret

; Carrega o seletor da TSS no Registro de Tarefa
tss_load:
    mov ax, di      ; O seletor é passado como argumento em DI
    ltr ax          ; Carrega o Task Register
    ret

