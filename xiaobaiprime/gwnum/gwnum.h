// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

#ifndef _GWNUM_H
#define _GWNUM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "gwcommon.h"
#include "giants.h"
#include "gwthread.h"

typedef struct gwhandle_struct gwhandle;

typedef double *gwnum;

typedef gwnum *gwarray;

#define GWNUM_VERSION		"30.19"
#define GWNUM_MAJOR_VERSION	30
#define GWNUM_MINOR_VERSION	19

#define GWERROR_VERSION		1001	
#define GWERROR_TOO_LARGE	1002	
#define GWERROR_K_TOO_SMALL	1003	
#define GWERROR_K_TOO_LARGE	1004	
#define GWERROR_MALLOC		1005	
#define GWERROR_VERSION_MISMATCH 1006	
#define GWERROR_STRUCT_SIZE_MISMATCH 1007 
					
#define GWERROR_TOO_SMALL	1008	
#define GWERROR_NO_INIT		1009	
#define GWERROR_ZERO_THREADS	1010	
#define GWERROR_INTERNAL	2000	

#define GWERROR_BAD_FFT_DATA	-1	
#define GWERROR_PARTIAL_FFT	-1009	
#define GWERROR_FFT		-1010	
#define GWERROR_FFT_FOR_FMA	-1011	

#define gwinit(h)		gwinit2 (h, sizeof (gwhandle), GWNUM_VERSION)

void gwinit2 (
	gwhandle *gwdata,	
	int	struct_size,	
	const char *version_string);

int gwsetup (
	gwhandle *gwdata,	
	double	k,		
	unsigned long b,	
	unsigned long n,	
	signed long c);		

int gwsetup_general_mod (
	gwhandle *gwdata,	
	const uint32_t *array,	
	uint32_t arraylen);	
int gwsetup_general_mod_64 (
	gwhandle *gwdata,	
	const uint64_t *array,	
	uint64_t arraylen);	

int gwsetup_without_mod (
	gwhandle *gwdata,	
	unsigned long n);	

void gwdone (
	gwhandle *gwdata);	

#define gwset_num_threads(h,n)		((h)->num_threads = n)
#define gwget_num_threads(h)		((h)->num_threads)

#define gwset_thread_callback(h,n)		((h)->thread_callback = n)
#define gwset_thread_callback_data(h,d)		((h)->thread_callback_data = d)

#define gwset_safety_margin(h,m)	((h)->safety_margin = (float) (m))

#define gwset_maxmulbyconst(h,c)	((h)->maxmulbyconst = c)
#define gwsetmaxmulbyconst		gwset_maxmulbyconst

#define gwset_irrational_general_mod(h,b)  ((h)->use_irrational_general_mod = (char) (b))

#define gwset_larger_fftlen_count(h,n)	((h)->larger_fftlen_count = n)

#define gwset_minimum_fftlen(h,n)	((h)->minimum_fftlen = n)

#define gwset_will_hyperthread(h,n)	((h)->will_hyperthread = n)
#define gwclear_will_hyperthread(h)	((h)->will_hyperthread = 0)

#define gwset_use_benchmarks(h)		((h)->use_benchmarks = 1)
#define gwclear_use_benchmarks(h)	((h)->use_benchmarks = 0)

#define gwset_bench_cores(h,n)		((h)->bench_num_cores = n)

#define gwset_bench_workers(h,n)	((h)->bench_num_workers = n)

#define gwset_will_error_check(h)		((h)->will_error_check = 1)
#define gwset_will_error_check_near_limit(h)	((h)->will_error_check = 2)
#define gwclear_will_error_check(h)		((h)->will_error_check = 0)

#define gwset_use_large_pages(h)	((h)->use_large_pages = 1)
#define gwclear_use_large_pages(h)	((h)->use_large_pages = 0)
#define gwget_use_large_pages(h)	((h)->use_large_pages)
#define gw_using_large_pages(h)		((h)->large_pages_ptr != NULL)

#define gwset_use_spin_wait(h,n)	((h)->use_spin_wait = (char) (n))

#define gwset_using_polymult(h)		((h)->polymult = TRUE)

#define gwset_information_only(h)	((h)->information_only = TRUE)

#define gwset_hyperthread_prefetch(h)	
#define gwclear_hyperthread_prefetch(h)	

#define gwset_specific_fftlen(h,n)	((h)->minimum_fftlen = n)

gwnum gwalloc (
	gwhandle *gwdata);	

void gwfree (
	gwhandle *gwdata,	
	gwnum	val);		

gwarray gwalloc_array (		
	gwhandle *gwdata,	
	uint64_t n);		

void gwfree_array (
	gwhandle *gwdata,	
	gwarray	array);		

void gwfreeall (
	gwhandle *gwdata);	

void gwfree_internal_memory (
	gwhandle *gwdata);	

void gwfree_cached (
	gwhandle *gwdata);	

void gwuser_init_FFT1 (		
	gwhandle *gwdata);	

void dbltogw (gwhandle *, double, gwnum);

void s64togw (gwhandle *, int64_t, gwnum);

void u64togw (gwhandle *, uint64_t, gwnum);

void binarytogw (
	gwhandle *gwdata,	
	const uint32_t *array,	
	uint32_t arraylen,	
	gwnum	n);		

void binary64togw (
	gwhandle *gwdata,	
	const uint64_t *array,	
	uint64_t arraylen,	
	gwnum	n);		

long gwtobinary (
	gwhandle *gwdata,	
	gwnum	n,		
	uint32_t *array,	
	uint32_t arraylen);	

long gwtobinary64 (
	gwhandle *gwdata,	
	gwnum	n,		
	uint64_t *array,	
	uint32_t arraylen);	

void gw_random_number (
	gwhandle *gwdata,	
	gwnum	x);		

#define gwswap(s,d)		{gwnum t; t = s; s = d; d = t;}
#define gwsquare2(h,s,d,opt)	gwmul3 (h,s,s,d,opt)

void gwerror_checking (gwhandle *, int e);		

#define GWMULBYCONST_MAX	255		
void gwsetmulbyconst (gwhandle *gwdata, long s);

void gwcopy (			
	gwhandle *gwdata,	
	gwnum	s,		
	gwnum	d);		

void gwfft (			
	gwhandle *gwdata,	
	gwnum	s,		
	gwnum	d);		

void gwfft_for_fma (		
				
	gwhandle *gwdata,	
	gwnum	s,		
	gwnum	d);		

#define GWMUL_FFT_S1		0x0001		
#define GWMUL_PRESERVE_S1	0x0002		
#define GWMUL_FFT_S2		0x0004		
#define GWMUL_PRESERVE_S2	0x0008		
#define GWMUL_FFT_S3		0x0010		
#define GWMUL_PRESERVE_S3	0x0020		
#define GWMUL_FFT_S4		0x0040		
#define GWMUL_PRESERVE_S4	0x0080		
#define GWMUL_ADDINCONST	0x0100		
#define GWMUL_MULBYCONST	0x0200		
#define GWMUL_STARTNEXTFFT	0x0400		

#define GWMUL_FFT_S12		(GWMUL_FFT_S1 | GWMUL_FFT_S2)
#define GWMUL_FFT_S13		(GWMUL_FFT_S1 | GWMUL_FFT_S3)
#define GWMUL_FFT_S14		(GWMUL_FFT_S1 | GWMUL_FFT_S4)
#define GWMUL_FFT_S23		(GWMUL_FFT_S2 | GWMUL_FFT_S3)
#define GWMUL_FFT_S24		(GWMUL_FFT_S2 | GWMUL_FFT_S4)
#define GWMUL_FFT_S34		(GWMUL_FFT_S3 | GWMUL_FFT_S4)
#define GWMUL_FFT_S123		(GWMUL_FFT_S1 | GWMUL_FFT_S2 | GWMUL_FFT_S3)
#define GWMUL_FFT_S124		(GWMUL_FFT_S1 | GWMUL_FFT_S2 | GWMUL_FFT_S4)
#define GWMUL_FFT_S134		(GWMUL_FFT_S1 | GWMUL_FFT_S3 | GWMUL_FFT_S4)
#define GWMUL_FFT_S234		(GWMUL_FFT_S2 | GWMUL_FFT_S3 | GWMUL_FFT_S4)
#define GWMUL_FFT_S1234		(GWMUL_FFT_S1 | GWMUL_FFT_S2 | GWMUL_FFT_S3 | GWMUL_FFT_S4)

void gwmul3 (			
	gwhandle *gwdata,	
	gwnum	s1,		
	gwnum	s2,		
	gwnum	d,		
	int	options);
void gwmul3_carefully (		
	gwhandle *gwdata,	
	gwnum	s1,		
	gwnum	s2,		
	gwnum	d,		
	int	options);
void gwaddmul4 (		
	gwhandle *gwdata,	
	gwnum	s1,		
	gwnum	s2,		
	gwnum	s3,		
	gwnum	d,		
	int	options);
void gwsubmul4 (		
	gwhandle *gwdata,	
	gwnum	s1,		
	gwnum	s2,		
	gwnum	s3,		
	gwnum	d,		
	int	options);
void gwmuladd4 (		
	gwhandle *gwdata,	
	gwnum	s1,		
	gwnum	s2,		
	gwnum	s3,		
	gwnum	d,		
	int	options);
void gwmulsub4 (		
	gwhandle *gwdata,	
	gwnum	s1,		
	gwnum	s2,		
	gwnum	s3,		
	gwnum	d,		
	int	options);
void gwmulmuladd5 (		
	gwhandle *gwdata,	
	gwnum	s1,		
	gwnum	s2,		
	gwnum	s3,		
	gwnum	s4,		
	gwnum	d,		
	int	options);	
void gwmulmulsub5 (		
	gwhandle *gwdata,	
	gwnum	s1,		
	gwnum	s2,		
	gwnum	s3,		
	gwnum	s4,		
	gwnum	d,		
	int	options);	

#define GWADD_SQUARE_INPUT		0x0001	
#define GWADD_MANY_INPUTS		0x0001	
#define GWADD_MUL_INPUT			0x0002	
#define GWADD_ADD_INPUT			0x0004	
#define GWADD_NON_RANDOM_DATA		0x0010	
#define GWADD_GUARANTEED_OK		0x2000	
#define GWADD_DELAY_NORMALIZE		0x4000	
#define GWADD_FORCE_NORMALIZE		0x8000	

void gwadd3o (			
	gwhandle *gwdata,	
	gwnum	s1,		
	gwnum	s2,		
	gwnum	d,		
	int	options);
void gwsub3o (			
	gwhandle *gwdata,	
	gwnum	s1,		
	gwnum	s2,		
	gwnum	d,		
	int	options);
void gwaddsub4o (		
	gwhandle *gwdata,	
	gwnum	s1,		
	gwnum	s2,		
	gwnum	d1,		
	gwnum	d2,		
	int	options);

void gwset_carefully_count (
	gwhandle *gwdata,	
	int	n);		
				
void gwsetaddin (gwhandle *, long);

#define is_gwsetaddin_emulated(h)	((h)->k != 1.0 && labs ((h)->c) != 1)

void gwsetaddinatpowerofb (gwhandle *, long, unsigned long);

void gwsetpostmulbyconstaddin (gwhandle *, int);

void gwsmalladd (gwhandle *gwdata, int64_t addin, gwnum g);

#define GWSMALLMUL_MAX		67108864.0		
void gwsmallmul (gwhandle *gwdata, double mult, gwnum g);

void gwunfft (gwhandle *, gwnum s, gwnum d);
void gwunfft2 (gwhandle *, gwnum s, gwnum d, int options);

#define gwtouch(h,s)		gwcopy (h,s,s)

#define gw_test_for_error(h)		((h)->GWERROR)
#define gw_test_illegal_sumout(h)	((h)->GWERROR & 1)
#define gw_clear_error(h)		((h)->GWERROR = 0)

double gw_get_maxerr (gwhandle *gwdata);
void gw_clear_maxerr (gwhandle *gwdata);

int gwnear_fft_limit (gwhandle *gwdata, double pct);

#define gw_passes_safety_margin(h,safetyval)		((h)->EXTRA_BITS/2.0f > (safetyval))

void gwset_polymult_safety_margin (gwhandle *gwdata, float safetyval);

void gwerror_text (gwhandle *gwdata, int error_code, char *buf, int buflen);

#define gwnum_is_gpl()		(0)

#define gwfftlen(h)		((h)->FFTLEN)

void gwfft_description (gwhandle *, char *buf);

#define gwmodulo_as_string(h)	((h)->GWSTRING_REP)

uint64_t gw_get_fft_count (gwhandle *);
void gw_clear_fft_count (gwhandle *);

unsigned long gwnum_size (gwhandle *);

unsigned long gwmemused (gwhandle *);

#define gwfma_will_alloc_a_gwnum(h)	((h)->FFT1_state != 2)

#define gwnum_is_normalized(h,g)	(unnorms(g) == 0.0f)

#define gwnum_is_not_ffted(h,g)		(FFT_state(g) == NOT_FFTed)
#define gwnum_is_partially_ffted(h,g)	(FFT_state(g) == PARTIALLY_FFTed)
#define gwnum_is_fully_ffted(h,g)	(FFT_state(g) == FULLY_FFTed)
#define gwnum_is_ffted_for_fma(h,g)	(FFT_state(g) == FFTed_FOR_FMA)

#define FFT_state(x)		((uint32_t *)(x))[-7]
#define	NOT_FFTed		0
#define PARTIALLY_FFTed		1
#define FULLY_FFTed		3
#define	FFTed_FOR_FMA		4
#define unnorms(x)		((float *)(x))[-1]

#define gwnum_datasize(h)	(h)->datasize

#define GWADD_NORMALIZE_IF(b)		((b) ? GWADD_FORCE_NORMALIZE : GWADD_DELAY_NORMALIZE)
#define GWADD_DELAYNORM_IF(b)		GWADD_NORMALIZE_IF(!(b))
#define GWMUL_STARTNEXTFFT_IF(b)	((b) ? GWMUL_STARTNEXTFFT : 0)

#define square_safe(h,numadds1)				((h)->EXTRA_BITS >= EB_GWMUL_SAVINGS + numadds_to_eb(numadds1) * 2.0f)
#define mul_safe(h,numadds1,numadds2)			((h)->EXTRA_BITS >= numadds_to_eb(numadds1) + numadds_to_eb(numadds2))
#define addmul_safe(h,numadds1,numadds2,numadds3)	mul_safe(h,(numadds1)+(numadds2)+1,numadds3)
#define muladd_safe(h,numadds1,numadds2,numadds3)	(FFT1_is_simple(h) ? mul_safe(h,numadds1,numadds2) : mulmuladd_safe(h,numadds1,numadds2,0,numadds3))
#define squareadd_safe(h,numadds1,numadds2,numadds3)	(FFT1_is_simple(h) ? square_safe(h,numadds1) : squaremuladd_safe(h,numadds1,numadds2,0,numadds3))
#define FFT1_is_simple(h)				(!(h)->GENERAL_MMGW_MOD && ((h)->k == 1.0 || labs((h)->c) == 1))

#define mulmuladd_safe(h,adds1,adds2,adds3,adds4)	((h)->EXTRA_BITS >= mma5_pairs_eb((adds1)+(adds2),(adds3)+(adds4)))
#define squaremuladd_safe(h,adds1,adds2,adds3,adds4)	((h)->EXTRA_BITS >= mma5_sqr_penalty+mma5_pairs_eb(2*(adds1)+1,2*(adds3)))
#define mulsquareadd_safe(h,adds1,adds2,adds3,adds4)	((h)->EXTRA_BITS >= mma5_sqr_penalty+mma5_pairs_eb(2*(adds1),2*(adds3)+1))
#define squaresquareadd_safe(h,adds1,adds2,adds3,adds4)	((h)->EXTRA_BITS >= 2*mma5_sqr_penalty+mma5_pairs_eb(2*(adds1)+1,2*(adds3)+1))
#define mma5_pairs_eb(sum1,sum2)			((sum1)<(sum2)?numadds_to_eb(sum1)+numadds_to_eb((sum2)+1):numadds_to_eb((sum1)+1)+numadds_to_eb(sum2))	
#define mma5_sqr_penalty				(EB_GWMUL_SAVINGS - EB_FIRST_ADD)

int gwiszero (gwhandle *, gwnum);

int gwequal (gwhandle *, gwnum, gwnum);

int gwclone (
	gwhandle *cloned_gwdata,	
	gwhandle *gwdata);		

void gwclone_merge_stats (
	gwhandle *dest_gwdata,		
	gwhandle *cloned_gwdata);	

#include "giants.h"

int gwsetup_general_mod_giant (
	gwhandle *gwdata,	
	giant n);		

void gianttogw (gwhandle *, giant, gwnum);

int gwtogiant (gwhandle *, gwnum, giant);

#define MAX_ZEROPAD_K	2251799813685247.0	

#define MAX_ZEROPAD_C	8388607			

#define EB_GWMUL_SAVINGS	0.5271f
#define EB_FIRST_ADD		0.5089f
#define EB_SECOND_ADD		0.2879f
#define EB_THIRD_ADD		0.2177f

#define numadds_to_eb(n)	((n) > 2.0f ? EB_FIRST_ADD + EB_SECOND_ADD + ((n) - 2.0f) * EB_THIRD_ADD : \
				 (n) > 1.0f ? EB_FIRST_ADD + EB_SECOND_ADD : (n) > 0.0f ? EB_FIRST_ADD : 0.0f)
#define eb_to_numadds(n)	((n) > EB_FIRST_ADD + EB_SECOND_ADD ? 2.0f + ((n) - EB_FIRST_ADD - EB_SECOND_ADD) / EB_THIRD_ADD : \
				 (n) > EB_FIRST_ADD ? 2.0f : (n) > 0.0f ? 1.0f : 0.0f)

#define oldmulbyconst(h)		((h)->GLOBAL_MULBYCONST ? GWMUL_MULBYCONST : 0)	
#define oldstartnextfft(h)		((h)->GLOBAL_POSTFFT ? GWMUL_STARTNEXTFFT : 0)	

#define gwsquare(h,s)			gwsquare2 (h,s,s,oldstartnextfft(h) | oldmulbyconst(h) | GWMUL_ADDINCONST)
#define gwsquare2_deprecated(h,s,d)	gwsquare2 (h,s,d,oldstartnextfft(h) | oldmulbyconst(h) | GWMUL_ADDINCONST)
#define gwmul(h,s,d)			gwmul3(h,s,d,d,GWMUL_FFT_S1 | oldstartnextfft(h) | oldmulbyconst(h) | GWMUL_ADDINCONST)
#define gwsafemul(h,s,d)		gwmul3(h,s,d,d,GWMUL_PRESERVE_S1 | oldstartnextfft(h) | oldmulbyconst(h) | GWMUL_ADDINCONST)
#define gwfftmul(h,s,d)			gwmul3(h,s,d,d,oldstartnextfft(h) | oldmulbyconst(h) | GWMUL_ADDINCONST)
#define gwfftfftmul(h,s1,s2,d)		gwmul3(h,s1,s2,d,oldstartnextfft(h) | oldmulbyconst(h) | GWMUL_ADDINCONST)
#define gwsquare_carefully(h,x)		gwmul3_carefully(h,x,x,x,oldstartnextfft(h) | oldmulbyconst(h) | GWMUL_ADDINCONST)
#define gwsquare2_carefully(h,s,d)	gwmul3_carefully(h,s,s,d,GWMUL_PRESERVE_S1 | GWMUL_PRESERVE_S2 | oldstartnextfft(h) | oldmulbyconst(h) | GWMUL_ADDINCONST)
#define gwmul_carefully(h,s,d)		gwmul3_carefully(h,s,d,d,GWMUL_PRESERVE_S1 | oldstartnextfft(h) | oldmulbyconst(h) | GWMUL_ADDINCONST)
#define gwset_square_carefully_count(h,n) DEPRECATED - use gwset_carefully_count

#define gwadd(h,s,d)			gwadd3 (h,s,d,d)
#define gwsub(h,s,d)			gwsub3 (h,d,s,d)
#define gwaddsub(h,a,b)			gwaddsub4 (h,a,b,a,b)
#define gwaddquick(h,s,d)		gwadd3quick (h,s,d,d)
#define gwsubquick(h,s,d)		gwsub3quick (h,d,s,d)
#define gwaddsubquick(h,a,b)		gwaddsub4quick (h,a,b,a,b)
#define gwfftadd(h,s,d)			gwadd3 (h,s,d,d)
#define gwfftsub(h,s,d)			gwsub3 (h,d,s,d)
#define gwfftaddsub(h,a,b)		gwaddsub4 (h,a,b,a,b)
#define gwfftadd3(h,s1,s2,d)		gwadd3 (h,s1,s2,d)
#define gwfftsub3(h,s1,s2,d)		gwsub3 (h,s1,s2,d)
#define gwfftaddsub4(h,s1,s2,d1,d2)	gwaddsub4 (h,s1,s2,d1,d2)
#define gwadd3(h,s1,s2,d)		gwadd3o (h,s1,s2,d,GWADD_MUL_INPUT)
#define gwsub3(h,s1,s2,d)		gwsub3o (h,s1,s2,d,GWADD_MUL_INPUT)
#define gwaddsub4(h,s1,s2,d1,d2)	gwaddsub4o (h,s1,s2,d1,d2,GWADD_MUL_INPUT)
#define gwadd3quick(h,s1,s2,d)		gwadd3o (h,s1,s2,d,GWADD_DELAY_NORMALIZE)
#define gwsub3quick(h,s1,s2,d)		gwsub3o (h,s1,s2,d,GWADD_DELAY_NORMALIZE)
#define gwaddsub4quick(h,s1,s2,d1,d2)	gwaddsub4o (h,s1,s2,d1,d2,GWADD_DELAY_NORMALIZE)
#define force_normalize(x)		DEPRECATED - use GWADD_FORCE_NORMALIZE

#define gwset_sum_inputs_checking(h,b)
#define gw_test_mismatched_sums(h)	FALSE
#define gwsuminp(h,g)			((g)[-2])
#define gwsumout(h,g)			((g)[-3])

#define gwsetnormroutine(h,z,e,c) {(h)->GLOBAL_MULBYCONST=(c)?1:0;gwerror_checking(h,e);}

#define gwstartnextfft(h,state)	{(h)->GLOBAL_POSTFFT = (state);}

#define gwaddsmall(h,g,a) gwsmalladd(h,a,g)	
#define gwmulsmall(h,g,m) gwsmallmul(h,m,g)
#define GWSMALLADD_MAX		1125899906842624	

#define gwsetaddinatbit(h,v,b)	gwsetaddinatpowerofb(h,v,b)

#define norm_count(h)	unnorms(h)

#define ALL_COMPLEX_FFT	NEGACYCLIC_FFT

void binarylongstogw (
	gwhandle *gwdata,	
	const unsigned long *array, 
	unsigned long arraylen,	
	gwnum	n);		

long gwtobinarylongs (
	gwhandle *gwdata,	
	gwnum	n,		
	unsigned long *array,	
	unsigned long arraylen);

#define ES1_SUCCESS		0	
#define ES1_FACTOR_FOUND	1	
#define ES1_CANNOT_DO_IT	2	
#define ES1_MEMORY		3	
#define ES1_INTERRUPT		4	
#define ES1_CANNOT_DO_QUICKLY	5	
#define ES1_HARDWARE_ERROR	6	

#define ES1_DO_SLOW_CASE	0x1	

int gwnum_ecmStage1_u32 (
	double	k,			
	unsigned long b,		
	unsigned long n,		
	signed long c,			
	uint32_t *num_being_factored_array, 
	unsigned long num_being_factored_array_len,
	uint64_t B1,			
	uint64_t *B1_done,		
	uint32_t *A_array,		
	unsigned long A_array_len,
	uint32_t *x_array,		
	unsigned long *x_array_len,
	uint32_t *z_array,		
	unsigned long *z_array_len,
	int	(*stop_check_proc)(int),
					
	unsigned long options);

int gwnum_ecmStage1_u64 (
	double	k,			
	unsigned long b,		
	unsigned long n,		
	signed long c,			
	uint64_t *num_being_factored_array, 
	unsigned long num_being_factored_array_len,
	uint64_t B1,			
	uint64_t *B1_done,		
	uint64_t *A_array,		
	unsigned long A_array_len,
	uint64_t *x_array,		
	unsigned long *x_array_len,
	uint64_t *z_array,		
	unsigned long *z_array_len,
	int	(*stop_check_proc)(int),
					
	unsigned long options);

struct gwasm_jmptab {
	uint32_t max_exp;	
	uint32_t fftlen;	
	float	timing;		
	uint32_t flags;		
	void	*proc_ptr;	
	uint32_t mem_needed;	
	int32_t counts[8];
};

struct gwasm_alt_jmptab {	
	uint32_t max_exp;	
	uint32_t fftlen;	
	float	timing;		
	uint32_t flags;		
	void	*proc_ptr;	
	void	*pass2_proc_ptr;
	uint32_t mem_needed;	
	int32_t counts[8];
};

struct pass1_carry_sections {
	gwatomic change_in_progress;	
	int	section_state;		
	unsigned int start_block;	
	unsigned int next_block;	
	unsigned int last_block;	
	int	carry_in_blocks_finished; 
	int	carry_out_section;	
};

#define FFT_TYPE_HOME_GROWN		0
#define FFT_TYPE_RADIX_4		1
#define FFT_TYPE_RADIX_4_DELAYED	2
#define FFT_TYPE_RADIX_4_DWPN		3	

struct gwhandle_struct {
	
	float	safety_margin;		
	float	polymult_safety_margin;	
	unsigned long minimum_fftlen;	
	int	maxmulbyconst;		
					
	unsigned int num_threads;	
	char	larger_fftlen_count;	
	char	force_general_mod;	
	char	use_irrational_general_mod; 
					
	char	use_large_pages;	
	char	use_benchmarks;		
	char	will_hyperthread;	
	char	will_error_check;	
	char	information_only;	
	char	use_spin_wait;		
					
	unsigned char scramble_arrays;	
					
	int	bench_num_cores;	
	int	bench_num_workers;	
	int	radix_bigwords;		
					
	int	required_pass2_size;	
					
	double	k;			
	unsigned long b;		
	unsigned long n;		
	signed long c;			
	unsigned long FFTLEN;		
	unsigned long PASS1_SIZE;	
	unsigned long PASS2_SIZE;	
	int	cpu_flags;		
	char	ZERO_PADDED_FFT;	
	char	NEGACYCLIC_FFT;		
	char	RATIONAL_FFT;		
	char	GENERAL_MOD;		
	char	GENERAL_MMGW_MOD;	
	char	NO_PREFETCH_FFT;	
	char	IN_PLACE_FFT;		
	char	ERROR_CHECKING;		
	char	GLOBAL_MULBYCONST;	 
	char	GLOBAL_POSTFFT;		 
	char	POSTFFT;		
	int	FFT_TYPE;		
	int	ARCH;			
	void	(*GWPROCPTRS[15])(void*); 
	giant	GW_MODULUS;		
	gwnum	GW_MODULUS_FFT;		
	gwnum	GW_RECIP_FFT;		
	unsigned long GW_ZEROWORDSLOW;	
	unsigned long GW_GEN_MOD_MAX;	
	unsigned long GW_GEN_MOD_MAX_OFFSET; 
	unsigned long saved_copyz_n;	
	gwnum	N_Q;			
	gwnum	Np_R;			
	gwnum	R2_4;			
	unsigned long NUM_B_PER_SMALL_WORD; 
	double	avg_num_b_per_word;	
	double	bit_length;		
	double	fft_max_bits_per_word;	
	long	FOURKBGAPSIZE;		
	long	PASS2GAPSIZE;		
	unsigned long PASS1_CACHE_LINES; 
	unsigned long mem_needed;	
	unsigned long SCRATCH_SIZE;	
	float	EXTRA_BITS;		
					
	gwnum	GW_RANDOM;		
	gwnum	GW_RANDOM_SQUARED;	
	gwnum	GW_RANDOM_FFT;		
	gwnum	GW_FFT1;		
	gwnum	GW_ADDIN;		
	gwnum	GW_POSTADDIN;		
	long	emulate_addin_value;	
	long	emulate_postaddin_value;
	double	asm_addin_value;	
	double	asm_postaddin_value;	
	char	FFT1_state;		
	char	FFT1_user_allocated;	
	char	polymult;		
	char	paranoid_mul_careful;	
	char	GWSTRING_REP[60];	
	int	GWERROR;		
	int	mulbyconst;		
	uint64_t fft_count;		
	uint64_t read_count;		
	uint64_t write_count;		
	const struct gwasm_jmptab *jmptab; 
	void	*asm_data;		
	void	*dd_data;		
	ghandle	gdata;			
	double	*gwnum_memory;		
	unsigned long datasize;		
	unsigned long GW_ALIGNMENT;	
	unsigned long GW_ALIGNMENT_MOD; 
	gwnum	*gwnum_alloc;		
	unsigned int gwnum_alloc_count; 
	unsigned int gwnum_alloc_array_size; 
	gwnum	*gwnum_free;		
	unsigned int gwnum_free_count;	
	unsigned int gwnum_max_free_count; 
	gwarray	array_list;		
	size_t	GW_BIGBUF_SIZE;		
	char	*GW_BIGBUF;		
	void	*large_pages_ptr;	
	void	*large_pages_gwnum;	
	void	(*thread_callback)(int, int, void *); 
					
	void	*thread_callback_data;	
	gwmutex alloc_lock;		
	gwmutex	thread_lock;		
	gwevent	work_to_do;		
	gwatomic alt_work_to_do;	
	gwevent	all_helpers_done;	
	gwatomic num_active_helpers;	
	short volatile helpers_must_exit; 
	short volatile all_work_assigned; 
	gwevent can_carry_into;		
	gwatomic can_carry_into_counter;
	int	pass1_state;		
					
	void	*pass1_var_data;	
	void	*adjusted_pass2_premults; 
					
	unsigned long biglit_data_offset; 
	unsigned long pass1_var_data_size; 
	unsigned long pass2_premult_block_size; 
	gwatomic next_block;		
	unsigned long num_pass1_blocks; 
	unsigned long num_pass2_blocks; 
	unsigned long num_postfft_blocks; 
	gwthread *thread_ids;		
	void	**thread_allocs;	
	struct pass1_carry_sections *pass1_carry_sections; 
	int	pass1_carry_sections_unallocated; 
	void	*multithread_op_data;	
	uint32_t ASM_TIMERS[32];	
	int	bench_pick_nth_fft;	
					
	int	qa_pick_nth_fft;	
					
	int	qa_picked_nth_fft;	
	int	careful_count;		
	double	ZPAD_COPY7_ADJUST[7];	
	double	ZPAD_0_6_ADJUST[7];	
	unsigned long wpn_count;	
	gwatomic clone_count;		
	gwhandle *clone_of;		
	gwhandle *to_radix_gwdata;	
	gwhandle *from_radix_gwdata;	
	gwhandle *cyclic_gwdata;	
	gwhandle *negacyclic_gwdata;	
	gwhandle *active_child_gwdata;	
	gwhandle *parent_gwdata;	
};

#define GW_SMALL_HEADER_SIZE	32	
#define GW_ZPAD_HEADER_SIZE	96	
#define GW_LARGE_HEADER_SIZE	192	
#define GW_HEADER_SIZE(h)	(!(h)->ZERO_PADDED_FFT ? GW_SMALL_HEADER_SIZE : !(h)->polymult ? GW_ZPAD_HEADER_SIZE : GW_LARGE_HEADER_SIZE)

typedef struct {
	int	flags;		
	gwarray *prev;		
	gwarray next;		
} gwarray_header;

#define MAX_PRIME	79300000L	
#define MAX_PRIME_SSE2	595800000L	
#define MAX_PRIME_AVX	920800000L	
#define MAX_PRIME_FMA3	922668300L	
#define MAX_PRIME_AVX512 1169000000L	
#define MAX_FFTLEN	4194304L	
#define MAX_FFTLEN_SSE2	33554432L	
#define MAX_FFTLEN_AVX	52428800L	
#define MAX_FFTLEN_FMA3	52428800L	
#define MAX_FFTLEN_AVX512 67108864L	

unsigned long gwmap_to_fftlen (double, unsigned long, unsigned long, signed long);
unsigned long gwmap_with_cpu_flags_to_fftlen (int, double, unsigned long, unsigned long, signed long);
double gwmap_to_timing (double, unsigned long, unsigned long, signed long);
unsigned long gwmap_to_memused (double, unsigned long, unsigned long, signed long);
unsigned long gwmap_fftlen_to_max_exponent (unsigned long fftlen);
unsigned long gwmap_with_cpu_flags_fftlen_to_max_exponent (int, unsigned long fftlen);
unsigned long gwmap_to_estimated_size (double, unsigned long, unsigned long, signed long);

void gw_as_string(char *buf, double k, unsigned long b, unsigned long n, signed long c);

int gwinfo (gwhandle *, double, unsigned long, unsigned long, signed long);
double virtual_bits_per_word (gwhandle *);
unsigned long addr_offset (gwhandle *, unsigned long);
double *addr (gwhandle *, gwnum, unsigned long);
int get_fft_value (gwhandle *, gwnum, unsigned long, long *);
void set_fft_value (gwhandle *, gwnum, unsigned long, long);
int is_big_word (gwhandle *, unsigned long);
void bitaddr (gwhandle *, unsigned long, unsigned long *, unsigned long *);
void specialmodg (gwhandle *, giant);
#define gw_set_max_allocs(h,n)	if ((h)->gwnum_alloc==NULL) (h)->gwnum_alloc_array_size=n
void init_FFT1 (gwhandle *);

typedef struct gwiter_struct {
	gwhandle *gwdata;		
	gwnum	g;			
	uint32_t index;			
	intptr_t addr_offset;		
	bool	big_word;		
	uint32_t switcher;		
	uint32_t ao_values[9];		
	uint64_t cached_data[12];	
} gwiter;
void gwiter_init_zero (gwhandle *h, gwiter *iter, gwnum g);					
void gwiter_init_write_only (gwhandle *h, gwiter *iter, gwnum g);				
void gwiter_init (gwhandle *h, gwiter *iter, gwnum g, uint32_t);				
void gwiter_next (gwiter *iter);								
#define gwiter_index(iter)		((iter)->index)						
#define gwiter_addr_offset(iter)	((iter)->addr_offset)					
#define gwiter_addr(iter)		((double *)((char *)((iter)->g) + (iter)->addr_offset))	
#define gwiter_is_big_word(iter)	((iter)->big_word)					
int gwiter_get_fft_value (gwiter *iter, int32_t *);						
void gwiter_set_fft_value (gwiter *iter, int32_t);						

#define gwfree_temporarily(h,g)
#define gwrealloc_temporarily(h,g)

void *gwgiantalloc (void *);
void gwgiantfree (void *, void *);
void gwgiantdealloc (void *);

extern void (*OutputBothRoutine)(int, const char *);

int gwtimeit (void *);
#define get_asm_timers(g) ((uint32_t *) &(g)->ASM_TIMERS)

#ifdef __cplusplus
}
#endif

#endif
