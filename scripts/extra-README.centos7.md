# Extra builds (flat) — CentOS 7 / glibc 2.17 package

The dispatcher and GCC fallbacks were built in **manylinux2014 (glibc 2.17)**;
the pinned upstream binaries are verified against the same glibc ceiling.
The complete package runs on **CentOS 7 / RHEL 7** and newer.

Root `../bwa-mem2` is a **super-dispatcher** over the upstream v2.2.1
Intel-optimized ISA siblings. It hands AMD EPYC and Intel AVX-512 CPUs without
CLWB to `bwa-mem2.gcc-full` here. On AMD, `gcc-full` prefers AVX2 over
AVX512BW. Prefer `../bwa-mem2`.

| File | Role |
|------|------|
| `bwa-mem2.gcc-full` | GCC multi handoff target (+ `.gcc-full.<isa>` siblings) |
| `bwa-mem2.gcc-avx2-fleet` | GCC ≤ AVX2 pin |
| `bwa-mem2.sse41` … `bwa-mem2.avx512bw` | Single-ISA pins |

```sh
../bwa-mem2 mem ref.fa r1.fq r2.fq > out.sam
../bwa-mem2 which
```
