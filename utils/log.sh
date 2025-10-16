#!/usr/bin/env bash
# utils/log_text_with_ignore.sh
# Gera log apenas de arquivos texto de uma pasta, permite passar arquivos/padrões para IGNORAR.
# Uso:
#   ./utils/log_text_with_ignore.sh [SRC_DIR] [OUT_FILE] [IGNORA1] [IGNORA2] [...]
# Ex:
#   ./utils/log_text_with_ignore.sh kernel/src /sdcard/log.txt NeoOS.iso "*.bin" "old_*"

set -eo pipefail

SRC_DIR="${1:-kernel/src}"
OUT_FILE="${2:-/sdcard/log.txt}"
shift 2 || true

# Lista de padrões/entradas a ignorar (podem ser glob patterns, nomes ou caminhos)
IGNORES=("$@")

# Função que decide se um arquivo deve ser ignorado.
# Retorna 0 se IGNORAR, 1 se NÃO IGNORAR.
should_ignore() {
  local file="$1"
  local file_basename="$(basename "$file")"
  local file_real="$(realpath "$file" 2>/dev/null || printf "%s" "$file")"
  # Se for o proprio arquivo de saída -> ignorar
  local out_real="$(realpath "$OUT_FILE" 2>/dev/null || printf "%s" "$OUT_FILE")"
  if [ "$file_real" = "$out_real" ]; then
    return 0
  fi

  # Checa cada padrão passado
  for pat in "${IGNORES[@]}"; do
    [ -z "$pat" ] && continue

    # 1) se pat é um caminho absoluto exato
    if [ "$pat" = "$file_real" ]; then
      return 0
    fi

    # 2) se pat é o mesmo nome do arquivo
    if [ "$pat" = "$file_basename" ]; then
      return 0
    fi

    # 3) se pat contém glob metacharacters -> compara como glob
    #    (ex: *.iso, build/*, subdir/*.c)
    #    usamos double-bracket glob match
    if [[ "$file" == $pat ]] || [[ "$file_basename" == $pat ]]; then
      return 0
    fi

    # 4) se pat tem barra mas não é absoluto, testa também com path relativo
    #    ex: "kernel/src/old_*" com arquivo "kernel/src/old_file.c"
    if [[ "$file" == $pat ]] ; then
      return 0
    fi
  done

  return 1
}

# Verificações básicas
if [ ! -d "$SRC_DIR" ]; then
  echo "ERRO: pasta '$SRC_DIR' não existe." >&2
  exit 1
fi

echo "Gerando log em: $OUT_FILE"
echo "Pasta origem: $SRC_DIR"
if [ "${#IGNORES[@]}" -gt 0 ]; then
  echo "Ignorando padrões/arquivos: ${IGNORES[*]}"
else
  echo "Nenhum arquivo/padrão sendo ignorado."
fi
echo "========================================" > "$OUT_FILE"
echo >> "$OUT_FILE"

shopt -s nullglob

for file in "$SRC_DIR"/*; do
  [ -f "$file" ] || continue

  if should_ignore "$file"; then
    echo "Pulando (ignorando): $(basename "$file")"
    continue
  fi

  # Só processa se for texto
  if grep -Iq . "$file" 2>/dev/null; then
    fname="$(basename "$file")"
    echo "Processando: $fname"
    {
      echo
      echo "### ARQUIVO: $fname"
      echo "# CAMINHO: $file"
      echo "----------------------------------------"
      cat "$file"
    } >> "$OUT_FILE"
  else
    # É binário -> ignorado (não coloca Base64 pra evitar letras doidas)
    echo "### ARQUIVO BINÁRIO: $(basename "$file") (ignorado)" >> "$OUT_FILE"
  fi
done

echo
echo "✅ Log gerado em $OUT_FILE"
echo "Abrindo para visualização/cópia…"

if command -v nano >/dev/null 2>&1; then
  nano "$OUT_FILE"
else
  less "$OUT_FILE"
fi
