/*----------------------------------------------------------------------
| gwbench.c
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

/* Include files */

#ifdef __linux__		
#define _GNU_SOURCE
#define __USE_UNIX98
#include <sys/mman.h>
#endif
#include <stdio.h>
#include "cpuid.h"
#include "gwnum.h"
#include "gwbench.h"
#include "gwini.h"
#include "gwthread.h"

#include "sqlite3.c"

int	BENCH_DB_INITIALIZED = 0;
gwmutex	SQL_MUTEX;				
sqlite3 *BENCH_DB = NULL;			
int	get_max_sql_stmt_prepared = FALSE;	
sqlite3_stmt *get_max_sql_stmt;

int	BENCH_NUM_CORES = 0;
int	BENCH_NUM_WORKERS = 0;

void gwbench_read_data (int cpu_flags)
{
	char	sqlite_file[80];		
	char	gwnum_version_string[10];
	char	cpuid_brand_string[49];
	char	bench_data[250];
	int	i, errcode;
	sqlite3_stmt *sql_stmt;

	if (BENCH_DB_INITIALIZED) return;
	BENCH_DB_INITIALIZED = 1;

	gwmutex_init (&SQL_MUTEX);
	gwmutex_lock (&SQL_MUTEX);

	BENCH_NUM_CORES = IniGetInt (GWNUMINI_FILE, "BenchCores", 0);
	BENCH_NUM_WORKERS = IniGetInt (GWNUMINI_FILE, "BenchWorkers", 0);

	IniGetString (GWNUMINI_FILE, "SQLiteFile", sqlite_file, sizeof (sqlite_file), ":memory:");
	errcode = sqlite3_open_v2 (sqlite_file, &BENCH_DB, SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
	if (errcode != SQLITE_OK) goto db_error;

	errcode = sqlite3_exec (BENCH_DB,
				"CREATE TABLE bench_data (fftlen INT, num_cores INT, num_workers INT, num_hyperthreads INT, \
							  impl INT, bench_date DATE, bench_length INT, throughput REAL)",
				NULL, NULL, NULL);
	if (errcode != SQLITE_OK) goto db_error;

	IniGetString (GWNUMINI_FILE, "GwnumVersion", gwnum_version_string, sizeof (gwnum_version_string), NULL);
	if (strcmp (gwnum_version_string, GWNUM_FFT_IMPL_VERSION)) {
		if (strcmp (gwnum_version_string, GWNUM_FFT_IMPL_VERSION_BAD_SSE2)) goto empty_the_db;
		if (! (cpu_flags & (CPU_AVX512F | CPU_FMA3 | CPU_AVX))) goto empty_the_db;
	}

	IniGetString (GWNUMINI_FILE, "CpuBrand", cpuid_brand_string, sizeof (cpuid_brand_string), NULL);
	if (strcmp (cpuid_brand_string, CPU_BRAND)) goto empty_the_db;

	errcode = sqlite3_prepare_v2 (BENCH_DB, "INSERT INTO bench_data VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8)", -1, &sql_stmt, NULL);
	if (errcode != SQLITE_OK) goto stmt_error;

	for (i = 1; ; i++) {
		int	fftlen, bench_length, num_cores, num_workers, num_hyperthreads, impl_id;
		char	fftlen_multiplier, bench_date[80];
		double	throughput;

		IniGetNthString (GWNUMINI_FILE, "BenchData", i, bench_data, sizeof (bench_data), NULL);
		if (bench_data[0] == 0) break;

		sscanf (bench_data, "%d%c,%d,%d,%d,%08X,%10s,%d,%lf",
			&fftlen, &fftlen_multiplier, &num_cores, &num_workers, &num_hyperthreads,
			&impl_id, bench_date, &bench_length, &throughput);
		if (fftlen_multiplier == ',') 
			sscanf (bench_data, "%d,%d,%d,%d,%08X,%10s,%d,%lf",
				&fftlen, &num_cores, &num_workers, &num_hyperthreads,
				&impl_id, bench_date, &bench_length, &throughput);
		if (fftlen_multiplier == 'K' || fftlen_multiplier == 'k') fftlen <<= 10;
		if (fftlen_multiplier == 'M' || fftlen_multiplier == 'm') fftlen <<= 20;

		if (num_cores < 1 || num_workers < 1 || throughput <= 0.0) continue;

		errcode = sqlite3_bind_int (sql_stmt, 1, fftlen);
		if (errcode != SQLITE_OK) goto stmt_error;

		errcode = sqlite3_bind_int (sql_stmt, 2, num_cores);
		if (errcode != SQLITE_OK) goto stmt_error;

		errcode = sqlite3_bind_int (sql_stmt, 3, num_workers);
		if (errcode != SQLITE_OK) goto stmt_error;

		errcode = sqlite3_bind_int (sql_stmt, 4, num_hyperthreads);
		if (errcode != SQLITE_OK) goto stmt_error;

		errcode = sqlite3_bind_int (sql_stmt, 5, impl_id);
		if (errcode != SQLITE_OK) goto stmt_error;

		errcode = sqlite3_bind_text (sql_stmt, 6, bench_date, -1, SQLITE_TRANSIENT);
		if (errcode != SQLITE_OK) goto stmt_error;

		errcode = sqlite3_bind_int (sql_stmt, 7, bench_length);
		if (errcode != SQLITE_OK) goto stmt_error;

		errcode = sqlite3_bind_double (sql_stmt, 8, throughput);
		if (errcode != SQLITE_OK) goto stmt_error;

		errcode = sqlite3_step (sql_stmt);
		if (errcode != SQLITE_DONE) goto stmt_error;

		errcode = sqlite3_reset (sql_stmt);
		if (errcode != SQLITE_OK) goto stmt_error;
	}
	sqlite3_finalize (sql_stmt);

empty_the_db:
	errcode = sqlite3_exec (BENCH_DB,
				"CREATE VIEW best3 AS \
					SELECT * FROM bench_data x WHERE rowid IN ( \
						SELECT rowid FROM bench_data y \
						WHERE x.fftlen = y.fftlen AND x.impl = y.impl AND x.num_cores = y.num_cores AND \
						      x.num_workers = y.num_workers AND x.num_hyperthreads = y.num_hyperthreads \
						ORDER BY throughput DESC LIMIT 3)", NULL, NULL, NULL);
	if (errcode != SQLITE_OK) goto db_error;

	errcode = sqlite3_exec (BENCH_DB,
				"CREATE VIEW avgbest3 AS \
					SELECT fftlen, impl, num_cores, num_workers, num_hyperthreads, \
						SUM (throughput * bench_length) / SUM (bench_length) AS avg_throughput \
					FROM best3 GROUP BY fftlen, impl, num_cores, num_workers, num_hyperthreads", NULL, NULL, NULL);
	if (errcode != SQLITE_OK) goto db_error;

	errcode = sqlite3_exec (BENCH_DB, "CREATE INDEX bd_index1 ON bench_data (fftlen, num_workers, impl)", NULL, NULL, NULL);
	if (errcode != SQLITE_OK) goto db_error;

	gwmutex_unlock (&SQL_MUTEX);
	return;

stmt_error:
	sqlite3_finalize (sql_stmt);
db_error:
	sqlite3_close_v2 (BENCH_DB);
	BENCH_DB = NULL;
	gwmutex_unlock (&SQL_MUTEX);
}

void gwbench_write_data (void)
{
	char	bench_data[250];
	int	i, errcode;
	sqlite3_stmt *sql_stmt;

	if (BENCH_DB == NULL) return;

	gwmutex_lock (&SQL_MUTEX);

	IniDelayWrites (GWNUMINI_FILE);
	IniWriteString (GWNUMINI_FILE, "GwnumVersion", GWNUM_FFT_IMPL_VERSION);
	IniWriteString (GWNUMINI_FILE, "CpuBrand", CPU_BRAND);

	errcode = sqlite3_prepare_v2 (BENCH_DB, "SELECT * FROM bench_data ORDER BY 1,2,3,4,5,6", -1, &sql_stmt, NULL);
	if (errcode != SQLITE_OK) goto stmt_error;

	IniWriteNthString (GWNUMINI_FILE, "BenchData", 0, NULL);
	for (i = 1; ; i++) {
		int	fftlen, bench_length, num_cores, num_workers, num_hyperthreads, impl_id;
		const unsigned char *bench_date;
		double	throughput;

		errcode = sqlite3_step (sql_stmt);
		if (errcode == SQLITE_DONE) break;
		if (errcode != SQLITE_ROW) goto stmt_error;

		fftlen = sqlite3_column_int (sql_stmt, 0);
		num_cores = sqlite3_column_int (sql_stmt, 1);
		num_workers = sqlite3_column_int (sql_stmt, 2);
		num_hyperthreads = sqlite3_column_int (sql_stmt, 3);
		impl_id = sqlite3_column_int (sql_stmt, 4);
		bench_date = sqlite3_column_text (sql_stmt, 5);
		bench_length = sqlite3_column_int (sql_stmt, 6);
		throughput = sqlite3_column_double (sql_stmt, 7);

		sprintf (bench_data, "%d%s,%d,%d,%d,%08X,%s,%d,%.2f",
			 (fftlen & 0x3FF) ? fftlen : fftlen >> 10, (fftlen & 0x3FF) ? "" : "K",
			 num_cores, num_workers, num_hyperthreads, impl_id, bench_date, bench_length, throughput);

		IniWriteNthString (GWNUMINI_FILE, "BenchData", i, bench_data);
	}

	IniResumeImmediateWrites (GWNUMINI_FILE);
	sqlite3_finalize (sql_stmt);
	gwmutex_unlock (&SQL_MUTEX);
	return;

stmt_error:
	sqlite3_finalize (sql_stmt);
	sqlite3_close_v2 (BENCH_DB);
	BENCH_DB = NULL;
	gwmutex_unlock (&SQL_MUTEX);
}

void gwbench_add_data (
	gwhandle *gwdata,			
	struct gwbench_add_struct *data)	
{
	int	errcode;
	sqlite3_stmt *sql_stmt;

	if (BENCH_DB == NULL) return;

	gwmutex_lock (&SQL_MUTEX);

	if (get_max_sql_stmt_prepared) {
		get_max_sql_stmt_prepared = FALSE;
		sqlite3_finalize (get_max_sql_stmt);
	}

	errcode = sqlite3_prepare_v2 (BENCH_DB, "INSERT INTO bench_data VALUES (?1, ?2, ?3, ?4, ?5, date('now'), ?6, ?7)", -1, &sql_stmt, NULL);
	if (errcode != SQLITE_OK) goto stmt_error;

	errcode = sqlite3_bind_int (sql_stmt, 1, gwdata->FFTLEN);
	if (errcode != SQLITE_OK) goto stmt_error;

	errcode = sqlite3_bind_int (sql_stmt, 2, data->num_cores);
	if (errcode != SQLITE_OK) goto stmt_error;

	errcode = sqlite3_bind_int (sql_stmt, 3, data->num_workers);
	if (errcode != SQLITE_OK) goto stmt_error;

	errcode = sqlite3_bind_int (sql_stmt, 4, data->num_hyperthreads);
	if (errcode != SQLITE_OK) goto stmt_error;

	errcode = sqlite3_bind_int (sql_stmt, 5, gwbench_implementation_id (gwdata, data->error_checking));
	if (errcode != SQLITE_OK) goto stmt_error;

	errcode = sqlite3_bind_int (sql_stmt, 6, data->bench_length);
	if (errcode != SQLITE_OK) goto stmt_error;

	errcode = sqlite3_bind_double (sql_stmt, 7, data->throughput);
	if (errcode != SQLITE_OK) goto stmt_error;

	errcode = sqlite3_step (sql_stmt);
	if (errcode != SQLITE_DONE) goto stmt_error;

	sqlite3_finalize (sql_stmt);
	gwmutex_unlock (&SQL_MUTEX);
	return;

stmt_error:
	sqlite3_finalize (sql_stmt);
	gwmutex_unlock (&SQL_MUTEX);
}

int gwbench_implementation_id (
	gwhandle *gwdata,			
	int	error_checking)			
{
	int	clm;

	if (gwdata->cpu_flags & CPU_AVX512F) clm = gwdata->PASS1_CACHE_LINES / 8;
	else if (gwdata->cpu_flags & CPU_AVX) clm = gwdata->PASS1_CACHE_LINES / 4;
	else clm = gwdata->PASS1_CACHE_LINES / 2;
	return (internal_implementation_id (gwdata->FFTLEN, gwdata->FFT_TYPE, gwdata->NEGACYCLIC_FFT, gwdata->NO_PREFETCH_FFT,
					    gwdata->IN_PLACE_FFT, error_checking, gwdata->PASS2_SIZE, gwdata->ARCH, clm));
}

int internal_implementation_id (
	int	fftlen,
	int	fft_type,
	int	negacyclic,
	int	no_prefetch,
	int	in_place,
	int	error_check,
	int	pass2_size,
	int	architecture,
	int	clm)
{
	int	pass2_multiplier;		
	int	pass2_pow2;

	if (clm <= 1) clm = 0;
	else if (clm == 2) clm = 1;
	else clm = 2;
	
	if (pass2_size == 0) pass2_multiplier = 0;					
	else if (pass2_size % 9 == 0) pass2_multiplier = 0, pass2_size /= 9;		
	else if (pass2_size % 15 == 0) pass2_multiplier = 2, pass2_size /= 15;		
	else if (pass2_size % 25 == 0) pass2_multiplier = 5, pass2_size /= 25;		
	else if (pass2_size % 21 == 0) pass2_multiplier = 6, pass2_size /= 21;		
	else if (pass2_size % 35 == 0) pass2_multiplier = 7, pass2_size /= 35;		
	else if (pass2_size % 49 == 0) pass2_multiplier = 8, pass2_size /= 49;		
	else if (pass2_size % 12 == 0) pass2_multiplier = 1, pass2_size /= 12;		
	else if (pass2_size % 20 == 0) pass2_multiplier = 4, pass2_size /= 20;		
	else if (pass2_size % 28 == 0) pass2_multiplier = 9, pass2_size /= 28;		
	else if (pass2_size % 16 == 0) pass2_multiplier = 3, pass2_size /= 16;		
	else pass2_multiplier = 10;							
	for (pass2_pow2 = 0; pass2_size >= 2; pass2_pow2++, pass2_size >>= 1);
	
	error_check = !!error_check;
	negacyclic = !!negacyclic;
	no_prefetch = !!no_prefetch;
	in_place = !!in_place;

	return ((negacyclic << 27) + (fft_type << 24) +		
		(no_prefetch << 21) + (in_place << 20) +	
		(error_check << 16) +				
		(architecture << 12) +				
		(pass2_multiplier << 8) +			
		(pass2_pow2 << 4) +				
#ifndef X86_64
		(1 << 3) +					
#endif
		(clm << 0));					
}

int internal_implementation_ids_match (
	int	impl_id,
	int	fftlen,
	int	fft_type,
	int	no_prefetch,
	int	in_place,
	int	pass2_size,
	int	architecture,
	int	clm)
{
	int	pass2_multiplier;		
	int	pass2_pow2;

	if (clm <= 1) clm = 0;
	else if (clm == 2) clm = 1;
	else clm = 2;
	
	if (pass2_size == 0) pass2_multiplier = 0;					
	else if (pass2_size % 9 == 0) pass2_multiplier = 0, pass2_size /= 9;		
	else if (pass2_size % 15 == 0) pass2_multiplier = 2, pass2_size /= 15;		
	else if (pass2_size % 25 == 0) pass2_multiplier = 5, pass2_size /= 25;		
	else if (pass2_size % 21 == 0) pass2_multiplier = 6, pass2_size /= 21;		
	else if (pass2_size % 35 == 0) pass2_multiplier = 7, pass2_size /= 35;		
	else if (pass2_size % 49 == 0) pass2_multiplier = 8, pass2_size /= 49;		
	else if (pass2_size % 12 == 0) pass2_multiplier = 1, pass2_size /= 12;		
	else if (pass2_size % 20 == 0) pass2_multiplier = 4, pass2_size /= 20;		
	else if (pass2_size % 28 == 0) pass2_multiplier = 9, pass2_size /= 28;		
	else if (pass2_size % 16 == 0) pass2_multiplier = 3, pass2_size /= 16;		
	else pass2_multiplier = 10;							
	for (pass2_pow2 = 0; pass2_size >= 2; pass2_pow2++, pass2_size >>= 1);
	
	impl_id &= ~0x8010008;
	
	no_prefetch = !!no_prefetch;
	in_place = !!in_place;
	
	return (impl_id == (fft_type << 24) + (no_prefetch << 21) + (in_place << 20) +
			   (architecture << 12) + (pass2_multiplier << 8) + (pass2_pow2 << 4) + (clm << 0));
}

void gwbench_get_max_throughput (
	int	fftlen,				
	int	arch,				
	int	num_cores,			
	int	num_workers,			
	int	num_hyperthreads,		
	int	negacyclic,			
	int	error_check,			
	int	*impl,				
	double	*throughput)			
{
	int	errcode, impl_bits, min_arch, max_arch;

	*impl = -1;
	*throughput = -1.0;

	if (BENCH_NUM_CORES) num_cores = BENCH_NUM_CORES;
	if (BENCH_NUM_WORKERS) num_workers = BENCH_NUM_WORKERS;

	if (BENCH_DB == NULL) return;

	gwmutex_lock (&SQL_MUTEX);

#ifndef X86_64
	impl_bits = 0x8;		
#else
	impl_bits = 0;			
#endif
	if (negacyclic) impl_bits |= 0x8000000;
	if (error_check) impl_bits |= 0x10000;

	min_arch = 0, max_arch = 15;	

	if (arch >= 0 && arch <= 2) min_arch = 0, max_arch = 3;
	
	if (arch >= 5 && arch <= 6) min_arch = 0, max_arch = 6;
	
	if (arch == 3) min_arch = 0, max_arch = 4;
	if (arch == 4) min_arch = 3, max_arch = 4;
	
	if (arch == 8) min_arch = 8, max_arch = 8;

	min_arch = min_arch << 12;
	max_arch = max_arch << 12;

	if (!get_max_sql_stmt_prepared) {
		errcode = sqlite3_prepare_v2 (BENCH_DB, "SELECT impl, avg_throughput FROM avgbest3 \
							 WHERE fftlen = ?1 AND num_cores = ?2 AND num_workers = ?3 AND \
								num_hyperthreads = ?4 AND (impl & 0x8010008) = ?5 AND \
								(impl & 0xF000) BETWEEN ?6 AND ?7 \
							 ORDER BY avg_throughput DESC LIMIT 1", -1, &get_max_sql_stmt, NULL);
		if (errcode != SQLITE_OK) goto stmt_error;
		get_max_sql_stmt_prepared = TRUE;
	}

	errcode = sqlite3_bind_int (get_max_sql_stmt, 1, fftlen);
	if (errcode != SQLITE_OK) goto stmt_error;

	errcode = sqlite3_bind_int (get_max_sql_stmt, 2, num_cores);
	if (errcode != SQLITE_OK) goto stmt_error;

	errcode = sqlite3_bind_int (get_max_sql_stmt, 3, num_workers);
	if (errcode != SQLITE_OK) goto stmt_error;

	errcode = sqlite3_bind_int (get_max_sql_stmt, 4, num_hyperthreads);
	if (errcode != SQLITE_OK) goto stmt_error;

	errcode = sqlite3_bind_int (get_max_sql_stmt, 5, impl_bits);
	if (errcode != SQLITE_OK) goto stmt_error;

	errcode = sqlite3_bind_int (get_max_sql_stmt, 6, min_arch);
	if (errcode != SQLITE_OK) goto stmt_error;

	errcode = sqlite3_bind_int (get_max_sql_stmt, 7, max_arch);
	if (errcode != SQLITE_OK) goto stmt_error;

	errcode = sqlite3_step (get_max_sql_stmt);
	if (errcode == SQLITE_ROW) {
		*impl = sqlite3_column_int (get_max_sql_stmt, 0);
		*throughput = sqlite3_column_double (get_max_sql_stmt, 1);
	}
	else if (errcode != SQLITE_DONE) goto stmt_error;

	sqlite3_reset (get_max_sql_stmt);
	gwmutex_unlock (&SQL_MUTEX);
	return;

stmt_error:
	sqlite3_finalize (get_max_sql_stmt);
	get_max_sql_stmt_prepared = FALSE;
	gwmutex_unlock (&SQL_MUTEX);
	return;
}

void gwbench_get_num_benchmarks (
	double k,
	unsigned long b,
	unsigned long n,
	signed long c,
	unsigned long minimum_fftlen,
	int	num_cores,
	int	num_workers,
	int	hyperthreading,
	int	error_check,
	unsigned long *fftlen,
	int	*negacyclic,
	int	*num_benchmarks)
{
	gwhandle gwdata;			
	sqlite3_stmt *sql_stmt;
	int	errcode, impl_bits, count, impls;

	*fftlen = 0;
	*num_benchmarks = 9999;

	if (!BENCH_DB_INITIALIZED) return;
	if (BENCH_DB == NULL) return;

	gwinit (&gwdata);
	gwclear_use_benchmarks (&gwdata);
	gwdata.minimum_fftlen = minimum_fftlen;
	gwdata.bench_num_cores = num_cores;
	gwdata.bench_num_workers = num_workers;
	gwdata.will_hyperthread = hyperthreading;
	gwdata.bench_pick_nth_fft = 1;				
	if (gwinfo (&gwdata, k, b, n, c)) return;		
	*fftlen = gwdata.jmptab->fftlen;			
	*negacyclic = gwdata.NEGACYCLIC_FFT;

#ifndef X86_64
	impl_bits = 0x8;		
#else
	impl_bits = 0;			
#endif
	if (gwdata.NEGACYCLIC_FFT) impl_bits |= 0x8000000;
	if (error_check) impl_bits |= 0x10000;

	gwmutex_lock (&SQL_MUTEX);

	errcode = sqlite3_prepare_v2 (BENCH_DB, "SELECT COUNT(*), COUNT (DISTINCT IMPL) FROM bench_data \
						 WHERE fftlen = ?1 AND num_cores = ?2 AND num_workers = ?3 AND \
						 num_hyperthreads = ?4 AND (impl & 0x8010008) = ?5", -1, &sql_stmt, NULL);
	if (errcode != SQLITE_OK) goto stmt_error;

	errcode = sqlite3_bind_int (sql_stmt, 1, gwdata.jmptab->fftlen);
	if (errcode != SQLITE_OK) goto stmt_error;

	errcode = sqlite3_bind_int (sql_stmt, 2, num_cores);
	if (errcode != SQLITE_OK) goto stmt_error;

	errcode = sqlite3_bind_int (sql_stmt, 3, num_workers);
	if (errcode != SQLITE_OK) goto stmt_error;

	errcode = sqlite3_bind_int (sql_stmt, 4, hyperthreading ? 2 : 1);
	if (errcode != SQLITE_OK) goto stmt_error;

	errcode = sqlite3_bind_int (sql_stmt, 5, impl_bits);
	if (errcode != SQLITE_OK) goto stmt_error;

	errcode = sqlite3_step (sql_stmt);
	if (errcode == SQLITE_ROW) {
		count = sqlite3_column_int (sql_stmt, 0);
		impls = sqlite3_column_int (sql_stmt, 1);
	}
	else if (errcode != SQLITE_DONE) goto stmt_error;
	sqlite3_reset (sql_stmt);

	if (impls) *num_benchmarks = count / impls;

stmt_error:
	sqlite3_finalize (sql_stmt);
	gwmutex_unlock (&SQL_MUTEX);
}
