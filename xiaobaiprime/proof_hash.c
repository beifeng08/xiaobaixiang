// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

#include "memory.h"
#include "sha3.h"
#include "sha3.c"

typedef unsigned char hash256_t[32];		

int sha3_hash_and_gwnum (
	gwhandle *gwdata,
	hash256_t *prepend_hash,
	gwnum	x,
	hash256_t *result)
{
	int	residue_size, arraylen, i, len;
	uint32_t *array = NULL;
	sha3_ctx_t sha3;
	int	retcode = 0;

	sha3_init (&sha3, 32);
	if (prepend_hash != NULL) sha3_update (&sha3, prepend_hash, 32);

	residue_size = divide_rounding_up ((int) ceil (gwdata->bit_length), 8);
	arraylen = divide_rounding_up (residue_size, 4);
	array = (uint32_t *) malloc (divide_rounding_up (residue_size, 4) * sizeof(uint32_t));
	if (array == NULL) goto err;

	len = gwtobinary (gwdata, x, array, arraylen);
	if (len < 0) goto err;

	for (i = 0; i < len; i++) {
		uint32_t val = array[i];
		((unsigned char *)&array[i])[0] = val & 0xFF;
		((unsigned char *)&array[i])[1] = (val >> 8) & 0xFF;
		((unsigned char *)&array[i])[2] = (val >> 16) & 0xFF;
		((unsigned char *)&array[i])[3] = (val >> 24) & 0xFF;
	}
	for ( ; i < (int) arraylen; i++) array[i] = 0;

	sha3_update (&sha3, array, residue_size);
	sha3_final (result, &sha3);

	retcode = 1;

err:	if (array != NULL) free (array);
	return (retcode);
}

int sha3_gwnum (
	gwhandle *gwdata,
	gwnum	x,
	hash256_t *result)
{
	return (sha3_hash_and_gwnum (gwdata, NULL, x, result));
}

int roothash (
	gwhandle *gwdata,
	gwnum	x,
	hash256_t *result)
{
	return (sha3_hash_and_gwnum (gwdata, NULL, x, result));
}

int hash (
	gwhandle *gwdata,
	hash256_t *prevh,
	gwnum	x,
	hash256_t *result)
{
	return (sha3_hash_and_gwnum (gwdata, prevh, x, result));
}

uint64_t truncate_hash (
	hash256_t h,			
	int	hashlen)		
{
	int	i;
	uint64_t temp64 = 0;

	for (i = 7; i >= 0; i--) temp64 = (temp64 << 8) + h[i];

	if (hashlen >= 64) return (temp64);
	return (temp64 & ((1ULL << hashlen) - 1));
}

char *hash_to_string (
	hash256_t h)			
{
	int	i;
static	char retbuf[65];		

	for (i = 31; i >= 0; i--) sprintf (retbuf+(31-i)*2, "%02X", (int) h[i]);
	return (retbuf);
}
