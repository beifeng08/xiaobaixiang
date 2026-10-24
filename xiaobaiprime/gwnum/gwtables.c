/*----------------------------------------------------------------------
| gwtables.c
|
| This file contains the C routines to build sin/cos and weights tables
| that the FFT assembly code needs.
| 
|  Copyright 2002-2023 Mersenne Research, Inc.  All rights reserved.
+---------------------------------------------------------------------*/

/* Include files */

#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include "gwnum.h"
#include "gwtables.h"
#include "gwdbldbl.h"

#define USE_WPN4
#define USE_REDUCED_SINCOS_FFTS

#define round_to_cache_line(p)	(void *) (((intptr_t)(p) + 63) & ~63)
#define bitset(a,i)		{ ((char *)a)[(i) >> 3] |= (1 << ((i) & 7)); }

unsigned long pow_two_above_or_equal (
	unsigned long n)
{
	unsigned long result;

	result = 1;
	for (n = n - 1; n; n = n >> 1) result = result << 1;
	return (result);
}

static __inline int zr4dwpn_delay_count (gwhandle *gwdata)
{

	if (gwdata->PASS1_SIZE == 192 || gwdata->PASS1_SIZE == 960 || gwdata->PASS1_SIZE == 1152 || gwdata->PASS1_SIZE == 1344 ||
	    gwdata->PASS1_SIZE == 1920 || gwdata->PASS1_SIZE == 2304 || gwdata->PASS1_SIZE == 3072) return (12);  
	return (8);		
}

double *zr4dwpn_build_pass1_table (
	gwhandle *gwdata,	
	double	*table)		
{
	struct gwasm_data *asm_data = (struct gwasm_data *) gwdata->asm_data;
	unsigned long pass1_size, pass1_increment, delay_count;
	unsigned long group, i, j, k, N, temp, upper_avx512_word;

	if (gwdata->PASS1_SIZE == 0) {

		if (!gwdata->NEGACYCLIC_FFT) {
			double *weights, *inverse_weights;
			weights = table;
			inverse_weights = weights + 8;
			table = inverse_weights + 8;
			for (i = 0; i < 8; i++) {
				gwfft_weights3 (gwdata->dd_data, i, weights, NULL, inverse_weights);
				weights++;
				inverse_weights++;
			}
		}

		else {
			double	*inverse_weights, colweights[128];

			inverse_weights = table;
			table = inverse_weights + 8;
			for (i = 0; i < 8; i++) *inverse_weights++ = gwfft_weight_inverse_squared (gwdata->dd_data, i);
			
			gwfft_colweights (gwdata->dd_data, &colweights, 8);
			
			N = gwdata->FFTLEN / 2;			
			upper_avx512_word = 8;
			for (j = 0; j < gwdata->FFTLEN / 2; j += 8 * upper_avx512_word) {
				gwsincos1plus01234567by8_colweighted (j, 1, N * 4, &colweights, table);
				gwsincos1plus01234567by8_colweighted (j + upper_avx512_word, 1, N * 4, &colweights, table + 1);
				gwsincos1plus01234567by8_colweighted (j + 2 * upper_avx512_word, 1, N * 4, &colweights, table + 2);
				gwsincos1plus01234567by8_colweighted (j + 3 * upper_avx512_word, 1, N * 4, &colweights, table + 3);
				gwsincos1plus01234567by8_colweighted (j + 4 * upper_avx512_word, 1, N * 4, &colweights, table + 4);
				gwsincos1plus01234567by8_colweighted (j + 5 * upper_avx512_word, 1, N * 4, &colweights, table + 5);
				gwsincos1plus01234567by8_colweighted (j + 6 * upper_avx512_word, 1, N * 4, &colweights, table + 6);
				gwsincos1plus01234567by8_colweighted (j + 7 * upper_avx512_word, 1, N * 4, &colweights, table + 7);
				table += 128;
			}
		}

		table = (double *) ((char *) table + gwdata->FFTLEN / 8 * 7 / 8);

		gwdata->biglit_data_offset = (unsigned long) ((char *) table - (char *) gwdata->pass1_var_data);
		if (gwdata->RATIONAL_FFT);
		else if (gwdata->ZERO_PADDED_FFT) table = (double *) ((char *) table + gwdata->FFTLEN / 64 / 2);
		else table = (double *) ((char *) table + gwdata->FFTLEN / 64);

		table = round_to_cache_line(table);
		gwdata->pass1_var_data_size = (unsigned long) ((char *) table - (char *) gwdata->pass1_var_data);

		return (table);
	}

	pass1_size = gwdata->PASS1_SIZE;
	upper_avx512_word = gwdata->PASS2_SIZE;
	pass1_increment = gwdata->PASS2_SIZE * 8;

	delay_count = zr4dwpn_delay_count (gwdata);

	for (group = 0; group < upper_avx512_word; group += gwdata->PASS1_CACHE_LINES) {

		pass1_size = gwdata->PASS1_SIZE;
		pass1_size /= (delay_count * 2);	
		N = gwdata->PASS2_SIZE;

		N = N * 8;

		{
			double *weights, *inverse_weights;
			weights = table;
			inverse_weights = weights + gwdata->PASS1_CACHE_LINES;
			table = inverse_weights + gwdata->PASS1_CACHE_LINES;
			for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
				gwfft_weights3 (gwdata->dd_data, group + i, weights, NULL, inverse_weights);
				weights++;
				inverse_weights++;
			}
		}

		for (i = 0; i < gwdata->PASS1_CACHE_LINES; i += 8) {
			
			temp = group + i;
			gwsincos1234by8_raw (temp, N, table);
			gwsincos1234by8_raw (temp + 1, N, table+1);
			gwsincos1234by8_raw (temp + 2, N, table+2);
			gwsincos1234by8_raw (temp + 3, N, table+3);
			gwsincos1234by8_raw (temp + 4, N, table+4);
			gwsincos1234by8_raw (temp + 5, N, table+5);
			gwsincos1234by8_raw (temp + 6, N, table+6);
			gwsincos1234by8_raw (temp + 7, N, table+7);
			table += 64;
		}

		if (!gwdata->NEGACYCLIC_FFT) {
			for (i = 0; i < gwdata->PASS1_CACHE_LINES; i += 8) {

				temp = group + i;
				gwsincos159Dby8 (temp, N*2, table);
				gwsincos159Dby8 (temp + 1, N*2, table+1);
				gwsincos159Dby8 (temp + 2, N*2, table+2);
				gwsincos159Dby8 (temp + 3, N*2, table+3);
				gwsincos159Dby8 (temp + 4, N*2, table+4);
				gwsincos159Dby8 (temp + 5, N*2, table+5);
				gwsincos159Dby8 (temp + 6, N*2, table+6);
				gwsincos159Dby8 (temp + 7, N*2, table+7);
				table += 64;
			}
		}

		for (k = 0; k < delay_count; k++) {
			if (k == 0 && !gwdata->NEGACYCLIC_FFT) continue;
			for (i = 0; i < gwdata->PASS1_CACHE_LINES; i += 8) {
				unsigned long bigN, ktemp, actemp, avx512_word;

				for (avx512_word = 0; avx512_word < 8; avx512_word++) {
					unsigned long final_group = group + i + avx512_word;

					if (gwdata->NEGACYCLIC_FFT) {
						bigN = gwdata->FFTLEN * 2;
						actemp = final_group;
					} else {
						bigN = gwdata->FFTLEN;
						actemp = 0;
					}

					if (gwdata->NEGACYCLIC_FFT) {
						if (k <= delay_count/2) ktemp = k * final_group * 4;
						else ktemp = bigN - (delay_count - k) * final_group * 4;
					}
					else {
						ktemp = k * final_group;
					}

					gwsincos1by8_raw (actemp + ktemp, bigN, table + avx512_word);
				}
				table += 16;
			}
		}
		pass1_size /= 8;

		while (pass1_size == 12) {
			N = N * 12;

			for (j = 0; j < N / 12; j += pass1_increment) {
				for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
					temp = (group + j + i);
					gwsincos123456by8 (temp, N, table);
					gwsincos123456by8 (temp + upper_avx512_word, N, table+1);
					gwsincos123456by8 (temp + 2 * upper_avx512_word, N, table+2);
					gwsincos123456by8 (temp + 3 * upper_avx512_word, N, table+3);
					gwsincos123456by8 (temp + 4 * upper_avx512_word, N, table+4);
					gwsincos123456by8 (temp + 5 * upper_avx512_word, N, table+5);
					gwsincos123456by8 (temp + 6 * upper_avx512_word, N, table+6);
					gwsincos123456by8 (temp + 7 * upper_avx512_word, N, table+7);
					table += 96;

					if (!gwdata->NEGACYCLIC_FFT) {
						gwsincos13579Bby8 (temp, N*2, table);
						gwsincos13579Bby8 (temp + upper_avx512_word, N*2, table+1);
						gwsincos13579Bby8 (temp + 2 * upper_avx512_word, N*2, table+2);
						gwsincos13579Bby8 (temp + 3 * upper_avx512_word, N*2, table+3);
						gwsincos13579Bby8 (temp + 4 * upper_avx512_word, N*2, table+4);
						gwsincos13579Bby8 (temp + 5 * upper_avx512_word, N*2, table+5);
						gwsincos13579Bby8 (temp + 6 * upper_avx512_word, N*2, table+6);
						gwsincos13579Bby8 (temp + 7 * upper_avx512_word, N*2, table+7);
						table += 96;
					}
				}
			}
			pass1_size /= 12;
		}

		while (pass1_size == 10) {
			N = N * 10;

			for (j = 0; j < N / 10; j += pass1_increment) {
				for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
					temp = (group + j + i);
					gwsincos12345by8 (temp, N, table);
					gwsincos12345by8 (temp + upper_avx512_word, N, table+1);
					gwsincos12345by8 (temp + 2 * upper_avx512_word, N, table+2);
					gwsincos12345by8 (temp + 3 * upper_avx512_word, N, table+3);
					gwsincos12345by8 (temp + 4 * upper_avx512_word, N, table+4);
					gwsincos12345by8 (temp + 5 * upper_avx512_word, N, table+5);
					gwsincos12345by8 (temp + 6 * upper_avx512_word, N, table+6);
					gwsincos12345by8 (temp + 7 * upper_avx512_word, N, table+7);
					table += 80;

					if (!gwdata->NEGACYCLIC_FFT) {
						gwsincos13579by8 (temp, N*2, table);
						gwsincos13579by8 (temp + upper_avx512_word, N*2, table+1);
						gwsincos13579by8 (temp + 2 * upper_avx512_word, N*2, table+2);
						gwsincos13579by8 (temp + 3 * upper_avx512_word, N*2, table+3);
						gwsincos13579by8 (temp + 4 * upper_avx512_word, N*2, table+4);
						gwsincos13579by8 (temp + 5 * upper_avx512_word, N*2, table+5);
						gwsincos13579by8 (temp + 6 * upper_avx512_word, N*2, table+6);
						gwsincos13579by8 (temp + 7 * upper_avx512_word, N*2, table+7);
						table += 80;
					}
				}
			}
			pass1_size /= 10;
		}

		while (pass1_size % 5 == 0) {
			N = N * 5;

			for (j = 0; j < N / 5; j += pass1_increment) {
				for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
					temp = (group + j + i);
					gwsincos12by8 (temp, N, table);
					gwsincos12by8 (temp + upper_avx512_word, N, table+1);
					gwsincos12by8 (temp + 2 * upper_avx512_word, N, table+2);
					gwsincos12by8 (temp + 3 * upper_avx512_word, N, table+3);
					gwsincos12by8 (temp + 4 * upper_avx512_word, N, table+4);
					gwsincos12by8 (temp + 5 * upper_avx512_word, N, table+5);
					gwsincos12by8 (temp + 6 * upper_avx512_word, N, table+6);
					gwsincos12by8 (temp + 7 * upper_avx512_word, N, table+7);
					table += 32;

					if (!gwdata->NEGACYCLIC_FFT) {
						gwsincos13by8 (temp, N*2, table);
						gwsincos13by8 (temp + upper_avx512_word, N*2, table+1);
						gwsincos13by8 (temp + 2 * upper_avx512_word, N*2, table+2);
						gwsincos13by8 (temp + 3 * upper_avx512_word, N*2, table+3);
						gwsincos13by8 (temp + 4 * upper_avx512_word, N*2, table+4);
						gwsincos13by8 (temp + 5 * upper_avx512_word, N*2, table+5);
						gwsincos13by8 (temp + 6 * upper_avx512_word, N*2, table+6);
						gwsincos13by8 (temp + 7 * upper_avx512_word, N*2, table+7);
						table += 32;
					}
				}
			}
			pass1_size /= 5;
		}

		while (pass1_size % 6 == 0) {
			N = N * 6;

			for (j = 0; j < N / 6; j += pass1_increment) {
				for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
					temp = (group + j + i);
					gwsincos123by8 (temp, N, table);
					gwsincos123by8 (temp + upper_avx512_word, N, table+1);
					gwsincos123by8 (temp + 2 * upper_avx512_word, N, table+2);
					gwsincos123by8 (temp + 3 * upper_avx512_word, N, table+3);
					gwsincos123by8 (temp + 4 * upper_avx512_word, N, table+4);
					gwsincos123by8 (temp + 5 * upper_avx512_word, N, table+5);
					gwsincos123by8 (temp + 6 * upper_avx512_word, N, table+6);
					gwsincos123by8 (temp + 7 * upper_avx512_word, N, table+7);
					table += 48;

					if (!gwdata->NEGACYCLIC_FFT) {
						gwsincos135by8 (temp, N*2, table);
						gwsincos135by8 (temp + upper_avx512_word, N*2, table+1);
						gwsincos135by8 (temp + 2 * upper_avx512_word, N*2, table+2);
						gwsincos135by8 (temp + 3 * upper_avx512_word, N*2, table+3);
						gwsincos135by8 (temp + 4 * upper_avx512_word, N*2, table+4);
						gwsincos135by8 (temp + 5 * upper_avx512_word, N*2, table+5);
						gwsincos135by8 (temp + 6 * upper_avx512_word, N*2, table+6);
						gwsincos135by8 (temp + 7 * upper_avx512_word, N*2, table+7);
						table += 48;
					}
				}
			}
			pass1_size /= 6;
		}

		while (pass1_size % 7 == 0) {
			N = N * 7;

			for (j = 0; j < N / 7; j += pass1_increment) {
				for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
					temp = (group + j + i);
					gwsincos123by8_special7 (temp, N, table);
					gwsincos123by8_special7 (temp + upper_avx512_word, N, table+1);
					gwsincos123by8_special7 (temp + 2 * upper_avx512_word, N, table+2);
					gwsincos123by8_special7 (temp + 3 * upper_avx512_word, N, table+3);
					gwsincos123by8_special7 (temp + 4 * upper_avx512_word, N, table+4);
					gwsincos123by8_special7 (temp + 5 * upper_avx512_word, N, table+5);
					gwsincos123by8_special7 (temp + 6 * upper_avx512_word, N, table+6);
					gwsincos123by8_special7 (temp + 7 * upper_avx512_word, N, table+7);
					table += 48;

					if (!gwdata->NEGACYCLIC_FFT) {
						gwsincos135by8_special7 (temp, N*2, table);
						gwsincos135by8_special7 (temp + upper_avx512_word, N*2, table+1);
						gwsincos135by8_special7 (temp + 2 * upper_avx512_word, N*2, table+2);
						gwsincos135by8_special7 (temp + 3 * upper_avx512_word, N*2, table+3);
						gwsincos135by8_special7 (temp + 4 * upper_avx512_word, N*2, table+4);
						gwsincos135by8_special7 (temp + 5 * upper_avx512_word, N*2, table+5);
						gwsincos135by8_special7 (temp + 6 * upper_avx512_word, N*2, table+6);
						gwsincos135by8_special7 (temp + 7 * upper_avx512_word, N*2, table+7);
						table += 48;
					}
				}
			}
			pass1_size /= 7;
		}

		while (pass1_size == 16) {
			N = N * 16;

			for (j = 0; j < N / 16; j += pass1_increment) {
				for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
					temp = (group + j + i);
					gwsincos12345678by8 (temp, N, table);
					gwsincos12345678by8 (temp + upper_avx512_word, N, table+1);
					gwsincos12345678by8 (temp + 2 * upper_avx512_word, N, table+2);
					gwsincos12345678by8 (temp + 3 * upper_avx512_word, N, table+3);
					gwsincos12345678by8 (temp + 4 * upper_avx512_word, N, table+4);
					gwsincos12345678by8 (temp + 5 * upper_avx512_word, N, table+5);
					gwsincos12345678by8 (temp + 6 * upper_avx512_word, N, table+6);
					gwsincos12345678by8 (temp + 7 * upper_avx512_word, N, table+7);
					table += 128;

					if (!gwdata->NEGACYCLIC_FFT) {
						gwsincos13579BDFby8 (temp, N*2, table);
						gwsincos13579BDFby8 (temp + upper_avx512_word, N*2, table+1);
						gwsincos13579BDFby8 (temp + 2 * upper_avx512_word, N*2, table+2);
						gwsincos13579BDFby8 (temp + 3 * upper_avx512_word, N*2, table+3);
						gwsincos13579BDFby8 (temp + 4 * upper_avx512_word, N*2, table+4);
						gwsincos13579BDFby8 (temp + 5 * upper_avx512_word, N*2, table+5);
						gwsincos13579BDFby8 (temp + 6 * upper_avx512_word, N*2, table+6);
						gwsincos13579BDFby8 (temp + 7 * upper_avx512_word, N*2, table+7);
						table += 128;
					}
				}
			}
			pass1_size /= 16;
		}

		while (pass1_size % 8 == 0) {
			N = N * 8;

			for (j = 0; j < N / 8; j += pass1_increment) {
				for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
					temp = (group + j + i);
					gwsincos1234by8 (temp, N, table);
					gwsincos1234by8 (temp + upper_avx512_word, N, table+1);
					gwsincos1234by8 (temp + 2 * upper_avx512_word, N, table+2);
					gwsincos1234by8 (temp + 3 * upper_avx512_word, N, table+3);
					gwsincos1234by8 (temp + 4 * upper_avx512_word, N, table+4);
					gwsincos1234by8 (temp + 5 * upper_avx512_word, N, table+5);
					gwsincos1234by8 (temp + 6 * upper_avx512_word, N, table+6);
					gwsincos1234by8 (temp + 7 * upper_avx512_word, N, table+7);
#ifdef TRY_SQRT2_TO_REDUCE_ROUNDOFF
{
	gwsincos1234by8_sqrthalf (temp, N, table);
	gwsincos1234by8_sqrthalf (temp + upper_avx512_word, N, table+1);
	gwsincos1234by8_sqrthalf (temp + 2 * upper_avx512_word, N, table+2);
	gwsincos1234by8_sqrthalf (temp + 3 * upper_avx512_word, N, table+3);
	gwsincos1234by8_sqrthalf (temp + 4 * upper_avx512_word, N, table+4);
	gwsincos1234by8_sqrthalf (temp + 5 * upper_avx512_word, N, table+5);
	gwsincos1234by8_sqrthalf (temp + 6 * upper_avx512_word, N, table+6);
	gwsincos1234by8_sqrthalf (temp + 7 * upper_avx512_word, N, table+7);
}
#endif
					table += 64;

					if (!gwdata->NEGACYCLIC_FFT) {
						gwsincos1357by8 (temp, N*2, table);
						gwsincos1357by8 (temp + upper_avx512_word, N*2, table+1);
						gwsincos1357by8 (temp + 2 * upper_avx512_word, N*2, table+2);
						gwsincos1357by8 (temp + 3 * upper_avx512_word, N*2, table+3);
						gwsincos1357by8 (temp + 4 * upper_avx512_word, N*2, table+4);
						gwsincos1357by8 (temp + 5 * upper_avx512_word, N*2, table+5);
						gwsincos1357by8 (temp + 6 * upper_avx512_word, N*2, table+6);
						gwsincos1357by8 (temp + 7 * upper_avx512_word, N*2, table+7);
						table += 64;
					}
				}
			}
			pass1_size /= 8;
		}

		table = (double *) ((char *) table + gwdata->PASS1_SIZE * gwdata->PASS1_CACHE_LINES / 64);

		if (group == 0) gwdata->biglit_data_offset = (unsigned long) ((char *) table - (char *) gwdata->pass1_var_data);
		if (gwdata->RATIONAL_FFT);
		else if (gwdata->ZERO_PADDED_FFT) table = (double *) ((char *) table + gwdata->PASS1_SIZE * gwdata->PASS1_CACHE_LINES / 64 / 2);
		else table = (double *) ((char *) table + gwdata->PASS1_SIZE * gwdata->PASS1_CACHE_LINES / 64);

		table = round_to_cache_line(table);
		if (group == 0) gwdata->pass1_var_data_size = (unsigned long) ((char *) table - (char *) gwdata->pass1_var_data);
	}

	return (table);
}

double *zr4dwpn_build_fixed_pass1_table (
	gwhandle *gwdata,	
	double	*table)		
{
	unsigned long upper_avx512_word, pass1_increment, pass1_size, i, j, N, delay_count;

	upper_avx512_word = gwdata->PASS2_SIZE;
	pass1_increment = 8 * gwdata->PASS2_SIZE;
	pass1_size = gwdata->PASS1_SIZE;
	delay_count = zr4dwpn_delay_count (gwdata);

	if (! gwdata->NEGACYCLIC_FFT) {
		N = gwdata->FFTLEN;
		if (delay_count == 8) {
			for (j = 0; j < N / 16; j += pass1_increment) {
				for (i = 1; i <= 7; i++) {	
					gwsincos1by8 (i * j, N, table);
					gwsincos1by8 (i * (j + upper_avx512_word), N, table+1);
					gwsincos1by8 (i * (j + 2 * upper_avx512_word), N, table+2);
					gwsincos1by8 (i * (j + 3 * upper_avx512_word), N, table+3);
					gwsincos1by8 (i * (j + 4 * upper_avx512_word), N, table+4);
					gwsincos1by8 (i * (j + 5 * upper_avx512_word), N, table+5);
					gwsincos1by8 (i * (j + 6 * upper_avx512_word), N, table+6);
					gwsincos1by8 (i * (j + 7 * upper_avx512_word), N, table+7);
					table += 16;
				}
			}
			N = N / 16;
		}
		if (delay_count == 12) {
			for (j = 0; j < N / 24; j += pass1_increment) {
				for (i = 1; i <= 11; i++) {	
					gwsincos1by8 (i * j, N, table);
					gwsincos1by8 (i * (j + upper_avx512_word), N, table+1);
					gwsincos1by8 (i * (j + 2 * upper_avx512_word), N, table+2);
					gwsincos1by8 (i * (j + 3 * upper_avx512_word), N, table+3);
					gwsincos1by8 (i * (j + 4 * upper_avx512_word), N, table+4);
					gwsincos1by8 (i * (j + 5 * upper_avx512_word), N, table+5);
					gwsincos1by8 (i * (j + 6 * upper_avx512_word), N, table+6);
					gwsincos1by8 (i * (j + 7 * upper_avx512_word), N, table+7);
					table += 16;
				}
			}
			N = N / 24;
		}
	}

	else {
		N = gwdata->FFTLEN / 2;

		if (delay_count == 5) {
			for (j = 0; j < N / 5; j += pass1_increment) {
				
				gwsincos1plus01234by8 (j, N / 5, N * 4, table);
				gwsincos1plus01234by8 (j + upper_avx512_word, N / 5, N * 4, table + 1);
				gwsincos1plus01234by8 (j + 2 * upper_avx512_word, N / 5, N * 4, table + 2);
				gwsincos1plus01234by8 (j + 3 * upper_avx512_word, N / 5, N * 4, table + 3);
				gwsincos1plus01234by8 (j + 4 * upper_avx512_word, N / 5, N * 4, table + 4);
				gwsincos1plus01234by8 (j + 5 * upper_avx512_word, N / 5, N * 4, table + 5);
				gwsincos1plus01234by8 (j + 6 * upper_avx512_word, N / 5, N * 4, table + 6);
				gwsincos1plus01234by8 (j + 7 * upper_avx512_word, N / 5, N * 4, table + 7);
				table += 80;
				
				gwsincos12by8 (j, N, table);
				gwsincos12by8 (j + upper_avx512_word, N, table+1);
				gwsincos12by8 (j + 2 * upper_avx512_word, N, table+2);
				gwsincos12by8 (j + 3 * upper_avx512_word, N, table+3);
				gwsincos12by8 (j + 4 * upper_avx512_word, N, table+4);
				gwsincos12by8 (j + 5 * upper_avx512_word, N, table+5);
				gwsincos12by8 (j + 6 * upper_avx512_word, N, table+6);
				gwsincos12by8 (j + 7 * upper_avx512_word, N, table+7);
				table += 32;
			}
			N = N / 5;
		}

		if (delay_count == 8) {
			for (j = 0; j < N / 8; j += pass1_increment) {
				
				gwcos1plus01234567by8 (j, N / 8, N * 4, table);
				gwcos1plus01234567by8 (j + upper_avx512_word, N / 8, N * 4, table + 1);
				gwcos1plus01234567by8 (j + 2 * upper_avx512_word, N / 8, N * 4, table + 2);
				gwcos1plus01234567by8 (j + 3 * upper_avx512_word, N / 8, N * 4, table + 3);
				gwcos1plus01234567by8 (j + 4 * upper_avx512_word, N / 8, N * 4, table + 4);
				gwcos1plus01234567by8 (j + 5 * upper_avx512_word, N / 8, N * 4, table + 5);
				gwcos1plus01234567by8 (j + 6 * upper_avx512_word, N / 8, N * 4, table + 6);
				gwcos1plus01234567by8 (j + 7 * upper_avx512_word, N / 8, N * 4, table + 7);
				table += 64;
				
				gwsincos1234by8 (j, N, table);
				gwsincos1234by8 (j + upper_avx512_word, N, table+1);
				gwsincos1234by8 (j + 2 * upper_avx512_word, N, table+2);
				gwsincos1234by8 (j + 3 * upper_avx512_word, N, table+3);
				gwsincos1234by8 (j + 4 * upper_avx512_word, N, table+4);
				gwsincos1234by8 (j + 5 * upper_avx512_word, N, table+5);
				gwsincos1234by8 (j + 6 * upper_avx512_word, N, table+6);
				gwsincos1234by8 (j + 7 * upper_avx512_word, N, table+7);
#ifdef TRY_SQRT2_TO_REDUCE_ROUNDOFF
{
	gwsincos1234by8_sqrthalf (j, N, table);
	gwsincos1234by8_sqrthalf (j + upper_avx512_word, N, table+1);
	gwsincos1234by8_sqrthalf (j + 2 * upper_avx512_word, N, table+2);
	gwsincos1234by8_sqrthalf (j + 3 * upper_avx512_word, N, table+3);
	gwsincos1234by8_sqrthalf (j + 4 * upper_avx512_word, N, table+4);
	gwsincos1234by8_sqrthalf (j + 5 * upper_avx512_word, N, table+5);
	gwsincos1234by8_sqrthalf (j + 6 * upper_avx512_word, N, table+6);
	gwsincos1234by8_sqrthalf (j + 7 * upper_avx512_word, N, table+7);
}
#endif
				table += 64;
			}
			N = N / 8;
		}

		if (delay_count == 12) {
			for (j = 0; j < N / 12; j += pass1_increment) {
				
				gwcos1plus0123456789ABby8 (j, N / 12, N * 4, table);
				gwcos1plus0123456789ABby8 (j + upper_avx512_word, N / 12, N * 4, table + 1);
				gwcos1plus0123456789ABby8 (j + 2 * upper_avx512_word, N / 12, N * 4, table + 2);
				gwcos1plus0123456789ABby8 (j + 3 * upper_avx512_word, N / 12, N * 4, table + 3);
				gwcos1plus0123456789ABby8 (j + 4 * upper_avx512_word, N / 12, N * 4, table + 4);
				gwcos1plus0123456789ABby8 (j + 5 * upper_avx512_word, N / 12, N * 4, table + 5);
				gwcos1plus0123456789ABby8 (j + 6 * upper_avx512_word, N / 12, N * 4, table + 6);
				gwcos1plus0123456789ABby8 (j + 7 * upper_avx512_word, N / 12, N * 4, table + 7);
				table += 96;
				
				gwsincos123456by8 (j, N, table);
				gwsincos123456by8 (j + upper_avx512_word, N, table+1);
				gwsincos123456by8 (j + 2 * upper_avx512_word, N, table+2);
				gwsincos123456by8 (j + 3 * upper_avx512_word, N, table+3);
				gwsincos123456by8 (j + 4 * upper_avx512_word, N, table+4);
				gwsincos123456by8 (j + 5 * upper_avx512_word, N, table+5);
				gwsincos123456by8 (j + 6 * upper_avx512_word, N, table+6);
				gwsincos123456by8 (j + 7 * upper_avx512_word, N, table+7);
				table += 96;
			}
			N = N / 12;
		}
	}

	return (table);
}

double *zr4_build_pass2_complex_table (
	gwhandle *gwdata,	
	double	*table)		
{
	unsigned long i, N;

	N = gwdata->PASS2_SIZE;

	if (gwdata->PASS2_SIZE != 640 && gwdata->PASS2_SIZE != 4480 && gwdata->PASS2_SIZE != 6400 &&
	    gwdata->PASS2_SIZE != 7680 && gwdata->PASS2_SIZE != 10240) {
		while (N % 5 == 0) {
			for (i = 0; i < N / 5; i += 8) {
				gwsincos12by8 (i, N, table);
				gwsincos12by8 (i+1, N, table+1);
				gwsincos12by8 (i+2, N, table+2);
				gwsincos12by8 (i+3, N, table+3);
				gwsincos12by8 (i+4, N, table+4);
				gwsincos12by8 (i+5, N, table+5);
				gwsincos12by8 (i+6, N, table+6);
				gwsincos12by8 (i+7, N, table+7);
				table += 32;
			}
			N = N / 5;
			if (N == 10*64) break;  
		}
	}

	while (N % 7 == 0) {
		for (i = 0; i < N / 7; i += 8) {
			gwsincos123by8_special7 (i, N, table);
			gwsincos123by8_special7 (i+1, N, table+1);
			gwsincos123by8_special7 (i+2, N, table+2);
			gwsincos123by8_special7 (i+3, N, table+3);
			gwsincos123by8_special7 (i+4, N, table+4);
			gwsincos123by8_special7 (i+5, N, table+5);
			gwsincos123by8_special7 (i+6, N, table+6);
			gwsincos123by8_special7 (i+7, N, table+7);
			table += 48;
		}
		N = N / 7;
	}

	while (N % 10 == 0) {
		for (i = 0; i < N / 10; i += 8) {
			gwsincos12345by8 (i, N, table);
			gwsincos12345by8 (i+1, N, table+1);
			gwsincos12345by8 (i+2, N, table+2);
			gwsincos12345by8 (i+3, N, table+3);
			gwsincos12345by8 (i+4, N, table+4);
			gwsincos12345by8 (i+5, N, table+5);
			gwsincos12345by8 (i+6, N, table+6);
			gwsincos12345by8 (i+7, N, table+7);
			table += 80;
		}
		N = N / 10;
	}

	while (N == 12*64 || N == 12*6*64 || N == 12*8*64 || N == 12*12*64 || N == 12*16*64) {
		for (i = 0; i < N / 12; i += 8) {
			gwsincos123456by8 (i, N, table);
			gwsincos123456by8 (i+1, N, table+1);
			gwsincos123456by8 (i+2, N, table+2);
			gwsincos123456by8 (i+3, N, table+3);
			gwsincos123456by8 (i+4, N, table+4);
			gwsincos123456by8 (i+5, N, table+5);
			gwsincos123456by8 (i+6, N, table+6);
			gwsincos123456by8 (i+7, N, table+7);
			table += 96;
		}
		N = N / 12;
	}

	while (N % 6 == 0) {
		for (i = 0; i < N / 6; i += 8) {
			gwsincos123by8 (i, N, table);
			gwsincos123by8 (i+1, N, table+1);
			gwsincos123by8 (i+2, N, table+2);
			gwsincos123by8 (i+3, N, table+3);
			gwsincos123by8 (i+4, N, table+4);
			gwsincos123by8 (i+5, N, table+5);
			gwsincos123by8 (i+6, N, table+6);
			gwsincos123by8 (i+7, N, table+7);
			table += 48;
		}
		N = N / 6;
	}

	while (N == 16*64 || N == 16*8*64 || N == 16*16*64) {
		for (i = 0; i < N / 16; i += 8) {
			gwsincos12345678by8 (i, N, table);
			gwsincos12345678by8 (i+1, N, table+1);
			gwsincos12345678by8 (i+2, N, table+2);
			gwsincos12345678by8 (i+3, N, table+3);
			gwsincos12345678by8 (i+4, N, table+4);
			gwsincos12345678by8 (i+5, N, table+5);
			gwsincos12345678by8 (i+6, N, table+6);
			gwsincos12345678by8 (i+7, N, table+7);
			table += 128;
		}
		N = N / 16;
	}

	while (N > 8) {
		for (i = 0; i < N / 8; i += 8) {
			gwsincos1234by8 (i, N, table);
			gwsincos1234by8 (i+1, N, table+1);
			gwsincos1234by8 (i+2, N, table+2);
			gwsincos1234by8 (i+3, N, table+3);
			gwsincos1234by8 (i+4, N, table+4);
			gwsincos1234by8 (i+5, N, table+5);
			gwsincos1234by8 (i+6, N, table+6);
			gwsincos1234by8 (i+7, N, table+7);
#ifdef TRY_SQRT2_TO_REDUCE_ROUNDOFF
if (N > 64) {
	gwsincos1234by8_sqrthalf (i, N, table);
	gwsincos1234by8_sqrthalf (i+1, N, table+1);
	gwsincos1234by8_sqrthalf (i+2, N, table+2);
	gwsincos1234by8_sqrthalf (i+3, N, table+3);
	gwsincos1234by8_sqrthalf (i+4, N, table+4);
	gwsincos1234by8_sqrthalf (i+5, N, table+5);
	gwsincos1234by8_sqrthalf (i+6, N, table+6);
	gwsincos1234by8_sqrthalf (i+7, N, table+7);
}
#endif
			table += 64;
		}
		N = N / 8;
	}

	return (table);
}

double *zr4_build_pass2_real_table (
	gwhandle *gwdata,	
	double	*table)		
{
	unsigned long avx512_increment, j, N;

	if (gwdata->NEGACYCLIC_FFT) return (table);

	avx512_increment = 1;
	N = gwdata->PASS2_SIZE;

	if (gwdata->PASS2_SIZE != 640 && gwdata->PASS2_SIZE != 4480 && gwdata->PASS2_SIZE != 6400 &&
	    gwdata->PASS2_SIZE != 7680 && gwdata->PASS2_SIZE != 10240) {
		while (N % 5 == 0) {
			for (j = 0; j < N / 5; j += 8) {
				gwsincos13by8 (j, N*2, table);
				gwsincos13by8 (j + avx512_increment, N*2, table+1);
				gwsincos13by8 (j + 2*avx512_increment, N*2, table+2);
				gwsincos13by8 (j + 3*avx512_increment, N*2, table+3);
				gwsincos13by8 (j + 4*avx512_increment, N*2, table+4);
				gwsincos13by8 (j + 5*avx512_increment, N*2, table+5);
				gwsincos13by8 (j + 6*avx512_increment, N*2, table+6);
				gwsincos13by8 (j + 7*avx512_increment, N*2, table+7);
				table += 32;
			}
			N = N / 5;
			if (N == 10*64) break;
		}
	}

	while (N % 7 == 0) {
		for (j = 0; j < N / 7; j += 8) {
			gwsincos135by8_special7 (j, N*2, table);
			gwsincos135by8_special7 (j + avx512_increment, N*2, table+1);
			gwsincos135by8_special7 (j + 2*avx512_increment, N*2, table+2);
			gwsincos135by8_special7 (j + 3*avx512_increment, N*2, table+3);
			gwsincos135by8_special7 (j + 4*avx512_increment, N*2, table+4);
			gwsincos135by8_special7 (j + 5*avx512_increment, N*2, table+5);
			gwsincos135by8_special7 (j + 6*avx512_increment, N*2, table+6);
			gwsincos135by8_special7 (j + 7*avx512_increment, N*2, table+7);
			table += 48;
		}
		N = N / 7;
	}

	while (N % 10 == 0) {
		for (j = 0; j < N / 10; j += 8) {
			gwsincos13579by8 (j, N*2, table);
			gwsincos13579by8 (j + avx512_increment, N*2, table+1);
			gwsincos13579by8 (j + 2*avx512_increment, N*2, table+2);
			gwsincos13579by8 (j + 3*avx512_increment, N*2, table+3);
			gwsincos13579by8 (j + 4*avx512_increment, N*2, table+4);
			gwsincos13579by8 (j + 5*avx512_increment, N*2, table+5);
			gwsincos13579by8 (j + 6*avx512_increment, N*2, table+6);
			gwsincos13579by8 (j + 7*avx512_increment, N*2, table+7);
			table += 80;
		}
		N = N / 10;
	}

	while (N == 12*64 || N == 12*6*64 || N == 12*8*64 || N == 12*12*64 || N == 12*16*64) {
		for (j = 0; j < N / 12; j += 8) {
			gwsincos13579Bby8 (j, N*2, table);
			gwsincos13579Bby8 (j + avx512_increment, N*2, table+1);
			gwsincos13579Bby8 (j + 2*avx512_increment, N*2, table+2);
			gwsincos13579Bby8 (j + 3*avx512_increment, N*2, table+3);
			gwsincos13579Bby8 (j + 4*avx512_increment, N*2, table+4);
			gwsincos13579Bby8 (j + 5*avx512_increment, N*2, table+5);
			gwsincos13579Bby8 (j + 6*avx512_increment, N*2, table+6);
			gwsincos13579Bby8 (j + 7*avx512_increment, N*2, table+7);
			table += 96;
		}
		N = N / 12;
	}

	while (N % 6 == 0) {
		for (j = 0; j < N / 6; j += 8) {
			gwsincos135by8 (j, N*2, table);
			gwsincos135by8 (j + avx512_increment, N*2, table+1);
			gwsincos135by8 (j + 2*avx512_increment, N*2, table+2);
			gwsincos135by8 (j + 3*avx512_increment, N*2, table+3);
			gwsincos135by8 (j + 4*avx512_increment, N*2, table+4);
			gwsincos135by8 (j + 5*avx512_increment, N*2, table+5);
			gwsincos135by8 (j + 6*avx512_increment, N*2, table+6);
			gwsincos135by8 (j + 7*avx512_increment, N*2, table+7);
			table += 48;
		}
		N = N / 6;
	}

	while (N == 16*64 || N == 16*8*64 || N == 16*16*64) {
		for (j = 0; j < N / 16; j += 8) {
			gwsincos13579BDFby8 (j, N*2, table);
			gwsincos13579BDFby8 (j + avx512_increment, N*2, table+1);
			gwsincos13579BDFby8 (j + 2*avx512_increment, N*2, table+2);
			gwsincos13579BDFby8 (j + 3*avx512_increment, N*2, table+3);
			gwsincos13579BDFby8 (j + 4*avx512_increment, N*2, table+4);
			gwsincos13579BDFby8 (j + 5*avx512_increment, N*2, table+5);
			gwsincos13579BDFby8 (j + 6*avx512_increment, N*2, table+6);
			gwsincos13579BDFby8 (j + 7*avx512_increment, N*2, table+7);
			table += 128;
		}
		N = N / 16;
	}

	while (N > 8) {
		for (j = 0; j < N / 8; j += 8) {
			gwsincos1357by8 (j, N*2, table);
			gwsincos1357by8 (j + avx512_increment, N*2, table+1);
			gwsincos1357by8 (j + 2*avx512_increment, N*2, table+2);
			gwsincos1357by8 (j + 3*avx512_increment, N*2, table+3);
			gwsincos1357by8 (j + 4*avx512_increment, N*2, table+4);
			gwsincos1357by8 (j + 5*avx512_increment, N*2, table+5);
			gwsincos1357by8 (j + 6*avx512_increment, N*2, table+6);
			gwsincos1357by8 (j + 7*avx512_increment, N*2, table+7);
			table += 64;
		}
		N = N / 8;
	}

	return (table);
}

double *zr4dwpn_build_biglit_table (
	gwhandle *gwdata,	
	double	*table)		
{
	struct gwasm_data *asm_data = (struct gwasm_data *) gwdata->asm_data;
	unsigned char *p, *temp_biglit_table;
	unsigned long upper_avx512_word, group, h, i, j, k, num_combos, temp_biglit_table_size, varblock_biglit_data_size;
	uint64_t combos[129];		
	int	next_combo[129];
	uint64_t *output_ptr;		
const	int	START_OF_CHAIN = 0x8000;
const	int	END_OF_CHAIN = 0x4000;

	memset (asm_data->u.zmm.ZMM_FIRST_BIGLIT_VALUES, 0, sizeof (asm_data->u.zmm.ZMM_FIRST_BIGLIT_VALUES));

	if (gwdata->RATIONAL_FFT) return (table);

	if (gwdata->PASS1_SIZE == 0) {
		upper_avx512_word = 8;
		
		varblock_biglit_data_size = gwdata->FFTLEN / 64;
	} else {
		upper_avx512_word = gwdata->PASS2_SIZE;
		
		varblock_biglit_data_size = gwdata->PASS1_SIZE * gwdata->PASS1_CACHE_LINES / 64;
	}
	if (gwdata->ZERO_PADDED_FFT) varblock_biglit_data_size = varblock_biglit_data_size / 2;

	temp_biglit_table_size = gwdata->FFTLEN / 8;
	if (gwdata->ZERO_PADDED_FFT) temp_biglit_table_size = temp_biglit_table_size / 2;
	p = temp_biglit_table = (unsigned char *) malloc (temp_biglit_table_size);
	for (group = 0; group < upper_avx512_word; group += gwdata->PASS1_CACHE_LINES) {
	    for (h = 0; h < gwdata->FFTLEN / 2; h += 4 * 8 * upper_avx512_word) {
		for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
		    for (j = 0; j < 4 * 8 * upper_avx512_word; j += 8 * upper_avx512_word) {
			for (k = 0; k < gwdata->FFTLEN; k += gwdata->FFTLEN / 2) {
				unsigned long word;

				if (k && gwdata->ZERO_PADDED_FFT) continue;

				word = group + h + i + j + k;
				*p = is_big_word (gwdata, word);
				if (is_big_word (gwdata, word + upper_avx512_word)) *p += 2;
				if (is_big_word (gwdata, word + 2 * upper_avx512_word)) *p += 4;
				if (is_big_word (gwdata, word + 3 * upper_avx512_word)) *p += 8;
				if (is_big_word (gwdata, word + 4 * upper_avx512_word)) *p += 16;
				if (is_big_word (gwdata, word + 5 * upper_avx512_word)) *p += 32;
				if (is_big_word (gwdata, word + 6 * upper_avx512_word)) *p += 64;
				if (is_big_word (gwdata, word + 7 * upper_avx512_word)) *p += 128;

				if (word < sizeof (asm_data->u.zmm.ZMM_FIRST_BIGLIT_VALUES) * 8 && (*p & 1))
					bitset (asm_data->u.zmm.ZMM_FIRST_BIGLIT_VALUES, word);

				p++;
			}
		    }
		}
	    }
	}

	p = temp_biglit_table;
	num_combos = 0;
	for (i = 0; i < temp_biglit_table_size; i += 8) {
		uint64_t combo = * (uint64_t *) (p+i);
		
		for (j = 0; ; j++) {
			if (j == num_combos) {
				combos[num_combos++] = combo;
				break;
			}
			if (combo == combos[j]) break;
		}
		
		p[i] = (unsigned char) j;
	}

	for (i = 0; i < num_combos; i++) next_combo[i] = START_OF_CHAIN + END_OF_CHAIN;
	
	for (i = 0; i < num_combos; i++) {
		int	chain_end;
		
		if (! (next_combo[i] & START_OF_CHAIN)) continue;
		
		for (chain_end = i; ! (next_combo[chain_end] & END_OF_CHAIN); chain_end = next_combo[chain_end] & 0xFF);
		
		for (j = 0; j < num_combos; j++) {
			
			if (! (next_combo[j] & END_OF_CHAIN)) continue;
			
			if (j == chain_end) continue;
			
			if ((combos[j] >> 32) == (combos[i] & 0xFFFFFFFF)) {
				next_combo[j] = (next_combo[j] & START_OF_CHAIN) + i;
				next_combo[i] &= ~START_OF_CHAIN;
				break;
			}
		}
	}

	asm_data->compressed_biglits = table;
	table = (double *) (((char *) table) + 768);
	output_ptr = (uint64_t *) asm_data->compressed_biglits;
	for (i = 0; i < num_combos; i++) {
		
		if (! (next_combo[i] & START_OF_CHAIN)) continue;
		
		for (j = i; ; j = next_combo[j] & 0xFFF) {
			
			*output_ptr = combos[j];
			
			combos[j] = ((char *) output_ptr - (char *) asm_data->compressed_biglits) / sizeof (uint32_t);
			
			output_ptr++;
			
			if (next_combo[j] & END_OF_CHAIN) break;
			
			output_ptr = (uint64_t *) (((char *) output_ptr) - sizeof (uint32_t));
		}
	}
	ASSERTG ((char *) output_ptr <= (char *) table);

	p = (unsigned char *) gwdata->pass1_var_data + gwdata->biglit_data_offset;
	
	i = 0;
	
	for (group = 0; group < upper_avx512_word; group += gwdata->PASS1_CACHE_LINES) {
		for (k = 0; k < varblock_biglit_data_size; k++) {
			
			j = temp_biglit_table[i];  i += 8;
			
			*p++ = (unsigned char) combos[j];
		}
		p += gwdata->pass1_var_data_size - varblock_biglit_data_size;
	}

	free (temp_biglit_table);

	return (table);
}

double *zr4dwpn_build_norm_table (
	gwhandle *gwdata,	
	double	*table)		
{
	unsigned long delay_count, i, j, k, avx512_word, upper_avx512_word, grp;
	double	*weights, *inverse_weights;

	if (gwdata->PASS1_SIZE == 0) {
		delay_count = 1;
		upper_avx512_word = 8;
	} else {
		delay_count = zr4dwpn_delay_count (gwdata);
		upper_avx512_word = gwdata->PASS2_SIZE;
	}

	if (table != NULL) {	
		weights = table;
		table = table + 2 * gwdata->FFTLEN / upper_avx512_word;
	} else {		
		struct gwasm_data *asm_data = (struct gwasm_data *) gwdata->asm_data;
		weights = (double *) asm_data->norm_grp_mults;
	}
	inverse_weights = weights + gwdata->FFTLEN / upper_avx512_word;

	if (table != NULL) {
	    for (i = 0; i < gwdata->FFTLEN / 2 / delay_count; i += 8 * upper_avx512_word) {
		for (j = 0; j < gwdata->FFTLEN / 2; j += gwdata->FFTLEN / 2 / delay_count) {
		    for (k = 0; k < gwdata->FFTLEN; k += gwdata->FFTLEN / 2) {
			for (avx512_word = 0; avx512_word < 8 * upper_avx512_word; avx512_word += upper_avx512_word) {

				grp = i + j + k + avx512_word;
				*weights++ = gwfft_weight_no_c (gwdata->dd_data, grp);
			}
		    }
		}
	    }
	}

	else {
	    if (gwdata->PASS1_SIZE == 0 || (gwdata->PASS1_SIZE && gwdata->NEGACYCLIC_FFT) || labs (gwdata->c) != 1 || delay_count == 8 || delay_count == 12)
	    for (i = 0; i < gwdata->FFTLEN / 2 / delay_count; i += 8 * upper_avx512_word) {
		for (j = 0; j < gwdata->FFTLEN / 2; j += gwdata->FFTLEN / 2 / delay_count) {
		    for (k = 0; k < gwdata->FFTLEN; k += gwdata->FFTLEN / 2) {
			for (avx512_word = 0; avx512_word < 8 * upper_avx512_word; avx512_word += upper_avx512_word) {
				double	ttp;
				unsigned long sine_word;

				grp = i + j + k + avx512_word;
				if (gwdata->PASS1_SIZE && gwdata->NEGACYCLIC_FFT) {
					
					if ((delay_count == 8 && k != 0 && j <= 3 * (gwdata->FFTLEN / 2 / delay_count)) ||
					    (delay_count == 12 && k != 0 && j <= 5 * (gwdata->FFTLEN / 2 / delay_count)))
						ttp = gwfft_weight_over_weight (gwdata->dd_data, grp, grp - k);
					
					else {
						sine_word = i + j + avx512_word;
						gwfft_weights_times_sine (gwdata->dd_data, grp, sine_word, gwdata->FFTLEN * 2, &ttp, NULL);
					}
				}
				else if ((delay_count == 8 && k != 0 && j != 0 && j <= 3 * (gwdata->FFTLEN / 2 / delay_count)) ||
					 (delay_count == 12 && k != 0 && j != 0 && j <= 5 * (gwdata->FFTLEN / 2 / delay_count)))
					ttp = gwfft_weight_over_weight (gwdata->dd_data, grp, grp - k);
				else
					ttp = gwfft_weight (gwdata->dd_data, grp);
				*weights++ = ttp;
			}
		    }
		}
	    }

	    for (i = 0; i < gwdata->FFTLEN / 2; i += 8 * upper_avx512_word) {
		for (k = 0; k < gwdata->FFTLEN; k += gwdata->FFTLEN / 2) {
		    for (avx512_word = 0; avx512_word < 8 * upper_avx512_word; avx512_word += upper_avx512_word) {
			double	ttmp;

			if ((gwdata->ZERO_PADDED_FFT || gwdata->RATIONAL_FFT) && k) continue;

			grp = i + k + avx512_word;
			if (gwdata->PASS1_SIZE && gwdata->NEGACYCLIC_FFT) {
				
				unsigned long sine_word = i + avx512_word;
				gwfft_weights_times_sine (gwdata->dd_data, grp, sine_word, gwdata->FFTLEN * 2, NULL, &ttmp);
			} else
				ttmp = gwfft_weight_inverse (gwdata->dd_data, grp);
			*inverse_weights++ = ttmp;
		    }
		}
	    }
	}

	return (table);
}

double *zr4dwpn_build_fudge_table (
	gwhandle *gwdata,	
	double	*table)		
{
	struct gwasm_data *asm_data = (struct gwasm_data *) gwdata->asm_data;
	uint64_t *compressed_fudges, *xor_masks, mask, mask_bit;
	unsigned long num_grp_mults, delay_count, upper_avx512_word, num_fudges, varblock_fudge_data_size;
	unsigned long group, h, i, j, k, avx512_word;
	unsigned char *p;

	if (gwdata->PASS1_SIZE == 0) {
		delay_count = 1;
		upper_avx512_word = 8;
		
		varblock_fudge_data_size = gwdata->FFTLEN / 8 * 7 / 8;
	} else {
		delay_count = zr4dwpn_delay_count (gwdata);
		upper_avx512_word = gwdata->PASS2_SIZE;
		num_grp_mults = gwdata->PASS1_SIZE;
		
		varblock_fudge_data_size = num_grp_mults * gwdata->PASS1_CACHE_LINES / 64;
	}

	if (gwdata->PASS1_SIZE) {
		double	*norm_grp_mults;

		ASSERTG (num_grp_mults <= 4096);  
		asm_data->compressed_fudges = table;
		table = (double *) ((char *) table + 192 * sizeof (uint64_t));

		compressed_fudges = (uint64_t *) asm_data->compressed_fudges;
		memset (compressed_fudges, 0, num_grp_mults / 8);	
		norm_grp_mults = (double *) asm_data->norm_grp_mults;
		for (i = 0; i < num_grp_mults; i++) if (norm_grp_mults[i] < norm_grp_mults[i&63]) bitset (compressed_fudges, i);
		num_fudges = num_grp_mults / 64;
	}

	p = (unsigned char *) gwdata->pass1_var_data + gwdata->biglit_data_offset - varblock_fudge_data_size;

	mask = 0;
	mask_bit = 1;
	for (group = 0; group < upper_avx512_word; group += gwdata->PASS1_CACHE_LINES) {
	    for (h = 0, xor_masks = compressed_fudges;
		 h < gwdata->FFTLEN / 2 / delay_count;
		 h += 8 * upper_avx512_word, xor_masks += delay_count / 4) {
		for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++, xor_masks -= delay_count / 4) {
		    unsigned long col;
		    double col_weight_exponent;
		    col = group + i;
		    col_weight_exponent = gwfft_weight_exponent (gwdata->dd_data, col);
		    
		    for (j = 0; j < gwdata->FFTLEN / 2; j += gwdata->FFTLEN / 2 / delay_count) {
			for (k = 0; k < gwdata->FFTLEN; k += gwdata->FFTLEN / 2) {
			    for (avx512_word = 0; avx512_word < 8 * upper_avx512_word; avx512_word += upper_avx512_word) { 
				unsigned long grp;
				double	grp_weight_exponent;
				grp = h + j + k + avx512_word;
				grp_weight_exponent = gwfft_weight_exponent (gwdata->dd_data, grp);
				if (gwfft_weight_exponent (gwdata->dd_data, grp + col) + 0.5 < grp_weight_exponent + col_weight_exponent)
					mask |= mask_bit;
				mask_bit <<= 1;
			    }
			    
			    if (gwdata->PASS1_SIZE == 0) {
				
				if (col != 0) *p++ = (unsigned char) mask;
			    }
			    
			    else {
				int	index;
				if (mask_bit) continue;	
				
				mask ^= *xor_masks++;
				
				for (index = 0; ; index++) {
					if (index == num_fudges) {
						compressed_fudges[num_fudges++] = mask;
						ASSERTG (num_fudges < 192);
						break;
					}
					if (compressed_fudges[index] == mask) break;
				}
				
				*p++ = (unsigned char) index;
			    }
			    
			    mask = 0;
			    mask_bit = 1;
			}
		    }
		}
	    }
	    
	    p += gwdata->pass1_var_data_size - varblock_fudge_data_size;
	}

	(void) zr4dwpn_build_norm_table (gwdata, NULL);

	return (table);
}

double *yr4_build_onepass_sincos_table (
	gwhandle *gwdata,	
	double	*table)		
{
	unsigned long size, avx_increment, j, N, temp;
	int	pow2_count;

	size = gwdata->FFTLEN / 8;
	for (pow2_count = 0; (size & 1) == 0; size /= 2) pow2_count++;

	size = gwdata->FFTLEN / 8;		
	avx_increment = gwdata->FFTLEN / 4;

	if (pow2_count & 1) {
		N = 8;
		size /= 8;
	} else {
		N = 4;
		size /= 4;
	}

	if ((size & 3) == 0) {
		while ((size & 3) == 0) {
			N = N * 4;
			size /= 4;
		}

		if (!gwdata->NEGACYCLIC_FFT) {
			for (j = 0; j < N / 4; j++) {
				gwsincos125by4 (j, N*2, table);				
				gwsincos12by4 (j + avx_increment, N, table+1);		
				table[3] = table[2] = table[1];
				table[7] = table[6] = table[5];
				table[11] = table[10] = table[9];
				table[15] = table[14] = table[13];
				table[19] = table[18] = table[17] = -table[1];
				table[23] = table[22] = table[21] = -table[5];
				table += 24;
			}
		}

		else {
			for (j = 0; j < N / 4; j++) {
				gwsincos12by1 (j, N, table);
				table += 4;
			}
		}
	}

	while (size % 5 == 0) {
		while ((size % 5) == 0) {
			N = N * 5;
			size /= 5;
		}

		if (!gwdata->NEGACYCLIC_FFT) {
			for (j = 0; j < N / 5; j++) {
				gwsincos13by4 (j, N*2, table);				
				gwsincos12by4 (j + avx_increment, N, table+1);		
				table[16] = table[8];					
				table[20] = table[12];
				table[8] = table[1];
				table[12] = table[5];
				table[24] = table[9];
				table[28] = table[13];
				table[3] = table[2] = table[1];				
				table[7] = table[6] = table[5];
				table[11] = table[10] = table[9];
				table[15] = table[14] = table[13];
				table[19] = table[18] = table[17] = -table[9];
				table[23] = table[22] = table[21] = -table[13];
				table[27] = table[26] = table[25] = -table[1];
				table[31] = table[30] = table[29] = -table[5];
				table += 32;
			}
		}

		else {
			for (j = 0; j < N / 5; j++) {
				gwsincos12by1 (j, N, table);
				table += 4;
			}
		}
	}

	while (size % 3 == 0) {
		while ((size % 3) == 0) {
			N = N * 3;
			size /= 3;
		}

		if (!gwdata->NEGACYCLIC_FFT) {
			for (j = 0; j < N / 3; j++) {
				gwsincos12by4 (j, N*2, table);				
				gwsincos1by4 (j + avx_increment, N, table+1);		
				table[3] = table[2] = table[1];
				table[7] = table[6] = table[5];
				table[11] = table[10] = table[9] = -table[1];
				table[15] = table[14] = table[13] = -table[5];
				table += 16;
			}
		}

		else {
			for (j = 0; j < N / 3; j++) {
				gwsincos1by1 (j, N, table);
				table += 2;
			}
		}
	}
	ASSERTG (size == 1);
	if (size != 1) gwdata->GWERROR = GWERROR_INTERNAL + 1;

	avx_increment = gwdata->FFTLEN / 32;
	if (!gwdata->NEGACYCLIC_FFT) {
		N = gwdata->FFTLEN;
		for (j = 0; j < avx_increment; j++) {
			gwsincos125by4 (j, N, table);
			gwsincos125by4 (j + avx_increment, N, table+1);
			gwsincos125by4 (j + 2*avx_increment, N, table+2);
			gwsincos125by4 (j + 3*avx_increment, N, table+3);
			table += 24;
		}
	}

	else {
		N = gwdata->FFTLEN / 2;
		for (j = 0; j < avx_increment; j++) {

			temp = j;
			gwsincos1plus0123by4 (temp, 4 * temp, N * 4, table); 
			temp += avx_increment;
			gwsincos1plus0123by4 (temp, 4 * temp, N * 4, table+1);
			temp += avx_increment;
			gwsincos1plus0123by4 (temp, 4 * temp, N * 4, table+2);
			temp += avx_increment;
			gwsincos1plus0123by4 (temp, 4 * temp, N * 4, table+3);
			table += 32;
		}
	}

	return (table);
}

double *yr4_build_onepass_biglit_table (
	gwhandle *gwdata,	
	double	*table)		
{
	struct gwasm_data *asm_data = (struct gwasm_data *) gwdata->asm_data;
	unsigned char *p = (unsigned char *) table;
	unsigned long i, j, top5bits;

	memset (asm_data->u.ymm.YMM_FIRST_BIGLIT_VALUES, 0, sizeof (asm_data->u.ymm.YMM_FIRST_BIGLIT_VALUES));

	if (gwdata->ZERO_PADDED_FFT) {
		memset (p, 0, gwdata->FFTLEN / 8);
		for (i = 0; i < gwdata->FFTLEN / 2; i++) {

			if (! is_big_word (gwdata, i)) continue;

			top5bits = i / (gwdata->FFTLEN >> 5); j = i - top5bits * (gwdata->FFTLEN >> 5);
			j = ((top5bits >> 2) & 3) * (gwdata->FFTLEN >> 2) + (j << 3) + ((top5bits >> 4) << 2) + (top5bits & 3);

			p[j >> 3] += 16 << (j & 3);

			if (i < sizeof (asm_data->u.ymm.YMM_FIRST_BIGLIT_VALUES))
				asm_data->u.ymm.YMM_FIRST_BIGLIT_VALUES[i] = 16;
		}

		return ((double *) (p + gwdata->FFTLEN / 8));
	}

	memset (p, 0, gwdata->FFTLEN / 4);
	for (i = 0; i < gwdata->FFTLEN; i++) {

		if (! is_big_word (gwdata, i)) continue;

		top5bits = i / (gwdata->FFTLEN >> 5); j = i - top5bits * (gwdata->FFTLEN >> 5);
		j = ((top5bits >> 2) & 3) * (gwdata->FFTLEN >> 2) + (j << 3) + ((top5bits >> 4) << 2) + (top5bits & 3);

		p[j >> 2] += 16 << (j & 3);

		if (i < sizeof (asm_data->u.ymm.YMM_FIRST_BIGLIT_VALUES))
			asm_data->u.ymm.YMM_FIRST_BIGLIT_VALUES[i] = 16;
	}

	return ((double *) (p + gwdata->FFTLEN / 4));
}

double *yr4_build_onepass_norm_table (
	gwhandle *gwdata,	
	double	*table)		
{
	struct gwasm_data *asm_data = (struct gwasm_data *) gwdata->asm_data;
	unsigned long i;
	intptr_t first_8_offsets[8];

	if (gwdata->ZERO_PADDED_FFT) {
		for (i = 0; i < gwdata->FFTLEN / 2; i++) {
			double	ttp, ttmp;
			unsigned long top5bits, j, table_entry;

			gwfft_weights3 (gwdata->dd_data, i, &ttp, NULL, &ttmp);

			top5bits = i / (gwdata->FFTLEN >> 5); j = i - top5bits * (gwdata->FFTLEN >> 5);
			j = ((top5bits >> 2) & 3) * (gwdata->FFTLEN >> 2) + (j << 3) + ((top5bits >> 4) << 2) + (top5bits & 3);
			table_entry = j >> 3;

			table[table_entry*8+(j&3)] = ttmp;
			table[table_entry*8+4+(j&3)] = ttp;

			if (i <= 7) first_8_offsets[i] = (&table[table_entry*8+(j&3)] - table) * sizeof (double);
		}

		table += gwdata->FFTLEN;
	}

	else {
		for (i = 0; i < gwdata->FFTLEN; i++) {
			double	ttp, ttmp;
			unsigned long top5bits, j, table_entry;

			gwfft_weights3 (gwdata->dd_data, i, &ttp, NULL, &ttmp);

			top5bits = i / (gwdata->FFTLEN >> 5); j = i - top5bits * (gwdata->FFTLEN >> 5);
			j = ((top5bits >> 2) & 3) * (gwdata->FFTLEN >> 2) + (j << 3) + ((top5bits >> 4) << 2) + (top5bits & 3);
			table_entry = j >> 2;

			table[table_entry*8+(j&3)] = ttmp;
			table[table_entry*8+4+(j&3)] = ttp;

			if (i <= 7) first_8_offsets[i] = (&table[table_entry*8+(j&3)] - table) * sizeof (double);
		}

		table += gwdata->FFTLEN * 2;
	}

	asm_data->u.ymm.YMM_NORM_INCR7 = first_8_offsets[7] - first_8_offsets[6];
	asm_data->u.ymm.YMM_NORM_INCR6 = first_8_offsets[6] - first_8_offsets[5];
	asm_data->u.ymm.YMM_NORM_INCR5 = first_8_offsets[5] - first_8_offsets[4];
	asm_data->u.ymm.YMM_NORM_INCR4 = first_8_offsets[4] - first_8_offsets[3];
	asm_data->u.ymm.YMM_NORM_INCR3 = first_8_offsets[3] - first_8_offsets[2];
	asm_data->u.ymm.YMM_NORM_INCR2 = first_8_offsets[2] - first_8_offsets[1];
	asm_data->u.ymm.YMM_NORM_INCR1 = first_8_offsets[1] - first_8_offsets[0];

	return (table);
}

double *yr4dwpn_build_pass1_table (
	gwhandle *gwdata,	
	double	*table)		
{
	struct gwasm_data *asm_data = (struct gwasm_data *) gwdata->asm_data;
	unsigned long pass1_size, pass1_increment, delay_count;
	unsigned long group, i, j, k, N, temp, upper_avx_word;
	int	pow2_count;
	int	wpn4 = FALSE;		

	pass1_size = gwdata->PASS1_SIZE;
	upper_avx_word = gwdata->PASS2_SIZE;
	pass1_increment = gwdata->PASS2_SIZE * 4;

#ifdef USE_REDUCED_SINCOS_FFTS
	if (pass1_size == 1792 && !gwdata->NEGACYCLIC_FFT)
		delay_count = 56;
	else if (pass1_size % 7 == 0)
		delay_count = 14;
	else if (pass1_size == 384 || pass1_size == 768)
		delay_count = 12;
	else if ((pass1_size == 640 && gwdata->NEGACYCLIC_FFT) ||
		 (pass1_size == 1280 && gwdata->NEGACYCLIC_FFT))
		delay_count = 20;
	else if (pass1_size == 1280 && !gwdata->NEGACYCLIC_FFT)
		delay_count = 40;
	else if (pass1_size == 1536 && !gwdata->NEGACYCLIC_FFT)
		delay_count = 48;
	else if (pass1_size % 5 == 0 && !gwdata->NEGACYCLIC_FFT)
		delay_count = 10;
	else if ((pass1_size == 256 && !gwdata->NEGACYCLIC_FFT) ||
		 (pass1_size == 512 && !gwdata->NEGACYCLIC_FFT))
		delay_count = 8;
	else if ((pass1_size == 512 && gwdata->NEGACYCLIC_FFT) ||
		 (pass1_size == 1536 && gwdata->NEGACYCLIC_FFT) ||
		 pass1_size == 1024 ||
		 pass1_size == 2048)
		delay_count = 16;
	else
		delay_count = 4;
					
#else
	if (pass1_size % 7 == 0)
		delay_count = 14;
	else if ((pass1_size == 384 && !gwdata->NEGACYCLIC_FFT) ||
		 (pass1_size == 768 && !gwdata->NEGACYCLIC_FFT) ||
		 (pass1_size == 1536 && !gwdata->NEGACYCLIC_FFT))
		delay_count = 12;
	else if (pass1_size % 5 == 0 && !gwdata->NEGACYCLIC_FFT)
		delay_count = 10;
	else if (pass1_size == 256 && !gwdata->NEGACYCLIC_FFT)
		delay_count = 8;
	else if ((pass1_size == 512 && !gwdata->NEGACYCLIC_FFT) ||
		 (pass1_size == 1024 && !gwdata->NEGACYCLIC_FFT) ||
		 (pass1_size == 1536 && gwdata->NEGACYCLIC_FFT) ||
		 pass1_size == 2048)
		delay_count = 16;
	else
		delay_count = 4;
#endif

	pass1_size /= (delay_count * 2);
	for (pow2_count = 0; (pass1_size & 1) == 0; pass1_size /= 2) pow2_count++;

#ifdef USE_REDUCED_SINCOS_FFTS
	if (pow2_count & 1) gwdata->wpn_count = 8;
	else if (gwdata->PASS1_SIZE == 2048) gwdata->wpn_count = 16;
	else gwdata->wpn_count = 4;
#else
	if (pow2_count & 1) gwdata->wpn_count = 8;
	else if ((gwdata->PASS1_SIZE == 1536 && !gwdata->NEGACYCLIC_FFT) ||
		 gwdata->PASS1_SIZE == 1792 ||
		 gwdata->PASS1_SIZE == 2048) gwdata->wpn_count = 16;
	else gwdata->wpn_count = 4;
#endif
#ifdef USE_WPN4
	{
		gwdata->wpn_count *= 4;
		wpn4 = TRUE;
	}
#endif

	asm_data->count2 = gwdata->wpn_count / 4;
	asm_data->count3 = asm_data->addcount1 / asm_data->count2;
	if (asm_data->count2 == 1) {
		asm_data->count4 = 1;
		asm_data->count5 = asm_data->count3 / 2;
	} else {
		asm_data->count4 = asm_data->count2 / 2;
		asm_data->count5 = asm_data->count3;
	}

	gwdata->pass1_var_data = table;

	for (group = 0; group < upper_avx_word; group += gwdata->PASS1_CACHE_LINES) {

		pass1_size = gwdata->PASS1_SIZE;
		pass1_size /= (delay_count * 2);	
		N = gwdata->PASS2_SIZE;

#ifdef USE_REDUCED_SINCOS_FFTS
		if (pow2_count & 1) {
			N = N * 8;

			for (i = 0; i < gwdata->PASS1_CACHE_LINES; i += 4) {
				
				temp = group + i;
				gwsincos1234by4_raw (temp, N, table);
				gwsincos1234by4_raw (temp + 1, N, table+1);
				gwsincos1234by4_raw (temp + 2, N, table+2);
				gwsincos1234by4_raw (temp + 3, N, table+3);
				table += 32;
			}

			if (!gwdata->NEGACYCLIC_FFT) {
				for (i = 0; i < gwdata->PASS1_CACHE_LINES; i += 4) {
					
					temp = group + i;
					gwsincos159Dby4 (temp, N*2, table);
					gwsincos159Dby4 (temp + 1, N*2, table+1);
					gwsincos159Dby4 (temp + 2, N*2, table+2);
					gwsincos159Dby4 (temp + 3, N*2, table+3);
					table += 32;
				}
			}

			for (k = 0; k < delay_count; k++) {
				if (k == 0 && !gwdata->NEGACYCLIC_FFT) continue;
				for (i = 0; i < gwdata->PASS1_CACHE_LINES; i += 4) {
					unsigned long bigN, ktemp, actemp, avx_word;

					for (avx_word = 0; avx_word < 4; avx_word++) {
						unsigned long final_group = group + i + avx_word;

						if (gwdata->NEGACYCLIC_FFT) {
							bigN = gwdata->FFTLEN * 2;
							actemp = final_group;
						} else {
							bigN = gwdata->FFTLEN;
							actemp = 0;
						}

						if (gwdata->NEGACYCLIC_FFT && delay_count == 4) {
							if (k == 0)
								ktemp = 0;
							else if (k == 1)
								ktemp = 2 * final_group * 4;
							else if (k == 2)
								ktemp = 1 * final_group * 4;
							else
								ktemp = bigN - 1 * final_group * 4;
						} else if (gwdata->NEGACYCLIC_FFT && delay_count == 12) {
							
							int	kmap[12] = {0,4,-4, 2,6,-2, 1,5,-3, -1,3,-5};
							if (kmap[k] >= 0)
								ktemp = kmap[k] * final_group * 4;
							else
								ktemp = bigN + kmap[k] * final_group * 4;
						} else if (gwdata->NEGACYCLIC_FFT && delay_count == 20) {
							
							int	kmap[20] = {0,4,8,-8,-4, 2,6,10,-6,-2, 1,5,9,-7,-3, -1,3,7,-9,-5};
							if (kmap[k] >= 0)
								ktemp = kmap[k] * final_group * 4;
							else
								ktemp = bigN + kmap[k] * final_group * 4;
						} else if (gwdata->NEGACYCLIC_FFT) {
							
							int	kmap[16] = {0,8,4,-4, 2,10,6,-2, 1,9,5,-3, -1,7,3,-5};
							if (kmap[k] >= 0)
								ktemp = kmap[k] * final_group * 4;
							else
								ktemp = bigN + kmap[k] * final_group * 4;
						} else if (delay_count == 4) {
							if (k == 0)
								ktemp = 0;
							else if (k == 1)
								ktemp = 2 * final_group;
							else if (k == 2)
								ktemp = 1 * final_group;
							else
								ktemp = 5 * final_group;
						} else if (delay_count == 16) {
							
							int	kmap[16] = {0,8,4,20, 2,18,10,-6, 1,17,9,-7, 5,21,13,-3};
							if (kmap[k] >= 0)
								ktemp = kmap[k] * final_group;
							else
								ktemp = bigN + kmap[k] * final_group;
						} else if (delay_count == 32) {
							
							int	kmap[32] = {0,16,8,40,    1,33,17,-15,  2,34,18,-14,
									    3,35,19,-13,  4,36,20,-12,  5,37,21,-11,
									    6,38,22,-10,  7,39,23,-9};
							if (kmap[k] >= 0)
								ktemp = kmap[k] * final_group;
							else
								ktemp = bigN + kmap[k] * final_group;
						} else if (delay_count == 40) {
							
							int	kmap[40] = {0,20,10,50,   1,41,21,-19,  2,42,22,-18,
									    3,43,23,-17,  4,44,24,-16,  5,45,25,-15,
									    6,46,26,-14,  7,47,27,-13,  8,48,28,-12,  9,49,29,-11};
							if (kmap[k] >= 0)
								ktemp = kmap[k] * final_group;
							else
								ktemp = bigN + kmap[k] * final_group;
						} else if (delay_count == 48) {
							
							int	kmap[48] = {0,24,12,60,   1,49,25,-23,  2,50,26,-22,
									    3,51,27,-21,  4,52,28,-20,  5,53,29,-19,
									    6,54,30,-18,  7,55,31,-17,  8,56,32,-16,
									    9,57,33,-15, 10,58,34,-14, 11,59,35,-13};
							if (kmap[k] >= 0)
								ktemp = kmap[k] * final_group;
							else
								ktemp = bigN + kmap[k] * final_group;
						} else if (delay_count == 56) {
							
							int	kmap[56] = {0,28,14,70,   1,57,29,-27,  2,58,30,-26,
									    3,59,31,-25,  4,60,32,-24,  5,61,33,-23,
									    6,62,34,-22,  7,63,35,-21,  8,64,36,-20,
									    9,65,37,-19, 10,66,38,-18, 11,67,39,-17,
									   12,68,40,-16, 13,69,41,-15};
							if (kmap[k] >= 0)
								ktemp = kmap[k] * final_group;
							else
								ktemp = bigN + kmap[k] * final_group;
						} else {			
							
							ktemp = k * final_group;
						}

						gwsincos1by4_raw (actemp + ktemp, bigN, table + avx_word);
					}
					table += 8;
				}
			}
			pass1_size /= 8;
		}

		else {
			N = N * 4;

			for (i = 0; i < gwdata->PASS1_CACHE_LINES; i += 4) {
				
				temp = group + i;
				gwsincos12by4_raw (temp, N, table);
				gwsincos12by4_raw (temp + 1, N, table+1);
				gwsincos12by4_raw (temp + 2, N, table+2);
				gwsincos12by4_raw (temp + 3, N, table+3);
				table += 16;
			}

			if (!gwdata->NEGACYCLIC_FFT) {
				for (i = 0; i < gwdata->PASS1_CACHE_LINES; i += 4) {
					
					temp = group + i;
					gwsincos15by4 (temp, N*2, table);
					gwsincos15by4 (temp + 1, N*2, table+1);
					gwsincos15by4 (temp + 2, N*2, table+2);
					gwsincos15by4 (temp + 3, N*2, table+3);
					table += 16;
				}
			}

			for (k = 0; k < delay_count; k++) {
				if (k == 0 && !gwdata->NEGACYCLIC_FFT) continue;
				for (i = 0; i < gwdata->PASS1_CACHE_LINES; i += 4) {
					unsigned long bigN, ktemp, actemp, avx_word;

					for (avx_word = 0; avx_word < 4; avx_word++) {
						unsigned long final_group = group + i + avx_word;

						if (gwdata->NEGACYCLIC_FFT) {
							bigN = gwdata->FFTLEN * 2;
							actemp = final_group;
						} else {
							bigN = gwdata->FFTLEN;
							actemp = 0;
						}

						if (gwdata->NEGACYCLIC_FFT && delay_count == 4) {
							if (k == 0)
								ktemp = 0;
							else if	(k == 1)
								ktemp = 2 * final_group * 4;
							else if (k == 2)
								ktemp = 1 * final_group * 4;
							else
								ktemp = bigN - 1 * final_group * 4;
						} else if (gwdata->NEGACYCLIC_FFT && delay_count == 12) {
							
							int	kmap[12] = {0,4,-4, 2,6,-2, 1,5,-3, -1,3,-5};
							if (kmap[k] >= 0)
								ktemp = kmap[k] * final_group * 4;
							else
								ktemp = bigN + kmap[k] * final_group * 4;
						} else if (gwdata->NEGACYCLIC_FFT && delay_count == 20) {
							
							int	kmap[20] = {0,4,8,-8,-4, 2,6,10,-6,-2, 1,5,9,-7,-3, -1,3,7,-9,-5};
							if (kmap[k] >= 0)
								ktemp = kmap[k] * final_group * 4;
							else
								ktemp = bigN + kmap[k] * final_group * 4;
						} else if (gwdata->NEGACYCLIC_FFT) {
							
							int	kmap[16] = {0,8,4,-4, 2,10,6,-2, 1,9,5,-3, -1,7,3,-5};
							if (kmap[k] >= 0)
								ktemp = kmap[k] * final_group * 4;
							else
								ktemp = bigN + kmap[k] * final_group * 4;
						} else if (delay_count == 4) {
							if (k == 0)
								ktemp = 0;
							else if	(k == 1)
								ktemp = 2 * final_group;
							else if (k == 2)
								ktemp = 1 * final_group;
							else
								ktemp = 5 * final_group;
						} else if (delay_count == 16) {
							
							int	kmap[16] = {0,8,4,20, 2,18,10,-6, 1,17,9,-7, 5,21,13,-3};
							if (kmap[k] >= 0)
								ktemp = kmap[k] * final_group;
							else
								ktemp = bigN + kmap[k] * final_group;
						} else if (delay_count == 32) {
							
							int	kmap[32] = {0,16,8,40,    1,33,17,-15,  2,34,18,-14,
									    3,35,19,-13,  4,36,20,-12,  5,37,21,-11,
									    6,38,22,-10,  7,39,23,-9};
							if (kmap[k] >= 0)
								ktemp = kmap[k] * final_group;
							else
								ktemp = bigN + kmap[k] * final_group;
						} else if (delay_count == 40) {
							
							int	kmap[40] = {0,20,10,50,   1,41,21,-19,  2,42,22,-18,
									    3,43,23,-17,  4,44,24,-16,  5,45,25,-15,
									    6,46,26,-14,  7,47,27,-13,  8,48,28,-12,  9,49,29,-11};
							if (kmap[k] >= 0)
								ktemp = kmap[k] * final_group;
							else
								ktemp = bigN + kmap[k] * final_group;
						} else if (delay_count == 48) {
							
							int	kmap[48] = {0,24,12,60,   1,49,25,-23,  2,50,26,-22,
									    3,51,27,-21,  4,52,28,-20,  5,53,29,-19,
									    6,54,30,-18,  7,55,31,-17,  8,56,32,-16,
									    9,57,33,-15, 10,58,34,-14, 11,59,35,-13};
							if (kmap[k] >= 0)
								ktemp = kmap[k] * final_group;
							else
								ktemp = bigN + kmap[k] * final_group;
						} else if (delay_count == 56) {
							
							int	kmap[56] = {0,28,14,70,   1,57,29,-27,  2,58,30,-26,
									    3,59,31,-25,  4,60,32,-24,  5,61,33,-23,
									    6,62,34,-22,  7,63,35,-21,  8,64,36,-20,
									    9,65,37,-19, 10,66,38,-18, 11,67,39,-17,
									   12,68,40,-16, 13,69,41,-15};
							if (kmap[k] >= 0)
								ktemp = kmap[k] * final_group;
							else
								ktemp = bigN + kmap[k] * final_group;
						} else {			
							
							ktemp = k * final_group;
						}

						gwsincos1by4_raw (actemp + ktemp, bigN, table + avx_word);
					}
					table += 8;
				}
			}
			pass1_size /= 4;
		}

#else
		if (pow2_count & 1) {
			N = N * 8;

			if (!gwdata->NEGACYCLIC_FFT) {
				for (i = 0; i < gwdata->PASS1_CACHE_LINES; i += 4) {
					
					temp = group + i;
					gwsincos159Dby4 (temp, N*2, table);
					gwsincos159Dby4 (temp + 1, N*2, table+1);
					gwsincos159Dby4 (temp + 2, N*2, table+2);
					gwsincos159Dby4 (temp + 3, N*2, table+3);
					table += 32;
				}
			}

			for (k = 0; k < delay_count; k++) {
				for (i = 0; i < gwdata->PASS1_CACHE_LINES; i += 4) {
					unsigned long bigN, ktemp, actemp, avx_word;

					for (avx_word = 0; avx_word < 4; avx_word++) {
						unsigned long final_group = group + i + avx_word;

						if (gwdata->NEGACYCLIC_FFT) {
							bigN = gwdata->FFTLEN * 2;
							actemp = final_group;
						} else {
							bigN = gwdata->FFTLEN;
							actemp = 0;
						}

						if (gwdata->NEGACYCLIC_FFT && delay_count == 4) {
							if (k == 0)
								ktemp = 0;
							else if (k == 1)
								ktemp = 2 * final_group * 4;
							else if (k == 2)
								ktemp = 1 * final_group * 4;
							else
								ktemp = bigN - 1 * final_group * 4;
						} else if (gwdata->NEGACYCLIC_FFT) {
							
							int	kmap[16] = {0,8,4,-4,2,10,6,-2,1,9,5,-3,-1,7,3,-5};
							if (kmap[k] >= 0)
								ktemp = kmap[k] * final_group * 4;
							else
								ktemp = bigN + kmap[k] * final_group * 4;
						} else if (delay_count == 4) {
							if (k == 0)
								ktemp = 0;
							else if (k == 1)
								ktemp = 2 * final_group;
							else if (k == 2)
								ktemp = 1 * final_group;
							else
								ktemp = 5 * final_group;
						} else if (delay_count == 16) {
							
							int	kmap[16] = {0,8,4,20,2,18,10,-6,1,17,9,-7,5,21,13,-3};
							if (kmap[k] >= 0)
								ktemp = kmap[k] * final_group;
							else
								ktemp = bigN + kmap[k] * final_group;
						} else {			
							
							ktemp = k * final_group;
						}

						temp = final_group * (bigN / N);
						gwsincos1plus01234567by4 (actemp + ktemp, temp, bigN, table + avx_word); 
					}
					table += 64;
				}
			}
			pass1_size /= 8;
		}

		else {
			N = N * 4;

			if (!gwdata->NEGACYCLIC_FFT) {
				for (i = 0; i < gwdata->PASS1_CACHE_LINES; i += 4) {
					
					temp = group + i;
					gwsincos15by4 (temp, N*2, table);
					gwsincos15by4 (temp + 1, N*2, table+1);
					gwsincos15by4 (temp + 2, N*2, table+2);
					gwsincos15by4 (temp + 3, N*2, table+3);
					table += 16;
				}
			}

			for (k = 0; k < delay_count; k++) {
				for (i = 0; i < gwdata->PASS1_CACHE_LINES; i += 4) {
					unsigned long bigN, ktemp, actemp, avx_word;

					for (avx_word = 0; avx_word < 4; avx_word++) {
						unsigned long final_group = group + i + avx_word;

						if (gwdata->NEGACYCLIC_FFT) {
							bigN = gwdata->FFTLEN * 2;
							actemp = final_group;
						} else {
							bigN = gwdata->FFTLEN;
							actemp = 0;
						}

						if (gwdata->NEGACYCLIC_FFT && delay_count == 4) {
							if (k == 0)
								ktemp = 0;
							else if	(k == 1)
								ktemp = 2 * final_group * 4;
							else if (k == 2)
								ktemp = 1 * final_group * 4;
							else
								ktemp = bigN - 1 * final_group * 4;
						} else if (gwdata->NEGACYCLIC_FFT) {
							
							int	kmap[16] = {0,8,4,-4,2,10,6,-2,1,9,5,-3,-1,7,3,-5};
							if (kmap[k] >= 0)
								ktemp = kmap[k] * final_group * 4;
							else
								ktemp = bigN + kmap[k] * final_group * 4;
						} else if (delay_count == 4) {
							if (k == 0)
								ktemp = 0;
							else if	(k == 1)
								ktemp = 2 * final_group;
							else if (k == 2)
								ktemp = 1 * final_group;
							else
								ktemp = 5 * final_group;
						} else if (delay_count == 16) {
							
							int	kmap[16] = {0,8,4,20,2,18,10,-6,1,17,9,-7,5,21,13,-3};
							if (kmap[k] >= 0)
								ktemp = kmap[k] * final_group;
							else
								ktemp = bigN + kmap[k] * final_group;
						} else {			
							
							ktemp = k * final_group;
						}

						temp = final_group * (bigN / N);
						gwsincos1plus0123by4 (actemp + ktemp, temp, bigN, table + avx_word); 
					}
					table += 32;
				}
			}
			pass1_size /= 4;
		}
#endif

		while ((pass1_size & 3) == 0) {

			N = N * 4;

			if (wpn4 && N == gwdata->PASS2_SIZE * gwdata->wpn_count) {
				double *weights, *inv_weights;

				weights = table;
				table += N / pass1_increment * gwdata->PASS1_CACHE_LINES;
				inv_weights = table;
				table += N / pass1_increment * gwdata->PASS1_CACHE_LINES;

				for (j = 0; j < N / 4; j += pass1_increment) {
				    for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
					temp = (group + j + i);
					gwfft_weights3 (gwdata->dd_data, temp, weights, NULL, inv_weights);
					gwfft_weights3 (gwdata->dd_data, temp + N/4, weights+1, NULL, inv_weights+1);
					gwfft_weights3 (gwdata->dd_data, temp + 2*N/4, weights+2, NULL, inv_weights+2);
					gwfft_weights3 (gwdata->dd_data, temp + 3*N/4, weights+3, NULL, inv_weights+3);
					weights += 4;
					inv_weights += 4;
				    }
				}
			}

			if (wpn4 || N != gwdata->PASS2_SIZE * gwdata->wpn_count * 4) {

				for (j = 0; j < N / 4; j += pass1_increment) {
				    for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
					temp = (group + j + i);
					gwsincos12by4 (temp, N, table);
					gwsincos12by4 (temp + upper_avx_word, N, table+1);
					gwsincos12by4 (temp + 2 * upper_avx_word, N, table+2);
					gwsincos12by4 (temp + 3 * upper_avx_word, N, table+3);
					table += 16;

					if (!gwdata->NEGACYCLIC_FFT) {
						gwsincos15by4 (temp, N*2, table);
						gwsincos15by4 (temp + upper_avx_word, N*2, table+1);
						gwsincos15by4 (temp + 2 * upper_avx_word, N*2, table+2);
						gwsincos15by4 (temp + 3 * upper_avx_word, N*2, table+3);
						table += 16;
					}

				    }
				}
			}

			if (!wpn4 && N == gwdata->PASS2_SIZE * gwdata->wpn_count * 4) {
				double *weights, *inv_weights;

				weights = table;
				table += N / 4 / pass1_increment * gwdata->PASS1_CACHE_LINES;
				inv_weights = table;
				table += N / 4 / pass1_increment * gwdata->PASS1_CACHE_LINES;

				for (j = 0; j < N / 4; j += pass1_increment) {
				    for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
					temp = (group + j + i);
					gwsincos012by4_weighted (gwdata->dd_data, temp, upper_avx_word, N, temp, table);
					*weights++ = table[24];
					*inv_weights++ = table[25];
					table += 24;

					if (!gwdata->NEGACYCLIC_FFT) {
						gwsincos15by4_weighted (gwdata->dd_data, temp, upper_avx_word, N*2, temp, table);
						table += 24;
					}
				    }
				}
			}

			pass1_size /= 4;
		}

		while (pass1_size % 5 == 0) {
			N = N * 5;

			for (j = 0; j < N / 5; j += pass1_increment) {
				for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
					temp = (group + j + i);
					gwsincos12by4 (temp, N, table);
					gwsincos12by4 (temp + upper_avx_word, N, table+1);
					gwsincos12by4 (temp + 2 * upper_avx_word, N, table+2);
					gwsincos12by4 (temp + 3 * upper_avx_word, N, table+3);
					table += 16;

					if (!gwdata->NEGACYCLIC_FFT) {
						gwsincos13by4 (temp, N*2, table);
						gwsincos13by4 (temp + upper_avx_word, N*2, table+1);
						gwsincos13by4 (temp + 2 * upper_avx_word, N*2, table+2);
						gwsincos13by4 (temp + 3 * upper_avx_word, N*2, table+3);
						table += 16;
					}
				}
			}
			pass1_size /= 5;
		}

		while (pass1_size % 3 == 0) {
			N = N * 3;

			for (j = 0; j < N / 3; j += pass1_increment) {
				for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
					temp = (group + j + i);
					gwsincos1by4 (temp, N, table);
					gwsincos1by4 (temp + upper_avx_word, N, table+1);
					gwsincos1by4 (temp + 2 * upper_avx_word, N, table+2);
					gwsincos1by4 (temp + 3 * upper_avx_word, N, table+3);
					table += 8;

					if (!gwdata->NEGACYCLIC_FFT) {
						gwsincos1by4 (temp, N*2, table);
						gwsincos1by4 (temp + upper_avx_word, N*2, table+1);
						gwsincos1by4 (temp + 2 * upper_avx_word, N*2, table+2);
						gwsincos1by4 (temp + 3 * upper_avx_word, N*2, table+3);
						table += 8;
					}
				}
			}
			pass1_size /= 3;
		}

		if (group == 0) gwdata->pass1_var_data_size = (unsigned long) ((char *) table - (char *) gwdata->pass1_var_data);
	}

	return (table);
}

double *yr4dwpn_build_fixed_pass1_table (
	gwhandle *gwdata,	
	double	*table)		
{
	unsigned long upper_avx_word, pass1_increment, pass1_size, i, j, N;

	upper_avx_word = gwdata->PASS2_SIZE;
	pass1_increment = 4 * gwdata->PASS2_SIZE;
	pass1_size = gwdata->PASS1_SIZE;

	if (!gwdata->NEGACYCLIC_FFT) {
		N = gwdata->FFTLEN;
		if (pass1_size == 256 || pass1_size == 512) {
			for (j = 0; j < N / 16; j += pass1_increment) {
				for (i = 1; i <= 7; i++) {	
					gwsincos1by4 (i * j, N, table);
					gwsincos1by4 (i * (j + upper_avx_word), N, table+1);
					gwsincos1by4 (i * (j + 2 * upper_avx_word), N, table+2);
					gwsincos1by4 (i * (j + 3 * upper_avx_word), N, table+3);
					table += 8;
				}
			}
			N = N / 16;
		}
		else if (pass1_size % 20 == 0) {
			for (j = 0; j < N / 20; j += pass1_increment) {
				for (i = 1; i <= 9; i++) {	
					gwsincos1by4 (i * j, N, table);
					gwsincos1by4 (i * (j + upper_avx_word), N, table+1);
					gwsincos1by4 (i * (j + 2 * upper_avx_word), N, table+2);
					gwsincos1by4 (i * (j + 3 * upper_avx_word), N, table+3);
					table += 8;
				}
			}
			N = N / 20;
		}
		else if (pass1_size == 384 || pass1_size == 768 || pass1_size == 1536) {
			for (j = 0; j < N / 24; j += pass1_increment) {
				for (i = 1; i <= 11; i++) {	
					gwsincos1by4 (i * j, N, table);
					gwsincos1by4 (i * (j + upper_avx_word), N, table+1);
					gwsincos1by4 (i * (j + 2 * upper_avx_word), N, table+2);
					gwsincos1by4 (i * (j + 3 * upper_avx_word), N, table+3);
					table += 8;
				}
			}
			N = N / 24;
		}
		else if (pass1_size % 28 == 0) {
			for (j = 0; j < N / 28; j += pass1_increment) {
				for (i = 1; i <= 13; i++) {	
					gwsincos1by4 (i * j, N, table);
					gwsincos1by4 (i * (j + upper_avx_word), N, table+1);
					gwsincos1by4 (i * (j + 2 * upper_avx_word), N, table+2);
					gwsincos1by4 (i * (j + 3 * upper_avx_word), N, table+3);
					table += 8;
				}
			}
			N = N / 28;
		}
		else {
			for (j = 0; j < N / 8; j += pass1_increment) {
				gwsincos125by4 (j, N, table);
				gwsincos125by4 (j + upper_avx_word, N, table+1);
				gwsincos125by4 (j + 2 * upper_avx_word, N, table+2);
				gwsincos125by4 (j + 3 * upper_avx_word, N, table+3);
				table += 24;
			}
			N = N / 8;
		}
		
		if (pass1_size == 1024 ||
#ifdef USE_REDUCED_SINCOS_FFTS
		    pass1_size == 1280 ||
		    pass1_size == 1536 ||
		    pass1_size == 1792 ||
#endif
		    pass1_size == 2048) {
			
			for (j = 0; j < N / 4; j += pass1_increment) {
				gwsincos12by4 (j, N, table);
				gwsincos12by4 (j + upper_avx_word, N, table+1);
				gwsincos12by4 (j + 2 * upper_avx_word, N, table+2);
				gwsincos12by4 (j + 3 * upper_avx_word, N, table+3);
				table += 16;
				gwsincos15by4 (j, N*2, table);
				gwsincos15by4 (j + upper_avx_word, N*2, table+1);
				gwsincos15by4 (j + 2 * upper_avx_word, N*2, table+2);
				gwsincos15by4 (j + 3 * upper_avx_word, N*2, table+3);
				table += 16;
			}
			N = N / 4;
		}
	}

	else {
		N = gwdata->FFTLEN / 2;
		for (j = 0; j < N / 4; j += pass1_increment) {
			
			gwsincos1plus0123by4 (j, N / 4, N * 4, table);
			gwsincos1plus0123by4 (j + upper_avx_word, N / 4, N * 4, table + 1);
			gwsincos1plus0123by4 (j + 2 * upper_avx_word, N / 4, N * 4, table + 2);
			gwsincos1plus0123by4 (j + 3 * upper_avx_word, N / 4, N * 4, table + 3);
			table += 32;
			
			gwsincos12by4 (j, N, table);
			gwsincos12by4 (j + upper_avx_word, N, table+1);
			gwsincos12by4 (j + 2 * upper_avx_word, N, table+2);
			gwsincos12by4 (j + 3 * upper_avx_word, N, table+3);
			table += 16;
		}
		N = N / 4;
		
#ifdef USE_REDUCED_SINCOS_FFTS
		if (pass1_size == 384 || pass1_size == 768) {
			for (j = 0; j < N / 3; j += pass1_increment) {
				gwsincos1by4 (j, N, table);
				gwsincos1by4 (j + upper_avx_word, N, table+1);
				gwsincos1by4 (j + 2 * upper_avx_word, N, table+2);
				gwsincos1by4 (j + 3 * upper_avx_word, N, table+3);
				table += 8;
			}
			N = N / 3;
		}
		if (pass1_size == 512 || pass1_size == 1024 || pass1_size == 1536 || pass1_size == 2048) {
			
			for (j = 0; j < N / 4; j += pass1_increment) {
				gwsincos12by4 (j, N, table);
				gwsincos12by4 (j + upper_avx_word, N, table+1);
				gwsincos12by4 (j + 2 * upper_avx_word, N, table+2);
				gwsincos12by4 (j + 3 * upper_avx_word, N, table+3);
				table += 16;
			}
			N = N / 4;
		}
		if (pass1_size == 640 || pass1_size == 1280) {
			for (j = 0; j < N / 5; j += pass1_increment) {
				gwsincos12by4 (j, N, table);
				gwsincos12by4 (j + upper_avx_word, N, table+1);
				gwsincos12by4 (j + 2 * upper_avx_word, N, table+2);
				gwsincos12by4 (j + 3 * upper_avx_word, N, table+3);
				table += 16;
			}
			N = N / 5;
		}
#else
		if (pass1_size == 1536 || pass1_size == 2048) {
			
			for (j = 0; j < N / 4; j += pass1_increment) {
				gwsincos12by4 (j, N, table);
				gwsincos12by4 (j + upper_avx_word, N, table+1);
				gwsincos12by4 (j + 2 * upper_avx_word, N, table+2);
				gwsincos12by4 (j + 3 * upper_avx_word, N, table+3);
				table += 16;
			}
			N = N / 4;
		}
#endif
	}

	return (table);
}

double *yr4_build_pass2_complex_table (
	gwhandle *gwdata,	
	double	*table)		
{
	unsigned long i, j, N;
	unsigned int pow2_count;

	N = gwdata->PASS2_SIZE;
	while (N % 3 == 0) {
		for (i = 0; i < N / 3; i += 4) {
			gwsincos1by4 (i, N, table);
			gwsincos1by4 (i+1, N, table+1);
			gwsincos1by4 (i+2, N, table+2);
			gwsincos1by4 (i+3, N, table+3);
			table += 8;
		}
		N = N / 3;
	}

	while (N % 5 == 0) {
		for (i = 0; i < N / 5; i += 4) {
			gwsincos12by4 (i, N, table);
			gwsincos12by4 (i+1, N, table+1);
			gwsincos12by4 (i+2, N, table+2);
			gwsincos12by4 (i+3, N, table+3);
			table += 16;
		}
		N = N / 5;
	}

	for (i = 0; i < N / 4; i += 4) {
		gwsincos12by4 (i, N, table);
		gwsincos12by4 (i+1, N, table+1);
		gwsincos12by4 (i+2, N, table+2);
		gwsincos12by4 (i+3, N, table+3);
		table += 16;
	}
	N = N / 4;

	for (i = N, pow2_count = 0; (i & 1) == 0; i >>= 1) pow2_count++;
	if (pow2_count & 1) N = 128;		
	else if (N > 256) N = 256;		
	for (i = 0; i < 4; i++) {
		for (j = 0; j < N / 4; j += 4) {
			gwsincos12by1 (i + j, N, table);
			table += 4;
		}
	}

	return (table);
}

double *yr4_build_pass2_real_table (
	gwhandle *gwdata,	
	double	*table)		
{
	unsigned long avx_increment, j, N;

	if (gwdata->NEGACYCLIC_FFT) return (table);

	avx_increment = 1;
	N = gwdata->PASS2_SIZE;

	while (N % 3 == 0) {
		for (j = 0; j < N / 3; j += 4) {
			gwsincos1by4 (j, N*2, table);		
			gwsincos1by4 (j + avx_increment, N*2, table+1);
			gwsincos1by4 (j + 2*avx_increment, N*2, table+2);
			gwsincos1by4 (j + 3*avx_increment, N*2, table+3);
			table += 8;
		}
		N = N / 3;
	}

	while (N % 5 == 0) {
		for (j = 0; j < N / 5; j += 4) {
			gwsincos13by4 (j, N*2, table);	
			gwsincos13by4 (j + avx_increment, N*2, table+1);
			gwsincos13by4 (j + 2*avx_increment, N*2, table+2);
			gwsincos13by4 (j + 3*avx_increment, N*2, table+3);
			table += 16;
		}
		N = N / 5;
	}

	for (j = 0; j < N / 4; j += 4) {
		gwsincos15by4 (j, N*2, table);
		gwsincos15by4 (j + avx_increment, N*2, table+1);
		gwsincos15by4 (j + 2*avx_increment, N*2, table+2);
		gwsincos15by4 (j + 3*avx_increment, N*2, table+3);
		table += 16;
	}
	N = N / 4;

	avx_increment = N * 2;
	for (j = 0; j < N / 4; j++) {
		gwsincos125by4 (j, N*2, table);				
		gwsincos12by4 (j + avx_increment, N, table+1);		
		table[3] = table[2] = table[1];
		table[7] = table[6] = table[5];
		table[11] = table[10] = table[9];
		table[15] = table[14] = table[13];
		table[19] = table[18] = table[17] = -table[1];
		table[23] = table[22] = table[21] = -table[5];
		table += 24;
	}

	return (table);
}

double *yr4dwpn_build_norm_table (
	gwhandle *gwdata,	
	double	*table)		
{
	unsigned long i, j, num_cols, upper_avx_word;

	upper_avx_word = gwdata->PASS2_SIZE;
	num_cols = gwdata->PASS2_SIZE * gwdata->wpn_count;
	for (i = 0; i < gwdata->FFTLEN / 2; i += num_cols) {
		for (j = 0; j < gwdata->FFTLEN; j += gwdata->FFTLEN / 2) {
			double	double1_weight, double2_weight, double3_weight, double4_weight;
			int	double1_sort, double2_sort, double3_sort, double4_sort;
			double	ttp, ttmp, ttp_over_b, ttmp_times_b, *tab20;

			if (gwdata->ZERO_PADDED_FFT && j >= gwdata->FFTLEN / 2) continue;

			double1_weight = gwfft_weight_exponent (gwdata->dd_data, i + j);
			double2_weight = gwfft_weight_exponent (gwdata->dd_data, i + j + upper_avx_word);
			double3_weight = gwfft_weight_exponent (gwdata->dd_data, i + j + 2 * upper_avx_word);
			double4_weight = gwfft_weight_exponent (gwdata->dd_data, i + j + 3 * upper_avx_word);

			double1_sort = (double1_weight > double2_weight) +
				       (double1_weight > double3_weight) +
				       (double1_weight > double4_weight);
			double2_sort = (double2_weight > double1_weight) +
				       (double2_weight > double3_weight) +
				       (double2_weight > double4_weight);
			double3_sort = (double3_weight > double1_weight) +
				       (double3_weight > double2_weight) +
				       (double3_weight > double4_weight);
			double4_sort = (double4_weight > double1_weight) +
				       (double4_weight > double2_weight) +
				       (double4_weight > double3_weight);

			gwfft_weights_fudged (gwdata->dd_data, i + j, gwdata->b, &ttp, &ttmp, &ttp_over_b, &ttmp_times_b);

			table[0] = ttmp;
			table[4] = double1_sort < 3 ? ttmp : ttmp_times_b;
			table[8] = double1_sort < 2 ? ttmp : ttmp_times_b;
			table[12] = double1_sort < 1 ? ttmp : ttmp_times_b;
			table[16] = ttmp_times_b;
			tab20 = table + 20;
			tab20[0] = ttp;
			tab20[4] = double1_sort < 3 ? ttp : ttp_over_b;
			tab20[8] = double1_sort < 2 ? ttp : ttp_over_b;
			tab20[12] = double1_sort < 1 ? ttp : ttp_over_b;
			tab20[16] = ttp_over_b;

			gwfft_weights_fudged (gwdata->dd_data, i + j + upper_avx_word, gwdata->b, &ttp, &ttmp, &ttp_over_b, &ttmp_times_b);
			table[1] = ttmp;
			table[5] = double2_sort < 3 ? ttmp : ttmp_times_b;
			table[9] = double2_sort < 2 ? ttmp : ttmp_times_b;
			table[13] = double2_sort < 1 ? ttmp : ttmp_times_b;
			table[17] = ttmp_times_b;
			tab20[1] = ttp;
			tab20[5] = double2_sort < 3 ? ttp : ttp_over_b;
			tab20[9] = double2_sort < 2 ? ttp : ttp_over_b;
			tab20[13] = double2_sort < 1 ? ttp : ttp_over_b;
			tab20[17] = ttp_over_b;

			gwfft_weights_fudged (gwdata->dd_data, i + j + 2 * upper_avx_word, gwdata->b, &ttp, &ttmp, &ttp_over_b, &ttmp_times_b);
			table[2] = ttmp;
			table[6] = double3_sort < 3 ? ttmp : ttmp_times_b;
			table[10] = double3_sort < 2 ? ttmp : ttmp_times_b;
			table[14] = double3_sort < 1 ? ttmp : ttmp_times_b;
			table[18] = ttmp_times_b;
			tab20[2] = ttp;
			tab20[6] = double3_sort < 3 ? ttp : ttp_over_b;
			tab20[10] = double3_sort < 2 ? ttp : ttp_over_b;
			tab20[14] = double3_sort < 1 ? ttp : ttp_over_b;
			tab20[18] = ttp_over_b;

			gwfft_weights_fudged (gwdata->dd_data, i + j + 3 * upper_avx_word, gwdata->b, &ttp, &ttmp, &ttp_over_b, &ttmp_times_b);
			table[3] = ttmp;
			table[7] = double4_sort < 3 ? ttmp : ttmp_times_b;
			table[11] = double4_sort < 2 ? ttmp : ttmp_times_b;
			table[15] = double4_sort < 1 ? ttmp : ttmp_times_b;
			table[19] = ttmp_times_b;
			tab20[3] = ttp;
			tab20[7] = double4_sort < 3 ? ttp : ttp_over_b;
			tab20[11] = double4_sort < 2 ? ttp : ttp_over_b;
			tab20[15] = double4_sort < 1 ? ttp : ttp_over_b;
			tab20[19] = ttp_over_b;

			table += 40;
		}
	}
	return (table);
}

double *yr4dwpn_build_biglit_table (
	gwhandle *gwdata,	
	double	*table)		
{
	struct gwasm_data *asm_data = (struct gwasm_data *) gwdata->asm_data;
const	int	START_OF_CHAIN = 0x8000;
const	int	END_OF_CHAIN = 0x4000;
	int	combos[16], next_combo[16];
	unsigned char combo_recorded[256];
	unsigned char *p;
	unsigned long upper_avx_word, fftlen_over_2, num_cols;
	unsigned long group, i, j, k, m, n, num_combos;

	memset (asm_data->u.ymm.YMM_FIRST_BIGLIT_VALUES, 0, sizeof (asm_data->u.ymm.YMM_FIRST_BIGLIT_VALUES));

	memset (combo_recorded, 0, sizeof (combo_recorded));
	upper_avx_word = gwdata->PASS2_SIZE;
	fftlen_over_2 = gwdata->FFTLEN / 2;
	num_combos = 0;

	for (i = 0; i < upper_avx_word; i++) {
		for (j = 0; j < fftlen_over_2; j += 4*upper_avx_word) {
			int	combo;

			combo = (is_big_word (gwdata, i + j + 3*upper_avx_word) << 7) +
				(is_big_word (gwdata, i + j + 2*upper_avx_word) << 6) +
				(is_big_word (gwdata, i + j + upper_avx_word) << 5) +
				(is_big_word (gwdata, i + j) << 4) +
				(is_big_word (gwdata, i + j + fftlen_over_2 + 3*upper_avx_word) << 3) +
				(is_big_word (gwdata, i + j + fftlen_over_2 + 2*upper_avx_word) << 2) +
				(is_big_word (gwdata, i + j + fftlen_over_2 + upper_avx_word) << 1) +
				(is_big_word (gwdata, i + j + fftlen_over_2));

			if (! combo_recorded[combo]) {
				combo_recorded[combo] = 1;
				combos[num_combos++] = combo;
			}
		}
	}

	for (i = 0; i < num_combos; i++)
		next_combo[i] = START_OF_CHAIN + END_OF_CHAIN;

	for (i = 0; i < num_combos; i++) {
		int	chain_end;

		if (! (next_combo[i] & START_OF_CHAIN)) continue;

		for (chain_end = i; ! (next_combo[chain_end] & END_OF_CHAIN); chain_end = next_combo[chain_end] & 0xFF);

		for (j = 0; j < num_combos; j++) {

			if (! (next_combo[j] & END_OF_CHAIN)) continue;
			
			if (j == chain_end) continue;

			if ((combos[i] >> 4) == (combos[j] & 0x0F)) {
				next_combo[j] = (next_combo[j] & START_OF_CHAIN) + i;
				next_combo[i] &= ~START_OF_CHAIN;
				break;
			}
		}
	}

	n = 0;
	for (i = 0; i < num_combos; i++) {
		if (! (next_combo[i] & START_OF_CHAIN)) continue;

		for (j = i; ; j = next_combo[j] & 0xFF) {
			((char *)gwdata->ASM_TIMERS)[n++] = (combos[j] >> 4);
			if (next_combo[j] & END_OF_CHAIN) {
				((char *)gwdata->ASM_TIMERS)[n++] = combos[j] & 0xF;
				break;
			}
		}
	}
GWASSERT (n <= 24);	
	GWASSERT (n <= 48);
	if (n > 48) gwdata->GWERROR = GWERROR_INTERNAL + 2;

	num_cols = gwdata->PASS2_SIZE * gwdata->wpn_count;

	p = (unsigned char *) table;
	for (group = 0; group < upper_avx_word; group += gwdata->PASS1_CACHE_LINES) {
	    for (n = 0; n < gwdata->FFTLEN / 2; n += num_cols) {
	        for (j = 0; j < num_cols; j += gwdata->PASS2_SIZE * 4) {
		    for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
			for (k = 0; k < gwdata->FFTLEN; k += gwdata->FFTLEN / 2) {
				unsigned long word;

				word = group + j + n + i + k;
				*p = is_big_word (gwdata, word);
				if (is_big_word (gwdata, word + upper_avx_word)) *p += 2;
				if (is_big_word (gwdata, word + 2 * upper_avx_word)) *p += 4;
				if (is_big_word (gwdata, word + 3 * upper_avx_word)) *p += 8;
   
				if (gwfft_weight_exponent (gwdata->dd_data, word) + 0.5 <
				    gwfft_weight_exponent (gwdata->dd_data, n + k) +
				    gwfft_weight_exponent (gwdata->dd_data, group + j + i))
					*p += 16;
				if (gwfft_weight_exponent (gwdata->dd_data, word + upper_avx_word) + 0.5 <
				    gwfft_weight_exponent (gwdata->dd_data, n + k + upper_avx_word) +
				    gwfft_weight_exponent (gwdata->dd_data, group + j + i))
					*p += 32;
				if (gwfft_weight_exponent (gwdata->dd_data, word + 2 * upper_avx_word) + 0.5 <
				    gwfft_weight_exponent (gwdata->dd_data, n + k + 2 * upper_avx_word) +
				    gwfft_weight_exponent (gwdata->dd_data, group + j + i))
					*p += 64;
				if (gwfft_weight_exponent (gwdata->dd_data, word + 3 * upper_avx_word) + 0.5 <
				    gwfft_weight_exponent (gwdata->dd_data, n + k + 3 * upper_avx_word) +
				    gwfft_weight_exponent (gwdata->dd_data, group + j + i))
					*p += 128;

				if (*p & 32) *p -= 16;
				if (*p & 128) *p -= 64;
				*p = ((*p >> 6) + ((*p >> 4) & 0x3)) * 16 + (*p & 0xF);
				p++;
			}

			p -= 2;
			for (m = 0; m <= 46; m++) {
				if (((char *)gwdata->ASM_TIMERS)[m]   == (p[0] & 0xF) &&
				    ((char *)gwdata->ASM_TIMERS)[m+1] == (p[1] & 0xF))
					break;
			}
			ASSERTG (m != 47);
			if (m == 47) gwdata->GWERROR = GWERROR_INTERNAL + 3;

			p[0] = ((p[0] >> 4) << 5) + ((p[1] >> 4) << 2);

			p[1] = (unsigned char) (m << 2);

			if (group + n + j + i < sizeof (asm_data->u.ymm.YMM_FIRST_BIGLIT_VALUES))
				asm_data->u.ymm.YMM_FIRST_BIGLIT_VALUES[group + n + j + i] = p[1];

			p += 2;
		    }
		}
	    }
	}
	return ((double *) p);
}

double *r4_build_pass1_table (
	gwhandle *gwdata,	
	double	*table)		
{
	unsigned long pass1_size, pass1_increment, first_levels_size;
	unsigned long group, i, j, k, N, temp, upper_sse2_word;
	int	pow2_count;

	pass1_increment = gwdata->PASS2_SIZE;
	upper_sse2_word = pass1_increment / 2;
	pass1_size = gwdata->PASS1_SIZE;

	if (pass1_size % 7 == 0)
		first_levels_size = 28;
	else if (pass1_size % 5 == 0 && !gwdata->NEGACYCLIC_FFT)
		first_levels_size = 20;
	else
		first_levels_size = 8;

	pass1_size /= first_levels_size;
	for (pow2_count = 0; (pass1_size & 1) == 0; pass1_size /= 2) pow2_count++;

	gwdata->pass1_var_data = table;

	for (group = 0; group < upper_sse2_word; group += gwdata->PASS1_CACHE_LINES) {

		pass1_size = gwdata->PASS1_SIZE;
		pass1_size /= first_levels_size; 
		N = gwdata->PASS2_SIZE;

		if (pow2_count & 1) {
			N = N * 8;

			if (!gwdata->NEGACYCLIC_FFT) {
				for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
					temp = (group + i);
					gwsincos1plus0123by2 (temp, pass1_increment, N*2, table);
					gwsincos1plus0123by2 (temp + upper_sse2_word, pass1_increment, N*2, table+1);
					table += 16;
				}
			}

			for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {

				temp = group + i;
				gwsincos1plus01234567by2 (0, temp, N, table);
				gwsincos1plus01234567by2 (0, temp + upper_sse2_word, N, table+1);
				table += 32;
			}
			pass1_size /= 8;
		}

		while ((pass1_size & 3) == 0) {
			N = N * 4;

			if (!gwdata->NEGACYCLIC_FFT) {
				for (j = 0; j < N*2 / 8; j += pass1_increment) {
					for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
						temp = (group + j + i);
						gwsincos15by2 (temp, N*2, table);
						gwsincos15by2 (temp + upper_sse2_word, N*2, table+1);
						table += 8;
					}
				}
			}

			for (j = 0; j < N / 4; j += pass1_increment) {
				for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
					temp = (group + j + i);
					gwsincos12by2 (temp, N, table);
					gwsincos12by2 (temp + upper_sse2_word, N, table+1);
					table += 8;
				}
			}
			pass1_size /= 4;
		}

		while (pass1_size % 5 == 0) {
			N = N * 5;

			if (!gwdata->NEGACYCLIC_FFT) {
				for (j = 0; j < N / 5; j += pass1_increment) {
					for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
						temp = (group + j + i);
						gwsincos13by2 (temp, N*2, table);
						gwsincos13by2 (temp + upper_sse2_word, N*2, table+1);
						table += 8;
					}
				}
			}

			for (j = 0; j < N / 5; j += pass1_increment) {
				for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
					temp = (group + j + i);
					gwsincos12by2 (temp, N, table);
					gwsincos12by2 (temp + upper_sse2_word, N, table+1);
					table += 8;
				}
			}
			pass1_size /= 5;
		}

		while (pass1_size % 3 == 0) {
			N = N * 3;

			if (!gwdata->NEGACYCLIC_FFT) {
				for (j = 0; j < N / 3; j += pass1_increment) {
					for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
						temp = (group + j + i);
						gwsincos1by2 (temp, N*2, table);
						gwsincos1by2 (temp + upper_sse2_word, N*2, table+1);
						table += 4;
					}
				}
			}

			for (j = 0; j < N / 3; j += pass1_increment) {
				for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
					temp = (group + j + i);
					gwsincos1by2 (temp, N, table);
					gwsincos1by2 (temp + upper_sse2_word, N, table+1);
					table += 4;
				}
			}
			pass1_size /= 3;
		}
		ASSERTG (pass1_size == 1);
		if (pass1_size != 1) gwdata->GWERROR = GWERROR_INTERNAL + 4;

		if (!gwdata->NEGACYCLIC_FFT) {
			N = gwdata->FFTLEN;
			pass1_size = gwdata->PASS1_SIZE;
			if (pass1_size % 20 == 0) {
				for (j = 0; j < N / 20; j += pass1_increment) {
					for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
						temp = (group + j + i);
						for (k = 1; k <= 9; k++) {	
							gwsincos1by2 (k * temp, N, table);
							gwsincos1by2 (k * (temp + upper_sse2_word), N, table+1);
							table += 4;
						}
					}
				}
			}
			else if (pass1_size % 28 == 0) {
				for (j = 0; j < N / 28; j += pass1_increment) {
					for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
						temp = (group + j + i);
						for (k = 1; k <= 13; k++) {	
							gwsincos1by2 (k * temp, N, table);
							gwsincos1by2 (k * (temp + upper_sse2_word), N, table+1);
							table += 4;
						}
					}
				}
			}
			else {
				for (j = 0; j < N / 8; j += pass1_increment) {
					for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
						temp = (group + j + i);
						gwsincos125by2 (temp, N, table);
						gwsincos125by2 (temp + upper_sse2_word, N, table+1);
						table += 12;
					}
				}
			}
		}

		else {
			N = gwdata->FFTLEN / 2;
			for (j = 0; j < N / 4; j += pass1_increment) {
				for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
					temp = (group + j + i);

					gwsincos1plus0123by2 (temp, 4 * temp, N * 4, table); 
					temp += upper_sse2_word;
					gwsincos1plus0123by2 (temp, 4 * temp, N * 4, table+1);
					table += 16;
				}
			}
		}

		if (group == 0) gwdata->pass1_var_data_size = (unsigned long) ((char *) table - (char *) gwdata->pass1_var_data);
	}

	return (table);
}

double *r4_build_pass2_complex_table (
	gwhandle *gwdata,	
	double	*table)		
{
	unsigned long i, N, limit, aux_table_size;

	if (gwdata->PASS2_SIZE <= 640 && gwdata->PASS2_SIZE % 3 != 0)
		aux_table_size = 0;
	else {
		for (i = 0, N = gwdata->PASS2_SIZE; (N & 1) == 0; N >>= 1) i++;
		if (i < 8)
			aux_table_size = 1 << i;
		else
			aux_table_size = (i & 1) ? 512 : 256;
	}
	
	N = gwdata->PASS2_SIZE;
	if (N % 3 == 0) {
		if (!aux_table_size ||  N / aux_table_size % 2 == 0) limit = N / 2;
		else if (N % 5 == 0) limit = N * 2 / 5;
		else limit = N / 3;
		for (i = 0; i < limit; i++) {
			gwsincos1by2 (i, N, table);
			table[1] = table[0];
			table[3] = table[2];
			table += 4;
		}
	}

	else {
		if (!aux_table_size ||  N / aux_table_size % 2 == 0) limit = N / 4;
		else limit = N / 5;
		for (i = 0; i < limit; i++) {
			gwsincos12by2 (i, N, table);
			table[1] = table[0];	
			table[3] = table[2];
			table[5] = table[4];	
			table[7] = table[6];
			table += 8;
		}
	}

	if (aux_table_size) {
		N = aux_table_size;
		for (i = 0; i < N / 4; i++) {
			gwsincos12by2 (i, N, table);
			table[1] = table[0];	
			table[3] = table[2];
			table[5] = table[4];	
			table[7] = table[6];
			table += 8;
		}
	}

	return (table);
}

double *r4_build_pass2_real_table (
	gwhandle *gwdata,	
	double	*table)		
{
	unsigned long i, N;

	if (gwdata->NEGACYCLIC_FFT) return (table);

	N = gwdata->PASS2_SIZE;
	if (N % 3 == 0) {
		for (i = 0; i < N / 3; i++) {
			gwsincos1by1 (i, N * 2, table);
			table += 2;
		}
		while (N % 3 == 0) N = N / 3;
	}

	if (N % 5 == 0) {
		for (i = 0; i < N / 5; i++) {
			gwsincos13by1 (i, N * 2, table);
			table += 4;
		}
		while (N % 5 == 0) N = N / 5;
	}

	for (i = 0; i < N / 4; i++) {
		gwsincos15by1 (i, N * 2, table);
		table += 4;
	}

	return (table);
}

double *r4delay_build_fixed_premult_table (
	gwhandle *gwdata,	
	double	*table)		
{
	unsigned long pass1_increment, j, N;

	if (gwdata->NEGACYCLIC_FFT) {
		N = gwdata->FFTLEN / 2;
		pass1_increment = gwdata->PASS2_SIZE;
		for (j = 0; j < N / 4; j += pass1_increment) {

			gwsincos1plus0123by2 (j, N / 4, N * 4, table);
			table[1] = table[0];	
			table[3] = table[2];
			table[5] = table[4];	
			table[7] = table[6];
			table[9] = table[8];	
			table[11] = table[10];
			table[13] = table[12];	
			table[15] = table[14];
			table += 16;
		}
	}

	return (table);
}

double *r4delay_build_pass1_table (
	gwhandle *gwdata,	
	double	*table)		
{
	unsigned long pass1_size, pass1_increment, delay_count;
	unsigned long group, i, j, k, N, temp, upper_sse2_word;
	int	pow2_count;

	pass1_increment = gwdata->PASS2_SIZE;
	upper_sse2_word = pass1_increment / 2;
	pass1_size = gwdata->PASS1_SIZE;

	if (pass1_size % 7 == 0)
		delay_count = 14;
	else if (pass1_size % 5 == 0 && !gwdata->NEGACYCLIC_FFT)
		delay_count = 10;
 	else if ((pass1_size == 512 && !gwdata->NEGACYCLIC_FFT) ||
		 (pass1_size == 1024 && !gwdata->NEGACYCLIC_FFT) ||
		 (pass1_size == 2560 && gwdata->NEGACYCLIC_FFT) ||
		 (pass1_size == 5120 && gwdata->NEGACYCLIC_FFT) ||
		 pass1_size == 1536 || pass1_size == 2048 || pass1_size == 3072 || pass1_size == 4096)
		delay_count = 16;
	else
		delay_count = 4;

	pass1_size /= (delay_count * 2);
	for (pow2_count = 0; (pass1_size & 1) == 0; pass1_size /= 2) pow2_count++;

	gwdata->pass1_var_data = table;

	for (group = 0; group < upper_sse2_word; group += gwdata->PASS1_CACHE_LINES) {

		pass1_size = gwdata->PASS1_SIZE;
		pass1_size /= (delay_count * 2);	
		N = gwdata->PASS2_SIZE;

		if (pow2_count & 1) {
			N = N * 8;

			if (!gwdata->NEGACYCLIC_FFT) {
				for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
					temp = (group + i);
					gwsincos1plus0123by2 (temp, pass1_increment, N*2, table);
					gwsincos1plus0123by2 (temp + upper_sse2_word, pass1_increment, N*2, table+1);
					table += 16;
				}
			}

			for (k = 0; k < delay_count; k++) {
				for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
					unsigned long bigN, ktemp, actemp;

					if (gwdata->NEGACYCLIC_FFT) {
						bigN = gwdata->FFTLEN * 2;
						actemp = group + i;
					} else {
						bigN = gwdata->FFTLEN;
						actemp = 0;
					}

					if (gwdata->NEGACYCLIC_FFT && delay_count == 4) {
						if (k == 0)
							ktemp = 0;
						else if (k == 1)
							ktemp = 2 * (group + i) * 4;
						else if (k == 2)
							ktemp = 1 * (group + i) * 4;
						else
							ktemp = bigN - 1 * (group + i) * 4;
					} else if (gwdata->NEGACYCLIC_FFT) {
						
						int	kmap[16] = {0,8,4,-4,2,10,6,-2,1,9,5,-3,-1,7,3,-5};
						if (kmap[k] >= 0)
							ktemp = kmap[k] * (group + i) * 4;
						else
							ktemp = bigN + kmap[k] * (group + i) * 4;
					} else if (delay_count == 4) {
						if (k == 0)
							ktemp = 0;
						else if (k == 1)
							ktemp = 2 * (group + i);
						else if (k == 2)
							ktemp = 1 * (group + i);
						else
							ktemp = 5 * (group + i);
					} else if (delay_count == 16) {
						
						int	kmap[16] = {0,8,4,20,2,18,10,-6,1,17,9,-7,5,21,13,-3};
						if (kmap[k] >= 0)
							ktemp = kmap[k] * (group + i);
						else
							ktemp = bigN + kmap[k] * (group + i);
					} else {			
						
						ktemp = k * (group + i);
					}

					temp = (group + i) * (bigN / N);
					gwsincos1plus01234567by2 (actemp + ktemp, temp, bigN, table); 
					temp = (group + i + upper_sse2_word) * (bigN / N);
					if (gwdata->NEGACYCLIC_FFT) actemp += upper_sse2_word;
					gwsincos1plus01234567by2 (actemp + ktemp, temp, bigN, table+1);
					table += 32;
				}
			}
			pass1_size /= 8;
		}

		else {
			N = N * 4;

			if (!gwdata->NEGACYCLIC_FFT) {
				for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
					temp = (group + i);
					gwsincos15by2 (temp, N*2, table);
					gwsincos15by2 (temp + upper_sse2_word, N*2, table+1);
					table += 8;
				}
			}

			for (k = 0; k < delay_count; k++) {
				for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
					unsigned long bigN, ktemp, actemp;

					if (gwdata->NEGACYCLIC_FFT) {
						bigN = gwdata->FFTLEN * 2;
						actemp = group + i;
					} else {
						bigN = gwdata->FFTLEN;
						actemp = 0;
					}

					if (gwdata->NEGACYCLIC_FFT && delay_count == 4) {
						if (k == 0)
							ktemp = 0;
						else if (k == 1)
							ktemp = 2 * (group + i) * 4;
						else if (k == 2)
							ktemp = 1 * (group + i) * 4;
						else
							ktemp = bigN - 1 * (group + i) * 4;
					} else if (gwdata->NEGACYCLIC_FFT) {
						
							int	kmap[16] = {0,8,4,-4,2,10,6,-2,1,9,5,-3,-1,7,3,-5};
							if (kmap[k] >= 0)
								ktemp = kmap[k] * (group + i) * 4;
							else
								ktemp = bigN + kmap[k] * (group + i) * 4;
					} else if (delay_count == 4) {
						if (k == 0)
							ktemp = 0;
						else if (k == 1)
							ktemp = 2 * (group + i);
						else if (k == 2)
							ktemp = 1 * (group + i);
						else
							ktemp = 5 * (group + i);
					} else if (delay_count == 16) {
						
						int	kmap[16] = {0,8,4,20,2,18,10,-6,1,17,9,-7,5,21,13,-3};
						if (kmap[k] >= 0)
							ktemp = kmap[k] * (group + i);
						else
							ktemp = bigN + kmap[k] * (group + i);
					} else {			
						
						ktemp = k * (group + i);
					}

					temp = (group + i) * (bigN / N);
					gwsincos1plus0123by2 (actemp + ktemp, temp, bigN, table); 
					temp = (group + i + upper_sse2_word) * (bigN / N);
					if (gwdata->NEGACYCLIC_FFT) actemp += upper_sse2_word;
					gwsincos1plus0123by2 (actemp + ktemp, temp, bigN, table+1);
					table += 16;
				}
			}
			pass1_size /= 4;
		}

		while ((pass1_size & 3) == 0) {
			N = N * 4;

			if (!gwdata->NEGACYCLIC_FFT) {
				for (j = 0; j < N*2 / 8; j += pass1_increment) {
					for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
						temp = (group + j + i);
						gwsincos15by2 (temp, N*2, table);
						gwsincos15by2 (temp + upper_sse2_word, N*2, table+1);
						table += 8;
					}
				}
			}

			for (j = 0; j < N / 4; j += pass1_increment) {
				for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
					temp = (group + j + i);
					gwsincos12by2 (temp, N, table);
					gwsincos12by2 (temp + upper_sse2_word, N, table+1);
					table += 8;
				}
			}
			pass1_size /= 4;
		}

		while (pass1_size % 5 == 0) {
			N = N * 5;

			if (!gwdata->NEGACYCLIC_FFT) {
				for (j = 0; j < N / 5; j += pass1_increment) {
					for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
						temp = (group + j + i);
						gwsincos13by2 (temp, N*2, table);
						gwsincos13by2 (temp + upper_sse2_word, N*2, table+1);
						table += 8;
					}
				}
			}

			for (j = 0; j < N / 5; j += pass1_increment) {
				for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
					temp = (group + j + i);
					gwsincos12by2 (temp, N, table);
					gwsincos12by2 (temp + upper_sse2_word, N, table+1);
					table += 8;
				}
			}
			pass1_size /= 5;
		}

		while (pass1_size % 3 == 0) {
			N = N * 3;

			if (!gwdata->NEGACYCLIC_FFT) {
				for (j = 0; j < N / 3; j += pass1_increment) {
					for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
						temp = (group + j + i);
						gwsincos1by2 (temp, N*2, table);
						gwsincos1by2 (temp + upper_sse2_word, N*2, table+1);
						table += 4;
					}
				}
			}

			for (j = 0; j < N / 3; j += pass1_increment) {
				for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
					temp = (group + j + i);
					gwsincos1by2 (temp, N, table);
					gwsincos1by2 (temp + upper_sse2_word, N, table+1);
					table += 4;
				}
			}
			pass1_size /= 3;
		}
		ASSERTG (pass1_size == 1);
		if (pass1_size != 1) gwdata->GWERROR = GWERROR_INTERNAL + 5;

		if (group == 0) gwdata->pass1_var_data_size = (unsigned long) ((char *) table - (char *) gwdata->pass1_var_data);
	}

	return (table);
}

double *r4delay_build_fixed_pass1_table (
	gwhandle *gwdata,	
	double	*table)		
{
	unsigned long pass1_increment, upper_sse2_word, pass1_size, i, j, N;

	pass1_increment = gwdata->PASS2_SIZE;
	upper_sse2_word = pass1_increment / 2;
	pass1_size = gwdata->PASS1_SIZE;

	if (!gwdata->NEGACYCLIC_FFT) {
		N = gwdata->FFTLEN;
		if (pass1_size % 20 == 0) {
			for (j = 0; j < N / 20; j += pass1_increment) {
				for (i = 1; i <= 9; i++) {	
					gwsincos1by2 (i * j, N, table);
					gwsincos1by2 (i * (j + upper_sse2_word), N, table+1);
					table += 4;
				}
			}
			N = N / 20;
		}
		else if (pass1_size % 28 == 0) {
			for (j = 0; j < N / 28; j += pass1_increment) {
				for (i = 1; i <= 13; i++) {	
					gwsincos1by2 (i * j, N, table);
					gwsincos1by2 (i * (j + upper_sse2_word), N, table+1);
					table += 4;
				}
			}
			N = N / 28;
		}
		else {
			for (j = 0; j < N / 8; j += pass1_increment) {
				gwsincos125by2 (j, N, table);
				gwsincos125by2 (j + upper_sse2_word, N, table+1);
				table += 12;
			}
			N = N / 8;
			
			if (pass1_size == 512 || pass1_size == 1024 || pass1_size == 1536 ||
			    pass1_size == 2048 || pass1_size == 3072 || pass1_size == 4096) {
				
				for (j = 0; j < N*2 / 8; j += pass1_increment) {
					gwsincos15by2 (j, N*2, table);
					gwsincos15by2 (j + upper_sse2_word, N*2, table+1);
					table += 8;
				}
				
				for (j = 0; j < N / 4; j += pass1_increment) {
					gwsincos12by2 (j, N, table);
					gwsincos12by2 (j + upper_sse2_word, N, table+1);
					table += 8;
				}
				N = N / 4;
			}
		}
	}

	else {
		N = gwdata->FFTLEN / 2;
		for (j = 0; j < N / 4; j += pass1_increment) {
			gwsincos12by2 (j, N, table);
			gwsincos12by2 (j + upper_sse2_word, N, table+1);
			table += 8;
		}
		N = N / 4;
		
		if (pass1_size == 1536 || pass1_size == 2048 || pass1_size == 2560 ||
		    pass1_size == 3072 || pass1_size == 4096 || pass1_size == 5120) {
			
			for (j = 0; j < N / 4; j += pass1_increment) {
				gwsincos12by2 (j, N, table);
				gwsincos12by2 (j + upper_sse2_word, N, table+1);
				table += 8;
			}
			N = N / 4;
		}
	}

	return (table);
}

double *r4dwpn_build_pass1_table (
	gwhandle *gwdata,	
	double	*table)		
{
	struct gwasm_data *asm_data = (struct gwasm_data *) gwdata->asm_data;
	unsigned long pass1_size, pass1_increment, delay_count;
	unsigned long group, i, j, k, N, temp, upper_sse2_word;
	int	pow2_count;

	pass1_increment = gwdata->PASS2_SIZE;
	upper_sse2_word = pass1_increment / 2;
	pass1_size = gwdata->PASS1_SIZE;

	if (pass1_size % 7 == 0)
		delay_count = 14;
	else if (pass1_size % 5 == 0 && !gwdata->NEGACYCLIC_FFT)
		delay_count = 10;
 	else if ((pass1_size == 512 && !gwdata->NEGACYCLIC_FFT) ||
		 (pass1_size == 1024 && !gwdata->NEGACYCLIC_FFT) ||
		 (pass1_size == 2560 && gwdata->NEGACYCLIC_FFT) ||
		 (pass1_size == 5120 && gwdata->NEGACYCLIC_FFT) ||
		 pass1_size == 1536 || pass1_size == 2048 || pass1_size == 3072 || pass1_size == 4096)
		delay_count = 16;
	else
		delay_count = 4;

	pass1_size /= (delay_count * 2);
	for (pow2_count = 0; (pass1_size & 1) == 0; pass1_size /= 2) pow2_count++;

	if (pow2_count & 1) gwdata->wpn_count = 8;
	else if (gwdata->PASS1_SIZE == 1792 || gwdata->PASS1_SIZE == 2048) gwdata->wpn_count = 16;
	else gwdata->wpn_count = 4;
	asm_data->count2 = gwdata->wpn_count;
	asm_data->count3 = asm_data->addcount1 / gwdata->wpn_count;

	gwdata->pass1_var_data = table;

	for (group = 0; group < upper_sse2_word; group += gwdata->PASS1_CACHE_LINES) {

		pass1_size = gwdata->PASS1_SIZE;
		pass1_size /= (delay_count * 2);	
		N = gwdata->PASS2_SIZE;

		if (pow2_count & 1) {
			N = N * 8;

			if (!gwdata->NEGACYCLIC_FFT) {
				for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
					temp = (group + i);
					gwsincos1plus0123by2 (temp, pass1_increment, N*2, table);
					gwsincos1plus0123by2 (temp + upper_sse2_word, pass1_increment, N*2, table+1);
					table += 16;
				}
			}

			for (k = 0; k < delay_count; k++) {
				for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
					unsigned long bigN, ktemp, actemp;

					if (gwdata->NEGACYCLIC_FFT) {
						bigN = gwdata->FFTLEN * 2;
						actemp = group + i;
					} else {
						bigN = gwdata->FFTLEN;
						actemp = 0;
					}

					if (gwdata->NEGACYCLIC_FFT && delay_count == 4) {
						if (k == 0)
							ktemp = 0;
						else if (k == 1)
							ktemp = 2 * (group + i) * 4;
						else if (k == 2)
							ktemp = 1 * (group + i) * 4;
						else
							ktemp = bigN - 1 * (group + i) * 4;
					} else if (gwdata->NEGACYCLIC_FFT) {
						
						int	kmap[16] = {0,8,4,-4,2,10,6,-2,1,9,5,-3,-1,7,3,-5};
						if (kmap[k] >= 0)
							ktemp = kmap[k] * (group + i) * 4;
						else
							ktemp = bigN + kmap[k] * (group + i) * 4;
					} else if (delay_count == 4) {
						if (k == 0)
							ktemp = 0;
						else if (k == 1)
							ktemp = 2 * (group + i);
						else if (k == 2)
							ktemp = 1 * (group + i);
						else
							ktemp = 5 * (group + i);
					} else if (delay_count == 16) {
						
						int	kmap[16] = {0,8,4,20,2,18,10,-6,1,17,9,-7,5,21,13,-3};
						if (kmap[k] >= 0)
							ktemp = kmap[k] * (group + i);
						else
							ktemp = bigN + kmap[k] * (group + i);
					} else {			
						
						ktemp = k * (group + i);
					}

					temp = (group + i) * (bigN / N);
					gwsincos1plus01234567by2 (actemp + ktemp, temp, bigN, table); 
					temp = (group + i + upper_sse2_word) * (bigN / N);
					if (gwdata->NEGACYCLIC_FFT) actemp += upper_sse2_word;
					gwsincos1plus01234567by2 (actemp + ktemp, temp, bigN, table+1); 
					table += 32;
				}
			}
			pass1_size /= 8;
		}

		else {
			N = N * 4;

			if (!gwdata->NEGACYCLIC_FFT) {
				for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
					temp = (group + i);
					gwsincos15by2 (temp, N*2, table);
					gwsincos15by2 (temp + upper_sse2_word, N*2, table+1);
					table += 8;
				}
			}

			for (k = 0; k < delay_count; k++) {
				for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
					unsigned long bigN, ktemp, actemp;

					if (gwdata->NEGACYCLIC_FFT) {
						bigN = gwdata->FFTLEN * 2;
						actemp = group + i;
					} else {
						bigN = gwdata->FFTLEN;
						actemp = 0;
					}

					if (gwdata->NEGACYCLIC_FFT && delay_count == 4) {
						if (k == 0)
							ktemp = 0;
						else if (k == 1)
							ktemp = 2 * (group + i) * 4;
						else if (k == 2)
							ktemp = 1 * (group + i) * 4;
						else
							ktemp = bigN - 1 * (group + i) * 4;
					} else if (gwdata->NEGACYCLIC_FFT) {
						
							int	kmap[16] = {0,8,4,-4,2,10,6,-2,1,9,5,-3,-1,7,3,-5};
							if (kmap[k] >= 0)
								ktemp = kmap[k] * (group + i) * 4;
							else
								ktemp = bigN + kmap[k] * (group + i) * 4;
					} else if (delay_count == 4) {
						if (k == 0)
							ktemp = 0;
						else if (k == 1)
							ktemp = 2 * (group + i);
						else if (k == 2)
							ktemp = 1 * (group + i);
						else
							ktemp = 5 * (group + i);
					} else if (delay_count == 16) {
						
						int	kmap[16] = {0,8,4,20,2,18,10,-6,1,17,9,-7,5,21,13,-3};
						if (kmap[k] >= 0)
							ktemp = kmap[k] * (group + i);
						else
							ktemp = bigN + kmap[k] * (group + i);
					} else {			
						
						ktemp = k * (group + i);
					}

					temp = (group + i) * (bigN / N);
					gwsincos1plus0123by2 (actemp + ktemp, temp, bigN, table); 
					temp = (group + i + upper_sse2_word) * (bigN / N);
					if (gwdata->NEGACYCLIC_FFT) actemp += upper_sse2_word;
					gwsincos1plus0123by2 (actemp + ktemp, temp, bigN, table+1);
					table += 16;
				}
			}
			pass1_size /= 4;
		}

		while ((pass1_size & 3) == 0) {

			N = N * 4;

			if (N != gwdata->PASS2_SIZE * gwdata->wpn_count * 4) {

				if (!gwdata->NEGACYCLIC_FFT) {
					for (j = 0; j < N*2 / 8; j += pass1_increment) {
					    for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
						temp = (group + j + i);
						gwsincos15by2 (temp, N*2, table);
						gwsincos15by2 (temp + upper_sse2_word, N*2, table+1);
						table += 8;
					    }
					}
				}

				for (j = 0; j < N / 4; j += pass1_increment) {
				    for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
					temp = (group + j + i);
					gwsincos12by2 (temp, N, table);
					gwsincos12by2 (temp + upper_sse2_word, N, table+1);
					table += 8;
				    }
				}
			}

			else {

				if (!gwdata->NEGACYCLIC_FFT) {
					for (j = 0; j < N*2 / 8; j += pass1_increment) {
					    for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
						temp = (group + j + i);
						gwsincos15by2_weighted (gwdata->dd_data, temp, upper_sse2_word, N*2, temp, table);
						table += 12;
					    }
					}
				}

				for (j = 0; j < N / 4; j += pass1_increment) {
				    for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
					temp = (group + j + i);
					gwsincos012by2_weighted (gwdata->dd_data, temp, upper_sse2_word, N, temp, table);
					table += 16;
				    }
				}
			}

			pass1_size /= 4;
		}

		while (pass1_size % 5 == 0) {
			N = N * 5;

			if (!gwdata->NEGACYCLIC_FFT) {
				for (j = 0; j < N / 5; j += pass1_increment) {
					for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
						temp = (group + j + i);
						gwsincos13by2 (temp, N*2, table);
						gwsincos13by2 (temp + upper_sse2_word, N*2, table+1);
						table += 8;
					}
				}
			}

			for (j = 0; j < N / 5; j += pass1_increment) {
				for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
					temp = (group + j + i);
					gwsincos12by2 (temp, N, table);
					gwsincos12by2 (temp + upper_sse2_word, N, table+1);
					table += 8;
				}
			}
			pass1_size /= 5;
		}

		while (pass1_size % 3 == 0) {
			N = N * 3;

			if (!gwdata->NEGACYCLIC_FFT) {
				for (j = 0; j < N / 3; j += pass1_increment) {
					for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
						temp = (group + j + i);
						gwsincos1by2 (temp, N*2, table);
						gwsincos1by2 (temp + upper_sse2_word, N*2, table+1);
						table += 4;
					}
				}
			}

			for (j = 0; j < N / 3; j += pass1_increment) {
				for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
					temp = (group + j + i);
					gwsincos1by2 (temp, N, table);
					gwsincos1by2 (temp + upper_sse2_word, N, table+1);
					table += 4;
				}
			}
			pass1_size /= 3;
		}

		if (group == 0) gwdata->pass1_var_data_size = (unsigned long) ((char *) table - (char *) gwdata->pass1_var_data);
	}

	return (table);
}

double *r4_build_norm_table (
	gwhandle *gwdata,	
	double	*table,		
	int	col)		
{
	unsigned long i, k, num_cols;

	if (gwdata->PASS2_SIZE == 0) {
		if (!col) return (table);

		for (i = 0; i < gwdata->FFTLEN; i++) {
			unsigned long j, table_entry;
			double	ttp, ttmp;

			gwfft_weights3 (gwdata->dd_data, i, &ttp, NULL, &ttmp);

			j = addr_offset (gwdata, i) / sizeof (double);
			table_entry = j >> 1;

			table[table_entry*4+(j&1)] = ttmp;
			table[table_entry*4+2+(j&1)] = ttp;
		}
		return (table + gwdata->FFTLEN + gwdata->FFTLEN);
	}

	num_cols = gwdata->PASS2_SIZE / 2;
	if (col) {

		for (i = 0; i < num_cols; i++) {
			double	ttp, ttmp;

			gwfft_weights3 (gwdata->dd_data, i, &ttp, NULL, &ttmp);

			table[i*4] = ttmp;
			table[i*4+1] = ttmp;
			table[i*4+2] = ttp;
			table[i*4+3] = ttp;
		}
		return (table + num_cols * 4);
	}

	else {
		unsigned long h, hlimit, haddin, m, mmult, u, umult;

		umult = gwdata->FFTLEN / 2;
		hlimit = gwdata->FFTLEN / 4 / (2*num_cols);
		for (h = 0; h < hlimit; h++) {
			haddin = h * 2 * num_cols;
			mmult = gwdata->FFTLEN / 4;
			for (u = 0; u < 2; u++) {
				for (m = 0; m < 2; m++) {
					for (k = 0; k < 2; k++) {
						double	ttp, ttmp;
						long	n;

						n = haddin + u * umult + m * mmult + k * num_cols;
						gwfft_weights3 (gwdata->dd_data, n, &ttp, &ttmp, NULL);

						table[k] = ttmp;
						table[2+k] = ttp;
					}
					table += 4;
				}
			}
		}
		return (table);
	}
}

double *r4dwpn_build_norm_table (
	gwhandle *gwdata,	
	double	*table)		
{
	unsigned long i, j, num_cols, upper_sse2_word, second_word_in_cache_line;

	upper_sse2_word = gwdata->PASS2_SIZE / 2;
	second_word_in_cache_line = gwdata->FFTLEN / 4;
	num_cols = gwdata->PASS2_SIZE * gwdata->wpn_count;
	for (i = 0; i < gwdata->FFTLEN / 4; i += num_cols) {
		for (j = 0; j < gwdata->FFTLEN; j += gwdata->FFTLEN / 2) {
			double	word1_lower_weight, word1_upper_weight, word2_lower_weight, word2_upper_weight;
			int	word1_lower_sort, word1_upper_sort, word2_lower_sort, word2_upper_sort;
			double	ttp, ttmp, ttp_over_b, ttmp_times_b, *tab10;

			word1_lower_weight = gwfft_weight_exponent (gwdata->dd_data, i + j);
			word1_upper_weight = gwfft_weight_exponent (gwdata->dd_data, i + j + upper_sse2_word);
			word2_lower_weight = gwfft_weight_exponent (gwdata->dd_data, i + j + second_word_in_cache_line);
			word2_upper_weight = gwfft_weight_exponent (gwdata->dd_data, i + j + second_word_in_cache_line + upper_sse2_word);

			word1_lower_sort = (word1_lower_weight > word1_upper_weight) +
					   (word1_lower_weight > word2_lower_weight) +
					   (word1_lower_weight > word2_upper_weight);
			word1_upper_sort = (word1_upper_weight > word1_lower_weight) +
					   (word1_upper_weight > word2_lower_weight) +
					   (word1_upper_weight > word2_upper_weight);
			word2_lower_sort = (word2_lower_weight > word1_lower_weight) +
					   (word2_lower_weight > word1_upper_weight) +
					   (word2_lower_weight > word2_upper_weight);
			word2_upper_sort = (word2_upper_weight > word1_lower_weight) +
					   (word2_upper_weight > word1_upper_weight) +
					   (word2_upper_weight > word2_lower_weight);

			gwfft_weights_fudged (gwdata->dd_data, i + j, gwdata->b, &ttp, &ttmp, &ttp_over_b, &ttmp_times_b);

			table[0] = ttmp;
			table[2] = word1_lower_sort < 3 ? ttmp : ttmp_times_b;
			table[4] = word1_lower_sort < 2 ? ttmp : ttmp_times_b;
			table[6] = word1_lower_sort < 1 ? ttmp : ttmp_times_b;
			table[8] = ttmp_times_b;
			tab10 = table + 10;
			tab10[0] = ttp;
			tab10[2] = word1_lower_sort < 3 ? ttp : ttp_over_b;
			tab10[4] = word1_lower_sort < 2 ? ttp : ttp_over_b;
			tab10[6] = word1_lower_sort < 1 ? ttp : ttp_over_b;
			tab10[8] = ttp_over_b;

			gwfft_weights_fudged (gwdata->dd_data, i + j + upper_sse2_word, gwdata->b, &ttp, &ttmp, &ttp_over_b, &ttmp_times_b);

			table[1] = ttmp;
			table[3] = word1_upper_sort < 3 ? ttmp : ttmp_times_b;
			table[5] = word1_upper_sort < 2 ? ttmp : ttmp_times_b;
			table[7] = word1_upper_sort < 1 ? ttmp : ttmp_times_b;
			table[9] = ttmp_times_b;
			tab10[1] = ttp;
			tab10[3] = word1_upper_sort < 3 ? ttp : ttp_over_b;
			tab10[5] = word1_upper_sort < 2 ? ttp : ttp_over_b;
			tab10[7] = word1_upper_sort < 1 ? ttp : ttp_over_b;
			tab10[9] = ttp_over_b;

			table += 20;

			gwfft_weights_fudged (gwdata->dd_data, i + j + second_word_in_cache_line, gwdata->b, &ttp, &ttmp, &ttp_over_b, &ttmp_times_b);

			table[0] = ttmp;
			table[2] = word2_lower_sort < 3 ? ttmp : ttmp_times_b;
			table[4] = word2_lower_sort < 2 ? ttmp : ttmp_times_b;
			table[6] = word2_lower_sort < 1 ? ttmp : ttmp_times_b;
			table[8] = ttmp_times_b;
			tab10 = table + 10;
			tab10[0] = ttp;
			tab10[2] = word2_lower_sort < 3 ? ttp : ttp_over_b;
			tab10[4] = word2_lower_sort < 2 ? ttp : ttp_over_b;
			tab10[6] = word2_lower_sort < 1 ? ttp : ttp_over_b;
			tab10[8] = ttp_over_b;

			gwfft_weights_fudged (gwdata->dd_data, i + j + second_word_in_cache_line + upper_sse2_word, gwdata->b, &ttp, &ttmp, &ttp_over_b, &ttmp_times_b);

			table[1] = ttmp;
			table[3] = word2_upper_sort < 3 ? ttmp : ttmp_times_b;
			table[5] = word2_upper_sort < 2 ? ttmp : ttmp_times_b;
			table[7] = word2_upper_sort < 1 ? ttmp : ttmp_times_b;
			table[9] = ttmp_times_b;
			tab10[1] = ttp;
			tab10[3] = word2_upper_sort < 3 ? ttp : ttp_over_b;
			tab10[5] = word2_upper_sort < 2 ? ttp : ttp_over_b;
			tab10[7] = word2_upper_sort < 1 ? ttp : ttp_over_b;
			tab10[9] = ttp_over_b;

			table += 20;
		}
	}
	return (table);
}

double *r4_build_biglit_table (
	gwhandle *gwdata,	
	double	*table)		
{
	unsigned char *p;
	unsigned long h, i, j, k, m, u, gap;
	unsigned long hlimit, haddin, mmult, umult;

	if (gwdata->PASS2_SIZE == 0) {

		p = (unsigned char *) table;
		for (i = 0; i < gwdata->FFTLEN; i++) {
			unsigned long table_entry;

			j = addr_offset (gwdata, i) / sizeof (double);
			table_entry = j >> 1;

			if ((j & 1) == 0) {
				p[table_entry] = is_big_word (gwdata, i) * 16;
			}

			else {
				if (is_big_word (gwdata, i)) p[table_entry] += 32;
			}
		}
		return ((double *) (p + gwdata->FFTLEN / 2));
	}

	gap = gwdata->PASS2_SIZE / 2;

	p = (unsigned char *) table;
	umult = gwdata->FFTLEN / 2;
	hlimit = gwdata->FFTLEN / 4 / (2*gap);
	for (i = 0; i < gap; i += gwdata->PASS1_CACHE_LINES) {
		for (h = 0; h < hlimit; h++) {
			haddin = h * 2 * gap;
			mmult = gwdata->FFTLEN / 4;
			for (j = 0; j < gwdata->PASS1_CACHE_LINES; j++) {
				for (u = 0; u < 2; u++) {
					for (m = 0; m < 2; m++) {
						for (k = 0; k < 2 * gap; k += gap) {
							unsigned long word;

							word = haddin + i + j + u * umult + m * mmult + k;
							if (k == 0) *p = is_big_word (gwdata, word) * 16;
							else if (is_big_word (gwdata, word)) *p += 32;

							if (gwfft_weight_exponent (gwdata->dd_data, word) + 0.5 <
							    gwfft_weight_exponent (gwdata->dd_data, word % gap) +
							    gwfft_weight_exponent (gwdata->dd_data, word - word % gap)) {
								if (k == 0) *p += 64;
								else *p += 128;
							}

							if (word == 2)
								((struct gwasm_data *) gwdata->asm_data)->BIGLIT_INCR2 =
									(uint32_t) ((char *) p - (char *) table);
							if (word == 4)
								((struct gwasm_data *) gwdata->asm_data)->BIGLIT_INCR4 =
									(uint32_t) ((char *) p - (char *) table);
						}
						p++;
					}
				}
			}
		}
	}
	return ((double *) p);
}

double *r4dwpn_build_biglit_table (
	gwhandle *gwdata,	
	double	*table)		
{
const	int	START_OF_CHAIN = 0x8000;
const	int	END_OF_CHAIN = 0x4000;
const	int	CHAIN_3_COMMON = 0x2000;
const	int	CHAIN_2_COMMON = 0x1000;
const	int	CHAIN_1_COMMON = 0x0800;
	int	combos[16], next_combo[16];
	unsigned char combo_recorded[256];
	unsigned char *p;
	unsigned long upper_sse2_word, fftlen_over_4, num_cols;
	unsigned long group, i, j, k, m, n, num_combos;

	memset (combo_recorded, 0, sizeof (combo_recorded));
	upper_sse2_word = gwdata->PASS2_SIZE / 2;
	fftlen_over_4 = gwdata->FFTLEN / 4;
	num_combos = 0;

	for (i = 0; i < upper_sse2_word; i++) {
		for (j = 0; j < fftlen_over_4; j += 2*upper_sse2_word) {
			int	combo;

			combo = (is_big_word (gwdata, i + j + upper_sse2_word) << 7) +
				(is_big_word (gwdata, i + j) << 6) +
				(is_big_word (gwdata, i + j + fftlen_over_4 + upper_sse2_word) << 5) +
				(is_big_word (gwdata, i + j + fftlen_over_4) << 4) +
				(is_big_word (gwdata, i + j + 2*fftlen_over_4 + upper_sse2_word) << 3) +
				(is_big_word (gwdata, i + j + 2*fftlen_over_4) << 2) +
				(is_big_word (gwdata, i + j + 3*fftlen_over_4 + upper_sse2_word) << 1) +
				(is_big_word (gwdata, i + j + 3*fftlen_over_4));

			if (! combo_recorded[combo]) {
				combo_recorded[combo] = 1;
				combos[num_combos++] = combo;
			}
		}
	}

	for (i = 0; i < num_combos; i++)
		next_combo[i] = START_OF_CHAIN + END_OF_CHAIN;

	for (n = 3; n != 0; n--) {

		for (i = 0; i < num_combos; i++) {
			int	chain_end;

			if (! (next_combo[i] & START_OF_CHAIN)) continue;

			for (chain_end = i; ! (next_combo[chain_end] & END_OF_CHAIN); chain_end = next_combo[chain_end] & 0xFF);

			for (j = 0; j < num_combos; j++) {

				if (! (next_combo[j] & END_OF_CHAIN)) continue;
				
				if (j == chain_end) continue;

				if (n == 3 && (combos[i] >> 2) == (combos[j] & 0x3F)) {
					next_combo[j] = (next_combo[j] & START_OF_CHAIN) + CHAIN_3_COMMON + i;
					next_combo[i] &= ~START_OF_CHAIN;
					break;
				}
				if (n == 2 && (combos[i] >> 4) == (combos[j] & 0x0F)) {
					next_combo[j] = (next_combo[j] & START_OF_CHAIN) + CHAIN_2_COMMON + i;
					next_combo[i] &= ~START_OF_CHAIN;
					break;
				}
				if (n == 1 && (combos[i] >> 6) == (combos[j] & 0x03)) {
					next_combo[j] = (next_combo[j] & START_OF_CHAIN) + CHAIN_1_COMMON + i;
					next_combo[i] &= ~START_OF_CHAIN;
					break;
				}
			}
		}
	}

	n = 0;
	for (i = 0; i < num_combos; i++) {
		if (! (next_combo[i] & START_OF_CHAIN)) continue;

		for (j = i; ; j = next_combo[j] & 0xFF) {
			((char *)gwdata->ASM_TIMERS)[n++] = (combos[j] >> 6);
			if (! (next_combo[j] & CHAIN_3_COMMON))
				((char *)gwdata->ASM_TIMERS)[n++] = (combos[j] >> 4) & 3;
			if (! (next_combo[j] & (CHAIN_3_COMMON + CHAIN_2_COMMON)))
				((char *)gwdata->ASM_TIMERS)[n++] = (combos[j] >> 2) & 3;
			if (! (next_combo[j] & (CHAIN_3_COMMON + CHAIN_2_COMMON + CHAIN_1_COMMON)))
				((char *)gwdata->ASM_TIMERS)[n++] = combos[j] & 3;
			if (next_combo[j] & END_OF_CHAIN) break;
		}
	}
	GWASSERT (n <= 48);		
	if (n > 48) gwdata->GWERROR = GWERROR_INTERNAL + 6;

	num_cols = gwdata->PASS2_SIZE * gwdata->wpn_count;

	p = (unsigned char *) table;
	for (group = 0; group < upper_sse2_word; group += gwdata->PASS1_CACHE_LINES) {
	    for (n = 0; n < gwdata->FFTLEN / 4; n += num_cols) {
	        for (j = 0; j < num_cols; j += gwdata->PASS2_SIZE) {
		    for (i = 0; i < gwdata->PASS1_CACHE_LINES; i++) {
			for (k = 0; k < gwdata->FFTLEN; k += gwdata->FFTLEN / 4) {
			    for (m = 0; m < gwdata->PASS2_SIZE; m += upper_sse2_word) {
				unsigned long word;

				word = group + j + n + i + k + m;
				if (m == 0) *p = is_big_word (gwdata, word) * 16;
				else if (is_big_word (gwdata, word)) *p += 32;

				if (gwfft_weight_exponent (gwdata->dd_data, word) + 0.5 <
				    gwfft_weight_exponent (gwdata->dd_data, n + k + m) +
				    gwfft_weight_exponent (gwdata->dd_data, group + j + i)) {
					if (m == 0) *p += 64;
					else *p += 128;
				}

				if (word == 2)
					((struct gwasm_data *) gwdata->asm_data)->BIGLIT_INCR2 =
						(uint32_t) ((char *) p - (char *) table);
				if (word == 4)
					((struct gwasm_data *) gwdata->asm_data)->BIGLIT_INCR4 =
						(uint32_t) ((char *) p - (char *) table);
			    }

			    if (*p >= 128) *p -= 64;

			    p++;
			}

			p -= 4;
			for (m = 0; m <= 44; m++) {
				if (((char *)gwdata->ASM_TIMERS)[m]   == (p[0] & 48) >> 4 &&
				    ((char *)gwdata->ASM_TIMERS)[m+1] == (p[1] & 48) >> 4 &&
				    ((char *)gwdata->ASM_TIMERS)[m+2] == (p[2] & 48) >> 4 &&
				    ((char *)gwdata->ASM_TIMERS)[m+3] == (p[3] & 48) >> 4)
					break;
			}
			ASSERTG (m != 45);
			if (m == 45) gwdata->GWERROR = GWERROR_INTERNAL + 7;

			p[0] = ((p[1] >> 6) + (p[0] >> 6)) * 16 + ((p[3] >> 6) + (p[2] >> 6)) * 2;
			p[1] = (unsigned char) (m << 2);
			p += 2;
		    }
		}
	    }
	}
	return ((double *) p);
}

double *hg_build_sin_cos_table (
	double	*table,		
	unsigned long N,	
				
	int	hermetian_skip,	
	int	type)		
				
{
	unsigned long i;

	if (type == 2 && hermetian_skip) type = 3;

	if (N <= 8) return (table);
	if (N >= 9 && N <= 16) N = N * 2;
	if (N >= 33 && N <= 64 && type == 1 && hermetian_skip) N = N * 2;

	if (!hermetian_skip) N = N / 2;

	for (i = hermetian_skip ? ((N & 4) ? 4 : 8) : 0; i < N; i += 4) {
		unsigned long shifted_i, shifted_N, flipped_i;
		double	sincos[6];

		shifted_i = i; shifted_N = N; flipped_i = 0;
		while ((shifted_N & 1) == 0) {
			flipped_i <<= 1;
			if (shifted_i & 1) flipped_i++;
			shifted_i >>= 1;
			shifted_N >>= 1;
		}
		flipped_i = (flipped_i * shifted_N) + shifted_i;

		if (hermetian_skip) {
			if (shifted_i > shifted_N / 2) continue;
			if (shifted_i == 0) {
				unsigned long j;
				for (j = i; j > 3; j >>= 1);
				if (j == 3) continue;
			}
		}

		gwsincos123by1 (flipped_i, N, (double *) &sincos);

		if (type == 0) {
			memcpy (table, sincos, sizeof (sincos));
			table += 6;
		} else if (type == 1) {
			table[0] = table[1] = sincos[0];
			table[2] = table[3] = sincos[1];
			table[4] = table[5] = sincos[2];
			table[6] = table[7] = sincos[3];
			table[8] = table[9] = sincos[4];
			table[10] = table[11] = sincos[5];
			table += 12;
		} else if (type == 2) {
			table[0] = sincos[0];
			table[2] = sincos[1];
			table[4] = sincos[2];
			table[6] = sincos[3];
			table[8] = sincos[4];
			table[10] = sincos[5];
			type++;
		} else {
			table[1] = sincos[0];
			table[3] = sincos[1];
			table[5] = sincos[2];
			table[7] = sincos[3];
			table[9] = sincos[4];
			table[11] = sincos[5];
			type--;
			table += 12;
		}
	}
	return (table);
}

double *hg_build_premult_table (
	gwhandle *gwdata,	
	double	*table)		
{
	unsigned long i, N, incr, type, m_fudge;

	N = gwdata->FFTLEN;
	incr = gwdata->PASS2_SIZE;
	if (gwdata->NEGACYCLIC_FFT) N = N / 2;

 	if (gwdata->NEGACYCLIC_FFT) i = 0;
	else i = incr * 4;

	m_fudge = N / gwdata->PASS2_SIZE;

	type = 0;
	for ( ; i < N; i += incr) {
		unsigned long shifted_i, shifted_N, flipped_i, k, l, m;
		unsigned long grouping_size;
		double	*table_start;
		double	sincos[2];

		shifted_i = i; shifted_N = N; flipped_i = 0;
		while ((shifted_N & 1) == 0) {
			flipped_i <<= 1;
			if (shifted_i & 1) flipped_i++;
			shifted_i >>= 1;
			shifted_N >>= 1;
		}
		flipped_i = (flipped_i * shifted_N) + shifted_i;

		if (!gwdata->NEGACYCLIC_FFT) {
			if (shifted_i > shifted_N / 2) continue;
			if (shifted_i == 0) {
				unsigned long j;
				for (j = i; j > 3; j >>= 1);	
				if (j == 3) continue;
			}
		}

		grouping_size = 4;
		if (gwdata->PASS2_SIZE == 1024) grouping_size = 8;
		if (gwdata->PASS2_SIZE == 2048) grouping_size = 8;
		if (gwdata->PASS2_SIZE == 4096) grouping_size = 16;
		if (gwdata->PASS2_SIZE == 8192) grouping_size = 16;
		table_start = table;
		for (k = 0; k < incr / 4; k += grouping_size) {

			for (l = 0; l < 4; l++) {
				unsigned long real_k, pm;

				real_k = l * incr/4 + k;
				pm = real_k * flipped_i;
				if (!gwdata->NEGACYCLIC_FFT) {
					gwsincos (pm % N, N, (double *) &sincos);
				}

				else {
					pm = pm * 4 + real_k;
					gwsincos (pm % (N*4), N*4, (double *) &sincos);
				}

				table[l*4+type] = sincos[0];
				table[l*4+2+type] = sincos[1];
			}
			table += 16;
		}
	
		for (m = 0; m < 4; m++) {
		    unsigned long flipped_m;
		    flipped_m = ((m & 1) << 1) + ((m & 2) >> 1);
		    for (k = 0; k < grouping_size; k++) {
			for (l = 0; l < 4; l++) {
				unsigned long pm;

				pm = k * flipped_i +
				     l * flipped_m * N/16 +
				     (k & 3) * flipped_m * m_fudge;
				if (!gwdata->NEGACYCLIC_FFT) {
					gwsincos (pm % N, N, (double *) &sincos);
				}

				else {
					pm = pm * 4 + k;
					gwsincos (pm % (N*4), N*4, (double *) &sincos);
				}

				table[l*4+type] = sincos[0];
				table[l*4+2+type] = sincos[1];
			}
			table += 16;
		    }
		}

		if (type == 0) table = table_start;
		type = 1 - type;
 	}

	return (table);
}

double *hg_build_plus1_table (
	gwhandle *gwdata,	
	double	*table)		
{
	unsigned long i, j, k, l, N;
	int	pfa;

	pfa = (gwdata->FFTLEN != pow_two_above_or_equal (gwdata->FFTLEN));

	if (gwdata->PASS2_SIZE == 0) N = gwdata->FFTLEN;
	else N = gwdata->PASS1_SIZE * 2;

	for (i = 0; i < N / (pfa ? 24 : 32); i++) {
	for (l = 0; l < 2; l++) {
		double	sincos[2];

		for (k = 0; k < (unsigned long) (pfa ? 3 : 4); k++) {
		for (j = 0; j < 2; j++) {
			long	temp;

			if (pfa)
				temp = (long) ((i * 2 + l * N/12 + j + k * N/6) % N);
			else
				temp = (long) ((i * 4 + l * 2 + j + k * N/8) % N);
			gwsincos (temp, N*2, (double *) &sincos);

			table[0+j] = sincos[0];
			table[2+j] = sincos[1];

			if (gwdata->PASS2_SIZE) {
				j = 1;
				table[0+j] = sincos[0];
				table[2+j] = sincos[1];
			}
		}
		table += 4;
		}
	}
 	}

	return (table);
}

double *hg_build_norm_table (
	gwhandle *gwdata,	
	double	*table,		
	int	col)		
{
	unsigned long i, k, num_cols;

	if (gwdata->PASS2_SIZE == 0) {
		if (!col) return (table);

		for (i = 0; i < gwdata->FFTLEN; i++) {
			unsigned long j, table_entry;
			double	ttp, ttmp;

			gwfft_weights3 (gwdata->dd_data, i, &ttp, NULL, &ttmp);

			j = addr_offset (gwdata, i) / sizeof (double);
			table_entry = j >> 1;

			table[table_entry*4+(j&1)] = ttmp;
			table[table_entry*4+2+(j&1)] = ttp;
		}
		return (table + gwdata->FFTLEN + gwdata->FFTLEN);
	}

	num_cols = gwdata->PASS2_SIZE / 2;
	if (col) {

		for (i = 0; i < num_cols; i++) {
			double	ttp, ttmp;

			gwfft_weights3 (gwdata->dd_data, i, &ttp, NULL, &ttmp);

			table[i*4] = ttmp;
			table[i*4+1] = ttmp;
			table[i*4+2] = ttp;
			table[i*4+3] = ttp;
		}
		return (table + num_cols * 4);
	}

	else {
		unsigned long pfa, h, hlimit, haddin, m, mmult, u, umult;

		for (pfa = gwdata->FFTLEN; pfa > 8; pfa >>= 1);

		umult = gwdata->FFTLEN / 2;
		hlimit = gwdata->FFTLEN / 4 / (2*num_cols);
		for (h = 0; h < hlimit; h++) {
			if (pfa == 5) {
				if (h < hlimit / 5) {
					haddin = h * 2 * num_cols;
					mmult = gwdata->FFTLEN / 20;
				} else {
					haddin = gwdata->FFTLEN/10 + (h - hlimit/5) * 2 * num_cols;
					mmult = gwdata->FFTLEN / 5;
				}
			} else if (pfa == 7) {
				if (h < hlimit / 7) {
					haddin = h * 2 * num_cols;
					mmult = gwdata->FFTLEN / 28;
				} else if (h < 3 * hlimit / 7) {
					haddin = gwdata->FFTLEN/14 + (h - hlimit/7) * 2 * num_cols;
					mmult = gwdata->FFTLEN / 14;
				} else {
					haddin = 3*gwdata->FFTLEN/14 + (h - 3*hlimit/7) * 2 * num_cols;
					mmult = gwdata->FFTLEN / 7;
				}
			} else {
				haddin = h * 2 * num_cols;
				mmult = gwdata->FFTLEN / 4;
			}
			for (u = 0; u < 2; u++) {
			for (m = 0; m < 2; m++) {
			for (k = 0; k < 2; k++) {
				double	ttp, ttmp;
				long	n;

				n = haddin + u * umult + m * mmult + k * num_cols;
				gwfft_weights3 (gwdata->dd_data, n, &ttp, &ttmp, NULL);

				table[k] = ttmp;
				table[2+k] = ttp;
			}
			table += 4;
			}
			}
		}
		return (table);
	}
}

double *hg_build_biglit_table (
	gwhandle *gwdata,	
	double	*table)		
{
	unsigned char *p;
	unsigned long h, i, j, k, m, u, gap;
	unsigned long pfa, hlimit, haddin, mmult, umult;

	if (gwdata->PASS2_SIZE == 0) {

		p = (unsigned char *) table;
		for (i = 0; i < gwdata->FFTLEN; i++) {
			unsigned long table_entry;

			j = addr_offset (gwdata, i) / sizeof (double);
			table_entry = j >> 1;

			if ((j & 1) == 0) {
				p[table_entry] = is_big_word (gwdata, i) * 16;
			}

			else {
				if (is_big_word (gwdata, i)) p[table_entry] += 32;
			}
		}
		return ((double *) (p + gwdata->FFTLEN / 2));
	}

	for (pfa = gwdata->FFTLEN; pfa > 8; pfa >>= 1);

	gap = gwdata->PASS2_SIZE / 2;

	p = (unsigned char *) table;
	umult = gwdata->FFTLEN / 2;
	hlimit = gwdata->FFTLEN / 4 / (2*gap);
	for (i = 0; i < gap; i += gwdata->PASS1_CACHE_LINES) {
	for (h = 0; h < hlimit; h++) {
		if (pfa == 5) {
			if (h < hlimit / 5) {
				haddin = h * 2 * gap;
				mmult = gwdata->FFTLEN / 20;
			} else {
				haddin = gwdata->FFTLEN/10 + (h - hlimit/5) * 2 * gap;
				mmult = gwdata->FFTLEN / 5;
			}
		} else if (pfa == 7) {
			if (h < hlimit / 7) {
				haddin = h * 2 * gap;
				mmult = gwdata->FFTLEN / 28;
			} else if (h < 3 * hlimit / 7) {
				haddin = gwdata->FFTLEN/14 + (h - hlimit/7) * 2 * gap;
				mmult = gwdata->FFTLEN / 14;
			} else {
				haddin = 3*gwdata->FFTLEN/14 + (h - 3*hlimit/7) * 2 * gap;
				mmult = gwdata->FFTLEN / 7;
			}
		} else {
			haddin = h * 2 * gap;
			mmult = gwdata->FFTLEN / 4;
		}
	for (j = 0; j < gwdata->PASS1_CACHE_LINES; j++) {
	for (u = 0; u < 2; u++) {
	for (m = 0; m < 2; m++) {
	for (k = 0; k < 2 * gap; k += gap) {
		unsigned long word;

		word = haddin + i + j + u * umult + m * mmult + k;
		if (k == 0) *p = is_big_word (gwdata, word) * 16;
		else if (is_big_word (gwdata, word)) *p += 32;

		if (gwfft_weight_exponent (gwdata->dd_data, word) + 0.5 <
		    gwfft_weight_exponent (gwdata->dd_data, word % gap) +
		    gwfft_weight_exponent (gwdata->dd_data, word - word % gap)) {
			if (k == 0) *p += 64;
			else *p += 128;
		}

		if (word == 2)
			((struct gwasm_data *) gwdata->asm_data)->BIGLIT_INCR2 =
				(uint32_t) ((char *) p - (char *) table);
		if (word == 4)
			((struct gwasm_data *) gwdata->asm_data)->BIGLIT_INCR4 =
				(uint32_t) ((char *) p - (char *) table);
	}
	p++;
	}
	}
	}
	}
	}
	return ((double *) p);
}

#ifndef X86_64

double *x87_build_sin_cos_table (
	double	*table,		
	unsigned long N,
	int	hermetian_skip)	
{
	unsigned long i;

	if (N <= 8) return (table);
	if (N >= 9 && N <= 16) N = N * 2;

	if (!hermetian_skip) N = N / 2;

	for (i = hermetian_skip ? ((N & 4) ? 4 : 8) : 0; i < N; i += 4) {
		unsigned long shifted_i, shifted_N, flipped_i;
		double	sincos[6];

		shifted_i = i; shifted_N = N; flipped_i = 0;
		while ((shifted_N & 1) == 0) {
			flipped_i <<= 1;
			if (shifted_i & 1) flipped_i++;
			shifted_i >>= 1;
			shifted_N >>= 1;
		}
		flipped_i = (flipped_i * shifted_N) + shifted_i;

		if (hermetian_skip) {
			if (shifted_i > shifted_N / 2) continue;
			if (shifted_i == 0) {
				unsigned long j;
				for (j = i; j > 3; j >>= 1);
				if (j == 3) continue;
			}
		}

		gwsincos123by1 (flipped_i, N, (double *) &sincos);

		memcpy (table, sincos, sizeof (sincos));
		table += 6;
	}
	return (table);
}

double *x87_build_premult_table (
	gwhandle *gwdata,	
	double	*table)		
{
	unsigned long i, N, incr;

	N = gwdata->FFTLEN;
	incr = gwdata->PASS2_SIZE;
	if (gwdata->NEGACYCLIC_FFT) N = N / 2;

	if (gwdata->NEGACYCLIC_FFT) i = 0;
	else i = incr * 2;

	for ( ; i < N; i += incr) {
		unsigned long shifted_i, shifted_N, flipped_i, k, l;
		double	sincos[2];

		shifted_i = i; shifted_N = N; flipped_i = 0;
		while ((shifted_N & 1) == 0) {
			flipped_i <<= 1;
			if (shifted_i & 1) flipped_i++;
			shifted_i >>= 1;
			shifted_N >>= 1;
		}
		flipped_i = (flipped_i * shifted_N) + shifted_i;

		if (!gwdata->NEGACYCLIC_FFT) {
			if (shifted_i > shifted_N / 2) continue;
			if (shifted_i == 0) {
				unsigned long j;
				for (j = i; j > 3; j >>= 1);	
				if (j == 3) continue;
			}
		}

		for (k = 0; k < incr / 4; k += 4) {

			for (l = 0; l < 4; l++) {

				if (!gwdata->NEGACYCLIC_FFT) {
					gwsincos (((l * incr/4 + k) * flipped_i) % N, N, (double *) &sincos);
				}

				else {
					gwsincos (((l * incr/4 + k) * flipped_i * 4 + l*incr/4+k) % (N*4), N*4, (double *) &sincos);
				}

				table[l*2] = sincos[0];
				table[l*2+1] = sincos[1];
			}
			table += 8;
		}
	
		for (k = 0; k < 4; k++) {
			for (l = 0; l < 4; l++) {

				if (!gwdata->NEGACYCLIC_FFT) {
					gwsincos ((k * flipped_i + l * N/16) % N, N, (double *) &sincos);
				}

				else {
					gwsincos (((k * flipped_i * 2 + l * N/8) *2 + k) % (N*4), N*4, (double *) &sincos);
				}

				table[l*2] = sincos[0];
				table[l*2+1] = sincos[1];
			}
			table += 8;
		}
 	}

	return (table);
}

double *x87_build_plus1_table (
	gwhandle *gwdata,	
	double	*table)		
{
	unsigned long i, k, N;
	int	pfa;

	pfa = (gwdata->FFTLEN != pow_two_above_or_equal (gwdata->FFTLEN));

	if (gwdata->PASS2_SIZE == 0) N = gwdata->FFTLEN;
	else N = gwdata->PASS1_SIZE;

	for (i = 0; i < N / (pfa ? 6 : 8); i++) {
		double	sincos[2];

		for (k = 0; k < (unsigned long) (pfa ? 3 : 4); k++) {
			long	temp;

			if (pfa)
				temp = (long) ((i + k * N/6) % N);
			else
				temp = (long) ((i + k * N/8) % N);
			gwsincos (temp, N*2, (double *) &sincos);

			table[0] = sincos[0];
			table[1] = sincos[1];
			table += 2;
		}
	}

	return (table);
}

double *x87_build_norm_table (
	gwhandle *gwdata,	
	double	*table,		
	int	col)		
{
	unsigned long i, k, num_cols;

	if (gwdata->PASS2_SIZE == 0) {
		if (!col) return (table);

		for (i = 0; i < gwdata->FFTLEN; i++) {
			unsigned long j;
			double	ttp, ttmp;

			gwfft_weights3 (gwdata->dd_data, i, &ttp, NULL, &ttmp);

			j = addr_offset (gwdata, i) / sizeof (double);

			table[j*2] = ttmp;
			table[j*2+1] = ttp;
		}
		return (table + gwdata->FFTLEN + gwdata->FFTLEN);
	}

	num_cols = gwdata->PASS2_SIZE;
	if (col) {

		for (i = 0; i < num_cols; i++) {
			double	ttp, ttmp;

			gwfft_weights3 (gwdata->dd_data, i, &ttp, NULL, &ttmp);

			table[i+i] = ttmp;
			table[i+i+1] = ttp;
		}
		return (table + num_cols * 2);
	}

	else {
		unsigned long num_grps;
		
		num_grps = gwdata->FFTLEN / num_cols;
		for (i = 0; i < num_grps; i++) {
			double	ttp, ttmp;

			gwfft_weights3 (gwdata->dd_data, i * num_cols, &ttp, &ttmp, NULL);

			if (i < num_grps / 2) k = i * 2;
			else k = (i - num_grps / 2) * 2 + 1;
			table[k+k] = ttmp;
			table[k+k+1] = ttp;
		}
		return (table + num_grps * 2);
	}
}

double *x87_build_biglit_table (
	gwhandle *gwdata,	
	double	*table)		
{
	unsigned char *p;
	unsigned long i, j, k, m;

	if (gwdata->PASS2_SIZE == 0) {

		p = (unsigned char *) table;
		for (i = 0; i < gwdata->FFTLEN; i++) {
			unsigned long table_entry;

			j = addr_offset (gwdata, i) / sizeof (double);
			table_entry = j >> 1;

			if ((j & 1) == 0) {
				p[table_entry] = is_big_word (gwdata, i) * 16;
			}

			else {
				if (is_big_word (gwdata, i)) p[table_entry] += 32;
			}
		}
		return ((double *) (p + gwdata->FFTLEN / 2));
	}

	p = (unsigned char *) table;
	for (i = 0; i < gwdata->PASS2_SIZE; i += gwdata->PASS1_CACHE_LINES * 2) {
	for (j = 0; j < gwdata->FFTLEN / 2; j += gwdata->PASS2_SIZE) {
	for (k = 0; k < gwdata->PASS1_CACHE_LINES * 2; k++) {
	for (m = 0; m < gwdata->FFTLEN; m += gwdata->FFTLEN / 2) {
		unsigned long word;

		word = i + j + k + m;
		if (m == 0) *p = is_big_word (gwdata, word) * 16;
		else if (is_big_word (gwdata, word)) *p += 32;

		if (gwfft_weight_exponent (gwdata->dd_data, word) + 0.5 <
		    gwfft_weight_exponent (gwdata->dd_data, word % gwdata->PASS2_SIZE) +
		    gwfft_weight_exponent (gwdata->dd_data, word - word % gwdata->PASS2_SIZE)) {
			if (m == 0) *p += 64;
			else *p += 128;
		}

		if (word == 2)
			((struct gwasm_data *) gwdata->asm_data)->BIGLIT_INCR2 =
				(uint32_t) ((char *) p - (char *) table);
		if (word == 4)
			((struct gwasm_data *) gwdata->asm_data)->BIGLIT_INCR4 =
				(uint32_t) ((char *) p - (char *) table);
	}
	p++;
	}
	}
	}
	return ((double *) p);
}

#endif
