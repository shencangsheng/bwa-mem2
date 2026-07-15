# Extra builds (flat)

The `*_centos7_x64-linux` package runs on CentOS 7.9 / glibc 2.17 and newer.
Its root dispatcher uses upstream Intel-optimized binaries on compatible Intel
CPUs, then hands AMD EPYC and Intel AVX-512 CPUs without CLWB to the portable
GCC binaries in this directory.

See `extra/README.md` inside the package for the full file list.
