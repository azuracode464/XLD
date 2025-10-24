; Em: cpu/idt/idt.asm

section .text
bits 64

global idt_load

idt_load:
    ; O primeiro argumento (rdi) contém o ponteiro para a nossa IDTPointer
    lidt [rdi]
    ret

