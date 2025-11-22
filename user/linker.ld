/* Linker script para programas de usuário ELF */
ENTRY(_start)
OUTPUT_FORMAT(elf64-x86-64)

PHDRS {
    text PT_LOAD;
}

SECTIONS {
    /* Os programas de usuário começam em um endereço virtual baixo, como 0x400000 */
    . = 0x400000;

    .text : {
        *(.text .text.*)
    } :text

    .rodata : {
        *(.rodata .rodata.*)
    }

    /DISCARD/ : {
        *(.eh_frame*)
        *(.comment*)
    }
}

