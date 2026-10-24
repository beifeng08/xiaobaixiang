/*----------------------------------------------------------------------
| Copyright 2020-2022 Mersenne Research, Inc.  All rights reserved
|
| Auxiliary routines to exponentiate gwnums
+---------------------------------------------------------------------*/

/* Includes */

#include "common.h"		
#include "exponentiate.h"

#define cvt_mem_to_gwnums(g,m) ((unsigned long)(((double)(m)*1048576.0-1000000.0-(double)gwmemused(g))/(double)gwnum_size(g)))

void simple_exponentiate (gwhandle *gwdata, gwnum x, uint64_t power)
{
	uint64_t current_bit;
	int	first_squaring;
	gwnum	scratch;		

	if (power == 0) { dbltogw (gwdata, 1.0, x); return; }
	if (power == 1) return;

	scratch = gwalloc (gwdata);

	for (current_bit = 0x8000000000000000ULL; power < current_bit; current_bit >>= 1);

	first_squaring = 1;
	for (current_bit >>= 1; current_bit; current_bit >>= 1) {
		if (first_squaring) {
			gwfft (gwdata, x, scratch);
			gwmul3 (gwdata, scratch, scratch, x, (current_bit != 1 || (power & 1) == 0) ? GWMUL_STARTNEXTFFT : 0);
			first_squaring = 0;
		} else
			gwsquare2 (gwdata, x, x, (current_bit != 1 || (power & 1) == 0) ? GWMUL_STARTNEXTFFT : 0);
		if (power & current_bit)
			gwmul3 (gwdata, scratch, x, x, current_bit != 1 ? GWMUL_STARTNEXTFFT : 0);
	}

	gwfree (gwdata, scratch);
}

#define is_bit_set(array,bitnum)	((array)[(bitnum)>>6] & (1ULL << ((bitnum) & 63)))

void exponentiate_window3 (gwhandle *gwdata, gwnum x, uint64_t *power, int arraylen)
{
	int	current_bit, top3, first_squaring_done;
	gwnum	x1, x3, x5, x7;		
	int	x7_is_really_x2;

	x1 = gwalloc (gwdata);
	x3 = gwalloc (gwdata);
	x5 = gwalloc (gwdata);
	x7 = gwalloc (gwdata);

	gwfft (gwdata, x, x1);
	gwmul3 (gwdata, x1, x1, x7, GWMUL_STARTNEXTFFT); x7_is_really_x2 = 1;
	gwmul3 (gwdata, x7, x1, x3, GWMUL_FFT_S1 | GWMUL_STARTNEXTFFT);
	gwmul3 (gwdata, x7, x3, x5, GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);

	current_bit = arraylen * 64 - 1;
	while (!is_bit_set (power, current_bit)) current_bit--;

	top3 = 4;
	if (is_bit_set (power, current_bit - 1)) top3 += 2;
	if (is_bit_set (power, current_bit - 2)) top3 += 1;
	if (top3 == 4) gwmul3 (gwdata, x3, x5, x, GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
	if (top3 == 5) gwmul3 (gwdata, x5, x5, x, GWMUL_FFT_S1 | GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
	if (top3 == 6) {
		gwmul3 (gwdata, x7, x5, x7, GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT); x7_is_really_x2 = 0;
		gwmul3 (gwdata, x5, x7, x, GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
	}
	if (top3 == 7) {
		gwmul3 (gwdata, x7, x5, x7, GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT); x7_is_really_x2 = 0;
		gwmul3 (gwdata, x7, x7, x, GWMUL_FFT_S1 | GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
	}
	first_squaring_done = 1;	

	for (current_bit -= 3; current_bit >= 0; current_bit--) {
		if (first_squaring_done) first_squaring_done = 0;
		else gwsquare2 (gwdata, x, x, current_bit || is_bit_set (power, 0) ? GWMUL_STARTNEXTFFT : 0);
		if (!is_bit_set (power, current_bit)) continue;
		
		if (current_bit >= 2 && is_bit_set (power, current_bit - 2)) {
			gwsquare2 (gwdata, x, x, GWMUL_STARTNEXTFFT);
			gwsquare2 (gwdata, x, x, GWMUL_STARTNEXTFFT);
			if (is_bit_set (power, current_bit - 1)) {	
				if (x7_is_really_x2) { gwmul3 (gwdata, x7, x5, x7, GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT); x7_is_really_x2 = 0; }
				gwmul3 (gwdata, x7, x, x, GWMUL_FFT_S1 | (current_bit != 2 ? GWMUL_STARTNEXTFFT : 0));
			} else {					
				gwmul3 (gwdata, x5, x, x, GWMUL_FFT_S1 | (current_bit != 2 ? GWMUL_STARTNEXTFFT : 0));
			}
			current_bit -= 2;
		} else {
			if (current_bit >= 1 && is_bit_set (power, current_bit - 1)) {	
				gwsquare2 (gwdata, x, x, GWMUL_STARTNEXTFFT);
				gwmul3 (gwdata, x3, x, x, current_bit != 1 ? GWMUL_STARTNEXTFFT : 0);
				current_bit -= 1;
			} else {				
				gwmul3 (gwdata, x1, x, x, current_bit != 1 ? GWMUL_STARTNEXTFFT : 0);
			}
		}
	}

	gwfree (gwdata, x1);
	gwfree (gwdata, x3);
	gwfree (gwdata, x5);
	gwfree (gwdata, x7);
}

void exponentiate_windowed (gwhandle *gwdata, gwnum x, uint64_t *power, int bitlen, int num_temps)
{
	gwnum	*xm;			
	int	max_mult, current_bit, topbits;
	bool	first_squaring_done;

	max_mult = num_temps * 2 - 1;
	xm = (gwnum *) malloc ((max_mult + 1) * sizeof (gwnum));

	for (int i = 1; i <= max_mult; i += 2) {
		xm[i] = gwalloc (gwdata);
		if (xm[i] == NULL) {
			max_mult = i - 2;
			break;
		}
	}

	gwfft (gwdata, x, xm[1]);
	gwmul3 (gwdata, xm[1], xm[1], xm[max_mult], GWMUL_STARTNEXTFFT);
	for (int i = 3; i <= max_mult; i += 2) gwmul3 (gwdata, xm[max_mult], xm[i-2], xm[i], GWMUL_FFT_S1 | GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);

	for (topbits = 1, current_bit = bitlen - 2; topbits + topbits + 1 <= max_mult; current_bit--)
		topbits = topbits + topbits + (is_bit_set (power, current_bit) ? 1 : 0);
	if ((topbits & 1) == 0)
		gwmul3 (gwdata, xm[topbits-1], xm[topbits+1], x, GWMUL_FFT_S1 | GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
	else
		gwmul3 (gwdata, xm[topbits], xm[topbits], x, GWMUL_FFT_S1 | GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
	first_squaring_done = 1;	

	while (current_bit >= 0) {
		int	mult;
		if (first_squaring_done) first_squaring_done = 0;
		else gwsquare2 (gwdata, x, x, current_bit || is_bit_set (power, 0) ? GWMUL_STARTNEXTFFT : 0);
		if (!is_bit_set (power, current_bit)) { current_bit--; continue; }
		
		for (mult = 1, current_bit--; current_bit >= 0 && mult + mult + 1 <= max_mult; current_bit--)
			mult = mult + mult + (is_bit_set (power, current_bit) ? 1 : 0);
		while ((mult & 1) == 0) mult >>= 1, current_bit++;
		for (int temp = mult; temp > 1; temp >>= 1) gwsquare2 (gwdata, x, x, GWMUL_STARTNEXTFFT);
		gwmul3 (gwdata, xm[mult], x, x, GWMUL_FFT_S1 | (current_bit != 0 ? GWMUL_STARTNEXTFFT : 0));
	}

	for (int i = 1; i <= max_mult; i += 2) gwfree (gwdata, xm[i]);
	free (xm);
}

void exponentiate_array (gwhandle *gwdata, gwnum x, uint64_t *power, int arraylen, int num_temps)
{
	
	while (arraylen >= 1 && power[arraylen-1] == 0) arraylen--;

	if (arraylen <= 1) {
		if (arraylen == 0) simple_exponentiate (gwdata, x, 0);
		else if (power[0] <= 256 || num_temps < 4) simple_exponentiate (gwdata, x, power[0]);
		else exponentiate_window3 (gwdata, x, power, arraylen);
	}
	
	else if (num_temps <= 4) {
		
		exponentiate_window3 (gwdata, x, power, arraylen);
	}
	
	else {
		int	bitlen;
		struct {
			int	temps;		
			double	cost;		
		} best[3], midpoint;
		#define expcost(x,t)	x.temps = t, x.cost = (t+1) + (bitlen-(log2(t)+2)) + (bitlen-(log2(t)+1))/(log2(t)+2)

		for (bitlen = arraylen * 64; !is_bit_set (power, bitlen-1); bitlen--);

		expcost (best[0], 4);
		expcost (best[1], 250);
		expcost (best[2], 500);

		while (best[0].cost < best[1].cost) {
			best[2] = best[1];
			expcost (best[1], (best[0].temps + best[2].temps) / 2);
		}
		
		while (best[1].cost > best[2].cost) {
			best[0] = best[1];
			best[1] = best[2];
			expcost (best[2], best[1].temps * 2);
		}

		while (best[0].temps + 2 != best[2].temps) {
			
			if (best[1].temps - best[0].temps > best[2].temps - best[1].temps) {	
				expcost (midpoint, (best[0].temps + best[1].temps) / 2);
				if (midpoint.cost < best[1].cost) {				
					best[2] = best[1];
					best[1] = midpoint;
				} else {							
					best[0] = midpoint;
				}
			} else {								
				expcost (midpoint, (best[1].temps + best[2].temps) / 2);
				if (midpoint.cost < best[1].cost) {				
					best[0] = best[1];
					best[1] = midpoint;
				} else {							
					best[2] = midpoint;
				}
			}
		}

		exponentiate_windowed (gwdata, x, power, bitlen, _intmin (num_temps, best[1].temps));
	}
}

void exponentiate (gwhandle *gwdata, gwnum x, uint64_t power)
{
	exponentiate_array (gwdata, x, &power, 1, 16);
}

void exponentiate_mpz (gwhandle *gwdata, gwnum x, mpz_t power)
{
	exponentiate_mpz_limited_temps (gwdata, x, power, cvt_mem_to_gwnums (gwdata, 1024));
}

void exponentiate_mpz_limited_temps (gwhandle *gwdata, gwnum x, mpz_t power, int num_temps)
{
	uint64_t *array;
	size_t	len;

	array = (uint64_t *) malloc (divide_rounding_up (mpz_sizeinbase (power, 2), 64) * sizeof (uint64_t));
	mpz_export (array, &len, -1, sizeof (uint64_t), 0, 0, power);
	exponentiate_array (gwdata, x, array, (int) len, num_temps);
	free (array);
}

