# Extra builds (flat) — Intel / modern glibc package

This tarball was built on **Ubuntu 22.04** (typically needs **GLIBC 2.33+**).
It will **not** run on CentOS 7 — use the `*_centos7_x64-linux` tarball there.

Root `../bwa-mem2` prefers **Intel official** ISA siblings when CLWB is present;
without CLWB but with AVX-512 it hands off here (`gcc-full`, then
`intel-avx512-noclwb`). Prefer `../bwa-mem2`.

| File | Toolchain | Role |
|------|-----------|------|
| `bwa-mem2.gcc-full` | GCC multi | Auto handoff; heterogeneous fleets |
| `bwa-mem2.gcc-avx2-fleet` | GCC ≤ AVX2 | Zen1–Zen3 / AVX2-only pin |
| `bwa-mem2.intel-avx512-noclwb` | Intel AVX-512 | Fallback if gcc-full missing |
| `bwa-mem2.sse41` … `bwa-mem2.avx512bw` | GCC single ISA | Pin one binary per image |

```sh
../bwa-mem2 mem ref.fa r1.fq r2.fq > out.sam
../bwa-mem2 which
./bwa-mem2.gcc-full mem ref.fa r1.fq r2.fq > out.sam
```
