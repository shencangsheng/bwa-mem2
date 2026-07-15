# Loaded via MAKEFILES (before the upstream Makefile).
# Do NOT pass CXXFLAGS= on the make command line — that replaces the whole
# variable and drops `-msse4.1` / `-march=...` from Makefile's CXXFLAGS+=.
# Do NOT use `override` here — MAKEFILES is read first, and override would
# block the upstream Makefile's later CXXFLAGS+= $(ARCH_FLAGS).
#
# oneAPI icpx (LLVM) defaults to C++17; ksort.h still uses removed `register`.
CXXFLAGS += -Wno-register
