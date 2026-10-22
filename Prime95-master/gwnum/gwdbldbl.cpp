// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

#include "gwdbldbl.h"
#include <cstring>
#include <memory>

#define QD

#ifndef X86_64
#define x86
#endif

#ifdef QD
#include "dd.cc"
#endif

#ifdef KEITH_BRIGGS
#define DD_INLINE
#include "doubledouble.h"
#include "doubledouble.cc"
#include "math.cc"
#define	dd_real doubledouble
#define	_2pi TwoPi
#define	_log2 Log2
#endif

#define epsilon 5.5271478752604445602472651921923E-76

#define NUM_SLOPPY_MULTS	20
struct gwdbldbl_constants {
	uint32_t frac_bpw3;		
	uint32_t frac_bpw2;		
	uint32_t frac_bpw1;		
	uint32_t int_bpw;		
	dd_real	gw__b;
	dd_real	gw__logb;
	dd_real	gw__num_b_per_word;
	int	gw__c_is_one;
	dd_real gw__logb_abs_c_div_fftlen;
	dd_real gw__fftlen_inverse;
	dd_real gw__over_fftlen;
	double	gwdbl__b;
	double	gwdbl__b_inverse;
	double	gwdbl__logb;
	double	gwdbl__logb_abs_c_div_fftlen;
	double	fast_sloppy_multiplier[NUM_SLOPPY_MULTS];
	double	fast_inv_sloppy_multiplier[NUM_SLOPPY_MULTS];
};
#define dd_data		((struct gwdbldbl_constants *) dd_data_arg)

double gwfft_weight_over_b (void *dd_data_arg, unsigned long j);

double dderr (dd_real x) {
	double	y = double (x);
	double	err = double (fabs (x - y));
	double	absx = double (fabs (x));
	double	pcterr = err / absx * pow (2, 53.0);
	double	rounderr = err * pow (2.0, 53.0 - ceil (log(absx)/log(2.0)));
	return (y);
}

extern "C"
void gwasm_constants (
	double	*asm_values)
{
	dd_real arg, sine1, cosine1, sine2, cosine2, sine3, cosine3;
#define P951			asm_values[0]
#define P618			asm_values[1]
#define P309			asm_values[2]
#define M262			asm_values[3]
#define P588			asm_values[4]
#define M162			asm_values[5]
#define M809			asm_values[6]
#define M382			asm_values[7]
#define P866			asm_values[8]
#define P433			asm_values[9]
#define P577			asm_values[10]
#define P975			asm_values[11]
#define P434_P975		asm_values[12]
#define P180			asm_values[13]
#define P623			asm_values[14]
#define M358			asm_values[15]
#define P404			asm_values[16]
#define M223			asm_values[17]
#define M901			asm_values[18]
#define M691			asm_values[19]
#define P924			asm_values[20]
#define P383			asm_values[21]
#define P782			asm_values[22]
#define P434			asm_values[23]
#define P975_P434		asm_values[24]
#define P782_P434		asm_values[25]
#define P259			asm_values[26]
#define P966			asm_values[27]
#define P259_P707		asm_values[28]
#define P966_P707		asm_values[29]
#define P924_P383		asm_values[30]
#define P981_P195		asm_values[31]
#define P981			asm_values[32]
#define P195			asm_values[33]
#define P831_P556		asm_values[34]
#define P831			asm_values[35]
#define P556			asm_values[36]
#define P556_P195		asm_values[37]
#define P901_P975		asm_values[38]
#define P623_P975		asm_values[39]
#define P223_P975		asm_values[40]
#define P1_P975			asm_values[41]
#define P782_P975		asm_values[42]

	x86_FIX

	arg = dd_real::_2pi / 5.0;		
	sincos (arg, sine1, cosine1);

	arg = arg * 2.0;			
	sincos (arg, sine2, cosine2);

	P951 = double (sine1);
	P618 = double (sine2 / P951);		

	P309 = double (cosine1);
	M262 = double (cosine2 / P309);		

	P588 = double (sine2);
	M162 = double (-sine1 / P588);		

	M809 = double (cosine2);
	M382 = double (cosine1 / M809);		

	arg = dd_real::_2pi / 3.0;		
	sine1 = sin (arg);			

	P866 = double (sine1);
	P433 = double (sine1 * 0.5);		
	P577 = double (dd_real (0.5) / sine1);	

	arg = dd_real::_2pi / 7.0;		
	sincos (arg, sine1, cosine1);		

	arg = arg * 2.0;			
	sincos (arg, sine2, cosine2);		

	arg = arg * 1.5;			
	sincos (arg, sine3, cosine3);		

	P782 = double (sine1);
	P975 = double (sine2);
	P434 = double (sine3);
	P623 = double (cosine1);
	M223 = double (cosine2);
	M901 = double (cosine3);

	P434_P975 = double (sine3 / P975);	
	P180 = double (sine1 / P975 / P434_P975); 
	M358 = double (cosine2 / P623);		
	P404 = double (cosine3 / P623 / M358);	
	M691 = double (cosine1 / M901);		
	P782_P434 = double (sine1 / P434);	
	P975_P434 = double (sine2 / P434);	

	P782_P975 = double (sine1 / P975);	
	P901_P975 = double (-cosine3 / P975);
	P623_P975 = double (cosine1 / P975);
	P223_P975 = double (-cosine2 / P975);
	P1_P975 = 1.0 / P975;

	arg = dd_real::_2pi / 24.0;		
	sincos (arg, sine1, cosine1);		

	P259 = double (sine1);
	P966 = double (cosine1);

	P259_P707 = double (sine1 / sqrt (0.5));
	P966_P707 = double (cosine1 / sqrt (0.5));

	arg = dd_real::_2pi / 16.0;		
	sincos (arg, sine1, cosine1);		
	P924 = double (cosine1);
	P383 = double (sine1);
	P924_P383 = double (cosine1 / P383);

	arg = dd_real::_2pi / 32.0;		
	sincos (arg, sine1, cosine1);		
	P981 = double (cosine1);
	P195 = double (sine1);
	P981_P195 = double (cosine1 / P195);

	arg = dd_real::_2pi / 32.0;		
	arg = 3.0 * arg;			
	sincos (arg, sine1, cosine1);		
	P831 = double (cosine1);
	P556 = double (sine1);
	P831_P556 = double (cosine1 / P556);
	P556_P195 = double (sine1 / P195);

	END_x86_FIX
}

extern "C"
void gwsincos (
	unsigned long x,
	unsigned long N,
	double	*results)
{
	gwsincos1by (x, N, results, 1);
}

extern "C"
void gwsincos1by (
	unsigned long x,
	unsigned long N,
	double	*results,
	int	incr)
{
	gwsincos12345by (x, N, results, incr, 1);
}

extern "C"
void gwsincos12345by (
	unsigned long x,
	unsigned long N,
	double	*results,
	int	incr,
	int	amt)
{
	dd_real arg1, sine, cosine, sine2, cosine2, sine3, cosine3, sine4, cosine4, sine5, cosine5, sine6, cosine6, sine7, cosine7;
	dd_real	sine8, cosine8;

	x86_FIX
	arg1 = dd_real::_2pi * (double) x / (double) N;
	sincos (arg1, sine, cosine);
	results[0] = sine;
	results[0] += epsilon;		
	results[incr] = cosine / results[0];
	results += incr + incr;
	if (amt == 1) goto done;

	sine2 = sine * cosine * 2.0;
	cosine2 = sqr (cosine) - sqr (sine);
	results[0] = sine2;
	results[0] += epsilon;		
	results[incr] = cosine2 / results[0];
	results += incr + incr;
	if (amt == 2) goto done;

	sine3 = sine * cosine2 + sine2 * cosine;
	cosine3 = cosine * cosine2 - sine * sine2;
	results[0] = sine3;
	results[0] += epsilon;		
	results[incr] = cosine3 / results[0];
	results += incr + incr;
	if (amt == 3) goto done;

	sine4 = sine2 * cosine2 * 2.0;
	cosine4 = sqr (cosine2) - sqr (sine2);
	results[0] = sine4;
	results[0] += epsilon;		
	results[incr] = cosine4 / results[0];
	results += incr + incr;
	if (amt == 4) goto done;

	sine5 = sine * cosine4 + sine4 * cosine;
	cosine5 = cosine * cosine4 - sine * sine4;
	results[0] = sine5;
	results[0] += epsilon;		
	results[incr] = cosine5 / results[0];
	results += incr + incr;
	if (amt == 5) goto done;

	sine6 = sine2 * cosine4 + sine4 * cosine2;
	cosine6 = cosine2 * cosine4 - sine2 * sine4;
	results[0] = sine6;
	results[0] += epsilon;		
	results[incr] = cosine6 / results[0];
	results += incr + incr;
	if (amt == 6) goto done;

	sine7 = sine3 * cosine4 + sine4 * cosine3;
	cosine7 = cosine3 * cosine4 - sine3 * sine4;
	results[0] = sine7;
	results[0] += epsilon;		
	results[incr] = cosine7 / results[0];
	results += incr + incr;
	if (amt == 7) goto done;

	sine8 = sine4 * cosine4 + sine4 * cosine4;
	cosine8 = cosine4 * cosine4 - sine4 * sine4;
	results[0] = sine8;
	results[0] += epsilon;		
	results[incr] = cosine8 / results[0];
done:	;
	END_x86_FIX
}

extern "C"
void gwsincos13579by (
	unsigned long x,
	unsigned long N,
	double	*results,
	int	incr,
	int	amt)
{
	dd_real arg1, sine, cosine, sine2, cosine2, sine3, cosine3, sine5, cosine5, sine7, cosine7, sine9, cosine9, sine11, cosine11;
	dd_real sine13, cosine13, sine15, cosine15;

	x86_FIX
	arg1 = dd_real::_2pi * (double) x / (double) N;
	sincos (arg1, sine, cosine);
	results[0] = sine;
	results[0] += epsilon;		
	results[incr] = cosine / results[0];
	results += incr + incr;
	if (amt == 1) goto done;

	sine2 = sine * cosine * 2.0;
	cosine2 = sqr (cosine) - sqr (sine);

	sine3 = sine * cosine2 + sine2 * cosine;
	cosine3 = cosine * cosine2 - sine * sine2;

	results[0] = sine3;
	results[0] += epsilon;		
	results[incr] = cosine3 / results[0];
	results += incr + incr;
	if (amt == 2) goto done;

	sine5 = sine3 * cosine2 + sine2 * cosine3;
	cosine5 = cosine3 * cosine2 - sine3 * sine2;

	results[0] = sine5;
	results[0] += epsilon;		
	results[incr] = cosine5 / results[0];
	results += incr + incr;
	if (amt == 3) goto done;

	sine7 = sine5 * cosine2 + sine2 * cosine5;
	cosine7 = cosine5 * cosine2 - sine5 * sine2;

	results[0] = sine7;
	results[0] += epsilon;		
	results[incr] = cosine7 / results[0];
	results += incr + incr;
	if (amt == 4) goto done;

	sine9 = sine7 * cosine2 + sine2 * cosine7;
	cosine9 = cosine7 * cosine2 - sine7 * sine2;

	results[0] = sine9;
	results[0] += epsilon;		
	results[incr] = cosine9 / results[0];
	results += incr + incr;
	if (amt == 5) goto done;

	sine11 = sine9 * cosine2 + sine2 * cosine9;
	cosine11 = cosine9 * cosine2 - sine9 * sine2;

	results[0] = sine11;
	results[0] += epsilon;		
	results[incr] = cosine11 / results[0];
	results += incr + incr;
	if (amt == 6) goto done;

	sine13 = sine11 * cosine2 + sine2 * cosine11;
	cosine13 = cosine11 * cosine2 - sine11 * sine2;

	results[0] = sine13;
	results[0] += epsilon;		
	results[incr] = cosine13 / results[0];
	results += incr + incr;
	if (amt == 7) goto done;

	sine15 = sine13 * cosine2 + sine2 * cosine13;
	cosine15 = cosine13 * cosine2 - sine13 * sine2;

	results[0] = sine15;
	results[0] += epsilon;		
	results[incr] = cosine15 / results[0];
done:	;
	END_x86_FIX
}

extern "C"
void gwsincos159by (
	unsigned long x,
	unsigned long N,
	double	*results,
	int	incr,
	int	amt)
{
	dd_real arg1, sine, cosine, sine2, cosine2, sine4, cosine4, sine5, cosine5, sine9, cosine9, sine13, cosine13;

	x86_FIX
	arg1 = dd_real::_2pi * (double) x / (double) N;
	sincos (arg1, sine, cosine);
	results[0] = sine;
	results[0] += epsilon;		
	results[incr] = cosine / results[0];
	results += incr + incr;
	if (amt == 1) goto done;

	sine2 = sine * cosine * 2.0;
	cosine2 = sqr (cosine) - sqr (sine);

	sine4 = sine2 * cosine2 * 2.0;
	cosine4 = sqr (cosine2) - sqr (sine2);

	sine5 = sine * cosine4 + sine4 * cosine;
	cosine5 = cosine * cosine4 - sine * sine4;

	results[0] = sine5;
	results[0] += epsilon;		
	results[incr] = cosine5 / results[0];
	results += incr + incr;
	if (amt == 2) goto done;

	sine9 = sine5 * cosine4 + sine4 * cosine5;
	cosine9 = cosine5 * cosine4 - sine5 * sine4;

	results[0] = sine9;
	results[0] += epsilon;		
	results[incr] = cosine9 / results[0];
	results += incr + incr;
	if (amt == 3) goto done;

	sine13 = sine9 * cosine4 + sine4 * cosine9;
	cosine13 = cosine9 * cosine4 - sine9 * sine4;

	results[0] = sine13;
	results[0] += epsilon;		
	results[incr] = cosine13 / results[0];
done:	;
	END_x86_FIX
}

extern "C"
void gwsincos125by (
	unsigned long x,
	unsigned long N,
	double	*results,
	int	incr)
{
	dd_real arg1, sine, cosine, sine2, cosine2, sine4, cosine4, sine5, cosine5;

	x86_FIX
	arg1 = dd_real::_2pi * (double) x / (double) N;
	sincos (arg1, sine, cosine);
	results[0] = sine;
	results[0] += epsilon;		
	results[incr] = cosine / results[0];
	results += incr + incr;

	sine2 = sine * cosine * 2.0;
	cosine2 = sqr (cosine) - sqr (sine);

	results[0] = sine2;
	results[0] += epsilon;		
	results[incr] = cosine2 / results[0];
	results += incr + incr;

	sine4 = sine2 * cosine2 * 2.0;
	cosine4 = sqr (cosine2) - sqr (sine2);

	sine5 = sine * cosine4 + sine4 * cosine;
	cosine5 = cosine * cosine4 - sine * sine4;

	results[0] = sine5;
	results[0] += epsilon;		
	results[incr] = cosine5 / results[0];
	END_x86_FIX
}

extern "C"
void gwsincos1234by_raw (
	unsigned long x,
	unsigned long N,
	double	*results,
	int	incr,
	int	amt)
{
	dd_real arg1, sine, cosine, sine2, cosine2, sine3, cosine3, sine4, cosine4;

	x86_FIX
	arg1 = dd_real::_2pi * (double) x / (double) N;
	sincos (arg1, sine, cosine);
	results[0] = sine;
	results[incr] = cosine;
	results += incr + incr;
	if (amt == 1) goto done;

	sine2 = sine * cosine * 2.0;
	cosine2 = sqr (cosine) - sqr (sine);
	results[0] = sine2;
	results[incr] = cosine2;
	results += incr + incr;
	if (amt == 2) goto done;

	sine3 = sine * cosine2 + sine2 * cosine;
	cosine3 = cosine * cosine2 - sine * sine2;
	results[0] = sine3;
	results[incr] = cosine3;
	results += incr + incr;
	if (amt == 3) goto done;

	sine4 = sine2 * cosine2 * 2.0;
	cosine4 = sqr (cosine2) - sqr (sine2);
	results[0] = sine4;
	results[incr] = cosine4;
done:	;
	END_x86_FIX
}

extern "C"
void gwsincos1plusby (
	unsigned long x,
	unsigned long inc,
	unsigned long N,
	double	*results,
	int	incr,
	int	amt)
{
	dd_real twopi_over_N, sineinc, cosineinc, sine0, cosine0;
	int	num_output;

	x86_FIX
	twopi_over_N = dd_real::_2pi / (double) N;
	sincos (twopi_over_N * (double) inc, sineinc, cosineinc);
	sincos (twopi_over_N * (double) x, sine0, cosine0);

	for (num_output = 0; ; ) {
		dd_real last_sine;
		
		results[0] = sine0;
		results[0] += epsilon;
		results[incr] = cosine0 / results[0];
		results += incr + incr;
		num_output++;
		
		if (num_output == amt) break;
		
		last_sine = sine0;
		sine0 = sine0 * cosineinc + cosine0 * sineinc;
		cosine0 = cosine0 * cosineinc - last_sine * sineinc;
	}
	END_x86_FIX
}

extern "C"
void gwcos1plusby (
	unsigned long x,
	unsigned long inc,
	unsigned long N,
	double	*results,
	int	incr,
	int	amt)
{
	dd_real twopi_over_N, sineinc, cosineinc, sine0, cosine0;
	int	num_output;

	x86_FIX
	twopi_over_N = dd_real::_2pi / (double) N;
	sincos (twopi_over_N * (double) inc, sineinc, cosineinc);
	sincos (twopi_over_N * (double) x, sine0, cosine0);

	for (num_output = 0; ; ) {
		dd_real last_sine;
		
		results[0] = cosine0 / (sine0 + epsilon);
		results += incr;
		num_output++;
		
		if (num_output == amt) break;
		
		last_sine = sine0;
		sine0 = sine0 * cosineinc + cosine0 * sineinc;
		cosine0 = cosine0 * cosineinc - last_sine * sineinc;
	}
	END_x86_FIX
}

#ifdef TRY_SQRT2_TO_REDUCE_ROUNDOFF
extern "C"
void gwsincos1234by_sqrthalf (
	unsigned long x,
	unsigned long N,
	double	*results,
	int	incr)
{
	dd_real arg1, sine, cosine, sine2, cosine2, sine3, cosine3, sine4, cosine4;
	dd_real sqrthalf;

	x86_FIX
	sqrthalf = sqrt (dd_real (0.5));
	arg1 = dd_real::_2pi * (double) x / (double) N;
	sincos (arg1, sine, cosine);
	results[0] = sine * sqrthalf;
	results[0] += epsilon;		
	results[incr] = cosine * sqrthalf / results[0];
	results += incr + incr;

	sine2 = sine * cosine * 2.0;
	cosine2 = sqr (cosine) - sqr (sine);
	results[0] = sine2;
	results[0] += epsilon;		
	results[incr] = cosine2 / results[0];
	results += incr + incr;

	sine3 = sine * cosine2 + sine2 * cosine;
	cosine3 = cosine * cosine2 - sine * sine2;
	results[0] = sine3 * sqrthalf;
	results[0] += epsilon;		
	results[incr] = cosine3 * sqrthalf / results[0];
	results += incr + incr;

	sine4 = sine2 * cosine2 * 2.0;
	cosine4 = sqr (cosine2) - sqr (sine2);
	results[0] = sine4;
	results[0] += epsilon;		
	results[incr] = cosine4 / results[0];
	END_x86_FIX
}
#endif

extern "C"
void gwsincos123by_special7 (
	unsigned long x,
	unsigned long N,
	double	*results,
	int	incr)
{
	dd_real arg1, sine, cosine, sine2, cosine2, sine3, cosine3, tmp;
	dd_real sine975;

	arg1 = dd_real::_2pi * 2.0 / 7.0;		
	sine975 = sin (arg1);				

	x86_FIX
	arg1 = dd_real::_2pi * (double) x / (double) N;
	sincos (arg1, sine, cosine);
	tmp = sine + epsilon;		
	results[0] = tmp * sine975;
	results[incr] = cosine / tmp;
	results += incr + incr;

	sine2 = sine * cosine * 2.0;
	cosine2 = sqr (cosine) - sqr (sine);
	tmp = sine2 + epsilon;		
	results[0] = tmp * sine975;
	results[incr] = cosine2 / tmp;
	results += incr + incr;

	sine3 = sine * cosine2 + sine2 * cosine;
	cosine3 = cosine * cosine2 - sine * sine2;
	tmp = sine3 + epsilon;		
	results[0] = tmp * sine975;
	results[incr] = cosine3 / tmp;
	END_x86_FIX
}

extern "C"
void gwsincos135by_special7 (
	unsigned long x,
	unsigned long N,
	double	*results,
	int	incr)
{
	dd_real arg1, sine, cosine, sine2, cosine2, sine3, cosine3, sine5, cosine5, tmp;
	dd_real sine975;

	arg1 = dd_real::_2pi * 2.0 / 7.0;		
	sine975 = sin (arg1);				

	x86_FIX
	arg1 = dd_real::_2pi * (double) x / (double) N;
	sincos (arg1, sine, cosine);
	tmp = sine + epsilon;		
	results[0] = tmp * sine975;
	results[incr] = cosine / tmp;
	results += incr + incr;

	sine2 = sine * cosine * 2.0;
	cosine2 = sqr (cosine) - sqr (sine);

	sine3 = sine * cosine2 + sine2 * cosine;
	cosine3 = cosine * cosine2 - sine * sine2;
	tmp = sine3 + epsilon;		
	results[0] = tmp * sine975;
	results[incr] = cosine3 / tmp;
	results += incr + incr;

	sine5 = sine3 * cosine2 + sine2 * cosine3;
	cosine5 = cosine3 * cosine2 - sine3 * sine2;
	tmp = sine5 + epsilon;		
	results[0] = tmp * sine975;
	results[incr] = cosine5 / tmp;
	END_x86_FIX
}

extern "C"
void *gwdbldbl_data_alloc (void)
{
	return (malloc (sizeof (struct gwdbldbl_constants)));
}

extern "C"
void gwfft_weight_setup (
	void	*dd_data_arg,
	int	zero_pad,
	double	k,
	unsigned long b,
	unsigned long n,
	signed long c,
	unsigned long fftlen)
{
	dd_real	tmp;

	x86_FIX
	dd_data->gw__b = dd_real ((double) b);
	dd_data->gwdbl__b = (double) b;
	dd_data->gw__logb = log (dd_real ((double) b));
	dd_data->gwdbl__logb = double (dd_data->gw__logb);
	dd_data->gw__fftlen_inverse = dd_real (1.0) / dd_real ((double) fftlen);
	if (zero_pad) {
		dd_data->gw__num_b_per_word = dd_real ((double) (n + n)) / dd_real ((double) fftlen);
		dd_data->gw__c_is_one = TRUE;
		dd_data->gw__over_fftlen = dd_real (2.0) * dd_data->gw__fftlen_inverse;
	} else {
		
		dd_data->gw__num_b_per_word = (dd_real ((double) n) + log (dd_real (k)) / dd_data->gw__logb) / dd_real ((double) fftlen);
		dd_data->gw__c_is_one = (abs ((int) c) == 1);
		dd_data->gw__over_fftlen = dd_real (k * 2.0) / dd_real ((double) fftlen);
		dd_data->gw__logb_abs_c_div_fftlen = log (dd_real (abs ((int) c))) / dd_data->gw__logb / dd_real ((double) fftlen);
		dd_data->gwdbl__logb_abs_c_div_fftlen = double (dd_data->gw__logb_abs_c_div_fftlen);
	}
	dd_data->gwdbl__b_inverse = 1.0 / (double) b;

	tmp = dd_data->gw__num_b_per_word;
	dd_data->int_bpw = (uint32_t) double (floor (tmp));		
	tmp = (tmp - (double) dd_data->int_bpw) * 4294967296.0;
	dd_data->frac_bpw1 = (uint32_t) double (floor (tmp));		
	tmp = (tmp - (double) dd_data->frac_bpw1) * 4294967296.0;
	dd_data->frac_bpw2 = (uint32_t) double (floor (tmp));		
	tmp = (tmp - (double) dd_data->frac_bpw2) * 4294967296.0;
	dd_data->frac_bpw3 = (uint32_t) double (floor (tmp));		

	for (int i = 0; i < NUM_SLOPPY_MULTS; i++) {
		dd_data->fast_sloppy_multiplier[i] = gwfft_weight_over_b (dd_data, i+1);
		dd_data->fast_inv_sloppy_multiplier[i] = gwfft_weight_inverse (dd_data, i+1);
	}
	END_x86_FIX
}

dd_real inline map_to_weight_power (
	void	*dd_data_arg,
	unsigned long j)
{
	uint32_t tmp1;
	uint64_t tmp2, tmp3;
	dd_real	result;

	tmp1 = (uint32_t) j * (uint32_t) dd_data->frac_bpw1;		
	tmp2 = (uint64_t) j * (uint64_t) dd_data->frac_bpw2;		
	tmp3 = (uint64_t) j * (uint64_t) dd_data->frac_bpw3;		
	tmp2 += (tmp3 >> 32);						
	tmp2 += (((uint64_t) tmp1) << 32);				
	tmp2 = 0 - tmp2;						

	result = (double) (tmp2 & ~(uint64_t) 0xFFFFFFFF);
	result += (double) (tmp2 & (uint64_t) 0xFFFFFFFF);
	result *= 5.4210108624275221700372640043497e-20;		

	if (! dd_data->gw__c_is_one) result += dd_data->gw__logb_abs_c_div_fftlen * (double) j;

	return (result);
}

double inline map_to_weight_power_sloppy (
	void	*dd_data_arg,
	unsigned long j)
{
	uint32_t tmp1;
	uint64_t tmp2, tmp3;
	double	result;

	tmp1 = (uint32_t) j * (uint32_t) dd_data->frac_bpw1;		
	tmp2 = (uint64_t) j * (uint64_t) dd_data->frac_bpw2;		
	tmp3 = (uint64_t) j * (uint64_t) dd_data->frac_bpw3;		
	tmp2 += (tmp3 >> 32);						
	tmp2 += (((uint64_t) tmp1) << 32);				
	tmp2 = 0 - tmp2;						

	result = (double) (int64_t) (tmp2 >> 1);			
	result *= 1.0842021724855044340074528008699e-19;		

	if (! dd_data->gw__c_is_one) result += dd_data->gwdbl__logb_abs_c_div_fftlen * (double) j;

	return (result);
}

inline dd_real map_to_weight_power_no_c (
	void	*dd_data_arg,
	unsigned long j)
{
	uint32_t tmp1;
	uint64_t tmp2, tmp3;
	dd_real	result;

	tmp1 = (uint32_t) j * (uint32_t) dd_data->frac_bpw1;		
	tmp2 = (uint64_t) j * (uint64_t) dd_data->frac_bpw2;		
	tmp3 = (uint64_t) j * (uint64_t) dd_data->frac_bpw3;		
	tmp2 += (tmp3 >> 32);						
	tmp2 += (((uint64_t) tmp1) << 32);				
	tmp2 = 0 - tmp2;						

	result = (double) (tmp2 & ~(uint64_t) 0xFFFFFFFF);
	result += (double) (tmp2 & (uint64_t) 0xFFFFFFFF);
	result *= 5.4210108624275221700372640043497e-20;		

	return (result);
}

inline double map_to_weight_power_no_c_sloppy (
	void	*dd_data_arg,
	unsigned long j)
{
	uint32_t tmp1;
	uint64_t tmp2, tmp3;
	double	result;

	tmp1 = (uint32_t) j * (uint32_t) dd_data->frac_bpw1;		
	tmp2 = (uint64_t) j * (uint64_t) dd_data->frac_bpw2;		
	tmp3 = (uint64_t) j * (uint64_t) dd_data->frac_bpw3;		
	tmp2 += (tmp3 >> 32);						
	tmp2 += (((uint64_t) tmp1) << 32);				
	tmp2 = 0 - tmp2;						

	result = (double) (int64_t) (tmp2 >> 1);			
	result *= 1.0842021724855044340074528008699e-19;		

	return (result);
}

inline uint32_t map_to_fft_base (
	void	*dd_data_arg,
	unsigned long j)
{
	uint32_t result;
	uint64_t tmp1, tmp2, tmp3;

	result = j * dd_data->int_bpw;					
	tmp1 = (uint64_t) j * (uint64_t) dd_data->frac_bpw1;		
	tmp2 = (uint64_t) j * (uint64_t) dd_data->frac_bpw2;		
	tmp3 = (uint64_t) j * (uint64_t) dd_data->frac_bpw3;		
	tmp2 += (tmp3 >> 32);						
	tmp1 += (tmp2 >> 32);						
	result += (uint32_t) (tmp1 >> 32);				

	return (result + (((uint32_t) tmp1 | (uint32_t) tmp2) ? 1 : 0));
}

extern "C"
double gwfft_weight (
	void	*dd_data_arg,
	unsigned long j)
{
	dd_real	bpower, result;

	x86_FIX
	bpower = map_to_weight_power (dd_data_arg, j);
	result = exp (dd_data->gw__logb * bpower);
	END_x86_FIX
	return (double (result));
}

double gwfft_weight_over_b (
	void	*dd_data_arg,
	unsigned long j)
{
	dd_real	bpower, result;

	x86_FIX
	bpower = map_to_weight_power (dd_data_arg, j);
	result = exp (dd_data->gw__logb * (bpower - 1.0));
	END_x86_FIX
	return (double (result));
}

extern "C"
double gwfft_weight_sloppy (
	void	*dd_data_arg,
	unsigned long j)
{
	double	bpower = map_to_weight_power_sloppy (dd_data_arg, j);
	return (exp (dd_data->gwdbl__logb * bpower));
}

extern "C"
double gwfft_weight_over_weight (
	void	*dd_data_arg,
	unsigned long j,
	unsigned long k)
{
	dd_real	bpower, div_bpower, result;

	x86_FIX
	bpower = map_to_weight_power (dd_data_arg, j);
	div_bpower = map_to_weight_power (dd_data_arg, k);
	result = exp (dd_data->gw__logb * bpower) / (double) exp (dd_data->gw__logb * div_bpower);
	END_x86_FIX
	return (double (result));
}

extern "C"
double gwfft_weight_inverse (
	void	*dd_data_arg,
	unsigned long j)
{
	dd_real bpower, result;

	x86_FIX
	bpower = map_to_weight_power (dd_data_arg, j);
	result = exp (dd_data->gw__logb * -bpower);
	END_x86_FIX
	return (double (result));
}

extern "C"
double gwfft_weight_inverse_squared (
	void	*dd_data_arg,
	unsigned long j)
{
	dd_real bpower, result;

	x86_FIX
	bpower = map_to_weight_power (dd_data_arg, j);
	result = exp (dd_data->gw__logb * -2.0 * bpower);
	result = result * dd_data->gw__over_fftlen;
	END_x86_FIX
	return (double (result));
}

extern "C"
double gwfft_weight_inverse_sloppy (
	void	*dd_data_arg,
	unsigned long j)
{
	double bpower = map_to_weight_power_sloppy (dd_data_arg, j);
	return (exp (dd_data->gwdbl__logb * -bpower));
}

extern "C"
double gwfft_weight_inverse_over_fftlen (
	void	*dd_data_arg,
	unsigned long j)
{
	dd_real bpower, result;

	x86_FIX
	bpower = map_to_weight_power (dd_data_arg, j);
	result = exp (dd_data->gw__logb * -bpower) * dd_data->gw__over_fftlen;
	END_x86_FIX
	return (double (result));
}

extern "C"
void gwfft_weights3 (
	void	*dd_data_arg,
	unsigned long j,
	double	*fft_weight,
	double	*fft_weight_inverse,
	double	*fft_weight_inverse_over_fftlen)
{
	dd_real bpower, weight;

	x86_FIX
	bpower = map_to_weight_power (dd_data_arg, j);
	weight = exp (dd_data->gw__logb * bpower);
	if (fft_weight != NULL) *fft_weight = double (weight);
	if (fft_weight_inverse != NULL) *fft_weight_inverse = double (1.0 / weight);
	if (fft_weight_inverse_over_fftlen != NULL) *fft_weight_inverse_over_fftlen = double (dd_data->gw__over_fftlen / weight);
	END_x86_FIX
}

extern "C"
double gwfft_weight_exponent (
	void	*dd_data_arg,
	unsigned long j)
{
	if (j == 0) return (0);
	return (map_to_weight_power_no_c_sloppy (dd_data_arg, j));
}

extern "C"
double gwfft_weight_no_c (
	void	*dd_data_arg,
	unsigned long j)
{
	dd_real bpower, result;

	x86_FIX
	bpower = map_to_weight_power_no_c (dd_data_arg, j);
	result = exp (dd_data->gw__logb * bpower);
	END_x86_FIX
	return (double (result));
}

extern "C"
unsigned long gwfft_base (
	void	*dd_data_arg,
	unsigned long j)
{

	if (j == 0) return (0);

	return (map_to_fft_base (dd_data_arg, j));
}

extern "C"
void gwsincos012by_weighted (
	void	*dd_data_arg,
	unsigned long x,
	unsigned long upper_x_incr,
	unsigned long N,
	unsigned long col,
	double	*results,
	int	incr)
{
	dd_real twopi_over_N, sine, cosine, sine2, cosine2;
	dd_real bpower, weight, inv_weight;
	int	i;

	x86_FIX
	bpower = map_to_weight_power (dd_data_arg, col);
	weight = exp (dd_data->gw__logb * bpower);
	inv_weight = dd_data->gw__over_fftlen / weight;

	if (incr >= 4) {
		results[24] = weight;
		results[25] = inv_weight;
	} else {
		for (i = 0; i < incr; i++) {
			results[i] = weight;
			results[incr+i] = inv_weight;
		}
		results += 2*incr;
	}

	twopi_over_N = dd_real::_2pi / (double) N;
	for (i = 0; i < incr; i++) {
		sincos (twopi_over_N * (double) (x + i * upper_x_incr), sine, cosine);

		sine2 = sine * cosine * 2.0;
		cosine2 = sqr (cosine) - sqr (sine);

		sine = sine + epsilon;		
		sine2 = sine2 + epsilon;

		if (incr < 4) {
			results[i] = sine * weight;
			results[incr+i] = cosine / sine;
			results[2*incr+i] = sine * inv_weight;
			results[3*incr+i] = sine2 * weight;
			results[4*incr+i] = cosine2 / sine2;
			results[5*incr+i] = sine2 * inv_weight;
		} else {
			results[i] = cosine / sine;
			results[incr+i] = cosine2 / sine2;
			results[2*incr+i] = sine * weight;
			results[3*incr+i] = sine2 * weight;
			results[4*incr+i] = sine * inv_weight;
			results[5*incr+i] = sine2 * inv_weight;
		}
	}
	END_x86_FIX
}

extern "C"
void gwsincos15by_weighted (
	void	*dd_data_arg,
	unsigned long x,
	unsigned long upper_x_incr,
	unsigned long N,
	unsigned long col,
	double	*results,
	int	incr)
{
	dd_real twopi_over_N, sine, cosine, sine2, cosine2, sine4, cosine4, sine5, cosine5;
	dd_real bpower, weight, inv_weight;
	int	i;

	x86_FIX
	bpower = map_to_weight_power (dd_data_arg, col);
	weight = exp (dd_data->gw__logb * bpower);
	inv_weight = dd_data->gw__over_fftlen / weight;

	twopi_over_N = dd_real::_2pi / (double) N;
	for (i = 0; i < incr; i++) {
		sincos (twopi_over_N * (double) (x + i * upper_x_incr), sine, cosine);

		sine2 = sine * cosine * 2.0;
		cosine2 = sqr (cosine) - sqr (sine);

		sine4 = sine2 * cosine2 * 2.0;
		cosine4 = sqr (cosine2) - sqr (sine2);

		sine5 = sine * cosine4 + sine4 * cosine;
		cosine5 = cosine * cosine4 - sine * sine4;

		sine = sine + epsilon;		
		sine5 = sine5 + epsilon;

		if (incr < 4) {
			results[i] = sine * weight;
			results[incr+i] = cosine / sine;
			results[2*incr+i] = sine * inv_weight;
			results[3*incr+i] = sine5 * weight;
			results[4*incr+i] = cosine5 / sine5;
			results[5*incr+i] = sine5 * inv_weight;
		} else {
			results[i] = cosine / sine;
			results[incr+i] = cosine5 / sine5;
			results[2*incr+i] = sine * weight;
			results[3*incr+i] = sine5 * weight;
			results[4*incr+i] = sine * inv_weight;
			results[5*incr+i] = sine5 * inv_weight;
		}
	}
	END_x86_FIX
}

extern "C"
void gwfft_colweights (
	void	*dd_data_arg,
	void	*colweights_arg,	
	int	count)			
{
	unsigned long j;
	dd_real	*colweights;
	dd_real temp, bpower;

	x86_FIX
	colweights = (dd_real *) colweights_arg;
	for (j = 0; j < (unsigned long) count; j++) {
		bpower = map_to_weight_power (dd_data_arg, j);
		*colweights++ = exp (dd_data->gw__logb * bpower);
	}
	END_x86_FIX
}

extern "C"
void gwsincos1plusby_colweighted (
	unsigned long x,
	unsigned long inc,
	unsigned long N,
	void	*colweights_arg,	
	double	*results,
	int	incr,
	int	amt)
{
	dd_real	*colweights;
	dd_real twopi_over_N, sineinc, cosineinc, sine0, cosine0;
	int	num_output;

	x86_FIX
	twopi_over_N = dd_real::_2pi / (double) N;
	sincos (twopi_over_N * (double) inc, sineinc, cosineinc);
	sincos (twopi_over_N * (double) x, sine0, cosine0);

	colweights = (dd_real *) colweights_arg;
	for (num_output = 0; ; colweights++) {
		dd_real last_sine;
		
		results[0] = sine0;
		results[0] += epsilon;
		results[incr] = cosine0 / results[0];
		results[0] = sine0 * *colweights;
		results[0] += epsilon;
		results += incr + incr;
		num_output++;
		
		if (num_output == amt) break;
		
		last_sine = sine0;
		sine0 = sine0 * cosineinc + cosine0 * sineinc;
		cosine0 = cosine0 * cosineinc - last_sine * sineinc;
	}
	END_x86_FIX
}

extern "C"
void gwfft_weights_fudged (
	void	*dd_data_arg,
	unsigned long j,
	unsigned long b,
	double	*fft_weight,
	double	*fft_weight_inverse,
	double	*fft_weight_over_b,
	double	*fft_weight_inverse_times_b)
{
	dd_real bpower, weight, weight_inverse;

	x86_FIX
	bpower = map_to_weight_power (dd_data_arg, j);
	weight = exp (dd_data->gw__logb * bpower);
	*fft_weight = double (weight);
	*fft_weight_over_b = double (weight / (double) b);
	weight_inverse = 1.0 / weight;
	*fft_weight_inverse = double (weight_inverse);
	*fft_weight_inverse_times_b = double (weight_inverse * (double) b);
	END_x86_FIX
}

extern "C"
void gwfft_weights_times_sine (
	void	*dd_data_arg,
	unsigned long j,		
	unsigned long x,		
	unsigned long N,		
	double	*fft_weight,		
	double	*fft_weight_inverse)	
{
	dd_real bpower, weight, sine0;

	x86_FIX
	bpower = map_to_weight_power (dd_data_arg, j);
	weight = exp (dd_data->gw__logb * bpower);

	if (x == 0) {
		sine0 = epsilon;	
	} else {
		sine0 = sin (dd_real::_2pi * (double) x / (double) N);
	}

	if (fft_weight != NULL) *fft_weight = double (weight * sine0);
	if (fft_weight_inverse != NULL) *fft_weight_inverse = double (sine0 / weight);

	END_x86_FIX
}

extern "C"
double gwfft_partial_weight (
	void	*dd_data_arg,
	unsigned long j,
	unsigned long col)
{
	dd_real j_bpower, col_bpower, result;

	x86_FIX
	j_bpower = map_to_weight_power (dd_data_arg, j);
	col_bpower = map_to_weight_power (dd_data_arg, col);
	result = exp (dd_data->gw__logb * (j_bpower - col_bpower));
	END_x86_FIX
	return (double (result));
}

extern "C"
double gwfft_partial_weight_sloppy (
	void	*dd_data_arg,
	unsigned long j,
	unsigned long col)
{

	double j_bpower = map_to_weight_power_sloppy (dd_data_arg, j);
	double col_bpower = map_to_weight_power_sloppy (dd_data_arg, col);
	return (exp (dd_data->gwdbl__logb * (j_bpower - col_bpower)));
}

extern "C"
double gwfft_partial_weight_inverse (
	void	*dd_data_arg,
	unsigned long j,
	unsigned long col)
{
	dd_real j_bpower, col_bpower, result;

	x86_FIX
	j_bpower = map_to_weight_power (dd_data_arg, j);
	col_bpower = map_to_weight_power (dd_data_arg, col);
	result = exp (dd_data->gw__logb * (col_bpower - j_bpower));
	END_x86_FIX
	return (double (result));
}

extern "C"
double gwfft_partial_weight_inverse_sloppy (
	void	*dd_data_arg,
	unsigned long j,
	unsigned long col)
{
	double j_bpower = map_to_weight_power_sloppy (dd_data_arg, j);
	double col_bpower = map_to_weight_power_sloppy (dd_data_arg, col);
	return (exp (dd_data->gwdbl__logb * (col_bpower - j_bpower)));
}

double inline cached_map_to_weight_power_sloppy (
	void	*dd_data_arg,
	uint64_t *cached_data,
	unsigned long j)
{
	
	uint64_t tmp2 = base128middle;
	tmp2 = ~tmp2;							
	double result = (double) (int64_t) (tmp2 >> 1);			
	result *= 1.0842021724855044340074528008699e-19;		
	if (! dd_data->gw__c_is_one) result += dd_data->gwdbl__logb_abs_c_div_fftlen * (double) j;
	return (result);
}

extern "C"
double gwcached_weight_sloppy (
	void	*dd_data_arg,
	uint64_t *cached_data,
	unsigned long j,
	int32_t	val)
{
	double	bpower, weight;

	if (! dd_data->gw__c_is_one) {
		bpower = cached_map_to_weight_power_sloppy (dd_data_arg, cached_data, j);
		return ((double) val * exp (dd_data->gw__logb * bpower));
	}

	if (j > last_weight_j && j <= last_weight_j + NUM_SLOPPY_MULTS) {
		weight = last_weight_sloppy * dd_data->fast_sloppy_multiplier[j - last_weight_j - 1];

		if (double (weight) < dd_data->gwdbl__b_inverse + 0.001 || (double (weight) > 0.999 && double (weight) < 1.001)) {
			bpower = cached_map_to_weight_power_sloppy (dd_data_arg, cached_data, j);
			weight = exp (dd_data->gwdbl__logb * bpower);
			last_weight_j = j;
			last_weight_sloppy = weight;
		}

		if (double (weight) < 1.0) return (((double) val * dd_data->gwdbl__b) * weight);
	}

	else if (j == last_weight_j)
		weight = last_weight_sloppy;

	else {
		bpower = cached_map_to_weight_power_sloppy (dd_data_arg, cached_data, j);
		weight = exp (dd_data->gwdbl__logb * bpower);
		last_weight_j = j;
		last_weight_sloppy = weight;
	}

	return ((double) val * weight);
}

extern "C"
double gwcached_weight_inverse_sloppy (
	void	*dd_data_arg,
	uint64_t *cached_data,
	unsigned long j)
{
	double	bpower, result;

	if (! dd_data->gw__c_is_one) {
		bpower = cached_map_to_weight_power_sloppy (dd_data_arg, cached_data, j);
		return (exp (dd_data->gwdbl__logb * -bpower));
	}

	if (j > last_inv_weight_j && j <= last_inv_weight_j + NUM_SLOPPY_MULTS) {
		result = last_inv_weight_sloppy * dd_data->fast_inv_sloppy_multiplier[j - last_inv_weight_j - 1];
		int mul_by_b_required = (double (result) <= dd_data->gwdbl__b_inverse);
		if (mul_by_b_required) result *= dd_data->gwdbl__b;

		if (j == last_inv_weight_j + NUM_SLOPPY_MULTS) {
			inv_weight_sloppy_muls++;
			if (dd_data->gwdbl__b == 2.0) {
				if (inv_weight_sloppy_muls <= 48) {
					last_inv_weight_j = j;
					last_inv_weight_sloppy = result;
				}
			} else {
				if (mul_by_b_required) inv_weight_sloppy_muls++;
				if (inv_weight_sloppy_muls <= 47) {
					last_inv_weight_j = j;
					last_inv_weight_sloppy = result;
				}
			}
		}

		if (double (result) < dd_data->gwdbl__b_inverse + 0.001 || double (result) > 0.999) {
			bpower = cached_map_to_weight_power_sloppy (dd_data_arg, cached_data, j);
			result = exp (dd_data->gwdbl__logb * -bpower);
			last_inv_weight_j = j;
			last_inv_weight_sloppy = result;
			inv_weight_sloppy_muls = 0;
		}
	}

	else if (j == last_inv_weight_j)
		result = last_inv_weight_sloppy;

	else {
		bpower = cached_map_to_weight_power_sloppy (dd_data_arg, cached_data, j);
		result = exp (dd_data->gwdbl__logb * -bpower);
		last_inv_weight_j = j;
		last_inv_weight_sloppy = result;
		inv_weight_sloppy_muls = 0;
	}

	return (result);
}

double cached_partial_weight_init (
	void	*dd_data_arg,
	uint64_t *cached_data,
	unsigned long j,
	unsigned long col)
{
	
	uint64_t jtmp2 = base128middle;
	jtmp2 = ~jtmp2;								

	uint32_t ctmp1 = (uint32_t) col * (uint32_t) dd_data->frac_bpw1;	
	uint64_t ctmp2 = (uint64_t) col * (uint64_t) dd_data->frac_bpw2;	
	uint64_t ctmp3 = (uint64_t) col * (uint64_t) dd_data->frac_bpw3;	
	ctmp2 += (ctmp3 >> 32);							
	ctmp2 += (((uint64_t) ctmp1) << 32);					
	ctmp2 = 0 - ctmp2;							

	partial_transition = ctmp2 - jtmp2;

     	double j_bpower = (double) (int64_t) (jtmp2 >> 1);			
	j_bpower *= 1.0842021724855044340074528008699e-19;			
     	double col_bpower = (double) (int64_t) (ctmp2 >> 1);			
	col_bpower *= 1.0842021724855044340074528008699e-19;			

	double bpower = col_bpower - j_bpower;
	if (bpower < 0.0) bpower += 1.0;

	if (! dd_data->gw__c_is_one) bpower += dd_data->gwdbl__logb_abs_c_div_fftlen * (double) ((double) col - (double) j);

	return bpower;
}

int inline cached_partial_which_weight (
	uint64_t *cached_data)
{
	
	return (base128middle < partial_transition);
}

extern "C"
double gwcached_partial_weight_sloppy (
	void	*dd_data_arg,
	uint64_t *cached_data,
	unsigned long j)
{
	
	if (((uint64_t *) partial_weight)[0] == 0) {		
		unsigned long col = dwpncol_value;
		double bpower = cached_partial_weight_init (dd_data_arg, cached_data, j, col);
		partial_weight[0] = exp (dd_data->gwdbl__logb * -bpower);
		partial_weight[1] = partial_weight[0] * dd_data->gwdbl__b;
	}

	return (partial_weight[cached_partial_which_weight (cached_data)]);
}

extern "C"
double gwcached_partial_weight_inverse_sloppy (
	void	*dd_data_arg,
	uint64_t *cached_data,
	unsigned long j)
{
	
	if (((uint64_t *) partial_inv_weight)[0] == 0) {	
		unsigned long col = dwpncol_value;
		double bpower = cached_partial_weight_init (dd_data_arg, cached_data, j, col);
		partial_inv_weight[0] = exp (dd_data->gwdbl__logb * bpower);
		partial_inv_weight[1] = partial_inv_weight[0] * dd_data->gwdbl__b_inverse;
	}

	return (partial_inv_weight[cached_partial_which_weight (cached_data)]);
}

