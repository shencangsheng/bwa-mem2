# Extra builds (flat)

Release ships **two** tarballs — pick by OS:

| Tarball suffix | glibc | Root binaries |
|----------------|-------|---------------|
| `*_centos7_x64-linux` | 2.17 (CentOS 7+) | GCC multi |
| `*_intel_x64-linux` | 2.33+ (Ubuntu 22.04+) | Intel official |

See `extra/README.md` inside each package for that flavor’s file list.
On CentOS 7, only the `centos7` tarball works (`GLIBC_2.33` errors mean you
grabbed the Intel package).
