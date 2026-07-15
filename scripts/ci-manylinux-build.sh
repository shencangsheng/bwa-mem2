#!/usr/bin/env bash
# Build one PLATFORM inside quay.io/pypa/manylinux2014_x86_64 (glibc 2.17).
# Invoked from the host runner via: docker run -v "$PWD":/src -w /src ... bash scripts/ci-manylinux-build.sh
#
# CentOS 7 ships glibc 2.17; binaries from ubuntu-22.04 need GLIBC_2.33+ and fail
# with: version `GLIBC_2.33' not found.
set -euo pipefail

PLATFORM="${1:?usage: $0 <PLATFORM>}"
ARTIFACT="${2:?usage: $0 <PLATFORM> <artifact-name>}"

# Read all of stdin, then print first/last line.
# Under pipefail, `cmd | head -n1` / `tail -n1` closes the pipe early → SIGPIPE (exit 141).
first_line() {
  local out
  out="$(cat)"
  printf '%s\n' "${out%%$'\n'*}"
}

last_line() {
  local out
  out="$(cat)"
  printf '%s\n' "${out##*$'\n'}"
}

echo "==> manylinux2014 build PLATFORM=${PLATFORM} artifact=${ARTIFACT}"
echo "glibc (build host):"
ldd --version | first_line

# CentOS 7 vault / manylinux image already has a recent gcc toolchain.
yum install -y -q zlib-devel >/dev/null

# Prefer the manylinux-provided modern GCC when present.
if [[ -x /opt/rh/devtoolset-10/root/usr/bin/g++ ]]; then
  # shellcheck disable=SC1091
  source /opt/rh/devtoolset-10/enable || true
fi
# manylinux2014 images also ship /usr/local or /opt/python toolchains; default g++ is fine if ≥7.
g++ --version | first_line

make -f Makefile.platforms clean || true
make -f Makefile.platforms PLATFORM="${PLATFORM}" PORTABLE=1

echo "==> built binaries"
ls -lh bwa-mem2 bwa-mem2.* 2>/dev/null || ls -lh bwa-mem2*
file bwa-mem2 bwa-mem2.* 2>/dev/null || file bwa-mem2*

# Reject any GLIBC symbol newer than 2.17 (CentOS 7 ceiling).
check_glibc() {
  local bin="$1"
  [[ -f "$bin" ]] || return 0
  local ver
  # Consume the full pipeline before taking the last line (avoids SIGPIPE under pipefail).
  ver="$(objdump -T "$bin" 2>/dev/null | grep -oE 'GLIBC_[0-9.]+' | sed 's/^GLIBC_//' | sort -Vu | last_line || true)"
  echo "  ${bin}: max ${ver:+GLIBC_${ver}}${ver:-none}"
  if [[ -n "${ver}" ]]; then
    # sort -V: if the higher of {ver, 2.17} is not 2.17, then ver > 2.17
    if [[ "$(printf '%s\n' "${ver}" "2.17" | sort -V | last_line)" != "2.17" ]]; then
      echo "ERROR: ${bin} requires GLIBC_${ver} (> GLIBC_2.17); not CentOS 7 portable" >&2
      exit 1
    fi
  fi
}

echo "==> glibc symbol check (must be ≤ 2.17)"
for b in bwa-mem2 bwa-mem2.sse41 bwa-mem2.sse42 bwa-mem2.avx bwa-mem2.avx2 bwa-mem2.avx512bw; do
  check_glibc "$b"
done

STAGE="stage/${ARTIFACT}"
mkdir -p "${STAGE}"
for b in bwa-mem2 bwa-mem2.sse41 bwa-mem2.sse42 bwa-mem2.avx \
         bwa-mem2.avx2 bwa-mem2.avx512bw; do
  [[ -e "$b" ]] && cp -a "$b" "${STAGE}/"
done
{
  echo "platform=${PLATFORM}"
  echo "compiler=gcc"
  echo "artifact=${ARTIFACT}"
  echo "container=quay.io/pypa/manylinux2014_x86_64"
  echo "glibc_target=2.17"
  echo "note=CentOS 7 / RHEL 7 portable (Intel oneAPI skipped: modern icx needs newer glibc)"
  g++ --version | first_line
  ldd --version | first_line
} > "${STAGE}/BUILD_INFO.txt"
ls -lh "${STAGE}"
