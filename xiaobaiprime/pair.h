/*----------------------------------------------------------------------
| pair.h
|
| This file contains forward declarations for pair.cpp
| 
|  Copyright 2021 Mersenne Research, Inc.
|  All Rights Reserved.
+---------------------------------------------------------------------*/

#ifndef _PAIR_H
#define _PAIR_H

void estimate_pairing (
	int	second_missing_prime,
	uint64_t B1,
	uint64_t B2,
	int	totrels,
	int	numrels,
	float	*pair_pct,
	float	*timing);

uint64_t estimate_max_pairmap_Dsections (
	double	max_pairmap_size,	
	uint64_t numDsections,		
	double	est_numpairs);		

int fill_pairmap (			
	int	thread_num,		
	void	**sieve_info,		
	int	D,			
	int	WINDOW_SIZE,		
	int	ALT_D,			
	int	ALT_D_OFFSET,		
	int	ALT_MORE_RELPS,		
	int	totrels,		
	int16_t	*relp_sets,				
	uint64_t first_relocatable,	
	uint64_t last_relocatable,	
	uint64_t B2_start,		
	uint64_t B2,			
	uint64_t max_pairmap_Dsections,	
	uint8_t	**pairmap,		
	uint64_t *pairmap_size);	

uint32_t next_pair (			
	uint8_t	**pairmap_ptr);		

#endif
