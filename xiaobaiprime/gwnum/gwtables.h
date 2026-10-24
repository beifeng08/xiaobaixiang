/*----------------------------------------------------------------------
| This file contains various utility routines that may be used by gwnum setup.
|
|  Copyright 2011-2023 Mersenne Research, Inc.  All rights reserved.
+---------------------------------------------------------------------*/

#ifndef _GWTABLES_H
#define _GWTABLES_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef X86_64
#define NEW_STACK_SIZE	0
#else
#define NEW_STACK_SIZE	(8192+256)
#endif

struct gwasm_data {
	void	*DESTARG;		
	intptr_t DIST_TO_FFTSRCARG;	
	intptr_t DIST_TO_MULSRCARG;	
	void	*NORMRTN;		
	void	*SAVED_RSP;		
	void	*SRCARG;		
	void	*SRC2ARG;		
	void	*DEST2ARG;		
	void	*data_addr;		
	void	*data_prefetch;		
	void	*premult_addr;		
	void	*premult_prefetch;	
	void	*pass1_wake_up_threads;	
	void	*pass1_pre_carries;	
	void	*pass1_post_carries;	
	void	*pass1_get_next_block;	

	double	DBLARG;			
	uint32_t NUMARG;		
	uint32_t FFTLEN;		
	double	MAXERR;			
	char	NEGACYCLIC_FFT;		
	char	B_IS_2;			
	char	RATIONAL_FFT;		
	char	ZERO_PADDED_FFT;	
	char	ZPAD_TYPE;		
	char	ffttype;		
	char	TOP_CARRY_NEEDS_ADJUSTING; 
	char	SPREAD_CARRY_OVER_EXTRA_WORDS; 
					
	char	zero_fft;		
	char	const_fft;		
	char	add_sub_smallmul_op;	
	char	mul4_opcode;		
	char	aux_initialized;	
	char	UNUSED_CHARS[3];
	uint32_t ADDIN_ROW;		
	uint32_t ADDIN_OFFSET;
	double	ADDIN_VALUE;		
	double	POSTADDIN_VALUE;	

	int32_t	thread_num;		
	uint32_t this_block;		
	uint32_t next_block;		
	uint32_t last_pass1_block;	
	int32_t	normcount1;
	int32_t	count1;			
	int32_t	count2;
	int32_t	count3;
	int32_t	count4;
	int32_t	count5;
	int32_t	addcount1;		
	int32_t	normval1;		
	int32_t	normval4;
	int32_t	cache_line_multiplier;	
	uint32_t BIGLIT_INCR2;				
	uint32_t BIGLIT_INCR4;				

	void	*carries;		
	void	*norm_grp_mults;	
	void	*norm_col_mults;		
	void	*norm_biglit_array;		
	void	*norm_ptr1;
	union {
		void	*norm_ptr2;	
		intptr_t normblkdst4;	
	};
	intptr_t normblkdst;		
	intptr_t normblkdst8;		
	intptr_t normval2;		
	intptr_t normval3;
	void	*scratch_area;		
	void	*plus1_premults;		
	intptr_t fourKBgapsize;		
	intptr_t pass2gapsize;		
	intptr_t pass1blkdst;		
	intptr_t pass2blkdst;		

	void	*compressed_biglits;	
	void	*compressed_fudges;	
	gwhandle *gwdata;		
	void	*pass2_wake_up_threads;	
	void	*pass2_get_next_block;	
	void	(*thread_work_routine)(void*); 
	void	*xsincos_complex;	
	void	*sincos1;
	void	*sincos2;
	void	*sincos3;
	void	*sincos4;
	void	*sincos5;
	gwthread UNUSED_hyperthread_id;	
	gwevent UNUSED_hyperthread_work_to_do;	
	void	*SRC3ARG;		
	uint32_t *ASM_TIMERS;		

	uint32_t COPYZERO[8];		
	double	K;			
	double	INVERSE_K;		
	double	TWO_TO_17;		
	double	CARRY_ADJUST1;		

	double	CARRY_ADJUST2;		
	double	CARRY_ADJUST3;		
	double	CARRY_ADJUST4;		
	double	CARRY_ADJUST5;		
	double	CARRY_ADJUST6;		
	double	CARRY_ADJUST7;
	double	CARRY_ADJUST1_HI;
	double	CARRY_ADJUST1_LO;

	uint32_t HIGH_WORD1_OFFSET;	
	uint32_t HIGH_WORD2_OFFSET;	
	uint32_t HIGH_WORD3_OFFSET;	
	uint32_t HIGH_SCRATCH1_OFFSET;	
	uint32_t HIGH_SCRATCH2_OFFSET;	
	uint32_t HIGH_SCRATCH3_OFFSET;	
	double	ZPAD_INVERSE_K6;	
	double	ZPAD_K6_HI;
	double	ZPAD_K6_MID;
	double	ZPAD_K6_LO;
	double	ZPAD_SHIFT6;

	double	ZPAD_INVERSE_K5;
	double	ZPAD_K5_HI;
	double	ZPAD_K5_MID;
	double	ZPAD_K5_LO;
	double	ZPAD_SHIFT5;
	double	ZPAD_INVERSE_K4;
	double	ZPAD_K4_HI;
	double	ZPAD_K4_MID;

	double	ZPAD_K4_LO;
	double	ZPAD_SHIFT4;
	double	ZPAD_INVERSE_K3;
	double	ZPAD_K3_HI;
	double	ZPAD_K3_MID;
	double	ZPAD_K3_LO;
	double	ZPAD_SHIFT3;
	double	ZPAD_INVERSE_K2;

	double	ZPAD_K2_HI;
	double	ZPAD_K2_MID;
	double	ZPAD_K2_LO;
	double	ZPAD_SHIFT2;
	double	ZPAD_INVERSE_K1;
	double	ZPAD_K1_HI;
	double	ZPAD_K1_LO;
	double	ZPAD_SHIFT1;

	double	ZPAD0_6[7];		
	double	ZPAD_LSW_ADJUST;	

	double	ttmp_ff_inv;		
	double	UNUSED_DOUBLES[7];

	union {
	    struct zmm_data {
		intptr_t ZMM_SRC_INCR;		
		void	*ZMM_PASS2_ROUTINE;	
		void	*ZMM_CARRIES_ROUTINE;	
		void	*ZMM_OP_CARRIES_ROUTINE; 

		double	ZMM_MINUS_C;		
		double	ZMM_MULCONST;
		double	ZMM_MINUS_C_TIMES_MULCONST;
		char	ZMM_FIRST_BIGLIT_VALUES[1];  
		char	ZMM_UNUSED_CHARS[7];

		double	ZMM_RNDVAL;		
		int32_t	ZMM_ABSVAL[2];		
		double	ZMM_LARGE_BASE;		
		double	ZMM_LARGE_BASE_INVERSE;	
		double	ZMM_RNDVAL_TIMES_LARGE_BASE; 
		double	ZMM_RNDVAL_OVER_LARGE_BASE; 
		double	ZMM_SMALL_BASE;		
		double	ZMM_SMALL_BASE_INVERSE;	
		double	ZMM_RNDVAL_TIMES_SMALL_BASE; 
		double	ZMM_RNDVAL_OVER_SMALL_BASE; 

		double	ZMM_K_LO;		
		double	ZMM_K_HI_OVER_SMALL_BASE; 
		double	ZMM_K_HI_OVER_LARGE_BASE; 
		double	ZMM_K_TIMES_MULCONST_LO; 
		double	ZMM_K_TIMES_MULCONST_HI_OVER_SMALL_BASE; 
		double	ZMM_K_TIMES_MULCONST_HI_OVER_LARGE_BASE; 

		double	ZMM_ONE;		
		double	ZMM_P383;		
		double	ZMM_SQRTHALF;		
		double	ZMM_P383_1;		
		
		double	ZMM_TWO;		
		double	ZMM_HALF;		
		double	ZMM_P924_P383;		

		double	ZMM_P981_P195;		
		double	ZMM_P195;		
		double	ZMM_P831_P556;		
		double	ZMM_P556_P195;		

		double	ZMM_SQRT2;		

		double	ZMM_P866;		
		double	ZMM_P259_P707;		
		double	ZMM_P966_P707;		

		double	ZMM_P309;		
		double	ZMM_P809;		
		double	ZMM_P951;		
		double	ZMM_P588_P951;		

		double	UNUSED_P623;
		double	UNUSED_P901;
		double	UNUSED_P975;
		double	UNUSED_P223;
		double	ZMM_P434_P975;		
		double	ZMM_P782_P975;		
		double	ZMM_P901_P975;		
		double	ZMM_P623_P975;		
		double	ZMM_P223_P975;		
		double	ZMM_P1_P975;		

		double	ZMM_B;			
		double	ZMM_ONE_OVER_B;

		int64_t	ZMM_PERMUTE1;		
		int64_t	ZMM_PERMUTE2;		

		double	UNUSED_DOUBLES[7];	

		double	ZMM_TMPS[512];		
	    } zmm;

	    struct ymm_data {
		intptr_t YMM_SRC_INCR1;		
		intptr_t YMM_SRC_INCR2;
		intptr_t YMM_SRC_INCR3;
		intptr_t YMM_SRC_INCR4;
		intptr_t YMM_SRC_INCR5;
		intptr_t YMM_SRC_INCR6;
		intptr_t YMM_SRC_INCR7;
		intptr_t YMM_NORM_INCR1;	
		intptr_t YMM_NORM_INCR2;
		intptr_t YMM_NORM_INCR3;
		intptr_t YMM_NORM_INCR4;
		intptr_t YMM_NORM_INCR5;
		intptr_t YMM_NORM_INCR6;
		intptr_t YMM_NORM_INCR7;
		void	*YMM_CARRIES_ROUTINE;	
		void	*YMM_PASS2_ROUTINE;	

		double	YMM_MINUS_C[4];	
		double	YMM_HALF[4];	

		double	YMM_SQRTHALF[4];
		double	YMM_UNUSED_DOUBLES1[4];

		double	YMM_MAXERR[4];	
		int32_t	YMM_ABSVAL[8];	

		double	YMM_BIGVAL[4];	
		double	YMM_BIGBIGVAL[4]; 

		double	YMM_NORM012_FF[4]; 
		double	YMM_MULCONST[4];

		double	YMM_K_LO[4];	
		double	YMM_K_HI[4];	

		double	YMM_K_TIMES_MULCONST_LO[4]; 
		double	YMM_K_TIMES_MULCONST_HI[4]; 

		double	YMM_MINUS_C_TIMES_MULCONST[4];
		char	YMM_FIRST_BIGLIT_VALUES[8];  
		double	UNUSED_YMM_DOUBLES[3];

		double	UNUSED_YMM_DOUBLES2[4];
		double	YMM_P924_P383[4]; 

		double	YMM_P383[4];	
		double	YMM_P924[4];	

		double	YMM_P866[4];	
		double	YMM_P588[4];	

		double	YMM_P309[4];
		double	YMM_P809[4];

		double	YMM_P951[4];
		double	YMM_P618[4];	

		double	YMM_ONE[4];	
		double	YMM_TWO[4];	

		double	YMM_P975[4];	
		double	YMM_P782[4];

		double	YMM_P623[4];
		double	YMM_P901[4];

		double	YMM_P434[4];
		double	YMM_P223[4];

		double	YMM_P975_P434[4];	
		double	YMM_P782_P434[4];

		double	YMM_P259[4];		
		double	YMM_P966[4];

		double	YMM_P259_P707[4];	
		double	YMM_P966_P707[4];

		double	YMM_LIMIT_BIGMAX[192];	
		double	YMM_LIMIT_INVERSE[192];
		double	YMM_TMPS[256];		
	    } ymm;

	    struct xmm_data {
		void	*sincos6;		
		void	*sincos7;		
		void	*sincos8;		
		void	*sincos9;		
		void	*sincos10;		
		void	*sincos11;		
		intptr_t ZPAD_WORD5_OFFSET;	
		intptr_t ZPAD_WORD5_RBP_OFFSET;	
		void	*pass2_premults;	
		void	*UNUSED_XMM_PTRS[7];

		double	XMM_TWO[2];		
		double	XMM_HALF[2];		
		double	XMM_SQRTHALF[2];	
		double	XMM_SUMOUT[2];		

		double	XMM_MAXERR[2];		
		int32_t	XMM_ABSVAL[4];		
		double	XMM_BIGVAL[2];		
		double	XMM_BIGBIGVAL[2];	

		double	XMM_BIGVAL_NEG[2];
		double	XMM_MINUS_C[2];		
		double	XMM_NORM012_FF[2];	
		double	XMM_MULCONST[2];

		double	XMM_K_LO[2];		
		double	XMM_K_HI[2];		
		double	XMM_K_TIMES_MULCONST_LO[2]; 
		double	XMM_K_TIMES_MULCONST_HI[2]; 

		double	XMM_MINUS_C_TIMES_MULCONST[2];
		double	XMM_P309[2];		
		double	XMM_P809[2];
		double	XMM_P951[2];

		double	XMM_P588[2];
		double	XMM_P618[2];		
		double	XMM_M809[2];
		double	XMM_M262[2];

		double	XMM_M382[2];
		double	XMM_M162[2];
		double	XMM_P866[2];		
		double	XMM_P924[2];		

		double	XMM_P383[2];
		double	XMM_M358[2];
		double	XMM_P404[2];
		double	XMM_P445[2];

		double	XMM_P180[2];
		double	XMM_P975[2];		
		double	XMM_P623[2];
		double	XMM_P901[2];

		double	XMM_P782[2];
		double	XMM_P434[2];
		double	XMM_P223[2];
		double	UNUSED_XMM_DOUBLES[2];

		double	XMM_TMP1_8[16];		
		double	XMM_LIMIT_BIGMAX[96];	
		double	XMM_LIMIT_INVERSE[96];
		double	XMM_LIMIT_BIGMAX_NEG[96];
		double	XMM_TTP_FUDGE[32];
		double	XMM_TTMP_FUDGE[32];
		double	XMM_COL_MULTS[1024];
	    } xmm;

	    struct x87_data {
		void	*sincos6;		
		void	*sincos7;		
		void	*sincos8;		
		void	*sincos9;		
		void	*sincos10;		
		void	*zpad_addr;		
		void	*pass2_premults;	
		char	POSTFFT;		
		char	UNUSED_X87_CHARS[3];
		void	*UNUSED_X87_PTRS[8];

		double	SQRTHALF;		
		double	SUMOUT;			
		float	BIGVAL;			
		float	BIGBIGVAL;
		double	MINUS_C;		
		double	NORM012_FF;		
		double	MULCONST;
		double	K_TIMES_MULCONST_LO;	
		double	K_TIMES_MULCONST_HI;	

		double	MINUS_C_TIMES_MULCONST;
		double	K_TIMES_MULCONST_HI_1;	
		double	K_TIMES_MULCONST_HI_2;	
		double	K_HI;			
		double	K_LO;			
		double	K_HI_1;			
		double	K_HI_2;			
		double	ALT_K_HI;		

		double	ALT_K_LO;		
		double	P309;
		double	M809;
		double	M262;
		double	M382;
		double	P951;
		double	P588;
		double	M162;

		double	P618;
		double	P623;
		double	M358;
		double	P404;
		double	P975;
		double	P445;
		double	P180;
		double	M223;

		double	M901;
		double	M691;
		double	P866;
		double	P433;
		double	P577;
		double	UNUSED_X87_DOUBLES[3];

		float	P25;			
		float	P75;			
		float	P3;			
		float	HALF;			
		double	TMP1_6[6];		

		double	LIMIT_BIGMAX[32];	
		double	LIMIT_INVERSE[32];
		double	LIMIT_BIGMAX_NEG[32];
		double	TTP_FUDGE[32];
		double	TTMP_FUDGE[32];
	    } x87;
	} u;
};

unsigned long pow_two_above_or_equal (unsigned long n);

double *zr4dwpn_build_pass1_table (gwhandle *gwdata, double *table);
double *zr4dwpn_build_fixed_pass1_table (gwhandle *gwdata, double *table);
double *zr4dwpn_build_biglit_table (gwhandle *gwdata, double *table);
double *zr4dwpn_build_fudge_table (gwhandle *gwdata, double *table);
double *zr4dwpn_build_norm_table (gwhandle *gwdata, double *table);
double *zr4_build_pass2_complex_table (gwhandle *gwdata, double *table);
double *zr4_build_pass2_real_table (gwhandle *gwdata, double *table);

double *yr4_build_onepass_sincos_table (gwhandle *gwdata, double *table);
double *yr4_build_onepass_biglit_table (gwhandle *gwdata, double *table);
double *yr4_build_onepass_norm_table (gwhandle *gwdata, double *table);
double *yr4dwpn_build_pass1_table (gwhandle *gwdata, double *table);
double *yr4dwpn_build_norm_table (gwhandle *gwdata, double *table);
double *yr4dwpn_build_biglit_table (gwhandle *gwdata, double *table);
double *yr4dwpn_build_fixed_pass1_table (gwhandle *gwdata, double *table);
double *yr4_build_pass2_complex_table (gwhandle *gwdata, double *table);
double *yr4_build_pass2_real_table (gwhandle *gwdata, double *table);

double *r4_build_pass1_table (gwhandle *gwdata, double *table);
double *r4_build_pass2_complex_table (gwhandle *gwdata, double *table);
double *r4_build_pass2_real_table (gwhandle *gwdata, double *table);
double *r4delay_build_fixed_premult_table (gwhandle *gwdata, double *table);
double *r4delay_build_pass1_table (gwhandle *gwdata, double *table);
double *r4delay_build_fixed_pass1_table (gwhandle *gwdata, double *table);
double *r4dwpn_build_pass1_table (gwhandle *gwdata, double *table);
double *r4_build_norm_table (gwhandle *gwdata, double *table, int col);
double *r4dwpn_build_norm_table (gwhandle *gwdata, double *table);
double *r4_build_biglit_table (gwhandle *gwdata, double *table);
double *r4dwpn_build_biglit_table (gwhandle *gwdata, double *table);
double *hg_build_sin_cos_table (double	*table, unsigned long N, int hermetian_skip, int type);
double *hg_build_premult_table (gwhandle *gwdata, double *table);
double *hg_build_plus1_table (gwhandle *gwdata, double *table);
double *hg_build_norm_table (gwhandle *gwdata, double *table, int col);
double *hg_build_biglit_table (gwhandle *gwdata, double *table);

double *x87_build_sin_cos_table (double	*table, unsigned long N, int hermetian_skip);
double *x87_build_premult_table (gwhandle *gwdata, double *table);
double *x87_build_plus1_table (gwhandle *gwdata, double *table);
double *x87_build_norm_table (gwhandle *gwdata, double *table, int col);
double *x87_build_biglit_table (gwhandle *gwdata, double *table);

#ifdef __cplusplus
}
#endif

#endif
