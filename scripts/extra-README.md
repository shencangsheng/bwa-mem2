# Extra builds (flat)

Root `../bwa-mem2` is the upstream-style **Intel official** multi build
(`make CXX=icpc multi`). Files here are alternatives — no subdirectories.

| File | Toolchain | When to use |
|------|-----------|-------------|
| `bwa-mem2.gcc-full` | GCC multi | Heterogeneous fleets; AMD / no CLWB |
| `bwa-mem2.gcc-avx2-fleet` | GCC ≤ AVX2 | Zen1–Zen3 / Intel AVX2-only |
| `bwa-mem2.intel-avx512-noclwb` | Intel AVX-512 | icpc AVX-512 without CLWB |
| `bwa-mem2.sse41` … `bwa-mem2.avx512bw` | GCC single ISA | Pin one binary per image |

Multi bundles keep ISA siblings next to the dispatcher (required by `runsimd`):

```
bwa-mem2.gcc-full
bwa-mem2.gcc-full.sse41
bwa-mem2.gcc-full.sse42
bwa-mem2.gcc-full.avx
bwa-mem2.gcc-full.avx2
bwa-mem2.gcc-full.avx512bw
```

```sh
../bwa-mem2 mem ref.fa r1.fq r2.fq > out.sam                 # Intel official (root)
./bwa-mem2.gcc-full mem ref.fa r1.fq r2.fq > out.sam         # GCC multi
./bwa-mem2.avx2 mem ref.fa r1.fq r2.fq > out.sam             # pinned AVX2
```

Each variant also has `BUILD_INFO.<name>.txt`.