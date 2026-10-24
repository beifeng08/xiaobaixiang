/*----------------------------------------------------------------------
| polymult.h
|
| This file contains the headers and definitions that are used in the
| polynomial multiplication built upon the gwnum library.
| 
|  Copyright 2021-2023 Mersenne Research, Inc.  All rights reserved.
+---------------------------------------------------------------------*/

#ifndef _POLYMULT_H
#define _POLYMULT_H

#include "gwthread.h"		

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pmhandle_struct pmhandle;

float polymult_safety_margin (uint64_t invec1_size, uint64_t invec2_size);

uint64_t polymult_fft_size (uint64_t n);

uint64_t polymult_mem_required (
	pmhandle *pmdata,		
	uint64_t invec1_size,		
	uint64_t invec2_size,		
	int	options);		

void polymult_init (
	pmhandle *pmdata,		
	gwhandle *gwdata);		

#define polymult_set_max_num_threads(h,n)	(h)->max_num_threads = (h)->num_threads = (n)
#define polymult_set_num_threads(h,n)		(h)->num_threads = (n)

void polymult_default_tuning (
	pmhandle *pmdata,		
	uint32_t L2_CACHE_SIZE,		
	uint32_t L3_CACHE_SIZE);	

void polymult_done (
	pmhandle *pmdata);		

void polymult (
	pmhandle *pmdata,		
	gwnum	*invec1,		
	uint64_t invec1_size,		
	gwnum	*invec2,		
	uint64_t invec2_size,		
	gwnum	*outvec,		
	uint64_t outvec_size,		
					
	int	options);

void polymult_fma (
	pmhandle *pmdata,		
	gwnum	*invec1,		
	uint64_t invec1_size,		
	gwnum	*invec2,		
	uint64_t invec2_size,		
	gwnum	*outvec,		
	uint64_t outvec_size,		
					
	gwnum	*fmavec,		
	int	options);

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
	int	options);

#define POLYMULT_INVEC1_MONIC	0x1	
#define POLYMULT_INVEC2_MONIC	0x2	
#define POLYMULT_INVEC1_RLP	0x4	
#define POLYMULT_INVEC2_RLP	0x8	
#define POLYMULT_INVEC1_NEGATE	0x10	
#define POLYMULT_INVEC2_NEGATE	0x20	
#define POLYMULT_INVEC1_MONIC_RLP	(POLYMULT_INVEC1_MONIC | POLYMULT_INVEC1_RLP)		
#define POLYMULT_INVEC2_MONIC_RLP	(POLYMULT_INVEC2_MONIC | POLYMULT_INVEC2_RLP)		
#define POLYMULT_INVEC1_MONIC_NEGATE	(POLYMULT_INVEC1_MONIC | POLYMULT_INVEC1_NEGATE)	
#define POLYMULT_INVEC2_MONIC_NEGATE	(POLYMULT_INVEC2_MONIC | POLYMULT_INVEC2_NEGATE)	
#define POLYMULT_CIRCULAR	0x100	
					
#define POLYMULT_MULHI		0x200	
					
#define POLYMULT_MULLO		0x400	
					
#define POLYMULT_MULMID		0x800	
#define POLYMULT_NO_UNFFT	0x1000	
					
#define POLYMULT_STARTNEXTFFT	0x2000	
#define	POLYMULT_NEXTFFT	0x4000	
					
#define POLYMULT_FMADD		0x8000	
#define POLYMULT_FMSUB		0x10000	
#define POLYMULT_FNMADD		0x20000	
#define POLYMULT_UNFFT_TOP	0x40000	
					
#define POLYMULT_SAVE_PLAN	0x80000	
#define POLYMULT_USE_PLAN	0x100000 

#define	POLYMULT_PRE_FFT	0x40	
#define	POLYMULT_PRE_COMPRESS	0x80	

#define is_preprocessed_poly(p)		(((preprocessed_poly_header *)((char *)(p)-sizeof(gwarray_header)))->self_ptr == (p))
#define is_preffted_poly(p)		(((preprocessed_poly_header *)((char *)(p)-sizeof(gwarray_header)))->options & POLYMULT_PRE_FFT)
#define is_compressed_poly(p)		(((preprocessed_poly_header *)((char *)(p)-sizeof(gwarray_header)))->options & POLYMULT_PRE_COMPRESS)
#define preprocessed_fft_size(p)	(intptr_t) (((preprocessed_poly_header *)((char *)(p)-sizeof(gwarray_header)))->line_size)
#define preprocessed_num_elements(p)	(((preprocessed_poly_header *)((char *)(p)-sizeof(gwarray_header)))->num_compressed_blks)
#define preprocessed_element_size(p)	(intptr_t) (((preprocessed_poly_header *)((char *)(p)-sizeof(gwarray_header)))->element_size)
#define preprocessed_poly_size(p)	(preprocessed_num_elements(p) * preprocessed_element_size(p))
#define preprocessed_monics_included(p)	(((preprocessed_poly_header *)((char *)(p)-sizeof(gwarray_header)))->monic_ones_included)
#define preprocessed_top_unnorms(p)	(((preprocessed_poly_header *)((char *)(p)-sizeof(gwarray_header)))->top_unnorms)

gwarray polymult_preprocess (		
	pmhandle *pmdata,		
	gwnum	*invec1,		
	uint64_t invec1_size,		
	uint64_t invec2_size,		
	uint64_t outvec_size,		
	int	options);		

typedef struct pmargument_struct {	
	gwnum	*invec2;	
	uint64_t invec2_size;	
	gwnum	*outvec;	
	uint64_t outvec_size;	
	gwnum	*fmavec;	
	uint64_t circular_size;	
	uint64_t first_mulmid;	
	int	options;	
} polymult_arg;

void polymult_several (		
	pmhandle *pmdata,	
	gwnum	*invec1,	
	uint64_t invec1_size,	
	polymult_arg *other_polys, 
	int	num_other_polys,
	int	options);	

#define polymult_must_unfft(h,c) (FFT_state(c)==FFTed_FOR_FMA || (FFT_state(c)==FULLY_FFTed && EB_GWMUL_SAVINGS + numadds_to_eb(unnorms(c))*2.0f > (h)->EXTRA_BITS))

void polymult_launch_helpers (
	pmhandle *pmdata);		

void poly_helper_example (int helper_num, gwhandle *gwdata, void *info);

void poly_copy (pmhandle *pmdata, gwnum *invec, gwnum *outvec, uint64_t poly_size);

void poly_fft_coefficients (pmhandle *pmdata, gwnum *vec, uint64_t poly_size);
void poly_unfft_coefficients (pmhandle *pmdata, gwnum *vec, uint64_t poly_size);
void poly_unfft_fft_coefficients (pmhandle *pmdata, gwnum *vec, uint64_t poly_size);

struct pmhandle_struct {
	gwhandle *gwdata;		
	int	max_num_threads;	
	int	num_threads;		
	int	num_lines;		
	gwevent work_to_do;		
	gwatomic alt_work_to_do;	
	gwevent	all_helpers_done;	
	gwatomic num_active_helpers;	
	gwmutex	poly_mutex;		
	gwatomic next_thread_num;	
	bool volatile all_work_assigned; 
	gwthread *thread_ids;		
	uint64_t twiddles_initialized;	
	bool	twiddles_are_from_cache; 
	double	*twiddles1;		
	double	*twiddles2;		
	
	int	KARAT_BREAK;		
	int	FFT_BREAK;		
	
	uint32_t two_pass_start;	
	uint32_t max_pass2_size;	
	uint64_t mt_ffts_start;		
	uint64_t mt_ffts_end;		
	uint64_t streamed_stores_start;	
	uint64_t strided_writes_end;	
	
	bool	cached_twiddles_enabled;
	bool	twiddle_cache_additions_disabled; 
	struct {
		uint64_t size;		
		double	*twiddles1;	
		double	*twiddles2;	
	} cached_twiddles[40];
	int	cached_twiddles_count;	
	
	gwnum	*invec1;		
	uint64_t invec1_size;		
	polymult_arg *other_polys;	
	int	num_other_polys;	
	int	options;
	bool	mt_polymult_line;	
	void	*plan;			
	void	(*internal_callback)(int, pmhandle *, void *); 
	void	*internal_callback_data; 
	
	void	(*helper_callback)(int, gwhandle *, void *); 
	void	*helper_callback_data;	
	int volatile helper_opcode;	
	gwatomic helper_counter;	
	int	saved_gwdata_num_threads; 
	gwhandle *stats_gwdata;		
};

typedef struct {
	gwarray_header linkage;		
	gwnum	*self_ptr;		
	uint64_t element_size;		
	uint64_t line_size;		
	int	num_compressed_blks;	
	int	options;		
	float	top_unnorms;		
	bool	monic_ones_included;	
} preprocessed_poly_header;

#ifdef __cplusplus
}
#endif

#endif
