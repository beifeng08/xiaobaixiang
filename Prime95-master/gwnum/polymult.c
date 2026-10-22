// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include "cpuid.h"
#include "gwnum.h"
#include "gwutil.h"
#include "polymult.h"
#if defined (SSE2) || defined (AVX) || defined (FMA) || defined (AVX512)
#include <immintrin.h>
#endif

#define HELPER_PREPROCESS_LINE		1
#define HELPER_PRE_FFT			2
#define HELPER_POLYMULT_LINE		3
#define HELPER_POST_UNFFT		4
#define HELPER_EXIT			5

void pick_pass_sizes (pmhandle *pmdata, uint64_t fftsize, uint32_t *p1size, uint32_t *p2size);
unsigned long cache_line_offset (gwhandle *gwdata, unsigned long i);
void generate_sincos (pmhandle *pmdata, uint64_t);
void zpad7_fft (gwhandle *gwdata, gwnum g);
uint64_t compress_line (char *buf, uint64_t size, int num_compressed_blks);
void decompress_line_slice (char *inbuf, char *outbuf, 	preprocessed_poly_header *hdr, uint64_t slice_start, uint64_t slice_end, int cvdt_size);
void polymult_pre_fft (pmhandle *pmdata, gwhandle *);
void polymult_post_unfft (pmhandle *pmdata, gwhandle *);
void polymult_line_preprocess_dbl (pmhandle *pmdata);
void polymult_line_preprocess_sse2 (pmhandle *pmdata);
void polymult_line_preprocess_avx (pmhandle *pmdata);
void polymult_line_preprocess_fma (pmhandle *pmdata);
void polymult_line_preprocess_avx512 (pmhandle *pmdata);
void polymult_line_dbl (pmhandle *pmdata);
void polymult_line_sse2 (pmhandle *pmdata);
void polymult_line_avx (pmhandle *pmdata);
void polymult_line_fma (pmhandle *pmdata);
void polymult_line_avx512 (pmhandle *pmdata);

typedef struct {
	bool	strip_monic_from_invec1;
	int	impl;			
	uint64_t fft_size;		
	uint64_t adjusted_invec1_size;	
	uint64_t adjusted_invec2_size;
	uint64_t adjusted_outvec_size;
	uint64_t true_invec1_size;	
	uint64_t true_invec2_size;
	uint64_t true_outvec_size;
	uint64_t max_element_size;	
	bool	combine_two_lines;	
	preprocessed_poly_header *hdr;  
} preprocess_plan;

typedef struct {
	bool	mt_polymult_line;	
	uint64_t alloc_invec1_size;	
	uint64_t alloc_invec2_size;	
	uint64_t alloc_outvec_size;	
	uint64_t alloc_tmpvec_size;	
	float	unnorms1;		
#ifdef GDEBUG
	uint64_t hash;
#endif
	struct planpart {
		bool	emulate_circular;	
		bool	strip_monic_from_invec1;
		bool	strip_monic_from_invec2;
		bool	post_monic_addin_invec1;
		bool	post_monic_addin_invec2;
		bool	streamed_stores;	
		int	impl;			
		uint64_t fft_size;		
		uint64_t adjusted_invec1_size;	
		uint64_t adjusted_invec2_size;
		uint64_t adjusted_outvec_size;
		uint64_t true_invec1_size;	
		uint64_t true_invec2_size;
		uint64_t true_outvec_size;
		uint64_t circular_size;		
		uint64_t LSWs_skipped;		
		uint64_t MSWs_skipped;		
		int64_t addin[4];		
		int64_t subout;			
		int	adjusted_shift;		
		int	adjusted_pad;		
		float	unnorms2;		
	} planpart[1];
} polymult_plan;

#define POLYMULT_IMPL_BRUTE		0		
#define POLYMULT_IMPL_KARATSUBA		1		
#define POLYMULT_IMPL_FFT		2		

#define PLAN_OPTIONS	(POLYMULT_SAVE_PLAN | POLYMULT_USE_PLAN)					
#define INVEC1_OPTIONS	(POLYMULT_INVEC1_MONIC | POLYMULT_INVEC1_RLP | POLYMULT_INVEC1_NEGATE)		
#define INVEC2_OPTIONS	(POLYMULT_INVEC2_MONIC | POLYMULT_INVEC2_RLP | POLYMULT_INVEC2_NEGATE)		
#define OUTVEC_OPTIONS	(~(PLAN_OPTIONS | INVEC1_OPTIONS | INVEC2_OPTIONS))				

#define vector_size_in_doubles(f)		(((f) & CPU_AVX512F) ? 8 : ((f) & CPU_AVX) ? 4 :((f) & CPU_SSE2) ? 2 : 1)
#define vector_size_in_bytes(f)			(((f) & CPU_AVX512F) ? 8*8 : ((f) & CPU_AVX) ? 4*8 :((f) & CPU_SSE2) ? 2*8 : 1*8)
#define complex_vector_size_in_doubles(f)	(((f) & CPU_AVX512F) ? 8*2 : ((f) & CPU_AVX) ? 4*2 :((f) & CPU_SSE2) ? 2*2 : 1*2)
#define complex_vector_size_in_bytes(f)		(((f) & CPU_AVX512F) ? 8*2*8 : ((f) & CPU_AVX) ? 4*2*8 :((f) & CPU_SSE2) ? 2*2*8 : 1*2*8)

#if defined (AVX512)
#define VLEN			512			
#define VDT			__m512d			
#define VFMA			1			
#define	prep_line		prep_line_avx512	
#define	read_line		read_line_avx512
#define	read_line_slice		read_line_slice_avx512
#define	read_preprocess_line	read_preprocess_line_avx512
#define	read_preprocess_line_slice read_preprocess_line_slice_avx512
#define	write_line		write_line_avx512
#define	write_line_slice	write_line_slice_avx512
#define	write_line_slice_strided write_line_slice_strided_avx512
#define	brute_line		brute_line_avx512
#define	karatsuba_line		karatsuba_line_avx512
#define	fft_line		fft_line_avx512
#define	fft_line_pass		fft_line_pass_avx512
#define	monic_line_add_hi	monic_line_add_hi_avx512
#define	monic_line_add_lo	monic_line_add_lo_avx512
#define	monic_line_adjustments	monic_line_adjustments_avx512
#define	polymult_line_preprocess polymult_line_preprocess_avx512
#define	polymult_line		polymult_line_avx512
#define broadcastsd		_mm512_set1_pd
#define addpd			_mm512_add_pd
#define subpd			_mm512_sub_pd
#define mulpd			_mm512_mul_pd
#define divpd			_mm512_div_pd
#define fmaddpd			_mm512_fmadd_pd
#define fmsubpd			_mm512_fmsub_pd
#define fnmaddpd		_mm512_fnmadd_pd
#define streampd(a,b)		_mm512_stream_pd((double*)(a),b)
#elif defined (FMA)
#define VLEN			256			
#define VDT			__m256d			
#define VFMA			1			
#define	prep_line		prep_line_avx		
#define	read_line		read_line_avx
#define	read_line_slice		read_line_slice_avx
#define	read_preprocess_line	read_preprocess_line_avx
#define	read_preprocess_line_slice read_preprocess_line_slice_avx
#define	write_line		write_line_avx
#define	write_line_slice	write_line_slice_avx
#define	write_line_slice_strided write_line_slice_strided_avx
#define	brute_line		brute_line_fma
#define	karatsuba_line		karatsuba_line_fma
#define	fft_line		fft_line_fma
#define	fft_line_pass		fft_line_pass_fma
#define	monic_line_add_hi	monic_line_add_hi_fma
#define	monic_line_add_lo	monic_line_add_lo_fma
#define	monic_line_adjustments	monic_line_adjustments_fma
#define	polymult_line_preprocess polymult_line_preprocess_fma
#define	polymult_line		polymult_line_fma
#define broadcastsd		_mm256_set1_pd
#define addpd			_mm256_add_pd
#define subpd			_mm256_sub_pd
#define mulpd			_mm256_mul_pd
#define divpd			_mm256_div_pd
#define fmaddpd			_mm256_fmadd_pd
#define fmsubpd			_mm256_fmsub_pd
#define fnmaddpd		_mm256_fnmadd_pd
#define streampd(a,b)		_mm256_stream_pd((double*)(a),b)
#elif defined (AVX)
#define VLEN			256			
#define VDT			__m256d			
#define VFMA			0			
#define	prep_line		prep_line_avx		
#define	read_line		read_line_avx
#define	read_line_slice		read_line_slice_avx
#define	read_preprocess_line	read_preprocess_line_avx
#define	read_preprocess_slice_line read_preprocess_line_slice_avx
#define	write_line		write_line_avx
#define	write_line_slice	write_line_slice_avx
#define	write_line_slice_strided write_line_slice_strided_avx
#define	brute_line		brute_line_avx
#define	karatsuba_line		karatsuba_line_avx
#define	fft_line		fft_line_avx
#define	fft_line_pass		fft_line_pass_avx
#define	monic_line_add_hi	monic_line_add_hi_avx
#define	monic_line_add_lo	monic_line_add_lo_avx
#define	monic_line_adjustments	monic_line_adjustments_avx
#define	polymult_line_preprocess polymult_line_preprocess_avx
#define	polymult_line		polymult_line_avx
#define broadcastsd		_mm256_set1_pd
#define addpd			_mm256_add_pd
#define subpd			_mm256_sub_pd
#define mulpd			_mm256_mul_pd
#define divpd			_mm256_div_pd
#define streampd(a,b)		_mm256_stream_pd((double*)(a),b)
#elif defined (SSE2)
#define VLEN			128			
#define VDT			__m128d			
#define VFMA			0			
#define	prep_line		prep_line_sse2		
#define	read_line		read_line_sse2
#define	read_line_slice		read_line_slice_sse2
#define	read_preprocess_line	read_preprocess_line_sse2
#define	read_preprocess_line_slice read_preprocess_line_slice_sse2
#define	write_line		write_line_sse2
#define	write_line_slice	write_line_slice_sse2
#define	write_line_slice_strided write_line_slice_strided_sse2
#define	brute_line		brute_line_sse2
#define	karatsuba_line		karatsuba_line_sse2
#define	fft_line		fft_line_sse2
#define	fft_line_pass		fft_line_pass_sse2
#define	monic_line_add_hi	monic_line_add_hi_sse2
#define	monic_line_add_lo	monic_line_add_lo_sse2
#define	monic_line_adjustments	monic_line_adjustments_sse2
#define	polymult_line_preprocess polymult_line_preprocess_sse2
#define	polymult_line		polymult_line_sse2
#define broadcastsd		_mm_set1_pd
#define addpd			_mm_add_pd
#define subpd			_mm_sub_pd
#define mulpd			_mm_mul_pd
#define divpd			_mm_div_pd
#define streampd(a,b)		_mm_stream_pd((double*)(a),b)
#else
#define VLEN			64			
#define VDT			double			
#define VFMA			0			
#define	prep_line		prep_line_dbl		
#define	read_line		read_line_dbl
#define	read_line_slice		read_line_slice_dbl
#define	read_preprocess_line_slice read_preprocess_line_slice_dbl
#define	write_line		write_line_dbl
#define	write_line_slice	write_line_slice_dbl
#define	write_line_slice_strided write_line_slice_strided_dbl
#define	brute_line		brute_line_dbl
#define	karatsuba_line		karatsuba_line_dbl
#define	fft_line		fft_line_dbl
#define	fft_line_pass		fft_line_pass_dbl
#define	monic_line_add_hi	monic_line_add_hi_dbl
#define	monic_line_add_lo	monic_line_add_lo_dbl
#define	monic_line_adjustments	monic_line_adjustments_dbl
#define	polymult_line_preprocess polymult_line_preprocess_dbl
#define	polymult_line		polymult_line_dbl
#define broadcastsd		_mm64_set1_pd
#define addpd			_mm64_add_pd
#define subpd			_mm64_sub_pd
#define mulpd			_mm64_mul_pd
#define divpd			_mm64_div_pd
#define streampd		_mm64_stream_pd

#define __m64d			double
#define _mm64_set1_pd(a)	(a)
#define _mm64_add_pd(a,b)	((a)+(b))
#define _mm64_sub_pd(a,b)	((a)-(b))
#define _mm64_mul_pd(a,b)	((a)*(b))
#define _mm64_div_pd(a,b)	((a)/(b))
#define _mm64_stream_pd(a,b)	(*(double *)(a)=(b))
#endif

#if !VFMA
#define fmaddpd(a,b,c)	addpd(mulpd(a,b),c)
#define fmsubpd(a,b,c)	subpd(mulpd(a,b),c)
#define fnmaddpd(a,b,c)	subpd(c,mulpd(a,b))
#endif

typedef struct {
	VDT	real;
	VDT	imag;
} CVDT;
#define cvload(a,b,c)		(a).real = broadcastsd (b), (a).imag = broadcastsd (c)
#define cvrmul(a,const_d,c)	(a).real = mulpd (const_d, (c).real), (a).imag = mulpd (const_d, (c).imag)				
#define cvadd(a,b,c)		(a).real = addpd ((b).real, (c).real), (a).imag = addpd ((b).imag, (c).imag)				
#define cvsub(a,b,c)		(a).real = subpd ((b).real, (c).real), (a).imag = subpd ((b).imag, (c).imag)				
#define cviadd(a,b,c)		(a).real = subpd ((b).real, (c).imag), (a).imag = addpd ((b).imag, (c).real)				
#define cvisub(a,b,c)		(a).real = addpd ((b).real, (c).imag), (a).imag = subpd ((b).imag, (c).real)				
#define cvaddfm(a,b,const_d,c)	(a).real = fmaddpd (const_d, (c).real, (b).real), (a).imag = fmaddpd (const_d, (c).imag, (b).imag)	
#define cvsubfm(a,b,const_d,c)	(a).real = fnmaddpd (const_d, (c).real, (b).real), (a).imag = fnmaddpd (const_d, (c).imag, (b).imag)	
#define cviaddfm(a,b,const_d,c)	(a).real = fnmaddpd (const_d, (c).imag, (b).real), (a).imag = fmaddpd (const_d, (c).real, (b).imag)	
#define cvisubfm(a,b,const_d,c)	(a).real = fmaddpd (const_d, (c).imag, (b).real), (a).imag = fnmaddpd (const_d, (c).real, (b).imag)	
#define cvmul(a,b,c)		{ VDT __tmpproduct__ = mulpd ((b).imag, (c).imag); \
				  (a).imag = fmaddpd ((b).imag, (c).real, mulpd ((b).real, (c).imag)); \
				  (a).real = fmsubpd ((b).real, (c).real, __tmpproduct__); }						
#define cvconjmul(a,b,c)	(a).real = fmaddpd ((b).real, (c).real, mulpd ((b).imag, (c).imag)), \
				(a).imag = fmsubpd ((b).imag, (c).real, mulpd ((b).real, (c).imag))					

#define cvaddsub(a1,a2,b,c)		cvadd (a1, b, c), cvsub (a2, b, c)					
#define cvsubadd(a1,a2,b,c)		cvaddsub (a2, a1, b, c)							
#define cviaddsub(a1,a2,b,c)		cviadd (a1, b, c), cvisub (a2, b, c)					
#define cvisubadd(a1,a2,b,c)		cviaddsub (a2, a1, b, c)						
#if !VFMA
#define cvaddsubfm(a1,a2,b,const_d,c)	{ CVDT _dc; cvrmul (_dc, const_d, c); cvadd (a1, b, _dc); cvsub (a2, b, _dc); }		
#define cviaddsubfm(a1,a2,b,const_d,c)	{ CVDT _dc; cvrmul (_dc, const_d, c); cviadd (a1, b, _dc); cvisub (a2, b, _dc); }	
#else
#define cvaddsubfm(a1,a2,b,const_d,c)	cvaddfm (a1, b, const_d, c), cvsubfm (a2, b, const_d, c)		
#define cviaddsubfm(a1,a2,b,const_d,c)	cviaddfm (a1, b, const_d, c), cvisubfm (a2, b, const_d, c)		
#endif
#define cvsubaddfm(a1,a2,b,const_d,c)	cvaddsubfm (a2,a1,b,const_d,c)						
#define cvisubaddfm(a1,a2,b,const_d,c)	cviaddsubfm (a2,a1,b,const_d,c)						

typedef struct {
	VDT	cos_over_sin;
	VDT	sin;
} TVDT;
#define twidload(t,b)			(t).cos_over_sin = broadcastsd ((b)[0]), (t).sin = broadcastsd ((b)[1])
#define twidmul(a,b,t)			(a).real = mulpd (fmsubpd ((b).real, (t).cos_over_sin, (b).imag), (t).sin), \
					(a).imag = mulpd (fmaddpd ((b).imag, (t).cos_over_sin, (b).real), (t).sin)	
#define twidconjmul(a,b,t)		(a).real = mulpd (fmaddpd ((b).real, (t).cos_over_sin, (b).imag), (t).sin), \
					(a).imag = mulpd (fmsubpd ((b).imag, (t).cos_over_sin, (b).real), (t).sin)	
#define twidmulandconjmul(d1,d2,b,t)	{ CVDT bsin; bsin.real = mulpd ((b).real, (t).sin), bsin.imag = mulpd ((b).imag, (t).sin), \
					  (d1).real = fmsubpd (bsin.real, (t).cos_over_sin, bsin.imag), \
					  (d1).imag = fmaddpd (bsin.imag, (t).cos_over_sin, bsin.real), \
					  (d2).real = fmaddpd (bsin.real, (t).cos_over_sin, bsin.imag), \
					  (d2).imag = fmsubpd (bsin.imag, (t).cos_over_sin, bsin.real); }		
#define twidmuldelay(a,b,t)		(a).real = fmsubpd ((b).real, (t).cos_over_sin, (b).imag), \
					(a).imag = fmaddpd ((b).imag, (t).cos_over_sin, (b).real)			
#define twidconjmuldelay(a,b,t)		(a).real = fmaddpd ((b).real, (t).cos_over_sin, (b).imag), \
					(a).imag = fmsubpd ((b).imag, (t).cos_over_sin, (b).real)			

#if !defined (SSE2) && !defined (AVX) && !defined (FMA) && !defined (AVX512)

float polymult_safety_margin (uint64_t invec1_size, uint64_t invec2_size) {
	
	uint64_t n = (invec1_size < invec2_size) ? invec1_size : invec2_size;
	
	uint64_t fft_size = (invec1_size > invec2_size) ? invec1_size : invec2_size;
	
	return ((float) (log2 ((double) n) / 2.0 + 0.45 * log2 ((double) fft_size) / log2 (1300000.0)) / 2.0f);
}

uint64_t polymult_fft_size (uint64_t n)
{

	int	pfas[] = {40, 45, 48, 50, 54, 60, 64, 72, 75};		

	if (n <= 32) for (int i = 4; ; i *= 2) if (i >= n) return (i);

	for (uint64_t two_multiplier = 1; ; two_multiplier *= 2) {
		if (n > two_multiplier * 75) continue;
		for (int i = 0; i < sizeof (pfas) / sizeof (int); i++) {
			uint64_t fft_size = two_multiplier * pfas[i];
			if (fft_size >= n && (fft_size % 4 == 0 || fft_size % 2 == 1)) return (fft_size);  
		}
	}
}

void pick_pass_sizes (
	pmhandle *pmdata,		
	uint64_t fftsize,		
	uint32_t *p1size,		
	uint32_t *p2size)		
{
	uint32_t powers_of_two;
	double	log_max_p2size, log_fftsize;

	if (fftsize <= pmdata->two_pass_start) { *p1size = 1; *p2size = (uint32_t) fftsize; return; }

	log_max_p2size = log2 ((double) pmdata->max_pass2_size);

	log_fftsize = log2 ((double) fftsize);
	*p2size = (uint32_t) log_fftsize / 2;
	if (*p2size > (uint32_t) log_max_p2size) *p2size = (uint32_t) log_max_p2size;

	for (powers_of_two = 0; (fftsize & (1ULL << powers_of_two)) == 0; powers_of_two++);
	if (*p2size > powers_of_two) *p2size = powers_of_two;
	if ((powers_of_two & 1) == 0) {		
		if ((*p2size & 1) == 1) *p2size -= 1;
	} else {
		if ((*p2size & 1) == 0) *p2size -= 1;
	}
	*p1size = (uint32_t) (fftsize >> *p2size);
	*p2size = 1 << *p2size;
}

uint64_t polymult_mem_required (
	pmhandle *pmdata,		
	uint64_t invec1_size,		
	uint64_t invec2_size,		
	int	options)		
{
	uint64_t adjusted_invec1_size, adjusted_invec2_size, adjusted_outvec_size, fft_size, memory;
	uint32_t pass1_size, pass2_size;
	bool	post_process_monics;

	ASSERTG (pmdata->num_threads >= 1);

	int complex_vector_size = complex_vector_size_in_doubles (pmdata->gwdata->cpu_flags);

	adjusted_invec1_size = (options & POLYMULT_INVEC1_RLP) ? 2 * invec1_size - 1 : invec1_size;
	adjusted_invec2_size = (options & POLYMULT_INVEC2_RLP) ? 2 * invec2_size - 1 : invec2_size;
	adjusted_outvec_size = adjusted_invec1_size + adjusted_invec2_size - 1;
	if (options & POLYMULT_CIRCULAR) {
		if (options & POLYMULT_INVEC1_MONIC) adjusted_invec1_size += (options & POLYMULT_INVEC1_RLP) ? 2 : 1;
		if (options & POLYMULT_INVEC2_MONIC) adjusted_invec2_size += (options & POLYMULT_INVEC2_RLP) ? 2 : 1;
		
		adjusted_outvec_size = (adjusted_invec1_size > adjusted_invec2_size ? adjusted_invec1_size : adjusted_invec2_size);
		post_process_monics = FALSE;
	} else
		post_process_monics = TRUE;

	fft_size = polymult_fft_size (adjusted_outvec_size);
	pick_pass_sizes (pmdata, fft_size, &pass1_size, &pass2_size);
	memory = fft_size * complex_vector_size;		
	memory += (fft_size + 2) * complex_vector_size;		
	if (post_process_monics && (options & (POLYMULT_INVEC2_MONIC | POLYMULT_INVEC1_MONIC))) memory += fft_size * complex_vector_size;

	if (pass1_size > 1)
		memory += (uint64_t) pass1_size * complex_vector_size * (fft_size >= pmdata->mt_ffts_start && fft_size < pmdata->mt_ffts_end ? pmdata->num_threads : 1);
	
	memory *= (fft_size < pmdata->mt_ffts_start || fft_size >= pmdata->mt_ffts_end) ? pmdata->num_threads : 1;

	return (memory + (fft_size % 3 == 0 ? fft_size * 2 / 3 : fft_size) * sizeof (double));
}

void polymult_init (
	pmhandle *pmdata,		
	gwhandle *gwdata)		
{
	memset (pmdata, 0, sizeof (pmhandle));
	pmdata->gwdata = gwdata;

	pmdata->max_num_threads = gwget_num_threads (gwdata);
	pmdata->num_threads = pmdata->max_num_threads;

	int complex_vector_size = complex_vector_size_in_doubles (gwdata->cpu_flags);

	int num_doubles = gwfftlen (pmdata->gwdata);					
	if (pmdata->gwdata->GENERAL_MMGW_MOD) num_doubles *= 2;				
	if (!pmdata->gwdata->NEGACYCLIC_FFT) num_doubles += 2;				
	if (pmdata->gwdata->ZERO_PADDED_FFT) num_doubles += 14;				
	pmdata->num_lines = divide_rounding_up (num_doubles, complex_vector_size);	

	if (pmdata->gwdata->cpu_flags & CPU_AVX) {
		pmdata->KARAT_BREAK = 32;			
		pmdata->FFT_BREAK = 64;				
	}
	else {
		pmdata->KARAT_BREAK = 32;			
		pmdata->FFT_BREAK = 64;				
	}

	pmdata->cached_twiddles_enabled = TRUE;
	pmdata->twiddle_cache_additions_disabled = FALSE;

	polymult_default_tuning (pmdata, 256, 6144);

	if (!pmdata->gwdata->information_only) {
		if (pmdata->gwdata->GENERAL_MMGW_MOD || pmdata->gwdata->k != 1.0) gwuser_init_FFT1 (pmdata->gwdata);
		if (pmdata->gwdata->ZERO_PADDED_FFT) zpad7_fft (pmdata->gwdata, pmdata->gwdata->GW_FFT1);
	}
}

void polymult_default_tuning (
	pmhandle *pmdata,		
	uint32_t L2_CACHE_SIZE,		
	uint32_t L3_CACHE_SIZE)		
{
	
	int complex_vector_size = complex_vector_size_in_doubles (pmdata->gwdata->cpu_flags);

	int lines_we_could_process = divide_rounding_up (pmdata->num_lines, pmdata->num_threads) * pmdata->num_threads;
	double mt_line_efficiency = (double) pmdata->num_lines / (double) lines_we_could_process;

	pmdata->two_pass_start = L2_CACHE_SIZE * 4 * (1024 / complex_vector_size) / 2;

	pmdata->max_pass2_size = L2_CACHE_SIZE * (1024 / complex_vector_size) / 2;

	pmdata->mt_ffts_start = (uint64_t) ((double) L3_CACHE_SIZE * 1024.0 / complex_vector_size / pmdata->num_threads * mt_line_efficiency * 0.8);

	pmdata->mt_ffts_end = (mt_line_efficiency > 0.98 ? (uint64_t) L3_CACHE_SIZE * 1024 / complex_vector_size : 0xFFFFFFFFFFFFFFFFULL);

	pmdata->strided_writes_end = (uint64_t) L3_CACHE_SIZE * 1024 / complex_vector_size;

	pmdata->streamed_stores_start = L3_CACHE_SIZE * (1024 / complex_vector_size) / pmdata->num_lines;
}

void polymult_done (
	pmhandle *pmdata)		
{
	if (pmdata->thread_ids != NULL) {
		pmdata->helper_opcode = HELPER_EXIT;
		gwevent_signal (&pmdata->work_to_do);
		atomic_set (pmdata->alt_work_to_do, 1);
		for (int i = 1; i < pmdata->max_num_threads; i++) gwthread_wait_for_exit (&pmdata->thread_ids[i]);
		free (pmdata->thread_ids);
		pmdata->thread_ids = NULL;
		gwmutex_destroy (&pmdata->poly_mutex);
		gwevent_destroy (&pmdata->work_to_do);
		gwevent_destroy (&pmdata->all_helpers_done);
	}
	if (pmdata->cached_twiddles_enabled) {
		for (int i = 0; i < pmdata->cached_twiddles_count; i++) {
			aligned_free (pmdata->cached_twiddles[i].twiddles1);
			aligned_free (pmdata->cached_twiddles[i].twiddles2);
		}
		pmdata->cached_twiddles_count = 0;
	}
	if (pmdata->twiddles_initialized && !pmdata->twiddles_are_from_cache) {
		aligned_free (pmdata->twiddles1);
		aligned_free (pmdata->twiddles2);
	}
	pmdata->twiddles_initialized = 0;
	pmdata->twiddles1 = NULL;
	pmdata->twiddles2 = NULL;
}

unsigned long cache_line_offset (
	gwhandle *gwdata,	
	unsigned long i)
{
	unsigned long addr, grps;

	if (gwdata->cpu_flags & CPU_AVX512F) {
		if (gwdata->PASS1_SIZE == 0) {
			addr = i * 64;
			
			if (gwdata->FOURKBGAPSIZE) addr = addr + (addr / (gwdata->FOURKBGAPSIZE << 6)) * 64;
		}
		else {
			grps = i / (gwdata->PASS2_SIZE / 4);
			addr = i * 64;
			
			addr = addr + (addr >> 12) * gwdata->FOURKBGAPSIZE + grps * gwdata->PASS2GAPSIZE;
		}
	}

	else if (gwdata->cpu_flags & CPU_AVX) {
		if (gwdata->PASS2_SIZE == 0) {
			addr = i * 64;
			
			if (gwdata->FOURKBGAPSIZE) addr = addr + (addr / (gwdata->FOURKBGAPSIZE << 6)) * 64;
		}
		else {
			grps = i / (gwdata->PASS2_SIZE / 4);
			addr = i * 64;
			
			addr = addr + (addr >> 12) * gwdata->FOURKBGAPSIZE + grps * gwdata->PASS2GAPSIZE;
		}
	}

	else if (gwdata->cpu_flags & CPU_SSE2) {

		if (gwdata->PASS2_SIZE == 0) {
			addr = i * 64;
		}

		else if (gwdata->FFT_TYPE == FFT_TYPE_HOME_GROWN) {
			grps = i / (gwdata->PASS2_SIZE / 2);
			addr = i * 64;
			
			addr = addr + (addr >> 13) * 128 + grps * gwdata->PASS2GAPSIZE;
		}

		else {
			grps = i / (gwdata->PASS2_SIZE / 2);
			addr = i * 64;
			
			addr = addr + (addr >> 13) * 128 + grps * gwdata->PASS2GAPSIZE;
		}
	}

	else if (gwdata->PASS2_SIZE == 0) {
		addr = i * 64;
	}
	else {
		grps = (i * 8) / gwdata->PASS2_SIZE;
		addr = i * 64 + grps * 64;
	}

	return (addr);
}

const long double w1c = 0.8854560256532098959003755220151;	
const long double w1s = 0.4647231720437685456560153351331;	
const long double w2c = 0.56806474673115580251180755912752;	
const long double w2s = 0.82298386589365639457961742343938;	
const long double w3c = 0.12053668025532305334906768745254;	
const long double w3s = 0.99270887409805399280075164949252;	
const long double w4c = -0.35460488704253562596963789260002;	
const long double w4s = 0.93501624268541482343978459983783;	
const long double w5c = -0.74851074817110109863463059970135;	
const long double w5s = 0.66312265824079520237678549266677;	
const long double w6c = -0.97094181742605202715698227629379;	
const long double w6s = 0.23931566428755776714875372626021;	
const long double w7c = -0.97094181742605202715698227629379;	
const long double w7s = -0.23931566428755776714875372626021;	
const long double w8c = -0.74851074817110109863463059970135;	
const long double w8s = -0.66312265824079520237678549266677;	
const long double w9c = -0.35460488704253562596963789260002;	
const long double w9s = -0.93501624268541482343978459983783;	
const long double w10c = 0.12053668025532305334906768745254;	
const long double w10s = -0.99270887409805399280075164949252;	
const long double w11c = 0.56806474673115580251180755912752;	
const long double w11s = -0.82298386589365639457961742343938;	
const long double w12c = 0.8854560256532098959003755220151;	
const long double w12s = -0.4647231720437685456560153351331;	

void zpad7_fft (
	gwhandle *gwdata,		
	gwnum	g)			
{
	double r1, r2, r3, r4, r5, r6, r7;		

	r1 = g[-11];		
	r2 = g[-10];		
	r3 = g[-9];		
	r4 = g[-8];		
	r5 = g[-7];		
	r6 = g[-6];		
	r7 = g[-5];		

	g[-12] = r1 + r2 + r3 + r4 + r5 + r6 + r7;
	g[-13] = r1 + w1c*r2 +  w2c*r3 +  w3c*r4 +  w4c*r5 +  w5c*r6 +  w6c*r7;
	g[-14] = r1 + w2c*r2 +  w4c*r3 +  w6c*r4 +  w8c*r5 + w10c*r6 + w12c*r7;
	g[-15] = r1 + w3c*r2 +  w6c*r3 +  w9c*r4 + w12c*r5 +  w2c*r6 +  w5c*r7;
	g[-16] = r1 + w4c*r2 +  w8c*r3 + w12c*r4 +  w3c*r5 +  w7c*r6 + w11c*r7;
	g[-17] = r1 + w5c*r2 + w10c*r3 +  w2c*r4 +  w7c*r5 + w12c*r6 +  w4c*r7;
	g[-18] = r1 + w6c*r2 + w12c*r3 +  w5c*r4 + w11c*r5 +  w4c*r6 + w10c*r7;
	g[-19] =      w1s*r2 +  w2s*r3 +  w3s*r4 +  w4s*r5 +  w5s*r6 +  w6s*r7;
	g[-20] =      w2s*r2 +  w4s*r3 +  w6s*r4 +  w8s*r5 + w10s*r6 + w12s*r7;
	g[-21] =      w3s*r2 +  w6s*r3 +  w9s*r4 + w12s*r5 +  w2s*r6 +  w5s*r7;
	g[-22] =      w4s*r2 +  w8s*r3 + w12s*r4 +  w3s*r5 +  w7s*r6 + w11s*r7;
	g[-23] =      w5s*r2 + w10s*r3 +  w2s*r4 +  w7s*r5 + w12s*r6 +  w4s*r7;
	g[-24] =      w6s*r2 + w12s*r3 +  w5s*r4 + w11s*r5 +  w4s*r6 + w10s*r7;
}

void zpad7_unfft (
	gwhandle *gwdata,		
	gwnum	g)			
{
const	double INV13 = 0.07692307692307692307692307692308;	
	double r1, r2, r3, r4, r5, r6, r7;			
	double i2, i3, i4, i5, i6, i7;				
	double o[7];						

	r1 = g[-12];
	r2 = g[-13];
	r3 = g[-14];
	r4 = g[-15];
	r5 = g[-16];
	r6 = g[-17];
	r7 = g[-18];
	i2 = g[-19];
	i3 = g[-20];
	i4 = g[-21];
	i5 = g[-22];
	i6 = g[-23];
	i7 = g[-24];

	o[0] = r1 + 2.0 * ( w6c*r2 + w12c*r3 +  w5c*r4 + w11c*r5 +  w4c*r6 + w10c*r7 +  w6s*i2 + w12s*i3 +  w5s*i4 + w11s*i5 +  w4s*i6 + w10s*i7);
	o[1] = r1 + 2.0 * ( w7c*r2 +  w1c*r3 +  w8c*r4 +  w2c*r5 +  w9c*r6 +  w3c*r7 +  w7s*i2 +  w1s*i3 +  w8s*i4 +  w2s*i5 +  w9s*i6 +  w3s*i7);
	o[2] = r1 + 2.0 * ( w8c*r2 +  w3c*r3 + w11c*r4 +  w6c*r5 +  w1c*r6 +  w9c*r7 +  w8s*i2 +  w3s*i3 + w11s*i4 +  w6s*i5 +  w1s*i6 +  w9s*i7);
	o[3] = r1 + 2.0 * ( w9c*r2 +  w5c*r3 +  w1c*r4 + w10c*r5 +  w6c*r6 +  w2c*r7 +  w9s*i2 +  w5s*i3 +  w1s*i4 + w10s*i5 +  w6s*i6 +  w2s*i7);
	o[4] = r1 + 2.0 * (w10c*r2 +  w7c*r3 +  w4c*r4 +  w1c*r5 + w11c*r6 +  w8c*r7 + w10s*i2 +  w7s*i3 +  w4s*i4 +  w1s*i5 + w11s*i6 +  w8s*i7);
	o[5] = r1 + 2.0 * (w11c*r2 +  w9c*r3 +  w7c*r4 +  w5c*r5 +  w3c*r6 +  w1c*r7 + w11s*i2 +  w9s*i3 +  w7s*i4 +  w5s*i5 +  w3s*i6 +  w1s*i7);
	o[6] = r1 + 2.0 * (w12c*r2 + w11c*r3 + w10c*r4 +  w9c*r5 +  w8c*r6 +  w7c*r7 + w12s*i2 + w11s*i3 + w10s*i4 +  w9s*i5 +  w8s*i6 +  w7s*i7);

	g[-11] = o[0] * INV13;	
	g[-10] = o[1] * INV13;	
	g[-9]  = o[2] * INV13;	
	g[-8]  = o[3] * INV13;	
	g[-7]  = o[4] * INV13;	
	g[-6]  = o[5] * INV13;	
	g[-5]  = o[6] * INV13;	
}

void generate_sincos (
	pmhandle *pmdata,		
	uint64_t fft_size)
{
	const long double twopi = 2.0L * 3.1415926535897932384626433L;
#define gen_twid(t,a)	{long double c,s; c=cosl(a); s=sinl(a)+1e-80; (t)[0]=(double)(c/s); (t)[1]=(double)s; }

	if (pmdata->twiddles_initialized && pmdata->twiddles_initialized == fft_size) return;

	if (pmdata->num_threads > 1) gwmutex_lock (&pmdata->poly_mutex);

	if (pmdata->twiddles_initialized && pmdata->twiddles_initialized == fft_size) {
		if (pmdata->num_threads > 1) gwmutex_unlock (&pmdata->poly_mutex);
		return;
	}

	if (pmdata->twiddles_initialized && !pmdata->twiddles_are_from_cache) {
		aligned_free (pmdata->twiddles1);
		aligned_free (pmdata->twiddles2);
	}

	if (pmdata->cached_twiddles_enabled) {
		for (int i = 0; i < pmdata->cached_twiddles_count; i++) {
			if (fft_size == pmdata->cached_twiddles[i].size) {
				pmdata->twiddles1 = pmdata->cached_twiddles[i].twiddles1;
				pmdata->twiddles2 = pmdata->cached_twiddles[i].twiddles2;
				pmdata->twiddles_initialized = fft_size;
				pmdata->twiddles_are_from_cache = TRUE;
				if (pmdata->num_threads > 1) gwmutex_unlock (&pmdata->poly_mutex);
				return;
			}
		}
	}

	pmdata->twiddles_initialized = 0;
	pmdata->twiddles1 = NULL;
	pmdata->twiddles2 = NULL;

	uint64_t size = fft_size;
	if (size % 3 == 0) {
		long double twopi_over_size = twopi / (long double) size;
		pmdata->twiddles1 = (double *) aligned_malloc ((size_t) (size/3 * 2 * sizeof (double)), 64);
		for (unsigned int i = 0; i < size/3; i++) {
			long double angle = twopi_over_size * (long double) i;
			gen_twid (&pmdata->twiddles1[2*i], angle);
		}
		while (size % 3 == 0) size /= 3;
	}

	long double twopi_over_size = twopi / (long double) size;
	pmdata->twiddles2 = (double *) aligned_malloc ((size_t) (size/4 * 2 * 2 * sizeof (double)), 64);
	for (unsigned int i = 0; i < size/4; i++) {
		double long angle = twopi_over_size * (long double) i;
		gen_twid (&pmdata->twiddles2[2*(2*i)], angle);
		gen_twid (&pmdata->twiddles2[2*(2*i+1)], 2.0L*angle);
	}

	pmdata->twiddles_initialized = fft_size;
	pmdata->twiddles_are_from_cache = FALSE;

	if (pmdata->cached_twiddles_enabled && !pmdata->twiddle_cache_additions_disabled) {
		int i;
		if (pmdata->cached_twiddles_count < 40) i = pmdata->cached_twiddles_count++;
		else {
			i = fft_size % 37;		
			aligned_free (pmdata->cached_twiddles[i].twiddles1);
			aligned_free (pmdata->cached_twiddles[i].twiddles2);
		}
		pmdata->cached_twiddles[i].size = fft_size;
		pmdata->cached_twiddles[i].twiddles1 = pmdata->twiddles1;
		pmdata->cached_twiddles[i].twiddles2 = pmdata->twiddles2;
		pmdata->twiddles_are_from_cache = TRUE;
	}

	if (pmdata->num_threads > 1) gwmutex_unlock (&pmdata->poly_mutex);
}

uint64_t compress_line (		
	char	*buf,			
	uint64_t size,			
	int	num_compressed_blks)	
{
#define COUNT_BASE_EXPO		960		
#define COUNT_MAX_EXPO		1080		
	short	counts[COUNT_MAX_EXPO-COUNT_BASE_EXPO+1];
	int	max_expo;		
	int	peak;			
	bool	above_peak_flag;	

	memset (&counts, 0, sizeof (counts));
	max_expo = 0;
	for (int i = 0; i < size && i < 8000; i += 8) {
		uint64_t expo = * (uint64_t *) (buf + i);
		expo = (expo << 1) >> 53;		
		if (expo < COUNT_BASE_EXPO) continue;	
		ASSERTG (expo <= COUNT_MAX_EXPO);	
		expo -= COUNT_BASE_EXPO;		
		counts[expo]++;
		if ((int) expo > max_expo) max_expo = (int) expo;
	}

	peak = 0;
	for (int i = (max_expo > 18 ? max_expo - 18 : 0); i < max_expo; i++)
		if (peak == 0 || counts[i-2] + counts[i-1] + counts[i] > counts[peak-2] + counts[peak-1] + counts[peak]) peak = i;

	above_peak_flag = (peak < 5 || counts[peak+1] > counts[peak-5]);

	*buf = ((peak & 0x7F) << 1) + (above_peak_flag & 1);
	peak += COUNT_BASE_EXPO;		

	uint64_t max_compressed_blk_size = 0;
	uint64_t blk_size = size / num_compressed_blks;
	ASSERTG (size % num_compressed_blks == 0);
	for (int blk = 0; blk < num_compressed_blks; blk++) {

		int	outval = 0;		
		int	outlen = 0;		
		unsigned char mbuf[48];		
		int	mlen = 0;		
		int	last_m48len = 0;	
#define put_bits(v,len)	outval = (outval << (len)) + (int)(v), outlen += len
#define put_m48(m)	{ union {uint64_t a; char b[8];} x; x.a = m; memcpy (mbuf + mlen, x.b, 6); mlen += 6; last_m48len = 6; }
#define emptyQ()	{ if (outlen>=8) outlen-=8, *outptr++ = (unsigned char)(outval>>outlen); \
			  if (mlen > last_m48len) mlen-=last_m48len, memcpy(outptr,mbuf,mlen), outptr+=mlen, memcpy(mbuf,mbuf+mlen,last_m48len), mlen=last_m48len; \
			  while (outlen>=8) outlen-=8, *outptr++ = (unsigned char)(outval>>outlen); \
			  if (outlen == 0 && mlen) memcpy(outptr,mbuf,mlen), outptr+=mlen, mlen=0; \
			  last_m48len = 0; }
		unsigned char *outptr = (unsigned char *) buf + 1 + blk * blk_size;
		unsigned char *blk_start = outptr;

		for (uint64_t i = 0; i < blk_size; i += sizeof (double)) {
			uint64_t expo, sign, nibble, mantissa48;

			expo = * (uint64_t *) (buf + blk * blk_size + i);
			sign = expo >> 63;
			nibble = (expo << 12) >> 60;
			mantissa48 = expo & 0xFFFFFFFFFFFFULL;
			expo = (expo << 1) >> 53;	

			emptyQ ();

			if (expo < peak - 50) expo = 0;

			if (expo == peak) put_bits (0, 2);
			else if (expo == peak-1) put_bits (1, 2);
			else if (expo == peak-2) put_bits (2, 2);
			else if (expo == peak-3) put_bits (12, 4);
			else if (expo == peak-4) put_bits (13, 4);
			else if (above_peak_flag && expo == peak+1) put_bits (14, 4);
			else if (above_peak_flag && expo == peak-5) put_bits (30, 5);
			else if (!above_peak_flag && expo == peak-5) put_bits (14, 4);
			else if (!above_peak_flag && expo == peak-6) put_bits (30, 5);
			else {
				put_bits (31, 5);
				ASSERTG (expo == 0 || (expo >= peak-50 && expo <= peak-6) || (expo >= peak+1 && expo <= peak+18));
				if (expo > peak) expo = expo - (peak+1) + 1;		
				else if (expo > 0) expo = expo - (peak-50) + 19;	
				ASSERTG (expo <= 63);
				put_bits (expo, 6);
			}

			if (expo != 0) { put_bits (sign, 1); put_bits (nibble, 4); put_m48 (mantissa48); }
		}

		if (outlen & 7) put_bits (0, 8 - (outlen & 7));
		emptyQ ();

		if ((uint64_t) (outptr - blk_start) > max_compressed_blk_size) max_compressed_blk_size = outptr - blk_start;
	}
#undef put_bits
#undef put_m48
#undef emptyQ

	return (1 + num_compressed_blks * max_compressed_blk_size);
}

void decompress_line_slice (
	char	*inbuf,			
	char	*outbuf,		
	preprocessed_poly_header *hdr,  
	uint64_t slice_start,		
	uint64_t slice_end,		
	int	cvdt_size)		
{
	int	peak;			
	bool	above_peak_flag;	

	peak = ((unsigned char) *inbuf) >> 1;  peak += 960;
	above_peak_flag = *inbuf & 1;

	uint64_t doubles_to_skip = slice_start * (cvdt_size / sizeof (double));

	uint64_t total_size = (uint64_t) (slice_end - slice_start) * cvdt_size;
	uint64_t compressed_blk_size = (hdr->element_size - 1) / hdr->num_compressed_blks;
	uint64_t uncompressed_blk_size = (uint64_t) hdr->line_size * cvdt_size / hdr->num_compressed_blks;
	ASSERTG ((doubles_to_skip * sizeof (double)) % uncompressed_blk_size == 0);	
	for (uint64_t blk = doubles_to_skip / 512; total_size; blk++) {
		uint64_t size;		

		unsigned char *inptr = (unsigned char *) inbuf + 1 + blk * compressed_blk_size;
		unsigned int inval, inbits;	
		inval = inbits = 0;
#define get_bits(v,len)	{ if (inbits<len) inval=(inval<<8)+(*inptr++),inbits+=8; inbits-=len; v=(inval>>inbits)&((1<<(len))-1); }
#define get_m48(v)	{ union {uint64_t a; char b[8];} x; x.a = 0; memcpy (x.b, inptr, 6); inptr += 6; v = x.a; }

		size = uncompressed_blk_size;
		total_size -= size;

#ifdef STRAIGHTFORWARD_IMPL
		while (size) {
			uint64_t expo, sign, nibble, mantissa48;
			int	code;

			get_bits (code, 2);
			if (code <= 2) expo = peak - code;
			else {
				get_bits (code, 2);
				if (code <= 1) expo = peak - 3 - code;
				else if (code == 2) expo = above_peak_flag ? peak + 1 : peak - 5;
				else {
					get_bits (code, 1);
					if (code == 0) expo = above_peak_flag ? peak - 5 : peak - 6;
					else {
						get_bits (code, 6);
						if (code >= 19) expo = code - 19 + (peak-50);	
						else if (code >= 1) expo = code - 1 + (peak+1);	
						else expo = 0;
					}
				}
			}

			if (expo != 0) { get_bits (sign, 1); get_bits (nibble, 4); get_m48 (mantissa48); }
			else sign = nibble = mantissa48 = 0;

			* (uint64_t *) outbuf = (sign << 63) + (expo << 52) + (nibble << 48) + mantissa48;
			outbuf += sizeof (double);
			size -= sizeof (double);
		}

#else		

		for ( ; size; outbuf += sizeof (double), size -= sizeof (double)) {
			int	code, more_code, expo, sign, nibble;

			get_bits (code, 7);

			if (code < 0b1100000) expo = peak - (code >> 5);
			else if (code < 0b1111000) {
				if (code < 0b1110000) expo = peak - 3 - ((code & 0b0011000) >> 3);
				else expo = above_peak_flag ? peak + 1 : peak - 5;
				get_bits (more_code, 2);
				code = (code << 2) + more_code;
			} else if (code < 0b1111100) {
				expo = above_peak_flag ? peak - 5 : peak - 6;
				get_bits (more_code, 3);
				code = (code << 3) + more_code;
			} else {
				get_bits (more_code, 4);
				code = ((code & 0b0000011) << 4) + more_code;
				if (code >= 19) expo = code - 19 + (peak-50);	
				else if (code >= 1) expo = code - 1 + (peak+1);	
				else { memset (outbuf, 0, sizeof (double)); continue; }
				get_bits (code, 5);
			}

			sign = code >> 4;
			nibble = code & 0xF;

			memcpy (outbuf, inptr, 6), inptr += 6;

			uint16_t top2 = (uint16_t) ((sign << 15) + (expo << 4) + nibble);
			memcpy (outbuf+6, &top2, 2);
		}
#endif

#undef get_bits
#undef get_m48
	}
}

#endif

typedef struct {
	gwnum	*data;				
	uint64_t size;				
	CVDT	*vec;				
	int	options;			
	bool	post_process_monics;		
	gwnum	*fma_data;			
	uint64_t LSWs_skipped;			
	bool	streamed_stores;		
	gwatomic blknum;			
	uint64_t slice_size;			
	struct line_specific_data {
		int	line;			
		int	index;			
		unsigned long offset;		
#ifdef SSE2
		int	imag_dist;		
#endif
	} lsd;
} read_write_line_slice_data;

void prep_line (gwhandle *gwdata, int line, read_write_line_slice_data *rwlsd);
void read_line (pmhandle *pmdata, read_write_line_slice_data *rwlsd);
void write_line (pmhandle *pmdata, read_write_line_slice_data *rwlsd);
void write_line_slice_strided (pmhandle *pmdata, read_write_line_slice_data *rwlsd, CVDT *vec, uint64_t slice_start, uint64_t slice_end, uint64_t stride_offset, uint64_t stride);
void fft_line_pass (int helper_num, pmhandle *pmdata, void *pdarg);

#if defined (AVX512)

void prep_line (gwhandle *gwdata, int line, read_write_line_slice_data *rwlsd)
{
	
	if (gwdata->GENERAL_MMGW_MOD) {
		int cyclic_lines = gwdata->FFTLEN * sizeof (double) / sizeof (CVDT) + 1;
		if (line < cyclic_lines) prep_line (gwdata->cyclic_gwdata, line, rwlsd);
		else {
			prep_line (gwdata->negacyclic_gwdata, line - cyclic_lines, rwlsd);
			rwlsd->lsd.line += cyclic_lines;
			rwlsd->lsd.index += cyclic_lines;
			rwlsd->lsd.offset += round_up_to_multiple_of (gwnum_datasize(gwdata->cyclic_gwdata) + GW_HEADER_SIZE(gwdata->negacyclic_gwdata), 128) / sizeof (VDT);
		}
		return;
	}

	int index = line;
	if (!gwdata->NEGACYCLIC_FFT) index--;

	ASSERTG (index * 16 < (int) gwdata->FFTLEN);

	unsigned long offset;
	if (index >= 1 || (index == 0 && gwdata->NEGACYCLIC_FFT)) offset = cache_line_offset (gwdata, index * 2) / sizeof (VDT);

	rwlsd->lsd.line = line;
	rwlsd->lsd.index = index;
	rwlsd->lsd.offset = offset;
}

void read_line_slice_avx512 (int helper_num, pmhandle *pmdata, void *rwlsd)
{
	gwhandle *gwdata = pmdata->gwdata;
	gwnum	*data = ((read_write_line_slice_data *) rwlsd)->data;
	uint64_t size = ((read_write_line_slice_data *) rwlsd)->size;
	int	index = ((read_write_line_slice_data *) rwlsd)->lsd.index;
	int	options = ((read_write_line_slice_data *) rwlsd)->options;
	bool	post_process_monics = ((read_write_line_slice_data *) rwlsd)->post_process_monics;
	uint64_t slice_size = ((read_write_line_slice_data *) rwlsd)->slice_size;

	for ( ; ; ) {
		uint64_t slice_start, slice_end;
		
		if (slice_size == size) {
			slice_start = 0;
			slice_end = size;
		} else {
			int64_t atomic_val = atomic_fetch_incr (((read_write_line_slice_data *) rwlsd)->blknum);
			slice_start = atomic_val * slice_size;
			if (slice_start >= size) break;
			slice_end = slice_start + slice_size;
			if (slice_end > size) slice_end = size;
		}

		CVDT *vec = ((read_write_line_slice_data *) rwlsd)->vec;

		if (!post_process_monics && (options & (POLYMULT_INVEC1_MONIC | POLYMULT_INVEC2_MONIC)) && (options & (POLYMULT_INVEC1_RLP | POLYMULT_INVEC2_RLP))) {
			if (slice_start == 0) {
				if (!gwdata->GENERAL_MMGW_MOD && gwdata->k == 1.0) {
					vec[0].real = broadcastsd (1.0);
					vec[0].imag = broadcastsd (0.0);
				} else {
					read_write_line_slice_data fft1_rwlsd = * (read_write_line_slice_data *) rwlsd;
					fft1_rwlsd.data = &gwdata->GW_FFT1;
					fft1_rwlsd.size = 1;
					fft1_rwlsd.vec = vec;
					fft1_rwlsd.options = 0;
					fft1_rwlsd.post_process_monics = FALSE;
					read_line (pmdata, &fft1_rwlsd);
				}
			}
			vec++;
		}

		if (options & (POLYMULT_INVEC1_RLP | POLYMULT_INVEC2_RLP)) vec += (size-1);

		if (index == -1) {			
			for (uint64_t j = slice_start; j < slice_end; j++) {
				if (data[j] == NULL) { memset (&vec[j], 0, sizeof (CVDT)); continue; }
				
				if (!gwdata->ZERO_PADDED_FFT) {
					vec[j].real = _mm512_set_pd (0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, data[j][0]);
					vec[j].imag = broadcastsd (0.0);
				}
				
				else {
					vec[j].real = _mm512_set_pd (data[j][-18], data[j][-17], data[j][-16], data[j][-15],
								     data[j][-14], data[j][-13], data[j][-12], data[j][0]);
					vec[j].imag = _mm512_set_pd (data[j][-24], data[j][-23], data[j][-22], data[j][-21],
								     data[j][-20], data[j][-19], 0.0, 0.0);
				}
			}
		}
		else if (index == 0 && !gwdata->NEGACYCLIC_FFT) {	
			for (uint64_t j = slice_start; j < slice_end; j++) {
				if (data[j] == NULL) { memset (&vec[j], 0, sizeof (CVDT)); continue; }
				vec[j].real = _mm512_set_pd (data[j][7], data[j][6], data[j][5], data[j][4], data[j][3], data[j][2], data[j][1], data[j][8]);
				vec[j].imag = _mm512_set_pd (data[j][15], data[j][14], data[j][13], data[j][12], data[j][11], data[j][10], data[j][9], 0.0);
			}
		}
		else {				
			unsigned long offset = ((read_write_line_slice_data *) rwlsd)->lsd.offset;
			for (uint64_t j = slice_start; j < slice_end; j++) {
				if (data[j] == NULL) { memset (&vec[j], 0, sizeof(CVDT)); continue; }
				ASSERTG (FFT_state (data[j]) == FULLY_FFTed);
				vec[j].real = ((VDT *) data[j])[offset];
				vec[j].imag = ((VDT *) data[j])[offset+1];
			}
		}

		if (options & (POLYMULT_INVEC1_NEGATE | POLYMULT_INVEC2_NEGATE)) {
			VDT zero = broadcastsd (0.0);
			for (uint64_t j = slice_start; j < slice_end; j++) {
				vec[j].real = subpd (zero, vec[j].real);
				vec[j].imag = subpd (zero, vec[j].imag);
			}
		}

		if (options & (POLYMULT_INVEC1_RLP | POLYMULT_INVEC2_RLP)) {
			for (uint64_t j = slice_start; j < slice_end; j++) vec[-(int64_t)j] = vec[j];
		}

		if (!post_process_monics && (options & (POLYMULT_INVEC1_MONIC | POLYMULT_INVEC2_MONIC))) {
			if (slice_start == 0) {
				if (!gwdata->GENERAL_MMGW_MOD && gwdata->k == 1.0) {
					vec[size].real = broadcastsd (1.0);
					vec[size].imag = broadcastsd (0.0);
				} else {
					read_write_line_slice_data fft1_rwlsd = * (read_write_line_slice_data *) rwlsd;
					fft1_rwlsd.data = &gwdata->GW_FFT1;
					fft1_rwlsd.size = 1;
					fft1_rwlsd.vec = &vec[size];
					fft1_rwlsd.options = 0;
					fft1_rwlsd.post_process_monics = FALSE;
					read_line (pmdata, &fft1_rwlsd);
				}
			}
		}

		if (slice_end == size) break;
	}
}

void write_line_slice_strided_avx512 (pmhandle *pmdata, read_write_line_slice_data *rwlsd, CVDT *vec, uint64_t slice_start, uint64_t slice_end, uint64_t stride_offset, uint64_t stride)
{
	gwhandle *gwdata = pmdata->gwdata;
	gwnum	*data = rwlsd->data;
	gwnum	*fma_data = rwlsd->fma_data;
	int	index = rwlsd->lsd.index;
	int	options = rwlsd->options;
	CVDT	fmaval, oneval;

	uint64_t j_in = divide_rounding_up (slice_start + rwlsd->LSWs_skipped - stride_offset, stride);
	uint64_t j_out = j_in * stride + stride_offset - rwlsd->LSWs_skipped;

	union {
		VDT	a;
		double	b[8];
	} x;
	if (index == -1) {			
		for ( ; j_out < slice_end; j_out += stride, j_in++) {
			if (data[j_out] == NULL) continue;
			if (options & (POLYMULT_FMADD | POLYMULT_FMSUB | POLYMULT_FNMADD)) {
				if (fma_data == NULL || fma_data[j_out] == NULL)
					fmaval.real = broadcastsd (0.0), fmaval.imag = broadcastsd (0.0);
				else
					fmaval.real = _mm512_set_pd (fma_data[j_out][-18], fma_data[j_out][-17], fma_data[j_out][-16], fma_data[j_out][-15],
								     fma_data[j_out][-14], fma_data[j_out][-13], fma_data[j_out][-12], fma_data[j_out][0]),
					fmaval.imag = _mm512_set_pd (fma_data[j_out][-24], fma_data[j_out][-23], fma_data[j_out][-22], fma_data[j_out][-21],
								     fma_data[j_out][-20], fma_data[j_out][-19], 0.0, 0.0);
				if (gwdata->GENERAL_MMGW_MOD || gwdata->k != 1.0) {
					oneval.real = _mm512_set_pd (gwdata->GW_FFT1[-18], gwdata->GW_FFT1[-17], gwdata->GW_FFT1[-16], gwdata->GW_FFT1[-15],
								     gwdata->GW_FFT1[-14], gwdata->GW_FFT1[-13], gwdata->GW_FFT1[-12], gwdata->GW_FFT1[0]);
					oneval.imag = _mm512_set_pd (gwdata->GW_FFT1[-24], gwdata->GW_FFT1[-23], gwdata->GW_FFT1[-22], gwdata->GW_FFT1[-21],
								     gwdata->GW_FFT1[-20], gwdata->GW_FFT1[-19], 0.0, 0.0);
					cvmul (fmaval, fmaval, oneval);
				}
				if (options & POLYMULT_FMADD) cvadd (vec[j_in], vec[j_in], fmaval);
				else if (options & POLYMULT_FMSUB) cvsub (vec[j_in], vec[j_in], fmaval);
				else cvsub (vec[j_in], fmaval, vec[j_in]);
			}
			
			x.a = vec[j_in].real;
			data[j_out][0] = x.b[0];
			
			if (gwdata->ZERO_PADDED_FFT) {
				data[j_out][-18] = x.b[7];
				data[j_out][-17] = x.b[6];
				data[j_out][-16] = x.b[5];
				data[j_out][-15] = x.b[4];
				data[j_out][-14] = x.b[3];
				data[j_out][-13] = x.b[2];
				data[j_out][-12] = x.b[1];
				x.a = vec[j_in].imag;
				data[j_out][-24] = x.b[7];
				data[j_out][-23] = x.b[6];
				data[j_out][-22] = x.b[5];
				data[j_out][-21] = x.b[4];
				data[j_out][-20] = x.b[3];
				data[j_out][-19] = x.b[2];
			}
		}
	}
	else if (index == 0 && !gwdata->NEGACYCLIC_FFT) {	
		for ( ; j_out < slice_end; j_out += stride, j_in++) {
			if (data[j_out] == NULL) continue;
			if (options & (POLYMULT_FMADD | POLYMULT_FMSUB | POLYMULT_FNMADD)) {
				if (fma_data == NULL || fma_data[j_out] == NULL)
					fmaval.real = broadcastsd (0.0), fmaval.imag = broadcastsd (0.0);
				else
					fmaval.real = _mm512_set_pd (fma_data[j_out][7], fma_data[j_out][6], fma_data[j_out][5], fma_data[j_out][4],
								     fma_data[j_out][3], fma_data[j_out][2], fma_data[j_out][1], fma_data[j_out][8]),
					fmaval.imag = _mm512_set_pd (fma_data[j_out][15], fma_data[j_out][14], fma_data[j_out][13], fma_data[j_out][12],
								     fma_data[j_out][11], fma_data[j_out][10], fma_data[j_out][9], 0.0);
				if (gwdata->GENERAL_MMGW_MOD || gwdata->k != 1.0) {
					oneval.real = _mm512_set_pd (gwdata->GW_FFT1[7], gwdata->GW_FFT1[6], gwdata->GW_FFT1[5], gwdata->GW_FFT1[4],
								     gwdata->GW_FFT1[3], gwdata->GW_FFT1[2], gwdata->GW_FFT1[1], gwdata->GW_FFT1[8]);
					oneval.imag = _mm512_set_pd (gwdata->GW_FFT1[15], gwdata->GW_FFT1[14], gwdata->GW_FFT1[13], gwdata->GW_FFT1[12],
								     gwdata->GW_FFT1[11], gwdata->GW_FFT1[10], gwdata->GW_FFT1[9], 0.0);
					cvmul (fmaval, fmaval, oneval);
				}
				if (options & POLYMULT_FMADD) cvadd (vec[j_in], vec[j_in], fmaval);
				else if (options & POLYMULT_FMSUB) cvsub (vec[j_in], vec[j_in], fmaval);
				else cvsub (vec[j_in], fmaval, vec[j_in]);
			}
			x.a = vec[j_in].real, data[j_out][7] = x.b[7], data[j_out][6] = x.b[6], data[j_out][5] = x.b[5], data[j_out][4] = x.b[4],
					      data[j_out][3] = x.b[3], data[j_out][2] = x.b[2], data[j_out][1] = x.b[1], data[j_out][8] = x.b[0];
			x.a = vec[j_in].imag, data[j_out][15] = x.b[7], data[j_out][14] = x.b[6], data[j_out][13] = x.b[5], data[j_out][12] = x.b[4],
					      data[j_out][11] = x.b[3], data[j_out][10] = x.b[2], data[j_out][9] = x.b[1];
		}
	}
	else {				
		unsigned long offset = rwlsd->lsd.offset;
		for ( ; j_out < slice_end; j_out += stride, j_in++) {
			if (data[j_out] == NULL) continue;
			if (options & (POLYMULT_FMADD | POLYMULT_FMSUB | POLYMULT_FNMADD)) {
				if (fma_data == NULL || fma_data[j_out] == NULL) fmaval.real = broadcastsd (0.0), fmaval.imag = broadcastsd (0.0);
				else fmaval.real = ((VDT *) fma_data[j_out])[offset], fmaval.imag = ((VDT *) fma_data[j_out])[offset+1];
				if (gwdata->GENERAL_MMGW_MOD || gwdata->k != 1.0) {
					oneval.real = ((VDT *) gwdata->GW_FFT1)[offset], oneval.imag = ((VDT *) gwdata->GW_FFT1)[offset+1];
					cvmul (fmaval, fmaval, oneval);
				}
				if (options & POLYMULT_FMADD) cvadd (vec[j_in], vec[j_in], fmaval);
				else if (options & POLYMULT_FMSUB) cvsub (vec[j_in], vec[j_in], fmaval);
				else cvsub (vec[j_in], fmaval, vec[j_in]);
			}
			if (rwlsd->streamed_stores) {
				streampd (&((VDT *) data[j_out])[offset], vec[j_in].real);
				streampd (&((VDT *) data[j_out])[offset+1], vec[j_in].imag);
			} else {
				((VDT *) data[j_out])[offset] = vec[j_in].real;
				((VDT *) data[j_out])[offset+1] = vec[j_in].imag;
			}
		}
	}
}

void write_line_slice_avx512 (int helper_num, pmhandle *pmdata, void *rwlsd)
{
	gwhandle *gwdata = pmdata->gwdata;
	uint64_t size = ((read_write_line_slice_data *) rwlsd)->size;
	uint64_t slice_size = ((read_write_line_slice_data *) rwlsd)->slice_size;
	CVDT	*vec = ((read_write_line_slice_data *) rwlsd)->vec;

	for ( ; ; ) {
		uint64_t slice_start, slice_end;
		
		if (slice_size == size) {
			slice_start = 0;
			slice_end = size;
		} else {
			int64_t atomic_val = atomic_fetch_incr (((read_write_line_slice_data *) rwlsd)->blknum);
			slice_start = atomic_val * slice_size;
			if (slice_start >= size) break;
			slice_end = slice_start + slice_size;
			if (slice_end > size) slice_end = size;
		}

		write_line_slice_strided (pmdata, (read_write_line_slice_data *) rwlsd, vec, slice_start, slice_end, 0, 1);

		if (slice_end == size) break;
	}
}

#elif defined (FMA)

#elif defined (AVX)

void prep_line (gwhandle *gwdata, int line, read_write_line_slice_data *rwlsd)
{
	
	if (gwdata->GENERAL_MMGW_MOD) {
		int cyclic_lines = gwdata->FFTLEN * sizeof (double) / sizeof (CVDT) + 1;
		if (line < cyclic_lines) prep_line (gwdata->cyclic_gwdata, line, rwlsd);
		else {
			prep_line (gwdata->negacyclic_gwdata, line - cyclic_lines, rwlsd);
			rwlsd->lsd.line += cyclic_lines;
			rwlsd->lsd.index += cyclic_lines;
			rwlsd->lsd.offset += round_up_to_multiple_of (gwnum_datasize(gwdata->cyclic_gwdata) + GW_HEADER_SIZE(gwdata->negacyclic_gwdata), 128) / sizeof (VDT);
		}
		return;
	}

	int index = line;
	if (!gwdata->NEGACYCLIC_FFT) index--;

	if (gwdata->ZERO_PADDED_FFT) index--;

	ASSERTG (index * 8 < (int) gwdata->FFTLEN);

	unsigned long offset;
	if (index >= 1 || (index == 0 && gwdata->NEGACYCLIC_FFT)) offset = cache_line_offset (gwdata, index) / sizeof (VDT);

	rwlsd->lsd.line = line;
	rwlsd->lsd.index = index;
	rwlsd->lsd.offset = offset;
}

void read_line_slice_avx (int helper_num, pmhandle *pmdata, void *rwlsd)
{
	gwhandle *gwdata = pmdata->gwdata;
	gwnum	*data = ((read_write_line_slice_data *) rwlsd)->data;
	uint64_t size = ((read_write_line_slice_data *) rwlsd)->size;
	int	index = ((read_write_line_slice_data *) rwlsd)->lsd.index;
	int	options = ((read_write_line_slice_data *) rwlsd)->options;
	bool	post_process_monics = ((read_write_line_slice_data *) rwlsd)->post_process_monics;
	uint64_t slice_size = ((read_write_line_slice_data *) rwlsd)->slice_size;

	for ( ; ; ) {
		uint64_t slice_start, slice_end;
		
		if (slice_size == size) {
			slice_start = 0;
			slice_end = size;
		} else {
			int64_t atomic_val = atomic_fetch_incr (((read_write_line_slice_data *) rwlsd)->blknum);
			slice_start = atomic_val * slice_size;
			if (slice_start >= size) break;
			slice_end = slice_start + slice_size;
			if (slice_end > size) slice_end = size;
		}

		CVDT *vec = ((read_write_line_slice_data *) rwlsd)->vec;

		if (!post_process_monics && (options & (POLYMULT_INVEC1_MONIC | POLYMULT_INVEC2_MONIC)) && (options & (POLYMULT_INVEC1_RLP | POLYMULT_INVEC2_RLP))) {
			if (slice_start == 0) {
				if (!gwdata->GENERAL_MMGW_MOD && gwdata->k == 1.0) {
					vec[0].real = broadcastsd (1.0);
					vec[0].imag = broadcastsd (0.0);
				} else {
					read_write_line_slice_data fft1_rwlsd = * (read_write_line_slice_data *) rwlsd;
					fft1_rwlsd.data = &gwdata->GW_FFT1;
					fft1_rwlsd.size = 1;
					fft1_rwlsd.vec = vec;
					fft1_rwlsd.options = 0;
					fft1_rwlsd.post_process_monics = FALSE;
					read_line (pmdata, &fft1_rwlsd);
				}
			}
			vec++;
		}

		if (options & (POLYMULT_INVEC1_RLP | POLYMULT_INVEC2_RLP)) vec += (size-1);

		if (index == -2) {			
			for (uint64_t j = slice_start; j < slice_end; j++) {
				if (data[j] == NULL) { memset (&vec[j], 0, sizeof (CVDT)); continue; }
				
				vec[j].real = _mm256_set_pd (data[j][-18], data[j][-17], data[j][-16], data[j][-15]);
				vec[j].imag = _mm256_set_pd (data[j][-24], data[j][-23], data[j][-22], data[j][-21]);
			}
		}
		else if (index == -1) {			
			for (uint64_t j = slice_start; j < slice_end; j++) {
				if (data[j] == NULL) { memset (&vec[j], 0, sizeof (CVDT)); continue; }
				
				if (!gwdata->ZERO_PADDED_FFT) {
					vec[j].real = _mm256_set_pd (0.0, 0.0, 0.0, data[j][0]);
					vec[j].imag = broadcastsd (0.0);
				}
				
				else {
					vec[j].real = _mm256_set_pd (data[j][-14], data[j][-13], data[j][-12], data[j][0]);
					vec[j].imag = _mm256_set_pd (data[j][-20], data[j][-19], 0.0, 0.0);
				}
			}
		}
		else if (index == 0 && !gwdata->NEGACYCLIC_FFT) {	
			for (uint64_t j = slice_start; j < slice_end; j++) {
				if (data[j] == NULL) { memset (&vec[j], 0, sizeof (CVDT)); continue; }
				vec[j].real = _mm256_set_pd (data[j][3], data[j][2], data[j][1], data[j][4]);
				vec[j].imag = _mm256_set_pd (data[j][7], data[j][6], data[j][5], 0.0);
			}
		}
		else {				
			unsigned long offset = ((read_write_line_slice_data *) rwlsd)->lsd.offset;
			for (uint64_t j = slice_start; j < slice_end; j++) {
				if (data[j] == NULL) { memset (&vec[j], 0, sizeof (CVDT)); continue; }
				ASSERTG (FFT_state (data[j]) == FULLY_FFTed);
				vec[j].real = ((VDT *) data[j])[offset];
				vec[j].imag = ((VDT *) data[j])[offset+1];
			}
		}

		if (options & (POLYMULT_INVEC1_NEGATE | POLYMULT_INVEC2_NEGATE)) {
			VDT zero = broadcastsd (0.0);
			for (uint64_t j = slice_start; j < slice_end; j++) {
				vec[j].real = subpd (zero, vec[j].real);
				vec[j].imag = subpd (zero, vec[j].imag);
			}
		}

		if (options & (POLYMULT_INVEC1_RLP | POLYMULT_INVEC2_RLP)) {
			for (uint64_t j = slice_start; j < slice_end; j++) vec[-(int64_t)j] = vec[j];
		}

		if (!post_process_monics && (options & (POLYMULT_INVEC1_MONIC | POLYMULT_INVEC2_MONIC))) {
			if (slice_start == 0) {
				if (!gwdata->GENERAL_MMGW_MOD && gwdata->k == 1.0) {
					vec[size].real = broadcastsd (1.0);
					vec[size].imag = broadcastsd (0.0);
				} else {
					read_write_line_slice_data fft1_rwlsd = * (read_write_line_slice_data *) rwlsd;
					fft1_rwlsd.data = &gwdata->GW_FFT1;
					fft1_rwlsd.size = 1;
					fft1_rwlsd.vec = &vec[size];
					fft1_rwlsd.options = 0;
					fft1_rwlsd.post_process_monics = FALSE;
					read_line (pmdata, &fft1_rwlsd);
				}
			}
		}

		if (slice_end == size) break;
	}
}

void write_line_slice_strided_avx (pmhandle *pmdata, read_write_line_slice_data *rwlsd, CVDT *vec, uint64_t slice_start, uint64_t slice_end, uint64_t stride_offset, uint64_t stride)
{
	gwhandle *gwdata = pmdata->gwdata;
	gwnum	*data = rwlsd->data;
	gwnum	*fma_data = rwlsd->fma_data;
	uint64_t index = rwlsd->lsd.index;
	int	options = rwlsd->options;
	CVDT	fmaval, oneval;

	uint64_t j_in = divide_rounding_up (slice_start + rwlsd->LSWs_skipped - stride_offset, stride);
	uint64_t j_out = j_in * stride + stride_offset - rwlsd->LSWs_skipped;

	union {
		VDT	a;
		double	b[4];
	} x;
	if (index == -2) {			
		for ( ; j_out < slice_end; j_out += stride, j_in++) {
			if (data[j_out] == NULL) continue;
			if (options & (POLYMULT_FMADD | POLYMULT_FMSUB | POLYMULT_FNMADD)) {
				if (fma_data == NULL || fma_data[j_out] == NULL)
					fmaval.real = broadcastsd (0.0), fmaval.imag = broadcastsd (0.0);
				else
					fmaval.real = _mm256_set_pd (fma_data[j_out][-18], fma_data[j_out][-17], fma_data[j_out][-16], fma_data[j_out][-15]),
					fmaval.imag = _mm256_set_pd (fma_data[j_out][-24], fma_data[j_out][-23], fma_data[j_out][-22], fma_data[j_out][-21]);
				if (gwdata->GENERAL_MMGW_MOD || gwdata->k != 1.0) {
					oneval.real = _mm256_set_pd (gwdata->GW_FFT1[-18], gwdata->GW_FFT1[-17], gwdata->GW_FFT1[-16], gwdata->GW_FFT1[-15]),
					oneval.imag = _mm256_set_pd (gwdata->GW_FFT1[-24], gwdata->GW_FFT1[-23], gwdata->GW_FFT1[-22], gwdata->GW_FFT1[-21]);
					cvmul (fmaval, fmaval, oneval);
				}
				if (options & POLYMULT_FMADD) cvadd (vec[j_in], vec[j_in], fmaval);
				else if (options & POLYMULT_FMSUB) cvsub (vec[j_in], vec[j_in], fmaval);
				else cvsub (vec[j_in], fmaval, vec[j_in]);
			}
			x.a = vec[j_in].real;
			data[j_out][-18] = x.b[3];
			data[j_out][-17] = x.b[2];
			data[j_out][-16] = x.b[1];
			data[j_out][-15] = x.b[0];
			x.a = vec[j_in].imag;
			data[j_out][-24] = x.b[3];
			data[j_out][-23] = x.b[2];
			data[j_out][-22] = x.b[1];
			data[j_out][-21] = x.b[0];
		}
	}
	else if (index == -1) {			
		for ( ; j_out < slice_end; j_out += stride, j_in++) {
			if (data[j_out] == NULL) continue;
			if (options & (POLYMULT_FMADD | POLYMULT_FMSUB | POLYMULT_FNMADD)) {
				if (fma_data == NULL || fma_data[j_out] == NULL)
					fmaval.real = broadcastsd (0.0), fmaval.imag = broadcastsd (0.0);
				else
					fmaval.real = _mm256_set_pd (fma_data[j_out][-14], fma_data[j_out][-13], fma_data[j_out][-12], fma_data[j_out][0]),
					fmaval.imag = _mm256_set_pd (fma_data[j_out][-20], fma_data[j_out][-19], 0.0, 0.0);
				if (gwdata->GENERAL_MMGW_MOD || gwdata->k != 1.0) {
					oneval.real = _mm256_set_pd (gwdata->GW_FFT1[-14], gwdata->GW_FFT1[-13], gwdata->GW_FFT1[-12], gwdata->GW_FFT1[0]),
					oneval.imag = _mm256_set_pd (gwdata->GW_FFT1[-20], gwdata->GW_FFT1[-19], 0.0, 0.0);
					cvmul (fmaval, fmaval, oneval);
				}
				if (options & POLYMULT_FMADD) cvadd (vec[j_in], vec[j_in], fmaval);
				else if (options & POLYMULT_FMSUB) cvsub (vec[j_in], vec[j_in], fmaval);
				else cvsub (vec[j_in], fmaval, vec[j_in]);
			}
			
			x.a = vec[j_in].real;
			data[j_out][0] = x.b[0];
			
			if (gwdata->ZERO_PADDED_FFT) {
				data[j_out][-14] = x.b[3];
				data[j_out][-13] = x.b[2];
				data[j_out][-12] = x.b[1];
				x.a = vec[j_in].imag;
				data[j_out][-20] = x.b[3];
				data[j_out][-19] = x.b[2];
			}
		}
	}
	else if (index == 0 && !gwdata->NEGACYCLIC_FFT) {			
		for ( ; j_out < slice_end; j_out += stride, j_in++) {
			if (data[j_out] == NULL) continue;
			if (options & (POLYMULT_FMADD | POLYMULT_FMSUB | POLYMULT_FNMADD)) {
				if (fma_data == NULL || fma_data[j_out] == NULL)
					fmaval.real = broadcastsd (0.0), fmaval.imag = broadcastsd (0.0);
				else
					fmaval.real = _mm256_set_pd (fma_data[j_out][3], fma_data[j_out][2], fma_data[j_out][1], fma_data[j_out][4]),
					fmaval.imag = _mm256_set_pd (fma_data[j_out][7], fma_data[j_out][6], fma_data[j_out][5], 0.0);
				if (gwdata->GENERAL_MMGW_MOD || gwdata->k != 1.0) {
					oneval.real = _mm256_set_pd (gwdata->GW_FFT1[3], gwdata->GW_FFT1[2], gwdata->GW_FFT1[1], gwdata->GW_FFT1[4]),
					oneval.imag = _mm256_set_pd (gwdata->GW_FFT1[7], gwdata->GW_FFT1[6], gwdata->GW_FFT1[5], 0.0);
					cvmul (fmaval, fmaval, oneval);
				}
				if (options & POLYMULT_FMADD) cvadd (vec[j_in], vec[j_in], fmaval);
				else if (options & POLYMULT_FMSUB) cvsub (vec[j_in], vec[j_in], fmaval);
				else cvsub (vec[j_in], fmaval, vec[j_in]);
			}
			x.a = vec[j_in].real, data[j_out][3] = x.b[3], data[j_out][2] = x.b[2], data[j_out][1] = x.b[1], data[j_out][4] = x.b[0];
			x.a = vec[j_in].imag, data[j_out][7] = x.b[3], data[j_out][6] = x.b[2], data[j_out][5] = x.b[1];
		}
	}
	else {				
		unsigned long offset = ((read_write_line_slice_data *) rwlsd)->lsd.offset;
		for ( ; j_out < slice_end; j_out += stride, j_in++) {
			if (data[j_out] == NULL) continue;
			if (options & (POLYMULT_FMADD | POLYMULT_FMSUB | POLYMULT_FNMADD)) {
				if (fma_data == NULL || fma_data[j_out] == NULL) fmaval.real = broadcastsd (0.0), fmaval.imag = broadcastsd (0.0);
				else fmaval.real = ((VDT *) fma_data[j_out])[offset], fmaval.imag = ((VDT *) fma_data[j_out])[offset+1];
				if (gwdata->GENERAL_MMGW_MOD || gwdata->k != 1.0) {
					oneval.real = ((VDT *) gwdata->GW_FFT1)[offset], oneval.imag = ((VDT *) gwdata->GW_FFT1)[offset+1];
					cvmul (fmaval, fmaval, oneval);
				}
				if (options & POLYMULT_FMADD) cvadd (vec[j_in], vec[j_in], fmaval);
				else if (options & POLYMULT_FMSUB) cvsub (vec[j_in], vec[j_in], fmaval);
				else cvsub (vec[j_in], fmaval, vec[j_in]);
			}
			if (rwlsd->streamed_stores) {
				streampd (&((VDT *) data[j_out])[offset], vec[j_in].real);
				streampd (&((VDT *) data[j_out])[offset+1], vec[j_in].imag);
			} else {
				((VDT *) data[j_out])[offset] = vec[j_in].real;
				((VDT *) data[j_out])[offset+1] = vec[j_in].imag;
			}
		}
	}
}

void write_line_slice_avx (int helper_num, pmhandle *pmdata, void *rwlsd)
{
	gwhandle *gwdata = pmdata->gwdata;
	uint64_t size = ((read_write_line_slice_data *) rwlsd)->size;
	uint64_t slice_size = ((read_write_line_slice_data *) rwlsd)->slice_size;
	CVDT	*vec = ((read_write_line_slice_data *) rwlsd)->vec;

	for ( ; ; ) {
		uint64_t slice_start, slice_end;
		
		if (slice_size == size) {
			slice_start = 0;
			slice_end = size;
		} else {
			int64_t atomic_val = atomic_fetch_incr (((read_write_line_slice_data *) rwlsd)->blknum);
			slice_start = atomic_val * slice_size;
			if (slice_start >= size) break;
			slice_end = slice_start + slice_size;
			if (slice_end > size) slice_end = size;
		}

		write_line_slice_strided (pmdata, (read_write_line_slice_data *) rwlsd, vec, slice_start, slice_end, 0, 1);

		if (slice_end == size) break;
	}
}

#elif defined (SSE2)

void prep_line (gwhandle *gwdata, int line, read_write_line_slice_data *rwlsd)
{
	
	if (gwdata->GENERAL_MMGW_MOD) {
		int cyclic_lines = gwdata->FFTLEN * sizeof (double) / sizeof (CVDT) + 1;
		if (line < cyclic_lines) prep_line (gwdata->cyclic_gwdata, line, rwlsd);
		else {
			prep_line (gwdata->negacyclic_gwdata, line - cyclic_lines, rwlsd);
			rwlsd->lsd.line += cyclic_lines;
			rwlsd->lsd.index += cyclic_lines;
			rwlsd->lsd.offset += round_up_to_multiple_of (gwnum_datasize(gwdata->cyclic_gwdata) + GW_HEADER_SIZE(gwdata->negacyclic_gwdata), 128) / sizeof (VDT);
		}
		return;
	}

	int index = line;
	if (!gwdata->NEGACYCLIC_FFT) index--;

	if (gwdata->ZERO_PADDED_FFT) index -= 3;

	ASSERTG (index * 4 < (int) gwdata->FFTLEN);

	int	imag_dist;
	if (gwdata->FFT_TYPE > 0 &&					
	    (gwdata->PASS2_SIZE == 1536 || gwdata->PASS2_SIZE == 2048 || gwdata->PASS2_SIZE == 2560 ||
	     gwdata->PASS2_SIZE == 4608 || gwdata->PASS2_SIZE == 6144 || gwdata->PASS2_SIZE == 7680 ||
	     gwdata->PASS2_SIZE == 8192 || gwdata->PASS2_SIZE == 10240 || gwdata->PASS2_SIZE == 12800)) imag_dist = 32;
	else if (gwdata->NEGACYCLIC_FFT) imag_dist = 16;
	else if (gwdata->PASS2_SIZE == 0 && index < 4) imag_dist = 8;
	else if (gwdata->PASS2_SIZE != 0 && gwdata->FFT_TYPE == 0 && (index <= 1 || index == 4 || index == 5)) imag_dist = 8;	
	else imag_dist = 16;

	unsigned long offset;
	if (index >= 1 || (index == 0 && gwdata->NEGACYCLIC_FFT)) offset = cache_line_offset (gwdata, index / 2) / sizeof (VDT);

	rwlsd->lsd.line = line;
	rwlsd->lsd.index = index;
	rwlsd->lsd.offset = offset;
	rwlsd->lsd.imag_dist = imag_dist;
}

void read_line_slice_sse2 (int helper_num, pmhandle *pmdata, void *rwlsd)
{
	gwhandle *gwdata = pmdata->gwdata;
	gwnum	*data = ((read_write_line_slice_data *) rwlsd)->data;
	uint64_t size = ((read_write_line_slice_data *) rwlsd)->size;
	int	index = ((read_write_line_slice_data *) rwlsd)->lsd.index;
	int	options = ((read_write_line_slice_data *) rwlsd)->options;
	bool	post_process_monics = ((read_write_line_slice_data *) rwlsd)->post_process_monics;
	uint64_t slice_size = ((read_write_line_slice_data *) rwlsd)->slice_size;
	int	imag_dist = ((read_write_line_slice_data *) rwlsd)->lsd.imag_dist;

	for ( ; ; ) {
		uint64_t slice_start, slice_end;
		
		if (slice_size == size) {
			slice_start = 0;
			slice_end = size;
		} else {
			int64_t atomic_val = atomic_fetch_incr (((read_write_line_slice_data *) rwlsd)->blknum);
			slice_start = atomic_val * slice_size;
			if (slice_start >= size) break;
			slice_end = slice_start + slice_size;
			if (slice_end > size) slice_end = size;
		}

		CVDT *vec = ((read_write_line_slice_data *) rwlsd)->vec;

		if (!post_process_monics && (options & (POLYMULT_INVEC1_MONIC | POLYMULT_INVEC2_MONIC)) && (options & (POLYMULT_INVEC1_RLP | POLYMULT_INVEC2_RLP))) {
			if (slice_start == 0) {
				if (!gwdata->GENERAL_MMGW_MOD && gwdata->k == 1.0) {
					vec[0].real = broadcastsd (1.0);
					vec[0].imag = broadcastsd (0.0);
				} else {
					read_write_line_slice_data fft1_rwlsd = * (read_write_line_slice_data *) rwlsd;
					fft1_rwlsd.data = &gwdata->GW_FFT1;
					fft1_rwlsd.size = 1;
					fft1_rwlsd.vec = vec;
					fft1_rwlsd.options = 0;
					fft1_rwlsd.post_process_monics = FALSE;
					read_line (pmdata, &fft1_rwlsd);
				}
			}
			vec++;
		}

		if (options & (POLYMULT_INVEC1_RLP | POLYMULT_INVEC2_RLP)) vec += (size-1);

		if (index <= -2) {			
			for (uint64_t j = slice_start; j < slice_end; j++) {
				if (data[j] == NULL) { memset (&vec[j], 0, sizeof (CVDT)); continue; }
				
				vec[j].real = _mm_set_pd (data[j][index*2-10], data[j][index*2-9]);
				vec[j].imag = _mm_set_pd (data[j][index*2-16], data[j][index*2-15]);
			}
		}
		else if (index == -1) {			
			for (uint64_t j = slice_start; j < slice_end; j++) {
				if (data[j] == NULL) { memset (&vec[j], 0, sizeof (CVDT)); continue; }
				
				if (!gwdata->ZERO_PADDED_FFT) {
					vec[j].real = _mm_set_pd (0.0, data[j][0]);
					vec[j].imag = broadcastsd (0.0);
				}
				
				else {
					vec[j].real = _mm_set_pd (data[j][-12], data[j][0]);
					vec[j].imag = broadcastsd (0.0);
				}
			}
		}
		else if (index == 0 && !gwdata->NEGACYCLIC_FFT) {	
			for (uint64_t j = slice_start; j < slice_end; j++) {
				if (data[j] == NULL) { memset (&vec[j], 0, sizeof (CVDT)); continue; }
				if (imag_dist == 8) {
					vec[j].real = _mm_set_pd (data[j][2], data[j][1]);
					vec[j].imag = _mm_set_pd (data[j][3], 0.0);
				} else if (imag_dist == 16) {
					vec[j].real = _mm_set_pd (data[j][1], data[j][2]);
					vec[j].imag = _mm_set_pd (data[j][3], 0.0);
				} else {
					vec[j].real = _mm_set_pd (data[j][1], data[j][4]);
					vec[j].imag = _mm_set_pd (data[j][5], 0.0);
				}
			}
		}
		else {				
			unsigned long offset = ((read_write_line_slice_data *) rwlsd)->lsd.offset;
			for (uint64_t j = slice_start; j < slice_end; j++) {
				if (data[j] == NULL) { memset (&vec[j], 0, sizeof (CVDT)); continue; }
				ASSERTG (FFT_state (data[j]) == FULLY_FFTed);
				if (imag_dist == 8) {
					vec[j].real = _mm_set_pd (data[j][index*4+2], data[j][index*4]);
					vec[j].imag = _mm_set_pd (data[j][index*4+3], data[j][index*4+1]);
				} else if (imag_dist == 16) {
					vec[j].real = ((VDT *) data[j])[offset+(index&1)*2];
					vec[j].imag = ((VDT *) data[j])[offset+(index&1)*2+1];
				} else {
					vec[j].real = ((VDT *) data[j])[offset+(index&1)];
					vec[j].imag = ((VDT *) data[j])[offset+(index&1)+2];
				}
			}
		}

		if (options & (POLYMULT_INVEC1_NEGATE | POLYMULT_INVEC2_NEGATE)) {
			VDT zero = broadcastsd (0.0);
			for (uint64_t j = slice_start; j < slice_end; j++) {
				vec[j].real = subpd (zero, vec[j].real);
				vec[j].imag = subpd (zero, vec[j].imag);
			}
		}

		if (options & (POLYMULT_INVEC1_RLP | POLYMULT_INVEC2_RLP)) {
			for (uint64_t j = slice_start; j < slice_end; j++) vec[-(int64_t)j] = vec[j];
		}

		if (!post_process_monics && (options & (POLYMULT_INVEC1_MONIC | POLYMULT_INVEC2_MONIC))) {
			if (slice_start == 0) {
				if (!gwdata->GENERAL_MMGW_MOD && gwdata->k == 1.0) {
					vec[size].real = broadcastsd (1.0);
					vec[size].imag = broadcastsd (0.0);
				} else {
					read_write_line_slice_data fft1_rwlsd = * (read_write_line_slice_data *) rwlsd;
					fft1_rwlsd.data = &gwdata->GW_FFT1;
					fft1_rwlsd.size = 1;
					fft1_rwlsd.vec = &vec[size];
					fft1_rwlsd.options = 0;
					fft1_rwlsd.post_process_monics = FALSE;
					read_line (pmdata, &fft1_rwlsd);
				}
			}
		}

		if (slice_end == size) break;
	}
}

void write_line_slice_strided_sse2 (pmhandle *pmdata, read_write_line_slice_data *rwlsd, CVDT *vec, uint64_t slice_start, uint64_t slice_end, uint64_t stride_offset, uint64_t stride)
{
	gwhandle *gwdata = pmdata->gwdata;
	gwnum	*data = rwlsd->data;
	gwnum	*fma_data = rwlsd->fma_data;
	int	index = rwlsd->lsd.index;
	int	options = rwlsd->options;
	int	imag_dist = rwlsd->lsd.imag_dist;
	CVDT	fmaval, oneval;

	uint64_t j_in = divide_rounding_up (slice_start + rwlsd->LSWs_skipped - stride_offset, stride);
	uint64_t j_out = j_in * stride + stride_offset - rwlsd->LSWs_skipped;

	union {
		VDT	a;
		double	b[2];
	} x;
	if (index <= -2) {		
		for ( ; j_out < slice_end; j_out += stride, j_in++) {
			if (data[j_out] == NULL) continue;
			if (options & (POLYMULT_FMADD | POLYMULT_FMSUB | POLYMULT_FNMADD)) {
				if (fma_data == NULL || fma_data[j_out] == NULL)
					fmaval.real = broadcastsd (0.0), fmaval.imag = broadcastsd (0.0);
				else
					fmaval.real = _mm_set_pd (fma_data[j_out][index*2-10], fma_data[j_out][index*2-9]),
					fmaval.imag = _mm_set_pd (fma_data[j_out][index*2-16], fma_data[j_out][index*2-15]);
				if (gwdata->GENERAL_MMGW_MOD || gwdata->k != 1.0) {
					oneval.real = _mm_set_pd (gwdata->GW_FFT1[index*2-10], gwdata->GW_FFT1[index*2-9]),
					oneval.imag = _mm_set_pd (gwdata->GW_FFT1[index*2-16], gwdata->GW_FFT1[index*2-15]);
					cvmul (fmaval, fmaval, oneval);
				}
				if (options & POLYMULT_FMADD) cvadd (vec[j_in], vec[j_in], fmaval);
				else if (options & POLYMULT_FMSUB) cvsub (vec[j_in], vec[j_in], fmaval);
				else cvsub (vec[j_in], fmaval, vec[j_in]);
			}
			x.a = vec[j_in].real;
			data[j_out][index*2-10] = x.b[1];
			data[j_out][index*2-9] = x.b[0];
			x.a = vec[j_in].imag;
			data[j_out][index*2-16] = x.b[1];
			data[j_out][index*2-15] = x.b[0];
		}
	}
	else if (index == -1) {		
		for ( ; j_out < slice_end; j_out += stride, j_in++) {
			if (data[j_out] == NULL) continue;
			if (options & (POLYMULT_FMADD | POLYMULT_FMSUB | POLYMULT_FNMADD)) {
				if (fma_data == NULL || fma_data[j_out] == NULL) fmaval.real = broadcastsd (0.0), fmaval.imag = broadcastsd (0.0);
				else fmaval.real = _mm_set_pd (fma_data[j_out][-12], fma_data[j_out][0]);
				if (gwdata->GENERAL_MMGW_MOD || gwdata->k != 1.0) {
					oneval.real = _mm_set_pd (gwdata->GW_FFT1[-12], gwdata->GW_FFT1[0]);
					fmaval.real = mulpd (fmaval.real, oneval.real);
				}
				if (options & POLYMULT_FMADD) vec[j_in].real = addpd (vec[j_in].real, fmaval.real);
				else if (options & POLYMULT_FMSUB) vec[j_in].real = subpd (vec[j_in].real, fmaval.real);
				else vec[j_in].real = subpd (fmaval.real, vec[j_in].real);
			}
			
			x.a = vec[j_in].real;
			data[j_out][0] = x.b[0];
			
			if (gwdata->ZERO_PADDED_FFT) data[j_out][-12] = x.b[1];
		}
	}
	else if (index == 0 && !gwdata->NEGACYCLIC_FFT) {		
		for ( ; j_out < slice_end; j_out += stride, j_in++) {
			if (data[j_out] == NULL) continue;
			if (options & (POLYMULT_FMADD | POLYMULT_FMSUB | POLYMULT_FNMADD)) {
				if (fma_data == NULL || fma_data[j_out] == NULL)
					fmaval.real = broadcastsd (0.0), fmaval.imag = broadcastsd (0.0);
				else if (imag_dist == 8)
					fmaval.real = _mm_set_pd (fma_data[j_out][2], fma_data[j_out][1]), fmaval.imag = _mm_set_pd (fma_data[j_out][3], 0.0);
				else if (imag_dist == 16)
					fmaval.real = _mm_set_pd (fma_data[j_out][1], fma_data[j_out][2]), fmaval.imag = _mm_set_pd (fma_data[j_out][3], 0.0);
				else
					fmaval.real = _mm_set_pd (fma_data[j_out][1], fma_data[j_out][4]), fmaval.imag = _mm_set_pd (fma_data[j_out][5], 0.0);
				if (gwdata->GENERAL_MMGW_MOD || gwdata->k != 1.0) {
					if (imag_dist == 8)
						oneval.real = _mm_set_pd (gwdata->GW_FFT1[2], gwdata->GW_FFT1[1]),
						oneval.imag = _mm_set_pd (gwdata->GW_FFT1[3], 0.0);
					else if (imag_dist == 16)
						oneval.real = _mm_set_pd (gwdata->GW_FFT1[1], gwdata->GW_FFT1[2]),
						oneval.imag = _mm_set_pd (gwdata->GW_FFT1[3], 0.0);
					else
						oneval.real = _mm_set_pd (gwdata->GW_FFT1[1], gwdata->GW_FFT1[4]),
						oneval.imag = _mm_set_pd (gwdata->GW_FFT1[5], 0.0);
					cvmul (fmaval, fmaval, oneval);
				}
				if (options & POLYMULT_FMADD) cvadd (vec[j_in], vec[j_in], fmaval);
				else if (options & POLYMULT_FMSUB) cvsub (vec[j_in], vec[j_in], fmaval);
				else cvsub (vec[j_in], fmaval, vec[j_in]);
			}
			if (imag_dist == 8) {
				x.a = vec[j_in].real, data[j_out][2] = x.b[1], data[j_out][1] = x.b[0];
				x.a = vec[j_in].imag, data[j_out][3] = x.b[1];
			} else if (imag_dist == 16) {
				x.a = vec[j_in].real, data[j_out][1] = x.b[1], data[j_out][2] = x.b[0];
				x.a = vec[j_in].imag, data[j_out][3] = x.b[1];
			} else {
				x.a = vec[j_in].real, data[j_out][1] = x.b[1], data[j_out][4] = x.b[0];
				x.a = vec[j_in].imag, data[j_out][5] = x.b[1];
			}
		}
	}
	else {				
		unsigned long offset = ((read_write_line_slice_data *) rwlsd)->lsd.offset;
		for ( ; j_out < slice_end; j_out += stride, j_in++) {
			if (data[j_out] == NULL) continue;
			if (options & (POLYMULT_FMADD | POLYMULT_FMSUB | POLYMULT_FNMADD)) {
				if (fma_data == NULL || fma_data[j_out] == NULL)
					fmaval.real = broadcastsd (0.0), fmaval.imag = broadcastsd (0.0);
				else if (imag_dist == 8)
					fmaval.real = _mm_set_pd (fma_data[j_out][index*4+2], fma_data[j_out][index*4]),
					fmaval.imag = _mm_set_pd (fma_data[j_out][index*4+3], fma_data[j_out][index*4+1]);
				else if (imag_dist == 16)
					fmaval.real = ((VDT *) fma_data[j_out])[offset+(index&1)*2],
					fmaval.imag = ((VDT *) fma_data[j_out])[offset+(index&1)*2+1];
				else 
					fmaval.real = ((VDT *) fma_data[j_out])[offset+(index&1)],
					fmaval.imag = ((VDT *) fma_data[j_out])[offset+(index&1)+2];
				if (gwdata->GENERAL_MMGW_MOD || gwdata->k != 1.0) {
					if (imag_dist == 8)
						oneval.real = _mm_set_pd (gwdata->GW_FFT1[index*4+2], gwdata->GW_FFT1[index*4]),
						oneval.imag = _mm_set_pd (gwdata->GW_FFT1[index*4+3], gwdata->GW_FFT1[index*4+1]);
					else if (imag_dist == 16)
						oneval.real = ((VDT *) gwdata->GW_FFT1)[offset+(index&1)*2],
						oneval.imag = ((VDT *) gwdata->GW_FFT1)[offset+(index&1)*2+1];
					else 
						oneval.real = ((VDT *) gwdata->GW_FFT1)[offset+(index&1)],
						oneval.imag = ((VDT *) gwdata->GW_FFT1)[offset+(index&1)+2];
					cvmul (fmaval, fmaval, oneval);
				}
				if (options & POLYMULT_FMADD) cvadd (vec[j_in], vec[j_in], fmaval);
				else if (options & POLYMULT_FMSUB) cvsub (vec[j_in], vec[j_in], fmaval);
				else cvsub (vec[j_in], fmaval, vec[j_in]);
			}
			if (imag_dist == 8) {
				x.a = vec[j_in].real, data[j_out][index*4+2] = x.b[1], data[j_out][index*4] = x.b[0];
				x.a = vec[j_in].imag, data[j_out][index*4+3] = x.b[1], data[j_out][index*4+1] = x.b[0];
			} else if (imag_dist == 16) {
				((VDT *) data[j_out])[offset+(index&1)*2] = vec[j_in].real;
				((VDT *) data[j_out])[offset+(index&1)*2+1] = vec[j_in].imag;
			} else {
				((VDT *) data[j_out])[offset+(index&1)] = vec[j_in].real;
				((VDT *) data[j_out])[offset+(index&1)+2] = vec[j_in].imag;
			}
		}
	}
}

void write_line_slice_sse2 (int helper_num, pmhandle *pmdata, void *rwlsd)
{
	gwhandle *gwdata = pmdata->gwdata;
	uint64_t size = ((read_write_line_slice_data *) rwlsd)->size;
	uint64_t slice_size = ((read_write_line_slice_data *) rwlsd)->slice_size;
	CVDT	*vec = ((read_write_line_slice_data *) rwlsd)->vec;

	for ( ; ; ) {
		uint64_t slice_start, slice_end;
		
		if (slice_size == size) {
			slice_start = 0;
			slice_end = size;
		} else {
			int64_t atomic_val = atomic_fetch_incr (((read_write_line_slice_data *) rwlsd)->blknum);
			slice_start = atomic_val * slice_size;
			if (slice_start >= size) break;
			slice_end = slice_start + slice_size;
			if (slice_end > size) slice_end = size;
		}

		write_line_slice_strided (pmdata, (read_write_line_slice_data *) rwlsd, vec, slice_start, slice_end, 0, 1);

		if (slice_end == size) break;
	}
}

#elif !defined (X86_64)				

void prep_line (gwhandle *gwdata, int line, read_write_line_slice_data *rwlsd)
{
	
	if (gwdata->GENERAL_MMGW_MOD) {
		int cyclic_lines = gwdata->FFTLEN * sizeof (double) / sizeof (CVDT) + 1;
		if (line < cyclic_lines) prep_line (gwdata->cyclic_gwdata, line, rwlsd);
		else {
			prep_line (gwdata->negacyclic_gwdata, line - cyclic_lines, rwlsd);
			rwlsd->lsd.line += cyclic_lines;
			rwlsd->lsd.index += cyclic_lines;
			rwlsd->lsd.offset += round_up_to_multiple_of (gwnum_datasize(gwdata->cyclic_gwdata) + GW_HEADER_SIZE(gwdata->negacyclic_gwdata), 128) / sizeof (VDT);
		}
		return;
	}

	int index = line;
	if (!gwdata->NEGACYCLIC_FFT) index--;

	if (gwdata->ZERO_PADDED_FFT) index -= 7;

	ASSERTG (index * 2 < (int) gwdata->FFTLEN);

	unsigned long offset;
	if (index >= 1 || (index == 0 && gwdata->NEGACYCLIC_FFT)) offset = cache_line_offset (gwdata, index / 4) / sizeof (VDT);

	rwlsd->lsd.line = line;
	rwlsd->lsd.index = index;
	rwlsd->lsd.offset = offset;
}

void read_line_slice_dbl (int helper_num, pmhandle *pmdata, void *rwlsd)
{
	gwhandle *gwdata = pmdata->gwdata;
	gwnum	*data = ((read_write_line_slice_data *) rwlsd)->data;
	uint64_t size = ((read_write_line_slice_data *) rwlsd)->size;
	int	index = ((read_write_line_slice_data *) rwlsd)->lsd.index;
	int	options = ((read_write_line_slice_data *) rwlsd)->options;
	bool	post_process_monics = ((read_write_line_slice_data *) rwlsd)->post_process_monics;
	uint64_t slice_size = ((read_write_line_slice_data *) rwlsd)->slice_size;

	for ( ; ; ) {
		uint64_t slice_start, slice_end;
		
		if (slice_size == size) {
			slice_start = 0;
			slice_end = size;
		} else {
			int64_t atomic_val = atomic_fetch_incr (((read_write_line_slice_data *) rwlsd)->blknum);
			slice_start = atomic_val * slice_size;
			if (slice_start >= size) break;
			slice_end = slice_start + slice_size;
			if (slice_end > size) slice_end = size;
		}

		CVDT *vec = ((read_write_line_slice_data *) rwlsd)->vec;

		if (!post_process_monics && (options & (POLYMULT_INVEC1_MONIC | POLYMULT_INVEC2_MONIC)) && (options & (POLYMULT_INVEC1_RLP | POLYMULT_INVEC2_RLP))) {
			if (slice_start == 0) {
				if (!gwdata->GENERAL_MMGW_MOD && gwdata->k == 1.0) {
					vec[0].real = broadcastsd (1.0);
					vec[0].imag = broadcastsd (0.0);
				} else {
					read_write_line_slice_data fft1_rwlsd = * (read_write_line_slice_data *) rwlsd;
					fft1_rwlsd.data = &gwdata->GW_FFT1;
					fft1_rwlsd.size = 1;
					fft1_rwlsd.vec = vec;
					fft1_rwlsd.options = 0;
					fft1_rwlsd.post_process_monics = FALSE;
					read_line (pmdata, &fft1_rwlsd);
				}
			}
			vec++;
		}

		if (options & (POLYMULT_INVEC1_RLP | POLYMULT_INVEC2_RLP)) vec += (size-1);

		if (index <= -2) {			
			for (uint64_t j = slice_start; j < slice_end; j++) {
				if (data[j] == NULL) { memset (&vec[j], 0, sizeof (CVDT)); continue; }
				
				vec[j].real = data[j][index-10];
				vec[j].imag = data[j][index-17];
			}
		}
		else if (index == -1) {			
			for (uint64_t j = slice_start; j < slice_end; j++) {
				if (data[j] == NULL) { memset (&vec[j], 0, sizeof (CVDT)); continue; }
				vec[j].real = data[j][0];
				vec[j].imag = 0.0;
			}
		}
		else if (index == 0 && !gwdata->NEGACYCLIC_FFT) {			
			for (uint64_t j = slice_start; j < slice_end; j++) {
				if (data[j] == NULL) { memset (&vec[j], 0, sizeof (CVDT)); continue; }
				if (gwdata->PASS2_SIZE == 0) vec[j].real = data[j][1];
				else vec[j].real = data[j][2];
				vec[j].imag = 0.0;
			}
		}
		else {				
			unsigned long offset = ((read_write_line_slice_data *) rwlsd)->lsd.offset;
			for (uint64_t j = slice_start; j < slice_end; j++) {
				if (data[j] == NULL) { memset (&vec[j], 0, sizeof (CVDT)); continue; }
				ASSERTG (FFT_state (data[j]) == FULLY_FFTed);
				if (gwdata->PASS2_SIZE == 0 && index < 8 && !gwdata->NEGACYCLIC_FFT) {
					vec[j].real = data[j][offset+(index&3)*2];
					vec[j].imag = data[j][offset+(index&3)*2+1];
				} else {
					vec[j].real = data[j][offset+(index&2)*2+(index&1)];
					vec[j].imag = data[j][offset+(index&2)*2+(index&1)+2];
				}
			}
		}

		if (options & (POLYMULT_INVEC1_NEGATE | POLYMULT_INVEC2_NEGATE)) {
			for (uint64_t j = slice_start; j < slice_end; j++) {
				vec[j].real = -vec[j].real;
				vec[j].imag = -vec[j].imag;
			}
		}

		if (options & (POLYMULT_INVEC1_RLP | POLYMULT_INVEC2_RLP)) {
			for (uint64_t j = slice_start; j < slice_end; j++) vec[-(int64_t)j] = vec[j];
		}

		if (!post_process_monics && (options & (POLYMULT_INVEC1_MONIC | POLYMULT_INVEC2_MONIC))) {
			if (slice_start == 0) {
				if (!gwdata->GENERAL_MMGW_MOD && gwdata->k == 1.0) {
					vec[size].real = broadcastsd (1.0);
					vec[size].imag = broadcastsd (0.0);
				} else {
					read_write_line_slice_data fft1_rwlsd = * (read_write_line_slice_data *) rwlsd;
					fft1_rwlsd.data = &gwdata->GW_FFT1;
					fft1_rwlsd.size = 1;
					fft1_rwlsd.vec = &vec[size];
					fft1_rwlsd.options = 0;
					fft1_rwlsd.post_process_monics = FALSE;
					read_line (pmdata, &fft1_rwlsd);
				}
			}
		}

		if (slice_end == size) break;
	}
}

void write_line_slice_strided_dbl (pmhandle *pmdata, read_write_line_slice_data *rwlsd, CVDT *vec, uint64_t slice_start, uint64_t slice_end, uint64_t stride_offset, uint64_t stride)
{
	gwhandle *gwdata = pmdata->gwdata;
	gwnum	*data = rwlsd->data;
	gwnum	*fma_data = rwlsd->fma_data;
	int	index = rwlsd->lsd.index;
	int	options = rwlsd->options;
	CVDT	fmaval, oneval;

	uint64_t j_in = divide_rounding_up (slice_start + rwlsd->LSWs_skipped - stride_offset, stride);
	uint64_t j_out = j_in * stride + stride_offset - rwlsd->LSWs_skipped;

	if (index <= -2) {			
		for ( ; j_out < slice_end; j_out += stride, j_in++) {
			if (data[j_out] == NULL) continue;
			if (options & (POLYMULT_FMADD | POLYMULT_FMSUB | POLYMULT_FNMADD)) {
				if (fma_data == NULL || fma_data[j_out] == NULL) fmaval.real = broadcastsd (0.0), fmaval.imag = broadcastsd (0.0);
				else fmaval.real = fma_data[j_out][index-10], fmaval.imag = fma_data[j_out][index-17];
				if (gwdata->GENERAL_MMGW_MOD || gwdata->k != 1.0) {
					oneval.real = gwdata->GW_FFT1[index-10], oneval.imag = gwdata->GW_FFT1[index-17];
					cvmul (fmaval, fmaval, oneval);
				}
				if (options & POLYMULT_FMADD) cvadd (vec[j_in], vec[j_in], fmaval);
				else if (options & POLYMULT_FMSUB) cvsub (vec[j_in], vec[j_in], fmaval);
				else cvsub (vec[j_in], fmaval, vec[j_in]);
			}
			data[j_out][index-10] = vec[j_in].real;
			data[j_out][index-17] = vec[j_in].imag;
		}
	}
	else if (index == -1) {			
		for ( ; j_out < slice_end; j_out += stride, j_in++) {
			if (data[j_out] == NULL) continue;
			if (options & (POLYMULT_FMADD | POLYMULT_FMSUB | POLYMULT_FNMADD)) {
				if (fma_data == NULL || fma_data[j_out] == NULL) fmaval.real = broadcastsd (0.0);
				else fmaval.real = fma_data[j_out][0];
				if (gwdata->GENERAL_MMGW_MOD || gwdata->k != 1.0) {
					oneval.real = gwdata->GW_FFT1[0];
					fmaval.real = mulpd (fmaval.real, oneval.real);
				}
				if (options & POLYMULT_FMADD) vec[j_in].real = vec[j_in].real + fmaval.real;
				else if (options & POLYMULT_FMSUB) vec[j_in].real = vec[j_in].real - fmaval.real;
				else vec[j_in].real = fmaval.real - vec[j_in].real;
			}
			data[j_out][0] = vec[j_in].real;
		}
	}
	else if (index == 0 && !gwdata->NEGACYCLIC_FFT) {		
		for ( ; j_out < slice_end; j_out += stride, j_in++) {
			if (data[j_out] == NULL) continue;
			if (options & (POLYMULT_FMADD | POLYMULT_FMSUB | POLYMULT_FNMADD)) {
				if (fma_data == NULL || fma_data[j_out] == NULL) fmaval.real = broadcastsd (0.0);
				else if (gwdata->PASS2_SIZE == 0) fmaval.real = fma_data[j_out][1];
				else fmaval.real = fma_data[j_out][2];
				if (gwdata->GENERAL_MMGW_MOD || gwdata->k != 1.0) {
					oneval.real = gwdata->GW_FFT1[2];
					fmaval.real = mulpd (fmaval.real, oneval.real);
				}
				if (options & POLYMULT_FMADD) vec[j_in].real = vec[j_in].real + fmaval.real;
				else if (options & POLYMULT_FMSUB) vec[j_in].real = vec[j_in].real - fmaval.real;
				else vec[j_in].real = fmaval.real - vec[j_in].real;
			}
			if (gwdata->PASS2_SIZE == 0) data[j_out][1] = vec[j_in].real;
			else data[j_out][2] = vec[j_in].real;
		}
	}
	else {				
		unsigned long offset = ((read_write_line_slice_data *) rwlsd)->lsd.offset;
		for ( ; j_out < slice_end; j_out += stride, j_in++) {
			if (data[j_out] == NULL) continue;
			if (options & (POLYMULT_FMADD | POLYMULT_FMSUB | POLYMULT_FNMADD)) {
				if (fma_data == NULL || fma_data[j_out] == NULL) fmaval.real = broadcastsd (0.0), fmaval.imag = broadcastsd (0.0);
				else if (gwdata->PASS2_SIZE == 0 && index < 8 && !gwdata->NEGACYCLIC_FFT)
					fmaval.real = fma_data[j_out][offset+(index&3)*2], fmaval.imag = fma_data[j_out][offset+(index&3)*2+1];
				else
					fmaval.real = fma_data[j_out][offset+(index&2)*2+(index&1)], fmaval.imag = fma_data[j_out][offset+(index&2)*2+(index&1)+2];
				if (gwdata->GENERAL_MMGW_MOD || gwdata->k != 1.0) {
					if (gwdata->PASS2_SIZE == 0 && index < 8 && !gwdata->NEGACYCLIC_FFT)
						oneval.real = gwdata->GW_FFT1[offset+(index&3)*2], oneval.imag = gwdata->GW_FFT1[offset+(index&3)*2+1];
					else
						oneval.real = gwdata->GW_FFT1[offset+(index&2)*2+(index&1)], oneval.imag = gwdata->GW_FFT1[offset+(index&2)*2+(index&1)+2];
					cvmul (fmaval, fmaval, oneval);
				}
				if (options & POLYMULT_FMADD) cvadd (vec[j_in], vec[j_in], fmaval);
				else if (options & POLYMULT_FMSUB) cvsub (vec[j_in], vec[j_in], fmaval);
				else cvsub (vec[j_in], fmaval, vec[j_in]);
			}
			if (gwdata->PASS2_SIZE == 0 && index < 8 && !gwdata->NEGACYCLIC_FFT) {
				data[j_out][offset+(index&3)*2] = vec[j_in].real;
				data[j_out][offset+(index&3)*2+1] = vec[j_in].imag;
			} else {
				data[j_out][offset+(index&2)*2+(index&1)] = vec[j_in].real;
				data[j_out][offset+(index&2)*2+(index&1)+2] = vec[j_in].imag;
			}
		}
	}
}

void write_line_slice_dbl (int helper_num, pmhandle *pmdata, void *rwlsd)
{
	gwhandle *gwdata = pmdata->gwdata;
	uint64_t size = ((read_write_line_slice_data *) rwlsd)->size;
	uint64_t slice_size = ((read_write_line_slice_data *) rwlsd)->slice_size;
	CVDT	*vec = ((read_write_line_slice_data *) rwlsd)->vec;

	for ( ; ; ) {
		uint64_t slice_start, slice_end;
		
		if (slice_size == size) {
			slice_start = 0;
			slice_end = size;
		} else {
			int64_t atomic_val = atomic_fetch_incr (((read_write_line_slice_data *) rwlsd)->blknum);
			slice_start = atomic_val * slice_size;
			if (slice_start >= size) break;
			slice_end = slice_start + slice_size;
			if (slice_end > size) slice_end = size;
		}

		write_line_slice_strided (pmdata, (read_write_line_slice_data *) rwlsd, vec, slice_start, slice_end, 0, 1);

		if (slice_end == size) break;
	}
}

#endif

#if defined (AVX512) || defined (FMA) || defined (AVX) || defined (SSE2) || !defined (X86_64)

void brute_line (
	CVDT	*invec1,		
	uint64_t invec1_size,		
	CVDT	*invec2,		
	uint64_t invec2_size,		
	CVDT	*outvec,		
	uint64_t circular_size,		
	gwnum	*gw_outvec,		
	uint64_t gw_outvec_size,	
	int	adjusted_shift,		
	uint64_t LSWs_skipped)		
{
	
	for (int out = 0; out < invec1_size + invec2_size - 1; out++) {
		
		int adjusted_out = out + adjusted_shift;
		bool first_time = TRUE;
		if (adjusted_out >= circular_size) {
			adjusted_out = adjusted_out % circular_size;
			first_time = FALSE;
		}
		
		if (gw_outvec != NULL &&
		    (adjusted_out < LSWs_skipped || adjusted_out >= LSWs_skipped + gw_outvec_size || gw_outvec[adjusted_out - LSWs_skipped] == NULL)) continue;
		
		uint64_t in1 = out < invec2_size ? 0 : out - invec2_size + 1;	
		int64_t in2 = out - in1;					
		
		for ( ; in1 < invec1_size && in2 >= 0; in1++, in2--) {
			CVDT tmp;
			cvmul (tmp, invec1[in1], invec2[in2]);
			if (first_time) {
				outvec[adjusted_out] = tmp;
				first_time = FALSE;
			} else {
				cvadd (outvec[adjusted_out], outvec[adjusted_out], tmp);
			}
		}
	}
}

void karatsuba_line (
	pmhandle *pmdata,		
	CVDT	*invec1,		
	uint64_t invec1_size,		
	CVDT	*invec2,		
	uint64_t invec2_size,		
	CVDT	*tmp,			
	CVDT	*outvec)		
{
	uint64_t i, a_size, b_size, c_size, d_size, a_plus_b_size, c_plus_d_size, bd_size, ac_size, a_plus_b_times_c_plus_d_size;
	CVDT	*a, *b, *c, *d, *ac, *bd, *a_plus_b, *c_plus_d, *a_plus_b_times_c_plus_d;

	if (invec1_size == 1 || invec2_size == 1 || invec1_size + invec2_size < pmdata->KARAT_BREAK) {
		brute_line (invec1, invec1_size, invec2, invec2_size, outvec, invec1_size + invec2_size - 1, NULL, 0, 0, 0);
		return;
	}

	if (invec1_size > invec2_size) {
		uint64_t tmpsize = invec1_size; invec1_size = invec2_size; invec2_size = tmpsize;
		CVDT *tmpvec = invec1; invec1 = invec2; invec2 = tmpvec;
	}

	d_size = (invec2_size + 1) / 2;
	c_size = invec2_size - d_size;
	c = invec2 + d_size, d = invec2;

	if (d_size >= invec1_size) {
		
		bd = outvec;
		karatsuba_line (pmdata, invec1, invec1_size, d, d_size, tmp, bd);
		bd_size = invec1_size + d_size - 1;
		
		CVDT *bc = tmp;
		uint64_t bc_size = invec1_size + c_size - 1;
		tmp += bc_size;
		karatsuba_line (pmdata, invec1, invec1_size, c, c_size, tmp, bc);
		
		for (i = 0; i < invec1_size - 1; i++) cvadd (outvec[i + d_size], outvec[i + d_size], bc[i]);
		for ( ; i < bc_size; i++) outvec[i + d_size] = bc[i];
		return;
	}

	b_size = d_size;
	a_size = invec1_size - b_size;
	a = invec1 + b_size, b = invec1;

	a_plus_b = outvec;
	for (i = 0; i < a_size && i < b_size; i++) cvadd (a_plus_b[i], a[i], b[i]);

	for ( ; i < b_size; i++) a_plus_b[i] = b[i];		
	a_plus_b_size = i;
	
	c_plus_d = &outvec[i];
	for (i = 0; i < c_size && i < d_size; i++) cvadd (c_plus_d[i], c[i], d[i]);

	for ( ; i < d_size; i++) c_plus_d[i] = d[i];		
	c_plus_d_size = i;
	
	a_plus_b_times_c_plus_d = tmp;
	a_plus_b_times_c_plus_d_size = a_plus_b_size + c_plus_d_size - 1;
	tmp += a_plus_b_times_c_plus_d_size;
	karatsuba_line (pmdata, a_plus_b, a_plus_b_size, c_plus_d, c_plus_d_size, tmp, a_plus_b_times_c_plus_d);

	bd = outvec;
	karatsuba_line (pmdata, b, b_size, d, d_size, tmp, bd);
	bd_size = b_size + d_size - 1;
	
	memset (&outvec[bd_size], 0, sizeof (CVDT));
	
	ac = &outvec[bd_size+1];
	karatsuba_line (pmdata, a, a_size, c, c_size, tmp, ac);
	ac_size = a_size + c_size - 1;

	for (i = 0; i < ac_size; i++) cvsub (a_plus_b_times_c_plus_d[i], a_plus_b_times_c_plus_d[i], ac[i]);
	
	for (i = 0; i < bd_size; i++) cvsub (a_plus_b_times_c_plus_d[i], a_plus_b_times_c_plus_d[i], bd[i]);
	
	for (i = 0; i < a_plus_b_times_c_plus_d_size; i++) cvadd (outvec[i + b_size], outvec[i + b_size], a_plus_b_times_c_plus_d[i]);
}

#ifndef FMA

void read_preprocess_line_slice (int helper_num, pmhandle *pmdata, void *rwlsd)
{
	gwhandle *gwdata = pmdata->gwdata;
	gwnum	*data = ((read_write_line_slice_data *) rwlsd)->data;
	uint64_t size = ((read_write_line_slice_data *) rwlsd)->size;
	int	line = ((read_write_line_slice_data *) rwlsd)->lsd.line;
	int	options = ((read_write_line_slice_data *) rwlsd)->options;
	bool	post_process_monics = ((read_write_line_slice_data *) rwlsd)->post_process_monics;

	preprocessed_poly_header *hdr;  
	CVDT	*first_element;		
	CVDT	*element;		
	uint64_t slice_size;		

	hdr = (preprocessed_poly_header *) ((char *) data - sizeof (gwarray_header));
	first_element = (CVDT *) round_up_to_multiple_of ((intptr_t) hdr + sizeof (preprocessed_poly_header), 64);

	int index_to_read = line;
	if (!(hdr->options & POLYMULT_PRE_FFT) && !gwdata->NEGACYCLIC_FFT && !gwdata->ZERO_PADDED_FFT) {
		index_to_read--;
		if (index_to_read < 0) index_to_read = 0;
	}
	element = (CVDT *) ((char *) first_element + index_to_read * hdr->element_size);

	if (hdr->options & POLYMULT_PRE_FFT) size = hdr->line_size;
	else ASSERTG (size == hdr->line_size);

	if (pmdata->mt_polymult_line) slice_size = size;
	else if (!(hdr->options & POLYMULT_PRE_COMPRESS)) slice_size = divide_rounding_up (size, 512);
	else slice_size = hdr->line_size / hdr->num_compressed_blks;

	for ( ; ; ) {
		uint64_t slice_start, slice_end;
		
		if (slice_size == size) {
			slice_start = 0;
			slice_end = size;
		} else {
			int64_t atomic_val = atomic_fetch_incr (((read_write_line_slice_data *) rwlsd)->blknum);
			slice_start = atomic_val * slice_size;
			if (slice_start >= size) break;
			slice_end = slice_start + slice_size;
			if (slice_end > size) slice_end = size;
		}

		CVDT *vec = ((read_write_line_slice_data *) rwlsd)->vec;

		if (hdr->options & POLYMULT_PRE_FFT) {
			if (!(hdr->options & POLYMULT_PRE_COMPRESS))
				memcpy (&vec[slice_start], &element[slice_start], (size_t) (slice_end - slice_start) * sizeof (CVDT));
			else
				decompress_line_slice ((char *) element, (char *) &vec[slice_start], hdr, slice_start, slice_end, sizeof (CVDT));
			if (slice_end == size) break;
			else continue;
		}

		if (!post_process_monics && (options & (POLYMULT_INVEC1_MONIC | POLYMULT_INVEC2_MONIC)) && (options & (POLYMULT_INVEC1_RLP | POLYMULT_INVEC2_RLP))) {
			if (slice_start == 0) {
				if (!gwdata->GENERAL_MMGW_MOD && gwdata->k == 1.0) {
					vec[0].real = broadcastsd (1.0);
					vec[0].imag = broadcastsd (0.0);
				} else {
					read_write_line_slice_data fft1_rwlsd;
					prep_line (pmdata->gwdata, line, &fft1_rwlsd);
					fft1_rwlsd.data = &gwdata->GW_FFT1;
					fft1_rwlsd.size = 1;
					fft1_rwlsd.vec = vec;
					fft1_rwlsd.options = 0;
					fft1_rwlsd.post_process_monics = FALSE;
					read_line (pmdata, &fft1_rwlsd);
				}
			}
			vec++;
		}

		if (options & (POLYMULT_INVEC1_RLP | POLYMULT_INVEC2_RLP)) vec += (size-1);

		ASSERTG ((hdr->options & POLYMULT_PRE_COMPRESS) || hdr->element_size == size * sizeof (CVDT));
		if (!(hdr->options & POLYMULT_PRE_COMPRESS)) memcpy (&vec[slice_start], &element[slice_start], (size_t) (slice_end - slice_start) * sizeof (CVDT));
		else decompress_line_slice ((char *) element, (char *) &vec[slice_start], hdr, slice_start, slice_end, sizeof (CVDT));

		if (!gwdata->NEGACYCLIC_FFT && !gwdata->ZERO_PADDED_FFT && index_to_read == 0) {
			for (uint64_t j = slice_start; j < slice_end; j++) {
				union { VDT a; double b[VLEN / sizeof(double)]; } r, i;
				r.a = vec[j].real;
				i.a = vec[j].imag;
				if (line == 0) {
					memset (&r, 0, sizeof (r));
					r.b[0] = i.b[0];
					memset (&i, 0, sizeof (i));
				} else {
					i.b[0] = 0.0;
				}
				vec[j].real = r.a;
				vec[j].imag = i.a;
			}
		}

		if (options & (POLYMULT_INVEC1_RLP | POLYMULT_INVEC2_RLP)) {
			for (uint64_t j = slice_start; j < slice_end; j++) vec[-(int64_t)j] = vec[j];
		}

		if (!post_process_monics && (options & (POLYMULT_INVEC1_MONIC | POLYMULT_INVEC2_MONIC))) {
			if (slice_start == 0) {
				if (!gwdata->GENERAL_MMGW_MOD && gwdata->k == 1.0) {
					vec[size].real = broadcastsd (1.0);
					vec[size].imag = broadcastsd (0.0);
				} else {
					read_write_line_slice_data fft1_rwlsd;
					prep_line (pmdata->gwdata, line, &fft1_rwlsd);
					fft1_rwlsd.data = &gwdata->GW_FFT1;
					fft1_rwlsd.size = 1;
					fft1_rwlsd.vec = &vec[size];
					fft1_rwlsd.options = 0;
					fft1_rwlsd.post_process_monics = FALSE;
					read_line (pmdata, &fft1_rwlsd);
				}
			}
		}

		if (slice_end == size) break;
	}
}

void read_line (pmhandle *pmdata, read_write_line_slice_data *rwlsd)
{
	
	if (!is_preprocessed_poly (rwlsd->data)) {
		if (pmdata->mt_polymult_line || rwlsd->size < 512) {
			rwlsd->slice_size = rwlsd->size;
			read_line_slice (0, pmdata, rwlsd);
		} else {
			
			rwlsd->slice_size = divide_rounding_up (rwlsd->size, 512);
			atomic_set (rwlsd->blknum, 0);
			pmdata->internal_callback = &read_line_slice;
			pmdata->internal_callback_data = rwlsd;
			polymult_launch_helpers (pmdata);
		}
	}
	
	else {
		if (pmdata->mt_polymult_line) {
			read_preprocess_line_slice (0, pmdata, rwlsd);
		} else {
			atomic_set (rwlsd->blknum, 0);
			pmdata->internal_callback = &read_preprocess_line_slice;
			pmdata->internal_callback_data = rwlsd;
			polymult_launch_helpers (pmdata);
		}
	}
}

void write_line (pmhandle *pmdata, read_write_line_slice_data *rwlsd)
{
	
	if (pmdata->mt_polymult_line) {
		rwlsd->slice_size = rwlsd->size;
		write_line_slice (0, pmdata, rwlsd);
	} else {
		
		rwlsd->slice_size = divide_rounding_up (rwlsd->size, 512);
		atomic_set (rwlsd->blknum, 0);
		pmdata->internal_callback = &write_line_slice;
		pmdata->internal_callback_data = rwlsd;
		polymult_launch_helpers (pmdata);
	}
}

#endif

#define	FORWARD_FFT_INVEC1	0x1	
#define	FORWARD_FFT_INVEC2	0x2	
#define FORWARD_FFT_ONLY	0x4	

typedef struct {
	read_write_line_slice_data *rwlsd1;	
	read_write_line_slice_data *rwlsd2;	
	read_write_line_slice_data *rwlsd3;	
	CVDT	*invec1;			
	CVDT	*invec2;			
	CVDT	*outvec;			
	CVDT	*scratch;			
	int	options;			
	bool	multithreading;			
	bool	write_direct;			
	uint64_t invec1_size;			
	uint64_t invec2_size;			
	uint64_t fft_size;			
	unsigned int powers_of_two;		
	uint32_t pass1_size;			
	uint32_t pass2_size;			
	uint64_t starting_twiddle3_stride;	
	uint64_t starting_twiddle45_stride;	
	unsigned int pass;			
	unsigned int pass_size;			
	unsigned int pass_stride;		
	unsigned int num_exterior_blks;		
	unsigned int num_interior_blks;		
	VDT	inv_fft_size;			
	VDT	inv_fft_size_707;		
	gwatomic blknum;			
} fft_line_pass_data;

void fft_line_pass (
	int	helper_num,		
	pmhandle *pmdata,		
	void	*pdarg)			
{
	fft_line_pass_data *pd = (fft_line_pass_data *) pdarg;	
	CVDT	*scratch;		
	int	non_atomic_val = 0;	

	scratch = pd->scratch + helper_num * pd->pass1_size;

	for ( ; ; ) {
		unsigned int blknum;

		if (pd->multithreading) blknum = (int) atomic_fetch_incr (pd->blknum);
		else blknum = non_atomic_val++;
		if (blknum >= pd->num_exterior_blks * pd->num_interior_blks) break;

#ifdef THREE_PASS_FFTS_SUPPORTED_SOMEDAY
		unsigned int blke = blknum / pd->num_exterior_blks;			
		unsigned int blki = blknum - blke * pd->num_exterior_blks;		
#else
		unsigned int blke = (pd->num_exterior_blks == 1 ? 0 : blknum);		
		unsigned int blki = (pd->num_exterior_blks == 1 ? blknum : 0);		
#endif

		CVDT *src;				
		CVDT *dest;				
		unsigned int size;			
		unsigned int stride;			
		unsigned int instride1;			
		unsigned int outstride1;		
		uint64_t twiddle3_stride;		
		uint64_t twiddle45_stride;		
		uint64_t src_size;			
		bool	zpad;				

		twiddle3_stride = pd->starting_twiddle3_stride;
		twiddle45_stride = pd->starting_twiddle45_stride;

		if (pd->pass <= 2) {
		    for (int srcarg = 1; srcarg <= 2; srcarg++) {

			if (srcarg == 1 && !(pd->options & FORWARD_FFT_INVEC1)) continue;
			if (srcarg == 2 && !(pd->options & FORWARD_FFT_INVEC2)) continue;

			if (pd->pass == 1) {

src = (srcarg == 1 ? pd->rwlsd1->vec : pd->rwlsd2->vec) + blki;

				dest = scratch;
				instride1 = pd->pass2_size;
				outstride1 = 1;
				src_size = divide_rounding_up ((srcarg == 1 ? pd->invec1_size : pd->invec2_size) - blknum, instride1);
				zpad = (src_size != pd->pass_size);
			}
			else {

if (pd->pass1_size == 1) src = (srcarg == 1 ? pd->rwlsd1->vec : pd->rwlsd2->vec), dest = (srcarg == 1 ? pd->invec1 : pd->invec2);
else src = dest = (srcarg == 1 ? pd->invec1 : pd->invec2) + blke * pd->pass2_size;
				instride1 = outstride1 = 1;
				src_size = (srcarg == 1 ? pd->invec1_size : pd->invec2_size);
				zpad = (pd->pass1_size == 1 && src_size != pd->pass_size);
			}

			twiddle3_stride = pd->starting_twiddle3_stride;
			twiddle45_stride = pd->starting_twiddle45_stride;

			size = pd->pass_size;

			for ( ; size % 3 == 0; size /= 3, twiddle3_stride *= 3, src = dest, instride1 = 1, zpad = FALSE) {
			    VDT half = broadcastsd (0.5);
			    VDT _866 = broadcastsd (0.86602540378443864676372317075294);
			    stride = size / 3;
			    if (pd->pass == 1 && stride == 1) dest = (srcarg == 1 ? pd->invec1 : pd->invec2) + blki, outstride1 = pd->pass2_size;
			    for (unsigned int group = 0; group < pd->pass_size; group += stride * 3) {
				uint64_t group_member = 0;		
				uint64_t last_group_member;		

				if (!zpad) last_group_member = stride;			
				else if (src_size <= 2 * stride) last_group_member = 0;	
				else last_group_member = src_size - 2 * stride;		
				for ( ; group_member < last_group_member; group_member++) {
					uint64_t twiddle_idx = ((group_member * pd->pass_stride) + blki) * twiddle3_stride;
					CVDT a = src[instride1*(group + group_member)];
					CVDT b = src[instride1*(group + group_member + stride)];
					CVDT c = src[instride1*(group + group_member + 2*stride)];
					CVDT b_plus_c, b_minus_c, tmp23;

					cvaddsub (b_plus_c, b_minus_c, b, c);			
					cvsubfm (tmp23, a, half, b_plus_c);			

					if (twiddle_idx == 0) {	
						cviaddsubfm (b, c, tmp23, _866, b_minus_c);	
						cvadd (a, a, b_plus_c);				
						dest[outstride1*(group + group_member)] = a;
						dest[outstride1*(group + group_member + stride)] = b;
						dest[outstride1*(group + group_member + 2*stride)] = c;
					}
					else {			
						TVDT twiddle1; twidload (twiddle1, &pmdata->twiddles1[2*twiddle_idx]);
						VDT sin866 = mulpd (_866, twiddle1.sin);
						cvrmul (tmp23, twiddle1.sin, tmp23);
						cvadd (a, a, b_plus_c);				
						cviaddsubfm (b, c, tmp23, sin866, b_minus_c);	
						dest[outstride1*(group + group_member)] = a;
						twidmuldelay     (dest[outstride1*(group + group_member + stride)], b, twiddle1);
						twidconjmuldelay (dest[outstride1*(group + group_member + 2*stride)], c, twiddle1);
					}
				}
				if (group_member == stride) continue;

				ASSERTG (group == 0);  ASSERTG (outstride1 == 1); ASSERTG (instride1 == pd->pass_stride); ASSERTG (twiddle3_stride == 1);
				if (group_member != 0) last_group_member = stride;	
				else if (src_size <= stride) last_group_member = 0;	
				else last_group_member = src_size - stride;		
				for ( ; group_member < last_group_member; group_member++) {
					uint64_t twiddle_idx = group_member * instride1 + blki;
					CVDT a = src[instride1*(group_member)];
					CVDT b_in = src[instride1*(group_member + stride)];
					CVDT b, c, tmp23;

					cvsubfm (tmp23, a, half, b_in);				

					if (twiddle_idx == 0) {	
						cvadd (a, a, b_in);				
						cviaddsubfm (b, c, tmp23, _866, b_in);		
						dest[group_member] = a;
						dest[group_member + stride] = b;
						dest[group_member + 2*stride] = c;
					}
					else {			
						TVDT twiddle1; twidload (twiddle1, &pmdata->twiddles1[2*twiddle_idx]);
						VDT sin866 = mulpd (_866, twiddle1.sin);
						cvrmul (tmp23, twiddle1.sin, tmp23);
						cvadd (a, a, b_in);				
						cviaddsubfm (b, c, tmp23, sin866, b_in);	
						dest[group_member] = a;
						twidmuldelay     (dest[group_member + stride], b, twiddle1);
						twidconjmuldelay (dest[group_member + 2*stride], c, twiddle1);
					}
				}

				if (group_member != 0) last_group_member = stride;	
				else last_group_member = src_size;			
				for ( ; group_member < last_group_member; group_member++) {
					uint64_t twiddle_idx = group_member * instride1 + blki;
					CVDT a = src[instride1*(group_member)];

					if (twiddle_idx == 0) {	
						dest[group_member] = a;
						dest[group_member + stride] = a;
						dest[group_member + 2*stride] = a;
					}
					else {			
						TVDT twiddle1; twidload (twiddle1, &pmdata->twiddles1[2*twiddle_idx]);
						dest[group_member] = a;
						twidmulandconjmul (dest[group_member + stride], dest[group_member + 2*stride], a, twiddle1);
					}
				}

				for ( ; group_member < stride; group_member++) {
					memset (&dest[group_member], 0, sizeof (CVDT));
					memset (&dest[group_member + stride], 0, sizeof (CVDT));
					memset (&dest[group_member + 2*stride], 0, sizeof (CVDT));
				}
			    }
			}

			for ( ; size % 5 == 0; size /= 5, twiddle45_stride *= 5, src = dest, instride1 = 1, zpad = FALSE) {
			    VDT _309 = broadcastsd (0.30901699437494742410229341718282);
			    VDT _809 = broadcastsd (0.80901699437494742410229341718282);
			    VDT _951 = broadcastsd (0.95105651629515357211643933337938);
			    VDT _588_951 = broadcastsd (0.61803398874989484820458683436564);
			    VDT _m588 = broadcastsd (-0.58778525229247312916870595463907);
			    stride = size / 5;
			    if (pd->pass == 1 && stride == 1) dest = (srcarg == 1 ? pd->invec1 : pd->invec2) + blki, outstride1 = pd->pass2_size;
			    for (unsigned int group = 0; group < pd->pass_size; group += stride * 5) {
				uint64_t group_member = 0;		
				uint64_t last_group_member;		

				if (!zpad) last_group_member = stride;			
				else if (src_size <= 4 * stride) last_group_member = 0;	
				else last_group_member = src_size - 4 * stride;		
				for ( ; group_member < last_group_member; group_member++) {
					uint64_t twiddle_idx = ((group_member * pd->pass_stride) + blki) * twiddle45_stride;
					CVDT a = src[instride1*(group + group_member)];
					CVDT b = src[instride1*(group + group_member + stride)];
					CVDT c = src[instride1*(group + group_member + 2*stride)];
					CVDT d = src[instride1*(group + group_member + 3*stride)];
					CVDT e = src[instride1*(group + group_member + 4*stride)];

					CVDT b_plus_e, b_minus_e, c_plus_d, c_minus_d, tmp25a, tmp25b, tmp34a, tmp34b;

					cvadd (b_plus_e, b, e);					
					cvsub (b_minus_e, b, e);				
					cvadd (c_plus_d, c, d);					
					cvsub (c_minus_d, c, d);				

					cvaddfm (tmp25a, a, _309, b_plus_e);
					cvsubfm (tmp34a, a, _809, b_plus_e);
					cvadd   (a, a, b_plus_e);
					cvaddfm (tmp25b, b_minus_e, _588_951, c_minus_d);	
					cvsubfm (tmp34b, c_minus_d, _588_951, b_minus_e);	
					cvsubfm (tmp25a, tmp25a, _809, c_plus_d);		
					cvaddfm (tmp34a, tmp34a, _309, c_plus_d);		
					cvadd   (a, a, c_plus_d);				

					if (twiddle_idx == 0) {	
						cviaddsubfm (b, e, tmp25a, _951, tmp25b);	
						cvisubaddfm (c, d, tmp34a, _951, tmp34b);	
						dest[outstride1*(group + group_member)] = a;
						dest[outstride1*(group + group_member + stride)]   = b;
						dest[outstride1*(group + group_member + 2*stride)] = c;
						dest[outstride1*(group + group_member + 3*stride)] = d;
						dest[outstride1*(group + group_member + 4*stride)] = e;
					}
					else {			
						TVDT twiddle1; twidload (twiddle1, &pmdata->twiddles2[4*twiddle_idx]);
						TVDT twiddle2; twidload (twiddle2, &pmdata->twiddles2[4*twiddle_idx+2]);
						VDT sin1_951 = mulpd (_951, twiddle1.sin);
						VDT sin2_951 = mulpd (_951, twiddle2.sin);
						cvrmul   (tmp25a, twiddle1.sin, tmp25a);
						cvrmul   (tmp34a, twiddle2.sin, tmp34a);
						cviaddsubfm (b, e, tmp25a, sin1_951, tmp25b);	
						cvisubaddfm (c, d, tmp34a, sin2_951, tmp34b);	
						dest[outstride1*(group + group_member)] = a;
						twidmuldelay     (dest[outstride1*(group + group_member + stride)], b, twiddle1);
						twidmuldelay     (dest[outstride1*(group + group_member + 2*stride)], c, twiddle2);
						twidconjmuldelay (dest[outstride1*(group + group_member + 3*stride)], d, twiddle2);
						twidconjmuldelay (dest[outstride1*(group + group_member + 4*stride)], e, twiddle1);
					}
				}
				if (group_member == stride) continue;

				ASSERTG (group == 0);  ASSERTG (outstride1 == 1); ASSERTG (instride1 == pd->pass_stride); ASSERTG (twiddle45_stride == 1);
				if (group_member != 0) last_group_member = stride;	
				else if (src_size <= 3 * stride) last_group_member = 0;	
				else last_group_member = src_size - 3 * stride;		
				for ( ; group_member < last_group_member; group_member++) {
					uint64_t twiddle_idx = group_member * instride1 + blki;
					CVDT a = src[instride1*(group_member)];
					CVDT b = src[instride1*(group_member + stride)];
					CVDT c = src[instride1*(group_member + 2*stride)];
					CVDT d = src[instride1*(group_member + 3*stride)];
					CVDT e, c_plus_d, c_minus_d, tmp25a, tmp25b, tmp34a, tmp34b;

					cvadd (c_plus_d, c, d);					
					cvsub (c_minus_d, c, d);				

					cvaddfm (tmp25a, a, _309, b);
					cvsubfm (tmp34a, a, _809, b);
					cvadd   (a, a, b);
					cvaddfm (tmp25b, b, _588_951, c_minus_d);		
					cvsubfm (tmp34b, c_minus_d, _588_951, b);		
					cvsubfm (tmp25a, tmp25a, _809, c_plus_d);		
					cvaddfm (tmp34a, tmp34a, _309, c_plus_d);		
					cvadd   (a, a, c_plus_d);				

					if (twiddle_idx == 0) {	
						cviaddsubfm (b, e, tmp25a, _951, tmp25b);	
						cvisubaddfm (c, d, tmp34a, _951, tmp34b);	
						dest[group_member] = a;
						dest[group_member + stride]   = b;
						dest[group_member + 2*stride] = c;
						dest[group_member + 3*stride] = d;
						dest[group_member + 4*stride] = e;
					}
					else {			
						TVDT twiddle1; twidload (twiddle1, &pmdata->twiddles2[4*twiddle_idx]);
						TVDT twiddle2; twidload (twiddle2, &pmdata->twiddles2[4*twiddle_idx+2]);
						VDT sin1_951 = mulpd (_951, twiddle1.sin);
						VDT sin2_951 = mulpd (_951, twiddle2.sin);
						cvrmul   (tmp25a, twiddle1.sin, tmp25a);
						cvrmul   (tmp34a, twiddle2.sin, tmp34a);
						cviaddsubfm (b, e, tmp25a, sin1_951, tmp25b);	
						cvisubaddfm (c, d, tmp34a, sin2_951, tmp34b);	
						dest[group_member] = a;
						twidmuldelay     (dest[group_member + stride], b, twiddle1);
						twidmuldelay     (dest[group_member + 2*stride], c, twiddle2);
						twidconjmuldelay (dest[group_member + 3*stride], d, twiddle2);
						twidconjmuldelay (dest[group_member + 4*stride], e, twiddle1);
					}
				}

				if (group_member != 0) last_group_member = stride;	
				else if (src_size <= 2 * stride) last_group_member = 0;	
				else last_group_member = src_size - 2 * stride;		
				for ( ; group_member < last_group_member; group_member++) {
					uint64_t twiddle_idx = group_member * instride1 + blki;
					CVDT a = src[instride1*(group_member)];
					CVDT b = src[instride1*(group_member + stride)];
					CVDT c = src[instride1*(group_member + 2*stride)];
					CVDT d, e, tmp25a, tmp25b, tmp34a, tmp34b;

					cvaddfm (tmp25a, a, _309, b);
					cvsubfm (tmp34a, a, _809, b);
					cvadd   (a, a, b);
					cvaddfm (tmp25b, b, _588_951, c);			
					cvsubfm (tmp34b, c, _588_951, b);			
					cvsubfm (tmp25a, tmp25a, _809, c);			
					cvaddfm (tmp34a, tmp34a, _309, c);			
					cvadd   (a, a, c);					

					if (twiddle_idx == 0) {	
						cviaddsubfm (b, e, tmp25a, _951, tmp25b);	
						cvisubaddfm (c, d, tmp34a, _951, tmp34b);	
						dest[group_member] = a;
						dest[group_member + stride]   = b;
						dest[group_member + 2*stride] = c;
						dest[group_member + 3*stride] = d;
						dest[group_member + 4*stride] = e;
					}
					else {			
						TVDT twiddle1; twidload (twiddle1, &pmdata->twiddles2[4*twiddle_idx]);
						TVDT twiddle2; twidload (twiddle2, &pmdata->twiddles2[4*twiddle_idx+2]);
						VDT sin1_951 = mulpd (_951, twiddle1.sin);
						VDT sin2_951 = mulpd (_951, twiddle2.sin);
						cvrmul   (tmp25a, twiddle1.sin, tmp25a);
						cvrmul   (tmp34a, twiddle2.sin, tmp34a);
						cviaddsubfm (b, e, tmp25a, sin1_951, tmp25b);	
						cvisubaddfm (c, d, tmp34a, sin2_951, tmp34b);	
						dest[group_member] = a;
						twidmuldelay     (dest[group_member + stride], b, twiddle1);
						twidmuldelay     (dest[group_member + 2*stride], c, twiddle2);
						twidconjmuldelay (dest[group_member + 3*stride], d, twiddle2);
						twidconjmuldelay (dest[group_member + 4*stride], e, twiddle1);
					}
				}

				if (group_member != 0) last_group_member = stride;	
				else if (src_size <= stride) last_group_member = 0;	
				else last_group_member = src_size - stride;		
				for ( ; group_member < last_group_member; group_member++) {
					uint64_t twiddle_idx = group_member * instride1 + blki;
					CVDT a = src[instride1*(group_member)];
					CVDT b_in = src[instride1*(group_member + stride)];
					CVDT b, c, d, e, tmp25a, tmp34a;

					cvaddfm (tmp25a, a, _309, b_in);			
					cvsubfm (tmp34a, a, _809, b_in);			
					cvadd   (a, a, b_in);					

					if (twiddle_idx == 0) {	
						cviaddsubfm (b, e, tmp25a, _951, b_in);		
						cvisubaddfm (c, d, tmp34a, _m588, b_in);	
						dest[group_member] = a;
						dest[group_member + stride]   = b;
						dest[group_member + 2*stride] = c;
						dest[group_member + 3*stride] = d;
						dest[group_member + 4*stride] = e;
					}
					else {			
						TVDT twiddle1; twidload (twiddle1, &pmdata->twiddles2[4*twiddle_idx]);
						TVDT twiddle2; twidload (twiddle2, &pmdata->twiddles2[4*twiddle_idx+2]);
						VDT sin1_951 = mulpd (_951, twiddle1.sin);
						VDT sin2_m588 = mulpd (_m588, twiddle2.sin);
						cvrmul   (tmp25a, twiddle1.sin, tmp25a);
						cvrmul   (tmp34a, twiddle2.sin, tmp34a);
						cviaddsubfm (b, e, tmp25a, sin1_951, b_in);	
						cvisubaddfm (c, d, tmp34a, sin2_m588, b_in);	
						dest[group_member] = a;
						twidmuldelay     (dest[group_member + stride], b, twiddle1);
						twidmuldelay     (dest[group_member + 2*stride], c, twiddle2);
						twidconjmuldelay (dest[group_member + 3*stride], d, twiddle2);
						twidconjmuldelay (dest[group_member + 4*stride], e, twiddle1);
					}
				}

				if (group_member != 0) last_group_member = stride;	
				else last_group_member = src_size;			
				for ( ; group_member < last_group_member; group_member++) {
					uint64_t twiddle_idx = group_member * instride1 + blki;
					CVDT a = src[instride1*(group_member)];

					if (twiddle_idx == 0) {	
						dest[group_member] = a;
						dest[group_member + stride] = a;
						dest[group_member + 2*stride] = a;
						dest[group_member + 3*stride] = a;
						dest[group_member + 4*stride] = a;
					}
					else {			
						TVDT twiddle1; twidload (twiddle1, &pmdata->twiddles2[4*twiddle_idx]);
						TVDT twiddle2; twidload (twiddle2, &pmdata->twiddles2[4*twiddle_idx+2]);
						dest[group_member] = a;
						twidmulandconjmul (dest[group_member + stride], dest[group_member + 4*stride], a, twiddle1);
						twidmulandconjmul (dest[group_member + 2*stride], dest[group_member + 3*stride], a, twiddle2);
					}
				}

				for ( ; group_member < stride; group_member++) {
					memset (&dest[group_member], 0, sizeof (CVDT));
					memset (&dest[group_member + stride], 0, sizeof (CVDT));
					memset (&dest[group_member + 2*stride], 0, sizeof (CVDT));
					memset (&dest[group_member + 3*stride], 0, sizeof (CVDT));
					memset (&dest[group_member + 4*stride], 0, sizeof (CVDT));
				}
			    }
			}

			for ( ; size != 8 && size % 4 == 0; size /= 4, twiddle45_stride *= 4, src = dest, instride1 = 1, zpad = FALSE) {
			    stride = size / 4;
			    if (pd->pass == 1 && stride == 1) dest = (srcarg == 1 ? pd->invec1 : pd->invec2) + blki, outstride1 = pd->pass2_size;
			    for (unsigned int group = 0; group < pd->pass_size; group += stride * 4) {
				uint64_t group_member = 0;		
				uint64_t last_group_member;		

				if (!zpad) last_group_member = stride;			
				else if (src_size <= 3 * stride) last_group_member = 0;	
				else last_group_member = src_size - 3 * stride;		
				for ( ; group_member < last_group_member; group_member++) {
					uint64_t twiddle_idx = ((group_member * pd->pass_stride) + blki) * twiddle45_stride;
					CVDT a = src[instride1*(group + group_member)];
					CVDT b = src[instride1*(group + group_member + stride)];
					CVDT c = src[instride1*(group + group_member + 2*stride)];
					CVDT d = src[instride1*(group + group_member + 3*stride)];
					CVDT new_a, new_b, new_c, new_d;

					cvaddsub (new_a, new_c, a, c);				
					cvaddsub (new_b, new_d, b, d);				

					if (twiddle_idx == 0) {	
						cvaddsub (a, b, new_a, new_b);			
						cviaddsub (c, d, new_c, new_d);			
						dest[outstride1*(group + group_member)] = a;
						dest[outstride1*(group + group_member + 1*stride)] = c;		
						dest[outstride1*(group + group_member + 2*stride)] = b;
						dest[outstride1*(group + group_member + 3*stride)] = d;
					}
					else {			
						TVDT twiddle1; twidload (twiddle1, &pmdata->twiddles2[4*twiddle_idx]);
						TVDT twiddle2; twidload (twiddle2, &pmdata->twiddles2[4*twiddle_idx+2]);
						cvrmul   (new_c, twiddle1.sin, new_c);
						cvaddsub (a, b, new_a, new_b);			
						cviaddsubfm (c, d, new_c, twiddle1.sin, new_d);	
						dest[outstride1*(group + group_member)] = a;
						twidmuldelay     (dest[outstride1*(group + group_member + 1*stride)], c, twiddle1);
						twidmul          (dest[outstride1*(group + group_member + 2*stride)], b, twiddle2);
						twidconjmuldelay (dest[outstride1*(group + group_member + 3*stride)], d, twiddle1);
					}
				}
				if (group_member == stride) continue;

				ASSERTG (group == 0);  ASSERTG (outstride1 == 1); ASSERTG (instride1 == pd->pass_stride); ASSERTG (twiddle45_stride == 1);
				if (group_member != 0) last_group_member = stride;	
				else if (src_size <= 2 * stride) last_group_member = 0;	
				else last_group_member = src_size - 2 * stride;		
				for ( ; group_member < last_group_member; group_member++) {
					uint64_t twiddle_idx = group_member * instride1 + blki;
					CVDT a = src[instride1*(group_member)];
					CVDT b_in = src[instride1*(group_member + stride)];
					CVDT c = src[instride1*(group_member + 2*stride)];
					CVDT b, d, new_a, new_c;

					cvaddsub (new_a, new_c, a, c);				

					if (twiddle_idx == 0) {	
						cviaddsub (c, d, new_c, b_in);			
						cvaddsub (a, b, new_a, b_in);			
						dest[group_member] = a;
						dest[group_member + 1*stride] = c;		
						dest[group_member + 2*stride] = b;
						dest[group_member + 3*stride] = d;
					}
					else {			
						TVDT twiddle1; twidload (twiddle1, &pmdata->twiddles2[4*twiddle_idx]);
						TVDT twiddle2; twidload (twiddle2, &pmdata->twiddles2[4*twiddle_idx+2]);
						cvrmul   (new_c, twiddle1.sin, new_c);
						cvaddsub (a, b, new_a, b_in);			
						cviaddsubfm (c, d, new_c, twiddle1.sin, b_in);	
						dest[group_member] = a;
						twidmuldelay     (dest[group_member + 1*stride], c, twiddle1);
						twidmul          (dest[group_member + 2*stride], b, twiddle2);
						twidconjmuldelay (dest[group_member + 3*stride], d, twiddle1);
					}
				}

				if (group_member != 0) last_group_member = stride;	
				else if (src_size <= stride) last_group_member = 0;	
				else last_group_member = src_size - stride;		
				for ( ; group_member < last_group_member; group_member++) {
					uint64_t twiddle_idx = group_member * instride1 + blki;
					CVDT a_in = src[instride1*(group_member)];
					CVDT b_in = src[instride1*(group_member + stride)];
					CVDT a, b, c, d;

					if (twiddle_idx == 0) {	
						cvaddsub (a, b, a_in, b_in);			
						cviaddsub (c, d, a_in, b_in);			
						dest[group_member] = a;
						dest[group_member + 1*stride] = c;		
						dest[group_member + 2*stride] = b;
						dest[group_member + 3*stride] = d;
					}
					else {			
						TVDT twiddle1; twidload (twiddle1, &pmdata->twiddles2[4*twiddle_idx]);
						TVDT twiddle2; twidload (twiddle2, &pmdata->twiddles2[4*twiddle_idx+2]);
						CVDT new_c;
						cvrmul   (new_c, twiddle1.sin, a_in);
						cvaddsub (a, b, a_in, b_in);			
						cviaddsubfm (c, d, new_c, twiddle1.sin, b_in);	
						dest[group_member] = a;
						twidmuldelay     (dest[group_member + 1*stride], c, twiddle1);
						twidmul          (dest[group_member + 2*stride], b, twiddle2);
						twidconjmuldelay (dest[group_member + 3*stride], d, twiddle1);
					}
				}

				if (group_member != 0) last_group_member = stride;	
				else last_group_member = src_size;			
				for ( ; group_member < last_group_member; group_member++) {
					uint64_t twiddle_idx = group_member * instride1 + blki;
					CVDT a = src[instride1*(group_member)];

					if (twiddle_idx == 0) {	
						dest[group_member] = a;
						dest[group_member + 1*stride] = a;
						dest[group_member + 2*stride] = a;
						dest[group_member + 3*stride] = a;
					}
					else {			
						TVDT twiddle1; twidload (twiddle1, &pmdata->twiddles2[4*twiddle_idx]);
						TVDT twiddle2; twidload (twiddle2, &pmdata->twiddles2[4*twiddle_idx+2]);
						dest[group_member] = a;
						twidmulandconjmul (dest[group_member + 1*stride], dest[group_member + 3*stride], a, twiddle1);
						twidmul           (dest[group_member + 2*stride], a, twiddle2);
					}
				}

				for ( ; group_member < stride; group_member++) {
					memset (&dest[group_member], 0, sizeof (CVDT));
					memset (&dest[group_member + stride], 0, sizeof (CVDT));
					memset (&dest[group_member + 2*stride], 0, sizeof (CVDT));
					memset (&dest[group_member + 3*stride], 0, sizeof (CVDT));
				}
			    }
			}

			ASSERTG (!zpad);
			if (size == 8) {
			    VDT _707 = broadcastsd (0.70710678118654752440084436210485);
			    stride = 1;
			    for (unsigned int group = 0; group < pd->pass_size; group += stride * 8) {

				for (unsigned int group_member = 0; group_member < stride; group_member++) {
					CVDT a = src[group + group_member];
					CVDT b = src[group + group_member + stride];
					CVDT c = src[group + group_member + 2*stride];
					CVDT d = src[group + group_member + 3*stride];
					CVDT e = src[group + group_member + 4*stride];
					CVDT f = src[group + group_member + 5*stride];
					CVDT g = src[group + group_member + 6*stride];
					CVDT h = src[group + group_member + 7*stride];

					CVDT new_a, new_b, new_c, new_d, new_e, new_f, new_g, new_h, twid_f, twid_h;
					cvaddsub (new_a, new_e, a, e);				
					cvaddsub (new_b, new_f, b, f);				
					cvaddsub (new_c, new_g, c, g);				
					cvaddsub (new_d, new_h, d, h);				

					cvaddsub (a, c, new_a, new_c);				
					cvaddsub (b, d, new_b, new_d);				
					cviaddsub (e, g, new_e, new_g);				
					cviaddsub (f, h, new_f, new_h);				

					cvaddsub (new_a, new_b, a, b);				
					cviaddsub (new_c, new_d, c, d);				
					twid_f.real = subpd (f.real, f.imag);			
					twid_f.imag = addpd (f.real, f.imag);
					twid_h.real = addpd (h.real, h.imag);			
					twid_h.imag = subpd (h.imag, h.real);
					cvaddsubfm (new_e, new_f, e, _707, twid_f);		
					cvsubaddfm (new_g, new_h, g, _707, twid_h);		

					dest[group + group_member] = new_a;			
					dest[group + group_member + 4*stride] = new_b;
					dest[group + group_member + 2*stride] = new_c;
					dest[group + group_member + 6*stride] = new_d;
					dest[group + group_member + 1*stride] = new_e;
					dest[group + group_member + 5*stride] = new_f;
					dest[group + group_member + 3*stride] = new_g;
					dest[group + group_member + 7*stride] = new_h;
				}
			    }
			    twiddle45_stride *= 8;
			}
		    }
		}

		if (pd->pass == 1 || (pd->options & FORWARD_FFT_ONLY)) continue;

		if (pd->pass == 2) {
		    for (unsigned int j = 0; j < pd->pass_size; j++) {
			CVDT x = pd->invec1[blke * pd->pass_size + j];
			CVDT y = pd->invec2[blke * pd->pass_size + j];
			cvmul (pd->outvec[blke * pd->pass_size + j], x, y);
			if (pd->powers_of_two == 0) cvrmul (pd->outvec[blke * pd->pass_size + j], pd->inv_fft_size, pd->outvec[blke * pd->pass_size + j]);
		    }
		}

		if (pd->pass == 2) {
			src = dest = pd->outvec + blke * pd->pass_size;
			instride1 = outstride1 = 1;
		} else {
			src = pd->outvec + blki;
			dest = scratch;
			instride1 = pd->pass2_size;
			outstride1 = 1;
		}

		stride = 1;

		if (pd->pass == 2 && pd->powers_of_two & 1) {	
			twiddle45_stride /= 8;			
			for (unsigned int group = 0; group < pd->pass_size; group += stride * 8) {
				for (unsigned int group_member = 0; group_member < stride; group_member++) {
					CVDT a = src[group + group_member];
					CVDT b = src[group + group_member + stride];
					CVDT c = src[group + group_member + 2*stride];
					CVDT d = src[group + group_member + 3*stride];
					CVDT e = src[group + group_member + 4*stride];
					CVDT f = src[group + group_member + 5*stride];
					CVDT g = src[group + group_member + 6*stride];
					CVDT h = src[group + group_member + 7*stride];

					CVDT new_a, new_b, new_c, new_d, new_e, new_f, new_g, new_h, twid_f, twid_h;

					cvrmul (a, pd->inv_fft_size, a);
					cvaddsubfm (new_a, new_e, a, pd->inv_fft_size, e);	
					cvaddsub (new_b, new_f, b, f);				
					cvaddsub (new_c, new_g, c, g);				
					cvaddsub (new_d, new_h, d, h);				

					cvaddsubfm  (a, c, new_a, pd->inv_fft_size, new_c);	
					cvisubaddfm (e, g, new_e, pd->inv_fft_size, new_g);	
					cvaddsub (b, d, new_b, new_d);				
					cvisubadd (f, h, new_f, new_h);				

					cvaddsubfm  (new_a, new_b, a, pd->inv_fft_size, b);	
					cvisubaddfm (new_c, new_d, c, pd->inv_fft_size, d);	
					twid_f.real = addpd (f.real, f.imag);			
					twid_f.imag = subpd (f.imag, f.real);
					twid_h.real = subpd (h.real, h.imag);			
					twid_h.imag = addpd (h.real, h.imag);
					cvaddsubfm (new_e, new_f, e, pd->inv_fft_size_707, twid_f); 
					cvsubaddfm (new_g, new_h, g, pd->inv_fft_size_707, twid_h); 

					dest[group + group_member] = new_a;			
					dest[group + group_member + 4*stride] = new_b;
					dest[group + group_member + 2*stride] = new_c;
					dest[group + group_member + 6*stride] = new_d;
					dest[group + group_member + 1*stride] = new_e;
					dest[group + group_member + 5*stride] = new_f;
					dest[group + group_member + 3*stride] = new_g;
					dest[group + group_member + 7*stride] = new_h;
				}
			}
			stride *= 8;			
		}

		for ( ; (size = pd->pass_size / stride) % 4 == 0; stride *= 4, src = dest, instride1 = 1) {
			twiddle45_stride /= 4;
			if (pd->pass == 3 && stride * 4 == pd->pass_size && !pd->write_direct) dest = pd->outvec + blki, outstride1 = pd->pass2_size;
			for (unsigned int group = 0; group < pd->pass_size; group += stride * 4) {
				for (unsigned int group_member = 0; group_member < stride; group_member++) {
					uint64_t twiddle_idx = ((group_member * pd->pass_stride) + blki) * twiddle45_stride;
					CVDT a = src[instride1*(group + group_member)];
					CVDT b = src[instride1*(group + group_member + stride)];
					CVDT c = src[instride1*(group + group_member + 2*stride)];
					CVDT d = src[instride1*(group + group_member + 3*stride)];

					CVDT	new_a, new_b, new_c, new_d;
					if (twiddle_idx == 0) {
						if (pd->pass != 2 || stride != 1) {	
							cvaddsub (new_a, new_c, a, c);	
							cvaddsub (new_b, new_d, b, d);	

							cvaddsub (a, b, new_a, new_b);	
							cvisubadd (c, d, new_c, new_d);	
						} else {				
							cvrmul (a, pd->inv_fft_size, a);
							cvaddsubfm (new_a, new_c, a, pd->inv_fft_size, c);	
							cvaddsub   (new_b, new_d, b, d);			

							cvaddsubfm  (a, b, new_a, pd->inv_fft_size, new_b);	
							cvisubaddfm (c, d, new_c, pd->inv_fft_size, new_d);	
						}
					}
					
					else {
						CVDT	twiddled_b, twiddled_c, twiddled_d;
						TVDT twiddle1; twidload (twiddle1, &pmdata->twiddles2[4*twiddle_idx]);
						TVDT twiddle2; twidload (twiddle2, &pmdata->twiddles2[4*twiddle_idx+2]);
						twidconjmuldelay (twiddled_b, b, twiddle1);
						twidconjmuldelay (twiddled_c, c, twiddle2);
						twidmuldelay     (twiddled_d, d, twiddle1);
						cvaddsubfm (new_a, new_c, a, twiddle2.sin, twiddled_c);		
						cvaddsub (new_b, new_d, twiddled_b, twiddled_d);		

						cvaddsubfm (a, b, new_a, twiddle1.sin, new_b);			
						cvisubaddfm (c, d, new_c, twiddle1.sin, new_d);			
					}

					dest[outstride1*(group + group_member)]            = a;		
					dest[outstride1*(group + group_member + 2*stride)] = b;
					dest[outstride1*(group + group_member + 1*stride)] = c;
					dest[outstride1*(group + group_member + 3*stride)] = d;
				}
			}
		}

		for ( ; (size = pd->pass_size / stride) % 5 == 0; stride *= 5, src = dest, instride1 = 1) {
			twiddle45_stride /= 5;
			if (pd->pass == 3 && stride * 5 == pd->pass_size && !pd->write_direct) dest = pd->outvec + blki, outstride1 = pd->pass2_size;
			VDT _309 = broadcastsd (0.30901699437494742410229341718282);
			VDT _809 = broadcastsd (0.80901699437494742410229341718282);
			VDT _951 = broadcastsd (0.95105651629515357211643933337938);
			VDT _588_951 = broadcastsd (0.61803398874989484820458683436564);
			for (unsigned int group = 0; group < pd->pass_size; group += stride * 5) {
				for (unsigned int group_member = 0; group_member < stride; group_member++) {
					uint64_t twiddle_idx = ((group_member * pd->pass_stride) + blki) * twiddle45_stride;
					CVDT a = src[instride1*(group + group_member)];
					CVDT b = src[instride1*(group + group_member + stride)];
					CVDT c = src[instride1*(group + group_member + 2*stride)];
					CVDT d = src[instride1*(group + group_member + 3*stride)];
					CVDT e = src[instride1*(group + group_member + 4*stride)];

					CVDT	b_plus_e, b_minus_e, c_plus_d, c_minus_d;
					if (twiddle_idx == 0) {
						cvaddsub (b_plus_e, b_minus_e, b, e);		
						cvaddsub (c_plus_d, c_minus_d, c, d);		
					}
					
					else {
						CVDT twiddled_b, twiddled_c, twiddled_d, twiddled_e;
						TVDT twiddle1; twidload (twiddle1, &pmdata->twiddles2[4*twiddle_idx]);
						TVDT twiddle2; twidload (twiddle2, &pmdata->twiddles2[4*twiddle_idx+2]);
						twidconjmul (twiddled_b, b, twiddle1);
						twidconjmul (twiddled_c, c, twiddle2);
						twidmuldelay (twiddled_d, d, twiddle2);
						twidmuldelay (twiddled_e, e, twiddle1);
						cvaddsubfm (b_plus_e, b_minus_e, twiddled_b, twiddle1.sin, twiddled_e);	
						cvaddsubfm (c_plus_d, c_minus_d, twiddled_c, twiddle2.sin, twiddled_d);	
					}

					CVDT tmp25a, tmp25b, tmp34a, tmp34b;
					cvaddfm (tmp25a, a, _309, b_plus_e); cvsubfm (tmp25a, tmp25a, _809, c_plus_d);	
					cvsubfm (tmp34a, a, _809, b_plus_e); cvaddfm (tmp34a, tmp34a, _309, c_plus_d);	
					cvaddfm (tmp25b, b_minus_e, _588_951, c_minus_d);				
					cvsubfm (tmp34b, c_minus_d, _588_951, b_minus_e);				
					cvisubaddfm (b, e, tmp25a, _951, tmp25b);		
					cviaddsubfm (c, d, tmp34a, _951, tmp34b);		
					cvadd (a, a, b_plus_e);					
					cvadd (a, a, c_plus_d);

					dest[outstride1*(group + group_member)]            = a;
					dest[outstride1*(group + group_member + stride)]   = b;
					dest[outstride1*(group + group_member + 2*stride)] = c;
					dest[outstride1*(group + group_member + 3*stride)] = d;
					dest[outstride1*(group + group_member + 4*stride)] = e;
				}
			}
		}

		for ( ; (size = pd->pass_size / stride) % 3 == 0; stride *= 3, src = dest, instride1 = 1) {
			twiddle3_stride /= 3;
			if (pd->pass == 3 && stride * 3 == pd->pass_size && !pd->write_direct) dest = pd->outvec + blki, outstride1 = pd->pass2_size;
			VDT half = broadcastsd (0.5);
			VDT _866 = broadcastsd (0.86602540378443864676372317075294);
			for (unsigned int group = 0; group < pd->pass_size; group += stride * 3) {
				for (unsigned int group_member = 0; group_member < stride; group_member++) {
					uint64_t twiddle_idx = ((group_member * pd->pass_stride) + blki) * twiddle3_stride;
					CVDT a = src[instride1*(group + group_member)];
					CVDT b = src[instride1*(group + group_member + stride)];
					CVDT c = src[instride1*(group + group_member + 2*stride)];

					CVDT	b_plus_c, b_minus_c;
					if (twiddle_idx == 0) {
						cvadd (b_plus_c, b, c);			
						cvsub (b_minus_c, b, c);		
					}
					
					else {
						CVDT twiddled_b, twiddled_c;
						TVDT twiddle1; twidload (twiddle1, &pmdata->twiddles1[2*twiddle_idx]);
						twidconjmul  (twiddled_b, b, twiddle1);
						twidmuldelay (twiddled_c, c, twiddle1);
						cvaddsubfm (b_plus_c, b_minus_c, twiddled_b, twiddle1.sin, twiddled_c);	
					}

					CVDT tmp23; cvsubfm (tmp23, a, half, b_plus_c);		
					cvisubaddfm (b, c, tmp23, _866, b_minus_c);		
					cvadd (a, a, b_plus_c);					

					dest[outstride1*(group + group_member)] = a;
					dest[outstride1*(group + group_member + stride)] = b;
					dest[outstride1*(group + group_member + 2*stride)] = c;
				}
			}
		}

		if (pd->write_direct) {
			
			if (pd->pass == 3) write_line_slice_strided (pmdata, pd->rwlsd3, src, 0, pd->rwlsd3->size, blki, pd->pass2_size);
			
			else if (pd->pass == 2 && pd->pass1_size == 1) write_line_slice_strided (pmdata, pd->rwlsd3, src, 0, pd->rwlsd3->size, 0, 1);
		}
	}
}

void fft_line (
	pmhandle *pmdata,			
	fft_line_pass_data *pd)			
{
	unsigned int threes_in_fft_size;	

	generate_sincos (pmdata, pd->fft_size);

	for (pd->powers_of_two = 0; (pd->fft_size & (1ULL << pd->powers_of_two)) == 0; pd->powers_of_two++);	
	for (threes_in_fft_size = 1; pd->fft_size % (threes_in_fft_size * 3) == 0; threes_in_fft_size *= 3);	
	pd->inv_fft_size = broadcastsd (1.0 / (double) pd->fft_size);
	pd->inv_fft_size_707 = broadcastsd (0.70710678118654752440084436210485 / (double) pd->fft_size);

	pick_pass_sizes (pmdata, pd->fft_size, &pd->pass1_size, &pd->pass2_size);

	for (pd->pass = 1; pd->pass <= 3; pd->pass++) {

		if (pd->pass != 2 && pd->pass1_size == 1) continue;
		
		if (pd->pass == 1 && !(pd->options & FORWARD_FFT_INVEC1) && !(pd->options & FORWARD_FFT_INVEC2)) continue;

		if (pd->pass == 1) {
			pd->starting_twiddle3_stride = 1;
			pd->starting_twiddle45_stride = 1;
		} else if (pd->pass == 2) {
			
			if (!(pd->options & FORWARD_FFT_INVEC1) && !(pd->options & FORWARD_FFT_INVEC2)) {
				pd->starting_twiddle3_stride = threes_in_fft_size;
				pd->starting_twiddle45_stride = pd->fft_size / threes_in_fft_size;
			}
			
			else if (pd->pass1_size == 1) {
				pd->starting_twiddle3_stride = 1;
				pd->starting_twiddle45_stride = 1;
			}
			
			else {
				ASSERTG (pd->starting_twiddle3_stride = 9999);
				pd->starting_twiddle45_stride = pd->pass1_size / threes_in_fft_size;
			}
		} else {
			pd->starting_twiddle3_stride = threes_in_fft_size;
			pd->starting_twiddle45_stride = pd->pass1_size / threes_in_fft_size;
		}

		if (pd->pass != 2) {
			pd->pass_size = pd->pass1_size;
			pd->pass_stride = pd->pass2_size;
			pd->num_exterior_blks = 1;
			pd->num_interior_blks = pd->pass2_size;
		} else {
			pd->pass_size = pd->pass2_size;
			pd->pass_stride = 1;
			pd->num_exterior_blks = pd->pass1_size;
			pd->num_interior_blks = 1;
		}

		if (pmdata->mt_polymult_line) {
			pd->multithreading = FALSE;
			fft_line_pass (0, pmdata, pd);
		} else {
			atomic_set (pd->blknum, 0);
			pd->multithreading = TRUE;
			pmdata->internal_callback = &fft_line_pass;
			pmdata->internal_callback_data = pd;
			polymult_launch_helpers (pmdata);
		}
	}
}

__inline void monic_line_add_hi (CVDT *fft1, CVDT *addinvec, uint64_t addinvec_size, CVDT *outvec, uint64_t outvec_size, uint64_t circular_size, bool leading_one, bool trailing_one)
{
	
	int64_t outvec_index = (outvec_size-1) % circular_size;
	for (addinvec += addinvec_size-1; addinvec_size; addinvec--, addinvec_size--, outvec_index--) {
		if (outvec_index < 0) outvec_index = circular_size - 1;
		if (leading_one) { leading_one = FALSE; continue; }
		if (trailing_one && addinvec_size == 1) continue;
		if (fft1 == NULL) {
			cvadd (outvec[outvec_index], outvec[outvec_index], addinvec[0]);
		} else {
			CVDT	tmp;
			cvmul (tmp, *fft1, addinvec[0]);
			cvadd (outvec[outvec_index], outvec[outvec_index], tmp);
		}
	}
}

__inline void monic_line_add_lo (CVDT *fft1, CVDT *addinvec, uint64_t addinvec_size, CVDT *outvec, uint64_t outvec_size, uint64_t circular_size, bool leading_one, bool trailing_one)
{
	
	for (uint64_t outvec_index = 0; addinvec_size; addinvec++, addinvec_size--, outvec_index++) {
		if (outvec_index == circular_size) outvec_index = 0;
		if (trailing_one) { trailing_one = FALSE; continue; }
		if (leading_one && addinvec_size == 1) continue;
		if (fft1 == NULL) {
			cvadd (outvec[outvec_index], outvec[outvec_index], addinvec[0]);
		} else {
			CVDT	tmp;
			cvmul (tmp, *fft1, addinvec[0]);
			cvadd (outvec[outvec_index], outvec[outvec_index], tmp);
		}
	}
}

void monic_line_adjustments (
	pmhandle *pmdata,
	CVDT	*addinvec,		
	uint64_t addinvec_size,		
	int	addinvec_options,	
	bool	addinvec_monics_stripped, 
	CVDT	*outvec,		
	uint64_t outvec_size,		
	uint64_t circular_size,		

	int	line,			
	int	options)		
{
	gwhandle *gwdata = pmdata->gwdata;
	CVDT	fft1, *fft1p;

	bool leading_one = (addinvec_options & (POLYMULT_INVEC1_MONIC | POLYMULT_INVEC2_MONIC));
	bool trailing_one = (leading_one && (addinvec_options & (POLYMULT_INVEC1_RLP | POLYMULT_INVEC2_RLP)));

	if (leading_one && addinvec_monics_stripped) addinvec_size++;			
	if (trailing_one && addinvec_monics_stripped) addinvec--, addinvec_size++;	
   
	if (gwdata->GENERAL_MMGW_MOD || gwdata->k != 1.0) {
		read_write_line_slice_data rwlsd;
		prep_line (pmdata->gwdata, line, &rwlsd);
		rwlsd.data = &gwdata->GW_FFT1;
		rwlsd.size = 1;
		rwlsd.vec = &fft1;
		rwlsd.options = 0;
		rwlsd.post_process_monics = FALSE;
		read_line (pmdata, &rwlsd);
		fft1p = &fft1;
	} else {
		fft1p = NULL;
	}

	monic_line_add_hi (fft1p, addinvec, addinvec_size, outvec, outvec_size, circular_size, leading_one, trailing_one);

	if (options & (POLYMULT_INVEC1_RLP | POLYMULT_INVEC2_RLP))
		monic_line_add_lo (fft1p, addinvec, addinvec_size, outvec, outvec_size, circular_size, leading_one, trailing_one);
}

void polymult_line_preprocess (
	pmhandle *pmdata)		
{
	preprocess_plan *plan = (preprocess_plan *) pmdata->plan;	
	preprocessed_poly_header *hdr = plan->hdr;	
	gwnum	*gw_invec1 = pmdata->invec1;		
	uint64_t invec1_size = pmdata->invec1_size;	
	int	options = pmdata->options;
	uint64_t fft_size = plan->fft_size;
	int	line;
	int	lines_combined;				
	CVDT	*invec1, *scratch;
	char	*first_element;				
	uint64_t max_element_size;

	scratch = NULL;
	if (plan->impl == POLYMULT_IMPL_FFT && options & POLYMULT_PRE_FFT) {
		unsigned int pass1_size, pass2_size;
		pick_pass_sizes (pmdata, fft_size, &pass1_size, &pass2_size);
		if (pass1_size > 1) scratch = (CVDT *) aligned_malloc (pass1_size * sizeof (CVDT), 64);
	}

	if ((options & POLYMULT_INVEC1_RLP) && !(options & POLYMULT_PRE_FFT)) options &= ~POLYMULT_INVEC1_RLP;

	max_element_size = 0;

	first_element = (char *) round_up_to_multiple_of ((intptr_t) hdr + sizeof (preprocessed_poly_header), 64);

	read_write_line_slice_data rwlsd;
	rwlsd.data = gw_invec1;
	rwlsd.size = invec1_size;
	rwlsd.options = options & INVEC1_OPTIONS;
	rwlsd.post_process_monics = plan->strip_monic_from_invec1;
	while ((line = (int) atomic_fetch_incr (pmdata->helper_counter)) < pmdata->num_lines) {

		lines_combined = plan->combine_two_lines && line > 0;

		invec1 = (CVDT *) (first_element + (line - lines_combined) * hdr->element_size);

		if (line <= 1 && plan->combine_two_lines) {
			if (line == 1) continue;
			CVDT *temp = (CVDT *) aligned_malloc ((size_t) plan->adjusted_invec1_size * sizeof (CVDT), 64);
			prep_line (pmdata->gwdata, 0, &rwlsd);
			rwlsd.vec = temp;
			read_line (pmdata, &rwlsd);
			prep_line (pmdata->gwdata, 1, &rwlsd);
			rwlsd.vec = invec1;
			read_line (pmdata, &rwlsd);
			for (int j = 0; j < plan->adjusted_invec1_size; j++) {
				double	line0_real;
				union { VDT a; double b[VLEN / sizeof(double)]; } x;
				
				x.a = temp[j].real; line0_real = x.b[0];
				
				x.a = invec1[j].imag; x.b[0] = line0_real; invec1[j].imag = x.a;
			}
			aligned_free (temp);
		} else {
			prep_line (pmdata->gwdata, line, &rwlsd);
			rwlsd.vec = invec1;
			read_line (pmdata, &rwlsd);
		}

		if (options & POLYMULT_PRE_FFT) {
			fft_line_pass_data fftd;
			fftd.rwlsd1 = &rwlsd;
			fftd.rwlsd3 = NULL;
			fftd.invec1 = invec1;
			fftd.invec1_size = plan->adjusted_invec1_size;
			fftd.scratch = scratch;
			fftd.options = FORWARD_FFT_INVEC1 | FORWARD_FFT_ONLY;
			fftd.fft_size = fft_size;
			fftd.write_direct = FALSE;
			fft_line (pmdata, &fftd);
		}

		if (options & POLYMULT_PRE_COMPRESS) {
			uint64_t compressed_element_size = compress_line ((char *) invec1, hdr->element_size, hdr->num_compressed_blks);
			ASSERTG (compressed_element_size <= hdr->element_size);
			if (compressed_element_size > max_element_size) max_element_size = compressed_element_size;
		}
	}

	aligned_free (scratch);

	gwmutex_lock (&pmdata->poly_mutex);
	if (max_element_size > plan->max_element_size) plan->max_element_size = max_element_size;
	gwmutex_unlock (&pmdata->poly_mutex);
}

void polymult_line (
	pmhandle *pmdata)		
{
	polymult_plan *plan = (polymult_plan *) pmdata->plan;		
	uint64_t true_invec1_size, true_invec2_size, true_outvec_size;
	uint64_t invec1_size, invec2_size, outvec_size, fft_size;
	uint32_t alloc_scratch_size;
	int	fft_options, line;
	bool	invec1_pre_ffted, invec2_pre_ffted;
	CVDT	*invec1_buffer, *invec2_buffer, *outvec_buffer, *tmpvec_buffer, *scratch_buffer;
	CVDT	*invec1;
	fft_line_pass_data fftd;
	read_write_line_slice_data rwlsd1, rwlsd2[4], rwlsd3[4];

	ASSERTG (pmdata->num_other_polys <= 4);			

	gwnum	*gw_invec1 = pmdata->invec1;			
	uint64_t gw_invec1_size = pmdata->invec1_size;		

	invec1_pre_ffted = is_preprocessed_poly (gw_invec1) && is_preffted_poly (gw_invec1);

	alloc_scratch_size = 0;
	for (int i = 0; i < pmdata->num_other_polys; i++) {
		uint32_t pass1_size, pass2_size;
		if (plan->planpart[i].impl != POLYMULT_IMPL_FFT) continue;
		pick_pass_sizes (pmdata, plan->planpart[i].fft_size, &pass1_size, &pass2_size);
		if (pass1_size > 1 && pass1_size > alloc_scratch_size) alloc_scratch_size = pass1_size;
	}
	if (!pmdata->mt_polymult_line) alloc_scratch_size *= pmdata->num_threads;

	invec1_buffer = (CVDT *) (plan->alloc_invec1_size ? aligned_malloc ((size_t) plan->alloc_invec1_size * sizeof (CVDT), 64) : NULL);
	invec2_buffer = (CVDT *) (plan->alloc_invec2_size ? aligned_malloc ((size_t) plan->alloc_invec2_size * sizeof (CVDT), 64) : NULL);
	tmpvec_buffer = (CVDT *) (plan->alloc_tmpvec_size ? aligned_malloc ((size_t) plan->alloc_tmpvec_size * sizeof (CVDT), 64) : NULL);
	outvec_buffer = (CVDT *) aligned_malloc ((size_t) plan->alloc_outvec_size * sizeof (CVDT), 64);
	scratch_buffer = (CVDT *) (alloc_scratch_size ? aligned_malloc ((size_t) alloc_scratch_size * sizeof (CVDT), 64) : NULL);

	invec1 = (invec1_buffer == NULL ? tmpvec_buffer : invec1_buffer);
	rwlsd1.data = gw_invec1;
	rwlsd1.size = gw_invec1_size;
	rwlsd1.vec = invec1;
	rwlsd1.options = pmdata->options & INVEC1_OPTIONS;

	for (int i = 0; i < pmdata->num_other_polys; i++) {
		int	options = pmdata->options | pmdata->other_polys[i].options;
		struct planpart *planpart = &plan->planpart[i];

		bool	strip_monic_from_invec1;	
		bool	post_monic_addin_invec2;	
		bool	strip_monic_from_invec2;	
		bool	post_monic_addin_invec1;	
		strip_monic_from_invec1 = planpart->strip_monic_from_invec1;
		strip_monic_from_invec2 = planpart->strip_monic_from_invec2;
		post_monic_addin_invec1 = planpart->post_monic_addin_invec1;
		post_monic_addin_invec2 = planpart->post_monic_addin_invec2;

		rwlsd1.post_process_monics = strip_monic_from_invec1;		

		CVDT *outvec = outvec_buffer + planpart->adjusted_shift;
		CVDT *invec2 = (planpart->impl == POLYMULT_IMPL_FFT && !post_monic_addin_invec2) ? outvec : invec2_buffer;
		rwlsd2[i].data = pmdata->other_polys[i].invec2;		
		rwlsd2[i].size = pmdata->other_polys[i].invec2_size;	
		rwlsd2[i].vec = invec2;
		rwlsd2[i].options = options & INVEC2_OPTIONS;
		rwlsd2[i].post_process_monics = strip_monic_from_invec2;

		rwlsd3[i].data = pmdata->other_polys[i].outvec;		
		rwlsd3[i].size = pmdata->other_polys[i].outvec_size;	
		rwlsd3[i].vec = outvec_buffer;
		rwlsd3[i].fma_data = pmdata->other_polys[i].fmavec;		
		rwlsd3[i].LSWs_skipped = planpart->LSWs_skipped;		
		rwlsd3[i].streamed_stores = planpart->streamed_stores;		
		rwlsd3[i].options = options;
	}

	while ((line = (int) atomic_fetch_incr (pmdata->helper_counter)) < pmdata->num_lines) {
	    CVDT *invec2, *outvec, *true_outvec, *tmpvec, *scratch;
	    tmpvec = tmpvec_buffer;
	    scratch = scratch_buffer;

	    prep_line (pmdata->gwdata, line, &rwlsd1);

	    uint64_t prev_fft_size = 0;
	    for (int i = 0; i < pmdata->num_other_polys; i++) {
		gwnum	*gw_invec2 = pmdata->other_polys[i].invec2;		
		gwnum	*gw_outvec = pmdata->other_polys[i].outvec;		
		uint64_t gw_outvec_size = pmdata->other_polys[i].outvec_size;	
		int	options = pmdata->options | pmdata->other_polys[i].options;
		struct planpart *planpart = &plan->planpart[i];

		rwlsd2[i].lsd = rwlsd1.lsd;			
		rwlsd3[i].lsd = rwlsd1.lsd;			

		bool	strip_monic_from_invec1;	
		bool	post_monic_addin_invec2;	
		bool	strip_monic_from_invec2;	
		bool	post_monic_addin_invec1;	
		uint64_t circular_size, LSWs_skipped;

		strip_monic_from_invec1 = planpart->strip_monic_from_invec1;
		strip_monic_from_invec2 = planpart->strip_monic_from_invec2;
		post_monic_addin_invec1 = planpart->post_monic_addin_invec1;
		post_monic_addin_invec2 = planpart->post_monic_addin_invec2;
		fft_size = planpart->fft_size;
		circular_size = planpart->circular_size;
		LSWs_skipped = planpart->LSWs_skipped;

		invec1_size = planpart->adjusted_invec1_size;
		invec2_size = planpart->adjusted_invec2_size;
		outvec_size = planpart->adjusted_outvec_size;
		true_invec1_size = planpart->true_invec1_size;
		true_invec2_size = planpart->true_invec2_size;
		true_outvec_size = planpart->true_outvec_size;

		true_outvec = outvec_buffer;
		outvec = outvec_buffer + planpart->adjusted_shift;

		for (int i = 0; i < planpart->adjusted_shift; i++) memset (&true_outvec[i], 0, sizeof (CVDT));
		
		for (int i = 0; i < planpart->adjusted_pad; i++) memset (&true_outvec[outvec_size+planpart->adjusted_shift+i], 0, sizeof (CVDT));

		invec2_pre_ffted = is_preprocessed_poly (gw_invec2) && is_preffted_poly (gw_invec2);

		if (i == 0) read_line (pmdata, &rwlsd1);
		
		ASSERTG (i == 0 || strip_monic_from_invec1 == plan->planpart[i-1].strip_monic_from_invec1);

		invec2 = (planpart->impl == POLYMULT_IMPL_FFT && !post_monic_addin_invec2) ? outvec : invec2_buffer;
		read_line (pmdata, &rwlsd2[i]);

		if (planpart->impl == POLYMULT_IMPL_BRUTE) {
			brute_line (invec1, invec1_size, invec2, invec2_size, true_outvec, circular_size, gw_outvec, gw_outvec_size, planpart->adjusted_shift, LSWs_skipped);
		}

 		else if (planpart->impl == POLYMULT_IMPL_KARATSUBA) {
			karatsuba_line (pmdata, invec1, invec1_size, invec2, invec2_size, tmpvec, outvec);
			
			prev_fft_size = 0;
		}

		else {
			
			if (invec1_pre_ffted || (i > 0 && fft_size == prev_fft_size)) fft_options = 0;
			else fft_options = FORWARD_FFT_INVEC1;
			
			if (!invec2_pre_ffted) fft_options |= FORWARD_FFT_INVEC2;
			
			if (fft_options & FORWARD_FFT_INVEC1) {

			}
			
			if (fft_options & FORWARD_FFT_INVEC2) {

			}
			
			fftd.rwlsd1 = &rwlsd1;
			fftd.rwlsd2 = &rwlsd2[i];
			fftd.rwlsd3 = &rwlsd3[i];
			fftd.invec1 = tmpvec;
			fftd.invec1_size = invec1_size;
			fftd.invec2 = outvec;
			fftd.invec2_size = invec2_size;
			fftd.outvec = outvec;
			fftd.scratch = scratch;
			fftd.options = fft_options;
			fftd.fft_size = fft_size;
			fftd.write_direct = (fft_size < pmdata->strided_writes_end && !planpart->emulate_circular &&
					     !post_monic_addin_invec1 && !post_monic_addin_invec2);
			fft_line (pmdata, &fftd);
			
			prev_fft_size = fft_size;
		}

		if (planpart->emulate_circular) {
			uint64_t i, adjusted_i;		
			for (i = circular_size + LSWs_skipped, adjusted_i = LSWs_skipped; i < true_outvec_size; ) {
				if (adjusted_i >= LSWs_skipped + gw_outvec_size) {
					i = divide_rounding_up (i, circular_size) * circular_size + LSWs_skipped;
					adjusted_i = LSWs_skipped;
				} else {
					cvadd (true_outvec[adjusted_i], true_outvec[adjusted_i], true_outvec[i]);
					i++, adjusted_i++;
				}
			}
		}

		if (post_monic_addin_invec1)
			monic_line_adjustments (pmdata, invec1, invec1_size, options & POLYMULT_INVEC1_MONIC_RLP, strip_monic_from_invec1,
						true_outvec, true_outvec_size, circular_size, line, options & POLYMULT_INVEC2_MONIC_RLP);
		if (post_monic_addin_invec2)
			monic_line_adjustments (pmdata, invec2, invec2_size, options & POLYMULT_INVEC2_MONIC_RLP, strip_monic_from_invec2,
						true_outvec, true_outvec_size, circular_size, line, options & POLYMULT_INVEC1_MONIC_RLP);

		if (planpart->impl != POLYMULT_IMPL_FFT || !fftd.write_direct) write_line (pmdata, &rwlsd3[i]);
	    }
	}

	aligned_free (invec1_buffer);
	aligned_free (invec2_buffer);
	aligned_free (outvec_buffer);
	aligned_free (tmpvec_buffer);
	aligned_free (scratch_buffer);
}

#endif

#if !defined (SSE2) && !defined (AVX) && !defined (FMA) && !defined (AVX512)

void polymult_dispatch (
	int	helper_num,			
	pmhandle *pmdata,
	gwhandle *gwdata)			
{
	
	if (pmdata->internal_callback != NULL) {
		(*pmdata->internal_callback) (helper_num, pmdata, pmdata->internal_callback_data);
		return;
	}
	
	switch (pmdata->helper_opcode) {
	case 0:
		
		(*pmdata->helper_callback) (helper_num, gwdata, pmdata->helper_callback_data);
		break;

	case HELPER_PRE_FFT:
		polymult_pre_fft (pmdata, gwdata);
		break;

	case HELPER_POLYMULT_LINE:
#ifdef X86_64
		
		if (pmdata->gwdata->cpu_flags & CPU_AVX512F) polymult_line_avx512 (pmdata); else
#endif
		
		if (pmdata->gwdata->cpu_flags & CPU_FMA3) polymult_line_fma (pmdata);
		
		else if (pmdata->gwdata->cpu_flags & CPU_AVX) polymult_line_avx (pmdata);
		
		else if (pmdata->gwdata->cpu_flags & CPU_SSE2) polymult_line_sse2 (pmdata);
		
#ifndef X86_64
		else polymult_line_dbl (pmdata);
#endif
		break;

	case HELPER_POST_UNFFT:
		polymult_post_unfft (pmdata, gwdata);
		break;

	case HELPER_PREPROCESS_LINE:
#ifdef X86_64
		if (pmdata->gwdata->cpu_flags & CPU_AVX512F) polymult_line_preprocess_avx512 (pmdata); else
#endif
		if (pmdata->gwdata->cpu_flags & CPU_FMA3) polymult_line_preprocess_fma (pmdata);
		else if (pmdata->gwdata->cpu_flags & CPU_AVX) polymult_line_preprocess_avx (pmdata);
		else if (pmdata->gwdata->cpu_flags & CPU_SSE2) polymult_line_preprocess_sse2 (pmdata);
#ifndef X86_64
		else polymult_line_preprocess_dbl (pmdata);
#endif
		break;
	}

}

void polymult_thread (void *arg)
{
	pmhandle *pmdata = (pmhandle *) arg;
	gwhandle cloned_gwdata;
	bool	cloned_gwdata_initialized = FALSE;

	int thread_num = (int) atomic_incr_fetch (pmdata->next_thread_num);
	if (pmdata->gwdata->thread_callback != NULL)
		(*pmdata->gwdata->thread_callback) (thread_num, 20, pmdata->gwdata->thread_callback_data);

	for ( ; ; ) {
	    int	num_active_helpers;

	    if (pmdata->gwdata->use_spin_wait > thread_num) atomic_spinwait (pmdata->alt_work_to_do, 1);
	    else gwevent_wait (&pmdata->work_to_do, 0);

	    if (pmdata->helper_opcode == HELPER_EXIT) break;

	    if (pmdata->all_work_assigned) continue;
	    num_active_helpers = (int) atomic_incr_fetch (pmdata->num_active_helpers);
	    if (!pmdata->all_work_assigned) {		

		if (num_active_helpers < pmdata->num_threads) {
			    
		    if (!cloned_gwdata_initialized && pmdata->helper_opcode != HELPER_PREPROCESS_LINE && pmdata->helper_opcode != HELPER_POLYMULT_LINE) {
			gwclone (&cloned_gwdata, pmdata->gwdata);		
			cloned_gwdata_initialized = TRUE;
		    }

		    polymult_dispatch (thread_num, pmdata, &cloned_gwdata);

		    if (pmdata->helper_opcode != HELPER_PREPROCESS_LINE && pmdata->helper_opcode != HELPER_POLYMULT_LINE) {
			gwmutex_lock (&pmdata->poly_mutex);
			if (pmdata->stats_gwdata == NULL) pmdata->stats_gwdata = &cloned_gwdata;	
			else gwclone_merge_stats (pmdata->stats_gwdata, &cloned_gwdata);		
			gwmutex_unlock (&pmdata->poly_mutex);
		    }
		}

		pmdata->all_work_assigned = TRUE;		
		gwevent_reset (&pmdata->work_to_do);		
		if (pmdata->gwdata->use_spin_wait >= 2) atomic_set (pmdata->alt_work_to_do, 0);
	    }

	    if (atomic_decr_fetch (pmdata->num_active_helpers) == 0) gwevent_signal (&pmdata->all_helpers_done);
	}

	if (pmdata->gwdata->thread_callback != NULL)
		(*pmdata->gwdata->thread_callback) (thread_num, 21, pmdata->gwdata->thread_callback_data);

	if (cloned_gwdata_initialized) gwdone (&cloned_gwdata);
}

void polymult_launch_helpers (
	pmhandle *pmdata)		
{
	int	num_threads;		

	if (pmdata->internal_callback == NULL) atomic_set (pmdata->helper_counter, 0);	

	num_threads = pmdata->num_threads;
	if (pmdata->helper_opcode == HELPER_POLYMULT_LINE && !pmdata->mt_polymult_line && pmdata->internal_callback == NULL) num_threads = 1;

	if (num_threads > 1) {
		
		pmdata->saved_gwdata_num_threads = gwget_num_threads (pmdata->gwdata);
		gwset_num_threads (pmdata->gwdata, 1);
		
		if (pmdata->thread_ids == NULL) {
			atomic_set (pmdata->next_thread_num, 0);
			gwmutex_init (&pmdata->poly_mutex);
			gwevent_init (&pmdata->work_to_do);
			gwevent_init (&pmdata->all_helpers_done);
			atomic_set (pmdata->alt_work_to_do, 0);			
			pmdata->thread_ids = (gwthread *) malloc (pmdata->max_num_threads * sizeof (gwthread));
			for (int i = 1; i < pmdata->max_num_threads; i++) gwthread_create_waitable (&pmdata->thread_ids[i], &polymult_thread, (void *) pmdata);
		}
		
		pmdata->stats_gwdata = NULL;
		pmdata->all_work_assigned = FALSE;			
		gwevent_signal (&pmdata->work_to_do);			
		if (pmdata->gwdata->use_spin_wait >= 2) atomic_set (pmdata->alt_work_to_do, 1);
	}

	polymult_dispatch (0, pmdata, pmdata->gwdata);

	if (num_threads > 1) {
		
		pmdata->all_work_assigned = TRUE;
		gwevent_reset (&pmdata->work_to_do);
		if (pmdata->gwdata->use_spin_wait >= 2) atomic_set (pmdata->alt_work_to_do, 0);
		
		if (pmdata->gwdata->use_spin_wait) atomic_spinwait (pmdata->num_active_helpers, 0);
		else while (atomic_get (pmdata->num_active_helpers)) {
			gwevent_reset (&pmdata->all_helpers_done);
			if (atomic_get (pmdata->num_active_helpers)) gwevent_wait (&pmdata->all_helpers_done, 0);
		}
		
		if (pmdata->stats_gwdata != NULL) gwclone_merge_stats (pmdata->gwdata, pmdata->stats_gwdata);	
		gwset_num_threads (pmdata->gwdata, pmdata->saved_gwdata_num_threads);
	}

	pmdata->helper_opcode = 0;
	pmdata->internal_callback = NULL;
}

gwarray polymult_preprocess (		
	pmhandle *pmdata,		
	gwnum	*invec1,		
	uint64_t invec1_size,		
	uint64_t invec2_size,		
	uint64_t outvec_size,		
	int	options)		
{
	preprocess_plan plan;		
	int	num_elements;
	uint64_t element_size;
	bool	must_fft;

	ASSERTG (!is_preprocessed_poly (invec1));

	plan.adjusted_invec1_size = (options & POLYMULT_INVEC1_RLP) ? 2 * invec1_size - 1 : invec1_size;
	plan.adjusted_invec2_size = (options & POLYMULT_INVEC2_RLP) ? 2 * invec2_size - 1 : invec2_size;
	plan.adjusted_outvec_size = plan.adjusted_invec1_size + plan.adjusted_invec2_size - 1;
	plan.true_invec1_size = plan.adjusted_invec1_size + ((options & POLYMULT_INVEC1_MONIC) ? ((options & POLYMULT_INVEC1_RLP) ? 2 : 1) : 0);
	plan.true_invec2_size = plan.adjusted_invec2_size + ((options & POLYMULT_INVEC2_MONIC) ? ((options & POLYMULT_INVEC2_RLP) ? 2 : 1) : 0);
	plan.true_outvec_size = plan.true_invec1_size + plan.true_invec2_size - 1;

	if ((options & POLYMULT_INVEC1_MONIC) && (options & POLYMULT_INVEC2_MONIC)) {
		plan.strip_monic_from_invec1 = TRUE;
	}
	
	else if (options & POLYMULT_CIRCULAR && outvec_size == polymult_fft_size (outvec_size)) {
		plan.strip_monic_from_invec1 = FALSE;
		plan.adjusted_invec1_size = plan.true_invec1_size;
		plan.adjusted_invec2_size = plan.true_invec2_size;
		plan.adjusted_outvec_size = outvec_size;
	}
	
	else if (polymult_fft_size (plan.adjusted_outvec_size) < polymult_fft_size (plan.true_outvec_size)) {
		plan.strip_monic_from_invec1 = TRUE;
	}
	
	else {
		plan.strip_monic_from_invec1 = FALSE;
		plan.adjusted_invec1_size = plan.true_invec1_size;
		plan.adjusted_invec2_size = plan.true_invec2_size;
		plan.adjusted_outvec_size = plan.true_outvec_size;
	}

	must_fft = FALSE;

	if ((!must_fft && plan.adjusted_outvec_size < pmdata->KARAT_BREAK)) {
		plan.impl = POLYMULT_IMPL_BRUTE;
		options &= ~POLYMULT_PRE_FFT;
	}
	else if (!must_fft && !(options & POLYMULT_CIRCULAR) && plan.adjusted_outvec_size < pmdata->FFT_BREAK) {
		plan.impl = POLYMULT_IMPL_KARATSUBA;
		options &= ~POLYMULT_PRE_FFT;
	}
	else {
		plan.impl = POLYMULT_IMPL_FFT;
		plan.fft_size = polymult_fft_size (plan.adjusted_outvec_size);
	}

	if (!(options & POLYMULT_PRE_FFT)) {
		plan.strip_monic_from_invec1 = TRUE;
		plan.adjusted_invec1_size = invec1_size;
	}

	int complex_vector_size = complex_vector_size_in_bytes (pmdata->gwdata->cpu_flags);
	
	element_size = complex_vector_size * (uint64_t) ((options & POLYMULT_PRE_FFT) ? plan.fft_size : invec1_size);

	plan.combine_two_lines = !pmdata->gwdata->NEGACYCLIC_FFT && !pmdata->gwdata->ZERO_PADDED_FFT && !(options & POLYMULT_PRE_FFT);
	num_elements = plan.combine_two_lines ? pmdata->num_lines - 1 : pmdata->num_lines;
	plan.hdr = (preprocessed_poly_header *) malloc ((size_t) (sizeof (preprocessed_poly_header) + 64 + num_elements * element_size));
	if (plan.hdr == NULL) return (NULL);
	memset (plan.hdr, 0, sizeof (preprocessed_poly_header));
	plan.hdr->element_size = element_size;
	plan.hdr->options = options;
	plan.hdr->monic_ones_included = !plan.strip_monic_from_invec1;
	plan.hdr->top_unnorms = unnorms (invec1[invec1_size-1]);
	plan.hdr->line_size = (options & POLYMULT_PRE_FFT) ? plan.fft_size : invec1_size;
	
	if (pmdata->num_threads == 1 || element_size < 32768 * sizeof (double)) plan.hdr->num_compressed_blks = 1;
	else for (int i = 1; i <= 512 && element_size % (i * sizeof (double)) == 0; i *= 2) plan.hdr->num_compressed_blks = (int) (element_size / (i * sizeof (double)));

	pmdata->plan = &plan;
	pmdata->invec1 = invec1;
	pmdata->invec1_size = invec1_size;
	pmdata->num_other_polys = 0;
	pmdata->options = options;
	plan.max_element_size = 0;

	pmdata->helper_opcode = HELPER_PRE_FFT;		
	polymult_launch_helpers (pmdata);

	pmdata->mt_polymult_line = TRUE;
	pmdata->helper_opcode = HELPER_PREPROCESS_LINE;	
	polymult_launch_helpers (pmdata);
	
	if (options & POLYMULT_PRE_COMPRESS) {
		uint64_t uncompressed_blk_size = element_size / plan.hdr->num_compressed_blks;
		uint64_t compressed_blk_size = (plan.max_element_size - 1) / plan.hdr->num_compressed_blks;

		char *first_element = (char *) round_up_to_multiple_of ((intptr_t) plan.hdr + sizeof (preprocessed_poly_header), 64);
		for (int line = 0; line < num_elements; line++) {
			char *src = first_element + line * element_size;
			char *dest = first_element + line * plan.max_element_size;
			
			*dest++ = *src++;
			
			for (int blk = 0; blk < plan.hdr->num_compressed_blks; blk++) {
				memmove (dest, src, (size_t) compressed_blk_size);
				dest += compressed_blk_size;
				src += uncompressed_blk_size;
			}
		}
		plan.hdr = (preprocessed_poly_header *) realloc (plan.hdr, (size_t) (sizeof (preprocessed_poly_header) + 64 + num_elements * plan.max_element_size));
		plan.hdr->element_size = plan.max_element_size;
	}

	plan.hdr->linkage.next = pmdata->gwdata->array_list;		
	plan.hdr->linkage.prev = &pmdata->gwdata->array_list;	
	if (plan.hdr->linkage.next != NULL) {			
		gwarray_header *next_header = (gwarray_header *) ((char *) plan.hdr->linkage.next - sizeof (gwarray_header));
		next_header->prev = &plan.hdr->linkage.next;
	}
	pmdata->gwdata->array_list = (gwarray) &plan.hdr->self_ptr;	

	plan.hdr->self_ptr = (gwnum *) &plan.hdr->self_ptr;
	return ((gwnum *) &plan.hdr->self_ptr);
}

void polymult (
	pmhandle *pmdata,		
	gwnum	*invec1,		
	uint64_t invec1_size,		
	gwnum	*invec2,		
	uint64_t invec2_size,		
	gwnum	*outvec,		
	uint64_t outvec_size,		
	int	options)
{
	polymult_arg arg;

	ASSERTG (!(options & POLYMULT_CIRCULAR) || !(options & (POLYMULT_MULHI | POLYMULT_MULMID | POLYMULT_MULLO)));
	
	ASSERTG (!(options & (POLYMULT_FMADD | POLYMULT_FMSUB | POLYMULT_FNMADD)));

	arg.invec2 = invec2;
	arg.invec2_size = invec2_size;
	arg.outvec = outvec;
	arg.outvec_size = outvec_size;
	arg.fmavec = NULL;
	arg.circular_size = (options & POLYMULT_CIRCULAR) ? outvec_size : 0;
	arg.first_mulmid = 0;
	arg.options = options & (INVEC2_OPTIONS | OUTVEC_OPTIONS);

	polymult_several (pmdata, invec1, invec1_size, &arg, 1, options & (PLAN_OPTIONS | INVEC1_OPTIONS));
}

void polymult_fma (
	pmhandle *pmdata,		
	gwnum	*invec1,		
	uint64_t invec1_size,		
	gwnum	*invec2,		
	uint64_t invec2_size,		
	gwnum	*outvec,		
	uint64_t outvec_size,		
	gwnum	*fmavec,		
	int	options)
{
	polymult_arg arg;

	ASSERTG (!(options & POLYMULT_CIRCULAR) || !(options & (POLYMULT_MULHI | POLYMULT_MULMID | POLYMULT_MULLO)));
	
	ASSERTG ((options & (POLYMULT_FMADD | POLYMULT_FMSUB | POLYMULT_FNMADD)) && fmavec != NULL);

	arg.invec2 = invec2;
	arg.invec2_size = invec2_size;
	arg.outvec = outvec;
	arg.outvec_size = outvec_size;
	arg.fmavec = fmavec;
	arg.circular_size = (options & POLYMULT_CIRCULAR) ? outvec_size : 0;
	arg.first_mulmid = 0;
	arg.options = options & (INVEC2_OPTIONS | OUTVEC_OPTIONS);

	polymult_several (pmdata, invec1, invec1_size, &arg, 1, options & (PLAN_OPTIONS | INVEC1_OPTIONS));
}

void polymult2 (
	pmhandle *pmdata,		
	gwnum	*invec1,		
	uint64_t invec1_size,		
	gwnum	*invec2,		
	uint64_t invec2_size,		
	gwnum	*outvec,		
	uint64_t outvec_size,		
	gwnum	*fmavec,		
	uint64_t circular_size,		
	uint64_t first_mulmid,		
	int	options)
{
	polymult_arg arg;

	ASSERTG (((options & POLYMULT_CIRCULAR) && circular_size > 0) || (!(options & POLYMULT_CIRCULAR) && circular_size == 0));
	
	ASSERTG (((options & POLYMULT_MULMID) && first_mulmid >= 0) || (!(options & POLYMULT_MULMID) && first_mulmid == 0));
	
	ASSERTG (((options & (POLYMULT_FMADD | POLYMULT_FMSUB | POLYMULT_FNMADD)) && fmavec != NULL) ||
		 (!(options & (POLYMULT_FMADD | POLYMULT_FMSUB | POLYMULT_FNMADD)) && fmavec == NULL));

	arg.invec2 = invec2;
	arg.invec2_size = invec2_size;
	arg.outvec = outvec;
	arg.outvec_size = outvec_size;
	arg.fmavec = fmavec;
	arg.circular_size = circular_size;
	arg.first_mulmid = first_mulmid;
	arg.options = options & (INVEC2_OPTIONS | OUTVEC_OPTIONS);

	polymult_several (pmdata, invec1, invec1_size, &arg, 1, options & (PLAN_OPTIONS | INVEC1_OPTIONS));
}

void polymult_several (		
	pmhandle *pmdata,	
	gwnum	*invec1,	
	uint64_t invec1_size,	
	polymult_arg *other_polys, 
	int	num_other_polys,
	int	options)	
{
	polymult_plan *plan;		
	int	invec1_options;		
	int	global_invec2_options;	

	invec1_options = options & INVEC1_OPTIONS;
	global_invec2_options = options & (INVEC2_OPTIONS | OUTVEC_OPTIONS);
	for (int i = 0; i < num_other_polys; i++) other_polys[i].options |= global_invec2_options;

	if (options & POLYMULT_USE_PLAN) {
	    plan = (polymult_plan *) pmdata->plan;
#define ps_hash(a,b,c,d,e,f) ((a)*3+(b)*17+(c)*131071+(d)*8191001+(e)*1001+(f)*65537)
	    ASSERTG (ps_hash (invec1_size, num_other_polys, other_polys[0].invec2_size, other_polys[0].outvec_size, other_polys[0].options & 0x7FFFF, options & 0x7FFFF) == ((polymult_plan *) pmdata->plan)->hash);
	}

	else {

	    pmdata->plan = plan = (polymult_plan *) malloc (sizeof (polymult_plan) + (num_other_polys-1) * sizeof (struct planpart));
#ifdef GDEBUG
	    plan->hash = ps_hash (invec1_size, num_other_polys, other_polys[0].invec2_size, other_polys[0].outvec_size, other_polys[0].options & 0x7FFFF, options & 0x7FFFF);
#endif

	    bool invec1_preprocessed = is_preprocessed_poly (invec1);
	    bool invec1_pre_ffted = invec1_preprocessed && is_preffted_poly (invec1);

	    plan->mt_polymult_line = TRUE;

	    uint64_t largest_fft_size = 0;		
	    bool largest_strip_monic_from_invec1;	
	    bool must_replan;				
	    bool exists_pre_ffted_invec2 = FALSE;	
	    bool pass2_strip_monic_from_invec1;		

	    for (int pass = 1; pass == 1 || must_replan; pass++) {
		must_replan = FALSE;

		plan->alloc_invec1_size = 0;
		plan->alloc_invec2_size = 0;
		plan->alloc_outvec_size = 0;
		plan->alloc_tmpvec_size = 0;

		for (int i = 0; i < num_other_polys; i++) {
		    gwnum *invec2 = other_polys[i].invec2;		
		    uint64_t invec2_size = other_polys[i].invec2_size;	
		    gwnum *outvec = other_polys[i].outvec;		
		    uint64_t outvec_size = other_polys[i].outvec_size;	
		    gwnum *fmavec = other_polys[i].fmavec;		
		    uint64_t circular_size = other_polys[i].circular_size; 
		    int	options;					
		    bool must_fft = FALSE;				
		    uint64_t fft_size;

		    options = invec1_options | other_polys[i].options;

		    ASSERTG (!(other_polys[i].options & (PLAN_OPTIONS | INVEC1_OPTIONS)));
		    
		    ASSERTG (!(options & POLYMULT_MULHI) || !(options & (POLYMULT_MULMID | POLYMULT_MULLO)));
		    ASSERTG (!(options & POLYMULT_MULMID) || !(options & (POLYMULT_MULHI | POLYMULT_MULLO)));
		    ASSERTG (!(options & POLYMULT_MULLO) || !(options & (POLYMULT_MULHI | POLYMULT_MULMID)));
		    
		    ASSERTG (((options & POLYMULT_CIRCULAR) && circular_size > 0) || (!(options & POLYMULT_CIRCULAR) && circular_size == 0));
		    
		    ASSERTG ((options & POLYMULT_MULMID) || other_polys[i].first_mulmid == 0);
		    
		    ASSERTG (fmavec == NULL || !is_preprocessed_poly (fmavec));

		    bool invec2_preprocessed = is_preprocessed_poly (invec2);
		    bool invec2_pre_ffted = invec2_preprocessed && is_preffted_poly (invec2);
		    exists_pre_ffted_invec2 |= invec2_pre_ffted;

		    if (invec1_pre_ffted) {
			must_fft = TRUE;
			fft_size = preprocessed_fft_size (invec1);
		    }
		    if (invec2_pre_ffted) {
			ASSERTG (!must_fft || fft_size == preprocessed_fft_size (invec2));
			must_fft = TRUE;
			fft_size = preprocessed_fft_size (invec2);
		    }

		    uint64_t adjusted_invec1_size = (options & POLYMULT_INVEC1_RLP) ? 2 * invec1_size - 1 : invec1_size;
		    uint64_t adjusted_invec2_size = (options & POLYMULT_INVEC2_RLP) ? 2 * invec2_size - 1 : invec2_size;
		    uint64_t adjusted_outvec_size = adjusted_invec1_size + adjusted_invec2_size - 1;
		    uint64_t true_invec1_size = adjusted_invec1_size + ((options & POLYMULT_INVEC1_MONIC) ? ((options & POLYMULT_INVEC1_RLP) ? 2 : 1) : 0);
		    uint64_t true_invec2_size = adjusted_invec2_size + ((options & POLYMULT_INVEC2_MONIC) ? ((options & POLYMULT_INVEC2_RLP) ? 2 : 1) : 0);
		    uint64_t true_outvec_size = true_invec1_size + true_invec2_size - 1;

		    plan->planpart[i].true_invec1_size = true_invec1_size;
		    plan->planpart[i].true_invec2_size = true_invec2_size;
		    plan->planpart[i].true_outvec_size = true_outvec_size;

		    if (circular_size >= true_outvec_size) options &= ~POLYMULT_CIRCULAR;
		    
		    if (!(options & POLYMULT_CIRCULAR)) circular_size = true_outvec_size;

		    int64_t addin1, addin2, addin3, addin4, subout;	
		    addin1 = addin2 = addin3 = addin4 = subout = -1;	

		    uint64_t LSWs_skipped;		
		    uint64_t MSWs_skipped;		

		    if (!(options & POLYMULT_INVEC1_MONIC) && !(options & POLYMULT_INVEC2_MONIC)) {
			LSWs_skipped = 0;
			MSWs_skipped = 0;
		    }

		    else if ((options & POLYMULT_INVEC1_MONIC) && !(options & POLYMULT_INVEC2_MONIC)) {
			
			if (options & POLYMULT_INVEC1_RLP) {
				LSWs_skipped = 0;
				MSWs_skipped = 0;
			}
			
			else {
				LSWs_skipped = 0;
				MSWs_skipped = 0;
			}
		    }

		    else if (!(options & POLYMULT_INVEC1_MONIC) && (options & POLYMULT_INVEC2_MONIC)) {
			
			if (options & POLYMULT_INVEC2_RLP) {
				LSWs_skipped = 0;
				MSWs_skipped = 0;
			}
			
			else {
				LSWs_skipped = 0;
				MSWs_skipped = 0;
			}
		    }

		    else if (!(options & POLYMULT_INVEC1_RLP) && !(options & POLYMULT_INVEC2_RLP)) {
			addin1 = true_outvec_size - 1;				
			MSWs_skipped = 1;					
			LSWs_skipped = 0;
		    }

		    else if ((options & POLYMULT_INVEC1_RLP) && (options & POLYMULT_INVEC2_RLP)) {
			addin1 = true_outvec_size - 1;				
			addin2 = true_invec2_size - 1;				
			addin3 = true_invec1_size - 1;				
			addin4 = 0;						
			MSWs_skipped = 1;					
			LSWs_skipped = 0;					
		    }

		    else if (!(options & POLYMULT_INVEC2_RLP)) {
			addin1 = true_outvec_size - 1;				
			addin2 = true_invec2_size - 1;				
			MSWs_skipped = 1;					
			LSWs_skipped = 0;
		    }

		    else if (!(options & POLYMULT_INVEC2_RLP)) {
			addin1 = true_outvec_size - 1;				
			addin3 = true_invec1_size - 1;				
			MSWs_skipped = 1;					
			LSWs_skipped = 0;
		    }

		    if ((options & POLYMULT_INVEC1_RLP) && (options & POLYMULT_INVEC2_RLP)) {
			LSWs_skipped = true_outvec_size / 2;			
		    }

		    if (options & POLYMULT_CIRCULAR) {
			MSWs_skipped = 0;
			LSWs_skipped = 0;
		    }

		    ASSERTG ((!(options & (POLYMULT_MULHI | POLYMULT_MULMID | POLYMULT_MULLO)) && outvec_size == circular_size - MSWs_skipped - LSWs_skipped) ||
			     ((options & (POLYMULT_MULHI | POLYMULT_MULLO)) && outvec_size <= circular_size - MSWs_skipped - LSWs_skipped) ||
			     ((options & POLYMULT_MULMID) && outvec_size <= circular_size - MSWs_skipped - LSWs_skipped - other_polys[i].first_mulmid));

		    if (options & POLYMULT_MULHI) LSWs_skipped = circular_size - MSWs_skipped - outvec_size;
		    if (options & POLYMULT_MULMID) LSWs_skipped += other_polys[i].first_mulmid;
		    if (options & (POLYMULT_MULMID | POLYMULT_MULLO)) MSWs_skipped = circular_size - LSWs_skipped - outvec_size;
		    ASSERTG (circular_size == MSWs_skipped + outvec_size + LSWs_skipped);

		    if (addin1 >= (int64_t) circular_size) addin1 %= circular_size;
		    if (addin2 >= (int64_t) circular_size) addin2 %= circular_size;
		    if (addin3 >= (int64_t) circular_size) addin3 %= circular_size;
		    if (addin4 >= (int64_t) circular_size) addin4 %= circular_size;

		    addin1 -= LSWs_skipped;
		    addin2 -= LSWs_skipped;
		    addin3 -= LSWs_skipped;
		    addin4 -= LSWs_skipped;

		    if (addin1 >= 0 && (addin1 >= (int64_t) outvec_size || outvec[addin1] == NULL)) addin1 = -1;
		    if (addin2 >= 0 && (addin2 >= (int64_t) outvec_size || outvec[addin2] == NULL)) addin2 = -1;
		    if (addin3 >= 0 && (addin3 >= (int64_t) outvec_size || outvec[addin3] == NULL)) addin3 = -1;
		    if (addin4 >= 0 && (addin4 >= (int64_t) outvec_size || outvec[addin4] == NULL)) addin4 = -1;

		    bool	will_brute_force;		
		    will_brute_force = !must_fft && (true_invec1_size < pmdata->KARAT_BREAK || true_invec2_size < pmdata->KARAT_BREAK);
		    
		    plan->planpart[i].addin[0] = addin1;
		    plan->planpart[i].addin[1] = addin2;
		    plan->planpart[i].addin[2] = addin3;
		    plan->planpart[i].addin[3] = addin4;
		    plan->planpart[i].LSWs_skipped = LSWs_skipped;
		    plan->planpart[i].MSWs_skipped = MSWs_skipped;

		    bool must_strip_a_monic;			
		    bool invec1_can_be_stripped_cheaply;	
		    bool invec2_can_be_stripped_cheaply;	
		    bool strip_monic_from_invec1, strip_monic_from_invec2, post_monic_addin_invec1, post_monic_addin_invec2;

		    must_strip_a_monic = (addin1 >= 0 || addin2 >= 0 || addin3 >= 0 || addin4 >= 0);

		    invec1_can_be_stripped_cheaply = (options & POLYMULT_INVEC1_MONIC) &&
				true_invec2_size - (((options & POLYMULT_INVEC2_MONIC) && (options & POLYMULT_INVEC2_RLP)) ? 1 : 0) <= MSWs_skipped &&
				(!(options & POLYMULT_INVEC1_RLP) || true_invec2_size - ((options & POLYMULT_INVEC2_MONIC) ? 1 : 0) <= LSWs_skipped);
		    invec2_can_be_stripped_cheaply = (options & POLYMULT_INVEC2_MONIC) &&
				true_invec1_size - (((options & POLYMULT_INVEC1_MONIC) && (options & POLYMULT_INVEC1_RLP)) ? 1 : 0) <= MSWs_skipped &&
				(!(options & POLYMULT_INVEC2_RLP) || true_invec1_size - ((options & POLYMULT_INVEC1_MONIC) ? 1 : 0) <= LSWs_skipped);

		    must_strip_a_monic |= (invec1_pre_ffted && !preprocessed_monics_included (invec1));
		    must_strip_a_monic |= (invec2_pre_ffted && !preprocessed_monics_included (invec2));

		    bool can_use_circular_fft_size = FALSE;		
		    bool can_use_circular_if_monic_stripped = FALSE;	
		    uint64_t circular_fft_size;
		    if (options & POLYMULT_CIRCULAR) {
			
			ASSERTG (!must_fft || (fft_size >= circular_size && (fft_size <= circular_size + LSWs_skipped + 1 || fft_size >= true_outvec_size)));
			
			circular_fft_size = must_fft ? fft_size : polymult_fft_size (circular_size);
			
			if (circular_size >= true_invec1_size && circular_size >= true_invec2_size &&
			    circular_fft_size < true_outvec_size &&
			    (circular_fft_size == circular_size ||
			     (true_outvec_size - circular_size <= LSWs_skipped && !must_strip_a_monic) ||
			     (true_outvec_size - circular_size <= LSWs_skipped + 1 && (options & (POLYMULT_INVEC1_MONIC | POLYMULT_INVEC2_MONIC)) &&
			      !((options & POLYMULT_INVEC1_MONIC) && (options & POLYMULT_INVEC2_MONIC) && (options & (POLYMULT_INVEC1_RLP | POLYMULT_INVEC2_RLP)))))) {
				can_use_circular_fft_size = TRUE;
				can_use_circular_if_monic_stripped = !(circular_fft_size == circular_size ||
								       (true_outvec_size - circular_size <= LSWs_skipped && !must_strip_a_monic));
			}
		    }

		    else if (MSWs_skipped > 0 && LSWs_skipped > 0 && (!must_fft || fft_size < true_outvec_size)) {
			uint64_t savings = (LSWs_skipped <= MSWs_skipped) ? LSWs_skipped : MSWs_skipped;
			circular_fft_size = polymult_fft_size (true_outvec_size - savings);
			if (must_fft && fft_size > circular_fft_size) circular_fft_size = fft_size;
			if (circular_fft_size < true_outvec_size) can_use_circular_fft_size = TRUE;

		    }

		    int	best_choice = 0;
		    uint64_t best_fft_size = 0;
		    uint64_t best_poly_addin_size = 0;

		    if (!must_strip_a_monic && (pass == 1 || !pass2_strip_monic_from_invec1)) {
			uint64_t possible_fft_size = polymult_fft_size (true_outvec_size);
			if (can_use_circular_fft_size && !can_use_circular_if_monic_stripped && circular_fft_size < possible_fft_size) possible_fft_size = circular_fft_size;
			if (must_fft && fft_size > possible_fft_size) possible_fft_size = fft_size;
			if (!must_fft && pass == 2 && possible_fft_size < largest_fft_size) possible_fft_size = largest_fft_size;
			if (!must_fft || possible_fft_size <= fft_size) {
				best_choice = 1;
				best_fft_size = possible_fft_size;
				best_poly_addin_size = 0;
				
				if (will_brute_force) goto use_it;
			}
		    }

		    if (!must_strip_a_monic && (pass == 1 || !pass2_strip_monic_from_invec1) &&
		    (options & POLYMULT_INVEC1_MONIC) && (options & POLYMULT_INVEC2_MONIC) && LSWs_skipped == 0) {
			uint64_t possible_fft_size = polymult_fft_size (true_outvec_size - 1);
			if ((must_fft || possible_fft_size >= pmdata->FFT_BREAK) &&
			    (best_choice == 0 || possible_fft_size < best_fft_size) &&
			    (pass == 1 || possible_fft_size == largest_fft_size) &&
			    (!must_fft || (must_fft && fft_size == possible_fft_size))) {
				best_choice = 2;
				best_fft_size = possible_fft_size;
				best_poly_addin_size = 0;
			}
		    }

		    if ((options & POLYMULT_INVEC2_MONIC) &&
			(pass == 1 || !pass2_strip_monic_from_invec1) &&
			(!invec2_pre_ffted || !preprocessed_monics_included (invec2)) &&
			(invec2_can_be_stripped_cheaply || !invec1_pre_ffted)) {
			uint64_t possible_fft_size = polymult_fft_size (true_outvec_size - (true_invec2_size - adjusted_invec2_size));
			uint64_t possible_poly_addin_size = (invec2_can_be_stripped_cheaply ? 0 : adjusted_invec1_size);
			if (can_use_circular_fft_size && circular_fft_size < possible_fft_size) possible_fft_size = circular_fft_size;
			if (!must_fft && pass == 2 && possible_fft_size < largest_fft_size) possible_fft_size = largest_fft_size;
			if (must_fft && fft_size > possible_fft_size) possible_fft_size = fft_size;
			if ((!must_fft || possible_fft_size <= fft_size) &&
			    (best_choice == 0 || possible_fft_size < best_fft_size ||
			     (possible_fft_size == best_fft_size && possible_poly_addin_size < best_poly_addin_size))) {
				best_choice = 3;
				best_fft_size = possible_fft_size;
				best_poly_addin_size = possible_poly_addin_size;
			}
		    }

		    if ((options & POLYMULT_INVEC1_MONIC) &&
			(pass == 1 || pass2_strip_monic_from_invec1) &&
			(!invec1_pre_ffted || !preprocessed_monics_included (invec1)) &&
			(invec1_can_be_stripped_cheaply || !invec2_pre_ffted)) {
			uint64_t possible_fft_size = polymult_fft_size (true_outvec_size - (true_invec1_size - adjusted_invec1_size));
			uint64_t possible_poly_addin_size = (invec1_can_be_stripped_cheaply ? 0 : adjusted_invec2_size);
			if (can_use_circular_fft_size && circular_fft_size < possible_fft_size) possible_fft_size = circular_fft_size;
			if (!must_fft && pass == 2 && possible_fft_size < largest_fft_size) possible_fft_size = largest_fft_size;
			if (must_fft && fft_size > possible_fft_size) possible_fft_size = fft_size;
			if ((!must_fft || possible_fft_size <= fft_size) &&
			    (best_choice == 0 || possible_fft_size < best_fft_size ||
			     (possible_fft_size == best_fft_size && possible_poly_addin_size < best_poly_addin_size))) {
				best_choice = 4;
				best_fft_size = possible_fft_size;
				best_poly_addin_size = possible_poly_addin_size;
			}
		    }

		    if ((options & POLYMULT_INVEC1_MONIC) && (options & POLYMULT_INVEC2_MONIC) &&
			(pass == 1 || pass2_strip_monic_from_invec1) &&
			(!invec1_pre_ffted || !preprocessed_monics_included (invec1)) &&
			(!invec2_pre_ffted || !preprocessed_monics_included (invec2)) &&
			(invec1_can_be_stripped_cheaply || !invec2_pre_ffted) &&
			(invec2_can_be_stripped_cheaply || !invec1_pre_ffted)) {
			uint64_t possible_fft_size = polymult_fft_size (adjusted_outvec_size);
			uint64_t possible_poly_addin_size = (invec1_can_be_stripped_cheaply ? 0 : adjusted_invec2_size) +
							    (invec2_can_be_stripped_cheaply ? 0 : adjusted_invec1_size);
			if (can_use_circular_fft_size && circular_fft_size < possible_fft_size) possible_fft_size = circular_fft_size;
			if (!must_fft && pass == 2 && possible_fft_size < largest_fft_size) possible_fft_size = largest_fft_size;
			if (must_fft && fft_size > possible_fft_size) possible_fft_size = fft_size;
			if ((!must_fft || possible_fft_size <= fft_size) &&
			    (best_choice == 0 || possible_fft_size < best_fft_size ||
			     (possible_fft_size == best_fft_size && possible_poly_addin_size < best_poly_addin_size))) {
				best_choice = 5;
				best_fft_size = possible_fft_size;
				best_poly_addin_size = possible_poly_addin_size;
			}
		    }

use_it:		    ASSERTG (best_choice != 0);

		    if (best_choice == 1) {		
			strip_monic_from_invec1 = post_monic_addin_invec2 = FALSE;
			strip_monic_from_invec2 = post_monic_addin_invec1 = FALSE;
			adjusted_invec1_size = true_invec1_size;
			adjusted_invec2_size = true_invec2_size;
		    }
		    if (best_choice == 2) {		
			strip_monic_from_invec1 = post_monic_addin_invec2 = FALSE;
			strip_monic_from_invec2 = post_monic_addin_invec1 = FALSE;
			adjusted_invec1_size = true_invec1_size;
			adjusted_invec2_size = true_invec2_size;
			circular_size = best_fft_size;
			subout = 0;			
		    }
		    if (best_choice == 3) {		
			strip_monic_from_invec1 = post_monic_addin_invec2 = FALSE;
			strip_monic_from_invec2 = TRUE; post_monic_addin_invec1 = !invec2_can_be_stripped_cheaply;
			adjusted_invec1_size = true_invec1_size;
		    }
		    if (best_choice == 4) {		
			strip_monic_from_invec1 = TRUE; post_monic_addin_invec2 = !invec1_can_be_stripped_cheaply;
			strip_monic_from_invec2 = post_monic_addin_invec1 = FALSE;
			adjusted_invec2_size = true_invec2_size;
		    }
		    if (best_choice == 5) {		
			strip_monic_from_invec1 = TRUE; post_monic_addin_invec2 = !invec1_can_be_stripped_cheaply;
			strip_monic_from_invec2 = TRUE; post_monic_addin_invec1 = !invec2_can_be_stripped_cheaply;
		    }
		    adjusted_outvec_size = adjusted_invec1_size + adjusted_invec2_size - 1;

		    if (pass == 1) {
			if (best_fft_size != largest_fft_size) {
				
				if (largest_fft_size != 0) must_replan = TRUE;
				
				if (best_fft_size > largest_fft_size) {
					largest_fft_size = best_fft_size;
					largest_strip_monic_from_invec1 = strip_monic_from_invec1;
				}
			} else {
				
				if (strip_monic_from_invec1 != largest_strip_monic_from_invec1) {
					largest_strip_monic_from_invec1 = TRUE;
					must_replan = TRUE;
				}
			}
		    }
		    
		    ASSERTG (pass == 1 || must_fft || best_fft_size == largest_fft_size);

		    if (will_brute_force) {
			plan->planpart[i].impl = POLYMULT_IMPL_BRUTE;
		    }
		    
		    else if (!must_fft && !can_use_circular_fft_size && adjusted_outvec_size < pmdata->FFT_BREAK) {
			plan->planpart[i].impl = POLYMULT_IMPL_KARATSUBA;
		    }
		    
		    else {
			plan->planpart[i].impl = POLYMULT_IMPL_FFT;
			plan->planpart[i].fft_size = fft_size = best_fft_size;
			int complex_vector_size = complex_vector_size_in_bytes (pmdata->gwdata->cpu_flags);
			if (pmdata->num_threads > 1 && fft_size >= pmdata->mt_ffts_start && fft_size < pmdata->mt_ffts_end)
				plan->mt_polymult_line = FALSE;
		    }
		    
		    plan->planpart[i].streamed_stores = (fft_size >= pmdata->streamed_stores_start);

		    if (can_use_circular_fft_size && plan->planpart[i].impl == POLYMULT_IMPL_FFT && fft_size == circular_fft_size) {
			circular_size = circular_fft_size;

		    }

		    plan->planpart[i].adjusted_pad = plan->planpart[i].adjusted_shift = 0;
		    if (post_monic_addin_invec1) {
			plan->planpart[i].adjusted_pad++;					
			if (options & POLYMULT_INVEC2_RLP) plan->planpart[i].adjusted_shift++;	
		    }
		    if (post_monic_addin_invec2) {
			plan->planpart[i].adjusted_pad++;					
			if (options & POLYMULT_INVEC1_RLP) plan->planpart[i].adjusted_shift++;	
		    }

		    plan->planpart[i].adjusted_invec1_size = adjusted_invec1_size;
		    plan->planpart[i].adjusted_invec2_size = adjusted_invec2_size;
		    plan->planpart[i].adjusted_outvec_size = adjusted_outvec_size;
		    plan->planpart[i].post_monic_addin_invec1 = post_monic_addin_invec1;
		    plan->planpart[i].post_monic_addin_invec2 = post_monic_addin_invec2;
		    plan->planpart[i].strip_monic_from_invec1 = strip_monic_from_invec1;
		    plan->planpart[i].strip_monic_from_invec2 = strip_monic_from_invec2;
		    plan->planpart[i].subout = subout;

		    plan->planpart[i].emulate_circular = ((options & POLYMULT_CIRCULAR) && circular_size != true_outvec_size &&
							  ((plan->planpart[i].impl == POLYMULT_IMPL_FFT && circular_size != fft_size) ||
							   plan->planpart[i].impl == POLYMULT_IMPL_KARATSUBA));
		    plan->planpart[i].circular_size = circular_size;

		    if (plan->planpart[i].impl == POLYMULT_IMPL_BRUTE) {
			if (adjusted_invec1_size > plan->alloc_invec1_size) plan->alloc_invec1_size = adjusted_invec1_size;
			if (adjusted_invec2_size > plan->alloc_invec2_size) plan->alloc_invec2_size = adjusted_invec2_size;
			if (true_outvec_size > plan->alloc_outvec_size) plan->alloc_outvec_size = true_outvec_size;
		    }
		    else if (plan->planpart[i].impl == POLYMULT_IMPL_KARATSUBA) {
			if (adjusted_invec1_size > plan->alloc_invec1_size) plan->alloc_invec1_size = adjusted_invec1_size;
			if (adjusted_invec2_size > plan->alloc_invec2_size) plan->alloc_invec2_size = adjusted_invec2_size;
			if (true_outvec_size > plan->alloc_outvec_size) plan->alloc_outvec_size = true_outvec_size;
			if (adjusted_outvec_size + 32 > plan->alloc_tmpvec_size) plan->alloc_tmpvec_size = adjusted_outvec_size + 32; 
		    }
		    else {
			
			if ((post_monic_addin_invec1 || fft_size != largest_fft_size || strip_monic_from_invec1 != largest_strip_monic_from_invec1) &&
			    true_invec1_size > plan->alloc_invec1_size)
				plan->alloc_invec1_size = true_invec1_size;
			
			if (fft_size > plan->alloc_tmpvec_size) plan->alloc_tmpvec_size = fft_size;
			
			if (post_monic_addin_invec2 && true_invec2_size > plan->alloc_invec2_size) plan->alloc_invec2_size = true_invec2_size;
			
			if (fft_size + plan->planpart[i].adjusted_pad + plan->planpart[i].adjusted_shift > plan->alloc_outvec_size)
				plan->alloc_outvec_size = fft_size + plan->planpart[i].adjusted_pad + plan->planpart[i].adjusted_shift;
		    }
		}

		pass2_strip_monic_from_invec1 = largest_strip_monic_from_invec1;
		
		if (exists_pre_ffted_invec2) {
		    if (largest_strip_monic_from_invec1) largest_fft_size = polymult_fft_size (largest_fft_size + 1);
		    pass2_strip_monic_from_invec1 = FALSE;
		}
	    }
	}

	pmdata->invec1 = invec1;
	pmdata->invec1_size = invec1_size;
	pmdata->other_polys = other_polys;
	pmdata->num_other_polys = num_other_polys;
	pmdata->options = invec1_options;
	pmdata->mt_polymult_line = plan->mt_polymult_line;

	pmdata->helper_opcode = HELPER_PRE_FFT;		
	polymult_launch_helpers (pmdata);

	pmdata->helper_opcode = HELPER_POLYMULT_LINE;	
	polymult_launch_helpers (pmdata);

	plan->unnorms1 = is_preprocessed_poly (invec1) ? preprocessed_top_unnorms (invec1) : unnorms (invec1[invec1_size-1]);
	for (int i = 0; i < num_other_polys; i++) {
		gwnum *invec2 = other_polys[i].invec2;			
		uint64_t invec2_size = other_polys[i].invec2_size;	
		plan->planpart[i].unnorms2 = is_preprocessed_poly (invec2) ? preprocessed_top_unnorms (invec2) : unnorms (invec2[invec2_size-1]);
	}

	pmdata->helper_opcode = HELPER_POST_UNFFT;	
	polymult_launch_helpers (pmdata);

	if (!(options & POLYMULT_SAVE_PLAN)) free (pmdata->plan), pmdata->plan = NULL;
}

void polymult_pre_fft (
	pmhandle *pmdata,		
	gwhandle *gwdata)		
{
	uint64_t j, atomic_val;		
	uint64_t atomics_processed = 0;	

	atomic_val = atomic_fetch_incr (pmdata->helper_counter);

	gwnum *invec1 = pmdata->invec1;
	if (! is_preprocessed_poly (invec1)) {
		uint64_t invec1_size = pmdata->invec1_size;
		for  ( ; (j = atomic_val - atomics_processed) < invec1_size; atomic_val = atomic_fetch_incr (pmdata->helper_counter)) {
			if (invec1[j] == NULL) continue;
			gwfft (gwdata, invec1[j], invec1[j]);
			if (gwdata->ZERO_PADDED_FFT) zpad7_fft (gwdata, invec1[j]);
		}
		atomics_processed += invec1_size;
	}

	for (int i = 0; i < pmdata->num_other_polys; i++) {
		gwnum *invec2 = pmdata->other_polys[i].invec2;
		
		if (! is_preprocessed_poly (invec2)) {
			uint64_t invec2_size = pmdata->other_polys[i].invec2_size;
			for  ( ; (j = atomic_val - atomics_processed) < invec2_size; atomic_val = atomic_fetch_incr (pmdata->helper_counter)) {
				if (invec2[j] == NULL) continue;
				gwfft (gwdata, invec2[j], invec2[j]);
				if (gwdata->ZERO_PADDED_FFT) zpad7_fft (gwdata, invec2[j]);
			}
			atomics_processed += invec2_size;
		}
		
		gwnum *fmavec = pmdata->other_polys[i].fmavec;
		if (fmavec != NULL) {
			uint64_t outvec_size = pmdata->other_polys[i].outvec_size;
			for  ( ; (j = atomic_val - atomics_processed) < outvec_size; atomic_val = atomic_fetch_incr (pmdata->helper_counter)) {
				if (fmavec[j] == NULL) continue;
				gwfft (gwdata, fmavec[j], fmavec[j]);
				if (gwdata->ZERO_PADDED_FFT) zpad7_fft (gwdata, fmavec[j]);
			}
			atomics_processed += outvec_size;
		}
	}
}

void polymult_post_unfft (
	pmhandle *pmdata,		
	gwhandle *gwdata)		
{
	uint64_t j, atomic_val;			
	uint64_t atomics_processed = 0;		
	polymult_plan *plan = pmdata->plan;	

	atomic_val = atomic_fetch_incr (pmdata->helper_counter);

	for (int i = 0; i < pmdata->num_other_polys; i++) {
		gwnum	*outvec = pmdata->other_polys[i].outvec;
		uint64_t outvec_size = pmdata->other_polys[i].outvec_size;
		int	options = pmdata->other_polys[i].options + pmdata->options;
		int	addin_value = 1;

		if (options & POLYMULT_FNMADD) addin_value = -1;

		for  ( ; (j = atomic_val - atomics_processed) < outvec_size; atomic_val = atomic_fetch_incr (pmdata->helper_counter)) {
			if (outvec[j] == NULL) continue;
			if (gwdata->ZERO_PADDED_FFT) zpad7_unfft (gwdata, outvec[j]);

			FFT_state (outvec[j]) = (!gwdata->GENERAL_MMGW_MOD && gwdata->k == 1.0 ? FULLY_FFTed : FFTed_FOR_FMA);
			if (j == outvec_size - 1 &&							
			    plan->planpart[i].impl != POLYMULT_IMPL_FFT &&				
			    ! (options & POLYMULT_UNFFT_TOP) &&						
			    ((options & POLYMULT_INVEC1_MONIC && options & POLYMULT_INVEC2_MONIC &&	
			      ((plan->planpart[i].circular_size == plan->planpart[i].true_outvec_size &&
			        plan->planpart[i].MSWs_skipped == 1) ||					
			       (plan->planpart[i].circular_size == plan->planpart[i].true_outvec_size - 1 &&
			        plan->planpart[i].MSWs_skipped == 0))) ||				
			     (!(options & POLYMULT_INVEC1_MONIC) != !(options & POLYMULT_INVEC2_MONIC) && 
			      plan->planpart[i].circular_size == plan->planpart[i].true_outvec_size &&	
			      plan->planpart[i].MSWs_skipped == 0))) {					
				float	unnorms1 = plan->unnorms1;		
				float	unnorms2 = plan->planpart[i].unnorms2;	
				
				ASSERTG (gwdata->polymult_safety_margin > 0.0);
				if (! (options & POLYMULT_INVEC2_MONIC))			
					unnorms (outvec[j]) = unnorms2;
				else if (! (options & POLYMULT_INVEC1_MONIC))			
					unnorms (outvec[j]) = unnorms1;
				else								
					unnorms (outvec[j]) = unnorms1 + unnorms2 + 1.0f;
			} else
				unnorms (outvec[j]) = 999.0e9f;					

			bool j_affected_by_addin = (j == plan->planpart[i].addin[0] || j == plan->planpart[i].addin[1] ||
						    j == plan->planpart[i].addin[2] || j == plan->planpart[i].addin[3] || j == plan->planpart[i].subout);
			if (!j_affected_by_addin) {
				if (options & POLYMULT_NO_UNFFT) continue;
				if (!polymult_must_unfft (gwdata, outvec[j])) continue;
			}

			gwunfft2 (gwdata, outvec[j], outvec[j],
				  ((options & (POLYMULT_STARTNEXTFFT | POLYMULT_NEXTFFT)) && !j_affected_by_addin) ? GWMUL_STARTNEXTFFT : 0);
			if (j_affected_by_addin) {
				if (j == plan->planpart[i].addin[0]) gwaddsmall (gwdata, outvec[j], addin_value);
				if (j == plan->planpart[i].addin[1]) gwaddsmall (gwdata, outvec[j], addin_value);
				if (j == plan->planpart[i].addin[2]) gwaddsmall (gwdata, outvec[j], addin_value);
				if (j == plan->planpart[i].addin[3]) gwaddsmall (gwdata, outvec[j], addin_value);
				if (j == plan->planpart[i].subout) gwaddsmall (gwdata, outvec[j], -addin_value);
			}

			if (options & POLYMULT_NEXTFFT) gwfft (gwdata, outvec[j], outvec[j]);
		}
		atomics_processed += outvec_size;
	}
}

const int HELPER_OPCODE_COPY = 100001;
const int HELPER_OPCODE_FFT = 100002;
const int HELPER_OPCODE_UNFFT = 100003;
const int HELPER_OPCODE_UNFFT_FFT = 100004;

struct poly_helper_example_data {
	pmhandle *pmdata;		
	int	opcode;			
	union {
		struct {		
			gwnum	*invec;
			gwnum	*outvec;
			uint64_t poly_size;
		} copy_args;
		struct {		
			gwnum	*vec;
			uint64_t poly_size;
		} fft_args;
	};
};
   
void poly_copy (pmhandle *pmdata, gwnum *invec, gwnum *outvec, uint64_t poly_size) {
	struct poly_helper_example_data helper_data;
	
	helper_data.opcode = HELPER_OPCODE_COPY;			
	helper_data.pmdata = pmdata;					
	helper_data.copy_args.invec = invec;				
	helper_data.copy_args.outvec = outvec;				
	helper_data.copy_args.poly_size = poly_size;			
	
	pmdata->helper_callback = &poly_helper_example;
	pmdata->helper_callback_data = &helper_data;
	polymult_launch_helpers (pmdata);				
}

void poly_fft_coefficients (pmhandle *pmdata, gwnum *vec, uint64_t poly_size) {
	struct poly_helper_example_data helper_data;
	
	helper_data.opcode = HELPER_OPCODE_FFT;				
	helper_data.pmdata = pmdata;					
	helper_data.fft_args.vec = vec;					
	helper_data.fft_args.poly_size = poly_size;			
	
	pmdata->helper_callback = &poly_helper_example;
	pmdata->helper_callback_data = &helper_data;
	polymult_launch_helpers (pmdata);				
}
void poly_unfft_coefficients (pmhandle *pmdata, gwnum *vec, uint64_t poly_size) {
	struct poly_helper_example_data helper_data;
	
	helper_data.opcode = HELPER_OPCODE_UNFFT;			
	helper_data.pmdata = pmdata;					
	helper_data.fft_args.vec = vec;					
	helper_data.fft_args.poly_size = poly_size;			
	
	pmdata->helper_callback = &poly_helper_example;
	pmdata->helper_callback_data = &helper_data;
	polymult_launch_helpers (pmdata);				
}
void poly_unfft_fft_coefficients (pmhandle *pmdata, gwnum *vec, uint64_t poly_size) {
	struct poly_helper_example_data helper_data;
	
	helper_data.opcode = HELPER_OPCODE_UNFFT_FFT;			
	helper_data.pmdata = pmdata;					
	helper_data.fft_args.vec = vec;					
	helper_data.fft_args.poly_size = poly_size;			
	
	pmdata->helper_callback = &poly_helper_example;
	pmdata->helper_callback_data = &helper_data;
	polymult_launch_helpers (pmdata);				
}

void poly_helper_example (
	int	helper_num,		
	gwhandle *gwdata,		
	void	*info)			
{
	struct poly_helper_example_data *helper_data = (struct poly_helper_example_data *) info;
	pmhandle *pmdata = helper_data->pmdata;

	if (helper_data->opcode == HELPER_OPCODE_COPY) {
		
		gwnum *invec = helper_data->copy_args.invec;
		gwnum *outvec = helper_data->copy_args.outvec;
		for ( ; ; ) {
			
			uint64_t coeff = atomic_fetch_incr (pmdata->helper_counter);

			if (coeff >= helper_data->copy_args.poly_size) break;

			gwcopy (gwdata, invec[coeff], outvec[coeff]);
		}
	}

	else if (helper_data->opcode == HELPER_OPCODE_FFT) {
		
		gwnum *vec = helper_data->fft_args.vec;
		for ( ; ; ) {
			
			uint64_t coeff = atomic_fetch_incr (pmdata->helper_counter);

			if (coeff >= helper_data->fft_args.poly_size) break;

			gwfft (gwdata, vec[coeff], vec[coeff]);
		}
	}

	else if (helper_data->opcode == HELPER_OPCODE_UNFFT) {
		
		gwnum *vec = helper_data->fft_args.vec;
		for ( ; ; ) {
			
			uint64_t coeff = atomic_fetch_incr (pmdata->helper_counter);

			if (coeff >= helper_data->fft_args.poly_size) break;

			if (polymult_must_unfft (gwdata, vec[coeff])) gwunfft (gwdata, vec[coeff], vec[coeff]);
		}
	}

	else if (helper_data->opcode == HELPER_OPCODE_UNFFT_FFT) {
		
		gwnum *vec = helper_data->fft_args.vec;
		for ( ; ; ) {
			
			uint64_t coeff = atomic_fetch_incr (pmdata->helper_counter);

			if (coeff >= helper_data->fft_args.poly_size) break;

			if (polymult_must_unfft (gwdata, vec[coeff])) gwunfft2 (gwdata, vec[coeff], vec[coeff], GWMUL_STARTNEXTFFT);
			gwfft (gwdata, vec[coeff], vec[coeff]);
		}
	}

	else
		ASSERTG(FALSE);
}

#endif
