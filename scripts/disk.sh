#!/usr/bin/env bash
# disk.sh - Cria disk.img FAT32 com 1MB e arquivo hello.txt

echo "=== Criando disco FAT32 de 1MB ==="

# 1. Cria imagem vazia de 1MB
echo "[1/5] Criando imagem de 1MB..."
dd if=/dev/zero of=disk.img bs=1M count=1 status=none

# 2. Cria tabela de partição MBR com uma partição FAT32
echo "[2/5] Criando MBR e partição..."
echo -e "o\nn\np\n1\n\n\nt\nc\nw" | fdisk disk.img > /dev/null 2>&1

# 3. Monta a imagem como loop device (no Termux precisa de proot ou método alternativo)
# Como Termux não tem loop devices fácil, vamos usar mcopy do mtools

echo "[3/5] Formatando como FAT32..."
# Cria sistema de arquivos FAT32 usando mkfs.fat do pacote dosfstools
if command -v mkfs.fat > /dev/null 2>&1; then
    # Se mkfs.fat estiver disponível (pode não estar no Termux)
    mkfs.fat -F 32 disk.img
else
    # Alternativa: usar mformat do mtools
    echo "Usando mtools para formatar..."
    mformat -F -v XLDDISK -i disk.img ::
fi

# 4. Cria arquivo hello.txt
echo "[4/5] Criando hello.txt..."
echo "Hello from XLDfat32 kernel!" > hello.txt
echo "Compiled on phone with green line!" >> hello.txt
echo "Booted via Limine + FAT32!" >> hello.txt

# 5. Copia arquivo para o disco usando mcopy
echo "[5/5] Copiando hello.txt para /root/hello.txt..."
if command -v mcopy > /dev/null 2>&1; then
    # Cria diretório /root
    mmd -i disk.img ::/root
    # Copia arquivo
    mcopy -i disk.img hello.txt ::/root/hello.txt
    
    # Lista conteúdo para verificar
    echo "=== Conteúdo do disco ==="
    mdir -i disk.img ::/
    mdir -i disk.img ::/root
else
    echo "ERRO: mtools não instalado"
    echo "Instale: pkg install mtools"
    exit 1
fi

# 6. Limpa arquivo temporário
rm -f hello.txt

echo "=== Disco criado com sucesso! ==="
echo "Tamanho: $(stat -c%s disk.img) bytes"
echo "Use em QEMU: qemu-system-x86_64 -drive file=disk.img,format=raw"
echo "Ou no seu kernel com: fat32_mount()"
