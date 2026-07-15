# Extra builds (flat)

Root `../bwa-mem2` is a **super-dispatcher**: it prefers Intel official ISA
siblings when safe, and automatically hands off here when the CPU has AVX-512
but lacks CLWB (AMD Zen4, some cloud VMs). You usually only run `../bwa-mem2`.

| File | Toolchain | Role |
|------|-----------|------|
| `bwa-mem2.gcc-full` | GCC multi | Auto handoff target; heterogeneous fleets |
| `bwa-mem2.gcc-avx2-fleet` | GCC ≤ AVX2 | Zen1–Zen3 / Intel AVX2-only pin |
| `bwa-mem2.intel-avx512-noclwb` | Intel AVX-512 | Fallback handoff if gcc-full missing |
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
../bwa-mem2 mem ref.fa r1.fq r2.fq > out.sam                 # auto (recommended)
../bwa-mem2 which                                            # show selection
./bwa-mem2.gcc-full mem ref.fa r1.fq r2.fq > out.sam         # force GCC multi
./bwa-mem2.avx2 mem ref.fa r1.fq r2.fq > out.sam             # pinned AVX2
```

Each variant also has `BUILD_INFO.<name>.txt`.
