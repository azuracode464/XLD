#!/usr/bin/env python3
import struct
import sys
import os

# --- Constantes do ELF ---
ELF_MAGIC = b'\x7fELF'
PT_LOAD = 1
PF_X = 1  # Executable
PF_W = 2  # Writable
PF_R = 4  # Readable

# --- Constantes do SBF ---
SBF_MAGIC = b'SBF\x00'

def read_u16(data, offset):
    return struct.unpack_from('<H', data, offset)[0]

def read_u32(data, offset):
    return struct.unpack_from('<I', data, offset)[0]

def read_u64(data, offset):
    return struct.unpack_from('<Q', data, offset)[0]

def main():
    if len(sys.argv) != 3:
        print(f"Uso: {sys.argv[0]} <input.elf> <output.sbf>")
        sys.exit(1)

    input_path = sys.argv[1]
    output_path = sys.argv[2]

    try:
        with open(input_path, 'rb') as f:
            elf_data = f.read()
    except FileNotFoundError:
        print(f"Erro: Arquivo '{input_path}' não encontrado.")
        sys.exit(1)

    # 1. Validar Header ELF Básico
    if elf_data[0:4] != ELF_MAGIC:
        print("Erro: Não é um arquivo ELF válido.")
        sys.exit(1)

    # Verificar se é 64-bit (offset 4 == 2) e Little Endian (offset 5 == 1)
    if elf_data[4] != 2 or elf_data[5] != 1:
        print("Erro: Apenas ELF 64-bit Little Endian é suportado.")
        sys.exit(1)

    # 2. Ler Cabeçalho ELF (Offset padrão para ELF64)
    e_entry = read_u64(elf_data, 0x18)     # Entry Point Virtual Address
    e_phoff = read_u64(elf_data, 0x20)     # Program Header Offset
    e_phentsize = read_u16(elf_data, 0x36) # Tamanho de cada entrada PH
    e_phnum = read_u16(elf_data, 0x38)     # Número de entradas PH

    print(f"ELF: Entry=0x{e_entry:x}, PHOffset=0x{e_phoff:x}, PHNum={e_phnum}")

    text_bytes = b''
    data_bytes = b''
    bss_size = 0

    # 3. Iterar sobre os Program Headers (Segmentos)
    # O Kernel carrega SEGMENTOS, não seções.
    found_text = False
    
    for i in range(e_phnum):
        offset = e_phoff + (i * e_phentsize)
        
        p_type = read_u32(elf_data, offset)
        p_flags = read_u32(elf_data, offset + 4)
        p_offset = read_u64(elf_data, offset + 8)  # Offset no arquivo
        p_vaddr = read_u64(elf_data, offset + 16)  # Endereço virtual
        p_filesz = read_u64(elf_data, offset + 32) # Tamanho no arquivo
        p_memsz = read_u64(elf_data, offset + 40)  # Tamanho na memória

        # Estamos interessados apenas em PT_LOAD (1)
        if p_type != PT_LOAD:
            continue

        # Extrair dados do segmento
        segment_data = elf_data[p_offset : p_offset + p_filesz]

        # Lógica simplificada de detecção:
        # Se tem flag EXEC (PF_X=1), tratamos como TEXT
        # Se tem flag WRITE (PF_W=2) e não EXEC, tratamos como DATA
        
        if (p_flags & PF_X):
            print(f"  Segmento TEXT encontrado: Offset=0x{p_offset:x} Size={p_filesz} bytes")
            text_bytes = segment_data
            found_text = True
        elif (p_flags & PF_W):
            print(f"  Segmento DATA encontrado: Offset=0x{p_offset:x} Size={p_filesz} bytes")
            data_bytes = segment_data
            # BSS é a memória extra alocada que não está no arquivo (memsz - filesz)
            if p_memsz > p_filesz:
                bss_size += (p_memsz - p_filesz)
                print(f"  BSS detectado: {p_memsz - p_filesz} bytes")

    if not found_text:
        print("Aviso: Nenhum segmento executável encontrado. O arquivo pode estar vazio ou incorreto.")

    # 4. Construir o Header SBF
    # Struct C:
    # magic(4), bits(1), endian(1), flags(2), entry(8), 
    # text_off(4), text_sz(4), data_off(4), data_sz(4), bss_sz(4)
    # Total: 36 bytes

    header_size = 36
    text_offset = header_size
    data_offset = text_offset + len(text_bytes)

    # Formato struct.pack: < (little endian), 4s B B H Q I I I I I
    sbf_header = struct.pack(
        '<4sBBHQIIIII',
        SBF_MAGIC,          # magic
        64,                 # bits
        1,                  # endian
        0,                  # flags
        e_entry,            # entry_point
        text_offset,        # text_offset
        len(text_bytes),    # text_size
        data_offset,        # data_offset
        len(data_bytes),    # data_size
        bss_size            # bss_size
    )

    # 5. Escrever Arquivo SBF
    with open(output_path, 'wb') as f:
        f.write(sbf_header)
        f.write(text_bytes)
        f.write(data_bytes)

    print(f"\nSucesso! Arquivo SBF gerado em: {output_path}")
    print(f"  Header: {header_size} bytes")
    print(f"  Text:   {len(text_bytes)} bytes")
    print(f"  Data:   {len(data_bytes)} bytes")
    print(f"  BSS:    {bss_size} bytes")
    print(f"  Total:  {os.path.getsize(output_path)} bytes")

if __name__ == '__main__':
    main()

