// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

#include "ecm.h"
#include "exponentiate.h"
#include "polymult.h"

static const char ERRMSG0[] = "Iteration: %ld/%ld, %s";
static const char ERRMSG1A[] = "ERROR: ILLEGAL SUMOUT\n";
static const char ERRMSG1C[] = "Possible error: round off (%.10g) > %.5g\n";
static const char ERRMSG1D[] = "ERROR: Shift counter corrupt.\n";
static const char ERRMSG1E[] = "ERROR: Illegal double encountered.\n";
static const char ERRMSG1F[] = "ERROR: FFT data has been zeroed!\n";
static const char ERRMSG1G[] = "ERROR: Jacobi error check failed!\n";
static const char ERRMSG2[] = "Possible hardware failure, consult readme.txt file.\n";
static const char ERRMSG3[] = "Continuing from last save file.\n";
static const char ERRMSG4[] = "Waiting five minutes before restarting.\n";
static const char ERRMSG5[] = "For added safety, redoing iteration using a slower, more reliable method.\n";
static const char ERRMSG6[] = "ERROR: Comparing PRP double-check values failed.  Rolling back to iteration %lu.\n";
static const char ERRMSG7[] = "ERROR: Comparing Gerbicz checksum values failed.  Rolling back to iteration %lu.\n";
static const char ERRMSG8[] = "ERROR: Invalid FFT data.  Restarting from last save file.\n";
static const char ERRMSG9[] = "ERROR: Invalid PRP state.  Restarting from last save file.\n";
static const char ERROK[] = "Disregard last error.  Result is reproducible and thus not a hardware problem.\n";
static const char READFILEERR[] = "Error reading intermediate file: %s\n";
static const char WRITEFILEERR[] = "Error writing intermediate file: %s\n";
static const char ALTSAVE_MSG[] = "Trying backup intermediate file: %s\n";
const char ALLSAVEBAD_MSG[] = "All intermediate files bad.  Temporarily abandoning work unit.\n";

struct pause_info {
	int	thread_num;		
	int	low_mem;		
	int	workers_affected;	
	char	*program_name;		
	char	matching_program[80];	
	struct pause_info *next;	
};

int	PAUSE_MUTEX_INITIALIZED = 0;
gwmutex	PAUSE_MUTEX;		
struct pause_info *PAUSE_DATA = NULL;
int	PAUSE_WHILE_RUNNING_FREQ = 10;
int	PAUSEABLE_WORKERS_RUNNING = FALSE;

int	STOP_FOR_RESTART = FALSE;
				
int	STOP_FOR_REREAD_INI = FALSE;
				
char	STOP_FOR_MEM_CHANGED[MAX_NUM_WORKERS] = {0};
				
int	STOP_FOR_BATTERY = FALSE;
				
int	STOP_FOR_AUTOBENCH = FALSE;
				
char	STOP_FOR_PRIORITY_WORK[MAX_NUM_WORKERS] = {0};
				
struct pause_info *STOP_FOR_PAUSE[MAX_NUM_WORKERS] = {NULL};
				
struct pause_info *STOP_FOR_LOW_MEMORY = NULL; 
				
int	STOP_FOR_LOADAVG = 0;	
				
char	STOP_FOR_THROTTLE[MAX_NUM_WORKERS] = {0};
				
char	STOP_FOR_ABORT[MAX_NUM_WORKERS] = {0};
				
char	ACTIVE_WORKERS[MAX_NUM_WORKERS] = {0};
				
char	WRITE_SAVE_FILES[MAX_NUM_WORKERS] = {0};
				
char	JACOBI_ERROR_CHECK[MAX_NUM_WORKERS] = {0};
				
char	WORK_AVAILABLE_OR_STOP_INITIALIZED[MAX_NUM_WORKERS] = {0};
gwevent WORK_AVAILABLE_OR_STOP[MAX_NUM_WORKERS] = {0};
				
char	USER_START_OR_STOP_INITIALIZED[MAX_NUM_WORKERS] = {0};
gwevent	USER_START_OR_STOP[MAX_NUM_WORKERS] = {0};
				
char	END_PAUSE_OR_STOP_INITIALIZED[MAX_NUM_WORKERS] = {0};
gwevent END_PAUSE_OR_STOP[MAX_NUM_WORKERS] = {0};
				
char	END_LOADAVG_OR_STOP_INITIALIZED[MAX_NUM_WORKERS] = {0};
gwevent END_LOADAVG_OR_STOP[MAX_NUM_WORKERS] = {0};
				
char	OFF_BATTERY_OR_STOP_INITIALIZED[MAX_NUM_WORKERS] = {0};
gwevent OFF_BATTERY_OR_STOP[MAX_NUM_WORKERS] = {0};
				
char	MEM_WAIT_OR_STOP_INITIALIZED[MAX_NUM_WORKERS] = {0};
gwevent MEM_WAIT_OR_STOP[MAX_NUM_WORKERS] = {0};
				
#define DEFAULT_MEM_USAGE 48	
unsigned long AVAIL_MEM = 0;	
unsigned long MAX_MEM = 0;	
unsigned long AVAIL_MEM_PER_WORKER[MAX_NUM_WORKERS] = {0};	
unsigned long MAX_HIGH_MEM_WORKERS =  0;				
char	MEM_FLAGS[MAX_NUM_WORKERS] = {0};			
unsigned int MEM_IN_USE[MAX_NUM_WORKERS] = {0};			

#define MEM_RESTART_LOWMEM_ENDS 0x1		
#define MEM_RESTART_MAX_MEM_AVAILABLE 0x2	
						
#define MEM_RESTART_MAX_MEM_CHANGE 0x4		
#define MEM_RESTART_TOO_MANY_HIGHMEM 0x8	
#define MEM_RESTART_MORE_AVAIL 0x10		
#define MEM_RESTART_IF_MORE 0x20		

char	MEM_RESTART_FLAGS[MAX_NUM_WORKERS] = {0};
unsigned int MEM_RESTART_MIN_AMOUNT[MAX_NUM_WORKERS] = {0};
unsigned int MEM_RESTART_DESIRED_AMOUNT[MAX_NUM_WORKERS] = {0};	
unsigned int MEM_RESTART_IF_MORE_AMOUNT[MAX_NUM_WORKERS] = {0};

int	MEM_MUTEX_INITIALIZED = FALSE;
gwmutex	MEM_MUTEX;		

void clear_timers (
	double	*timers,
	int	num_timers)
{
	int	i;
	for (i = 0; i < num_timers; i++) timers[i] = 0.0;
}

void clear_timer (
	double	*timers,
	int	i)
{
	timers[i] = 0.0;
}

void start_timer (
	double	*timers,
	int	i)
{
	if (RDTSC_TIMING < 10) {
		timers[i] -= getHighResTimer ();
	} else if (RDTSC_TIMING > 10 && (CPU_FLAGS & CPU_RDTSC)) {
		uint32_t hi, lo;
		rdtsc (&hi, &lo);
		timers[i] -= (double) hi * 4294967296.0 + lo;
	} else {
		struct timeval start_time;
		gettimeofday (&start_time, NULL);
		timers[i] -= (double) start_time.tv_sec * 1000000.0 + start_time.tv_usec;
	}
}

void start_timer_from_zero (
	double	*timers,
	int	i)
{
	clear_timer (timers, i);
	start_timer (timers, i);
}

void end_timer (
	double	*timers,
	int	i)
{
	if (RDTSC_TIMING < 10) {
		timers[i] += getHighResTimer ();
	} else if (RDTSC_TIMING > 10 && (CPU_FLAGS & CPU_RDTSC)) {
		uint32_t hi, lo;
		rdtsc (&hi, &lo);
		timers[i] += (double) hi * 4294967296.0 + lo;
	} else {
		struct timeval end_time;
		gettimeofday (&end_time, NULL);
		timers[i] += (double) end_time.tv_sec * 1000000.0 + end_time.tv_usec;
	}
}

void divide_timer (
	double	*timers,
	int	i,
	int	j)
{
	timers[i] = timers[i] / (double) j;
}

double timer_value (
	double	*timers,
	int	i)
{
	if (RDTSC_TIMING < 10)
		return (timers[i] / getHighResTimerFrequency ());
	else if (RDTSC_TIMING > 10 && (CPU_FLAGS & CPU_RDTSC))
		return (timers[i] / CPU_SPEED / 1000000.0);
	else
		return (timers[i] / 1000000.0);
}

void print_timer (
	double	*timers,
	int	i,
	char	*buf,
	int	flags)
{
	double	t;

	buf += strlen (buf);
	t = timer_value (timers, i);
	if (t < 0.0) {
		strcpy (buf, "Unknown");
		timers[i] = 0.0;
	}

	else {
		int	style;

		style = IniGetInt (INI_FILE, "TimingOutput", 0);
		if (style == 0) {
			if (flags & TIMER_MS) style = 4;
			else style = 1;
		}

		if (style == 1)
			sprintf (buf, "%.3f sec.", t);
		else if (style == 2)
			sprintf (buf, "%.1f ms.", t * 1000.0);
		else if (style == 3)
			sprintf (buf, "%.2f ms.", t * 1000.0);
		else
			sprintf (buf, "%.3f ms.", t * 1000.0);

		if (RDTSC_TIMING == 12 && (CPU_FLAGS & CPU_RDTSC))
			sprintf (buf+strlen(buf), " (%.0f clocks)", timers[i]);
		else if (RDTSC_TIMING == 2)
			sprintf (buf+strlen(buf), " (%.0f clocks)", t * CPU_SPEED * 1000000.0);
	}

	if (flags & TIMER_NL) strcat (buf, "\n");

	if (flags & TIMER_CLR) timers[i] = 0.0;
	if ((flags & TIMER_OPT_CLR) && !CUMULATIVE_TIMING) timers[i] = 0.0;
}

uint32_t get_ranked_core (		
	uint32_t core_num)		
{
	uint32_t i, num_performance_cores, num_efficiency_cores;
	core_num++;			
	ASSERTG (core_num <= HW_NUM_CORES);
	num_performance_cores = 0;
	num_efficiency_cores = 0;
	for (i = 0; i < HW_NUM_CORES; i++) {
		if (HW_CORES[i].ranking >= 1) num_performance_cores++; else num_efficiency_cores++;
		if (core_num <= HW_NUM_COMPUTE_CORES && core_num == num_performance_cores) return (i);
		if (core_num > HW_NUM_COMPUTE_CORES && core_num - HW_NUM_COMPUTE_CORES == num_efficiency_cores) return (i);
	}
	ASSERTG (0);
	return (0);
}

uint32_t get_ranked_num_threads (
	uint32_t base_core_num,		
	uint32_t num_cores,		
	bool	hyperthreading)		
{
	if (!hyperthreading) return (num_cores);
	uint32_t num_threads = 0;
	for (uint32_t core = base_core_num; core < (int) HW_NUM_CORES && core < base_core_num + num_cores; core++)
		num_threads += HW_CORES[get_ranked_core(core)].num_threads;
	return (num_threads);
}

uint32_t get_worker_num_threads (
	uint32_t worker_num,		
	bool	hyperthreading)		
{
	
	if (!hyperthreading) return (CORES_PER_TEST[worker_num]);

	char	section_name[32];
	const char *p;
	sprintf (section_name, "Worker #%d", worker_num + 1);
	p = IniSectionGetStringRaw (INI_FILE, section_name, "Affinity");
	if (p != NULL) return (countCommas (p) + 1);

	if (NUM_WORKERS == HW_NUM_COMPUTE_CORES || NUM_WORKERS == HW_NUM_CORES) return (get_ranked_num_threads (worker_num, 1, hyperthreading));

	uint32_t worker_core_count, cores_used_by_lower_workers, base_core_num;
	worker_core_count = cores_used_by_lower_workers = 0;
	for (uint32_t i = 0; i < NUM_WORKERS; i++) {
		worker_core_count += CORES_PER_TEST[i];
		if (i < worker_num) cores_used_by_lower_workers += CORES_PER_TEST[i];
	}
	if (worker_core_count < HW_NUM_COMPUTE_CORES && HW_NUM_COMPUTE_CORES == HW_NUM_CORES) base_core_num = cores_used_by_lower_workers + 1;
	else if (worker_core_count <= HW_NUM_CORES) base_core_num = cores_used_by_lower_workers;
	else base_core_num = cores_used_by_lower_workers % HW_NUM_CORES;
	
	return (get_ranked_num_threads (base_core_num, CORES_PER_TEST[worker_num], hyperthreading));
}

uint32_t get_cores_in_threading_node (
	uint32_t node_num)		
{

	if (node_num < HW_NUM_COMPUTE_THREADING_NODES) return (HW_NUM_COMPUTE_CORES / HW_NUM_COMPUTE_THREADING_NODES);
	return ((HW_NUM_CORES - HW_NUM_COMPUTE_CORES) / (HW_NUM_THREADING_NODES - HW_NUM_COMPUTE_THREADING_NODES));
}

uint32_t map_aux_to_core (
	uint32_t base_core_num,		
	uint32_t aux_thread_num,	
	bool	hyperthreading)		
{
	ASSERTG (base_core_num < (int) HW_NUM_CORES);
	
	if (!hyperthreading) return (get_ranked_core ((base_core_num + aux_thread_num) % HW_NUM_CORES));
	
	uint32_t total_threads = 0;
	uint32_t hwloc_core = 0;
	for (int core = base_core_num; ; core++) {
		if (core == (int) HW_NUM_CORES) core = 0;		
		hwloc_core = get_ranked_core (core);
		total_threads += HW_CORES[hwloc_core].num_threads;
		if (aux_thread_num < total_threads) break;
	}
	return (hwloc_core);
}

void SetPriority (
	struct PriorityInfo *info)
{
	int	bind_type, core;
#ifdef BIND_TYPE_1_USED
	int	logical_CPU;
#endif
	char	logical_CPU_string[255];
	char	logical_CPU_substring[255];
	char	buf[255];

	if (IniGetInt (INI_FILE, "EnableSetPriority", 1))
		setOsThreadPriority (PRIORITY);

	if (! IniGetInt (INI_FILE, "EnableSetAffinity", 1)) return;

 	if (!OS_CAN_SET_AFFINITY) return;

	switch (info->type) {

	case SET_PRIORITY_QA:
		return;

	case SET_PRIORITY_TIME:
		bind_type = 0;				
		
		core = map_aux_to_core (0, info->aux_thread_num, info->time_hyperthreading);
		break;

	case SET_PRIORITY_BENCHMARKING:
		bind_type = 0;				
		
		core = map_aux_to_core (info->bench_base_core_num, info->aux_thread_num, info->bench_hyperthreading);
		break;

	case SET_PRIORITY_BUSY_LOOP:
		bind_type = 0;				
		core = get_ranked_core (info->busy_loop_core);
		break;

	case SET_PRIORITY_TORTURE:
		bind_type = 0;				
		core = get_ranked_core (info->torture_core_num);
		break;

	case SET_PRIORITY_NORMAL_WORK:
		{
			char	section_name[32];
			const char *p;
			sprintf (section_name, "Worker #%d", info->worker_num+1);
			p = IniSectionGetStringRaw (INI_FILE, section_name, "Affinity");
			if (p != NULL) {
				bind_type = 2;				
				truncated_strcpy (logical_CPU_string, sizeof (logical_CPU_string), p);
				break;
			}
		}

		if (NUM_WORKERS == HW_NUM_COMPUTE_CORES || NUM_WORKERS == HW_NUM_CORES) {
			bind_type = 0;				
			core = get_ranked_core (info->worker_num);
			break;
		}

		if (info->aux_polymult && info->aux_thread_num >= CORES_PER_TEST[info->worker_num] * (info->normal_work_hyperthreading ? 2 : 1)) {
			bind_type = 3;
			break;
		}

		uint32_t worker_core_count, cores_used_by_lower_workers;
		worker_core_count = cores_used_by_lower_workers = 0;
		for (uint32_t i = 0; i < NUM_WORKERS; i++) {
			worker_core_count += CORES_PER_TEST[i];
			if (i < info->worker_num) cores_used_by_lower_workers += CORES_PER_TEST[i];
		}

		if (worker_core_count < HW_NUM_COMPUTE_CORES && HW_NUM_COMPUTE_CORES == HW_NUM_CORES && IniGetInt (INI_FILE, "ReserveCore0", 0)) {
			bind_type = 0;			
			
			core = map_aux_to_core (cores_used_by_lower_workers + 1, info->aux_thread_num, info->normal_work_hyperthreading);
			break;
		}

		if (worker_core_count <= HW_NUM_CORES) {
			bind_type = 0;			
			
			core = map_aux_to_core (cores_used_by_lower_workers, info->aux_thread_num, info->normal_work_hyperthreading);
			break;
		}

		bind_type = 0;			
		core = map_aux_to_core (cores_used_by_lower_workers % HW_NUM_CORES, info->aux_thread_num, info->normal_work_hyperthreading);
		break;
	}

	if (bind_type == 2) {		
		uint32_t i;
		const char *p;
		for (i = 0, p = logical_CPU_string; i <= info->aux_thread_num && *p; i++) {
			while (isspace (*p)) p++;
			const char *start = p;
			char end_char = 0;
			if (*p == '{') end_char = '}';
			if (*p == '(') end_char = ')';
			if (*p == '[') end_char = ']';
			if (end_char) {
				p = strchr (start, end_char);
				if (p == NULL) {
					sprintf (buf, "Error parsing affinity string: %s\n", logical_CPU_string);
					OutputStr (info->worker_num, buf);
					return;
				}
				truncated_strcpy_with_len (logical_CPU_substring, sizeof (logical_CPU_substring), start + 1, (int) (p - start - 1));
				if (strchr (p, ',') != NULL) p = strchr (p, ',') + 1;
				else p = p + strlen(p);
			} else {
				p = strchr (start, ',');
				if (p != NULL) {
					truncated_strcpy_with_len (logical_CPU_substring, sizeof (logical_CPU_substring), start, (int) (p - start));
					p++;
				} else {
					truncated_strcpy (logical_CPU_substring, sizeof (logical_CPU_substring), start);
					p = start + strlen (start);
				}
			}
		}
	}

	if (HW_NUM_CORES > 1 && info->verbosity >= 1) {
		if (info->aux_hyperthread) sprintf (buf, "Setting affinity to run prefetching hyperthread on ");
		else if (info->aux_polymult) sprintf (buf, "Setting affinity to run polymult helper thread on ");
		else if (info->aux_thread_num == 0) strcpy (buf, "Setting affinity to run worker on ");
		else sprintf (buf, "Setting affinity to run helper thread %d on ", info->aux_thread_num);
		if (bind_type == 0) sprintf (buf+strlen(buf), "CPU core #%d\n", core+1);
#ifdef BIND_TYPE_1_USED
		else if (bind_type == 1) sprintf (buf+strlen(buf), "logical CPU #%d (zero-based)\n", logical_CPU);
#endif
		else if (bind_type == 3) sprintf (buf+strlen(buf), "any performance core\n");
		else sprintf (buf+strlen(buf), "logical CPUs %s (zero-based)\n", logical_CPU_substring);
		OutputStr (info->worker_num, buf);
	}

	if (bind_type == 0) {
		int	num_cores;
		hwloc_obj_t obj;
		num_cores = hwloc_get_nbobjs_by_type (hwloc_topology, HWLOC_OBJ_CORE);
		if (num_cores < 1) num_cores = hwloc_get_nbobjs_by_type (hwloc_topology, HWLOC_OBJ_PU);
		if (num_cores < 1) num_cores = 1;	
		if (core >= num_cores) {		
			sprintf (buf, "Error setting affinity to core #%d.  There are %d cores.\n", core+1, num_cores);
			OutputStr (info->worker_num, buf);
			core %= num_cores;
		}
		obj = hwloc_get_obj_by_type (hwloc_topology, HWLOC_OBJ_CORE, core);			
		if (obj == NULL) obj = hwloc_get_obj_by_type (hwloc_topology, HWLOC_OBJ_PU, core);	
		if (obj != NULL) {
			if (hwloc_set_cpubind (hwloc_topology, obj->cpuset, HWLOC_CPUBIND_THREAD)) { 
				char	str[80];
				int	error = errno;
				hwloc_bitmap_snprintf (str, sizeof (str), obj->cpuset);
				sprintf (buf, "Error setting affinity to cpuset %s: %s\n", str, strerror (error));
				OutputStr (info->worker_num, buf);
			}
			else if (info->verbosity >= 2) {
				char	str[80];
				hwloc_bitmap_snprintf (str, sizeof (str), obj->cpuset);
				sprintf (buf, "Affinity set to cpuset %s\n", str);
				OutputStr (info->worker_num, buf);
			}
		}
		else {					
			sprintf (buf, "Error getting hwloc object for core #%d.  Affinity not set.\n", core+1);
			OutputStr (info->worker_num, buf);
		}
	}

#ifdef BIND_TYPE_1_USED
	else if (bind_type == 1) {
		int	num_logical_CPUs;
		hwloc_obj_t obj;
		num_logical_CPUs = hwloc_get_nbobjs_by_type (hwloc_topology, HWLOC_OBJ_PU);
		if (logical_CPU >= num_logical_CPUs) {		
			sprintf (buf, "Error setting affinity to logical CPU #%d (zero-based).  There are %d logical CPUs.\n", logical_CPU, num_logical_CPUs);
			OutputStr (info->worker_num, buf);
			logical_CPU %= num_logical_CPUs;
		}
		obj = hwloc_get_obj_by_type (hwloc_topology, HWLOC_OBJ_PU, logical_CPU);	
		if (obj) {
			if (hwloc_set_cpubind (hwloc_topology, obj->cpuset, HWLOC_CPUBIND_THREAD)) { 
				char	str[80];
				int	error = errno;
				hwloc_bitmap_snprintf (str, sizeof (str), obj->cpuset);
				sprintf (buf, "Error setting affinity to cpuset %s: %s\n", str, strerror (error));
				OutputStr (info->worker_num, buf);
			}
			else if (info->verbosity >= 2) {
				char	str[80];
				hwloc_bitmap_snprintf (str, sizeof (str), obj->cpuset);
				sprintf (buf, "Affinity set to cpuset %s\n", str);
				OutputStr (info->worker_num, buf);
			}
		}
	}
#endif

	else if (bind_type == 3) {
		hwloc_bitmap_t cpuset = hwloc_bitmap_alloc ();
		for (int core = 0; core < (int) HW_NUM_CORES; core++) {
			if (HW_CORES[core].ranking != 1) continue;		
			hwloc_obj_t obj = hwloc_get_obj_by_type (hwloc_topology, HWLOC_OBJ_CORE, core);		
			if (obj == NULL) obj = hwloc_get_obj_by_type (hwloc_topology, HWLOC_OBJ_PU, core);	
			if (obj == NULL) continue;				
			hwloc_bitmap_or (cpuset, cpuset, obj->cpuset);
		}
		if (hwloc_set_cpubind (hwloc_topology, cpuset, HWLOC_CPUBIND_THREAD)) {
			char	str[80];
			int	error = errno;
			hwloc_bitmap_snprintf (str, sizeof (str), cpuset);
			sprintf (buf, "Error setting affinity to cpuset %s: %s\n", str, strerror (error));
			OutputStr (info->worker_num, buf);
		}
		else if (info->verbosity >= 2) {
			char	str[80];
			hwloc_bitmap_snprintf (str, sizeof (str), cpuset);
			sprintf (buf, "Affinity set to cpuset %s\n", str);
			OutputStr (info->worker_num, buf);
		}
		hwloc_bitmap_free (cpuset);
	}

	else {
		char *p, *dash, *comma;
		hwloc_obj_t obj;
		hwloc_bitmap_t cpuset;
		int	start, end;

		cpuset = hwloc_bitmap_alloc ();
		for (p = logical_CPU_substring; ; p = comma+1) {	
			start = atoi (p);
			dash = strchr (p, '-');
			comma = strchr (p, ',');
			if (dash != NULL && (comma == NULL || dash < comma)) end = atoi (dash+1);
			else end = start;
			while (start <= end) {
				obj = hwloc_get_obj_by_type (hwloc_topology, HWLOC_OBJ_PU, start++);
				if (obj) hwloc_bitmap_or (cpuset, cpuset, obj->cpuset);
			}
			if (comma == NULL) break;
		}
		if (hwloc_set_cpubind (hwloc_topology, cpuset, HWLOC_CPUBIND_THREAD)) {	
			char	str[80];
			int	error = errno;
			hwloc_bitmap_snprintf (str, sizeof (str), cpuset);
			sprintf (buf, "Error setting affinity to cpuset %s: %s\n", str, strerror (error));
			OutputStr (info->worker_num, buf);
		}
		else if (info->verbosity >= 2) {
			char	str[80];
			hwloc_bitmap_snprintf (str, sizeof (str), cpuset);
			sprintf (buf, "Affinity set to cpuset %s\n", str);
			OutputStr (info->worker_num, buf);
		}
		hwloc_bitmap_free (cpuset);
	}
}

void SetAuxThreadPriority (int aux_thread_num, int action, void *data)
{

	if (action == 0 || action == 10 || action == 20) {
		struct PriorityInfo sp_info;
		memcpy (&sp_info, data, sizeof (struct PriorityInfo));
		sp_info.aux_thread_num = aux_thread_num;
		sp_info.aux_hyperthread = (action == 10);
		sp_info.aux_polymult = (action == 20);
		SetPriority (&sp_info);
	}

	if (action == 1 || action == 11 || action == 21) {
		registerThreadTermination ();
	}
}

EXTERNC int stopCheck (
	int	thread_num)	
{

	stopCheckCallback (thread_num);

	if (WORKERS_STOPPING) return (STOP_ESCAPE);

	if (thread_num < 0) return (0);

	if (STOP_FOR_RESTART) return (STOP_RESTART);

	if (STOP_FOR_REREAD_INI) return (STOP_REREAD_INI);

	if (STOP_FOR_MEM_CHANGED[thread_num]) {
		STOP_FOR_MEM_CHANGED[thread_num] = 0;
		return (STOP_MEM_CHANGED);
	}

	if (STOP_FOR_BATTERY) return (STOP_BATTERY);

	if (STOP_FOR_AUTOBENCH) return (STOP_AUTOBENCH);

	if (STOP_FOR_PRIORITY_WORK[thread_num]) {
		STOP_FOR_PRIORITY_WORK[thread_num] = 0;
		return (STOP_PRIORITY_WORK);
	}

	if (STOP_FOR_ABORT[thread_num]) {
		STOP_FOR_ABORT[thread_num] = 0;
		return (STOP_ABORT);
	}

	if (!ACTIVE_WORKERS[thread_num]) return (STOP_WORKER);

	if (STOP_FOR_PAUSE[thread_num] != NULL) {
		return (STOP_PAUSE);

	}

	if (STOP_FOR_LOADAVG) {
		STOP_FOR_LOADAVG--;
		implement_loadavg (thread_num);
		return (stopCheck (thread_num));
	}

	if (STOP_FOR_THROTTLE[thread_num]) {
		STOP_FOR_THROTTLE[thread_num] = 0;
		implementThrottle (thread_num);
	}

	return (0);
}

int stop_events_initialized = FALSE;

void init_stop_code (void)
{
	STOP_FOR_RESTART = FALSE;
	STOP_FOR_REREAD_INI = FALSE;
	STOP_FOR_BATTERY = FALSE;
	STOP_FOR_AUTOBENCH = FALSE;
	STOP_FOR_LOADAVG = 0;
	memset (STOP_FOR_MEM_CHANGED, 0, sizeof (STOP_FOR_MEM_CHANGED));
	memset (STOP_FOR_PRIORITY_WORK, 0, sizeof (STOP_FOR_PRIORITY_WORK));
	memset (STOP_FOR_PAUSE, 0, sizeof (STOP_FOR_PAUSE));
	memset (STOP_FOR_THROTTLE, 0, sizeof (STOP_FOR_THROTTLE));
	memset (STOP_FOR_ABORT, 0, sizeof (STOP_FOR_ABORT));
	memset (WRITE_SAVE_FILES, 0, sizeof (WRITE_SAVE_FILES));
	memset (JACOBI_ERROR_CHECK, 0, sizeof (JACOBI_ERROR_CHECK));
}

void restart_waiting_workers (
	int	restart_flags)
{
	int	thread_num;
	for (thread_num = 0; thread_num < MAX_NUM_WORKERS; thread_num++)
		restart_one_waiting_worker (thread_num, restart_flags);
}

void restart_one_waiting_worker (
	int	thread_num,
	int	restart_flags)
{
	if (restart_flags & RESTART_USER_START &&
	    USER_START_OR_STOP_INITIALIZED[thread_num]) {
		gwevent_signal (&USER_START_OR_STOP[thread_num]);
	}
	if (restart_flags & RESTART_WORK_AVAILABLE &&
	    WORK_AVAILABLE_OR_STOP_INITIALIZED[thread_num]) {
		gwevent_signal (&WORK_AVAILABLE_OR_STOP[thread_num]);
	}
	if (restart_flags & RESTART_END_PAUSE &&
	    END_PAUSE_OR_STOP_INITIALIZED[thread_num]) {
		gwevent_signal (&END_PAUSE_OR_STOP[thread_num]);
	}
	if (restart_flags & RESTART_LOADAVG &&
	    END_LOADAVG_OR_STOP_INITIALIZED[thread_num]) {
		gwevent_signal (&END_LOADAVG_OR_STOP[thread_num]);
	}
	if (restart_flags & RESTART_MEM_WAIT &&
	    MEM_WAIT_OR_STOP_INITIALIZED[thread_num]) {
		gwevent_signal (&MEM_WAIT_OR_STOP[thread_num]);
	}
	if (restart_flags & RESTART_BATTERY &&
	    OFF_BATTERY_OR_STOP_INITIALIZED[thread_num]) {
		gwevent_signal (&OFF_BATTERY_OR_STOP[thread_num]);
	}
}

void stop_workers_for_escape (void)
{
	if (WORKERS_ACTIVE) {
		OutputStr (MAIN_THREAD_NUM, "Stopping all worker windows.\n");
		WORKERS_STOPPING = TRUE;
		restart_waiting_workers (RESTART_ALL);
		raiseAllWorkersPriority ();
	}
}

void stop_workers_for_restart (void)
{
	if (WORKERS_ACTIVE &&
	    LAUNCH_TYPE == LD_CONTINUE &&
	    ! STOP_FOR_RESTART) {
		OutputStr (MAIN_THREAD_NUM, "Restarting all worker windows.\n");
		STOP_FOR_RESTART = TRUE;
		restart_waiting_workers (RESTART_ALL);
		if (NUM_WORKERS > WORKERS_ACTIVE)
			create_worker_windows (NUM_WORKERS);
	}
}

void stop_workers_for_add_files (void)
{
	if (WORKERS_ACTIVE && ! STOP_FOR_RESTART) {
		OutputStr (MAIN_THREAD_NUM, "Restarting all worker windows to process .add file.\n");
		STOP_FOR_RESTART = TRUE;
		restart_waiting_workers (RESTART_ALL);
	}
}

void stop_workers_for_reread_ini (void)
{
	if (WORKERS_ACTIVE && ! STOP_FOR_REREAD_INI) {
		OutputStr (MAIN_THREAD_NUM, "Restarting all worker windows with new timed INI settings.\n");
		STOP_FOR_REREAD_INI = TRUE;
		restart_waiting_workers (RESTART_ALL);
	}
}

void stop_worker_for_mem_changed (
	int	thread_num)
{
	if (WORKERS_ACTIVE && ! STOP_FOR_MEM_CHANGED[thread_num]) {
		OutputStr (thread_num, "Restarting worker with new memory settings.\n");
		MEM_FLAGS[thread_num] |= MEM_RESTARTING;
		STOP_FOR_MEM_CHANGED[thread_num] = 1;
		restart_one_waiting_worker (thread_num, RESTART_ALL);
	}
}

void stop_worker_for_advanced_test (
	int	thread_num)
{
	if (WORKERS_ACTIVE && ! STOP_FOR_PRIORITY_WORK[thread_num]) {
		OutputStr (thread_num, "Restarting worker to do LL test.\n");
		STOP_FOR_PRIORITY_WORK[thread_num] = 1;
	}
}

void stop_worker_for_priority_work (
	int	thread_num)
{
	if (WORKERS_ACTIVE && ! STOP_FOR_PRIORITY_WORK[thread_num]) {
		OutputStr (thread_num, "Restarting worker to do priority work.\n");
		STOP_FOR_PRIORITY_WORK[thread_num] = 1;
	}
}

void stop_workers_for_throttle (void)
{
	if (WORKERS_ACTIVE)
		memset (STOP_FOR_THROTTLE, 1, sizeof (STOP_FOR_THROTTLE));
}

void stop_worker_for_abort (
	int	thread_num)
{
	if (WORKERS_ACTIVE)
		STOP_FOR_ABORT[thread_num] = 1;
}

void start_save_files_timer ()
{
	add_timed_event (TE_SAVE_FILES, DISK_WRITE_TIME * 60);
}

void stop_save_files_timer ()
{
	delete_timed_event (TE_SAVE_FILES);
}

void saveFilesTimer ()
{
	memset (WRITE_SAVE_FILES, 1, sizeof (WRITE_SAVE_FILES));
	start_save_files_timer ();
}

int testSaveFilesFlag (
	int	thread_num)
{
	if (WRITE_SAVE_FILES[thread_num]) {
		WRITE_SAVE_FILES[thread_num] = 0;
		return (TRUE);
	}
	return (FALSE);
}

void start_Jacobi_timer ()
{
	add_timed_event (TE_JACOBI, JACOBI_TIME * 60 * 60);
}

void stop_Jacobi_timer ()
{
	delete_timed_event (TE_JACOBI);
}

void JacobiTimer ()
{
	memset (JACOBI_ERROR_CHECK, 1, sizeof (JACOBI_ERROR_CHECK));
	start_Jacobi_timer ();
}

int testJacobiFlag (
	int	thread_num)
{
	if (JACOBI_ERROR_CHECK[thread_num]) {
		JACOBI_ERROR_CHECK[thread_num] = 0;
		return (TRUE);
	}
	return (FALSE);
}

void read_mem_info (void)
{
	const char *p;
	int	tnum;
	unsigned int seconds, seconds_until_reread;

	if (!MEM_MUTEX_INITIALIZED) {
		MEM_MUTEX_INITIALIZED = 1;
		gwmutex_init (&MEM_MUTEX);
	}

	gwmutex_lock (&MEM_MUTEX);

	delete_timed_event (TE_MEM_CHANGE);

	seconds_until_reread = 0;
	AVAIL_MEM = IniGetTimedInt (INI_FILE, "Memory", 256, &seconds);
	if (seconds && (seconds_until_reread == 0 || seconds < seconds_until_reread))
		seconds_until_reread = seconds;
	for (tnum = 0; tnum < (int) MAX_NUM_WORKERS; tnum++) {
		char	section_name[32];
		sprintf (section_name, "Worker #%d", tnum+1);
		AVAIL_MEM_PER_WORKER[tnum] = IniSectionGetTimedInt (INI_FILE, section_name, "Memory", AVAIL_MEM, &seconds);
		if (seconds && (seconds_until_reread == 0 || seconds < seconds_until_reread))
			seconds_until_reread = seconds;
	}

	MAX_MEM = 256;
	p = IniSectionGetStringRaw (INI_FILE, NULL, "Memory");
	if (p != NULL) for ( ; ; ) {
		unsigned long mem = atol (p);
		if (mem > MAX_MEM) MAX_MEM = mem;
		p = strstr (p, " else ");
		if (p == NULL) break;
		p = p + 6;
	}

	MAX_HIGH_MEM_WORKERS = IniGetTimedInt (INI_FILE, "MaxHighMemWorkers", 1, &seconds);
	if (seconds == 0 && MAX_HIGH_MEM_WORKERS < 1) MAX_HIGH_MEM_WORKERS = 1;
	if (seconds && (seconds_until_reread == 0 || seconds < seconds_until_reread)) seconds_until_reread = seconds;

	if (seconds_until_reread) add_timed_event (TE_MEM_CHANGE, seconds_until_reread);

	gwmutex_unlock (&MEM_MUTEX);
}

void init_mem_state (void)
{
	int	i;

	for (i = 0; i < MAX_NUM_WORKERS; i++) {
		MEM_FLAGS[i] = MEM_USAGE_NOT_SET;
		MEM_IN_USE[i] = DEFAULT_MEM_USAGE;
		MEM_RESTART_FLAGS[i] = 0;
	}
}

void clear_memory_restart_flags (
	int	thread_num)
{
	MEM_RESTART_FLAGS[thread_num] = 0;
}

void set_default_memory_usage (
	int	thread_num)
{
	MEM_FLAGS[thread_num] = MEM_USAGE_NOT_SET;
	MEM_IN_USE[thread_num] = DEFAULT_MEM_USAGE;

	MEM_RESTART_FLAGS[thread_num] &= ~MEM_RESTART_MAX_MEM_CHANGE;
	MEM_RESTART_FLAGS[thread_num] &= ~MEM_RESTART_IF_MORE;
}

void set_restart_if_max_memory_change (
	int	thread_num)
{
	MEM_RESTART_FLAGS[thread_num] |= MEM_RESTART_MAX_MEM_CHANGE;
}

void clear_restart_if_max_memory_change (
	int	thread_num)
{
	MEM_RESTART_FLAGS[thread_num] &= ~MEM_RESTART_MAX_MEM_CHANGE;
}

void set_restart_if_more_memory_available (
	int	thread_num,
	unsigned int memory)		
{
	MEM_RESTART_FLAGS[thread_num] |= MEM_RESTART_IF_MORE;
	MEM_RESTART_IF_MORE_AMOUNT[thread_num] = memory;
}

int avail_mem_not_sufficient (
	int	thread_num,
	unsigned long min_memory,	
	unsigned long desired_memory)	
{
	OutputStr (thread_num, "Other workers are using lots of memory now.\n");
	if (MEM_RESTART_FLAGS[thread_num] & MEM_RESTART_MORE_AVAIL) {
		if (min_memory < MEM_RESTART_MIN_AMOUNT[thread_num])
			MEM_RESTART_MIN_AMOUNT[thread_num] = min_memory;
		if (desired_memory < MEM_RESTART_DESIRED_AMOUNT[thread_num])
			MEM_RESTART_DESIRED_AMOUNT[thread_num] = desired_memory;
	} else {
		MEM_RESTART_FLAGS[thread_num] |= MEM_RESTART_MORE_AVAIL;
		MEM_RESTART_MIN_AMOUNT[thread_num] = min_memory;
		MEM_RESTART_DESIRED_AMOUNT[thread_num] = desired_memory;
	}
	return (STOP_NOT_ENOUGH_MEM);
}

int are_threads_using_lots_of_memory (
	int	thread_num)
{
	int	max_high_mem, i;
	unsigned int high_mem_threshold;

	max_high_mem = MAX_HIGH_MEM_WORKERS;
	if (max_high_mem >= (int) NUM_WORKERS) return (FALSE);
	if (max_high_mem <= 0) return (TRUE);

	high_mem_threshold = IniGetInt (INI_FILE, "HighMemThreshold", 250);
	for (i = 0; i < (int) NUM_WORKERS; i++)
		if (i != thread_num &&
		    ((MEM_FLAGS[i] & MEM_WILL_BE_VARIABLE_USAGE) ||
		     (MEM_FLAGS[i] & MEM_VARIABLE_USAGE && MEM_IN_USE[i] >= high_mem_threshold))) {
			max_high_mem--;
			if (max_high_mem == 0) return (TRUE);
		}
	return (FALSE);
}

int set_memory_usage (
	int	thread_num,
	int	flags,		
	unsigned long memory)	
{
	int	i, best_thread, worst_thread, all_threads_set;
	unsigned long mem_usage;

	gwmutex_lock (&MEM_MUTEX);

	if (flags & MEM_VARIABLE_USAGE)
		MEM_FLAGS[thread_num] |= MEM_VARIABLE_USAGE;
	else
		MEM_FLAGS[thread_num] &= ~MEM_VARIABLE_USAGE;
	MEM_FLAGS[thread_num] &= ~MEM_WILL_BE_VARIABLE_USAGE;

	if (flags & MEM_USAGE_NOT_SET)
		MEM_FLAGS[thread_num] |= MEM_USAGE_NOT_SET;
	else
		MEM_FLAGS[thread_num] &= ~MEM_USAGE_NOT_SET;
	MEM_FLAGS[thread_num] &= ~MEM_RESTARTING;

	MEM_IN_USE[thread_num] = memory;

	mem_usage = 0;
	worst_thread = -1;
	for (i = 0; i < (int) NUM_WORKERS; i++) {
		mem_usage += MEM_IN_USE[i];
		if ((MEM_FLAGS[i] & MEM_VARIABLE_USAGE ||
		     MEM_FLAGS[i] & MEM_WILL_BE_VARIABLE_USAGE) &&
		    (worst_thread == -1 ||
		     MEM_IN_USE[i] > MEM_IN_USE[worst_thread]))
			worst_thread = i;
	}

	if (mem_usage > AVAIL_MEM + 250) {

		if (worst_thread == thread_num) {
			set_default_memory_usage (thread_num);
			gwmutex_unlock (&MEM_MUTEX);
			return (TRUE);
		}

		if (worst_thread >= 0 && MEM_FLAGS[worst_thread] & MEM_VARIABLE_USAGE) {
			stop_worker_for_mem_changed (worst_thread);

			MEM_FLAGS[thread_num] |= MEM_WAITING;
			gwevent_init (&MEM_WAIT_OR_STOP[thread_num]);
			gwevent_reset (&MEM_WAIT_OR_STOP[thread_num]);
			MEM_WAIT_OR_STOP_INITIALIZED[thread_num] = 1;
			gwmutex_unlock (&MEM_MUTEX);
			gwevent_wait (&MEM_WAIT_OR_STOP[thread_num], 20);
			gwmutex_lock (&MEM_MUTEX);
			MEM_WAIT_OR_STOP_INITIALIZED[thread_num] = 0;
			gwevent_destroy (&MEM_WAIT_OR_STOP[thread_num]);
			MEM_FLAGS[thread_num] &= ~MEM_WAITING;
		}
	}

	all_threads_set = TRUE;
	for (i = 0; i < (int) NUM_WORKERS; i++) {
		if (MEM_FLAGS[i] & MEM_WILL_BE_VARIABLE_USAGE) continue;
		if (MEM_FLAGS[i] & MEM_USAGE_NOT_SET ||
		    MEM_FLAGS[i] & MEM_RESTARTING) {
			all_threads_set = FALSE;
			break;
		}
	}

	if (all_threads_set) {
		best_thread = -1;
		for (i = 0; i < (int) NUM_WORKERS; i++) {
			if (MEM_FLAGS[i] & MEM_WAITING) {
				best_thread = i;
				break;
			}
			if (MEM_FLAGS[i] & MEM_WILL_BE_VARIABLE_USAGE &&
			    (best_thread == -1 ||
			     MEM_IN_USE[i] < MEM_IN_USE[best_thread]))
				best_thread = i;
		}
		if (best_thread >= 0) {
			restart_one_waiting_worker (best_thread, RESTART_MEM_WAIT);
			all_threads_set = FALSE;
		}
	}

	if (all_threads_set && AVAIL_MEM > mem_usage + 100) {
		for (i = 0; i < (int) NUM_WORKERS; i++) {
			if (! (MEM_RESTART_FLAGS[i] & MEM_RESTART_TOO_MANY_HIGHMEM)) continue;
			if (are_threads_using_lots_of_memory (i)) continue;
			stop_worker_for_mem_changed (i);
			all_threads_set = FALSE;
		}
	}

	if (all_threads_set && AVAIL_MEM > mem_usage + 100) {
		best_thread = -1;
		for (i = 0; i < (int) NUM_WORKERS; i++) {
			if (MEM_RESTART_FLAGS[i] & MEM_RESTART_IF_MORE &&
			    MEM_RESTART_IF_MORE_AMOUNT[i] < AVAIL_MEM - mem_usage)
				best_thread = i;
		}
		if (best_thread >= 0) {
			stop_worker_for_mem_changed (best_thread);
			all_threads_set = FALSE;
		}
	}
		
	if (all_threads_set && AVAIL_MEM > mem_usage + 100) {
		best_thread = -1;
		for (i = 0; i < (int) NUM_WORKERS; i++) {
			if (MEM_RESTART_FLAGS[i] & MEM_RESTART_MORE_AVAIL &&
			    MEM_RESTART_MIN_AMOUNT[i] < AVAIL_MEM - mem_usage)
				best_thread = i;
		}
		if (best_thread >= 0) {
			stop_worker_for_mem_changed (best_thread);
			all_threads_set = FALSE;
		}
	}

	gwmutex_unlock (&MEM_MUTEX);
	return (FALSE);
}

unsigned long max_mem (
	int	thread_num)
{
	char	section_name[32];
	unsigned long memory;
	const char *p;

	sprintf (section_name, "Worker #%d", thread_num+1);
	p = IniSectionGetStringRaw (INI_FILE, section_name, "Memory");
	if (p == NULL) return (MAX_MEM);

	memory = 0;
	for ( ; ; ) {
		unsigned long temp = atol (p);
		if (temp > memory) memory = temp;
		p = strstr (p, " else ");
		if (p == NULL) break;
		p = p + 6;
	}

	if (memory < MAX_MEM) return (memory);
	return (MAX_MEM);
}

int avail_mem (
	int	thread_num,
	unsigned long minimum_memory,	
	unsigned long desired_memory,	
	unsigned int *memory)		
{
	int	i, fixed_threads[MAX_NUM_WORKERS];
	unsigned long fixed_usage, variable_usage, num_variable_threads, avail, diff;

	if (is_LowMemWhileRunning_active (thread_num)) {
		MEM_RESTART_FLAGS[thread_num] |= MEM_RESTART_LOWMEM_ENDS;
		return (STOP_NOT_ENOUGH_MEM);
	}

	if (IniGetInt (INI_FILE, "OnlyRunStage2WithMaxMemory", 0) && AVAIL_MEM != MAX_MEM) {
		OutputStr (thread_num, "Waiting for maximum available memory to run stage 2.\n");
		MEM_RESTART_FLAGS[thread_num] |= MEM_RESTART_MAX_MEM_AVAILABLE;
		return (STOP_NOT_ENOUGH_MEM);
	}

	gwmutex_lock (&MEM_MUTEX);

	if (are_threads_using_lots_of_memory (thread_num)) {
		MEM_RESTART_FLAGS[thread_num] |= MEM_RESTART_TOO_MANY_HIGHMEM;
		gwmutex_unlock (&MEM_MUTEX);
		OutputStr (thread_num, "Exceeded limit on number of workers that can use lots of memory.\n");
		return (STOP_NOT_ENOUGH_MEM);
	}

	MEM_FLAGS[thread_num] |= MEM_WILL_BE_VARIABLE_USAGE;
	MEM_IN_USE[thread_num] = desired_memory;

	for (i = 0; i < (int) NUM_WORKERS; i++) {
		if (i == thread_num) continue;
		if (MEM_FLAGS[i] & MEM_USAGE_NOT_SET || MEM_FLAGS[i] & MEM_RESTARTING) {
			gwevent_init (&MEM_WAIT_OR_STOP[thread_num]);
			gwevent_reset (&MEM_WAIT_OR_STOP[thread_num]);
			MEM_WAIT_OR_STOP_INITIALIZED[thread_num] = 1;
			gwmutex_unlock (&MEM_MUTEX);
			gwevent_wait (&MEM_WAIT_OR_STOP[thread_num], 5 + thread_num);
			gwmutex_lock (&MEM_MUTEX);
			MEM_WAIT_OR_STOP_INITIALIZED[thread_num] = 0;
			gwevent_destroy (&MEM_WAIT_OR_STOP[thread_num]);
			break;
		}
	}

	fixed_usage = 0;
	variable_usage = 0;
	num_variable_threads = 0;
	for (i = 0; i < (int) NUM_WORKERS; i++) {
		if (MEM_FLAGS[i] & MEM_VARIABLE_USAGE ||
		    MEM_FLAGS[i] & MEM_WILL_BE_VARIABLE_USAGE) {
			num_variable_threads++;
			variable_usage += MEM_IN_USE[i];
			fixed_threads[i] = FALSE;
		} else {
			fixed_usage += MEM_IN_USE[i];
			fixed_threads[i] = TRUE;
		}
	}

	avail = (AVAIL_MEM > fixed_usage) ? AVAIL_MEM - fixed_usage : 0;
	for (i = 0; i < (int) NUM_WORKERS; i++) {
		unsigned long avail_per_worker;
		if (i == thread_num) continue;
		if (fixed_threads[i]) continue;
		avail_per_worker = avail / num_variable_threads;

		if (MEM_FLAGS[i] & MEM_VARIABLE_USAGE &&
		    (MEM_IN_USE[i] < avail_per_worker ||
		     fixed_usage + variable_usage <= AVAIL_MEM)) {
			avail -= MEM_IN_USE[i];
			fixed_threads[i] = TRUE;
			num_variable_threads--;
			i = -1;	    
			continue;
		}

		if (MEM_FLAGS[i] & MEM_WILL_BE_VARIABLE_USAGE &&
		    AVAIL_MEM_PER_WORKER[i] < avail_per_worker) {
			avail -= AVAIL_MEM_PER_WORKER[i];
			fixed_threads[i] = TRUE;
			num_variable_threads--;
			i = -1;	    
			continue;
		}
	}
	avail = avail / num_variable_threads;

	gwmutex_unlock (&MEM_MUTEX);

	if (avail < minimum_memory)
		return (avail_mem_not_sufficient (thread_num, minimum_memory, desired_memory));

	if (fixed_usage + variable_usage <= AVAIL_MEM)
		*memory = desired_memory +
			  (AVAIL_MEM - (fixed_usage + variable_usage)) / num_variable_threads;
	else
		*memory = avail;

	if (*memory > AVAIL_MEM_PER_WORKER[thread_num])
		*memory = AVAIL_MEM_PER_WORKER[thread_num];

	diff = desired_memory - minimum_memory;
	if (*memory <= minimum_memory + diff / 4)
		set_restart_if_more_memory_available (thread_num, diff / 4);
	else if (*memory <= minimum_memory + diff / 2)
		set_restart_if_more_memory_available (thread_num, diff / 2);

	return (0);
}

void mem_settings_have_changed (void)
{
	unsigned int old_avail_mem, old_max_mem;
	int	tnum;

	old_avail_mem = AVAIL_MEM;
	old_max_mem = MAX_MEM;
	read_mem_info ();

	if (! WORKERS_ACTIVE) return;

	if (MAX_MEM != old_max_mem)
		for (tnum = 0; tnum < (int) NUM_WORKERS; tnum++)
			if (MEM_RESTART_FLAGS[tnum] & MEM_RESTART_MAX_MEM_CHANGE)
				stop_worker_for_mem_changed (tnum);

	if (AVAIL_MEM == MAX_MEM)
		for (tnum = 0; tnum < (int) NUM_WORKERS; tnum++)
			if (MEM_RESTART_FLAGS[tnum] & MEM_RESTART_MAX_MEM_AVAILABLE)
				stop_worker_for_mem_changed (tnum);

	if (AVAIL_MEM > old_avail_mem)
		for (tnum = 0; tnum < (int) NUM_WORKERS; tnum++)
			if (MEM_RESTART_FLAGS[tnum] & MEM_RESTART_MORE_AVAIL)
				stop_worker_for_mem_changed (tnum);

	for (tnum = 0; tnum < (int) NUM_WORKERS; tnum++)
		if (MEM_FLAGS[tnum] & MEM_VARIABLE_USAGE &&
		    MEM_IN_USE[tnum] > AVAIL_MEM_PER_WORKER[tnum] + 10)
			stop_worker_for_mem_changed (tnum);

	if (AVAIL_MEM < old_avail_mem) {
		unsigned long mem_usage;
		int	worst_thread;

		mem_usage = 0;
		worst_thread = -1;
		for (tnum = 0; tnum < (int) NUM_WORKERS; tnum++) {
			mem_usage += MEM_IN_USE[tnum];
			if (MEM_FLAGS[tnum] & MEM_USAGE_NOT_SET ||
			    MEM_FLAGS[tnum] & MEM_RESTARTING ||
			    MEM_FLAGS[tnum] & MEM_WILL_BE_VARIABLE_USAGE) {
				worst_thread = -1;
				break;
			}
			if (MEM_FLAGS[tnum] & MEM_VARIABLE_USAGE &&
			    (worst_thread == -1 ||
			     MEM_IN_USE[tnum] > MEM_IN_USE[worst_thread]))
				worst_thread = tnum;
		}
		if (mem_usage > AVAIL_MEM + 32 && worst_thread != -1)
			stop_worker_for_mem_changed (worst_thread);
	}
}

void stop_high_memory_workers (void)
{
	int	i;

	if (! WORKERS_ACTIVE) return;

	gwmutex_lock (&MEM_MUTEX);

	for (i = 0; i < (int) NUM_WORKERS; i++) {
		if (MEM_FLAGS[i] & MEM_VARIABLE_USAGE ||
		    MEM_FLAGS[i] & MEM_WILL_BE_VARIABLE_USAGE)
			stop_worker_for_mem_changed (i);
	}

	gwmutex_unlock (&MEM_MUTEX);
}

void restart_high_memory_workers (void)
{
	int	tnum;

	if (! WORKERS_ACTIVE) return;

	for (tnum = 0; tnum < (int) NUM_WORKERS; tnum++)
		if (MEM_RESTART_FLAGS[tnum] & MEM_RESTART_LOWMEM_ENDS) {
			MEM_RESTART_FLAGS[tnum] &= ~MEM_RESTART_LOWMEM_ENDS;
			stop_worker_for_mem_changed (tnum);
		}
}

void start_battery_timer (void)
{
	if (RUN_ON_BATTERY) return;
	add_timed_event (TE_BATTERY_CHECK, TE_BATTERY_CHECK_FREQ);
}

void stop_battery_timer (void)
{
	delete_timed_event (TE_BATTERY_CHECK);
}

void run_on_battery_changed (void)
{
	if (WORKERS_ACTIVE) {
		stop_battery_timer ();
		start_battery_timer ();
	}
}

void test_battery (void)
{
	if (OnBattery ()) STOP_FOR_BATTERY = TRUE;
	else if (STOP_FOR_BATTERY) {
		STOP_FOR_BATTERY = FALSE;
		restart_waiting_workers (RESTART_BATTERY);
	}
}

void implement_stop_battery (
	int	thread_num)
{

	title (thread_num, "Battery Pause");
	OutputStr (thread_num, "Worker stopped while on battery power.\n");
	ChangeIcon (thread_num, IDLE_ICON);

	if (OnBattery ()) {
		gwevent_init (&OFF_BATTERY_OR_STOP[thread_num]);
		gwevent_reset (&OFF_BATTERY_OR_STOP[thread_num]);
		OFF_BATTERY_OR_STOP_INITIALIZED[thread_num] = 1;
		gwevent_wait (&OFF_BATTERY_OR_STOP[thread_num], 0);
		OFF_BATTERY_OR_STOP_INITIALIZED[thread_num] = 0;
		gwevent_destroy (&OFF_BATTERY_OR_STOP[thread_num]);
	}

	title (thread_num, "Working");
	OutputStr (thread_num, "AC power restored, restarting worker.\n");
	ChangeIcon (thread_num, WORKING_ICON);
}

void implement_stop_autobench (
	int	thread_num)
{

	title (thread_num, "Benchmark Pause");
	OutputStr (thread_num, "Worker stopped while running needed benchmarks.\n");
	ChangeIcon (thread_num, IDLE_ICON);

	gwevent_wait (&AUTOBENCH_EVENT, 0);

	title (thread_num, "Working");
	OutputStr (thread_num, "Benchmarks complete, restarting worker.\n");
	ChangeIcon (thread_num, WORKING_ICON);
}

void start_priority_work_timer (void)
{
	if (SEQUENTIAL_WORK == 1) return;
	add_timed_event (TE_PRIORITY_WORK, TE_PRIORITY_WORK_FREQ);
}

void stop_priority_work_timer (void)
{
	delete_timed_event (TE_PRIORITY_WORK);
}

int isPriorityWork (
	struct work_unit *w)
{
	if (w->work_type == WORK_CERT) return (TRUE);
	if (w->work_type == WORK_ADVANCEDTEST) return (TRUE);
	if (SEQUENTIAL_WORK == 0) {
		if ((w->work_type == WORK_TEST || w->work_type == WORK_DBLCHK) && (w->sieve_depth < w->factor_to || !w->pminus1ed))
			return (TRUE);
		if (w->work_type == WORK_PRP && (w->sieve_depth < w->factor_to || w->tests_saved > 0.0))
			return (TRUE);
	}
	return (FALSE);
}

void check_for_priority_work (void)
{
	int	tnum;
	struct work_unit *w;

	for (tnum = 0; tnum < (int) NUM_WORKERS; tnum++) {
		w = NULL;
		for ( ; ; ) {
			w = getNextWorkToDoLine (tnum, w, SHORT_TERM_USE);
			if (w == NULL) break;
			if (isPriorityWork (w)) {
				if (isWorkUnitActive (w)) {
					decrementWorkUnitUseCount (w, SHORT_TERM_USE);
					break;
				}
				stop_worker_for_priority_work (tnum);
			}
		}
       }
}

void mark_workers_active (
	int	thread_num)	
{
	int	i;

	memset (ACTIVE_WORKERS, 0, sizeof (ACTIVE_WORKERS));
	for (i = 0; i < thread_num; i++) ACTIVE_WORKERS[i] = 1;
	if (thread_num <= 0) ACTIVE_WORKERS[-thread_num] = 1;
}

void start_one_worker (
	int	thread_num)
{
	if (thread_num < 0 || thread_num >= (int) WORKERS_ACTIVE) {
		OutputStr (MAIN_THREAD_NUM, "Worker number out of range.\n");
		return;
	}
	if (ACTIVE_WORKERS[thread_num]) {
		OutputStr (MAIN_THREAD_NUM, "Worker is already running.\n");
		return;
	}
	ACTIVE_WORKERS[thread_num] = 1;

	restart_one_waiting_worker (thread_num, RESTART_USER_START);
}

void stop_one_worker (
	int	thread_num)
{
	if (thread_num < 0 || thread_num >= (int) WORKERS_ACTIVE) {
		OutputStr (MAIN_THREAD_NUM, "Worker number out of range.\n");
		return;
	}
	if (!ACTIVE_WORKERS[thread_num]) {
		OutputStr (MAIN_THREAD_NUM, "Worker is already stopped.\n");
		return;
	}
	ACTIVE_WORKERS[thread_num] = 0;
}

void implement_stop_one_worker (
	int	thread_num)
{

	if (ACTIVE_WORKERS[thread_num]) return;

	OutputStr (thread_num, "Stopping worker at user request.\n");
	ChangeIcon (thread_num, IDLE_ICON);	

	set_memory_usage (thread_num, 0, 0);

	gwevent_init (&USER_START_OR_STOP[thread_num]);
	gwevent_reset (&USER_START_OR_STOP[thread_num]);
	USER_START_OR_STOP_INITIALIZED[thread_num] = 1;
	gwevent_wait (&USER_START_OR_STOP[thread_num], 0);
	USER_START_OR_STOP_INITIALIZED[thread_num] = 0;
	gwevent_destroy (&USER_START_OR_STOP[thread_num]);

	OutputStr (thread_num, "Resuming worker at user request.\n");
	ChangeIcon (thread_num, WORKING_ICON);
}

unsigned int active_workers_count (void)
{
	unsigned int i, count;

	for (i = count = 0; i < WORKERS_ACTIVE; i++)
		if (ACTIVE_WORKERS[i]) count++;
	return (count);
}

void parse_pause_info (
       char	*buf,		
       int	thread_num,	
       int	low_mem)	
{
	struct pause_info *data;
	char	*p, *bracket, *comma;

	if (*buf == 0) return;

	for (p = buf; ; p = comma + 1) {
		comma = strchr (p, ',');
		if (comma != NULL) *comma = 0;

		data = (struct pause_info *) malloc (sizeof (struct pause_info));
		if (data == NULL) return;
		data->next = PAUSE_DATA;
		PAUSE_DATA = data;

		data->thread_num = thread_num;
		data->low_mem = low_mem;
		bracket = strchr (p, '[');
		if (bracket != NULL) {
			*bracket = 0;
			data->workers_affected = atoi (bracket+1);
		} else
			data->workers_affected = MAX_NUM_WORKERS;

		if (!low_mem && *p == '*')
			data->program_name = NULL;
		else {
			data->program_name = (char *) malloc (strlen (p) + 1);
			if (data->program_name == NULL) return;
			strupper (p);
			strcpy (data->program_name, p);
		}

		if (comma == NULL) break;
	}
}

void read_pause_info (void)
{
	int	tnum;
	char	buf[250];
	unsigned int seconds, seconds_until_reread;

	if (!PAUSE_MUTEX_INITIALIZED) {
		PAUSE_MUTEX_INITIALIZED = 1;
		gwmutex_init (&PAUSE_MUTEX);
	}

	gwmutex_lock (&PAUSE_MUTEX);

	delete_timed_event (TE_READ_PAUSE_DATA);

	while (PAUSE_DATA != NULL) {
		struct pause_info *p;
		p = PAUSE_DATA;
		PAUSE_DATA = p->next;
		if (p->program_name != NULL) free (p->program_name);
		free (p);
	}

	seconds_until_reread = 0;
	IniGetTimedString (INI_FILE, "PauseWhileRunning", buf, sizeof (buf), NULL, &seconds);
	if (seconds && (seconds_until_reread == 0 || seconds < seconds_until_reread))
		seconds_until_reread = seconds;
	parse_pause_info (buf, ALL_WORKERS, FALSE);
	for (tnum = 0; tnum < (int) NUM_WORKERS; tnum++) {
		char	section_name[32];
		sprintf (section_name, "Worker #%d", tnum+1);
		IniSectionGetTimedString (INI_FILE, section_name, "PauseWhileRunning", buf, sizeof (buf), NULL, &seconds);
		if (seconds && (seconds_until_reread == 0 || seconds < seconds_until_reread))
			seconds_until_reread = seconds;
		parse_pause_info (buf, tnum, FALSE);
	}
	PAUSE_WHILE_RUNNING_FREQ = IniGetTimedInt (INI_FILE, "PauseCheckInterval", 10, &seconds);
	if (seconds && (seconds_until_reread == 0 || seconds < seconds_until_reread))
		seconds_until_reread = seconds;

	IniGetTimedString (INI_FILE, "LowMemWhileRunning", buf, sizeof (buf), NULL, &seconds);
	if (seconds && (seconds_until_reread == 0 || seconds < seconds_until_reread))
		seconds_until_reread = seconds;
	parse_pause_info (buf, ALL_WORKERS, TRUE);

	if (seconds_until_reread)
		add_timed_event (TE_READ_PAUSE_DATA, seconds_until_reread);

	if (PAUSEABLE_WORKERS_RUNNING) {
		delete_timed_event (TE_PAUSE_WHILE);
		checkPauseWhileRunning ();
	}

	gwmutex_unlock (&PAUSE_MUTEX);
}

void start_pause_while_running_timer (void)
{
	PAUSEABLE_WORKERS_RUNNING = TRUE;
	if (PAUSE_DATA == NULL) return;
	add_timed_event (TE_PAUSE_WHILE, 0);		
}

void stop_pause_while_running_timer (void)
{
	PAUSEABLE_WORKERS_RUNNING = FALSE;
	delete_timed_event (TE_PAUSE_WHILE);
}

int best_pause_candidate (
	struct pause_info **workers_to_pause)
{
	int	i;

	for (i = 0; i < (int) NUM_WORKERS; i++) {
		if (!ACTIVE_WORKERS[i]) continue;
		if (workers_to_pause[i] != NULL) continue;
		if (WORK_AVAILABLE_OR_STOP_INITIALIZED[i]) return (i);
		if (STOP_FOR_PAUSE[i] != NULL) return (i);
	}

	for (i = 0; i < (int) NUM_WORKERS; i++) {
		if (!ACTIVE_WORKERS[i]) continue;
		if (workers_to_pause[i] != NULL) continue;
		if (MEM_FLAGS[i] & MEM_USAGE_NOT_SET) return (i);
		if (MEM_FLAGS[i] & MEM_RESTARTING) return (i);
		if (MEM_RESTART_FLAGS[i] & ~MEM_RESTART_MAX_MEM_CHANGE) return (i);
	}

	for (i = 0; i < (int) NUM_WORKERS; i++) {
		if (!ACTIVE_WORKERS[i]) continue;
		if (workers_to_pause[i] != NULL) continue;
		return (i);
	}

	return (0);
}

void checkPauseWhileRunning (void)
{
	struct pause_info *p, *lowmem;
	struct pause_info *workers_to_pause[MAX_NUM_WORKERS];
	int	i, named_program_entries;

	named_program_entries = FALSE;
	for (p = PAUSE_DATA; p != NULL; p = p->next) {
		p->matching_program[0] = 0;
		if (p->program_name != NULL) named_program_entries = TRUE;
	}

	checkPauseListCallback ();

	lowmem = NULL;
	for (p = PAUSE_DATA; p != NULL; p = p->next) {
		if (!p->low_mem) continue;
		if (p->matching_program[0]) lowmem = p;
	}
	p = STOP_FOR_LOW_MEMORY;
	STOP_FOR_LOW_MEMORY = lowmem;
	if (p == NULL && STOP_FOR_LOW_MEMORY != NULL) {
		char	buf[150];
		sprintf (buf, "Entering a period of low memory usage because %s is running.\n", lowmem->matching_program);
		OutputStr (MAIN_THREAD_NUM, buf);
		stop_high_memory_workers ();
	}
	if (p != NULL && STOP_FOR_LOW_MEMORY != NULL && p != STOP_FOR_LOW_MEMORY) {
		char	buf[150];
		sprintf (buf, "Now using little memory because %s is running.\n", lowmem->matching_program);
		OutputStr (MAIN_THREAD_NUM, buf);
	}
	if (p != NULL && STOP_FOR_LOW_MEMORY == NULL) {
		restart_high_memory_workers ();
	}

	memset (workers_to_pause, 0, sizeof (workers_to_pause));
	for (p = PAUSE_DATA; p != NULL; p = p->next) {
		if (p->thread_num == ALL_WORKERS) continue;
		if (p->low_mem) continue;
		if (p->program_name == NULL || p->matching_program[0])
			workers_to_pause[p->thread_num] = p;
	}

	for (p = PAUSE_DATA; p != NULL; p = p->next) {
		if (p->low_mem) continue;
		if (p->program_name == NULL || p->matching_program[0]) {
			int	count;
			count = p->workers_affected;
			if (p->thread_num != ALL_WORKERS) count--;
			for (i = 0; i < count; i++)
				workers_to_pause[best_pause_candidate (workers_to_pause)] = p;
		}
	}

	for (i = 0; i < (int) NUM_WORKERS; i++) {
		p = STOP_FOR_PAUSE[i];
		STOP_FOR_PAUSE[i] = workers_to_pause[i];
		if (p != NULL && STOP_FOR_PAUSE[i] != NULL && p != STOP_FOR_PAUSE[i]) {
			char	buf[140];
			sprintf (buf, "Now paused because %s is running.\n", STOP_FOR_PAUSE[i]->matching_program);
			OutputStr (i, buf);
		}
		if (p != NULL && STOP_FOR_PAUSE[i] == NULL)
			restart_one_waiting_worker (i, RESTART_END_PAUSE);
	}

	if (named_program_entries)
		add_timed_event (TE_PAUSE_WHILE, PAUSE_WHILE_RUNNING_FREQ);
}

void isInPauseList (
	char	*program_name)
{
	struct pause_info *p;
	char	buf[512];

	strcpy (buf, program_name);
	strupper (buf);
	for (p = PAUSE_DATA; p != NULL; p = p->next) {
		if (p->program_name != NULL && p->matching_program[0] == 0 && strstr (buf, p->program_name) != NULL) {
			strcpy (buf, program_name);		
			buf[sizeof(p->matching_program)-1] = 0;
			strcpy (p->matching_program, buf);
		}
	}
}

void implement_pause (
	int	thread_num)
{
	struct pause_info *p;
	char	buf[140];

	gwmutex_lock (&PAUSE_MUTEX);

	p = STOP_FOR_PAUSE[thread_num];
	if (p == NULL) {
		gwmutex_unlock (&PAUSE_MUTEX);
		return;
	}

	if (p->program_name == NULL) {
		time_t	sleep_time;
		char	*time_as_string;

		sleep_time = timed_event_fire_time (TE_READ_PAUSE_DATA);
		time_as_string = sleep_time ? ctime (&sleep_time) : "forever";
		if (NUM_WORKERS == 1)
			sprintf (buf, "Sleeping until %s\n", time_as_string);
		else if (p->workers_affected == 1)
			sprintf (buf, "Sleeping one worker until %s\n", time_as_string);
		else if (p->workers_affected == MAX_NUM_WORKERS)
			sprintf (buf, "Sleeping all workers until %s\n", time_as_string);
		else
			sprintf (buf, "Sleeping %d workers until %s\n", p->workers_affected, time_as_string);
		OutputStr (thread_num, buf);
		title (thread_num, "Sleeping");
	} else {
		sprintf (buf, "Pausing because %s is running.\n", p->matching_program);
		OutputStr (thread_num, buf);
		title (thread_num, "Paused");
	}

	gwmutex_unlock (&PAUSE_MUTEX);

	gwevent_init (&END_PAUSE_OR_STOP[thread_num]);
	gwevent_reset (&END_PAUSE_OR_STOP[thread_num]);
	END_PAUSE_OR_STOP_INITIALIZED[thread_num] = 1;
	gwevent_wait (&END_PAUSE_OR_STOP[thread_num], 0);
	END_PAUSE_OR_STOP_INITIALIZED[thread_num] = 0;
	gwevent_destroy (&END_PAUSE_OR_STOP[thread_num]);

	OutputStr (thread_num, "Resuming processing.\n");
	title (thread_num, "Resuming");
}

int is_LowMemWhileRunning_active (
	int	thread_num)
{
	struct pause_info *p;
	char	buf[140];

	gwmutex_lock (&PAUSE_MUTEX);

	p = STOP_FOR_LOW_MEMORY;
	if (p == NULL) {
		gwmutex_unlock (&PAUSE_MUTEX);
		return (FALSE);
	}

	sprintf (buf, "Cannot use lots of memory because %s is running.\n", p->matching_program);
	OutputStr (thread_num, buf);

	gwmutex_unlock (&PAUSE_MUTEX);
	return (TRUE);
}

long LOAD_CHECK_TIME = 0;
double HI_LOAD = 0.0;
double LO_LOAD = 0.0;

void read_load_average_info (void)
{
	HI_LOAD = IniGetFloat (INI_FILE, "MaxLoad", 0.0);
	LO_LOAD = IniGetFloat (INI_FILE, "MinLoad", 0.0);
	LOAD_CHECK_TIME = IniGetInt (INI_FILE, "PauseTime", 20);
}

void start_load_average_timer (void)
{
	if (HI_LOAD <= 0.0 || LOAD_CHECK_TIME <= 0) return;
	if (get_load_average () < 0.0) return;
	add_timed_event (TE_LOAD_AVERAGE, LOAD_CHECK_TIME);
}

void stop_load_average_timer (void)
{
	delete_timed_event (TE_LOAD_AVERAGE);
}

void checkLoadAverage (void)
{
	double	load;
	long	recheck_interval;
	int	i;

	load = get_load_average ();
	recheck_interval = LOAD_CHECK_TIME;

	if (load >= HI_LOAD) {
		double	threads_per_worker;
		int	workers_to_stop;

		threads_per_worker = (double) HW_NUM_CORES / (double) NUM_WORKERS;
		if (threads_per_worker < 1.0) threads_per_worker = 1.0;
		workers_to_stop = (int) ((load - HI_LOAD) / threads_per_worker);
		if (workers_to_stop < 1) workers_to_stop = 1;
		STOP_FOR_LOADAVG = workers_to_stop;
		if (recheck_interval < 65) recheck_interval = 65;
	}

	if (load >= 0.0 && load <= LO_LOAD) {
		for (i = 0; i < (int) NUM_WORKERS; i++) {
			if (END_LOADAVG_OR_STOP_INITIALIZED[i]) {
				restart_one_waiting_worker (i, RESTART_LOADAVG);
				if (recheck_interval < 65) recheck_interval = 65;
				break;
			}
		}
	}

	add_timed_event (TE_LOAD_AVERAGE, recheck_interval);
}

void implement_loadavg (
	int	thread_num)
{
	char	buf[140];

	sprintf (buf, "Pausing due to high load.\n");
	OutputStr (thread_num, buf);
	title (thread_num, "Paused");

	gwevent_init (&END_LOADAVG_OR_STOP[thread_num]);
	gwevent_reset (&END_LOADAVG_OR_STOP[thread_num]);
	END_LOADAVG_OR_STOP_INITIALIZED[thread_num] = 1;
	gwevent_wait (&END_LOADAVG_OR_STOP[thread_num], 0);
	END_LOADAVG_OR_STOP_INITIALIZED[thread_num] = 0;
	gwevent_destroy (&END_LOADAVG_OR_STOP[thread_num]);

	OutputStr (thread_num, "Resuming processing.\n");
	title (thread_num, "Resuming");
}

int	THROTTLE_SLEEP_TIME_IN_SEC = 0;
int	THROTTLE_SLEEP_TIME_IN_MS = 0;

void start_throttle_timer (void)
{
	if (THROTTLE_PCT <= 0 || THROTTLE_PCT >= 100) return;
	THROTTLE_SLEEP_TIME_IN_SEC = (int)
		((double) TE_THROTTLE_FREQ * (100.0 / (double) THROTTLE_PCT - 1.0));
	THROTTLE_SLEEP_TIME_IN_MS = (int)
		((double) (TE_THROTTLE_FREQ + THROTTLE_SLEEP_TIME_IN_SEC) *
			  (100.0 - (double) THROTTLE_PCT) * 10.0);
	add_timed_event (TE_THROTTLE, TE_THROTTLE_FREQ);
}

void stop_throttle_timer (void)
{
	delete_timed_event (TE_THROTTLE);
}

int handleThrottleTimerEvent (void)
{
	stop_workers_for_throttle ();

	return (TE_THROTTLE_FREQ + THROTTLE_SLEEP_TIME_IN_SEC);
}

void implementThrottle (
	int	thread_num)
{
	int	totaltime;

	for (totaltime = 0; totaltime < THROTTLE_SLEEP_TIME_IN_MS; totaltime += 100) {
		if (WORKERS_STOPPING) return;
		Sleep (100);
	}
}

int isKnownMersennePrime (
	unsigned long p)
{
	return (p == 2 || p == 3 || p == 5 || p == 7 || p == 13 || p == 17 || p == 19 || p == 31 || p == 61 || p == 89 || p == 107 ||
		p == 127 || p == 521 || p == 607 || p == 1279 || p == 2203 || p == 2281 || p == 3217 || p == 4253 || p == 4423 ||
		p == 9689 || p == 9941 || p == 11213 || p == 19937 || p == 21701 || p == 23209 || p == 44497 || p == 86243 ||
		p == 110503 || p == 132049 || p == 216091 || p == 756839 || p == 859433 || p == 1257787 || p == 1398269 || p == 2976221 ||
		p == 3021377 || p == 6972593 || p == 13466917 || p == 20996011 || p == 24036583 || p == 25964951 || p == 30402457 ||
		p == 32582657 || p == 37156667 || p == 42643801 || p == 43112609 || p == 57885161 || p == 74207281 || p == 77232917 ||
		p == 82589933);
}

void makestr (
	unsigned long hsw,
	unsigned long msw,
	unsigned long lsw,
	char	*buf)			
{
	int	i, j, k, carry;
	unsigned long x[3];
	char	pow[80];

	x[0] = hsw; x[1] = msw; x[2] = lsw;
	for (i = 0; i < 79; i++) pow[i] = '0', buf[i] = '0';
	pow[78] = '1';
	pow[79] = buf[79] = 0;

	for (i = 3; i--; ) {
		for (j = 0; j < 32; j++) {
			if (x[i] & 1) {
				carry = 0;
				for (k = 79; k--; ) {
					buf[k] = buf[k] - '0' +
						pow[k] - '0' + carry;
					carry = buf[k] / 10;
					buf[k] %= 10;
					buf[k] += '0';
				}
			}
			carry = 0;
			for (k = 79; k--; ) {
				pow[k] = (pow[k] - '0') * 2 + carry;
				carry = pow[k] / 10;
				pow[k] %= 10;
				pow[k] += '0';
			}
			x[i] >>= 1;
		}
	}
	while (buf[0] == '0') safe_strcpy (buf, buf+1);
}

int SleepFive (
	int	thread_num)
{
	int	i;

	OutputStr (thread_num, ERRMSG4);
	BlinkIcon (thread_num, 10);		
	for (i = 0; i < 100; i++) {
		Sleep (100);
		if (WORKERS_STOPPING) return (STOP_ESCAPE);
	}
	ChangeIcon (thread_num, IDLE_ICON);	
	for (i = 0; i < 2900; i++) {
		Sleep (100);
		if (WORKERS_STOPPING) return (STOP_ESCAPE);
	}
	ChangeIcon (thread_num, WORKING_ICON);	
	return (0);
}

void calc_output_frequencies (
	gwhandle *gwdata,		
	double	*output_frequency,	
	double	*output_title_frequency)
{
	int	scaled_freq, title_freq;
	double	exp, temp;

	scaled_freq = (int) IniGetInt (INI_FILE, "ScaleOutputFrequency", 0);
	if (!scaled_freq) {
		*output_frequency = 1.0;
	} else {
		*output_frequency = gwmap_to_timing (1.0, 2, 50000000, -1) / gwmap_to_timing (gwdata->k, gwdata->b, gwdata->n, gwdata->c);
		if (gwget_num_threads (gwdata) > 1 && NUM_WORKERS < HW_NUM_CORES)
			*output_frequency /= 1.8 * (gwget_num_threads (gwdata) - 1);
		
		exp = floor (_log10 (*output_frequency));
		temp = *output_frequency * pow (10.0, -exp);
		if (temp < 1.25) temp = 1.0;
		else if (temp <1.75) temp = 1.5;
		else if (temp < 2.25) temp = 2.0;
		else if (temp < 2.75) temp = 2.5;
		else temp = floor (temp + 0.5);
		*output_frequency = temp * pow (10.0, exp);
	}

	title_freq = (int) IniGetInt (INI_FILE, "TitleOutputFrequency", 1);
	if (title_freq < 1) title_freq = 1;
	*output_title_frequency = *output_frequency / (double) title_freq;
}

double trunc_percent (
	double	percent)
{
	percent *= 100.0;
	if (percent > 100.0) percent = 100.0;
	percent -= 0.5 * pow (10.0, - (double) PRECISION);
	if (percent < 0.0) return (0.0);
	return (percent);
}

void formatETA (
	double	howlong,		
	char	*buf)
{
	double days, hours, minutes, seconds;
	days = floor (howlong / 86400.0);  howlong -= days * 86400.0;
	hours = floor (howlong / 3600.0);  howlong -= hours * 3600.0;
	minutes = floor (howlong / 60.0);  howlong -= minutes * 60.0;
	seconds = floor (howlong);
	if (days >= 3.0)
		sprintf (buf, ", ETA: %dd %02d:%02d", (int) days, (int) hours, (int) minutes);
	else
		sprintf (buf, ", ETA: %02d:%02d:%02d", (int) (days * 24.0 + hours), (int) minutes, (int) seconds);
}

struct LaunchData {
	int	thread_num;		
	unsigned int num_threads;	
	int	num_to_mark_active;	
	unsigned long p;		
	unsigned long iters;		
	int	bench_type;		
	int	delay_amount;		
	int	stop_reason;		
};

void create_worker_windows (
	int	num_threads)
{
	int	tnum;
	char	buf[80];

	for (tnum = 0; tnum < num_threads; tnum++) {
		create_window (tnum);
		if (HW_NUM_THREADS > 1) sprintf (buf, "Worker #%d", tnum+1);
		else strcpy (buf, "Worker");
		base_title (tnum, buf);
	}
}

int LaunchWorkers (
	int	thread_num,		
	int	wait_flag)		
{
	struct LaunchData *ld;
	gwthread thread_handle;

	if (WORKERS_ACTIVE && (LAUNCH_TYPE == LD_CONTINUE || LAUNCH_TYPE == LD_TORTURE)) {
		if (thread_num == ALL_WORKERS) {
			for (thread_num = 0; thread_num < (int) WORKERS_ACTIVE; thread_num++)
				if (! ACTIVE_WORKERS[thread_num])
					start_one_worker (thread_num);
		} else
			start_one_worker (thread_num);
		return (0);
	}

	ld = (struct LaunchData *) malloc (sizeof (struct LaunchData));
	if (ld == NULL) return (OutOfMemory (MAIN_THREAD_NUM));
	ld->num_threads = NUM_WORKERS;
	LAUNCH_TYPE = LD_CONTINUE;
	create_worker_windows (NUM_WORKERS);
	ld->num_to_mark_active = (thread_num == ALL_WORKERS ? NUM_WORKERS : -thread_num);
	if (wait_flag) {
		gwthread_create_waitable (&thread_handle, &Launcher, ld);
		gwthread_wait_for_exit (&thread_handle);
	} else
		gwthread_create (&thread_handle, &Launcher, ld);
	return (0);
}

int LaunchTortureTest (
	unsigned long num_threads,	
	int	wait_flag)		
{
	struct LaunchData *ld;
	gwthread thread_handle;

	ld = (struct LaunchData *) malloc (sizeof (struct LaunchData));
	if (ld == NULL) return (OutOfMemory (MAIN_THREAD_NUM));
	ld->num_threads = num_threads;
	LAUNCH_TYPE = LD_TORTURE;
	create_worker_windows (num_threads);
	ld->num_to_mark_active = num_threads;
	if (wait_flag) {
		gwthread_create_waitable (&thread_handle, &Launcher, ld);
		gwthread_wait_for_exit (&thread_handle);
	} else
		gwthread_create (&thread_handle, &Launcher, ld);
	return (0);
}

int LaunchBench (
	int	bench_type,		
	int	wait_flag)		
{
	struct LaunchData *ld;
	gwthread thread_handle;

	ld = (struct LaunchData *) malloc (sizeof (struct LaunchData));
	if (ld == NULL) return (OutOfMemory (MAIN_THREAD_NUM));
	ld->num_threads = 1;
	ld->bench_type = bench_type;
	LAUNCH_TYPE = LD_BENCH;
	create_worker_windows (1);
	ld->num_to_mark_active = 1;
	if (wait_flag) {
		gwthread_create_waitable (&thread_handle, &Launcher, ld);
		gwthread_wait_for_exit (&thread_handle);
	} else
		gwthread_create (&thread_handle, &Launcher, ld);
	return (0);
}

int LaunchAdvancedTime (
	unsigned long p,		
	unsigned long iters)		
{
	struct LaunchData *ld;
	gwthread thread_handle;

	ld = (struct LaunchData *) malloc (sizeof (struct LaunchData));
	if (ld == NULL) return (OutOfMemory (MAIN_THREAD_NUM));
	if (p >= 9900 && p <= 9919) ld->num_threads = p - 9900 + 1;
	else if (p >= 9920 && p <= 9939) ld->num_threads = p - 9920 + 1;
	else ld->num_threads = 1;
	ld->p = p;
	ld->iters = iters;
	LAUNCH_TYPE = LD_TIME;
	create_worker_windows (ld->num_threads);
	ld->num_to_mark_active = ld->num_threads;
	gwthread_create (&thread_handle, &Launcher, ld);
	return (0);
}

void Launcher (void *arg)
{
	struct LaunchData *ld;
	unsigned int tnum;
	int	stop_reason;
	gwthread handles[MAX_NUM_WORKERS];
	struct LaunchData *ldwork[MAX_NUM_WORKERS];
	int	delay_amount, total_delay_amount;

	ld = (struct LaunchData *) arg;

	if (WORKERS_ACTIVE) {
		stop_workers_for_escape ();
		while (WORKERS_STOPPING) Sleep (50);
	}

	WORKERS_ACTIVE = ld->num_threads;
	WORKERS_STOPPING = FALSE;
	mark_workers_active (ld->num_to_mark_active);

	if (ld->num_threads > 1)
		OutputStr (MAIN_THREAD_NUM, "Starting workers.\n");
	else
		OutputStr (MAIN_THREAD_NUM, "Starting worker.\n");

	clear_comm_rate_limits ();

again:	clearThreadHandleArray ();

	stop_reason = readIniFiles ();
	if (stop_reason) {
		OutputStr (MAIN_THREAD_NUM, "Error rereading INI files.\n");
		return;
	}
	if (LAUNCH_TYPE == LD_CONTINUE) ld->num_threads = NUM_WORKERS;

	init_stop_code ();

	init_mem_state ();

	PreLaunchCallback (LAUNCH_TYPE);

	ChangeIcon (MAIN_THREAD_NUM, WORKING_ICON);

	if (LAUNCH_TYPE == LD_CONTINUE) {

		start_save_files_timer ();

		start_Jacobi_timer ();

		start_battery_timer ();

		start_priority_work_timer ();

		start_pause_while_running_timer ();

		start_load_average_timer ();

		start_throttle_timer ();
	}

	delay_amount = IniGetInt (INI_FILE, "StaggerStarts", 5);
	total_delay_amount = 0;
	for (tnum = 1; tnum < ld->num_threads; tnum++) {
		ldwork[tnum] = (struct LaunchData *) malloc (sizeof (struct LaunchData));
		if (ldwork[tnum] == NULL) {
			OutOfMemory (MAIN_THREAD_NUM);
			return;
		}
		memcpy (ldwork[tnum], ld, sizeof (struct LaunchData));
		ldwork[tnum]->thread_num = tnum;
		total_delay_amount += delay_amount;
		ldwork[tnum]->delay_amount = total_delay_amount;
		gwthread_create_waitable (&handles[tnum], &LauncherDispatch, ldwork[tnum]);
	}

	ld->thread_num = 0;
	ld->delay_amount = 0;
	LauncherDispatch (ld);
	stop_reason = ld->stop_reason;

	for (tnum = 1; tnum < ld->num_threads; tnum++) {
		gwthread_wait_for_exit (&handles[tnum]);
		if (stop_reason == 0)
			stop_reason = ldwork[tnum]->stop_reason;
		else if (stop_reason == STOP_ESCAPE ||
			 ldwork[tnum]->stop_reason == STOP_ESCAPE)
			stop_reason = STOP_ESCAPE;
		free (ldwork[tnum]);
	}

	if (LAUNCH_TYPE == LD_CONTINUE) {
		writeWorkToDoFile (TRUE);

		stop_save_files_timer ();
		stop_Jacobi_timer ();
		stop_battery_timer ();
		stop_priority_work_timer ();
		stop_pause_while_running_timer ();
		stop_load_average_timer ();
		stop_throttle_timer ();
	}

	ChangeIcon (MAIN_THREAD_NUM, IDLE_ICON);

	PostLaunchCallback (LAUNCH_TYPE);

	if (stop_reason == STOP_RESTART) {
		OutputStr (MAIN_THREAD_NUM, "Restarting all workers using new settings.\n");
		goto again;
	}

	if (stop_reason == STOP_REREAD_INI) {
		OutputStr (MAIN_THREAD_NUM, "Restarting all workers using new timed prime.txt settings.\n");
		goto again;
	}

	if (LAUNCH_TYPE == LD_CONTINUE || LAUNCH_TYPE == LD_TORTURE)
		OutputStr (MAIN_THREAD_NUM, "Execution halted.\n");
	if (LAUNCH_TYPE == LD_CONTINUE)
		OutputStr (MAIN_THREAD_NUM, "Choose Test/Continue to restart.\n");

	WORKERS_ACTIVE = 0;
	WORKERS_STOPPING = FALSE;

	free (ld);
}

void LauncherDispatch (void *arg)
{
	struct LaunchData *ld;
	int	stop_reason;

	ld = (struct LaunchData *) arg;

	if (ld->delay_amount && LAUNCH_TYPE == LD_CONTINUE) {
		char	buf[50];
		int	totaltime;

		title (ld->thread_num, "Waiting to start");
		sprintf (buf, "Waiting %d seconds to stagger worker starts.\n", ld->delay_amount);
		OutputStr (ld->thread_num, buf);

		for (totaltime = 0; totaltime < ld->delay_amount * 1000; totaltime += 100) {
			if (WORKERS_STOPPING) break;
			Sleep (100);
		}
	}

	title (ld->thread_num, "Starting");
	OutputStr (ld->thread_num, "Worker starting\n");
	ChangeIcon (ld->thread_num, WORKING_ICON);

	switch (LAUNCH_TYPE) {
	case LD_CONTINUE:
		stop_reason = primeContinue (ld->thread_num);
		break;
	case LD_TIME:
		stop_reason = primeTime (ld->thread_num, ld->p, ld->iters);
		break;
	case LD_BENCH:
		stop_reason = primeBench (ld->thread_num, ld->bench_type);
		break;
	case LD_TORTURE:
		stop_reason = tortureTest (ld->thread_num, ld->num_threads);
		break;
	}

	title (ld->thread_num, "Not running");
	OutputStr (ld->thread_num, "Worker stopped.\n");
	ChangeIcon (ld->thread_num, IDLE_ICON);

	ld->stop_reason = stop_reason;
}

int primeContinue (
	int	thread_num)
{
	struct PriorityInfo sp_info;
	struct work_unit *w;
	unsigned int pass;
	int	stop_reason;

	memset (&sp_info, 0, sizeof (sp_info));
	sp_info.type = SET_PRIORITY_NORMAL_WORK;
	sp_info.worker_num = thread_num;
	sp_info.verbosity = IniGetInt (INI_FILE, "AffinityVerbosity", 1);
	sp_info.normal_work_hyperthreading = FALSE;
	SetPriority (&sp_info);

	for ( ; ; ) {

	stop_reason = stopCheck (thread_num);
	if (stop_reason) goto check_stop_code;

	clear_memory_restart_flags (thread_num);

	for (pass = (WELL_BEHAVED_WORK || SEQUENTIAL_WORK == 1) ? 2 : 1;
	     pass <= 3;
	     pass++) {

	    for (w = NULL; ; ) {

		sp_info.type = SET_PRIORITY_NORMAL_WORK;
		sp_info.worker_num = thread_num;
		sp_info.verbosity = IniGetInt (INI_FILE, "AffinityVerbosity", 1);
		sp_info.normal_work_hyperthreading = FALSE;
		sp_info.aux_thread_num = 0;

		w = getNextWorkToDoLine (thread_num, w, LONG_TERM_USE);
		if (w == NULL) break;
		if (w->work_type == WORK_NONE) continue;

		set_default_memory_usage (thread_num);

		if (w->work_type == WORK_FACTOR && pass == 2) {
			stop_reason = primeFactor (thread_num, &sp_info, w, 0);
		}

		if (w->work_type == WORK_PFACTOR && pass == 2) {
			stop_reason = pfactor (thread_num, &sp_info, w);
		}

		if (w->work_type == WORK_ADVANCEDTEST ||
		    w->work_type == WORK_TEST ||
		    w->work_type == WORK_DBLCHK) {
			stop_reason = prime (thread_num, &sp_info, w, pass);
		}

		if (w->work_type == WORK_ECM && pass == 2) {
			stop_reason = ecm (thread_num, &sp_info, w);
		}

		if (w->work_type == WORK_PMINUS1 && pass == 2) {
			stop_reason = pminus1 (thread_num, &sp_info, w);
		}

		if (w->work_type == WORK_PPLUS1 && pass == 2) {
			stop_reason = pplus1 (thread_num, &sp_info, w);
		}

		if (w->work_type == WORK_PRP) {
			stop_reason = prp (thread_num, &sp_info, w, pass);
		}

		if (w->work_type == WORK_CERT && pass == 1) {
			stop_reason = cert (thread_num, &sp_info, w, pass);
		}

		set_default_memory_usage (thread_num);

		if (stop_reason == STOP_WORK_UNIT_COMPLETE) {
			rolling_average_work_unit_complete (thread_num, w);
			deleteWorkToDoLine (thread_num, w, FALSE);
			stop_reason = 0;
		}

		if (stop_reason == STOP_PREV_WORK_UNIT) {
			w = w->prev->prev;		
			stop_reason = 0;
		}

		if (stop_reason == STOP_NOT_ENOUGH_MEM) {
			OutputStr (thread_num, "Looking for work that uses less memory.\n");
			stop_reason = 0;
		}

		if (stop_reason == STOP_RETRY_LATER) {
			OutputStr (thread_num, "Aborting processing of this work unit -- will try again later.\n");
			stop_reason = 0;
		}

		if (stop_reason == STOP_ABORT)
			OutputStr (thread_num, "Aborting processing of this work unit.\n");

		if (stop_reason) {
			decrementWorkUnitUseCount (w, LONG_TERM_USE);
			goto check_stop_code;
		}

	    }

	}

check_stop_code:

	if (stop_reason == STOP_ABORT) continue;

	if (stop_reason == STOP_PRIORITY_WORK) continue;

	if (stop_reason == STOP_MEM_CHANGED) continue;

	if (stop_reason == STOP_WORKER) {
		implement_stop_one_worker (thread_num);
		continue;
	}

	if (stop_reason == STOP_PAUSE) {
		implement_pause (thread_num);
		continue;
	}

	if (stop_reason == STOP_BATTERY) {
		if (thread_num == MAIN_THREAD_NUM) PostLaunchCallback (9999);
		implement_stop_battery (thread_num);
		if (thread_num == MAIN_THREAD_NUM) PreLaunchCallback (9999);
		continue;
	}

	if (stop_reason == STOP_AUTOBENCH) {
		implement_stop_autobench (thread_num);
		continue;
	}

	if (stop_reason) return (stop_reason);

#if defined (__linux__) || defined (__FreeBSD__)
	{
		int	exit_wait_time = IniGetInt (INI_FILE, "ExitWhenOutOfWork", 0);
		if (exit_wait_time) {
			Sleep (exit_wait_time * 1000);
			return (0);
		}
	}
#endif

	title (thread_num, "Waiting for work");
	OutputStr (thread_num, "No work to do at the present time.  Waiting.\n");
	ChangeIcon (thread_num, IDLE_ICON);

	set_memory_usage (thread_num, 0, 0);

	spoolMessage (MSG_CHECK_WORK_QUEUE, NULL);

	gwevent_init (&WORK_AVAILABLE_OR_STOP[thread_num]);
	gwevent_reset (&WORK_AVAILABLE_OR_STOP[thread_num]);
	WORK_AVAILABLE_OR_STOP_INITIALIZED[thread_num] = 1;
	gwevent_wait (&WORK_AVAILABLE_OR_STOP[thread_num], 3600);
	WORK_AVAILABLE_OR_STOP_INITIALIZED[thread_num] = 0;
	gwevent_destroy (&WORK_AVAILABLE_OR_STOP[thread_num]);
	OutputStr (thread_num, "Resuming.\n");
	ChangeIcon (thread_num, WORKING_ICON);

	}
}

int testUniqueFileName (
	int	thread_num,
	char	*filename)
{
static	int	USED_FILENAMES_MUTEX_INITIALIZED = FALSE;
static	gwmutex	USED_FILENAMES_MUTEX;
static	char	USED_FILENAMES[MAX_NUM_WORKERS][32];
	int	i;

	if (!USED_FILENAMES_MUTEX_INITIALIZED) {
		USED_FILENAMES_MUTEX_INITIALIZED = 1;
		gwmutex_init (&USED_FILENAMES_MUTEX);
		for (i = 0; i < MAX_NUM_WORKERS; i++) USED_FILENAMES[i][0] = 0;
	}

	gwmutex_lock (&USED_FILENAMES_MUTEX);
	for (i = 0; i < MAX_NUM_WORKERS; i++) {
		if (i != thread_num && strcmp (filename, USED_FILENAMES[i]) == 0) {
			gwmutex_unlock (&USED_FILENAMES_MUTEX);
			return (FALSE);
		}
	}

	strcpy (USED_FILENAMES[thread_num], filename);
	gwmutex_unlock (&USED_FILENAMES_MUTEX);
	return (TRUE);
}

void uniquifySaveFile (
	int	thread_num,
	char	*filename)
{
	char	original_filename[32];
	int	i;

	strcpy (original_filename, filename);

	sprintf (filename, "%s_%d", original_filename, thread_num+1);
	if (fileExists (filename) && testUniqueFileName (thread_num, filename)) return;

	strcpy (filename, original_filename);
	if (fileExists (filename) && testUniqueFileName (thread_num, filename)) return;

	for (i = 0; i < MAX_NUM_WORKERS; i++) {
		sprintf (filename, "%s_%d", original_filename, i+1);
		if (fileExists (filename) && testUniqueFileName (thread_num, filename)) return;
	}

	strcpy (filename, original_filename);
	if (testUniqueFileName (thread_num, filename)) return;

	sprintf (filename, "%s_%d", original_filename, thread_num+1);
	if (testUniqueFileName (thread_num, filename)) return;

	for (i = 0; i < MAX_NUM_WORKERS; i++) {
		sprintf (filename, "%s_%d", original_filename, i+1);
		if (testUniqueFileName (thread_num, filename)) return;
	}
}

void readSaveFileStateInit (
	readSaveFileState *state,
	int	thread_num,
	char	*filename)
{
	state->thread_num = thread_num;
	state->read_attempt = 0;
	state->a_save_file_existed = 0;
	state->a_non_bad_save_file_existed = 0;
	state->num_original_bad_files = -1;
	state->num_save_files_renamed = 0;
	strcpy (state->base_filename, filename);
}

int saveFileExists (
	readSaveFileState *state)
{
	int	maxbad;
	char	buf[256];

	for ( ; ; ) {
		state->read_attempt++;

		if (state->read_attempt == 1) {
			strcpy (state->current_filename, state->base_filename);
			if (fileExists (state->current_filename)) goto winner;
			continue;
		}

		if (state->read_attempt == 2) {
			sprintf (state->current_filename, "%s.bu", state->base_filename);
			if (fileExists (state->current_filename)) goto winner;
			continue;
		}

		if (state->read_attempt <= NUM_BACKUP_FILES + NUM_JACOBI_BACKUP_FILES) {
			sprintf (state->current_filename, "%s.bu%d", state->base_filename, state->read_attempt - 1);
			if (fileExists (state->current_filename)) goto winner;
			continue;
		}

		if (state->read_attempt == NUM_BACKUP_FILES + NUM_JACOBI_BACKUP_FILES + 1) {
			state->read_attempt = 49;	
			sprintf (state->current_filename, "%s.write", state->base_filename);
			if (fileExists (state->current_filename)) goto winner;
			continue;
		}

		if (state->num_original_bad_files >= 0) maxbad = state->num_original_bad_files;
		else maxbad = IniGetInt (INI_FILE, "MaxBadSaveFiles", 10);
		if (state->read_attempt >= 50 && state->read_attempt < 50 + maxbad) {
			sprintf (state->current_filename, "%s.bad%d", state->base_filename, maxbad - (state->read_attempt - 50));
			if (fileExists (state->current_filename)) goto winner;
			continue;
		}

		break;
	}

	return (FALSE);

winner:	if (state->read_attempt != 1) {
		sprintf (buf, ALTSAVE_MSG, state->current_filename);
		OutputBoth (state->thread_num, buf);
	}
	state->a_save_file_existed = 1;
	if (state->read_attempt < 50) state->a_non_bad_save_file_existed = 1;
	return (TRUE);
}

void saveFileBad (
	readSaveFileState *state)
{
	char	buf[256];
	char	filename[256];
	int	i, maxbad;

	sprintf (buf, READFILEERR, state->current_filename);
	OutputBoth (state->thread_num, buf);

	if (state->read_attempt >= 50) return;

	maxbad = IniGetInt (INI_FILE, "MaxBadSaveFiles", 10);
	if (maxbad == 0) return;

	if (state->num_original_bad_files < 0) {
		state->num_original_bad_files = 0;
		for (i = 1; i <= maxbad; i++) {
			sprintf (buf, "%s.bad%d", state->base_filename, i);
			if (! fileExists (buf)) break;
			state->num_original_bad_files++;
		}
	}

	if (state->num_original_bad_files + state->num_save_files_renamed >= maxbad) {
		sprintf (filename, "%s.bad1", state->base_filename);
		_unlink (filename);
		for (i = 2; i <= maxbad; i++) {
			char	oldname[80];
			sprintf (filename, "%s.bad%d", state->base_filename, i-1);
			sprintf (oldname, "%s.bad%d", state->base_filename, i);
			rename (oldname, filename);
		}
		if (state->num_original_bad_files) state->num_original_bad_files--;
		else state->num_save_files_renamed--;
	}

	sprintf (filename, "%s.bad%d", state->base_filename, state->num_original_bad_files + state->num_save_files_renamed + 1);
	sprintf (buf, "Renaming %s to %s\n", state->current_filename, filename);
	OutputBoth (state->thread_num, buf);
	rename (state->current_filename, filename);
	state->num_save_files_renamed++;
}

void writeSaveFileStateInit (
	writeSaveFileState *state,
	char	*filename,
	int	num_special_save_files)
{
	strcpy (state->base_filename, filename);
	state->num_ordinary_save_files = NUM_BACKUP_FILES;
	state->num_special_save_files = num_special_save_files;
	state->special = 0;			
}

int openWriteSaveFile (
	writeSaveFileState *state)
{
	char	output_filename[32];
	int	fd;

	if (state->num_ordinary_save_files == 99)
		strcpy (output_filename, state->base_filename);
	else
		sprintf (output_filename, "%s.write", state->base_filename);

	fd = _open (output_filename, _O_BINARY | _O_WRONLY | _O_TRUNC | _O_CREAT, CREATE_FILE_ACCESS);
	return (fd);
}

void closeWriteSaveFile (
	writeSaveFileState *state,
	int	fd)
{
	char	dest_filename[32], src_filename[32];
	int	rename_count;

	_commit (fd);
	_close (fd);

	if (state->num_ordinary_save_files == 99) return;

	state->special <<= 1;

	rename_count = bittst (&state->special, state->num_ordinary_save_files) ?
			       state->num_ordinary_save_files + state->num_special_save_files : state->num_ordinary_save_files;

	if (rename_count == 1) strcpy (dest_filename, state->base_filename);
	else if (rename_count == 2) sprintf (dest_filename, "%s.bu", state->base_filename);
	else sprintf (dest_filename, "%s.bu%d", state->base_filename, rename_count-1);
	_unlink (dest_filename);

	while (rename_count--) {
		if (rename_count == 0) sprintf (src_filename, "%s.write", state->base_filename);
		else if (rename_count == 1) strcpy (src_filename, state->base_filename);
		else if (rename_count == 2) sprintf (src_filename, "%s.bu", state->base_filename);
		else sprintf (src_filename, "%s.bu%d", state->base_filename, rename_count-1);
		rename (src_filename, dest_filename);
		strcpy (dest_filename, src_filename);
	}
}

void setWriteSaveFileSpecial (
	writeSaveFileState *state)
{
	state->special |= 1;
}

void deleteWriteSaveFile (
	writeSaveFileState *state,
	int	fd)
{
	char	output_filename[32];

	_close (fd);
	if (state->num_ordinary_save_files == 99)
		strcpy (output_filename, state->base_filename);
	else
		sprintf (output_filename, "%s.write", state->base_filename);
	_unlink (output_filename);
}

void unlinkSaveFiles (
	writeSaveFileState *state)
{
	int	i, maxbad;
	char	unlink_filename[80];

	maxbad = IniGetInt (INI_FILE, "MaxBadSaveFiles", 10);
	for (i = 1; i <= maxbad; i++) {
		sprintf (unlink_filename, "%s.bad%d", state->base_filename, i);
		_unlink (unlink_filename);
	}
	if (state->num_ordinary_save_files != 99) {
		for (i = 1; i < state->num_ordinary_save_files + state->num_special_save_files; i++) {
			if (i == 1) sprintf (unlink_filename, "%s.bu", state->base_filename);
			else sprintf (unlink_filename, "%s.bu%d", state->base_filename, i);
			_unlink (unlink_filename);
		}
	}
	if (state->base_filename[0] == 'p') {
		sprintf (unlink_filename, "q%s", state->base_filename+1);
		_unlink (unlink_filename);
		sprintf (unlink_filename, "r%s", state->base_filename+1);
		_unlink (unlink_filename);
	}
	sprintf (unlink_filename, "%s.write", state->base_filename);
	_unlink (unlink_filename);
	_unlink (state->base_filename);
}

#ifndef X86_64

#define NEW_STACK_SIZE	(4096+256)
struct facasm_data {
	uint32_t EXPONENT;		
	uint32_t FACPASS;		
	uint32_t FACHSW;		
	uint32_t FACMSW;		
	uint32_t FACLSW;		
	uint32_t cpu_flags;		
	uint32_t firstcall;		
	uint32_t pad[5];
	uint32_t xmm_data[188];		
};

typedef struct {
	int	num_threads;		
	double	endpt;			
	struct PriorityInfo *sp_info;	
	struct	facasm_data *asm_data;	
} fachandle;

EXTERNC void setupf (struct facasm_data *);	
EXTERNC int factor64 (struct facasm_data *);	

int factorSetup (
	int	thread_num,
	unsigned long p,
	fachandle *facdata)
{
	void	*asm_data_alloc;
	struct facasm_data *asm_data;

	memset (facdata, 0, sizeof (fachandle));

	asm_data_alloc = aligned_malloc (1000000, 4096);
	if (asm_data_alloc == NULL) {
		OutputStr (thread_num, "Error allocating memory for trial factoring.\n");
		return (STOP_OUT_OF_MEM);
	}
	facdata->asm_data = asm_data = (struct facasm_data *) ((char *) asm_data_alloc + NEW_STACK_SIZE);
	memset (asm_data, 0, 65536);

	asm_data->EXPONENT = p;
	asm_data->cpu_flags = CPU_FLAGS;
	asm_data->firstcall = 0;
	if (!IniGetInt (INI_FILE, "FactorUsingSSE2", 1)) asm_data->cpu_flags &= ~CPU_SSE2;

	return (0);
}

int factorPassSetup (
	int	thread_num,
	unsigned long pass,
	fachandle *facdata)		
{
	struct facasm_data *asm_data;

	asm_data = (struct facasm_data *) facdata->asm_data;
	asm_data->FACPASS = pass;
	setupf (asm_data);

	if (asm_data->cpu_flags & CPU_SSE2) {
		unsigned long i, p, bits_in_factor;
		uint32_t *xmm_data;

		if (asm_data->FACHSW)
			bits_in_factor = 64, i = asm_data->FACHSW;
		else if (asm_data->FACMSW)
			bits_in_factor = 32, i = asm_data->FACMSW;
		else return (0);
		while (i) bits_in_factor++, i >>= 1;

		if (bits_in_factor <= 63) return (0);

		xmm_data = asm_data->xmm_data;
		p = asm_data->EXPONENT;
		for (i = 0; p > bits_in_factor + 59; i++) {
			xmm_data[48+i*2] = (p & 1) ? 1 : 0;
			p >>= 1;
		}
		xmm_data[0] =					
		xmm_data[2] = p >= 90 ? 0 : (1 << (p - 60));
		xmm_data[40] = 62 - (120 - bits_in_factor);	
		xmm_data[42] = 62 - (p - bits_in_factor);	
		xmm_data[44] = bits_in_factor - 61;		
		xmm_data[112] = i;				
		*(double *)(&xmm_data[110]) =			
			pow ((double) 2.0, (int) (bits_in_factor + 62));
	}

	return (0);
}

#define FACTOR_CHUNK_SIZE		16			

int factorChunk (
	fachandle *facdata)		
{
	return (factor64 (facdata->asm_data));
}

int factorChunksProcessed (
	fachandle *facdata)		
{
	return (1);
}

void factorFindSmallestNotTFed (
	fachandle *facdata)		
{
	return;
}

void factorDone (
	fachandle *facdata)		
{

	if (facdata->asm_data != NULL) {
		aligned_free ((char *) facdata->asm_data - NEW_STACK_SIZE);
		facdata->asm_data = NULL;
	}
}

int getAnotherFactorIfAny (
	fachandle *facdata)
{
	return (FALSE);
}

#endif

#ifdef X86_64

#define SIEVE_SIZE_IN_BYTES	(12 * 1024)		
#define INITSIEVE_PRIMES1	(7 * 11 * 13 * 17)	
#define INITSIEVE_PRIMES2	(11 * 13 * 17 * 19)	
#define INITSIEVE_PRIMES3	(13 * 17 * 19 * 23)	
#define INITSIEVE_PRIMES4	(17 * 19 * 23)		

#define MAX_TF_FOUND_COUNT	10			

struct facasm_data {
	uint64_t p;			
	uint64_t facdists[65];		
	uint64_t facdist12K;		
	uint64_t savefac1;		
	uint64_t savefac0;		
	void	*sieve;			
	void	*primearray;		
	void	*offsetarray;		
	void	*initsieve;		
	double	TWO_TO_FACSIZE_PLUS_62;	

	uint32_t FACHSW;		
	uint32_t FACMSW;		
	uint32_t FACLSW;		
	uint32_t cpu_flags;		
	uint32_t SSE2_LOOP_COUNTER;	
	uint32_t FMA_INITVAL_TYPE;	
	uint32_t FMA_64_MINUS_LOW_BITS;	
	uint32_t initstart;		
	uint32_t alternate_sieve_count;	

	uint32_t pad[3];		
	uint32_t xmm_data[80];		

	uint32_t other_asm_temps_and_consts[600];
};

struct tf_thread_info {
	int	pool;				
	int	thread_can_sieve;		
	int	last_siever_used;		
	int	last_sieve_area_used;		
	gwthread thread_id;			
};

struct siever_info {
	int	state;				
	int	last_thread_used;		
	uint64_t next_sieve_first_factor[2];	
	void	*offsetarray;			
	uint64_t num_areas_to_sieve;		
};

struct sieve_area_info {
	int	state;				
	uint64_t first_factor[2];		
	void	*sieve;				
};

struct pool_info {
	int	num_sievers_active;		
	int	num_free_sieve_areas;		
	int	num_sieved_sieve_areas;		
	int	last_sieve_area_for_sieve;	
	int	last_sieve_area_for_tf;		
	struct sieve_area_info *sieve_areas;	
};

typedef struct {
	int	num_threads;			
	struct tf_thread_info *tf_threads;	

	int	num_pools;			
	int	num_threads_per_pool;		
	int	num_sieve_areas_per_pool;	
	int	pooling;			
	struct pool_info *pools;		

	int	num_sievers;			
	struct siever_info *sievers;		
	uint64_t total_num_areas_to_sieve;	

	double	endpt;				
	struct PriorityInfo *sp_info;		
	struct	facasm_data *asm_data;		
	int	factoring_pass;			
	int	pass_complete;			
	uint32_t initsieve_primes;		
	uint32_t one_eighth_initsieve_primes;	
	uint32_t *modinvarray;			
	int	num_chunks_TFed;		
	uint64_t total_num_chunks_TFed;		
	uint64_t total_num_chunks_to_TF;	
	uint32_t found_lsw[MAX_TF_FOUND_COUNT];	
	uint32_t found_msw[MAX_TF_FOUND_COUNT];	
	uint32_t found_hsw[MAX_TF_FOUND_COUNT];	
	uint32_t found_count;			
	unsigned int num_active_threads;	
	int	threads_must_exit;		
	gwmutex	thread_lock;			
	gwevent	thread_work_to_do;		
	gwevent	sieved_areas_available;		
	gwevent	all_threads_started;		
	gwevent	all_threads_done;		
						
} fachandle;

#define SIEVER_ASSIGNED			0x8000	
#define SIEVER_ACTIVE			0	
#define SIEVER_INACTIVE			1	
#define SIEVER_UNINITIALIZED		2	

#define SIEVE_AREA_FREE			0	
#define SIEVE_AREA_SIEVING		1	
#define SIEVE_AREA_SIEVED		2	
#define SIEVE_AREA_TFING		3	

EXTERNC int factor64_pass_setup (struct facasm_data *);	
EXTERNC int factor64_small (struct facasm_data *);	
EXTERNC int factor64_sieve (struct facasm_data *);	
EXTERNC int factor64_tf (struct facasm_data *);		

int factorChunkMultithreaded (fachandle *facdata, struct facasm_data *asm_data, int aux_thread_num);
void factorFindSmallestNotTFed (fachandle *facdata);

struct factor_thread_data {
	fachandle *facdata;				
	struct facasm_data *asm_data;			
	int	thread_num;
};

void factor_auxiliary_thread (void *arg)
{
	struct factor_thread_data *info;
	fachandle *facdata;
	struct facasm_data *asm_data, *main_thread_asm_data;

	info = (struct factor_thread_data *) arg;
	facdata = info->facdata;
	asm_data = info->asm_data;
	main_thread_asm_data = facdata->asm_data;

	SetAuxThreadPriority (info->thread_num, 0, facdata->sp_info);

	for ( ; ; ) {
		gwevent_wait (&facdata->thread_work_to_do, 0);
		if (facdata->threads_must_exit) break;

		memcpy (asm_data, main_thread_asm_data, sizeof (struct facasm_data));

		gwmutex_lock (&facdata->thread_lock);
		facdata->num_active_threads++;
		if (facdata->num_active_threads == facdata->num_threads - 1) {
			gwevent_reset (&facdata->thread_work_to_do);
			gwevent_signal (&facdata->all_threads_started);
		}
		gwmutex_unlock (&facdata->thread_lock);

		for ( ; ; ) {
			if (facdata->threads_must_exit) break;
			if (factorChunkMultithreaded (facdata, asm_data, info->thread_num)) break;
		}

		gwevent_wait (&facdata->all_threads_started, 0);

		gwmutex_lock (&facdata->thread_lock);
		facdata->num_active_threads--;
		if (facdata->num_active_threads == 0) gwevent_signal (&facdata->all_threads_done);
		gwmutex_unlock (&facdata->thread_lock);
		if (facdata->threads_must_exit) break;
	}

	SetAuxThreadPriority (info->thread_num, 1, NULL);

	aligned_free (info->asm_data);
	free (arg);
}

int factorSetup (
	int	thread_num,
	unsigned long p,
	fachandle *facdata)
{
	struct facasm_data *asm_data;
	int	i, pct, mem_needed;
	struct PriorityInfo *sp_info;
	int	num_small_primes;		
	int	num_siever_groups;		
	int	num_siever_threads;		
	int	num_siever_threads_per_pool;	
			
	i = facdata->num_threads;
	sp_info = facdata->sp_info;
	memset (facdata, 0, sizeof (fachandle));
	facdata->num_threads = i;
	facdata->sp_info = sp_info;

	facdata->asm_data = asm_data = (struct facasm_data *) aligned_malloc (sizeof (struct facasm_data), 64);
	if (asm_data == NULL) {
memerr:		OutputStr (thread_num, "Error allocating memory for trial factoring.\n");
		return (STOP_OUT_OF_MEM);
	}
	memset (asm_data, 0, sizeof (struct facasm_data));

	asm_data->p = p;
	asm_data->cpu_flags = CPU_FLAGS;
	if (!IniGetInt (INI_FILE, "FactorUsingSSE2", 0)) asm_data->cpu_flags &= ~CPU_SSE2;
	asm_data->alternate_sieve_count = IniGetInt (INI_FILE, "AlternateTFSieveCount", 9);
	if (asm_data->alternate_sieve_count < 1) asm_data->alternate_sieve_count = 1;

	facdata->num_threads_per_pool = IniGetInt (INI_FILE, "ThreadsPerTFPool", CPU_HYPERTHREADS);
	if (facdata->num_threads_per_pool < 1) facdata->num_threads_per_pool = 1;
	if (facdata->num_threads_per_pool > facdata->num_threads) facdata->num_threads_per_pool = facdata->num_threads;
	if (facdata->num_threads % facdata->num_threads_per_pool) facdata->num_threads_per_pool = 1;

	mem_needed = facdata->num_threads * sizeof (struct tf_thread_info);
	facdata->tf_threads = (struct tf_thread_info *) malloc (mem_needed);
	if (facdata->tf_threads == NULL) goto memerr;
	memset (facdata->tf_threads, 0, mem_needed);

	facdata->num_pools = facdata->num_threads / facdata->num_threads_per_pool;
	mem_needed = facdata->num_pools * sizeof (struct pool_info);
	facdata->pools = (struct pool_info *) malloc (mem_needed);
	if (facdata->pools == NULL) goto memerr;
	memset (facdata->pools, 0, mem_needed);

	pct = IniGetInt (INI_FILE, "PercentTFSieverThreads", 50);
	if (pct < 1) pct = 1;
	if (pct > 100) pct = 100;
	facdata->pooling = (pct != 100);
	num_siever_threads_per_pool = (pct * facdata->num_threads_per_pool + 99) / 100;

	for (i = 0; i < facdata->num_threads; i++) {
		int	thread_within_pool;
		facdata->tf_threads[i].pool = i / facdata->num_threads_per_pool;
		thread_within_pool = i % facdata->num_threads_per_pool;
		facdata->tf_threads[i].thread_can_sieve =
			(thread_within_pool == 0 ||
			 thread_within_pool * num_siever_threads_per_pool / facdata->num_threads_per_pool !=
			 (thread_within_pool-1) * num_siever_threads_per_pool / facdata->num_threads_per_pool);
		facdata->tf_threads[i].last_sieve_area_used = thread_within_pool;
	}

	num_siever_threads = facdata->num_pools * num_siever_threads_per_pool;
	if (num_siever_threads == 1) {
		num_siever_groups = 1;
		facdata->num_sievers = 1;
	} else if (num_siever_threads <= 30) {
		num_siever_groups = (num_siever_threads + 5) / 6 * 7;
		facdata->num_sievers = (num_siever_threads + 5) / 6 * 6;
	} else if (num_siever_threads <= 360) {
		num_siever_groups = (num_siever_threads + 59) / 60 * 77;
		facdata->num_sievers = (num_siever_threads + 59) / 60 * 60;
	} else {
		num_siever_groups = (num_siever_threads + 719) / 720 * 1001;
		facdata->num_sievers = (num_siever_threads + 719) / 720 * 720;
	}

	if (facdata->num_sievers < 6 && IniGetInt (INI_FILE, "UseMaxSieverAllocs", 3) == 1)
		num_siever_groups = 7, facdata->num_sievers = 6;
	
	if (facdata->num_sievers < 60 && IniGetInt (INI_FILE, "UseMaxSieverAllocs", 3) == 2)
		num_siever_groups = 77, facdata->num_sievers = 60;
	
	if (facdata->num_sievers < 720 && IniGetInt (INI_FILE, "UseMaxSieverAllocs", 3) == 3)
		num_siever_groups = 1001, facdata->num_sievers = 720;

	if (num_siever_groups % 13 == 0) facdata->initsieve_primes = INITSIEVE_PRIMES4;
	else if (num_siever_groups % 11 == 0) facdata->initsieve_primes = INITSIEVE_PRIMES3;
	else if (num_siever_groups % 7 == 0) facdata->initsieve_primes = INITSIEVE_PRIMES2;
	else facdata->initsieve_primes = INITSIEVE_PRIMES1;

	{
		int	prime;			
		uint32_t *primearray;		
		int	maxprime;		
		int	estimated_num_primes;	
		int	stop_reason;		
		void	*sieve_info;		

		maxprime = IniGetInt (INI_FILE, "MaxTFSievePrime",
				      (CPU_FLAGS & CPU_AVX512F) ? 155000 :	
				      (CPU_FLAGS & CPU_FMA3) ? 145000 :		
				      1000000);					
		if (maxprime <= 5000) maxprime = 5000;				
		if (maxprime >= 100000000) maxprime = 100000000;		
		estimated_num_primes = (int) ((double) maxprime / (log ((double) maxprime) - 1.0) * 1.01);
		asm_data->primearray = malloc (estimated_num_primes * sizeof (uint32_t));
		if (asm_data->primearray == NULL) goto memerr;
		primearray = (uint32_t *) asm_data->primearray;
		sieve_info = NULL;  
		stop_reason = start_sieve_with_limit (thread_num, 7, (int) sqrt ((double) maxprime) + 1, &sieve_info);
		if (stop_reason) return (stop_reason);
		for (num_small_primes = 0; num_small_primes < estimated_num_primes; ) {
			prime = (int) sieve (sieve_info);
			if (prime > maxprime) break;
			if (prime == p) continue;				
			if (num_siever_groups % prime == 0) continue;		
			if (facdata->initsieve_primes % prime == 0) continue;	
			*primearray++ = prime;	
			num_small_primes++;
		}
		*primearray = 0;		
		end_sieve (sieve_info);
	}

	for (i = 0; i < 65; i++) asm_data->facdists[i] = i * 120 * (uint64_t) num_siever_groups * (uint64_t) p; 
	asm_data->facdist12K = (SIEVE_SIZE_IN_BYTES * 8) * 120 * (uint64_t) num_siever_groups * (uint64_t) p; 

	facdata->modinvarray = (uint32_t *) malloc (num_small_primes * sizeof (uint32_t));
	if (facdata->modinvarray == NULL) goto memerr;
	for (i = 0; i < num_small_primes; i++)
		facdata->modinvarray[i] = (uint32_t) modinv (asm_data->facdists[1], ((uint32_t *)asm_data->primearray)[i]);

	mem_needed = facdata->num_sievers * sizeof (struct siever_info);
	facdata->sievers = (struct siever_info *) malloc (mem_needed);
	if (facdata->sievers == NULL) goto memerr;
	memset (facdata->sievers, 0, mem_needed);
	for (i = 0; i < facdata->num_sievers; i++) {
		facdata->sievers[i].offsetarray = malloc (num_small_primes * sizeof (uint32_t));
		if (facdata->sievers[i].offsetarray == NULL) goto memerr;
	}

	if (!facdata->pooling)
		facdata->num_sieve_areas_per_pool = facdata->num_threads_per_pool;
	else
		facdata->num_sieve_areas_per_pool = facdata->num_threads_per_pool * IniGetInt (INI_FILE, "TFSieveAreasPerThread", 4);
	mem_needed = facdata->num_sieve_areas_per_pool * sizeof (struct sieve_area_info);
	for (i = 0; i < facdata->num_pools; i++) {
		int	j;
		facdata->pools[i].sieve_areas = (struct sieve_area_info *) malloc (mem_needed);
		if (facdata->pools[i].sieve_areas == NULL) goto memerr;
		memset (facdata->pools[i].sieve_areas, 0, mem_needed);
		for (j = 0; j < facdata->num_sieve_areas_per_pool; j++) {
			facdata->pools[i].sieve_areas[j].sieve = aligned_malloc (SIEVE_SIZE_IN_BYTES, 128);
			if (facdata->pools[i].sieve_areas[j].sieve == NULL) goto memerr;
			facdata->pools[i].sieve_areas[j].state = SIEVE_AREA_FREE;
		}
	}
			
	asm_data->initsieve = malloc (facdata->initsieve_primes + SIEVE_SIZE_IN_BYTES);
	if (asm_data->initsieve == NULL) goto memerr;
	memset (asm_data->initsieve, 0xFF, facdata->initsieve_primes + SIEVE_SIZE_IN_BYTES);
	for (i = 7; i <= 31; i += 2) {
		unsigned int j;
		if (facdata->initsieve_primes % i) continue;
		for (j = 0; j < (facdata->initsieve_primes + SIEVE_SIZE_IN_BYTES) * 8; j += i) bitclr (asm_data->initsieve, j);
	}

	facdata->one_eighth_initsieve_primes = (uint32_t) modinv (8 * asm_data->facdists[1], facdata->initsieve_primes);

	gwmutex_init (&facdata->thread_lock);
	gwevent_init (&facdata->thread_work_to_do);
	gwevent_init (&facdata->sieved_areas_available);
	gwevent_init (&facdata->all_threads_started);
	gwevent_init (&facdata->all_threads_done);
	gwevent_signal (&facdata->all_threads_done);

	facdata->threads_must_exit = FALSE;
	for (i = 1; i < facdata->num_threads; i++) {
		struct factor_thread_data *info;

		info = (struct factor_thread_data *) malloc (sizeof (struct factor_thread_data));
		if (info == NULL) return (GWERROR_MALLOC);
		info->facdata = facdata;
		info->thread_num = i;
		
		info->asm_data = (struct facasm_data *) aligned_malloc (sizeof (struct facasm_data), 128);
		if (info->asm_data == NULL) {
			free (info);
			return (GWERROR_MALLOC);
		}
		
		gwthread_create_waitable (&facdata->tf_threads[i].thread_id, &factor_auxiliary_thread, info);
	}

	return (0);
}

void factorDone (
	fachandle *facdata)		
{
	struct facasm_data *asm_data = facdata->asm_data;
	int	i, j;

	if (facdata->num_threads > 1 && facdata->thread_lock != NULL) {

		facdata->threads_must_exit = TRUE;
		gwevent_signal (&facdata->thread_work_to_do);
		gwevent_signal (&facdata->sieved_areas_available);

		for (i = 1; i < facdata->num_threads; i++)
			if (facdata->tf_threads[i].thread_id)
				gwthread_wait_for_exit (&facdata->tf_threads[i].thread_id);
	}

	free (facdata->tf_threads);
	facdata->tf_threads = NULL;

	gwmutex_destroy (&facdata->thread_lock);
	gwevent_destroy (&facdata->thread_work_to_do);
	gwevent_destroy (&facdata->sieved_areas_available);
	gwevent_destroy (&facdata->all_threads_started);
	gwevent_destroy (&facdata->all_threads_done);
	facdata->thread_lock = NULL;

	if (asm_data != NULL) {
		free (asm_data->initsieve);
		free (asm_data->primearray);
		aligned_free (asm_data);
		facdata->asm_data = NULL;
	}
	free (facdata->modinvarray);
	facdata->modinvarray = NULL;

	if (facdata->sievers != NULL) {
		for (i = 0; i < facdata->num_sievers; i++)
			free (facdata->sievers[i].offsetarray);
		free (facdata->sievers);
		facdata->sievers = NULL;
	}

	if (facdata->pools != NULL) {
		for (i = 0; i < facdata->num_pools; i++) {
			if (facdata->pools[i].sieve_areas != NULL) {
				for (j = 0; j < facdata->num_sieve_areas_per_pool; j++)
					aligned_free (facdata->pools[i].sieve_areas[j].sieve);
				free (facdata->pools[i].sieve_areas);
			}
		}
		free (facdata->pools);
		facdata->pools = NULL;
	}
}

void add96_64 (uint64_t *a_hi, uint64_t *a_lo, uint64_t b) { *a_lo += b; *a_hi += (*a_lo < b) ? 1 : 0; }
uint32_t mod96_32 (uint64_t a_hi, uint64_t a_lo, uint32_t b) { return ((uint32_t) (((((((a_hi % b) << 32) + (a_lo >> 32)) % b) << 32) + (a_lo & 0xFFFFFFFF)) % b)); }

int factorPassSetup (
	int	thread_num,
	unsigned long pass,
	fachandle *facdata)		
{
	struct facasm_data *asm_data = facdata->asm_data;
	uint32_t fachsw, facmsw;
	unsigned int i, j, bits_in_factor;

	facdata->factoring_pass = pass;

	fachsw = asm_data->FACHSW;
	facmsw = asm_data->FACMSW;
	if (fachsw == 0 && facmsw < 0x1000) facmsw = 0x1000;

	if (fachsw) i = fachsw, bits_in_factor = 64;
	else i = facmsw, bits_in_factor = 32;
	while (i) bits_in_factor++, i >>= 1;
	if (bits_in_factor < 50) bits_in_factor = 50;		

	if (bits_in_factor >= 45 && (asm_data->cpu_flags & CPU_AVX512F)) {
		uint32_t *dword_data;
		double	*zmm_data;
		uint64_t *zmm64_data;
		int	split_bits, low_bits;
		unsigned long p;

		split_bits = bits_in_factor + (bits_in_factor + 1) / 2;
		low_bits = (split_bits - 52 + 1) / 2;

		dword_data = (uint32_t *) asm_data->xmm_data;
		p = (unsigned long) asm_data->p;
		for (i = 0; p > 2 * bits_in_factor + 1; i++) {
			dword_data[i] = (p & 1) ? 64 : 0;		
			p >>= 1;
		}

		asm_data->FMA_INITVAL_TYPE =
			((int) p < split_bits) ? -1 :
			((int) p > split_bits+1) ? 0 :
			((int) p == split_bits) ? 1 : 2;
		asm_data->FMA_64_MINUS_LOW_BITS = 64 - low_bits;
		asm_data->SSE2_LOOP_COUNTER = i;

		zmm64_data = (uint64_t *) &dword_data[32];
		zmm64_data[0] = (1ULL << low_bits) - 1; 
		zmm_data = (double *) &zmm64_data[1];
		zmm_data[0] = 3.0 * pow ((double) 2.0, (51 + split_bits)); 
		zmm_data[1] = 3.0 * pow ((double) 2.0, (51 + 2*low_bits)); 
		zmm_data[2] = 3.0 * pow ((double) 2.0, (51 + low_bits)); 
		zmm_data[3] = 3.0 * pow ((double) 2.0, (51 - split_bits)); 
		zmm_data[4] = 3.0 * pow ((double) 2.0, (51 + low_bits - split_bits)); 
		if ((int) p > split_bits+1)
			zmm_data[5] = pow ((double) 2.0, (double) (p-1)); 
		else
			zmm_data[5] = pow ((double) 2.0, (double) p); 
		zmm_data[6] = pow ((double) 2.0, split_bits); 
		zmm_data[7] = pow ((double) 2.0, low_bits); 
	}

	else if (bits_in_factor >= 45 && (asm_data->cpu_flags & CPU_FMA3)) {
		uint32_t *dword_data;
		double	*ymm_data;
		uint64_t *ymm64_data;
		int	split_bits, low_bits;
		unsigned long p;

		split_bits = bits_in_factor + (bits_in_factor + 1) / 2;
		low_bits = (split_bits - 52 + 1) / 2;

		dword_data = (uint32_t *) asm_data->xmm_data;
		p = (unsigned long) asm_data->p;
		for (i = 0; p > 2 * bits_in_factor + 1; i++) {
			dword_data[i] = (p & 1) ? 32 : 0;		
			p >>= 1;
		}

		asm_data->FMA_INITVAL_TYPE =
			((int) p < split_bits) ? -1 :
			((int) p > split_bits+1) ? 0 :
			((int) p == split_bits) ? 1 : 2;
		asm_data->FMA_64_MINUS_LOW_BITS = 64 - low_bits;
		asm_data->SSE2_LOOP_COUNTER = i;

		ymm64_data = (uint64_t *) &dword_data[32];
		ymm64_data[0] = ymm64_data[1] = ymm64_data[2] = ymm64_data[3] = (1ULL << low_bits) - 1; 
		ymm_data = (double *) &ymm64_data[4];
		ymm_data[0] = ymm_data[1] = ymm_data[2] = ymm_data[3] = 3.0 * pow ((double) 2.0, (51 + split_bits)); 
		ymm_data[4] = ymm_data[5] = ymm_data[6] = ymm_data[7] = 3.0 * pow ((double) 2.0, (51 + 2*low_bits)); 
		ymm_data[8] = ymm_data[9] = ymm_data[10] = ymm_data[11] = 3.0 * pow ((double) 2.0, (51 + low_bits)); 
		ymm_data[12] = ymm_data[13] = ymm_data[14] = ymm_data[15] = 3.0 * pow ((double) 2.0, (51 - split_bits)); 
		ymm_data[16] = ymm_data[17] = ymm_data[18] = ymm_data[19] = 3.0 * pow ((double) 2.0, (51 + low_bits - split_bits)); 
		if ((int) p > split_bits+1)
			ymm_data[20] = ymm_data[21] = ymm_data[22] = ymm_data[23] = pow ((double) 2.0, (double) (p-1)); 
		else
			ymm_data[20] = ymm_data[21] = ymm_data[22] = ymm_data[23] = pow ((double) 2.0, (double) p); 
		ymm_data[24] = ymm_data[25] = ymm_data[26] = ymm_data[27] = pow ((double) 2.0, split_bits); 
		ymm_data[28] = ymm_data[29] = ymm_data[30] = ymm_data[31] = pow ((double) 2.0, low_bits); 
	}

	else if (bits_in_factor >= 64 && (asm_data->cpu_flags & (CPU_SSE2 | CPU_AVX2))) {
		uint32_t *xmm_data;
		unsigned long p;

		xmm_data = asm_data->xmm_data;
		p = (unsigned long) asm_data->p;
		for (i = 0; p > bits_in_factor + 59; i++) {
			xmm_data[16+i*2] = (p & 1) ? 1 : 0;
			p >>= 1;
		}
		xmm_data[0] = xmm_data[2] =			
		xmm_data[4] = xmm_data[6] = p >= 90 ? 0 : (1 << (p - 60));
		xmm_data[8] = 62 - (120 - bits_in_factor);	
		xmm_data[10] = 62 - (p - bits_in_factor);	
		xmm_data[12] = bits_in_factor - 61;		
		asm_data->SSE2_LOOP_COUNTER = i;
		asm_data->TWO_TO_FACSIZE_PLUS_62 = pow ((double) 2.0, (int) (bits_in_factor + 62));
	}

	facdata->total_num_areas_to_sieve = 0;
	for (i = 0; (int) i < facdata->num_sievers; i++) {
		uint64_t savefac0, savefac1;
		double	first_factor;

		if (i == 0) {
			uint64_t rem;
			uint32_t rems[16] = {1,7,17,23,31,41,47,49,71,73,79,89,97,103,113,119};

			savefac0 = fachsw;
			savefac1 = ((uint64_t) facmsw) << 32;

			rem = mod96_32 (savefac0, savefac1, (uint32_t) asm_data->p * 2);
			add96_64 (&savefac0, &savefac1, asm_data->p * 2 - rem + 1);

			for ( ; ; ) {
				if (mod96_32 (savefac0, savefac1, 120) == rems[pass]) break;
				add96_64 (&savefac0, &savefac1, asm_data->p * 2);
			}
		}

		else {
			savefac0 = facdata->sievers[i-1].next_sieve_first_factor[0];
			savefac1 = facdata->sievers[i-1].next_sieve_first_factor[1];
			add96_64 (&savefac0, &savefac1, 120 * asm_data->p);
		}

		for ( ; ; ) {
			if ((asm_data->facdists[1] % 7 == 0 && mod96_32 (savefac0, savefac1, 7) == 0) ||
			    (asm_data->facdists[1] % 11 == 0 && mod96_32 (savefac0, savefac1, 11) == 0) ||
			    (asm_data->facdists[1] % 13 == 0 && mod96_32 (savefac0, savefac1, 13) == 0))
				add96_64 (&savefac0, &savefac1, 120 * asm_data->p);
			else
				break;
		}

		facdata->sievers[i].next_sieve_first_factor[0] = savefac0;
		facdata->sievers[i].next_sieve_first_factor[1] = savefac1;
		facdata->sievers[i].state = SIEVER_UNINITIALIZED;

		first_factor = (double) savefac0 * pow (2.0, 64.0) + (double) savefac1;
		if (facdata->endpt > first_factor)
			facdata->sievers[i].num_areas_to_sieve = (uint64_t) ((facdata->endpt - first_factor) / (double) asm_data->facdist12K) + 1;
		else
			facdata->sievers[i].num_areas_to_sieve = 0;
		facdata->total_num_areas_to_sieve += facdata->sievers[i].num_areas_to_sieve;
	}

	asm_data->savefac0 = facdata->sievers[0].next_sieve_first_factor[0];
	asm_data->savefac1 = facdata->sievers[0].next_sieve_first_factor[1];
	factor64_pass_setup (asm_data);

	j = 0;
	for (i = 0; (int) i < facdata->num_threads; i++) {
		if (facdata->tf_threads[i].thread_can_sieve) {
			facdata->sievers[j].last_thread_used = i;
			facdata->sievers[j].state |= SIEVER_ASSIGNED;
			facdata->tf_threads[i].last_siever_used = j++;
		}
	}

	for (i = 0; (int) i < facdata->num_pools; i++) {
		facdata->pools[i].num_sievers_active = 0;
		facdata->pools[i].num_free_sieve_areas = facdata->num_sieve_areas_per_pool;
		facdata->pools[i].num_sieved_sieve_areas = 0;
		facdata->pools[i].last_sieve_area_for_sieve = 0;
		facdata->pools[i].last_sieve_area_for_tf = 0;
	}
	facdata->num_chunks_TFed = 0;
	facdata->total_num_chunks_TFed = 0;
	facdata->total_num_chunks_to_TF = facdata->total_num_areas_to_sieve;
	facdata->pass_complete = FALSE;

	if (facdata->num_threads > 1) {
		facdata->num_active_threads = 0;			
		gwevent_reset (&facdata->all_threads_started);		
		gwevent_reset (&facdata->all_threads_done);		
		gwevent_signal (&facdata->thread_work_to_do);		
	}

	return (0);
}

void factorPassSetupPart2 (
	fachandle *facdata,		
	struct facasm_data *asm_data,	
	struct siever_info *siever)	
{
	uint32_t *primep, *modinvp, *offsetp;

	modinvp = facdata->modinvarray;
	offsetp = (uint32_t *) siever->offsetarray;
	for (primep = (uint32_t *) asm_data->primearray; *primep; primep++, offsetp++, modinvp++) {
		uint32_t temp;
		temp = mod96_32 (asm_data->savefac0, asm_data->savefac1, *primep);
		if (temp) temp = (uint32_t) (*primep - (((uint64_t) temp * (uint64_t) *modinvp) % *primep));
		*offsetp = temp;
	}
}

#define FACTOR_CHUNK_SIZE	(SIEVE_SIZE_IN_BYTES >> 10)		

int factorChunk (			
	fachandle *facdata)		
{
	struct facasm_data *asm_data = facdata->asm_data;
	int	res;

	if (facdata->factoring_pass == 0 && asm_data->FACHSW == 0 && asm_data->FACMSW < 0x1000) {
		asm_data->savefac1 = 2 * (uint64_t) asm_data->p + 1;		
		while (factor64_small (asm_data) == 1) {			
			if (facdata->found_count < MAX_TF_FOUND_COUNT) {	
				facdata->found_lsw[facdata->found_count] = (uint32_t) asm_data->savefac1;
				facdata->found_msw[facdata->found_count] = (uint32_t) (asm_data->savefac1 >> 32);
				facdata->found_hsw[facdata->found_count] = 0;
				facdata->found_count++;
			}
			asm_data->savefac1 += 2 * (uint64_t) asm_data->p;	
		}
	}

	res = factorChunkMultithreaded (facdata, asm_data, 0);
	
	if (res) {
		gwevent_wait (&facdata->all_threads_done, 0);
		facdata->pass_complete = TRUE;
	}
	
	if (facdata->found_count) {
		
		do {
			res = factorChunkMultithreaded (facdata, asm_data, 0);
		} while (!res);
		gwevent_wait (&facdata->all_threads_done, 0);
		facdata->pass_complete = TRUE;
		facdata->found_count--;
		asm_data->FACLSW = facdata->found_lsw[facdata->found_count];
		asm_data->FACMSW = facdata->found_msw[facdata->found_count];
		asm_data->FACHSW = facdata->found_hsw[facdata->found_count];
		return (1);					
	}
	return (2);						
}

int factorChunkMultithreaded (		
	fachandle *facdata,		
	struct facasm_data *asm_data,	
	int	aux_thread_num)		
{
	int	i, res, siever_uninitialized;
	struct tf_thread_info *tf_thread;
	struct pool_info *pool;
	struct sieve_area_info *sieve_area;

	tf_thread = &facdata->tf_threads[aux_thread_num];
	pool = &facdata->pools[tf_thread->pool];

	gwmutex_lock (&facdata->thread_lock);

	if (facdata->total_num_areas_to_sieve > 0 &&				
	    (!facdata->pooling ||						
	     (tf_thread->thread_can_sieve &&					
	      pool->num_free_sieve_areas &&					
	      pool->num_sieved_sieve_areas < facdata->num_threads_per_pool*2))){
		struct siever_info *siever;
		siever = &facdata->sievers[tf_thread->last_siever_used];

		if (siever->last_thread_used == aux_thread_num &&		
		    (siever->state == SIEVER_ASSIGNED + SIEVER_UNINITIALIZED ||	
		     ((siever->num_areas_to_sieve & 0x3F) &&			
		      (siever->num_areas_to_sieve >= 20 ||			
		       facdata->num_threads == 1))))				
			;							

		else {
			int	i, best_siever;
			uint64_t score, best_score_thusfar;

			if (siever->last_thread_used == aux_thread_num) siever->state = SIEVER_INACTIVE;

			if (facdata->num_threads == 1) {
				best_siever = tf_thread->last_siever_used + 1;
				if (best_siever == facdata->num_sievers) best_siever = 0;
			}

			else {
				best_siever = -1;
				if (siever->last_thread_used == aux_thread_num) {
					best_siever = tf_thread->last_siever_used;
					best_score_thusfar = siever->num_areas_to_sieve;
					if (siever->num_areas_to_sieve > 128) best_score_thusfar += 128;
					else if (siever->num_areas_to_sieve > 64) best_score_thusfar += 64;
				}
				for (i = 0; i < facdata->num_sievers; i++) {
					
					if (facdata->sievers[i].state == SIEVER_ASSIGNED + SIEVER_ACTIVE) continue;
					
					if ((facdata->sievers[i].state & SIEVER_ASSIGNED) &&
					    facdata->sievers[i].num_areas_to_sieve > 20) continue;
					
					score = facdata->sievers[i].num_areas_to_sieve;
					if (best_siever == -1 || score > best_score_thusfar) {
						best_siever = i;
						best_score_thusfar = score;
					}
				}
			}

			tf_thread->last_siever_used = best_siever;
			siever = &facdata->sievers[best_siever];
			siever->state |= SIEVER_ASSIGNED;
			siever->last_thread_used = aux_thread_num;
		}

		if (siever->num_areas_to_sieve == 0) {
			if (facdata->pooling && pool->num_sieved_sieve_areas) goto tf;
			gwevent_reset (&facdata->sieved_areas_available);
			gwmutex_unlock (&facdata->thread_lock);
			gwevent_wait (&facdata->sieved_areas_available, 0);
			return (FALSE);
		}

		pool->num_sievers_active++;
		siever_uninitialized = (siever->state == SIEVER_ASSIGNED + SIEVER_UNINITIALIZED);
		siever->state = SIEVER_ASSIGNED + SIEVER_ACTIVE;
		siever->num_areas_to_sieve--;
		facdata->total_num_areas_to_sieve--;
		
		if (pool->sieve_areas[tf_thread->last_sieve_area_used].state == SIEVE_AREA_FREE)
			i = tf_thread->last_sieve_area_used;
		
		else for (i = pool->last_sieve_area_for_sieve + 1; ; i++) {
			if (i == facdata->num_sieve_areas_per_pool) i = 0;
			if (pool->sieve_areas[i].state == SIEVE_AREA_FREE) break;
		}
		tf_thread->last_sieve_area_used = i;
		pool->last_sieve_area_for_sieve = i;
		sieve_area = &pool->sieve_areas[i];
		pool->num_free_sieve_areas--;
		sieve_area->state = SIEVE_AREA_SIEVING;		
		gwmutex_unlock (&facdata->thread_lock);
		sieve_area->first_factor[0] = asm_data->savefac0 = siever->next_sieve_first_factor[0];
		sieve_area->first_factor[1] = asm_data->savefac1 = siever->next_sieve_first_factor[1];
		asm_data->initstart = (uint32_t)
			((uint64_t) mod96_32 (asm_data->savefac0, asm_data->savefac1, facdata->initsieve_primes) *
			 (uint64_t) facdata->one_eighth_initsieve_primes % facdata->initsieve_primes);
		asm_data->sieve = sieve_area->sieve;
		asm_data->offsetarray = siever->offsetarray;
		if (siever_uninitialized) factorPassSetupPart2 (facdata, asm_data, siever);
		factor64_sieve (asm_data);
		gwmutex_lock (&facdata->thread_lock);
		siever->next_sieve_first_factor[0] = asm_data->savefac0; 
		siever->next_sieve_first_factor[1] = asm_data->savefac1;
		sieve_area->state = SIEVE_AREA_SIEVED;
		pool->num_sieved_sieve_areas++;
		gwevent_signal (&facdata->sieved_areas_available);
		siever->state = SIEVER_ASSIGNED + SIEVER_INACTIVE;
		pool->num_sievers_active--;

		if (facdata->pooling) {
			gwmutex_unlock (&facdata->thread_lock);
			return (FALSE);
		}
	}

	while (pool->num_sieved_sieve_areas == 0) {
		if ((!facdata->pooling || pool->num_sievers_active == 0) && facdata->total_num_areas_to_sieve == 0) {
			gwmutex_unlock (&facdata->thread_lock);
			return (TRUE);			
		}
		gwevent_reset (&facdata->sieved_areas_available);
		gwmutex_unlock (&facdata->thread_lock);
		gwevent_wait (&facdata->sieved_areas_available, 0);
		if (facdata->threads_must_exit) return (TRUE);
		gwmutex_lock (&facdata->thread_lock);
	}

tf:	if (!facdata->pooling)
		i = tf_thread->last_sieve_area_used;
	
	else for (i = pool->last_sieve_area_for_tf+1; ; i++) {
		if (i == facdata->num_sieve_areas_per_pool) i = 0;
		if (pool->sieve_areas[i].state == SIEVE_AREA_SIEVED) break;
	}
	tf_thread->last_sieve_area_used = i;
	pool->last_sieve_area_for_tf = i;
	sieve_area = &pool->sieve_areas[i];
	pool->num_sieved_sieve_areas--;
	sieve_area->state = SIEVE_AREA_TFING;			
	asm_data->savefac0 = sieve_area->first_factor[0];
	asm_data->savefac1 = sieve_area->first_factor[1];
	asm_data->sieve = sieve_area->sieve;
	gwmutex_unlock (&facdata->thread_lock);
	res = factor64_tf (asm_data);
	gwmutex_lock (&facdata->thread_lock);
	if (res == 1) {							
		
		if (facdata->found_count == 0 && !IniGetInt (INI_FILE, "TFFullBitLevel", 0)) {
			double factor = ((double) asm_data->FACHSW * 4294967296.0 + (double) asm_data->FACMSW) * 4294967296.0;
			uint64_t reduction = (uint64_t) ((facdata->endpt - factor) / (double) asm_data->facdist12K);
			for (i = 0; i < facdata->num_sievers; i++) {
				if (facdata->sievers[i].num_areas_to_sieve >= reduction) {
					facdata->total_num_chunks_to_TF -= reduction;
					facdata->total_num_areas_to_sieve -= reduction;
					facdata->sievers[i].num_areas_to_sieve -= reduction;
				} else {
					facdata->total_num_chunks_to_TF -= facdata->sievers[i].num_areas_to_sieve;
					facdata->total_num_areas_to_sieve -= facdata->sievers[i].num_areas_to_sieve;
					facdata->sievers[i].num_areas_to_sieve = 0;
				}
			}
			gwevent_signal (&facdata->sieved_areas_available); 
		}
		
		if (facdata->found_count < MAX_TF_FOUND_COUNT) {	
			facdata->found_lsw[facdata->found_count] = asm_data->FACLSW;
			facdata->found_msw[facdata->found_count] = asm_data->FACMSW;
			facdata->found_hsw[facdata->found_count] = asm_data->FACHSW;
			facdata->found_count++;
		}
	}
	facdata->num_chunks_TFed++;
	facdata->total_num_chunks_TFed++;
	sieve_area->state = SIEVE_AREA_FREE;			
	pool->num_free_sieve_areas++;
	gwmutex_unlock (&facdata->thread_lock);
	return (FALSE);							
}

int factorChunksProcessed (
	fachandle *facdata)		
{
	int	res;
	res = facdata->num_chunks_TFed;
	facdata->num_chunks_TFed = 0;		
	return (res);
}

void factorFindSmallestNotTFed (
	fachandle *facdata)		
{
	int	i, j;
	uint64_t smallest, temp;
	struct facasm_data *asm_data = facdata->asm_data;

	gwmutex_lock (&facdata->thread_lock);
	for (i = 0; i < facdata->num_sievers; i++) {
		temp = (facdata->sievers[i].next_sieve_first_factor[0] << 32) +
		       (facdata->sievers[i].next_sieve_first_factor[1] >> 32);
		if (i == 0 || temp < smallest) smallest = temp;
	}
	for (i = 0; i < facdata->num_pools; i++) {
		for (j = 0; j < facdata->num_sieve_areas_per_pool; j++) {
			if (facdata->pools[i].sieve_areas[j].state != SIEVE_AREA_SIEVED) continue;
			temp = (facdata->pools[i].sieve_areas[j].first_factor[0] << 32) +
			       (facdata->pools[i].sieve_areas[j].first_factor[1] >> 32);
			if (temp < smallest) smallest = temp;
		}
	}

	asm_data->FACHSW = (uint32_t) (smallest >> 32);
	asm_data->FACMSW = (uint32_t) smallest;
	gwmutex_unlock (&facdata->thread_lock);
}

int getAnotherFactorIfAny (
	fachandle *facdata)
{
	if (facdata->num_threads <= 1 || facdata->found_count == 0) return (FALSE);
	facdata->found_count--;
	facdata->asm_data->FACLSW = facdata->found_lsw[facdata->found_count];
	facdata->asm_data->FACMSW = facdata->found_msw[facdata->found_count];
	facdata->asm_data->FACHSW = facdata->found_hsw[facdata->found_count];
	return (TRUE);
}

#endif

static const char FACMSG[] = "Trial factoring M%ld to 2^%d is %.*f%% complete.";
static const char SHORT_FACMSG[] = "Trial factoring M%ld to 2^%d.";

#define FACTOR_MAGICNUM		0x1567234D
#define FACTOR_VERSION		1

int primeFactor (
	int	thread_num,
	struct PriorityInfo *sp_info,	
	struct work_unit *w,
	unsigned int factor_limit_adjustment)
{
	fachandle facdata;		
	unsigned long p;		
	unsigned long bits;		
	unsigned long test_bits;	
	long	factor_found;		
	int	fd;			
	int	first_iter_msg, continuation, stop_reason, find_smaller_factor;
	unsigned long endpthi, endptlo;
	double	endpt, startpt;		
	unsigned long pass;		
	unsigned long report_bits;	
					
	readSaveFileState read_save_file_state; 
	writeSaveFileState write_save_file_state; 
	char	filename[32];
	char	buf[200], JSONbuf[4000], str[80];
	double	timers[2];

begin:	factor_found = 0;
	p = w->n;
	bits = (unsigned int) w->sieve_depth;

	test_bits = (unsigned int) w->factor_to - factor_limit_adjustment;

	if (bits >= test_bits) {
		if (w->work_type == WORK_FACTOR) {
			struct primenetAssignmentResult pkt;
			memset (&pkt, 0, sizeof (pkt));
			strcpy (pkt.computer_guid, COMPUTER_GUID);
			strcpy (pkt.assignment_uid, w->assignment_uid);
			pkt.result_type = PRIMENET_AR_TF_NOFACTOR;
			pkt.n = p;
			pkt.done = TRUE;
			spoolMessage (PRIMENET_ASSIGNMENT_RESULT, &pkt);
			return (STOP_WORK_UNIT_COMPLETE);
		}
		return (0);
	}

	if (HYPERTHREAD_TF) sp_info->normal_work_hyperthreading = TRUE;
	facdata.num_threads = get_worker_num_threads (thread_num, HYPERTHREAD_TF);
	facdata.sp_info = sp_info;
	stop_reason = factorSetup (thread_num, p, &facdata);
	if (stop_reason) {
		factorDone (&facdata);
		return (stop_reason);
	}

	set_memory_usage (thread_num, 0, 1);

	if (test_bits > 78 && !(facdata.asm_data->cpu_flags & (CPU_SSE2 | CPU_AVX2 | CPU_FMA3 | CPU_AVX512F))) {
		OutputBoth (thread_num, "Pre-SSE2 trial factoring code cannot go above 2^78.\n");
		test_bits = 78;
	}

	continuation = FALSE;
	tempFileName (w, filename);
	filename[0] = 'f';
	readSaveFileStateInit (&read_save_file_state, thread_num, filename);
	writeSaveFileStateInit (&write_save_file_state, filename, 0);
	for ( ; ; ) {

		if (! saveFileExists (&read_save_file_state)) {
			
			if (read_save_file_state.a_non_bad_save_file_existed) {
				OutputBoth (thread_num, ALLSAVEBAD_MSG);
				return (0);
			}
			
			break;
		}

		fd = _open (read_save_file_state.current_filename, _O_BINARY | _O_RDONLY);
		if (fd > 0) {
			unsigned long fachsw, facmsw;
			uint32_t version, sum;
			if (read_magicnum (fd, FACTOR_MAGICNUM) &&
			    read_header (fd, &version, w, &sum) &&
			    version == FACTOR_VERSION &&
			    read_long (fd, (unsigned long *) &factor_found, NULL) &&
			    read_long (fd, &bits, NULL) &&
			    read_long (fd, &pass, NULL) &&
			    read_long (fd, &fachsw, NULL) &&
			    read_long (fd, &facmsw, NULL) &&
			    read_long (fd, &endpthi, NULL) &&
			    read_long (fd, &endptlo, NULL) &&
			    (fachsw < endpthi || (fachsw == endpthi && facmsw < endptlo))) {
				facdata.asm_data->FACHSW = fachsw;
				facdata.asm_data->FACMSW = facmsw;
				continuation = TRUE;
			}
			_close (fd);
			if (continuation) break;
		}

		saveFileBad (&read_save_file_state);
	}

	sprintf (buf, "Factoring M%ld", p);
	title (thread_num, buf);
	sprintf (buf, "%s trial factoring of M%ld to 2^%lu\n", continuation ? "Resuming" : "Starting", p, test_bits);
	OutputStr (thread_num, buf);

	clear_timers (timers, sizeof (timers) / sizeof (timers[0]));

	report_bits = (unsigned long) (64.0 + log2 ((double) p / 100000000.0));
	if (report_bits >= test_bits) report_bits = test_bits;

	while (test_bits > bits || continuation) {
	    unsigned int end_bits;
	    unsigned long iters, iters_r, iters_just_processed;

	    end_bits = (bits < 60) ? 60 : bits + 1;
	    if (end_bits > test_bits) end_bits = test_bits;
	    sprintf (w->stage, "TF%d", end_bits);

	    if (!continuation) {
		if (end_bits < 64) {
			endpthi = 0;
			endptlo = 1L << (end_bits-32);
		} else {
			endpthi = 1L << (end_bits-64);
			endptlo = 0;
		}
	    }

	    if (bits < 32) startpt = 0.0;
	    else startpt = pow ((double) 2.0, (int) (bits-32));
	    endpt = endpthi * 4294967296.0 + endptlo;

	    iters_r = 0;
	    iters = 0;
	    first_iter_msg = (continuation ? 1 : 2);
	    if (! continuation) pass = 0;
	    for ( ; pass < 16; pass++) {

		if (continuation)
			continuation = FALSE;
		else {
			if (bits < 50) {
				facdata.asm_data->FACHSW = 0;
				facdata.asm_data->FACMSW = 0;
			} else if (bits < 64) {
				facdata.asm_data->FACHSW = 0;
				facdata.asm_data->FACMSW = 1L << (bits-32);
			} else {
				facdata.asm_data->FACHSW = 1L << (bits-64);
				facdata.asm_data->FACMSW = 0;
			}
		}

		if (facdata.asm_data->FACHSW == 0 &&
		    facdata.asm_data->FACMSW == 0 && pass != 0)
			facdata.asm_data->FACMSW = 1;

		facdata.endpt = endpt * 4294967296.0;
		stop_reason = factorPassSetup (thread_num, pass, &facdata);
		if (stop_reason) {
			factorDone (&facdata);
			return (stop_reason);
		}

		for ( ; ; ) {
			int	res;
			double	currentpt;

			start_timer (timers, 0);
			res = factorChunk (&facdata);
			end_timer (timers, 0);

			if (res == 1) {
				stackgiant (f, 10);
				stackgiant (x, 10);
				itog ((int) facdata.asm_data->FACHSW, f); gshiftleft (32, f);
				uladdg (facdata.asm_data->FACMSW, f); gshiftleft (32, f);
				uladdg (facdata.asm_data->FACLSW, f);
				itog (2, x);
				powermod (x, p, f);

				if (isone (x)) break;

				OutputBoth (thread_num, "ERROR: Incorrect factor found.\n");
				factorDone (&facdata);
				stop_reason = SleepFive (thread_num);
				if (stop_reason) return (stop_reason);
				goto begin;
			}

#ifdef X86_64
			
			currentpt = (double) facdata.total_num_chunks_to_TF - (double) facdata.total_num_chunks_TFed;
			
			currentpt /= (double) facdata.num_sievers;
			
			currentpt *= (double) facdata.asm_data->facdist12K;
			
			currentpt = endpt - currentpt / 4294967296.0;
#else
			currentpt = facdata.asm_data->FACHSW * 4294967296.0 + facdata.asm_data->FACMSW;
			if (currentpt > endpt) currentpt = endpt;
#endif
			w->pct_complete = (pass + (currentpt - startpt) / (endpt - startpt)) / 16.0;

			stop_reason = stopCheck (thread_num);
			iters_just_processed = factorChunksProcessed (&facdata);
			iters += iters_just_processed;
			if (((iters * FACTOR_CHUNK_SIZE) >> 7) >= ITER_OUTPUT || first_iter_msg) {
				double	pct;
				pct = trunc_percent (w->pct_complete);
				if (first_iter_msg == 2) {
					sprintf (buf, "M%ld to 2^%d", p, end_bits);
				} else {
					sprintf (buf, "%.*f%% of M%ld to 2^%d", (int) PRECISION, pct, p, end_bits);
				}
				title (thread_num, buf);
				if (first_iter_msg == 2) {
					sprintf (buf, SHORT_FACMSG, p, end_bits);
				} else {
					sprintf (buf, FACMSG, p, end_bits, (int) PRECISION, pct);
				}
				if (first_iter_msg) {
					strcat (buf, "\n");
					clear_timer (timers, 0);
				} else {
					strcat (buf, "  Time: ");
					print_timer (timers, 0, buf, TIMER_NL | TIMER_OPT_CLR);
				}
				OutputStr (thread_num, buf);
				iters = 0;
				first_iter_msg = FALSE;
			}

			iters_r += iters_just_processed;
			if (((iters_r * FACTOR_CHUNK_SIZE) >> 10) >= ITER_OUTPUT_RES || (NO_GUI && stop_reason)) {
				sprintf (buf, FACMSG, p, end_bits, (int) PRECISION, trunc_percent (w->pct_complete));
				strcat (buf, "\n");
				writeResults (buf);
				iters_r = 0;
			}

			if (stop_reason || testSaveFilesFlag (thread_num)) {
				factorFindSmallestNotTFed (&facdata);
				if (facdata.asm_data->FACHSW > endpthi ||
				    (facdata.asm_data->FACHSW == endpthi && facdata.asm_data->FACMSW >= endptlo)) {
					if (endptlo == 0) facdata.asm_data->FACHSW = endpthi - 1;
					else facdata.asm_data->FACHSW = endpthi;
					facdata.asm_data->FACMSW = endptlo - 1;
				}
				fd = openWriteSaveFile (&write_save_file_state);
				if (fd > 0 &&
				    write_header (fd, FACTOR_MAGICNUM, FACTOR_VERSION, w) &&
				    write_long (fd, factor_found, NULL) &&
				    write_long (fd, bits, NULL) &&
				    write_long (fd, pass, NULL) &&
				    write_long (fd, facdata.asm_data->FACHSW, NULL) &&
				    write_long (fd, facdata.asm_data->FACMSW, NULL) &&
				    write_long (fd, endpthi, NULL) &&
				    write_long (fd, endptlo, NULL))
					closeWriteSaveFile (&write_save_file_state, fd);
				else {
					sprintf (buf, WRITEFILEERR, filename);
					OutputBoth (thread_num, buf);
					OutputBothErrno (thread_num);
					if (fd > 0) deleteWriteSaveFile (&write_save_file_state, fd);
				}
				if (stop_reason) {
					factorDone (&facdata);
					return (stop_reason);
				}
			}

#ifdef X86_64
			if (facdata.pass_complete) goto nextpass;
#else
			if (facdata.asm_data->FACHSW > endpthi ||
			    (facdata.asm_data->FACHSW == endpthi && facdata.asm_data->FACMSW >= endptlo))
				goto nextpass;
#endif
		}

		factor_found = TRUE;

		find_smaller_factor = (end_bits <= (unsigned int) IniGetInt (INI_FILE, "FindSmallestFactor", 56));

		do {
			makestr (facdata.asm_data->FACHSW, facdata.asm_data->FACMSW, facdata.asm_data->FACLSW, str);
			sprintf (buf, "M%ld has a factor: %s (TF:%d-%d)\n", p, str, (int) w->sieve_depth, (int) test_bits);
			OutputStr (thread_num, buf);
			formatMsgForResultsFile (buf, w);
			writeResults (buf);

			strcpy (JSONbuf, "{\"status\":\"F\"");
			JSONaddExponent (JSONbuf, w);
			strcat (JSONbuf, ", \"worktype\":\"TF\"");
			sprintf (JSONbuf+strlen(JSONbuf), ", \"factors\":[\"%s\"]", str);
			sprintf (JSONbuf+strlen(JSONbuf), ", \"bitlo\":%d", (int) ((bits < report_bits) ? w->sieve_depth : bits));
			sprintf (JSONbuf+strlen(JSONbuf), ", \"bithi\":%d, \"rangecomplete\":false", end_bits);
			JSONaddProgramTimestamp (JSONbuf);
			JSONaddExponentKnownFactors (JSONbuf, w);
			JSONaddUserComputerAID (JSONbuf, w);
			strcat (JSONbuf, "}");
			if (IniGetInt (INI_FILE, "OutputJSON", 1)) writeResultsJSON (JSONbuf);

			if (strlen (str) >= 19 || IniGetInt (INI_FILE, "SendAllFactorData", 0)) {
				struct primenetAssignmentResult pkt;
				memset (&pkt, 0, sizeof (pkt));
				strcpy (pkt.computer_guid, COMPUTER_GUID);
				strcpy (pkt.assignment_uid, w->assignment_uid);
				strcpy (pkt.message, buf);
				pkt.result_type = PRIMENET_AR_TF_FACTOR;
				pkt.n = p;
				strcpy (pkt.factor, str);
				pkt.start_bits = (bits < report_bits) ? (unsigned int) w->sieve_depth : bits;
				pkt.done = !find_smaller_factor;
				strcpy (pkt.JSONmessage, JSONbuf);
				spoolMessage (PRIMENET_ASSIGNMENT_RESULT, &pkt);
			}
		} while (getAnotherFactorIfAny (&facdata));

		if (!find_smaller_factor) break;

		if (facdata.asm_data->FACMSW != 0xFFFFFFFF) {
			endpthi = facdata.asm_data->FACHSW;
			endptlo = facdata.asm_data->FACMSW+1;
		} else {
			endpthi = facdata.asm_data->FACHSW+1;
			endptlo = 0;
		}
	        endpt = endpthi * 4294967296.0 + endptlo;

nextpass:	;
	    }

	    if (factor_found) {
		if (w->assignment_uid[0] && find_smaller_factor) {
			struct primenetAssignmentResult pkt;
			memset (&pkt, 0, sizeof (pkt));
			strcpy (pkt.computer_guid, COMPUTER_GUID);
			strcpy (pkt.assignment_uid, w->assignment_uid);
			pkt.result_type = PRIMENET_AR_NO_RESULT;
			pkt.done = TRUE;
			spoolMessage (PRIMENET_ASSIGNMENT_RESULT, &pkt);
		}
		if (!find_smaller_factor) break;
	    }

	    if (end_bits >= report_bits) {
		unsigned int start_bits;

		start_bits = (end_bits == report_bits) ? (unsigned int) w->sieve_depth : bits;
		if (start_bits < 32)
		    sprintf (buf, "M%ld no factor to 2^%d, Wh%d: %08lX\n", p, end_bits, PORT, SEC3 (p));
		else
		    sprintf (buf, "M%ld no factor from 2^%d to 2^%d, Wh%d: %08lX\n", p, start_bits, end_bits, PORT, SEC3 (p));
		OutputStr (thread_num, buf);
		formatMsgForResultsFile (buf, w);
		writeResults (buf);

		strcpy (JSONbuf, "{\"status\":\"NF\"");
		JSONaddExponent (JSONbuf, w);
		strcat (JSONbuf, ", \"worktype\":\"TF\"");
		sprintf (JSONbuf+strlen(JSONbuf), ", \"bitlo\":%d", start_bits);
		sprintf (JSONbuf+strlen(JSONbuf), ", \"bithi\":%d, \"rangecomplete\":true", end_bits);
		sprintf (JSONbuf+strlen(JSONbuf), ", \"security-code\":\"%08lX\"", SEC3 (p));
		JSONaddProgramTimestamp (JSONbuf);
		JSONaddExponentKnownFactors (JSONbuf, w);
		JSONaddUserComputerAID (JSONbuf, w);
		strcat (JSONbuf, "}");
		if (IniGetInt (INI_FILE, "OutputJSON", 1)) writeResultsJSON (JSONbuf);

		if (end_bits >= 60 || IniGetInt (INI_FILE, "SendAllFactorData", 0)) {
			struct primenetAssignmentResult pkt;
			memset (&pkt, 0, sizeof (pkt));
			strcpy (pkt.computer_guid, COMPUTER_GUID);
			strcpy (pkt.assignment_uid, w->assignment_uid);
			strcpy (pkt.message, buf);
			pkt.result_type = PRIMENET_AR_TF_NOFACTOR;
			pkt.n = p;
			pkt.start_bits = start_bits;
			pkt.end_bits = end_bits;
			pkt.done = (w->work_type == WORK_FACTOR) && (end_bits >= test_bits);
			strcpy (pkt.JSONmessage, JSONbuf);
			spoolMessage (PRIMENET_ASSIGNMENT_RESULT, &pkt);
		}
	    }

	    bits = end_bits;
	}

	factorDone (&facdata);

	unlinkSaveFiles (&write_save_file_state);

	if (factor_found) invalidateNextRollingAverageUpdate ();

	if (factor_found || w->work_type == WORK_FACTOR)
		return (STOP_WORK_UNIT_COMPLETE);

	w->sieve_depth = bits;
	stop_reason = updateWorkToDoLine (thread_num, w);
	if (stop_reason) return (stop_reason);

	return (0);
}

typedef struct {		
	gwhandle gwdata;	
				
	gwnum	lldata;		
	unsigned long units_bit; 
} llhandle;

int lucasSetup (
	int	thread_num,	
	unsigned long p,	
	unsigned long fftlen,	
	llhandle *lldata)	
{
	int	res;
	int	c;

	lldata->lldata = NULL;
	lldata->units_bit = 0;

	c = (fftlen & 1) ? 1 : -1;
	fftlen &= ~1;

	gwset_safety_margin (&lldata->gwdata, IniGetFloat (INI_FILE, "ExtraSafetyMargin", 0.0));
	gwset_minimum_fftlen (&lldata->gwdata, fftlen);
	if (!IniGetInt (INI_FILE, "GW_BENCH_SPECIAL", 0))
		res = gwsetup (&lldata->gwdata, 1.0, 2, p, c);
	else {	
		res = gwsetup (&lldata->gwdata, 
			       IniGetFloat (INI_FILE, "GW_BENCH_SPECIAL_K", 1),
			       IniGetInt (INI_FILE, "GW_BENCH_SPECIAL_B", 2),
			       IniGetInt (INI_FILE, "GW_BENCH_SPECIAL_N", p),
			       IniGetInt (INI_FILE, "GW_BENCH_SPECIAL_C", -1));
	}

	if (res) {
		if (!lldata->gwdata.bench_pick_nth_fft) {
			char	buf[180];
			sprintf (buf, "Cannot initialize FFT code, errcode=%d\n", res);
			OutputBoth (thread_num, buf);
			gwerror_text (&lldata->gwdata, res, buf, sizeof (buf) - 1);
			strcat (buf, "\n");
			OutputBoth (thread_num, buf);
		}
		return (STOP_FATAL_ERROR);
	}

	lldata->lldata = gwalloc (&lldata->gwdata);
	if (lldata->lldata == NULL) {
		gwdone (&lldata->gwdata);
		OutputStr (thread_num, "Error allocating memory for FFT data.\n");
		return (STOP_OUT_OF_MEM);
	}
	return (0);
}

void lucasDone (
	llhandle *lldata)	
{

	gwfree (&lldata->gwdata, lldata->lldata);

	gwdone (&lldata->gwdata);
}

int generateResidue64 (
	llhandle *lldata,
	unsigned long p,	
	unsigned long *reshi,
	unsigned long *reslo)
{
	uint64_t res64;
	giant	tmp;
	int	err_code;

	*reshi = *reslo = 0;
	tmp = popg (&lldata->gwdata.gdata, (p >> 5) + 5);
	err_code = gwtogiant (&lldata->gwdata, lldata->lldata, tmp);
	if (err_code < 0) {
		pushg (&lldata->gwdata.gdata, 1);
		return (err_code);
	}
	if (tmp->sign == 0) {
		pushg (&lldata->gwdata.gdata, 1);
		return (0);
	}

	res64 = 0;
	if (lldata->units_bit + 64 > p) {
		if (tmp->sign > 0) res64 = tmp->n[0];
		if (tmp->sign > 1) res64 += ((uint64_t) tmp->n[1]) << 32;
		res64 <<= p - lldata->units_bit;
		if (p < 64) res64 &= (((uint64_t) 1) << p) - 1;
	}

	gshiftright (lldata->units_bit, tmp);
	if (tmp->sign > 0) res64 += tmp->n[0];
	if (tmp->sign > 1) res64 += ((uint64_t) tmp->n[1]) << 32;
	pushg (&lldata->gwdata.gdata, 1);

	*reslo = (uint32_t) res64;
	*reshi = (uint32_t) (res64 >> 32);
	return (1);
}

#define LL_MAGICNUM		0x2c7330a8
#define LL_VERSION		1
#define LL_ERROR_COUNT_OFFSET	52

int writeLLSaveFile (
	llhandle *lldata,
	writeSaveFileState *write_save_file_state,
	struct work_unit *w,
	unsigned long counter,
	unsigned long error_count)
{
	int	fd;
	uint32_t sum = 0;

	fd = openWriteSaveFile (write_save_file_state);
	if (fd < 0) return (FALSE);

	if (!write_header (fd, LL_MAGICNUM, LL_VERSION, w)) goto err;

	if (!write_long (fd, error_count, &sum)) goto err;
	if (!write_long (fd, counter, &sum)) goto err;
	if (!write_long (fd, lldata->units_bit, &sum)) goto err;
	if (!write_gwnum (fd, &lldata->gwdata, lldata->lldata, &sum)) goto err;

	if (!write_checksum (fd, sum)) goto err;

	closeWriteSaveFile (write_save_file_state, fd);
	return (TRUE);

err:	deleteWriteSaveFile (write_save_file_state, fd);
	return (FALSE);
}

void writeNewErrorCount (
	char	*filename,
	unsigned long new_error_count)
{
	int	fd;
	unsigned long old_error_count;
	uint32_t sum;

	fd = _open (filename, _O_BINARY | _O_RDWR);
	if (fd < 0) return;

	if (!read_checksum (fd, &sum)) goto err;
	_lseek (fd, LL_ERROR_COUNT_OFFSET, SEEK_SET);
	if (!read_long (fd, &old_error_count, NULL)) goto err;

	sum = sum - old_error_count + new_error_count;

	if (!write_checksum (fd, sum)) goto err;
	_lseek (fd, LL_ERROR_COUNT_OFFSET, SEEK_SET);
	if (!write_long (fd, new_error_count, NULL)) goto err;

err:	_close (fd);
}

int readLLSaveFile (
	llhandle *lldata,
	char	*filename,
	struct work_unit *w,
	unsigned long *counter,
	unsigned long *error_count)
{
	int	fd;
	uint32_t version, sum, filesum;

	fd = _open (filename, _O_BINARY | _O_RDONLY);
	if (fd <= 0) return (FALSE);

	if (!read_magicnum (fd, LL_MAGICNUM)) goto err;
	if (!read_header (fd, &version, w, &filesum)) goto err;
	if (version != LL_VERSION) goto err;

	sum = 0;
	if (!read_long (fd, error_count, &sum)) goto err;
	if (!read_long (fd, counter, &sum)) goto err;
	if (!read_long (fd, &lldata->units_bit, &sum)) goto err;
	if (lldata->units_bit >= lldata->gwdata.n) goto err;

	if (!read_gwnum (fd, &lldata->gwdata, lldata->lldata, &sum)) goto err;

	if (filesum != sum) goto err;
	_close (fd);
	return (TRUE);
err:	_close (fd);
	return (FALSE);
}

void inc_error_count (
	int	type,
	unsigned long *error_count)
{
	unsigned long addin, orin, maxval;

	addin = orin = 0;
	if (type == 4) addin = 1 << 4, maxval = 0x0F << 4;		
	else if (type == 1) addin = 1 << 8, maxval = 0x3F << 8;		
	else if (type == 5) orin = 1 << 14;				
	else if (type == 6) orin = 1 << 15;				
	else if (type == 2) addin = 1 << 16, maxval = 0xF << 16;	
	else if (type == 7) addin = 1 << 20, maxval = 0xF << 20;	
	else if (type == 3) addin = 1 << 24, maxval = 0x3F << 24;	

	if (addin && (*error_count & maxval) != maxval) *error_count += addin;
	*error_count |= orin;
}

int make_error_count_message (
	unsigned long error_count,
	int	message_type,		
	char	*buf,
	int	buflen)
{
	int	count_repeatable, count_suminp, count_roundoff, count_illegal_sumout, count_total;
	int	count_jacobi, count_gerbicz, count_bad_errors;
	int	force_high_confidence;
	char	local_buf[400], counts_buf[200], confidence[25];

	force_high_confidence = message_type & 0x8000;
	message_type = message_type & 0x7FFF;

	count_repeatable = (error_count >> 24) & 0x3F;
	count_illegal_sumout = (error_count >> 16) & 0xF;
	count_roundoff = (error_count >> 8) & 0x3F;
	count_jacobi = (error_count >> 4) & 0xF;
	count_gerbicz = (error_count >> 20) & 0xF;
	count_suminp = error_count & 0xF;

	count_total = count_illegal_sumout + count_suminp + count_roundoff + count_jacobi + count_gerbicz;
	if (count_total - count_repeatable == 0) return (FALSE);

	counts_buf[0] = 0;

	if (message_type == 1) {
		sprintf (counts_buf, ", errors: %d", count_total);
	}

	if (message_type == 2) {
		sprintf (counts_buf, "%d error%s", count_total, count_total > 1 ? "s": "");
		if (count_repeatable == 1) {
			strcpy (local_buf, ", 1 was repeatable (not an error)");
			strcat (counts_buf, local_buf);
		} else if (count_repeatable > 1) {
			sprintf (local_buf, ", %d%s were repeatable (not errors)",
				 count_repeatable, count_repeatable == 0x3F ? " or more" : "");
			strcat (counts_buf, local_buf);
		}
	}

	if (message_type == 3) {
		if (count_jacobi >= 1) {
			sprintf (local_buf, "%d%s Jacobi error%s, ",
				 count_jacobi, count_jacobi == 0xF ? " or more" : "", count_jacobi > 1 ? "s" : "");
			strcat (counts_buf, local_buf);
		}
		if (count_gerbicz >= 1) {
			sprintf (local_buf, "%d%s Gerbicz/double-check error%s, ",
				 count_gerbicz, count_gerbicz == 0xF ? " or more" : "", count_gerbicz > 1 ? "s" : "");
			strcat (counts_buf, local_buf);
		}
		if (count_roundoff >= 1) {
			sprintf (local_buf, "%d%s ROUNDOFF > 0.4, ",
				 count_roundoff, count_roundoff == 0x3F ? " or more" : "");
			strcat (counts_buf, local_buf);
		}
		if (count_suminp >= 1) {
			sprintf (local_buf, "%d%s SUM(INPUTS) != SUM(OUTPUTS), ",
				 count_suminp, count_suminp == 0xF ? " or more" : "");
			strcat (counts_buf, local_buf);
		}
		if (count_illegal_sumout >= 1) {
			sprintf (local_buf, "%d%s ILLEGAL SUMOUT/bad FFT data, ",
				 count_illegal_sumout, count_illegal_sumout == 0xF ? " or more" : "");
			strcat (counts_buf, local_buf);
		}
		counts_buf[strlen(counts_buf)-2] = 0;
		if (count_repeatable >= 1) {
			if (count_repeatable == 1)
				strcpy (local_buf, "of which 1 was repeatable (not a hardware error)");
			else
				sprintf (local_buf, "of which %d were repeatable (not hardware errors)", count_repeatable);
			if (strlen (counts_buf) <= 40) strcat (counts_buf, " ");
			else strcat (counts_buf, "\n");
			strcat (counts_buf, local_buf);
		}
		strcat (counts_buf, ".\n");
	}

	count_bad_errors = count_jacobi + count_suminp + count_roundoff - count_repeatable;
	if (force_high_confidence) count_bad_errors = 0;
	strcpy (confidence, count_bad_errors == 0 ? "excellent" :
			    count_bad_errors <= 3 ? "fair" :
			    count_bad_errors <= 6 ? "poor" : "very poor");

	if (message_type == 1) {
		sprintf (local_buf, ", %s, confidence: %s", counts_buf, confidence);
	}
	if (message_type == 2) {
		if (count_jacobi || count_gerbicz) sprintf (local_buf, "Hardware errors!  %s.  Confidence in end result is %s.\n", counts_buf, confidence);
		else sprintf (local_buf, "Possible hardware errors!  %s.  Confidence in end result is %s.\n", counts_buf, confidence);
	}
	if (message_type == 3) {
		if (count_jacobi || count_gerbicz) strcpy (local_buf, "Hardware errors have occurred during the test!");
		else strcpy (local_buf, "Possible hardware errors have occurred during the test!");
		if (strlen (counts_buf) <= 25) strcat (local_buf, " ");
		else strcat (local_buf, "\n");
		strcat (local_buf, counts_buf);
		sprintf (local_buf+strlen(local_buf), "Confidence in final result is %s.\n", confidence);
	}

	if ((int) strlen (local_buf) >= buflen) local_buf[buflen-1] = 0;
	strcpy (buf, local_buf);
	return (TRUE);
}

void lucas_fixup (
	llhandle *lldata,
	unsigned long p)	
{

	lldata->units_bit <<= 1;
	if (lldata->units_bit >= p) lldata->units_bit -= p;

	gwsetaddinatpowerofb (&lldata->gwdata, -2, lldata->units_bit);
}

void generateRandomData (
	llhandle *lldata)
{
	unsigned long i;

	srand ((unsigned) time (NULL));
	for (i = 0; i < gwfftlen (&lldata->gwdata); i++) {
		set_fft_value (&lldata->gwdata, lldata->lldata, i, rand() & 0xFF);
	}
}

int exponent_near_fft_limit (
	gwhandle *gwdata)		
{
	return (gwnear_fft_limit (gwdata, IniGetFloat (INI_FILE, "NearFFTLimitPct", 0.5)));
}

void good_news (void *arg)
{
	char	buf[80];

	title (MAIN_THREAD_NUM, "New Prime!!!");
	sprintf (buf, "New Mersenne Prime!!!!  M%d is prime!\n", (int) (intptr_t) arg);
	while (WORKERS_ACTIVE && ! WORKERS_STOPPING) {
		OutputStr (MAIN_THREAD_NUM, buf);
		flashWindowAndBeep ();
		Sleep (50);
	}
}

int rotateg (				
	giant	v,			
	unsigned long p,		
	unsigned long shift_count,	
	ghandle	*gdata)			
{
	giant	vlo;

	if (shift_count == 0) return (1);

	vlo = popg (gdata, (p >> 5) + 5);
	if (vlo == NULL) return (0);

	gtogshiftright (shift_count, v, vlo);		
	gmaskbits (shift_count, v);			
	gshiftleft (p - shift_count, v);		
	addg (vlo, v);					
	pushg (gdata, 1);				
	return (1);
}

int gwrotate_right (			
	gwhandle *gwdata,
	gwnum	x,
	int	shift_count)
{
	giant	tmp;

	ASSERTG (gwdata->k == 1.0 && gwdata->b == 2 && gwdata->c == -1);
	if (shift_count == 0) return (TRUE);

	tmp = popg (&gwdata->gdata, ((unsigned long) gwdata->bit_length >> 5) + 5);
	if (tmp == NULL) return (FALSE);

	if (gwtogiant (gwdata, x, tmp)) {
		pushg (&gwdata->gdata, 1);
		return (FALSE);
	}

	rotateg (tmp, gwdata->n, shift_count, &gwdata->gdata);

	gianttogw (gwdata, tmp, x);

	pushg (&gwdata->gdata, 1);
	return (TRUE);
}

int gwrotate_left (			
	gwhandle *gwdata,
	gwnum	x,
	int	shift_count)
{
	ASSERTG (gwdata->k == 1.0 && gwdata->b == 2 && gwdata->c == -1);
	if (shift_count == 0) return (TRUE);
	return (gwrotate_right (gwdata, x, gwdata->n - shift_count));
}

int jacobi_test (
	int	thread_num,		
	unsigned long p,		
	llhandle *lldata)		
{
	giant	v;
	mpz_t	a, b;
	int	err_code, Jacobi_symbol, silent_Jacobi;
	double	timers[1];
	char	buf[80];

	clear_timers (timers, sizeof (timers) / sizeof (timers[0]));
	start_timer (timers, 0);

	v = popg (&lldata->gwdata.gdata, (p >> 5) + 5);
	if (v == NULL) goto oom;
	err_code = gwtogiant (&lldata->gwdata, lldata->lldata, v);
	if (err_code < 0) {		
		OutputBoth (thread_num, "LL value corrupt.  Could not run Jacobi error check.\n");
		pushg (&lldata->gwdata.gdata, 1);
		return (0);
	}
	if (! rotateg (v, p, lldata->units_bit, &lldata->gwdata.gdata)) {
		pushg (&lldata->gwdata.gdata, 1);
		goto oom;
	}

	mpz_init (a);
	gtompz (v, a);
	pushg (&lldata->gwdata.gdata, 1);

	mpz_init (b);
	mpz_ui_pow_ui (b, 2, p);
	mpz_sub_ui (b, b, 1);

	silent_Jacobi = IniGetInt (INI_FILE, "SilentJacobi", 0);
	if (!silent_Jacobi) OutputStr (thread_num, "Running Jacobi error check.  ");
	mpz_sub_ui (a, a, 2);
	if (mpz_sgn (a) < 0) mpz_add (a, a, b);
	Jacobi_symbol = mpz_jacobi (a, b);

	end_timer (timers, 0);
	sprintf (buf, "%s.  Time: %6.3f sec.\n", Jacobi_symbol == -1 ? "Passed" : "Failed", timer_value (timers, 0));
	if (!silent_Jacobi) OutputStrNoTimeStamp (thread_num, buf);
	else if (Jacobi_symbol != -1) OutputStr (thread_num, "Jacobi error-check failed\n");

	mpz_clear (a);
	mpz_clear (b);
	return (Jacobi_symbol == -1);

oom:	OutputBoth (thread_num, "Memory allocation error.  Could not run Jacobi error check.\n");
	return (1);			
}

int prime (
	int	thread_num,		
	struct PriorityInfo *sp_info,	
	struct work_unit *w,		
	int	pass)			
{
	llhandle lldata;
	unsigned long p;
	unsigned long counter;
	unsigned long error_count;
	unsigned long restart_error_count = 0;	
	unsigned long iters;
	readSaveFileState read_save_file_state; 
	writeSaveFileState write_save_file_state; 
	char	filename[32];
	double	timers[2];
	double	inverse_p;
	double	reallyminerr = 1.0;
	double	reallymaxerr = 0.0;
	double	*addr1;
	int	Jacobi_testing_enabled, pass_to_factor;
	int	first_iter_msg, near_fft_limit, sleep5;
	unsigned long high32, low32;
	int	rc, isPrime, stop_reason;
	char	buf[400], JSONbuf[4000], fft_desc[200];
	int	slow_iteration_count;
	double	best_iteration_time;
	unsigned long last_counter = 0xFFFFFFFF;	
	int	maxerr_recovery_mode = 0;		
	double	last_maxerr = 0.0;
	double	allowable_maxerr, output_frequency, output_title_frequency;
	int	error_count_messages;

	p = w->n;

	Jacobi_testing_enabled = IniGetInt (INI_FILE, "JacobiErrorCheck", 1);
	{
		int	major_ver, minor_ver;
		sscanf (gmp_version, "%d.%d", &major_ver, &minor_ver);
		if (major_ver < 5 || (major_ver == 5 && minor_ver < 1)) Jacobi_testing_enabled = FALSE;
	}

	pass_to_factor = (WELL_BEHAVED_WORK || SEQUENTIAL_WORK != 0) ? 2 : 1;
	if (pass < pass_to_factor) return (0);

	if ((w->work_type == WORK_TEST || w->work_type == WORK_DBLCHK) && ! IniGetInt (INI_FILE, "SkipTrialFactoring", 0)) {
		struct PriorityInfo sp_info_copy;
		memcpy (&sp_info_copy, sp_info, sizeof (struct PriorityInfo));
		stop_reason = primeFactor (thread_num, &sp_info_copy, w, 1);
		if (stop_reason) return (stop_reason);
	}

	if ((w->work_type == WORK_TEST || w->work_type == WORK_DBLCHK) && ! w->pminus1ed && pass != 3) {
		stop_reason = pfactor (thread_num, sp_info, w);
		if (stop_reason) return (stop_reason);
	}

	if ((w->work_type == WORK_TEST || w->work_type == WORK_DBLCHK) && ! IniGetInt (INI_FILE, "SkipTrialFactoring", 0)) {
		struct PriorityInfo sp_info_copy;
		memcpy (&sp_info_copy, sp_info, sizeof (struct PriorityInfo));
		stop_reason = primeFactor (thread_num, &sp_info_copy, w, 0);
		if (stop_reason) return (stop_reason);
	}

	if (pass == 1 && w->work_type != WORK_ADVANCEDTEST) return (0);

#ifdef ONE_HOUR_SELF_TEST
	if (w->work_type == WORK_TEST || w->work_type == WORK_DBLCHK) {
		stop_reason = selfTest (thread_num, sp_info, w);
		if (stop_reason) return (stop_reason);
	}
#endif

	tempFileName (w, filename);
	writeSaveFileStateInit (&write_save_file_state, filename, NUM_JACOBI_BACKUP_FILES);

begin:	gwinit (&lldata.gwdata);
	if (IniGetInt (INI_FILE, "UseLargePages", 0)) gwset_use_large_pages (&lldata.gwdata);
	if (HYPERTHREAD_LL) sp_info->normal_work_hyperthreading = TRUE, gwset_will_hyperthread (&lldata.gwdata, 2);
	gwset_bench_cores (&lldata.gwdata, HW_NUM_CORES);
	gwset_bench_workers (&lldata.gwdata, NUM_WORKERS);
	if (ERRCHK) gwset_will_error_check (&lldata.gwdata);
	else gwset_will_error_check_near_limit (&lldata.gwdata);
	gwset_num_threads (&lldata.gwdata, get_worker_num_threads (thread_num, HYPERTHREAD_LL));
	gwset_thread_callback (&lldata.gwdata, SetAuxThreadPriority);
	gwset_thread_callback_data (&lldata.gwdata, sp_info);
	gwset_use_spin_wait (&lldata.gwdata, IniGetInt (INI_FILE, "SpinWait", 0));
	stop_reason = lucasSetup (thread_num, p, w->minimum_fftlen, &lldata);
	if (stop_reason) return (stop_reason);

	set_memory_usage (thread_num, 0, cvt_gwnums_to_mem (&lldata.gwdata, 1));

	readSaveFileStateInit (&read_save_file_state, thread_num, filename);
	for ( ; ; ) {

		if (! saveFileExists (&read_save_file_state)) {
			
			if (read_save_file_state.a_non_bad_save_file_existed ||
			    (pass == 3 && read_save_file_state.a_save_file_existed)) {
				OutputBoth (thread_num, ALLSAVEBAD_MSG);
				return (0);
			}
			
			counter = 2;
			error_count = 0;
			first_iter_msg = FALSE;
			break;
		}

		if (readLLSaveFile (&lldata, read_save_file_state.current_filename, w, &counter, &error_count) &&
		    counter <= w->n &&
		    (!Jacobi_testing_enabled || jacobi_test (thread_num, p, &lldata))) {
			first_iter_msg = TRUE;
			if (Jacobi_testing_enabled) setWriteSaveFileSpecial (&write_save_file_state);
			break;
		}

		saveFileBad (&read_save_file_state);
	}

	if (restart_error_count) error_count = restart_error_count;

	best_iteration_time = 1.0e50;
	slow_iteration_count = 0;

	clear_timers (timers, sizeof (timers) / sizeof (timers[0]));

	sprintf (buf, "Iteration %ld of LL M%ld", counter, p);
	title (thread_num, buf);

	strcpy (w->stage, "LL");
	inverse_p = 1.0 / (double) p;
	w->pct_complete = (double) counter * inverse_p;
	calc_output_frequencies (&lldata.gwdata, &output_frequency, &output_title_frequency);

	if (counter == 2) {
		unsigned long S0;
		if ((S0 = IniGetInt (INI_FILE, "InitialLLValue", 4)) != 4) {
			if (S0 == 23) {	
				giant tmp;
				tmp = allocgiant ((p >> 5) + 5);
				if (tmp == NULL) return (OutOfMemory (thread_num));
				ultog (2, tmp);
				power (tmp, p);
				iaddg (1, tmp);
				dbldivg (3, tmp);
				gianttogw (&lldata.gwdata, tmp, lldata.lldata);
			} else {
				dbltogw (&lldata.gwdata, (double) S0, lldata.lldata);
			}
			lldata.units_bit = 0;
		} else {
			unsigned long i, word, bit_in_word;
			uint32_t hi, lo;
			
			srand ((unsigned) time (NULL));
			lldata.units_bit = (rand () << 16) + rand ();
			if (CPU_FLAGS & CPU_RDTSC) { rdtsc(&hi,&lo); lldata.units_bit += lo; }
			
			lldata.units_bit = IniGetInt (INI_FILE, "InitialShiftCount", lldata.units_bit);
			
			lldata.units_bit = lldata.units_bit % p;
			
			bitaddr (&lldata.gwdata, (lldata.units_bit + 2) % p, &word, &bit_in_word);
			for (i = 0; i < gwfftlen (&lldata.gwdata); i++) {
				set_fft_value (&lldata.gwdata, lldata.lldata, i, (i == word) ? (1L << bit_in_word) : 0);
			}
		}
	}

	gwfft_description (&lldata.gwdata, fft_desc);
	sprintf (buf, "%s primality test of M%ld using %s\n", (counter == 2) ? "Starting" : "Resuming", p, fft_desc);
	OutputStr (thread_num, buf);

	near_fft_limit = exponent_near_fft_limit (&lldata.gwdata);

	allowable_maxerr = IniGetFloat (INI_FILE, "MaxRoundoffError", (float) (near_fft_limit ? 0.421875 : 0.40625));

	addr1 = addr (&lldata.gwdata, lldata.lldata, 1);

	iters = 0;
	error_count_messages = IniGetInt (INI_FILE, "ErrorCountMessages", 3);
	while (counter < p) {
		int	saving, Jacobi_testing, echk, sending_residue, interim_residue, interim_file;
		int	actual_frequency;

		stop_reason = stopCheck (thread_num);

		saving = counter+1 != p && (stop_reason || counter == last_counter-8 || counter == last_counter || testSaveFilesFlag (thread_num));

		Jacobi_testing = Jacobi_testing_enabled && (counter+1 == p || (!stop_reason && saving && testJacobiFlag (thread_num)));

		echk = saving || near_fft_limit || ERRCHK || (counter >= p - 50) || ((counter & 127) == 0);
		gw_clear_maxerr (&lldata.gwdata);

		sending_residue = ((counter+1) == 500002 || ((counter+1) % 5000000 == 2)) && IniGetInt (INI_FILE, "SendInterimResidues", 0);
		interim_residue = (INTERIM_RESIDUES && (counter+1) % INTERIM_RESIDUES <= 2);
		interim_file = (INTERIM_FILES && (counter+1) % INTERIM_FILES == 0);

		timers[1] = 0.0;
		start_timer (timers, 1);

		if (maxerr_recovery_mode && counter == last_counter) {
			gwsetnormroutine (&lldata.gwdata, 0, echk, 0);
			gwstartnextfft (&lldata.gwdata, 0);
			lucas_fixup (&lldata, p);
			gwsquare_carefully (&lldata.gwdata, lldata.lldata);
			maxerr_recovery_mode = 0;

			gw_clear_error (&lldata.gwdata);
			echk = 0;
		}

#ifndef SERVER_TESTING
		else {
			gwsetnormroutine (&lldata.gwdata, 0, echk, 0);
			gwstartnextfft (&lldata.gwdata,
					!saving && !maxerr_recovery_mode && !Jacobi_testing &&
					!sending_residue && !interim_residue && !interim_file &&
					counter+1 != p);
			lucas_fixup (&lldata, p);
			gwsquare (&lldata.gwdata, lldata.lldata);
		}
#endif

		end_timer (timers, 1);
		timers[0] += timers[1];
		iters++;

		if (echk) {
			if (gw_get_maxerr (&lldata.gwdata) < reallyminerr && counter > 30)
				reallyminerr = gw_get_maxerr (&lldata.gwdata);
			if (gw_get_maxerr (&lldata.gwdata) > reallymaxerr)
				reallymaxerr = gw_get_maxerr (&lldata.gwdata);
		}

		if (gw_test_illegal_sumout (&lldata.gwdata)) {
			sprintf (buf, ERRMSG0, counter, p, ERRMSG1A);
			OutputBoth (thread_num, buf);
			inc_error_count (2, &error_count);
			sleep5 = TRUE;
			goto restart;
		}

		if (echk && gw_get_maxerr (&lldata.gwdata) > allowable_maxerr) {
			if (counter == last_counter && gw_get_maxerr (&lldata.gwdata) == last_maxerr) {
				OutputBoth (thread_num, ERROK);
				inc_error_count (3, &error_count);
				gw_clear_error (&lldata.gwdata);
				OutputBoth (thread_num, ERRMSG5);
				maxerr_recovery_mode = 1;
				sleep5 = FALSE;
				goto restart;
			} else {
				char	msg[100];
				sprintf (msg, ERRMSG1C, gw_get_maxerr (&lldata.gwdata), allowable_maxerr);
				sprintf (buf, ERRMSG0, counter, p, msg);
				OutputBoth (thread_num, buf);
				last_counter = counter;
				last_maxerr = gw_get_maxerr (&lldata.gwdata);
				inc_error_count (1, &error_count);
				sleep5 = FALSE;
				goto restart;
			}
		}

		if (lldata.units_bit >= p) {
			sprintf (buf, ERRMSG0, counter, p, ERRMSG1D);
			OutputBoth (thread_num, buf);
			inc_error_count (6, &error_count);
			sleep5 = TRUE;
			goto restart;
		}

		if (Jacobi_testing && !jacobi_test (thread_num, p, &lldata)) {
			sprintf (buf, ERRMSG0, counter, p, ERRMSG1G);
			OutputBoth (thread_num, buf);
			inc_error_count (4, &error_count);
			sleep5 = FALSE;
			goto restart;
		}

#ifndef SERVER_TESTING
		if (*addr1 == 0.0 && p > 1000 &&
		    counter > 50 && counter < p-2 && counter != last_counter) {
			unsigned long i;
			for (i = 2; ; i++) {
				if (*addr (&lldata.gwdata, lldata.lldata, i) != 0.0) break;
				if (i == 50) {
					sprintf (buf, ERRMSG0, counter, p, ERRMSG1F);
					OutputBoth (thread_num, buf);
					inc_error_count (5, &error_count);
					last_counter = counter;
					sleep5 = TRUE;
					goto restart;
				}
			}
		}
#endif

		counter++;
		w->pct_complete = (double) counter * inverse_p;

		actual_frequency = (int) (ITER_OUTPUT * output_title_frequency);
		if (actual_frequency < 1) actual_frequency = 1;
		if (counter % actual_frequency == 0 || first_iter_msg) {
			sprintf (buf, "%.*f%% of LL M%ld", (int) PRECISION, trunc_percent (w->pct_complete), p);
			title (thread_num, buf);
		}

		actual_frequency = (int) (ITER_OUTPUT * output_frequency);
		if (actual_frequency < 1) actual_frequency = 1;
		if (counter % actual_frequency == 0 || first_iter_msg) {
			sprintf (buf, "Iteration: %ld / %ld [%.*f%%]", counter, p, (int) PRECISION, trunc_percent (w->pct_complete));
			
			if (error_count_messages == 1)
				make_error_count_message (error_count, error_count_messages,
							  buf + strlen (buf),
							  (int) (sizeof (buf) - strlen (buf)));
			
			if (first_iter_msg) {
				strcat (buf, ".\n");
				clear_timer (timers, 0);
				first_iter_msg = FALSE;
			}
			
			else if (!CLASSIC_OUTPUT) {
				double speed;
				
				if ((OUTPUT_ROUNDOFF || ERRCHK) && reallymaxerr >= 0.001) {
					sprintf (buf+strlen(buf), ", roundoff: %5.3f", reallymaxerr);
					if (!CUMULATIVE_ROUNDOFF) reallyminerr = 1.0, reallymaxerr = 0.0;
				}
				
				speed = timer_value (timers, 0) / (double) iters;
				sprintf (buf+strlen(buf), ", ms/iter: %6.3f", speed * 1000.0);
				clear_timer (timers, 0);
				iters = 0;
				
				formatETA ((p - counter) * speed, buf+strlen(buf));
				strcat (buf, "\n");
			}
			
			else {
				
				if (ERRCHK && counter > 30) {
					sprintf (buf+strlen(buf), ".  Round off: %10.10f to %10.10f", reallyminerr, reallymaxerr);
					if (!CUMULATIVE_ROUNDOFF) reallyminerr = 1.0, reallymaxerr = 0.0;
				}
				if (CUMULATIVE_TIMING) {
					strcat (buf, ".  Total time: ");
					print_timer (timers, 0, buf, TIMER_NL);
				} else {
					strcat (buf, ".  Per iteration time: ");
					divide_timer (timers, 0, iters);
					print_timer (timers, 0, buf, TIMER_NL | TIMER_CLR);
					iters = 0;
				}
			}
			OutputStr (thread_num, buf);

			if (error_count_messages >= 2 &&
			    make_error_count_message (error_count, error_count_messages, buf, sizeof (buf)))
				OutputStr (thread_num, buf);
		}

		if (counter % ITER_OUTPUT_RES == 0 || (NO_GUI && stop_reason)) {
			sprintf (buf, "Iteration %ld / %ld\n", counter, p);
			writeResults (buf);
		}

		if (saving) {
			if (! writeLLSaveFile (&lldata, &write_save_file_state, w, counter, error_count)) {
				sprintf (buf, WRITEFILEERR, filename);
				OutputBoth (thread_num, buf);
				OutputBothErrno (thread_num);
			}
			if (Jacobi_testing) setWriteSaveFileSpecial (&write_save_file_state);
		}

		if (stop_reason) {
			sprintf (buf, "Stopping primality test of M%ld at iteration %ld [%.*f%%]\n",
				 p, counter, (int) PRECISION, trunc_percent (w->pct_complete));
			OutputStr (thread_num, buf);
			lucasDone (&lldata);
			return (stop_reason);
		}

		if (sending_residue && w->assignment_uid[0]) {
			struct primenetAssignmentProgress pkt;
			memset (&pkt, 0, sizeof (pkt));
			strcpy (pkt.computer_guid, COMPUTER_GUID);
			pkt.cpu_num = thread_num;
			strcpy (pkt.assignment_uid, w->assignment_uid);
			strcpy (pkt.stage, w->stage);
			pkt.pct_complete = w->pct_complete * 100.0;
			pkt.end_date = (unsigned long) work_estimate (thread_num, w);
			pkt.next_update = (uint32_t) (DAYS_BETWEEN_CHECKINS * 86400.0);
			pkt.fftlen = w->fftlen;
			pkt.iteration = counter - 2;
			if (generateResidue64 (&lldata, p, &high32, &low32) < 0) {
				OutputBoth (thread_num, ERRMSG8);
				inc_error_count (2, &error_count);
				sleep5 = TRUE;
				goto restart;
			}
			sprintf (pkt.residue, "%08lX%08lX", high32, low32);
			sprintf (pkt.error_count, "%08lX", error_count);
			spoolMessage (-PRIMENET_ASSIGNMENT_PROGRESS, &pkt);
		}

		if (interim_residue) {
			if (generateResidue64 (&lldata, p, &high32, &low32) < 0) {
				OutputBoth (thread_num, ERRMSG8);
				inc_error_count (2, &error_count);
				sleep5 = TRUE;
				goto restart;
			}
			sprintf (buf, "M%ld interim LL residue %08lX%08lX at iteration %ld\n", p, high32, low32, counter);
			OutputBoth (thread_num, buf);
		}

		if (interim_file) {
			char	interimfile[32];
			writeSaveFileState state;
			sprintf (interimfile, "%s.%03ld", filename, counter / INTERIM_FILES);
			writeSaveFileStateInit (&state, interimfile, 0);
			state.num_ordinary_save_files = 99;
			if (! writeLLSaveFile (&lldata, &state, w, counter, error_count)) {
				sprintf (buf, WRITEFILEERR, interimfile);
				OutputBoth (thread_num, buf);
				OutputBothErrno (thread_num);
			}

		}

		if (HYPERTHREADING_BACKOFF && p > 10000000) {
			if (timers[1] < best_iteration_time)
				best_iteration_time = timers[1];
			if (timers[1] > 1.40 * best_iteration_time) {
				if (slow_iteration_count == 10) {
					sprintf (buf, "Pausing %lu seconds.\n", HYPERTHREADING_BACKOFF);
					OutputStr (thread_num, buf);
					Sleep (HYPERTHREADING_BACKOFF * 1000);
				}
				slow_iteration_count++;
			} else
				slow_iteration_count = 0;
		}
	}

	rc = generateResidue64 (&lldata, p, &high32, &low32);
	if (rc < 0) {
		sprintf (buf, ERRMSG0, counter, p, ERRMSG1E);
		OutputBoth (thread_num, buf);
		inc_error_count (2, &error_count);
		sleep5 = TRUE;
		goto restart;
	}
	isPrime = (rc == 0);

	if (isPrime)
		sprintf (buf, "M%ld is prime! Wh%d: %08lX,%08lX\n", p, PORT, SEC1 (p), error_count);
	else
		sprintf (buf,
			 "M%ld is not prime. Res64: %08lX%08lX. Wh%d: %08lX,%ld,%08lX\n",
			 p, high32, low32, PORT,
			 SEC2 (p, high32, low32, lldata.units_bit, error_count),
			 lldata.units_bit, error_count);
	OutputStr (thread_num, buf);
	formatMsgForResultsFile (buf, w);
	rc = writeResults (buf);

	sprintf (JSONbuf, "{\"status\":\"%s\"", isPrime ? "P" : "C");
	JSONaddExponent (JSONbuf, w);
	strcat (JSONbuf, ", \"worktype\":\"LL\"");
	if (!isPrime) sprintf (JSONbuf+strlen(JSONbuf), ", \"res64\":\"%08lX%08lX\"", high32, low32);
	sprintf (JSONbuf+strlen(JSONbuf), ", \"fft-length\":%lu", lldata.gwdata.FFTLEN);
	sprintf (JSONbuf+strlen(JSONbuf), ", \"shift-count\":%ld", lldata.units_bit);
	sprintf (JSONbuf+strlen(JSONbuf), ", \"error-code\":\"%08lX\"", error_count);
	sprintf (JSONbuf+strlen(JSONbuf), ", \"security-code\":\"%08lX\"", SEC2 (p, high32, low32, lldata.units_bit, error_count));
	JSONaddProgramTimestamp (JSONbuf);
	if (error_count & 0x3FF0) {
		strcat (JSONbuf, ", \"errors\":{");
		if (error_count & 0x3F00) sprintf (JSONbuf+strlen(JSONbuf), "\"Roundoff\":%lu, ", (error_count >> 8) & 0x3F);
		if (error_count & 0xF0) sprintf (JSONbuf+strlen(JSONbuf), "\"Jacobi\":%lu, ", (error_count >> 4) & 0xF);
		JSONbuf[strlen(JSONbuf)-2] = 0;
		strcat (JSONbuf, "}");
	}
	JSONaddExponentKnownFactors (JSONbuf, w);
	JSONaddUserComputerAID (JSONbuf, w);
	strcat (JSONbuf, "}");
	if (IniGetInt (INI_FILE, "OutputJSON", 1)) writeResultsJSON (JSONbuf);

	if (p > 1000000 && (!isPrime || !isKnownMersennePrime (p))) {
		struct primenetAssignmentResult pkt;
		memset (&pkt, 0, sizeof (pkt));
		strcpy (pkt.computer_guid, COMPUTER_GUID);
		strcpy (pkt.assignment_uid, w->assignment_uid);
		strcpy (pkt.message, buf);
		pkt.result_type = isPrime ? PRIMENET_AR_LL_PRIME : PRIMENET_AR_LL_RESULT;
		pkt.n = p;
		sprintf (pkt.residue, "%08lX%08lX", high32, low32);
		pkt.shift_count = lldata.units_bit;
		sprintf (pkt.error_count, "%08lX", error_count);
		pkt.fftlen = gwfftlen (&lldata.gwdata);
		pkt.done = TRUE;
		strcpy (pkt.JSONmessage, JSONbuf);
		spoolMessage (PRIMENET_ASSIGNMENT_RESULT, &pkt);
	}

	if (!isPrime || isKnownMersennePrime (p)) {
		if (rc) unlinkSaveFiles (&write_save_file_state);
	}

	lucasDone (&lldata);

	if (isPrime && !SILENT_VICTORY && !isKnownMersennePrime (p)) {
		gwthread thread_handle;
		gwthread_create (&thread_handle, &good_news, (void *) (intptr_t) p);
	}

	return (STOP_WORK_UNIT_COMPLETE);

restart:if (sleep5) OutputBoth (thread_num, ERRMSG2);
	OutputBoth (thread_num, ERRMSG3);

	restart_error_count = error_count;

	if (sleep5) {
		stop_reason = SleepFive (thread_num);
		if (stop_reason) return (stop_reason);
	}

	lucasDone (&lldata);
	goto begin;
}

static const char TORTURE1[] = "Beginning a continuous torture test on your computer.\n";
#if defined (__linux__) || defined (__FreeBSD__) || defined (__EMX__)
static const char TORTURE2[] = "Please read stress.txt.  Hit ^C to end this test.\n";
#else
static const char TORTURE2[] = "Please read stress.txt.  Choose Test/Stop to end this test.\n";
#endif
static const char SELF1[] = "Test %i%s, %i Lucas-Lehmer %siterations of M%ld using %s.\n";
static const char SELFFAIL[] = "FATAL ERROR: Final result was %08lX, expected: %08lX.\n";
static const char SELFFAIL1[] = "ERROR: ILLEGAL SUMOUT\n";
static const char SELFFAIL3[] = "FATAL ERROR: Rounding was %.10g, expected less than 0.4\n";
static const char SELFFAIL4[] = "Possible hardware failure, consult readme.txt file, restarting test.\n";
static const char SELFFAIL5[] = "Hardware failure detected running %lu%s FFT size, consult stress.txt file.\n";
static const char SELFFAIL6[] = "Maximum number of warnings exceeded.\n";
static const char SELFFAIL7[] = "TORTURE TEST FAILED on worker #%d.\n";

static const char SELFPASS[] = "Self-test %i%s%s passed!\n";

struct self_test_info {
	unsigned long p;
	unsigned long iters;
	unsigned long reshi;
};

#define MAX_SELF_TEST_ITERS	357
const struct self_test_info SELF_TEST_DATA[MAX_SELF_TEST_ITERS] = {
{560000001, 100, 0x7F853A0A}, {420000001, 150, 0x89665E7E}, {280000001, 200, 0xC32CAD46}, {210000001, 300, 0x89823329},
{140000001, 400, 0x15EF4F24}, {110000001, 500, 0x893C9000}, {78643201, 400, 0x2D9C8904}, {78643199, 400, 0x7D469182},
{75497473, 400, 0x052C7FD8}, {75497471, 400, 0xCCE7495D}, {71303169, 400, 0x467A9338}, {71303167, 400, 0xBBF8B37D},
{68157441, 400, 0xBE71E616}, {68157439, 400, 0x93A71CC2}, {66060289, 400, 0xF296BB99}, {66060287, 400, 0x649EEF2A},
{62390273, 400, 0xBC8DFC27}, {62390271, 400, 0xDE7D5B5E}, {56623105, 400, 0x0AEBF972}, {56623103, 400, 0x1BA96297},
{53477377, 400, 0x5455F347}, {53477375, 400, 0xCE1C7F78}, {50331649, 400, 0x3D746AC8}, {50331647, 400, 0xE23F2DE6},
{49807361, 400, 0xB43EF4C5}, {49807359, 400, 0xA8BEB02D}, {47185921, 400, 0xD862563C}, {47185919, 400, 0x17281086},
{41943041, 400, 0x0EDA1F92}, {41943039, 400, 0xDE6911AE}, {39845889, 400, 0x43D8A96A}, {39845887, 400, 0x3D118E8F},
{37748737, 400, 0x38261154}, {37748735, 400, 0x22B34CD2}, {35651585, 400, 0xB0E48D2E}, {35651583, 400, 0xCC3340C6},
{34865153, 400, 0xD2C00E6C}, {34865151, 400, 0xFA644F69}, {33030145, 400, 0x83E5738D}, {33030143, 400, 0x6EDBC5B5},
{31195137, 400, 0xFF9591CF}, {31195135, 400, 0x04577C70}, {29884417, 400, 0xACC36457}, {29884415, 400, 0xC0FE7B1E},
{28311553, 400, 0x780EB8F5}, {28311551, 400, 0xE6D128C3}, {26738689, 400, 0x09DC45B0}, {26738687, 400, 0xDC7C074A},
{24903681, 400, 0xA482CF1E}, {24903679, 400, 0x4B3F5121}, {23592961, 400, 0xAFE3C198}, {23592959, 400, 0xCF9AD48C},
{20971521, 400, 0x304EC13B}, {20971519, 400, 0x9C4E157E}, {19922945, 400, 0x83FE36D9}, {19922943, 400, 0x9C60E7A2},
{18874369, 400, 0x83A9F8CB}, {18874367, 400, 0x5A6E22E0}, {17825793, 400, 0xF3A90A5E}, {17825791, 400, 0x6477CA76},
{17432577, 400, 0xCAB36E6A}, {17432575, 400, 0xB8F814C6}, {16515073, 400, 0x91EFCB1C}, {16515071, 400, 0xA0C35CD9},
{15597569, 400, 0x12E057AD}, {15597567, 400, 0xC4EFAEFD}, {14942209, 400, 0x1C912A7B}, {14942207, 400, 0xABA9EA6E},
{14155777, 400, 0x4A943A4E}, {14155775, 400, 0x00789FB9}, {13369345, 400, 0x27A041EE}, {13369343, 400, 0xA8B01A41},
{12451841, 400, 0x4DC891F6}, {12451839, 400, 0xA75BF824}, {11796481, 400, 0xFDD67368}, {11796479, 400, 0xE0237D19},
{10485761, 400, 0x15419597}, {10485759, 400, 0x154D473B}, {10223617, 400, 0x26039EB7}, {10223615, 400, 0xC9DFB1A4},
{9961473, 400, 0x3EB29644}, {9961471, 400, 0xE2AB9CB2}, {9437185, 400, 0x42609D65}, {9437183, 400, 0x77ED0792},
{8716289, 400, 0xCCA0C17B}, {8716287, 400, 0xD47E0E85}, {8257537, 400, 0x80B5C05F}, {8257535, 400, 0x278AE556},
{7798785, 400, 0x55A2468D}, {7798783, 400, 0xCF62032E}, {7471105, 400, 0x0AE03D3A}, {7471103, 400, 0xD8AB333B},
{7077889, 400, 0xC516359D}, {7077887, 400, 0xA23EA7B3}, {6684673, 400, 0xA7576F00}, {6684671, 400, 0x057E57F4},
{6422529, 400, 0xC779D2C3}, {6422527, 400, 0xA8263D37}, {6225921, 400, 0xB46AEB2F}, {6225919, 400, 0xD0A5FD5F},
{5898241, 400, 0xE46E76F9}, {5898239, 400, 0x29ED63B2}, {5505025, 400, 0x83566CC3}, {5505023, 400, 0x0B9CBE64},
{5242881, 400, 0x3CC408F6}, {5242879, 400, 0x0EA4D112}, {4980737, 400, 0x6A2056EF}, {4980735, 400, 0xE03CC669},
{4718593, 400, 0x87622D6B}, {4718591, 400, 0xF79922E2}, {4587521, 400, 0xE189A38A}, {4587519, 400, 0x930FF36C},
{4358145, 400, 0xDFEBF850}, {4358143, 400, 0xBB63D330}, {4128769, 400, 0xC0844AD1}, {4128767, 400, 0x25BDBFC3},
{3932161, 400, 0x7A525A7E}, {3932159, 400, 0xF30C9045}, {3735553, 400, 0xFAD79E97}, {3735551, 400, 0x005ED15A},
{3538945, 400, 0xDDE5BA46}, {3538943, 400, 0x15ED5982}, {3342337, 400, 0x1A6E87E9}, {3342335, 400, 0xECEEA390},
{3276801, 400, 0x3341C77F}, {3276799, 400, 0xACA2EE28}, {3112961, 400, 0x2BDF9D2B}, {3112959, 400, 0xA0AC8635},
{2949121, 400, 0x36EDB768}, {2949119, 400, 0x53FD5473}, {2785281, 400, 0x66816C94}, {2785279, 400, 0x059E8D6B},
{2654999, 400, 0x07EE900D}, {2654777, 400, 0x9E744AA3}, {2654557, 400, 0x4106F46F}, {2654333, 400, 0xE81BCF40},
{2654227, 400, 0xAC3B7F2B}, {2654003, 400, 0x1ED556E7}, {2620317, 400, 0x09DB64F8}, {2605473, 400, 0xE11637FC},
{2584313, 400, 0x800E9A61}, {2573917, 400, 0x939DA3B3}, {2555671, 400, 0x9DC03527}, {2543123, 400, 0x379CA95D},
{2540831, 400, 0x2CF01FBB}, {2539613, 400, 0x4146EECA}, {2495213, 400, 0x89D80370}, {2444819, 400, 0x56BF33C5},
{2432123, 400, 0x5952676A}, {2408449, 400, 0x437ECC01}, {2408447, 400, 0x8C91E8AA}, {2388831, 400, 0xA508B5A7},
{2359297, 400, 0x73A131F0}, {2332451, 400, 0xAB209667}, {2330097, 400, 0x3FB88055}, {2329999, 400, 0xF5E902A5},
{2329557, 400, 0x2E267692}, {2328527, 400, 0xC55E3E05}, {2328117, 400, 0x0A24673A}, {2293761, 400, 0x9E4BBB8A},
{2293759, 400, 0x1901F07B}, {2288753, 400, 0xA4DD9AAD}, {2266413, 400, 0x675257DB}, {2244765, 400, 0xC08FF487},
{2236671, 400, 0x45EB162A}, {2222517, 400, 0x1A128B22}, {2200339, 400, 0x0EB0E827}, {2193011, 400, 0x382B6E4B},
{2188001, 400, 0xD012AF6A}, {2166567, 400, 0x509BA41A}, {2144651, 400, 0x54CFC0E6}, {2130357, 400, 0xC25804D8},
{2122923, 400, 0xA47068E6}, {2100559, 400, 0xACFAB4E1}, {2088461, 400, 0xEA01E860}, {2066543, 400, 0x847DF0D0},
{2044767, 400, 0x04225888}, {2022823, 400, 0x6EA34B32}, {2009987, 400, 0xE6A59DEF}, {2004817, 400, 0xC2FF3440}, 
{1999999, 400, 0xD645A18F}, {1998973, 400, 0xCBDD74F7}, {1997651, 400, 0x666B0CB1}, {1977987, 400, 0x30D1CD1F},
{1970009, 400, 0x646E0DFA}, {1966081, 400, 0xB88828A1}, {1966079, 400, 0x5BD87C45}, {1955087, 400, 0x5B9426A4},
{1899247, 400, 0x11C76E04}, {1877431, 400, 0xA3299B39}, {1855067, 400, 0x35243683}, {1833457, 400, 0xCF630DC0},
{1811987, 400, 0x7C7022EC}, {1799789, 400, 0xEFEC47B7}, {1777773, 400, 0x0F16E2D6}, {1755321, 400, 0x1AC5D492},
{1733333, 400, 0x5DA0555E}, {1711983, 400, 0xDC19DA8B}, {1699779, 400, 0x2B44914E}, {1679779, 400, 0x51D75E0F},
{1677323, 400, 0x03D3980B}, {1675001, 400, 0x50A94DB7}, {1673881, 400, 0xA51D38E4}, {1672771, 400, 0x5474B6F9},
{1671221, 400, 0x2710DDEA}, {1670551, 400, 0x31FC3838}, {1599549, 400, 0x48EA38B2}, {1577771, 400, 0xCE84D9DC},
{1555947, 400, 0x6797EEF4}, {1533349, 400, 0xD6897409}, {1511861, 400, 0x8A8177AC}, {1499625, 400, 0x56BB6FB3},
{1477941, 400, 0xF3DD8ED3}, {1455931, 400, 0x31A222C7}, {1433069, 400, 0x28F01E1B}, {1411747, 400, 0x680C6E39},
{1399449, 400, 0xB7F01A54}, {1377247, 400, 0xE656F652}, {1355991, 400, 0xB2AA2819}, {1350061, 400, 0x31F9A728},
{1344999, 400, 0x158AA064}, {1344997, 400, 0x1D059D4F}, {1310721, 400, 0x5694A427}, {1310719, 400, 0x258BDDE3}, 
{1288771, 400, 0x7431D9E2}, {1266711, 400, 0xB4BC4E8D}, {1244881, 400, 0x48BC9FF9}, {1222991, 400, 0x3F5FC39E},
{1200881, 400, 0xD5DF4944}, {1188441, 400, 0xD9D8968B}, {1166661, 400, 0xD4AB97F4}, {1144221, 400, 0x9940943B},
{1122001, 400, 0x647406B8}, {1100881, 400, 0x3AD40CE0}, {1088511, 400, 0xD578BB51}, {1066837, 400, 0x2F82BFBB},
{1044811, 400, 0x7C6EDDD1}, {1022991, 400, 0x6A1C2DD4}, {1000001, 400, 0x2879748F}, {983041, 1000, 0x29216C43},
{974849, 1000, 0x79791EDB}, {942079, 1000, 0xE528A9B0}, {933889, 1000, 0x62490F57}, {917503, 1000, 0x5F244685},
{901121, 1000, 0x26C4E660}, {884735, 1000, 0x7BC7A661}, {860161, 1000, 0x41185F27}, {851967, 1000, 0x331AA906},
{835585, 1000, 0x706437D3}, {819199, 1000, 0x48AFB0A5}, {802817, 1000, 0xA9645693}, {786431, 1000, 0x61080929},
{778241, 1000, 0x1729A6C4}, {753663, 1000, 0x99C43F31}, {745473, 1000, 0x7BCE80AA}, {737279, 1000, 0x1B07A764}, 
{720897, 1000, 0x1E96863D}, {688127, 1000, 0xD01E5A85}, {659457, 1000, 0x589C16A4}, {655359, 1000, 0x1107F161},
{638977, 1000, 0xC88F34B4}, {630783, 1000, 0x4DD2E603}, {622593, 1000, 0x26F6FC8C}, {614399, 1000, 0x6408F880},
{602113, 1000, 0xEFCD5BA8}, {589823, 1000, 0x0290B60B}, {573441, 1000, 0xF8F039AA}, {565247, 1000, 0xF4CA3679}, 
{557057, 1000, 0xC30FE589}, {540673, 1000, 0xDA0E0D99}, {532479, 1000, 0x0072FE03}, {524289, 1000, 0xCD591388},
{516095, 1000, 0xA6BD9423}, {487423, 1000, 0x933FFF17}, {471041, 1000, 0x18752F40}, {466943, 1000, 0xE10EE929},
{458753, 1000, 0xE296CC00}, {450559, 1000, 0x025B1320}, {442369, 1000, 0xC22471C3}, {425985, 1000, 0xB2095F04},
{417791, 1000, 0x080C803C}, {389119, 1000, 0x6188E38D}, {376833, 1000, 0x915D5458}, {372735, 1000, 0xCB6D00A2},
{368641, 1000, 0x72414364}, {360447, 1000, 0x84AC5CFD}, {344065, 1000, 0x4D0C5089}, {327681, 1000, 0x3101106A},
{319487, 1000, 0x24ED1BE9}, {294913, 1000, 0xF55727C6}, {286719, 1000, 0x1A31A686}, {282625, 1000, 0x34A4E699},
{278527, 1000, 0x05530FD5}, {272385, 1000, 0xEF6EC4B4}, {270335, 1000, 0x14AD54BE}, {266241, 1000, 0xE0969CED},
{262143, 1000, 0xA9638844}, {258049, 1000, 0x73F541AD}, {243713, 1000, 0xC19AEA91}, {245759, 1000, 0x7538BF0B},
{233473, 1000, 0x1964F897}, {235519, 1000, 0x661BBC3F}, {229375, 1000, 0x6E42B26A}, {225281, 1000, 0x20876BED},
{221183, 1000, 0xA89E7764}, {215041, 1000, 0x04D5D2F0}, {212991, 1000, 0x1FF00E2A}, {208897, 1000, 0x9D4DCF0E},
{204799, 1000, 0xD20C2126}, {200705, 1000, 0xFAD28F5A}, {196607, 1000, 0x3190D5F5}, {194561, 1000, 0x6ED1CB70},
{188415, 1000, 0x86FCE4D6}, {186369, 1000, 0x0F001482}, {184319, 1000, 0x360EF08E}, {180225, 1000, 0x35D49D74},
{172031, 1000, 0xC5AF29DB}, {164865, 1000, 0x4942B002}, {163839, 1000, 0x1CA6048E}, {159745, 1000, 0xB5CD03A1},
{157695, 1000, 0x5422FACF}, {155649, 1000, 0xA7B736AF}, {153599, 1000, 0xA9FAAE69}, {150529, 1000, 0x7412F09C},
{147455, 1000, 0x176B299A}, {143361, 1000, 0x580F4DC4}, {141311, 1000, 0x7A5D0730}, {139265, 1000, 0xBCE1FC80},
{136191, 1000, 0xC4591D37}, {135169, 1000, 0x65AC10A4}, {133119, 1000, 0xDE51C35A}, {131073, 1000, 0xC5AAB12F},
{129023, 1000, 0xAFE1E7A8}, {122881, 1000, 0x652827AC}, {121855, 1000, 0x55AF2385}, {117761, 1000, 0x054A26F6},
{116735, 1000, 0xB85F0E8E}, {114689, 1000, 0x0BBAF161}, {112639, 1000, 0x6FA4DB24}, {110593, 1000, 0x1A56AA86},
{107519, 1000, 0x80F177D8}, {106497, 1000, 0xF833D925}, {104447, 1000, 0xF9F3CDFD}, {102401, 1000, 0x19F967B4},
{100351, 1000, 0x1DE12CE6}, {96607, 10000, 0xFC88D4F0}, {94561, 10000, 0xBD7FAFD2}, {94319, 10000, 0x1B306C54},
{83839, 10000, 0x528128D0}, {82031, 10000, 0xDCEA44BA}, {79745, 10000, 0xEC4F1AF4}, {77455, 10000, 0x56774C7A},
{75649, 10000, 0xF1498DE6}, {73361, 10000, 0x058CB75D}, {71311, 10000, 0xF85DC1F2}, {66241, 10000, 0x877A4E1E},
{65281, 10000, 0x75108380}
};

#define MAX_SELF_TEST_ITERS2	403
const struct self_test_info SELF_TEST_DATA2[MAX_SELF_TEST_ITERS2] = {
{560000001, 100, 0x7F853A0A}, {420000001, 150, 0x89665E7E}, {280000001, 200, 0xC32CAD46}, {210000001, 300, 0x89823329},
{140000001, 400, 0x15EF4F24}, {110000001, 500, 0x893C9000}, {77497473, 900, 0xF0B43F54}, {76497471, 900, 0xF30AFA95},
{75497473, 900, 0x32D8D3A7}, {75497471, 900, 0x9E689331}, {74497473, 900, 0xD43166A4}, {73497471, 900, 0x639E4F0C},
{72303169, 900, 0x74BDED5C}, {71303169, 900, 0xA2147B5C}, {71303167, 900, 0x717525AB}, {70303167, 900, 0xD716B4F0},
{68060289, 1000, 0xF90C7BFF}, {67060287, 1000, 0xFE9BF47C}, {66060289, 1000, 0x057C60F5}, {66060287, 1000, 0x2ECC97CE},
{65390273, 1000, 0xC55C6369}, {64390271, 1000, 0x48552448}, {63390273, 1000, 0x6FF8CD84}, {62390273, 1000, 0x42ACEB15},
{62390271, 1000, 0x48764DF8}, {61390271, 1000, 0xD5408698}, {57623105, 1200, 0x098B4491}, {56623105, 1200, 0x5E720717},
{56623103, 1200, 0x1980D8BC}, {55623103, 1200, 0xEDD592B6}, {53477377, 1200, 0xBAEF5CCC}, {53477375, 1200, 0x2F296FC8},
{52331647, 1200, 0xA1EAE85D}, {51331649, 1200, 0xE3B39845}, {50331649, 1200, 0x53543DF2}, {50331647, 1200, 0x0049E54B},
{48185921, 1500, 0x78F4AEAA}, {47185921, 1500, 0x4D7FFDDC}, {47185919, 1500, 0x059D196F}, {46185919, 1500, 0x38B1D9AD},
{45943041, 1500, 0x7670FDDF}, {44943039, 1500, 0xA859BBD7}, {43943041, 1500, 0xD673E000}, {42943039, 1500, 0x6B69D8CE},
{41943041, 1500, 0x6E92CE47}, {41943039, 1500, 0x888BEE79}, {39151585, 1900, 0x3B06496C}, {38748737, 1900, 0x6429E0FD},
{38251583, 1900, 0x04AD7F99}, {37748737, 1900, 0x47659BC5}, {37748735, 1900, 0x2DFA41B0}, {36748735, 1900, 0x1A1DA557},
{36251585, 1900, 0x83F23FA8}, {35651585, 1900, 0x3598B4B9}, {35651583, 1900, 0x7E443962}, {35251583, 1900, 0x1CE4D084},
{34230145, 2100, 0x0FDE9717}, {33730143, 2100, 0x54EB5333}, {33030145, 2100, 0xF37897B8}, {33030143, 2100, 0x52B3981B},
{32595137, 2100, 0xA76D0805}, {32095135, 2100, 0xCF443ACD}, {31595137, 2100, 0xA6DEA70A}, {31195137, 2100, 0x0777442D},
{31195135, 2100, 0x9B265F8F}, {30695135, 2100, 0xA3BC760F}, {29311553, 2500, 0xFD1D6D74}, {28811551, 2500, 0xE720BFD3},
{28311553, 2500, 0xA11F75AB}, {28311551, 2500, 0x7E0471E5}, {27738689, 2500, 0xD246DC55}, {27238687, 2500, 0x806A3A62},
{26738689, 2500, 0x8E8450B1}, {26738687, 2500, 0xD4A0DBC9}, {26138689, 2500, 0x47C47755}, {25638687, 2500, 0x7E9C7E8E},
{24903681, 3100, 0x50835AB8}, {24903679, 3100, 0xAE3D2F94}, {24092961, 3100, 0x7B540B4D}, {23892959, 3100, 0xA0D4EC50},
{23592961, 3100, 0x47FBD6FE}, {23592959, 3100, 0x09FD89AB}, {22971521, 3100, 0x99DFEDB9}, {21871519, 3100, 0x35A8B46A},
{20971521, 3100, 0x94C12572}, {20971519, 3100, 0x1F6D3003}, {19922945, 4000, 0x86B106EB}, {19922943, 4000, 0xE1CE3C1A},
{19374367, 4000, 0xD1045A66}, {19174369, 4000, 0x3247CE82}, {18874369, 4000, 0x33BB2689}, {18874367, 4000, 0x6856F21F},
{18474367, 4000, 0x95E2F6FA}, {18274367, 4000, 0x61182009}, {18274369, 4000, 0xB2FD8175}, {18074369, 4000, 0x7F242A6E},
{17432577, 4500, 0x632CAD0B}, {17432575, 4500, 0xC9C79F07}, {17115073, 4500, 0xF2B70D4B}, {16815071, 4500, 0x71B22529},
{16515073, 4500, 0xAB1CC854}, {16515071, 4500, 0xF54D05D7}, {16297569, 4500, 0x6B5F72DA}, {15997567, 4500, 0x9669F188},
{15597569, 4500, 0x352BFCCF}, {15597567, 4500, 0x36B164ED}, {14942209, 5300, 0xEA5DB53B}, {14942207, 5300, 0x6CC650A2},
{14155777, 5300, 0xEB7C125D}, {14155775, 5300, 0xB4C8B09B}, {13969343, 5300, 0x832359A5}, {13669345, 5300, 0x7EE99140},
{13369345, 5300, 0xCDF43471}, {13369343, 5300, 0x343FEA12}, {13069345, 5300, 0x65B17A9B}, {12969343, 5300, 0x063F492B},
{12451841, 6500, 0xCB168E5D}, {12451839, 6500, 0xE91EEB5A}, {12196481, 6500, 0x0A261B7E}, {11796481, 6500, 0x38100A5F},
{11796479, 6500, 0x78FCF8C5}, {11596479, 6500, 0x8C481635}, {11285761, 6500, 0x2580BC8D}, {10885759, 6500, 0x54030992},
{10485761, 6500, 0x054660AA}, {10485759, 6500, 0x50F74AF0}, {9961473, 7800, 0x7991161C}, {9961471, 7800, 0x627F3BEE},
{9837183, 7800, 0xBC67A608}, {9737185, 7800, 0x9A0CBC59}, {9537183, 7800, 0xA6A509A6}, {9437185, 7800, 0x877C09B6},
{9437183, 7800, 0x1D259540}, {9337185, 7800, 0x5EF3F14C}, {9237183, 7800, 0x5780245F}, {9137185, 7800, 0x6C1162A9},
{8716289, 9000, 0x2011133F}, {8716287, 9000, 0xEEEC1181}, {8516289, 9000, 0xF1D93A69}, {8316287, 9000, 0x53D6E3CB},
{8257537, 9000, 0x38DB98D6}, {8257535, 9000, 0x7D1BECA7}, {8098785, 9000, 0x51E9FA27}, {7998783, 9000, 0xF7F14FF2},
{7798785, 9000, 0x8437BC4D}, {7798783, 9000, 0x9E28D8E1}, {7471105, 11000, 0xEFDA89EA}, {7471103, 11000, 0x4061C4BF},
{7377889, 11000, 0x65ABE846}, {7277887, 11000, 0x02B0EBD7}, {7077889, 11000, 0x336E1030}, {7077887, 11000, 0x685B792E},
{6984673, 11000, 0x3AE19FAF}, {6884671, 11000, 0x2A0ED16A}, {6684673, 11000, 0x206A3512}, {6684671, 11000, 0x4FD9980A},
{6225921, 13000, 0x1A922371}, {6225919, 13000, 0xC0F63BD8}, {6198241, 13000, 0xDA664501}, {6098239, 13000, 0xB92015CD},
{5898241, 13000, 0xDA384BD9}, {5898239, 13000, 0x20B59AC8}, {5705025, 13000, 0x941A2DA0}, {5605023, 13000, 0xCFDF5835},
{5505025, 13000, 0x37A6C972}, {5505023, 13000, 0x6252AB5C}, {5120737, 17000, 0x512705D0}, {5030735, 17000, 0x633E3E74},
{4980737, 17000, 0xD8245D49}, {4980735, 17000, 0xFB2C3530}, {4888593, 17000, 0xE3C6EDBC}, {4818591, 17000, 0x89E7FE48},
{4718593, 17000, 0xA23C713D}, {4718591, 17000, 0xC7BA41D6}, {4698593, 17000, 0xA0194103}, {4648591, 17000, 0xD5A50A23},
{4501145, 19000, 0x7BAF4344}, {4458143, 19000, 0x686F6B13}, {4358145, 19000, 0x682E6643}, {4358143, 19000, 0x974DA6CC},
{4298769, 19000, 0x1FC0E577}, {4228767, 19000, 0x46B5F3CD}, {4128769, 19000, 0x59332478}, {4128767, 19000, 0x4AF5C8B8},
{4028769, 19000, 0x542C17CB}, {3978767, 19000, 0x76E41351}, {3835553, 22000, 0x9058FE40}, {3785551, 22000, 0x45EF5C15},
{3735553, 22000, 0x2700B350}, {3735551, 22000, 0x09EDCEAD}, {3688945, 22000, 0x626C29D3}, {3618943, 22000, 0x82B1D4D1},
{3538945, 22000, 0x70331CC6}, {3538943, 22000, 0x00FEB746}, {3342337, 22000, 0x7CEE24AE}, {3342335, 22000, 0x1802D072},
{3242961, 27000, 0xE877F863}, {3172959, 27000, 0x04C9F1F7}, {3112961, 27000, 0x241E93DB}, {3112959, 27000, 0x8D359307},
{2949121, 27000, 0x6B545E09}, {2949119, 27000, 0xAFD6F417}, {2885281, 27000, 0x439E57E6}, {2785281, 27000, 0xB4E40DFE},
{2785279, 27000, 0x3787D3FA}, {2685279, 27000, 0x902967B7}, {2605473, 34000, 0xE21C344E}, {2584313, 34000, 0xFDBCFCB2},
{2573917, 34000, 0x89B5012C}, {2540831, 34000, 0x201BAA90}, {2539613, 34000, 0x2226BA6B}, {2495213, 34000, 0xE3577D9F},
{2408447, 34000, 0x594C9155}, {2388831, 34000, 0x55CE9F16}, {2359297, 34000, 0x09A72A40}, {2359295, 34000, 0x621E8BF9},
{2244765, 39000, 0xEC2F362D}, {2236671, 39000, 0x4B50CA20}, {2222517, 39000, 0x8DA427C0}, {2193011, 39000, 0xD1DE8993},
{2130357, 39000, 0x4B5EBB90}, {2122923, 39000, 0x5F9110FC}, {2100559, 39000, 0xE0CF8904}, {2088461, 39000, 0x26AD1DEA},
{2066543, 39000, 0xB78C9237}, {2004817, 39000, 0x3D7838F8}, {1933071, 46000, 0x86323D21}, {1911957, 46000, 0x500CFEAD},
{1899247, 46000, 0x128667DF}, {1877431, 46000, 0x2A59B6B5}, {1855067, 46000, 0xBE9AABF5}, {1833457, 46000, 0xB84D7929},
{1777773, 46000, 0x771E0A9D}, {1755321, 46000, 0xF93334E3}, {1699779, 46000, 0x07B46DEE}, {1677323, 46000, 0x910E0320},
{1633941, 56000, 0x455509CD}, {1611557, 56000, 0x0F51FA1E}, {1599549, 56000, 0x646A96B0}, {1577771, 56000, 0xA4A21303},
{1555947, 56000, 0x80B84725}, {1533349, 56000, 0x23E9F7B1}, {1477941, 56000, 0x593F208F}, {1455931, 56000, 0x11002C52},
{1433069, 56000, 0x5B641D8B}, {1411747, 56000, 0x5EAE18A8}, {1322851, 75000, 0xD5C50F2E}, {1310721, 75000, 0x855E44A2},
{1310719, 75000, 0xC0836C1F}, {1300993, 75000, 0xF62263D6}, {1288771, 75000, 0x867EBBAB}, {1266711, 75000, 0xBA1FF3BE},
{1244881, 75000, 0xCE8199EB}, {1222991, 75000, 0xCDE49EF5}, {1200881, 75000, 0xC8610F6C}, {1188441, 75000, 0xFC772495},
{1150221, 84000, 0xA3334541}, {1144221, 84000, 0x44307B03}, {1122001, 84000, 0x9B937DCF}, {1108511, 84000, 0x9F3D191E},
{1100881, 84000, 0xBAF4EA2D}, {1096837, 84000, 0xAA9396F1}, {1088511, 84000, 0xB0CB2704}, {1066837, 84000, 0x031F202C},
{1044811, 84000, 0x7EA89CFE}, {1022991, 84000, 0xD42294C8}, {983041, 100000, 0x4052BBC0}, {974849, 100000, 0xB0E9EB07},
{942079, 100000, 0xEE230987}, {933889, 100000, 0x58FA63B0}, {917503, 100000, 0x8B457209}, {901121, 100000, 0xD2325FC4},
{884735, 100000, 0xCBB5A603}, {860161, 100000, 0xBC240C77}, {854735, 100000, 0xE8BE766D}, {851967, 100000, 0x09AD9B74},
{827279, 120000, 0x64B01894}, {819199, 120000, 0xF97F1E2B}, {802817, 120000, 0xC4EDBC3C}, {795473, 120000, 0x046584E0},
{786431, 120000, 0xC6BA553D}, {778241, 120000, 0x856A5147}, {753663, 120000, 0xC7895B4A}, {745473, 120000, 0x42B47EA2},
{737279, 120000, 0x29E477B8}, {720897, 120000, 0x97111FA7}, {662593, 160000, 0x32472A99}, {659457, 160000, 0xEF49D340},
{655359, 160000, 0x75C12C38}, {644399, 160000, 0xDE632783}, {638977, 160000, 0xDCDB98B4}, {630783, 160000, 0x6B8F0706},
{622593, 160000, 0xD732286D}, {614399, 160000, 0x2489EFB3}, {612113, 160000, 0xCAE00EC6}, {602113, 160000, 0x792AD67D},
{580673, 180000, 0xC508CAFA}, {573441, 180000, 0xB0680C2B}, {565247, 180000, 0xF1DBB762}, {557057, 180000, 0x374F647B},
{544767, 180000, 0x3DC41F49}, {540673, 180000, 0x949A4CB7}, {532479, 180000, 0xEA06DC97}, {524289, 180000, 0xA76CE14A},
{522479, 180000, 0xAA8EAC14}, {516095, 180000, 0x04F0CC23}, {501041, 210000, 0xD9F72F62}, {496943, 210000, 0xD62D5380},
{487423, 210000, 0x55ACB2FD}, {471041, 210000, 0xB6AEAB0E}, {466943, 210000, 0x251CDE78}, {458753, 210000, 0xDC40CADB},
{450559, 210000, 0x2AD0CF72}, {442369, 210000, 0x5FF2E46E}, {441041, 210000, 0x1194CC23}, {436943, 210000, 0x0272AF35},
{420217, 270000, 0xD233852A}, {409601, 270000, 0x6F89825C}, {401407, 270000, 0x3D9DE818}, {393217, 270000, 0xDE8E6FF0},
{392119, 270000, 0x30CA58B7}, {389119, 270000, 0x80975797}, {376833, 270000, 0xC75824DB}, {372735, 270000, 0xF8BE0932},
{368641, 270000, 0xA48AC5E3}, {360447, 270000, 0x7DD29C13}, {339487, 340000, 0xA7311A6D}, {335393, 340000, 0xD9704DF2},
{331681, 340000, 0x3316A003}, {329727, 340000, 0xE46D5991}, {327681, 340000, 0xBEDA4A7B}, {319487, 340000, 0xB25C84FF},
{315393, 340000, 0xF5AD1DDA}, {311295, 340000, 0xFE41A12A}, {308295, 340000, 0x03AAC47E}, {307201, 340000, 0xFC08ACCC},
{291913, 380000, 0xC56AB884}, {286719, 380000, 0x248EF622}, {282625, 380000, 0x50A98488}, {280335, 380000, 0x9B64A843},
{278527, 380000, 0x39D5B7DB}, {274335, 380000, 0x48623B41}, {270335, 380000, 0xC04B857A}, {266241, 380000, 0xFE4475F6},
{262143, 380000, 0xADC3ECE9}, {260335, 380000, 0x15B8F9EF}, {250519, 460000, 0xA2FE3B50}, {245759, 460000, 0xC6D800D6},
{245281, 460000, 0x4F23AA34}, {243713, 460000, 0xB30EC823}, {235519, 460000, 0x31FD709E}, {233473, 460000, 0x8FCC69C2},
{231183, 460000, 0xD59255CC}, {229375, 460000, 0x788520D0}, {225281, 460000, 0xD669C8BC}, {221183, 460000, 0x9B915F4B},
{212991, 560000, 0x0555250D}, {210415, 560000, 0x3FC3CCD7}, {208897, 560000, 0x9FF8F462}, {204799, 560000, 0x294EB549},
{200705, 560000, 0x80B1222F}, {196607, 560000, 0x8AB8D945}, {194561, 560000, 0x4140E623}, {188415, 560000, 0xFA0A3453},
{186369, 560000, 0xAC17EAB6}, {184319, 560000, 0x835F341B}, {172031, 800000, 0xF6BD0728}, {163839, 800000, 0x26C78657},
{159745, 800000, 0x6ACBB961}, {157695, 800000, 0x3EA979F3}, {155649, 800000, 0x09C7ADE4}, {153599, 800000, 0xF601EB92},
{147455, 800000, 0x0AA97D21}, {143361, 800000, 0xEA6A01F1}, {141311, 800000, 0x9BB8A6A3}, {135169, 800000, 0xECA55A45},
{133119, 800000, 0x2BF9C8EB}, {131073, 800000, 0x7468DFA2}, {129023, 920000, 0x7B8900B5}, {122881, 920000, 0x3DE6AE55},
{121855, 920000, 0x146C8E62}, {117761, 920000, 0xD0E39716}, {116735, 920000, 0x67FDC6D0}, {114689, 920000, 0x77254063},
{112639, 920000, 0x0872C067}, {110593, 920000, 0x3EAFFEE3}, {107519, 1120000, 0xDD5F3AED}, {106497, 1120000, 0x2F182F25},
{104447, 1120000, 0xC5885E9D}, {102401, 1120000, 0xAC7AEC15}, {100351, 1120000, 0xEE0C6D03}, {96607, 1120000, 0xA9D88469},
{94561, 1120000, 0xCC083FA7}, {94319, 1120000, 0x6769F679}, {83839, 1600000, 0x08EAB191}, {82031, 1600000, 0x26CFC344},
{79745, 1600000, 0xCED1C919}, {77455, 1600000, 0x2552E499}, {75649, 1600000, 0x06D05124}, {73361, 1600000, 0x6DD17477},
{71311, 1600000, 0x919E2A20}, {66241, 1600000, 0x296D46E6}, {65281, 1600000, 0x6232240E}
};

#define MAX_SELF_TEST_ITERS3	488
const struct self_test_info SELF_TEST_DATA3[MAX_SELF_TEST_ITERS3] = {
{900000001, 100, 0xCF8ADC6F}, {800000001, 100, 0x5171F784}, {700000001, 200, 0x52DE4DB3}, {600000001, 200, 0x1E01473F},
{560000001, 400, 0x5D2075F2}, {420000001, 600, 0x76973D8D}, {280000001, 800, 0xA4B0C213}, {210000001, 1200, 0x9B0FEEA5},
{140000001, 1600, 0xEC8F25E6}, {110000001, 2000, 0xD7EE8401}, {77497473, 3600, 0xEE1F9603}, {76497471, 3600, 0xABE435B0},
{75497473, 3600, 0x36285106}, {75497471, 3600, 0xE8CC66CA}, {74497473, 3600, 0x24B8A2BF}, {73497471, 3600, 0xC12E28E9},
{72303169, 3600, 0x51A924BC}, {71303169, 3600, 0x8FB537CB}, {71303167, 3600, 0xB71873A1}, {70303167, 3600, 0x92EFC50B},
{68060289, 4000, 0xA2629086}, {67060287, 4000, 0x23347B16}, {66060289, 4000, 0xDA787057}, {66060287, 4000, 0x0810958A},
{65390273, 4000, 0xAD06FF26}, {64390271, 4000, 0xE3A7F5DB}, {63390273, 4000, 0x874392AC}, {62390273, 4000, 0xB4718A58},
{62390271, 4000, 0x80C10B5F}, {61390271, 4000, 0xCAD8F47A}, {57623105, 4800, 0x1C2BA27E}, {56623105, 4800, 0xBA735E8B},
{56623103, 4800, 0x13519FDB}, {55623103, 4800, 0xE787C20E}, {53477377, 4800, 0xB35788F2}, {53477375, 4800, 0x03E36F38},
{52331647, 4800, 0xDC9F1FA1}, {51331649, 4800, 0x82533823}, {50331649, 4800, 0x97F22401}, {50331647, 4800, 0x5A2FDCC0},
{48185921, 6000, 0x966A35F6}, {47185921, 6000, 0xD8378EF6}, {47185919, 6000, 0xD04DD7C3}, {46185919, 6000, 0x3BA8288B},
{45943041, 6000, 0xFF87BC35}, {44943039, 6000, 0x726253F8}, {43943041, 6000, 0x8E343AC4}, {42943039, 6000, 0xADF105FF},
{41943041, 6000, 0xE0C8040C}, {41943039, 6000, 0x5EF2E3E9}, {39151585, 7600, 0x294D16AC}, {38748737, 7600, 0xBA261FA4},
{38251583, 7600, 0xA64744BA}, {37748737, 7600, 0xCEA0A996}, {37748735, 7600, 0x71246EC6}, {36748735, 7600, 0xDF0D4C96},
{36251585, 7600, 0x6941330C}, {35651585, 7600, 0x9454919C}, {35651583, 7600, 0xE953A8B3}, {35251583, 7600, 0x95E45098},
{34230145, 8400, 0x0FF2D27E}, {33730143, 8400, 0xA815C3CD}, {33030145, 8400, 0x2968002F}, {33030143, 8400, 0x4AFDF43B},
{32595137, 8400, 0x979CF919}, {32095135, 8400, 0x7C0E8693}, {31595137, 8400, 0x6FD95140}, {31195137, 8400, 0xAA6AD58C},
{31195135, 8400, 0x65EE1BF7}, {30695135, 8400, 0x9D10BC3A}, {29311553, 10000, 0xB8C54183}, {28811551, 10000, 0xC70F9D7E},
{28311553, 10000, 0xEA018EED}, {28311551, 10000, 0x43E2096F}, {27738689, 10000, 0x0EA59538}, {27238687, 10000, 0xC53169EE},
{26738689, 10000, 0x9F98CF04}, {26738687, 10000, 0x733122D3}, {26138689, 10000, 0xD88162ED}, {25638687, 10000, 0x6ADB6B49},
{24903681, 12400, 0xEF9EC005}, {24903679, 12400, 0xAB56E004}, {24092961, 12400, 0x3518F8DD}, {23892959, 12400, 0xE0AEFA13},
{23592961, 12400, 0xD1EC53D7}, {23592959, 12400, 0xB006AE40}, {22971521, 12400, 0xA8964CC4}, {21871519, 12400, 0x4DDF7551},
{20971521, 12400, 0x8927FFB4}, {20971519, 12400, 0x7B3217C2}, {19922945, 16000, 0x069C3DCD}, {19922943, 16000, 0xBED8A46E},
{19374367, 16000, 0x11A21885}, {19174369, 16000, 0x2BB5AEAD}, {18874369, 16000, 0xF47D9EC1}, {18874367, 16000, 0xC342E089},
{18474367, 16000, 0x8AC5B7C8}, {18274367, 16000, 0x4DB0F691}, {18274369, 16000, 0x9886B1C9}, {18074369, 16000, 0x241D5A65},
{17432577, 18000, 0xE7FEF929}, {17432575, 18000, 0xE0673389}, {17115073, 18000, 0xCA8909F8}, {16815071, 18000, 0x4C1D976F},
{16515073, 18000, 0xE86FAE0C}, {16515071, 18000, 0x37F5DF1E}, {16297569, 18000, 0x82A0AF96}, {15997567, 18000, 0x321905E4},
{15597569, 18000, 0x2790951D}, {15597567, 18000, 0xFD88F93B}, {14942209, 21000, 0xE9467E64}, {14942207, 21000, 0x781D4424},
{14155777, 21000, 0xBA64B1E8}, {14155775, 21000, 0xF88B7AAE}, {13969343, 21000, 0xD091E8C3}, {13669345, 21000, 0xE57FED05},
{13369345, 21000, 0xCEEEA179}, {13369343, 21000, 0xBB87F46F}, {13069345, 21000, 0x47222D3F}, {12969343, 21000, 0x477EEFE4},
{12451841, 26000, 0x9A1DC942}, {12451839, 26000, 0x8FEFE60F}, {12196481, 26000, 0x1AD3B450}, {11796481, 26000, 0x6A42C88D},
{11796479, 26000, 0x1A3C83A4}, {11596479, 26000, 0x69D18B9B}, {11285761, 26000, 0x6980EFB6}, {10885759, 26000, 0x223C49A6},
{10485761, 26000, 0xBD0AFF34}, {10485759, 26000, 0xD4216A83}, {9961473, 31000, 0x25DE6210}, {9961471, 31000, 0x2FE72634},
{9837183, 31000, 0x44128AF8}, {9737185, 31000, 0x84C70161}, {9537183, 31000, 0x017BE747}, {9437185, 31000, 0x3D38D6E4},
{9437183, 31000, 0xCF2C58C4}, {9337185, 31000, 0x13BFB2D4}, {9237183, 31000, 0xBBC6391C}, {9137185, 31000, 0x23AF0A31},
{8716289, 36000, 0x10FB9FB7}, {8716287, 36000, 0xDF905C4F}, {8516289, 36000, 0xCB8D21BD}, {8316287, 36000, 0xC61BC2BA},
{8257537, 36000, 0x2F93BEA5}, {8257535, 36000, 0xA9B6681A}, {8098785, 36000, 0x7CEFE90D}, {7998783, 36000, 0x32CA4DC8},
{7798785, 36000, 0xB2669EFF}, {7798783, 36000, 0xF2D393AC}, {7471105, 44000, 0x3D4F6CBB}, {7471103, 44000, 0x51F68987},
{7377889, 44000, 0xD3F710E4}, {7277887, 44000, 0xAF76194F}, {7077889, 44000, 0x815E3804}, {7077887, 44000, 0x2C55F47D},
{6984673, 44000, 0xE530552B}, {6884671, 44000, 0x96085903}, {6684673, 44000, 0x5143D5DB}, {6684671, 44000, 0xD153D55E},
{6225921, 52000, 0xF11B3E86}, {6225919, 52000, 0x26AEF35D}, {6198241, 52000, 0x55A1AD52}, {6098239, 52000, 0xEE20AC08},
{5898241, 52000, 0x024AA620}, {5898239, 52000, 0x36EC9FDB}, {5705025, 52000, 0x87610A79}, {5605023, 52000, 0xBA409794},
{5505025, 52000, 0x0D4AD8BF}, {5505023, 52000, 0xAA82E4D6}, {5120737, 68000, 0xF6376191}, {5030735, 68000, 0x6608D7A5},
{4980737, 68000, 0xE0F7D92F}, {4980735, 68000, 0x8EFD0C10}, {4888593, 68000, 0xF25A28E9}, {4818591, 68000, 0x5EF8173F},
{4718593, 68000, 0x8A2349A7}, {4718591, 68000, 0xD6782279}, {4698593, 68000, 0xE695F8C3}, {4648591, 68000, 0xEEAC3CB7},
{4501145, 76000, 0x9EA735B3}, {4458143, 76000, 0x5D196BB0}, {4358145, 76000, 0x69BA2CCC}, {4358143, 76000, 0x9C4DD97B},
{4298769, 76000, 0xAA0A48AD}, {4228767, 76000, 0xD3AF13A3}, {4128769, 76000, 0xCC2E5548}, {4128767, 76000, 0xB2F51617},
{4028769, 76000, 0x6186CC09}, {3978767, 76000, 0x40CC887E}, {3835553, 88000, 0xE3CA2ED9}, {3785551, 88000, 0x1BD285F6},
{3735553, 88000, 0xAF0621FF}, {3735551, 88000, 0xBC97EF83}, {3688945, 88000, 0xBE99894A}, {3618943, 88000, 0x9D3E55C1},
{3538945, 88000, 0x9757CD7F}, {3538943, 88000, 0xB3AA0A96}, {3342337, 88000, 0xE78AC3D0}, {3342335, 88000, 0x6127F902},
{3242961, 110000, 0x0722ADC3}, {3172959, 110000, 0xA4F278FB}, {3112961, 110000, 0x98E79B6B}, {3112959, 110000, 0x3EC57BE5},
{2949121, 110000, 0x7E5BA333}, {2949119, 110000, 0xE6D8CF29}, {2885281, 110000, 0x4F575F34}, {2785281, 110000, 0x73483675},
{2785279, 110000, 0x95FDDD37}, {2685279, 110000, 0x018291EF}, {2605473, 140000, 0xB0C85136}, {2584313, 140000, 0x90790AD6},
{2573917, 140000, 0x303B334A}, {2540831, 140000, 0x031C1AA0}, {2539613, 140000, 0x79A266C8}, {2495213, 140000, 0x18EE9970},
{2408447, 140000, 0x7B7030D4}, {2388831, 140000, 0x3339B0E9}, {2359297, 140000, 0x4B5D9EF4}, {2359295, 140000, 0xA8FD205D},
{2244765, 160000, 0xE719BC36}, {2236671, 160000, 0x642AE29B}, {2222517, 160000, 0x1E20BD07}, {2193011, 160000, 0x3C64988F},
{2130357, 160000, 0xAA1D86BC}, {2122923, 160000, 0x42499686}, {2100559, 160000, 0x31F3C1EB}, {2088461, 160000, 0xE48241A0},
{2066543, 160000, 0x3BBBFBD6}, {2004817, 160000, 0x5F9B943D}, {1933071, 180000, 0x09344960}, {1911957, 180000, 0x66F5EC79},
{1899247, 180000, 0x6D8B1D9B}, {1877431, 180000, 0x325EB183}, {1855067, 180000, 0xCB9EED7F}, {1833457, 180000, 0x0663527F},
{1777773, 180000, 0xB78DA358}, {1755321, 180000, 0xDE573EE9}, {1699779, 180000, 0x8745CD26}, {1677323, 180000, 0xE138A3E2},
{1633941, 220000, 0x8D116786}, {1611557, 220000, 0x8CD83629}, {1599549, 220000, 0xD950AEE1}, {1577771, 220000, 0xB592C606},
{1555947, 220000, 0xD7C183D6}, {1533349, 220000, 0xBAE10734}, {1477941, 220000, 0x903394EC}, {1455931, 220000, 0x22203D42},
{1433069, 220000, 0x1CB8E61C}, {1411747, 220000, 0x6104BE9F}, {1322851, 300000, 0x20B81597}, {1310721, 300000, 0xE89D646E},
{1310719, 300000, 0x41AE4CA1}, {1300993, 300000, 0xD34E4497}, {1288771, 300000, 0x128E16D1}, {1266711, 300000, 0x840497CE},
{1244881, 300000, 0x8AFB3D24}, {1222991, 300000, 0xDAFAE5FB}, {1200881, 300000, 0x5190783B}, {1188441, 300000, 0xF5FD938D},
{1150221, 330000, 0x7311E3A0}, {1144221, 330000, 0x6A2EB001}, {1122001, 330000, 0x25448CBB}, {1108511, 330000, 0x36C4124A},
{1100881, 330000, 0x957930CB}, {1096837, 330000, 0x39C43852}, {1088511, 330000, 0x79B0E4DB}, {1066837, 330000, 0x4FDDE395},
{1044811, 330000, 0x70108FEE}, {1022991, 330000, 0xACCCA430}, {983041, 400000, 0x205E1EF2}, {974849, 400000, 0x2E8CEF15},
{942079, 400000, 0xCDF36D31}, {933889, 400000, 0x1A75EF3C}, {917503, 400000, 0x91D50B39}, {901121, 400000, 0x5E87DF64},
{884735, 400000, 0xE12C485D}, {860161, 400000, 0x524E6891}, {854735, 400000, 0x8B9BF82E}, {851967, 400000, 0xAF790945},
{827279, 480000, 0xE880E7E1}, {819199, 480000, 0x6A230C26}, {802817, 480000, 0x62EA07D7}, {795473, 480000, 0x0FE31D56},
{786431, 480000, 0xCF4CE6EF}, {778241, 480000, 0x8E467FCA}, {753663, 480000, 0x85D18DAE}, {745473, 480000, 0x06C55332},
{737279, 480000, 0xE19FE986}, {720897, 480000, 0xC83C96AA}, {662593, 640000, 0x42DD71CD}, {659457, 640000, 0x1B973A76},
{655359, 640000, 0x4B3D2077}, {644399, 640000, 0x0C222CE6}, {638977, 640000, 0x3CB3F547}, {630783, 640000, 0x926291B7},
{622593, 640000, 0x4BE31D76}, {614399, 640000, 0x87AD01DB}, {612113, 640000, 0xE29B49BB}, {602113, 640000, 0xE61272B7},
{580673, 720000, 0xC5E9DE8B}, {573441, 720000, 0xDC079BC0}, {565247, 720000, 0xCF1CA37C}, {557057, 720000, 0x9EEF945E},
{544767, 720000, 0xCC75A226}, {540673, 720000, 0x223549D1}, {532479, 720000, 0x40759687}, {524289, 720000, 0xA30037F1},
{522479, 720000, 0xAE25C4CA}, {516095, 720000, 0x2968525A}, {501041, 840000, 0x5D010F00}, {496943, 840000, 0x264D9BA7},
{487423, 840000, 0xE5FE5968}, {471041, 840000, 0x2A4CFB08}, {466943, 840000, 0x7CD3183C}, {458753, 840000, 0x84645EE0},
{450559, 840000, 0xE84CD133}, {442369, 840000, 0x930A5D84}, {441041, 840000, 0x7F778EED}, {436943, 840000, 0x31400F2C},
{420217, 1100000, 0x4D58EEF3}, {409601, 1100000, 0x4938363A}, {401407, 1100000, 0x92B347B5}, {393217, 1100000, 0xF6D354E3},
{392119, 1100000, 0x1D1D9D2E}, {389119, 1100000, 0x4DF62116}, {376833, 1100000, 0x4F526504}, {372735, 1100000, 0x3A3B365A},
{368641, 1100000, 0xBF818C14}, {360447, 1100000, 0xFAEF41BB}, {339487, 1400000, 0x62266123}, {335393, 1400000, 0x9198809B},
{331681, 1400000, 0x093642F5}, {329727, 1400000, 0xE092ED88}, {327681, 1400000, 0xD127F6AF}, {319487, 1400000, 0x7EDD49B9},
{315393, 1400000, 0x2AD1CBBB}, {311295, 1400000, 0xB501E32F}, {308295, 1400000, 0x58F6B52C}, {307201, 1400000, 0x382936EE},
{291913, 1500000, 0x34AC1486}, {286719, 1500000, 0x32151B08}, {282625, 1500000, 0x98F655CC}, {280335, 1500000, 0x47FF5C70},
{278527, 1500000, 0xF74DF4BE}, {274335, 1500000, 0x2322F4FA}, {270335, 1500000, 0xC065C6F4}, {266241, 1500000, 0x120A64F0},
{262143, 1500000, 0x7A473DE6}, {260335, 1500000, 0xB69E9EB9}, {250519, 1800000, 0x763B1556}, {245759, 1800000, 0xFBB67721},
{245281, 1800000, 0xF640633D}, {243713, 1800000, 0xCDC2C7AA}, {235519, 1800000, 0xC4A7AD0F}, {233473, 1800000, 0x39EF35D2},
{231183, 1800000, 0xB8792E3B}, {229375, 1800000, 0xE028677D}, {225281, 1800000, 0xFC11CE76}, {221183, 1800000, 0xACCF7139},
{212991, 2200000, 0x161FB56E}, {210415, 2200000, 0x7B60E81C}, {208897, 2200000, 0x63514A8F}, {204799, 2200000, 0xB1925D4B},
{200705, 2200000, 0x91E5EF6D}, {196607, 2200000, 0x0B2FA06D}, {194561, 2200000, 0x004E1A6D}, {188415, 2200000, 0x7C10EA53},
{186369, 2200000, 0xE723EC59}, {184319, 2200000, 0x1EC9F330}, {172031, 3200000, 0xA8289A03}, {163839, 3200000, 0x9BCEAD72},
{159745, 3200000, 0x4D30796D}, {157695, 3200000, 0x2719836B}, {155649, 3200000, 0x7C4B1002}, {153599, 3200000, 0x10F2B05E},
{147455, 3200000, 0x3BD06944}, {143361, 3200000, 0xA5C7C148}, {141311, 3200000, 0x71A19953}, {138527, 4000000, 0xD2E65D57},
{136241, 4000000, 0x99EE467C}, {135169, 4000000, 0x1115D06F}, {134335, 4000000, 0x32AA5A36}, {132143, 4000000, 0x392D9060},
{130335, 4000000, 0x689E07C6}, {130331, 4000000, 0xA791824A}, {125759, 5000000, 0x68E60664}, {125281, 5000000, 0x99421692},
{123713, 5000000, 0x883AC578}, {120519, 5000000, 0x6915D35E}, {119375, 5000000, 0x5930769A}, {115519, 5000000, 0x4092717B},
{115281, 5000000, 0x9C03F336}, {113473, 5000000, 0x15571C02}, {111183, 5000000, 0xAE6E91FF}, {111181, 5000000, 0x0E250D4F},
{108897, 6000000, 0xBEE96B52}, {104799, 6000000, 0xFF4FDA4D}, {102991, 6000000, 0x272CB267}, {100705, 6000000, 0xC0D285CF},
{100415, 6000000, 0x8FC75796}, {98415, 6000000, 0x55F0423B}, {96607, 6000000, 0xF60BA9EB}, {96369, 6000000, 0x5015EFE2},
{94561, 6000000, 0x27F5F9D8}, {94319, 6000000, 0x22FEEB22}, {83839, 7000000, 0xF3816D12}, {82031, 7000000, 0xD102B9B5},
{79745, 7000000, 0xDA483FC0}, {77695, 7000000, 0x62E51145}, {77455, 7000000, 0x9AEBD3EA}, {75649, 7000000, 0x64961C9D},
{73599, 7000000, 0x16415370}, {73361, 7000000, 0x87ED3BB9}, {71311, 7000000, 0x8F9E1C81}, {68527, 7000000, 0x44F5B375},
{66241, 8000000, 0x58E92942}, {65759, 8000000, 0x45F7CAD9}, {65281, 8000000, 0x71D13735}, {65169, 8000000, 0x9291C45D},
{64335, 8000000, 0x179EEB42}, {63713, 8000000, 0xC4D70CD3}, {62143, 8000000, 0x4EADFEDC}, {60519, 9000000, 0x59187C4E},
{60337, 8000000, 0x06B2F274}, {60335, 8000000, 0xF4A5C109}, {59375, 9000000, 0xECACBD29}, {58897, 9000000, 0x2D49F445},
{55519, 9000000, 0xCC57A689}, {55281, 9000000, 0x370811FF}, {54799, 11000000, 0xDF09CED1}, {53473, 11000000, 0x7527A443},
{52991, 11000000, 0x3F3E6D08}, {51183, 11000000, 0x63B7BC14}, {51181, 11000000, 0xCF23C3FE}, {50705, 11000000, 0xD7755CCA},
{50415, 11000000, 0xF13B5703}, {48415, 11000000, 0x18720A74}, {46607, 11000000, 0xEFAD69EB}, {46369, 11000000, 0x36576FF2},
{44561, 11000000, 0xBBFF519A}, {44319, 11000000, 0x67D8C7C8}, {43839, 14000000, 0xAB4287EC}, {42031, 14000000, 0x07E5F336},
{39745, 14000000, 0xB1F4CDA4}, {37695, 14000000, 0x5E68A976}, {37455, 14000000, 0x160940DF}, {35649, 14000000, 0x82C7BF50},
{35169, 14000000, 0xA9AA21D4}, {34527, 14000000, 0x746D4F98}, {33599, 14000000, 0xEEC3F4A4}, {33361, 14000000, 0x8060C92F},
{33241, 16000000, 0x6B01C7DF}, {32759, 16000000, 0x114A953B}, {32335, 16000000, 0x3E6C186B}, {32281, 16000000, 0x19CCFAF9},
{31713, 16000000, 0x8AFEC931}, {31311, 16000000, 0x7BF36CD8}, {31143, 16000000, 0x311C5B29}, {30519, 16000000, 0xD6403088},
{30335, 16000000, 0xCC66B636}, {30331, 16000000, 0x06615CC4}, {29897, 18000000, 0x376F4617}, {29375, 18000000, 0x2F7EEC43},
{27799, 18000000, 0xB887EB2B}, {27519, 18000000, 0x3E2FC829}, {27281, 18000000, 0x61D01456}, {26991, 18000000, 0x5500B456},
{26473, 22000000, 0x18B413C8}, {25705, 22000000, 0xE3A124A8}, {25415, 22000000, 0x5BAB711C}, {25183, 22000000, 0x4D02C76D},
{25181, 22000000, 0x54240561}, {24415, 22000000, 0x76E207FC}, {23607, 22000000, 0xE0ED69CD}, {23369, 22000000, 0x7B6B1955},
{22561, 22000000, 0xF68FF31A}, {22319, 22000000, 0x65736E87}, {21839, 28000000, 0x04ECD2F5}, {21031, 28000000, 0xBD7BB022},
{19745, 28000000, 0xC0E93F7C}, {18695, 28000000, 0x0E424A5F}, {18455, 28000000, 0xC0C87A73}, {17649, 28000000, 0xECB5F4E2},
{16599, 28000000, 0x2614F881}, {16361, 28000000, 0x9575CC6F}, {15311, 28000000, 0xDB715E07}, {15169, 28000000, 0xB7945996}
};

int selfTestInternal (
	int	thread_num,
	struct PriorityInfo *sp_info,
	unsigned long fftlen,
	int	num_threads,		
	int	is_small,		
	unsigned int test_time,		
	int	*torture_index,		
	unsigned int memory,		
	void	*bigbuf,		
	const struct self_test_info *test_data, 
	unsigned int test_data_count,
	int	disabled_cpu_flags,	
	int	*completed,		
	int	*errors,		
	int	*warnings)		
{
	int	num_hyperthreads;
	llhandle lldata;
	unsigned long k, limit;
	unsigned int i, iter;
	char	buf[256], addl_msg[80];

	time_t	start_time, current_time;
	int	alternate_in_place, in_place, stop_reason;

	title (thread_num, "Torture Test");
	addl_msg[0] = 0;
	num_hyperthreads = get_ranked_num_threads (sp_info->torture_core_num, 1, sp_info->torture_hyperthreading);
	if (num_hyperthreads > 1 && num_threads == 1) sprintf (addl_msg, " (thread %d of %d)", sp_info->torture_thread_num, num_hyperthreads);

	alternate_in_place = IniGetInt (INI_FILE, "TortureAlternateInPlace", 1);

	limit = gwmap_with_cpu_flags_fftlen_to_max_exponent (CPU_FLAGS & ~disabled_cpu_flags, fftlen);

	time (&start_time);

	i = (torture_index == NULL) ? 0 : *torture_index;

	for (iter = 1; ; iter++) {
		char	fft_desc[200];
		unsigned long p, reshi, reslo;
		unsigned int ll_iters, num_gwnums;
		gwnum	*gwarray, g;

		for ( ; ; i++) {

			if (i == test_data_count) i = 0;

			p = test_data[i].p;
			if (p > limit) continue;

			if (p / fftlen < 10) continue;

			break;
		}

		if (CPU_ARCHITECTURE == CPU_ARCHITECTURE_AMD_BULLDOZER) {
			CPU_ARCHITECTURE = CPU_ARCHITECTURE_AMD_ZEN;
			gwinit (&lldata.gwdata);
			CPU_ARCHITECTURE = CPU_ARCHITECTURE_AMD_BULLDOZER;
		} else
			gwinit (&lldata.gwdata);
		lldata.gwdata.cpu_flags &= ~disabled_cpu_flags;
		gwclear_use_benchmarks (&lldata.gwdata);
		gwset_num_threads (&lldata.gwdata, num_threads);
		gwset_thread_callback (&lldata.gwdata, SetAuxThreadPriority);
		gwset_thread_callback_data (&lldata.gwdata, sp_info);
		lldata.gwdata.GW_BIGBUF = (char *) bigbuf;
		lldata.gwdata.GW_BIGBUF_SIZE = (bigbuf != NULL) ? (intptr_t) memory * (intptr_t) 1048576 : 0;
		stop_reason = lucasSetup (thread_num, p, fftlen, &lldata);
		if (stop_reason) return (stop_reason);

		ll_iters = test_data[i].iters;
		in_place = (is_small || memory <= 8 || (alternate_in_place && (iter & 1) == 0));
		if (in_place)
			num_gwnums = 1;
		else {
			num_gwnums = (int) cvt_mem_to_gwnums (&lldata.gwdata, memory);
			if (num_gwnums < 1) num_gwnums = 1;
			if (num_gwnums > ll_iters) num_gwnums = ll_iters;
		}

		gwfft_description (&lldata.gwdata, fft_desc);
		sprintf (buf, SELF1, iter, addl_msg, ll_iters, in_place ? "in-place " : "", p, fft_desc);
		OutputStr (thread_num, buf);

		gwarray = (gwnum *) malloc (num_gwnums * sizeof (gwnum));
		if (gwarray == NULL) {
			lucasDone (&lldata);
			return (OutOfMemory (thread_num));
		}
		gwarray[0] = lldata.lldata;
		for (k = 1; k < num_gwnums; k++) {
			gwarray[k] = gwalloc (&lldata.gwdata);
			if (gwarray[k] == NULL) {
				num_gwnums = k;
				break;
			}
		}

restart_test:	dbltogw (&lldata.gwdata, 4.0, lldata.lldata);
		g = lldata.lldata;

		gwerror_checking (&lldata.gwdata, 1);
		gwsetaddin (&lldata.gwdata, -2);
		for (k = 0; k < ll_iters; k++) {
			gwnum	prev;

			prev = g;
			if (num_gwnums > 1) g = gwarray[k % num_gwnums];

			gwsquare2 (&lldata.gwdata, prev, g, GWMUL_STARTNEXTFFT_IF(k != ll_iters - 1) | GWMUL_ADDINCONST);

			if (gw_test_illegal_sumout (&lldata.gwdata)) {
				sprintf (buf, SELFFAIL7, thread_num+1);
				OutputStr (MAIN_THREAD_NUM, buf);
				OutputBoth (thread_num, SELFFAIL1);
				flashWindowAndBeep ();
				(*warnings)++;
				if (*warnings < 100) {
					OutputBoth (thread_num, SELFFAIL4);
					goto restart_test;
				} else {
					OutputBoth (thread_num, SELFFAIL6);
					lucasDone (&lldata);
					free (gwarray);
					return (STOP_FATAL_ERROR);
				}
			}

			if (gw_get_maxerr (&lldata.gwdata) > 0.45) {
				sprintf (buf, SELFFAIL7, thread_num+1);
				OutputStr (MAIN_THREAD_NUM, buf);
				sprintf (buf, SELFFAIL3, gw_get_maxerr (&lldata.gwdata));
				OutputBoth (thread_num, buf);
				sprintf (buf, SELFFAIL5, (fftlen % 1024 == 0) ? fftlen >> 10 : fftlen, (fftlen % 1024 == 0) ? "K" : "");
				OutputBoth (thread_num, buf);
				flashWindowAndBeep ();
				(*errors)++;
				lucasDone (&lldata);
				free (gwarray);
				return (STOP_FATAL_ERROR);
			}

			stop_reason = stopCheck (thread_num);
			if (stop_reason) {
				lucasDone (&lldata);
				free (gwarray);
				return (stop_reason);
			}
		}

		for (k = 1; k < num_gwnums; k++) {
			if (gwarray[k] != g) gwfree (&lldata.gwdata, gwarray[k]);
		}

		gwswap (g, lldata.lldata);
		generateResidue64 (&lldata, p, &reshi, &reslo);
		gwswap (g, lldata.lldata);

		lucasDone (&lldata);
		free (gwarray);
		(*completed)++;
		if (reshi != test_data[i].reshi) {
			sprintf (buf, SELFFAIL7, thread_num+1);
			OutputStr (MAIN_THREAD_NUM, buf);
			sprintf (buf, SELFFAIL, reshi, test_data[i].reshi);
			OutputBoth (thread_num, buf);
			sprintf (buf, SELFFAIL5, (fftlen % 1024 == 0) ? fftlen >> 10 : fftlen, (fftlen % 1024 == 0) ? "K" : "");
			OutputBoth (thread_num, buf);
			flashWindowAndBeep ();
			(*errors)++;
			return (STOP_FATAL_ERROR);
		}

		i++;

		if (test_time == 1) break;
		time (&current_time);
		if ((unsigned int) (current_time - start_time) + 30 >= test_time * 60) break;
	}

	if (torture_index != NULL) *torture_index = i;

	if (fftlen % 1024 == 0) sprintf (buf, SELFPASS, (int) (fftlen/1024), "K", addl_msg);
	else sprintf (buf, SELFPASS, (int) fftlen, "", addl_msg);
	OutputBoth (thread_num, buf);

	return (0);
}

struct torture_info_arg {
	gwthread thread_id;
	
	int	thread_num;		
	struct PriorityInfo sp_info;
	unsigned long fftlen;
	int	num_threads;		
	int	is_small;		
	unsigned int test_time;		
	int	*torture_index;		
	unsigned int memory;		
	void	*bigbuf;		
	const struct self_test_info *test_data; 
	unsigned int test_data_count;
	int	disabled_cpu_flags;	
	
	int	tests_completed;
	int	self_test_errors;
	int	self_test_warnings;
	int	stop_reason;
};

void tortureOneHyperthread (void *arg)
{
	struct torture_info_arg *info;

	info = (struct torture_info_arg *) arg;
	info->tests_completed = 0;
	info->self_test_errors = 0;
	info->self_test_warnings = 0;

	SetPriority (&info->sp_info);
	info->stop_reason = selfTestInternal (info->thread_num, &info->sp_info, info->fftlen, info->num_threads, info->is_small, info->test_time,
					      info->torture_index, info->memory, info->bigbuf, info->test_data, info->test_data_count, info->disabled_cpu_flags,
					      &info->tests_completed, &info->self_test_errors, &info->self_test_warnings);
}

#ifdef ONE_HOUR_SELF_TEST

static const char SELFMSG1A[] = "The program will now perform a self-test to make sure the\n";
static const char SELFMSG1B[] = "Lucas-Lehmer code is working properly on your computer.\n";
static const char SELFMSG1C[] = "This will take about an hour.\n";

int selfTest (
	int	thread_num,
	struct PriorityInfo *sp_info,
	struct work_unit *w)
{
	unsigned long fftlen;
	char	iniName[32];
	int	tests_completed, self_test_errors, self_test_warnings;

	if (w->minimum_fftlen)
		fftlen = w->minimum_fftlen;
	else
		fftlen = gwmap_to_fftlen (1.0, 2, w->n, -1);

	if (fftlen < 65536) return (0);

	sprintf (iniName, SelfTestIniMask, (int) (fftlen/1024));
	if (IniGetInt (INI_FILE, iniName, 0)) return (0);
#ifdef SERVER_TESTING
	return (0);
#endif

	OutputStr (thread_num, SELFMSG1A);
	OutputStr (thread_num, SELFMSG1B);
	OutputStr (thread_num, SELFMSG1C);

	tests_completed = 0;
	self_test_errors = 0;
	self_test_warnings = 0;
	return (selfTestInternal (thread_num, sp_info, fftlen, 1, 60, NULL, 0, NULL, 0, &tests_completed, &self_test_errors, &self_test_warnings));
}
#endif

void tortureTestDefaultSizes (
	int	torture_type,		
	int	num_threads,		
	int	*minfft,		
	int	*maxfft)		
{
	int	min_adjusted_L2_cache_size, min_adjusted_L3_cache_size, min_adjusted_L4_cache_size;
	int	max_adjusted_L2_cache_size, max_adjusted_L3_cache_size, max_adjusted_L4_cache_size;

	min_adjusted_L2_cache_size = max_adjusted_L2_cache_size = 0;
	min_adjusted_L3_cache_size = max_adjusted_L3_cache_size = 0;
	min_adjusted_L4_cache_size = max_adjusted_L4_cache_size = 0;

	if (CPU_NUM_L2_CACHES) {
		int	cores_per_L2_cache, min_workers_per_L2_cache, max_workers_per_L2_cache;

		cores_per_L2_cache = HW_NUM_CORES / CPU_NUM_L2_CACHES;

		min_workers_per_L2_cache = divide_rounding_up (num_threads, CPU_NUM_L2_CACHES);
		max_workers_per_L2_cache = num_threads / HW_NUM_CORES * cores_per_L2_cache + _intmin (num_threads % HW_NUM_CORES, cores_per_L2_cache);

		max_adjusted_L2_cache_size = CPU_TOTAL_L2_CACHE_SIZE / CPU_NUM_L2_CACHES / min_workers_per_L2_cache;
		min_adjusted_L2_cache_size = CPU_TOTAL_L2_CACHE_SIZE / CPU_NUM_L2_CACHES / max_workers_per_L2_cache;

		if (CPU_L2_CACHE_INCLUSIVE == 0 && CPU_NUM_L1_CACHES) {
			max_adjusted_L2_cache_size += CPU_TOTAL_L1_CACHE_SIZE / CPU_NUM_L1_CACHES;
			min_adjusted_L2_cache_size += CPU_TOTAL_L1_CACHE_SIZE / CPU_NUM_L1_CACHES;
		}
	}

	if (CPU_NUM_L3_CACHES) {
		int	cores_per_L3_cache, min_workers_per_L3_cache, max_workers_per_L3_cache;

		cores_per_L3_cache = HW_NUM_CORES / CPU_NUM_L3_CACHES;
		min_workers_per_L3_cache = divide_rounding_up (num_threads, CPU_NUM_L3_CACHES);
		max_workers_per_L3_cache = num_threads / HW_NUM_CORES * cores_per_L3_cache + _intmin (num_threads % HW_NUM_CORES, cores_per_L3_cache);

		max_adjusted_L3_cache_size = CPU_TOTAL_L3_CACHE_SIZE / CPU_NUM_L3_CACHES / min_workers_per_L3_cache;
		min_adjusted_L3_cache_size = CPU_TOTAL_L3_CACHE_SIZE / CPU_NUM_L3_CACHES / max_workers_per_L3_cache;

		if (CPU_L3_CACHE_INCLUSIVE == 0) {
			max_adjusted_L3_cache_size += max_adjusted_L2_cache_size;
			min_adjusted_L3_cache_size += min_adjusted_L2_cache_size;
		}
	}

	if (CPU_NUM_L4_CACHES) {
		int	cores_per_L4_cache, min_workers_per_L4_cache, max_workers_per_L4_cache;

		cores_per_L4_cache = HW_NUM_CORES / CPU_NUM_L4_CACHES;
		min_workers_per_L4_cache = divide_rounding_up (num_threads, CPU_NUM_L4_CACHES);
		max_workers_per_L4_cache = num_threads / HW_NUM_CORES * cores_per_L4_cache + _intmin (num_threads % HW_NUM_CORES, cores_per_L4_cache);

		max_adjusted_L4_cache_size = CPU_TOTAL_L4_CACHE_SIZE / CPU_NUM_L4_CACHES / min_workers_per_L4_cache;
		min_adjusted_L4_cache_size = CPU_TOTAL_L4_CACHE_SIZE / CPU_NUM_L4_CACHES / max_workers_per_L4_cache;

		if (CPU_L4_CACHE_INCLUSIVE == 0) {
			max_adjusted_L4_cache_size += max_adjusted_L3_cache_size;
			min_adjusted_L4_cache_size += min_adjusted_L3_cache_size;
		}
	}

	if (torture_type == 0) {		
		*minfft = 4;
		*maxfft = min_adjusted_L2_cache_size / 12;
	}
	if (torture_type == 1) {		
		*minfft = max_adjusted_L2_cache_size / 7;
		*maxfft = min_adjusted_L3_cache_size / 12;
	}
	if (torture_type == 2) {		
		*minfft = max_adjusted_L3_cache_size / 7;
		*maxfft = min_adjusted_L4_cache_size / 12;
	}
	if (torture_type == 3) {		
		*minfft = (max_adjusted_L4_cache_size ? max_adjusted_L4_cache_size :
			   max_adjusted_L3_cache_size ? max_adjusted_L3_cache_size : max_adjusted_L2_cache_size) / 7;
		*maxfft = (CPU_TOTAL_L4_CACHE_SIZE ? 32768 : 8192);
	}
	if (torture_type == 4) {		
		*minfft = 4;
		*maxfft = (CPU_TOTAL_L4_CACHE_SIZE ? 32768 : 8192);
	}
}

int tortureTest (
	int	thread_num,
	int	num_torture_workers)
{
	struct PriorityInfo sp_info;
	int	num_threads;		
	const struct self_test_info *test_data; 
	unsigned int test_data_count;
	int	num_lengths;		
	int	num_large_lengths;	
	int	current_small_index, current_large_index; 
	unsigned long lengths[500];	
	int	data_index[500];	
	int	test_time, disabled_cpu_flags;
	int	tests_completed, self_test_errors, self_test_warnings;
	int	i, run_indefinitely, stop_reason;
	unsigned long fftlen, min_fft, max_fft, max_small_fftlen;
	time_t	start_time;
	unsigned int memory;		
	void	*bigbuf = NULL;

	memset (&sp_info, 0, sizeof (sp_info));
	sp_info.type = SET_PRIORITY_TORTURE;
	sp_info.worker_num = thread_num;
	sp_info.torture_core_num = thread_num;
	sp_info.torture_hyperthreading = IniGetInt (INI_FILE, "TortureHyperthreading", 1);
	sp_info.verbosity = IniGetInt (INI_FILE, "AffinityVerbosityTorture", 0);
	SetPriority (&sp_info);

	num_threads = get_ranked_num_threads (sp_info.torture_core_num, 1, sp_info.torture_hyperthreading);

	tests_completed = 0;
	self_test_errors = 0;
	self_test_warnings = 0;

	if (CPU_FLAGS & CPU_AVX) {
		test_data = SELF_TEST_DATA3;
		test_data_count = MAX_SELF_TEST_ITERS3;
	} else if (CPU_SPEED >= 1000.0) {
		test_data = SELF_TEST_DATA2;
		test_data_count = MAX_SELF_TEST_ITERS2;
	} else {
		test_data = SELF_TEST_DATA;
		test_data_count = MAX_SELF_TEST_ITERS;
	}

	if (CPU_TOTAL_L4_CACHE_SIZE) max_small_fftlen = (CPU_TOTAL_L4_CACHE_SIZE / 12 / num_torture_workers) << 10;
	else if (CPU_TOTAL_L3_CACHE_SIZE) max_small_fftlen = (CPU_TOTAL_L3_CACHE_SIZE / 12 / num_torture_workers) << 10;
	else if (CPU_TOTAL_L2_CACHE_SIZE) max_small_fftlen = (CPU_TOTAL_L2_CACHE_SIZE / 12 / num_torture_workers) << 10;
	else if (CPU_TOTAL_L1_CACHE_SIZE) max_small_fftlen = (CPU_TOTAL_L1_CACHE_SIZE / 12 / num_torture_workers) << 10;
	else max_small_fftlen = 8 << 10;

loop:	run_indefinitely = TRUE;

	OutputStr (thread_num, TORTURE1);
	OutputStr (thread_num, TORTURE2);

	min_fft = IniGetInt (INI_FILE, "MinTortureFFT", 4) * 1024;
	if (min_fft < 32) min_fft = 32;
	max_fft = IniGetInt (INI_FILE, "MaxTortureFFT", 4096) * 1024;
	memory = IniGetInt (INI_FILE, "TortureMem", 8);
	memory = memory / num_torture_workers;
	while (memory > 8 && bigbuf == NULL) {
		bigbuf = aligned_malloc ((intptr_t) memory * (intptr_t) 1048576, 128);
		if (bigbuf == NULL) memory--;
	}

	num_lengths = 0;
	num_large_lengths = 0;
	disabled_cpu_flags = IniGetInt (INI_FILE, "TortureWeak", 0);
	fftlen = gwmap_with_cpu_flags_to_fftlen (CPU_FLAGS & ~disabled_cpu_flags, 1.0, 2, 15 * min_fft, -1);
	while (fftlen && fftlen <= max_fft) {
		unsigned long max_exponent = gwmap_with_cpu_flags_fftlen_to_max_exponent (CPU_FLAGS & ~disabled_cpu_flags, fftlen);
		if (max_exponent == 0) break;			
		if (fftlen >= min_fft && max_exponent > test_data[test_data_count-1].p) {
			lengths[num_lengths] = fftlen;
			data_index[num_lengths++] = 0;
			if (fftlen > max_small_fftlen * 2) num_large_lengths++;
		}
		fftlen = gwmap_with_cpu_flags_to_fftlen (CPU_FLAGS & ~disabled_cpu_flags, 1.0, 2, max_exponent + 100, -1);
	}

	if (num_lengths == 0) {
		OutputStr (thread_num, "No FFT lengths available in the range specified.\n");
		return (0);
	}

	time (&start_time);
	current_small_index = 0;
	current_large_index = num_lengths - num_large_lengths;
	test_time = IniGetInt (INI_FILE, "TortureTime", 15);
	for (i = 0; ; i++) {
		int	index;

		index = i * num_large_lengths % num_lengths;
		if (index < num_large_lengths) {
			index = current_large_index++;
			if (current_large_index == num_lengths) current_large_index = 0;
		} else {
			index = current_small_index++;
			if (current_small_index == num_lengths) current_small_index = 0;
		}

		if (sp_info.torture_hyperthreading &&
		    (!((CPU_FLAGS & ~disabled_cpu_flags) & (CPU_SSE2 | CPU_AVX | CPU_AVX512F)) ||
		     lengths[index] < 262144 ||
		     !IniGetInt (INI_FILE, "TortureMultiThreadedFFTs", 1))) {
			struct torture_info_arg *info = (struct torture_info_arg *) malloc (num_threads * sizeof (struct torture_info_arg));
			for (int i = 0; i < num_threads; i++) {
				info[i].thread_num = thread_num;
				info[i].sp_info = sp_info;
				info[i].sp_info.torture_thread_num = i + 1;
				info[i].fftlen = lengths[index];
				info[i].num_threads = 1;
				info[i].is_small = lengths[index] <= max_small_fftlen;
				info[i].test_time = test_time;
				info[i].torture_index = &data_index[index];
				info[i].memory = memory / num_threads;
				info[i].bigbuf = (bigbuf != NULL ? (char *) bigbuf + i * (intptr_t) info[i].memory * (intptr_t) 1048576 : NULL);
				info[i].test_data = test_data;
				info[i].test_data_count = test_data_count;
				info[i].disabled_cpu_flags = disabled_cpu_flags;
				gwthread_create_waitable (&info[i].thread_id, &tortureOneHyperthread, (void *) &info[i]);
			}

			stop_reason = 0;
			for (i = 0; i < num_threads; i++) {
				gwthread_wait_for_exit (&info[i].thread_id);
				tests_completed += info[i].tests_completed;
				self_test_errors += info[i].self_test_errors;
				self_test_warnings += info[i].self_test_warnings;
				if (info[i].stop_reason) stop_reason = info[i].stop_reason;
			}
			free (info);
		}

		else {
			stop_reason = selfTestInternal (thread_num, &sp_info, lengths[index], num_threads, lengths[index] <= max_small_fftlen, test_time,
							&data_index[index], memory, bigbuf, test_data, test_data_count, disabled_cpu_flags,
							&tests_completed, &self_test_errors, &self_test_warnings);
		}
		if (stop_reason) break;
	}

	{
		char	buf[200];
		time_t	current_time;
		int	hours, minutes;

		time (&current_time);
		minutes = (int) (current_time - start_time) / 60;
		hours = minutes / 60;
		minutes = minutes % 60;
		sprintf (buf, "Torture Test completed %d tests in ", tests_completed);
		if (hours > 1) sprintf (buf+strlen(buf), "%d hours, ", hours);
		else if (hours == 1) strcat (buf, "1 hour, ");
		sprintf (buf+strlen(buf), "%d minutes - %d errors, %d warnings.\n", minutes, self_test_errors, self_test_warnings);
		OutputStr (thread_num, buf);
	}

	aligned_free (bigbuf);
	bigbuf = NULL;

	while (stop_reason == STOP_WORKER) {
		implement_stop_one_worker (thread_num);
		stop_reason = stopCheck (thread_num);
		if (stop_reason == 0) goto loop;
	}

	return (stop_reason);
}

int lucas_QA (
	int	thread_num,
	int	type)
{
	llhandle lldata;
	FILE	*fd;
	int	stop_reason;

	title (thread_num, "QA");
	srand ((unsigned) time (NULL));

	fd = fopen ("qa", "r");
	if (fd == NULL) {
		OutputStr (thread_num, "File named 'qa' could not be opened.\n");
		return (STOP_FILE_IO_ERROR);
	}

	for ( ; ; ) {
		unsigned long k, b, p, fftlen, iters;
		int	c;
		char	buf[500], res[80];
		unsigned long units_bit, i, maxerrcnt;
		double	maxerr, toterr, M, S;
		unsigned long ge_300, ge_325, ge_350, ge_375, ge_400;
		unsigned int iters_unchecked, M_count;

		p = 0;
		if (type != 5) {
			(void) fscanf (fd, "%lu,%lu,%lu,%lu,%s\n", &p, &fftlen, &iters, &units_bit, res);
			k = 1; b = 2;
			c = (fftlen & 1) ? 1 : -1;
			fftlen &= ~1;
		} else
			(void) fscanf (fd, "%lu,%lu,%lu,%d,%lu,%lu,%lu,%s\n", &k, &b, &p, &c, &fftlen, &iters, &units_bit, res);
		if (p == 0) break;

		gwinit (&lldata.gwdata);
		gwset_minimum_fftlen (&lldata.gwdata, fftlen);
		if (gwsetup (&lldata.gwdata, (double) k, b, p, c)) goto not_impl;

		lldata.lldata = gwalloc (&lldata.gwdata);
		lldata.units_bit = units_bit;

		if (lldata.units_bit >= p) {
			uint32_t hi, lo;
			lldata.units_bit = (rand () << 16) + rand ();
			if (CPU_FLAGS & CPU_RDTSC) {
				rdtsc (&hi, &lo);
				lldata.units_bit += lo;
			}
			lldata.units_bit = lldata.units_bit % p;
		}

		if (type >= 4)
			gw_random_number (&lldata.gwdata, lldata.lldata);
		else {
			unsigned long word, bit_in_word;
			bitaddr (&lldata.gwdata, (lldata.units_bit + 2) % p, &word, &bit_in_word);
			for (i = 0; i < gwfftlen (&lldata.gwdata); i++)
				set_fft_value (&lldata.gwdata, lldata.lldata, i, (i == word) ? (1L << bit_in_word) : 0);
		}

		ge_300 = ge_325 = ge_350 = ge_375 = ge_400 = 0;
		maxerr = 0.0; maxerrcnt = 0; toterr = 0.0;
		iters_unchecked = (type > 3) ? 2 : 40;
		M = 0.0;  S = 0.0;  M_count = 0;

		for (i = 0; i < iters; i++) {

			gwsetnormroutine (&lldata.gwdata, 0, 1, 0);
			gwstartnextfft (&lldata.gwdata, i < iters / 2);
			lucas_fixup (&lldata, p);
			gwsquare (&lldata.gwdata, lldata.lldata);

			if (i > iters_unchecked) {
				double	newM;
				toterr += gw_get_maxerr (&lldata.gwdata);
				M_count++;
				newM = M + (gw_get_maxerr (&lldata.gwdata) - M) / M_count;
				S = S + (gw_get_maxerr (&lldata.gwdata) - M) * (gw_get_maxerr (&lldata.gwdata) - newM);
				M = newM;

				if (gw_get_maxerr (&lldata.gwdata) >= 0.300) ge_300++;
				if (gw_get_maxerr (&lldata.gwdata) >= 0.325) ge_325++;
				if (gw_get_maxerr (&lldata.gwdata) >= 0.350) ge_350++;
				if (gw_get_maxerr (&lldata.gwdata) >= 0.375) ge_375++;
				if (gw_get_maxerr (&lldata.gwdata) >= 0.400) ge_400++;

				if (gw_get_maxerr (&lldata.gwdata) > maxerr) maxerr = gw_get_maxerr (&lldata.gwdata), maxerrcnt = 1;
				else if (gw_get_maxerr (&lldata.gwdata) == maxerr) maxerrcnt++;
			}
			gw_clear_maxerr (&lldata.gwdata);

			if (gw_test_illegal_sumout (&lldata.gwdata)) {
				OutputBoth (thread_num, "Warning: ILLEGAL SUMOUT\n");
				dbltogw (&lldata.gwdata, 11.0, lldata.lldata);
				gw_clear_error (&lldata.gwdata);
			}

			if (gw_test_mismatched_sums (&lldata.gwdata)) {
				OutputBoth (thread_num, "Warning: SUMOUT MISMATCH\n");
				gw_clear_error (&lldata.gwdata);
			}

			stop_reason = stopCheck (thread_num);
			if (stop_reason) {
				lucasDone (&lldata);
				fclose (fd);
				return (stop_reason);
			}
		}

		if (type == 0) {
			unsigned long reshi, reslo;
			generateResidue64 (&lldata, p, &reshi, &reslo);
			sprintf (buf, "%08lX%08lX", reshi, reslo);
			if (_stricmp (res, buf)) {
				sprintf (buf, "Warning: Residue mismatch. Was %08lX%08lX, expected %s\n", reshi, reslo, res);
				OutputBoth (thread_num, buf);
			}
		}

		S = sqrt (S / (M_count - 1));
		toterr /= M_count;
		sprintf (buf, "avg: %6.6f, stddev: %6.6f, #stdev to 0.5: %6.6f\n", toterr, S, (0.50 - toterr) / S);
		OutputBoth (thread_num, buf);

		sprintf (buf, "Exp/iters: %lu/%lu, maxerr: %6.6f/%lu, %lu/%lu/%lu/%lu/%lu\n", p, iters, maxerr, maxerrcnt, ge_300, ge_325, ge_350, ge_375, ge_400);
		OutputBoth (thread_num, buf);

		lucasDone (&lldata);
not_impl:	;
	}
	fclose (fd);

	return (0);
}

int primeSieveTest (
	int	thread_num)
{
	fachandle facdata;
	char	buf[500];
	FILE	*fd;
	unsigned long p;
	int	stop_reason;
	uint32_t res, carryl, carryh;

	fd = fopen ("factors", "r");
	if (fd == NULL) {
		OutputBoth (thread_num, "File named 'factors' not found.\n");
		return (0);
	}

	while (fscanf (fd, "%ld", &p) && p) {
		unsigned long fachi, facmid, faclo;
		unsigned long i, pass;
		char fac[480];
		char *f;

		(void) fscanf (fd, "%s", fac);
		fachi = facmid = faclo = 0;
		for (f = fac; *f; f++) {
			if (*f < '0' || *f > '9') continue;
			res = *f - '0';
			carryl = 0;
			muladdhlp (&res, &carryl, &carryh, faclo, 10);
			faclo = res;
			res = carryl;
			carryl = 0;
			muladdhlp (&res, &carryl, &carryh, facmid, 10);
			facmid = res;
			fachi = fachi * 10 + carryl;
			if (fachi >= 268435456 ||
			    (fachi >= 4194304 && !(CPU_FLAGS & CPU_FMA3)) ||
			    (fachi >= 16384 && !(CPU_FLAGS & CPU_SSE2))) {
				sprintf (buf, "%ld %s factor too big.\n", p, fac);
				OutputBoth (thread_num, buf);
				goto nextp;
			}
		}

		if (fachi == 0 && facmid < 0x1000) {
			sprintf (buf, "%ld %s factor too small.\n", p, fac);
			OutputBoth (thread_num, buf);
			goto nextp;
		}

		if (! isPrime (p)) {
			sprintf (buf, "%ld not a prime.\n", p);
			OutputBoth (thread_num, buf);
			goto nextp;
		}

		i = (fachi % 120 * 16 + facmid % 120 * 16 + faclo % 120) % 120;
		if (i == 1) pass = 0;
		else if (i == 7) pass = 1;
		else if (i == 17) pass = 2;
		else if (i == 23) pass = 3;
		else if (i == 31) pass = 4;
		else if (i == 41) pass = 5;
		else if (i == 47) pass = 6;
		else if (i == 49) pass = 7;
		else if (i == 71) pass = 8;
		else if (i == 73) pass = 9;
		else if (i == 79) pass = 10;
		else if (i == 89) pass = 11;
		else if (i == 97) pass = 12;
		else if (i == 103) pass = 13;
		else if (i == 113) pass = 14;
		else if (i == 119) pass = 15;
		else goto bad;
		if (IniGetInt (INI_FILE, "QAMultithreadedTF", 0))
			facdata.num_threads = (facmid & 0x7) + 1;
		else
			facdata.num_threads = 1;
		stop_reason = factorSetup (thread_num, p, &facdata);
		if (stop_reason) {
			fclose (fd);
			return (stop_reason);
		}
		if (fachi >= 16384 && !(facdata.asm_data->cpu_flags & (CPU_SSE2 | CPU_AVX2 | CPU_FMA3 | CPU_AVX512F))) {
			sprintf (buf, "%ld %s factor too big.\n", p, fac);
			OutputBoth (thread_num, buf);
			factorDone (&facdata);
			goto nextp;
		}
		
		{
			double	fltfac;
			fltfac = ((double) fachi * 4294967296.0 + (double) facmid) * 4294967296.0 + (double) faclo;
			facdata.endpt = pow (2.0, ceil (log2 (fltfac)));
			if (facdata.endpt > fltfac * 1.1) facdata.endpt = fltfac * 1.1;
		}
		facdata.asm_data->FACHSW = fachi;
		facdata.asm_data->FACMSW = facmid & ~0xFFF;
		stop_reason = factorPassSetup (thread_num, pass, &facdata);
		if (stop_reason) {
			fclose (fd);
			factorDone (&facdata);
			return (stop_reason);
		}

		do {
			if (factorChunk (&facdata) != 2) {
				if (facdata.asm_data->FACHSW == fachi &&
				    facdata.asm_data->FACMSW == facmid &&
				    facdata.asm_data->FACLSW == faclo) {
					sprintf (buf, "%ld %s factored OK.\n", p, fac);
				} else {
					sprintf (buf, "%ld %s different factor found.\n", p, fac);
				}
				OutputSomewhere (thread_num, buf);
				factorDone (&facdata);
				goto nextp;
			}
#ifdef X86_64
		} while (facdata.total_num_chunks_TFed != facdata.total_num_chunks_to_TF && facdata.total_num_chunks_TFed < 200000);
#else
		} while (facdata.asm_data->FACMSW <= facmid);
#endif

bad:		sprintf (buf, "%ld %s factor not found.\n", p, fac);
		OutputBoth (thread_num, buf);

		factorDone (&facdata);
nextp:		stop_reason = stopCheck (thread_num);
		if (stop_reason) {
			fclose (fd);
			return (stop_reason);
		}
		p = 0;
	}

	fclose (fd);
	return (0);
}

int cpuid_dump (
	int	thread_num)
{
	struct cpuid_data reg;
	unsigned int i, j, max_cpuid_value, max_extended_cpuid_value;
	char	buf[200];
#define dumpreg() {sprintf(buf,"i: %08lX, EAX: %08lX, EBX: %08lX, ECX: %08lX, EDX: %08lX\n",(long)i,(long)reg.EAX,(long)reg.EBX,(long)reg.ECX,(long)reg.EDX); OutputBoth(thread_num,buf);}

	Cpuid (0, &reg);
	max_cpuid_value = reg.EAX;
	Cpuid (0x80000000, &reg);
	max_extended_cpuid_value = reg.EAX;

	for (i = 0; i <= max_cpuid_value; i++) {
		for (j = 0; j < 5; j++) {
			memset (&reg, 0, sizeof (reg));
			reg.ECX = j;
			Cpuid (i, &reg);
			dumpreg ();
			if (i != 4 && i != 7 && i != 11) break;
		}
	}

	for (i = 0x80000000; i <= max_extended_cpuid_value; i++) {
		for (j = 0; j < 5; j++) {
			memset (&reg, 0, sizeof (reg));
			reg.ECX = j;
			Cpuid (i, &reg);
			dumpreg ();
			if (i != 0x8000001D) break;
		}
	}

	Cpuid (1, &reg);
	if ((reg.ECX >> 27) & 0x1) {
		Xgetbv (0, &reg);
		dumpreg ();
		if (IniGetInt (INI_FILE, "AdditionalGetBV", 0)) {
			Xgetbv (IniGetInt (INI_FILE, "AdditionalGetBV", 0), &reg);
			dumpreg ();
		}
	}

	return (0);
}

int primeTime (
	int	thread_num,
	unsigned long p,
	unsigned long iterations)
{
static	int	time_negacyclic = 0;		
	struct PriorityInfo sp_info;
#define SAVED_LIMIT	10
	llhandle lldata;
	unsigned long i, j, saved, save_limit;
	int	min_cores, max_cores, incr_cores, min_hyperthreads, max_hyperthreads;
	int	num_cores, num_hyperthreads;
	char	buf[256], fft_desc[200];
	double	time, saved_times[SAVED_LIMIT];
	int	days, hours, minutes, stop_reason, print_every_iter;
	uint32_t *ASM_TIMERS;
	uint32_t best_asm_timers[32] = {0};
	double	timers[2];

	if (p >= 9900 && p <= 9999) {
		memset (&sp_info, 0, sizeof (sp_info));
		sp_info.type = SET_PRIORITY_QA;
		sp_info.worker_num = thread_num;
		sp_info.normal_work_hyperthreading = FALSE;
		sp_info.verbosity = IniGetInt (INI_FILE, "AffinityVerbosityQA", 0);
		SetPriority (&sp_info);

		if (p >= 9994 && p <= 9999)
			return (lucas_QA (thread_num, 9999 - p));
		if (p == 9993)
			return (pplus1_QA (thread_num, &sp_info));
		if (p == 9992)
			return (pminus1_QA (thread_num, &sp_info));
		if (p == 9991)
			return (ecm_QA (thread_num, &sp_info));
		if (p == 9990)
			return (primeSieveTest (thread_num));
		if (p == 9950)
			return (cpuid_dump (thread_num));
		if (p == 9951) {
			time_negacyclic = !time_negacyclic;
			return (0);
		}
		if (p >= 9900 && p <= 9919)
			return (test_randomly (thread_num, &sp_info));
		return (test_all_impl (thread_num, &sp_info));
	}

	if (p >= 8900 && p <= 8999) {
		char	buf[200];

		int polymem = IniGetInt (INI_FILE, "PolyMem", (long) (0.8 * physical_memory ()));
		if (polymem > (int) physical_memory ()) polymem = (int) (0.9 * physical_memory ());

		sprintf (buf, "Polymult timing using %d threads, %dMB\n", HW_NUM_CORES, polymem);
		OutputBoth (thread_num, buf);

		memset (&sp_info, 0, sizeof (sp_info));
		sp_info.type = SET_PRIORITY_NORMAL_WORK;
		sp_info.worker_num = thread_num;
		sp_info.verbosity = IniGetInt (INI_FILE, "AffinityVerbosity", 1);
		sp_info.normal_work_hyperthreading = FALSE;
		SetPriority (&sp_info);

		for (int EXP = 1000; EXP < 500000000; EXP *= 4) {
			int iters = 100000 / EXP;	
			
			for (int CNT = 3125; CNT < 500000000; CNT *= 2, iters = iters / 2) {
				
				if ((double) CNT * (gwmap_to_estimated_size (1.0, 2, EXP, -1) + 64) > polymem * 1048576.0) break;

				bool	first_poly = TRUE;
				bool	mt_polymult_line;		
				bool	preprocess;			
				bool	preFFT;				
				bool	compress;			
				bool	strided_write;			
				bool	streamed_stores;		
				bool	scrambled;			
				for (int mm = 0; mm < 2; mm++) { mt_polymult_line = mm;		
				for (int sc = 0; sc < 2; sc++) { scrambled = sc;
				for (int sw = 0; sw < 2; sw++) { strided_write = sw;
				for (int ss = 0; ss < 2; ss++) { streamed_stores = ss;
				for (int pp = 0; pp < 5; pp++) {
				preprocess = (pp != 0);	
				preFFT = (pp >= 3);
				compress = (pp == 2) || (pp == 4);
				if (p == 8900 && pp > 0) continue;
				if (p == 8901 && sc > 0) continue;

				gwhandle gwdata;
				pmhandle pmdata;
				gwarray a, b;
				gwinit (&gwdata); 
				gwset_num_threads (&gwdata, HW_NUM_CORES);
				gwset_thread_callback (&gwdata, SetAuxThreadPriority);
				gwset_thread_callback_data (&gwdata, &sp_info);
				gwset_use_spin_wait (&gwdata, 1);
				gwsetup (&gwdata, 1.0, 2, EXP, -1);
				if (stopCheck (thread_num)) { gwdone (&gwdata); return 1; }
				polymult_init (&pmdata, &gwdata);
				polymult_default_tuning (&pmdata,
					IniGetInt (INI_FILE, "PolymultCacheSize", CPU_NUM_L2_CACHES > 0 ? CPU_TOTAL_L2_CACHE_SIZE / CPU_NUM_L2_CACHES : 256),
					IniGetInt (INI_FILE, "PolymultCacheSize2", CPU_NUM_L3_CACHES > 0 ? CPU_TOTAL_L3_CACHE_SIZE / CPU_NUM_L3_CACHES : 6144));
				polymult_set_max_num_threads (&pmdata, HW_NUM_CORES);
				pmdata.mt_ffts_start = mt_polymult_line ? 0xFFFFFFFFFFFFFFFFULL : 0;
				pmdata.mt_ffts_end = 0xFFFFFFFFFFFFFFFFULL;
				pmdata.strided_writes_end = strided_write ? 0xFFFFFFFFFFFFFFFFULL : 0;
				pmdata.streamed_stores_start = streamed_stores ? 0 : 0xFFFFFFFFFFFFFFFFULL;
				gwdata.scramble_arrays = (char) scrambled;

				if ((!preprocess ? 1.0 : !preFFT ? 1.5 : 2.0) * CNT * (gwnum_datasize (&gwdata) + 64) +
				    (double) polymult_mem_required (&pmdata, CNT/2, CNT/2, 0) > polymem * 1048576.0) {
					polymult_done (&pmdata);
					gwdone (&gwdata);
					continue;
				}

				a = b = gwalloc_array (&gwdata, CNT);
				dbltogw (&gwdata, 10001.0, a[0]);
				for (int i = 1; i < CNT/2; i++) gwcopy (&gwdata, a[0], a[i]);
				if (preprocess) a = polymult_preprocess (&pmdata, a, CNT/2, CNT/2, CNT-1, (preFFT ? POLYMULT_PRE_FFT : 0) + (compress ? POLYMULT_PRE_COMPRESS : 0));
				clear_timers (timers, sizeof (timers) / sizeof (timers[0]));
				start_timer (timers, 0);
				if (iters == 0) iters = 1;
				for (int i = 0; i < iters; i++) polymult (&pmdata, a, CNT/2, a, CNT/2, b, (CNT/2)*2-1, POLYMULT_NEXTFFT);
				end_timer (timers, 0);
				timers[0] /= iters;
				if (first_poly) {
					int aligned_gwnum_size = gwnum_datasize (&gwdata) + GW_HEADER_SIZE (&gwdata);
					aligned_gwnum_size = round_up_to_multiple_of (aligned_gwnum_size, 64);
					sprintf (buf, "Poly-size %d, gwnum FFT size %ld, gwnum size %d, mod 256 %d\n", CNT, gwdata.FFTLEN, aligned_gwnum_size, aligned_gwnum_size % 256);
					OutputBoth (thread_num, buf);
					first_poly = FALSE;
				}
				sprintf (buf, "  multithread %s, %s%s%s writes, Time: ",
					 mt_polymult_line ? "lines" : "fft passes",
					 !scrambled ? "" : "scrambled, ",
					 !preprocess ? "" : !preFFT ? (!compress ? "preprocessed, " : "pre-compressed, ") : (!compress ? "pre-FFTed, " : "pre-FFT-compressed, "),
					 !streamed_stores ? (strided_write ? "strided" : "standard") : (strided_write ? "strided streamed" : "standard streamed"));
				print_timer (timers, 0, buf, TIMER_NL | TIMER_CLR | TIMER_MS);
				OutputBoth (thread_num, buf);
				polymult_done (&pmdata);
				gwdone (&gwdata);
				}}}}}
			}
		}
		return (0);
	}

	memset (&sp_info, 0, sizeof (sp_info));
	sp_info.type = SET_PRIORITY_TIME;
	sp_info.worker_num = thread_num;
	sp_info.verbosity = IniGetInt (INI_FILE, "AffinityVerbosityTime", 0);
	sp_info.time_hyperthreading = FALSE;
	SetPriority (&sp_info);

	min_cores = IniGetInt (INI_FILE, "AdvancedTimeMinCores", 1);
	if (min_cores < 1) min_cores = 1;
	if (min_cores > (int) HW_NUM_CORES) min_cores = HW_NUM_CORES;

	max_cores = IniGetInt (INI_FILE, "AdvancedTimeMaxCores", HW_NUM_CORES);
	if (max_cores < 1) max_cores = 1;
	if (max_cores > (int) HW_NUM_CORES) max_cores = HW_NUM_CORES;

	incr_cores = IniGetInt (INI_FILE, "AdvancedTimeCoresIncrement", 1);
	if (incr_cores < 1) incr_cores = 1;

	if (HW_NUM_CORES == HW_NUM_THREADS || !OS_CAN_SET_AFFINITY) {
		min_hyperthreads = 1;
		max_hyperthreads = 1;
	} else {
		min_hyperthreads = IniGetInt (INI_FILE, "AdvancedTimeMinHyperthreads", 1);
		if (min_hyperthreads < 1) min_hyperthreads = 1;
		if (min_hyperthreads > 2) min_hyperthreads = 2;
		max_hyperthreads = IniGetInt (INI_FILE, "AdvancedTimeMaxHyperthreads", 2);
		if (max_hyperthreads < 1) max_hyperthreads = 1;
		if (max_hyperthreads > 2) max_hyperthreads = 2;
	}

	print_every_iter = IniGetInt (INI_FILE, "PrintTimedIterations", 1);

	for (num_hyperthreads = min_hyperthreads; num_hyperthreads <= max_hyperthreads; num_hyperthreads++) {
		sp_info.time_hyperthreading = (num_hyperthreads > 1);
		for (num_cores = min_cores; num_cores <= max_cores; num_cores += incr_cores) {

			clear_timers (timers, sizeof (timers) / sizeof (timers[0]));

			sprintf (buf, "\nTiming using %d core%s,%s hyperthreading\n", num_cores, num_cores > 1 ? "s" : "", num_hyperthreads > 1 ? "" : " no");
			OutputStr (thread_num, buf);

			gwinit (&lldata.gwdata);
			gwset_bench_cores (&lldata.gwdata, HW_NUM_COMPUTE_CORES);	
			gwset_bench_workers (&lldata.gwdata, NUM_WORKERS);	
			if (ERRCHK) gwset_will_error_check (&lldata.gwdata);
			else gwset_will_error_check_near_limit (&lldata.gwdata);
			if (IniGetInt (INI_FILE, "UseLargePages", 0)) gwset_use_large_pages (&lldata.gwdata);
			
			if (IniGetInt (INI_FILE, "TimeSpecificFFTImplementations", 0)) lldata.gwdata.bench_pick_nth_fft = p % 100;
			gwset_num_threads (&lldata.gwdata, get_ranked_num_threads (0, num_cores, num_hyperthreads > 1));
			gwset_thread_callback (&lldata.gwdata, SetAuxThreadPriority);
			gwset_thread_callback_data (&lldata.gwdata, &sp_info);
			gwset_use_spin_wait (&lldata.gwdata, IniGetInt (INI_FILE, "SpinWait", 0));
			stop_reason = lucasSetup (thread_num, p, time_negacyclic, &lldata);
			if (stop_reason) return (stop_reason);
			ASM_TIMERS = get_asm_timers (&lldata.gwdata);
			memset (ASM_TIMERS, 0, 32 * sizeof (uint32_t));

			gwfft_description (&lldata.gwdata, fft_desc);
			sprintf (buf, "Using %s\n", fft_desc);
			OutputStr (thread_num, buf);
			title (thread_num, "Timing");

			generateRandomData (&lldata);

			gwsetnormroutine (&lldata.gwdata, 0, ERRCHK != 0, 0);
			gwstartnextfft (&lldata.gwdata, TRUE);
			gwsquare (&lldata.gwdata, lldata.lldata);

			save_limit = (p <= 4000000) ? SAVED_LIMIT : 1;
			for (i = 0, saved = 0; i < iterations; i++) {

				start_timer (timers, 0);
				gwsquare (&lldata.gwdata, lldata.lldata);
				end_timer (timers, 0);
				timers[1] += timers[0];
				saved_times[saved++] = timers[0];
				timers[0] = 0;

				for (j = 0; j < 32; j++)
					if (i == 0 || ASM_TIMERS[j] < best_asm_timers[j])
						best_asm_timers[j] = ASM_TIMERS[j];

				if (saved == save_limit || i == iterations - 1) {
					if (print_every_iter)
						for (j = 0; j < saved; j++) {
							sprintf (buf, "p: %lu.  Time: ", p);
							timers[0] = saved_times[j];
							print_timer (timers, 0, buf, TIMER_MS | TIMER_NL | TIMER_CLR);
							OutputStr (thread_num, buf);
						}
					saved = 0;
				}

				stop_reason = stopCheck (thread_num);
				if (stop_reason) {
					lucasDone (&lldata);
					return (stop_reason);
				}
			}
			lucasDone (&lldata);
			time = timer_value (timers, 1);

			sprintf (buf, "Iterations: %lu.  Total time: ", iterations);
			print_timer (timers, 1, buf, TIMER_NL | TIMER_CLR);
			OutputStr (thread_num, buf);
			time = time * p / iterations;
			days = (int) (time / 86400.0); time -= (double) days * 86400.0;
			hours = (int) (time / 3600.0); time -= (double) hours * 3600.0;
			minutes = (int) (time / 60.0);
			strcpy (buf, "Estimated time to complete this exponent: ");
			sprintf (buf+strlen(buf), days == 1 ? "%d day, " : "%d days, ", days);
			sprintf (buf+strlen(buf), hours == 1 ? "%d hour, " : "%d hours, ", hours);
			sprintf (buf+strlen(buf), minutes == 1 ? "%d minute.\n" : "%d minutes.\n", minutes);
			OutputStr (thread_num, buf);

			for (i = 0; i < 32; i++) {
				sprintf (buf, "timer %lu: %d\n", i, (int) best_asm_timers[i]);
				if (best_asm_timers[i]) OutputBoth (thread_num, buf);
			}

		}
	}

	return (0);
}

int	last_bench_core_num = 0;

void bench_busy_loop (void *arg)
{
	int	cpu_num;
	struct PriorityInfo sp_info;

	if (! (CPU_FLAGS & CPU_SSE2)) return;

	cpu_num = (int) (intptr_t) arg;
	memset (&sp_info, 0, sizeof (sp_info));
	sp_info.type = SET_PRIORITY_BUSY_LOOP;
	sp_info.worker_num = MAIN_THREAD_NUM;
	sp_info.busy_loop_core = cpu_num;
	SetPriority (&sp_info);

	while (cpu_num > last_bench_core_num) one_hundred_thousand_clocks ();
}

void add_bench_data_to_pkt (
	struct primenetBenchmarkData *pkt,	
	const char *bench_format_mask,		
	unsigned long fftlen,			
	double numeric_value,			
	int   smaller_is_better)		
{
	char	bench_str[20];
	int	i;

	sprintf (bench_str, bench_format_mask, fftlen / 1024);

	for (i = 0; i < (int) pkt->num_data_points; i++) {
		if (strcmp (pkt->data_points[i].bench, bench_str) == 0) {
			if ((smaller_is_better && numeric_value < pkt->data_points[i].timing) ||
			    (!smaller_is_better && numeric_value > pkt->data_points[i].timing))
				pkt->data_points[i].timing = numeric_value;
			return;
		}
	}

	if (pkt->num_data_points < PRIMENET_BENCH_MAX_DATAPOINTS) {
		strcpy (pkt->data_points[i].bench, bench_str);
		pkt->data_points[pkt->num_data_points].timing = numeric_value;
		pkt->num_data_points++;
	}
}

static const char BENCH1[] = "Your timings will be written to the results.bench.txt file.\n";
static const char BENCH2[] = "Compare your results to other computers at http://www.mersenne.org/report_benchmarks\n";

int factorBench (
	int	thread_num)
{
	fachandle facdata;
	unsigned long num_lengths, i, j;
	double	best_time;
	char	buf[512];
	int	bit_lengths[] = {61, 62, 63, 64, 65, 66, 67, 75, 76, 77};
	int	res, stop_reason;
	double	timers[2];

	if (IniGetInt (INI_FILE, "BenchDummyWorkers", 0)) {
		last_bench_core_num = 0;				
		for (i = 1; i < HW_NUM_COMPUTE_CORES; i++) {	
			gwthread thread_id;
			gwthread_create (&thread_id, &bench_busy_loop, (void *) (intptr_t) i);
		}
	}

	num_lengths = sizeof (bit_lengths) / sizeof (int);
	for (i = 0; i < num_lengths; i++) {

		facdata.num_threads = 1;
		stop_reason = factorSetup (thread_num, 35000011, &facdata);
		if (stop_reason) {
			last_bench_core_num = HW_NUM_CORES;
			return (stop_reason);
		}
		if (bit_lengths[i] <= 64) {
			facdata.asm_data->FACHSW = 0;
			facdata.asm_data->FACMSW = 1L << (bit_lengths[i]-33);
		} else {
			facdata.asm_data->FACHSW = 1L << (bit_lengths[i]-65);
			facdata.asm_data->FACMSW = 0;
		}
		facdata.endpt = (facdata.asm_data->FACHSW * 4294967296.0 + facdata.asm_data->FACMSW) * 4294967296.0 * 2.0;
		stop_reason = factorPassSetup (thread_num, 0, &facdata);
		if (stop_reason) {
			last_bench_core_num = HW_NUM_CORES;
			return (stop_reason);
		}

		sprintf (buf, "Timing trial factoring of M35000011 with %d bit length factors.  ", bit_lengths[i]);
		OutputStr (thread_num, buf);

		res = factorChunk (&facdata);
		(void) factorChunksProcessed (&facdata);

		for (j = 0; j < 10; j++) {
			stop_reason = stopCheck (thread_num);
			if (stop_reason) {
				OutputStrNoTimeStamp (thread_num, "\n");
				OutputStr (thread_num, "Execution halted.\n");
				factorDone (&facdata);
				last_bench_core_num = HW_NUM_CORES;
				return (stop_reason);
			}
			clear_timers (timers, sizeof (timers) / sizeof (timers[0]));
			start_timer (timers, 0);
			res = factorChunk (&facdata);
			end_timer (timers, 0);
			if (j == 0 || timers[0] < best_time) best_time = timers[0] / factorChunksProcessed (&facdata);
		}
		factorDone (&facdata);

		best_time = best_time / (FACTOR_CHUNK_SIZE / 16.0);
		timers[0] = best_time;
		strcpy (buf, "Best time: ");
		print_timer (timers, 0, buf, TIMER_NL | TIMER_MS);
		OutputStrNoTimeStamp (thread_num, buf);
		sprintf (buf, "Best time for %d bit trial factors: ", bit_lengths[i]);
		print_timer (timers, 0, buf, TIMER_NL | TIMER_MS);
		writeResultsBench (buf);
	}

	last_bench_core_num = HW_NUM_CORES;
	writeResultsBench ("\n");
	return (0);
}

int	num_bench_workers = 0;
int	num_bench_workers_initialized = 0;
int	bench_worker_finished = 0;
int	bench_workers_time = 0;			
gwmutex	bench_workers_mutex;
gwevent	bench_workers_sync;

struct prime_bench_arg {
	int	main_thread_num;
	unsigned long fftlen;
	int	plus1;
	int	core_num;
	int	core_count;
	bool	hyperthreading;
	int	impl;
	int	iterations;
	int	error_check;
	double	total_time;
};

void primeBenchOneWorker (void *arg)
{
	struct PriorityInfo sp_info;
	llhandle lldata;
	int	stop_reason;
	double	timers[2];
	struct prime_bench_arg *info;

	info = (struct prime_bench_arg *) arg;
	info->iterations = 0;
	info->total_time = 0.0;

	memset (&sp_info, 0, sizeof (sp_info));
	sp_info.type = SET_PRIORITY_BENCHMARKING;
	sp_info.worker_num = info->main_thread_num;
	sp_info.verbosity = IniGetInt (INI_FILE, "AffinityVerbosityBench", 0);
	sp_info.bench_base_core_num = info->core_num;
	sp_info.bench_hyperthreading = info->hyperthreading;
	SetPriority (&sp_info);

	gwinit (&lldata.gwdata);
	gwset_num_threads (&lldata.gwdata, get_ranked_num_threads (info->core_num, info->core_count, info->hyperthreading));
	gwset_thread_callback (&lldata.gwdata, SetAuxThreadPriority);
	gwset_thread_callback_data (&lldata.gwdata, &sp_info);
	lldata.gwdata.bench_pick_nth_fft = info->impl;
	stop_reason = lucasSetup (info->main_thread_num, info->fftlen * 17 + 1, info->fftlen + info->plus1, &lldata);
	if (stop_reason) {
		gwevent_signal (&bench_workers_sync);
		return;
	}

	generateRandomData (&lldata);

	gwmutex_lock (&bench_workers_mutex);
	num_bench_workers_initialized++;
	if (num_bench_workers_initialized == num_bench_workers) {
		if (IniGetInt (INI_FILE, "BenchInitCompleteMessage", 0))
			OutputStr (info->main_thread_num, "Benchmark initialization complete.\n");
		gwevent_signal (&bench_workers_sync);
	}
	gwmutex_unlock (&bench_workers_mutex);
	gwevent_wait (&bench_workers_sync, 0);

	gwsetnormroutine (&lldata.gwdata, 0, info->error_check, 0);
	gwstartnextfft (&lldata.gwdata, TRUE);
	gwsquare (&lldata.gwdata, lldata.lldata);

	for ( ; ; ) {
		stop_reason = stopCheck (info->main_thread_num);
		if (stop_reason) break;
		clear_timers (timers, sizeof (timers) / sizeof (timers[0]));
		start_timer (timers, 0);
		gwsquare (&lldata.gwdata, lldata.lldata);
		end_timer (timers, 0);
		
		if (bench_worker_finished) break;
		
		info->iterations++;
		if (timer_value (timers, 0) <= 0.0) info->total_time += 1.0;
		else info->total_time += timer_value (timers, 0);
		if (info->total_time > bench_workers_time) {
			bench_worker_finished = TRUE;
			break;
		}
	}
	lucasDone (&lldata);
}

int primeBenchMultipleWorkersInternal (
	int	thread_num,
	struct primenetBenchmarkData *pkt,
	unsigned long min_FFT_length,
	unsigned long max_FFT_length,
	int	only_time_5678,
	int	time_negacyclic,
	int	all_bench,
	const char *bench_cores,
	int	bench_hyperthreading,
	const char *bench_workers,
	int	bench_arch,
	int	bench_oddballs,
	int	bench_error_check,
	int	min_cores,
	int	max_cores,
	int	incr_cores,
	int	min_workers,
	int	max_workers,
	int	incr_workers)
{
	llhandle lldata;
	char	buf[4096];
	int	workers, cpus, hypercpus, impl;
	int	plus1, is_a_5678;
	int	i, stop_reason;
	unsigned long fftlen;
	double	throughput;
	gwthread thread_id[MAX_NUM_WORKERS];
	struct prime_bench_arg info[MAX_NUM_WORKERS];

	gwmutex_init (&bench_workers_mutex);
	gwevent_init (&bench_workers_sync);

	for (plus1 = 0; plus1 <= 1; plus1++) {
	  if (plus1 == 0 && time_negacyclic == 2) continue;
	  if (plus1 == 1 && time_negacyclic == 0) continue;
	  for (fftlen = min_FFT_length * 1024; fftlen <= max_FFT_length * 1024; fftlen += 10) {

	    gwinit (&lldata.gwdata);
	    if (all_bench) lldata.gwdata.bench_pick_nth_fft = 1;
	    stop_reason = lucasSetup (thread_num, fftlen * 17 + 1, fftlen + plus1, &lldata);
	    if (stop_reason) {
		    if (all_bench) gwbench_write_data ();	
		    gwmutex_destroy (&bench_workers_mutex);
		    gwevent_destroy (&bench_workers_sync);
		    return (stop_reason);
	    }

	    fftlen = gwfftlen (&lldata.gwdata);
	    if (fftlen > max_FFT_length * 1024) {
		    lucasDone (&lldata);
		    break;
	    }

	    for (i = fftlen; i >= 9 && (i & 1) == 0; i >>= 1);
	    is_a_5678 = (i <= 8);
	    if (only_time_5678 && !is_a_5678) {
		    lucasDone (&lldata);
		    continue;
	    }

	    if (bench_arch && bench_arch != lldata.gwdata.ARCH) {
		    lucasDone (&lldata);
		    continue;
	    }

	    for (impl = 1; ; impl++) {
		if (impl > 1) {
			if (!all_bench) break;
			gwinit (&lldata.gwdata);
			lldata.gwdata.bench_pick_nth_fft = impl;
			stop_reason = lucasSetup (thread_num, fftlen * 17 + 1, fftlen + plus1, &lldata);
			if (stop_reason) break;	
		}

		if (IniGetInt (INI_FILE, "gwinit_only", 0)) {
			lucasDone (&lldata);
			continue;
		}

		for (cpus = min_cores; cpus <= max_cores; cpus += incr_cores) {
		    if (! is_number_in_list (cpus, bench_cores)) continue;
		    for (hypercpus = 1; hypercpus <= 2; hypercpus++) {
			if (hypercpus > 1 && (HW_NUM_CORES == HW_NUM_THREADS || !bench_hyperthreading)) break;
			
			if (!OS_CAN_SET_AFFINITY && hypercpus > 1 && cpus != HW_NUM_CORES) continue;
			for (workers = min_workers; workers <= cpus && workers <= max_workers; workers += incr_workers) {
			    int	nodes_left, workers_left, node, worker_num, cores_left, core_num;
			    int efficiency_workers, unused_cores_left;

			    if (! is_number_in_list (workers, bench_workers)) continue;
			    if (cpus % workers != 0 && !bench_oddballs) continue;
			    
			    if ((cpus > workers || hypercpus > 1) && ! (CPU_FLAGS & CPU_SSE2)) continue;

			    sprintf (buf, "Timing %lu%s%s FFT, %d core%s%s, %d worker%s.  ",
				     (fftlen & 0x3FF) ? fftlen : fftlen / 1024,
				     (fftlen & 0x3FF) ? "" : "K",
				     plus1 ? " all-complex" : "",
				     cpus, cpus > 1 ? "s" : "",
				     hypercpus > 1 ? " hyperthreaded" : "",
				     workers, workers > 1 ? "s" : "");
			    OutputStr (thread_num, buf);

			    num_bench_workers = workers;
			    num_bench_workers_initialized = 0;
			    bench_worker_finished = FALSE;
			    gwevent_reset (&bench_workers_sync);

			    if (cpus <= (int) HW_NUM_COMPUTE_CORES) {
				unused_cores_left = HW_NUM_COMPUTE_CORES - cpus;	
				efficiency_workers = 0;
			    } else {
				unused_cores_left = 0;					
				if (workers == 1) efficiency_workers = 0;
				else if (workers <= (int) HW_NUM_COMPUTE_CORES) efficiency_workers = 1;
				else efficiency_workers = workers - HW_NUM_COMPUTE_CORES;
			    }

			    nodes_left = HW_NUM_COMPUTE_THREADING_NODES;
			    cores_left = HW_NUM_COMPUTE_CORES;
			    workers_left = workers - efficiency_workers;

			    core_num = 0;
			    worker_num = 0;
			    node = 0;
			    while (workers_left) {
				int	nodes_to_use, cores_this_nodeset, unused_cores_this_nodeset, workers_this_nodeset, nodesets_left;

				cores_this_nodeset = 0;
				nodes_to_use = divide_rounding_up (nodes_left, workers_left);
				nodesets_left = _intmin (nodes_left, workers_left);
				while (nodes_to_use--) {
					int cores_this_node = get_cores_in_threading_node (node);
					cores_this_nodeset += cores_this_node;

					if (nodes_to_use == 0) {
						workers_this_nodeset = divide_rounding (workers_left * cores_this_nodeset, cores_left);
						unused_cores_this_nodeset =
							divide_rounding (cores_this_nodeset * nodesets_left - cores_left + unused_cores_left, nodesets_left);
					}

					node++;
					nodes_left--;

					if (node == HW_NUM_COMPUTE_THREADING_NODES) {
						nodes_left += HW_NUM_THREADING_NODES - HW_NUM_COMPUTE_THREADING_NODES;
						cores_left += HW_NUM_CORES - HW_NUM_COMPUTE_CORES;
						if (cpus > (int) HW_NUM_COMPUTE_CORES) unused_cores_left += HW_NUM_CORES - cpus;
						workers_left += efficiency_workers;
						if (efficiency_workers == 0 && cpus > (int) HW_NUM_COMPUTE_CORES) nodes_to_use = nodes_left;
					}
				}

				cores_left -= cores_this_nodeset;
				unused_cores_left -= unused_cores_this_nodeset;

				for ( ; workers_this_nodeset; workers_this_nodeset--) {
				    int	cores_to_use = (cores_this_nodeset - unused_cores_this_nodeset) / workers_this_nodeset;
				    info[worker_num].main_thread_num = thread_num;
				    info[worker_num].fftlen = fftlen;
				    info[worker_num].plus1 = plus1;
				    info[worker_num].impl = (all_bench ? impl : 0);
				    info[worker_num].core_num = core_num;
				    info[worker_num].core_count = cores_to_use;
				    info[worker_num].hyperthreading = (hypercpus > 1);
				    info[worker_num].error_check = bench_error_check;
				    gwthread_create_waitable (&thread_id[worker_num], &primeBenchOneWorker, (void *) &info[worker_num]);
				    core_num += cores_to_use;
				    cores_this_nodeset -= cores_to_use;
				    worker_num++;
				    workers_left--;
				}
				core_num += unused_cores_this_nodeset;
			    }
			    
			    ASSERTG (workers_left == 0 && cores_left == 0);

			    for (i = 0; i < workers; i++) gwthread_wait_for_exit (&thread_id[i]);
			    stop_reason = stopCheck (thread_num);
			    if (stop_reason) {
				if (all_bench) gwbench_write_data ();	
				lucasDone (&lldata);
				gwmutex_destroy (&bench_workers_mutex);
				gwevent_destroy (&bench_workers_sync);
				return (stop_reason);
			    }

			    strcpy (buf, "Average times: ");
			    throughput = 0.0;
			    for (i = 0; i < workers; i++) {
				if (i) strcat (buf, ", ");
				if (info[i].iterations) {
					sprintf (buf+strlen(buf), "%5.2f", info[i].total_time / info[i].iterations * 1000.0);
					throughput = throughput + info[i].iterations / info[i].total_time;
				} else
					strcat (buf, "INF");
			    }
			    sprintf (buf+strlen(buf), " ms.  Total throughput: %5.2f iter/sec.\n", throughput);
			    OutputStrNoTimeStamp (thread_num, buf);

			    if (all_bench) {
				sprintf (buf,
					 "FFTlen=%lu%s%s, Type=%d, Arch=%d, Pass1=%lu, Pass2=%lu, clm=%lu",
					 (fftlen & 0x3FF) ? fftlen : fftlen / 1024,
					 (fftlen & 0x3FF) ? "" : "K",
					 plus1 ? " all-complex" : "",
					 lldata.gwdata.FFT_TYPE, lldata.gwdata.ARCH,
					 fftlen / (lldata.gwdata.PASS2_SIZE ? lldata.gwdata.PASS2_SIZE : 1),
					 lldata.gwdata.PASS2_SIZE,
					 lldata.gwdata.PASS1_CACHE_LINES / ((CPU_FLAGS & CPU_AVX512F) ? 8 : ((CPU_FLAGS & CPU_AVX) ? 4 : 2)));
			    } else {
				sprintf (buf, "Timings for %lu%s%s FFT length",
					 (fftlen & 0x3FF) ? fftlen : fftlen / 1024,
					 (fftlen & 0x3FF) ? "" : "K",
					 plus1 ? " all-complex" : "");
			    }
			    if (hypercpus <= 2)
				sprintf (buf+strlen(buf), " (%d core%s%s, %d worker%s): ",
					 cpus, cpus > 1 ? "s" : "",
					 hypercpus > 1 ? " hyperthreaded" : "",
					 workers, workers > 1 ? "s" : "");
			    else
				sprintf (buf+strlen(buf), " (%d core%s, %d threads, %d worker%s): ",
					 cpus, cpus > 1 ? "s" : "",
					 hypercpus * cpus,
					 workers, workers > 1 ? "s" : "");

			    throughput = 0.0;
			    for (i = 0; i < workers; i++) {
				if (i) strcat (buf, ", ");
				if (info[i].iterations) {
					sprintf (buf+strlen(buf), "%5.2f", info[i].total_time / info[i].iterations * 1000.0);
					throughput = throughput + info[i].iterations / info[i].total_time;
				} else
					strcat (buf, "INF");
			    }
			    sprintf (buf+strlen(buf), " ms.  Throughput: %5.2f iter/sec.\n", throughput);
			    writeResultsBench (buf);

			    if (all_bench) {
				struct gwbench_add_struct bench_data;
				bench_data.version = GWBENCH_ADD_VERSION;
				bench_data.throughput = throughput;
				bench_data.bench_length = bench_workers_time;
				bench_data.num_cores = cpus;
				bench_data.num_workers = workers;
				bench_data.num_hyperthreads = hypercpus;
				bench_data.error_checking = bench_error_check;
				gwbench_add_data (&lldata.gwdata, &bench_data);
			    }

			    if (!all_bench && is_a_5678 && !plus1 && cpus == HW_NUM_CORES && hypercpus == 1 && fftlen / 1024 >= 1024)
				add_bench_data_to_pkt (pkt, "TP%luK", fftlen, throughput, FALSE);

			} 
		    } 
		} 
		lucasDone (&lldata);
	    }  
	  }  
	}  
	OutputStr (thread_num, "\n");
	writeResultsBench ("\n");

	if (all_bench) gwbench_write_data ();

	OutputStr (thread_num, "Throughput benchmark complete.\n");
	gwmutex_destroy (&bench_workers_mutex);
	gwevent_destroy (&bench_workers_sync);
	return (0);
}

int primeBenchMultipleWorkers (
	int	thread_num)
{
	char	bench_cores[512], bench_workers[512];
	int	min_cores, max_cores, incr_cores, min_workers, max_workers, incr_workers;
	int	all_bench, stop_reason;
	struct primenetBenchmarkData pkt;

	memset (&pkt, 0, sizeof (pkt));
	strcpy (pkt.computer_guid, COMPUTER_GUID);

	OutputStr (thread_num, "Benchmarking multiple workers to measure the impact of memory bandwidth\n");

	bench_workers_time = IniGetInt (INI_FILE, "BenchTime", 10);

	IniGetString (INI_FILE, "BenchCores", bench_cores, sizeof(bench_cores), NULL); 
	IniGetString (INI_FILE, "BenchWorkers", bench_workers, sizeof(bench_workers), NULL); 
	all_bench = IniGetInt (INI_FILE, "AllBench", 0);		

	min_cores = IniGetInt (INI_FILE, "BenchMinCores", 1);
	if (min_cores < 1) min_cores = 1;
	if (min_cores > (int) HW_NUM_CORES) min_cores = HW_NUM_CORES;

	max_cores = IniGetInt (INI_FILE, "BenchMaxCores", HW_NUM_CORES);
	if (max_cores < 1) max_cores = 1;
	if (max_cores > (int) HW_NUM_CORES) max_cores = HW_NUM_CORES;

	incr_cores = IniGetInt (INI_FILE, "BenchCoresIncrement", 1);
	if (incr_cores < 1) incr_cores = 1;

	min_workers = IniGetInt (INI_FILE, "BenchMinWorkers", 1);
	if (min_workers < 1) min_workers = 1;
	if (min_workers > (int) HW_NUM_CORES) min_workers = HW_NUM_CORES;

	max_workers = IniGetInt (INI_FILE, "BenchMaxWorkers", HW_NUM_CORES);
	if (max_workers < 1) max_workers = 1;
	if (max_workers > (int) HW_NUM_CORES) max_workers = HW_NUM_CORES;

	incr_workers = IniGetInt (INI_FILE, "BenchWorkersIncrement", 1);
	if (incr_workers < 1) incr_workers = 1;

	stop_reason = primeBenchMultipleWorkersInternal (
		thread_num,
		&pkt,
		IniGetInt (INI_FILE, "MinBenchFFT", 1024),
		IniGetInt (INI_FILE, "MaxBenchFFT", 8192),
		IniGetInt (INI_FILE, "OnlyBench5678", 0),			
		IniGetInt (INI_FILE, "BenchNegacyclic", 0),
		all_bench,
		bench_cores,
		IniGetInt (INI_FILE, "BenchHyperthreads", 1),			
		bench_workers,
		IniGetInt (INI_FILE, "BenchArch", 0),				
		IniGetInt (INI_FILE, "BenchOddWorkers", 1),			
		IniGetInt (INI_FILE, "BenchErrorCheck", 0),			
		min_cores,
		max_cores,
		incr_cores,
		min_workers,
		max_workers,
		incr_workers);

	if (all_bench) gwbench_write_data ();

	if (stop_reason) return (stop_reason);

	if (pkt.num_data_points)
		spoolMessage (PRIMENET_BENCHMARK_DATA, &pkt);

	OutputStr (thread_num, "Throughput benchmark complete.\n");
	return (0);
}

void autoBench (void)
{
	char	bench_cores[10], bench_workers[10];
	int	num_cores, num_workers, autobench_num_benchmarks, tnum, i, num_ffts_to_bench;
	double	autobench_days_of_work;
	struct {
		unsigned long fftlen;
		int	negacyclic;
	} ffts_to_bench[500];
	struct primenetBenchmarkData pkt;

	if (!WORKERS_ACTIVE || WORKERS_STOPPING || LAUNCH_TYPE != LD_CONTINUE) return;

	if (!RUN_ON_BATTERY && OnBattery ()) return;

	for (i = 0; i < (int) NUM_WORKERS; i++) if (STOP_FOR_PAUSE[i] != NULL) return;

	for (i = 0; i < (int) NUM_WORKERS; i++)
		if ((MEM_FLAGS[i] & (MEM_VARIABLE_USAGE | MEM_WILL_BE_VARIABLE_USAGE)) && MEM_IN_USE[i] >= 250) return;

	if (BENCH_NUM_CORES)
		num_cores = BENCH_NUM_CORES;		
	else {
		
		num_cores = HW_NUM_CORES;
	}
	num_workers = BENCH_NUM_WORKERS ? BENCH_NUM_WORKERS : NUM_WORKERS; 

	autobench_days_of_work = (double) IniGetInt (INI_FILE, "AutoBenchDaysOfWork", 7);
	autobench_num_benchmarks = IniGetInt (INI_FILE, "AutoBenchNumBenchmarks", 10);

	num_ffts_to_bench = 0;
	for (tnum = 0; tnum < (int) NUM_WORKERS; tnum++) {
	    struct work_unit *w;
	    double	est;

	    est = 0.0;
	    for (int pass = 0; pass <= 1; pass++)
	    for (w = NULL; ; ) {
		int	negacyclic, num_benchmarks;
		unsigned long minimum_fftlen, first_fftlen, fftlen;
		bool	have_first_fftlen;
		double	plausible_fftlen_multiplier;

		w = getNextWorkToDoLine (tnum, w, SHORT_TERM_USE);
		if (w == NULL) break;
		if (w->work_type == WORK_NONE) continue;
		if (pass == 0 && w->work_type != WORK_CERT) continue;
		if (pass == 1 && w->work_type == WORK_CERT) continue;

		if (est > autobench_days_of_work * 86400.0) continue;
		est += work_estimate (tnum, w);

		if (w->work_type == WORK_FACTOR) continue;
		if (w->work_type == WORK_CERT) continue;

		have_first_fftlen = FALSE;
		plausible_fftlen_multiplier = (w->work_type == WORK_ECM || w->work_type == WORK_PMINUS1 || w->work_type == WORK_PFACTOR) ? 1.15 : 1.03;
		for (minimum_fftlen = w->minimum_fftlen; ; minimum_fftlen = fftlen + 10) {

		    gwbench_get_num_benchmarks (w->k, w->b, w->n, w->c, minimum_fftlen, num_cores, num_workers, HYPERTHREAD_LL, ERRCHK,
						&fftlen, &negacyclic, &num_benchmarks);
		    if (fftlen == 0) break;

		    if (!have_first_fftlen) { first_fftlen = fftlen; have_first_fftlen = TRUE; }
		    else if (fftlen > first_fftlen * plausible_fftlen_multiplier) break;

		    if (num_benchmarks >= autobench_num_benchmarks) continue;

		    if (fftlen < 8192) continue;

		    for (i = 0; ; i++) {
			if (i == num_ffts_to_bench) {
				ffts_to_bench[num_ffts_to_bench].fftlen = fftlen;
				ffts_to_bench[num_ffts_to_bench].negacyclic = negacyclic;
				num_ffts_to_bench++;
				break;
			}
			if (ffts_to_bench[i].fftlen == fftlen && ffts_to_bench[i].negacyclic == negacyclic) break;
		    }
		}
	    }
	}

	if (num_ffts_to_bench == 0) return;

	memset (&pkt, 0, sizeof (pkt));
	strcpy (pkt.computer_guid, COMPUTER_GUID);

	OutputStr (MAIN_THREAD_NUM, "Benchmarking multiple workers to tune FFT selection.\n");

	sprintf (bench_cores, "%d", num_cores);
	sprintf (bench_workers, "%d", num_workers);
	bench_workers_time = IniGetInt (INI_FILE, "AutoBenchTime", 12);		

	gwevent_reset (&AUTOBENCH_EVENT);
	STOP_FOR_AUTOBENCH = TRUE;

	Sleep (3000);

	for (i = 0; i < num_ffts_to_bench; i++) {
		int	stop_reason;

		stop_reason = primeBenchMultipleWorkersInternal (
			MAIN_THREAD_NUM,				
			&pkt,
			ffts_to_bench[i].fftlen / 1024,			
			ffts_to_bench[i].fftlen / 1024,			
			FALSE,						
			ffts_to_bench[i].negacyclic,
			TRUE,						
			bench_cores,
			HYPERTHREAD_LL,					
			bench_workers,
			0,						
			1,						
			ERRCHK,						
			num_cores,					
			num_cores,					
			1,						
			num_workers,					
			num_workers,					
			1);						

		if (stop_reason) break;
	}

	gwbench_write_data ();

	memset (JACOBI_ERROR_CHECK, 0, sizeof (JACOBI_ERROR_CHECK));
	start_Jacobi_timer ();

	STOP_FOR_AUTOBENCH = FALSE;
	gwevent_signal (&AUTOBENCH_EVENT);
}

int primeBench (
	int	thread_num,
	int	bench_type)
{
	struct PriorityInfo sp_info;
	llhandle lldata;
	unsigned long i, impl, j, iterations;
	double	best_time, total_time;
	char	buf[512];
	char	bench_cores[512];
	int	min_cores, max_cores, incr_cores, cores, hypercpu;
	int	all_bench, only_time_5678, time_negacyclic, plus1, stop_reason;
	int	is_a_5678, bench_hyperthreading, bench_arch;
	unsigned long fftlen, min_FFT_length, max_FFT_length;
	double	timers[2];
	struct primenetBenchmarkData pkt;

	title (thread_num, "Benchmarking");
	OutputStr (thread_num, BENCH1);
	OutputBothBench (thread_num, BENCH2);

	getCpuDescription (buf, 1);
	writeResultsBench (buf);
	if (IniGetInt (INI_FILE, "BenchOutputTopology", 1)) {
		writeResultsBench ("Machine topology as determined by hwloc library:\n");
		topology_print_children (hwloc_get_root_obj (hwloc_topology), 0);
	}

#ifdef X86_64
	sprintf (buf, "Prime95 64-bit version %s, RdtscTiming=%d\n", VERSION, RDTSC_TIMING);
#else
	sprintf (buf, "Prime95 32-bit version %s, RdtscTiming=%d\n", VERSION, RDTSC_TIMING);
#endif
	writeResultsBench (buf);

	memset (&sp_info, 0, sizeof (sp_info));
	sp_info.type = SET_PRIORITY_BENCHMARKING;
	sp_info.worker_num = thread_num;
	sp_info.verbosity = IniGetInt (INI_FILE, "AffinityVerbosityBench", 0);
	sp_info.bench_hyperthreading = FALSE;
	SetPriority (&sp_info);

	if (bench_type == 0) {
		return (primeBenchMultipleWorkers (thread_num));
	}

	if (bench_type == 2) {
		return (factorBench (thread_num));
	}

	memset (&pkt, 0, sizeof (pkt));
	strcpy (pkt.computer_guid, COMPUTER_GUID);

	min_FFT_length = IniGetInt (INI_FILE, "MinBenchFFT", 1024);
	max_FFT_length = IniGetInt (INI_FILE, "MaxBenchFFT", 8192);
	only_time_5678 = IniGetInt (INI_FILE, "OnlyBench5678", 1);
	time_negacyclic = IniGetInt (INI_FILE, "BenchNegacyclic", 0);
	all_bench = 0; 
	bench_arch = IniGetInt (INI_FILE, "BenchArch", 0);			
	IniGetString (INI_FILE, "BenchCores", bench_cores, sizeof(bench_cores), NULL); 
	bench_hyperthreading = IniGetInt (INI_FILE, "BenchHyperthreads", 1);	

	if (IniGetInt (INI_FILE, "BenchDummyWorkers", 0)) {
		last_bench_core_num = 0;				
		for (i = 1; i < HW_NUM_COMPUTE_CORES; i++) {	
			gwthread thread_id;
			gwthread_create (&thread_id, &bench_busy_loop, (void *) (intptr_t) i);
		}
	}

	min_cores = IniGetInt (INI_FILE, "BenchMinCores", 1);
	if (min_cores < 1) min_cores = 1;
	if (min_cores > (int) HW_NUM_CORES) min_cores = HW_NUM_CORES;

	max_cores = IniGetInt (INI_FILE, "BenchMaxCores", HW_NUM_CORES);
	if (max_cores < 1) max_cores = 1;
	if (max_cores > (int) HW_NUM_CORES) max_cores = HW_NUM_CORES;

	incr_cores = IniGetInt (INI_FILE, "BenchCoresIncrement", 1);
	if (incr_cores < 1) incr_cores = 1;

	for (cores = min_cores; cores <= max_cores; cores += incr_cores) {
	  if (! is_number_in_list (cores, bench_cores)) continue;
	  for (hypercpu = 1; hypercpu <= 2; hypercpu++) {
	    if (hypercpu > 1 && !bench_hyperthreading) break;
	    if (hypercpu > 1 && HW_NUM_CORES == HW_NUM_THREADS) break;
	    
	    if (hypercpu > 1 && cores != 1 && cores != HW_NUM_COMPUTE_CORES && cores != HW_NUM_CORES) continue;
	    
	    if (!OS_CAN_SET_AFFINITY && hypercpu > 1 && cores != HW_NUM_CORES) continue;
	    
	    if (cores > 1 || hypercpu > 1) {
	      if (! (CPU_FLAGS & CPU_SSE2)) continue;  
	      sprintf (buf, "Timing%s FFTs using %d core%s.\n", hypercpu > 1 ? " hyperthreaded" : "", cores, cores > 1 ? "s" : "");
	      OutputBothBench (thread_num, buf);
	    }

	    last_bench_core_num = cores - 1;

	    for (plus1 = 0; plus1 <= 1; plus1++) {
	      if (plus1 == 0 && time_negacyclic == 2) continue;
	      if (plus1 == 1 && time_negacyclic == 0) continue;
	      for (fftlen = min_FFT_length * 1024; fftlen <= max_FFT_length * 1024; fftlen += 10) {
	        for (impl = 1; ; impl++) {
		  if (impl > 1 && !all_bench) break;

		  gwinit (&lldata.gwdata);
		  if (IniGetInt (INI_FILE, "UseLargePages", 0)) gwset_use_large_pages (&lldata.gwdata);
		  gwset_num_threads (&lldata.gwdata, get_ranked_num_threads (0, cores, hypercpu > 1));
		  sp_info.bench_base_core_num = 0;
		  sp_info.bench_hyperthreading = (hypercpu > 1);
		  gwset_thread_callback (&lldata.gwdata, SetAuxThreadPriority);
		  gwset_thread_callback_data (&lldata.gwdata, &sp_info);
		  if (all_bench) lldata.gwdata.bench_pick_nth_fft = impl;
		  gwset_use_spin_wait (&lldata.gwdata, IniGetInt (INI_FILE, "SpinWait", 0));
		  stop_reason = lucasSetup (thread_num, fftlen * 17 + 1, fftlen + plus1, &lldata);
		  if (stop_reason) {
			
			if (!all_bench) fftlen = max_FFT_length * 1024;
			break;
		  }
		  fftlen = gwfftlen (&lldata.gwdata);
		  if (fftlen > max_FFT_length * 1024) {
			lucasDone (&lldata);
			break;
		  }

		  if (fftlen & 0x3FF) {
			lucasDone (&lldata);
			break;
		  }

		  for (i = fftlen; i >= 9 && (i & 1) == 0; i >>= 1);
		  is_a_5678 = (i <= 8);
		  if (only_time_5678 && !is_a_5678) {
			lucasDone (&lldata);
			break;
		  }

		  if (bench_arch && bench_arch != lldata.gwdata.ARCH) {
			lucasDone (&lldata);
			continue;
		  }

		  if (all_bench && impl == 1) writeResultsBench ("\n");

		  iterations = (unsigned long) (10 * 1792 * CPU_SPEED / 1400 / (fftlen / 1024));
		  if (iterations < 10) iterations = 10;
		  if (iterations < 25 && (CPU_FLAGS & CPU_AVX)) iterations = 25;
		  if (iterations > 100) iterations = 100;

		  sprintf (buf, "Timing %lu iterations of %luK%s FFT length%s.  ",
			 iterations, fftlen / 1024,
			 plus1 ? " all-complex" : "",
			 gw_using_large_pages (&lldata.gwdata) ? " using large pages" : "");
		  OutputStr (thread_num, buf);

		  generateRandomData (&lldata);

		  gwsetnormroutine (&lldata.gwdata, 0, 0, 0);
		  gwstartnextfft (&lldata.gwdata, TRUE);
		  gwsquare (&lldata.gwdata, lldata.lldata);

		  total_time = 0.0;
		  for (j = 0; j < iterations; j++) {
			stop_reason = stopCheck (thread_num);
			if (stop_reason) {
				OutputStrNoTimeStamp (thread_num, "\n");
				OutputStr (thread_num, "Execution halted.\n");
				lucasDone (&lldata);
				last_bench_core_num = HW_NUM_CORES;
				return (stop_reason);
			}
			clear_timers (timers, sizeof (timers) / sizeof (timers[0]));
			start_timer (timers, 0);
			gwsquare (&lldata.gwdata, lldata.lldata);
			end_timer (timers, 0);
			total_time += timers[0];
			if (j == 0 || timers[0] < best_time) best_time = timers[0];
		  }
		  lucasDone (&lldata);

		  timers[0] = best_time;
		  strcpy (buf, "Best time: ");
		  print_timer (timers, 0, buf, TIMER_MS);
		  timers[0] = total_time / iterations;
		  strcat (buf, ", avg time: ");
		  print_timer (timers, 0, buf, TIMER_NL | TIMER_MS);
		  OutputStrNoTimeStamp (thread_num, buf);
		  if (all_bench) {
			sprintf (buf,
				 "Time FFTlen=%luK%s, Type=%d, Arch=%d, Pass1=%lu, Pass2=%lu, clm=%lu: ",
				 fftlen / 1024, plus1 ? " all-complex" : "",
				 lldata.gwdata.FFT_TYPE, lldata.gwdata.ARCH,
				 fftlen / (lldata.gwdata.PASS2_SIZE ? lldata.gwdata.PASS2_SIZE : 1),
				 lldata.gwdata.PASS2_SIZE,
				 lldata.gwdata.PASS1_CACHE_LINES / ((CPU_FLAGS & CPU_AVX512F) ? 8 : ((CPU_FLAGS & CPU_AVX) ? 4 : 2)));
			timers[0] = best_time;
			print_timer (timers, 0, buf, TIMER_MS);
			timers[0] = total_time / iterations;
			strcat (buf, ", avg: ");
			print_timer (timers, 0, buf, TIMER_NL | TIMER_MS);
		  } else {
			sprintf (buf, "Best time for %luK%s FFT length: ", fftlen / 1024, plus1 ? " all-complex" : "");
			timers[0] = best_time;
			print_timer (timers, 0, buf, TIMER_MS);
			timers[0] = total_time / iterations;
			strcat (buf, ", avg: ");
			print_timer (timers, 0, buf, TIMER_NL | TIMER_MS);
		  }
		  writeResultsBench (buf);

		  if (!all_bench && is_a_5678 && !plus1 && cores == 1 && hypercpu == 1 && fftlen / 1024 >= 1024)
			add_bench_data_to_pkt (&pkt, "FFT%luK", fftlen, timer_value (timers, 0), TRUE);

		  if (!all_bench && is_a_5678 && !plus1 && cores == HW_NUM_COMPUTE_CORES && hypercpu == 1 && fftlen / 1024 >= 1024)
			add_bench_data_to_pkt (&pkt, "AC%luK", fftlen, timer_value (timers, 0), TRUE);

	        }  
	      }  
	    }  
	  } 
	} 

	last_bench_core_num = HW_NUM_CORES;

	OutputStr (thread_num, "FFT timings benchmark complete.\n");
	writeResultsBench ("\n");

	if (pkt.num_data_points)
		spoolMessage (PRIMENET_BENCHMARK_DATA, &pkt);

	return (0);
}

struct prp_state {
	int	thread_num;		
	gwnum	x;			
	unsigned long counter;		
	unsigned long units_bit;	
	unsigned long error_count;	
	unsigned int prp_base;		
	int	two_power_opt;		
	int	residue_type;		
	int	error_check_type;	
	int	state;			
	gwnum	alt_x;			
					
	unsigned long alt_units_bit;	
	gwnum	u0;			
	gwnum	d;			
	unsigned long L;		
	unsigned long start_counter;	
	unsigned long next_mul_counter;	
	unsigned long end_counter;	
	unsigned long proof_num_iters;	
	int	proof_version;		
	int	proof_power;		
	int	proof_power_mult;	
	int	hashlen;		
	int	isProbablePrime;	
	int	have_res2048;		
	char	residues_filename[500];	
	int	residue_size;		
	int	md5_residues;		
	int	max_emergency_allocs;	
	int	num_emergency_allocs;	
	char	**emergency_allocs;	
	int	first_emergency_residue_number; 
	char	res2048[513];		
	char	res64[17];		
};

#define PRP_ERRCHK_NONE		0	
#define PRP_ERRCHK_GERBICZ	1	
#define PRP_ERRCHK_DBLCHK	2	

#define PRP_STATE_NORMAL		0	
#define PRP_STATE_DCHK_PASS1		10	
#define PRP_STATE_DCHK_PASS2		11	
#define PRP_STATE_GERB_START_BLOCK	22	
#define PRP_STATE_GERB_MID_BLOCK	23	
#define PRP_STATE_GERB_MID_BLOCK_MULT	24	
#define PRP_STATE_GERB_END_BLOCK	25	
#define PRP_STATE_GERB_END_BLOCK_MULT	26	
#define PRP_STATE_GERB_FINAL_MULT	27	

void createProofResiduesFile (
	gwhandle *gwdata,
	struct prp_state *ps,
	int	prefill)
{
	int	fd, i, j;
	uint64_t total_size, randomizer, *p;
	char	buf[65536];

	fd = _open (ps->residues_filename, _O_BINARY | _O_WRONLY | _O_TRUNC | _O_CREAT, CREATE_FILE_ACCESS);
	if (fd < 0) {
		sprintf (buf, "Cannot create PRP proof interim residues file: %s\n", ps->residues_filename);
		OutputBoth (ps->thread_num, buf);
		OutputBothErrno (ps->thread_num);
		OutputBoth (ps->thread_num, "PRP test will use emergency memory to hold interim proof residues in hopes the file can be created later.\n");
		return;
	}

	if (prefill) {
		
		sprintf (buf, "Preallocating disk space for the proof interim residues file %s\n", ps->residues_filename);
		OutputStr (ps->thread_num, buf);

		srand ((unsigned) time (NULL));
		for (i = 0; i < sizeof (buf); i++) buf[i] = rand() & 0xFF;

		if (!ps->md5_residues)
			total_size = (1ULL << ps->proof_power) * (uint64_t) ps->residue_size;
		else
			total_size = (1ULL << ps->proof_power) * (uint64_t) (ps->residue_size + 16);

		for (i = 0; i < (int) divide_rounding_up (total_size, sizeof (buf)); i++) {
			if (_write (fd, buf, sizeof (buf)) != sizeof (buf)) {
				int	num_residues, new_power, new_power_mult;
				sprintf (buf, "Error pre-allocating proof interim residues file\n");
				OutputBoth (ps->thread_num, buf);
				OutputBothErrno (ps->thread_num);

				num_residues = (int) ((uint64_t) i * (uint64_t) sizeof (buf) / (uint64_t) (ps->residue_size + 16));
				if (num_residues <= 31) new_power = 0;	
				else if (num_residues <= 63) new_power = 5, new_power_mult = 3;
				else if (num_residues <= 127) new_power = 6, new_power_mult = 2;
				else if (num_residues <= 255) new_power = 7, new_power_mult = 2;
				else if (num_residues <= 511) new_power = 8, new_power_mult = 1;
				else if (num_residues <= 1023) new_power = 9, new_power_mult = 1;
				else new_power = 10, new_power_mult = 1;

				if (new_power) {
					if (_chsize_s (fd, (1ULL << new_power) * (uint64_t) (ps->residue_size + 16)) < 0) new_power = 0;
				}

				if (new_power) {
					sprintf (buf, "Will use proof power %d instead of %d.\n", new_power, ps->proof_power);
					OutputBoth (ps->thread_num, buf);
				} else {
					OutputBoth (ps->thread_num, "Could not create decent sized interim proof residues file.  No PRP proof will be done.\n");
				}

				ps->proof_power = new_power;
				ps->proof_power_mult = new_power_mult;
				break;
			}

			p = (uint64_t *) buf;
			randomizer = p[i & (sizeof (buf) / 8 - 1)] << 1;
			for (j = 0; j < sizeof (buf) / 8; j++) *p++ ^= randomizer;
		}
	}

	_close (fd);
	if (ps->proof_power == 0) _unlink (ps->residues_filename);
}

unsigned long proofResidueIteration (
	struct prp_state *ps,
	int	residue_number)
{
	unsigned long result = 0;
	unsigned long n = ps->proof_num_iters;
	unsigned long bit;
	for (bit = (1 << ps->proof_power); bit; bit >>= 1) {
		if (residue_number & bit) result += n;
		n = (n + 1) / 2;
	}
	return (result);
}

void outputProofResidue (
	gwhandle *gwdata,
	struct prp_state *ps,
	int	residue_number,			
	giant	g)				
{
	int	i, fd;

	if (residue_number) {
		char	*buf, *p;

		if (ps->num_emergency_allocs == ps->max_emergency_allocs) {
			outputProofResidue (gwdata, ps, 0, NULL);
			if (ps->num_emergency_allocs == ps->max_emergency_allocs) {
				OutputBoth (ps->thread_num, "No more emergency memory is available to hold interim proof residues.\n");
				goto abort_proof;
			}
		}

		buf = (char *) malloc (round_up_to_multiple_of (ps->residue_size, 4));
		if (buf == NULL) {
			OutputStr (ps->thread_num, "Emergency memory allocation error.\n");
			goto abort_proof;
		}
		ps->emergency_allocs[ps->num_emergency_allocs++] = buf;
		if (ps->num_emergency_allocs == 1) ps->first_emergency_residue_number = residue_number;

		memset (buf, 0, ps->residue_size);
		for (i = 0, p = buf; i < g->sign; i++, p += 4) {
			p[0] = g->n[i] & 0xFF;
			p[1] = (g->n[i] >> 8) & 0xFF;
			p[2] = (g->n[i] >> 16) & 0xFF;
			p[3] = (g->n[i] >> 24) & 0xFF;
		}
	}

	if (ps->num_emergency_allocs == 0) return;

	fd = _open (ps->residues_filename, _O_BINARY | _O_WRONLY | _O_CREAT, CREATE_FILE_ACCESS);
	if (fd < 0) {
		char	buf[1000];
		sprintf (buf, "Cannot open PRP proof interim residues file: %s\n", ps->residues_filename);
		OutputBoth (ps->thread_num, buf);
		OutputBothErrno (ps->thread_num);
		OutputStr (ps->thread_num, "Keeping proof interim residues in emergency memory in hopes problem will resolve itself.\n");
		OutputStr (ps->thread_num, "Errors such as disk full and network disk offline can be temporary.\n");
		return;
	}

	while (ps->num_emergency_allocs) {
		int	file_residue_number;

		file_residue_number = ps->first_emergency_residue_number - 1;
		file_residue_number &= (1 << ps->proof_power) - 1;

		if (!ps->md5_residues) {		
			if (_lseeki64 (fd, (int64_t) file_residue_number * (int64_t) ps->residue_size, SEEK_SET) < 0 ||
			    _write (fd, ps->emergency_allocs[0], ps->residue_size) != ps->residue_size) {
				char	buf[1000];
				sprintf (buf, "Error writing to PRP proof interim residues file: %s\n", ps->residues_filename);
				OutputBoth (ps->thread_num, buf);
				OutputBothErrno (ps->thread_num);
				OutputStr (ps->thread_num, "Keeping proof interim residues in emergency memory in hopes problem will resolve itself.\n");
				OutputStr (ps->thread_num, "Errors such as disk full and network disk offline can be temporary.\n");
				break;
			}
		}

		else {
			unsigned char MD5[16];
			md5_digest_buffer (MD5, ps->emergency_allocs[0], ps->residue_size);
			if (_lseeki64 (fd, (int64_t) file_residue_number * (int64_t) (ps->residue_size + 16), SEEK_SET) < 0 ||
			    _write (fd, MD5, 16) != 16 ||
			    _write (fd, ps->emergency_allocs[0], ps->residue_size) != ps->residue_size) {
				char	buf[1000];
				sprintf (buf, "Error writing to PRP proof interim residues file: %s\n", ps->residues_filename);
				OutputBoth (ps->thread_num, buf);
				OutputBothErrno (ps->thread_num);
				OutputStr (ps->thread_num, "Keeping proof interim residues in emergency memory in hopes problem will resolve itself.\n");
				OutputStr (ps->thread_num, "Errors such as disk full and network disk offline can be temporary.\n");
				break;
			}

		}

		free (ps->emergency_allocs[0]);
		ps->num_emergency_allocs--;
		memmove (ps->emergency_allocs, ps->emergency_allocs + 1, ps->num_emergency_allocs * sizeof (char *));
		ps->first_emergency_residue_number++;
	}

	_close (fd);
	return;

abort_proof:
	OutputBoth (ps->thread_num, "Aborting PRP proof.\n");
	for (i = 0; i < ps->num_emergency_allocs; i++) free (ps->emergency_allocs[i]);
	ps->num_emergency_allocs = 0;
	_unlink (ps->residues_filename);
	ps->proof_power = 0;
}

#include "proof_hash.c"

int readResidue (			
	struct prp_state *ps,
	gwhandle *gwdata,
	int	fd,
	int	residue_number,		
	gwnum	x)
{
	int	i;
	uint32_t *array;		
	uint32_t arraylen;		

	arraylen = divide_rounding_up (ps->residue_size, 4);
	array = (uint32_t *) malloc (arraylen * sizeof(uint32_t));
	if (array == NULL) {
		OutputBoth (ps->thread_num, "Error allocating memory for reading PRP proof interim residue.\n");
		return (FALSE);
	}
	array[arraylen-1] = 0;		

	if (ps->num_emergency_allocs && residue_number >= ps->first_emergency_residue_number) {
		memcpy (array, ps->emergency_allocs[residue_number - ps->first_emergency_residue_number], ps->residue_size);
	}

	else {
		if (!ps->md5_residues) {	
			if (_lseeki64 (fd, ((int64_t) residue_number - 1) * (int64_t) ps->residue_size, SEEK_SET) < 0 ||
			    _read (fd, array, ps->residue_size) != ps->residue_size) {
				OutputBoth (ps->thread_num, "Error reading PRP proof interim residues file.\n");
				OutputBothErrno (ps->thread_num);
				free (array);
				return (FALSE);
			}
		}
		else {
			unsigned char expected_MD5[16];	
			unsigned char actual_MD5[16];	
			if (_lseeki64 (fd, ((int64_t) residue_number - 1) * (int64_t) (ps->residue_size + 16), SEEK_SET) < 0 ||
			    _read (fd, expected_MD5, 16) != 16 ||
			    _read (fd, array, ps->residue_size) != ps->residue_size) {
				OutputBoth (ps->thread_num, "Error reading PRP proof interim residues file.\n");
				OutputBothErrno (ps->thread_num);
				free (array);
				return (FALSE);
			}
			md5_digest_buffer (actual_MD5, array, ps->residue_size);
			if (memcmp (expected_MD5, actual_MD5, 16) != 0) {
				OutputBoth (ps->thread_num, "MD5 error reading PRP proof interim residues file.\n");
				free (array);
				return (-1);
			}
		}
	}

	for (i = 0; i < (int) arraylen; i++) {
		uint32_t val;
		val = ((unsigned char *)&array[i])[3];
		val = (val << 8) + ((unsigned char *)&array[i])[2];
		val = (val << 8) + ((unsigned char *)&array[i])[1];
		val = (val << 8) + ((unsigned char *)&array[i])[0];
		array[i] = val;
	}

	binarytogw (gwdata, array, arraylen, x);
	free (array);
	return (TRUE);
}

int writeResidue (			
	struct prp_state *ps,
	gwhandle *gwdata,
	int	fd,
	gwnum	x,
	MD5_CTX *context)
{
	long	len;
	int	i;
	uint32_t *array;		
	uint32_t arraylen;		

	arraylen = divide_rounding_up (ps->residue_size, 4);
	array = (uint32_t *) malloc (divide_rounding_up (ps->residue_size, 4) * sizeof(uint32_t));
	if (array == NULL) {
		OutputBoth (ps->thread_num, "Error allocating memory for writing PRP proof residue.\n");
		return (FALSE);
	}

	len = gwtobinary (gwdata, x, array, arraylen);
	if (len < 0) {
		OutputBoth (ps->thread_num, "Error converting PRP proof residue to binary.\n");
		free (array);
		return (FALSE);
	}

	for (i = 0; i < len; i++) {
		uint32_t val = array[i];
		((unsigned char *)&array[i])[0] = val & 0xFF;
		((unsigned char *)&array[i])[1] = (val >> 8) & 0xFF;
		((unsigned char *)&array[i])[2] = (val >> 16) & 0xFF;
		((unsigned char *)&array[i])[3] = (val >> 24) & 0xFF;
	}
	for ( ; i < (int) arraylen; i++) array[i] = 0;

	if (_write (fd, array, ps->residue_size) != ps->residue_size) {
		OutputBoth (ps->thread_num, "Error writing proof file.\n");
		OutputBothErrno (ps->thread_num);
		free (array);
		return (FALSE);
	}
	if (context != NULL) MD5Update (context, array, ps->residue_size);

	free (array);
	return (TRUE);
}

#ifdef EASY_TO_READ_CODE

int calc_middle (			
	struct prp_state *ps,
	gwhandle *gwdata,
	int	fd,			
	int	base,			
	int	end,			
	int	i,			
	uint64_t *hash_array,		
	gwnum	result)			
{
	gwnum	tmp = NULL;
	int	rc;

	if (i == 0) return (readResidue (ps, gwdata, fd, (base + end) / 2, result));	
	
	rc = calc_middle (ps, gwdata, fd, base, base + (end - base) / 2, i - 1, hash_array + 1, result);
	if (rc <= 0) goto done;
	exponentiate (gwdata, result, *hash_array);
	
	tmp = gwalloc (gwdata);
	rc = calc_middle (ps, gwdata, fd, base + (end - base) / 2, end, i - 1, hash_array + 1, tmp);
	if (rc <= 0) goto done;
	gwmul (gwdata, tmp, result);
	rc = TRUE;

done:	gwfree (gwdata, tmp);
	return (rc);
}

#else					

int calc_middle_mult (struct prp_state *, gwhandle *, int, int, int, int, uint64_t *, gwnum);

int calc_middle (			
	struct prp_state *ps,
	gwhandle *gwdata,
	int	fd,			
	int	base,			
	int	end,			
	int	i,			
	uint64_t *hash_array,		
	gwnum	result)			
{
	int	rc;

	if (i == 0) return (readResidue (ps, gwdata, fd, (base + end) / 2, result));	
	
	rc = calc_middle (ps, gwdata, fd, base, base + (end - base) / 2, i - 1, hash_array + 1, result);
	if (rc <= 0) return (rc);
	exponentiate (gwdata, result, *hash_array);
	
	return (calc_middle_mult (ps, gwdata, fd, base + (end - base) / 2, end, i - 1, hash_array + 1, result));
}

int calc_middle_mult (			
	struct prp_state *ps,
	gwhandle *gwdata,
	int	fd,			
	int	base,			
	int	end,			
	int	i,			
	uint64_t *hash_array,		
	gwnum	result)			
{
	gwnum	tmp;
	int	rc;

	tmp = gwalloc (gwdata);
	if (tmp == NULL) {
		OutputBoth (ps->thread_num, "Error allocating memory for proof hash multiplications.\n");
		return (FALSE);
	}
	if (i == 0) {			
		rc = readResidue (ps, gwdata, fd, (base + end) / 2, tmp);
		if (rc <= 0) goto err;
		gwmul (gwdata, tmp, result);
		gwfree (gwdata, tmp);
		return (TRUE);
	}
	
	rc = calc_middle (ps, gwdata, fd, base, base + (end - base) / 2, i - 1, hash_array + 1, tmp);
	if (rc <= 0) goto err;
	exponentiate (gwdata, tmp, *hash_array);
	gwmul (gwdata, tmp, result);
	gwfree (gwdata, tmp);
	
	return (calc_middle_mult (ps, gwdata, fd, base + (end - base) / 2, end, i - 1, hash_array + 1, result));

err:	gwfree (gwdata, tmp);
	return (rc);
}

#endif

int generateProofFile (
	gwhandle *gwdata,		
	struct prp_state *ps,		
	struct work_unit *w,		
	int	proof_number,		
	char	proof_hash[33])		
{
	int	proofgen_waits;
	char	buf[512], filename[32], proof_filename[500], tmp_proof_filename[504];

	proofgen_waits = IniGetInt (INI_FILE, "MaxProofgenWaits", 48 * 60 / 5);		
	if (proofgen_waits < 1) proofgen_waits = 1;
	proofgen_waits = IniGetInt (INI_FILE, "CurrentProofgenWaits", proofgen_waits);

	sprintf (buf, "Generating%s proof for %s.  Proof power = %d, Hash length = %d\n",
		 ps->proof_power_mult > 1 ? " partial" : "", gwmodulo_as_string(gwdata), ps->proof_power, ps->hashlen);
	OutputStr (ps->thread_num, buf);

	tempFileName (w, filename);
	sprintf (proof_filename, "%s.proof", filename);
	sprintf (tmp_proof_filename, "%s.proof.tmp", filename);

	for ( ; ; ) {
		int	fd, fdout, rc, i, stop_reason;
		int64_t	proof_header_size, proof_residues_size, proof_file_size, proof_file_start_offset;
		uint64_t h[20];
		hash256_t rooth, *prevh, thish;
		gwnum	M;
		MD5_CTX context;
		unsigned char digest[16];
		char	MD5_output[33];

		fd = -1;
		fdout = -1;
		MD5Init (&context);
		M = gwalloc (gwdata);
		if (M == NULL) {
			OutputBoth (ps->thread_num, "Error allocating proof memory\n");
			goto pfail;
		}
		gwfree_internal_memory (gwdata);

		if (ps->num_emergency_allocs == 0 || ps->first_emergency_residue_number != 1) {
			fd = _open (ps->residues_filename, _O_BINARY | _O_RDONLY);
			if (fd < 0) {
				sprintf (buf, "Cannot open PRP proof interim residues file: %s\n", ps->residues_filename);
				OutputBoth (ps->thread_num, buf);
				OutputBothErrno (ps->thread_num);
				goto pfail;
			}
		}
		if (proof_number == 1) fdout = _open (tmp_proof_filename, _O_BINARY | _O_WRONLY | _O_CREAT, CREATE_FILE_ACCESS);
		else fdout = _open (tmp_proof_filename, _O_BINARY | _O_WRONLY);
		if (fdout < 0) {
			sprintf (buf, "Cannot %s PRP proof file: %s\n", proof_number == 1 ? "create" : "open", tmp_proof_filename);
			OutputBoth (ps->thread_num, buf);
			OutputBothErrno (ps->thread_num);
			goto pfail;
		}

		sprintf (buf, "PRP PROOF\nVERSION=%d\nHASHSIZE=%d\n", ps->proof_version, ps->hashlen);
		if (ps->proof_power_mult == 1) sprintf (buf+strlen(buf), "POWER=%d\n", ps->proof_power);
		else sprintf (buf+strlen(buf), "POWER=%dx%d\n", ps->proof_power, ps->proof_power_mult);
		if (ps->prp_base != 3) sprintf (buf+strlen(buf), "BASE=%u\n", ps->prp_base);
		if (_write (fdout, buf, (unsigned int) strlen (buf)) != (int) strlen (buf)) {
			OutputBoth (ps->thread_num, "Error writing proof file header.\n");
			OutputBothErrno (ps->thread_num);
			goto pfail;
		}
		MD5Update (&context, buf, (unsigned int) strlen (buf));

		sprintf (buf, "NUMBER=");
		if ((w->k != 1.0 || w->c != -1) && w->known_factors != NULL) strcat (buf, "(");
		strcat (buf, gwmodulo_as_string (gwdata));
		if ((w->k != 1.0 || w->c != -1) && w->known_factors != NULL) strcat (buf, ")");
		if (w->known_factors != NULL) {		
			char	*p;
			sprintf (buf + strlen (buf), "/%s", w->known_factors);
			while ((p = strchr (buf, ',')) != NULL) *p = '/';
		}
		strcat (buf, "\n");
		if (_write (fdout, buf, (unsigned int) strlen (buf)) != (int) strlen (buf)) {
			OutputBoth (ps->thread_num, "Error writing proof file header.\n");
			OutputBothErrno (ps->thread_num);
			goto pfail;
		}
		MD5Update (&context, buf, (unsigned int) strlen (buf));

		proof_header_size = _lseeki64 (fdout, 0, SEEK_CUR);

		proof_file_size = _lseeki64 (fdout, 0, SEEK_END);
		proof_residues_size = (ps->proof_power + 1) * ps->residue_size;
		int proofs_written = (int) ((proof_file_size - proof_header_size) / proof_residues_size);

		if (proofs_written >= proof_number) {
			_close (fd);
			_close (fdout);
			gwfree (gwdata, M);
			sprintf (buf, "Proof has already been written to %s.\n", tmp_proof_filename);
			OutputBoth (ps->thread_num, buf);
			break;
		}

		proof_file_start_offset = _lseeki64 (fdout, proof_header_size + (proof_number - 1) * proof_residues_size, SEEK_SET);
		if (proof_number == 0) proof_file_start_offset = 0;	
		else MD5Init (&context);				
   
		gw_clear_fft_count (gwdata);		
		gw_clear_error (gwdata);
		gw_clear_maxerr (gwdata);
		gwsetnormroutine (gwdata, 0, 1, 0);	

		rc = readResidue (ps, gwdata, fd, (1 << ps->proof_power), M);
		if (rc < 0) goto pfail_wont_get_better;
		if (!rc) goto pfail;
		if (!writeResidue (ps, gwdata, fdout, M, &context)) goto pfail;

		if (!roothash (gwdata, M, &rooth)) {
			OutputBoth (ps->thread_num, "Error computing proof file root hash.\n");
			goto pfail;
		}
		sprintf (buf, "Root hash = %s\n", hash_to_string (rooth));
		OutputStr (ps->thread_num, buf);

		rc = readResidue (ps, gwdata, fd, (1 << ps->proof_power) / 2, M);
		if (rc < 0) goto pfail_wont_get_better;
		if (!rc) goto pfail;
		if (!writeResidue (ps, gwdata, fdout, M, &context)) goto pfail;

		for (i = 0, prevh = &rooth; i < ps->proof_power - 1; i++, prevh = &thish) {
			if (!hash (gwdata, prevh, M, &thish)) {
				OutputBoth (ps->thread_num, "Error computing proof file hash.\n");
				goto pfail;
			}
			h[i] = truncate_hash (thish, ps->hashlen);
			sprintf (buf, "hash%d = %016" PRIX64 "\n", i, h[i]);
			OutputStr (ps->thread_num, buf);
			rc = calc_middle (ps, gwdata, fd, 0, 1 << ps->proof_power, i + 1, h, M);
			if (rc < 0) goto pfail_wont_get_better;
			if (!rc) goto pfail;
			if (!writeResidue (ps, gwdata, fdout, M, &context)) goto pfail;
		}
		MD5Final (digest, &context);
		md5_hexdigest_from_digest (MD5_output, digest);

		if (fd >= 0) _close (fd), fd = -1;
		if (fdout >= 0) _close (fdout), fdout = -1;
		gwfree (gwdata, M); M = NULL;

		if (gw_test_for_error (gwdata)) {
			OutputBoth (ps->thread_num, "An internal error occurred in the gwnum library.  Retrying proof.\n");
			goto pfail;
		}

#ifdef NOT_CODED_YET
		if (gw_get_maxerr (gwdata) > 0.45) goto somewhere; ???
#endif

		sprintf (buf, "Proof construction cost %" PRIu64 " squarings\n", gw_get_fft_count (gwdata) / 2);
		OutputStr (ps->thread_num, buf);

		md5_hexdigest_file_from_offset (proof_hash, tmp_proof_filename, proof_file_start_offset);
		if (strcmp (MD5_output, proof_hash) != 0) {
			OutputBoth (ps->thread_num, "The MD5 hash of the proof file is not correct.\n");
			goto pfail;
		}

		break;
		
pfail_wont_get_better:
		if (proofgen_waits > 2) proofgen_waits = 2;
pfail:		if (fd >= 0) _close (fd);
		if (fdout >= 0) {
			_chsize_s (fdout, proof_file_start_offset);
			_close (fdout);
			if (proof_file_start_offset == 0) _unlink (tmp_proof_filename);
		}
		if (M != NULL) gwfree (gwdata, M);

		proofgen_waits--;
		if (proofgen_waits == 0) {		
			OutputBoth (ps->thread_num, "Proof generation failed.\n");
			ps->proof_power = 0;		
			_unlink (ps->residues_filename);
			break;
		}

		OutputStr (ps->thread_num, "Waiting 5 minutes to try proof generation again.\n");
		stop_reason = SleepFive (ps->thread_num);
		if (stop_reason) {
			IniWriteInt (INI_FILE, "CurrentProofgenWaits", proofgen_waits);
			return (stop_reason);
		}
	}

	if (ps->proof_power) {
		if (proof_number == ps->proof_power_mult) {
			unsigned long verify_cost = divide_rounding_up (ps->proof_num_iters, 1 << ps->proof_power);
			sprintf (buf, "Proof verification will cost %lu squarings\n", verify_cost);
			OutputStr (ps->thread_num, buf);
			_unlink (proof_filename);
			if (rename (tmp_proof_filename, proof_filename)) {
				sprintf (buf, "Error renaming from %s to %s\n", tmp_proof_filename, proof_filename);
				OutputBoth (ps->thread_num, buf);
			}
			_unlink (ps->residues_filename);
			
			md5_hexdigest_file (proof_hash, proof_filename);

		}
		
		else if (!IniGetInt (INI_FILE, "PreallocateDisk", 1)) {
			_unlink (ps->residues_filename);
		}
	}

	for (int i = 0; i < ps->num_emergency_allocs; i++) free (ps->emergency_allocs[i]);
	ps->num_emergency_allocs = 0;

	IniWriteString (INI_FILE, "CurrentProofgenWaits", NULL);
	return (0);
}

#define PRP_MAGICNUM		0x87f2a91b
#define PRP_VERSION		7

int writePRPSaveFile (			
	gwhandle *gwdata,
	writeSaveFileState *write_save_file_state,
	struct work_unit *w,
	struct prp_state *ps)
{
	int	fd;
	uint32_t sum = 0;
	char	buf[512];

	if (ps->num_emergency_allocs) {
		outputProofResidue (gwdata, ps, 0, NULL);		
		if (ps->num_emergency_allocs) {
			OutputBoth (ps->thread_num, "Cannot create save files while holding proof interim residues in emergency memory.\n");
			OutputStr (ps->thread_num, "Continuing without creating this save file.\n");
			return (FALSE);
		}
	}

	fd = openWriteSaveFile (write_save_file_state);
	if (fd < 0) {
		sprintf (buf, "Unable to create PRP save file: %s\n", write_save_file_state->base_filename);
		OutputBoth (ps->thread_num, buf);
		OutputBothErrno (ps->thread_num);
		return (FALSE);
	}

	if (!write_header (fd, PRP_MAGICNUM, PRP_VERSION, w)) goto writeerr;
	if (!write_long (fd, ps->error_count, &sum)) goto writeerr;
	if (!write_long (fd, ps->counter, &sum)) goto writeerr;
	if (!write_long (fd, ps->prp_base, &sum)) goto writeerr;
	if (!write_long (fd, ps->units_bit, &sum)) goto writeerr;
	if (!write_long (fd, ps->two_power_opt, &sum)) goto writeerr;
	if (!write_long (fd, ps->residue_type, &sum)) goto writeerr;
	if (!write_long (fd, ps->error_check_type, &sum)) goto writeerr;
	if (!write_long (fd, ps->state, &sum)) goto writeerr;
	if (!write_long (fd, ps->alt_units_bit, &sum)) goto writeerr;
	if (!write_long (fd, ps->L, &sum)) goto writeerr;
	if (!write_long (fd, ps->start_counter, &sum)) goto writeerr;
	if (!write_long (fd, ps->next_mul_counter, &sum)) goto writeerr;
	if (!write_long (fd, ps->end_counter, &sum)) goto writeerr;
	if (!write_long (fd, ps->proof_power, &sum)) goto writeerr;
	if (!write_long (fd, ps->hashlen, &sum)) goto writeerr;
	if (!write_long (fd, ps->isProbablePrime, &sum)) goto writeerr;
	if (!write_long (fd, ps->have_res2048, &sum)) goto writeerr;
	if (!write_array (fd, ps->res2048, 512, &sum)) goto writeerr;
	if (!write_array (fd, ps->res64, 16, &sum)) goto writeerr;
	if (!write_long (fd, ps->proof_power_mult, &sum)) goto writeerr;
	if (!write_long (fd, ps->md5_residues, &sum)) goto writeerr;
	if (!write_long (fd, ps->proof_version, &sum)) goto writeerr;
	if (!write_gwnum (fd, gwdata, ps->x, &sum)) {
		sprintf (buf, "Error writing FFT data named x, state %d, to PRP save file %s\n", ps->state, write_save_file_state->base_filename);
		OutputBoth (ps->thread_num, buf);
		OutputBothErrno (ps->thread_num);
		goto err;
	}

	if (ps->state != PRP_STATE_NORMAL && ps->state != PRP_STATE_GERB_MID_BLOCK && ps->state != PRP_STATE_GERB_MID_BLOCK_MULT) {
		if (!write_gwnum (fd, gwdata, ps->alt_x, &sum)) {
			sprintf (buf, "Error writing FFT data named alt_x, state %d, PRP save file %s\n",
				 ps->state, write_save_file_state->base_filename);
			OutputBoth (ps->thread_num, buf);
			OutputBothErrno (ps->thread_num);
			goto err;
		}
	}

	if (ps->state != PRP_STATE_NORMAL && ps->state != PRP_STATE_DCHK_PASS1 && ps->state != PRP_STATE_DCHK_PASS2 &&
	    ps->state != PRP_STATE_GERB_START_BLOCK && ps->state != PRP_STATE_GERB_FINAL_MULT) {
		if (!write_gwnum (fd, gwdata, ps->u0, &sum)) {
			sprintf (buf, "Error writing FFT data named u0, state %d, PRP save file %s\n",
				 ps->state, write_save_file_state->base_filename);
			OutputBoth (ps->thread_num, buf);
			OutputBothErrno (ps->thread_num);
			goto err;
		}
	}

	if (ps->state != PRP_STATE_NORMAL && ps->state != PRP_STATE_DCHK_PASS1 && ps->state != PRP_STATE_DCHK_PASS2 &&
	    ps->state != PRP_STATE_GERB_START_BLOCK) {
		if (!write_gwnum (fd, gwdata, ps->d, &sum)) {
			sprintf (buf, "Error writing FFT data named d, state %d, PRP save file %s\n",
				 ps->state, write_save_file_state->base_filename);
			OutputBoth (ps->thread_num, buf);
			OutputBothErrno (ps->thread_num);
			goto err;
		}
	}

	if (!write_checksum (fd, sum)) goto writeerr;

	closeWriteSaveFile (write_save_file_state, fd);
	return (TRUE);

writeerr:
	sprintf (buf, WRITEFILEERR, write_save_file_state->base_filename);
	OutputBoth (ps->thread_num, buf);
	OutputBothErrno (ps->thread_num);
err:	deleteWriteSaveFile (write_save_file_state, fd);
	return (FALSE);
}

int readPRPSaveFile (
	gwhandle *gwdata,		
	char	*filename,		
	struct work_unit *w,		
	struct prp_state *ps)		
{
	int	fd;
	unsigned long savefile_prp_base;
	uint32_t version, sum, filesum;

	fd = _open (filename, _O_BINARY | _O_RDONLY);
	if (fd <= 0) return (FALSE);

	if (!read_magicnum (fd, PRP_MAGICNUM)) goto err;
	if (!read_header (fd, &version, w, &filesum)) goto err;
	if (version == 0 || version > PRP_VERSION) goto err;

	sum = 0;
	if (!read_long (fd, &ps->error_count, &sum)) goto err;
	if (!read_long (fd, &ps->counter, &sum)) goto err;

	if (version >= 2) {
		unsigned long savefile_two_power_opt;
		if (!read_long (fd, &savefile_prp_base, &sum)) goto err;
		if (!read_long (fd, &ps->units_bit, &sum)) goto err;
		if (!read_long (fd, &savefile_two_power_opt, &sum)) goto err;
		
		if (ps->two_power_opt != savefile_two_power_opt) goto err;
	} else {
		savefile_prp_base = 3;
		ps->units_bit = 0;
		if (ps->two_power_opt) goto err;		
	}

	if (savefile_prp_base != ps->prp_base) goto err;

	if (version >= 3) {
		unsigned long savefile_state, savefile_residue_type, savefile_error_check_type;
		
		if (!read_long (fd, &savefile_residue_type, &sum)) goto err;
		if (savefile_residue_type != ps->residue_type) goto err;
		
		if (!read_long (fd, &savefile_error_check_type, &sum)) goto err;
		if (savefile_error_check_type != ps->error_check_type) goto err;
		if (!read_long (fd, &savefile_state, &sum)) goto err;
		ps->state = savefile_state;
		if (!read_long (fd, &ps->alt_units_bit, &sum)) goto err;
		if (!read_long (fd, &ps->L, &sum)) goto err;
		if (!read_long (fd, &ps->start_counter, &sum)) goto err;
		if (!read_long (fd, &ps->next_mul_counter, &sum)) goto err;
		if (!read_long (fd, &ps->end_counter, &sum)) goto err;
	} else {
		ps->state = PRP_STATE_NORMAL;
		ps->error_check_type = PRP_ERRCHK_NONE;
	}
		
	if (version >= 5) {
		unsigned long savefile_proof_power, savefile_hashlen, savefile_isProbablePrime, savefile_have_res2048;
		if (!read_long (fd, &savefile_proof_power, &sum)) goto err;
		ps->proof_power = savefile_proof_power;
		if (!read_long (fd, &savefile_hashlen, &sum)) goto err;
		ps->hashlen = savefile_hashlen;
		if (!read_long (fd, &savefile_isProbablePrime, &sum)) goto err;
		ps->isProbablePrime = savefile_isProbablePrime;
		if (!read_long (fd, &savefile_have_res2048, &sum)) goto err;
		ps->have_res2048 = savefile_have_res2048;
		if (!read_array (fd, ps->res2048, 512, &sum)) goto err;
		ps->res2048[512] = 0;
		if (!read_array (fd, ps->res64, 16, &sum)) goto err;
		ps->res64[16] = 0;
	} else {
		ps->proof_power = 0;
	}

	if (version >= 6) {
		unsigned long savefile_proof_power_mult, savefile_md5_residues;
		if (!read_long (fd, &savefile_proof_power_mult, &sum)) goto err;
		ps->proof_power_mult = savefile_proof_power_mult;
		if (!read_long (fd, &savefile_md5_residues, &sum)) goto err;
		ps->md5_residues = savefile_md5_residues;
	} else {
		ps->proof_power_mult = 1;
		ps->md5_residues = 0;
	}

	if (version >= 7) {
		unsigned long savefile_proof_version;
		if (!read_long (fd, &savefile_proof_version, &sum)) goto err;
		ps->proof_version = savefile_proof_version;
	} else {
		ps->proof_version = 1;
	}

	if (version == 3 && (ps->state == PRP_STATE_GERB_MID_BLOCK_MULT ||
			     ps->state == PRP_STATE_GERB_END_BLOCK ||
			     ps->state == PRP_STATE_GERB_END_BLOCK_MULT)) goto err;

	if (!read_gwnum (fd, gwdata, ps->x, &sum)) goto err;

	if (version == 3 && ps->state == PRP_STATE_GERB_START_BLOCK) {
		gwcopy (gwdata, ps->x, ps->alt_x);
		ps->alt_units_bit = ps->units_bit;
	}
	else if (ps->state != PRP_STATE_NORMAL && ps->state != PRP_STATE_GERB_MID_BLOCK && ps->state != PRP_STATE_GERB_MID_BLOCK_MULT) {
		if (!read_gwnum (fd, gwdata, ps->alt_x, &sum)) goto err;
	}

	if (ps->state != PRP_STATE_NORMAL && ps->state != PRP_STATE_DCHK_PASS1 && ps->state != PRP_STATE_DCHK_PASS2 &&
	    ps->state != PRP_STATE_GERB_START_BLOCK && ps->state != PRP_STATE_GERB_FINAL_MULT) {
		if (!read_gwnum (fd, gwdata, ps->u0, &sum)) goto err;
	}

	if (ps->state != PRP_STATE_NORMAL && ps->state != PRP_STATE_DCHK_PASS1 && ps->state != PRP_STATE_DCHK_PASS2 &&
	    ps->state != PRP_STATE_GERB_START_BLOCK) {
		if (!read_gwnum (fd, gwdata, ps->d, &sum)) goto err;
	}

	if (filesum != sum) goto err;
	_close (fd);
	return (TRUE);
err:	_close (fd);
	return (FALSE);
}

void good_news_prp (void *arg)
{
	char	buf[800];
	int	i = 0;

	title (MAIN_THREAD_NUM, "New Probable Prime!!!");
	sprintf (buf, "New Probable Prime!!!!  %s is a probable prime!\n", (char *) arg);
	while (WORKERS_ACTIVE && ! WORKERS_STOPPING) {
		if ((i++ & 127) == 0) OutputStr (MAIN_THREAD_NUM, buf);
		flashWindowAndBeep ();
		Sleep (50);
	}
	free (arg);
}

int areTwoPRPValsEqual (
	gwhandle *gwdata,
	unsigned long p,				
	gwnum	val1,			
	unsigned long units_bit1,	
	gwnum	val2,			
	unsigned long units_bit2)	
{
	giant	tmp1, tmp2;
	int	diff, err_code1, err_code2;

	tmp1 = popg (&gwdata->gdata, ((unsigned long) gwdata->bit_length >> 5) + 5);
	err_code1 = gwtogiant (gwdata, val1, tmp1) || isZero (tmp1);
	rotateg (tmp1, p, units_bit1, &gwdata->gdata);
	tmp2 = popg (&gwdata->gdata, ((unsigned long) gwdata->bit_length >> 5) + 5);
	err_code2 = gwtogiant (gwdata, val2, tmp2) || isZero (tmp2);
	rotateg (tmp2, p, units_bit2, &gwdata->gdata);
	diff = gcompg (tmp1, tmp2);
	pushg (&gwdata->gdata, 2);
	return (err_code1 == 0 && err_code2 == 0 && !diff);
}

void basemulg (
	giant	v,			
	struct work_unit *w,		
	unsigned int prp_base,		
	int	power)			
{
	mpz_t	modulus, prp_base_power, tmp;

	if (power == 0) return;

	mpz_init_set_d (modulus, w->k);
	mpz_mul_2exp (modulus, modulus, w->n);
	mpz_add_si (modulus, modulus, w->c);

	mpz_init_set_si (tmp, power);
	mpz_init_set_ui (prp_base_power, prp_base);
	mpz_powm (prp_base_power, prp_base_power, tmp, modulus);

	gtompz (v, tmp);
	mpz_mul (tmp, tmp, prp_base_power);
	mpz_mod (tmp, tmp, modulus);
	mpztog (tmp, v);

	mpz_clear (tmp);
	mpz_clear (prp_base_power);
	mpz_clear (modulus);
}

int isPRPg (
	giant	v,			
	giant	N,			
	struct work_unit *w,		
	unsigned int prp_base,		
	int	prp_residue_type)	
{
	mpz_t	mpz_v, mpz_N, compare_val;
	int	result;

	if (prp_residue_type == PRIMENET_PRP_TYPE_FERMAT) return (isone (v));

	if (prp_residue_type == PRIMENET_PRP_TYPE_SPRP) {
		if (isone (v)) return (TRUE);
		subg (v, N); result = isone (N); addg (v, N);
		return (result);
	}

	mpz_init (mpz_v);
	gtompz (v, mpz_v);
	mpz_init (mpz_N);
	gtompz (N, mpz_N);
	mpz_init (compare_val);

	if (prp_residue_type == PRIMENET_PRP_TYPE_COFACTOR) {
		mpz_t	kbnc, known_factors, reduced_v;
		
		mpz_init (kbnc);
		mpz_ui_pow_ui (kbnc, w->b, w->n);
		mpz_mul_d (kbnc, kbnc, w->k);
		mpz_add_si (kbnc, kbnc, w->c);
		
		mpz_init (known_factors);
		mpz_divexact (known_factors, kbnc, mpz_N);
		mpz_clear (kbnc);
		
		mpz_sub_ui (known_factors, known_factors, 1);
		mpz_set_ui (compare_val, prp_base);
		mpz_powm (compare_val, compare_val, known_factors, mpz_N);
		mpz_clear (known_factors);
		
		mpz_init (reduced_v);
		mpz_mod (reduced_v, mpz_v, mpz_N);
		result = mpz_eq (reduced_v, compare_val);
		mpz_clear (reduced_v);
	}

	else {
		mpz_t	power;

		mpz_init (power);
		if (prp_residue_type == PRIMENET_PRP_TYPE_FERMAT_VAR) mpz_set_si (power, - (w->c - 1));
		else mpz_set_si (power, - (w->c - 1) / 2);
		mpz_set_ui (compare_val, prp_base);
		mpz_powm (compare_val, compare_val, power, mpz_N);
		mpz_clear (power);

		result = mpz_eq (mpz_v, compare_val);
		if (prp_residue_type == PRIMENET_PRP_TYPE_SPRP_VAR && !result) {
			mpz_sub (compare_val, mpz_N, compare_val);	
			result = mpz_eq (mpz_v, compare_val);
		}
	}

	mpz_clear (mpz_v);
	mpz_clear (mpz_N);
	mpz_clear (compare_val);
	return (result);
}

int prp (
	int	thread_num,		
	struct PriorityInfo *sp_info,	
	struct work_unit *w,		
	int	pass)			
{
	struct prp_state ps;
	gwhandle gwdata;
	giant	N, exp, tmp;
	int	pass_to_factor, first_iter_msg, i, res, stop_reason;
	int	echk, near_fft_limit, sleep5;
	int	interim_counter_off_one, interim_mul, mul_final;
	unsigned long explen, final_counter, iters;
	int	slow_iteration_count;
	double	timers[2];
	double	inverse_explen;
	double	reallyminerr = 1.0;
	double	reallymaxerr = 0.0;
	double	best_iteration_time;
	readSaveFileState read_save_file_state; 
	writeSaveFileState write_save_file_state; 
	char	filename[32];
	char	buf[1000], JSONbuf[4000], fft_desc[200];
	unsigned long last_counter = 0xFFFFFFFF;
	int	maxerr_recovery_mode = 0;	
	double	last_maxerr = 0.0;
	double	allowable_maxerr, output_frequency, output_title_frequency;
	char	string_rep[80];
	int	string_rep_truncated;
	int	error_count_messages;
	unsigned long restart_error_count = 0;	
	long	restart_counter = -1;		
	unsigned long excess_squarings;		
	int	proof_next_interim_residue;	
	unsigned long proof_next_interim_residue_iter; 
	unsigned long initiallog2k_iters, initial_nonproof_iters, final_residue_counter;
	int	proof_residue;			
	char	proof_hash[33];			

	memset (&ps, 0, sizeof (ps));
	ps.thread_num = thread_num;

	pass_to_factor = (WELL_BEHAVED_WORK || SEQUENTIAL_WORK != 0) ? 2 : 1;
	if (pass < pass_to_factor) return (0);

	if (w->work_type == WORK_PRP && w->k == 1.0 && w->b == 2 && w->c == -1 && ! IniGetInt (INI_FILE, "SkipTrialFactoring", 0)) {
		struct PriorityInfo sp_info_copy;
		memcpy (&sp_info_copy, sp_info, sizeof (struct PriorityInfo));
		stop_reason = primeFactor (thread_num, &sp_info_copy, w, 1);
		if (stop_reason) return (stop_reason);
	}

	if (w->work_type == WORK_PRP && w->tests_saved > 0.0 && pass != 3) {
		stop_reason = pfactor (thread_num, sp_info, w);
		if (stop_reason) return (stop_reason);
	}

	if (w->work_type == WORK_PRP && w->k == 1.0 && w->b == 2 && w->c == -1 && ! IniGetInt (INI_FILE, "SkipTrialFactoring", 0)) {
		struct PriorityInfo sp_info_copy;
		memcpy (&sp_info_copy, sp_info, sizeof (struct PriorityInfo));
		stop_reason = primeFactor (thread_num, &sp_info_copy, w, 0);
		if (stop_reason) return (stop_reason);
	}

	if (pass == 1) return (0);

#ifdef ONE_HOUR_SELF_TEST
	stop_reason = selfTest (thread_num, sp_info, w);
	if (stop_reason) return (stop_reason);
#endif

	if (w->prp_base) ps.prp_base = w->prp_base;
	else ps.prp_base = IniGetInt (INI_FILE, "PRPBase", 3);

	if (w->prp_residue_type) ps.residue_type = w->prp_residue_type;
	else ps.residue_type = IniGetInt (INI_FILE, "PRPResidueType", PRIMENET_PRP_TYPE_COFACTOR);
	if (w->known_factors == NULL && ps.residue_type == PRIMENET_PRP_TYPE_COFACTOR) ps.residue_type = PRIMENET_PRP_TYPE_FERMAT;

	ps.two_power_opt = (!IniGetInt (INI_FILE, "PRPStraightForward", 0) && w->b == 2 &&
			    (w->known_factors == NULL || ps.residue_type == PRIMENET_PRP_TYPE_COFACTOR));
	if (!ps.two_power_opt) {
		if (ps.residue_type == PRIMENET_PRP_TYPE_FERMAT_VAR) ps.residue_type = PRIMENET_PRP_TYPE_FERMAT;
		else if (ps.residue_type == PRIMENET_PRP_TYPE_SPRP_VAR) ps.residue_type = PRIMENET_PRP_TYPE_SPRP;
	}
	initiallog2k_iters = (int) floor (log2 (w->k));

	interim_counter_off_one = (ps.two_power_opt && w->k == 1.0 && w->c < 0);
	interim_mul = (ps.two_power_opt && w->c < 0);

	if (ps.two_power_opt && (ps.residue_type == PRIMENET_PRP_TYPE_FERMAT || ps.residue_type == PRIMENET_PRP_TYPE_COFACTOR))
		mul_final = w->c - 1;
	else if (ps.two_power_opt && ps.residue_type == PRIMENET_PRP_TYPE_SPRP)
		mul_final = (w->c - 1) / 2;
	else
		mul_final = 0;

	echk = IniGetInt (INI_FILE, "PRPErrorChecking", 1);
	if (echk == 0) ps.error_check_type = PRP_ERRCHK_NONE;
	if (echk == 1) ps.error_check_type = ps.two_power_opt ? PRP_ERRCHK_GERBICZ : PRP_ERRCHK_NONE;
	if (echk == 2) ps.error_check_type = ps.two_power_opt ? PRP_ERRCHK_GERBICZ : PRP_ERRCHK_DBLCHK;
	if (echk == 3) ps.error_check_type = PRP_ERRCHK_DBLCHK;

	ps.proof_version = 2;
	ps.proof_power = 0;
	ps.proof_power_mult = 1;
	
	ps.hashlen = IniSectionGetInt (INI_FILE, SEC_PrimeNet, KEY_ProofHashLength, 64);
	if (ps.hashlen < 32) ps.hashlen = 32;
	if (ps.hashlen > 64) ps.hashlen = 64;
	
	ps.md5_residues = TRUE;

	if (ps.two_power_opt && (ps.residue_type == PRIMENET_PRP_TYPE_FERMAT || ps.residue_type == PRIMENET_PRP_TYPE_COFACTOR)) {
		double	hashlen_adjust = ps.hashlen / 64.0;
		int	best_power = (double) w->n > hashlen_adjust * 414.2e6 ? 11 :
				     (double) w->n > hashlen_adjust * 106.5e6 ? 10 :
				     (double) w->n > hashlen_adjust * 26.6e6 ? 9 :
				     (double) w->n > hashlen_adjust * 6.7e6 ? 8 :
				     (double) w->n > hashlen_adjust * 1.7e6 ? 7 :
				     (double) w->n > hashlen_adjust * 420e3 ? 6 :
				     (double) w->n > hashlen_adjust * 105e3 ? 5 : 0;
		int	power_adjust;

		ps.proof_power = best_power;
		double	disk_space = (double) (1 << ps.proof_power) * (double) (w->n / 8);
		while (disk_space / 1.0e9 > CPU_WORKER_DISK_SPACE) ps.proof_power--, disk_space /= 2.0;

		ps.proof_power = IniGetInt (INI_FILE, "ProofPower", ps.proof_power);
		power_adjust = IniGetInt (INI_FILE, "ProofPowerAdjust", 0);
		if (power_adjust > 2) power_adjust = 2;
		if (power_adjust < -2) power_adjust = -2;
		ps.proof_power += power_adjust;
		if (ps.proof_power < 5) ps.proof_power = 0;
		if (ps.proof_power > best_power) ps.proof_power = best_power;

		if (ps.proof_power == 8 && best_power > 10) ps.proof_power_mult = 2;
		if (ps.proof_power == 7 && best_power > 9) ps.proof_power_mult = 2;
		if (ps.proof_power == 6 && best_power > 8) ps.proof_power_mult = 2;
		if (ps.proof_power == 5 && best_power > 7) ps.proof_power_mult = 3;
		ps.proof_power_mult = IniGetInt (INI_FILE, "ProofPowerMult", ps.proof_power_mult);
		if (ps.proof_power_mult < 1) ps.proof_power_mult = 1;
		if (ps.proof_power_mult > 4) ps.proof_power_mult = 4;

		ps.proof_power = IniSectionGetInt (INI_FILE, SEC_PrimeNet, KEY_ProofPower, ps.proof_power);
		ps.proof_power_mult = IniSectionGetInt (INI_FILE, SEC_PrimeNet, KEY_ProofPowerMult, ps.proof_power_mult);
	}

	tempFileName (w, filename);
	writeSaveFileStateInit (&write_save_file_state, filename, NUM_JACOBI_BACKUP_FILES);

begin:	N = exp = NULL;

	gwinit (&gwdata);
	gwsetmaxmulbyconst (&gwdata, ps.prp_base);
	if (IniGetInt (INI_FILE, "UseLargePages", 0)) gwset_use_large_pages (&gwdata);
	if (HYPERTHREAD_LL) sp_info->normal_work_hyperthreading = TRUE, gwset_will_hyperthread (&gwdata, 2);
	gwset_bench_cores (&gwdata, HW_NUM_CORES);
	gwset_bench_workers (&gwdata, NUM_WORKERS);
	if (ERRCHK) gwset_will_error_check (&gwdata);
	else gwset_will_error_check_near_limit (&gwdata);
	gwset_num_threads (&gwdata, get_worker_num_threads (thread_num, HYPERTHREAD_LL));
	gwset_thread_callback (&gwdata, SetAuxThreadPriority);
	gwset_thread_callback_data (&gwdata, sp_info);
	gwset_minimum_fftlen (&gwdata, w->minimum_fftlen);
	gwset_safety_margin (&gwdata, IniGetFloat (INI_FILE, "ExtraSafetyMargin", 0.0));
	gwset_use_spin_wait (&gwdata, IniGetInt (INI_FILE, "SpinWait", 0));
	res = gwsetup (&gwdata, w->k, w->b, w->n, w->c);

	if (res) {
		char	string_rep[80];
		gw_as_string (string_rep, w->k, w->b, w->n, w->c);
		sprintf (buf, "PRP cannot initialize FFT code for %s, errcode=%d\n", string_rep, res);
		OutputBoth (thread_num, buf);
		gwerror_text (&gwdata, res, buf, sizeof (buf) - 1);
		strcat (buf, "\n");
		OutputBoth (thread_num, buf);
		if (res == GWERROR_TOO_SMALL) return (STOP_WORK_UNIT_COMPLETE);
		return (STOP_FATAL_ERROR);
	}

	stop_reason = setN (thread_num, w, &N);
	if (stop_reason) goto exit;

	if (isone (N)) {
		sprintf (buf, "PRP test of one is not allowed.  Input string was: %s\n", string_rep);
		OutputBoth (thread_num, buf);
		stop_reason = STOP_WORK_UNIT_COMPLETE;
		goto exit;
	}

	{
		mpz_t	tmp;
		int	is_divisible;

		mpz_init (tmp);
		gtompz (N, tmp);
		is_divisible = mpz_divisible_ui_p (tmp, ps.prp_base);
		mpz_clear (tmp);
		if (is_divisible) {
			sprintf (buf, "PRP test of %s aborted -- number is divisible by %u\n", gwmodulo_as_string (&gwdata), ps.prp_base);
			OutputBoth (thread_num, buf);
			stop_reason = STOP_WORK_UNIT_COMPLETE;
			goto exit;
		}
	}

	if (ps.error_check_type == PRP_ERRCHK_NONE)
		set_memory_usage (thread_num, 0, cvt_gwnums_to_mem (&gwdata, 1));
	else
		set_memory_usage (thread_num, 0, cvt_gwnums_to_mem (&gwdata, 1) * 2);

	ps.x = gwalloc (&gwdata);
	if (ps.x == NULL) {
		OutputStr (thread_num, "Error allocating memory for FFT data.\n");
		stop_reason = STOP_OUT_OF_MEM;
		goto exit;
	}
	if (ps.error_check_type == PRP_ERRCHK_GERBICZ || ps.error_check_type == PRP_ERRCHK_DBLCHK) {
		ps.alt_x = gwalloc (&gwdata);
		if (ps.alt_x == NULL) {
			OutputStr (thread_num, "Error allocating memory for error checking.\n");
			stop_reason = STOP_OUT_OF_MEM;
			goto exit;
		}
	}
	if (ps.error_check_type == PRP_ERRCHK_GERBICZ) {
		ps.u0 = gwalloc (&gwdata);
		if (ps.u0 == NULL) {
			OutputStr (thread_num, "Error allocating memory for Gerbicz error checking.\n");
			stop_reason = STOP_OUT_OF_MEM;
			goto exit;
		}
		ps.d = gwalloc (&gwdata);
		if (ps.d == NULL) {
			OutputStr (thread_num, "Error allocating memory for Gerbicz error checking.\n");
			stop_reason = STOP_OUT_OF_MEM;
			goto exit;
		}
	}

	if (w->known_factors == NULL) {
		strcpy (string_rep, gwmodulo_as_string (&gwdata));
		string_rep_truncated = FALSE;
	} else {
		if (strchr (gwmodulo_as_string (&gwdata), '^') == NULL)
			strcpy (string_rep, gwmodulo_as_string (&gwdata));
		else
			sprintf (string_rep, "(%s)", gwmodulo_as_string (&gwdata));
		if (strlen (w->known_factors) < 40) {
			char	*p;
			strcat (string_rep, "/");
			strcat (string_rep, w->known_factors);
			while ((p = strchr (string_rep, ',')) != NULL) *p = '/';
			string_rep_truncated = FALSE;
		} else {
			strcat (string_rep, "/known_factors");
			string_rep_truncated = TRUE;
		}
	}

	sprintf (buf, "PRP %s", string_rep);
	title (thread_num, buf);

	readSaveFileStateInit (&read_save_file_state, thread_num, filename);
	for ( ; ; ) {

		if (! saveFileExists (&read_save_file_state)) {
			
			if (read_save_file_state.a_non_bad_save_file_existed ||
			    (pass == 3 && read_save_file_state.a_save_file_existed)) {
				OutputBoth (thread_num, ALLSAVEBAD_MSG);
				return (0);
			}
			
			ps.counter = 0;
			ps.error_count = 0;
			first_iter_msg = FALSE;
			break;
		}

		if (readPRPSaveFile (&gwdata, read_save_file_state.current_filename, w, &ps)) {
			if (restart_counter < 0 || ps.counter <= (unsigned long) restart_counter) {
				first_iter_msg = TRUE;
				break;
			} else {
				
				read_save_file_state.a_non_bad_save_file_existed = 0;
				_unlink (read_save_file_state.current_filename);
			}
		}

		else
			saveFileBad (&read_save_file_state);
	}

	if (restart_error_count) ps.error_count = restart_error_count;

	gwfft_description (&gwdata, fft_desc);
	strcpy (buf, (ps.counter == 0) ? "Starting " : "Resuming ");
	if (ps.error_check_type == PRP_ERRCHK_GERBICZ) sprintf (buf+strlen(buf), "Gerbicz error-checking ");
	if (ps.error_check_type == PRP_ERRCHK_DBLCHK) sprintf (buf+strlen(buf), "double-checking ");
	if (ps.prp_base != 3) sprintf (buf+strlen(buf), "%u-", ps.prp_base);
	sprintf (buf+strlen(buf), "PRP test of %s using %s\n", string_rep, fft_desc);
	OutputStr (thread_num, buf);

	excess_squarings = 0;
	ps.proof_num_iters = w->n;

	if (ps.proof_power) {
		int	max_emergency_memory;
		double	disk_space, proof_file_size;

		max_emergency_memory = IniGetInt (INI_FILE, "MaxEmergencyMemory", 1024);
		ps.residue_size = divide_rounding_up ((int) ceil(gwdata.bit_length), 8);
		ps.max_emergency_allocs = (int) ((double) max_emergency_memory * 1000000.0 / (double) ps.residue_size);
		if (ps.max_emergency_allocs < 1) ps.max_emergency_allocs = 1;
		if (ps.max_emergency_allocs > (1 << ps.proof_power)) ps.max_emergency_allocs = (1 << ps.proof_power);
		ps.emergency_allocs = (char **) malloc (ps.max_emergency_allocs * sizeof (char *));

		IniGetString (INI_FILE, "ProofResiduesDir", ps.residues_filename, sizeof (ps.residues_filename), NULL);
		DirPlusFilename (ps.residues_filename, filename);
		strcat (ps.residues_filename, ".residues");
		if (ps.counter == 0)
			createProofResiduesFile (&gwdata, &ps, IniGetInt (INI_FILE, "PreallocateDisk", 1));

		if (ps.proof_version == 1)
			excess_squarings = round_up_to_multiple_of (w->n, ps.proof_power_mult << ps.proof_power) - w->n;

		ps.proof_num_iters = (w->n + excess_squarings) / ps.proof_power_mult;
		initial_nonproof_iters = initiallog2k_iters + (w->n + excess_squarings) % ps.proof_power_mult;

		while (ps.counter >= initial_nonproof_iters + ps.proof_num_iters) initial_nonproof_iters += ps.proof_num_iters;
		for (proof_next_interim_residue = 1; ; proof_next_interim_residue++) {
			proof_next_interim_residue_iter = proofResidueIteration (&ps, proof_next_interim_residue);
			if (ps.state == PRP_STATE_GERB_MID_BLOCK_MULT || ps.state == PRP_STATE_GERB_MID_BLOCK_MULT) {
				if (ps.counter + 1 < initial_nonproof_iters + proof_next_interim_residue_iter) break;
			} else if (ps.state == PRP_STATE_DCHK_PASS2) {
				if (ps.end_counter < initial_nonproof_iters + proof_next_interim_residue_iter) break;
			} else {
				if (ps.counter < initial_nonproof_iters + proof_next_interim_residue_iter) break;
			}
		}

		disk_space = (double) (1 << ps.proof_power) * (double) (ps.residue_size + 16);
		proof_file_size = (double) (ps.proof_power + 1) * (double) ps.residue_size * (double) ps.proof_power_mult;
		if (ps.proof_power_mult == 1)
			sprintf (buf, "PRP proof using power=%d and %d-bit hash size.\n", ps.proof_power, ps.hashlen);
		else
			sprintf (buf, "PRP proof using power=%dx%d and %d-bit hash size.\n", ps.proof_power, ps.proof_power_mult, ps.hashlen);
		OutputStr (thread_num, buf);
		sprintf (buf, "Proof requires %.1fGB of temporary disk space and uploading a %.0fMB proof file.\n",
			 disk_space / 1.0e9, proof_file_size / 1.0e6);
		OutputStr (thread_num, buf);
	}

	exp = allocgiant (((unsigned long) (w->n * log2 (w->b) + excess_squarings) >> 5) + 5);
	if (exp == NULL) {
		stop_reason = OutOfMemory (thread_num);
		goto exit;
	}

	if (ps.two_power_opt) {
		int	gerbicz_squarings;
		if (ps.residue_type == PRIMENET_PRP_TYPE_FERMAT ||
		    ps.residue_type == PRIMENET_PRP_TYPE_FERMAT_VAR ||
		    ps.residue_type == PRIMENET_PRP_TYPE_COFACTOR)
			gerbicz_squarings = w->n;
		else
			gerbicz_squarings = w->n - 1;
		ultog (2, exp);
		power (exp, gerbicz_squarings + excess_squarings);
		dblmulg (w->k, exp);
	}

	else if (ps.residue_type == PRIMENET_PRP_TYPE_COFACTOR) {
		ultog (w->b, exp);
		power (exp, w->n);
		dblmulg (w->k, exp);
		iaddg (w->c, exp);
		iaddg (-1, exp);
	}

	else {
		gtog (N, exp);
		iaddg (-1, exp);
	}

	explen = bitlen (exp);
	final_counter = explen - 1;
	final_residue_counter = final_counter - excess_squarings;

	best_iteration_time = 1.0e50;
	slow_iteration_count = 0;

	clear_timers (timers, sizeof (timers) / sizeof (timers[0]));

	strcpy (w->stage, "PRP");
	inverse_explen = 1.0 / (double) final_counter;
	w->pct_complete = (double) ps.counter * inverse_explen;
	calc_output_frequencies (&gwdata, &output_frequency, &output_title_frequency);

	near_fft_limit = exponent_near_fft_limit (&gwdata);

	allowable_maxerr = IniGetFloat (INI_FILE, "MaxRoundoffError", (float) (near_fft_limit ? 0.421875 : 0.40625));

	if (ps.counter == 0) {
		
		if (w->k == 1.0 && w->b == 2 && w->n > 1000 && w->c == -1) {
			unsigned long word, bit_in_word;
			
			srand ((unsigned) time (NULL));
			ps.units_bit = (rand () << 16) + rand ();
			if (CPU_FLAGS & CPU_RDTSC) { uint32_t hi,lo; rdtsc(&hi,&lo); ps.units_bit += lo; }
			
			ps.units_bit = IniGetInt (INI_FILE, "InitialShiftCount", ps.units_bit);
			
			ps.units_bit = ps.units_bit % (w->n - 64);
			
			dbltogw (&gwdata, 0.0, ps.x);
			bitaddr (&gwdata, ps.units_bit, &word, &bit_in_word);
			set_fft_value (&gwdata, ps.x, word, (ps.prp_base << bit_in_word) & 0xFFFFFF);
			set_fft_value (&gwdata, ps.x, word+1, ps.prp_base >> (24 - bit_in_word));
		} else {
			ps.units_bit = 0;
			dbltogw (&gwdata, (double) ps.prp_base, ps.x);
		}

		if (ps.error_check_type == PRP_ERRCHK_NONE) {
			ps.state = PRP_STATE_NORMAL;
			ps.start_counter = 0;			
			ps.end_counter = 0;			
		}

		if (ps.error_check_type == PRP_ERRCHK_DBLCHK) {
			int	compare_interval = IniGetInt (INI_FILE, "PRPDoublecheckCompareInterval", 100000);
			int	iters_left = final_counter;
			iters_left = one_based_modulo (iters_left, ps.proof_num_iters);	
			if (compare_interval > iters_left) compare_interval = iters_left;
			ps.state = PRP_STATE_DCHK_PASS1;
			ps.start_counter = 0;
			ps.end_counter = compare_interval;
			if (ps.units_bit == 0) {
				gwcopy (&gwdata, ps.x, ps.alt_x);
				ps.alt_units_bit = 0;
			} else {
				gwadd3o (&gwdata, ps.x, ps.x, ps.alt_x, GWADD_FORCE_NORMALIZE); 
				ps.alt_units_bit = ps.units_bit + 1;
				if (ps.alt_units_bit >= w->n) ps.alt_units_bit -= w->n;
			}
		}

		if (ps.error_check_type == PRP_ERRCHK_GERBICZ) {
			
			gwcopy (&gwdata, ps.x, ps.alt_x);
			ps.alt_units_bit = ps.units_bit;
			
			if (w->k != 1.0) {
				ps.state = PRP_STATE_DCHK_PASS1;
				ps.start_counter = 0;
				ps.end_counter = initiallog2k_iters;
			} else {
				ps.state = PRP_STATE_GERB_START_BLOCK;
			}
		}
	}

	error_count_messages = IniGetInt (INI_FILE, "ErrorCountMessages", 3);
	if (ps.error_check_type != PRP_ERRCHK_NONE) error_count_messages |= 0x8000;

#ifdef CHECK_ITER
{giant t1, t2;
t1 = popg (&gwdata.gdata, ((unsigned long) gwdata.bit_length >> 4) + 5);
t2 = popg (&gwdata.gdata, ((unsigned long) gwdata.bit_length >> 4) + 5);
(void) gwtogiant (&gwdata, ps.x, t1);
rotateg (t1, w->n, ps.units_bit, &gwdata.gdata);
#endif
	gwsetmulbyconst (&gwdata, ps.prp_base);
	iters = 0;
	while (ps.counter < final_counter) {
		gwnum	x;			
		unsigned long *units_bit;	
		int	saving, saving_highly_reliable, sending_residue, interim_residue, interim_file;
		int	actual_frequency;

		if (ps.state == PRP_STATE_GERB_START_BLOCK) {
			int	iters_left = final_counter - ps.counter;
			iters_left = one_based_modulo (iters_left, ps.proof_num_iters);	
			if (iters_left < 49) {
				ps.state = PRP_STATE_DCHK_PASS1;
				ps.start_counter = ps.counter;
				ps.end_counter = ps.counter + iters_left;
				
				if (ps.alt_units_bit && ps.end_counter == final_counter) {
					gwadd3o (&gwdata, ps.alt_x, ps.alt_x, ps.alt_x, GWADD_FORCE_NORMALIZE); 
					ps.alt_units_bit = ps.alt_units_bit + 1;
					if (ps.alt_units_bit >= w->n) ps.alt_units_bit -= w->n;
				}
			} else {
				int	gerbicz_block_size;
				double	adjustment;
				ps.state = PRP_STATE_GERB_MID_BLOCK;
				adjustment = IniGetFloat (INI_FILE, "PRPGerbiczCompareIntervalAdj", 1.0);
				if (adjustment < 0.001 || adjustment > 1.0) adjustment = 0.5;
				gerbicz_block_size = (int) (adjustment * IniGetInt (INI_FILE, "PRPGerbiczCompareInterval", 1000000));
				if (gerbicz_block_size < 25) gerbicz_block_size = 25;
				if (gerbicz_block_size > iters_left) gerbicz_block_size = iters_left;
				ps.L = (unsigned long) sqrt ((double) gerbicz_block_size);
				ps.start_counter = ps.counter;
				ps.next_mul_counter = ps.counter + ps.L;
				ps.end_counter = ps.counter + ps.L * ps.L;
				gwswap (ps.alt_x, ps.u0);		
				gwcopy (&gwdata, ps.x, ps.d);		
				if (IniGetInt (INI_FILE, "GerbiczVerbosity", 1) > 1) {
					sprintf (buf, "Start Gerbicz block of size %ld at iteration %ld.\n", ps.L * ps.L, ps.start_counter+1);
					OutputBoth (thread_num, buf);
				}
			}
		}

		stop_reason = stopCheck (thread_num);
		saving = stop_reason || ps.counter == last_counter-8 || ps.counter == last_counter || testSaveFilesFlag (thread_num);
		saving_highly_reliable = FALSE;

		echk = ERRCHK || ps.counter < 50 || ps.counter >= final_counter-50 || saving ||
		       (ps.error_check_type == PRP_ERRCHK_NONE && (near_fft_limit || ((ps.counter & 127) == 0)));
		gw_clear_maxerr (&gwdata);

		proof_residue = ps.proof_power &&
				(ps.state == PRP_STATE_NORMAL || ps.state == PRP_STATE_DCHK_PASS1 || ps.state == PRP_STATE_GERB_MID_BLOCK) &&
				ps.counter > initial_nonproof_iters &&
				ps.counter - initial_nonproof_iters + 1 == proof_next_interim_residue_iter;

		sending_residue = ps.proof_power == 0 &&
				  (ps.state == PRP_STATE_NORMAL || ps.state == PRP_STATE_DCHK_PASS1 || ps.state == PRP_STATE_GERB_MID_BLOCK) &&
				  ps.counter > 0 &&
				  ((ps.counter+1-interim_counter_off_one) == 500000 ||
				   ((ps.counter+1-interim_counter_off_one) % 5000000 == 0 && IniGetInt (INI_FILE, "SendInterimResidues", 1)));
		interim_residue = INTERIM_RESIDUES &&
				  (ps.state == PRP_STATE_NORMAL || ps.state == PRP_STATE_DCHK_PASS1 || ps.state == PRP_STATE_GERB_MID_BLOCK) &&
				  (ps.counter > 0 && (ps.counter+1-interim_counter_off_one) % INTERIM_RESIDUES == 0);
		interim_file = INTERIM_FILES &&
			       (ps.state == PRP_STATE_NORMAL || ps.state == PRP_STATE_DCHK_PASS2 || ps.state == PRP_STATE_GERB_MID_BLOCK) &&
			       (ps.counter > 0 && (ps.counter+1) % INTERIM_FILES == 0);

		timers[1] = 0.0;
		start_timer (timers, 1);

		if (ps.state == PRP_STATE_GERB_MID_BLOCK_MULT) {
			gwstartnextfft (&gwdata, 0);		
			gwsetnormroutine (&gwdata, 0, 1, 0);	
			gwsafemul (&gwdata, ps.x, ps.d);	
			x = ps.d;				
		} else if (ps.state == PRP_STATE_GERB_END_BLOCK_MULT) {
			gwstartnextfft (&gwdata, 0);		
			gwsetnormroutine (&gwdata, 0, 1, 0);	
			gwmul (&gwdata, ps.u0, ps.alt_x);	
			x = ps.alt_x;				
		} else if (ps.state == PRP_STATE_GERB_FINAL_MULT) {
			gwcopy (&gwdata, ps.x, ps.u0);		
			gwstartnextfft (&gwdata, 0);		
			gwsetnormroutine (&gwdata, 0, 1, 0);	
			gwsafemul (&gwdata, ps.u0, ps.d);	
			x = ps.d;				
		}

		else {

			if (ps.state == PRP_STATE_NORMAL || ps.state == PRP_STATE_DCHK_PASS1 || ps.state == PRP_STATE_GERB_MID_BLOCK) {
				x = ps.x;
				units_bit = &ps.units_bit;
			} else {			
				x = ps.alt_x;
				units_bit = &ps.alt_units_bit;
			}

			gwstartnextfft (&gwdata,
					!saving && !maxerr_recovery_mode && ps.counter != ps.next_mul_counter-1 &&
					ps.counter != ps.end_counter-1 && ps.counter != final_residue_counter-1 &&
					((ps.counter > 35 && ps.counter < explen-35) || (ps.counter > explen+35 && ps.counter < final_counter)) &&
					!proof_residue && !sending_residue && !interim_residue && !interim_file);

#ifdef CHECK_ITER
squareg (t1);
if (bitval (exp, final_counter-ps.counter-1)) ulmulg (ps.prp_base, t1);
specialmodg (&gwdata, t1);
if (w->known_factors) modg (N, t1);
gwstartnextfft (&gwdata, 0);
echk=1;
#endif
			if (bitval (exp, final_counter-ps.counter-1)) {
				gwsetnormroutine (&gwdata, 0, echk, 1);
			} else {
				gwsetnormroutine (&gwdata, 0, echk, 0);
			}
			if (maxerr_recovery_mode && ps.counter == last_counter) {
				gwsquare_carefully (&gwdata, x);
				maxerr_recovery_mode = 0;
				last_counter = 0xFFFFFFFF;
				echk = 0;
			} else if (ps.counter < 30 || (ps.counter >= explen - 30 && ps.counter <= explen + 30))
				gwsquare_carefully (&gwdata, x);
			else
				gwsquare (&gwdata, x);

			*units_bit <<= 1;
			if (*units_bit >= w->n) *units_bit -= w->n;

#ifdef CHECK_ITER
(void) gwtogiant (&gwdata, ps.x, t2);
rotateg (t2, w->n, ps.units_bit, &gwdata.gdata);
if (w->known_factors) modg (N, t2);
if (gcompg (t1, t2) != 0)
OutputStr (thread_num, "Iteration failed.\n");

#endif
		}

#ifdef INTRODUCE_ERRORS
		if ((rand () & 0x7FFF) == 134)  
			*x += 5.0;
#endif

		end_timer (timers, 1);
		timers[0] += timers[1];
		iters++;

		if (echk) {
			if (ps.counter > 30 && gw_get_maxerr (&gwdata) < reallyminerr) reallyminerr = gw_get_maxerr (&gwdata);
			if (gw_get_maxerr (&gwdata) > reallymaxerr) reallymaxerr = gw_get_maxerr (&gwdata);
		}

		if (gw_test_illegal_sumout (&gwdata)) {
			sprintf (buf, ERRMSG0, ps.counter+1, final_counter, ERRMSG1A);
			OutputBoth (thread_num, buf);
			inc_error_count (2, &ps.error_count);
			last_counter = ps.counter;		
			restart_counter = -1;			
			sleep5 = TRUE;
			goto restart;
		}

		if (echk && gw_get_maxerr (&gwdata) > allowable_maxerr) {
			if (ps.counter == last_counter && gw_get_maxerr (&gwdata) == last_maxerr) {
				OutputBoth (thread_num, ERROK);
				inc_error_count (3, &ps.error_count);
				gw_clear_error (&gwdata);
				OutputBoth (thread_num, ERRMSG5);
				maxerr_recovery_mode = 1;
				restart_counter = ps.counter;		
				sleep5 = FALSE;
				goto restart;
			} else {
				char	msg[100];
				sprintf (msg, ERRMSG1C, gw_get_maxerr (&gwdata), allowable_maxerr);
				sprintf (buf, ERRMSG0, ps.counter+1, final_counter, msg);
				OutputBoth (thread_num, buf);
				inc_error_count (1, &ps.error_count);
				if (ps.error_check_type == PRP_ERRCHK_NONE ||
				    gw_get_maxerr (&gwdata) > IniGetFloat (INI_FILE, "RoundoffRollbackError", (float) 0.475)) {
					last_counter = ps.counter;
					last_maxerr = gw_get_maxerr (&gwdata);
					restart_counter = ps.counter;		
					sleep5 = FALSE;
					goto restart;
				}
			}
		}

		ps.counter++;
		w->pct_complete = (double) ps.counter * inverse_explen;
		if (ps.error_check_type == PRP_ERRCHK_DBLCHK) {
			unsigned long true_counter;
			true_counter = ps.start_counter + ((ps.counter - ps.start_counter) >> 1);
			if (ps.state == PRP_STATE_DCHK_PASS2) true_counter += ((ps.end_counter - ps.start_counter) >> 1);
			w->pct_complete = (double) true_counter * inverse_explen;
		}

		actual_frequency = (int) (ITER_OUTPUT * output_title_frequency);
		if (actual_frequency < 1) actual_frequency = 1;
		if (ps.counter % actual_frequency == 0 || first_iter_msg) {
			sprintf (buf, "%.*f%% of PRP %s", (int) PRECISION, trunc_percent (w->pct_complete), string_rep);
			title (thread_num, buf);
		}

		actual_frequency = (int) (ITER_OUTPUT * output_frequency);
		if (actual_frequency < 1) actual_frequency = 1;
		if ((ps.counter % actual_frequency == 0 && ps.state != PRP_STATE_GERB_MID_BLOCK_MULT &&
		     ps.state != PRP_STATE_GERB_END_BLOCK && ps.state != PRP_STATE_GERB_END_BLOCK_MULT &&
		     ps.state != PRP_STATE_GERB_FINAL_MULT) || first_iter_msg) {
			sprintf (buf, "Iteration: %ld / %ld [%.*f%%]",
				 ps.counter, final_counter, (int) PRECISION, trunc_percent (w->pct_complete));
			
			if ((error_count_messages & 0xFF) == 1)
				make_error_count_message (ps.error_count, error_count_messages, buf + strlen (buf),
							  (int) (sizeof (buf) - strlen (buf)));
			
			if (first_iter_msg) {
				strcat (buf, ".\n");
				clear_timer (timers, 0);
				first_iter_msg = FALSE;
			}
			
			else if (!CLASSIC_OUTPUT) {
				double speed;
				
				if ((OUTPUT_ROUNDOFF || ERRCHK) && reallymaxerr >= 0.001) {
					sprintf (buf+strlen(buf), ", roundoff: %5.3f", reallymaxerr);
					if (!CUMULATIVE_ROUNDOFF) reallyminerr = 1.0, reallymaxerr = 0.0;
				}
				
				speed = timer_value (timers, 0) / (double) iters;
				sprintf (buf+strlen(buf), ", ms/iter: %6.3f", speed * 1000.0);
				clear_timer (timers, 0);
				iters = 0;
				
				formatETA ((final_counter - ps.counter) * speed, buf+strlen(buf));
				strcat (buf, "\n");
			}
			
			else {
				
				if (ERRCHK && ps.counter > 30) {
					sprintf (buf+strlen(buf), ".  Round off: %10.10f to %10.10f", reallyminerr, reallymaxerr);
					if (!CUMULATIVE_ROUNDOFF) reallyminerr = 1.0, reallymaxerr = 0.0;
				}
				if (CUMULATIVE_TIMING) {
					strcat (buf, ".  Total time: ");
					print_timer (timers, 0, buf, TIMER_NL);
				} else {
					strcat (buf, ".  Per iteration time: ");
					divide_timer (timers, 0, iters);
					print_timer (timers, 0, buf, TIMER_NL | TIMER_CLR);
					iters = 0;
				}
			}
			OutputStr (thread_num, buf);

			if ((error_count_messages & 0xFF) >= 2 &&
			    make_error_count_message (ps.error_count, error_count_messages, buf, sizeof (buf)))
				OutputStr (thread_num, buf);
		}

		if ((ps.counter % ITER_OUTPUT_RES == 0 && ps.state != PRP_STATE_GERB_MID_BLOCK_MULT &&
		     ps.state != PRP_STATE_GERB_END_BLOCK && ps.state != PRP_STATE_GERB_END_BLOCK_MULT &&
		     ps.state != PRP_STATE_GERB_FINAL_MULT) || (NO_GUI && stop_reason)) {
			sprintf (buf, "Iteration %ld / %ld\n", ps.counter, final_counter);
			writeResults (buf);
		}

		if (proof_residue) {
			tmp = popg (&gwdata.gdata, ((unsigned long) gwdata.bit_length >> 5) + 5);
			if (gwtogiant (&gwdata, x, tmp) || tmp->sign <= 0) {
				pushg (&gwdata.gdata, 1);
				OutputBoth (thread_num, ERRMSG8);
				inc_error_count (2, &ps.error_count);
				last_counter = ps.counter;		
				restart_counter = -1;			
				sleep5 = TRUE;
				goto restart;
			}
			rotateg (tmp, w->n, *units_bit, &gwdata.gdata);
			outputProofResidue (&gwdata, &ps, proof_next_interim_residue, tmp);
			proof_next_interim_residue++;
			proof_next_interim_residue_iter = proofResidueIteration (&ps, proof_next_interim_residue);
			pushg (&gwdata.gdata, 1);
		}

		if (ps.counter == final_residue_counter) {
			if (ps.state == PRP_STATE_NORMAL || ps.state == PRP_STATE_GERB_MID_BLOCK || ps.state == PRP_STATE_DCHK_PASS1) {
				tmp = popg (&gwdata.gdata, ((unsigned long) gwdata.bit_length >> 5) + 5);
				if (gwtogiant (&gwdata, ps.x, tmp)) {
					pushg (&gwdata.gdata, 1);
					OutputBoth (thread_num, ERRMSG8);
					inc_error_count (2, &ps.error_count);
					restart_counter = -1;			
					sleep5 = TRUE;
					goto restart;
				}
				rotateg (tmp, w->n, ps.units_bit, &gwdata.gdata);
				if (mul_final) basemulg (tmp, w, ps.prp_base, mul_final);
				if (w->known_factors && ps.residue_type != PRIMENET_PRP_TYPE_COFACTOR) modg (N, tmp);
				ps.isProbablePrime = isPRPg (tmp, N, w, ps.prp_base, ps.residue_type);
				if (!ps.isProbablePrime) {
					sprintf (ps.res64, "%08lX%08lX", (unsigned long) (tmp->sign > 1 ? tmp->n[1] : 0), (unsigned long) tmp->n[0]);
					ps.have_res2048 = (tmp->sign > 64);
					if (ps.have_res2048) {
						int i;
						for (i = 63; i >= 0; i--) sprintf (ps.res2048+504-i*8, "%08lX", (unsigned long) tmp->n[i]);
					}
				}
				pushg (&gwdata.gdata, 1);
			}

			if (ps.state == PRP_STATE_DCHK_PASS2) {
				int	alt_match, alt_isProbablePrime;
				tmp = popg (&gwdata.gdata, ((unsigned long) gwdata.bit_length >> 5) + 5);
				if (gwtogiant (&gwdata, ps.alt_x, tmp)) {
					pushg (&gwdata.gdata, 1);
					OutputBoth (thread_num, ERRMSG8);
					inc_error_count (2, &ps.error_count);
					restart_counter = -1;			
					sleep5 = TRUE;
					goto restart;
				}
				rotateg (tmp, w->n, ps.alt_units_bit, &gwdata.gdata);
				if (mul_final) basemulg (tmp, w, ps.prp_base, mul_final);
				if (w->known_factors && ps.residue_type != PRIMENET_PRP_TYPE_COFACTOR) modg (N, tmp);
				alt_isProbablePrime = isPRPg (tmp, N, w, ps.prp_base, ps.residue_type);
				alt_match = (ps.isProbablePrime == alt_isProbablePrime);
				if (alt_match && !alt_isProbablePrime) {
					char	alt_res64[17];
					sprintf (alt_res64, "%08lX%08lX", (unsigned long) (tmp->sign > 1 ? tmp->n[1] : 0), (unsigned long) tmp->n[0]);
					alt_match = !strcmp (ps.res64, alt_res64);
				}
				if (!alt_match) {
					pushg (&gwdata.gdata, 1);
					OutputBoth (thread_num, ERRMSG8);
					inc_error_count (2, &ps.error_count);
					restart_counter = -1;			
					sleep5 = TRUE;
					goto restart;
				}
				pushg (&gwdata.gdata, 1);
			}
		}

		if (ps.state == PRP_STATE_DCHK_PASS1) {
			if (ps.counter < ps.end_counter);		
			else if (ps.counter == ps.end_counter) {	
				ps.state = PRP_STATE_DCHK_PASS2;
				ps.counter = ps.start_counter;
			} else {					
				OutputBoth (thread_num, ERRMSG9);
				inc_error_count (6, &ps.error_count);
				restart_counter = -1;			
				sleep5 = FALSE;
				goto restart;
			}
		}
		if (ps.state == PRP_STATE_DCHK_PASS2) {
			if (ps.counter < ps.end_counter);		
			else if (ps.counter == ps.end_counter) {	
				if (!areTwoPRPValsEqual (&gwdata, w->n, ps.x, ps.units_bit, ps.alt_x, ps.alt_units_bit)) {
					sprintf (buf, ERRMSG6, ps.start_counter);
					OutputBoth (thread_num, buf);
					inc_error_count (7, &ps.error_count);
					restart_counter = ps.start_counter;		
					sleep5 = FALSE;
					goto restart;
				}
				
				if (ps.error_check_type == PRP_ERRCHK_DBLCHK) {
					int	compare_interval = IniGetInt (INI_FILE, "PRPDoublecheckCompareInterval", 100000);
					int	iters_left = final_counter - ps.counter;
					iters_left = one_based_modulo (iters_left, ps.proof_num_iters);	
					if (compare_interval > iters_left) compare_interval = iters_left;
					ps.state = PRP_STATE_DCHK_PASS1;
					ps.start_counter = ps.counter;
					ps.end_counter = ps.start_counter + compare_interval;
				}
				
				else {
					ps.state = PRP_STATE_GERB_START_BLOCK;
				}
				
				if (final_counter - ps.counter >= 1000 || ps.error_count > 0) {
					saving = TRUE;
					saving_highly_reliable = TRUE;
				}
			} else {					
				OutputBoth (thread_num, ERRMSG9);
				inc_error_count (6, &ps.error_count);
				restart_counter = -1;			
				sleep5 = FALSE;
				goto restart;
			}
		}

		if (ps.state == PRP_STATE_GERB_MID_BLOCK) {
			if (ps.counter < ps.next_mul_counter);		
			else if (ps.counter == ps.end_counter) {	
				if (IniGetInt (INI_FILE, "GerbiczVerbosity", 1) > 1) OutputStr (thread_num, "Start Gerbicz error check.\n");
				
				gwcopy (&gwdata, ps.d, ps.alt_x);	
				ps.state = PRP_STATE_GERB_END_BLOCK;	
				ps.counter -= ps.L;			
			} else if (ps.counter == ps.next_mul_counter) {	
				
				ps.state = PRP_STATE_GERB_MID_BLOCK_MULT;
				ps.counter -= 1;
			} else {					
				OutputBoth (thread_num, ERRMSG9);
				inc_error_count (6, &ps.error_count);
				restart_counter = -1;			
				sleep5 = FALSE;
				goto restart;
			}
		}

		else if (ps.state == PRP_STATE_GERB_MID_BLOCK_MULT) {
			if (ps.counter < ps.end_counter) {		
				ps.state = PRP_STATE_GERB_MID_BLOCK;
				ps.next_mul_counter += ps.L;
			} else {					
				OutputBoth (thread_num, ERRMSG9);
				inc_error_count (6, &ps.error_count);
				restart_counter = -1;			
				sleep5 = FALSE;
				goto restart;
			}
		}

		else if (ps.state == PRP_STATE_GERB_END_BLOCK) {
			if (ps.counter < ps.end_counter);		
			else if (ps.counter == ps.end_counter) {	
				ps.state = PRP_STATE_GERB_END_BLOCK_MULT;
				ps.counter -= 1;
			} else {					
				OutputBoth (thread_num, ERRMSG9);
				inc_error_count (6, &ps.error_count);
				restart_counter = -1;			
				sleep5 = FALSE;
				goto restart;
			}
		}

		else if (ps.state == PRP_STATE_GERB_END_BLOCK_MULT) {
			if (ps.counter == ps.end_counter) {		
				ps.state = PRP_STATE_GERB_FINAL_MULT;
				ps.counter -= 1;
			} else {					
				OutputBoth (thread_num, ERRMSG9);
				inc_error_count (6, &ps.error_count);
				restart_counter = -1;			
				sleep5 = FALSE;
				goto restart;
			}
		}

		else if (ps.state == PRP_STATE_GERB_FINAL_MULT) {
			double	gerbicz_block_size_adjustment;
			
			gerbicz_block_size_adjustment = IniGetFloat (INI_FILE, "PRPGerbiczCompareIntervalAdj", 1.0);
			if (gerbicz_block_size_adjustment < 0.001 || gerbicz_block_size_adjustment > 1.0) gerbicz_block_size_adjustment = 0.5;
			
			if (!areTwoPRPValsEqual (&gwdata, w->n, ps.alt_x, 0, ps.d, 0)) {
				sprintf (buf, ERRMSG7, ps.start_counter);
				OutputBoth (thread_num, buf);
				gerbicz_block_size_adjustment *= 0.25;		
				if (gerbicz_block_size_adjustment < 0.001) gerbicz_block_size_adjustment = 0.001;
				IniWriteFloat (INI_FILE, "PRPGerbiczCompareIntervalAdj", (float) gerbicz_block_size_adjustment);
				inc_error_count (7, &ps.error_count);
				restart_counter = ps.start_counter;		
				sleep5 = FALSE;
				goto restart;
			}
			if (IniGetInt (INI_FILE, "GerbiczVerbosity", 1)) {
				sprintf (buf, "Gerbicz error check passed at iteration %ld.\n", ps.counter);
				OutputStr (thread_num, buf);
			}
			gerbicz_block_size_adjustment *= 1.0473;		
			if (gerbicz_block_size_adjustment > 1.0) gerbicz_block_size_adjustment = 1.0;
			IniWriteFloat (INI_FILE, "PRPGerbiczCompareIntervalAdj", (float) gerbicz_block_size_adjustment);
			
			ps.state = PRP_STATE_GERB_START_BLOCK;
			gwswap (ps.alt_x, ps.u0);
			ps.alt_units_bit = ps.units_bit;
			
			if (final_counter - ps.counter >= 1000 || gerbicz_block_size_adjustment < 1.0) {
				saving = TRUE;
				saving_highly_reliable = TRUE;
			}
		}

		if ((ps.state == PRP_STATE_NORMAL || ps.state == PRP_STATE_DCHK_PASS1 || ps.state == PRP_STATE_GERB_START_BLOCK) &&
		    ps.proof_power && ps.counter != final_counter && ps.counter == initial_nonproof_iters + ps.proof_num_iters) {
			int proof_number = (ps.counter - initiallog2k_iters) / ps.proof_num_iters;
			stop_reason = generateProofFile (&gwdata, &ps, w, proof_number, proof_hash);
			if (stop_reason) goto exit;
			initial_nonproof_iters += ps.proof_num_iters;
			proof_next_interim_residue = 1;
			proof_next_interim_residue_iter = proofResidueIteration (&ps, proof_next_interim_residue);
		}

		if (saving) {
			if (writePRPSaveFile (&gwdata, &write_save_file_state, w, &ps)) {
				
				if (saving_highly_reliable) setWriteSaveFileSpecial (&write_save_file_state);
			}
		}

		if (stop_reason) {
			sprintf (buf, "Stopping PRP test of %s at iteration %ld [%.*f%%]\n",
				 string_rep, ps.counter, (int) PRECISION, trunc_percent (w->pct_complete));
			OutputStr (thread_num, buf);
			goto exit;
		}

		if (sending_residue && w->assignment_uid[0]) {
			struct primenetAssignmentProgress pkt;
			memset (&pkt, 0, sizeof (pkt));
			strcpy (pkt.computer_guid, COMPUTER_GUID);
			pkt.cpu_num = thread_num;
			strcpy (pkt.assignment_uid, w->assignment_uid);
			strcpy (pkt.stage, w->stage);
			pkt.pct_complete = w->pct_complete * 100.0;
			pkt.end_date = (unsigned long) work_estimate (thread_num, w);
			pkt.next_update = (uint32_t) (DAYS_BETWEEN_CHECKINS * 86400.0);
			pkt.fftlen = w->fftlen;
			pkt.iteration = ps.counter - interim_counter_off_one;
			tmp = popg (&gwdata.gdata, ((unsigned long) gwdata.bit_length >> 5) + 5);
			if (gwtogiant (&gwdata, x, tmp)) {
				pushg (&gwdata.gdata, 1);
				OutputBoth (thread_num, ERRMSG8);
				inc_error_count (2, &ps.error_count);
				last_counter = ps.counter;		
				restart_counter = -1;			
				sleep5 = TRUE;
				goto restart;
			}
			rotateg (tmp, w->n, *units_bit, &gwdata.gdata);
			if (interim_mul) basemulg (tmp, w, ps.prp_base, -1);
			if (w->known_factors && ps.residue_type != PRIMENET_PRP_TYPE_COFACTOR) modg (N, tmp);
			sprintf (pkt.residue, "%08lX%08lX", (unsigned long) tmp->n[1], (unsigned long) tmp->n[0]);
			sprintf (pkt.error_count, "%08lX", ps.error_count);
			spoolMessage (-PRIMENET_ASSIGNMENT_PROGRESS, &pkt);
			pushg (&gwdata.gdata, 1);
		}

		if (interim_residue) {
			tmp = popg (&gwdata.gdata, ((unsigned long) gwdata.bit_length >> 5) + 5);
			if (gwtogiant (&gwdata, x, tmp)) {
				pushg (&gwdata.gdata, 1);
				OutputBoth (thread_num, ERRMSG8);
				inc_error_count (2, &ps.error_count);
				last_counter = ps.counter;		
				restart_counter = -1;			
				sleep5 = TRUE;
				goto restart;
			}
			rotateg (tmp, w->n, *units_bit, &gwdata.gdata);
			if (interim_mul) basemulg (tmp, w, ps.prp_base, -1);
			if (w->known_factors && ps.residue_type != PRIMENET_PRP_TYPE_COFACTOR) modg (N, tmp);
			sprintf (buf, "%s interim PRP residue %08lX%08lX at iteration %ld\n",
				 string_rep, (unsigned long) tmp->n[1], (unsigned long) tmp->n[0],
				 ps.counter - interim_counter_off_one);
			OutputBoth (thread_num, buf);
			pushg (&gwdata.gdata, 1);
		}

		if (interim_file) {
			char	interimfile[32];
			writeSaveFileState state;
			sprintf (interimfile, "%s.%03ld", filename, ps.counter / INTERIM_FILES);
			writeSaveFileStateInit (&state, interimfile, 0);
			state.num_ordinary_save_files = 99;
			writePRPSaveFile (&gwdata, &state, w, &ps);
		}

		if (HYPERTHREADING_BACKOFF && explen > 1500000) {
			if (timers[1] < best_iteration_time)
				best_iteration_time = timers[1];
			if (timers[1] > 1.40 * best_iteration_time) {
				if (slow_iteration_count == 10) {
					sprintf (buf, "Pausing %lu seconds.\n", HYPERTHREADING_BACKOFF);
					OutputStr (thread_num, buf);
					Sleep (HYPERTHREADING_BACKOFF * 1000);
				}
				slow_iteration_count++;
			} else
				slow_iteration_count = 0;
		}
	}
#ifdef CHECK_ITER
pushg(&gwdata.gdata, 2);}
#endif

	if (ps.state != PRP_STATE_NORMAL && ps.state != PRP_STATE_DCHK_PASS1 && ps.state != PRP_STATE_GERB_START_BLOCK) {
		OutputBoth (thread_num, ERRMSG9);
		inc_error_count (6, &ps.error_count);
		restart_counter = -1;			
		sleep5 = FALSE;
		goto restart;
	}

	gwfree (&gwdata, ps.u0);
	gwfree (&gwdata, ps.d);
	gwfree (&gwdata, ps.alt_x);
	gwfree (&gwdata, ps.x);

	if (ps.proof_power) {
		stop_reason = generateProofFile (&gwdata, &ps, w, ps.proof_power_mult, proof_hash);
		if (stop_reason) goto exit;
	}

	if (ps.isProbablePrime) {
		sprintf (buf, "%s is a probable prime", string_rep);
		if (ps.prp_base != 3) sprintf (buf+strlen(buf), " (%u-PRP)", ps.prp_base);
		strcat (buf, "!");
	} else {
		sprintf (buf, "%s is not prime.  ", string_rep);
		if (ps.prp_base != 3) sprintf (buf+strlen(buf), "Base-%u ", ps.prp_base);
		if (ps.residue_type != PRIMENET_PRP_TYPE_FERMAT) sprintf (buf+strlen(buf), "Type-%d ", ps.residue_type);
		sprintf (buf+strlen(buf), "RES64: %s.", ps.res64);
	}
	sprintf (buf+strlen(buf), " Wh%d: %08lX,", PORT, SEC1 (w->n));
	if (ps.units_bit) sprintf (buf+strlen(buf), "%ld,", ps.units_bit);
	sprintf (buf+strlen(buf), "%08lX\n", ps.error_count);
	OutputStr (thread_num, buf);
	formatMsgForResultsFile (buf, w);

	if ((ps.isProbablePrime && IniGetInt (INI_FILE, "OutputPrimes", 0)) ||
	    (!ps.isProbablePrime && IniGetInt (INI_FILE, "OutputComposites", 0)))
		writeResults (buf);

	if (string_rep_truncated) {
		char	*bigbuf;
		bigbuf = (char *) malloc (strlen (w->known_factors) + 100);
		if (bigbuf != NULL) {
			sprintf (bigbuf, "Known factors used for PRP test were: %s\n", w->known_factors);
			OutputStr (thread_num, bigbuf);
			if ((ps.isProbablePrime && IniGetInt (INI_FILE, "OutputPrimes", 0)) ||
			    (!ps.isProbablePrime && IniGetInt (INI_FILE, "OutputComposites", 0)))
				writeResults (bigbuf);
			free (bigbuf);
		}
	}

	sprintf (JSONbuf, "{\"status\":\"%s\"", ps.isProbablePrime ? "P" : "C");
	JSONaddExponent (JSONbuf, w);
	sprintf (JSONbuf+strlen(JSONbuf), ", \"worktype\":\"PRP-%u\"", ps.prp_base);
	if (!ps.isProbablePrime) {
		sprintf (JSONbuf+strlen(JSONbuf), ", \"res64\":\"%s\"", ps.res64);
		sprintf (JSONbuf+strlen(JSONbuf), ", \"residue-type\":%d", ps.residue_type);
		if (ps.have_res2048 && IniGetInt (INI_FILE, "OutputRes2048", 1))
			sprintf (JSONbuf+strlen(JSONbuf), ", \"res2048\":\"%s\"", ps.res2048);
	}
	sprintf (JSONbuf+strlen(JSONbuf), ", \"fft-length\":%lu", gwdata.FFTLEN);
	if (ps.units_bit) sprintf (JSONbuf+strlen(JSONbuf), ", \"shift-count\":%ld", ps.units_bit);
	sprintf (JSONbuf+strlen(JSONbuf), ", \"error-code\":\"%08lX\"", ps.error_count);
	sprintf (JSONbuf+strlen(JSONbuf), ", \"security-code\":\"%08lX\"", SEC1(w->n));
	JSONaddProgramTimestamp (JSONbuf);
	if (ps.error_check_type == PRP_ERRCHK_DBLCHK)
		sprintf (JSONbuf+strlen(JSONbuf), ", \"errors\":{\"dblchk\":%lu}", (ps.error_count >> 20) & 0xF);
	if (ps.error_check_type == PRP_ERRCHK_GERBICZ)
		sprintf (JSONbuf+strlen(JSONbuf), ", \"errors\":{\"gerbicz\":%lu}", (ps.error_count >> 20) & 0xF);
	if (ps.proof_power) {
		sprintf (JSONbuf+strlen(JSONbuf), ", \"proof\":{\"version\":%d, \"power\":%d", ps.proof_version, ps.proof_power);
		if (ps.proof_power_mult > 1) sprintf (JSONbuf+strlen(JSONbuf), ", \"power-multiplier\":%d", ps.proof_power_mult);
		sprintf (JSONbuf+strlen(JSONbuf), ", \"hashsize\":%d, \"md5\":\"%s\"}", ps.hashlen, proof_hash);
	}
	JSONaddExponentKnownFactors (JSONbuf, w);
	JSONaddUserComputerAID (JSONbuf, w);
	strcat (JSONbuf, "}");
	if (IniGetInt (INI_FILE, "OutputJSON", 1)) writeResultsJSON (JSONbuf);

	{
		struct primenetAssignmentResult pkt;
		memset (&pkt, 0, sizeof (pkt));
		strcpy (pkt.computer_guid, COMPUTER_GUID);
		strcpy (pkt.assignment_uid, w->assignment_uid);
		strcpy (pkt.message, buf);
		pkt.result_type = ps.isProbablePrime ? PRIMENET_AR_PRP_PRIME : PRIMENET_AR_PRP_RESULT;
		pkt.k = w->k;
		pkt.b = w->b;
		pkt.n = w->n;
		pkt.c = w->c;
		strcpy (pkt.residue, ps.res64);
		sprintf (pkt.error_count, "%08lX", ps.error_count);
		pkt.prp_base = ps.prp_base;
		pkt.prp_residue_type = ps.residue_type;
		pkt.shift_count = ps.units_bit;
		pkt.num_known_factors = (w->known_factors == NULL) ? 0 : countCommas (w->known_factors) + 1;
		pkt.gerbicz = (ps.error_check_type != PRP_ERRCHK_NONE);
		pkt.fftlen = gwfftlen (&gwdata);
		if (ps.proof_power) {
			pkt.proof_power = ps.proof_power;
			strcpy (pkt.proof_hash, proof_hash);
		}
		pkt.done = TRUE;
		strcpy (pkt.JSONmessage, JSONbuf);
		spoolMessage (PRIMENET_ASSIGNMENT_RESULT, &pkt);
	}

	unlinkSaveFiles (&write_save_file_state);

	if (ps.isProbablePrime && w->next != NULL && w->k == w->next->k && w->b == w->next->b && w->n == w->next->n && w->c == w->next->c)
		deleteWorkToDoLine (thread_num, w->next, FALSE);

	if (ps.isProbablePrime &&
	    ((!SILENT_VICTORY && w->k == 1.0 && w->b == 2 && w->c == -1 && !isKnownMersennePrime (w->n)) ||
	     (!SILENT_VICTORY_PRP && (w->k != 1.0 || w->b != 2 || w->c != -1)))) {
		gwthread thread_handle;
		char	*arg;
		arg = (char *) malloc (strlen (string_rep) + 1);
		strcpy (arg, string_rep);
		gwthread_create (&thread_handle, &good_news_prp, (void *) arg);
	}

	stop_reason = STOP_WORK_UNIT_COMPLETE;

exit:	gwdone (&gwdata);
	free (N);
	free (exp);
	for (i = 0; i < ps.num_emergency_allocs; i++) free (ps.emergency_allocs[i]);
	free (ps.emergency_allocs);
	return (stop_reason);

restart:if (sleep5) OutputBoth (thread_num, ERRMSG2);
	OutputBoth (thread_num, ERRMSG3);

	restart_error_count = ps.error_count;

	if (sleep5) {
		stop_reason = SleepFive (thread_num);
		if (stop_reason) return (stop_reason);
	}

	gwdone (&gwdata);
	free (N);
	free (exp);
	for (i = 0; i < ps.num_emergency_allocs; i++) free (ps.emergency_allocs[i]);
	free (ps.emergency_allocs);
	ps.emergency_allocs = NULL;
	ps.num_emergency_allocs = 0;
	goto begin;
}

#include "cert.c"
