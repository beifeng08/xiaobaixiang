/*----------------------------------------------------------------------
| gwbench.h
|
| This file contains routines to read, write, and use benchmarking data stored
| in the gwnum.txt INI file.
|
| Benchmarking data lets us select the fastest FFT implementation for every CPU.
| By default, gwnum picks a default FFT implementation based on the CPU architecture.
| We can fine-tune performance by benchmarking the different FFT implementations
| on the end user machine looking for an FFT implementation that is faster than the
| default selection.
|
| Copyright 2017-2024 Mersenne Research, Inc.  All rights reserved.
+---------------------------------------------------------------------*/

#ifndef _GWBENCH_H
#define _GWBENCH_H

#ifdef __cplusplus
extern "C" {
#endif

#define GWNUMINI_FILE	"gwnum.txt"			

#define GWNUM_FFT_IMPL_VERSION		"30.16"		
							
#define GWNUM_FFT_IMPL_VERSION_BAD_SSE2	"29.2"		

extern int BENCH_NUM_CORES;		
extern int BENCH_NUM_WORKERS;		

#define GWBENCH_ADD_VERSION		1
struct gwbench_add_struct {
	int	version;		
	double	throughput;		
	double	bench_length;		
	int	num_cores;		
	int	num_workers;		
	int	num_hyperthreads;	
	int	error_checking;		
};
void gwbench_add_data (gwhandle *, struct gwbench_add_struct *);
void gwbench_write_data (void);
void gwbench_get_num_benchmarks (double, unsigned long, unsigned long, signed long, unsigned long, int, int, int, int, unsigned long *, int *, int *);

void gwbench_read_data (int);
int gwbench_implementation_id (gwhandle *, int);
int internal_implementation_id (int, int, int, int, int, int, int, int, int);
int internal_implementation_ids_match (int, int, int, int, int, int, int, int);
void gwbench_get_max_throughput (int, int, int, int, int, int, int, int *, double *);

#ifdef __cplusplus
}
#endif

#endif
