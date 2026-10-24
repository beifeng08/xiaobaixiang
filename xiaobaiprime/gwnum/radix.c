// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include "cpuid.h"
#include "gwnum.h"
#include "gwutil.h"
#include "radix.h"

int radix_gwcopy (gwhandle *src_gwdata, gwhandle *dst_gwdata, gwnum src, gwnum dst, uint64_t big_word_flags, int num_big_word_flags);
void brute_convert (gwhandle *gwdata, giant g, gwnum x, int offset, uint64_t big_word_flags);
__inline uint64_t intgcd (uint64_t a, uint64_t b) { while (b != 0) { uint64_t temp = a % b; a = b; b = temp; } return a; }

int nonbase2_gianttogw (	
	gwhandle *gwdata,	
	giant	g,		
	gwnum	x)		
{
	gwhandle *work_gwdata;		
	gwnum	pow2_multiplier, t1, t3;
	int	i, err_code, num_chunks, fft_words_per_mult, num_big_word_flags;
	uint64_t big_word_flags;

#define BRUTE_FORCE_WORDS		2
#define giant_extract(s,off,len,d)	{if ((s)->sign <= (off)) (d)->sign = 0; \
					 else { (d)->sign = intmin ((s)->sign - (off), (len)); \
						memcpy ((d)->n, (s)->n+(off), (d)->sign * sizeof (uint32_t)); \
						while((d)->sign && (d)->n[(d)->sign-1] == 0) (d)->sign--;}}

	ASSERTG (g->sign >= 0);		

	if (g->sign <= BRUTE_FORCE_WORDS) {
		stackgiant(tmpg,BRUTE_FORCE_WORDS);
		gtog (g, tmpg);
		dbltogw (gwdata, 0.0, x);
		for (i = 0, big_word_flags = 0; i < 64; i++) if (is_big_word (gwdata, i)) big_word_flags |= ((uint64_t) 1) << i;
		brute_convert (gwdata, tmpg, x, 0, big_word_flags);
		return (0);
	}

	if (gwdata->to_radix_gwdata == NULL) {		
		int	exp, fftlen;			
		int	num_pairs;
		int	b_per_mult;			
		int	radix_bigwords_per_mult;	

		work_gwdata = (gwhandle *) malloc (sizeof (gwhandle));
		if (work_gwdata == NULL) goto oom;

		b_per_mult = (int) ceil ((double) (2 * BRUTE_FORCE_WORDS * 32 + 1) / log2 (gwdata->b));
		
		num_chunks = (int) ceil ((gwdata->bit_length + 32.0) / (BRUTE_FORCE_WORDS * 32));	
		num_pairs = (num_chunks + 1) / 2;							
		
		for (fftlen = 0; ; fftlen++) {
			double	words_per_mult;		
			int	option;

			gwinit (work_gwdata);
			gwset_maxmulbyconst (work_gwdata, 1);
			gwset_safety_margin (work_gwdata, -4.00);
			gwset_minimum_fftlen (work_gwdata, fftlen);
			work_gwdata->radix_bigwords = exp = num_pairs * b_per_mult;
			err_code = gwinfo (work_gwdata, 1.0, gwdata->b, exp, -1);
			if (err_code) {			
				ASSERTG (fftlen == 0);
				fftlen = exp - 1;	
				continue;		
			}

			fftlen = work_gwdata->FFTLEN;
			if (exp < fftlen) exp = fftlen;		
			words_per_mult = (double) b_per_mult * (double) fftlen / (double) exp;

			for (option = 1; option <= 2; option++) {
				int newwpm = (int) floor (words_per_mult) + (option - 1);
				int newbpm = round_up_to_multiple_of (b_per_mult, newwpm / (int) intgcd (fftlen, newwpm));
				if (newwpm > newbpm) newwpm = newbpm;		
				if (fftlen >= num_pairs * newwpm) {
					int non_zeros_per_newwpm = (int) ceil ((double) b_per_mult / ((double) newbpm / (double) newwpm));
					radix_bigwords_per_mult = ((b_per_mult - 1) % non_zeros_per_newwpm) + 1;
					gwinit (work_gwdata);
					gwset_maxmulbyconst (work_gwdata, 1);
					gwset_minimum_fftlen (work_gwdata, fftlen);
					work_gwdata->radix_bigwords = num_pairs * radix_bigwords_per_mult;
					err_code = gwinfo (work_gwdata, 1.0, gwdata->b, exp = newbpm * fftlen / newwpm, -1);
					if (err_code == 0 && fftlen >= (int) work_gwdata->FFTLEN) option = 99;	
				}
			}
			if (option >= 99) break;

		}
		gwinit (work_gwdata);
		gwset_maxmulbyconst (work_gwdata, 1);
		gwset_minimum_fftlen (work_gwdata, fftlen);
		work_gwdata->radix_bigwords = num_pairs * radix_bigwords_per_mult;
		if (gwdata->num_threads > 1) gwset_num_threads (work_gwdata, gwdata->num_threads);
		err_code = gwsetup (work_gwdata, 1.0, gwdata->b, exp, -1);
		if (err_code != 0) goto err;
		ASSERTG (fftlen == work_gwdata->FFTLEN);
		
		gwdata->to_radix_gwdata = work_gwdata;
	}
	work_gwdata = gwdata->to_radix_gwdata;

	pow2_multiplier = gwalloc (work_gwdata);
	if (pow2_multiplier == NULL) goto oom;
	t1 = gwalloc (work_gwdata);
	if (t1 == NULL) goto oom;
	t3 = gwalloc (work_gwdata);
	if (t3 == NULL) goto oom;

	num_chunks = divide_rounding_up (g->sign, BRUTE_FORCE_WORDS);
	fft_words_per_mult = (int) ceil (
			(2 * BRUTE_FORCE_WORDS * 32 + 1)	
			/ (log2 (gwdata->b) * work_gwdata->avg_num_b_per_word));	
	fft_words_per_mult = round_up_to_multiple_of (fft_words_per_mult, work_gwdata->FFTLEN / (int) intgcd (work_gwdata->n, work_gwdata->FFTLEN));

	ASSERTG (fft_words_per_mult <= 64);
	big_word_flags = 0;
	num_big_word_flags = fft_words_per_mult;
	for (i = 0; i < num_big_word_flags; i++) {
		if (is_big_word (work_gwdata, i)) big_word_flags |= ((uint64_t) 1) << i;
	}

	{
		stackgiant(tmpg,BRUTE_FORCE_WORDS+1);
		setone (tmpg);
		gshiftleft (32*BRUTE_FORCE_WORDS, tmpg);
		ulsubg (1, tmpg);
		dbltogw (work_gwdata, 0.0, pow2_multiplier);
		brute_convert (work_gwdata, tmpg, pow2_multiplier, 0, big_word_flags);
		*pow2_multiplier += 1.0;
	}
#ifdef GDEBUG
	gwsetnormroutine (work_gwdata,0,1,0);
#endif

	int	giant_offset, gwnum_offset, num_left;

	num_chunks = (num_chunks + 1) / 2;			
	giant_offset = 0;
	dbltogw (work_gwdata, 0.0, t3);				
	dbltogw (work_gwdata, 0.0, t1);				

	gwnum_offset = 0;
	for (num_left = num_chunks; num_left; num_left--) {
		stackgiant(tmpg,BRUTE_FORCE_WORDS);

		giant_extract (g, giant_offset, BRUTE_FORCE_WORDS, tmpg);
	        brute_convert (work_gwdata, tmpg, t3, gwnum_offset, big_word_flags);

		giant_extract (g, giant_offset + BRUTE_FORCE_WORDS, BRUTE_FORCE_WORDS, tmpg);
	        brute_convert (work_gwdata, tmpg, t1, gwnum_offset, big_word_flags);

		gwnum_offset += fft_words_per_mult;
		giant_offset += BRUTE_FORCE_WORDS * 2;
	}

	gwmul (work_gwdata, pow2_multiplier, t1);		
	gwaddquick (work_gwdata, t3, t1);			

	while (num_chunks > 1) {
		int	j, num_left, gwnum_offset;

		gwstartnextfft (work_gwdata, 1);
		gwsquare (work_gwdata, pow2_multiplier);
		gwstartnextfft (work_gwdata, 0);

		num_chunks = (num_chunks + 1) / 2;			
		dbltogw (work_gwdata, 0.0, t3);
		for (gwnum_offset = 0, num_left = num_chunks; num_left; num_left--, gwnum_offset += fft_words_per_mult * 2) {
			for (j = gwnum_offset; j < gwnum_offset + fft_words_per_mult; j++) {
				if (j + fft_words_per_mult >= (int) work_gwdata->FFTLEN) break;
				
				if (work_gwdata->RATIONAL_FFT || (work_gwdata->cpu_flags & CPU_AVX512F) || work_gwdata->FFT_TYPE != FFT_TYPE_RADIX_4_DWPN) {
					double	*upper_word;
					upper_word = addr (work_gwdata, t1, j + fft_words_per_mult);
					* addr (work_gwdata, t3, j) = *upper_word;
					*upper_word = 0.0;
				} else {
					long	val;
					get_fft_value (work_gwdata, t1, j + fft_words_per_mult, &val);
					set_fft_value (work_gwdata, t3, j, val);
					* addr (work_gwdata, t1, j + fft_words_per_mult) = 0.0;
				}
			}
		}

		gwmul (work_gwdata, pow2_multiplier, t3);		
		gwaddquick (work_gwdata, t3, t1);			
		fft_words_per_mult = fft_words_per_mult * 2;
	}
	ASSERTG (gw_get_maxerr(work_gwdata) < 0.43);

	gwfree (work_gwdata, pow2_multiplier);
	gwfree (work_gwdata, t3);

	err_code = radix_gwcopy (work_gwdata, gwdata, t1, x, big_word_flags, num_big_word_flags);
	if (err_code) goto err;

	gwfree (work_gwdata, t1);

	return (0);

oom:	err_code = GWERROR_MALLOC;
err:	if (work_gwdata != NULL && gwdata->to_radix_gwdata == NULL) free (work_gwdata);
	return (err_code);
}

void brute_convert (
	gwhandle *gwdata,
	giant	g,			
	gwnum	x,			
	int	offset,			
	uint64_t big_word_flags)	
{
	int	mask1, mask2;

	mask1 = intpow ((int) gwdata->b, (int) gwdata->NUM_B_PER_SMALL_WORD);
	mask2 = gwdata->b * mask1;

#if BRUTE_FORCE_WORDS <= 2

	if (g->sign == 0) return;
	ASSERTG (g->sign <= 2);
	{
		uint64_t g64, q;

		if (g->sign == 2) g64 = (((uint64_t) g->n[1]) << 32) + g->n[0];
		else g64 = g->n[0];
		while (g64) {
			int	mask;
			long	value;

			mask = (big_word_flags & 1) ? mask2 : mask1;
			big_word_flags >>= 1;

			q = g64 / mask;				
			value = (long) (g64 - q * mask);	
			set_fft_value (gwdata, x, offset++, value);
			g64 = q;				
		}
	}

#else
	{
		stackgiant(tmpg,BRUTE_FORCE_WORDS+1);
		stackgiant(q,BRUTE_FORCE_WORDS+1);
		while (g->sign) {
			int	mask;
			long	value;

			mask = (big_word_flags & 1) ? mask2 : mask1;
			big_word_flags >>= 1;

			itog (mask, tmpg);			
			gtog (g, q);
			divg (tmpg, q);				
			mulgi (&gwdata->gdata, q, tmpg);
			subg (tmpg, g);				
			value = (g->sign) ? g->n[0] : 0;	
			set_fft_value (gwdata, x, offset++, value);
			gtog (q, g);				
		}
	}
#endif

}

int radix_gwcopy (
	gwhandle *src_gwdata,
	gwhandle *dst_gwdata,
	gwnum	src,
	gwnum	dst,
	uint64_t big_word_flags,
	int	num_big_word_flags)
{
	int	srci, dsti, i, err_code, last_dsti;
	long	srcval, dstval, carry;
	int	srcb, dstb;
	int	radix_powers[32];

	radix_powers[0] = 1;
	radix_powers[1] = dst_gwdata->b;
	for (i = 2; i <= (int) dst_gwdata->NUM_B_PER_SMALL_WORD + 1; i++) radix_powers[i] = radix_powers[1] * radix_powers[i-1];

	srci = 0;
	srcb = 0;
	carry = 0;
	last_dsti = dst_gwdata->ZERO_PADDED_FFT ? dst_gwdata->FFTLEN / 2 + 3 : dst_gwdata->FFTLEN - 1;
	if (dst_gwdata->NUM_B_PER_SMALL_WORD == 0) while (!is_big_word (dst_gwdata, last_dsti)) last_dsti--;
	for (dsti = 0; dsti < (int) dst_gwdata->FFTLEN; dsti++) {
		int	b_we_have;

		dstb = dst_gwdata->NUM_B_PER_SMALL_WORD + !!is_big_word (dst_gwdata, dsti);
		if (dsti > last_dsti) dstb = 0;  
		dstval = 0;
		b_we_have = 0;

		while (b_we_have < dstb) {
			int	b_to_get, remainder;

			if (srcb == 0) {
				
				if (srci == src_gwdata->FFTLEN) break;
				
				err_code = get_fft_value (src_gwdata, src, srci, &srcval);
				if (err_code) return (err_code);
				srcb = src_gwdata->NUM_B_PER_SMALL_WORD + ((big_word_flags >> (srci % num_big_word_flags)) & 1);
				srci++;
			}

			b_to_get = dstb - b_we_have;
			if (b_to_get < srcb) {
				remainder = srcval % radix_powers[b_to_get];
				srcval = srcval / radix_powers[b_to_get];
				srcb -= b_to_get;
			} else {
				remainder = srcval;
				b_to_get = srcb;
				srcb = 0;
			}

			dstval += remainder * radix_powers[b_we_have];
			b_we_have += b_to_get;
		}

		dstval += carry;
		if (dsti == last_dsti) {			
			
			ASSERTG (srcb || srci != src_gwdata->FFTLEN || dst_gwdata->ZERO_PADDED_FFT);
			if (srcb == 0 && srci != src_gwdata->FFTLEN) {
				err_code = get_fft_value (src_gwdata, src, srci, &srcval);
				if (err_code) return (err_code);
			}
			ASSERTG (srcval == 0 || srcval == 1);
			if (srcval) dstval += radix_powers[dstb];
		}
		else if (dstval < -radix_powers[dstb] / 2) {
			dstval += radix_powers[dstb];
			carry = -1;
		}
		else if (dstval > radix_powers[dstb] / 2) {
			dstval -= radix_powers[dstb];
			carry = 1;
		} else
			carry = 0;
		set_fft_value (dst_gwdata, dst, dsti, dstval);
	}

	return (0);
}

struct src_data {
	int	srci;
	int	srcb;
	int	total_srcb;
	int64_t	srcval;
	int64_t	wrap_modulus;
	int64_t	wrap_val;
};

int source_extract (
	gwhandle *gwdata,
	gwnum	x,
	int	num_b_to_get,
	struct src_data *s,
	uint64_t *radix_powers,
	uint64_t *result,
	int	*wraparound_carry)		     
{
	int	b_we_have, b_to_get, err_code;
	int64_t remainder;

	*result = 0;
	for (b_we_have = 0; ; b_we_have += b_to_get) {

		if (s->srcb == 0 && s->total_srcb < (int) gwdata->n) {
			long	fftval;
			err_code = get_fft_value (gwdata, x, s->srci, &fftval);
			if (err_code) return (err_code);
			s->srcval += fftval;
			s->srcb = gwdata->NUM_B_PER_SMALL_WORD + !!is_big_word (gwdata, s->srci);
			s->total_srcb += s->srcb;
			(s->srci)++;
			
			if (s->total_srcb >= (int) gwdata->n) {
				int	b_above_wrap, limit;
				
				if (gwdata->k == 1.0 && !gwdata->ZERO_PADDED_FFT) {
					b_above_wrap = s->srcb;
					s->wrap_modulus = (int64_t) radix_powers[s->srcb];
					s->wrap_val = s->srcval;
					s->srcval = 0;
					s->srcb = 0;
				}
				
				else {
					int	b_below_wrap;
					
					b_above_wrap = s->total_srcb - gwdata->n;
					b_below_wrap = s->srcb - b_above_wrap;
					s->wrap_modulus = (int64_t) gwdata->k;
					s->wrap_val = s->srcval / (int64_t) radix_powers[b_below_wrap];
					s->srcval -= s->wrap_val * (int64_t) radix_powers[b_below_wrap];
					s->srcb = b_below_wrap;
				}
				
				limit = (gwdata->ZERO_PADDED_FFT) ? gwdata->FFTLEN / 2 + 4 : gwdata->FFTLEN;
				while (s->srci < limit) {
					err_code = get_fft_value (gwdata, x, s->srci, &fftval);
					if (err_code) return (err_code);
					s->wrap_val += fftval * (int64_t) radix_powers[b_above_wrap];
					b_above_wrap += gwdata->NUM_B_PER_SMALL_WORD + !!is_big_word (gwdata, s->srci);
					(s->srci)++;
				}
			}
		}

		if (s->srcb == 0 && s->total_srcb >= (int) gwdata->n) {
			s->srcval += s->wrap_val;
			if (s->wrap_modulus == 1) {			
				*wraparound_carry = (int) s->srcval;
				s->srcval = 0;
			}
			else if (s->srcval >= 0) {
				*wraparound_carry = (int) (s->srcval / s->wrap_modulus);
				s->srcval -= *wraparound_carry * s->wrap_modulus;
			}
			else {
				*wraparound_carry = (int) (s->srcval / s->wrap_modulus - 1);
				s->srcval -= *wraparound_carry * s->wrap_modulus;
			}
			s->srcb = 9999;
		}

		if (b_we_have == num_b_to_get) break;

		b_to_get = intmin (s->srcb, num_b_to_get - b_we_have);
		while ((int64_t) radix_powers[b_to_get] <= 0) b_to_get--;	
		remainder = s->srcval % (int64_t) radix_powers[b_to_get];
		s->srcval = s->srcval / (int64_t) radix_powers[b_to_get];
		s->srcb -= b_to_get;
		if (remainder < 0) {
			remainder += (int64_t) radix_powers[b_to_get];
			s->srcval -= 1;
		}

		*result += remainder * (int64_t) radix_powers[b_we_have];
	}

	return (0);
}

__inline void brute_output64 (
	gwhandle *gwdata,
	gwnum	x,
	int	gwnum_offset,
	int	num_fft_words,
	uint64_t value,
	uint64_t big_word_flags)
{
	int	i;

	for (i = 0; i < num_fft_words; i++) {
		int	num_bits = gwdata->NUM_B_PER_SMALL_WORD + (big_word_flags & 1);
		set_fft_value (gwdata, x, gwnum_offset + i, value & ((1 << num_bits) - 1));
		value >>= num_bits;
		big_word_flags >>= 1;
	}
}

int nonbase2_gwtogiant (	
	gwhandle *gwdata,	
	gwnum	x,		
	giant	g)		
{
	gwhandle *work_gwdata;		
	gwnum	powb_multiplier, t1, t3;
	int	i, b_per_uint64, err_code, num_chunks, fft_words_per_mult, num_big_word_flags;
	uint64_t big_word_flags;

	b_per_uint64 = (int) floor (64.0 / log2 (gwdata->b));
	
	num_chunks = (int) ceil (((double) gwdata->n + log2(gwdata->k) + 32.0) / (double) b_per_uint64);

	if (gwdata->from_radix_gwdata == NULL) {	
		int	exp, fftlen;			
		int	num_pairs;
		int	bits_per_mult;			

		work_gwdata = (gwhandle *) malloc (sizeof (gwhandle));
		if (work_gwdata == NULL) goto oom;

		bits_per_mult = (int) ceil ((double) (b_per_uint64 * 2) * log2 (gwdata->b) + 0.1) + 1;
		
		num_pairs = (num_chunks + 1) / 2;						
		
		for (fftlen = 0; ; fftlen++) {
			double	words_per_mult;		
			int	option;

			gwinit (work_gwdata);
			gwset_maxmulbyconst (work_gwdata, 1);
			gwset_minimum_fftlen (work_gwdata, fftlen);
			err_code = gwinfo (work_gwdata, 1.0, 2, exp = num_pairs * bits_per_mult, -1);
			if (err_code) goto err;		

			fftlen = work_gwdata->FFTLEN;
			words_per_mult = (double) bits_per_mult * (double) fftlen / (double) exp;

			if (words_per_mult == ceil (words_per_mult) && fftlen >= num_pairs * (int) words_per_mult) break;

			for (option = 1; option <= 2; option++) {
				int newwpm = (int) floor (words_per_mult) + (option - 1);
				int newbpm = round_up_to_multiple_of (bits_per_mult, newwpm / (int) intgcd (fftlen, newwpm));
				if (fftlen >= num_pairs * newwpm) {
					gwinit (work_gwdata);
					gwset_maxmulbyconst (work_gwdata, 1);
					gwset_minimum_fftlen (work_gwdata, fftlen);
					err_code = gwinfo (work_gwdata, 1.0, 2, exp = newbpm * fftlen / newwpm, -1);
					if (err_code == 0 && fftlen >= (int) work_gwdata->FFTLEN) option = 99;	
				}
			}
			if (option >= 99) break;

		}
		gwinit (work_gwdata);
		gwset_maxmulbyconst (work_gwdata, 1);
		gwset_minimum_fftlen (work_gwdata, fftlen);
		if (gwdata->num_threads > 1) gwset_num_threads (work_gwdata, gwdata->num_threads);
		err_code = gwsetup (work_gwdata, 1.0, 2, exp, -1);
		if (err_code) goto err;
		ASSERTG (fftlen == work_gwdata->FFTLEN);
		
		gwdata->from_radix_gwdata = work_gwdata;
	}
	work_gwdata = gwdata->from_radix_gwdata;

	powb_multiplier = gwalloc (work_gwdata);
	if (powb_multiplier == NULL) goto oom;
	t1 = gwalloc (work_gwdata);
	if (t1 == NULL) goto oom;
	t3 = gwalloc (work_gwdata);
	if (t3 == NULL) goto oom;

	fft_words_per_mult = (int) ceil ((((double) (b_per_uint64 * 2) * log2 (gwdata->b) + 0.1) + 1.0)	
					/ work_gwdata->avg_num_b_per_word);				
	fft_words_per_mult = round_up_to_multiple_of (fft_words_per_mult, work_gwdata->FFTLEN / (int) intgcd (work_gwdata->n, work_gwdata->FFTLEN));

	ASSERTG (fft_words_per_mult <= 64);
	big_word_flags = 0;
	num_big_word_flags = fft_words_per_mult;
	for (i = 0; i < num_big_word_flags; i++) {
		if (is_big_word (work_gwdata, i)) big_word_flags |= ((uint64_t) 1) << i;
	}

	{
		stackgiant (tmpg, 3);				
		ultog (gwdata->b, tmpg);
		power (tmpg, b_per_uint64);
		gianttogw (work_gwdata, tmpg, powb_multiplier);
	}
#ifdef GDEBUG
	gwsetnormroutine (work_gwdata,0,1,0);
#endif

	{
		int	gwnum_offset, num_left, wraparound_carry;
		uint64_t radix_powers[64];
		struct src_data s;

		radix_powers[0] = 1;
		radix_powers[1] = gwdata->b;
		for (i = 2; i <= b_per_uint64; i++) radix_powers[i] = radix_powers[1] * radix_powers[i-1];

		num_chunks = (num_chunks + 1) / 2;			
		dbltogw (work_gwdata, 0.0, t3);				
		dbltogw (work_gwdata, 0.0, t1);				

		memset (&s, 0, sizeof (s));

		gwnum_offset = 0;
		for (num_left = num_chunks; num_left; num_left--) {
			uint64_t value;

			err_code = source_extract (gwdata, x, b_per_uint64, &s, radix_powers, &value, &wraparound_carry);
			if (err_code) goto err;
			brute_output64 (work_gwdata, t3, gwnum_offset, (fft_words_per_mult + 1) / 2, value, big_word_flags);

			err_code = source_extract (gwdata, x, b_per_uint64, &s, radix_powers, &value, &wraparound_carry);
			if (err_code) goto err;
			brute_output64 (work_gwdata, t1, gwnum_offset, (fft_words_per_mult + 1) / 2, value, big_word_flags);

			gwnum_offset += fft_words_per_mult;
		}
		*t3 += (double) wraparound_carry * (double) -gwdata->c; 

		gwmul (work_gwdata, powb_multiplier, t1);		
		gwaddquick (work_gwdata, t3, t1);			
	}

	while (num_chunks > 1) {
		int	j, num_left, gwnum_offset;

		gwstartnextfft (work_gwdata, 1);
		gwsquare (work_gwdata, powb_multiplier);
		gwstartnextfft (work_gwdata, 0);

		num_chunks = (num_chunks + 1) / 2;			
		dbltogw (work_gwdata, 0.0, t3);
		for (gwnum_offset = 0, num_left = num_chunks; num_left; num_left--, gwnum_offset += fft_words_per_mult * 2) {
			for (j = gwnum_offset; j < gwnum_offset + fft_words_per_mult; j++) {
				if (j + fft_words_per_mult >= (int) work_gwdata->FFTLEN) break;
				
				if (work_gwdata->RATIONAL_FFT || (work_gwdata->cpu_flags & CPU_AVX512F) || work_gwdata->FFT_TYPE != FFT_TYPE_RADIX_4_DWPN) {
					double	*upper_word;
					upper_word = addr (work_gwdata, t1, j + fft_words_per_mult);
					* addr (work_gwdata, t3, j) = *upper_word;
					*upper_word = 0.0;
				} else {
					long	val;
					get_fft_value (work_gwdata, t1, j + fft_words_per_mult, &val);
					set_fft_value (work_gwdata, t3, j, val);
					* addr (work_gwdata, t1, j + fft_words_per_mult) = 0.0;
				}
			}
		}

		gwmul (work_gwdata, powb_multiplier, t3);		
		gwaddquick (work_gwdata, t3, t1);			
		fft_words_per_mult = fft_words_per_mult * 2;
	}
	ASSERTG (gw_get_maxerr(work_gwdata) < 0.43);

	gwfree (work_gwdata, powb_multiplier);
	gwfree (work_gwdata, t3);

	{
		gwiter	iter;
		int32_t	val;
		int64_t accum;
		int	bits, accumbits;
		unsigned long limit;
		uint32_t *outptr;

		accum = 0;
		accumbits = 0;
		outptr = g->n;
		limit = divide_rounding_up ((int) ceil (gwdata->bit_length), 32) + 1;
		for (gwiter_init_zero (work_gwdata, &iter, t1); ; ) {
			if (gwiter_index (&iter) < (int) work_gwdata->FFTLEN) {
				err_code = gwiter_get_fft_value (&iter, &val);
				if (err_code) goto err;
				bits = work_gwdata->NUM_B_PER_SMALL_WORD;
				if (gwiter_is_big_word (&iter)) bits++;
				accum += ((int64_t) val) << accumbits;
				accumbits += bits;
				gwiter_next (&iter);
				if (accumbits < 32) continue;
			}
			*outptr++ = (uint32_t) accum;
			accum >>= 32;
			accumbits -= 32;
			if (outptr == g->n + limit) break;
		}
		ASSERTG (accum == 0 || accum == -1);

		g->sign = (long) (outptr - g->n);
		while (g->sign && g->n[g->sign-1] == 0) g->sign--;

		stackgiant(upper,5);					
		gtogshiftrightsplit (work_gwdata->n, g, upper, accum);	
		if (!isZero (upper)) addg (upper, g);			
	}

	gwfree (work_gwdata, t1);

	return (0);

oom:	err_code = GWERROR_MALLOC;
err:	if (work_gwdata != NULL && gwdata->from_radix_gwdata == NULL) free (work_gwdata);
	return (err_code);
}
