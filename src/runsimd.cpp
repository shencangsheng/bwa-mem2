/*************************************************************************************
                           The MIT License

   BWA-MEM2  (Sequence alignment using Burrows-Wheeler Transform),
   Copyright (C) 2019  Intel Corporation, Heng Li.

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
    without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.

Contacts: Vasimuddin Md <vasimuddin.md@intel.com>; Sanchit Misra <sanchit.misra@intel.com>;
                                Heng Li <hli@jimmy.harvard.edu> 
*****************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include <assert.h>
#ifdef __cplusplus
extern "C" {
#endif
#include "safe_str_lib.h"
#ifdef __cplusplus
}
#endif

#define SIMD_SSE     0x1
#define SIMD_SSE2    0x2
#define SIMD_SSE3    0x4
#define SIMD_SSSE3   0x8
#define SIMD_SSE4_1  0x10
#define SIMD_SSE4_2  0x20
#define SIMD_AVX     0x40
#define SIMD_AVX2    0x80
#define SIMD_AVX512F 0x100
#define SIMD_AVX512BW 0x200

#ifndef _MSC_VER
// adapted from https://github.com/01org/linux-sgx/blob/master/common/inc/internal/linux/cpuid_gnu.h
void __cpuidex(int cpuid[4], int func_id, int subfunc_id)
{
#if defined(__x86_64__)
	__asm__ volatile ("cpuid"
			: "=a" (cpuid[0]), "=b" (cpuid[1]), "=c" (cpuid[2]), "=d" (cpuid[3])
			: "0" (func_id), "2" (subfunc_id));
#else // on 32bit, ebx can NOT be used as PIC code
	__asm__ volatile ("xchgl %%ebx, %1; cpuid; xchgl %%ebx, %1"
			: "=a" (cpuid[0]), "=r" (cpuid[1]), "=c" (cpuid[2]), "=d" (cpuid[3])
			: "0" (func_id), "2" (subfunc_id));
#endif
}
#endif

static int x86_simd(void)
{
	int flag = 0, cpuid[4], max_id;
	__cpuidex(cpuid, 0, 0);
	max_id = cpuid[0];
	if (max_id == 0) return 0;
	__cpuidex(cpuid, 1, 0);
	if (cpuid[3]>>25&1) flag |= SIMD_SSE;
	if (cpuid[3]>>26&1) flag |= SIMD_SSE2;
	if (cpuid[2]>>0 &1) flag |= SIMD_SSE3;
	if (cpuid[2]>>9 &1) flag |= SIMD_SSSE3;
	if (cpuid[2]>>19&1) flag |= SIMD_SSE4_1;
	if (cpuid[2]>>20&1) flag |= SIMD_SSE4_2;
	if (cpuid[2]>>28&1) flag |= SIMD_AVX;
	if (max_id >= 7) {
		__cpuidex(cpuid, 7, 0);
		if (cpuid[1]>>5 &1) flag |= SIMD_AVX2;
		if (cpuid[1]>>16&1) flag |= SIMD_AVX512F;
		if (cpuid[1]>>30&1) flag |= SIMD_AVX512BW;
	}
	return flag;
}

/* CPUID.(EAX=7,ECX=0):EBX[24] = CLWB.
 * Upstream icpc -xCORE-AVX512 binaries refuse to run without it (#160, #236). */
static int x86_has_clwb(void)
{
	int cpuid[4], max_id;
	__cpuidex(cpuid, 0, 0);
	max_id = cpuid[0];
	if (max_id < 7) return 0;
	__cpuidex(cpuid, 7, 0);
	return (cpuid[1] >> 24) & 1;
}

static int x86_is_intel(void)
{
	int cpuid[4];
	char vendor[13];
	__cpuidex(cpuid, 0, 0);
	memcpy(vendor, &cpuid[1], 4);
	memcpy(vendor + 4, &cpuid[3], 4);
	memcpy(vendor + 8, &cpuid[2], 4);
	vendor[12] = 0;
	return strcmp(vendor, "GenuineIntel") == 0;
}

static int exe_path(const char *exe, int max, char buf[], int *base_st)
{
	int i, len, last_slash, ret = 0;
	if (exe == 0 || max == 0) return -1;
	buf[0] = 0;
	len = strlen(exe);
	for (i = len - 1; i >= 0; --i)
		if (exe[i] == '/') break;
	last_slash = i;
	if (base_st) *base_st = last_slash + 1;
	if (exe[0] == '/') {
		if (max < last_slash + 2) return -1;
		strncpy_s(buf, max, exe, last_slash + 1);
		buf[last_slash + 1] = 0;
	} else if (last_slash >= 0) { // actually, can't be 0
		char *p;
		//int abs_len;
		p = getcwd(buf, max);
		if (p == 0) return -1;
		//abs_len = strlen(buf);
		//if (max < abs_len + 3 + last_slash) return -1;
		//buf[abs_len] = '/';
        strcat_s(buf, max, "/");
		//strncpy_s(buf + abs_len + 1, max - (abs_len + 1), exe, last_slash + 1);
        strncat_s(buf, max, exe, last_slash + 1);
		//buf[abs_len + last_slash + 2] = 0;
	} else {
		char *env, *p, *q, *tmp;
		int env_len, found = 0;
		struct stat st;
		env = getenv("PATH");
        assert(env != NULL);
		env_len = strlen(env);
		if ((tmp = (char*)malloc(env_len + len + 2)) == NULL) { fprintf( stderr, "ERROR: out of memory %s", __func__); exit(EXIT_FAILURE);}
		for (p = q = env;; ++p) {
			if (*p == ':' || *p == 0) {
				strncpy_s(tmp, env_len + len + 2, q, p - q);
				tmp[p - q] = '/';
				strcpy_s(tmp + (p - q + 1), env_len + len + 2 - (p - q + 1), exe);
				if (stat(tmp, &st) == 0 && (st.st_mode & S_IXUSR)) {
					found = 1;
					break;
				}
				if (*p == 0) break;
				q = p + 1;
			}
		}
		if (!found) {
			free(tmp);
			return -2; // shouldn't happen!
		}
		ret = exe_path(tmp, max, buf, 0);
		free(tmp);
	}
	return ret;
}

static const char *path_basename(const char *path)
{
	const char *slash = strrchr(path, '/');
	return slash ? slash + 1 : path;
}

/* Only the release-root name does portable handoff (avoid recursing from gcc-full). */
static int is_root_dispatcher(const char *prefix)
{
	return strcmp(path_basename(prefix), "bwa-mem2") == 0;
}

/* Append simd suffix to prefix; return 1 if the path exists and is executable.
 * On success prefix holds the full path; on failure prefix is restored. */
static int try_binary(char *prefix, const char *simd)
{
	struct stat st;
	int prefix_len = strlen(prefix);
	strcat_s(prefix, PATH_MAX, simd);
	if (stat(prefix, &st) == 0 && (st.st_mode & S_IXUSR))
		return 1;
	prefix[prefix_len] = 0;
	return 0;
}

/* Build <dir>/extra/<name> next to root dispatcher; return 1 if executable. */
static int try_extra_binary(const char *prefix, const char *name, char *out)
{
	struct stat st;
	const char *base = path_basename(prefix);
	size_t dir_len = (size_t)(base - prefix);

	if (dir_len + strlen("extra/") + strlen(name) + 1 > PATH_MAX)
		return 0;
	memcpy(out, prefix, dir_len);
	out[dir_len] = 0;
	strcat_s(out, PATH_MAX, "extra/");
	strcat_s(out, PATH_MAX, name);
	if (stat(out, &st) == 0 && (st.st_mode & S_IXUSR))
		return 1;
	return 0;
}

/* Official binaries use the Intel compiler runtime. Keep AMD on the GCC path,
 * and avoid Intel -xCORE-AVX512 when AVX512BW is present without CLWB. */
static const char *portable_handoff_reason(int simd, int is_intel, int has_clwb)
{
	if (!is_intel) return "non-Intel CPU";
	if ((simd & SIMD_AVX512BW) && !has_clwb) return "AVX512 without CLWB";
	return NULL;
}

/* Prefer highest ISA available on this CPU that also has a shipped binary.
 * allow_avx512 is false only for root Intel siblings when the CPU lacks CLWB. */
static const char *select_simd_suffix(int simd, int allow_avx512, char *prefix)
{
	if ((simd & SIMD_AVX512BW) && allow_avx512 && try_binary(prefix, ".avx512bw"))
		return "avx512bw";
	if ((simd & SIMD_AVX2) && try_binary(prefix, ".avx2")) return "avx2";
	if ((simd & SIMD_AVX) && try_binary(prefix, ".avx")) return "avx";
	if ((simd & SIMD_SSE4_2) && try_binary(prefix, ".sse42")) return "sse42";
	if ((simd & SIMD_SSE4_1) && try_binary(prefix, ".sse41")) return "sse41";
	return NULL;
}

/* Prefer full GCC multi; fall back to single no-CLWB AVX-512 pin. */
static const char *select_portable_handoff(const char *prefix, char *out)
{
	if (try_extra_binary(prefix, "bwa-mem2.gcc-full", out))
		return "gcc-full";
	if (try_extra_binary(prefix, "bwa-mem2.intel-avx512-noclwb", out))
		return "intel-avx512-noclwb";
	return NULL;
}

static void print_cpu_simd(int simd)
{
	int first = 1;
	fputs("cpu_simd:", stdout);
#define EMIT(flag, name) do { \
	if (simd & (flag)) { \
		fputs(first ? " " : ",", stdout); \
		fputs(name, stdout); \
		first = 0; \
	} \
} while (0)
	EMIT(SIMD_AVX512BW, "avx512bw");
	EMIT(SIMD_AVX2, "avx2");
	EMIT(SIMD_AVX, "avx");
	EMIT(SIMD_SSE4_2, "sse4_2");
	EMIT(SIMD_SSE4_1, "sse4_1");
#undef EMIT
	if (first) fputs(" none", stdout);
	fputc('\n', stdout);
}

/* execv replaces the process; set argv[0] so nested dispatchers resolve siblings. */
static void launch_path(char *argv[], char *path)
{
	fprintf(stderr, "Launching executable \"%s\"\n", path);
	argv[0] = path;
	execv(path, argv);
	fprintf(stderr, "ERROR: execv failed for %s\n", path);
}

static void test_and_launch(char *argv[], char *prefix, const char *simd)
{
	struct stat st;
	int prefix_len = strlen(prefix);
	strcat_s(prefix, PATH_MAX, simd);
	fprintf(stderr, "Looking to launch executable \"%s\", simd = %s\n", prefix, simd);
	if (stat(prefix, &st) == 0)
	{
		if (st.st_mode & S_IXUSR) {
			launch_path(argv, prefix);
		}
		else
		{
			fprintf(stderr, "(st.st_mode & S_IXUSR) = %d, can not run executable: %s\n", st.st_mode & S_IXUSR, prefix);
		}
	}
	else
	{
		fprintf(stderr, "stat(prefix, &st) = %d, can not run executable: %s\n", stat(prefix, &st), prefix);
	}
	prefix[prefix_len] = 0;
}

int main(int argc, char *argv[])
{
	char buf[PATH_MAX], handoff[PATH_MAX], *prefix, *argv0 = argv[0];
	int ret, base_st, simd, is_intel, has_clwb, allow_avx512;
	const char *handoff_reason;
	int show_which = (argc >= 2 &&
		(strcmp(argv[1], "which") == 0 || strcmp(argv[1], "--which") == 0));

	ret = exe_path(argv0, PATH_MAX, buf, &base_st);
	if (ret != 0) {
		fprintf(stderr, "ERROR: prefix is too long!\n");
		return 1;
	}
	if ((prefix = (char*)malloc(PATH_MAX)) == NULL) {
		fprintf(stderr, "ERROR: out of memory.\n");
		return 1;
	}
	strcpy_s(prefix, PATH_MAX, buf);
	strcat_s(prefix, PATH_MAX, &argv0[base_st]);
	simd = x86_simd();
	is_intel = x86_is_intel();
	has_clwb = x86_has_clwb();
	/* Root Intel .avx512bw needs CLWB; GCC multi / other names do not. */
	allow_avx512 = has_clwb || !is_root_dispatcher(prefix);
	handoff_reason = portable_handoff_reason(simd, is_intel, has_clwb);

	/* Super-dispatcher: root bwa-mem2 keeps Intel-compatible hosts on the
	 * official siblings and hands AMD / no-CLWB AVX-512 hosts to GCC.
	 * Nested dispatchers skip this and only do ISA selection. */
	if (is_root_dispatcher(prefix) && handoff_reason != NULL) {
		const char *portable = select_portable_handoff(prefix, handoff);
		if (portable != NULL) {
			if (show_which) {
				printf("binary: %s\n", handoff);
				printf("platform: %s\n", portable);
				printf("dispatch: portable (%s)\n", handoff_reason);
				print_cpu_simd(simd);
				printf("cpu_vendor: %s\n", is_intel ? "intel" : "non-intel");
				printf("cpu_clwb: %s\n", has_clwb ? "yes" : "no");
				free(prefix);
				return 0;
			}
			fprintf(stderr, "%s: handing off to portable build \"%s\"\n",
				handoff_reason, handoff);
			launch_path(argv, handoff);
			/* exec failed — never run Intel-only siblings on a non-Intel CPU. */
			if (!is_intel) {
				free(prefix);
				return 2;
			}
			/* Intel without CLWB can still fall through to an official AVX2 sibling. */
		} else {
			fprintf(stderr, "WARNING: %s and no extra/ portable build\n",
				handoff_reason);
			if (!is_intel) {
				free(prefix);
				return 2;
			}
		}
	}

	if (show_which) {
		const char *platform = select_simd_suffix(simd, allow_avx512, prefix);
		if (platform == NULL) {
			fprintf(stderr, "ERROR: fail to find the right executable\n");
			free(prefix);
			return 2;
		}
		printf("binary: %s\n", prefix);
		printf("platform: %s\n", platform);
		print_cpu_simd(simd);
		printf("cpu_vendor: %s\n", is_intel ? "intel" : "non-intel");
		printf("cpu_clwb: %s\n", has_clwb ? "yes" : "no");
		free(prefix);
		return 0;
	}

	if ((simd & SIMD_AVX512BW) && allow_avx512) test_and_launch(argv, prefix, ".avx512bw");
	if (simd & SIMD_AVX2) test_and_launch(argv, prefix, ".avx2");
	if (simd & SIMD_AVX) test_and_launch(argv, prefix, ".avx");
	if (simd & SIMD_SSE4_2) test_and_launch(argv, prefix, ".sse42");
	if (simd & SIMD_SSE4_1) test_and_launch(argv, prefix, ".sse41");
	free(prefix);
	fprintf(stderr, "ERROR: fail to find the right executable\n");
	return 2;
}
