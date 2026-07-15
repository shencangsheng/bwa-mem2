# Extra builds (flat) — CentOS 7 / glibc 2.17 package

This tarball was built in **manylinux2014 (glibc 2.17)** and runs on
**CentOS 7 / RHEL 7** and newer.

Root `../bwa-mem2` is a **super-dispatcher** over GCC ISA siblings. With AVX-512
but no CLWB it hands off to `bwa-mem2.gcc-full` here. Prefer `../bwa-mem2`.

| File | Role |
|------|------|
| `bwa-mem2.gcc-full` | GCC multi handoff target (+ `.gcc-full.<isa>` siblings) |
| `bwa-mem2.gcc-avx2-fleet` | GCC ≤ AVX2 pin |
| `bwa-mem2.sse41` … `bwa-mem2.avx512bw` | Single-ISA pins |

```sh
../bwa-mem2 mem ref.fa r1.fq r2.fq > out.sam
../bwa-mem2 which
```
