# Shared settings for platform wrapper Makefiles (included by Makefile.*).
# Upstream Makefile is never edited — only invoked via $(MAKE) -f Makefile ...

ROOT_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
UPSTREAM := $(ROOT_DIR)/Makefile

CXX ?= g++
CC  ?= gcc
# Static-link libgcc/libstdc++ for ECS image portability (upstream: portable=1)
PORTABLE ?= 1

# oneAPI icpx (LLVM) defaults to C++17; ksort.h still uses the removed
# `register` keyword. Keep this out of the upstream Makefile.
INTEL_CXXFLAGS ?= -Wno-register

SAFE_INC := -I$(ROOT_DIR)/ext/safestringlib/include
SAFE_LIB := -L$(ROOT_DIR)/ext/safestringlib -lsafestring

define clean_objs
	rm -f $(ROOT_DIR)/src/*.o $(ROOT_DIR)/libbwa.a
	$(MAKE) -C $(ROOT_DIR)/ext/safestringlib clean
endef

# $(1)=arch keyword or custom flag, $(2)=output EXE name
define build_one
	$(call clean_objs)
	$(MAKE) -C $(ROOT_DIR) -f Makefile \
		arch=$(1) EXE=$(2) portable=$(PORTABLE) CXX=$(CXX) CC=$(CC) all
endef

define build_dispatcher
	$(CXX) -Wall -O3 $(ROOT_DIR)/src/runsimd.cpp $(SAFE_INC) $(SAFE_LIB) \
		$(if $(filter 1,$(PORTABLE)),-static-libgcc -static-libstdc++) \
		-o $(ROOT_DIR)/bwa-mem2
endef
