#!/usr/bin/env bash
# Assemble one release tarball from downloaded CI artifacts under dist/bin-*.
#
# Usage:
#   scripts/ci-package.sh <centos7|intel> <VERSION> <dist-dir> <out-dir>
#
# centos7 → bwa-mem2-${VERSION}_centos7_x64-linux.tar.bz2  (glibc 2.17)
# intel   → bwa-mem2-${VERSION}_intel_x64-linux.tar.bz2    (glibc 2.33+, Intel root)
set -euo pipefail

FLAVOR="${1:?usage: $0 <centos7|intel> <VERSION> <dist-dir> <out-dir>}"
VERSION="${2:?}"
DIST="${3:?}"
OUT="${4:?}"

case "${FLAVOR}" in
  centos7)
    SUFFIX="centos7_x64-linux"
    ROOT_CANDIDATES=(bin-c7-root-gcc-multi c7-root-gcc-multi)
    SKIP_ROOT="c7-root-gcc-multi"
    MULTI_NAMES="gcc-full|gcc-avx2-fleet|c7-gcc-avx2-fleet"
    PREFIX_STRIP="c7-"
    ROOT_NOTE="super-dispatcher + GCC multi (manylinux2014 / glibc 2.17)"
    COPY_ROOT_AS_GCC_FULL=1
    ;;
  intel)
    SUFFIX="intel_x64-linux"
    ROOT_CANDIDATES=(bin-intel-official intel-official)
    SKIP_ROOT="intel-official"
    MULTI_NAMES="gcc-full|gcc-avx2-fleet|intel-avx512-noclwb"
    PREFIX_STRIP=""
    ROOT_NOTE="super-dispatcher + Intel official ISA siblings (ubuntu-22.04 / glibc 2.33+)"
    COPY_ROOT_AS_GCC_FULL=0
    ;;
  *)
    echo "ERROR: unknown flavor '${FLAVOR}' (want centos7|intel)" >&2
    exit 1
    ;;
esac

PKG="bwa-mem2-${VERSION}_${SUFFIX}"
mkdir -p "${OUT}/${PKG}/extra"

echo "Downloaded artifacts under ${DIST}:"
find "${DIST}" -type f | sort

find "${DIST}" -type f \( -name 'bwa-mem2' -o -name 'bwa-mem2.*' \) -exec chmod +x {} +

ROOT_SRC=""
for d in "${ROOT_CANDIDATES[@]}"; do
  if [[ -d "${DIST}/${d}" ]]; then ROOT_SRC="${DIST}/${d}"; break; fi
done
test -n "${ROOT_SRC}"
test -f "${ROOT_SRC}/bwa-mem2"
cp -a "${ROOT_SRC}"/. "${OUT}/${PKG}/"
mv "${OUT}/${PKG}/BUILD_INFO.txt" "${OUT}/${PKG}/BUILD_INFO.root.txt" 2>/dev/null || true

if [[ "${COPY_ROOT_AS_GCC_FULL}" -eq 1 ]]; then
  cp -a "${ROOT_SRC}/bwa-mem2" "${OUT}/${PKG}/extra/bwa-mem2.gcc-full"
  for isa in sse41 sse42 avx avx2 avx512bw; do
    if [[ -e "${ROOT_SRC}/bwa-mem2.${isa}" ]]; then
      cp -a "${ROOT_SRC}/bwa-mem2.${isa}" "${OUT}/${PKG}/extra/bwa-mem2.gcc-full.${isa}"
    fi
  done
  cp -a "${OUT}/${PKG}/BUILD_INFO.root.txt" "${OUT}/${PKG}/extra/BUILD_INFO.gcc-full.txt" 2>/dev/null || true
fi

for dir in "${DIST}"/bin-*; do
  [[ -d "$dir" ]] || continue
  name="${dir#${DIST}/bin-}"

  # Keep flavors isolated when both artifact trees share one dist/.
  if [[ "${FLAVOR}" == "centos7" ]]; then
    [[ "$name" == c7-* ]] || continue
  else
    [[ "$name" == c7-* ]] && continue
  fi

  if [[ "$name" == "${SKIP_ROOT}" ]]; then
    continue
  fi

  # centos7 artifacts are prefixed c7-; strip for public extra/ names.
  pub_name="$name"
  if [[ -n "${PREFIX_STRIP}" && "$name" == ${PREFIX_STRIP}* ]]; then
    pub_name="${name#"${PREFIX_STRIP}"}"
  fi
  case "$pub_name" in
    root-gcc-multi) continue ;;
  esac

  if [[ -f "${dir}/BUILD_INFO.txt" ]]; then
    cp -a "${dir}/BUILD_INFO.txt" "${OUT}/${PKG}/extra/BUILD_INFO.${pub_name}.txt"
  fi

  isa_count=0
  for isa in sse41 sse42 avx avx2 avx512bw; do
    if [[ -e "${dir}/bwa-mem2.${isa}" ]]; then
      isa_count=$((isa_count + 1))
    fi
  done

  is_multi=0
  if [[ "$pub_name" =~ ^(${MULTI_NAMES})$ ]]; then
    is_multi=1
  fi
  if [[ "${isa_count}" -ge 2 ]]; then
    is_multi=1
  fi

  if [[ "${is_multi}" -eq 1 && -e "${dir}/bwa-mem2" ]]; then
    cp -a "${dir}/bwa-mem2" "${OUT}/${PKG}/extra/bwa-mem2.${pub_name}"
    for isa in sse41 sse42 avx avx2 avx512bw; do
      if [[ -e "${dir}/bwa-mem2.${isa}" ]]; then
        cp -a "${dir}/bwa-mem2.${isa}" "${OUT}/${PKG}/extra/bwa-mem2.${pub_name}.${isa}"
      fi
    done
  else
    if [[ -e "${dir}/bwa-mem2" ]]; then
      cp -a "${dir}/bwa-mem2" "${OUT}/${PKG}/extra/bwa-mem2.${pub_name}"
    elif [[ -e "${dir}/bwa-mem2.avx512bw" ]]; then
      cp -a "${dir}/bwa-mem2.avx512bw" "${OUT}/${PKG}/extra/bwa-mem2.${pub_name}"
    else
      echo "ERROR: no binary in ${dir}" >&2
      ls -la "${dir}" >&2 || true
      exit 1
    fi
  fi
done

cp -a scripts/select-binary.sh README.md LICENSE "${OUT}/${PKG}/"
if [[ "${FLAVOR}" == "centos7" ]]; then
  cp -a scripts/extra-README.centos7.md "${OUT}/${PKG}/extra/README.md" 2>/dev/null \
    || cp -a scripts/extra-README.md "${OUT}/${PKG}/extra/README.md"
else
  cp -a scripts/extra-README.intel.md "${OUT}/${PKG}/extra/README.md" 2>/dev/null \
    || cp -a scripts/extra-README.md "${OUT}/${PKG}/extra/README.md"
fi
chmod +x "${OUT}/${PKG}"/bwa-mem2* "${OUT}/${PKG}/select-binary.sh" 2>/dev/null || true
chmod +x "${OUT}/${PKG}/extra"/bwa-mem2* 2>/dev/null || true

{
  echo "layout=single-tarball-flat-extra"
  echo "flavor=${FLAVOR}"
  echo "root=${ROOT_NOTE}"
  echo "extra=flat files under extra/ (see extra/README.md)"
  echo "note=Inspect: ./bwa-mem2 which"
} > "${OUT}/${PKG}/BUILD_INFO.txt"

echo "Package tree (${FLAVOR}):"
find "${OUT}/${PKG}" -maxdepth 2 -type f | sort
tar -C "${OUT}" -cjf "${OUT}/${PKG}.tar.bz2" "${PKG}"
ls -lh "${OUT}/${PKG}.tar.bz2"
echo "${OUT}/${PKG}.tar.bz2"
