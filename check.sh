#!/usr/bin/env bash

set -euo pipefail

usage() {
  cat <<'EOF'
Usage: ./check.sh

The script builds ./hexdump, compares its output against verify.py for every
inputs/*.bin fixture, then checks that symbol hexdump_avx is at least 90% AVX
instructions.
EOF
}

die() {
  echo "error: $*" >&2
  exit 1
}

pick_cc() {
  if command -v cc >/dev/null 2>&1; then
    echo "cc"
    return
  fi
  if command -v gcc >/dev/null 2>&1; then
    echo "gcc"
    return
  fi
  if command -v clang >/dev/null 2>&1; then
    echo "clang"
    return
  fi
  return 1
}

default_build() {
  local make_file=""
  local compiler=""

  for candidate in Makefile makefile GNUmakefile; do
    if [[ -f "$candidate" ]]; then
      make_file="$candidate"
      break
    fi
  done

  if [[ -n "$make_file" ]]; then
    command -v make >/dev/null 2>&1 || die "make is required to use $make_file"
    echo "==> building with make"
    make
    return
  fi

  [[ -f "hexdump.c" ]] || die "no Makefile found and no hexdump.c available for direct compilation"

  compiler="$(pick_cc)" || die "no C compiler found; install cc, gcc, or clang"
  echo "==> building with $compiler"
  "$compiler" -O3 -mavx2 -std=c11 -Wall -Wextra -o "./hexdump" hexdump.c
}

binary="./hexdump"
inputs_dir="inputs"
symbol="hexdump_avx"
min_avx_pct="90"
previous_max_file="previous_max.txt"

if (($# > 0)); then
  case "$1" in
    --help|-h)
      usage
      exit 0
      ;;
    *)
      die "check.sh takes no arguments"
      ;;
  esac
fi

command -v python3 >/dev/null 2>&1 || die "python3 is required"
command -v objdump >/dev/null 2>&1 || die "objdump is required"
default_build

[[ -f "$binary" ]] || die "binary not found: $binary"
[[ -d "$inputs_dir" ]] || die "inputs directory not found: $inputs_dir"

mapfile -t inputs < <(find "$inputs_dir" -maxdepth 1 -type f -name '*.bin' | sort)
(( ${#inputs[@]} > 0 )) || die "no .bin fixtures found in $inputs_dir"

expected_file="$(mktemp)"
actual_file="$(mktemp)"
disasm_file="$(mktemp)"
trap 'rm -f "$expected_file" "$actual_file" "$disasm_file"' EXIT

echo "==> checking output on ${#inputs[@]} fixture(s)"
for input_path in "${inputs[@]}"; do
  python3 verify.py "$input_path" >"$expected_file"
  "$binary" "$input_path" >"$actual_file"

  if ! cmp -s "$expected_file" "$actual_file"; then
    echo "mismatch for $input_path" >&2
    diff -u "$expected_file" "$actual_file" >&2 || true
    exit 1
  fi
done

echo "==> output matches verify.py"

echo "==> checking AVX ratio for symbol: $symbol"
objdump -d -M intel --disassemble="$symbol" "$binary" >"$disasm_file"

read -r total_count avx_count < <(
  awk '
    /^[[:space:]]*[0-9a-f]+:/ {
      field_count = split($0, fields, /\t+/)
      asm = fields[field_count]
      split(asm, parts, /[[:space:]]+/)
      op = parts[1]
      if (op == "") {
        next
      }
      total++
      if (op ~ /^v[[:alnum:]_.]+$/) {
        avx++
      }
    }
    END {
      print total + 0, avx + 0
    }
  ' "$disasm_file"
)

(( total_count > 0 )) || die "no instructions found for symbol '$symbol'"

avx_pct="$(
  awk -v avx="$avx_count" -v total="$total_count" 'BEGIN {
    printf "%.2f", (100.0 * avx) / total
  }'
)"

echo "AVX instructions: $avx_count / $total_count (${avx_pct}%)"
echo "AVX ratio: ${avx_pct}%"

previous_max="0.00"
if [[ -f "$previous_max_file" ]]; then
  previous_max="$(tr -d '[:space:]' < "$previous_max_file")"
fi

echo "Previous max AVX ratio: ${previous_max}%"

if awk -v current="$avx_pct" -v previous="$previous_max" 'BEGIN {
  exit !(current + 0 > previous + 0)
}'; then
  printf '%s\n' "$avx_pct" > "$previous_max_file"
  echo "Updated previous max AVX ratio to ${avx_pct}%"
fi

awk -v pct="$avx_pct" -v min="$min_avx_pct" 'BEGIN {
  exit !(pct + 0 >= min + 0)
}' || die "AVX ratio below threshold: ${avx_pct}% < ${min_avx_pct}%"

echo "==> AVX ratio meets threshold"
