#!/usr/bin/env bash

set -euo pipefail

usage() {
  cat <<'EOF'
Usage: ./check.sh

The script builds ./hexdump, streams inputs/*.bin fixtures into both
verify.py and ./hexdump across several hardcoded start addresses, then
reports the AVX instruction ratio for symbol hexdump_avx.
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
  local compiler=""

  [[ -f "hexdump.c" ]] || die "no hexdump.c available for compilation"

  compiler="$(pick_cc)" || die "no C compiler found; install cc, gcc, or clang"
  echo "==> building with $compiler"
  "$compiler" -O3 -mavx2 -std=c11 -Wall -Wextra -o "./hexdump" hexdump.c
}

binary="./hexdump"
symbol="hexdump_avx"
previous_max_file="previous_max.txt"
start_addrs=(
  "0x00000090E630F8E0"
  "0x00007FF61234A0C0"
  "0x0000000012345678"
  "0x0000ABCDEF102030"
)

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

command -v objdump >/dev/null 2>&1 || die "objdump is required"
default_build

[[ -f "$binary" ]] || die "binary not found: $binary"

actual_file="$(mktemp)"
expected_file="$(mktemp)"
disasm_file="$(mktemp)"
trap 'rm -f "$actual_file" "$expected_file" "$disasm_file"' EXIT

command -v python3 >/dev/null 2>&1 || die "python3 is required"

shopt -s nullglob
inputs=(inputs/*.bin)
shopt -u nullglob

(( ${#inputs[@]} > 0 )) || die "no fixture files found under inputs/"

echo "==> checking output against verify.py"

pass_count=0
for input_path in "${inputs[@]}"; do
  for start_addr in "${start_addrs[@]}"; do
    python3 verify.py "$start_addr" <"$input_path" >"$expected_file"
    if ! "$binary" "$start_addr" <"$input_path" >"$actual_file"; then
      die "binary failed for fixture: $input_path (start address $start_addr)"
    fi

    if ! cmp -s "$expected_file" "$actual_file"; then
      echo "Mismatch for $input_path at start address $start_addr" >&2
      diff -u "$expected_file" "$actual_file" >&2 || true
      die "output differs from verify.py"
    fi

    pass_count=$((pass_count + 1))
  done
done

echo "==> output matches verify.py on ${pass_count} fixture/address case(s)"

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
else
  echo "AVX ratio did not exceed the recorded max"
fi

echo "==> check completed"
