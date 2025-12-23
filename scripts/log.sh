#!/bin/bash
set -eo pipefail

# Diretório de origem padrão: kernel/src
SRC_DIR="${1:-kernel/src}"
# Arquivo de saída padrão: /sdcard/log.txt
OUT_FILE="${2:-/sdcard/log.txt}"
shift 2 || true

# Padrões/entradas a ignorar (todos os argumentos restantes)
IGNORES=("$@")

# Diretórios que sempre vamos ignorar
AUTO_IGNORES=(
  ".git"
  "build"
  "bin"
  "node_modules"
  "__pycache__"
)

# Função que decide se um arquivo deve ser ignorado.
# Retorna 0 se IGNORAR, 1 se NÃO IGNORAR.
should_ignore() {
  local file="$1"
  local file_basename
  file_basename="$(basename "$file")"

  # Ignora o próprio arquivo de saída
  local out_real file_real
  out_real="$(realpath "$OUT_FILE" 2>/dev/null || printf "%s" "$OUT_FILE")"
  file_real="$(realpath "$file" 2>/dev/null || printf "%s" "$file")"
  [ "$file_real" = "$out_real" ] && return 0

  # Ignora diretórios automáticos
  for dir in "${AUTO_IGNORES[@]}"; do
    [[ "$file" == *"/$dir"* ]] && return 0
  done

  # Checa cada padrão de ignore fornecido pelo usuário (suporta wildcards)
  for pat in "${IGNORES[@]}"; do
    [ -z "$pat" ] && continue
    if [[ "$file" == $pat ]] || [[ "$file_basename" == $pat ]]; then
      return 0
    fi
  done

  return 1
}

# --- Início da Execução ---
[ ! -d "$SRC_DIR" ] && { echo "ERRO: Diretório '$SRC_DIR' não existe." >&2; exit 1; }

echo "Gerando log em: $OUT_FILE"
echo "Pasta origem: $SRC_DIR"
[ "${#IGNORES[@]}" -gt 0 ] && echo "Ignorando padrões: ${IGNORES[*]}" || echo "Nenhum padrão de ignore especificado."

# Limpa/inicia o arquivo de log
{
  echo "========================================"
  echo " Log do Projeto: $SRC_DIR"
  echo " Gerado em: $(date)"
  echo "========================================"
} > "$OUT_FILE"

# Processamento recursivo
find "$SRC_DIR" -type f | while IFS= read -r file; do
  should_ignore "$file" && { echo "  -> Pulando (ignorado): $file"; continue; }

  fname_rel="${file#$SRC_DIR/}"

  # Verifica se é texto
  if grep -Iq . "$file" 2>/dev/null; then
    echo "Processando: $fname_rel"
    {
      echo
      echo "### ARQUIVO: $fname_rel"
      echo "----------------------------------------"
      cat "$file"
      echo
    } >> "$OUT_FILE"
  else
    # Arquivo binário: registra hash e tamanho
    hash_md5=$(md5sum "$file" | awk '{print $1}')
    size_bytes=$(stat -c%s "$file")
    echo "  -> Pulando (binário): $fname_rel"
    {
      echo
      echo "### ARQUIVO BINÁRIO: $fname_rel"
      echo " Tamanho: $size_bytes bytes | MD5: $hash_md5"
    } >> "$OUT_FILE"
  fi
done

echo
echo "✅ Log recursivo gerado com sucesso em $OUT_FILE"
