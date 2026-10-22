// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

#ifndef _CPUID_H
#define _CPUID_H

#ifdef __cplusplus
extern "C" {
#endif

#include "gwcommon.h"

void guessCpuType (void);
void guessCpuSpeed (void);

int isHighResTimerAvailable (void);
double getHighResTimer (void);
double getHighResTimerFrequency (void);

extern char CPU_BRAND[49];		
extern double CPU_SPEED;		
#define CPU_RDTSC		0x0001	
#define CPU_TSC_INVARIANT	0x0002	
#define CPU_CMOV		0x0004	
#define CPU_PREFETCH		0x0008	
#define CPU_TLB_PRIMING		0x0010	
#define CPU_MMX			0x0020	
#define CPU_3DNOW		0x0040	
#define CPU_3DNOW_PREFETCH	0x0080	
#define CPU_SSE			0x0100	
#define CPU_SSE2		0x0200	
#define CPU_SSE3		0x0400	
#define CPU_SSSE3		0x0800	
#define CPU_SSE41		0x1000	
#define CPU_SSE42		0x2000	
#define CPU_AVX			0x4000	
#define CPU_FMA3		0x8000	
#define CPU_FMA4		0x10000	
#define CPU_AVX2		0x20000	
#define CPU_PREFETCHW		0x40000	
#define CPU_PREFETCHWT1		0x80000	
#define CPU_AVX512F		0x100000
#define CPU_AVX512PF		0x200000
extern unsigned int CPU_FLAGS;		
extern unsigned int CPU_CORES;		
extern unsigned int CPU_HYPERTHREADS;	
					
extern int CPU_L1_CACHE_SIZE;		
extern int CPU_L2_CACHE_SIZE;		
extern int CPU_L3_CACHE_SIZE;		
extern int CPU_L1_CACHE_LINE_SIZE;
extern int CPU_L2_CACHE_LINE_SIZE;
extern int CPU_L3_CACHE_LINE_SIZE;
extern int CPU_L1_DATA_TLBS;
extern int CPU_L2_DATA_TLBS;
extern int CPU_L3_DATA_TLBS;
extern int CPU_L1_SET_ASSOCIATIVE;
extern int CPU_L2_SET_ASSOCIATIVE;
extern int CPU_L3_SET_ASSOCIATIVE;

extern unsigned int CPU_SIGNATURE;	

#define CPU_ARCHITECTURE_PRE_SSE2	0
#define CPU_ARCHITECTURE_PENTIUM_4	1
#define CPU_ARCHITECTURE_PENTIUM_M	2
#define CPU_ARCHITECTURE_CORE		3		
#define CPU_ARCHITECTURE_CORE_2		4
#define CPU_ARCHITECTURE_CORE_I7	5		
#define CPU_ARCHITECTURE_ATOM		6
#define CPU_ARCHITECTURE_PHI		7		
#define CPU_ARCHITECTURE_INTEL_OTHER	99
#define CPU_ARCHITECTURE_AMD_K8		100
#define CPU_ARCHITECTURE_AMD_K10	101
#define CPU_ARCHITECTURE_AMD_BULLDOZER	102
#define CPU_ARCHITECTURE_AMD_ZEN	103
#define CPU_ARCHITECTURE_AMD_OTHER	199
#define CPU_ARCHITECTURE_OTHER		999
extern int CPU_ARCHITECTURE;		

struct cpuid_data {
	uint32_t EAX;		
	uint32_t EBX;
	uint32_t ECX;
	uint32_t EDX;
};

unsigned long ecpuidsupport (void);
void ecpuid (struct cpuid_data *);

#define isCpuidSupported()	ecpuidsupport ()
#define Cpuid(a,s)		{ (s)->EAX=a; ecpuid (s); }

void exgetbv (struct cpuid_data *);
#define Xgetbv(a,s)		{ (s)->ECX=a; exgetbv (s); }

void erdtsc (uint32_t *hi, uint32_t *lo);
#define rdtsc(hi,lo)		erdtsc(hi,lo)

void fpu_init (void);

void one_hundred_thousand_clocks ();
void one_million_clocks ();

#ifdef __cplusplus
}
#endif

#endif
