// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

#ifndef _COMMONC_H
#define _COMMONC_H

#ifdef __cplusplus
extern "C" {
#endif

#define VERSION		"30.19"
#define BUILD_NUM	"20"

#define MIN_PRIME	5L		
#define MAX_FACTOR	1500000000	
#define MAX_FACTOR_BITS	87		
#define ERROR_RATE	0.018		
#define PRP_ERROR_RATE	0.0001		

#define MAX_NUM_WORKERS 1024		

#define FAC82	1071000000L
#define FAC81	842000000L
#define FAC80	662000000L
#define FAC79	516800000L
#define FAC78	408400000L
#define FAC77	322100000L
#define FAC76	253500000L
#define FAC75	199500000L
#define FAC74	153400000L
#define FAC73	120000000L
#define FAC72	96830000L
#define FAC71	77910000L
#define FAC70	60940000L
#define FAC69	48800000L
#define FAC68	38300000L
#define FAC67	29690000L	

#define FAC66	23390000L

#define FAC65	13380000L
#define FAC64	8250000L
#define FAC63	6515000L
#define FAC62	5160000L
#define FAC61	3960000L
#define FAC60	2950000L
#define FAC59	2360000L
#define FAC58	1930000L
#define FAC57	1480000L
#define FAC56	1000000L

extern int USE_V4;
extern char V4_USERID[15];
extern char V4_USERPWD[9];
extern char V4_USERNAME[80];

extern char INI_FILE[260];		
extern char WORKTODO_FILE[260];		
extern char RESFILE[260];		
extern char RESFILEBENCH[260];		
extern char SPOOL_FILE[260];		
extern char LOGFILE[260];		
extern int NO_GUI;			

extern char USERID[21];			
extern char COMPID[21];			
extern char COMPUTER_GUID[33];		
extern int USE_PRIMENET;		
extern int DIAL_UP;			
					
extern unsigned int NUM_WORKERS;	
extern unsigned int WORK_PREFERENCE[MAX_NUM_WORKERS];
					
extern unsigned int CORES_PER_TEST[MAX_NUM_WORKERS];
					
extern int HYPERTHREAD_TF;		
extern int HYPERTHREAD_LL;		
extern unsigned int DAYS_OF_WORK;	
					
extern int STRESS_TESTER;		
extern int volatile ERRCHK;		
extern unsigned int PRIORITY;		
extern int MANUAL_COMM;			
					
extern float volatile CPU_WORKER_DISK_SPACE; 
extern unsigned int volatile CPU_HOURS;	
extern int CLASSIC_OUTPUT;		
extern int OUTPUT_ROUNDOFF;		
extern unsigned long volatile ITER_OUTPUT;
extern unsigned long volatile ITER_OUTPUT_RES;
extern unsigned long volatile DISK_WRITE_TIME;
					
extern unsigned long volatile JACOBI_TIME; 
extern unsigned int MODEM_RETRY_TIME;	
					
extern unsigned int NETWORK_RETRY_TIME;	
					
extern float DAYS_BETWEEN_CHECKINS;	
					
extern int NUM_BACKUP_FILES;		
extern int NUM_JACOBI_BACKUP_FILES;	
extern int SILENT_VICTORY;		
extern int SILENT_VICTORY_PRP;		
extern int RUN_ON_BATTERY;		
extern int BATTERY_PERCENT;		
extern int DEFEAT_POWER_SAVE;		
extern int TRAY_ICON;			
extern int HIDE_ICON;			
extern int MERGE_WINDOWS;		
					
#define MERGE_MAIN_WINDOW	0x1	
#define MERGE_COMM_WINDOW	0x2	
#define MERGE_WORKER_WINDOWS	0x4	
#define MERGE_MAINCOMM_WINDOWS	0x8	
#define MERGE_NO_PREFIX		0x20	

extern double UNOFFICIAL_CPU_SPEED;	
					
extern unsigned int ROLLING_AVERAGE;	
					
extern unsigned int PRECISION;		
					
extern int RDTSC_TIMING;		
extern int TIMESTAMPING;		
extern int CUMULATIVE_TIMING;		
extern int CUMULATIVE_ROUNDOFF;		
extern int SEQUENTIAL_WORK;		
					
extern int WELL_BEHAVED_WORK;		
					
extern unsigned long INTERIM_FILES;	  
extern unsigned long INTERIM_RESIDUES;	
extern unsigned long HYPERTHREADING_BACKOFF; 
					
extern int THROTTLE_PCT;		

extern int STARTUP_IN_PROGRESS;		

extern int LAUNCH_TYPE;			
extern unsigned int WORKERS_ACTIVE;
extern int WORKERS_STOPPING;	

extern unsigned int WORKTODO_COUNT;	

extern int GIMPS_QUIT;			

extern gwthread COMMUNICATION_THREAD;	
extern gwthread UPLOAD_THREAD;		

extern gwevent AUTOBENCH_EVENT;		
extern gwevent PROOF_UPLOAD_EVENT;	

extern hwloc_topology_t hwloc_topology;	
extern uint32_t CPU_TOTAL_L1_CACHE_SIZE;
extern uint32_t CPU_TOTAL_L2_CACHE_SIZE;
extern uint32_t CPU_TOTAL_L3_CACHE_SIZE;
extern uint32_t CPU_TOTAL_L4_CACHE_SIZE;
extern uint32_t CPU_NUM_L1_CACHES;	
extern uint32_t CPU_NUM_L2_CACHES;	
extern uint32_t CPU_NUM_L3_CACHES;	
extern uint32_t CPU_NUM_L4_CACHES;	
extern int	CPU_L2_CACHE_INCLUSIVE;	
extern int	CPU_L3_CACHE_INCLUSIVE;	
extern int	CPU_L4_CACHE_INCLUSIVE;	
extern int OS_CAN_SET_AFFINITY;		
void topology_print_children (hwloc_obj_t obj, int);

extern uint32_t HW_NUM_CORES;		
extern uint32_t HW_NUM_THREADS;		
extern uint32_t HW_NUM_COMPUTE_CORES;	
extern uint32_t HW_NUM_THREADING_NODES;	
extern uint32_t HW_NUM_COMPUTE_THREADING_NODES;	
extern uint32_t HW_NUM_NUMA_NODES;	

struct hw_core_info {
	uint16_t num_threads;		
	uint16_t ranking;		
};
extern struct hw_core_info *HW_CORES;	

extern const char KEY_NumWorkers[];
extern const char KEY_QuitGIMPS[];

extern const char SEC_Internals[];
extern const char KEY_V30OptionsConverted[];
extern const char KEY_OldCpuSpeed[];
extern const char KEY_NewCpuSpeed[];
extern const char KEY_NewCpuSpeedCount[];
extern const char KEY_RollingAverage[];
extern const char KEY_RollingHash[];
extern const char KEY_RollingStartTime[];
extern const char KEY_RollingCompleteTime[];
extern const char KEY_CertDailyMBRemaining[];
extern const char KEY_CertDailyCPURemaining[];
extern const char KEY_CertDailyRemainingLastUpdate[];
extern const char KEY_Pid[];
extern const char KEY_SrvrUID[];
extern const char KEY_SrvrComputerName[];
extern const char KEY_SrvrP00[];
extern const char KEY_SrvrPO1[];
extern const char KEY_SrvrPO2[];
extern const char KEY_SrvrPO3[];
extern const char KEY_SrvrPO4[];
extern const char KEY_SrvrPO5[];
extern const char KEY_SrvrPO6[];
extern const char KEY_SrvrPO7[];
extern const char KEY_SrvrPO8[];
extern const char KEY_SrvrPO9[];
extern const char KEY_LastEndDatesSent[];
extern const char KEY_WGUID_version[];
extern const char KEY_CertErrorCount[];

extern const char SEC_PrimeNet[];
extern const char KEY_DialUp[];
extern const char KEY_ProxyHost[];
extern const char KEY_ProxyUser[];
extern const char KEY_ProxyPass[];
extern const char KEY_ProxyMask[];
extern const char KEY_Debug[];
extern const char KEY_UploadRateLimit[];
extern const char KEY_UploadStartTime[];
extern const char KEY_UploadEndTime[];
extern const char KEY_DownloadDailyLimit[];
extern const char KEY_DownloadRateLimit[];
extern const char KEY_ProofUploads[];
extern const char KEY_ProofHashLength[];
extern const char KEY_ProofPower[];
extern const char KEY_ProofPowerMult[];
extern const char KEY_UploadChunkSize[];

extern const char SEC_Windows[];
extern const char KEY_MergeWindows[];
extern const char KEY_TrayIcon[];
extern const char KEY_HideIcon[];
extern const char KEY_Left[];
extern const char KEY_Right[];
extern const char KEY_Top[];
extern const char KEY_Bottom[];
extern const char KEY_ExitOnX[];

void generate_application_string (char *);
void getCpuInfo (void);
void getCpuDescription (char *, int);

unsigned int countCommas (const char *);

int isPrime (unsigned long p);
int start_sieve (int thread_num, uint64_t start, void **returned_si);		
int start_sieve_with_limit (int thread_num, uint64_t start, uint32_t max_elimination_factor, void **returned_si);
uint64_t sieve (void *si);
void end_sieve (void *si);
uint64_t _intgcd (uint64_t, uint64_t);
uint64_t modinv (uint64_t, uint64_t);
int relatively_prime (uint64_t, uint64_t);

void sorted_add_unique (int *, int *, int);
int is_number_in_list (int, const char *);

unsigned int strToMinutes (const char *);
void minutesToStr (unsigned int, char *);
void write_memory_settings (unsigned int, unsigned int, unsigned int, unsigned int);
int read_memory_settings (unsigned int *, unsigned int *, unsigned int *, unsigned int *);

void nameAndReadIniFiles (int named_ini_files);
void initCommCode (void);
int readIniFiles (void);

void processTimedIniFile (const char *);

void incorporateIniAddFiles (void);
int incorporateWorkToDoAddFile (void);

void PTOGetAll (const char *ini_filename, const char *keyword, unsigned int *array,
		unsigned int def_val);
void PTOSetAll (const char *ini_filename, const char *keyword, const char *shadow_keyword,
		unsigned int *array, unsigned int new_val);
void PTOSetOne (const char *ini_filename, const char *keyword, const char *shadow_keyword,
		unsigned int *array, int tnum, unsigned int new_val);
int PTOIsGlobalOption (unsigned int *array);
int PTOHasOptionChanged (const char *shadow_keyword, unsigned int *array, int tnum);

#define MAIN_THREAD_NUM		-2
#define COMM_THREAD_NUM		-1
void create_window (int thread_num);
void destroy_window (int thread_num);
void TileViews (void);
void base_title (int, const char *);
void title (int, const char *);
#define	WORKING_ICON	0
#define	IDLE_ICON	1
void ChangeIcon (int, int);
void BlinkIcon (int, int);
EXTERNC void OutputBoth (int, const char *);
void OutputBothBench (int, const char *);
void OutputBothErrno (int);
void OutputStr (int, const char *);
void OutputStrNoTimeStamp (int, const char *);
void RealOutputStr (int, const char *);
void OutputSomewhere (int, const char *);
void LogMsg (const char *);
int OutOfMemory (int);

#define WORK_FACTOR		0
#define WORK_TEST		1
#define WORK_DBLCHK		2
#define WORK_ADVANCEDTEST	3
#define WORK_ECM		4
#define WORK_PMINUS1		5
#define WORK_PPLUS1		6
#define WORK_PFACTOR		7
#define WORK_PRP		10
#define WORK_CERT		11
#define WORK_NONE		100	
#define WORK_DELETED		101	

struct work_unit {		
	int	work_type;	
	char	assignment_uid[33]; 
	char	extension[9];	
	double	k;		
	unsigned long b;	
	unsigned long n;	
	signed long c;		
	unsigned long minimum_fftlen;
	double	sieve_depth;	
	double	factor_to;	
	int	pminus1ed;	
	uint64_t B1;		
	uint64_t B2;		
	uint64_t B2_start;	
	int	nth_run;	
	unsigned int skip_curves; 
	unsigned int curves_to_do; 
	uint64_t curve;		
	double	tests_saved;	
	unsigned int prp_base;		
	int	prp_residue_type; 
	int	prp_dblchk;	
	int	cert_squarings; 
	char	*gmp_ecm_file;	
	char	*known_factors;	
	char	*comment;	
		
	struct work_unit *next; 
	struct work_unit *prev; 
	int	in_use_count;	
	int	high_memory_usage;
				
	char	stage[10];	
	double	pct_complete;	
	unsigned long fftlen;	
	int	ra_failed;	
};
struct work_unit_array {	
	struct work_unit *first; 
	struct work_unit *last;	
};

int readWorkToDoFile (void);
int writeWorkToDoFile (int);
#define SHORT_TERM_USE		0
#define LONG_TERM_USE		1
struct work_unit *getNextWorkToDoLine (int, struct work_unit *, int);
void decrementWorkUnitUseCount (struct work_unit *, int);
#define ADD_TO_FRONT		1
#define ADD_TO_LOGICAL_END	2		
#define ADD_TO_END		3
#define ADD_BEFORE_SPECIFIC	4		
#define ADD_AFTER_SPECIFIC	5		
int addWorkToDoLine (int, struct work_unit *, int);
int updateWorkToDoLine (int, struct work_unit *);
int deleteWorkToDoLine (int, struct work_unit *, int);
int isWorkUnitActive (struct work_unit *);
int addToWorkUnitArray (unsigned int, struct work_unit *, int);

void rolling_average_work_unit_complete (int, struct work_unit *);
void invalidateNextRollingAverageUpdate (void);

double work_estimate (int thread_num, struct work_unit *);
unsigned int factorLimit (struct work_unit *);
void guess_pminus1_bounds (int, double, unsigned long, unsigned long, signed long, double, double, uint64_t *, uint64_t *, uint64_t *, double *);

void strupper (char *);
int isHex (const char *);
void tempFileName (struct work_unit *, char *);
int fileExists (const char *);
void DirPlusFilename (char *, const char *);

int read_array (int fd, char *buf, size_t len, uint32_t *sum);
int write_array (int fd, const char *buf, size_t len, uint32_t *sum);
int read_giant (int fd, giant g, uint32_t *sum);
int write_giant (int fd, giant g, uint32_t *sum);
int read_gwnum (int fd, gwhandle *gwdata, gwnum g, uint32_t *sum);
int write_gwnum (int fd, gwhandle *gwdata, gwnum g, uint32_t *sum);
int read_double (int fd, double *val, uint32_t *sum);
int write_double (int fd, double dbl, uint32_t *sum);
int read_int32 (int fd, int32_t *val, uint32_t *sum);
int write_int32 (int fd, int32_t val, uint32_t *sum);
int read_uint32 (int fd, uint32_t *val, uint32_t *sum);
int write_uint32 (int fd, uint32_t val, uint32_t *sum);
int read_int64 (int fd, int64_t *val, uint32_t *sum);
int write_int64 (int fd, int64_t val, uint32_t *sum);
int read_uint64 (int fd, uint64_t *val, uint32_t *sum);
int write_uint64 (int fd, uint64_t val, uint32_t *sum);
int read_magicnum (int fd, uint32_t magicnum);
int read_header (int fd, uint32_t *version, struct work_unit *w, uint32_t *sum);
int write_header (int fd, uint32_t magicnum, uint32_t version, struct work_unit *w);
int read_checksum (int fd, uint32_t *sum);
int write_checksum (int fd, uint32_t sum);

int read_long (int fd, unsigned long *val, uint32_t *sum);
#define write_long(fd,val,sum)	write_uint32 (fd, (uint32_t) (val), sum)
int read_int (int fd, int *val, uint32_t *sum);
#define write_int(fd,val,sum)	write_int32 (fd, (int32_t) (val), sum)
int read_slong (int fd, long *val, uint32_t *sum);
#define write_slong(fd,val,sum)	write_int32 (fd, (int32_t) (val), sum)

void formatMsgForResultsFile (char *, struct work_unit *);
int writeResults (const char *);
int writeResultsBench (const char *);
int writeResultsJSON (const char *);
void JSONaddExponent (char *JSONbuf, struct work_unit *w);
void JSONaddExponentKnownFactors (char *JSONbuf, struct work_unit *w);
void JSONaddProgramTimestamp (char *JSONbuf);
void JSONaddUserComputerAID (char *JSONbuf, struct work_unit *w);

unsigned long physical_memory (void);
unsigned long GetSuggestedMemory (unsigned long nDesiredMemory);
int getDefaultTimeFormat (void);

void init_spool_file_and_comm_code (void);
void set_comm_timers (void);
void clear_comm_rate_limits (void);
void do_manual_comm_now (void);
void pingServer (void);
void UpdateEndDates (void);
void ConditionallyUpdateEndDates (void);
#define MSG_CHECK_WORK_QUEUE		998
#define MSG_QUIT_GIMPS			999
void spoolMessage (short, void *);
void spoolExistingResultsFile (void);
int unreserve (unsigned long);
void salvageCorruptSpoolFile (void);
void proofUploader (void *);

int LoadPrimeNet (void);
void UnloadPrimeNet (void);
int PRIMENET (short, void *);
int ProofFileNames (char filenames[50][255]);
void ProofUpload (char *);
int ProofGetData (char *, void *, int, char *);
char getDirectorySeparator ();
void mallocFreeForOS ();

#define TE_MEM_CHANGE		0	
#define TE_PAUSE_WHILE		1	
#define TE_WORK_QUEUE_CHECK	2	
#define TE_COMM_SERVER		3	
#define TE_COMM_KILL		4	
#define TE_PRIORITY_WORK	5	
#define TE_COMPLETION_DATES	6	
#define TE_THROTTLE		7	
#define TE_SAVE_FILES		8	
#define TE_BATTERY_CHECK	9	
#define TE_ROLLING_AVERAGE	10	
#define TE_READ_PAUSE_DATA	11	
#define TE_READ_INI_FILE	12	
#define TE_LOAD_AVERAGE		13	
#define TE_BENCH		14	
#define TE_JACOBI		15	

#define MAX_TIMED_EVENTS	16	

void init_timed_event_handler (void);

void add_timed_event (
	int	event_number,		
	int	time_to_fire);		

void delete_timed_event (
	int	event_number);		

int is_timed_event_active (
	int	event_number);		

time_t timed_event_fire_time (
	int	event_number);		

#define TE_PRIORITY_WORK_FREQ	 1*60*60 
#define TE_BATTERY_CHECK_FREQ	 15	
#define TE_THROTTLE_FREQ	 5	
#define TE_ROLLING_AVERAGE_FREQ	 12*60*60 
#define TE_BENCH_FREQ		 21*60*60 

#ifdef __cplusplus
}
#endif

#endif
