/*----------------------------------------------------------------------
| Copyright 1995-2024 Mersenne Research, Inc.  All rights reserved
|
| This file contains routines and global variables that are common for
| all operating systems the program has been ported to.  It is included
| in one of the source code files of each port.  See common.h for the
| common #defines and common routine definitions.
|
| Commona contains information used only during setup
| Commonb contains information used only during execution
| Commonc contains information used during setup and execution
+---------------------------------------------------------------------*/

static const char JUNK[]="Copyright 1996-2024 Mersenne Research, Inc. All rights reserved";

char	INI_FILE[260] = {0};
char	WORKTODO_FILE[260] = {0};
char	RESFILE[260] = {0};
char	RESFILEBENCH[260] = {0};
char	RESFILEJSON[260] = {0};
char	SPOOL_FILE[260] = {0};
char	LOGFILE[260] = {0};
char	SCREENLOGFILE[260] = {0};
char	*RESFILES[3] = {RESFILE, RESFILEBENCH, RESFILEJSON};
int	NO_GUI = 1;

char	USERID[21] = {0};
char	COMPID[21] = {0};
char	COMPUTER_GUID[33] = {0};
char	HARDWARE_GUID[33] = {0};
char	WINDOWS_GUID[33] = {0};
int	USE_PRIMENET = 0;
int	DIAL_UP = 0;
unsigned int DAYS_OF_WORK = 5;
int	STRESS_TESTER = 0;
int volatile ERRCHK = 0;
unsigned int PRIORITY = 1;
unsigned int NUM_WORKERS = 1;		
unsigned int WORK_PREFERENCE[MAX_NUM_WORKERS] = {0};
unsigned int CORES_PER_TEST[MAX_NUM_WORKERS] = {1}; 
int	HYPERTHREAD_TF = 1;		
int	HYPERTHREAD_LL = 0;		
int	MANUAL_COMM = 0;
int	this_is_a_manual_comm = FALSE;	
float volatile CPU_WORKER_DISK_SPACE = 6.0;
unsigned int volatile CPU_HOURS = 0;
int	CLASSIC_OUTPUT = 0;
int	OUTPUT_ROUNDOFF = 0;
unsigned long volatile ITER_OUTPUT = 0;
unsigned long volatile ITER_OUTPUT_RES = 999999999;
unsigned long volatile DISK_WRITE_TIME = 30;
unsigned long volatile JACOBI_TIME = 12; 
unsigned int MODEM_RETRY_TIME = 2;
unsigned int NETWORK_RETRY_TIME = 70;
float	DAYS_BETWEEN_CHECKINS = 1.0;
int	NUM_BACKUP_FILES = 3;
int	NUM_JACOBI_BACKUP_FILES = 2;
int	SILENT_VICTORY = 0;
int	SILENT_VICTORY_PRP = 1;
int	RUN_ON_BATTERY = 1;
int	BATTERY_PERCENT = 0;
int	DEFEAT_POWER_SAVE = 1;
int	TRAY_ICON = TRUE;
int	HIDE_ICON = FALSE;
int	MERGE_WINDOWS = 0;		
double UNOFFICIAL_CPU_SPEED = 0.0;
unsigned int ROLLING_AVERAGE = 0;
unsigned int PRECISION = 2;
int	RDTSC_TIMING = 1;
int	TIMESTAMPING = 1;
int	TIMESTAMPING_UTC = 0;
int	LOGTIMESTAMPING = 1;
int	LOGTIMESTAMPING_UTC = 0;
int	SCREENLOG = 0;
int	CUMULATIVE_TIMING = 0;
int	CUMULATIVE_ROUNDOFF = 1;
int	SEQUENTIAL_WORK = -1;
int	WELL_BEHAVED_WORK = 0;
unsigned long INTERIM_FILES = 0;
unsigned long INTERIM_RESIDUES = 0;
unsigned long HYPERTHREADING_BACKOFF = 0;
int	THROTTLE_PCT = 0;

int	STARTUP_IN_PROGRESS = 0;	

gwmutex	OUTPUT_MUTEX;			
gwmutex	LOG_MUTEX;			
gwmutex	WORKTODO_MUTEX;			

gwevent AUTOBENCH_EVENT;		
gwevent PROOF_UPLOAD_EVENT;		

int	LAUNCH_TYPE = 0;		
unsigned int WORKERS_ACTIVE = 0;	
int	WORKERS_STOPPING = 0;		

struct work_unit_array WORK_UNITS[MAX_NUM_WORKERS] = {0}; 
unsigned int WORKTODO_COUNT = 0;	
unsigned int WORKTODO_IN_USE_COUNT = 0;	
int	WORKTODO_CHANGED = 0;		

hwloc_topology_t hwloc_topology;	
uint32_t CPU_TOTAL_L1_CACHE_SIZE = 0;	
uint32_t CPU_TOTAL_L2_CACHE_SIZE = 0;	
uint32_t CPU_TOTAL_L3_CACHE_SIZE = 0;	
uint32_t CPU_TOTAL_L4_CACHE_SIZE = 0;	
uint32_t CPU_NUM_L1_CACHES = 0;		
uint32_t CPU_NUM_L2_CACHES = 0;		
uint32_t CPU_NUM_L3_CACHES = 0;		
uint32_t CPU_NUM_L4_CACHES = 0;		
int	CPU_L2_CACHE_INCLUSIVE = -1;	
int	CPU_L3_CACHE_INCLUSIVE = -1;	
int	CPU_L4_CACHE_INCLUSIVE = -1;	
int	OS_CAN_SET_AFFINITY = 1;	

uint32_t HW_NUM_CORES;			
uint32_t HW_NUM_THREADS;		
uint32_t HW_NUM_COMPUTE_CORES;		
uint32_t HW_NUM_THREADING_NODES;	
uint32_t HW_NUM_COMPUTE_THREADING_NODES;
uint32_t HW_NUM_NUMA_NODES;		
struct hw_core_info *HW_CORES = NULL;	

const char KEY_NumWorkers[] = "NumWorkers";
const char KEY_QuitGIMPS[] = "QuitGIMPS";

const char SEC_Internals[] = "Internals";
const char KEY_V30OptionsConverted[] = "V30OptionsConverted";
const char KEY_OldCpuSpeed[] = "OldCpuSpeed";
const char KEY_NewCpuSpeed[] = "NewCpuSpeed";
const char KEY_NewCpuSpeedCount[] = "NewCpuSpeedCount";
const char KEY_RollingAverage[] = "RollingAverage";
const char KEY_RollingHash[] = "RollingHash";
const char KEY_RollingStartTime[] = "RollingStartTime";
const char KEY_RollingCompleteTime[] = "RollingCompleteTime";
const char KEY_CertDailyMBRemaining[] = "CertDailyMBRemaining";
const char KEY_CertDailyCPURemaining[] = "CertDailyCPURemaining";
const char KEY_CertDailyRemainingLastUpdate[] = "CertDailyRemainingLastUpdate";
const char KEY_Pid[] = "Pid";
const char KEY_SrvrUID[] = "SrvrUID";
const char KEY_SrvrComputerName[] = "SrvrComputerName";
const char KEY_SrvrP00[] = "SrvrP00";
const char KEY_SrvrPO1[] = "SrvrPO1";
const char KEY_SrvrPO2[] = "SrvrPO2";
const char KEY_SrvrPO3[] = "SrvrPO3";
const char KEY_SrvrPO4[] = "SrvrPO4";
const char KEY_SrvrPO5[] = "SrvrPO5";
const char KEY_SrvrPO6[] = "SrvrPO6";
const char KEY_SrvrPO7[] = "SrvrPO7";
const char KEY_SrvrPO8[] = "SrvrPO8";
const char KEY_SrvrPO9[] = "SrvrPO9";
const char KEY_LastEndDatesSent[] = "LastEndDatesSent";
const char KEY_WGUID_version[] = "WGUID_version";
const char KEY_CertErrorCount[] = "CertErrorCount";

const char SEC_PrimeNet[] = "PrimeNet";
const char KEY_DialUp[] = "DialUp";
const char KEY_ProxyHost[] = "ProxyHost";
const char KEY_ProxyUser[] = "ProxyUser";
const char KEY_ProxyPass[] = "ProxyPass";
const char KEY_ProxyMask[] = "ProxyMask";
const char KEY_Debug[] = "Debug";
const char KEY_UploadRateLimit[] = "UploadRateLimit";
const char KEY_UploadStartTime[] = "UploadStartTime";
const char KEY_UploadEndTime[] = "UploadEndTime";
const char KEY_DownloadDailyLimit[] = "DownloadDailyLimit";
const char KEY_DownloadRateLimit[] = "DownloadRateLimit";
const char KEY_ProofUploads[] = "ProofUploads";
const char KEY_ProofHashLength[] = "ProofHashLength";
const char KEY_ProofPower[] = "ProofPower";
const char KEY_ProofPowerMult[] = "ProofPowerMult";
const char KEY_UploadChunkSize[] = "UploadChunkSize";

const char SEC_Windows[] = "Windows";
const char KEY_MergeWindows[] = "MergeWindows";
const char KEY_TrayIcon[] = "TrayIcon";
const char KEY_HideIcon[] = "HideIcon";
const char KEY_Left[] = "Left";
const char KEY_Right[] = "Right";
const char KEY_Top[] = "Top";
const char KEY_Bottom[] = "Bottom";
const char KEY_ExitOnX[] = "ExitOnX";

int parseWorkToDoLine (char *line, struct work_unit *w);

void generate_application_string (
	char	*app_string)
{
#ifdef SECURITY_MODULES_PRESENT
	sprintf (app_string, "%s,SysCorePrime,v%s,build %s",
#else
	sprintf (app_string, "%s,Untrusted SysCorePrime,v%s,build %s",
#endif
		 PORT == 1 ? "Windows" :
		 PORT == 2 ? "Linux" :
		 PORT == 4 ? "Windows64" :
		 PORT == 5 ? "WindowsService" :
		 PORT == 6 ? "FreeBSD" :
		 PORT == 7 ? "OS/2" :
		 PORT == 8 ? "Linux64" :
		 PORT == 9 ? "Mac OS X" :
		 PORT == 10 ? "Mac OS X 64-bit" :
		 PORT == 11 ? "Haiku" :
		 PORT == 12 ? "FreeBSD 64-bit" : "Unknown",
		 VERSION, BUILD_NUM);
}

void calc_hardware_guid (void)
{
	char	buf[500];

	sprintf (buf, "%s%d", CPU_BRAND, CPU_SIGNATURE);
	md5_hexdigest_string (HARDWARE_GUID, buf);

	if (IniGetInt (INI_FILE, "FixedHardwareUID", 0)) {
		IniGetString (INI_FILE, "HardwareGUID", HARDWARE_GUID, sizeof (HARDWARE_GUID), HARDWARE_GUID);
		IniWriteString (INI_FILE, "HardwareGUID", HARDWARE_GUID);
	}
}

void calc_windows_guid (void)
{
#ifdef _WINDOWS_
	int	algorithm_version;
	char	buf[500];

	algorithm_version = IniSectionGetInt (INI_FILE, SEC_Internals, KEY_WGUID_version, 1);
	if (algorithm_version == 1) {
		getWindowsSerialNumber (buf);
		getWindowsSID (buf + strlen (buf));
	} else {
		getWindowsSerialNumber_2 (buf);
		getWindowsSID_2 (buf + strlen (buf));
	}
	md5_hexdigest_string (WINDOWS_GUID, buf);
#else
	WINDOWS_GUID[0] = 0;
#endif

	if (IniGetInt (INI_FILE, "FixedHardwareUID", 0)) {
		IniGetString (INI_FILE, "WindowsGUID", WINDOWS_GUID, sizeof (WINDOWS_GUID), WINDOWS_GUID);
		IniWriteString (INI_FILE, "WindowsGUID", WINDOWS_GUID);
	}
}

void clearCachedProgramOptions (void)
{
	int	tnum;
	char	section_name[32];

	IniSectionWriteString (INI_FILE, SEC_Internals, KEY_SrvrPO1, NULL);
	for (tnum = 0; tnum < MAX_NUM_WORKERS; tnum++) {
		sprintf (section_name, "Worker #%d", tnum+1);
		IniSectionWriteString (INI_FILE, section_name, KEY_SrvrPO1, NULL);
	}
	IniSectionWriteString (INI_FILE, SEC_Internals, KEY_SrvrPO2, NULL);
	IniSectionWriteString (INI_FILE, SEC_Internals, KEY_SrvrPO3, NULL);
	IniSectionWriteString (INI_FILE, SEC_Internals, KEY_SrvrPO4, NULL);
	IniSectionWriteString (INI_FILE, SEC_Internals, KEY_SrvrPO5, NULL);
	IniSectionWriteString (INI_FILE, SEC_Internals, KEY_SrvrPO6, NULL);
	IniSectionWriteString (INI_FILE, SEC_Internals, KEY_SrvrPO7, NULL);
	IniSectionWriteString (INI_FILE, SEC_Internals, KEY_SrvrPO8, NULL);
	IniSectionWriteString (INI_FILE, SEC_Internals, KEY_SrvrPO9, NULL);
}

void generate_computer_guid (void)
{
	char	buf[500];
	time_t	current_time;

	time (&current_time);
	sprintf (buf, "%s%d%f%d", CPU_BRAND, CPU_SIGNATURE, CPU_SPEED, (int) current_time);
	md5_hexdigest_string (COMPUTER_GUID, buf);
	IniWriteString (INI_FILE, "ComputerGUID", COMPUTER_GUID);

	IniSectionWriteString (INI_FILE, SEC_Internals, KEY_SrvrUID, NULL);
	IniSectionWriteString (INI_FILE, SEC_Internals, KEY_SrvrComputerName, NULL);
	clearCachedProgramOptions ();

	IniSectionWriteInt (INI_FILE, SEC_Internals, KEY_WGUID_version, 2);
	calc_windows_guid ();
}

void getCpuSpeed (void)
{
	int	temp, old_cpu_speed, report_new_cpu_speed;

	guessCpuSpeed ();

	temp = IniGetInt (INI_FILE, "CpuSpeed", 99);
	if (temp != 99) CPU_SPEED = temp;

	if (CPU_SPEED > 50000) CPU_SPEED = 50000;
	if (CPU_SPEED < 25) CPU_SPEED = 25;

	UNOFFICIAL_CPU_SPEED = CPU_SPEED;

	report_new_cpu_speed = FALSE;
	old_cpu_speed = IniSectionGetInt (INI_FILE, SEC_Internals, KEY_OldCpuSpeed, 0);
	if (CPU_SPEED < (double) old_cpu_speed * 0.97) {
		if (IniSectionGetInt (INI_FILE, SEC_Internals, KEY_NewCpuSpeedCount, 0) <= 5) {
			if (CPU_SPEED > (double) IniSectionGetInt (INI_FILE, SEC_Internals, KEY_NewCpuSpeed, 0))
				IniSectionWriteInt (INI_FILE, SEC_Internals, KEY_NewCpuSpeed, (int) (CPU_SPEED + 0.5));
			IniSectionWriteInt (INI_FILE, SEC_Internals, KEY_NewCpuSpeedCount, IniSectionGetInt (INI_FILE, SEC_Internals, KEY_NewCpuSpeedCount, 0) + 1);
			CPU_SPEED = old_cpu_speed;
		} else {
			if (CPU_SPEED < (double) IniSectionGetInt (INI_FILE, SEC_Internals, KEY_NewCpuSpeed, 0))
				CPU_SPEED = (double) IniSectionGetInt (INI_FILE, SEC_Internals, KEY_NewCpuSpeed, 0);
			report_new_cpu_speed = TRUE;
		}
	}

	else if (CPU_SPEED < (double) old_cpu_speed * 1.03) {
		IniSectionWriteInt (INI_FILE, SEC_Internals, KEY_NewCpuSpeedCount, 0);
		IniSectionWriteInt (INI_FILE, SEC_Internals, KEY_NewCpuSpeed, 0);
	}

	else {
		report_new_cpu_speed = TRUE;
	}

	if (report_new_cpu_speed) {
		IniSectionWriteInt (INI_FILE, SEC_Internals, KEY_OldCpuSpeed, (int) (CPU_SPEED + 0.5));
		IniSectionWriteInt (INI_FILE, SEC_Internals, KEY_NewCpuSpeedCount, 0);
		IniSectionWriteInt (INI_FILE, SEC_Internals, KEY_NewCpuSpeed, 0);
		if (old_cpu_speed) {
			if (WORKTODO_COUNT) {
				ROLLING_AVERAGE = (int) (ROLLING_AVERAGE * old_cpu_speed / CPU_SPEED);
				if (ROLLING_AVERAGE < 1000) ROLLING_AVERAGE = 1000;
			}
			else
				ROLLING_AVERAGE = 1000;
			IniSectionWriteInt (INI_FILE, SEC_Internals, KEY_RollingAverage, ROLLING_AVERAGE);
			IniSectionWriteInt (INI_FILE, SEC_Internals, KEY_RollingStartTime, 0);
			spoolMessage (PRIMENET_UPDATE_COMPUTER_INFO, NULL);
			UpdateEndDates ();
		}
	}
}

uint32_t calc_threading_nodes_for_cpuset (hwloc_bitmap_t cpuset)
{
	int	num_cores;
	int	num_objs;
	int	num_threading_nodes = 1;

	num_cores = hwloc_get_nbobjs_inside_cpuset_by_type (hwloc_topology, cpuset, HWLOC_OBJ_CORE);

	num_objs = hwloc_get_nbobjs_inside_cpuset_by_type (hwloc_topology, cpuset, HWLOC_OBJ_PACKAGE);
	if (num_objs < num_cores && num_objs > num_threading_nodes) num_threading_nodes = num_objs;

	num_objs = hwloc_get_nbobjs_inside_cpuset_by_type (hwloc_topology, cpuset, HWLOC_OBJ_NUMANODE);
	if (num_objs < num_cores && num_objs > num_threading_nodes) num_threading_nodes = num_objs;

	num_objs = hwloc_get_nbobjs_inside_cpuset_by_type (hwloc_topology, cpuset, HWLOC_OBJ_L4CACHE);
	if (num_objs < num_cores && num_objs > num_threading_nodes) num_threading_nodes = num_objs;
	num_objs = hwloc_get_nbobjs_inside_cpuset_by_type (hwloc_topology, cpuset, HWLOC_OBJ_L3CACHE);
	if (num_objs < num_cores && num_objs > num_threading_nodes) num_threading_nodes = num_objs;
	num_objs = hwloc_get_nbobjs_inside_cpuset_by_type (hwloc_topology, cpuset, HWLOC_OBJ_L2CACHE);
	if (num_objs < num_cores && num_objs > num_threading_nodes) num_threading_nodes = num_objs;

	return (num_threading_nodes);
}

void getCpuInfo (void)
{
	int	depth, i, temp;
	uint32_t core;

	guessCpuType ();

	HW_NUM_CORES = hwloc_get_nbobjs_by_type (hwloc_topology, HWLOC_OBJ_CORE);
	if (HW_NUM_CORES < 1) HW_NUM_CORES = hwloc_get_nbobjs_by_type (hwloc_topology, HWLOC_OBJ_PU);
	if (HW_NUM_CORES < 1) HW_NUM_CORES = 1;			
	HW_NUM_THREADS = hwloc_get_nbobjs_by_type (hwloc_topology, HWLOC_OBJ_PU);
	if (HW_NUM_THREADS < 1) HW_NUM_THREADS = 1;		
	HW_NUM_NUMA_NODES = hwloc_get_nbobjs_by_type (hwloc_topology, HWLOC_OBJ_NUMANODE);
	if (HW_NUM_NUMA_NODES < 1) HW_NUM_NUMA_NODES = 1;

	HW_NUM_CORES = IniGetInt (INI_FILE, "NumCPUs", HW_NUM_CORES);
	HW_NUM_CORES = IniGetInt (INI_FILE, "NumCores", HW_NUM_CORES);
	if (HW_NUM_CORES < 1) HW_NUM_CORES = 1;
	HW_NUM_THREADS = IniGetInt (INI_FILE, "NumThreads", HW_NUM_THREADS);
	if (HW_NUM_THREADS < 1) HW_NUM_THREADS = 1;
	HW_NUM_NUMA_NODES = IniGetInt (INI_FILE, "NumNUMANodes", HW_NUM_NUMA_NODES);
	if (HW_NUM_NUMA_NODES == 0) HW_NUM_NUMA_NODES = 1;

	free (HW_CORES);
	HW_CORES = (struct hw_core_info *) malloc (HW_NUM_CORES * sizeof (struct hw_core_info));
	for (core = 0; core < HW_NUM_CORES; core++) {
		HW_CORES[core].ranking = 1;		
		
		int	num_threads = 1;		
		hwloc_obj_t obj = hwloc_get_obj_by_type (hwloc_topology, HWLOC_OBJ_CORE, core);		
		if (obj == NULL) obj = hwloc_get_obj_by_type (hwloc_topology, HWLOC_OBJ_PU, core);	
		if (obj != NULL) num_threads = hwloc_get_nbobjs_inside_cpuset_by_type (hwloc_topology, obj->cpuset, HWLOC_OBJ_PU);
		
		char key[50];
		sprintf (key, "Core%" PRIu32 "NumThreads", core);
		num_threads = IniGetInt (INI_FILE, key, num_threads);
		if (num_threads < 1) num_threads = 1;
		HW_CORES[core].num_threads = (uint16_t) num_threads;
	}

	CPU_TOTAL_L1_CACHE_SIZE = CPU_NUM_L1_CACHES = 0;
	CPU_TOTAL_L2_CACHE_SIZE = CPU_NUM_L2_CACHES = 0;
	CPU_TOTAL_L3_CACHE_SIZE = CPU_NUM_L3_CACHES = 0;
	CPU_TOTAL_L4_CACHE_SIZE = CPU_NUM_L4_CACHES = 0;
	CPU_L2_CACHE_INCLUSIVE = -1;
	CPU_L3_CACHE_INCLUSIVE = -1;
	CPU_L4_CACHE_INCLUSIVE = -1;
	for (depth = 0; depth < hwloc_topology_get_depth (hwloc_topology); depth++) {
		for (i = 0; i < (int) hwloc_get_nbobjs_by_depth (hwloc_topology, depth); i++) {
			hwloc_obj_t obj;
			const char *inclusive;

			obj = hwloc_get_obj_by_depth (hwloc_topology, depth, i);
			if (obj == NULL || obj->attr == NULL) break; 
			if (obj->type == HWLOC_OBJ_L1CACHE) {
				CPU_TOTAL_L1_CACHE_SIZE += (uint32_t) (obj->attr->cache.size >> 10);
				CPU_NUM_L1_CACHES++;
				if (obj->attr->cache.linesize > 0) CPU_L1_CACHE_LINE_SIZE = obj->attr->cache.linesize;
				if (obj->attr->cache.associativity > 0) CPU_L1_SET_ASSOCIATIVE = obj->attr->cache.associativity;
			}
			else if (obj->type == HWLOC_OBJ_L2CACHE) {
				CPU_TOTAL_L2_CACHE_SIZE += (uint32_t) (obj->attr->cache.size >> 10);
				CPU_NUM_L2_CACHES++;
				if (obj->attr->cache.linesize > 0) CPU_L2_CACHE_LINE_SIZE = obj->attr->cache.linesize;
				if (obj->attr->cache.associativity > 0) CPU_L2_SET_ASSOCIATIVE = obj->attr->cache.associativity;
				inclusive = hwloc_obj_get_info_by_name (obj, "Inclusive");
				if (inclusive != NULL) CPU_L2_CACHE_INCLUSIVE = atoi (inclusive);
			}
			else if (obj->type == HWLOC_OBJ_L3CACHE) {
				CPU_TOTAL_L3_CACHE_SIZE += (uint32_t) (obj->attr->cache.size >> 10);
				CPU_NUM_L3_CACHES++;
				if (obj->attr->cache.linesize > 0) CPU_L3_CACHE_LINE_SIZE = obj->attr->cache.linesize;
				if (obj->attr->cache.associativity > 0) CPU_L3_SET_ASSOCIATIVE = obj->attr->cache.associativity;
				inclusive = hwloc_obj_get_info_by_name (obj, "Inclusive");
				if (inclusive != NULL) CPU_L3_CACHE_INCLUSIVE = atoi (inclusive);
			}
			else if (obj->type == HWLOC_OBJ_L4CACHE) {
				CPU_TOTAL_L4_CACHE_SIZE += (uint32_t) (obj->attr->cache.size >> 10);
				CPU_NUM_L4_CACHES++;
				inclusive = hwloc_obj_get_info_by_name (obj, "Inclusive");
				if (inclusive != NULL) CPU_L4_CACHE_INCLUSIVE = atoi (inclusive);
			}
		}
	}

	if (CPU_NUM_L1_CACHES) CPU_L1_CACHE_SIZE = CPU_TOTAL_L1_CACHE_SIZE / CPU_NUM_L1_CACHES;
	if (CPU_NUM_L2_CACHES) CPU_L2_CACHE_SIZE = CPU_TOTAL_L2_CACHE_SIZE / CPU_NUM_L2_CACHES;
	if (CPU_NUM_L3_CACHES) CPU_L3_CACHE_SIZE = CPU_TOTAL_L3_CACHE_SIZE / CPU_NUM_L3_CACHES;

	if (CPU_NUM_L1_CACHES == 0 && CPU_L1_CACHE_SIZE > 0) CPU_TOTAL_L1_CACHE_SIZE = CPU_L1_CACHE_SIZE * HW_NUM_CORES, CPU_NUM_L1_CACHES = HW_NUM_CORES;
	if (CPU_NUM_L2_CACHES == 0 && CPU_L2_CACHE_SIZE > 0) CPU_TOTAL_L2_CACHE_SIZE = CPU_L2_CACHE_SIZE, CPU_NUM_L2_CACHES = 1;
	if (CPU_NUM_L3_CACHES == 0 && CPU_L3_CACHE_SIZE > 0) CPU_TOTAL_L3_CACHE_SIZE = CPU_L3_CACHE_SIZE, CPU_NUM_L3_CACHES = 1;

	int kind_verbosity = IniGetInt (INI_FILE, "KindVerbosity", 0);
	int cpu_kinds = hwloc_cpukinds_get_nr (hwloc_topology, 0);
	if (kind_verbosity) {
		char	buf[200];
		sprintf (buf, "Hwloc %d kinds\n", cpu_kinds);
		writeResultsBench (buf);
	}

	HW_NUM_THREADING_NODES = 0;
	HW_NUM_COMPUTE_THREADING_NODES = 0;
	hwloc_bitmap_t cpuset = hwloc_bitmap_alloc ();
	for (int i = 0; i < cpu_kinds; i++) {
		int	efficiency;
		unsigned num_infos;
		struct hwloc_info_s *infos;
		uint32_t num_threading_nodes;

		if (hwloc_cpukinds_get_info (hwloc_topology, i, cpuset, &efficiency, &num_infos, &infos, 0) < 0) continue;
		if (kind_verbosity) {
			char	buf[200], str[80];
			hwloc_bitmap_snprintf (str, sizeof (str), cpuset);
			sprintf (buf, "Hwloc kind %d: Efficiency: %d, Cpuset: %s\n", i, efficiency, str);
			writeResultsBench (buf);
		}

		num_threading_nodes = calc_threading_nodes_for_cpuset (cpuset);
		HW_NUM_THREADING_NODES += num_threading_nodes;

		if (cpu_kinds == 1 || efficiency != 0) {
			HW_NUM_COMPUTE_THREADING_NODES += num_threading_nodes;
			continue;
		}

		for (core = 0; core < HW_NUM_CORES; core++) {
			hwloc_obj_t obj;
			obj = hwloc_get_obj_by_type (hwloc_topology, HWLOC_OBJ_CORE, core);			
			if (obj == NULL) obj = hwloc_get_obj_by_type (hwloc_topology, HWLOC_OBJ_PU, core);	
			if (obj != NULL && hwloc_bitmap_isincluded (obj->cpuset, cpuset))
				HW_CORES[core].ranking = 0;							
		}
	}
	hwloc_bitmap_free (cpuset);

	for (core = 0; core < HW_NUM_CORES; core++) {
		char key[50];
		sprintf (key, "Core%" PRIu32 "Ranking", core);
		HW_CORES[core].ranking = (uint16_t) IniGetInt (INI_FILE, key, HW_CORES[core].ranking);
	}

	HW_NUM_COMPUTE_CORES = 0;
	for (core = 0; core < HW_NUM_CORES; core++) if (HW_CORES[core].ranking >= 1) HW_NUM_COMPUTE_CORES++;
	ASSERTG (HW_NUM_COMPUTE_CORES > 0);

	calc_hardware_guid ();

	temp = IniGetInt (INI_FILE, "CpuSupportsRDTSC", 99);
	if (temp == 0) CPU_FLAGS &= ~CPU_RDTSC;
	if (temp == 1) CPU_FLAGS |= CPU_RDTSC;
	temp = IniGetInt (INI_FILE, "CpuSupportsCMOV", 99);
	if (temp == 0) CPU_FLAGS &= ~CPU_CMOV;
	if (temp == 1) CPU_FLAGS |= CPU_CMOV;
	temp = IniGetInt (INI_FILE, "CpuSupportsPrefetch", 99);
	if (temp == 0) CPU_FLAGS &= ~CPU_PREFETCH;
	if (temp == 1) CPU_FLAGS |= CPU_PREFETCH;
	temp = IniGetInt (INI_FILE, "CpuSupportsPrefetchw", 99);
	if (temp == 0) CPU_FLAGS &= ~CPU_PREFETCHW;
	if (temp == 1) CPU_FLAGS |= CPU_PREFETCHW;
	temp = IniGetInt (INI_FILE, "CpuSupportsPrefetchwt1", 99);
	if (temp == 0) CPU_FLAGS &= ~CPU_PREFETCHWT1;
	if (temp == 1) CPU_FLAGS |= CPU_PREFETCHWT1;
	temp = IniGetInt (INI_FILE, "CpuSupportsSSE", 99);
	if (temp == 0) CPU_FLAGS &= ~CPU_SSE;
	if (temp == 1) CPU_FLAGS |= CPU_SSE;
	temp = IniGetInt (INI_FILE, "CpuSupportsSSE2", 99);
	if (temp == 0) CPU_FLAGS &= ~CPU_SSE2;
	if (temp == 1) CPU_FLAGS |= CPU_SSE2;
	temp = IniGetInt (INI_FILE, "CpuSupportsSSE4", 99);
	if (temp == 0) CPU_FLAGS &= ~CPU_SSE41;
	if (temp == 1) CPU_FLAGS |= CPU_SSE41;
	temp = IniGetInt (INI_FILE, "CpuSupports3DNow", 99);
	if (temp == 0) CPU_FLAGS &= ~(CPU_3DNOW + CPU_3DNOW_PREFETCH);
	if (temp == 1) CPU_FLAGS |= (CPU_3DNOW + CPU_3DNOW_PREFETCH);
	temp = IniGetInt (INI_FILE, "CpuSupportsAVX", 99);
	if (temp == 0) CPU_FLAGS &= ~(CPU_AVX | CPU_FMA3);
	if (temp == 1) CPU_FLAGS |= CPU_AVX;
	temp = IniGetInt (INI_FILE, "CpuSupportsFMA3", 99);
	if (temp == 0) CPU_FLAGS &= ~CPU_FMA3;
	if (temp == 1) CPU_FLAGS |= (CPU_AVX | CPU_FMA3);
	temp = IniGetInt (INI_FILE, "CpuSupportsFMA4", 99);
	if (temp == 0) CPU_FLAGS &= ~CPU_FMA4;
	if (temp == 1) CPU_FLAGS |= CPU_FMA4;
	temp = IniGetInt (INI_FILE, "CpuSupportsAVX2", 99);
	if (temp == 0) CPU_FLAGS &= ~CPU_AVX2;
	if (temp == 1) CPU_FLAGS |= CPU_AVX2;
	temp = IniGetInt (INI_FILE, "CpuSupportsAVX512F", 99);
	if (temp == 0) CPU_FLAGS &= ~CPU_AVX512F;
	if (temp == 1) CPU_FLAGS |= CPU_AVX512F;

	CPU_TOTAL_L1_CACHE_SIZE = IniGetInt (INI_FILE, "CpuL1TotalCacheSize", CPU_TOTAL_L1_CACHE_SIZE);
	CPU_NUM_L1_CACHES = IniGetInt (INI_FILE, "CpuL1NumCaches", CPU_NUM_L1_CACHES);

	CPU_TOTAL_L2_CACHE_SIZE = IniGetInt (INI_FILE, "CpuL2TotalCacheSize", CPU_TOTAL_L2_CACHE_SIZE);
	CPU_NUM_L2_CACHES = IniGetInt (INI_FILE, "CpuL2NumCaches", CPU_NUM_L2_CACHES);
	CPU_L2_CACHE_SIZE = IniGetInt (INI_FILE, "CpuL2CacheSize", CPU_L2_CACHE_SIZE);
	CPU_L2_CACHE_LINE_SIZE = IniGetInt (INI_FILE, "CpuL2CacheLineSize", CPU_L2_CACHE_LINE_SIZE);
	CPU_L2_SET_ASSOCIATIVE = IniGetInt (INI_FILE, "CpuL2SetAssociative", CPU_L2_SET_ASSOCIATIVE);
	CPU_L2_CACHE_INCLUSIVE = IniGetInt (INI_FILE, "CpuL2CacheInclusive", CPU_L2_CACHE_INCLUSIVE);

	CPU_TOTAL_L3_CACHE_SIZE = IniGetInt (INI_FILE, "CpuL3TotalCacheSize", CPU_TOTAL_L3_CACHE_SIZE);
	CPU_NUM_L3_CACHES = IniGetInt (INI_FILE, "CpuL3NumCaches", CPU_NUM_L3_CACHES);
	CPU_L3_CACHE_SIZE = IniGetInt (INI_FILE, "CpuL3CacheSize", CPU_L3_CACHE_SIZE);
	CPU_L3_CACHE_LINE_SIZE = IniGetInt (INI_FILE, "CpuL3CacheLineSize", CPU_L3_CACHE_LINE_SIZE);
	CPU_L3_SET_ASSOCIATIVE = IniGetInt (INI_FILE, "CpuL3SetAssociative", CPU_L3_SET_ASSOCIATIVE);
	CPU_L3_CACHE_INCLUSIVE = IniGetInt (INI_FILE, "CpuL3CacheInclusive", CPU_L3_CACHE_INCLUSIVE);

	CPU_TOTAL_L4_CACHE_SIZE = IniGetInt (INI_FILE, "CpuL4TotalCacheSize", CPU_TOTAL_L4_CACHE_SIZE);
	CPU_NUM_L4_CACHES = IniGetInt (INI_FILE, "CpuL4NumCaches", CPU_NUM_L4_CACHES);
	CPU_L4_CACHE_INCLUSIVE = IniGetInt (INI_FILE, "CpuL4CacheInclusive", CPU_L4_CACHE_INCLUSIVE);

	IniGetString (INI_FILE, "CpuBrand", CPU_BRAND, sizeof(CPU_BRAND), CPU_BRAND);

	if (CPU_NUM_L1_CACHES > HW_NUM_CORES) CPU_NUM_L1_CACHES = HW_NUM_CORES;
	if (CPU_NUM_L2_CACHES > HW_NUM_CORES) CPU_NUM_L2_CACHES = HW_NUM_CORES;
	if (CPU_NUM_L3_CACHES > HW_NUM_CORES) CPU_NUM_L3_CACHES = HW_NUM_CORES;
	if (CPU_NUM_L4_CACHES > HW_NUM_CORES) CPU_NUM_L4_CACHES = HW_NUM_CORES;
	if (HW_NUM_NUMA_NODES > HW_NUM_CORES) HW_NUM_NUMA_NODES = HW_NUM_CORES;

	CPU_ARCHITECTURE = IniGetInt (INI_FILE, "CpuArchitecture", CPU_ARCHITECTURE);

	HW_NUM_THREADING_NODES = IniGetInt (INI_FILE, "NumThreadingNodes", HW_NUM_THREADING_NODES);
	if (HW_NUM_THREADING_NODES < 1) HW_NUM_THREADING_NODES = 1;
	HW_NUM_COMPUTE_THREADING_NODES = IniGetInt (INI_FILE, "NumComputeThreadingNodes", HW_NUM_COMPUTE_THREADING_NODES);
	if (HW_NUM_COMPUTE_THREADING_NODES < 1) HW_NUM_COMPUTE_THREADING_NODES = 1;

	getCpuSpeed ();
}

void getCpuDescription (
	char	*buf,			
	int	long_desc)		
{

	getCpuSpeed ();

	sprintf (buf, "%s\nCPU speed: %.2f MHz", CPU_BRAND, UNOFFICIAL_CPU_SPEED);
	if (HW_NUM_CORES != HW_NUM_COMPUTE_CORES)
		sprintf (buf + strlen (buf), ", %" PRIu32 "/%" PRIu32 " performance/efficiency cores", HW_NUM_COMPUTE_CORES, HW_NUM_CORES - HW_NUM_COMPUTE_CORES);
	else if (HW_NUM_CORES > 1 && HW_NUM_CORES != HW_NUM_THREADS)
		sprintf (buf + strlen (buf), ", %" PRIu32 " hyperthreaded cores", HW_NUM_CORES);
	else if (HW_NUM_CORES > 1)
		sprintf (buf + strlen (buf), ", %" PRIu32 " cores", HW_NUM_CORES);
	else if (HW_NUM_CORES != HW_NUM_THREADS)
		sprintf (buf + strlen (buf), ", with hyperthreading");
	strcat (buf, "\n");
	if (CPU_FLAGS) {
		strcat (buf, "CPU features: ");

		if (CPU_FLAGS & CPU_3DNOW) strcat (buf, "3DNow!, ");
		else if (CPU_FLAGS & CPU_3DNOW_PREFETCH) strcat (buf, "3DNow! Prefetch, ");
		else if (CPU_FLAGS & CPU_PREFETCHW) strcat (buf, "Prefetchw, ");
		else if (CPU_FLAGS & CPU_PREFETCH) strcat (buf, "Prefetch, ");
		if (CPU_FLAGS & CPU_SSE) strcat (buf, "SSE, ");
		if (CPU_FLAGS & CPU_SSE2) strcat (buf, "SSE2, ");
		if (CPU_FLAGS & CPU_SSE41) strcat (buf, "SSE4, ");
		if (CPU_FLAGS & CPU_AVX) strcat (buf, "AVX, ");
		if (CPU_FLAGS & CPU_AVX2) strcat (buf, "AVX2, ");
		if (CPU_FLAGS & (CPU_FMA3 | CPU_FMA4)) strcat (buf, "FMA, ");
		if (CPU_FLAGS & CPU_AVX512F) strcat (buf, "AVX512F, ");
		strcpy (buf + strlen (buf) - 2, "\n");
	}

	strcat (buf, "L1 cache size: ");
	if (CPU_NUM_L1_CACHES <= 0) strcat (buf, "unknown");
	if (CPU_NUM_L1_CACHES > 1) sprintf (buf + strlen (buf), "%dx", CPU_NUM_L1_CACHES);
	if (CPU_NUM_L1_CACHES >= 1) sprintf (buf + strlen (buf), "%d KB", CPU_TOTAL_L1_CACHE_SIZE / CPU_NUM_L1_CACHES);
	
	strcat (buf, ", L2 cache size: ");
	if (CPU_NUM_L2_CACHES <= 0) strcat (buf, "unknown");
	if (CPU_NUM_L2_CACHES > 1) sprintf (buf + strlen (buf), "%dx", CPU_NUM_L2_CACHES);
	if (CPU_NUM_L2_CACHES >= 1) {
		if ((CPU_TOTAL_L2_CACHE_SIZE / CPU_NUM_L2_CACHES) & 0x3FF)
			sprintf (buf + strlen (buf), "%d KB", CPU_TOTAL_L2_CACHE_SIZE / CPU_NUM_L2_CACHES);
		else
			sprintf (buf + strlen (buf), "%d MB", CPU_TOTAL_L2_CACHE_SIZE / CPU_NUM_L2_CACHES / 1024);
	}

	if (CPU_NUM_L3_CACHES > 0) {
		strcat (buf, CPU_NUM_L4_CACHES > 0 ? "\n" : ", ");
		strcat (buf, "L3 cache size: ");
		if (CPU_NUM_L3_CACHES > 1) sprintf (buf + strlen (buf), "%dx", CPU_NUM_L3_CACHES);
		if ((CPU_TOTAL_L3_CACHE_SIZE / CPU_NUM_L3_CACHES) & 0x3FF)
			sprintf (buf + strlen (buf), "%d KB", CPU_TOTAL_L3_CACHE_SIZE / CPU_NUM_L3_CACHES);
		else
			sprintf (buf + strlen (buf), "%d MB", CPU_TOTAL_L3_CACHE_SIZE / CPU_NUM_L3_CACHES / 1024);
	}

	if (CPU_NUM_L4_CACHES > 0) {
		strcat (buf, ", L4 cache size: ");
		if (CPU_NUM_L4_CACHES > 1) sprintf (buf + strlen (buf), "%dx", CPU_NUM_L4_CACHES);
		if ((CPU_TOTAL_L4_CACHE_SIZE / CPU_NUM_L4_CACHES) & 0x3FF)
			sprintf (buf + strlen (buf), "%d KB", CPU_TOTAL_L4_CACHE_SIZE / CPU_NUM_L4_CACHES);
		else
			sprintf (buf + strlen (buf), "%d MB", CPU_TOTAL_L4_CACHE_SIZE / CPU_NUM_L4_CACHES / 1024);
	}

	if (! long_desc) return;
	strcat (buf, "\nL1 cache line size: ");
	if (CPU_L1_CACHE_LINE_SIZE < 0) strcat (buf, "unknown");
	else sprintf (buf+strlen(buf), "%d bytes", CPU_L1_CACHE_LINE_SIZE);
	strcat (buf, ", L2 cache line size: ");
	if (CPU_L2_CACHE_LINE_SIZE < 0) strcat (buf, "unknown");
	else sprintf (buf+strlen(buf), "%d bytes", CPU_L2_CACHE_LINE_SIZE);
	strcat (buf, "\n");
}

void topology_print_children (
	hwloc_obj_t obj,
        int depth)
{
	char type[32], attr[1024], cpuset[256], buf[1500];
	unsigned int i;
	if (obj == NULL) return;  
	hwloc_obj_type_snprintf (type, sizeof(type), obj, 0);
	sprintf (buf, "%*s%s", 2*depth, " ", type);
	if (obj->os_index != (unsigned) -1)
		sprintf (buf+strlen(buf), "#%u", obj->os_index);
	hwloc_obj_attr_snprintf (attr, sizeof(attr), obj, ", ", 1 );
	if (obj->type == HWLOC_OBJ_CORE || obj->type == HWLOC_OBJ_PU)
		hwloc_bitmap_snprintf (cpuset, sizeof(cpuset), obj->cpuset);
	else
		cpuset[0] = 0;
	if (attr[0] && cpuset[0]) sprintf (buf+strlen(buf), " (%s, cpuset: %s)", attr, cpuset);
	else if (attr[0]) sprintf (buf+strlen(buf), " (%s)", attr);
	else if (cpuset[0]) sprintf (buf+strlen(buf), " (cpuset: %s)", cpuset);
	strcat (buf, "\n");
	writeResultsBench (buf);
	for (i = 0; i < obj->arity; i++) {
		topology_print_children (obj->children[i], depth + 1);
	}
}

int isPrime (
	unsigned long p)
{
	unsigned long i;
	for (i = 2; i < 0xFFFF && i * i <= p; i = (i + 1) | 1)
		if (p % i == 0) return (FALSE);
	return (TRUE);
}

typedef struct {
	uint32_t *primes;				
	uint64_t first_number;
	unsigned int bit_number;
	unsigned int num_primes;
	unsigned int num_elimination_primes;
	unsigned int max_elimination;
	uint64_t start;
	char	array[4096];
} sieve_info;

void fill_sieve (
	sieve_info *si)
{
	unsigned int i;
	uint32_t fmax;

	fmax = (uint32_t) sqrt ((double) (si->first_number + sizeof (si->array) * 8 * 2));
	for (i = si->num_primes; i < si->num_elimination_primes * 2; i += 2) {
		uint32_t f, r, bit;
		f = si->primes[i];
		if (f > fmax) break;
		if (si->first_number == 3) {
			bit = (f * f - 3) >> 1;
		} else {
			r = (uint32_t) (si->first_number % f);
			if (r == 0) bit = 0;
			else if (r & 1) bit = (f - r) / 2;
			else bit = (f + f - r) / 2;
			if (f == si->first_number + 2 * bit) bit += f;
		}
		si->primes[i+1] = bit;
	}
	si->num_primes = i;

	memset (si->array, 0xFF, sizeof (si->array));
	for (i = 0; i < si->num_primes; i += 2) {
		uint32_t f, bit;
		f = si->primes[i];
		for (bit = si->primes[i+1]; bit < sizeof (si->array) * 8; bit += f) bitclr (si->array, bit);
		si->primes[i+1] = bit - sizeof (si->array) * 8;
	}
	si->bit_number = 0;
}

int start_sieve (
	int	thread_num,	 
	uint64_t start,
	void	**si_to_recycle_or_returned_new_si)	
{
	
	return (start_sieve_with_limit (thread_num, start, 65536, si_to_recycle_or_returned_new_si));
}

int start_sieve_with_limit (
	int	thread_num,	 
	uint64_t start,					
	uint32_t max_elimination_factor,		
	void	**si_to_recycle_or_returned_new_si)	
{
	sieve_info *si;
	unsigned int i;

	if (*si_to_recycle_or_returned_new_si != NULL)
		si = (sieve_info *) *si_to_recycle_or_returned_new_si;
	else {
		si = (sieve_info *) malloc (sizeof (sieve_info));
		if (si == NULL) goto oom;
		*si_to_recycle_or_returned_new_si = si;
		memset (si, 0, sizeof (sieve_info));
	}

	if (start < 2) start = 2;
	si->start = start;
	start |= 1;

	if (si->primes != NULL && max_elimination_factor > si->max_elimination) {
		free (si->primes);
		si->primes = NULL;
		si->first_number = 0;
	}

	if (si->first_number && start >= si->first_number && start < si->first_number + sizeof (si->array) * 8 * 2) {
		si->bit_number = (unsigned int) (start - si->first_number) / 2;
		return (0);
	}

	if (si->primes == NULL) {
		uint32_t f;
		unsigned int estimated_num_primes;
		estimated_num_primes = (unsigned int) ((double) max_elimination_factor / (log ((double) max_elimination_factor) - 1.0) * 1.01);
		si->primes = (uint32_t *) malloc (estimated_num_primes * 2 * sizeof (uint32_t));
		if (si->primes == NULL) goto oom;
		for (i = 0, f = 3; f <= max_elimination_factor && i < estimated_num_primes; f += 2)
			if (isPrime (f)) si->primes[i*2] = f, i++;
		si->num_elimination_primes = i;
		si->max_elimination = max_elimination_factor;
	}

	si->first_number = start;
	si->num_primes = 0;
	fill_sieve (si);
	return (0);

oom:	*si_to_recycle_or_returned_new_si = NULL;
	free (si);
	return (OutOfMemory (thread_num));
}

uint64_t sieve (
	void	*si_arg)
{
	sieve_info *si = (sieve_info *) si_arg;

	if (si->start == 2) {
		si->start = 3;
		return (2);
	}
	for ( ; ; ) {
		unsigned int bit;
		if (si->bit_number == sizeof (si->array) * 8) {
			si->first_number += 2 * sizeof (si->array) * 8;
			fill_sieve (si);
		}
		bit = si->bit_number++;
		if (bittst (si->array, bit))
			return (si->first_number + 2 * bit);
	}
}

void end_sieve (
	void	*si_arg)
{
	sieve_info *si = (sieve_info *) si_arg;
	if (si == NULL) return;
	free (si->primes);
	free (si);
}

uint64_t _intgcd (
	uint64_t a,
	uint64_t b)
{
	while (b != 0) {
		uint64_t temp = a % b;
		a = b;
		b = temp;
	}
	return a;
}

int relatively_prime (
	uint64_t a,
	uint64_t b)
{
	return (_intgcd (a, b) == 1);
}

uint64_t modinv (uint64_t x, uint64_t f)		
{
	uint64_t u = x % f;
	uint64_t v = f;
	uint64_t tmp, q;
	int64_t c = 0, d = 1, stmp;

	while (u > 1) {
		q = u / v;
		tmp = v; v = u % v; u = tmp;
		stmp = c; c = d - q * c; d = stmp;
	}
	if (d < 0) d += f;
	return (d);
}

void sorted_add_unique (int *vals, int *numvals, int newval)
{
	int	i, j;

	for (i = 0; i < *numvals; i++) {
		if (vals[i] == newval) return;
		if (vals[i] < newval) continue;
		j = vals[i];
		vals[i] = newval;
		newval = j;
	}
	vals[*numvals] = newval;
	(*numvals)++;
}

int is_number_in_list (int val, const char *list)
{
	const char *p, *dash, *comma;
	int	start, end;

	for (p = list; ; p = comma+1) {		
		start = atoi (p);
		dash = strchr (p, '-');
		comma = strchr (p, ',');
		if (dash != NULL && (comma == NULL || dash < comma)) end = atoi (dash+1);
		else end = start;
		if (val >= start && val <= end) return (TRUE);
		if (comma == NULL) break;
	}
	return (FALSE);
}

void strupper (
	char	*p)
{
	for ( ; *p; p++) if (*p >= 'a' && *p <= 'z') *p = *p - 'a' + 'A';
}

int isHex (
	const char *p)
{
	for ( ; *p; p++) if (!(*p >= '0' && *p <= '9') && !(*p >= 'a' && *p <= 'f') && !(*p >= 'A' && *p <= 'F')) return (FALSE);
	return (TRUE);
}

unsigned int strToMinutes (
	const char *buf)
{
	unsigned int hours, minutes, pm;

	pm = (strchr (buf, 'P') != NULL || strchr (buf, 'p') != NULL);
	hours = atoi (buf);
	while (isdigit (*buf)) buf++;
	while (*buf && ! isdigit (*buf)) buf++;
	minutes = atoi (buf);
	if (hours == 12) hours -= 12;
	if (pm) hours += 12;
	minutes = hours * 60 + minutes;
	if (minutes > 1440) minutes %= 1440;
	return (minutes);
}

void minutesToStr (
	unsigned int minutes,
	char	*buf)
{
	unsigned int fmt_type, hours, pm;

	hours = minutes / 60;
	fmt_type = IniGetInt (INI_FILE, "AMPM", 0);
	if (fmt_type == 0) fmt_type = getDefaultTimeFormat ();
	if (fmt_type != 1) {
		sprintf (buf, "%d:%02d", hours, minutes % 60);
	} else {
		if (hours >= 12) hours -= 12, pm = 1;
		else pm = 0;
		if (hours == 0) hours = 12;
		sprintf (buf, "%d:%02d %s", hours, minutes % 60, pm ? "PM" : "AM");
	}
}

void write_memory_settings (
	unsigned int day_memory,
	unsigned int night_memory,
	unsigned int day_start_time,
	unsigned int day_end_time)
{
	char	buf[100];

	sprintf (buf, "%d during %d:%02d-%d:%02d else %d",
		 day_memory, day_start_time / 60, day_start_time % 60,
		 day_end_time / 60, day_end_time % 60, night_memory);
	IniWriteString (INI_FILE, "Memory", buf);
	IniWriteString (INI_FILE, "DayMemory", NULL);
	IniWriteString (INI_FILE, "NightMemory", NULL);
	IniWriteString (INI_FILE, "DayStartTime", NULL);
	IniWriteString (INI_FILE, "DayEndTime", NULL);
}

int read_memory_settings (
	unsigned int *day_memory,
	unsigned int *night_memory,
	unsigned int *day_start_time,
	unsigned int *day_end_time)
{
	const char *p, *during_clause, *else_clause;

	*day_memory = 256;
	*night_memory = 256;
	*day_start_time = 450;
	*day_end_time = 1410;

	p = IniSectionGetStringRaw (INI_FILE, NULL, "Memory");
	if (p == NULL) return (TRUE);

	*day_memory = atol (p);
	during_clause = strstr (p, " during ");
	if (during_clause == NULL) {
		*night_memory = *day_memory;
		return (TRUE);
	}

	p = during_clause + 8;
	*day_start_time = atol (p) * 60;
	while (isdigit (*p)) p++;
	if (*p++ != ':') return (FALSE);
	*day_start_time += atol (p);
	while (isdigit (*p)) p++;
	if (*p++ != '-') return (FALSE);
	*day_end_time = atol (p) * 60;
	while (isdigit (*p)) p++;
	if (*p++ != ':') return (FALSE);
	*day_end_time += atol (p);
	while (isdigit (*p)) p++;

	*day_start_time %= 1440;
	*day_end_time %= 1440;

	else_clause = strstr (p, " else ");
	if (p != else_clause) return (FALSE);
	*night_memory = atol (p + 6);

	return (strstr (p, " during ") == NULL);
}

void ini_error_handler (
	const char *filename,
	int	line_number,
	const char *line_text)
{
	char	buf[1200];
	sprintf (buf, "File %s, line number %d is not valid: %s\n", filename, line_number, line_text);
	OutputSomewhere (MAIN_THREAD_NUM, buf);
}

void moveToSection (const char *section, const char *key)
{
	char val[256];
	
	if (strstr (key, "%d") != NULL) {
		for (int i = 1; i < 10000; i++) {
			char	numbered_key[80];
			sprintf (numbered_key, key, i);
			IniGetString (INI_FILE, numbered_key, val, sizeof (val), NULL);
			if (val[0] == 0) return;
			IniSectionWriteString (INI_FILE, section, numbered_key, val);
			IniWriteString (INI_FILE, numbered_key, NULL);
		}
	}
	
	IniGetString (INI_FILE, key, val, sizeof (val), NULL);
	if (val[0]) {
		IniSectionWriteString (INI_FILE, section, key, val);
		IniWriteString (INI_FILE, key, NULL);
	}
}

void renameIniKey (const char *section, const char *old_key_name, const char *new_key_name)
{
	char val[256];
	IniSectionGetString (INI_FILE, section, old_key_name, val, sizeof (val), old_key_name);
	
	if (strcmp (val, old_key_name) == 0) return;
	
	IniSectionWriteString (INI_FILE, section, new_key_name, val);
	IniSectionWriteString (INI_FILE, section, old_key_name, NULL);
}

void nameAndReadIniFiles (
	int	named_ini_files)
{
	char	localini_file[260];
	char	buf[513];

	hwloc_topology_init (&hwloc_topology);
	hwloc_topology_load (hwloc_topology);

	{
		const struct hwloc_topology_support *support;
		OS_CAN_SET_AFFINITY = 1;
		support = hwloc_topology_get_support (hwloc_topology);
		if (support == NULL || ! support->cpubind->set_thread_cpubind) OS_CAN_SET_AFFINITY = 0;
	}

	gwmutex_init (&OUTPUT_MUTEX);
	gwmutex_init (&LOG_MUTEX);
	gwmutex_init (&WORKTODO_MUTEX);

	if (named_ini_files < 0) {
		strcpy (INI_FILE, "prime.txt");
		strcpy (localini_file, "local.txt");
		strcpy (WORKTODO_FILE, "worktodo.txt");
		strcpy (RESFILE, "results.txt");
		strcpy (RESFILEBENCH, "results.bench.txt");
		strcpy (RESFILEJSON, "results.json.txt");
		strcpy (SPOOL_FILE, "prime.spl");
		strcpy (LOGFILE, "prime.log");
		strcpy (SCREENLOGFILE, "screen.log");
	} else {
		sprintf (INI_FILE, "prim%04d.txt", named_ini_files);
		sprintf (localini_file, "loca%04d.txt", named_ini_files);
		sprintf (WORKTODO_FILE, "work%04d.txt", named_ini_files);
		sprintf (RESFILE, "resu%04d.txt", named_ini_files);
		sprintf (RESFILEBENCH, "resu%04d.bench.txt", named_ini_files);
		sprintf (RESFILEJSON, "resu%04d.json.txt", named_ini_files);
		sprintf (SPOOL_FILE, "prim%04d.spl", named_ini_files);
		sprintf (LOGFILE, "prim%04d.log", named_ini_files);
		sprintf (SCREENLOGFILE, "screen%04d.log", named_ini_files);
	}

	IniGetString (INI_FILE, "WorkingDir", buf, sizeof(buf), NULL);
	IniGetString (INI_FILE, "local.ini", localini_file, 260, localini_file);
	IniGetString (INI_FILE, "worktodo.ini", WORKTODO_FILE, 260, WORKTODO_FILE);
	IniGetString (INI_FILE, "results.txt", RESFILE, 260, RESFILE);
	IniGetString (INI_FILE, "results.bench.txt", RESFILEBENCH, 260, RESFILEBENCH);
	IniGetString (INI_FILE, "results.json.txt", RESFILEJSON, 260, RESFILEJSON);
	IniGetString (INI_FILE, "prime.spl", SPOOL_FILE, 260, SPOOL_FILE);
	IniGetString (INI_FILE, "prime.log", LOGFILE, 260, LOGFILE);
	IniGetString (INI_FILE, "screen.log", SCREENLOGFILE, 260, SCREENLOGFILE);
	IniGetString (INI_FILE, "prime.ini", INI_FILE, 260, INI_FILE);
	if (buf[0]) {
		(void) _chdir (buf);
		IniFileReread (INI_FILE);
	}

	{
		FILE *fd = fopen (localini_file, "r");
		if (fd != NULL) {
			char	section[80];
			char	line[1000];
			section[0] = 0;
			IniDelayWrites (INI_FILE);
			while (fgets (line, sizeof (line), fd)) {
				if (line[strlen(line)-1] == '\n') line[strlen(line)-1] = 0;
				if (line[0] && line[strlen(line)-1] == '\r') line[strlen(line)-1] = 0;
				if (line[0] == 0) continue;
				
				if (line[0] == '[') {
					strcpy (section, line+1);
					if (strchr (section, ']') != NULL) *strchr (section, ']') = 0;
					continue;
				}
				
				if (section[0]) IniSectionWriteString (INI_FILE, section, NULL, line);
				else IniWriteString (INI_FILE, NULL, line);
			}
			fclose (fd);
			_unlink (localini_file);
			IniResumeImmediateWrites (INI_FILE);
			IniFileReread (INI_FILE);
		}
	}

	renameIniKey (NULL, "WorkerThreads", KEY_NumWorkers);			
	renameIniKey (NULL, "NoMoreWork", KEY_QuitGIMPS);			
	renameIniKey (NULL, "BenchAllComplex", "BenchNegacyclic");		

	IniWriteString (INI_FILE, "RollingAverageIsFromV27", NULL);		
	IniWriteString (INI_FILE, "V24OptionsConverted", NULL);			
	IniWriteString (INI_FILE, "AskedAboutMemory", NULL);			
	IniWriteString (INI_FILE, "Advanced", NULL);				
	IniWriteString (INI_FILE, "Affinity", NULL);				
	IniWriteString (INI_FILE, "W0", NULL);					
	IniWriteString (INI_FILE, "OldStyleSoftCrossover", NULL);		
	IniWriteString (INI_FILE, "SoftCrossover", NULL);			
	IniWriteString (INI_FILE, "SoftCrossoverAdjust", NULL);			

	moveToSection (SEC_Internals, KEY_V30OptionsConverted);			
	moveToSection (SEC_Internals, KEY_OldCpuSpeed);				
	moveToSection (SEC_Internals, KEY_NewCpuSpeed);				
	moveToSection (SEC_Internals, KEY_NewCpuSpeedCount);			
	moveToSection (SEC_Internals, KEY_RollingAverage);			
	moveToSection (SEC_Internals, KEY_RollingHash);				
	moveToSection (SEC_Internals, KEY_RollingStartTime);			
	moveToSection (SEC_Internals, KEY_RollingCompleteTime);			
	moveToSection (SEC_Internals, KEY_CertDailyCPURemaining);		
	moveToSection (SEC_Internals, KEY_CertDailyMBRemaining);		
	moveToSection (SEC_Internals, KEY_CertDailyRemainingLastUpdate);	
	moveToSection (SEC_Internals, KEY_SrvrUID);				
	moveToSection (SEC_Internals, KEY_SrvrComputerName);			
	moveToSection (SEC_Internals, KEY_SrvrP00);				
	moveToSection (SEC_Internals, KEY_SrvrPO1);				
	moveToSection (SEC_Internals, KEY_SrvrPO2);				
	moveToSection (SEC_Internals, KEY_SrvrPO3);				
	moveToSection (SEC_Internals, KEY_SrvrPO4);				
	moveToSection (SEC_Internals, KEY_SrvrPO5);				
	moveToSection (SEC_Internals, KEY_SrvrPO6);				
	moveToSection (SEC_Internals, KEY_SrvrPO7);				
	moveToSection (SEC_Internals, KEY_SrvrPO8);				
	moveToSection (SEC_Internals, KEY_SrvrPO9);				
	moveToSection (SEC_Internals, KEY_LastEndDatesSent);			
	moveToSection (SEC_Internals, KEY_WGUID_version);			
	moveToSection (SEC_Internals, KEY_CertErrorCount);			

	moveToSection (SEC_PrimeNet, KEY_DialUp);				
	moveToSection (SEC_PrimeNet, KEY_ProxyHost);				
	moveToSection (SEC_PrimeNet, KEY_ProxyUser);				
	moveToSection (SEC_PrimeNet, KEY_ProxyPass);				
	moveToSection (SEC_PrimeNet, KEY_ProxyMask);				
	moveToSection (SEC_PrimeNet, KEY_Debug);				
	moveToSection (SEC_PrimeNet, KEY_UploadRateLimit);			
	moveToSection (SEC_PrimeNet, KEY_UploadStartTime);			
	moveToSection (SEC_PrimeNet, KEY_UploadEndTime);			
	moveToSection (SEC_PrimeNet, KEY_DownloadDailyLimit);			
	moveToSection (SEC_PrimeNet, KEY_DownloadRateLimit);			
	moveToSection (SEC_PrimeNet, KEY_ProofUploads);				
	moveToSection (SEC_PrimeNet, KEY_ProofHashLength);			
	
	moveToSection (SEC_PrimeNet, KEY_UploadChunkSize);			

	moveToSection (SEC_Windows, KEY_MergeWindows);				
	moveToSection (SEC_Windows, KEY_TrayIcon);				
	moveToSection (SEC_Windows, KEY_HideIcon);				
	moveToSection (SEC_Windows, KEY_Left);					
	moveToSection (SEC_Windows, KEY_Right);					
	moveToSection (SEC_Windows, KEY_Top);					
	moveToSection (SEC_Windows, KEY_Bottom);				
	moveToSection (SEC_Windows, KEY_ExitOnX);				
	moveToSection (SEC_Windows, "W%d");					

	LoadPrimenet ();
	init_spool_file_and_comm_code ();
	init_timed_event_handler ();

	MERGE_WINDOWS = (int) IniSectionGetInt (INI_FILE, SEC_Windows, KEY_MergeWindows, MERGE_MAINCOMM_WINDOWS);
	create_window (MAIN_THREAD_NUM);
	base_title (MAIN_THREAD_NUM, "Main thread");

	IniSetErrorCallback (ini_error_handler);

	sprintf (buf, "Mersenne number primality test program version %s build %s\n", VERSION, BUILD_NUM);
	OutputStr (MAIN_THREAD_NUM, buf);

	readIniFiles ();

	if (!COMPUTER_GUID[0]) generate_computer_guid ();

	calc_windows_guid ();

	if (STRESS_TESTER == 1 && (USE_PRIMENET || WORKTODO_COUNT)) {
		STRESS_TESTER = 0;
		IniWriteInt (INI_FILE, "StressTester", 0);
	}

	sprintf (buf, "Optimizing for CPU architecture: %s, ",
		 CPU_ARCHITECTURE == CPU_ARCHITECTURE_PRE_SSE2 ? "Pre-SSE2" :
		 CPU_ARCHITECTURE == CPU_ARCHITECTURE_PENTIUM_4 ? "Pentium 4" :
		 CPU_ARCHITECTURE == CPU_ARCHITECTURE_PENTIUM_M ? "Pentium M" :
		 CPU_ARCHITECTURE == CPU_ARCHITECTURE_CORE ? "Core Solo/Duo" :
		 CPU_ARCHITECTURE == CPU_ARCHITECTURE_CORE_2 ? "Core 2" :
		 CPU_ARCHITECTURE == CPU_ARCHITECTURE_CORE_I7 ? "Core i3/i5/i7" :
		 CPU_ARCHITECTURE == CPU_ARCHITECTURE_PHI ? "Xeon Phi" :
		 CPU_ARCHITECTURE == CPU_ARCHITECTURE_ATOM ? "Atom" :
		 CPU_ARCHITECTURE == CPU_ARCHITECTURE_INTEL_OTHER ? "Unknown Intel" :
		 CPU_ARCHITECTURE == CPU_ARCHITECTURE_AMD_K8 ? "AMD K8" :
		 CPU_ARCHITECTURE == CPU_ARCHITECTURE_AMD_K10 ? "AMD K10" :
		 CPU_ARCHITECTURE == CPU_ARCHITECTURE_AMD_BULLDOZER ? "AMD Bulldozer" :
		 CPU_ARCHITECTURE == CPU_ARCHITECTURE_AMD_ZEN ? "AMD Zen" :
		 CPU_ARCHITECTURE == CPU_ARCHITECTURE_OTHER ? "Not Intel and not AMD" : "Undefined");
	strcat (buf, "L2 cache size: ");
	if (CPU_NUM_L2_CACHES <= 0) strcat (buf, "unknown");
	if (CPU_NUM_L2_CACHES > 1) sprintf (buf + strlen (buf), "%dx", CPU_NUM_L2_CACHES);
	if (CPU_NUM_L2_CACHES >= 1) {
		if ((CPU_TOTAL_L2_CACHE_SIZE / CPU_NUM_L2_CACHES) & 0x3FF)
			sprintf (buf + strlen (buf), "%d KB", CPU_TOTAL_L2_CACHE_SIZE / CPU_NUM_L2_CACHES);
		else
			sprintf (buf + strlen (buf), "%d MB", CPU_TOTAL_L2_CACHE_SIZE / CPU_NUM_L2_CACHES / 1024);
	}
	if (CPU_NUM_L3_CACHES > 0) {
		strcat (buf, ", L3 cache size: ");
		if (CPU_NUM_L3_CACHES > 1) sprintf (buf + strlen (buf), "%dx", CPU_NUM_L3_CACHES);
		if ((CPU_TOTAL_L3_CACHE_SIZE / CPU_NUM_L3_CACHES) & 0x3FF)
			sprintf (buf + strlen (buf), "%d KB", CPU_TOTAL_L3_CACHE_SIZE / CPU_NUM_L3_CACHES);
		else
			sprintf (buf + strlen (buf), "%d MB", CPU_TOTAL_L3_CACHE_SIZE / CPU_NUM_L3_CACHES / 1024);
	}
	strcat (buf, "\n");
	OutputStr (MAIN_THREAD_NUM, buf);
	if (!OS_CAN_SET_AFFINITY) OutputStr (MAIN_THREAD_NUM, "OS does not support setting CPU affinity.\n");

	add_timed_event (TE_ROLLING_AVERAGE, 6*60*60);
	if (IniGetInt (INI_FILE, "AutoBench", 1)) {
		time_t	current_time;
		struct tm *x;
		int	seconds;
		gwevent_init (&AUTOBENCH_EVENT);
		time (&current_time);
		x = localtime (&current_time);
		if (x->tm_hour < 4)
			seconds = (5 - x->tm_hour) * 60 * 60;	
		else
			seconds = (29 - x->tm_hour) * 60 * 60;	
		add_timed_event (TE_BENCH, seconds);
	}

	if (IniSectionGetInt (INI_FILE, SEC_PrimeNet, KEY_ProofUploads, 1))
		gwthread_create (&UPLOAD_THREAD, &proofUploader, NULL);
}

void initCommCode (void) {

	set_comm_timers ();

	spoolMessage (PRIMENET_PROGRAM_OPTIONS, NULL);

	if (USE_PRIMENET && USERID[0] == 0)
		spoolMessage (PRIMENET_UPDATE_COMPUTER_INFO, NULL);
}

int good_default_for_num_workers (void)
{
	int	cores_per_node;

	cores_per_node = HW_NUM_COMPUTE_CORES / HW_NUM_COMPUTE_THREADING_NODES;
	return (HW_NUM_COMPUTE_THREADING_NODES * (cores_per_node <= 6 ? 1 : divide_rounding_up (cores_per_node, 4)));
}

void read_cores_per_test (void)
{
	int	i, all_the_same;
	unsigned int temp[MAX_NUM_WORKERS];

	PTOGetAll (INI_FILE, "ThreadsPerTest", temp, 0);

	if (temp[0] != 0) {
		int	total_threads, trim;
		
		for (i = 0, total_threads = 0; i < (int) NUM_WORKERS; i++) {
			if (temp[i] <= 0) temp[i] = 1;  
			total_threads += temp[i];
		}
		
		for (i = 0; total_threads > (int) HW_NUM_CORES && i < (int) NUM_WORKERS; i++) {
			trim = temp[i] / 2;
			if (total_threads - trim < (int) HW_NUM_CORES) trim = total_threads - HW_NUM_CORES;
			temp[i] -= trim;
			total_threads -= trim;
		}
		
		for (i = 0, all_the_same = TRUE; i < (int) NUM_WORKERS; i++) {
			if (i && temp[i] != temp[i-1]) all_the_same = FALSE;
		}
		
		memset (CORES_PER_TEST, 0xFF, sizeof (CORES_PER_TEST));
		if (all_the_same)
			PTOSetAll (INI_FILE, "CoresPerTest", NULL, CORES_PER_TEST, temp[0]);
		else for (i = 0; i < (int) NUM_WORKERS; i++)
			PTOSetOne (INI_FILE, "CoresPerTest", NULL, CORES_PER_TEST, i, temp[i]);
		
		PTOSetAll (INI_FILE, "ThreadsPerTest", NULL, temp, 0);
		IniWriteString (INI_FILE, "ThreadsPerTest", NULL);
	}

	PTOGetAll (INI_FILE, "CoresPerTest", CORES_PER_TEST, 0);

	if (CORES_PER_TEST[0] == 0) {
		int	cores, workers, node;

		i = 0;
		cores = HW_NUM_COMPUTE_CORES;
		workers = NUM_WORKERS;
		for (node = 0; node < (int) HW_NUM_COMPUTE_THREADING_NODES; node++) {
			
			int cores_this_node = get_cores_in_threading_node (node);
			int workers_this_node = divide_rounding (workers * cores_this_node, cores);
			
			cores -= cores_this_node;
			workers -= workers_this_node;
			
			for ( ; workers_this_node; workers_this_node--) {
				temp[i] = cores_this_node / workers_this_node;
				cores_this_node -= temp[i];
				i++;
			}
		}
		
		ASSERTG (workers == 0 && cores == 0);

		for (i = 0, all_the_same = TRUE; i < (int) NUM_WORKERS; i++) {
			if (i && temp[i] != temp[i-1]) all_the_same = FALSE;
		}
		
		if (all_the_same)
			PTOSetAll (INI_FILE, "CoresPerTest", NULL, CORES_PER_TEST, temp[0]);
		else for (i = 0; i < (int) NUM_WORKERS; i++)
			PTOSetOne (INI_FILE, "CoresPerTest", NULL, CORES_PER_TEST, i, temp[i]);
	}

	for (i = 0; i < (int) NUM_WORKERS; i++) {
		if (CORES_PER_TEST[i] < 1) CORES_PER_TEST[i] = 1;
		if (CORES_PER_TEST[i] > HW_NUM_CORES) CORES_PER_TEST[i] = HW_NUM_CORES;
	}
}

int readIniFiles (void)
{
	int	rc, temp;
	int	day_memory;
	int	night_memory;

	IniFileReread (INI_FILE);
	incorporateIniAddFiles ();

	getCpuInfo ();

	IniGetString (INI_FILE, "ComputerGUID", COMPUTER_GUID, sizeof (COMPUTER_GUID), NULL);
	IniGetString (INI_FILE, "V5UserID", USERID, sizeof (USERID), NULL);
	IniGetString (INI_FILE, "ComputerID", COMPID, sizeof (COMPID), NULL);
	sanitizeString (COMPID);
	USE_PRIMENET = (int) IniGetInt (INI_FILE, "UsePrimenet", 0);
	DIAL_UP = (int) IniSectionGetInt (INI_FILE, SEC_PrimeNet, KEY_DialUp, 0);
	DAYS_OF_WORK = (unsigned int) IniGetInt (INI_FILE, "DaysOfWork", 3);
	if (DAYS_OF_WORK > 180) DAYS_OF_WORK = 180;

	CPU_WORKER_DISK_SPACE = IniGetFloat (INI_FILE, "WorkerDiskSpace", 6.0);
	if (CPU_WORKER_DISK_SPACE < 0.0) CPU_WORKER_DISK_SPACE = 0.0;
	if (CPU_WORKER_DISK_SPACE > 1000.0) CPU_WORKER_DISK_SPACE = 1000.0;

	CPU_HOURS = (unsigned int) IniGetInt (INI_FILE, "CPUHours", 24);
	if (CPU_HOURS < 1) CPU_HOURS = 1;
	if (CPU_HOURS > 24) CPU_HOURS = 24;

	ROLLING_AVERAGE = (unsigned int) IniSectionGetInt (INI_FILE, SEC_Internals, KEY_RollingAverage, 1000);
	if (ROLLING_AVERAGE < 10) ROLLING_AVERAGE = 10;
	if (ROLLING_AVERAGE > 4000) ROLLING_AVERAGE = 4000;

	PRECISION = (unsigned int) IniGetInt (INI_FILE, "PercentPrecision", 2);
	if (PRECISION > 6) PRECISION = 6;

	CLASSIC_OUTPUT = IniGetInt (INI_FILE, "ClassicOutput", 0);
	OUTPUT_ROUNDOFF = IniGetInt (INI_FILE, "OutputRoundoff", 0);
	ITER_OUTPUT = IniGetInt (INI_FILE, "OutputIterations", 10000);
	if (ITER_OUTPUT > 999999999) ITER_OUTPUT = 999999999;
	if (ITER_OUTPUT <= 0) ITER_OUTPUT = 1;
	ITER_OUTPUT_RES = IniGetInt (INI_FILE, "ResultsFileIterations", 999999999);
	if (ITER_OUTPUT_RES > 999999999) ITER_OUTPUT_RES = 999999999;
	if (ITER_OUTPUT_RES < 1000) ITER_OUTPUT_RES = 1000;
	DISK_WRITE_TIME = IniGetInt (INI_FILE, "DiskWriteTime", 30);
	JACOBI_TIME = IniGetInt (INI_FILE, "JacobiErrorCheckingInterval", 12);
	if (JACOBI_TIME < 1) JACOBI_TIME = 1;
	MODEM_RETRY_TIME = (unsigned int) IniGetInt (INI_FILE, "NetworkRetryTime", 2);
	if (MODEM_RETRY_TIME < 1) MODEM_RETRY_TIME = 1;
	if (MODEM_RETRY_TIME > 300) MODEM_RETRY_TIME = 300;
	NETWORK_RETRY_TIME = (unsigned int)
		IniGetInt (INI_FILE, "NetworkRetryTime2",
			   MODEM_RETRY_TIME > 70 ? MODEM_RETRY_TIME : 70);
	if (NETWORK_RETRY_TIME < 1) NETWORK_RETRY_TIME = 1;
	if (NETWORK_RETRY_TIME > 300) NETWORK_RETRY_TIME = 300;

	DAYS_BETWEEN_CHECKINS = IniGetFloat (INI_FILE, "DaysBetweenCheckins", 1.0);
	if (DAYS_BETWEEN_CHECKINS > 7.0) DAYS_BETWEEN_CHECKINS = 7.0;				
	if (DAYS_BETWEEN_CHECKINS * 24.0 < 1.0) DAYS_BETWEEN_CHECKINS = (float) (1.0 / 24.0);	
	SILENT_VICTORY = (int) IniGetInt (INI_FILE, "SilentVictory", 0);
	SILENT_VICTORY_PRP = (int) IniGetInt (INI_FILE, "SilentVictoryPRP", 1);
	RUN_ON_BATTERY = (int) IniGetInt (INI_FILE, "RunOnBattery", 1);
	BATTERY_PERCENT = (int) IniGetInt (INI_FILE, "BatteryPercent", 0);
	DEFEAT_POWER_SAVE = (int) IniGetInt (INI_FILE, "DefeatPowerSave", 1);

	STRESS_TESTER = (int) IniGetInt (INI_FILE, "StressTester", 99);
	temp = (int) IniGetInt (INI_FILE, "ErrorCheck", 0);
	ERRCHK = (temp != 0);
	NUM_WORKERS = IniGetInt (INI_FILE, KEY_NumWorkers, good_default_for_num_workers ());
	if (NUM_WORKERS < 1) NUM_WORKERS = 1;
	if (NUM_WORKERS > MAX_NUM_WORKERS) NUM_WORKERS = MAX_NUM_WORKERS;
	IniWriteInt (INI_FILE, KEY_NumWorkers, NUM_WORKERS); 
	PRIORITY = (unsigned int) IniGetInt (INI_FILE, "Priority", 1);
	if (PRIORITY < 1) PRIORITY = 1;
	if (PRIORITY > 10) PRIORITY = 10;
	PTOGetAll (INI_FILE, "WorkPreference", WORK_PREFERENCE, 0);
	
	if (!IniSectionGetInt (INI_FILE, SEC_Internals, KEY_V30OptionsConverted, 0)) {
		int	i, all_the_same = TRUE;
		for (i = 0; i < MAX_NUM_WORKERS; i++) {
			if (WORK_PREFERENCE[i] == PRIMENET_WP_LL_FIRST) WORK_PREFERENCE[i] = PRIMENET_WP_PRP_FIRST;
			if (WORK_PREFERENCE[i] == PRIMENET_WP_LL_10M) WORK_PREFERENCE[i] = PRIMENET_WP_PRP_FIRST;
			if (WORK_PREFERENCE[i] == PRIMENET_WP_LL_FIRST_NOFAC) WORK_PREFERENCE[i] = PRIMENET_WP_PRP_FIRST;
			if (WORK_PREFERENCE[i] == PRIMENET_WP_LL_WORLD_RECORD) WORK_PREFERENCE[i] = PRIMENET_WP_PRP_WORLD_RECORD;
			if (WORK_PREFERENCE[i] == PRIMENET_WP_LL_100M) WORK_PREFERENCE[i] = PRIMENET_WP_PRP_100M;
			if (WORK_PREFERENCE[i] == PRIMENET_WP_LL_DBLCHK) WORK_PREFERENCE[i] = PRIMENET_WP_PRP_DBLCHK;
			if (i && WORK_PREFERENCE[i] != WORK_PREFERENCE[i-1]) all_the_same = FALSE;
		}
		if (all_the_same)
			PTOSetAll (INI_FILE, "WorkPreference", NULL, WORK_PREFERENCE, WORK_PREFERENCE[0]);
		else for (i = 0; i < (int) NUM_WORKERS; i++)
			PTOSetOne (INI_FILE, "WorkPreference", NULL, WORK_PREFERENCE, i, WORK_PREFERENCE[i]);
		spoolMessage (PRIMENET_PROGRAM_OPTIONS, NULL);
		IniSectionWriteInt (INI_FILE, SEC_Internals, KEY_V30OptionsConverted, 1);
	}
	read_cores_per_test ();		
	HYPERTHREAD_TF = IniGetInt (INI_FILE, "HyperthreadTF", OS_CAN_SET_AFFINITY);
	HYPERTHREAD_LL = IniGetInt (INI_FILE, "HyperthreadLL", 0);
	MANUAL_COMM = (int) IniGetInt (INI_FILE, "ManualComm", 0);
	HIDE_ICON = (int) IniSectionGetInt (INI_FILE, SEC_Windows, KEY_HideIcon, 0);
	TRAY_ICON = (int) IniSectionGetInt (INI_FILE, SEC_Windows, KEY_TrayIcon, 1);
	MERGE_WINDOWS = (int) IniSectionGetInt (INI_FILE, SEC_Windows, KEY_MergeWindows, MERGE_MAINCOMM_WINDOWS);

	temp = (int) IniGetInt (INI_FILE, "TwoBackupFiles", 2);
	NUM_BACKUP_FILES = (int) IniGetInt (INI_FILE, "NumBackupFiles", temp+1);
	NUM_JACOBI_BACKUP_FILES = (int) IniGetInt (INI_FILE, "JacobiBackupFiles", 2);

	day_memory = IniGetInt (INI_FILE, "DayMemory", 0);
	night_memory = IniGetInt (INI_FILE, "NightMemory", 0);
	if (day_memory || night_memory) {
		int	day_start_time, day_end_time;
		day_memory = (day_memory < 0) ? physical_memory () + day_memory : day_memory;
		if (day_memory < 8) day_memory = 8;
		if ((unsigned long) day_memory > physical_memory () - 8) day_memory = physical_memory () - 8;
		night_memory = (night_memory < 0) ? physical_memory () + night_memory : night_memory;
		if (night_memory < 8) night_memory = 8;
		if ((unsigned long) night_memory > physical_memory () - 8) night_memory = physical_memory () - 8;
		day_start_time = IniGetInt (INI_FILE, "DayStartTime", 450);
		day_end_time = IniGetInt (INI_FILE, "DayEndTime", 1410);
		write_memory_settings (day_memory, night_memory, day_start_time, day_end_time);
	}
	read_mem_info ();

	RDTSC_TIMING = IniGetInt (INI_FILE, "RdtscTiming", 1);
	if (RDTSC_TIMING < 10 && ! isHighResTimerAvailable ())
		RDTSC_TIMING += 10;

	TIMESTAMPING = IniGetInt (INI_FILE, "TimeStamp", 1);
	TIMESTAMPING_UTC = IniGetInt (INI_FILE, "TimeStampUTC", 0);
	LOGTIMESTAMPING = IniGetInt (INI_FILE, "LogTimeStamp", 1);
	LOGTIMESTAMPING_UTC = IniGetInt (INI_FILE, "LogTimeStampUTC", 0);
	SCREENLOG = IniGetInt (INI_FILE, "ScreenLog", 0);
	CUMULATIVE_TIMING = IniGetInt (INI_FILE, "CumulativeTiming", 0);
	CUMULATIVE_ROUNDOFF = IniGetInt (INI_FILE, "CumulativeRoundoff", 1);
	SEQUENTIAL_WORK = IniGetInt (INI_FILE, "SequentialWorkToDo", -1);
	WELL_BEHAVED_WORK = IniGetInt (INI_FILE, "WellBehavedWork", 0);

	read_pause_info ();
	read_load_average_info ();

	INTERIM_FILES = IniGetInt (INI_FILE, "InterimFiles", 0);
	INTERIM_RESIDUES = IniGetInt (INI_FILE, "InterimResidues", INTERIM_FILES);
	HYPERTHREADING_BACKOFF = IniGetInt (INI_FILE, "HyperthreadingBackoff", 0);

	THROTTLE_PCT = IniGetInt (INI_FILE, "Throttle", 0);

	rc = readWorkToDoFile ();
	if (rc) {
		OutputStr (MAIN_THREAD_NUM, "Error reading worktodo.txt file\n");
		return (rc);
	}

	return (0);
}

int addFileExists (void)
{
	char	filename[80];
	char	*dot;

	strcpy (filename, INI_FILE);
	dot = strrchr (filename, '.');
	if (dot != NULL) {
		strcpy (dot, ".add");
		if (fileExists (filename)) return (TRUE);
		strcpy (dot, ".add.txt");
		if (fileExists (filename)) return (TRUE);
	}
	return (FALSE);
}

void incorporateIniAddFiles (void)
{
	char	filename[80];
	char	*dot;

	strcpy (filename, INI_FILE);
	dot = strrchr (filename, '.');
	if (dot != NULL) {
		strcpy (dot, ".add");
		if (fileExists (filename))
			IniAddFileMerge (INI_FILE, filename, NULL);
		strcpy (dot, ".add.txt");
		if (fileExists (filename))
			IniAddFileMerge (INI_FILE, filename, NULL);
	}
}

int incorporateWorkToDoAddFile (void)
{
static	int	worktodo_add_disabled = FALSE;
	char	filename[80];
	char	*dot;
	int	rc;
	FILE	*fd;
	unsigned int tnum;
	char	line[2048];

	if (worktodo_add_disabled) return (0);

	strcpy (filename, WORKTODO_FILE);
	dot = strrchr (filename, '.');
	if (dot == NULL) return (0);

	strcpy (dot, ".add");
	fd = fopen (filename, "r");
	if (fd == NULL) {
		strcpy (dot, ".add.txt");
		fd = fopen (filename, "r");
		if (fd == NULL) return (0);
	}

	gwmutex_lock (&WORKTODO_MUTEX);
	tnum = 0;
	while (fgets (line, sizeof (line), fd)) {
		struct work_unit *w;
		int	rc;

		if (line[strlen(line)-1] == '\n') line[strlen(line)-1] = 0;
		if (line[0] && line[strlen(line)-1] == '\r') line[strlen(line)-1] = 0;
		if (line[0] == 0) continue;

		if (line[0] == '[') {
			struct work_unit *w;
			for (tnum = 0; ; tnum++) {
				w = WORK_UNITS[tnum].first;
				if (w == NULL) {
					if (tnum) break;
				} else {
					if (w->work_type == WORK_NONE && _stricmp (w->comment, line) == 0) break;
				}
				if (tnum == NUM_WORKERS - 1) {
					w = NULL;
					break;
				}
			}
			if (w != NULL) continue;
		}

		w = (struct work_unit *) malloc (sizeof (struct work_unit));
		if (w == NULL) goto nomem;

		rc = parseWorkToDoLine (line, w);
		if (rc) goto retrc;

		rc = addToWorkUnitArray (tnum, w, ADD_TO_END);
		if (rc) goto retrc;
	}

	fclose (fd);
	gwmutex_unlock (&WORKTODO_MUTEX);

	WORKTODO_CHANGED = TRUE;
	rc = writeWorkToDoFile (TRUE);
	if (rc) return (rc);

	_unlink (filename);
	if (fileExists (filename)) {
		OutputBoth (MAIN_THREAD_NUM, "ERROR: Can't delete worktodo.add file\n");
		worktodo_add_disabled = TRUE;
	}

	rc = readWorkToDoFile ();
	spoolMessage (MSG_CHECK_WORK_QUEUE, NULL);
	return (rc);

nomem:	rc = OutOfMemory (MAIN_THREAD_NUM);
retrc:	fclose (fd);
	gwmutex_unlock (&WORKTODO_MUTEX);
	return (rc);
}

void PTOGetAll (
	const char *ini_filename,	
	const char *keyword,		
	unsigned int *array,		
	unsigned int def_val)		
{
	int	i, global_val;
	unsigned int seconds;
	char	section_name[32];

	global_val = IniGetTimedInt (ini_filename, keyword, def_val, &seconds);
	if (seconds) add_timed_event (TE_READ_INI_FILE, seconds);
	for (i = 0; i < MAX_NUM_WORKERS; i++) {
		array[i] = global_val;
	}

	for (i = 0; i < MAX_NUM_WORKERS; i++) {
		sprintf (section_name, "Worker #%d", i+1);
		array[i] = IniSectionGetTimedInt (ini_filename, section_name, keyword, array[i], &seconds);
		if (seconds) add_timed_event (TE_READ_INI_FILE, seconds);
	}
}

void PTOSetAll (
	const char *ini_filename,	
	const char *keyword,		
	const char *shadow_keyword,	
	unsigned int *array,		
	unsigned int new_val)		
{
	int	i;
	char	section_name[32];

	IniWriteInt (ini_filename, keyword, new_val);
	for (i = 0; i < MAX_NUM_WORKERS; i++) {
		array[i] = new_val;
	}
	if (shadow_keyword != NULL) IniSectionWriteInt (INI_FILE, SEC_Internals, shadow_keyword, new_val);

	for (i = 0; i < MAX_NUM_WORKERS; i++) {
		sprintf (section_name, "Worker #%d", i+1);
		IniSectionWriteString (ini_filename, section_name, keyword, NULL);
		if (shadow_keyword != NULL) IniSectionWriteString (INI_FILE, section_name, shadow_keyword, NULL);
	}
}

void PTOSetOne (
	const char *ini_filename,	
	const char *keyword,		
	const char *shadow_keyword,	
	unsigned int *array,		
	int	tnum,			
	unsigned int new_val)		
{
	char	section_name[32];

	if (PTOIsGlobalOption (array)) {
		int	i;

		if (array[tnum] == new_val) return;

		IniWriteString (ini_filename, keyword, NULL);
		if (shadow_keyword != NULL) IniSectionWriteString (INI_FILE, SEC_Internals, shadow_keyword, NULL);
		for (i = 0; i < (int) NUM_WORKERS; i++) {
			sprintf (section_name, "Worker #%d", i+1);
			IniSectionWriteInt (ini_filename, section_name, keyword, array[i]);
			if (shadow_keyword != NULL) IniSectionWriteInt (INI_FILE, section_name, shadow_keyword, array[i]);
		}
	}

	array[tnum] = new_val;
	sprintf (section_name, "Worker #%d", tnum+1);
	IniSectionWriteInt (ini_filename, section_name, keyword, new_val);
	if (shadow_keyword != NULL) IniSectionWriteInt (INI_FILE, section_name, shadow_keyword, new_val);
}

int PTOIsGlobalOption (
	unsigned int *array)		
{
	for (int i = 1; i < (int) NUM_WORKERS; i++)
		if (array[i-1] != array[i]) return (FALSE);
	return (TRUE);
}

int PTOHasOptionChanged (
	const char *shadow_keyword,	
	unsigned int *array,		
	int	tnum)			
{
	char	section_name[32];

	if (tnum == -1) return (PTOIsGlobalOption (array) && array[0] != IniSectionGetInt (INI_FILE, SEC_Internals, shadow_keyword, -1));

	sprintf (section_name, "Worker #%d", tnum+1);
	return (!PTOIsGlobalOption (array) && array[tnum] != IniSectionGetInt (INI_FILE, section_name, shadow_keyword, -1));
}

void timestamp (
	char	*buf,
	int	type)		
{
	char	user_defined_fmt[100];
	const char *fmtstr;
	time_t	this_time;
	int	code, utc;

	if (type == 0) {	
		fmtstr = "%b %e %H:%M";				
		code = TIMESTAMPING;
		utc = TIMESTAMPING_UTC;
	} else {		
		fmtstr = "%a %b %e %H:%M:%S %Y";		
		code = LOGTIMESTAMPING;
		utc = LOGTIMESTAMPING_UTC;
	}
	if (code == 0) { *buf = 0; return; }			
	if (code == 2) fmtstr = "%b %e %H:%M:%S";		
	else if (code == 3) fmtstr = "%H:%M";			
	else if (code == 4) fmtstr = "%H:%M:%S";		
	else if (code == 5) fmtstr = "%Y-%m-%d %H:%M";		
	else if (code == 6) fmtstr = "%Y-%m-%d %H:%M:%S";	
	else if (code == 7) {					
		IniGetString (INI_FILE, type == 0 ? "TimestampFormat" : "LogTimestampFormat", user_defined_fmt, sizeof (user_defined_fmt), fmtstr);
		fmtstr = user_defined_fmt;
	}

	time (&this_time);
	if (utc) strftime (buf, 80, fmtstr, gmtime (&this_time));
	else strftime (buf, 80, fmtstr, localtime (&this_time));
}

void OutputStrInternal (
	int	thread_num,
	const char *buf)
{
static	int	last_char_out_was_newline = TRUE;
	
	RealOutputStr (thread_num, buf);
	if ((SCREENLOG & 0xF) == 1 ||					
	    ((SCREENLOG & 0x2) && thread_num >= 0) ||			
	    ((SCREENLOG & 0x4) && thread_num == COMM_THREAD_NUM) ||	
	    ((SCREENLOG & 0x8) && thread_num == MAIN_THREAD_NUM)) {	
		int fd = _open (SCREENLOGFILE, _O_TEXT | _O_RDWR | _O_CREAT | _O_APPEND, CREATE_FILE_ACCESS);
		if (fd >= 0) {
			if (last_char_out_was_newline && !(SCREENLOG & 0x10)) {
				char	prefix[40];
				if (thread_num == MAIN_THREAD_NUM) strcpy (prefix, "[Main window");
				else if (thread_num == COMM_THREAD_NUM) strcpy (prefix, "[Comm window");
				else if (NUM_WORKERS == 1 && WORKERS_ACTIVE == 1) strcpy (prefix, "[Worker");
				else sprintf (prefix, "[Worker #%d", thread_num+1);
				strcat (prefix, "] ");
				_write (fd, prefix, (unsigned int) strlen (prefix));
			}
			_write (fd, buf, (unsigned int) strlen (buf));
			_close (fd);
		}
		last_char_out_was_newline = (buf[strlen(buf)-1] == '\n');
	}
}

void OutputSomewhere (
	int	thread_num,
	const char *buf)
{
	if (NO_GUI) writeResults (buf);
	else OutputStr (thread_num, buf);
}

EXTERNC
void OutputBoth (
	int	thread_num,
	const char *buf)
{
	OutputStr (thread_num, buf);
	writeResults (buf);
}

void OutputBothBench (
	int	thread_num,
	const char *buf)
{
	OutputStr (thread_num, buf);
	writeResultsBench (buf);
}

#include "errno.h"

void OutputBothErrno (
	int	thread_num)
{
	char	buf[1024];
#ifdef WIN32
	int	errnum;
	unsigned long dos_errnum;
	char	errtxt[512];

	_get_errno (&errnum);
	if (errnum) {
		strerror_s (errtxt, sizeof (errtxt), errnum);
		sprintf (buf, "Errno: %d, %s\n", errnum, errtxt);
		OutputBoth (thread_num, buf);
		_set_errno (0);
	}

	_get_doserrno (&dos_errnum);
	if (dos_errnum) {
		sprintf (buf, "DOSerrno: %ld\n", dos_errnum);
		OutputBoth (thread_num, buf);
		_set_doserrno (0);
	}
#else
	if (errno) {
		sprintf (buf, "Errno: %d, %s\n", errno, strerror (errno));
		OutputBoth (thread_num, buf);
	}
#endif
}

void OutputStr (
	int	thread_num,
	const char *buf)
{

	gwmutex_lock (&OUTPUT_MUTEX);

	if (TIMESTAMPING) {
		char	tmpbuf[200], fmtbuf[202];

		timestamp (tmpbuf, 0);
		sprintf (fmtbuf, "[%s] ", tmpbuf);

		do {
			const char *eol;

			eol = strchr (buf, '\n');
			if (eol != NULL) eol++;
			else eol = buf + strlen (buf);
			OutputStrInternal (thread_num, fmtbuf);
			while (buf != eol) {
				int	len;
				len = (int) (eol - buf);
				if (len >= sizeof (tmpbuf)) len = sizeof (tmpbuf) - 1;
				memcpy (tmpbuf, buf, len);
				tmpbuf[len] = 0;
				OutputStrInternal (thread_num, tmpbuf);
				buf += len;
			}
		} while (*buf);
	}

	else
		OutputStrInternal (thread_num, buf);

	gwmutex_unlock (&OUTPUT_MUTEX);
}

void OutputStrNoTimeStamp (
	int	thread_num,
	const char *buf)
{
	gwmutex_lock (&OUTPUT_MUTEX);
	OutputStrInternal (thread_num, buf);
	gwmutex_unlock (&OUTPUT_MUTEX);
}

int OutOfMemory (
	int thread_num)
{
	OutputStr (thread_num, "Out of memory!\n");
	return (STOP_OUT_OF_MEM);
}

void JSONaddExponent (
	char	*JSONbuf,
	struct work_unit *w)
{
	if (w->k == 1.0 && w->b == 2 && w->c == -1) sprintf (JSONbuf+strlen(JSONbuf), ", \"exponent\":%lu", w->n);
	else sprintf (JSONbuf+strlen(JSONbuf), ", \"k\":%.0f, \"b\":%lu, \"n\":%lu, \"c\":%ld", w->k, w->b, w->n, w->c);
}

void JSONaddExponentKnownFactors (
	char	*JSONbuf,
	struct work_unit *w)
{
	if (w->known_factors != NULL) {
		char	fac_string[1210];
		char	*in, *out;
		fac_string[0] = 0;
		
		for (in = w->known_factors, out = fac_string; ; in++) {
			if (*in == ',') {
				strcpy (out, "\",\"");
				out += 3;
			} else if (out - fac_string > 1200) {
				strcpy (out, "...");
				break;
			} else
				*out++ = *in;
			if (*in == 0) break;
		}
		sprintf (JSONbuf+strlen(JSONbuf), ", \"known-factors\":[\"%s\"]", fac_string);
	}
}

void JSONaddProgramTimestamp (
	char	*JSONbuf)
{
	time_t rawtime;

	sprintf (JSONbuf+strlen(JSONbuf), ", \"program\":{\"name\":\"SysCorePrime\", \"version\":\"%s\", \"build\":%s, \"port\":%d}", VERSION, BUILD_NUM, PORT);
	time (&rawtime);
	strftime (JSONbuf+strlen(JSONbuf), 80, ", \"timestamp\":\"%Y-%m-%d %H:%M:%S\"", gmtime (&rawtime));
}

void JSONaddUserComputerAID (
	char	*JSONbuf,
	struct work_unit *w)
{
	if (USERID[0]) sprintf (JSONbuf+strlen(JSONbuf), ", \"user\":\"%s\"", USERID);
	if (COMPID[0]) sprintf (JSONbuf+strlen(JSONbuf), ", \"computer\":\"%s\"", COMPID);
	if (w != NULL && w->assignment_uid[0]) sprintf (JSONbuf+strlen(JSONbuf), ", \"aid\":\"%s\"", w->assignment_uid);
}

unsigned int countCommas (
	const char *p)
{
	unsigned int cnt;
	for (cnt = 0; ; cnt++) {
		p = strchr (p, ',');
		if (p == NULL) break;
		p++;
	}
	return (cnt);
}

void auxiliaryWorkUnitInit (
	struct work_unit *w)
{

	if ((w->work_type == WORK_FACTOR || w->work_type == WORK_TEST || w->work_type == WORK_DBLCHK || w->work_type == WORK_PRP) && w->factor_to == 0.0)
		w->factor_to = factorLimit (w);

	if (w->work_type == WORK_TEST) w->tests_saved = 2.0;
	if (w->work_type == WORK_DBLCHK) w->tests_saved = 1.0;

	if (w->tests_saved > 100) w->tests_saved = 100;
}

void pct_complete_from_savefile (
	struct work_unit *w)
{
	int	fd, res;
	uint32_t version;
	char	filename[32];

	tempFileName (w, filename);

	for ( ; ; ) {
		fd = _open (filename, _O_BINARY | _O_RDONLY);
		if (fd <= 0) {
			if ((w->work_type == WORK_TEST ||
			     w->work_type == WORK_DBLCHK ||
			     w->work_type == WORK_PRP) && filename[0] == 'p') {
				filename[0] = 'm';
				continue;
			}
			if ((w->work_type == WORK_TEST ||
			     w->work_type == WORK_DBLCHK) && filename[0] == 'm') {
				filename[0] = 'f';
				continue;
			}
			break;
		}

		res = read_header (fd, &version, w, NULL);
		_close (fd);
		if (!res) break;

		return;
	}

	w->stage[0] = 0;
	w->pct_complete = 0.0;
}

void addKnownFermatFactor (
	struct work_unit *w,	
	const char *factor)	
{
	if (w->known_factors == NULL) {
		w->known_factors = (char *) malloc (strlen (factor) + 1);
		if (w->known_factors != NULL) {
			strcpy (w->known_factors, factor);
			WORKTODO_CHANGED = TRUE;
		}
	}
	else if (strstr (w->known_factors, factor) == NULL) {
		char	*new_factor_list;
		new_factor_list = (char *) malloc (strlen (w->known_factors) + strlen (factor) + 2);
		if (new_factor_list != NULL) {
			sprintf (new_factor_list, "%s,%s", w->known_factors, factor);
			free (w->known_factors);
			w->known_factors = new_factor_list;
			WORKTODO_CHANGED = TRUE;
		}
	}
}

void addKnownFermatFactors (
	struct work_unit *w)	
{

	if (w->k == 1.0 && w->b == 2 && w->c == 1 &&
	    (w->work_type == WORK_ECM || w->work_type == WORK_PMINUS1 || w->work_type == WORK_PPLUS1) &&
	    IniGetInt (INI_FILE, "AddKnownFermatFactors", 1)) {
		if (w->n == 4096) addKnownFermatFactor (w, "114689");
		if (w->n == 4096) addKnownFermatFactor (w, "26017793");
		if (w->n == 4096) addKnownFermatFactor (w, "63766529");
		if (w->n == 4096) addKnownFermatFactor (w, "190274191361");
		if (w->n == 4096) addKnownFermatFactor (w, "1256132134125569");
		if (w->n == 4096) addKnownFermatFactor (w, "568630647535356955169033410940867804839360742060818433");
		if (w->n == 8192) addKnownFermatFactor (w, "2710954639361");
		if (w->n == 8192) addKnownFermatFactor (w, "2663848877152141313");
		if (w->n == 8192) addKnownFermatFactor (w, "3603109844542291969");
		if (w->n == 8192) addKnownFermatFactor (w, "319546020820551643220672513");
		if (w->n == 16384) addKnownFermatFactor (w, "116928085873074369829035993834596371340386703423373313");
		if (w->n == 32768) addKnownFermatFactor (w, "1214251009");
		if (w->n == 32768) addKnownFermatFactor (w, "2327042503868417");
		if (w->n == 32768) addKnownFermatFactor (w, "168768817029516972383024127016961");
		if (w->n == 65536) addKnownFermatFactor (w, "825753601");
		if (w->n == 65536) addKnownFermatFactor (w, "188981757975021318420037633");
		if (w->n == 131072) addKnownFermatFactor (w, "31065037602817");
		if (w->n == 131072) addKnownFermatFactor (w, "7751061099802522589358967058392886922693580423169");
		if (w->n == 262144) addKnownFermatFactor (w, "13631489");
		if (w->n == 262144) addKnownFermatFactor (w, "81274690703860512587777");
		if (w->n == 524288) addKnownFermatFactor (w, "70525124609");
		if (w->n == 524288) addKnownFermatFactor (w, "646730219521");
		if (w->n == 524288) addKnownFermatFactor (w, "37590055514133754286524446080499713");
		if (w->n == 2097152) addKnownFermatFactor (w, "4485296422913");
		if (w->n == 4194304) addKnownFermatFactor (w, "64658705994591851009055774868504577");
		if (w->n == 8388608) addKnownFermatFactor (w, "167772161");
		if (w->n == 33554432) addKnownFermatFactor (w, "25991531462657");
		if (w->n == 33554432) addKnownFermatFactor (w, "204393464266227713");
		if (w->n == 33554432) addKnownFermatFactor (w, "2170072644496392193");
		if (w->n == 67108864) addKnownFermatFactor (w, "76861124116481");
		if (w->n == 134217728) addKnownFermatFactor (w, "151413703311361");
		if (w->n == 134217728) addKnownFermatFactor (w, "231292694251438081");
		if (w->n == 268435456) addKnownFermatFactor (w, "1766730974551267606529");
		if (w->n == 536870912) addKnownFermatFactor (w, "2405286912458753");
	}
}	    

int addToWorkUnitArray (
	unsigned int tnum,	
	struct work_unit *w,	
	int	add_point)	
{
	struct work_unit *insertion_point;

	tnum = tnum % NUM_WORKERS;

	switch (add_point) {
	case ADD_TO_END:
		insertion_point = WORK_UNITS[tnum].last;
		break;

	case ADD_TO_FRONT:
		if (WORK_UNITS[tnum].first != NULL && WORK_UNITS[tnum].first->work_type == WORK_NONE)
			for (insertion_point = WORK_UNITS[tnum].first; insertion_point->next != NULL; insertion_point = insertion_point->next) {
				if (insertion_point->next->work_type != WORK_NONE) break;
			}
		else
			insertion_point = NULL;
		break;

	case ADD_TO_LOGICAL_END:
		for (insertion_point = WORK_UNITS[tnum].last; insertion_point != NULL; insertion_point = insertion_point->prev) {
			if (insertion_point->work_type != WORK_NONE || insertion_point->comment[0]) break;
		}
		break;

	case ADD_BEFORE_SPECIFIC:
		insertion_point = w->next->prev;
		break;
	case ADD_AFTER_SPECIFIC:
		insertion_point = w->next;
		break;
	}

	w->prev = insertion_point;
	if (insertion_point == NULL) {
		w->next = WORK_UNITS[tnum].first;
		WORK_UNITS[tnum].first = w;
	} else {
		w->next = insertion_point->next;
		insertion_point->next = w;
	}
	if (w->next == NULL)
		WORK_UNITS[tnum].last = w;
	else
		w->next->prev = w;

	if (w->work_type != WORK_NONE) {
		WORKTODO_COUNT++;
		restart_one_waiting_worker (tnum, RESTART_WORK_AVAILABLE);
	}

	WORKTODO_CHANGED = TRUE;

	return (0);
}

int parseWorkToDoLine (
	char	*line,
	struct work_unit *w)
{
	char	keyword[20], buf[200];
	char	*value;
	int	i;

	memset (w, 0, sizeof (struct work_unit));
	if (((line[0] < 'A' || line[0] > 'Z') && (line[0] < 'a' || line[0] > 'z'))) goto comment;

	for (i = 0; ; i++) {
		if (line[i] == '=') break;
		if (line[i] == 0 || i == sizeof (keyword) - 1) goto illegal_line;
		keyword[i] = toupper (line[i]);
	}
	keyword[i] = 0;
	value = line + i + 1;

	w->k = 1.0;
	w->b = 2;
	w->c = -1;

	if (toupper (value[0]) == 'N' && value[1] == '/' && toupper (value[2]) == 'A' && value[3] == ',') {
		w->ra_failed = TRUE;
		safe_strcpy (value, value+4);
	}
	for (i = 0; ; i++) {
		if (!(value[i] >= '0' && value[i] <= '9') && !(toupper (value[i]) >= 'A' && toupper (value[i]) <= 'F')) break;
		if (i == 31) {
			if (value[32] != ',') break;
			value[32] = 0;
			strcpy (w->assignment_uid, value);
			safe_strcpy (value, value+33);
			break;
		}
	}

	if (toupper (value[0]) == 'F' && toupper (value[1]) == 'F' && toupper (value[2]) == 'T') {
		int	sse2;
		unsigned long fftlen;
		char	*p;

		if (value[3] == '2') {
			sse2 = TRUE;
			p = value+5;
		} else {
			sse2 = FALSE;
			p = value+4;
		}
		fftlen = atoi (p);
		while (isdigit (*p)) p++;
		if (toupper (*p) == 'K') fftlen <<= 10, p++;
		if (toupper (*p) == 'M') fftlen <<= 20, p++;
		if (*p == ',') p++;
		safe_strcpy (value, p);
		if ((sse2 && (CPU_FLAGS & CPU_SSE2)) || (!sse2 && ! (CPU_FLAGS & CPU_SSE2)))
			w->minimum_fftlen = fftlen;
	}

	if ((value[0] == 'E' || value[0] == 'e') && (value[1] == 'X' || value[1] == 'x') && (value[2] == 'T' || value[2] == 't') && value[3] == '=') {
		char	*comma, *p;

		p = value+4;
		comma = strchr (p, ',');
		if (comma != NULL) {
			*comma = 0;
			if (strlen (p) > 8) p[8] = 0;
			strcpy (w->extension, p);
			safe_strcpy (value, comma+1);
		}
	}

	if (strcmp (keyword, "TEST") == 0) {
		float	sieve_depth;
		w->work_type = WORK_TEST;
		sieve_depth = 99.0;
		w->pminus1ed = 1;
		sscanf (value, "%lu,%f,%d", &w->n, &sieve_depth, &w->pminus1ed);
		w->sieve_depth = sieve_depth;
		w->tests_saved = 1.3;
	}
	else if (strcmp (keyword, "DOUBLECHECK") == 0) {
		float	sieve_depth;
		w->work_type = WORK_DBLCHK;
		sieve_depth = 99.0;
		w->pminus1ed = 1;
		sscanf (value, "%lu,%f,%d", &w->n, &sieve_depth, &w->pminus1ed);
		w->sieve_depth = sieve_depth;
		w->tests_saved = 1.0;
	}

	else if (strcmp (keyword, "ADVANCEDTEST") == 0) {
		w->work_type = WORK_ADVANCEDTEST;
		sscanf (value, "%lu", &w->n);
	}

	else if (strcmp (keyword, "FACTOR") == 0) {
		float	sieve_depth, factor_to;
		w->work_type = WORK_FACTOR;
		sieve_depth = 0.0;
		factor_to = 0.0;
		sscanf (value, "%lu,%f,%f", &w->n, &sieve_depth, &factor_to);
		w->sieve_depth = sieve_depth;
		w->factor_to = factor_to;
	}

	else if (strcmp (keyword, "PFACTOR") == 0) {
		float	sieve_depth;
		w->work_type = WORK_PFACTOR;
		sieve_depth = 0.0;
		if (countCommas (value) > 3) {		
			char	*q;
			float	tests_saved;
			tests_saved = 0.0;
			w->k = atof (value);
			if ((q = strchr (value, ',')) == NULL) goto illegal_line;
			sscanf (q+1, "%lu,%lu,%ld,%f,%f", &w->b, &w->n, &w->c, &sieve_depth, &tests_saved);
			for (i = 1; i <= 4; i++) if ((q = strchr (q+1, ',')) == NULL) goto illegal_line;
			q = strchr (q+1, ',');
			w->sieve_depth = sieve_depth;
			w->tests_saved = tests_saved;
			if (q != NULL && q[1] == '"') {
				w->known_factors = (char *) malloc (strlen (q));
				if (w->known_factors == NULL) goto nomem;
				strcpy (w->known_factors, q+2);
			}
		} else {				
			int	dblchk;
			sscanf (value, "%lu,%f,%d", &w->n, &sieve_depth, &dblchk);
			w->sieve_depth = sieve_depth;
			w->tests_saved = dblchk ? 1.0 : 1.3;
		}
	}

	else if (keyword[0] == 'E' && keyword[1] == 'C' && keyword[2] == 'M') {
		int	ecm_type;
		char	*q;
		w->work_type = WORK_ECM;
		if (strcmp (keyword, "ECM") == 0 || strcmp (keyword, "ECM2") == 0) ecm_type = 0;
		else if (strcmp (keyword, "ECMSTAGE2") == 0) ecm_type = 1;
		else if (strcmp (keyword, "ECMSTAGE2N") == 0) ecm_type = 2;
		else goto illegal_line;
		if (ecm_type != 2) {					
			w->k = atof (value);
			if ((q = strchr (value, ',')) == NULL) goto illegal_line;
			sscanf (q+1, "%lu,%lu,%ld", &w->b, &w->n, &w->c);
			for (i = 1; i <= 3; i++) if ((q = strchr (q+1, ',')) == NULL) goto illegal_line;
		}
		if (ecm_type != 0) {					
			int len;
			char *end;
			if (ecm_type == 2) q = value; else q++;
			if (q[0] == '"') {
				q++;
				end = strchr (q, '"');
				if (end == NULL) goto illegal_line;
			} else {
				end = strchr (q, ',');
				if (end == NULL) end = q + strlen (q);
			}
			len = (int) (end - q);
			w->gmp_ecm_file = (char *) malloc (len + 1);
			if (w->gmp_ecm_file == NULL) goto nomem;
			memcpy (w->gmp_ecm_file, q, len);
			w->gmp_ecm_file[len] = 0;
			q = strchr (q + len, ',');
		}
		if (ecm_type == 0) {					
			w->B1 = (uint64_t) atof (q+1);
			q = strchr (q+1, ',');
		}
		if (q != NULL && q[1] != '"') {				
			w->B2 = (uint64_t) atof (q+1);
			q = strchr (q+1, ',');
		}
		if (q != NULL && q[1] != '"' && ecm_type != 0) {	
			w->skip_curves = (uint64_t) atoi (q+1);
			q = strchr (q+1, ',');
		}
		if (ecm_type == 0) w->curves_to_do = 100;		
		if (q != NULL && q[1] != '"') {				
			w->curves_to_do = atoi (q+1);
			q = strchr (q+1, ',');
		}
		if (q != NULL && q[1] != '"' && ecm_type == 0) {	
			w->curve = atoll (q+1);
			q = strchr (q+1, ',');
		}
		if (q != NULL && q[1] == '"') {				
			w->known_factors = (char *) malloc (strlen (q));
			if (w->known_factors == NULL) goto nomem;
			strcpy (w->known_factors, q+2);
		}
	}

	else if (strcmp (keyword, "PMINUS1") == 0) {
		char	*q;
		w->work_type = WORK_PMINUS1;
		w->k = atof (value);
		if ((q = strchr (value, ',')) == NULL) goto illegal_line;
		sscanf (q+1, "%lu,%lu,%ld", &w->b, &w->n, &w->c);
		for (i = 1; i <= 3; i++) if ((q = strchr (q+1, ',')) == NULL) goto illegal_line;
		w->B1 = (uint64_t) atof (q+1);
		if ((q = strchr (q+1, ',')) == NULL) goto illegal_line;
		w->B2 = (uint64_t) atof (q+1);
		q = strchr (q+1, ',');
		w->sieve_depth = 0.0;
		if (q != NULL && q[1] != '"') {
			double j = atof (q+1);
			if (j <= 300.0) w->sieve_depth = j;
			q = strchr (q+1, ',');
		}
		w->B2_start = 0;
		if (q != NULL && q[1] != '"') {
			uint64_t j;
			j = (uint64_t) atof (q+1);
			if (j > w->B1) w->B2_start = j;
			q = strchr (q+1, ',');
		}
		if (q != NULL && q[1] == '"') {
			w->known_factors = (char *) malloc (strlen (q));
			if (w->known_factors == NULL) goto nomem;
			strcpy (w->known_factors, q+2);
		}
	}

	else if (strcmp (keyword, "PPLUS1") == 0) {
		char	*q;
		w->work_type = WORK_PPLUS1;
		w->k = atof (value);
		if ((q = strchr (value, ',')) == NULL) goto illegal_line;
		sscanf (q+1, "%lu,%lu,%ld", &w->b, &w->n, &w->c);
		for (i = 1; i <= 3; i++) if ((q = strchr (q+1, ',')) == NULL) goto illegal_line;
		w->B1 = (uint64_t) atof (q+1);
		if ((q = strchr (q+1, ',')) == NULL) goto illegal_line;
		w->B2 = (uint64_t) atof (q+1);
		q = strchr (q+1, ',');
		w->nth_run = 1;
		if (q != NULL && q[1] != '"') {
			int nth_run = atoi (q+1);
			if (nth_run >= 1 && nth_run <= 3) w->nth_run = nth_run;
			q = strchr (q+1, ',');
		}
		w->sieve_depth = 0.0;
		if (q != NULL && q[1] != '"') {
			double j = atof (q+1);
			if (j <= 300.0) w->sieve_depth = j;
			q = strchr (q+1, ',');
		}
		if (q != NULL && q[1] == '"') {
			w->known_factors = (char *) malloc (strlen (q));
			if (w->known_factors == NULL) goto nomem;
			strcpy (w->known_factors, q+2);
		}
	}

	else if (strcmp (keyword, "PRP") == 0 || strcmp (keyword, "PRPDC") == 0) {
		char	*q;

		w->work_type = WORK_PRP;
		w->prp_dblchk = (keyword[3] != 0);
		w->k = atof (value);
		if ((q = strchr (value, ',')) == NULL) goto illegal_line;
		sscanf (q+1, "%lu,%lu,%ld", &w->b, &w->n, &w->c);
		for (i = 1; i <= 2; i++) if ((q = strchr (q+1, ',')) == NULL) goto illegal_line;
		q = strchr (q+1, ',');

		w->sieve_depth = 99.0;		
		w->tests_saved = 0.0;		
		w->prp_base = 0;
		w->prp_residue_type = 0;
		if (q != NULL && q[1] != '"') {
			w->sieve_depth = atof (q+1);
			if ((q = strchr (q+1, ',')) == NULL) goto illegal_line;
			w->tests_saved = atof (q+1);
			q = strchr (q+1, ',');
			if (q != NULL && q[1] != '"') {
				w->prp_base = atoi (q+1);
				if ((q = strchr (q+1, ',')) == NULL) goto illegal_line;
				w->prp_residue_type = atoi (q+1);
				q = strchr (q+1, ',');
			}
		}
		if (q != NULL && q[1] == '"') {
			w->known_factors = (char *) malloc (strlen (q));
			if (w->known_factors == NULL) goto nomem;
			strcpy (w->known_factors, q+2);
		}
	}

	else if (strcmp (keyword, "CERT") == 0) {
		char	*q;

		w->work_type = WORK_CERT;
		w->k = atof (value);
		if ((q = strchr (value, ',')) == NULL) goto illegal_line;
		sscanf (q+1, "%lu,%lu,%ld,%d", &w->b, &w->n, &w->c, &w->cert_squarings);
		for (i = 1; i <= 3; i++) if ((q = strchr (q+1, ',')) == NULL) goto illegal_line;
	}

	else if (strcmp (keyword, "ADVANCEDFACTOR") == 0) {
		OutputSomewhere (MAIN_THREAD_NUM, "Worktodo error: AdvancedFactor no longer supported\n");
		goto comment;
	} else {
		goto illegal_line;
	}

	if (w->known_factors != NULL) {
		for (i = (int) strlen (w->known_factors); i > 0 && !isdigit (w->known_factors[i-1]); i--);
		w->known_factors[i] = 0;
		for (i = 0; i < (int) strlen (w->known_factors); i++)
			if (!isdigit (w->known_factors[i])) w->known_factors[i] = ',';
	}

	addKnownFermatFactors (w);

	if (w->k == 1.0 && w->b == 2 && !isPrime (w->n) && w->c == -1 && w->known_factors == NULL &&
	    w->work_type != WORK_ECM && w->work_type != WORK_PMINUS1 && w->work_type != WORK_PPLUS1 &&
	    !(w->work_type == WORK_PRP && IniGetInt (INI_FILE, "PhiExtensions", 0))) {
		sprintf (buf, "Error: Worktodo.txt file contained composite exponent: %ld\n", w->n);
		OutputBoth (MAIN_THREAD_NUM, buf);
		goto illegal_line;
	}
	if ((w->work_type == WORK_TEST || w->work_type == WORK_DBLCHK || w->work_type == WORK_ADVANCEDTEST) &&
	    (w->n < MIN_PRIME ||
	     (w->minimum_fftlen == 0 &&
	      w->n > (unsigned long) ((CPU_FLAGS & CPU_AVX512F) ? MAX_PRIME_AVX512 :
				      (CPU_FLAGS & CPU_FMA3) ? MAX_PRIME_FMA3 :
				      (CPU_FLAGS & CPU_AVX) ? MAX_PRIME_AVX :
				      (CPU_FLAGS & CPU_SSE2) ? MAX_PRIME_SSE2 : MAX_PRIME)))) {
		sprintf (buf, "Error: Worktodo.txt file contained bad LL exponent: %ld\n", w->n);
		OutputBoth (MAIN_THREAD_NUM, buf);
		goto illegal_line;
	}
	if (w->work_type == WORK_FACTOR && w->n < 200000) {
		sprintf (buf, "Error: Use ECM instead of trial factoring for exponent: %ld\n", w->n);
		OutputBoth (MAIN_THREAD_NUM, buf);
		goto illegal_line;
	}
	if (w->work_type == WORK_FACTOR && w->n > MAX_FACTOR && !IniGetInt (INI_FILE, "LargeTFexponents", 0)) {
		sprintf (buf, "Error: Worktodo.txt file contained bad factoring exponent: %ld\n", w->n);
		OutputBoth (MAIN_THREAD_NUM, buf);
		goto illegal_line;
	}
	if (w->work_type == WORK_FACTOR && w->factor_to > MAX_FACTOR_BITS && !IniGetInt (INI_FILE, "LargeTFbits", 0)) {
		sprintf (buf, "Error: Worktodo.txt file contained bad factoring limit: %d\n", (int) w->factor_to);
		OutputBoth (MAIN_THREAD_NUM, buf);
		goto illegal_line;
	}
	if ((w->work_type == WORK_PMINUS1 || w->work_type == WORK_PPLUS1 || w->work_type == WORK_ECM) && w->B1 < 50000 &&
	    w->gmp_ecm_file == NULL && !IniGetInt (INI_FILE, "AllowLowB1", 0)) {
		sprintf (buf, "Error: Worktodo.txt file has P-1/P+1/ECM with B1 < 50000 (exponent: %ld)\n", w->n);
		OutputBoth (MAIN_THREAD_NUM, buf);
		goto illegal_line;
	}

	auxiliaryWorkUnitInit (w);

	return (0);

illegal_line:
	sprintf (buf, "Illegal line in worktodo.txt file: %.80s\n", line);
	OutputSomewhere (MAIN_THREAD_NUM, buf);

comment:w->work_type = WORK_NONE;
	w->comment = (char *) malloc (strlen (line) + 1);
	if (w->comment == NULL) goto nomem;
	strcpy (w->comment, line);
	return (0);

nomem:	return (OutOfMemory (MAIN_THREAD_NUM));
}

int readWorkToDoFile (void)
{
	FILE	*fd;
	unsigned int tnum, i, linenum;
	int	rc;
	char	line[16384];

	for (i = 1; ; i++) {
		gwmutex_lock (&WORKTODO_MUTEX);

		if (WORKTODO_IN_USE_COUNT == 0 && !WORKTODO_CHANGED) break;
		gwmutex_unlock (&WORKTODO_MUTEX);
		if (i <= 10) {
			Sleep (50);
			continue;
		}

		return (0);
	}

	WORKTODO_CHANGED = FALSE;
	WORKTODO_COUNT = 0;

	for (tnum = 0; tnum < MAX_NUM_WORKERS; tnum++) {
		struct work_unit *w, *next_w;
		for (w = WORK_UNITS[tnum].first; w != NULL; w = next_w) {
			next_w = w->next;
			free (w->gmp_ecm_file);
			free (w->known_factors);
			free (w->comment);
			free (w);
		}
		WORK_UNITS[tnum].first = NULL;
		WORK_UNITS[tnum].last = NULL;
	}

	fd = fopen (WORKTODO_FILE, "r");
	if (fd == NULL) goto done;

	tnum = 0;
	linenum = 0;
	while (fgets (line, sizeof (line), fd)) {
	    struct work_unit *w;

	    if (line[strlen(line)-1] == '\n') line[strlen(line)-1] = 0;
	    if (line[0] && line[strlen(line)-1] == '\r') line[strlen(line)-1] = 0;
	    linenum++;

	    w = (struct work_unit *) malloc (sizeof (struct work_unit));
	    if (w == NULL) goto nomem;

	    if (line[0] == '[' && linenum > 1) {
		tnum++;
		if (tnum >= NUM_WORKERS) {
		    char	buf[100];
		    sprintf (buf,
			     "Too many sections in worktodo.txt.  Moving work from section #%u to #%u.\n",
			     tnum + 1, tnum % NUM_WORKERS + 1);
		    OutputSomewhere (MAIN_THREAD_NUM, buf);
		    safe_strcpy (line + 9, line);
		    memcpy (line, ";;MOVED;;", 9);
		    WORKTODO_CHANGED = TRUE;
		}
	    }

	    rc = parseWorkToDoLine (line, w);
	    if (rc) goto retrc;

	    rc = addToWorkUnitArray (tnum, w, ADD_TO_END);
	    if (rc) goto retrc;
	}

	for (tnum = 0; tnum < MAX_NUM_WORKERS; tnum++) {
	    struct work_unit *w;
	    int first_real_work_line;

	    first_real_work_line = TRUE;
	    for (w = WORK_UNITS[tnum].first; w != NULL; w = w->next) {

		w->stage[0] = 0;
		w->pct_complete = 0.0;

		if (w->work_type == WORK_NONE) continue;

		if (WELL_BEHAVED_WORK && !first_real_work_line) goto next_wu;

		if (!first_real_work_line) {
			int	tnum2;
			struct work_unit *w2;

			for (tnum2 = 0; tnum2 < MAX_NUM_WORKERS; tnum2++) {
				for (w2 = WORK_UNITS[tnum2].first; w2 != NULL; w2 = w2->next) {
					if (w2->work_type == WORK_NONE) continue;
					if (w2->work_type == w->work_type &&
					    w2->k == w->k &&
					    w2->b == w->b &&
					    w2->n == w->n &&
					    w2->c == w->c) goto next_wu;
					break;
				}
			}

			for (w2 = WORK_UNITS[tnum].first; w2 != w; w2 = w2->next) {
				if (w2->work_type == w->work_type &&
				    w2->k == w->k &&
				    w2->b == w->b &&
				    w2->n == w->n &&
				    w2->c == w->c) goto next_wu;
			}
		}

		pct_complete_from_savefile (w);

next_wu:	first_real_work_line = FALSE;
	    }
	}

	fclose (fd);
done:	gwmutex_unlock (&WORKTODO_MUTEX);

	writeWorkToDoFile (FALSE);

	return (incorporateWorkToDoAddFile ());

retrc:	fclose (fd);
	gwmutex_unlock (&WORKTODO_MUTEX);
	return (rc);

nomem:	fclose (fd);
	gwmutex_unlock (&WORKTODO_MUTEX);
	return (OutOfMemory (MAIN_THREAD_NUM));
}

int writeWorkToDoFile (
	int	force)		
{
	int	fd, last_line_was_blank;
	unsigned int tnum;

	if (!WORKTODO_CHANGED) return (0);

	if (WELL_BEHAVED_WORK && !force) {
		static time_t last_time_written = 0;
		time_t	current_time;
		time (&current_time);
		if (current_time < last_time_written + 1800) return (0);
		last_time_written = current_time;
	}

	gwmutex_lock (&WORKTODO_MUTEX);

	fd = _open (WORKTODO_FILE, _O_CREAT | _O_TRUNC | _O_WRONLY | _O_TEXT, CREATE_FILE_ACCESS);
	if (fd < 0) {
		OutputBoth (MAIN_THREAD_NUM, "Error creating worktodo.txt file\n");
		gwmutex_unlock (&WORKTODO_MUTEX);
		return (STOP_FILE_IO_ERROR);
	}

	last_line_was_blank = FALSE;
	for (tnum = 0; tnum < MAX_NUM_WORKERS; tnum++) {
	    struct work_unit *w;
	    unsigned int len;

	    if (tnum >= NUM_WORKERS && WORK_UNITS[tnum].first == NULL) break;

	    if (tnum || NUM_WORKERS > 0) {
		char	buf[40];
		if (tnum && !last_line_was_blank && _write (fd, "\n", 1) != 1)
			goto write_error;
		sprintf (buf, "[Worker #%d]\n", tnum+1);
		len = (unsigned int) strlen (buf);
		if (_write (fd, buf, len) != len) goto write_error;
		last_line_was_blank = FALSE;
	    }

	    for (w = WORK_UNITS[tnum].first; w != NULL; w = w->next) {
		char	idbuf[100];
		char	buf[4096];

		if (w->work_type == WORK_DELETED) continue;

		if (w == WORK_UNITS[tnum].first &&
		    w->work_type == WORK_NONE &&
		    w->comment[0] == '[') continue;

		idbuf[0] = 0;
		if (w->assignment_uid[0])
			sprintf (idbuf, "%s,", w->assignment_uid);
		else if (w->ra_failed)
			sprintf (idbuf, "%s,", "N/A");

		if (w->minimum_fftlen) {
			strcat (idbuf, "FFT");
			if (CPU_FLAGS & CPU_SSE2) strcat (idbuf, "2");
			if ((w->minimum_fftlen & 0xFFFFF) == 0)
				sprintf (idbuf+strlen(idbuf), "=%luM,", w->minimum_fftlen >> 20);
			else if ((w->minimum_fftlen & 0x3FF) == 0)
				sprintf (idbuf+strlen(idbuf), "=%luK,", w->minimum_fftlen >> 10);
			else
				sprintf (idbuf+strlen(idbuf), "=%lu,", w->minimum_fftlen);
		}

		if (w->extension[0]) {
			sprintf (idbuf+strlen(idbuf), "EXT=%s,", w->extension);
		}

		switch (w->work_type) {

		case WORK_NONE:
			strcpy (buf, w->comment);
			break;

		case WORK_TEST:
			if (w->sieve_depth != 99.0 || w->pminus1ed != 1)
				sprintf (buf, "Test=%s%lu,%.0f,%d", idbuf, w->n, w->sieve_depth, w->pminus1ed);
			else
				sprintf (buf, "Test=%s%lu", idbuf, w->n);
			break;

		case WORK_DBLCHK:
			if (w->sieve_depth != 99.0 || w->pminus1ed != 1)
				sprintf (buf, "DoubleCheck=%s%lu,%.0f,%d", idbuf, w->n, w->sieve_depth, w->pminus1ed);
			else
				sprintf (buf, "DoubleCheck=%s%lu", idbuf, w->n);
			break;

		case WORK_ADVANCEDTEST:
			sprintf (buf, "AdvancedTest=%lu", w->n);
			break;

		case WORK_FACTOR:
			sprintf (buf, "Factor=%s%ld,%.0f,%.0f", idbuf, w->n, w->sieve_depth, w->factor_to);
			break;

		case WORK_PFACTOR:
			sprintf (buf, "Pfactor=%s%.0f,%lu,%lu,%ld,%g,%g", idbuf, w->k, w->b, w->n, w->c, w->sieve_depth, w->tests_saved);
			if (w->known_factors != NULL) sprintf (buf + strlen (buf), ",\"%s\"", w->known_factors);
			break;

		case WORK_ECM:
			if (w->gmp_ecm_file != NULL) {
				if (w->n == 0) sprintf (buf, "ECMSTAGE2N=%s\"%s\"", idbuf, w->gmp_ecm_file);
				else sprintf (buf, "ECMSTAGE2=%s%.0f,%lu,%lu,%ld,\"%s\"", idbuf, w->k, w->b, w->n, w->c, w->gmp_ecm_file);
				if (w->B2 || w->skip_curves || w->curves_to_do) sprintf (buf + strlen (buf), ",%" PRIu64, w->B2);
				if (w->skip_curves || w->curves_to_do) sprintf (buf + strlen (buf), ",%u", w->skip_curves);
				if (w->curves_to_do) sprintf (buf + strlen (buf), ",%u", w->curves_to_do);
			} else {
				sprintf (buf, "ECM=%s%.0f,%lu,%lu,%ld,%" PRIu64, idbuf, w->k, w->b, w->n, w->c, w->B1);
				if (w->B2 || w->curves_to_do != 100 || w->curve) sprintf (buf + strlen (buf), ",%" PRIu64, w->B2);
				if (w->curves_to_do != 100 || w->curve) sprintf (buf + strlen (buf), ",%u", w->curves_to_do);
				if (w->curve) sprintf (buf + strlen (buf), ",%" PRIu64, w->curve);
			}
			if (w->known_factors != NULL) sprintf (buf + strlen (buf), ",\"%s\"", w->known_factors);
			break;

		case WORK_PMINUS1:
			sprintf (buf, "Pminus1=%s%.0f,%lu,%lu,%ld,%" PRIu64 ",%" PRIu64, idbuf, w->k, w->b, w->n, w->c, w->B1, w->B2);
			if (w->sieve_depth > 0.0) sprintf (buf + strlen (buf), ",%.0f", w->sieve_depth);
			if (w->B2_start > w->B1) sprintf (buf + strlen (buf), ",%" PRIu64, w->B2_start);
			if (w->known_factors != NULL) sprintf (buf + strlen (buf), ",\"%s\"", w->known_factors);
			break;

		case WORK_PPLUS1:
			sprintf (buf, "Pplus1=%s%.0f,%lu,%lu,%ld,%" PRIu64 ",%" PRIu64 ",%d", idbuf, w->k, w->b, w->n, w->c, w->B1, w->B2, w->nth_run);
			if (w->sieve_depth > 0.0) sprintf (buf + strlen (buf), ",%.0f", w->sieve_depth);
			if (w->known_factors != NULL) sprintf (buf + strlen (buf), ",\"%s\"", w->known_factors);
			break;

		case WORK_PRP:
			sprintf (buf, "PRP%s=%s%.0f,%lu,%lu,%ld", w->prp_dblchk ? "DC" : "", idbuf, w->k, w->b, w->n, w->c);
			if (w->sieve_depth != 99.0 || w->tests_saved > 0.0 || w->prp_base || w->prp_residue_type) {
				sprintf (buf + strlen (buf), ",%g,%g", w->sieve_depth, w->tests_saved);
				if (w->prp_base || w->prp_residue_type)
					sprintf (buf + strlen (buf), ",%u,%d", w->prp_base, w->prp_residue_type);
			}
			if (w->known_factors != NULL) sprintf (buf + strlen (buf), ",\"%s\"", w->known_factors);
			break;

		case WORK_CERT:
			sprintf (buf, "Cert=%s%.0f,%lu,%lu,%ld,%d", idbuf, w->k, w->b, w->n, w->c, w->cert_squarings);
			break;
		}

		strcat (buf, "\n");
		len = (unsigned int) strlen (buf);
		if (_write (fd, buf, len) != len) {
write_error:		OutputBoth (MAIN_THREAD_NUM, "Error writing worktodo.txt file\n");
			_close (fd);
			gwmutex_unlock (&WORKTODO_MUTEX);
			return (STOP_FILE_IO_ERROR);
		}
		last_line_was_blank = (len == 1);
	    }
	}

	_close (fd);
	WORKTODO_CHANGED = FALSE;
	gwmutex_unlock (&WORKTODO_MUTEX);
	return (0);
}

struct work_unit *getNextWorkToDoLine (
	int	thread_num,		
	struct work_unit *w,		
	int	usage)			
{
	struct work_unit *next;		

	ASSERTG (thread_num < (int) NUM_WORKERS);

	gwmutex_lock (&WORKTODO_MUTEX);

	next = (w == NULL) ? WORK_UNITS[thread_num].first : w->next;
	while (next != NULL && next->work_type == WORK_DELETED)
		next = next->next;

	if (w != NULL) {
		w->in_use_count--;
		WORKTODO_IN_USE_COUNT--;
		if (usage == LONG_TERM_USE) w->in_use_count &= ~0x80000000;

		if (w->work_type == WORK_DELETED && w->in_use_count == 0) {

			if (w->prev == NULL)
				WORK_UNITS[thread_num].first = w->next;
			else
				w->prev->next = w->next;
			if (w->next == NULL)
				WORK_UNITS[thread_num].last = w->prev;
			else
				w->next->prev = w->prev;

			free (w->known_factors);
			free (w->comment);
			free (w);
		}
	}

	if (next != NULL) {
		next->in_use_count++;
		WORKTODO_IN_USE_COUNT++;
		if (usage == LONG_TERM_USE) next->in_use_count |= 0x80000000;
	}

	gwmutex_unlock (&WORKTODO_MUTEX);
	return (next);
}

void decrementWorkUnitUseCount (
	struct work_unit *w,		
	int	usage)			
{
	ASSERTG (w->in_use_count != 0);

	gwmutex_lock (&WORKTODO_MUTEX);

	w->in_use_count--;
	WORKTODO_IN_USE_COUNT--;
	if (usage == LONG_TERM_USE) w->in_use_count &= ~0x80000000;

	gwmutex_unlock (&WORKTODO_MUTEX);
}

int addWorkToDoLine (
	int	tnum,		
	struct work_unit *w,	
	int	add_point)	
{
	struct work_unit *malloc_w;
	int	rc;

	auxiliaryWorkUnitInit (w);

	malloc_w = (struct work_unit *) malloc (sizeof (struct work_unit));
	if (malloc_w == NULL) return (OutOfMemory (MAIN_THREAD_NUM));
	memcpy (malloc_w, w, sizeof (struct work_unit));

	addKnownFermatFactors (malloc_w);

	gwmutex_lock (&WORKTODO_MUTEX);

	rc = addToWorkUnitArray (tnum, malloc_w, add_point);
	if (rc) goto retrc;

	gwmutex_unlock (&WORKTODO_MUTEX);
	return (writeWorkToDoFile (FALSE));

retrc:	gwmutex_unlock (&WORKTODO_MUTEX);
	return (rc);
}

int updateWorkToDoLine (
	int	tnum,		
	struct work_unit *w)	
{

	WORKTODO_CHANGED = TRUE;

	return (writeWorkToDoFile (FALSE));
}

int deleteWorkToDoLine (
	int	tnum,		
	struct work_unit *w,	
	int	stop_if_in_progress) 
{

	if (w->work_type == WORK_DELETED) return (0);

	gwmutex_lock (&WORKTODO_MUTEX);

	if ((w->in_use_count & 0x80000000) && stop_if_in_progress)
		stop_worker_for_abort (tnum);

	if (w->work_type != WORK_NONE) WORKTODO_COUNT--;

	w->work_type = WORK_DELETED;

	WORKTODO_CHANGED = TRUE;

	gwmutex_unlock (&WORKTODO_MUTEX);
	return (writeWorkToDoFile (FALSE));
}

int isWorkUnitActive (
	struct work_unit *w)	
{

	if (w->in_use_count & 0x80000000) return (TRUE);
	return (FALSE);
}

double work_estimate (
	int	thread_num,		
	struct work_unit *w)
{
	double	timing, est, pct_complete;
	int	can_use_multiple_threads;
	unsigned int i, total_cores;

	est = 0.0;

	pct_complete = w->pct_complete;
	if (pct_complete < 0.0) pct_complete = 0.0;
	if (pct_complete > 1.0) pct_complete = 1.0;

	can_use_multiple_threads = (CPU_FLAGS & CPU_SSE2 && w->n > 172800) || (CPU_FLAGS & CPU_AVX && w->n > 130000) || (CPU_FLAGS & CPU_AVX512F && w->n > 172100);

	if (w->work_type == WORK_ECM) {
		int	full_curves_to_do, stage, bits;
		double	stage1_time, stage2_time, overhead, B2_minus_B1;

		full_curves_to_do = w->curves_to_do;
		stage = 0;
		if (w->stage[0]) {
			full_curves_to_do -= atoi (&w->stage[1]);
			stage = atoi (&w->stage[strlen(w->stage)-1]);
		}

		timing = gwmap_to_timing (w->k, w->b, w->n, w->c);
		bits = (int) (w->n * log2 (w->b));
		if (bits <= 80000) overhead = 1.10;
		else if (bits >= 1500000) overhead = 1.20;
		else overhead = 1.10 + ((double) bits - 80000.0) / 1420000.0 * (1.20 - 1.10);

		stage1_time = (12.86 * w->B1) * timing * overhead;

		if (bits <= 100000) stage2_time = 0.0617;
		else if (bits >= 5000000) stage2_time = 0.1261;
		else stage2_time = 0.0617 + ((double) bits - 100000.0) / 4900000.0 * (0.1261 - 0.0617);
		B2_minus_B1 = (w->B2 > 0.0) ? w->B2 - w->B1 : 99.0 * w->B1;
		stage2_time = stage2_time * B2_minus_B1 * timing * overhead;

		est = (double) full_curves_to_do * (stage1_time + stage2_time);
		if (stage == 1)
			est += stage1_time * (1.0 - pct_complete) + stage2_time;
		if (stage == 2)
			est += stage2_time * (1.0 - pct_complete);

		est *= IniGetInt (INI_FILE, "ECMBoundsMultiplier", 1);

		if (w->gmp_ecm_file != NULL) est = 86400.0;
	}

	if (w->work_type == WORK_PMINUS1 || w->work_type == WORK_PFACTOR) {
		double	stage1_time, stage2_time;

		timing = gwmap_to_timing (w->k, w->b, w->n, w->c);

		if (w->work_type == WORK_PFACTOR) {
			
			stage1_time = 0.0117 * w->n * timing;
			stage2_time = 0.0058 * w->n * timing;
		} else {
			stage1_time = timing * (1.4545 * (double) w->B1);
			stage2_time = 0.5 * stage1_time;
		}

		int stage = w->stage[0] ? atoi (&w->stage[1]) : 0;
		if (stage == 0) est = stage1_time + stage2_time;
		if (stage == 1) est = stage1_time * (1.0 - pct_complete) + stage2_time;
		if (stage == 2) est = stage2_time * (1.0 - pct_complete);
	}

	if (w->work_type == WORK_PPLUS1) {
		int	stage;
		uint64_t B1, B2;
		double	stage1_time, stage2_time;

		B1 = w->B1;
		B2 = w->B2 ? w->B2 : 40 * w->B1;

		if (w->stage[0]) stage = atoi (&w->stage[1]);
		else stage = 0;

		timing = gwmap_to_timing (w->k, w->b, w->n, w->c);
		stage1_time = timing * (1.4545 * 1.52 * (double) B1);
		stage2_time = (B2 >= B1) ? timing * (0.06154 * (double) (B2 - B1)) * 1.285 : 0.0;

		if (stage == 0) est = stage1_time + stage2_time;
		if (stage == 1) est = stage1_time * (1.0 - pct_complete) + stage2_time;
		if (stage == 2) est = stage2_time * (1.0 - pct_complete);
	}

	if (w->work_type == WORK_FACTOR) {
		int	i, tf_level;

#ifdef X86_64
		can_use_multiple_threads = TRUE;
#else
		can_use_multiple_threads = FALSE;
#endif

		if (w->stage[0]) tf_level = atoi (&w->stage[2]);
		else tf_level = 0;

		est = 0.0;
		for (i = (int) w->sieve_depth+1; i <= (int) w->factor_to; i++) {
			if (i < 48) continue;
			timing = (i > 64) ? 20.2 : (i > 62) ? 16.1 : 15.1;
			if (i <= 72) timing *= 145000.0 * (1L << (i - 48)) / w->n;
			else timing *= 145000.0 * 16777216.0 * (1L << (i - 72)) / w->n;
			if (i == tf_level)
				est += timing * (1.0 - pct_complete);
			else
				est += timing;
		}
		est = est * 2000.0 / CPU_SPEED;

		if (CPU_ARCHITECTURE == CPU_ARCHITECTURE_CORE_2) est /= 2.5;
		if (CPU_ARCHITECTURE == CPU_ARCHITECTURE_CORE_I7 ||
		    CPU_ARCHITECTURE == CPU_ARCHITECTURE_PHI ||
		    CPU_ARCHITECTURE == CPU_ARCHITECTURE_INTEL_OTHER) est /= 3.1;
		if (CPU_ARCHITECTURE == CPU_ARCHITECTURE_AMD_K8 ||
		    CPU_ARCHITECTURE == CPU_ARCHITECTURE_AMD_K10 ||
		    CPU_ARCHITECTURE == CPU_ARCHITECTURE_AMD_BULLDOZER ||
		    CPU_ARCHITECTURE == CPU_ARCHITECTURE_AMD_OTHER) est /= 3.0;

		if (CPU_FLAGS & CPU_AVX512F) est /= 4.0;
		else if (CPU_FLAGS & CPU_AVX2) est /= 2.0;
	}

	if (w->work_type == WORK_TEST ||
	    w->work_type == WORK_ADVANCEDTEST ||
	    w->work_type == WORK_DBLCHK) {
		est = w->n * gwmap_to_timing (w->k, w->b, w->n, w->c);
		if (w->stage[0] == 'L') est *= (1.0 - pct_complete);
	}

	if (w->work_type == WORK_PRP) {
		est = w->n * log ((double) w->b) / log (2.0) * gwmap_to_timing (w->k, w->b, w->n, w->c);
		if (w->stage[0] == 'P') est *= (1.0 - pct_complete);
	}

	if (w->work_type == WORK_CERT) {
		est = w->cert_squarings * gwmap_to_timing (w->k, w->b, w->n, w->c);
		if (w->stage[0] == 'C') est *= (1.0 - pct_complete);
	}

	est *= (24.0 / CPU_HOURS) * (1000.0 / ROLLING_AVERAGE);

	if (can_use_multiple_threads && CORES_PER_TEST[thread_num] > 1) {
		double	effective_num_cpus, cpu_value;
		effective_num_cpus = 0.0;
		cpu_value = 1.0;
		for (i = 0; i < CORES_PER_TEST[thread_num]; i++) {
			effective_num_cpus += cpu_value;
			cpu_value *= (w->work_type == WORK_FACTOR ? 1.0 : 0.9);
		}
		est = est / effective_num_cpus;
	}

	total_cores = 0;
	for (i = 0; i < NUM_WORKERS; i++) total_cores += CORES_PER_TEST[i];
	if (total_cores > HW_NUM_CORES) est *= (double) total_cores / (double) HW_NUM_CORES;

	return (est);
}

unsigned int factorLimit (
	struct work_unit *w)
{
	unsigned long p;
	unsigned int test;

	if (w->factor_to != 0.0) return ((unsigned int) w->factor_to);

	p = w->n;
	if (p > FAC82) test = 82;	
	else if (p > FAC81) test = 81;	
	else if (p > FAC80) test = 80;	
	else if (p > FAC79) test = 79;	
	else if (p > FAC78) test = 78;	
	else if (p > FAC77) test = 77;	
	else if (p > FAC76) test = 76;	
	else if (p > FAC75) test = 75;	
	else if (p > FAC74) test = 74;	
	else if (p > FAC73) test = 73;	
	else if (p > FAC72) test = 72;	
	else if (p > FAC71) test = 71;	
	else if (p > FAC70) test = 70;	
	else if (p > FAC69) test = 69;	
	else if (p > FAC68) test = 68;	
	else if (p > FAC67) test = 67;	
	else if (p > FAC66) test = 66;	
	else if (p > FAC65) test = 65;	
	else if (p > FAC64) test = 64;	
	else if (p > FAC63) test = 63;	
	else if (p > FAC62) test = 62;	
	else if (p > FAC61) test = 61;	
	else if (p > FAC60) test = 60;	
	else if (p > FAC59) test = 59;	
	else if (p > FAC58) test = 58;	
	else if (p > FAC57) test = 57;	
	else if (p > FAC56) test = 56;	
	else test = 40;			

	if (w->work_type == WORK_DBLCHK) test--;

	return (test);
}

unsigned long string_to_hash (
	const char *str)		
{
	char	md5val[33];
	int	i, j;
	char	*p;
	unsigned long hash, val;

	md5_hexdigest_string (md5val, str);
	for (i = 0, p = md5val, hash = 0; i < 4; i++) {
		for (j = 0, val = 0; j < 8; j++, p++) {
			val = (val << 4) +
			      (*p < 'A' ? *p - '0' : *p - 'A' + 10);
		}
		hash += val;
	}
	return (hash & 0x7FFFFFFF);
}

unsigned long build_rolling_hash (
	struct work_unit *w)		
{
	char	buf[80];

	gw_as_string (buf, w->k, w->b, w->n, w->c);
	return (string_to_hash (buf));
}

void rolling_average_work_unit_complete (
	int	thread_num,		
	struct work_unit *completed_w)
{
	unsigned long hash, time_to_complete;
	struct work_unit *first_w, *second_w;

	hash = IniSectionGetInt (INI_FILE, SEC_Internals, KEY_RollingHash, 0);
	time_to_complete = IniSectionGetInt (INI_FILE, SEC_Internals, KEY_RollingCompleteTime, 0);

	gwmutex_lock (&WORKTODO_MUTEX);

	for (first_w = WORK_UNITS[thread_num].first;
	     first_w != NULL && first_w->work_type == WORK_NONE;
	     first_w = first_w->next);
	for (second_w = first_w->next;
	     second_w != NULL && second_w->work_type == WORK_NONE;
	     second_w = second_w->next);

	if (first_w == completed_w && second_w != NULL) {

		hash -= build_rolling_hash (first_w);
		hash += build_rolling_hash (second_w);

		time_to_complete += (unsigned long)
			work_estimate (thread_num, second_w);
	}

	gwmutex_unlock (&WORKTODO_MUTEX);

	IniSectionWriteInt (INI_FILE, SEC_Internals, KEY_RollingHash, hash);
	IniSectionWriteInt (INI_FILE, SEC_Internals, KEY_RollingCompleteTime, time_to_complete);
}

void adjust_rolling_average (void)
{
	unsigned int tnum;
	unsigned long time_to_complete, starting_time_to_complete;
	time_t	current_time, starting_time;
	unsigned long hash, time_in_this_period;
	double	rolling_average_this_period, pct;

	gwmutex_lock (&WORKTODO_MUTEX);

	hash = 0;
	time_to_complete = 0;
	for (tnum = 0; tnum < NUM_WORKERS; tnum++) {
		struct work_unit *w;

		for (w = WORK_UNITS[tnum].first; w != NULL; w = w->next)
			if (w->work_type != WORK_NONE &&
			    w->work_type != WORK_DELETED) break;
		if (w == NULL) continue;

		hash += build_rolling_hash (w);

		time_to_complete += (unsigned long) work_estimate (tnum, w);
	}

	gwmutex_unlock (&WORKTODO_MUTEX);

	starting_time = IniSectionGetInt (INI_FILE, SEC_Internals, KEY_RollingStartTime, 0);
	time (&current_time);

	if (hash != IniSectionGetInt (INI_FILE, SEC_Internals, KEY_RollingHash, 0)) goto no_update;

	time_in_this_period = (unsigned long) (current_time - starting_time);
	starting_time_to_complete = IniSectionGetInt (INI_FILE, SEC_Internals, KEY_RollingCompleteTime, 0);

	if (starting_time == 0 ||
	    current_time <= starting_time ||
	    time_in_this_period > 30 * 86400 ||
	    starting_time_to_complete <= time_to_complete)
		goto no_update;

	rolling_average_this_period =
		(double) (starting_time_to_complete - time_to_complete) /
		(double) NUM_WORKERS /
		(double) time_in_this_period *
		(double) ROLLING_AVERAGE;

	if (NUM_WORKERS > HW_NUM_CORES)
		rolling_average_this_period *= (double) NUM_WORKERS / (double) HW_NUM_CORES;

	if (rolling_average_this_period > 50000.0) goto no_update;
	if (rolling_average_this_period < 0.5 * ROLLING_AVERAGE) rolling_average_this_period = 0.5 * ROLLING_AVERAGE;
	if (rolling_average_this_period > 2.0 * ROLLING_AVERAGE) rolling_average_this_period = 2.0 * ROLLING_AVERAGE;

	pct = (double) time_in_this_period / (30.0 * 86400.0);
	ROLLING_AVERAGE = (unsigned long) ((1.0 - pct) * ROLLING_AVERAGE + pct * rolling_average_this_period + 0.5);

	if (ROLLING_AVERAGE < 20) ROLLING_AVERAGE = 20;
	if (ROLLING_AVERAGE > 4000) ROLLING_AVERAGE = 4000;

no_update:
	IniSectionWriteInt (INI_FILE, SEC_Internals, KEY_RollingHash, hash);
	IniSectionWriteInt (INI_FILE, SEC_Internals, KEY_RollingStartTime, (unsigned long) current_time);
	IniSectionWriteInt (INI_FILE, SEC_Internals, KEY_RollingCompleteTime, time_to_complete);
	IniSectionWriteInt (INI_FILE, SEC_Internals, KEY_RollingAverage, ROLLING_AVERAGE);
}

void invalidateNextRollingAverageUpdate (void)
{
	IniSectionWriteInt (INI_FILE, SEC_Internals, KEY_RollingStartTime, 0);
	adjust_rolling_average ();
}

void tempFileName (
	struct work_unit *w,
	char	*buf)
{
	char	c;
	unsigned long p;

	p = w->n;
	if (p < 80000000) {
		sprintf (buf, "p%07li", p % 10000000);
		if (p >= 10000000)	
			buf[1] = (char) ('A' + (p / 1000000) - 10);
		if (p >= 35000000)	
			c = buf[1], buf[1] = buf[2], buf[2] = (char)(c - 25);
		if (p >= 61000000)	
			c = buf[2], buf[2] = buf[3], buf[3] = (char)(c - 25);
	} else
		sprintf (buf, "p%ld", p);

	if (w->work_type == WORK_FACTOR) buf[0] = 'f';
	if (w->work_type == WORK_ECM) buf[0] = 'e';
	if (w->work_type == WORK_PMINUS1 || w->work_type == WORK_PFACTOR) buf[0] = 'm';
	if (w->work_type == WORK_PPLUS1) buf[0] = 'n';
	if (w->work_type == WORK_CERT) buf[0] = 'c';

	if (w->work_type == WORK_PFACTOR) {
		char	v258_filename[32];
		sprintf (v258_filename, "p%s", buf+1);
		rename (v258_filename, buf);
	}

	if (w->c == 1 && buf[0] == 'm') {
		char	v258_filename[32];
		sprintf (v258_filename, "l%s", buf+1);
		rename (v258_filename, buf);
	}
	if (w->c == 1 && buf[0] == 'e') {
		char	v258_filename[32];
		sprintf (v258_filename, "d%s", buf+1);
		rename (v258_filename, buf);
	}

	if (w->k != 1.0 || labs(w->c) != 1) {
		char	v258_filename[32];
		strcpy (v258_filename, buf);
		buf[1] = 0;
		if (w->k != 1.0) sprintf (buf+strlen(buf), "%g", fmod (w->k, 1000000.0));
		sprintf (buf+strlen(buf), "_%ld", p);
		if (labs(w->c) != 1) sprintf (buf+strlen(buf), "_%ld", labs(w->c) % 1000);
		rename (v258_filename, buf);
		if (buf[0] == 'p') {
			v258_filename[0] = buf[0] = 'q';
			rename (v258_filename, buf);
			buf[0] = 'p';
		}
	}

	if (w->extension[0]) {
		strcat (buf, ".");
		strcat (buf, w->extension);
	}
}

int fileExists (
	const char *filename)
{
	int	fd;
	fd = _open (filename, _O_RDONLY | _O_BINARY);
	if (fd < 0) return (0);
	_close (fd);
	return (1);
}

void DirPlusFilename (
	char	*dir,				
	const char *filename)
{
	if (dir[0]) {
		char	slash = getDirectorySeparator ();
		int	len = (int) strlen (dir);

		if (dir[len-1] != slash) {
			dir[len] = slash;
			dir[len+1] = 0;
		}
	}
	strcat (dir, filename);
}

int read_array (
	int	fd,
	char	*buf,
	size_t	len,
	uint32_t *sum)
{
	size_t	i;
	unsigned char *ubuf;

	if (__read (fd, buf, len) != len) return (FALSE);
	ubuf = (unsigned char *) buf;
	if (sum != NULL) for (i = 0; i < len; i++) *sum += (uint32_t) ubuf[i];
	return (TRUE);
}

int write_array (
	int	fd,
	const char *buf,
	size_t	len,
	uint32_t *sum)
{
	size_t	i;
	unsigned char *ubuf;

	if (len == 0) return (TRUE);
	if (__write (fd, buf, len) != len) return (FALSE);
	ubuf = (unsigned char *) buf;
	if (sum != NULL) for (i = 0; i < len; i++) *sum += (uint32_t) ubuf[i];
	return (TRUE);
}

int read_giant (
	int	fd,
	giant	g,
	uint32_t *sum)
{
	unsigned long i, len, bytes;

	if (!read_long (fd, &len, sum)) return (FALSE);
	if (len == 0) return (FALSE);

	bytes = len * sizeof (uint32_t);
	if (_read (fd, g->n, bytes) != bytes) return (FALSE);
	if (len && g->n[len-1] == 0) return (FALSE);
	g->sign = len;
	*sum += (uint32_t) len;
	for (i = 0; i < len; i++) *sum += (uint32_t) g->n[i];
	return (TRUE);
}

int write_giant (
	int	fd,
	giant	g,
	uint32_t *sum)
{
	unsigned long i, len, bytes;

	len = g->sign;
	if (len == 0) {
		OutputBoth (MAIN_THREAD_NUM, "In write_giant, unexpected len == 0 failure\n");
		return (FALSE);
	}
	if (!write_long (fd, len, sum)) {
		OutputBoth (MAIN_THREAD_NUM, "In write_giant, unexpected write_long failure\n");
		return (FALSE);
	}
	bytes = len * sizeof (uint32_t);
	if (_write (fd, g->n, bytes) != bytes) {
		char	buf[200];
		sprintf (buf, "In write_giant, unexpected write failure len = %lu\n", len);
		OutputBoth (MAIN_THREAD_NUM, buf);
		return (FALSE);
	}
	*sum += (uint32_t) len;
	for (i = 0; i < len; i++) *sum += (uint32_t) g->n[i];
	return (TRUE);
}

int read_gwnum (
	int	fd,
	gwhandle *gwdata,
	gwnum	g,
	uint32_t *sum)
{
	giant	tmp;

	tmp = popg (&gwdata->gdata, ((int) gwdata->bit_length >> 5) + 10);
	if (tmp == NULL) return (FALSE);	
						
	if (!read_giant (fd, tmp, sum)) {
		pushg (&gwdata->gdata, 1);
		return (FALSE);
	}

	gianttogw (gwdata, tmp, g);
	pushg (&gwdata->gdata, 1);
	return (TRUE);
}

int write_gwnum (
	int	fd,
	gwhandle *gwdata,
	gwnum	g,
	uint32_t *sum)
{
	giant	tmp;
	gwnum	tmp_gwnum = NULL;
	int	retcode;

	tmp = popg (&gwdata->gdata, ((int) gwdata->bit_length >> 5) + 10);
	if (tmp == NULL) {
		OutputBoth (MAIN_THREAD_NUM, "In write_gwnum, unexpected popg failure\n");
		return (FALSE);
	}
	if (!gwnum_is_not_ffted (gwdata, g)) {
		tmp_gwnum = gwalloc (gwdata);
		if (tmp_gwnum == NULL) {
			OutputBoth (MAIN_THREAD_NUM, "In write_gwnum, unexpected gwalloc failure\n");
			goto err;
		}
		gwunfft (gwdata, g, tmp_gwnum);
		g = tmp_gwnum;
	}
	retcode = gwtogiant (gwdata, g, tmp);
	if (retcode) {
		char	buf[200];
		sprintf (buf, "In write_gwnum, unexpected gwtogiant failure, retcode %d\n", retcode);
		OutputBoth (MAIN_THREAD_NUM, buf);
		goto err;
	}
	if (!write_giant (fd, tmp, sum)) goto err;
	pushg (&gwdata->gdata, 1);
	gwfree (gwdata, tmp_gwnum);
	return (TRUE);
err:	pushg (&gwdata->gdata, 1);
	gwfree (gwdata, tmp_gwnum);
	return (FALSE);
}

int read_int32 (
	int	fd,
	int32_t *val,
	uint32_t *sum)
{
	if (_read (fd, val, sizeof (int32_t)) != sizeof (int32_t)) return (FALSE);
	if (sum != NULL) *sum += (uint32_t) *val;
	return (TRUE);
}

int write_int32 (
	int	fd,
	int32_t val,
	uint32_t *sum)
{
	if (_write (fd, &val, sizeof (int32_t)) != sizeof (int32_t)) return (FALSE);
	if (sum != NULL) *sum += (uint32_t) val;
	return (TRUE);
}

int read_uint32 (
	int	fd,
	uint32_t *val,
	uint32_t *sum)
{
	if (_read (fd, val, sizeof (uint32_t)) != sizeof (uint32_t)) return (FALSE);
	if (sum != NULL) *sum += (uint32_t) *val;
	return (TRUE);
}

int write_uint32 (
	int	fd,
	uint32_t val,
	uint32_t *sum)
{
	if (_write (fd, &val, sizeof (uint32_t)) != sizeof (uint32_t)) return (FALSE);
	if (sum != NULL) *sum += (uint32_t) val;
	return (TRUE);
}

int read_int64 (
	int	fd,
	int64_t *val,
	uint32_t *sum)
{
	if (_read (fd, val, sizeof (int64_t)) != sizeof (int64_t)) return (FALSE);
	if (sum != NULL) *sum += (uint32_t) ((*val >> 32) + *val);
	return (TRUE);
}

int write_int64 (
	int	fd,
	int64_t val,
	uint32_t *sum)
{
	if (_write (fd, &val, sizeof (int64_t)) != sizeof (int64_t)) return (FALSE);
	if (sum != NULL) *sum += (uint32_t) ((val >> 32) + val);
	return (TRUE);
}

int read_uint64 (
	int	fd,
	uint64_t *val,
	uint32_t *sum)
{
	if (_read (fd, val, sizeof (uint64_t)) != sizeof (uint64_t)) return (FALSE);
	if (sum != NULL) *sum += (uint32_t) ((*val >> 32) + *val);
	return (TRUE);
}

int write_uint64 (
	int	fd,
	uint64_t val,
	uint32_t *sum)
{
	if (_write (fd, &val, sizeof (uint64_t)) != sizeof (uint64_t)) return (FALSE);
	if (sum != NULL) *sum += (uint32_t) ((val >> 32) + val);
	return (TRUE);
}

int read_double (
	int	fd,
	double	*val,
	uint32_t *sum)
{
	if (_read (fd, val, sizeof (double)) != sizeof (double)) return (FALSE);
	if (sum != NULL) *sum += (uint32_t) *val;
	return (TRUE);
}

int write_double (
	int	fd,
	double	val,
	uint32_t *sum)
{
	if (_write (fd, &val, sizeof (double)) != sizeof (double)) return (FALSE);
	if (sum != NULL) *sum += (uint32_t) val;
	return (TRUE);
}

int read_long (				
	int	fd,
	unsigned long *val,
	uint32_t *sum)
{
	uint32_t tmp;
	int retval = read_uint32 (fd, &tmp, sum);
	*val = tmp;
	return (retval);
}

int read_slong (			
	int	fd,
	long	*val,
	uint32_t *sum)
{
	int32_t tmp;
	int retval = read_int32 (fd, &tmp, sum);
	*val = tmp;
	return (retval);
}

int read_int (				
	int	fd,
	int	*val,
	uint32_t *sum)
{
	int32_t tmp;
	int retval = read_int32 (fd, &tmp, sum);
	*val = tmp;
	return (retval);
}

int read_magicnum (
	int	fd,
	uint32_t magicnum)
{
	uint32_t filenum;

	_lseek (fd, 0, SEEK_SET);
	if (!read_uint32 (fd, &filenum, NULL)) return (FALSE);

	return (filenum == magicnum);
}

int read_header (
	int	fd,
	uint32_t *version,
	struct work_unit *w,
	uint32_t *sum)
{
	double	k;
	unsigned long b, n;
	long	c;
	char	pad;
	char	stage[10];
	double	pct_complete;
	uint32_t trash_sum;

	_lseek (fd, sizeof (uint32_t), SEEK_SET);

	if (!read_uint32 (fd, version, NULL)) return (FALSE);
	if (!read_double (fd, &k, NULL)) return (FALSE);
	if (!read_long (fd, &b, NULL)) return (FALSE);
	if (!read_long (fd, &n, NULL)) return (FALSE);
	if (!read_slong (fd, &c, NULL)) return (FALSE);
	if (!read_array (fd, stage, 10, NULL)) return (FALSE);
	if (!read_array (fd, &pad, 1, NULL)) return (FALSE);
	if (!read_array (fd, &pad, 1, NULL)) return (FALSE);
	if (!read_double (fd, &pct_complete, NULL)) return (FALSE);
	if (sum == NULL) sum = &trash_sum;
	if (!read_uint32 (fd, sum, NULL)) return (FALSE);

	if (k != w->k || b != w->b || n != w->n || c != w->c) return (FALSE);

	stage[9] = 0;
	strcpy (w->stage, stage);
	if (pct_complete < 0.0) pct_complete = 0.0;
	if (pct_complete > 1.0) pct_complete = 1.0;
	w->pct_complete = pct_complete;

	return (TRUE);
}

int write_header (
	int	fd,
	uint32_t magicnum,
	uint32_t version,
	struct work_unit *w)
{
	char	pad = 0;
	uint32_t sum = 0;

	if (!write_uint32 (fd, magicnum, NULL)) return (FALSE);
	if (!write_uint32 (fd, version, NULL)) return (FALSE);
	if (!write_double (fd, w->k, NULL)) return (FALSE);
	if (!write_long (fd, w->b, NULL)) return (FALSE);
	if (!write_long (fd, w->n, NULL)) return (FALSE);
	if (!write_slong (fd, w->c, NULL)) return (FALSE);
	if (!write_array (fd, w->stage, 10, NULL)) return (FALSE);
	if (!write_array (fd, &pad, 1, NULL)) return (FALSE);
	if (!write_array (fd, &pad, 1, NULL)) return (FALSE);
	if (!write_double (fd, w->pct_complete, NULL)) return (FALSE);
	if (!write_uint32 (fd, sum, NULL)) return (FALSE);
	return (TRUE);
}

#define CHECKSUM_OFFSET	48

int read_checksum (
	int	fd,
	uint32_t *sum)
{
	_lseek (fd, CHECKSUM_OFFSET, SEEK_SET);
	if (!read_uint32 (fd, sum, NULL)) return (FALSE);
	return (TRUE);
}

int write_checksum (
	int	fd,
	uint32_t sum)
{
	_lseek (fd, CHECKSUM_OFFSET, SEEK_SET);
	if (!write_uint32 (fd, sum, NULL)) return (FALSE);
	return (TRUE);
}

void formatMsgForResultsFile (
	char	*buf,		
	struct work_unit *w)
{
	char	newbuf[2000];

	if (!USERID[0])
		strcpy (newbuf, buf);
	else if (!COMPID[0])
		sprintf (newbuf, "UID: %s, %s", USERID, buf);
	else
		sprintf (newbuf, "UID: %s/%s, %s", USERID, COMPID, buf);

	if (w->assignment_uid[0])
		sprintf (newbuf + strlen (newbuf) - 1,
			 ", AID: %s\n", w->assignment_uid);

	newbuf[200] = 0;
	strcpy (buf, newbuf);
}

int writeResultsInternal (
	int	which_results_file,	
	const char *msg,
	int	output_nl)
{
static	time_t	last_time[3] = {0};
	int	fd;

	if (which_results_file < 0 || which_results_file > 2) which_results_file = 0;

	gwmutex_lock (&OUTPUT_MUTEX);
	fd = _open (RESFILES[which_results_file], _O_TEXT | _O_RDWR | _O_CREAT | _O_APPEND, CREATE_FILE_ACCESS);
	if (fd < 0) {
		gwmutex_unlock (&OUTPUT_MUTEX);
		LogMsg ("Error opening results file to output this message:\n");
		LogMsg (msg);
		return (FALSE);
	}

	if (LOGTIMESTAMPING && which_results_file != 2) {
		time_t	this_time;

		int write_interval = IniGetInt (INI_FILE, "ResultsFileTimestampInterval", 300);

		time (&this_time);
		if (write_interval && this_time - last_time[which_results_file] > (time_t) write_interval) {
			char	tmpbuf[200], buf[203];
			last_time[which_results_file] = this_time;
			timestamp (tmpbuf, 1);
			sprintf (buf, "[%s]\n", tmpbuf);
			(void) _write (fd, buf, (int) strlen (buf));
		}
	}

	if (_write (fd, msg, (unsigned int) strlen (msg)) < 0) goto fail;
	if (output_nl && _write (fd, "\n", 1) < 0) goto fail;
	_close (fd);
	gwmutex_unlock (&OUTPUT_MUTEX);
	return (TRUE);

fail:	_close (fd);
	gwmutex_unlock (&OUTPUT_MUTEX);
	LogMsg ("Error writing message to results file:\n");
	LogMsg (msg);
	return (FALSE);
}

int writeResults (
	const char *msg)
{
	return (writeResultsInternal (0, msg, FALSE));
}

int writeResultsBench (
	const char *msg)
{
	return (writeResultsInternal (1, msg, FALSE));
}

int writeResultsJSON (
	const char *msg)
{
	return (writeResultsInternal (2, msg, TRUE));
}

#define	SPOOL_FILE_MAGICNUM	0x73d392ac
#define SPOOL_FILE_VERSION	1

#define SPOOL_FILE_HEADER_OFFSET (2 * sizeof (uint32_t))

#define SPOOL_FILE_MSG_OFFSET (4 * sizeof (uint32_t))

gwthread COMMUNICATION_THREAD = 0;	
gwthread UPLOAD_THREAD = 0;		
gwmutex	SPOOL_FILE_MUTEX;		
int	GET_PING_INFO = 0;		
int	GLOBAL_SEND_MSG_COUNT = 0;	
struct work_unit *LOCKED_WORK_UNIT = NULL; 
					
#define HEADER_FLAG_MSGS	0x0001	
#define HEADER_FLAG_PO		0x0004	
#define HEADER_FLAG_END_DATES	0x0008	
#define HEADER_FLAG_QUIT_GIMPS	0x0010	
#define HEADER_FLAG_UC		0x0020	
#define HEADER_FLAG_WORK_QUEUE	0x0040	

void communicateWithServer (void *arg);

void init_spool_file_and_comm_code (void)
{
	gwmutex_init (&SPOOL_FILE_MUTEX);
	GET_PING_INFO = 0;
}

void set_comm_timers (void)
{
	time_t	last_time, current_time;

	if (!USE_PRIMENET) {
		delete_timed_event (TE_COMM_SERVER);
		delete_timed_event (TE_COMPLETION_DATES);
		delete_timed_event (TE_WORK_QUEUE_CHECK);
		return;
	}
	if (MANUAL_COMM)
		delete_timed_event (TE_COMM_SERVER);

	create_window (COMM_THREAD_NUM);
	base_title (COMM_THREAD_NUM, "Communication thread");
	ChangeIcon (COMM_THREAD_NUM, IDLE_ICON);
	title (COMM_THREAD_NUM, "Inactive");

	time (&current_time);
	last_time = IniSectionGetInt (INI_FILE, SEC_Internals, KEY_LastEndDatesSent, 0);

	if (current_time < last_time ||
	    current_time > (time_t)(last_time + DAYS_BETWEEN_CHECKINS * 86400.0))
		UpdateEndDates ();
	else
		add_timed_event (TE_COMPLETION_DATES, (int) (last_time + DAYS_BETWEEN_CHECKINS * 86400.0 - current_time));

	add_timed_event (TE_WORK_QUEUE_CHECK, 5);  
}

void do_manual_comm_now (void)
{
	gwmutex_lock (&SPOOL_FILE_MUTEX);
	this_is_a_manual_comm = TRUE;
	if (!COMMUNICATION_THREAD) gwthread_create (&COMMUNICATION_THREAD, &communicateWithServer, NULL);
	gwmutex_unlock (&SPOOL_FILE_MUTEX);
}

void clear_comm_rate_limits (void)
{

	gwmutex_lock (&SPOOL_FILE_MUTEX);
	if (!MANUAL_COMM &&
	    !COMMUNICATION_THREAD &&
	    is_timed_event_active (TE_COMM_SERVER)) {
		delete_timed_event (TE_COMM_SERVER);
		gwthread_create (&COMMUNICATION_THREAD, &communicateWithServer, NULL);
	}
	gwmutex_unlock (&SPOOL_FILE_MUTEX);
}

void pingServer (void)
{

	gwmutex_lock (&SPOOL_FILE_MUTEX);
	GET_PING_INFO = 1;
	if (!COMMUNICATION_THREAD) gwthread_create (&COMMUNICATION_THREAD, &communicateWithServer, NULL);
	gwmutex_unlock (&SPOOL_FILE_MUTEX);
}

void UpdateEndDates (void)
{
	spoolMessage (PRIMENET_ASSIGNMENT_PROGRESS, NULL);
}

void spoolMessage (
	short	msgType,
	void	*msg)
{
	int	fd;
	unsigned long magicnum, version;
	unsigned long header_word;

	if (!USE_PRIMENET) return;

	gwmutex_lock (&SPOOL_FILE_MUTEX);

	fd = _open (SPOOL_FILE, _O_RDWR | _O_BINARY | _O_CREAT, CREATE_FILE_ACCESS);
	if (fd < 0) {
		LogMsg ("ERROR: Unable to open spool file.\n");
		gwmutex_unlock (&SPOOL_FILE_MUTEX);
		return;
	}

	if (!read_long (fd, &magicnum, NULL)) {
		write_long (fd, SPOOL_FILE_MAGICNUM, NULL);
		write_long (fd, SPOOL_FILE_VERSION, NULL);
		write_long (fd, 0, NULL);
		write_long (fd, 0, NULL);
		header_word = 0;
	}

	else if (magicnum != SPOOL_FILE_MAGICNUM ||
		 !read_long (fd, &version, NULL) ||
		 version != SPOOL_FILE_VERSION ||
		 !read_long (fd, &header_word, NULL)) {
		_close (fd);
		gwmutex_unlock (&SPOOL_FILE_MUTEX);
		salvageCorruptSpoolFile ();
		spoolMessage (msgType, msg);
		return;
	}

	if (msgType == MSG_CHECK_WORK_QUEUE)
		header_word |= HEADER_FLAG_WORK_QUEUE;

	else if (msgType == PRIMENET_UPDATE_COMPUTER_INFO)
		header_word |= HEADER_FLAG_UC;

	else if (msgType == PRIMENET_PROGRAM_OPTIONS) {
		header_word |= HEADER_FLAG_PO;
	}

	else if (msgType == PRIMENET_ASSIGNMENT_PROGRESS)
		header_word |= HEADER_FLAG_END_DATES + HEADER_FLAG_UC;

	else if (msgType == MSG_QUIT_GIMPS)
		header_word |= HEADER_FLAG_QUIT_GIMPS;

	else
		header_word |= HEADER_FLAG_MSGS;

	_lseek (fd, SPOOL_FILE_HEADER_OFFSET, SEEK_SET);
	write_long (fd, header_word, NULL);

	if (msgType == -PRIMENET_ASSIGNMENT_PROGRESS ||
	    msgType == PRIMENET_ASSIGNMENT_RESULT ||
	    msgType == PRIMENET_ASSIGNMENT_UNRESERVE ||
	    msgType == PRIMENET_BENCHMARK_DATA) {
		char	buf[1024];
		short	datalen;

		while (_read (fd, buf, sizeof (buf)));

		datalen = (msgType == -PRIMENET_ASSIGNMENT_PROGRESS) ? sizeof (struct primenetAssignmentProgress) :
			  (msgType == PRIMENET_ASSIGNMENT_RESULT) ? sizeof (struct primenetAssignmentResult) :
			  (msgType == PRIMENET_ASSIGNMENT_UNRESERVE) ? sizeof (struct primenetAssignmentUnreserve) :
			  sizeof (struct primenetBenchmarkData);
		
		if (msgType == -PRIMENET_ASSIGNMENT_PROGRESS) msgType = PRIMENET_ASSIGNMENT_PROGRESS;
		(void) _write (fd, &msgType, sizeof (short));
		if (msgType == PRIMENET_ASSIGNMENT_PROGRESS) msgType = -PRIMENET_ASSIGNMENT_PROGRESS;
		(void) _write (fd, &datalen, sizeof (short));
		(void) _write (fd, msg, datalen);
	}

	_close (fd);

	if (!STARTUP_IN_PROGRESS &&
	    !MANUAL_COMM &&
	    !COMMUNICATION_THREAD &&
	    !is_timed_event_active (TE_COMM_SERVER) &&
	    msgType != -PRIMENET_ASSIGNMENT_PROGRESS)
		gwthread_create (&COMMUNICATION_THREAD, &communicateWithServer, NULL);

	gwmutex_unlock (&SPOOL_FILE_MUTEX);
}

void spoolExistingResultsFile (void)
{
	int	i;
	char	*filename;
	char	line[256];
	FILE	*fd;

	for (i = 1; i <= 2; i++) {
		if (i == 1) filename = "results.txt";
		if (i == 2) {
			if (!strcmp (RESFILE, "results.txt")) continue;
			filename = RESFILE;
		}
		fd = fopen (filename, "r");
		if (fd == NULL) continue;
		while (fgets (line, sizeof (line) - 1, fd)) {
			if (line[0] == '[') continue;
			if (strstr (line, "Res64") == NULL &&
			    strstr (line, "factor:") == NULL &&
			    strstr (line, "completed P-1") == NULL &&
			    strstr (line, "no factor") == NULL) continue;
			if (line[0] == 'U' && strchr (line, ',') != NULL)
				safe_strcpy (line, strchr (line, ',') + 2);

		}
		fclose (fd);
	}
}

int unreserve (
	unsigned long p)
{
	unsigned int tnum;
	int	rc, found_one;

	found_one = FALSE;
	for (tnum = 0; tnum < NUM_WORKERS; tnum++) {
	    struct work_unit *w;

	    w = NULL;
	    for ( ; ; ) {

		w = getNextWorkToDoLine (tnum, w, SHORT_TERM_USE);
		if (w == NULL) break;
		if (w->work_type == WORK_NONE) continue;

		if (w->n != p) continue;
		found_one = TRUE;

		if (w->assignment_uid[0]) {
			struct primenetAssignmentUnreserve pkt;
			memset (&pkt, 0, sizeof (pkt));
			strcpy (pkt.computer_guid, COMPUTER_GUID);
			strcpy (pkt.assignment_uid, w->assignment_uid);
			spoolMessage (PRIMENET_ASSIGNMENT_UNRESERVE, &pkt);
		}

		rc = deleteWorkToDoLine (tnum, w, TRUE);
		if (rc) return (rc);
	    }
	}

	if (!found_one) {
		char	buf[90];
		sprintf (buf, "Error unreserving exponent: %lu not found in worktodo.txt\n", p);
		OutputStr (MAIN_THREAD_NUM, buf);
	}

	return (0);
}

void LogMsg (
	const char *str)
{
	int	fd;
	unsigned long filelen;
static	time_t	last_time = 0;
	time_t	this_time;

	OutputStr (COMM_THREAD_NUM, str);

	gwmutex_lock (&LOG_MUTEX);
	fd = _open (LOGFILE, _O_TEXT | _O_RDWR | _O_CREAT, CREATE_FILE_ACCESS);
	if (fd < 0) {
		gwmutex_unlock (&LOG_MUTEX);
		OutputStr (COMM_THREAD_NUM, "Unable to open log file.\n");
		return;
	}
	filelen = _lseek (fd, 0L, SEEK_END);

	if (filelen > (unsigned long) IniGetInt (INI_FILE, "MaxLogFileSize", 2000000)) {
		char	*buf, *p;
		int	bytes_read;

		buf = (char *) malloc (filelen);
		if (buf != NULL) {
			_lseek (fd, 100000L, SEEK_SET);
			strcpy (buf, "Prior log file entries removed.\n");
			for (p = buf + strlen (buf); (bytes_read = _read (fd, p, 50000)) != 0; p += bytes_read)
				;
		       	_close (fd);
			fd = _open (LOGFILE, _O_TEXT | _O_RDWR | _O_TRUNC, CREATE_FILE_ACCESS);
			if (fd < 0) {
				free (buf);
				gwmutex_unlock (&LOG_MUTEX);
				OutputStr (COMM_THREAD_NUM, "Unable to truncate log file.\n");
				return;
			}
			(void) _write (fd, buf, (unsigned int) (p - buf));
			free (buf);
		}
	}

	time (&this_time);
	if (this_time - last_time > 300) {
		char	tmpbuf[50], buf[100];
		last_time = this_time;
		timestamp (tmpbuf, 1);
		sprintf (buf, "[%s - ver %s]\n", tmpbuf, VERSION);
		(void) _write (fd, buf, (unsigned int) strlen (buf));
	}

	(void) _write (fd, str, (unsigned int) strlen (str));
	_close (fd);
	gwmutex_unlock (&LOG_MUTEX);
}

void kbnc_to_text (
	char	*buf,
	int	primenet_work_type,
	int	prp_dblchk,
	double	k,
	unsigned long b,
	unsigned long n,
	long	c)
{
	char	num[80], *work_type_str;

	switch (primenet_work_type) {
	case PRIMENET_WORK_TYPE_FACTOR:
		work_type_str = "Trial factor";
		k = 1.0; b = 2; c = -1;
		break;
	case PRIMENET_WORK_TYPE_FIRST_LL:
		work_type_str = "LL";
		k = 1.0; b = 2; c = -1;
		break;
	case PRIMENET_WORK_TYPE_DBLCHK:
		work_type_str = "Double check";
		k = 1.0; b = 2; c = -1;
		break;
	case PRIMENET_WORK_TYPE_ECM:
		work_type_str = "ECM";
		break;
	case PRIMENET_WORK_TYPE_PFACTOR:
		work_type_str = "P-1";
		k = 1.0; b = 2; c = -1;
		break;
	case PRIMENET_WORK_TYPE_PMINUS1:
		work_type_str = "P-1";
		break;
	case PRIMENET_WORK_TYPE_PPLUS1:
		work_type_str = "P+1";
		break;
	case PRIMENET_WORK_TYPE_PRP:
		work_type_str = prp_dblchk ? "PRPDC" : "PRP";
		break;
	case PRIMENET_WORK_TYPE_CERT:
		work_type_str = "CERT";
		break;
	default:
		work_type_str = "Unknown work type";
		break;
	}
	gw_as_string (num, k, b, n, c);
	sprintf (buf, "%s %s", work_type_str, num);
}

void aid_to_text (
	char	*buf,
	char	*aid)
{
	unsigned int tnum;
	struct work_unit *w;

	for (tnum = 0; tnum < NUM_WORKERS; tnum++) {
	    w = NULL;
	    for ( ; ; ) {
		w = getNextWorkToDoLine (tnum, w, SHORT_TERM_USE);
		if (w == NULL) break;
		if (w->work_type == WORK_NONE) continue;
		if (strcmp (aid, w->assignment_uid)) continue;
		gw_as_string (buf, w->k, w->b, w->n, w->c);
		decrementWorkUnitUseCount (w, SHORT_TERM_USE);
		return;
	    }
	}
	sprintf (buf, "assignment %s", aid);
}

void readMessage (
	int	fd,
	long	*offset,	
	short	*msgType,	
	void	*msg)
{
	short	datalen;

	for ( ; ; ) {
		*offset = _lseek (fd, 0, SEEK_CUR);
		if (_read (fd, msgType, sizeof (short)) != sizeof (short)) break;
		if (_read (fd, &datalen, sizeof (short)) != sizeof (short)) break;

		if (_read (fd, msg, datalen) != datalen) break;

		if (*msgType == -1) continue;

		if (*msgType == PRIMENET_ASSIGNMENT_RESULT) {
			struct primenetAssignmentResult *ar_msg = (struct primenetAssignmentResult *) msg;
			if (ar_msg->B1 >> 63) ar_msg->B1 = (uint64_t) *((double *) &ar_msg->B1);
			if (ar_msg->B2 >> 63) ar_msg->B2 = (uint64_t) *((double *) &ar_msg->B2);
		}

		if (*msgType == PRIMENET_ASSIGNMENT_PROGRESS ||
		    *msgType == PRIMENET_ASSIGNMENT_RESULT ||
		    *msgType == PRIMENET_ASSIGNMENT_UNRESERVE ||
		    *msgType == PRIMENET_BENCHMARK_DATA)
			return;

		LogMsg ("Corrupt spool file.  Message ignored.\n");
	}

	*msgType = 0;
}

int sendMessage (
	short	msgType,
	void	*msg)
{
	struct primenetAssignmentResult *pkt;
	int	local_send_msg_count;
	int	return_code;
	char	buf[400], info[200];

	if (!LoadPrimeNet ()) return (PRIMENET_ERROR_MODEM_OFF);

	pkt = (struct primenetAssignmentResult *) msg;

	switch (msgType) {
	case PRIMENET_UPDATE_COMPUTER_INFO:
		LogMsg ("Updating computer information on the server\n");
		break;
	case PRIMENET_PROGRAM_OPTIONS:
		LogMsg ("Exchanging program options with server\n");
		break;
	case PRIMENET_PING_SERVER:
		OutputStr (COMM_THREAD_NUM, "Contacting PrimeNet Server.\n");
		break;
	case PRIMENET_GET_ASSIGNMENT:
		if (!((struct primenetGetAssignment *)pkt)->get_cert_work) LogMsg ("Getting assignment from server\n");
		break;
	case PRIMENET_REGISTER_ASSIGNMENT:
		kbnc_to_text (info,
			      ((struct primenetRegisterAssignment *)pkt)->work_type, 0,
			      ((struct primenetRegisterAssignment *)pkt)->k,
			      ((struct primenetRegisterAssignment *)pkt)->b,
			      ((struct primenetRegisterAssignment *)pkt)->n,
			      ((struct primenetRegisterAssignment *)pkt)->c);
		sprintf (buf, "Registering assignment: %s\n", info);
		LogMsg (buf);
		break;
	case PRIMENET_ASSIGNMENT_PROGRESS:
		aid_to_text (info, ((struct primenetAssignmentProgress *)pkt)->assignment_uid);
		if (((struct primenetAssignmentProgress *)pkt)->iteration) {
			sprintf (buf, "Sending interim residue %d for %s\n", ((struct primenetAssignmentProgress *)pkt)->iteration, info);
			LogMsg (buf);
		} else {
			time_t	this_time;
			char	timebuf[30];
			time (&this_time);
			this_time += ((struct primenetAssignmentProgress *)pkt)->end_date;
			strcpy (timebuf, ctime (&this_time)+4);
			safe_strcpy (timebuf+6, timebuf+15);
			sprintf (buf, "Sending expected completion date for %s: %s", info, timebuf);
			LogMsg (buf);
		}
		break;
	case PRIMENET_ASSIGNMENT_UNRESERVE:
		aid_to_text (info, ((struct primenetAssignmentUnreserve *)pkt)->assignment_uid);
		sprintf (buf, "Unreserving %s\n", info);
		LogMsg (buf);
		break;
	case PRIMENET_ASSIGNMENT_RESULT:
		sprintf (buf, "Sending result to server: %s\n", ((struct primenetAssignmentResult *)pkt)->message);
		LogMsg (buf);
		break;
	case PRIMENET_BENCHMARK_DATA:
		LogMsg ("Sending benchmark data to server\n");
		break;
	}

	pkt->versionNumber = PRIMENET_VERSION;

	local_send_msg_count = ++GLOBAL_SEND_MSG_COUNT;
	add_timed_event (TE_COMM_KILL, 15*60);
	return_code = PRIMENET (msgType, pkt);
	delete_timed_event (TE_COMM_KILL);

	if (GLOBAL_SEND_MSG_COUNT != local_send_msg_count) return (9999);

	if (return_code == 0)
	switch (msgType) {
	case PRIMENET_GET_ASSIGNMENT:
		kbnc_to_text (info,
			      ((struct primenetGetAssignment *)pkt)->work_type,
			      ((struct primenetGetAssignment *)pkt)->prp_dblchk,
			      ((struct primenetGetAssignment *)pkt)->k,
			      ((struct primenetGetAssignment *)pkt)->b,
			      ((struct primenetGetAssignment *)pkt)->n,
			      ((struct primenetGetAssignment *)pkt)->c);
		sprintf (buf, "Got assignment %s: %s\n",
			 ((struct primenetGetAssignment *)pkt)->assignment_uid,
			 info);
		LogMsg (buf);
		break;
	case PRIMENET_REGISTER_ASSIGNMENT:
		sprintf (buf, "Assignment registered as: %s\n",
			 ((struct primenetRegisterAssignment *)pkt)->assignment_uid);
		LogMsg (buf);
		break;
	}

	return (return_code);
}

int getProgramOptions (void)
{
	struct primenetProgramOptions pkt;
	int	rc, tnum, mem_changed, restart, original_rob, mem_readable;
	unsigned int day_memory, night_memory, day_start_time, day_end_time;

	mem_readable = read_memory_settings (&day_memory, &night_memory,
					     &day_start_time, &day_end_time);

	restart = FALSE;
	mem_changed = FALSE;
	original_rob = RUN_ON_BATTERY;
	for (tnum = -1; tnum < (int) NUM_WORKERS; tnum++) {

		memset (&pkt, 0, sizeof (pkt));
		strcpy (pkt.computer_guid, COMPUTER_GUID);
		pkt.cpu_num = tnum;
		pkt.work_preference = -1;
		pkt.priority = -1;
		pkt.daysOfWork = -1;
		pkt.dayMemory = -1;
		pkt.nightMemory = -1;
		pkt.dayStartTime = -1;
		pkt.nightStartTime = -1;
		pkt.runOnBattery = -1;
		pkt.num_workers = -1;

		LOCKED_WORK_UNIT = NULL;
		rc = sendMessage (PRIMENET_PROGRAM_OPTIONS, &pkt);
		if (rc) return (rc);

		if (pkt.work_preference != -1) {
			if (tnum == -1) {
				PTOSetAll (INI_FILE, "WorkPreference", KEY_SrvrPO1, WORK_PREFERENCE, pkt.work_preference);
			} else {
				PTOSetOne (INI_FILE, "WorkPreference", KEY_SrvrPO1, WORK_PREFERENCE, tnum, pkt.work_preference);
			}
		}

		if (pkt.priority != -1) {
			PRIORITY = pkt.priority;
			IniWriteInt (INI_FILE, "Priority", PRIORITY);
			IniSectionWriteInt (INI_FILE, SEC_Internals, KEY_SrvrPO2, PRIORITY);
			restart = TRUE;
		}

		if (pkt.daysOfWork != -1) {
			DAYS_OF_WORK = pkt.daysOfWork;
			IniWriteInt (INI_FILE, "DaysOfWork", DAYS_OF_WORK);
			IniSectionWriteInt (INI_FILE, SEC_Internals, KEY_SrvrPO3, DAYS_OF_WORK);
		}

		if (pkt.dayMemory != -1) {
			if (day_memory != pkt.dayMemory) {
				day_memory = pkt.dayMemory;
				mem_changed = TRUE;
			}
			IniSectionWriteInt (INI_FILE, SEC_Internals, KEY_SrvrPO4, day_memory);
		}

		if (pkt.nightMemory != -1) {
			if (night_memory != pkt.nightMemory) {
				night_memory = pkt.nightMemory;
				mem_changed = TRUE;
			}
			IniSectionWriteInt (INI_FILE, SEC_Internals, KEY_SrvrPO5, night_memory);
		}

		if (pkt.dayStartTime != -1) {
			if (day_start_time != pkt.dayStartTime) {
				day_start_time = pkt.dayStartTime;
				mem_changed = TRUE;
			}
			IniSectionWriteInt (INI_FILE, SEC_Internals, KEY_SrvrPO6, day_start_time);
		}

		if (pkt.nightStartTime != -1) {
			if (day_end_time != pkt.nightStartTime) {
				day_end_time = pkt.nightStartTime;
				mem_changed = TRUE;
			}
			IniSectionWriteInt (INI_FILE, SEC_Internals, KEY_SrvrPO7, day_end_time);
		}

		if (pkt.runOnBattery != -1) {
			RUN_ON_BATTERY = pkt.runOnBattery;
			IniWriteInt (INI_FILE, "RunOnBattery", RUN_ON_BATTERY);
			IniSectionWriteInt (INI_FILE, SEC_Internals, KEY_SrvrPO8, RUN_ON_BATTERY);
		}

		if (pkt.num_workers != -1) {
			if (pkt.num_workers > (int) (HW_NUM_THREADS)) pkt.num_workers = HW_NUM_THREADS;
			NUM_WORKERS = pkt.num_workers;
			IniWriteInt (INI_FILE, KEY_NumWorkers, NUM_WORKERS);
			IniSectionWriteInt (INI_FILE, SEC_Internals, KEY_SrvrPO9, NUM_WORKERS);
			restart = TRUE;
		}
		if (mem_readable && mem_changed)
			write_memory_settings (day_memory, night_memory, day_start_time, day_end_time);
	}

	IniSectionWriteInt (INI_FILE, SEC_Internals, KEY_SrvrP00, pkt.options_counter);

	if (mem_readable && mem_changed) mem_settings_have_changed ();
	if (restart) stop_workers_for_restart ();
	if (original_rob != RUN_ON_BATTERY) run_on_battery_changed ();
	return (0);
}

int sendProgramOptions (
	int	*talked_to_server)
{
	struct primenetProgramOptions pkt;
	int	rc, tnum, options_changed, work_pref_changed, mem_readable;
	unsigned int local_od;
	unsigned int day_memory, night_memory, day_start_time, day_end_time;

	mem_readable = read_memory_settings (&day_memory, &night_memory,
					     &day_start_time, &day_end_time);

	for (tnum = -1; tnum < (int) NUM_WORKERS; tnum++) {

		memset (&pkt, 0, sizeof (pkt));
		strcpy (pkt.computer_guid, COMPUTER_GUID);
		pkt.cpu_num = tnum;

		work_pref_changed = FALSE;
		options_changed = FALSE;

		pkt.work_preference = -1;
		if (PTOHasOptionChanged (KEY_SrvrPO1, WORK_PREFERENCE, tnum)) {
			if (tnum == -1)
				pkt.work_preference = WORK_PREFERENCE[0];
			else
				pkt.work_preference = WORK_PREFERENCE[tnum];
			work_pref_changed = TRUE;
			options_changed = TRUE;
		}

		pkt.priority = -1;
		if (tnum == -1 && PRIORITY != IniSectionGetInt (INI_FILE, SEC_Internals, KEY_SrvrPO2, -1)) {
			pkt.priority = PRIORITY;
			options_changed = TRUE;
		}

		pkt.daysOfWork = -1;
		if (tnum == -1 && DAYS_OF_WORK != IniSectionGetInt (INI_FILE, SEC_Internals, KEY_SrvrPO3, -1)) {
			pkt.daysOfWork = DAYS_OF_WORK;
			options_changed = TRUE;
		}

		pkt.dayMemory = -1;
		if (tnum == -1 && mem_readable && day_memory != IniSectionGetInt (INI_FILE, SEC_Internals, KEY_SrvrPO4, -1)) {
			pkt.dayMemory = day_memory;
			options_changed = TRUE;
		}

		pkt.nightMemory = -1;
		if (tnum == -1 && mem_readable && night_memory != IniSectionGetInt (INI_FILE, SEC_Internals, KEY_SrvrPO5, -1)) {
			pkt.nightMemory = night_memory;
			options_changed = TRUE;
		}

		pkt.dayStartTime = -1;
		if (tnum == -1 && mem_readable && day_start_time != IniSectionGetInt (INI_FILE, SEC_Internals, KEY_SrvrPO6, -1)) {
			pkt.dayStartTime = day_start_time;
			options_changed = TRUE;
		}

		pkt.nightStartTime = -1;
		if (tnum == -1 && mem_readable && day_end_time != IniSectionGetInt (INI_FILE, SEC_Internals, KEY_SrvrPO7, -1)) {
			pkt.nightStartTime = day_end_time;
			options_changed = TRUE;
		}

		pkt.runOnBattery = -1;
		if (tnum == -1 && RUN_ON_BATTERY != IniSectionGetInt (INI_FILE, SEC_Internals, KEY_SrvrPO8, -1)) {
			pkt.runOnBattery = RUN_ON_BATTERY;
			options_changed = TRUE;
		}

		pkt.num_workers = -1;
		if (tnum == -1 && NUM_WORKERS != IniSectionGetInt (INI_FILE, SEC_Internals, KEY_SrvrPO9, -1)) {
			pkt.num_workers = NUM_WORKERS;
			options_changed = TRUE;
		}

		if (!options_changed) continue;

		LOCKED_WORK_UNIT = NULL;
		rc = sendMessage (PRIMENET_PROGRAM_OPTIONS, &pkt);
		if (rc) return (rc);
		*talked_to_server = TRUE;

		if (work_pref_changed) {
			if (tnum == -1)
				PTOSetAll (INI_FILE, "WorkPreference", KEY_SrvrPO1, WORK_PREFERENCE, WORK_PREFERENCE[0]);
			else
				PTOSetOne (INI_FILE, "WorkPreference", KEY_SrvrPO1, WORK_PREFERENCE, tnum, WORK_PREFERENCE[tnum]);
		}
		if (tnum == -1) {
			IniSectionWriteInt (INI_FILE, SEC_Internals, KEY_SrvrPO2, PRIORITY);
			IniSectionWriteInt (INI_FILE, SEC_Internals, KEY_SrvrPO3, DAYS_OF_WORK);
			if (mem_readable) {
				IniSectionWriteInt (INI_FILE, SEC_Internals, KEY_SrvrPO4, day_memory);
				IniSectionWriteInt (INI_FILE, SEC_Internals, KEY_SrvrPO5, night_memory);
				IniSectionWriteInt (INI_FILE, SEC_Internals, KEY_SrvrPO6, day_start_time);
				IniSectionWriteInt (INI_FILE, SEC_Internals, KEY_SrvrPO7, day_end_time);
			}
			IniSectionWriteInt (INI_FILE, SEC_Internals, KEY_SrvrPO8, RUN_ON_BATTERY);
			IniSectionWriteInt (INI_FILE, SEC_Internals, KEY_SrvrPO9, NUM_WORKERS);
		}

		local_od = IniSectionGetInt (INI_FILE, SEC_Internals, KEY_SrvrP00, -1) + 1;
		if (local_od > pkt.options_counter) local_od = pkt.options_counter;
		IniSectionWriteInt (INI_FILE, SEC_Internals, KEY_SrvrP00, local_od);
	}
	return (0);
}

#define RETRY_EXCEEDED "Retry count exceeded.\n"

void communicateWithServer (void *arg)
{
static	int	obsolete_client = FALSE;
static	int	send_message_retry_count = 0;
	unsigned long magicnum, version;
	unsigned long header_words[2];
				
	int	fd;		
	long	msg_offset;	
	unsigned int tnum;
	double	est, work_to_get, unreserve_threshold;
	int	rc, stop_reason;
	int	talked_to_server = FALSE;
	int	can_get_cert_work, can_get_small_cert_work, sent_prp_result;
	int	server_options_counter, retry_count;
	char	buf[1000];

	if (obsolete_client) goto leave;

	ChangeIcon (COMM_THREAD_NUM, WORKING_ICON);
	title (COMM_THREAD_NUM, "Active");

	if (COMPID[0] == 0) {
		IniGetString (INI_FILE, "ComputerID", COMPID, sizeof (COMPID), NULL);
		sanitizeString (COMPID);
	}

	retry_count = 0;
retry:

	if (GET_PING_INFO == 1) {
		struct primenetPingServer pkt;
		memset (&pkt, 0, sizeof (pkt));

		pkt.ping_type = 0;
		LOCKED_WORK_UNIT = NULL;
		rc = sendMessage (PRIMENET_PING_SERVER, &pkt);
		OutputStr (MAIN_THREAD_NUM, "\n");
		if (rc)
			OutputStr (MAIN_THREAD_NUM, "Failure pinging server");
		else
			OutputStr (MAIN_THREAD_NUM, pkt.ping_response);
		OutputStr (MAIN_THREAD_NUM, "\n");
		GET_PING_INFO = 0;
	}

	gwmutex_lock (&SPOOL_FILE_MUTEX);
	fd = _open (SPOOL_FILE, _O_RDWR | _O_BINARY);
	if (fd < 0) goto locked_leave;

	if (!read_long (fd, &magicnum, NULL) ||
	    magicnum != SPOOL_FILE_MAGICNUM ||
	    !read_long (fd, &version, NULL) ||
	    version != SPOOL_FILE_VERSION ||
	    !read_long (fd, &header_words[0], NULL) ||
	    !read_long (fd, &header_words[1], NULL)) {
		_close (fd);
		gwmutex_unlock (&SPOOL_FILE_MUTEX);
		salvageCorruptSpoolFile ();
		goto retry;
	}

	header_words[1] |= header_words[0];
	header_words[0] = 0;
	_lseek (fd, SPOOL_FILE_HEADER_OFFSET, SEEK_SET);
	write_long (fd, header_words[0], NULL);
	write_long (fd, header_words[1], NULL);

	_close (fd);
	gwmutex_unlock (&SPOOL_FILE_MUTEX);

	server_options_counter = -1;
	if (header_words[1] & HEADER_FLAG_UC) {
		struct primenetUpdateComputerInfo pkt;
		unsigned long server_uid, server_computer_name;

		memset (&pkt, 0, sizeof (pkt));
		strcpy (pkt.computer_guid, COMPUTER_GUID);
		if (!IniGetInt (INI_FILE, "FixedHardwareUID", 0)) {
			strcpy (pkt.hardware_guid, HARDWARE_GUID);
			strcpy (pkt.windows_guid, WINDOWS_GUID);
		}
		generate_application_string (pkt.application);
		strcpy (pkt.cpu_model, CPU_BRAND);
		pkt.cpu_features[0] = 0;

		if (CPU_FLAGS & CPU_PREFETCH) strcat (pkt.cpu_features, "Prefetch,");
		if (CPU_FLAGS & CPU_3DNOW) strcat (pkt.cpu_features, "3DNow!,");

		if (CPU_FLAGS & CPU_SSE) strcat (pkt.cpu_features, "SSE,");
		if (CPU_FLAGS & CPU_SSE2) strcat (pkt.cpu_features, "SSE2,");
		if (CPU_FLAGS & CPU_SSE41) strcat (pkt.cpu_features, "SSE4,");
		if (CPU_FLAGS & CPU_AVX) strcat (pkt.cpu_features, "AVX,");
		if (CPU_FLAGS & CPU_AVX2) strcat (pkt.cpu_features, "AVX2,");
		if (CPU_FLAGS & (CPU_FMA3 | CPU_FMA4)) strcat (pkt.cpu_features, "FMA,");
		if (CPU_FLAGS & CPU_AVX512F) strcat (pkt.cpu_features, "AVX512F,");
		if (pkt.cpu_features[0])
			pkt.cpu_features[strlen (pkt.cpu_features) - 1] = 0;
		pkt.L1_cache_size = CPU_L1_CACHE_SIZE;
		pkt.L2_cache_size = CPU_L2_CACHE_SIZE;
		pkt.L3_cache_size = CPU_L3_CACHE_SIZE;
		pkt.num_cpus = HW_NUM_CORES;
		pkt.num_hyperthread = divide_rounding_up (HW_NUM_THREADS, HW_NUM_CORES);
		pkt.mem_installed = physical_memory ();
		pkt.cpu_speed = (int) CPU_SPEED;
		pkt.hours_per_day = CPU_HOURS;
		pkt.rolling_average = ROLLING_AVERAGE;
		
		server_uid = IniSectionGetInt (INI_FILE, SEC_Internals, KEY_SrvrUID, 0);
		if (string_to_hash (USERID) != server_uid) strcpy (pkt.user_id, USERID);
		
		server_computer_name = IniSectionGetInt (INI_FILE, SEC_Internals, KEY_SrvrComputerName, 0);
		if (string_to_hash (COMPID) != server_computer_name) strcpy (pkt.computer_name, COMPID);
		LOCKED_WORK_UNIT = NULL;
		rc = sendMessage (PRIMENET_UPDATE_COMPUTER_INFO, &pkt);
		if (rc) goto error_exit;
		talked_to_server = TRUE;
		strcpy (USERID, pkt.user_id);
		IniWriteString (INI_FILE, "V5UserID", USERID);
		IniSectionWriteInt (INI_FILE, SEC_Internals, KEY_SrvrUID, string_to_hash (USERID));
		strcpy (COMPID, pkt.computer_name);
		IniWriteString (INI_FILE, "ComputerID", COMPID);
		IniSectionWriteInt (INI_FILE, SEC_Internals, KEY_SrvrComputerName, string_to_hash (COMPID));
		server_options_counter = pkt.options_counter;
	}

	if (server_options_counter == 1 && server_options_counter > IniSectionGetInt (INI_FILE, SEC_Internals, KEY_SrvrP00, 0)) {
		rc = getProgramOptions ();
		if (rc) goto error_exit;
		talked_to_server = TRUE;
	}

	rc = sendProgramOptions (&talked_to_server);
	if (rc) goto error_exit;

	if (server_options_counter > IniSectionGetInt (INI_FILE, SEC_Internals, KEY_SrvrP00, 0)) {
		rc = getProgramOptions ();
		if (rc) goto error_exit;
		talked_to_server = TRUE;
	}

	msg_offset = SPOOL_FILE_MSG_OFFSET;
	sent_prp_result = FALSE;
	for ( ; ; ) {
		short	msgType;
		union {
			struct primenetAssignmentProgress ap;
			struct primenetAssignmentResult ar;
			struct primenetAssignmentUnreserve au;
			struct primenetBenchmarkData bd;
		} msg;
		long	new_offset;

		gwmutex_lock (&SPOOL_FILE_MUTEX);
		fd = _open (SPOOL_FILE, _O_RDONLY | _O_BINARY);
		if (fd < 0) goto locked_leave;
		_lseek (fd, msg_offset, SEEK_SET);
		memset (&msg, 0, sizeof (msg));		
		readMessage (fd, &msg_offset, &msgType, &msg);
		new_offset = _lseek (fd, 0, SEEK_CUR);
		_close (fd);
		gwmutex_unlock (&SPOOL_FILE_MUTEX);

		if (msgType == 0) break;
		for ( ; ; ) {
			LOCKED_WORK_UNIT = NULL;
			rc = sendMessage (msgType, &msg);
			
			if ((rc == PRIMENET_ERROR_UNREGISTERED_CPU ||
			     rc == PRIMENET_ERROR_CPU_CONFIGURATION_MISMATCH ||
			     rc == PRIMENET_ERROR_STALE_CPU_INFO) &&
			    strcmp (msg.ar.computer_guid, COMPUTER_GUID)) {
				OutputStr (COMM_THREAD_NUM, "Retrying message with this computer's GUID.\n");
				strcpy (msg.ar.computer_guid, COMPUTER_GUID);
				continue;
			}
			break;
		}
		if (rc >= PRIMENET_FIRST_INTERNAL_ERROR ||
		    rc == PRIMENET_ERROR_SERVER_BUSY ||
		    rc == PRIMENET_ERROR_OBSOLETE_CLIENT ||
		    rc == PRIMENET_ERROR_UNREGISTERED_CPU ||
		    rc == PRIMENET_ERROR_CPU_CONFIGURATION_MISMATCH ||
		    rc == PRIMENET_ERROR_STALE_CPU_INFO)
			goto error_exit;
		talked_to_server = TRUE;

		if (rc == PRIMENET_ERROR_INVALID_ASSIGNMENT_KEY ||
		    rc == PRIMENET_ERROR_INVALID_RESULT_TYPE ||
		    rc == PRIMENET_ERROR_NO_ASSIGNMENT ||
		    rc == PRIMENET_ERROR_WORK_NO_LONGER_NEEDED ||
		    rc == PRIMENET_ERROR_INVALID_PARAMETER)
			rc = 0;

		if (rc) {
			if (send_message_retry_count++ < 5) {
				rc = PRIMENET_ERROR_SERVER_BUSY;
				goto error_exit;
			}
			LogMsg ("Deleting unprocessed message from spool file.\n");
		}
		send_message_retry_count = 0;

		if (msgType == PRIMENET_ASSIGNMENT_RESULT && (msg.ar.result_type == PRIMENET_AR_PRP_RESULT || msg.ar.result_type == PRIMENET_AR_PRP_PRIME))
			sent_prp_result = TRUE;

		gwmutex_lock (&SPOOL_FILE_MUTEX);
		fd = _open (SPOOL_FILE, _O_RDWR | _O_BINARY);
		if (fd < 0) goto locked_leave;
		_lseek (fd, msg_offset, SEEK_SET);
		msgType = -1;
		(void) _write (fd, &msgType, sizeof (short));
		_close (fd);
		gwmutex_unlock (&SPOOL_FILE_MUTEX);
		msg_offset = new_offset;
	}

	if (sent_prp_result || this_is_a_manual_comm || (header_words[1] & HEADER_FLAG_QUIT_GIMPS) || IniGetInt (INI_FILE, KEY_QuitGIMPS, 0))
		gwevent_signal (&PROOF_UPLOAD_EVENT);
	this_is_a_manual_comm = FALSE;		

	can_get_cert_work = (header_words[1] & HEADER_FLAG_WORK_QUEUE) && IniGetInt (INI_FILE, "CertWork", 1) && DAYS_OF_WORK > 0;
	if (WELL_BEHAVED_WORK || SEQUENTIAL_WORK == 1) can_get_cert_work = FALSE;
	if (!WORKERS_ACTIVE || WORKERS_STOPPING) can_get_cert_work = FALSE;
	if (CPU_HOURS <= 12) can_get_cert_work = FALSE;
	if (is_timed_event_active (TE_PAUSE_WHILE) || is_timed_event_active (TE_READ_PAUSE_DATA)) can_get_cert_work = FALSE;
	if (can_get_cert_work) {
		int	max_cert_assignments;
		can_get_small_cert_work = FALSE;
		max_cert_assignments = (IniGetInt (INI_FILE, "CertDailyCPULimit", 10) >= 50 ? 3 : 1);
		for (tnum = 0; tnum < NUM_WORKERS; tnum++) {
			struct work_unit *w;
			for (w = NULL; ; ) {
				w = getNextWorkToDoLine (tnum, w, SHORT_TERM_USE);
				if (w == NULL) break;
				if (w->work_type == WORK_CERT && --max_cert_assignments == 0) can_get_cert_work = FALSE;
				if (w->n < 50000000) can_get_small_cert_work = TRUE;
			}
		}
	}

	if (can_get_cert_work) {
		float	max_daily_MB_limit, max_daily_CPU_limit;
		float	daily_MB_limit_remaining, daily_CPU_limit_remaining;
		time_t	current_time, last_update_time;
		float	days_since_last_update;

		max_daily_MB_limit = (float) IniSectionGetInt (INI_FILE, SEC_PrimeNet, KEY_DownloadDailyLimit, 40); 
		max_daily_CPU_limit = (float) IniGetInt (INI_FILE, "CertDailyCPULimit", 10); 

		daily_MB_limit_remaining = IniSectionGetFloat (INI_FILE, SEC_Internals, KEY_CertDailyMBRemaining, max_daily_MB_limit);
		daily_CPU_limit_remaining = IniSectionGetFloat (INI_FILE, SEC_Internals, KEY_CertDailyCPURemaining, max_daily_CPU_limit);

		time (&current_time);
		last_update_time = IniSectionGetInt (INI_FILE, SEC_Internals, KEY_CertDailyRemainingLastUpdate, 0);
		days_since_last_update = (float) (current_time - last_update_time) / (float) 86400.0;
		if (days_since_last_update < 0.0) days_since_last_update = 0.0;
		IniSectionWriteInt (INI_FILE, SEC_Internals, KEY_CertDailyRemainingLastUpdate, (unsigned long) current_time);

		daily_MB_limit_remaining += days_since_last_update * max_daily_MB_limit;
		if (daily_MB_limit_remaining > max_daily_MB_limit) daily_MB_limit_remaining = max_daily_MB_limit;
		daily_CPU_limit_remaining += days_since_last_update * max_daily_CPU_limit;
		if (daily_CPU_limit_remaining > max_daily_CPU_limit) daily_CPU_limit_remaining = max_daily_CPU_limit;

		IniSectionWriteFloat (INI_FILE, SEC_Internals, KEY_CertDailyMBRemaining, daily_MB_limit_remaining);
		IniSectionWriteFloat (INI_FILE, SEC_Internals, KEY_CertDailyCPURemaining, daily_CPU_limit_remaining);

		if (daily_MB_limit_remaining <= 0.0 || daily_CPU_limit_remaining <= 0.0) can_get_cert_work = FALSE;
	}

	unreserve_threshold = IniGetInt (INI_FILE, "UnreserveDays", 30) * 86400.0;
	work_to_get = DAYS_OF_WORK * 86400.0;
	for (tnum = 0; tnum < NUM_WORKERS; tnum++) {
	    struct work_unit *w;
	    int	num_work_units;
	    int	first_work_unit_interruptable = TRUE;

	    num_work_units = 0;
	    est = 0.0;
	    for (int pass = 0; pass <= 1; pass++)
	    for (w = NULL; ; ) {
		int	registered_assignment;

		w = getNextWorkToDoLine (tnum, w, SHORT_TERM_USE);
		if (w == NULL) break;
		if (w->work_type == WORK_NONE) continue;
		if (pass == 0 && w->work_type != WORK_CERT) continue;
		if (pass == 1 && w->work_type == WORK_CERT) continue;

		if (header_words[1] & HEADER_FLAG_QUIT_GIMPS ||
		    (est >= work_to_get + unreserve_threshold &&
		     ! isWorkUnitActive (w) &&
		     w->pct_complete == 0.0 &&
		     num_work_units >= IniGetInt (INI_FILE, "UnreserveExponents", 4))) {

			if (w->assignment_uid[0]) {
				struct primenetAssignmentUnreserve pkt;
				memset (&pkt, 0, sizeof (pkt));
				strcpy (pkt.computer_guid, COMPUTER_GUID);
				strcpy (pkt.assignment_uid, w->assignment_uid);
				LOCKED_WORK_UNIT = w;
				rc = sendMessage (PRIMENET_ASSIGNMENT_UNRESERVE, &pkt);
				if (rc && rc != PRIMENET_ERROR_INVALID_ASSIGNMENT_KEY) {
					decrementWorkUnitUseCount (w, SHORT_TERM_USE);
					goto error_exit;
				}
				talked_to_server = TRUE;
			}
			stop_reason = deleteWorkToDoLine (tnum, w, TRUE);
			if (stop_reason) goto error_exit;
			continue;
		}

		if (est == 0.0 && IniGetInt (INI_FILE, "CertDailyCPULimit", 10) < 50 &&
		    (w->work_type == WORK_PMINUS1 || w->work_type == WORK_PFACTOR || w->work_type == WORK_ECM ||
		     w->work_type == WORK_PPLUS1 || w->pct_complete > 0.85))
			first_work_unit_interruptable = FALSE;

		num_work_units++;
		est += work_estimate (tnum, w);

		registered_assignment = FALSE;
		if (!w->assignment_uid[0] && !w->ra_failed) {
			struct primenetRegisterAssignment pkt;
			memset (&pkt, 0, sizeof (pkt));
			strcpy (pkt.computer_guid, COMPUTER_GUID);
			pkt.cpu_num = tnum;
			if (w->work_type == WORK_FACTOR)
				pkt.work_type = PRIMENET_WORK_TYPE_FACTOR;
			if (w->work_type == WORK_PMINUS1)
				pkt.work_type = PRIMENET_WORK_TYPE_PMINUS1;
			if (w->work_type == WORK_PPLUS1)
				pkt.work_type = PRIMENET_WORK_TYPE_PPLUS1;
			if (w->work_type == WORK_PFACTOR)
				pkt.work_type = PRIMENET_WORK_TYPE_PFACTOR;
			if (w->work_type == WORK_PPLUS1)
				pkt.work_type = PRIMENET_WORK_TYPE_PPLUS1;
			if (w->work_type == WORK_ECM)
				pkt.work_type = PRIMENET_WORK_TYPE_ECM;
			if (w->work_type == WORK_TEST || w->work_type == WORK_ADVANCEDTEST)
				pkt.work_type = PRIMENET_WORK_TYPE_FIRST_LL;
			if (w->work_type == WORK_DBLCHK)
				pkt.work_type = PRIMENET_WORK_TYPE_DBLCHK;
			if (w->work_type == WORK_PRP)
				pkt.work_type = PRIMENET_WORK_TYPE_PRP;
			pkt.k = w->k;
			pkt.b = w->b;
			pkt.n = w->n;
			pkt.c = w->c;
			pkt.how_far_factored = w->sieve_depth;
			pkt.factor_to = w->factor_to;
			pkt.has_been_pminus1ed = w->pminus1ed;
			pkt.B1 = w->B1;
			pkt.B2 = w->B2;
			pkt.curves = w->curves_to_do;
			pkt.tests_saved = w->tests_saved;
			LOCKED_WORK_UNIT = w;
			rc = sendMessage (PRIMENET_REGISTER_ASSIGNMENT, &pkt);
			if (rc &&
			    rc != PRIMENET_ERROR_NO_ASSIGNMENT &&
			    rc != PRIMENET_ERROR_INVALID_ASSIGNMENT_TYPE &&
			    rc != PRIMENET_ERROR_INVALID_PARAMETER) {
				decrementWorkUnitUseCount (w, SHORT_TERM_USE);
				goto error_exit;
			}
			talked_to_server = TRUE;
			if (rc)
				w->ra_failed = TRUE;
			else {
				strcpy (w->assignment_uid, pkt.assignment_uid);
				registered_assignment = TRUE;
			}
			updateWorkToDoLine (tnum, w);
		}

		if ((header_words[1] & HEADER_FLAG_END_DATES || registered_assignment) && w->assignment_uid[0]) {
			struct primenetAssignmentProgress pkt2;
			memset (&pkt2, 0, sizeof (pkt2));
			strcpy (pkt2.computer_guid, COMPUTER_GUID);
			pkt2.cpu_num = tnum;
			strcpy (pkt2.assignment_uid, w->assignment_uid);
			strcpy (pkt2.stage, w->stage);
			pkt2.pct_complete = w->pct_complete * 100.0;
			pkt2.end_date = (unsigned long) est;
			pkt2.next_update = (uint32_t) (DAYS_BETWEEN_CHECKINS * 86400.0);
			pkt2.fftlen = w->fftlen;
			LOCKED_WORK_UNIT = w;
			rc = sendMessage (PRIMENET_ASSIGNMENT_PROGRESS, &pkt2);
			if (rc == PRIMENET_ERROR_INVALID_ASSIGNMENT_KEY || rc == PRIMENET_ERROR_WORK_NO_LONGER_NEEDED) {
				est -= work_estimate (tnum, w);
				rc = deleteWorkToDoLine (tnum, w, TRUE);
			} else if (rc) {
				decrementWorkUnitUseCount (w, SHORT_TERM_USE);
				goto error_exit;
			}
			talked_to_server = TRUE;
		}
	    }

	    if (header_words[1] & HEADER_FLAG_QUIT_GIMPS || IniGetInt (INI_FILE, KEY_QuitGIMPS, 0) || !USE_PRIMENET) continue;

	    if (can_get_cert_work && first_work_unit_interruptable && num_work_units < 20 && ACTIVE_WORKERS[tnum] &&
		IniGetInt (INI_FILE, "CertWorker", tnum+1) == tnum+1) {
		int	num_certs = 0;

		can_get_cert_work = FALSE;	

		while (num_certs < 5) {
			struct primenetGetAssignment pkt1;
			struct work_unit w;

			memset (&pkt1, 0, sizeof (pkt1));
			strcpy (pkt1.computer_guid, COMPUTER_GUID);
			pkt1.cpu_num = tnum;
			pkt1.get_cert_work = IniGetInt (INI_FILE, "CertDailyCPULimit", 10); 
			if (pkt1.get_cert_work <= 0) pkt1.get_cert_work = 1;
			pkt1.min_exp = IniGetInt (INI_FILE, "CertMinExponent", can_get_small_cert_work ? 0 : 50000000);
			pkt1.max_exp = IniGetInt (INI_FILE, "CertMaxExponent", HW_NUM_CORES / NUM_WORKERS < 4 ? 200000000 : 0);
			LOCKED_WORK_UNIT = NULL;
			rc = sendMessage (PRIMENET_GET_ASSIGNMENT, &pkt1);
			
			if (rc) break;
			talked_to_server = TRUE;

			if (pkt1.work_type != PRIMENET_WORK_TYPE_CERT) {
				sprintf (buf, "Received unknown work type (expected 200): %lu.\n", (unsigned long) pkt1.work_type);
				LogMsg (buf);
				goto error_exit;
			}
			memset (&w, 0, sizeof (w));
			w.work_type = WORK_CERT;
			strcpy (w.assignment_uid, pkt1.assignment_uid);
			w.k = 1.0;
			w.b = 2;
			w.n = pkt1.n;
			w.c = -1;
			w.cert_squarings = pkt1.num_squarings;
			num_certs++;

			stop_reason = addWorkToDoLine (tnum, &w, ADD_TO_FRONT);
			if (stop_reason) goto error_exit;

			float	daily_MB_limit_remaining, daily_CPU_limit_remaining, daily_CPU_quota_used;
			daily_MB_limit_remaining = IniSectionGetFloat (INI_FILE, SEC_Internals, KEY_CertDailyMBRemaining, 40.0);
			daily_CPU_limit_remaining = IniSectionGetFloat (INI_FILE, SEC_Internals, KEY_CertDailyCPURemaining, 10.0);

			daily_MB_limit_remaining -= (float) w.n / (float) 8388608.0;	
			daily_CPU_quota_used = (float) w.cert_squarings / (float) 110000.0;	
			daily_CPU_quota_used *= (float) pow (2.1, log2 ((float) w.n / (float) 97300000.0));	
			daily_CPU_limit_remaining -= daily_CPU_quota_used;

			IniSectionWriteFloat (INI_FILE, SEC_Internals, KEY_CertDailyMBRemaining, daily_MB_limit_remaining);
			IniSectionWriteFloat (INI_FILE, SEC_Internals, KEY_CertDailyCPURemaining, daily_CPU_limit_remaining);

			if (daily_MB_limit_remaining <= 0.0 || daily_CPU_limit_remaining <= 0.0) break;

			if (pkt1.n < 50000000) continue;

			if (num_certs < IniGetInt (INI_FILE, "CertQuantity", 1)) continue;

			if (IniGetInt (INI_FILE, "CertDailyCPULimit", 10) < 50) break;
		}
	    }

	    while (est < work_to_get && num_work_units < IniGetInt (INI_FILE, "MaxExponents", 15)) {
		struct primenetGetAssignment pkt1;
		struct primenetAssignmentProgress pkt2;
		struct work_unit w;

		memset (&pkt1, 0, sizeof (pkt1));
		strcpy (pkt1.computer_guid, COMPUTER_GUID);
		pkt1.cpu_num = tnum;
		pkt1.temp_disk_space = CPU_WORKER_DISK_SPACE;
		pkt1.min_exp = IniGetInt (INI_FILE, "GetMinExponent", 0);
		pkt1.max_exp = IniGetInt (INI_FILE, "GetMaxExponent", 0);
		LOCKED_WORK_UNIT = NULL;
		rc = sendMessage (PRIMENET_GET_ASSIGNMENT, &pkt1);
		if (rc) goto error_exit;
		talked_to_server = TRUE;

		if (pkt1.n < 15000000 &&
		    (pkt1.work_type == PRIMENET_WORK_TYPE_FACTOR ||
		     pkt1.work_type == PRIMENET_WORK_TYPE_PFACTOR ||
		     pkt1.work_type == PRIMENET_WORK_TYPE_FIRST_LL ||
		     pkt1.work_type == PRIMENET_WORK_TYPE_DBLCHK)) {
			sprintf (buf, "Server sent bad exponent: %lu.\n", (unsigned long) pkt1.n);
			LogMsg (buf);
			goto error_exit;
		}

		memset (&w, 0, sizeof (w));
		strcpy (w.assignment_uid, pkt1.assignment_uid);
		w.k = 1.0;	
		w.b = 2;
		w.n = pkt1.n;
		w.c = -1;
		switch (pkt1.work_type) {
		case PRIMENET_WORK_TYPE_FACTOR:
			w.work_type = WORK_FACTOR;
			w.sieve_depth = pkt1.how_far_factored;
			w.factor_to = pkt1.factor_to;
			break;
		case PRIMENET_WORK_TYPE_PFACTOR:
			w.work_type = WORK_PFACTOR;
			w.sieve_depth = pkt1.how_far_factored;
			w.pminus1ed = pkt1.has_been_pminus1ed;
			w.tests_saved = pkt1.tests_saved;
			break;
		case PRIMENET_WORK_TYPE_FIRST_LL:
			w.work_type = WORK_TEST;
			w.sieve_depth = pkt1.how_far_factored;
			w.pminus1ed = pkt1.has_been_pminus1ed;
			break;
		case PRIMENET_WORK_TYPE_DBLCHK:
			w.work_type = WORK_DBLCHK;
			w.sieve_depth = pkt1.how_far_factored;
			w.pminus1ed = pkt1.has_been_pminus1ed;
			break;
		case PRIMENET_WORK_TYPE_PMINUS1:
			w.work_type = WORK_PMINUS1;
			w.k = pkt1.k;
			w.b = pkt1.b;
			w.c = pkt1.c;
			w.B1 = pkt1.B1;
			w.B2 = pkt1.B2;
			break;
		case PRIMENET_WORK_TYPE_PPLUS1:		
			w.work_type = WORK_PPLUS1;
			w.k = pkt1.k;
			w.b = pkt1.b;
			w.c = pkt1.c;
			w.nth_run = pkt1.nth_run;
			w.B1 = pkt1.B1;
			w.B2 = pkt1.B2;
			break;
		case PRIMENET_WORK_TYPE_ECM:
			w.work_type = WORK_ECM;
			w.k = pkt1.k;
			w.b = pkt1.b;
			w.c = pkt1.c;
			w.B1 = pkt1.B1;
			w.B2 = pkt1.B2;
			w.curves_to_do = pkt1.curves;
			break;
		case PRIMENET_WORK_TYPE_PRP:
			w.work_type = WORK_PRP;
			w.k = pkt1.k;
			w.b = pkt1.b;
			w.c = pkt1.c;
			w.sieve_depth = pkt1.how_far_factored;
			w.tests_saved = pkt1.tests_saved;
			if (w.tests_saved == 2) w.tests_saved = 1.3;
			w.prp_base = pkt1.prp_base;
			w.prp_residue_type = pkt1.prp_residue_type;
			w.prp_dblchk = pkt1.prp_dblchk;
			break;
		default:
			sprintf (buf, "Received unknown work type: %lu.\n", (unsigned long) pkt1.work_type);
			LogMsg (buf);
			goto error_exit;
		}
		if (pkt1.known_factors[0]) { 
			w.known_factors = (char *) malloc (strlen (pkt1.known_factors) + 1);
			if (w.known_factors == NULL) {
				LogMsg ("Memory allocation error\n");
				goto error_exit;
			}
			strcpy (w.known_factors, pkt1.known_factors);
		}

		stop_reason = addWorkToDoLine (tnum, &w, ADD_TO_LOGICAL_END);
		if (stop_reason) goto error_exit;

		num_work_units++;
		est = est + work_estimate (tnum, &w);

		memset (&pkt2, 0, sizeof (pkt2));
		strcpy (pkt2.computer_guid, COMPUTER_GUID);
		pkt2.cpu_num = tnum;
		strcpy (pkt2.assignment_uid, pkt1.assignment_uid);
		pkt2.pct_complete = 0.0;
		pkt2.end_date = (unsigned long) est;
		pkt2.next_update = (uint32_t) (DAYS_BETWEEN_CHECKINS * 86400.0);
		LOCKED_WORK_UNIT = NULL;
		rc = sendMessage (PRIMENET_ASSIGNMENT_PROGRESS, &pkt2);
		if (rc == PRIMENET_ERROR_INVALID_ASSIGNMENT_KEY) {
			w.assignment_uid[0] = 0;
			updateWorkToDoLine (tnum, &w);
		} else if (rc) {
			goto error_exit;
		}
		talked_to_server = TRUE;
	    }
	}

	char	proof_files[50][255];		
	if ((header_words[1] & HEADER_FLAG_QUIT_GIMPS && USE_PRIMENET) ||
	    (IniGetInt (INI_FILE, KEY_QuitGIMPS, 0) && WORKTODO_COUNT == 0 && ProofFileNames (proof_files) == 0)) {
		USE_PRIMENET = 0;
		IniWriteInt (INI_FILE, "UsePrimenet", 0);
		IniWriteString (INI_FILE, KEY_QuitGIMPS, NULL);
		OutputSomewhere (COMM_THREAD_NUM, "Successfully quit GIMPS.\n");
	}

	else if (header_words[1] & HEADER_FLAG_END_DATES) {
		time_t current_time;
		time (&current_time);
		IniSectionWriteInt (INI_FILE, SEC_Internals, KEY_LastEndDatesSent, (long) current_time);
		if (MANUAL_COMM) delete_timed_event (TE_COMPLETION_DATES);
		add_timed_event (TE_COMPLETION_DATES, (int) (DAYS_BETWEEN_CHECKINS * 86400.0));
	}

	gwmutex_lock (&SPOOL_FILE_MUTEX);
	fd = _open (SPOOL_FILE, _O_RDWR | _O_BINARY);
	if (fd < 0) goto locked_leave;
	_lseek (fd, SPOOL_FILE_HEADER_OFFSET, SEEK_SET);
	read_long (fd, &header_words[0], NULL);
	read_long (fd, &header_words[1], NULL);
	if (header_words[0]) {
		if (header_words[1] & HEADER_FLAG_QUIT_GIMPS)
			header_words[0] |= HEADER_FLAG_QUIT_GIMPS;
		header_words[1] = 0;
		_lseek (fd, SPOOL_FILE_HEADER_OFFSET, SEEK_SET);
		write_long (fd, header_words[0], NULL);
		write_long (fd, header_words[1], NULL);
		_close (fd);
		gwmutex_unlock (&SPOOL_FILE_MUTEX);
		goto retry;
	}
	_close (fd);
	_unlink (SPOOL_FILE);

	if (talked_to_server) OutputStr (COMM_THREAD_NUM, "Done communicating with server.\n");
	goto locked_leave;

error_exit:

	if (rc == PRIMENET_ERROR_INVALID_USER) {
		IniSectionGetString (INI_FILE, SEC_Internals, KEY_SrvrUID, USERID, sizeof (USERID), NULL);
		spoolMessage (PRIMENET_UPDATE_COMPUTER_INFO, NULL);
		if (retry_count++ < 10) goto retry;
		OutputStr (COMM_THREAD_NUM, RETRY_EXCEEDED);
	}

	if (rc == PRIMENET_ERROR_CPU_IDENTITY_MISMATCH) {
		generate_computer_guid ();

		spoolMessage (PRIMENET_UPDATE_COMPUTER_INFO, NULL);
		if (retry_count++ < 10) goto retry;
		OutputStr (COMM_THREAD_NUM, RETRY_EXCEEDED);
	}

	if (rc == PRIMENET_ERROR_STALE_CPU_INFO) {
		spoolMessage (PRIMENET_UPDATE_COMPUTER_INFO, NULL);
		if (retry_count++ < 10) goto retry;
		OutputStr (COMM_THREAD_NUM, RETRY_EXCEEDED);
	}

	if (rc == PRIMENET_ERROR_UNREGISTERED_CPU) {
		
		generate_computer_guid ();
		spoolMessage (PRIMENET_UPDATE_COMPUTER_INFO, NULL);
		if (retry_count++ < 10) goto retry;
		OutputStr (COMM_THREAD_NUM, RETRY_EXCEEDED);
	}

	if (rc == PRIMENET_ERROR_CPU_CONFIGURATION_MISMATCH) {
		clearCachedProgramOptions ();
		spoolMessage (PRIMENET_UPDATE_COMPUTER_INFO, NULL);
		if (retry_count++ < 10) goto retry;
		OutputStr (COMM_THREAD_NUM, RETRY_EXCEEDED);
	}

	if (rc == PRIMENET_ERROR_OBSOLETE_CLIENT) {

		OutputStr (MAIN_THREAD_NUM, "Client is obsolete.  Please upgrade.\n");
		obsolete_client = TRUE;
	}

	if (rc == PRIMENET_ERROR_INVALID_WORK_TYPE) {

		spoolMessage (PRIMENET_PROGRAM_OPTIONS, NULL);
		if (retry_count++ < 10) goto retry;
		OutputStr (COMM_THREAD_NUM, RETRY_EXCEEDED);
	}

	OutputStr (COMM_THREAD_NUM, "Visit http://mersenneforum.org for help.\n");

	if (!MANUAL_COMM) {
		unsigned int retry_time;
		retry_time = (rc == PRIMENET_ERROR_MODEM_OFF) ? MODEM_RETRY_TIME : NETWORK_RETRY_TIME;
		sprintf (buf, "Will try contacting server again in %d %s.\n", retry_time, retry_time == 1 ? "minute" : "minutes");
		OutputStr (COMM_THREAD_NUM, buf);
		ChangeIcon (COMM_THREAD_NUM, IDLE_ICON);
		sprintf (buf, "Waiting %d %s", retry_time, retry_time == 1 ? "minute" : "minutes");
		title (COMM_THREAD_NUM, buf);
		add_timed_event (TE_COMM_SERVER, retry_time * 60);
	}

leave:
	gwmutex_lock (&SPOOL_FILE_MUTEX);
locked_leave:
	COMMUNICATION_THREAD = 0;
	gwmutex_unlock (&SPOOL_FILE_MUTEX);
	if (!is_timed_event_active (TE_COMM_SERVER)) {
		ChangeIcon (COMM_THREAD_NUM, IDLE_ICON);
		title (COMM_THREAD_NUM, "Inactive");
	}
}

void salvageCorruptSpoolFile (void)
{
	int	fd;
	char	filename[128];
	char	inbuf[1000];
	struct primenetAssignmentResult pkt;
	char	*in, *out;
	int	i, j, len, pkts_sent;

	OutputBoth (MAIN_THREAD_NUM,
		    "Spool file is corrupt.  Attempting to salvage data.\n");

	strcpy (filename, SPOOL_FILE);
	filename[strlen(filename)-1]++;
	gwmutex_lock (&SPOOL_FILE_MUTEX);
	_unlink (filename);
	rename (SPOOL_FILE, filename);
	gwmutex_unlock (&SPOOL_FILE_MUTEX);

	fd = _open (filename, _O_RDONLY | _O_BINARY);
	if (fd >= 0) {
		memset (&pkt, 0, sizeof (pkt));
		strcpy (pkt.computer_guid, COMPUTER_GUID);
		pkt.result_type = PRIMENET_AR_NO_RESULT;
		pkts_sent = 0;
		strcpy (pkt.message, "RECOVERY: ");
		out = pkt.message + 10;
		for (i = 0; i < 100 && pkts_sent < 100; i++) {
			len = _read (fd, &inbuf, sizeof (inbuf));
			if (len <= 0) break;
			for (j = 0, in = inbuf; j < len; j++, in++) {
				if (! isprint (*in) && *in != '\n') continue;
				*out++ = *in;
				if (*in == '\n');
				else if ((int) (out - pkt.message) < sizeof (pkt.message) - 1) continue;
				else *out++ = '\n';
				*out++ = 0;
				if ((int) (out - pkt.message) > 20) {
					spoolMessage (PRIMENET_ASSIGNMENT_RESULT, &pkt);
					pkts_sent++;
				}
				out = pkt.message + 10;
			}
		}
		_close (fd);
		*out++ = '\n';
		*out++ = 0;
		if ((int) (out - pkt.message) > 20)
			spoolMessage (PRIMENET_ASSIGNMENT_RESULT, &pkt);
	}

	_unlink (filename);
}

gwmutex	TIMED_EVENTS_MUTEX;		
gwevent TIMED_EVENTS_CHANGED;		
					
gwthread TIMED_EVENTS_THREAD = 0;	

struct {
	int	active;			
	time_t	time_to_fire;		
} timed_events[MAX_TIMED_EVENTS];	

void timed_events_scheduler (void *arg);

void init_timed_event_handler (void)
{
	int	i;
	gwmutex_init (&TIMED_EVENTS_MUTEX);
	gwevent_init (&TIMED_EVENTS_CHANGED);
	TIMED_EVENTS_THREAD = 0;
	for (i = 0; i < MAX_TIMED_EVENTS; i++)
		timed_events[i].active = FALSE;
}

void add_timed_event (
	int	event_number,		
	int	time_to_fire)		
{
	time_t	this_time;

	gwmutex_lock (&TIMED_EVENTS_MUTEX);

	time (&this_time);
	if (!timed_events[event_number].active || this_time + time_to_fire < timed_events[event_number].time_to_fire) {
		timed_events[event_number].active = TRUE;
		timed_events[event_number].time_to_fire = this_time + time_to_fire;
	}

	if (!TIMED_EVENTS_THREAD)
		gwthread_create (&TIMED_EVENTS_THREAD, &timed_events_scheduler, NULL);
	else
		gwevent_signal (&TIMED_EVENTS_CHANGED);

	gwmutex_unlock (&TIMED_EVENTS_MUTEX);
}

void delete_timed_event (
	int	event_number)		
{
	gwmutex_lock (&TIMED_EVENTS_MUTEX);
	if (timed_events[event_number].active) {
		timed_events[event_number].active = FALSE;
		gwevent_signal (&TIMED_EVENTS_CHANGED);
	}
	gwmutex_unlock (&TIMED_EVENTS_MUTEX);
}

int is_timed_event_active (
	int	event_number)		
{
	return (timed_events[event_number].active);
}

time_t timed_event_fire_time (
	int	event_number)		
{
	return (timed_events[event_number].time_to_fire);
}

void timed_events_scheduler (void *arg)
{
	time_t	this_time;
	int	i;
	time_t	wake_up_time;
	int	there_are_active_events;

	for ( ; ; ) {

		gwmutex_lock (&TIMED_EVENTS_MUTEX);
		there_are_active_events = FALSE;
		for (i = 0; i < MAX_TIMED_EVENTS; i++) {
			if (!timed_events[i].active) continue;
			if (!there_are_active_events ||
			    wake_up_time > timed_events[i].time_to_fire)
				wake_up_time = timed_events[i].time_to_fire;
			there_are_active_events = TRUE;
		}
		gwevent_reset (&TIMED_EVENTS_CHANGED);
		gwmutex_unlock (&TIMED_EVENTS_MUTEX);

		time (&this_time);
		if (!there_are_active_events) wake_up_time = this_time + 1000000;
		if (wake_up_time > this_time) {
			int	rc;
			rc = gwevent_wait (&TIMED_EVENTS_CHANGED,
					   (int) (wake_up_time - this_time));
			if (rc != GWEVENT_TIMED_OUT) continue;
		}

		time (&this_time);
		for (i = 0; i < MAX_TIMED_EVENTS; i++) {
			int	fire;
			float	cert_freq;
			gwmutex_lock (&TIMED_EVENTS_MUTEX);
			fire = (timed_events[i].active && this_time >= timed_events[i].time_to_fire);
			gwmutex_unlock (&TIMED_EVENTS_MUTEX);
			if (!fire) continue;
			switch (i) {
			case TE_MEM_CHANGE:	
				timed_events[i].active = FALSE;
				mem_settings_have_changed ();
				break;
			case TE_PAUSE_WHILE:	
				timed_events[i].active = FALSE;
				checkPauseWhileRunning ();
				break;
			case TE_WORK_QUEUE_CHECK:	
				
				cert_freq = (float) (HW_NUM_CORES >= 20 ? 3.0 :		
						     HW_NUM_CORES >= 12 ? 4.0 :		
						     HW_NUM_CORES >= 7 ? 6.0 :		
						     HW_NUM_CORES >= 3 ? 8.0 :		
						     HW_NUM_CORES >= 2 ? 12.0 : 24.0);	
				
				if (IniGetInt (INI_FILE, "CertDailyCPULimit", 10) >= 50) cert_freq = 0.5;
				
				cert_freq = IniGetFloat (INI_FILE, "CertGetFrequency", cert_freq);
				if (cert_freq < 0.25) cert_freq = 0.25;
				
				if (!IniGetInt (INI_FILE, "CertWork", 1)) cert_freq = 8.0;
				
				timed_events[i].time_to_fire = this_time + (int) (cert_freq * 3600.0);
				spoolMessage (MSG_CHECK_WORK_QUEUE, NULL);
				break;
			case TE_COMM_SERVER:	
				timed_events[i].active = FALSE;
				gwthread_create (&COMMUNICATION_THREAD, &communicateWithServer, NULL);
				break;
			case TE_COMM_KILL:	
				timed_events[i].active = FALSE;
				GLOBAL_SEND_MSG_COUNT++;
				if (LOCKED_WORK_UNIT != NULL)
					decrementWorkUnitUseCount (LOCKED_WORK_UNIT, SHORT_TERM_USE);

				break;
			case TE_PRIORITY_WORK:	
				timed_events[i].time_to_fire = this_time + TE_PRIORITY_WORK_FREQ;
				check_for_priority_work ();
				break;
			case TE_COMPLETION_DATES:	
				timed_events[i].active = FALSE;
				UpdateEndDates ();
				break;
			case TE_THROTTLE:	
				timed_events[i].time_to_fire = this_time + handleThrottleTimerEvent ();
				break;
			case TE_SAVE_FILES:	
						
				timed_events[i].active = FALSE;
				if (addFileExists ()) stop_workers_for_add_files ();	
				incorporateWorkToDoAddFile ();				
				saveFilesTimer ();
				break;
			case TE_BATTERY_CHECK:	
				timed_events[i].time_to_fire = this_time + TE_BATTERY_CHECK_FREQ;
				test_battery ();
				break;
			case TE_ROLLING_AVERAGE: 
				timed_events[i].time_to_fire = this_time + TE_ROLLING_AVERAGE_FREQ;
				adjust_rolling_average ();
				break;
			case TE_READ_PAUSE_DATA: 
				timed_events[i].active = FALSE;
				read_pause_info ();
				break;
			case TE_READ_INI_FILE: 
				timed_events[i].active = FALSE;
				stop_workers_for_reread_ini ();
				break;
			case TE_LOAD_AVERAGE:	
				timed_events[i].active = FALSE;
				checkLoadAverage ();
				break;
			case TE_BENCH:		
				timed_events[i].time_to_fire = this_time + TE_BENCH_FREQ;
				autoBench ();
				break;
			case TE_JACOBI:		
				timed_events[i].active = FALSE;
				JacobiTimer ();
				break;
			}
		}

	}
}

int inUploadWindow (int *minutes_to_start)
{
	char	timebuf[80];
	unsigned int start_time, end_time, current_time;	
	time_t	current_t;
	struct tm *x;

	IniSectionGetString (INI_FILE, SEC_PrimeNet, KEY_UploadStartTime, timebuf, sizeof (timebuf), "00:00");
	start_time = strToMinutes (timebuf);
	IniSectionGetString (INI_FILE, SEC_PrimeNet, KEY_UploadEndTime, timebuf, sizeof (timebuf), "24:00");
	end_time = strToMinutes (timebuf);

	if ((start_time <= end_time && start_time + 120 > end_time) ||
	    (start_time > end_time && start_time + 120 > end_time + 1440))
		end_time = (start_time + 120) % 1440;

	time (&current_t);
	x = localtime (&current_t);
	current_time = x->tm_hour * 60 + x->tm_min;

	if (minutes_to_start != NULL)
		*minutes_to_start = (1440 + start_time - current_time) % 1440;

	if (start_time <= end_time) return (current_time >= start_time && current_time <= end_time);
	return (current_time >= start_time || current_time <= end_time);
}

void proofUploader (void *arg)
{
	char	proof_files[50][255];		
	int	i, minutes_to_wait, minutes_to_start, num_proof_files;

	gwevent_init (&PROOF_UPLOAD_EVENT);

	for ( ; ; gwevent_wait (&PROOF_UPLOAD_EVENT, minutes_to_wait * 60)) {

		gwevent_reset (&PROOF_UPLOAD_EVENT);
		minutes_to_wait = 60;

		if (!USE_PRIMENET) continue;

		if (!inUploadWindow (&minutes_to_start)) {
			ASSERTG (minutes_to_start >= 0);
			if (minutes_to_start < 60) minutes_to_wait = minutes_to_start;
			continue;
		}

		num_proof_files = ProofFileNames (proof_files);
		if (num_proof_files < 0) {
			OutputBoth (COMM_THREAD_NUM, "Unable to get list of proof files in the current directory\n");
			num_proof_files = 0;
		}

		if (num_proof_files) Sleep (60 * 1000);		

		for (i = 0; i < num_proof_files && inUploadWindow (NULL); i++) {
			ProofUpload (proof_files[i]);
		}
	}
}

