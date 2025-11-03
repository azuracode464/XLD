#!/bin/bash

# build.sh - Script completo para compilar o kernel, os programas,
#            gerenciar a imagem de disco e executar o sistema.

# --- Configuração ---
# Para o script se o ocorrer um erro
set -e

# Nome da imagem de disco rígido
HDD_IMAGE="template-x86_64.hdd"

# Diretório onde os programas ELF compilados são colocados
APPS_BIN_DIR="bin"

# --- Etapa 1: Limpeza e Compilação ---

echo ">>> Limpando artefatos antigos..."
make clean

echo ">>> Compilando o kernel e os programas..."
# O 'make' deve ser configurado para compilar tanto o kernel quanto os programas
# e colocar os executáveis dos programas no diretório $APPS_BIN_DIR.
make TOOLCHAIN=llvm

echo ">>> Build do kernel e dos programas concluído."

# --- Etapa 2: Gerenciamento da Imagem de Disco (.hdd) ---

echo ">>> Gerenciando a imagem de disco: $HDD_IMAGE"

# Verifica se a imagem de disco já existe.
if [ ! -f "$HDD_IMAGE" ]; then
    echo "--- Imagem de disco não encontrada. Criando uma nova..."
    
    # 1. Cria um arquivo de 64MB
    dd if=/dev/zero of="$HDD_IMAGE" bs=1M count=64
    
    # 2. Formata a imagem com mformat (FAT32), que é compatível com mtools
    echo "--- Formatando a imagem com mformat (FAT32)..."
    mformat -i "$HDD_IMAGE" -F -v "XLD_OS" ::
else
    echo "--- Imagem de disco já existe. Usando a imagem existente."
fi

# 3. Garante que o diretório /apps exista dentro da imagem
echo "--- Garantindo que o diretório /apps exista na imagem..."
# O '@' antes do comando suprime a saída de erro se o diretório já existir.
@mmd -i "$HDD_IMAGE" ::/apps 2>/dev/null || true

# 4. Copia todos os programas .elf para a imagem de disco
echo "--- Copiando programas .elf para a imagem de disco..."

# Verifica se o diretório de binários de aplicativos existe e tem arquivos .elf
if [ -d "$APPS_BIN_DIR" ] && [ "$(ls -A $APPS_BIN_DIR/*.elf 2>/dev/null)" ]; then
    for app in "$APPS_BIN_DIR"/*.elf; do
        if [ -f "$app" ]; then
            echo "    -> Copiando $(basename "$app")"
            mcopy -i "$HDD_IMAGE" "$app" ::/apps
        fi
    done
else
    echo "--- Aviso: Nenhum programa .elf encontrado em '$APPS_BIN_DIR'. O disco estará vazio."
fi

echo ">>> Imagem de disco preparada com sucesso."

# --- Etapa 3: Verificação e Execução (Opcional) ---

echo ">>> Conteúdo final do diretório /apps no disco:"
# Descomente a linha abaixo para executar o QEMU automaticamente após o build.
# read -p "Pressione [Enter] para iniciar o QEMU..."

# CORRETO - Apenas uma flag de disco, a -drive

# ... (parte de compilação e mtools do seu build.sh) ...

echo ">>> Iniciando QEMU..."
qemu-system-x86_64 \
    -cdrom template-x86_64.iso \
    -drive if=ide,format=raw,file=template-x86_64.hdd \
    -boot d \
    -no-reboot \
    -no-shutdown \
    -vnc :0 -monitor stdio
echo ">>> Sessão do QEMU finalizada."

