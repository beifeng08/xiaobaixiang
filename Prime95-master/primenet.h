// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

#define	PRIMENET_VERSION			5
#define	PRIMENET_TRANSACTION_API_VERSION	0.95

void LoadPrimenet ();

int PRIMENET (short operation, void *pkt);

#define PRIMENET_UPDATE_COMPUTER_INFO	100
#define PRIMENET_PROGRAM_OPTIONS	101
#define PRIMENET_GET_ASSIGNMENT		102
#define PRIMENET_REGISTER_ASSIGNMENT	103
#define PRIMENET_ASSIGNMENT_PROGRESS	104
#define PRIMENET_ASSIGNMENT_RESULT	105
#define PRIMENET_ASSIGNMENT_UNRESERVE	106
#define PRIMENET_BENCHMARK_DATA		107
#define PRIMENET_PING_SERVER		108

struct primenetUpdateComputerInfo {
	int32_t	versionNumber;
	char	computer_guid[33];
	char	hardware_guid[33];
	char	windows_guid[33];
	char	application[65];	
	char	cpu_model[65];		
	char	cpu_features[65];	
	int32_t	L1_cache_size;		
	int32_t	L2_cache_size;		
	int32_t	L3_cache_size;		
	uint32_t num_cpus;		
	uint32_t num_hyperthread;	
	uint32_t mem_installed;		
	uint32_t cpu_speed;		
	uint32_t hours_per_day;		
	uint32_t rolling_average;	
					
	char	user_id[21];
	char	computer_name[21];

	char	user_name[33];
	char	pad[1];			
	uint32_t options_counter;	
					
};

#define PRIMENET_WP_WHATEVER		0	
#define PRIMENET_WP_FACTOR_LMH		1	
#define PRIMENET_WP_FACTOR		2	
#define PRIMENET_WP_PMINUS1		3	
#define PRIMENET_WP_PFACTOR		4	
#define PRIMENET_WP_ECM_SMALL		5	
#define PRIMENET_WP_ECM_FERMAT		6	
#define PRIMENET_WP_ECM_CUNNINGHAM	7	
#define PRIMENET_WP_ECM_COFACTOR	8	
#define PRIMENET_WP_LL_FIRST		100	
#define PRIMENET_WP_LL_DBLCHK		101	
#define PRIMENET_WP_LL_WORLD_RECORD	102	
#define PRIMENET_WP_LL_100M		104	
#define PRIMENET_WP_PRP_FIRST		150	
#define PRIMENET_WP_PRP_DBLCHK		151	
#define PRIMENET_WP_PRP_WORLD_RECORD	152	
#define PRIMENET_WP_PRP_100M		153	
#define PRIMENET_WP_PRP_NO_PMINUS1	154	
#define PRIMENET_WP_PRP_DC_PROOF	155	
#define PRIMENET_WP_PRP_COFACTOR	160	
#define PRIMENET_WP_PRP_COFACTOR_DBLCHK	161	

#define PRIMENET_WP_LL_10M		103	
#define PRIMENET_WP_LL_FIRST_NOFAC	105	

struct primenetProgramOptions {
	int32_t	versionNumber;
	char	computer_guid[33];
	char	pad[3];
	int32_t cpu_num;		

	int32_t	num_workers;
	int32_t work_preference;	
	int32_t priority;
	int32_t daysOfWork;
	int32_t dayMemory;
	int32_t nightMemory;
	int32_t dayStartTime;
	int32_t nightStartTime;
	int32_t runOnBattery;

	uint32_t options_counter;	
					
};

#define PRIMENET_WORK_TYPE_FACTOR	2
#define PRIMENET_WORK_TYPE_PMINUS1	3
#define PRIMENET_WORK_TYPE_PFACTOR	4
#define PRIMENET_WORK_TYPE_ECM		5
#define PRIMENET_WORK_TYPE_PPLUS1	6		
#define PRIMENET_WORK_TYPE_FIRST_LL	100
#define PRIMENET_WORK_TYPE_DBLCHK	101
#define PRIMENET_WORK_TYPE_PRP		150
#define PRIMENET_WORK_TYPE_CERT		200

struct primenetGetAssignment {
	int32_t	versionNumber;
	char	computer_guid[33];
	char	pad[3];
	uint32_t cpu_num;		
	int	get_cert_work;		
					
	float	temp_disk_space;	
					
	uint32_t min_exp;		
	uint32_t max_exp;		

	char	assignment_uid[33];
	char	pad2[7];
	uint32_t work_type;
	double	k;			
	uint32_t b;			
	uint32_t n;			
	int32_t	c;			
	uint32_t has_been_pminus1ed;	
	double	how_far_factored;	
	double	factor_to;		
	uint64_t B1;			
	uint64_t B2;			
	double	tests_saved;		
	uint32_t curves;		
	uint32_t prp_base;		
	uint32_t prp_residue_type;	
	uint32_t prp_dblchk;		
	uint32_t num_squarings;		
	uint32_t nth_run;		
	char	known_factors[2000];	
};

struct primenetRegisterAssignment {
	int32_t	versionNumber;
	char	computer_guid[33];
	char	pad[3];
	uint32_t cpu_num;		
	uint32_t work_type;
	double	k;			
	uint32_t b;			
	uint32_t n;			
	int32_t	c;			
	uint32_t has_been_pminus1ed;	
	double	how_far_factored;	
					
	double	factor_to;		
					
	uint64_t B1;			
	uint64_t B2;			
	double	tests_saved;		
					
	uint32_t curves;		

	char	assignment_uid[33];
	char	pad2[3];
};

struct primenetAssignmentProgress {
	int32_t	versionNumber;
	char	computer_guid[33];
	char	assignment_uid[33];
	char	stage[11];		
	char	pad[3];
	uint32_t cpu_num;		
	double	pct_complete;		
	uint32_t end_date;		
	uint32_t next_update;		
	uint32_t fftlen;		
	uint32_t iteration;		
	char	residue[17];		
	char	error_count[9];		
};

#define PRIMENET_AR_NO_RESULT	0	
#define PRIMENET_AR_TF_FACTOR	1	
#define PRIMENET_AR_P1_FACTOR	2	
#define PRIMENET_AR_ECM_FACTOR	3	
#define PRIMENET_AR_TF_NOFACTOR	4	
#define PRIMENET_AR_P1_NOFACTOR	5	
#define PRIMENET_AR_ECM_NOFACTOR 6	
#define PRIMENET_AR_PP1_FACTOR	7	
#define PRIMENET_AR_PP1_NOFACTOR 8	
#define PRIMENET_AR_LL_RESULT	100	
#define PRIMENET_AR_LL_PRIME	101	
#define PRIMENET_AR_PRP_RESULT	150	
#define PRIMENET_AR_PRP_PRIME	151	
#define PRIMENET_AR_CERT	200	

#define	PRIMENET_PRP_TYPE_FERMAT	1	
#define	PRIMENET_PRP_TYPE_SPRP		2	
#define	PRIMENET_PRP_TYPE_FERMAT_VAR	3	
#define	PRIMENET_PRP_TYPE_SPRP_VAR	4	
#define	PRIMENET_PRP_TYPE_COFACTOR	5	

struct primenetAssignmentResult {
	int32_t	versionNumber;
	char	computer_guid[33];
	char	assignment_uid[33];
	char	message[201];		
	char	pad[1];
	uint32_t done;			
	uint32_t result_type;		
	double	k;			
	uint32_t b;			
	uint32_t n;			
	int32_t	c;			
	uint32_t curves;		
	double	start_bits;		
	double	end_bits;		
	uint64_t B1;			
	uint64_t B2;			
	uint32_t stage;			
	uint32_t shift_count;		
	uint32_t fftlen;		
	char	residue[17];		
	char	error_count[9];		
	char	factor[201];		
	char	cert_hash[65];		
	char	proof_hash[33];		
	char	pad2[3];
	uint32_t num_known_factors;	
	uint32_t gerbicz;		
	uint32_t prp_base;		
	uint32_t prp_residue_type;	
	uint32_t proof_power;		
	char	JSONmessage[2000];	
	uint32_t pp1_numerator;		
	uint32_t pp1_denominator;	

};

struct primenetAssignmentUnreserve {
	int32_t	versionNumber;
	char	computer_guid[33];
	char	assignment_uid[33];
	char	pad[2];
};

#define PRIMENET_BENCH_MAX_DATAPOINTS	50

struct primenetBenchmarkData {
	int32_t	versionNumber;
	char	computer_guid[33];
	char	user_comment[201];	
	char	pad[6];
	uint32_t num_data_points;
	struct {
		char bench[13];		
		char pad2[3];		
		double timing;		 
	} data_points[PRIMENET_BENCH_MAX_DATAPOINTS];

};

struct primenetPingServer {
	int32_t	versionNumber;
	uint32_t ping_type;

	char	ping_response[513];
	char	pad[3];
};

#define PRIMENET_ERROR_OK		0	
#define PRIMENET_ERROR_SERVER_BUSY	3	
#define PRIMENET_ERROR_INVALID_VERSION	4
#define PRIMENET_ERROR_INVALID_TRANSACTION 5
#define PRIMENET_ERROR_INVALID_PARAMETER 7
#define PRIMENET_ERROR_ACCESS_DENIED	9
#define PRIMENET_ERROR_DATABASE_CORRUPT	11
#define PRIMENET_ERROR_DATABASE_FULL_OR_BROKEN	13
#define PRIMENET_ERROR_INVALID_USER	21
#define PRIMENET_ERROR_UNREGISTERED_CPU	30
#define PRIMENET_ERROR_OBSOLETE_CLIENT	31
#define PRIMENET_ERROR_STALE_CPU_INFO	32
#define PRIMENET_ERROR_CPU_IDENTITY_MISMATCH 33
#define PRIMENET_ERROR_CPU_CONFIGURATION_MISMATCH 34
#define PRIMENET_ERROR_NO_ASSIGNMENT	40
#define PRIMENET_ERROR_INVALID_ASSIGNMENT_KEY 43
#define PRIMENET_ERROR_INVALID_ASSIGNMENT_TYPE 44
#define PRIMENET_ERROR_INVALID_RESULT_TYPE 45
#define PRIMENET_ERROR_INVALID_WORK_TYPE 46
#define PRIMENET_ERROR_WORK_NO_LONGER_NEEDED 47

#define PRIMENET_NO_ERROR		0
#define PRIMENET_FIRST_INTERNAL_ERROR	1000

#define PRIMENET_ERROR_CONNECT_FAILED	1000
#define PRIMENET_ERROR_SEND_FAILED	1001
#define PRIMENET_ERROR_RECV_FAILED	1002
#define PRIMENET_ERROR_SERVER_UNSPEC	1003
#define PRIMENET_ERROR_PNERRORRESULT	1004
#define PRIMENET_ERROR_PNERRORDETAIL	1005

#define PRIMENET_ERROR_CURL_INIT	1100
#define PRIMENET_ERROR_CURL_PERFORM	1101

#define PRIMENET_ERROR_MODEM_OFF	2000

