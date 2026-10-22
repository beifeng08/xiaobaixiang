// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

#ifndef _COMMONB_H
#define _COMMONB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "security.h"
#ifndef SEC1
#define SEC1(p)			0UL
#define SEC2(p,hi,lo,u,e)	0UL
#define SEC3(p)			0UL
#define SEC4(p)			0UL
#define SEC5(p,b1,b2)		0UL
#endif

#define ALL_WORKERS 8888	
int LaunchWorkers (int, int);
int LaunchTortureTest (unsigned long, int);
int LaunchBench (int, int);
int LaunchAdvancedTime (unsigned long, unsigned long);

#define LD_CONTINUE	0		
#define LD_TIME		1		
#define LD_BENCH	2		
#define LD_TORTURE	3		
void PreLaunchCallback (int launch_type);	
void PostLaunchCallback (int launch_type);	
void stopCheckCallback (int thread_num);
void checkPauseListCallback (void);

#define SET_PRIORITY_NORMAL_WORK	1
#define SET_PRIORITY_BENCHMARKING	2
#define SET_PRIORITY_TORTURE		3
#define SET_PRIORITY_TIME		4
#define SET_PRIORITY_QA			5
#define SET_PRIORITY_BUSY_LOOP		6
struct PriorityInfo {
 	uint32_t type;			
	uint32_t worker_num;		
	uint32_t verbosity;		
	uint32_t aux_thread_num;	
	bool	aux_hyperthread;	
	bool	aux_polymult;		
	union {
		struct {		
			int	normal_work_hyperthreading;	
		};
		struct {		
			int	torture_num_workers;		
			int	torture_core_num;		
			int	torture_thread_num;		
			bool	torture_hyperthreading;		
		};
		struct {		
			bool	time_hyperthreading;		
		};
		struct {		
			int	bench_base_core_num;		
			bool	bench_hyperthreading;		
		};
		struct {		
			int	busy_loop_core;			
		};
	};
};
void SetPriority (struct PriorityInfo *);
void SetAuxThreadPriority (int aux_thread_num, int action, void *data);

uint32_t get_ranked_core (uint32_t core_num);

uint32_t get_ranked_num_threads (uint32_t base_core_num, uint32_t num_cores, bool hyperthreading);

uint32_t get_worker_num_threads (uint32_t worker_num, bool hyperthreading);

void create_worker_windows (int num_threads);
void Launcher (void *arg);
void LauncherDispatch (void *arg);
int primeContinue (int);
void tortureTestDefaultSizes (int torture_type, int num_threads, int *minfft, int *maxfft);
int tortureTest (int, int);
int selfTest (int, struct PriorityInfo *, struct work_unit *);
int primeTime (int, unsigned long, unsigned long);
int primeBench (int, int);
int primeFactor (int, struct PriorityInfo *, struct work_unit *, unsigned int);
int prime (int, struct PriorityInfo *, struct work_unit *, int);
int prp (int, struct PriorityInfo *, struct work_unit *, int);
int cert (int, struct PriorityInfo *, struct work_unit *, int);
void autoBench (void);

int isKnownMersennePrime (unsigned long);
void makestr (unsigned long, unsigned long, unsigned long, char *);

int exponent_near_fft_limit (gwhandle *gwdata);
void calc_output_frequencies (gwhandle *gwdata, double *output_frequency, double *output_title_frequency);
double trunc_percent (double percent);
int testSaveFilesFlag (int thread_num);
int SleepFive (int thread_num);

#define TIMER_NL	0x1
#define TIMER_CLR	0x2
#define TIMER_OPT_CLR	0x4
#define TIMER_MS	0x8

void clear_timers (double *timers, int num_timers);
void clear_timer (double *timers, int i);
void start_timer (double *timers, int i);
void start_timer_from_zero (double *timers, int i);
void end_timer (double *timers, int i);
void divide_timer (double *timers, int i, int j);
double timer_value (double *timers, int i);
void print_timer (double *timers, int i, char *buf, int flags);

typedef struct read_save_file_state {
	int	thread_num;
	int	read_attempt;
	int	a_save_file_existed;
	int	a_non_bad_save_file_existed;
	int	num_original_bad_files;
	int	num_save_files_renamed;
	char	base_filename[80];
	char	current_filename[80];
} readSaveFileState;

void readSaveFileStateInit (readSaveFileState *state, int thread_num, char *filename);
int saveFileExists (readSaveFileState *state);
void saveFileBad (readSaveFileState *state);
extern const char ALLSAVEBAD_MSG[];

typedef struct write_save_file_state {
	char	base_filename[80];
	int	num_ordinary_save_files;
	int	num_special_save_files;		
	uint64_t special;			
} writeSaveFileState;

void uniquifySaveFile (int thread_num, char *filename);
void writeSaveFileStateInit (writeSaveFileState *state, char *filename, int num_special_save_files);
int openWriteSaveFile (writeSaveFileState *state);
void closeWriteSaveFile (writeSaveFileState *state, int fd);
void setWriteSaveFileSpecial (writeSaveFileState *state);
void deleteWriteSaveFile (writeSaveFileState *state, int fd);
void unlinkSaveFiles (writeSaveFileState *state);

#define STOP_ESCAPE		1	
#define STOP_OUT_OF_MEM		2	
#define STOP_FILE_IO_ERROR	3	
#define STOP_FATAL_ERROR	4	
#define STOP_ABORT		5	
#define STOP_WORKER		6	
#define STOP_PAUSE		7	
#define STOP_RETRY_LATER	8	
#define STOP_WORK_UNIT_COMPLETE	50	
#define STOP_PRIORITY_WORK	51	
#define STOP_BATTERY		52	
#define STOP_AUTOBENCH		53	
#define STOP_PREV_WORK_UNIT	54	
#define STOP_REREAD_INI		100	
#define STOP_RESTART		101	
#define STOP_MEM_CHANGED	102	
#define STOP_NOT_ENOUGH_MEM	103	

EXTERNC int stopCheck (int);
void stop_workers_for_escape (void);
void stop_workers_for_restart (void);
void stop_workers_for_add_files (void);
void stop_workers_for_reread_ini (void);
#define RESTART_ALL			0xFFFF
#define RESTART_WORK_AVAILABLE		0x0001
#define RESTART_USER_START		0x0002
#define RESTART_END_PAUSE		0x0004
#define RESTART_MEM_WAIT		0x0008
#define RESTART_BATTERY			0x0010
#define RESTART_LOADAVG			0x0020
void restart_waiting_workers (int);
void restart_one_waiting_worker (int, int);
void stop_worker_for_abort (int);

void mem_settings_have_changed (void);
unsigned long max_mem (int);
int avail_mem (int, unsigned long, unsigned long, unsigned int *);
#define MEM_USAGE_NOT_SET 0x1		
#define MEM_RESTARTING	0x2		
#define MEM_WILL_BE_VARIABLE_USAGE 0x4	
#define	MEM_VARIABLE_USAGE 0x8		
#define MEM_WAITING 0x10		
int set_memory_usage (int, int, unsigned long);
void set_restart_if_max_memory_change (int thread_num);
void clear_restart_if_max_memory_change (int thread_num);

int avail_mem_not_sufficient (int thread_num, unsigned long min_memory, unsigned long desired_memory);

#define array_gwnum_size(g)			round_up_to_multiple_of((gwnum_datasize(g)+GW_HEADER_SIZE(g)),64)
#define cvt_mem_to_gwnums(g,m)			cvt_mem_to_gwnums_adj(g,m,0.0)
#define cvt_mem_to_gwnums_adj(g,m,a)		((uint64_t)(((double)((m)-1)*1048576.0-(double)gwmemused(g))/(double)array_gwnum_size(g)+(a)))
#define cvt_gwnums_to_mem(g,n)			((unsigned long)(((double)gwmemused(g)+(double)(n)*(double)gwnum_size(g))/1048576.0)+2)
#define cvt_mem_to_array_gwnums(g,m)		cvt_mem_to_array_gwnums_adj(g,m,0.0)
#define cvt_mem_to_array_gwnums_adj(g,m,a)	((uint64_t)(((double)((m)-1)*1048576.0-(double)gwmemused(g))/(double)array_gwnum_size(g)+(a)))
#define cvt_array_gwnums_to_mem(g,n)		((unsigned long)(((double)gwmemused(g)+(double)(n)*(double)array_gwnum_size(g))/1048576.0)+2)
#define cvt_mem_to_estimated_gwnums(m,k,b,n,c)	((uint64_t)(((double)((m)-1)*1048576.0-(double)gwmap_to_memused(k,b,n,c))/(double)gwmap_to_estimated_size(k,b,n,c)))

void run_on_battery_changed (void);
int OnBattery (void);			
void test_battery (void);

void check_for_priority_work (void);
void stop_worker_for_advanced_test (int);

void start_one_worker (int);
void stop_one_worker (int);
unsigned int active_workers_count (void);

extern int PAUSEABLE_WORKERS_RUNNING;	
extern char **PAUSE_WHILE_RUNNING;	
extern int PAUSE_WHILE_RUNNING_FREQ;	
void read_pause_info (void);
void checkPauseWhileRunning (void);
void implement_pause (int thread_num);
int is_LowMemWhileRunning_active (int thread_num);
void isInPauseList (char *program_name);

void read_load_average_info (void);
double get_load_average (void);		
void checkLoadAverage (void);
void implement_loadavg (int thread_num);

void start_throttle_timer (void);
void stop_throttle_timer (void);
int handleThrottleTimerEvent (void);
void implementThrottle (int thread_num);

void clearThreadHandleArray (void);
void setOsThreadPriority (int);
void registerThreadTermination (void);
void raiseAllWorkersPriority (void);
void flashWindowAndBeep (void);
int primeSieveTest (int);
int test_randomly (int, struct PriorityInfo *);
int test_all_impl (int, struct PriorityInfo *);

#define BENCH_SPEED  "The CPU speed in Options/CPU may not be correct.  An incorrect value will result in inaccurate timings.  Are you sure this is the correct CPU speed value?"

#ifdef __cplusplus
}
#endif

#endif
