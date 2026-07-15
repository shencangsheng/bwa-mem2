#!/usr/bin/env bash
# Pick the best bwa-mem2.* binary for the current CPU from a directory
# that contains the multi build outputs (GCC builds: no CLWB gate).
#
# Usage:
#   ./select-binary.sh /path/to/bin-dir [bwa-mem2 args...]
#   BWA_MEM2_BIN=$(./select-binary.sh /path/to/bin-dir --print) 

set -euo pipefail

DIR="${1:?usage: $0 <bin-dir> [--print|bwa-mem2 args...]}"
shift || true

flags=""
if [[ -r /proc/cpuinfo ]]; then
  flags=$(grep -m1 '^flags' /proc/cpuinfo || true)
fi

pick=""
if [[ "$flags" == *avx512bw* ]] && [[ -x "$DIR/bwa-mem2.avx512bw" ]]; then
  pick="$DIR/bwa-mem2.avx512bw"
elif [[ "$flags" == *avx2* ]] && [[ -x "$DIR/bwa-mem2.avx2" ]]; then
  pick="$DIR/bwa-mem2.avx2"
elif [[ "$flags" == *avx* ]] && [[ -x "$DIR/bwa-mem2.avx" ]]; then
  pick="$DIR/bwa-mem2.avx"
elif [[ "$flags" == *sse4_2* ]] && [[ -x "$DIR/bwa-mem2.sse42" ]]; then
  pick="$DIR/bwa-mem2.sse42"
elif [[ "$flags" == *sse4_1* ]] && [[ -x "$DIR/bwa-mem2.sse41" ]]; then
  pick="$DIR/bwa-mem2.sse41"
elif [[ -x "$DIR/bwa-mem2" ]]; then
  pick="$DIR/bwa-mem2"
else
  echo "ERROR: no suitable bwa-mem2 binary in $DIR" >&2
  exit 1
fi

if [[ "${1:-}" == "--print" ]]; then
  printf '%s\n' "$pick"
  exit 0
fi

echo "Using $pick" >&2
exec "$pick" "$@"
