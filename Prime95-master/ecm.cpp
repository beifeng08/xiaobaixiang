// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

#include "common.h"		
#include "commonb.h"
#include "commonc.h"
#include "ecm.h"
#include "exponentiate.h"
#include "pair.h"
#include "pm1prob.h"
#include "polymult.h"
#include "primenet.h"
#include <iterator>
#include <map>

#include "resume_gmp.c"

int	QA_IN_PROGRESS = FALSE;
int	QA_TYPE = 0;
giant	QA_FACTOR = NULL;
int	PRAC_SEARCH = 7;

namespace local {
template< class Iterator >
std::reverse_iterator<Iterator> make_reverse_iterator(Iterator i)
{
    return std::reverse_iterator<Iterator>(i);
}
}

struct ws {
	gwnum	x;		
	gwnum	y;		
	gwnum	z;		
};

struct ed {
	gwnum	x;		
	gwnum	y;		
	gwnum	z;		
	gwnum	t;		
	bool	alternate_format; 
};

struct xz {
	gwnum	x;		
	gwnum	z;		
};

#ifdef BETTER_RELP_SETS
int16_t relp_set_hard4[]  = {4,1,1,  0,1,4,10  ,2,7};			
int16_t relp_set_hard4a[] = {4,0,1,  0,1,3,9   ,6};			

int16_t relp_set_hard5[]  = {5,1,1,  0,1,4,9,36   ,2,18};		

int16_t relp_set_hard5e[] = {5,0,1,  0,1,3,7,28   ,14};			
int16_t relp_set_hard6[]  = {6,2,1,  0,1,3,9,21,84  ,6,12,42};		

int16_t relp_set_hard6c[] = {6,1,1,  0,1,3,7,21,63  ,14,42};		
int16_t relp_set_hard6d[] = {6,0,1,  0,1,3,7,15,63  ,31};		
int16_t relp_set_hard7[]  = {7,4,1,  0,1,3,9,21,84,49   ,6,7,14,42,28};	

int16_t relp_set_hard7c[] = {7,0,1,  0,1,3,7,15,31,127  ,63};		

#endif

#define MAX_RELP_SETS	12

int16_t relp_set_easy2[]  = {2,0,0,  0,-1};								
int16_t relp_set_easy3[]  = {3,0,0,  0,1,-3};								
int16_t relp_set_easy4[]  = {4,0,0,  0,1,3,7};								
int16_t relp_set_easy5[]  = {5,0,0,  0,1,3,7,15};							
int16_t relp_set_easy6[]  = {6,0,0,  0,1,3,7,15,31};							
int16_t relp_set_easy7[]  = {7,0,0,  0,1,3,7,15,31,63};							
int16_t relp_set_easy8[]  = {8,0,0,  0,1,3,7,15,31,63,127};						
int16_t relp_set_easy9[]  = {9,0,0,  0,1,3,7,15,31,63,127,47};						
int16_t relp_set_easy10[] = {10,0,0,  0,1,3,6,13,27,55,111,223,51};					
int16_t relp_set_easy11[] = {11,0,0,  0,1,3,6,13,27,48,55,111,223,5};					
int16_t relp_set_easy12[] = {12,0,0,  0,1,3,5,6,13,27,48,55,111,223,96};				
int16_t relp_set_easy13[] = {13,0,0,  0,1,3,5,6,13,21,27,48,55,111,223,219};				
int16_t relp_set_easy14[] = {14,0,0,  0,1,3,5,6,13,21,27,48,55,111,219,223,209};			
int16_t relp_set_easy15[] = {15,0,0,  0,1,3,5,6,13,21,27,48,55,111,219,223,209,83};			
int16_t relp_set_easy16[] = {16,0,0,  0,1,3,5,6,13,21,27,48,55,83,111,209,219,223,201};			
int16_t relp_set_easy17[] = {17,0,0,  0,1,3,5,6,13,21,27,48,55,83,111,201,209,219,223,216};		
int16_t relp_set_easy18[] = {18,0,0,  0,1,3,5,6,13,21,27,48,55,83,111,201,209,216,219,223,231};		
int16_t relp_set_easy19[] = {19,0,0,  0,1,3,5,6,13,21,27,48,55,83,111,201,209,216,219,223,231,20};	
int16_t relp_set_easy20[] = {20,0,0,  0,1,3,5,6,13,20,21,27,48,55,83,111,201,209,216,219,223,231,222};	

int16_t *relp_set_selection (int L) {
	if (L > MAX_RELP_SETS) L = MAX_RELP_SETS;
	if (L <= 2) return relp_set_easy2;
	if (L <= 3) return relp_set_easy3;
	if (L <= 4) return relp_set_easy4;
	if (L <= 5) return relp_set_easy5;
	if (L <= 6) return relp_set_easy6;
	if (L <= 7) return relp_set_easy7;
	if (L <= 8) return relp_set_easy8;
	if (L <= 9) return relp_set_easy9;
	if (L <= 10) return relp_set_easy10;
	if (L <= 11) return relp_set_easy11;
	if (L <= 12) return relp_set_easy12;
	if (L <= 13) return relp_set_easy13;
	if (L <= 14) return relp_set_easy14;
	if (L <= 15) return relp_set_easy15;
	if (L <= 16) return relp_set_easy16;
	if (L <= 17) return relp_set_easy17;
	if (L <= 18) return relp_set_easy18;
	if (L <= 19) return relp_set_easy19;
	return relp_set_easy20;
}

int16_t get_max_relp_set (
	int16_t *relp_sets)
{
	int16_t	num_permanent_sets = *relp_sets;
	int16_t max_set;
	for (int16_t i = 0; i < num_permanent_sets; i++) {
		if (i == 0 || max_set < relp_sets[3 + i]) max_set = relp_sets[3 + i];
	}
	return (max_set);
}

class relp_set_data {
public:
	
	relp_set_data (int16_t b, bool c, bool d)
		{ last_full_used_by = last_partial_used_by = b; nQx_store = base = Dmultiple = diff = 0; permanent = c; partial = d; }
	
	bool is_last_use (int16_t relp_set, int i, int num_partials, int numrels, bool inverted_access_pattern) {
		
		if (relp_set == last_partial_used_by) return (TRUE);
		
		if (relp_set != last_full_used_by) return (FALSE);
		
		inverted_access_pattern ^= last_partial_use_inverted;
		
		return ((!inverted_access_pattern && i >= num_partials) || (inverted_access_pattern && i < numrels - num_partials));
	}
	
	int16_t		nQx_store;		
	int16_t		last_full_used_by;	
	int16_t		last_partial_used_by;	
	int16_t		base;			
	int16_t		Dmultiple;		
	int16_t		diff;			
	bool		permanent;		
	bool		partial;		
	bool		last_partial_use_inverted; 
};

struct relpclasscomp {
	bool operator() (const int16_t &lhs, const int16_t &rhs) const {
		return (abs(lhs) < abs(rhs) || (abs(lhs) == abs(rhs) && lhs < rhs));
	}
};
using relp_set_data_map = std::map<int16_t,relp_set_data,relpclasscomp>;

class Dmultiple_data {
public:
	
	Dmultiple_data (int16_t rlub, uint64_t dlub) {val.x = NULL; val.z = NULL; relp_last_used_by = rlub; Dmult_last_used_by = dlub; base = addin = diff = 0; }
	
	~Dmultiple_data () { if (!do_not_free_val) { gwfree (gwdata, val.x); gwfree (gwdata, val.z); } }
	
	void set_val_buffers (gwhandle *gw, gwnum x, bool do_not_free_flag) { gwdata = gw; val.x = x; val.z = NULL; do_not_free_val = do_not_free_flag; }
	void set_val_buffers (gwhandle *gw, gwnum x, gwnum z, bool do_not_free_flag) { gwdata = gw; val.x = x; val.z = z; do_not_free_val = do_not_free_flag; }
	
	void set_Dmult_last_used_by (uint64_t x) { if (Dmult_last_used_by < x) Dmult_last_used_by = x; }
	
	void free_if_Dmult_last_used_by (uint64_t x) {
		if (do_not_free_val) return;
		if (relp_last_used_by == 0 && Dmult_last_used_by == x) {
			gwfree (gwdata, val.x);
			gwfree (gwdata, val.z);
			val.x = val.z = NULL;
		}
	}
	
	bool will_free_due_to_relp_last_used_by (int16_t x) {
		if (do_not_free_val) return (FALSE);
		return (relp_last_used_by == x);
	}
	
	void free_if_relp_last_used_by (int16_t x) {
		if (will_free_due_to_relp_last_used_by (x)) {
			gwfree (gwdata, val.x);
			gwfree (gwdata, val.z);
			val.x = val.z = NULL;
		}
	}
	
	int16_t		relp_last_used_by;	
	uint64_t	Dmult_last_used_by;	
	uint64_t	base;			
	uint64_t	addin;			
	uint64_t	diff;			
	struct xz	val;			
	gwhandle	*gwdata;		
	bool		do_not_free_val;	
};
using Dmultiple_data_map = std::map<uint64_t,Dmultiple_data>;

void process_relp_sets (
	int	totrels,
	int	numrels,
	int16_t *relp_sets,
	relp_set_data_map &relp_set_map,
	Dmultiple_data_map &Dmultiple_map)
{
	int16_t	num_permanent_sets = *relp_sets++;
	int16_t num_intermediate_full_sets = *relp_sets++;
	int16_t num_intermediate_parital_sets = *relp_sets++;
	int16_t num_sets = num_permanent_sets + num_intermediate_full_sets + num_intermediate_parital_sets;

	for (int16_t i = 0; i < num_sets + 1; i++) {
		bool	permanent = (i < num_permanent_sets);
		bool	partial = (i == num_permanent_sets - 1) || (i >= num_sets - num_intermediate_parital_sets && i < num_sets);
		int16_t	set;

		if (i == num_sets) { if (relp_set_map.find(-1) != relp_set_map.end()) continue; set = -1; }
		else set = relp_sets[i];

		relp_set_map.insert ({set, {set, permanent, partial}});
	}

	int	relps_in_partial_set = one_based_modulo (totrels, numrels);
	int	permanent_nQx_index = 0;
	int	temporary_nQx_index = (num_permanent_sets - 1) * numrels + relps_in_partial_set;
	for (auto this_set = relp_set_map.begin(); this_set != relp_set_map.end(); ++this_set) {
		if (this_set->second.permanent) {
			this_set->second.nQx_store = permanent_nQx_index;
			permanent_nQx_index += this_set->second.partial ? relps_in_partial_set : numrels;
		} else {
			this_set->second.nQx_store = temporary_nQx_index;
			temporary_nQx_index += this_set->second.partial ? relps_in_partial_set : numrels;
		}
	}

	for (auto this_set = relp_set_map.begin(); this_set != relp_set_map.end(); ++this_set) {
		
		if (this_set->first == 0 || this_set->first == -1) continue;
		
		for (auto base_set = local::make_reverse_iterator(this_set); ; ++base_set) {		
			
			if (((this_set->first < 0) ^ (base_set->first < 0)) == 1) continue;
			
			int16_t	Dmultiple = abs(this_set->first) - abs(base_set->first);
			int16_t diff = (this_set->first > 0) ? base_set->first - Dmultiple : base_set->first + Dmultiple;
			auto diff_set = relp_set_map.find(diff);
			if (diff_set == relp_set_map.end()) continue;
			
			if (!this_set->second.partial && (base_set->second.partial || diff_set->second.partial)) continue;
			
			if (this_set->first < 0 && this_set->second.partial && diff_set->second.partial) continue;
			
			this_set->second.base = base_set->first;
			this_set->second.Dmultiple = Dmultiple;
			this_set->second.diff = diff;

			base_set->second.last_partial_used_by = this_set->first;
			base_set->second.last_partial_use_inverted = FALSE;
			if (!this_set->second.partial) base_set->second.last_full_used_by = this_set->first;

			diff_set->second.last_partial_used_by = this_set->first;
			diff_set->second.last_partial_use_inverted = (this_set->first < 0) ^ (diff_set->first < 0);
			if (!this_set->second.partial) diff_set->second.last_full_used_by = this_set->first;

			auto Dfind = Dmultiple_map.find(Dmultiple);
			if (Dfind != Dmultiple_map.end()) Dfind->second.relp_last_used_by = this_set->first;
			else Dmultiple_map.insert ({(uint64_t) Dmultiple, {this_set->first, (uint64_t) Dmultiple}});
			break;
		}
	}
}

void process_Dmult_map (
	Dmultiple_data_map &Dmultiple_map)
{

	for (auto this_Dmult = Dmultiple_map.rbegin(); this_Dmult != Dmultiple_map.rend(); ++this_Dmult) {
		if (this_Dmult->first == 1) break;

		uint64_t proposed_base, proposed_addin, proposed_diff, create_1, create_2;
		if ((this_Dmult->first & 1) == 0) {	
			proposed_base = this_Dmult->first / 2;
			proposed_addin = this_Dmult->first / 2;
			proposed_diff = 0;
			create_1 = proposed_base;
			create_2 = 0;
		} else {				
			proposed_base = this_Dmult->first / 2 + 1;
			proposed_addin = this_Dmult->first / 2;
			proposed_diff = 1;
			create_1 = proposed_base;
			create_2 = proposed_addin;
		}

		for (auto base_Dmult = Dmultiple_map.lower_bound((this_Dmult->first+1)/2); base_Dmult->first < this_Dmult->first; base_Dmult++) {
			
			uint64_t addin = this_Dmult->first - base_Dmult->first;
			uint64_t diff = base_Dmult->first - addin;
			bool	addin_exists = (Dmultiple_map.find(addin) != Dmultiple_map.end());
			bool	diff_exists = (diff == 0 || Dmultiple_map.find(diff) != Dmultiple_map.end());
			
			if (addin_exists && diff_exists) {
				proposed_base = base_Dmult->first;
				proposed_addin = addin;
				proposed_diff = diff;
				create_1 = create_2 = 0;
				break;
			}
			
			if (addin_exists && diff < create_1) {
				proposed_base = base_Dmult->first;
				proposed_addin = addin;
				proposed_diff = diff;
				create_1 = diff;
				create_2 = 0;
			}
			
			if (diff_exists && addin < create_1) {
				proposed_base = base_Dmult->first;
				proposed_addin = addin;
				proposed_diff = diff;
				create_1 = addin;
				create_2 = 0;
			}
		}
		
		this_Dmult->second.base = proposed_base;
		this_Dmult->second.addin = proposed_addin;
		this_Dmult->second.diff = proposed_diff;
		if (create_1) Dmultiple_map.insert ({create_1, {0, create_1}});		
		if (create_2) Dmultiple_map.insert ({create_2, {0, create_2}});		
		
		Dmultiple_map.find (proposed_base)->second.set_Dmult_last_used_by (this_Dmult->first);
		if (proposed_diff != 0) {
			Dmultiple_map.find (proposed_addin)->second.set_Dmult_last_used_by (this_Dmult->first);
			Dmultiple_map.find (proposed_diff)->second.set_Dmult_last_used_by (this_Dmult->first);
		}
	}
}

struct D_data {
	int	D;
	int	numrels;			
	int	first_missing_prime;		
	int	second_missing_prime;		
} D_data[] = {
	{ 2 * 3 * 5                  , 2 * 4 / 2, 7, 11 },			
	{ 2 * 3     * 7              , 2 * 6 / 2, 5, 11 },			
	{ 2 * 3 * 5               * 2, 2 * 4 / 2 * 2, 7, 11 },			
	{ 2 * 3 * 5               * 3, 2 * 4 / 2 * 3, 7, 11 },			
	{ 2 * 3 * 5               * 4, 2 * 4 / 2 * 4, 7, 11 },			
	{ 2 * 3     * 7           * 3, 2 * 6 / 2 * 3, 5, 11 },			
	{ 2 * 3 * 5               * 5, 2 * 4 / 2 * 5, 7, 11 },			
	{ 2 * 3 * 5 * 7              , 2 * 4 * 6 / 2, 11, 13 },			
	{ 2 * 3 * 5     * 11         , 2 * 4 * 10 / 2, 7, 13 },			
	{ 2 * 3     * 7           * 7, 2 * 6 / 2 * 7, 5, 11 },			
	{ 2 * 3 * 5 * 7           * 2, 2 * 4 * 6 / 2 * 2, 11, 13 },		
	{ 2 * 3     * 7 * 11         , 2 * 6 * 10 / 2, 5, 13 },			
	{ 2 * 3 * 5 * 7           * 3, 2 * 4 * 6 / 2 * 3, 11, 13 },		
	{ 2 * 3     * 7           *14, 2 * 6 / 2 * 14, 5, 11 },			
	{ 2 * 3 * 5 * 7           * 4, 2 * 4 * 6 / 2 * 4, 11, 13 },		
	{ 2 * 3     * 7           *16, 2 * 6 / 2 * 16, 5, 11 },			
	{ 2 * 3     * 7           *18, 2 * 6 / 2 * 18, 5, 11 },			
	{ 2 * 3 * 5 * 7           * 5, 2 * 4 * 6 / 2 * 5, 11, 13 },		
	{ 2 * 3     * 7 * 11      * 2, 2 * 6 * 10 / 2 * 2, 5, 13 },		
	{ 2 * 3     * 7           *21, 2 * 6 / 2 * 21, 5, 11 },			
	{ 2 * 3 * 5 * 7           * 6, 2 * 4 * 6 / 2 * 6, 11, 13 },		
	{ 2 * 3 * 5 * 7           * 7, 2 * 4 * 6 / 2 * 7, 11, 13 },		
	{ 2 * 3     * 7 * 11      * 3, 2 * 6 * 10 / 2 * 3, 5, 13 },		
	{ 2 * 3 * 5 * 7           * 8, 2 * 4 * 6 / 2 * 8, 11, 13 },		
	{ 2 * 3 * 5 * 7           * 9, 2 * 4 * 6 / 2 * 9, 11, 13 },		
	{ 2 * 3     * 7 * 11      * 4, 2 * 6 * 10 / 2 * 4, 5, 13 },		
	{ 2 * 3 * 5 * 7 * 11         , 2 * 4 * 6 * 10 / 2, 13, 17 },		
	{ 2 * 3 * 5 * 7      * 13    , 2 * 4 * 6 * 12 / 2, 11, 17 },		
	{ 2 * 3     * 7 * 11      * 6, 2 * 6 * 10 / 2 * 6, 5, 13 },		
	{ 2 * 3 * 5 * 7 * 11      * 2, 2 * 4 * 6 * 10 / 2 * 2, 13, 17 },	
	{ 2 * 3 * 5     * 11 * 13    , 2 * 4 * 10 * 12 / 2, 7, 17 },		
	{ 2 * 3 * 5 * 7 * 11      * 3, 2 * 4 * 6 * 10 / 2 * 3, 13, 17 },	
	{ 2 * 3     * 7 * 11 * 13    , 2 * 6 * 10 * 12 / 2, 5, 17 },		
	{ 2 * 3 * 5 * 7 * 11      * 4, 2 * 4 * 6 * 10 / 2 * 4, 13, 17 }		
};
#define NUM_D		(sizeof(D_data)/sizeof(struct D_data))
#define MAX_D		9240
#define MAX_RELPRIMES	960

#ifdef ALTERNATE_D_DATA_FOR_POLYS_BUILT_FROM_TABLE_BELOW
struct D_data poly_D_data[] = {
	{ 2 * 3                   * 9, 1 *9, 5, 7 },			
	{ 2 * 3                   *11, 10  , 5, 7 },			
	{ 2 * 3                   *13, 12  , 5, 7 },			
	{ 2 * 3                   *17, 16  , 5, 7 },			
	{ 2 * 3                   *19, 18  , 5, 7 },			
	{ 2 * 3 * 5                  , 4         , 7, 11 },		
	{ 2 * 3 * 5               * 3, 4       *3, 7, 11 },		
	{ 2 * 3 * 5               * 5, 4       *5, 7, 11 },		
	{ 2 * 3 * 5               * 9, 4       *9, 7, 11 },		
	{ 2 * 3 * 5               *13, 4 * 12    , 7, 11 },		
	{ 2 * 3 * 5               *15, 4      *15, 7, 11 },		
	{ 2 * 3 * 5               *17, 4 * 16    , 7, 11 },		
	{ 2 * 3 * 5               *19, 4 * 18    , 7, 11 },		
	{ 2 * 3     * 7              , 6        , 5, 11 },		
	{ 2 * 3     * 7           * 3, 6      *3, 5, 11 },		
	{ 2 * 3     * 7           * 7, 6      *7, 5, 11 },		
	{ 2 * 3     * 7           * 9, 6      *9, 5, 11 },		
	{ 2 * 3     * 7           *13, 6 * 12   , 5, 11 },		
	{ 2 * 3     * 7           *17, 6 * 16   , 5, 11 },		
	{ 2 * 3     * 7           *19, 6 * 18   , 5, 11 },		
	{ 2 * 3 * 5 * 7              , 4 * 6         , 11, 13 },	
	{ 2 * 3 * 5 * 7           * 3, 4 * 6       *3, 11, 13 },	
	{ 2 * 3 * 5 * 7           * 5, 4 * 6       *5, 11, 13 },	
	{ 2 * 3 * 5 * 7           * 7, 4 * 6       *7, 11, 13 },	
	{ 2 * 3 * 5 * 7           * 9, 4 * 6       *9, 11, 13 },	
	{ 2 * 3 * 5 * 7           *15, 4 * 6      *15, 11, 13 },	
	{ 2 * 3 * 5 * 7           *17, 4 * 6 * 16    , 11, 13 },	
	{ 2 * 3 * 5 * 7           *19, 4 * 6 * 18    , 11, 13 },	
	{ 2 * 3     * 7 * 11         , 6 * 10        , 5, 13 },		
	{ 2 * 3     * 7 * 11       *3, 6 * 10      *3, 5, 13 },		
	{ 2 * 3     * 7 * 11       *7, 6 * 10      *7, 5, 13 },		
	{ 2 * 3     * 7 * 11       *9, 6 * 10      *9, 5, 13 },		
	{ 2 * 3     * 7 * 11      *11, 6 * 10     *11, 5, 13 },		
	{ 2 * 3     * 7 * 11      *17, 6 * 10 * 16   , 5, 13 },		
	{ 2 * 3     * 7 * 11      *19, 6 * 10 * 18   , 5, 13 },		
	{ 2 * 3 * 5     * 11         , 4 * 10        , 7, 13 },		
	{ 2 * 3 * 5     * 11       *3, 4 * 10      *3, 7, 13 },		
	{ 2 * 3 * 5     * 11       *5, 4 * 10      *5, 7, 13 },		
	{ 2 * 3 * 5     * 11       *9, 4 * 10      *9, 7, 13 },		
	{ 2 * 3 * 5     * 11      *11, 4 * 10     *11, 7, 13 },		
	{ 2 * 3 * 5     * 11      *15, 4 * 10     *15, 7, 13 },		
	{ 2 * 3 * 5     * 11      *17, 4 * 10 * 16   , 7, 13 },		
	{ 2 * 3 * 5     * 11      *19, 4 * 10 * 18   , 7, 13 },		
	{ 2 * 3 * 5 * 7      * 13     , 4 * 6 * 12        , 11, 17 },	
	{ 2 * 3 * 5 * 7      * 13   *3, 4 * 6 * 12      *3, 11, 17 },	
	{ 2 * 3 * 5 * 7      * 13   *5, 4 * 6 * 12      *5, 11, 17 },	
	{ 2 * 3 * 5 * 7      * 13   *7, 4 * 6 * 12      *7, 11, 17 },	
	{ 2 * 3 * 5 * 7      * 13   *9, 4 * 6 * 12      *9, 11, 17 },	
	{ 2 * 3 * 5 * 7      * 13  *13, 4 * 6 * 12     *13, 11, 17 },	
	{ 2 * 3 * 5 * 7      * 13  *19, 4 * 6 * 12 * 18   , 11, 17 },	
	{ 2 * 3 * 5     * 11 * 13     , 4 * 10 * 12        , 7, 17 },	
	{ 2 * 3 * 5     * 11 * 13   *3, 4 * 10 * 12      *3, 7, 17 },	
	{ 2 * 3 * 5     * 11 * 13   *5, 4 * 10 * 12      *5, 7, 17 },	
	{ 2 * 3 * 5     * 11 * 13   *9, 4 * 10 * 12      *9, 7, 17 },	
	{ 2 * 3 * 5     * 11 * 13  *11, 4 * 10 * 12     *11, 7, 17 },	
	{ 2 * 3 * 5     * 11 * 13  *13, 4 * 10 * 12     *13, 7, 17 },	
	{ 2 * 3 * 5     * 11 * 13  *15, 4 * 10 * 12     *15, 7, 17 },	
	{ 2 * 3 * 5     * 11 * 13  *19, 4 * 10 * 12 * 18   , 7, 17 },	
	{ 2 * 3 * 5 * 7 * 11          , 4 * 6 * 10        , 13, 17 },	
	{ 2 * 3 * 5 * 7 * 11        *3, 4 * 6 * 10      *3, 13, 17 },	
	{ 2 * 3 * 5 * 7 * 11        *5, 4 * 6 * 10      *5, 13, 17 },	
	{ 2 * 3 * 5 * 7 * 11        *7, 4 * 6 * 10      *7, 13, 17 },	
	{ 2 * 3 * 5 * 7 * 11        *9, 4 * 6 * 10      *9, 13, 17 },	
	{ 2 * 3 * 5 * 7 * 11       *11, 4 * 6 * 10     *11, 13, 17 },	
	{ 2 * 3 * 5 * 7 * 11       *15, 4 * 6 * 10     *15, 13, 17 },	
	{ 2 * 3 * 5 * 7 * 11       *19, 4 * 6 * 10 * 18   , 13, 17 },	
	{ 2 * 3 * 5 * 7 * 11 * 13     , 4 * 6 * 10 * 12   , 17, 19 },	
	{ 2 * 3 * 5 * 7 * 11 * 13   *3, 4 * 6 * 10 * 12 *3, 17, 19 },	
	{ 2 * 3 * 5 * 7 * 11 * 13   *5, 4 * 6 * 10 * 12 *5, 17, 19 },	
	{ 2 * 3 * 5 * 7 * 11 * 13   *7, 4 * 6 * 10 * 12 *7, 17, 19 },	
	{ 2 * 3 * 5 * 7 * 11 * 13   *9, 4 * 6 * 10 * 12 *9, 17, 19 },	
	{ 2 * 3 * 5 * 7 * 11 * 13  *11, 4 * 6 * 10 * 12*11, 17, 19 },	
	{ 2 * 3 * 5 * 7 * 11 * 13  *13, 4 * 6 * 10 * 12*13, 17, 19 },	
	{ 2 * 3 * 5 * 7 * 11 * 13  *15, 4 * 6 * 10 * 12*15, 17, 19 },	
	{ 2 * 3 * 5 * 7 * 11 * 13  *19, 4 * 6 * 10 * 12*18, 17, 23 },	
	{ 2 * 3 * 5 * 7 * 11 * 13  *21, 4 * 6 * 10 * 12*21, 17, 19 },	
	{ 2 * 3 * 5 * 7 * 11 * 13  *23, 4 * 6 * 10 * 12*22, 17, 19 },	
	{ 2 * 3 * 5 * 7 * 11 * 13  *25, 4 * 6 * 10 * 12*25, 17, 19 },	
	{ 2 * 3 * 5 * 7 * 11 * 13  *27, 4 * 6 * 10 * 12*27, 17, 19 },	
	{ 2 * 3 * 5 * 7 * 11 * 13  *29, 4 * 6 * 10 * 12*28, 17, 19 },	
	{ 2 * 3 * 5 * 7 * 11 * 13  *31, 4 * 6 * 10 * 12*30, 17, 19 },	
	{ 2 * 3 * 5 * 7 * 11 * 13  *37, 4 * 6 * 10 * 12*36, 17, 19 },	
	{ 2 * 3 * 5 * 7 * 11 * 13  *41, 4 * 6 * 10 * 12*40, 17, 19 },	
	{ 2 * 3 * 5 * 7 * 11 * 13  *43, 4 * 6 * 10 * 12*42, 17, 19 },	
	{ 2 * 3 * 5 * 7 * 11 * 13  *47, 4 * 6 * 10 * 12*46, 17, 19 },	
	{ 2 * 3 * 5 * 7 * 11 * 13  *53, 4 * 6 * 10 * 12*52, 17, 19 },	
	{ 2 * 3 * 5 * 7 * 11 * 13  *59, 4 * 6 * 10 * 12*58, 17, 19 },	
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17     , 4 * 6 * 10 * 12 * 16   , 19, 23 },	
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17   *3, 4 * 6 * 10 * 12 * 16 *3, 19, 23 },	
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17   *5, 4 * 6 * 10 * 12 * 16 *5, 19, 23 },	
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17   *7, 4 * 6 * 10 * 12 * 16 *7, 19, 23 },	
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17   *9, 4 * 6 * 10 * 12 * 16 *9, 19, 23 },	
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17  *11, 4 * 6 * 10 * 12 * 16*11, 19, 23 },	
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17  *13, 4 * 6 * 10 * 12 * 16*13, 19, 23 },	
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17  *15, 4 * 6 * 10 * 12 * 16*15, 19, 23 },	
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17  *17, 4 * 6 * 10 * 12 * 16*17, 19, 23 },	
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17  *25, 4 * 6 * 10 * 12 * 16*25, 19, 23 },	
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17  *35, 4 * 6 * 10 * 12 * 16*35, 19, 23 },	
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17  *45, 4 * 6 * 10 * 12 * 16*45, 19, 23 },	
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17 * 19     , 4 * 6 * 10 * 12 * 16 * 18   , 23, 29 }, 
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17 * 19   *3, 4 * 6 * 10 * 12 * 16 * 18 *3, 23, 29 }, 
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17 * 19   *5, 4 * 6 * 10 * 12 * 16 * 18 *5, 23, 29 }, 
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17 * 19   *7, 4 * 6 * 10 * 12 * 16 * 18 *7, 23, 29 }, 
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17 * 19   *9, 4 * 6 * 10 * 12 * 16 * 18 *9, 23, 29 }, 
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17 * 19  *11, 4 * 6 * 10 * 12 * 16 * 18*11, 23, 29 }, 
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17 * 19  *13, 4 * 6 * 10 * 12 * 16 * 18*13, 23, 29 }, 
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17 * 19  *15, 4 * 6 * 10 * 12 * 16 * 18*15, 23, 29 }, 
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17 * 19  *17, 4 * 6 * 10 * 12 * 16 * 18*17, 23, 29 }, 
};
#endif

struct D_data poly_D_data[] = {
	{ 2 * 3 * 5                  , 4         , 7, 11 },		
	{ 2 * 3     * 7              , 6        , 5, 11 },		
	{ 2 * 3                   * 9, 1 *9, 5, 7 },			
	{ 2 * 3                   *11, 10  , 5, 7 },			
	{ 2 * 3 * 5               * 3, 4       *3, 7, 11 },		
	{ 2 * 3                   *17, 16  , 5, 7 },			
	{ 2 * 3     * 7           * 3, 6      *3, 5, 11 },		
	{ 2 * 3 * 5               * 5, 4       *5, 7, 11 },		
	{ 2 * 3 * 5 * 7              , 4 * 6         , 11, 13 },	
	{ 2 * 3 * 5               * 9, 4       *9, 7, 11 },		
	{ 2 * 3 * 5     * 11         , 4 * 10        , 7, 13 },		
	{ 2 * 3 * 5               *13, 4 * 12    , 7, 11 },		
	{ 2 * 3     * 7 * 11         , 6 * 10        , 5, 13 },		
	{ 2 * 3 * 5               *17, 4 * 16    , 7, 11 },		
	{ 2 * 3 * 5 * 7           * 3, 4 * 6       *3, 11, 13 },	
	{ 2 * 3     * 7           *17, 6 * 16   , 5, 11 },		
	{ 2 * 3     * 7           *19, 6 * 18   , 5, 11 },		
	{ 2 * 3 * 5 * 7           * 5, 4 * 6       *5, 11, 13 },	
	{ 2 * 3 * 5 * 7           * 7, 4 * 6       *7, 11, 13 },	
	{ 2 * 3 * 5     * 11       *5, 4 * 10      *5, 7, 13 },		
	{ 2 * 3 * 5 * 7           * 9, 4 * 6       *9, 11, 13 },	
	{ 2 * 3 * 5 * 7 * 11          , 4 * 6 * 10        , 13, 17 },	
	{ 2 * 3 * 5 * 7      * 13     , 4 * 6 * 12        , 11, 17 },	
	{ 2 * 3 * 5 * 7           *15, 4 * 6      *15, 11, 13 },	
	{ 2 * 3 * 5 * 7           *17, 4 * 6 * 16    , 11, 13 },	
	{ 2 * 3 * 5 * 7           *19, 4 * 6 * 18    , 11, 13 },	
	{ 2 * 3 * 5     * 11 * 13     , 4 * 10 * 12        , 7, 17 },	
	{ 2 * 3 * 5     * 11      *15, 4 * 10     *15, 7, 13 },		
	{ 2 * 3 * 5     * 11      *17, 4 * 10 * 16   , 7, 13 },		
	{ 2 * 3 * 5 * 7 * 11        *3, 4 * 6 * 10      *3, 13, 17 },	
	{ 2 * 3 * 5 * 7      * 13   *3, 4 * 6 * 12      *3, 11, 17 },	
	{ 2 * 3     * 7 * 11      *19, 6 * 10 * 18   , 5, 13 },		
	{ 2 * 3 * 5 * 7 * 11        *5, 4 * 6 * 10      *5, 13, 17 },	
	{ 2 * 3 * 5 * 7      * 13   *5, 4 * 6 * 12      *5, 11, 17 },	
	{ 2 * 3 * 5 * 7 * 11        *7, 4 * 6 * 10      *7, 13, 17 },	
	{ 2 * 3 * 5 * 7      * 13   *7, 4 * 6 * 12      *7, 11, 17 },	
	{ 2 * 3 * 5 * 7 * 11        *9, 4 * 6 * 10      *9, 13, 17 },	
	{ 2 * 3 * 5     * 11 * 13   *5, 4 * 10 * 12      *5, 7, 17 },	
	{ 2 * 3 * 5 * 7      * 13   *9, 4 * 6 * 12      *9, 11, 17 },	
	{ 2 * 3 * 5 * 7 * 11       *11, 4 * 6 * 10     *11, 13, 17 },	
	{ 2 * 3 * 5 * 7 * 11 * 13     , 4 * 6 * 10 * 12   , 17, 19 },	
	{ 2 * 3 * 5 * 7 * 11       *15, 4 * 6 * 10     *15, 13, 17 },	
	{ 2 * 3 * 5 * 7      * 13  *13, 4 * 6 * 12     *13, 11, 17 },	
	{ 2 * 3 * 5 * 7 * 11       *19, 4 * 6 * 10 * 18   , 13, 17 },	
	{ 2 * 3 * 5 * 7      * 13  *19, 4 * 6 * 12 * 18   , 11, 17 },	
	{ 2 * 3 * 5     * 11 * 13  *13, 4 * 10 * 12     *13, 7, 17 },	
	{ 2 * 3 * 5     * 11 * 13  *15, 4 * 10 * 12     *15, 7, 17 },	
	{ 2 * 3 * 5 * 7 * 11 * 13   *3, 4 * 6 * 10 * 12 *3, 17, 19 },	
	{ 2 * 3 * 5 * 7 * 11 * 13   *5, 4 * 6 * 10 * 12 *5, 17, 19 },	
	{ 2 * 3 * 5 * 7 * 11 * 13   *7, 4 * 6 * 10 * 12 *7, 17, 19 },	
	{ 2 * 3 * 5 * 7 * 11 * 13   *9, 4 * 6 * 10 * 12 *9, 17, 19 },	
	{ 2 * 3 * 5 * 7 * 11 * 13  *11, 4 * 6 * 10 * 12*11, 17, 19 },	
	{ 2 * 3 * 5 * 7 * 11 * 13  *13, 4 * 6 * 10 * 12*13, 17, 19 },	
	{ 2 * 3 * 5 * 7 * 11 * 13  *15, 4 * 6 * 10 * 12*15, 17, 19 },	
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17     , 4 * 6 * 10 * 12 * 16   , 19, 23 },	
	{ 2 * 3 * 5 * 7 * 11 * 13  *19, 4 * 6 * 10 * 12*18, 17, 23 },	
	{ 2 * 3 * 5 * 7 * 11 * 13  *21, 4 * 6 * 10 * 12*21, 17, 19 },	
	{ 2 * 3 * 5 * 7 * 11 * 13  *23, 4 * 6 * 10 * 12*22, 17, 19 },	
	{ 2 * 3 * 5 * 7 * 11 * 13  *25, 4 * 6 * 10 * 12*25, 17, 19 },	
	{ 2 * 3 * 5 * 7 * 11 * 13  *27, 4 * 6 * 10 * 12*27, 17, 19 },	
	{ 2 * 3 * 5 * 7 * 11 * 13  *29, 4 * 6 * 10 * 12*28, 17, 19 },	
	{ 2 * 3 * 5 * 7 * 11 * 13  *31, 4 * 6 * 10 * 12*30, 17, 19 },	
	{ 2 * 3 * 5 * 7 * 11 * 13  *37, 4 * 6 * 10 * 12*36, 17, 19 },	
	{ 2 * 3 * 5 * 7 * 11 * 13  *41, 4 * 6 * 10 * 12*40, 17, 19 },	
	{ 2 * 3 * 5 * 7 * 11 * 13  *43, 4 * 6 * 10 * 12*42, 17, 19 },	
	{ 2 * 3 * 5 * 7 * 11 * 13  *47, 4 * 6 * 10 * 12*46, 17, 19 },	
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17   *3, 4 * 6 * 10 * 12 * 16 *3, 19, 23 },	
	{ 2 * 3 * 5 * 7 * 11 * 13  *53, 4 * 6 * 10 * 12*52, 17, 19 },	
	{ 2 * 3 * 5 * 7 * 11 * 13  *59, 4 * 6 * 10 * 12*58, 17, 19 },	
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17   *5, 4 * 6 * 10 * 12 * 16 *5, 19, 23 },	
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17   *7, 4 * 6 * 10 * 12 * 16 *7, 19, 23 },	
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17   *9, 4 * 6 * 10 * 12 * 16 *9, 19, 23 },	
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17  *11, 4 * 6 * 10 * 12 * 16*11, 19, 23 },	
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17  *13, 4 * 6 * 10 * 12 * 16*13, 19, 23 },	
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17  *15, 4 * 6 * 10 * 12 * 16*15, 19, 23 },	
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17  *17, 4 * 6 * 10 * 12 * 16*17, 19, 23 },	
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17 * 19     , 4 * 6 * 10 * 12 * 16 * 18   , 23, 29 }, 
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17  *25, 4 * 6 * 10 * 12 * 16*25, 19, 23 },	
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17  *35, 4 * 6 * 10 * 12 * 16*35, 19, 23 },	
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17  *45, 4 * 6 * 10 * 12 * 16*45, 19, 23 },	
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17 * 19   *3, 4 * 6 * 10 * 12 * 16 * 18 *3, 23, 29 }, 
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17 * 19   *5, 4 * 6 * 10 * 12 * 16 * 18 *5, 23, 29 }, 
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17 * 19   *7, 4 * 6 * 10 * 12 * 16 * 18 *7, 23, 29 }, 
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17 * 19   *9, 4 * 6 * 10 * 12 * 16 * 18 *9, 23, 29 }, 
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17 * 19  *11, 4 * 6 * 10 * 12 * 16 * 18*11, 23, 29 }, 
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17 * 19  *13, 4 * 6 * 10 * 12 * 16 * 18*13, 23, 29 }, 
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17 * 19  *15, 4 * 6 * 10 * 12 * 16 * 18*15, 23, 29 }, 
	{ 2 * 3 * 5 * 7 * 11 * 13 * 17 * 19  *17, 4 * 6 * 10 * 12 * 16 * 18*17, 23, 29 }, 
};
#define POLY_NUM_D		(sizeof(poly_D_data)/sizeof(struct D_data))
#define POLY_MAX_D		164894730
#define POLY_MAX_RELPRIMES	14100480

int map_D_to_index (int D)
{
	for (int i = 0; i < NUM_D; i++) if (D_data[i].D == D) return (i);
	ASSERTG (0);
	return (0);
}

int map_polyD_to_index (int D)
{
	for (int i = 0; i < POLY_NUM_D; i++) if (poly_D_data[i].D == D) return (i);
	ASSERTG (0);
	return (0);
}

#define map_D_to_numrels(D)	D_data[map_D_to_index(D)].numrels

#define map_polyD_to_first_missing_prime(D)	poly_D_data[map_polyD_to_index(D)].first_missing_prime

struct common_cost_data {
	
	gwhandle *gwdata;		
	pmhandle *polydata;		
	long	stage1_fftlen;		
	long	stage2_fftlen;		
	double	stage1_cost;		
	double	gcd_cost;		
	double	modinv_cost;		
	double	poly_compression;	
	int	stage1_threads;		
	int	stage2_threads;		
	uint64_t numvals;		
					
	bool	use_poly_D_data;	
	bool	centers_on_Dmultiple;	
	int	required_missing;	
	uint64_t gap_start;		
	uint64_t gap_end;		
	
	double	max_pairmap_size;	
	int	D;			
	int	totrels;		
	int	numrels;		
	double	multiplier;		
	int	first_missing_prime;	
	int16_t *relp_sets;		
	uint64_t B1;			
	uint64_t B2;			
	uint64_t B2_start;		
	uint64_t numDsections;		
	uint64_t max_pairmap_Dsections;	
	int	numvals_consumed_by_pairmap; 
	double	est_pair_pct;		
	double	est_numprimes;		
	double	est_numpairs;		
	double	est_numsingles;		
	double	est_pairing_runtime;	
	double	est_init_transforms;	
	double	est_stage2_transforms;	
	double	est_init_polymult;	
	double	est_stage2_polymult;	
	double	est_stage2_stage1_ratio;
	uint64_t stage2_numvals;	
	double	stage2_cost;		
	double	total_cost;		
	double	efficiency;		
};

void init_gcd_costs (
	double	bit_length,		
	void	*cost_func_data)	
{
	struct common_cost_data *c = (struct common_cost_data *) cost_func_data;
	c->gcd_cost = 320.53 * log (bit_length) - 3302.0;
	if (c->gcd_cost < 100.0) c->gcd_cost = 100.0;
	c->modinv_cost = 570.16 * log (bit_length) - 6188.4;
	if (c->modinv_cost < 1.25 * c->gcd_cost) c->modinv_cost = 1.25 * c->gcd_cost;
	if (c->stage2_threads == 2) c->gcd_cost *= 1.9, c->modinv_cost *= 1.9;
	else if (c->stage2_threads == 3) c->gcd_cost *= 2.7, c->modinv_cost *= 2.7;
	else if (c->stage2_threads >= 4) c->gcd_cost *= 3.2, c->modinv_cost *= 3.2;
}

double best_stage2_impl_internal (
	uint64_t B,			
	uint64_t C_start,		
	uint64_t C,			
	uint64_t max_numvals,		
	double	(*cost_func)(void *),	
	void	*cost_func_data)	
{
	struct common_cost_data *c = (struct common_cost_data *) cost_func_data;
	uint64_t B2_end;
	float	pairing_percentage, pairing_runtime;
	double	efficiency, best_efficiency;
	int	D, i, best_i, j;

	struct D_data *d_data;		
	int	num_d;			
	if (c->use_poly_D_data) {
		d_data = poly_D_data;
		num_d = POLY_NUM_D;
	} else {
		d_data = D_data;
		num_d = NUM_D;
	}

	efficiency = best_efficiency = -1.0;
	for (j = 0; j < 2; j++) {

	    for (i = 0; i < num_d; i++) {

		if (j == 1) {
			if (best_efficiency < 0.0) break;
			i = best_i;
		}

		if (d_data[i].numrels > max_numvals) break;

		D = d_data[i].D;
		
		c->B1 = B;
		c->B2 = C;
		c->D = D;
		c->numrels = d_data[i].numrels;
		c->first_missing_prime = d_data[i].first_missing_prime;
		
		if (c->centers_on_Dmultiple) {
			B2_end = round_up_to_multiple_of (C - D / 2, D) + D / 2;
			c->B2_start = (uint64_t) floor ((double) (B2_end / (double) d_data[i].first_missing_prime));
			if (c->B2_start < C_start) c->B2_start = C_start;
			if (c->gap_end && c->B2_start < c->gap_end) c->B2_start = c->gap_end;
			if (c->B2_start < D / 2) break;		
			c->B2_start = round_down_to_multiple_of (c->B2_start + D / 2, D) - D / 2;
		}
		
		else {
			B2_end = round_up_to_multiple_of (C, D);
			c->B2_start = (uint64_t) floor ((double) (B2_end / (double) d_data[i].first_missing_prime));
			if (c->B2_start < C_start) c->B2_start = C_start;
			if (c->gap_end && c->B2_start < c->gap_end) c->B2_start = c->gap_end;
			c->B2_start = round_down_to_multiple_of (c->B2_start, D);
		}
		c->numDsections = (B2_end - c->B2_start) / D;

		if (!c->use_poly_D_data) {

			if (c->gap_start && c->gap_start * d_data[i].first_missing_prime > B2_end) continue;

			c->totrels = (max_numvals > MAX_RELP_SETS * d_data[i].numrels) ? MAX_RELP_SETS * d_data[i].numrels : (int) max_numvals;
			estimate_pairing (d_data[i].second_missing_prime, C_start, C, c->totrels, d_data[i].numrels, &pairing_percentage, &pairing_runtime);
			c->est_pair_pct = pairing_percentage;
			c->est_numpairs = (pairing_percentage * c->est_numprimes) / 2.0;
			c->est_numsingles = c->est_numprimes - c->est_numpairs * 2.0;
			double	est_totpairs = c->est_numpairs + c->est_numsingles;		
			c->max_pairmap_Dsections = estimate_max_pairmap_Dsections (c->max_pairmap_size, c->numDsections, est_totpairs);
			c->numvals_consumed_by_pairmap = (int)
				((est_totpairs > c->max_pairmap_size ? c->max_pairmap_size : est_totpairs) / (c->stage2_fftlen * sizeof (double)));
			
			if (c->totrels + c->numvals_consumed_by_pairmap > max_numvals) continue;

			if (c->stage2_fftlen < 512*1024)
				c->est_pairing_runtime = pairing_runtime / (0.00169 * pow(2.0,log2((double)c->stage2_fftlen/(512.0*1024.0))*1.0407)) * 2.0;
			else
				c->est_pairing_runtime = pairing_runtime / (0.00169 * pow(2.0,log2((double)c->stage2_fftlen/(512.0*1024.0))*1.093)) * 2.0;
			
			if (c->stage2_threads == 2) c->est_pairing_runtime *= 1.9;
			else if (c->stage2_threads == 3) c->est_pairing_runtime *= 2.7;
			else if (c->stage2_threads >= 4) c->est_pairing_runtime *= 3.2;

			c->multiplier = (double) c->totrels / (double) c->numrels;
			c->relp_sets = relp_set_selection ((int) ceil (c->multiplier));
		}

		else {
			if (c->required_missing && D % c->required_missing == 0) continue;
		}

		efficiency = (*cost_func) (cost_func_data);

		if (j == 1) break;

		if (efficiency > best_efficiency) {
			best_efficiency = efficiency;
			best_i = i;
		}

		else {
			if (best_efficiency >= 0.0 && i > best_i + 2) break;
		}
	    }
	}

	return  (efficiency);
}

double best_stage2_impl (
	uint64_t B,			
	uint64_t C_start,		
	uint64_t gap_start,		
	uint64_t gap_end,		
	uint64_t C,			
	uint64_t numvals,		
	double	(*cost_func)(void *),	
	void	*cost_func_data)	
{
	struct common_cost_data *c = (struct common_cost_data *) cost_func_data;
	struct best_stage2_cost {
		uint64_t numvals;
		double	efficiency;
	} best[3], midpoint;

	if (gap_start > gap_end) gap_start = gap_end = 0;
	ASSERTG (gap_start == 0 || gap_start >= C_start);
	ASSERTG (gap_start == 0 || gap_end < C);
	c->gap_start = (gap_start > C_start ? gap_start : 0);
	c->gap_end = (gap_end > C_start ? gap_end : 0);

	c->numvals = numvals;
	if (c->numvals < 4) return (-1.0);

	if (!c->use_poly_D_data) {
		int max_pairmap_size = IniGetInt (INI_FILE, "MaximumBitArraySize", 250);
		if (max_pairmap_size > 2000) max_pairmap_size = 2000;
		if (max_pairmap_size < 1) max_pairmap_size = 1;
		c->max_pairmap_size = (double) max_pairmap_size * 1000000.0;
		
		if (c->max_pairmap_size > (double) numvals / 2 * c->stage2_fftlen * sizeof (double))
			c->max_pairmap_size = (double) numvals / 2 * c->stage2_fftlen * sizeof (double);
	}

	c->est_numprimes = primes_less_than (C) - primes_less_than (C_start);
	if (gap_start) c->est_numprimes -= primes_less_than (gap_end) - primes_less_than (gap_start);

	if (c->use_poly_D_data) return (best_stage2_impl_internal (B, C_start, C, c->numvals, cost_func, cost_func_data));

	best[0].numvals = 4;
	best[0].efficiency = best_stage2_impl_internal (B, C_start, C, best[0].numvals, cost_func, cost_func_data);
	if (numvals == 4) return (best[0].efficiency);
	best[2].numvals = c->numvals;
	best[2].efficiency = best_stage2_impl_internal (B, C_start, C, best[2].numvals, cost_func, cost_func_data);
	if (numvals == 5) return (best[2].efficiency);
	best[1].numvals = (c->numvals + 4) / 2;
	best[1].efficiency = best_stage2_impl_internal (B, C_start, C, best[1].numvals, cost_func, cost_func_data);

	while (best[2].numvals - best[0].numvals > 2) {
		
		if (best[1].efficiency < best[0].efficiency && best[1].efficiency < best[2].efficiency) {
			if (best[0].efficiency > best[2].efficiency) best[2].efficiency = best[1].efficiency / 2.0;
			else best[0].efficiency = best[1].efficiency / 2.0;
		}
		
		if (best[1].efficiency < best[0].efficiency || best[1].efficiency == best[2].efficiency) {
			ASSERTG (best[1].efficiency >= best[2].efficiency);
			best[2] = best[1];
			best[1].numvals = (best[0].numvals + best[2].numvals) / 2;
			best[1].efficiency = best_stage2_impl_internal (B, C_start, C, best[1].numvals, cost_func, cost_func_data);
		}
		
		else if (best[1].efficiency < best[2].efficiency) {
			ASSERTG (best[1].efficiency >= best[0].efficiency);
			best[0] = best[1];
			best[1].numvals = (best[0].numvals + best[2].numvals) / 2;
			best[1].efficiency = best_stage2_impl_internal (B, C_start, C, best[1].numvals, cost_func, cost_func_data);
		}
		
		else if (best[1].numvals - best[0].numvals > best[2].numvals - best[1].numvals) {	
			midpoint.numvals = (best[0].numvals + best[1].numvals) / 2;
			midpoint.efficiency = best_stage2_impl_internal (B, C_start, C, midpoint.numvals, cost_func, cost_func_data);
			if (midpoint.efficiency > best[1].efficiency || best[1].efficiency == best[2].efficiency) {	
				best[2] = best[1];
				best[1] = midpoint;
			} else {						
				best[0] = midpoint;
			}
		} else {							
			midpoint.numvals = (best[1].numvals + best[2].numvals) / 2;
			midpoint.efficiency = best_stage2_impl_internal (B, C_start, C, midpoint.numvals, cost_func, cost_func_data);
			if (midpoint.efficiency > best[1].efficiency) {		
				best[0] = best[1];
				best[1] = midpoint;
			} else {						
				best[2] = midpoint;
			}
		}
	}

	if (best[2].efficiency > best[1].efficiency) best[1] = best[2];

	return (best_stage2_impl_internal (B, C_start, C, best[1].numvals, cost_func, cost_func_data));
}

int pair_window_size (
	double	bit_length,
	int16_t	*relp_sets)
{

	return (1000);
}

int setN (
	int	thread_num,
	struct work_unit *w,
	giant	*N)		
{
	unsigned long bits, p;
	char	buf[2500];

	bits = (unsigned long) (w->n * log2 (w->b));
	*N = allocgiant ((bits >> 5) + 5);
	if (*N == NULL) return (OutOfMemory (thread_num));

	if (IniGetInt (INI_FILE, "PhiExtensions", 0) &&
	    w->k == 1.0 && w->b == 2 && w->c == -1) {		
		unsigned int i,k,q,knownSmallMers[] = {3, 5, 7, 13, 17, 19, 31, 61, 89, 107, 127, 521, 607, 1279, 2203, 2281, 3217,
						       4253, 4423, 9689, 9941, 11213, 19937, 999999999}; 
		for (i=0; (q=knownSmallMers[i]) < w->n || q*q <= w->n; i++) if ((w->n%q) == 0) {
			giant tmp = allocgiant ((bits >> 5) + 5);
			if (!tmp) return (OutOfMemory (thread_num));
			ultog (1, tmp);
			ultog (1, *N);
			gshiftleft (w->n-w->n/q, *N);
			for (k=2; k < q; k++) {
				gshiftleft (w->n/q, tmp);
				addg (tmp, *N);
			}
			iaddg (1, *N);
			if (q != w->n/q) {
				ultog (w->b, tmp);
				power (tmp, w->n/q);
				iaddg (w->c, tmp);
				divg (tmp, *N);
			}
			free (tmp);
			 
			if (!w->known_factors || !strcmp (w->known_factors, "1")) {
				p = sprintf (buf, "M%lu", w->n/q); if(q != w->n/q) p += sprintf (buf+p, "/M%d", q);
				w->known_factors = (char *) malloc (p+1);
				memcpy (w->known_factors, buf, p+1);
			}
			return (0);
		}
        }

	if (IniGetInt (INI_FILE, "PhiExtensions", 0) &&
	    w->k == 1.0 && labs(w->c) == 1 && (w->n%3) == 0) {		
		giant	tmp = allocgiant ((bits >> 5) + 5);
		if (tmp == NULL) return (OutOfMemory (thread_num));
		ultog (w->b, tmp);
		power (tmp, w->n/3);
		gtog (tmp, *N);
		squareg (*N);
		if (w->c == 1) subg (tmp, *N); else addg (tmp, *N);
		iaddg (1, *N);
		free (tmp);
		 
		if (!w->known_factors) {
			p = sprintf (buf, "(%lu^%lu%+ld)", w->b, w->n/3, w->c);
			w->known_factors = (char *) malloc (p+1);
			memcpy (w->known_factors, buf, p+1);
		}
		return (0);
	}

	ultog (w->b, *N);
	power (*N, w->n);
	dblmulg (w->k, *N);
	iaddg (w->c, *N);

	if (w->known_factors != NULL) {
		mpz_t	__N, __f, __r;

		mpz_init (__N);
		mpz_init (__f);
		mpz_init (__r);
		gtompz (*N, __N);

		for (char *p = w->known_factors; ; ) {
			char	*comma, facstr[500];
			int	res;

			comma = strchr (p, ',');
			if (comma != NULL) *comma = 0;
			strcpy (facstr, p);
			if (comma != NULL) *comma = ',';
			res = mpz_set_str (__f, facstr, 10);
			if (res < 0 || mpz_cmp_ui (__f, 1) <= 0) {
				char	kbnc[80];
				gw_as_string (kbnc, w->k, w->b, w->n, w->c);
				sprintf (buf, "Error parsing known factors of %s near: '%s'\n", kbnc, facstr);
				OutputStr (thread_num, buf);
				mpz_clear (__N);
				mpz_clear (__f);
				mpz_clear (__r);
				deleteWorkToDoLine (thread_num, w, FALSE);
				return (STOP_ABORT);
			}

			mpz_fdiv_qr (__N, __r, __N, __f);	
			if (mpz_sgn (__r) != 0) {		
				char	kbnc[80];
				gw_as_string (kbnc, w->k, w->b, w->n, w->c);
				sprintf (buf, "%s does not divide %s\n", facstr, kbnc);
				OutputBoth (thread_num, buf);
				mpz_clear (__N);
				mpz_clear (__f);
				mpz_clear (__r);
				deleteWorkToDoLine (thread_num, w, FALSE);
				return (STOP_ABORT);
			}

			if (comma == NULL) break;
			p = comma + 1;
		}

		mpztog (__N, *N);
		mpz_clear (__N);
		mpz_clear (__f);
		mpz_clear (__r);
	}

	return (0);
}

int testFactor (
	gwhandle *gwdata,
	struct work_unit *w,
	giant	f)		
{
	giant	tmp;
	int	divides_ok;

	tmp = popg (&gwdata->gdata, f->sign + 5);	
	itog (w->b, tmp);
	powermod (tmp, w->n, f);
	dblmulg (w->k, tmp);
	iaddg (w->c, tmp);
	modgi (&gwdata->gdata, f, tmp);
	divides_ok = isZero (tmp);
	pushg (&gwdata->gdata, 1);
	if (!divides_ok) return (FALSE);

	if (QA_IN_PROGRESS) {
		tmp = popg (&gwdata->gdata, f->sign + 5);
		gtog (f, tmp);
		modg (QA_FACTOR, tmp);
		divides_ok = isZero (tmp);
		pushg (&gwdata->gdata, 1);
		if (!divides_ok) {
			char	buf[200];
			strcpy (buf, "ERROR: Factor not found. Expected ");
			gtoc (QA_FACTOR, buf+strlen(buf), 150);
			strcat (buf, "\n");
			OutputBoth (MAIN_THREAD_NUM, buf);
		}
	}

	return (TRUE);
}

char *addNewKnownFactor (
	struct work_unit *w,		
	const char *fac)		
{
	char	*new_known_factors;
	if (fac == NULL) {		
		new_known_factors = (char *) malloc (strlen (w->known_factors) + 1);
		strcpy (new_known_factors, w->known_factors);
	} else if (w->known_factors == NULL) {
		new_known_factors = (char *) malloc (strlen (fac) + 1);
		strcpy (new_known_factors, fac);
	} else {
		new_known_factors = (char *) malloc (strlen (w->known_factors) + 1 + strlen (fac) + 1);
		sprintf (new_known_factors, "%s,%s", w->known_factors, fac);
	}
	return (new_known_factors);
}

void generatePRPWorkUnit (
	struct work_unit *oldw,		
	const char *fac,		
	struct work_unit *neww)		
{
	
	*neww = *oldw;
	
	neww->work_type = WORK_PRP;
	neww->assignment_uid[0] = 0;
	neww->prp_base = 3;
	neww->prp_residue_type = 5;
	neww->prp_dblchk = FALSE;
	neww->sieve_depth = 99;
	neww->tests_saved = 0;
	neww->known_factors = addNewKnownFactor (oldw, fac);
	neww->stage[0] = 0;
	neww->pct_complete = 0.0;
	neww->ra_failed = !IniGetInt (INI_FILE, "RegisterPrpAssignment", 0);		
}

int gcd (
	int	thread_num,
	giant	gg,
	giant	N,		
	giant	*factor)	
{
	mpz_t	a, b;

	*factor = NULL;

	mpz_init (a);
	mpz_init (b);
	gtompz (gg, a);
	gtompz (N, b);
	mpz_gcd (a, a, b);

	if (mpz_cmp_ui (a, 1) && mpz_cmp (a, b)) {
		*factor = allocgiant ((int) divide_rounding_up (mpz_sizeinbase (a, 2), 32));
		if (*factor == NULL) goto oom;
		mpztog (a, *factor);
	}

	mpz_clear (a);
	mpz_clear (b);
	return (0);

oom:	return (OutOfMemory (thread_num));
}

int gcd (
	gwhandle *gwdata,
	int	thread_num,
	gwnum	gg,
	giant	N,		
	giant	*factor)	
{
	giant	v;
	int	rc;

	v = popg (&gwdata->gdata, ((int) gwdata->bit_length >> 5) + 10);
	if (v == NULL) goto oom;
	gwunfft (gwdata, gg, gg);		
	if (gwtogiant (gwdata, gg, v)) {	
		pushg (&gwdata->gdata, 1);
		return (0);
	}

	rc = gcd (thread_num, v, N, factor);
	pushg (&gwdata->gdata, 1);
	return (rc);

oom:	return (OutOfMemory (thread_num));
}

int isProbablePrime (
	gwhandle *gwdata,
	giant	N)
{
	int	i, j, len, retval;
	gwnum	t1, t2;
	giant	x;

	if (isone (N)) return (TRUE);

	retval = TRUE;		
	t1 = gwalloc (gwdata);
	len = bitlen (N);
	for (i = 3; retval && i <= 7; i += 2) {
		t2 = gwalloc (gwdata);
		dbltogw (gwdata, (double) 1.0, t1);
		dbltogw (gwdata, (double) i, t2);
		gwfft (gwdata, t2, t2);
		for (j = 1; j <= len; j++) {
			gwsquare2 (gwdata, t1, t1, 0);
			if (bitval (N, len-j)) gwmul3 (gwdata, t2, t1, t1, 0);
		}
		gwfree (gwdata, t2);
		x = popg (&gwdata->gdata, ((int) gwdata->bit_length >> 5) + 10);
		if (gwtogiant (gwdata, t1, x)) retval = FALSE;	
		else {
			modgi (&gwdata->gdata, N, x);
			iaddg (-i, x);
			if (!isZero (x)) retval = FALSE;	
		}
		pushg (&gwdata->gdata, 1);
	}
	gwfree (gwdata, t1);
	return (retval);
}

#define kruppa_power(B1)		(1.96617 - 0.06781 * _log10((double)(B1)))
#define kruppa_adjust(B2,B1)		(0.11343 + 0.88657 * pow (_log10((double)(B2) / (double)(B1)) / 2.0, kruppa_power (B1)))
#define kruppa_unadjust(worth,B1)	(uint64_t) round((B1) * pow (10.0, pow (((worth) - 0.11343) / 0.88657, 1.0 / kruppa_power (B1)) * 2.0))

uint64_t calc_new_first_relocatable (
	int	D,			
	uint64_t C_done)		
{
	int	D_index = map_D_to_index (D);
	return (divide_rounding_up (C_done, D_data[D_index].first_missing_prime));
}

#define POOL_3MULT		1	
#define POOL_3POINT44MULT	2	
#define POOL_3POINT57MULT	3	
#define POOL_N_SQUARED		4	

#define ECM_STATE_STAGE1_INIT	0	
#define ECM_STATE_STAGE1	1	
#define ECM_STATE_MIDSTAGE	2	
#define ECM_STATE_STAGE2	3	
#define ECM_STATE_GCD		4	

#define ECM_STAGE2_PAIRING	0	
#define ECM_STAGE2_POLYMULT	1	

typedef struct {
	gwhandle gwdata;	
	int	thread_num;	
	int	stage1_threads;	
	int	stage2_threads;	
	struct work_unit *w;	
	FILE	*gmp_ecm_file;	
	uint32_t curve;		
	uint32_t state;		
	uint64_t sigma;		
	uint64_t B;		
	uint64_t C;		
	giant	N;		
	char	*N_short_string_rep; 
	char	*N_full_string_rep; 
	giant	factor;		
	uint32_t sigma_type;	
	bool	montg_stage1;	
	bool	optimal_B2;	
	uint64_t average_B2;	
	unsigned long stage1_fftlen; 

	gwnum	Ad4;		
	gwnum	ed_a;		
	gwnum	ed_d;		
	gwnum	ed_half;	

	readSaveFileState read_save_file_state;	
	writeSaveFileState write_save_file_state; 
	void	*sieve_info;	
	uint64_t stage1_prime;	

	mpz_t	stage1_exp;	
	char	*NAF_codes;	
	bool	stage1_exp_initialized;
	uint64_t stage1_start_prime; 
	uint32_t stage1_exp_buffer_size; 
	uint32_t stage1_bitnum;	
	uint32_t NAF_dictionary_size; 
	struct ed *NAF_dictionary; 
	gwarray	NAF_gwnums;	

	struct xz xz;		
	struct ed dict_start;	
	struct ed e;		
	gwnum	gg;		

	int32_t	pool_type;	
	uint32_t pool_count;	
	uint32_t poolz_count;	
	gwnum	pool_modinv_value;
	gwnum	*pool_values;	
	gwnum	*poolz_values;	
	unsigned long modinv_count; 

	int32_t	stage2_type;	
	uint64_t stage2_numvals;
	uint64_t C_done;	
	uint32_t D;		
	uint32_t E;		
	uint32_t numrels;	
	uint64_t B2_start;	
	uint64_t numDsections;	
	uint64_t Dsection;	
	uint64_t first_relocatable; 
	uint64_t last_relocatable; 
	double	est_stage2_stage1_ratio; 
	double	pct_mem_to_use;	

	struct xz Qm, Qprevm, QD; 
	struct xz QD_Eover2;	
	int	Qm_state;	
	gwnum	*mQx;		
	uint32_t mQx_count;	
	uint32_t mQx_retcount;	
	giant	Ad4_binary;	
	giant	Qx_binary;	
	giant	Qz_binary;	
	giant	gg_binary;	

	int32_t	TWO_FFT_STAGE2;	
	uint32_t totrels;	
	int32_t	relp;		
	int16_t relp_sets[32];	
	gwnum	*nQx;		
	uint64_t max_pairmap_Dsections;	
	uint8_t	*pairmap;	
	uint64_t pairmap_size;	
	uint8_t *pairmap_ptr;	

	int32_t	required_missing; 
				 
	pmhandle polydata;	
	uint64_t poly_size;	
	gwarray	polyF;		
	gwarray	polyR;		
	gwarray	polyGH;		
	gwarray	polyGaux;	
	uint64_t polyGaux_size;	
	bool	use_preallocated_norm_temps; 
	gwnum	norm_pool_temps[5]; 
	int	norm_pool_temps_count; 
	int	Ftree_polys_in_mem; 
	gwarray polyFtree[40];	
	gwarray	Ftree;		

	int	helper_work;	
	gwarray poly1;		
} ecmhandle;

void normalize_pool_term (ecmhandle *);
void mQ_term (ecmhandle *);
void NAF_dictionary_free (ecmhandle *);

void ecm_mini_cleanup (
	ecmhandle *ecmdata)
{
	normalize_pool_term (ecmdata);
	free (ecmdata->mQx), ecmdata->mQx = NULL;
	free (ecmdata->Ad4_binary), ecmdata->Ad4_binary = NULL;
	free (ecmdata->Qx_binary), ecmdata->Qx_binary = NULL;
	free (ecmdata->Qz_binary), ecmdata->Qz_binary = NULL;
	free (ecmdata->gg_binary), ecmdata->gg_binary = NULL;
	if (ecmdata->stage2_type != ECM_STAGE2_POLYMULT) free (ecmdata->nQx);
	ecmdata->nQx = NULL;
	free (ecmdata->pairmap), ecmdata->pairmap = NULL;
	free (ecmdata->factor), ecmdata->factor = NULL;
	free (ecmdata->Ftree), ecmdata->Ftree = NULL;
	end_sieve (ecmdata->sieve_info), ecmdata->sieve_info = NULL;
	polymult_done (&ecmdata->polydata);
	if (ecmdata->stage1_exp_initialized) mpz_clear (ecmdata->stage1_exp), ecmdata->stage1_exp_initialized = FALSE;
	NAF_dictionary_free (ecmdata);
	gwfreeall (&ecmdata->gwdata);
	memset (&ecmdata->xz, 0, sizeof (ecmdata->xz));
	memset (&ecmdata->e, 0, sizeof (ecmdata->e));
	ecmdata->gg = NULL;
	ecmdata->Ad4 = NULL;
	ecmdata->ed_a = NULL;
	ecmdata->ed_d = NULL;
	ecmdata->ed_half = NULL;
}

void ecm_cleanup (
	ecmhandle *ecmdata)
{
	ecm_mini_cleanup (ecmdata);
	free (ecmdata->N), ecmdata->N = NULL;
	if (ecmdata->N_short_string_rep != gwmodulo_as_string (&ecmdata->gwdata)) free (ecmdata->N_short_string_rep), ecmdata->N_short_string_rep = NULL;
	if (ecmdata->N_full_string_rep != gwmodulo_as_string (&ecmdata->gwdata)) free (ecmdata->N_full_string_rep), ecmdata->N_full_string_rep = NULL;
	gwdone (&ecmdata->gwdata);
	if (ecmdata->gmp_ecm_file != NULL) fclose (ecmdata->gmp_ecm_file), ecmdata->gmp_ecm_file = NULL;
}

bool ws_alloc (			
	ecmhandle *ecmdata,
	struct ws *arg)
{
	arg->x = gwalloc (&ecmdata->gwdata);
	if (arg->x == NULL) return (FALSE);
	arg->y = gwalloc (&ecmdata->gwdata);
	if (arg->y == NULL) return (FALSE);
	arg->z = gwalloc (&ecmdata->gwdata);
	if (arg->z == NULL) return (FALSE);
	return (TRUE);
}

void ws_free (
	ecmhandle *ecmdata,
	struct ws *arg)
{
	gwfree (&ecmdata->gwdata, arg->x), arg->x = NULL;
	gwfree (&ecmdata->gwdata, arg->y), arg->y = NULL;
	gwfree (&ecmdata->gwdata, arg->z), arg->z = NULL;
}

void ws_check (
	ecmhandle *ecmdata,
	struct ws *in)
{
#ifdef GDEBUG
	gwnum	t0, t1, t2;
	int	a = -8;
	int	b = -32;

	t0 = gwalloc (&ecmdata->gwdata);
	t1 = gwalloc (&ecmdata->gwdata);
	t2 = gwalloc (&ecmdata->gwdata);

	gwsquare2 (&ecmdata->gwdata, in->y, t0, GWMUL_PRESERVE_S1);				
	gwmul3 (&ecmdata->gwdata, t0, in->z, t0, GWMUL_PRESERVE_S2);				
	gwsquare2 (&ecmdata->gwdata, in->x, t1, GWMUL_PRESERVE_S1);				
	gwmul3 (&ecmdata->gwdata, t1, in->x, t1, GWMUL_PRESERVE_S2);				
	gwmul3 (&ecmdata->gwdata, in->x, in->z, t2, GWMUL_PRESERVE_S1 | GWMUL_PRESERVE_S2);	
	gwmul3 (&ecmdata->gwdata, t2, in->z, t2, GWMUL_PRESERVE_S2);				
	gwsmallmul (&ecmdata->gwdata, a, t2);							
	gwadd3 (&ecmdata->gwdata, t1, t2, t1);							
	gwsquare2 (&ecmdata->gwdata, in->z, t2, GWMUL_PRESERVE_S1);				
	gwmul3 (&ecmdata->gwdata, t2, in->z, t2, GWMUL_PRESERVE_S2);				
	gwsmallmul (&ecmdata->gwdata, b, t2);							
	gwadd3 (&ecmdata->gwdata, t1, t2, t1);							
	gwsub3 (&ecmdata->gwdata, t0, t1, t0);							

	giant tmp = popg (&ecmdata->gwdata.gdata, ((unsigned long) ecmdata->gwdata.bit_length >> 5) + 5);
	gwtogiant (&ecmdata->gwdata, t0, tmp);
	ASSERTG (tmp->sign == 0);
	pushg (&ecmdata->gwdata.gdata, 1);

	gwfree (&ecmdata->gwdata, t0);
	gwfree (&ecmdata->gwdata, t1);
	gwfree (&ecmdata->gwdata, t2);
#endif
}

void ws_add (
	ecmhandle *ecmdata,
	struct ws *in0,
	struct ws *in1,
	struct ws *out)
{

	gwnum	T0, T1, T, U0, U1, U, U2, V, W, t0, U3;
	T0 = gwalloc (&ecmdata->gwdata);
	T1 = gwalloc (&ecmdata->gwdata);
	U0 = gwalloc (&ecmdata->gwdata);
	U1 = gwalloc (&ecmdata->gwdata);
	U = gwalloc (&ecmdata->gwdata);
	U2 = gwalloc (&ecmdata->gwdata);
	V = gwalloc (&ecmdata->gwdata);
	W = gwalloc (&ecmdata->gwdata);
	t0 = gwalloc (&ecmdata->gwdata);

	gwmul3 (&ecmdata->gwdata, in0->y, in1->z, T0, 0);		
	gwmul3 (&ecmdata->gwdata, in1->y, in0->z, T1, 0);		
	gwsub3 (&ecmdata->gwdata, T0, T1, T = T1);			

	gwmul3 (&ecmdata->gwdata, in0->x, in1->z, U0, 0);		
	gwmul3 (&ecmdata->gwdata, in1->x, in0->z, U1, 0);		
	gwsub3 (&ecmdata->gwdata, U0, U1, U);				

	gwsquare2 (&ecmdata->gwdata, U, U2, 0);				

	gwmul3 (&ecmdata->gwdata, in0->z, in1->z, V, 0);		

	gwmul3 (&ecmdata->gwdata, T, T, W, 0);				
	gwmul3 (&ecmdata->gwdata, W, V, W, 0);				
	gwadd3 (&ecmdata->gwdata, U0, U1, t0);				
	gwmul3 (&ecmdata->gwdata, U2, t0, t0, 0);			
	gwsub3 (&ecmdata->gwdata, W, t0, W);				

	gwmul3 (&ecmdata->gwdata, U, U2, U3 = U1, 0);			

	gwmul3 (&ecmdata->gwdata, U, W, out->x, 0);			

	gwmul3 (&ecmdata->gwdata, T0, U3, out->y, 0);			
	gwmul3 (&ecmdata->gwdata, U0, U2, t0, 0);			
	gwsub3 (&ecmdata->gwdata, t0, W, t0);				
	gwmul3 (&ecmdata->gwdata, T, t0, t0, 0);			
	gwsub3 (&ecmdata->gwdata, t0, out->y, out->y);			

	gwmul3 (&ecmdata->gwdata, U3, V, out->z, 0);			

	gwfree (&ecmdata->gwdata, T0);
	gwfree (&ecmdata->gwdata, T1);
	gwfree (&ecmdata->gwdata, U0);
	gwfree (&ecmdata->gwdata, U1);
	gwfree (&ecmdata->gwdata, U);
	gwfree (&ecmdata->gwdata, U2);
	gwfree (&ecmdata->gwdata, V);
	gwfree (&ecmdata->gwdata, W);
	gwfree (&ecmdata->gwdata, t0);
}

void ws_dbl (
	ecmhandle *ecmdata,
	struct ws *in,
	struct ws *out)
{
	int	a = -8;

	gwnum	T, U, V, W, t0;
	T = gwalloc (&ecmdata->gwdata);
	U = gwalloc (&ecmdata->gwdata);
	V = gwalloc (&ecmdata->gwdata);
	W = gwalloc (&ecmdata->gwdata);
	t0 = gwalloc (&ecmdata->gwdata);

	gwsetmulbyconst (&ecmdata->gwdata, 3);
	gwsquare2 (&ecmdata->gwdata, in->x, T, GWMUL_MULBYCONST);				
	gwsquare2 (&ecmdata->gwdata, in->z, t0, 0);						
	gwsmallmul (&ecmdata->gwdata, a, t0);							
	gwadd3 (&ecmdata->gwdata, T, t0, T);							

	gwsetmulbyconst (&ecmdata->gwdata, 2);
	gwmul3 (&ecmdata->gwdata, in->y, in->z, U, GWMUL_MULBYCONST);				

	gwmul3 (&ecmdata->gwdata, in->x, in->y, V, GWMUL_MULBYCONST);				
	gwmul3 (&ecmdata->gwdata, V, U, V, 0);							

	gwsquare2 (&ecmdata->gwdata, T, W, 0);							
	gwsub3 (&ecmdata->gwdata, W, V, W);							
	gwsub3 (&ecmdata->gwdata, W, V, W);							

	gwmul3 (&ecmdata->gwdata, U, W, out->x, 0);						

	gwmul3 (&ecmdata->gwdata, U, in->y, out->y, 0);						
	gwsquare2 (&ecmdata->gwdata, out->y, out->y, GWMUL_MULBYCONST);				
	gwsub3 (&ecmdata->gwdata, V, W, t0);							
	gwmul3 (&ecmdata->gwdata, T, t0, t0, 0);						
	gwsub3 (&ecmdata->gwdata, t0, out->y, out->y);						

	gwsquare2 (&ecmdata->gwdata, U, out->z, 0);						
	gwmul3 (&ecmdata->gwdata, out->z, U, out->z, 0);					

	gwfree (&ecmdata->gwdata, T);
	gwfree (&ecmdata->gwdata, U);
	gwfree (&ecmdata->gwdata, V);
	gwfree (&ecmdata->gwdata, W);
	gwfree (&ecmdata->gwdata, t0);
}

void ws_mul (
	ecmhandle *ecmdata,
	struct ws *arg,
	uint64_t n)
{
	struct ws orig;

	ws_alloc (ecmdata, &orig);
	gwcopy (&ecmdata->gwdata, arg->x, orig.x);
	gwcopy (&ecmdata->gwdata, arg->y, orig.y);
	gwcopy (&ecmdata->gwdata, arg->z, orig.z);

	uint64_t current_bit;
	for (current_bit = 0x8000000000000000ULL; n < current_bit; current_bit >>= 1);

	for (current_bit >>= 1; current_bit; current_bit >>= 1) {
		ws_dbl (ecmdata, arg, arg);
		if (n & current_bit) ws_add (ecmdata, arg, &orig, arg);
	}
	ws_check (ecmdata, arg);

	ws_free (ecmdata, &orig);
}

#define ed_swap(a,b)	{ struct ed t; t = a; a = b; b = t; }

bool ed_alloc (			
	ecmhandle *ecmdata,
	struct ed *arg)
{
	arg->x = gwalloc (&ecmdata->gwdata);
	if (arg->x == NULL) return (FALSE);
	arg->y = gwalloc (&ecmdata->gwdata);
	if (arg->y == NULL) return (FALSE);
	arg->z = gwalloc (&ecmdata->gwdata);
	if (arg->z == NULL) return (FALSE);
	arg->t = NULL;
	arg->alternate_format = FALSE;
	return (TRUE);
}

void ed_free (
	ecmhandle *ecmdata,
	struct ed *arg)
{
	gwfree (&ecmdata->gwdata, arg->x), arg->x = NULL;
	gwfree (&ecmdata->gwdata, arg->y), arg->y = NULL;
	gwfree (&ecmdata->gwdata, arg->z), arg->z = NULL;
	gwfree (&ecmdata->gwdata, arg->t), arg->t = NULL;
}

bool ed_check (				
	ecmhandle *ecmdata,
	struct ed *in)
{
	gwnum	t0, t1, t2, t3;
	bool	result;

	ASSERTG (!in->alternate_format);

	t0 = gwalloc (&ecmdata->gwdata);
	t1 = gwalloc (&ecmdata->gwdata);
	t2 = gwalloc (&ecmdata->gwdata);
	t3 = gwalloc (&ecmdata->gwdata);

	gwsquare2 (&ecmdata->gwdata, in->x, t0, GWMUL_PRESERVE_S1);			
	gwsquare2 (&ecmdata->gwdata, in->y, t1, GWMUL_PRESERVE_S1);			
	if (in->z != NULL) gwsquare2 (&ecmdata->gwdata, in->z, t2, GWMUL_PRESERVE_S1);	
	else dbltogw (&ecmdata->gwdata, 1.0, t2);
	gwmul3 (&ecmdata->gwdata, t0, t1, t3, 0);					
	gwmul3 (&ecmdata->gwdata, t3, ecmdata->ed_d, t3, 0);				
	if (ecmdata->ed_a != NULL) gwmul3 (&ecmdata->gwdata, t0, ecmdata->ed_a, t0, 0);	
	gwadd3 (&ecmdata->gwdata, t0, t1, t0);						
	gwmul3 (&ecmdata->gwdata, t0, t2, t0, 0);					
	gwsquare2 (&ecmdata->gwdata, t2, t2, 0);					
	gwadd3 (&ecmdata->gwdata, t2, t3, t2);						

	gwsub3 (&ecmdata->gwdata, t0, t2, t0);						
	giant tmp = popg (&ecmdata->gwdata.gdata, ((unsigned long) ecmdata->gwdata.bit_length >> 5) + 5);
	gwtogiant (&ecmdata->gwdata, t0, tmp);
	modg (ecmdata->N, tmp);
	result = (tmp->sign == 0);
	pushg (&ecmdata->gwdata.gdata, 1);

	gwfree (&ecmdata->gwdata, t0);
	gwfree (&ecmdata->gwdata, t1);
	gwfree (&ecmdata->gwdata, t2);
	gwfree (&ecmdata->gwdata, t3);

	return (result);
}

void create_ed_half (
	ecmhandle *ecmdata)
{
	ecmdata->ed_half = gwalloc (&ecmdata->gwdata);
	gshiftright (1, ecmdata->N);
	gianttogw (&ecmdata->gwdata, ecmdata->N, ecmdata->ed_half);
	gwsmalladd (&ecmdata->gwdata, 1, ecmdata->ed_half);
	gwfft (&ecmdata->gwdata, ecmdata->ed_half, ecmdata->ed_half);
	gshiftleft (1, ecmdata->N);		
	iaddg (1, ecmdata->N);
}

#define	ED_XYZ			0x1	
#define ED_RESULT_FOR_ADD	0x2	
#define ED_RESULT_FOR_DBL	0x4	
#define ED_STARTNEXTFFT		0x8	
#define ED_FFT_S1		0x10	
#define ED_FFT_S1Z		0x20	
#define ED_FFT_S1T		0x40	
#define ED_SUBTRACT		0x80	
#define ED_FORCE_EXTEND		0x100	
#define ED_NO_GWALLOC		0x200	

void ed_extend (
	ecmhandle *ecmdata,
	struct ed *e,
	int	options)
{
	int mul_options = (options & ED_STARTNEXTFFT) ? GWMUL_STARTNEXTFFT : 0;

	ASSERTG (!e->alternate_format);

	if (!(options & ED_FORCE_EXTEND) && e->t != NULL) return;
	
	if (!(options & ED_FORCE_EXTEND)) e->t = gwalloc (&ecmdata->gwdata);
	
	gwmul3 (&ecmdata->gwdata, e->x, e->y, e->t, GWMUL_FFT_S12 | mul_options);		
	if (e->z != NULL) {
		gwmul3 (&ecmdata->gwdata, e->x, e->z, e->x, GWMUL_FFT_S2 | mul_options);	
		gwmul3 (&ecmdata->gwdata, e->y, e->z, e->y, GWMUL_FFT_S2 | mul_options);	
		gwsquare2 (&ecmdata->gwdata, e->z, e->z, mul_options);				
	}
}

void ed_add (
	ecmhandle *ecmdata,
	struct ed *in1,			
	struct ed *in2,			
	struct ed *out,			
	int	options)
{
	gwnum	tmp1, tmp2;
	bool	safe11 = mul_safe (&ecmdata->gwdata, 1, 1);					
	int	subtract = options & ED_SUBTRACT;
	int	extended = options & ED_RESULT_FOR_ADD;
	int	mul_S1Z_options = (options & ED_FFT_S1Z) ? GWMUL_FFT_S1 : 0;
	int	mul_S1T_options = (options & ED_FFT_S1T) ? GWMUL_FFT_S1 : 0;
	int	mul_options = (options & ED_STARTNEXTFFT) ? GWMUL_STARTNEXTFFT : 0;

	ASSERTG (!in1->alternate_format && !in2->alternate_format);
	
	ASSERTG (ecmdata->ed_a == NULL || FFT_state (ecmdata->ed_a) == FULLY_FFTed);
	ASSERTG (!(options & ED_NO_GWALLOC) || (in1 != out && in1->t != NULL && out->t != NULL));

	if (in1->t == NULL) ed_extend (ecmdata, in1, ED_STARTNEXTFFT);
	if (in2->t == NULL) ed_extend (ecmdata, in2, ED_STARTNEXTFFT);

	gwnum in1z = in1->z;						
	gwnum in2z = in2->z;						
	if (out->z == NULL) out->z = gwalloc (&ecmdata->gwdata);	
	if (out->t != NULL) tmp1 = out->t;				
	else tmp1 = gwalloc (&ecmdata->gwdata);
	if (options & ED_NO_GWALLOC) tmp2 = in1->t;
	else tmp2 = gwalloc (&ecmdata->gwdata);

	gwnum D;
	if (in2z == NULL) D = in1->t;
	else gwmul3 (&ecmdata->gwdata, in1->t, in2z, D = tmp2, mul_S1T_options | GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT_IF(safe11 || !extended)); 
	
	gwnum C;
	if (in1z == NULL) C = in2->t;
	else gwmul3 (&ecmdata->gwdata, in1z, in2->t, C = out->z, mul_S1Z_options | GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT_IF(safe11 || !extended)); 
	gwaddsub4o (&ecmdata->gwdata, D, C, tmp2, out->z, GWADD_DELAYNORM_IF(safe11 || !extended));				
        if (subtract) {
		gwswap (tmp2, out->z);			
		gwmulmuladd5 (&ecmdata->gwdata, in1->x, in2->y, in1->y, in2->x, tmp1, GWMUL_FFT_S1234 | GWMUL_STARTNEXTFFT);	
		if (ecmdata->ed_a == NULL)
			gwmulmulsub5 (&ecmdata->gwdata, in1->y, in2->y, in1->x, in2->x, out->y, GWMUL_STARTNEXTFFT);		
		else {
			gwmul3 (&ecmdata->gwdata, in1->x, in2->x, out->x, GWMUL_STARTNEXTFFT);
			gwmulmulsub5 (&ecmdata->gwdata, in1->y, in2->y, ecmdata->ed_a, out->x, out->y, GWMUL_STARTNEXTFFT);	
		}
        } else {
		gwmulmulsub5 (&ecmdata->gwdata, in1->x, in2->y, in1->y, in2->x, tmp1, GWMUL_FFT_S1234 | GWMUL_STARTNEXTFFT);	
		if (ecmdata->ed_a == NULL)
			gwmulmuladd5 (&ecmdata->gwdata, in1->y, in2->y, in1->x, in2->x, out->y, GWMUL_STARTNEXTFFT);		
		else {
			gwmul3 (&ecmdata->gwdata, in1->x, in2->x, out->x, GWMUL_FFT_S1 | GWMUL_STARTNEXTFFT);
			gwmulmuladd5 (&ecmdata->gwdata, in1->y, in2->y, ecmdata->ed_a, out->x, out->y, GWMUL_STARTNEXTFFT);	
		}
	}
	if (!extended) {
		gwmul3 (&ecmdata->gwdata, tmp2, tmp1, tmp2, GWMUL_FFT_S2 | mul_options);			
		gwmul3 (&ecmdata->gwdata, tmp1, out->y, tmp1, GWMUL_FFT_S2 | mul_options);			
		gwmul3 (&ecmdata->gwdata, out->y, out->z, out->z, mul_options);					
		gwswap (out->z, out->y);
		gwswap (tmp2, out->x);
		gwswap (tmp1, out->z);
		out->t = NULL;
	} else {
		gwmul3 (&ecmdata->gwdata, tmp2, tmp1, out->x, GWMUL_FFT_S12 | mul_options);			
		gwmul3 (&ecmdata->gwdata, tmp1, out->y, tmp1, GWMUL_FFT_S2 | mul_options);			
		gwmul3 (&ecmdata->gwdata, out->y, out->z, out->y, GWMUL_FFT_S2 | mul_options);			
		gwmul3 (&ecmdata->gwdata, tmp2, out->z, tmp2, mul_options);					
		gwswap (tmp1, out->z);
		out->t = tmp2, tmp2 = NULL;
	}

	if (options & ED_NO_GWALLOC) {
		if (!extended) out->t = tmp2;
		in1->t = tmp1;
	} else {
		gwfree (&ecmdata->gwdata, tmp1);
		gwfree (&ecmdata->gwdata, tmp2);
	}
}

void ed_dbl (
	ecmhandle *ecmdata,
	struct ed *in,			
	struct ed *out,			
	int	options)
{
	gwnum	tmp1, tmp2;
	int	extended = options & ED_RESULT_FOR_ADD;
	int	mul_S1_options = (options & ED_FFT_S1) ? GWMUL_FFT_S1 : 0;
	int	mul_options = (options & ED_STARTNEXTFFT) ? GWMUL_STARTNEXTFFT : 0;
	int	can_alternate_format = !(options & ED_XYZ) && !extended;

	ASSERTG (ecmdata->ed_a == NULL || FFT_state (ecmdata->ed_a) == FULLY_FFTed);
	ASSERTG (!(options & ED_NO_GWALLOC) || (in != out && in->t != NULL && out->t != NULL && extended));

static	int algo5 = 2;		
static	int algo5a = 2;		
static	int algo7 = 2;		
	if (algo5 == 2) {
		algo5 = IniGetInt (INI_FILE, "EdDblAlgo5", 1);		
		algo5a = IniGetInt (INI_FILE, "EdDblAlgo5a", 1);	
		algo7 = IniGetInt (INI_FILE, "EdDblAlgo7", 1);		
	}

	gwnum inz = in->z;						
	if (out->z == NULL) out->z = gwalloc (&ecmdata->gwdata);	
	if (out->t != NULL) tmp1 = out->t, out->t = NULL;
	else tmp1 = gwalloc (&ecmdata->gwdata);
	if (options & ED_NO_GWALLOC) tmp2 = in->t;
	else tmp2 = gwalloc (&ecmdata->gwdata);
	gwsetmulbyconst (&ecmdata->gwdata, 2);

	if (algo5 && squaresquareadd_safe (&ecmdata->gwdata, 0, 0, 0, 0)) {
	    if (inz == NULL) dbltogw (&ecmdata->gwdata, 2.0, out->z);
	    else gwsquare2 (&ecmdata->gwdata, inz, out->z, mul_S1_options | GWMUL_MULBYCONST | GWMUL_STARTNEXTFFT);	
	    if (ecmdata->ed_a == NULL) {			
		gwsquare2 (&ecmdata->gwdata, in->x, tmp2, GWMUL_FFT_S1 | GWMUL_MULBYCONST | GWMUL_STARTNEXTFFT);	
		gwmul3 (&ecmdata->gwdata, in->x, in->y, tmp1, GWMUL_FFT_S2 | GWMUL_MULBYCONST | GWMUL_STARTNEXTFFT);	
		gwmulmuladd5 (&ecmdata->gwdata, in->y, in->y, in->x, in->x, out->y, GWMUL_STARTNEXTFFT);		
	    } else {						
		gwsquare2 (&ecmdata->gwdata, in->x, tmp2, GWMUL_FFT_S1 | GWMUL_STARTNEXTFFT);				
		gwmul3 (&ecmdata->gwdata, in->x, in->y, out->x, GWMUL_FFT_S2 | GWMUL_MULBYCONST | GWMUL_STARTNEXTFFT);	
		gwmulmuladd5 (&ecmdata->gwdata, ecmdata->ed_a, tmp2, in->y, in->y, out->y, GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT); 
		gwmul3 (&ecmdata->gwdata, ecmdata->ed_a, tmp2, tmp2, GWMUL_MULBYCONST | GWMUL_STARTNEXTFFT);		
		gwswap (tmp1, out->x);
	    }
	    if (!extended) {
		gwsubmul4 (&ecmdata->gwdata, out->y, tmp2, out->y, tmp2, GWMUL_FFT_S13 | mul_options);			
		gwsubmul4 (&ecmdata->gwdata, out->z, out->y, out->y, out->x, GWMUL_FFT_S1 | mul_options);		
		gwsubmul4 (&ecmdata->gwdata, out->z, out->y, tmp1, tmp1, mul_options);					
		gwswap (tmp2, out->y);
		gwswap (out->x, out->z);
		gwswap (tmp1, out->x);
	    } else {
		gwsubmul4 (&ecmdata->gwdata, out->y, tmp2, out->y, out->x, GWMUL_FFT_S123 | mul_options);		
		gwsubmul4 (&ecmdata->gwdata, out->y, tmp2, tmp1, tmp2, GWMUL_FFT_S3 | mul_options);			
		gwsubmul4 (&ecmdata->gwdata, out->z, out->y, tmp1, tmp1, GWMUL_FFT_S1 | mul_options);			
		gwsubmul4 (&ecmdata->gwdata, out->z, out->y, out->y, out->z, mul_options);				
		gwswap (out->x, out->y);
		gwswap (tmp2, out->t);
		gwswap (tmp1, out->x);
	    }
	}

	else if (algo5a && squaremuladd_safe (&ecmdata->gwdata, 0, 0, 0, 0)) {
	    if (ecmdata->ed_half == NULL) create_ed_half (ecmdata);
	    if (inz == NULL) dbltogw (&ecmdata->gwdata, 2.0, out->z);
	    else gwsquare2 (&ecmdata->gwdata, inz, out->z, mul_S1_options | GWMUL_MULBYCONST | GWMUL_STARTNEXTFFT);	
	    if (ecmdata->ed_a == NULL) {			
		gwsquare2 (&ecmdata->gwdata, in->x, tmp2, GWMUL_FFT_S1 | GWMUL_MULBYCONST | GWMUL_STARTNEXTFFT);	
		gwmul3 (&ecmdata->gwdata, in->x, in->y, tmp1, GWMUL_FFT_S2 | GWMUL_MULBYCONST | GWMUL_STARTNEXTFFT);	
		gwmulmuladd5 (&ecmdata->gwdata, ecmdata->ed_half, tmp2, in->y, in->y, out->y, GWMUL_STARTNEXTFFT);	
	    } else {						
		gwsquare2 (&ecmdata->gwdata, in->x, tmp2, GWMUL_FFT_S1 | GWMUL_STARTNEXTFFT);				
		gwmul3 (&ecmdata->gwdata, in->x, in->y, out->x, GWMUL_FFT_S2 | GWMUL_MULBYCONST | GWMUL_STARTNEXTFFT);	
		gwmul3 (&ecmdata->gwdata, ecmdata->ed_a, tmp2, tmp2, GWMUL_MULBYCONST | GWMUL_STARTNEXTFFT);		
		gwmulmuladd5 (&ecmdata->gwdata, ecmdata->ed_half, tmp2, in->y, in->y, out->y, GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT); 
		gwswap (tmp1, out->x);
	    }
	    if (!extended) {
		gwsubmul4 (&ecmdata->gwdata, out->y, tmp2, out->y, tmp2, GWMUL_FFT_S13 | mul_options);			
		gwsubmul4 (&ecmdata->gwdata, out->z, out->y, out->y, out->x, GWMUL_FFT_S1 | mul_options);		
		gwsubmul4 (&ecmdata->gwdata, out->z, out->y, tmp1, tmp1, mul_options);					
		gwswap (tmp2, out->y);
		gwswap (out->x, out->z);
		gwswap (tmp1, out->x);
	    } else {
		gwsubmul4 (&ecmdata->gwdata, out->y, tmp2, out->y, out->x, GWMUL_FFT_S123 | mul_options);		
		gwsubmul4 (&ecmdata->gwdata, out->y, tmp2, tmp1, tmp2, GWMUL_FFT_S3 | mul_options);			
		gwsubmul4 (&ecmdata->gwdata, out->z, out->y, tmp1, tmp1, GWMUL_FFT_S1 | mul_options);			
		gwsubmul4 (&ecmdata->gwdata, out->z, out->y, out->y, out->z, mul_options);				
		gwswap (out->x, out->y);
		gwswap (tmp2, out->t);
		gwswap (tmp1, out->x);
	    }
	}

	else if (algo7 && squareadd_safe (&ecmdata->gwdata, 0, 0, 0) && ecmdata->ed_a == NULL && inz != NULL && (in->alternate_format || can_alternate_format)) {
	    if (ecmdata->ed_half == NULL) create_ed_half (ecmdata);

	   if (!in->alternate_format) {
		gwsquare2 (&ecmdata->gwdata, inz, out->z, mul_S1_options | GWMUL_MULBYCONST | GWMUL_STARTNEXTFFT);		
		gwsquare2 (&ecmdata->gwdata, in->x, tmp2, GWMUL_FFT_S1);							
		gwmul3 (&ecmdata->gwdata, in->x, in->y, out->x, GWMUL_FFT_S2 | GWMUL_MULBYCONST | GWMUL_STARTNEXTFFT);		
		gwsquare2 (&ecmdata->gwdata, in->y, out->y, 0);									
		gwaddsub4o (&ecmdata->gwdata, out->y, tmp2, tmp2, out->y, GWADD_FORCE_NORMALIZE);				
		gwsubmul4 (&ecmdata->gwdata, out->z, tmp2, tmp2, tmp1, GWMUL_FFT_S12 | GWMUL_MULBYCONST | GWMUL_STARTNEXTFFT);	
		gwsubmul4 (&ecmdata->gwdata, out->z, tmp2, out->x, out->x, GWMUL_MULBYCONST | GWMUL_STARTNEXTFFT);		
		gwmulmuladd5 (&ecmdata->gwdata, tmp2, out->y, ecmdata->ed_half, out->x, out->y, GWMUL_FFT_S4 | GWMUL_STARTNEXTFFT); 
		gwswap (tmp1, out->z);
		out->alternate_format = TRUE;
	    }

	   else {
		gwsquare2 (&ecmdata->gwdata, in->y, tmp2, GWMUL_FFT_S1 | GWMUL_MULBYCONST | GWMUL_STARTNEXTFFT);		
		gwsubmul4 (&ecmdata->gwdata, in->y, in->x, in->y, out->y, GWMUL_FFT_S1 | GWMUL_MULBYCONST | GWMUL_STARTNEXTFFT); 
		gwmuladd4 (&ecmdata->gwdata, in->x, in->x, out->y, out->x, GWMUL_FFT_S3 | GWMUL_STARTNEXTFFT);			
		if (can_alternate_format) {			
		    gwmulsub4 (&ecmdata->gwdata, inz, inz, out->x, out->z, GWMUL_FFT_S3 | GWMUL_MULBYCONST | GWMUL_STARTNEXTFFT); 
		    gwmul3 (&ecmdata->gwdata, out->z, out->x, tmp1, GWMUL_FFT_S1 | GWMUL_STARTNEXTFFT);				
		    gwsubmul4 (&ecmdata->gwdata, tmp2, out->x, out->z, tmp2, GWMUL_STARTNEXTFFT);				
		    gwmulmuladd5 (&ecmdata->gwdata, out->y, out->x, ecmdata->ed_half, tmp2, out->y, GWMUL_FFT_S4 | GWMUL_STARTNEXTFFT);	
		    gwswap (tmp1, out->z);
		    gwswap (tmp2, out->x);
		} else {					
		    gwmulsub4 (&ecmdata->gwdata, inz, inz, out->x, out->z, GWMUL_FFT_S3 | GWMUL_STARTNEXTFFT);			
		    gwmul3 (&ecmdata->gwdata, out->z, out->x, tmp1, GWMUL_FFT_S1 | mul_options);				
		    gwsubmul4 (&ecmdata->gwdata, tmp2, out->x, out->z, out->z, GWMUL_FFT_S1 | mul_options);			
		    if (extended) gwsubmul4 (&ecmdata->gwdata, tmp2, out->x, out->y, tmp2, GWMUL_FFT_S3 | mul_options);		
		    gwmul3 (&ecmdata->gwdata, out->y, out->x, out->y, mul_options);						
		    gwswap (out->z, out->x);
		    gwswap (tmp1, out->z);
		    if (extended) gwswap (tmp2, out->t);
		    out->alternate_format = FALSE;
		}
	    }
	}

	else {													
	    bool safe21 = mul_safe (&ecmdata->gwdata, 2, 1);								
	    if (inz == NULL) dbltogw (&ecmdata->gwdata, 2.0, out->z);
	    else gwsquare2 (&ecmdata->gwdata, inz, out->z, mul_S1_options | GWMUL_MULBYCONST | GWMUL_STARTNEXTFFT);	
	    if (ecmdata->ed_a == NULL) {			
		gwsquare2 (&ecmdata->gwdata, in->x, tmp2, GWMUL_FFT_S1 | GWMUL_STARTNEXTFFT_IF(safe21));		
	    } else {					
		gwsquare2 (&ecmdata->gwdata, in->x, tmp2, GWMUL_FFT_S1 | GWMUL_STARTNEXTFFT);				
		gwmul3 (&ecmdata->gwdata, ecmdata->ed_a, tmp2, tmp2, GWMUL_STARTNEXTFFT_IF(safe21));			
	    }
	    gwmul3 (&ecmdata->gwdata, in->x, in->y, out->x, GWMUL_FFT_S2 | GWMUL_MULBYCONST | GWMUL_STARTNEXTFFT);	
	    gwsquare2 (&ecmdata->gwdata, in->y, out->y, GWMUL_STARTNEXTFFT_IF(safe21));					
	    gwaddsub4o (&ecmdata->gwdata, out->y, tmp2, tmp2, out->y, GWADD_DELAYNORM_IF(safe21));			
	    if (!extended) {
		gwmul3 (&ecmdata->gwdata, tmp2, out->y, out->y, GWMUL_FFT_S1 | mul_options);				
		gwsubmul4 (&ecmdata->gwdata, out->z, tmp2, tmp2, tmp1, GWMUL_FFT_S1 | mul_options);			
		gwsubmul4 (&ecmdata->gwdata, out->z, tmp2, out->x, out->x, mul_options);				
		gwswap (tmp1, out->z);
	    } else {
		gwmul3 (&ecmdata->gwdata, tmp2, out->y, tmp1, GWMUL_FFT_S12 | mul_options);				
		gwmul3 (&ecmdata->gwdata, out->x, out->y, out->y, GWMUL_FFT_S1 | mul_options);				
		gwsubmul4 (&ecmdata->gwdata, out->z, tmp2, out->x, out->x, GWMUL_FFT_S1 | mul_options);			
		gwsubmul4 (&ecmdata->gwdata, out->z, tmp2, tmp2, out->z, mul_options);					
		gwswap (out->y, out->t);
		gwswap (tmp1, out->y);
	    }
	}

	if (options & ED_NO_GWALLOC)
		in->t = (tmp1 != NULL ? tmp1 : tmp2);
	else {
		gwfree (&ecmdata->gwdata, tmp1);
		gwfree (&ecmdata->gwdata, tmp2);
	}
}

__inline int alloc_xz (			
	ecmhandle *ecmdata,
	struct xz *arg)
{
	arg->x = gwalloc (&ecmdata->gwdata);
	if (arg->x == NULL) return (FALSE);
	arg->z = gwalloc (&ecmdata->gwdata);
	if (arg->z == NULL) return (FALSE);
	return (TRUE);
}

__inline void free_xz (
	ecmhandle *ecmdata,
	struct xz *arg)
{
	gwfree (&ecmdata->gwdata, arg->x); arg->x = NULL;
	gwfree (&ecmdata->gwdata, arg->z); arg->z = NULL;
}

#define xzswap(a,b)	{ struct xz t; t = a; a = b; b = t; }

__inline void gwcopy_xz (
	ecmhandle *ecmdata,
	struct xz *src,
	struct xz *dst)
{
	gwcopy (&ecmdata->gwdata, src->x, dst->x);
	gwcopy (&ecmdata->gwdata, src->z, dst->z);
}

void ell_dbl_xz_scr (
	ecmhandle *ecmdata,
	struct xz *in,		
	struct xz *out,		
	struct xz *scr)		
{				
	gwnum	t2, t3, t4;

	ASSERTG (scr != out);

	if (square_safe (&ecmdata->gwdata, 1)) {
		t3 = scr->x;
		t2 = out->x;
		t4 = out->z;

		gwsetmulbyconst (&ecmdata->gwdata, 4);
		gwmul3 (&ecmdata->gwdata, in->x, in->z, t3, GWMUL_FFT_S12 | GWMUL_MULBYCONST | GWMUL_STARTNEXTFFT); 
		gwsub3o (&ecmdata->gwdata, in->x, in->z, t2, GWADD_DELAY_NORMALIZE);			
		gwsquare2 (&ecmdata->gwdata, t2, t2, GWMUL_STARTNEXTFFT);				
		gwmul3 (&ecmdata->gwdata, t2, ecmdata->Ad4, t4, GWMUL_FFT_S1 | GWMUL_STARTNEXTFFT);	
		gwaddmul4 (&ecmdata->gwdata, t2, t3, t4, out->x, GWMUL_FFT_S23 | GWMUL_STARTNEXTFFT);	
		gwaddmul4 (&ecmdata->gwdata, t4, t3, t3, out->z, GWMUL_STARTNEXTFFT);			
	}

	else {
		int	t1_safe = addmul_safe (&ecmdata->gwdata, 2,0,0);

		t2 = out->x;
		t3 = out->z;
		t4 = scr->x;

		gwsquare2 (&ecmdata->gwdata, in->x, scr->x, GWMUL_FFT_S1);					
		gwsquare2 (&ecmdata->gwdata, in->z, scr->z, GWMUL_FFT_S1);					
		gwsetmulbyconst (&ecmdata->gwdata, 2);
		gwmul3 (&ecmdata->gwdata, in->x, in->z, t3, GWMUL_MULBYCONST);					

		gwadd3o (&ecmdata->gwdata, scr->x, scr->z, t2, GWADD_DELAY_NORMALIZE);				
		gwsub3o (&ecmdata->gwdata, t2, t3, t2, GWADD_DELAYNORM_IF(t1_safe));				
		gwadd3o (&ecmdata->gwdata, t3, t3, t3, GWADD_FORCE_NORMALIZE);					

		gwmul3 (&ecmdata->gwdata, t2, ecmdata->Ad4, t4, GWMUL_FFT_S1 | GWMUL_STARTNEXTFFT);		
		gwaddmul4 (&ecmdata->gwdata, t2, t3, t4, out->x, GWMUL_FFT_S23 | GWMUL_STARTNEXTFFT);		
		gwaddmul4 (&ecmdata->gwdata, t4, t3, t3, out->z, GWMUL_STARTNEXTFFT);				
	}
}

void ell_dbl_xz_scr_last (
	ecmhandle *ecmdata,
	struct xz *in,		
	struct xz *out,		
	struct xz *scr)		
{				
	gwnum	t2, t3, t4;

	ASSERTG (scr != out);

	if (square_safe (&ecmdata->gwdata, 1)) {
		t3 = scr->x;
		t2 = out->x;
		t4 = out->z;

		gwsetmulbyconst (&ecmdata->gwdata, 4);
		gwmul3 (&ecmdata->gwdata, in->x, in->z, t3, GWMUL_FFT_S12 | GWMUL_MULBYCONST | GWMUL_STARTNEXTFFT); 
		gwsub3o (&ecmdata->gwdata, in->x, in->z, t2, GWADD_DELAY_NORMALIZE);			
		gwsquare2 (&ecmdata->gwdata, t2, t2, GWMUL_STARTNEXTFFT);				
		gwmul3 (&ecmdata->gwdata, t2, ecmdata->Ad4, t4, GWMUL_FFT_S1 | GWMUL_STARTNEXTFFT);	
		gwaddmul4 (&ecmdata->gwdata, t2, t3, t4, out->x, GWMUL_FFT_S23);			
		gwaddmul4 (&ecmdata->gwdata, t4, t3, t3, out->z, 0);					
	}

	else {
		int	t1_safe = addmul_safe (&ecmdata->gwdata, 2,0,0);

		t2 = out->x;
		t3 = out->z;
		t4 = scr->x;

		gwsquare2 (&ecmdata->gwdata, in->x, scr->x, GWMUL_FFT_S1);					
		gwsquare2 (&ecmdata->gwdata, in->z, scr->z, GWMUL_FFT_S1);					
		gwsetmulbyconst (&ecmdata->gwdata, 2);
		gwmul3 (&ecmdata->gwdata, in->x, in->z, t3, GWMUL_MULBYCONST);					

		gwadd3o (&ecmdata->gwdata, scr->x, scr->z, t2, GWADD_DELAY_NORMALIZE);				
		gwsub3o (&ecmdata->gwdata, t2, t3, t2, GWADD_DELAYNORM_IF(t1_safe));				
		gwadd3o (&ecmdata->gwdata, t3, t3, t3, GWADD_FORCE_NORMALIZE);					

		gwmul3 (&ecmdata->gwdata, t2, ecmdata->Ad4, t4, GWMUL_FFT_S1 | GWMUL_STARTNEXTFFT);		
		gwaddmul4 (&ecmdata->gwdata, t2, t3, t4, out->x, GWMUL_FFT_S23);				
		gwaddmul4 (&ecmdata->gwdata, t4, t3, t3, out->z, 0);						
	}
}

void ell_add_xz_scr (
	ecmhandle *ecmdata,
	struct xz *in1,
	struct xz *in2,
	struct xz *diff,
	struct xz *out,
	struct xz *scr)		
{				
	gwnum	t1, t2;
	int	options;

	ASSERTG (scr != in1 && scr != in2 && scr != diff);
	t1 = scr->z;
	t2 = scr->x;
	gwmulmulsub5 (&ecmdata->gwdata, in1->x, in2->z, in1->z, in2->x, t1, GWMUL_FFT_S12 | GWMUL_STARTNEXTFFT);	
	gwmulmulsub5 (&ecmdata->gwdata, in1->x, in2->x, in1->z, in2->z, t2, GWMUL_STARTNEXTFFT);			
	gwsquare2 (&ecmdata->gwdata, t1, t1, GWMUL_STARTNEXTFFT);				
	gwsquare2 (&ecmdata->gwdata, t2, t2, GWMUL_STARTNEXTFFT);				
	options = (diff == out) ? GWMUL_STARTNEXTFFT : GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT;
	gwmul3 (&ecmdata->gwdata, t2, diff->z, t2, options);					
	gwmul3 (&ecmdata->gwdata, t1, diff->x, t1, options);					
	if (out != scr) xzswap (*scr, *out);
}

#define ell_add_xz(h,i1,i2,dif,o)	ell_add_xz_scr(h,i1,i2,dif,o,o);

int ell_add_xz_noscr (
	ecmhandle *ecmdata,
	struct xz *in1,
	struct xz *in2,
	struct xz *diff,
	struct xz *out)
{
	struct xz scr;
	if (!alloc_xz (ecmdata, &scr)) return (OutOfMemory (ecmdata->thread_num));
	ell_add_xz_scr (ecmdata, in1, in2, diff, out, &scr);
	free_xz (ecmdata, &scr);
	return (0);
}

void ell_add_xz_last (
	ecmhandle *ecmdata,
	struct xz *in1,
	struct xz *in2,
	struct xz *diff,
	struct xz *out,
	struct xz *scr)
{				
	gwnum	t1, t2;

	ASSERTG (scr != in1 && scr != in2 && scr != diff);
	t1 = scr->z;
	t2 = scr->x;
	gwmulmulsub5 (&ecmdata->gwdata, in1->x, in2->z, in1->z, in2->x, t1, GWMUL_FFT_S12 | GWMUL_STARTNEXTFFT);	
	gwmulmulsub5 (&ecmdata->gwdata, in1->x, in2->x, in1->z, in2->z, t2, GWMUL_STARTNEXTFFT);			
	gwsquare2 (&ecmdata->gwdata, t1, t1, GWMUL_STARTNEXTFFT);				
	gwsquare2 (&ecmdata->gwdata, t2, t2, GWMUL_STARTNEXTFFT);				
	gwmul3 (&ecmdata->gwdata, t2, diff->z, t2, 0);						
	gwmul3 (&ecmdata->gwdata, t1, diff->x, t1, 0);						
	if (out != scr) xzswap (*scr, *out);
}

void ell_add_xz_1norm (
	ecmhandle *ecmdata,
	struct xz *in1,		
	gwnum	in2,		
	struct xz *diff,	
	struct xz *out)		
{				
	gwnum	t1, t2;

	ASSERTG (out != in1 && out->x != in2 && out->z != in2 && out != diff);

	t1 = out->z;
	gwmulsub4 (&ecmdata->gwdata, in1->z, in2, in1->x, t1, GWMUL_FFT_S12 | GWMUL_STARTNEXTFFT);	
	gwsquare2 (&ecmdata->gwdata, t1, t1, GWMUL_STARTNEXTFFT);				
	gwmul3 (&ecmdata->gwdata, t1, diff->x, t1, GWMUL_STARTNEXTFFT);				

	t2 = out->x;
	gwmulsub4 (&ecmdata->gwdata, in1->x, in2, in1->z, t2, GWMUL_STARTNEXTFFT);		
	gwsquare2 (&ecmdata->gwdata, t2, t2, GWMUL_STARTNEXTFFT);				
	gwmul3 (&ecmdata->gwdata, t2, diff->z, t2, 0);						
}

void ell_add_xz_2norm (
	ecmhandle *ecmdata,
	struct xz *in1,		
	gwnum	in2,		
	gwnum	diff,		
	struct xz *out)		
{				
	gwnum	t1, t2;

	ASSERTG (out != in1 && out->x != in2 && out->z != in2 && out->z != diff);

	t1 = out->z;
	gwmulsub4 (&ecmdata->gwdata, in1->z, in2, in1->x, t1, GWMUL_FFT_S12 | GWMUL_STARTNEXTFFT);	
	gwsquare2 (&ecmdata->gwdata, t1, t1, GWMUL_STARTNEXTFFT);				
	gwmul3 (&ecmdata->gwdata, t1, diff, t1, GWMUL_STARTNEXTFFT);				

	t2 = out->x;
	gwmulsub4 (&ecmdata->gwdata, in1->x, in2, in1->z, t2, GWMUL_STARTNEXTFFT);		
	gwsquare2 (&ecmdata->gwdata, t2, t2, GWMUL_STARTNEXTFFT);				
}

void ell_add_xz_3norm (
	ecmhandle *ecmdata,
	gwnum	in1,		
	gwnum	in2,		
	gwnum	diff,		
	struct xz *out)		
{				
	gwnum	t1, t2;

	ASSERTG (out->x != in1 && out->z != in1 && out->x != in2 && out->z != in2);

	t1 = out->z;
	gwsubmul4 (&ecmdata->gwdata, in1, in2, diff, t1, GWMUL_STARTNEXTFFT);			
	gwsubmul4 (&ecmdata->gwdata, in1, in2, t1, t1, GWMUL_STARTNEXTFFT);			

	t2 = out->x;
	
	if (is_gwsetaddin_emulated (&ecmdata->gwdata))
		gwmulsub4 (&ecmdata->gwdata, in1, in2, ecmdata->gwdata.GW_FFT1, t2, GWMUL_STARTNEXTFFT);
	else
		gwmul3 (&ecmdata->gwdata, in1, in2, t2, GWMUL_ADDINCONST | GWMUL_STARTNEXTFFT);	
	gwsquare2 (&ecmdata->gwdata, t2, t2, GWMUL_STARTNEXTFFT);				
}

#define swap(a,b)	{uint64_t t=a;a=b;b=t;}

int lucas_cost (
	uint64_t n,
	uint64_t d)
{
	uint64_t e;
	unsigned long c;

	if (d >= n || d <= n/2) return (999999999);		

	c = 0;
	while (n != 1) {
	    e = n - d;
	    d = d - e;

	    c += 12;

	    while (d != e) {
		if (d < e) {
			swap (d,e);
		}

		if (100 * d <= 296 * e) {
			d = d-e;
			c += 12;
		} else if ((d&1) == (e&1)) {
			d = (d-e) >> 1;
			c += 22;
		} else if ((d&1) == 0) {
			d = d >> 1;
			c += 22;

		} else {
			e = e >> 1;
			c += 22;
		}
	    }
	    c += 10;
	    if (d == 1) break;
	    n = d;
	    d = (uint64_t) ((double) n * 0.6180339887498948);
	}

	return (c);
}

int lucas_mul (
	ecmhandle *ecmdata,
	struct xz *A,
	uint64_t n,
	uint64_t d,
	int	last_mul)	
{
	uint64_t e;
	struct xz B, C, T;

	if (!alloc_xz (ecmdata, &B)) goto oom;
	if (!alloc_xz (ecmdata, &C)) goto oom;

	if (!alloc_xz (ecmdata, &T)) goto oom;

	while (n != 1) {
	    ell_dbl_xz_scr (ecmdata, A, &B, &C);				
										
	    e = n - d;
	    d = d - e;

	    if (e > d && 100 * e <= 296 * d) {
		swap (d, e);
		xzswap (*A, B);							
		ell_add_xz (ecmdata, A, &B, &B, &C);				
		xzswap (B, C);							
		d = d-e;
	    }
	    else if (d > e && 100 * d <= 296 * e) {
		ell_add_xz (ecmdata, A, &B, A, &C);				
		xzswap (B, C);							
		d = d-e;
	    } else {
		gwcopy (&ecmdata->gwdata, A->x, C.x);
		gwcopy (&ecmdata->gwdata, A->z, C.z);				
	    }

	    while (d != e) {
		if (d < e) {
			swap (d, e);
			xzswap (*A, B);
		}
		
		if (100 * d <= 296 * e) {					
			ell_add_xz_scr (ecmdata, A, &B, &C, &C, &T);		
			xzswap (B, C);						
			d = d-e;
		} else if ((d&1) == (e&1)) {
			ell_add_xz_scr (ecmdata, A, &B, &C, &B, &T);		
			ell_dbl_xz_scr (ecmdata, A, A, &T);			
			d = (d-e) >> 1;
		} else if ((d&1) == 0) {
			ell_add_xz_scr (ecmdata, A, &C, &B, &C, &T);		
			ell_dbl_xz_scr (ecmdata, A, A, &T);			
			d = d >> 1;
		}
		
		else {
			ell_add_xz_scr (ecmdata, &B, &C, A, &C, &T);		
			ell_dbl_xz_scr (ecmdata, &B, &B, &T);			
			e = e >> 1;
		}
	    }

	    if (d == 1 && last_mul) {
		ell_add_xz_last (ecmdata, &B, A, &C, A, &T);			
		break;
	    } else {
		ell_add_xz_scr (ecmdata, &B, A, &C, A, &T);			
	    }

	    n = d;
	    d = (uint64_t) ((double) n * 0.6180339887498948);
	}
	free_xz (ecmdata, &B);
	free_xz (ecmdata, &C);

	free_xz (ecmdata, &T);
	return (0);

oom:	return (OutOfMemory (ecmdata->thread_num));
}
#undef swap

__inline int lucas_cost_several (uint64_t n, uint64_t *d) {
	int	i, c, min;
	uint64_t testd;
	for (i = 0, testd = *d - PRAC_SEARCH / 2; i < PRAC_SEARCH; i++, testd++) {
		c = lucas_cost (n, testd);
		if (i == 0 || c < min) min = c, *d = testd;
	}
	return (min);
}

int ell_mul (
	ecmhandle *ecmdata,
	struct xz *arg,
	uint64_t n,
	int	last_mul)	
{
	unsigned long zeros;
	int	stop_reason;

	for (zeros = 0; (n & 1) == 0; zeros++) n >>= 1;

	if (n > 1) {
		int	c, min;
		uint64_t d, mind;

		mind = (uint64_t) ceil((double) 0.6180339887498948 * n);		
		min = lucas_cost_several (n, &mind);

		d = (uint64_t) ceil ((double) 0.7236067977499790 * n);			
		c = lucas_cost_several (n, &d);
		if (c < min) min = c, mind = d;

		d = (uint64_t) ceil ((double) 0.5801787282954641 * n);			
		c = lucas_cost_several (n, &d);
		if (c < min) min = c, mind = d;

		d = (uint64_t) ceil ((double) 0.6328398060887063 * n);			
		c = lucas_cost_several (n, &d);
		if (c < min) min = c, mind = d;

		d = (uint64_t) ceil ((double) 0.6124299495094950 * n);			
		c = lucas_cost_several (n, &d);
		if (c < min) min = c, mind = d;

		d = (uint64_t) ceil ((double) 0.6201819808074158 * n);			
		c = lucas_cost_several (n, &d);
		if (c < min) min = c, mind = d;

		d = (uint64_t) ceil ((double) 0.6172146165344039 * n);			
		c = lucas_cost_several (n, &d);
		if (c < min) min = c, mind = d;

		d = (uint64_t) ceil ((double) 0.6183471196562281 * n);			
		c = lucas_cost_several (n, &d);
		if (c < min) min = c, mind = d;

		d = (uint64_t) ceil ((double) 0.6179144065288179 * n);			
		c = lucas_cost_several (n, &d);
		if (c < min) min = c, mind = d;

		d = (uint64_t) ceil ((double) 0.6180796684698958 * n);			
		c = lucas_cost_several (n, &d);
		if (c < min) min = c, mind = d;

		stop_reason = lucas_mul (ecmdata, arg, n, mind, zeros == 0 && last_mul);
		if (stop_reason) return (stop_reason);
	}
	while (zeros--) {
		struct xz scr;
		if (!alloc_xz (ecmdata, &scr)) return (OutOfMemory (ecmdata->thread_num));
		if (zeros || !last_mul) ell_dbl_xz_scr (ecmdata, arg, arg, &scr);
		else ell_dbl_xz_scr_last (ecmdata, arg, arg, &scr);
		free_xz (ecmdata, &scr);
	}
	return (0);
}

int ecm_modinv (
	ecmhandle *ecmdata,
	gwnum	b)
{
	giant	v;

	v = popg (&ecmdata->gwdata.gdata, ((int) ecmdata->gwdata.bit_length >> 5) + 10);
	if (v == NULL) goto oom;
	if (gwtogiant (&ecmdata->gwdata, b, v)) {
		
		goto oom;
	}

#ifdef MODINV_USING_GIANTS

	int	stop_reason;

	gwfree_temporarily (&ecmdata->gwdata, b);
	stop_reason = invgi (&ecmdata->gwdata.gdata, ecmdata->thread_num, ecmdata->N, v);
	if (stop_reason == GIANT_OUT_OF_MEMORY)
		stop_reason = OutOfMemory (ecmdata->thread_num);
	gwrealloc_temporarily (&ecmdata->gwdata, b);
	if (stop_reason) {
		pushg (&ecmdata->gwdata.gdata, 1);
		return (stop_reason);
	}

	if (v->sign < 0) {
		negg (v);
		ecmdata->factor = allocgiant (v->sign);
		if (ecmdata->factor == NULL) goto oom;
		gtog (v, ecmdata->factor);
	}

	else {
		ecmdata->factor = NULL;
		gianttogw (&ecmdata->gwdata, v, b);
	}

#else
	{
	mpz_t	__v, __N, __gcd, __inv;

	mpz_init (__v);
	mpz_init (__N);
	mpz_init (__gcd);
	mpz_init (__inv);
	gtompz (v, __v);
	gtompz (ecmdata->N, __N);
	mpz_gcdext (__gcd, __inv, NULL, __v, __N);
	mpz_clear (__v);

	if (mpz_cmp_ui (__gcd, 1) && mpz_cmp (__gcd, __N)) {
		ecmdata->factor = allocgiant ((int) divide_rounding_up (mpz_sizeinbase (__gcd, 2), 32));
		if (ecmdata->factor == NULL) goto oom;
		mpztog (__gcd, ecmdata->factor);
	}

	else {
		ecmdata->factor = NULL;
		if (mpz_sgn (__inv) < 0) mpz_add (__inv, __inv, __N);
		mpztog (__inv, v);
		gianttogw (&ecmdata->gwdata, v, b);
	}

	mpz_clear (__gcd);
	mpz_clear (__inv);
	mpz_clear (__N);
	}
#endif

	pushg (&ecmdata->gwdata.gdata, 1);

	ecmdata->modinv_count++;
	return (0);

oom:	return (OutOfMemory (ecmdata->thread_num));
}

int normalize (
	ecmhandle *ecmdata,
	gwnum	a,
	gwnum	b)
{
	int	stop_reason;

	stop_reason = ecm_modinv (ecmdata, b);
	if (stop_reason) return (stop_reason);
	if (ecmdata->factor != NULL) return (0);
	gwmul3 (&ecmdata->gwdata, b, a, a, 0);
	return (0);
}

int normalize_pool_init (
	ecmhandle *ecmdata,
	uint64_t max_pool_size)
{
	ecmdata->pool_count = 0;
	ecmdata->poolz_count = 0;
	ecmdata->pool_values = NULL;
	ecmdata->poolz_values = NULL;
	ecmdata->pool_modinv_value = NULL;
	ecmdata->use_preallocated_norm_temps = FALSE;

	ecmdata->pool_values = (gwnum *) malloc ((size_t) (max_pool_size * sizeof (gwnum)));
	if (ecmdata->pool_values == NULL) goto oom;
	if (ecmdata->pool_type == POOL_3MULT) {		
		ecmdata->poolz_values = (gwnum *) malloc ((size_t) (max_pool_size * sizeof (gwnum)));
		if (ecmdata->poolz_values == NULL) goto oom;
	}
	if (ecmdata->pool_type == POOL_3POINT44MULT) {	
		ecmdata->poolz_values = (gwnum *) malloc ((size_t) ((divide_rounding_up (max_pool_size, 3) + 2) * sizeof (gwnum)));
		if (ecmdata->poolz_values == NULL) goto oom;
	}
	if (ecmdata->pool_type == POOL_3POINT57MULT) {	
		ecmdata->poolz_values = (gwnum *) malloc ((size_t) ((divide_rounding_up (max_pool_size, 7) + 5) * sizeof (gwnum)));
		if (ecmdata->poolz_values == NULL) goto oom;
	}

	return (0);

oom:	return (OutOfMemory (ecmdata->thread_num));
}

void normalize_pool_term (
	ecmhandle *ecmdata)
{
	if (ecmdata->poolz_values != NULL) {
		if (!ecmdata->use_preallocated_norm_temps) for (uint32_t i = 0; i < ecmdata->poolz_count; i++) gwfree (&ecmdata->gwdata, ecmdata->poolz_values[i]);
		free (ecmdata->poolz_values); ecmdata->poolz_values = NULL;
	}

	free (ecmdata->pool_values); ecmdata->pool_values = NULL;
	if (!ecmdata->use_preallocated_norm_temps) gwfree (&ecmdata->gwdata, ecmdata->pool_modinv_value); ecmdata->pool_modinv_value = NULL;
	ecmdata->pool_count = 0;
	ecmdata->poolz_count = 0;
}

int add_to_normalize_pool_ro (
	ecmhandle *ecmdata,
	gwnum	a,		
	gwnum	b)		
{
#define normalloc(d)		if(!ecmdata->use_preallocated_norm_temps){d = gwalloc (&ecmdata->gwdata); if (d == NULL) goto oom;} \
				else {ASSERTG(ecmdata->norm_pool_temps_count > 0); d = ecmdata->norm_pool_temps[--ecmdata->norm_pool_temps_count];}
#define normfree(d)		if(!ecmdata->use_preallocated_norm_temps){gwfree (&ecmdata->gwdata, d);} \
				else {ASSERTG(ecmdata->norm_pool_temps_count < 5); ecmdata->norm_pool_temps[ecmdata->norm_pool_temps_count++] = d;}
#define allocAndFFTb(d)		{gwfft(&ecmdata->gwdata,b,b);d=b;}
#define allocAndCopyb(d)	d=b
#define copyAndFFTb(d)		{gwswap(b,d);normfree(b);}

	switch (ecmdata->pool_type) {

	case POOL_3MULT:

		if (ecmdata->pool_count == 0) {
			allocAndFFTb (ecmdata->pool_modinv_value);
		}

		else {
			
			if (ecmdata->pool_count > 1) {
				gwmul3 (&ecmdata->gwdata, ecmdata->poolz_values[ecmdata->poolz_count-1], ecmdata->pool_modinv_value, ecmdata->pool_modinv_value, GWMUL_STARTNEXTFFT);
			}
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, a, a, GWMUL_FFT_S1 | GWMUL_STARTNEXTFFT);
			
			allocAndFFTb (ecmdata->poolz_values[ecmdata->poolz_count++]);
		}

		ecmdata->pool_values[ecmdata->pool_count++] = a;
		break;

	case POOL_3POINT44MULT:

		if (ecmdata->pool_count == 0) {
			
			allocAndFFTb (ecmdata->pool_modinv_value);
		}

		else if (ecmdata->pool_count == 1) {
			
			allocAndFFTb (ecmdata->poolz_values[ecmdata->poolz_count++]);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[0], ecmdata->poolz_values[ecmdata->poolz_count-1], ecmdata->pool_values[0], GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, a, ecmdata->pool_modinv_value, a, GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
		}

		else if (ecmdata->pool_count == 2) {
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, ecmdata->poolz_values[ecmdata->poolz_count-1], ecmdata->pool_modinv_value, GWMUL_STARTNEXTFFT);
			
			copyAndFFTb (ecmdata->poolz_values[ecmdata->poolz_count-1]);
		}

		else if (ecmdata->pool_count == 3) {
			gwnum	tmp1, tmp2;
			
			allocAndFFTb (tmp2);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[2], tmp2, ecmdata->pool_values[2], GWMUL_STARTNEXTFFT);
			
			tmp1 = ecmdata->poolz_values[ecmdata->poolz_count-1];
			gwmul3 (&ecmdata->gwdata, a, tmp1, a, GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, tmp1, tmp2, tmp1, GWMUL_STARTNEXTFFT);
			normfree (tmp2);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[0], tmp1, ecmdata->pool_values[0], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[1], tmp1, ecmdata->pool_values[1], GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[2], ecmdata->pool_modinv_value, ecmdata->pool_values[2], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, a, ecmdata->pool_modinv_value, a, GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
		}

		else if (ecmdata->pool_count % 3 == 1) {
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, ecmdata->poolz_values[ecmdata->poolz_count-1], ecmdata->pool_modinv_value, GWMUL_STARTNEXTFFT);
			
			copyAndFFTb (ecmdata->poolz_values[ecmdata->poolz_count-1]);
			
			gwmul3 (&ecmdata->gwdata, a, ecmdata->pool_modinv_value, a, GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
		}

		else if (ecmdata->pool_count % 3 == 2) {
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, ecmdata->poolz_values[ecmdata->poolz_count-1], ecmdata->pool_modinv_value, GWMUL_STARTNEXTFFT);
			
			allocAndFFTb (ecmdata->poolz_values[ecmdata->poolz_count++]);
		}

		else {
			gwnum	tmp1, tmp2;
			
			allocAndFFTb (tmp2);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[ecmdata->pool_count-1], tmp2, ecmdata->pool_values[ecmdata->pool_count-1], GWMUL_STARTNEXTFFT);
			
			tmp1 = ecmdata->poolz_values[ecmdata->poolz_count-1];
			gwmul3 (&ecmdata->gwdata, a, tmp1, a, GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, tmp1, tmp2, tmp1, GWMUL_STARTNEXTFFT);
			normfree (tmp2);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->poolz_values[ecmdata->poolz_count-2], tmp1, ecmdata->poolz_values[ecmdata->poolz_count-2], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[ecmdata->pool_count-2], tmp1, ecmdata->pool_values[ecmdata->pool_count-2], GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[ecmdata->pool_count-1], ecmdata->pool_modinv_value, ecmdata->pool_values[ecmdata->pool_count-1], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, a, ecmdata->pool_modinv_value, a, GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
		}

		ecmdata->pool_values[ecmdata->pool_count++] = a;
		break;

	case POOL_3POINT57MULT:

		if (ecmdata->pool_count == 0) {
			
			allocAndFFTb (ecmdata->pool_modinv_value);
		}

		else if (ecmdata->pool_count == 1) {
			
			allocAndFFTb (ecmdata->poolz_values[ecmdata->poolz_count++]);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[0], ecmdata->poolz_values[ecmdata->poolz_count-1], ecmdata->pool_values[0], GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, a, ecmdata->pool_modinv_value, a, GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
		}

		else if (ecmdata->pool_count == 2) {
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, ecmdata->poolz_values[ecmdata->poolz_count-1], ecmdata->pool_modinv_value, GWMUL_STARTNEXTFFT);
			
			copyAndFFTb (ecmdata->poolz_values[ecmdata->poolz_count-1]);
		}

		else if (ecmdata->pool_count == 3 || ecmdata->pool_count == 5) {
			
			gwnum	tmp;
			allocAndFFTb (tmp);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[ecmdata->pool_count-1], tmp, ecmdata->pool_values[ecmdata->pool_count-1], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, a, ecmdata->poolz_values[ecmdata->poolz_count-1], a, GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->poolz_values[ecmdata->poolz_count-1], tmp, ecmdata->poolz_values[ecmdata->poolz_count-1], GWMUL_STARTNEXTFFT);
			normfree (tmp);
		}

		else if (ecmdata->pool_count == 4 || ecmdata->pool_count == 6) {
			
			allocAndFFTb (ecmdata->poolz_values[ecmdata->poolz_count++]);
		}

		else if (ecmdata->pool_count == 7) {
			gwnum	b7, b8, b34, b56, b78, b5678, b6vals;
			
			normalloc (b6vals);
			
			b7 = ecmdata->poolz_values[2];
			b56 = ecmdata->poolz_values[1];
			b34 = ecmdata->poolz_values[0];
			ecmdata->poolz_count = 0;
			
			allocAndFFTb (b8);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[ecmdata->pool_count-1], b8, ecmdata->pool_values[ecmdata->pool_count-1], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, b7, b8, b8, GWMUL_FFT_S1 | GWMUL_STARTNEXTFFT);
			b78 = b8;
			
			gwmul3 (&ecmdata->gwdata, a, b7, a, GWMUL_STARTNEXTFFT);
			
			b5678 = b7;
			gwmul3 (&ecmdata->gwdata, b56, b78, b5678, GWMUL_FFT_S12 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, b34, b5678, b6vals, GWMUL_FFT_S12 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[0], b6vals, ecmdata->pool_values[0], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[1], b6vals, ecmdata->pool_values[1], GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, b5678, b6vals, GWMUL_FFT_S1 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[2], b6vals, ecmdata->pool_values[2], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[3], b6vals, ecmdata->pool_values[3], GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, b34, ecmdata->pool_modinv_value, GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, b78, ecmdata->pool_modinv_value, b6vals, GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[4], b6vals, ecmdata->pool_values[4], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[5], b6vals, ecmdata->pool_values[5], GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, b56, ecmdata->pool_modinv_value, b6vals, GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[6], b6vals, ecmdata->pool_values[6], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, a, b6vals, a, GWMUL_STARTNEXTFFT);
			
			ecmdata->poolz_values[0] = b5678;
			ecmdata->poolz_count = 1;
			
			normfree (b34);
			normfree (b56);
			normfree (b78);
			normfree (b6vals);
		}

		else if (ecmdata->pool_count % 7 == 1) {
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, ecmdata->poolz_values[ecmdata->poolz_count-1], ecmdata->pool_modinv_value, GWMUL_STARTNEXTFFT);
			
			copyAndFFTb (ecmdata->poolz_values[ecmdata->poolz_count-1]);
			
			gwmul3 (&ecmdata->gwdata, a, ecmdata->pool_modinv_value, a, GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
		}

		else if (ecmdata->pool_count % 7 == 2) {
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, ecmdata->poolz_values[ecmdata->poolz_count-1], ecmdata->pool_modinv_value, GWMUL_STARTNEXTFFT);
			
			allocAndFFTb (ecmdata->poolz_values[ecmdata->poolz_count++]);
		}

		else if (ecmdata->pool_count % 7 == 3 || ecmdata->pool_count % 7 == 5) {
			
			gwnum	tmp;
			allocAndFFTb (tmp);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[ecmdata->pool_count-1], tmp, ecmdata->pool_values[ecmdata->pool_count-1], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, a, ecmdata->poolz_values[ecmdata->poolz_count-1], a, GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->poolz_values[ecmdata->poolz_count-1], tmp, ecmdata->poolz_values[ecmdata->poolz_count-1], GWMUL_STARTNEXTFFT);
			normfree (tmp);
		}

		else if (ecmdata->pool_count % 7 == 4 || ecmdata->pool_count % 7 == 6) {
			
			allocAndFFTb (ecmdata->poolz_values[ecmdata->poolz_count++]);
		}

		else {
			gwnum	b14, b15, b1011, b1213, b1415, b12131415, b6vals;
			
			normalloc (b6vals);
			
			b14 = ecmdata->poolz_values[ecmdata->poolz_count-1];
			b1213 = ecmdata->poolz_values[ecmdata->poolz_count-2];
			b1011 = ecmdata->poolz_values[ecmdata->poolz_count-3];
			ecmdata->poolz_count -= 3;
			
			allocAndFFTb (b15);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[ecmdata->pool_count-1], b15, ecmdata->pool_values[ecmdata->pool_count-1], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, b14, b15, b15, GWMUL_FFT_S1 | GWMUL_STARTNEXTFFT);
			b1415 = b15;
			
			gwmul3 (&ecmdata->gwdata, a, b14, a, GWMUL_STARTNEXTFFT);
			
			b12131415 = b14;
			gwmul3 (&ecmdata->gwdata, b1213, b1415, b12131415, GWMUL_FFT_S12 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, b1011, b12131415, b6vals, GWMUL_FFT_S12 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->poolz_values[ecmdata->poolz_count-1], b6vals, ecmdata->poolz_values[ecmdata->poolz_count-1], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[ecmdata->pool_count-6], b6vals, ecmdata->pool_values[ecmdata->pool_count-6], GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, b12131415, b6vals, GWMUL_FFT_S1 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[ecmdata->pool_count-5], b6vals, ecmdata->pool_values[ecmdata->pool_count-5], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[ecmdata->pool_count-4], b6vals, ecmdata->pool_values[ecmdata->pool_count-4], GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, b1011, ecmdata->pool_modinv_value, GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, b1415, ecmdata->pool_modinv_value, b6vals, GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[ecmdata->pool_count-3], b6vals, ecmdata->pool_values[ecmdata->pool_count-3], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[ecmdata->pool_count-2], b6vals, ecmdata->pool_values[ecmdata->pool_count-2], GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, b1213, ecmdata->pool_modinv_value, b6vals, GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[ecmdata->pool_count-1], b6vals, ecmdata->pool_values[ecmdata->pool_count-1], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, a, b6vals, a, GWMUL_STARTNEXTFFT);
			
			ecmdata->poolz_values[ecmdata->poolz_count++] = b12131415;
			
			normfree (b1011);
			normfree (b1213);
			normfree (b1415);
			normfree (b6vals);
		}

		ecmdata->pool_values[ecmdata->pool_count++] = a;
		break;

	case POOL_N_SQUARED:

		if (ecmdata->pool_count == 0) {
			allocAndCopyb (ecmdata->pool_modinv_value);
		}

		else {
			gwnum	tmp;
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, a, a, GWMUL_FFT_S1 | GWMUL_STARTNEXTFFT);
			allocAndFFTb (tmp);
			for (uint32_t i = 0; i < ecmdata->pool_count; i++)
				gwmul3 (&ecmdata->gwdata, tmp, ecmdata->pool_values[i], ecmdata->pool_values[i], GWMUL_STARTNEXTFFT);
			gwmul3 (&ecmdata->gwdata, tmp, ecmdata->pool_modinv_value, ecmdata->pool_modinv_value, GWMUL_STARTNEXTFFT);
			normfree (tmp);
		}

		ecmdata->pool_values[ecmdata->pool_count++] = a;
		break;
	}
	return (0);

oom:	return (OutOfMemory (ecmdata->thread_num));
}

int normalize_pool (
	ecmhandle *ecmdata)
{
	int	stop_reason;

#undef normfree
#define normfree(d)		if(!ecmdata->use_preallocated_norm_temps){gwfree (&ecmdata->gwdata, d);}

	if (ecmdata->pool_type == POOL_3MULT && ecmdata->pool_count > 1) {
		gwmul3 (&ecmdata->gwdata, ecmdata->poolz_values[ecmdata->poolz_count-1], ecmdata->pool_modinv_value, ecmdata->pool_modinv_value, 0);
	}

	if (ecmdata->pool_type == POOL_3POINT44MULT) {

		if (ecmdata->pool_count == 2) {
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, ecmdata->poolz_values[0], ecmdata->pool_modinv_value, 0);
			normfree (ecmdata->poolz_values[0]);
			ecmdata->poolz_count = 0;
		}

		else if (ecmdata->pool_count == 3) {
			gwnum	b3 = ecmdata->poolz_values[0];
			ecmdata->poolz_count = 0;
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[0], b3, ecmdata->pool_values[0], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[1], b3, ecmdata->pool_values[1], GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[2], ecmdata->pool_modinv_value, ecmdata->pool_values[2], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, b3, ecmdata->pool_modinv_value, 0);
			normfree (b3);
		}

		else if (ecmdata->pool_count > 4 && ecmdata->pool_count % 3 == 2) {
			
			gwmul3 (&ecmdata->gwdata, ecmdata->poolz_values[ecmdata->poolz_count-1], ecmdata->pool_modinv_value, ecmdata->pool_modinv_value, 0);
		}

		else if (ecmdata->pool_count > 4 && ecmdata->pool_count % 3 == 0) {
			gwnum	b6 = ecmdata->poolz_values[--ecmdata->poolz_count];
			
			gwmul3 (&ecmdata->gwdata, ecmdata->poolz_values[ecmdata->poolz_count-1], b6, ecmdata->poolz_values[ecmdata->poolz_count-1], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[ecmdata->pool_count-2], b6, ecmdata->pool_values[ecmdata->pool_count-2], GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[ecmdata->pool_count-1], ecmdata->pool_modinv_value, ecmdata->pool_values[ecmdata->pool_count-1], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, b6, ecmdata->pool_modinv_value, 0);
			normfree (b6);
		}

		else if (ecmdata->pool_count >= 4 && ecmdata->pool_count % 3 == 1) {
			
			ecmdata->poolz_count--;
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, ecmdata->poolz_values[ecmdata->poolz_count], ecmdata->pool_modinv_value, 0);
			normfree (ecmdata->poolz_values[ecmdata->poolz_count]);
		}
	}

	if (ecmdata->pool_type == POOL_3POINT57MULT) {

		if (ecmdata->pool_count == 2) {
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, ecmdata->poolz_values[0], ecmdata->pool_modinv_value, 0);
			normfree (ecmdata->poolz_values[0]);
			ecmdata->poolz_count = 0;
		}

		else if (ecmdata->pool_count == 3) {
			gwnum	b3;
			
			b3 = ecmdata->poolz_values[0];
			ecmdata->poolz_count = 0;
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[0], b3, ecmdata->pool_values[0], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[1], b3, ecmdata->pool_values[1], GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[2], ecmdata->pool_modinv_value, ecmdata->pool_values[2], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, b3, ecmdata->pool_modinv_value, 0);
			
			normfree (b3);
		}

		else if (ecmdata->pool_count == 4) {
			gwnum	b34;
			
			b34 = ecmdata->poolz_values[0];
			ecmdata->poolz_count = 0;
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[0], b34, ecmdata->pool_values[0], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[1], b34, ecmdata->pool_values[1], GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[2], ecmdata->pool_modinv_value, ecmdata->pool_values[2], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[3], ecmdata->pool_modinv_value, ecmdata->pool_values[3], GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, b34, ecmdata->pool_modinv_value, 0);
			
			normfree (b34);
		}

		else if (ecmdata->pool_count == 5) {
			gwnum	b34, b5, b6vals;
			
			normalloc (b6vals);
			
			b5 = ecmdata->poolz_values[1];
			b34 = ecmdata->poolz_values[0];
			ecmdata->poolz_count = 0;
			
			gwmul3 (&ecmdata->gwdata, b34, b5, b6vals, GWMUL_FFT_S12 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[0], b6vals, ecmdata->pool_values[0], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[1], b6vals, ecmdata->pool_values[1], GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, b5, b6vals, GWMUL_FFT_S1 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[2], b6vals, ecmdata->pool_values[2], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[3], b6vals, ecmdata->pool_values[3], GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, b34, ecmdata->pool_modinv_value, GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[4], ecmdata->pool_modinv_value, ecmdata->pool_values[4], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, b5, ecmdata->pool_modinv_value, 0);
			
			normfree (b34);
			normfree (b5);
			normfree (b6vals);
		}

		else if (ecmdata->pool_count == 6) {
			gwnum	b34, b56, b6vals;
			
			normalloc (b6vals);
			
			b56 = ecmdata->poolz_values[1];
			b34 = ecmdata->poolz_values[0];
			ecmdata->poolz_count = 0;
			
			gwmul3 (&ecmdata->gwdata, b34, b56, b6vals, GWMUL_FFT_S12 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[0], b6vals, ecmdata->pool_values[0], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[1], b6vals, ecmdata->pool_values[1], GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, b56, b6vals, GWMUL_FFT_S1 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[2], b6vals, ecmdata->pool_values[2], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[3], b6vals, ecmdata->pool_values[3], GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, b34, ecmdata->pool_modinv_value, GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[4], ecmdata->pool_modinv_value, ecmdata->pool_values[4], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[5], ecmdata->pool_modinv_value, ecmdata->pool_values[5], GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, b56, ecmdata->pool_modinv_value, 0);
			
			normfree (b34);
			normfree (b56);
			normfree (b6vals);
		}

		else if (ecmdata->pool_count == 7) {
			gwnum	b7, b34, b56, b567, b6vals;
			
			normalloc (b567);
			normalloc (b6vals);
			
			b7 = ecmdata->poolz_values[2];
			b56 = ecmdata->poolz_values[1];
			b34 = ecmdata->poolz_values[0];
			ecmdata->poolz_count = 0;
			
			gwmul3 (&ecmdata->gwdata, b56, b7, b567, GWMUL_FFT_S12 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, b34, b567, b6vals, GWMUL_FFT_S12 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[0], b6vals, ecmdata->pool_values[0], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[1], b6vals, ecmdata->pool_values[1], GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, b567, b6vals, GWMUL_FFT_S1 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[2], b6vals, ecmdata->pool_values[2], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[3], b6vals, ecmdata->pool_values[3], GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, b34, ecmdata->pool_modinv_value, GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, b7, ecmdata->pool_modinv_value, b6vals, GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[4], b6vals, ecmdata->pool_values[4], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[5], b6vals, ecmdata->pool_values[5], GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, b56, ecmdata->pool_modinv_value, b6vals, GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[6], b6vals, ecmdata->pool_values[6], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, b567, ecmdata->pool_modinv_value, 0);
			
			normfree (b34);
			normfree (b56);
			normfree (b7);
			normfree (b567);
			normfree (b6vals);
		}

		else if (ecmdata->pool_count > 8 && ecmdata->pool_count % 7 == 2) {
			
			gwmul3 (&ecmdata->gwdata, ecmdata->poolz_values[ecmdata->poolz_count-1], ecmdata->pool_modinv_value, ecmdata->pool_modinv_value, 0);
		}

		else if (ecmdata->pool_count > 8 && ecmdata->pool_count % 7 == 3) {
			gwnum	b10;
			
			b10 = ecmdata->poolz_values[ecmdata->poolz_count-1];
			ecmdata->poolz_count -= 1;
			
			gwmul3 (&ecmdata->gwdata, ecmdata->poolz_values[ecmdata->poolz_count-1], b10, ecmdata->poolz_values[ecmdata->poolz_count-1], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[ecmdata->pool_count-2], b10, ecmdata->pool_values[ecmdata->pool_count-2], GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[ecmdata->pool_count-1], ecmdata->pool_modinv_value, ecmdata->pool_values[ecmdata->pool_count-1], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, b10, ecmdata->pool_modinv_value, 0);
			
			normfree (b10);
		}

		else if (ecmdata->pool_count > 8 && ecmdata->pool_count % 7 == 4) {
			gwnum	b1011;
			
			b1011 = ecmdata->poolz_values[ecmdata->poolz_count-1];
			ecmdata->poolz_count -= 1;
			
			gwmul3 (&ecmdata->gwdata, ecmdata->poolz_values[ecmdata->poolz_count-1], b1011, ecmdata->poolz_values[ecmdata->poolz_count-1], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[ecmdata->pool_count-3], b1011, ecmdata->pool_values[ecmdata->pool_count-3], GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[ecmdata->pool_count-2], ecmdata->pool_modinv_value, ecmdata->pool_values[ecmdata->pool_count-2], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[ecmdata->pool_count-1], ecmdata->pool_modinv_value, ecmdata->pool_values[ecmdata->pool_count-1], GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, b1011, ecmdata->pool_modinv_value, 0);
			
			normfree (b1011);
		}

		else if (ecmdata->pool_count > 8 && ecmdata->pool_count % 7 == 5) {
			gwnum	b1011, b12, b6vals;
			
			normalloc (b6vals);
			
			b12 = ecmdata->poolz_values[ecmdata->poolz_count-1];
			b1011 = ecmdata->poolz_values[ecmdata->poolz_count-2];
			ecmdata->poolz_count -= 2;
			
			gwmul3 (&ecmdata->gwdata, b1011, b12, b6vals, GWMUL_FFT_S12 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->poolz_values[ecmdata->poolz_count-1], b6vals, ecmdata->poolz_values[ecmdata->poolz_count-1], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[ecmdata->pool_count-4], b6vals, ecmdata->pool_values[ecmdata->pool_count-4], GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, b12, b6vals, GWMUL_FFT_S1 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[ecmdata->pool_count-3], b6vals, ecmdata->pool_values[ecmdata->pool_count-3], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[ecmdata->pool_count-2], b6vals, ecmdata->pool_values[ecmdata->pool_count-2], GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, b1011, ecmdata->pool_modinv_value, GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[ecmdata->pool_count-1], ecmdata->pool_modinv_value, ecmdata->pool_values[ecmdata->pool_count-1], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, b12, ecmdata->pool_modinv_value, 0);
			
			normfree (b1011);
			normfree (b12);
			normfree (b6vals);
		}

		else if (ecmdata->pool_count > 8 && ecmdata->pool_count % 7 == 6) {
			gwnum	b1011, b1213, b6vals;
			
			normalloc (b6vals);
			
			b1213 = ecmdata->poolz_values[ecmdata->poolz_count-1];
			b1011 = ecmdata->poolz_values[ecmdata->poolz_count-2];
			ecmdata->poolz_count -= 2;
			
			gwmul3 (&ecmdata->gwdata, b1011, b1213, b6vals, GWMUL_FFT_S12 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->poolz_values[ecmdata->poolz_count-1], b6vals, ecmdata->poolz_values[ecmdata->poolz_count-1], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[ecmdata->pool_count-5], b6vals, ecmdata->pool_values[ecmdata->pool_count-5], GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, b1213, b6vals, GWMUL_FFT_S1 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[ecmdata->pool_count-4], b6vals, ecmdata->pool_values[ecmdata->pool_count-4], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[ecmdata->pool_count-3], b6vals, ecmdata->pool_values[ecmdata->pool_count-3], GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, b1011, ecmdata->pool_modinv_value, GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[ecmdata->pool_count-2], ecmdata->pool_modinv_value, ecmdata->pool_values[ecmdata->pool_count-2], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[ecmdata->pool_count-1], ecmdata->pool_modinv_value, ecmdata->pool_values[ecmdata->pool_count-1], GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, b1213, ecmdata->pool_modinv_value, 0);
			
			normfree (b1011);
			normfree (b1213);
			normfree (b6vals);
		}

		else if (ecmdata->pool_count > 8 && ecmdata->pool_count % 7 == 0) {
			gwnum	b14, b1011, b1213, b121314, b6vals;
			
			normalloc (b121314);
			normalloc (b6vals);
			
			b14 = ecmdata->poolz_values[ecmdata->poolz_count-1];
			b1213 = ecmdata->poolz_values[ecmdata->poolz_count-2];
			b1011 = ecmdata->poolz_values[ecmdata->poolz_count-3];
			ecmdata->poolz_count -= 3;
			
			gwmul3 (&ecmdata->gwdata, b1213, b14, b121314, GWMUL_FFT_S12 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, b1011, b121314, b6vals, GWMUL_FFT_S12 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->poolz_values[ecmdata->poolz_count-1], b6vals, ecmdata->poolz_values[ecmdata->poolz_count-1], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[ecmdata->pool_count-6], b6vals, ecmdata->pool_values[ecmdata->pool_count-6], GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, b121314, b6vals, GWMUL_FFT_S1 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[ecmdata->pool_count-5], b6vals, ecmdata->pool_values[ecmdata->pool_count-5], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[ecmdata->pool_count-4], b6vals, ecmdata->pool_values[ecmdata->pool_count-4], GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, b1011, ecmdata->pool_modinv_value, GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, b14, ecmdata->pool_modinv_value, b6vals, GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[ecmdata->pool_count-3], b6vals, ecmdata->pool_values[ecmdata->pool_count-3], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[ecmdata->pool_count-2], b6vals, ecmdata->pool_values[ecmdata->pool_count-2], GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, b1213, ecmdata->pool_modinv_value, b6vals, GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_values[ecmdata->pool_count-1], b6vals, ecmdata->pool_values[ecmdata->pool_count-1], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, b121314, ecmdata->pool_modinv_value, 0);
			
			normfree (b14);
			normfree (b1011);
			normfree (b1213);
			normfree (b121314);
			normfree (b6vals);
		}

		else if (ecmdata->pool_count >= 8 && ecmdata->pool_count % 7 == 1) {
			
			ecmdata->poolz_count--;
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, ecmdata->poolz_values[ecmdata->poolz_count], ecmdata->pool_modinv_value, 0);
			normfree (ecmdata->poolz_values[ecmdata->poolz_count]);
		}
	}

	gwunfft (&ecmdata->gwdata, ecmdata->pool_modinv_value, ecmdata->pool_modinv_value);
	stop_reason = ecm_modinv (ecmdata, ecmdata->pool_modinv_value);
	if (stop_reason) return (stop_reason);
	if (ecmdata->factor != NULL) goto exit;

	switch (ecmdata->pool_type) {

	case POOL_3MULT:
		for (uint32_t i = ecmdata->pool_count-1; ; i--) {
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, ecmdata->pool_values[i], ecmdata->pool_values[i], GWMUL_FFT_S1 | GWMUL_STARTNEXTFFT);
			if (i == 0) break;
			ecmdata->poolz_count--;
			gwmul3 (&ecmdata->gwdata, ecmdata->poolz_values[ecmdata->poolz_count], ecmdata->pool_modinv_value, ecmdata->pool_modinv_value, GWMUL_STARTNEXTFFT);
			normfree (ecmdata->poolz_values[ecmdata->poolz_count]);
		}
		break;

	case POOL_3POINT44MULT:
		for (uint32_t i = ecmdata->pool_count-1; ; i--) {
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, ecmdata->pool_values[i], ecmdata->pool_values[i], GWMUL_FFT_S1 | GWMUL_STARTNEXTFFT);
			if (i == 0) break;
			if (i < 4 || i % 3 != 1) continue;
			
			ecmdata->poolz_count--;
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, ecmdata->poolz_values[ecmdata->poolz_count], ecmdata->pool_modinv_value, GWMUL_STARTNEXTFFT);
			normfree (ecmdata->poolz_values[ecmdata->poolz_count]);
		}
		break;

	case POOL_3POINT57MULT:
		for (uint32_t i = ecmdata->pool_count-1; ; i--) {
			
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, ecmdata->pool_values[i], ecmdata->pool_values[i], GWMUL_FFT_S1 | GWMUL_STARTNEXTFFT);
			if (i == 0) break;
			if (i < 8 || i % 7 != 1) continue;
			
			ecmdata->poolz_count--;
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, ecmdata->poolz_values[ecmdata->poolz_count], ecmdata->pool_modinv_value, GWMUL_STARTNEXTFFT);
			normfree (ecmdata->poolz_values[ecmdata->poolz_count]);
		}
		break;

	case POOL_N_SQUARED:
		for (uint32_t i = 0; i < ecmdata->pool_count; i++)
			gwmul3 (&ecmdata->gwdata, ecmdata->pool_modinv_value, ecmdata->pool_values[i], ecmdata->pool_values[i], GWMUL_FFT_S1 | GWMUL_STARTNEXTFFT);
		break;
	}

exit:	ecmdata->pool_count = 0;
	ecmdata->poolz_count = 0;
	normfree (ecmdata->pool_modinv_value); ecmdata->pool_modinv_value = NULL;
	ecmdata->norm_pool_temps_count = 0;
	return (0);

oom:	return (OutOfMemory (ecmdata->thread_num));
}

int choose12 (
	ecmhandle *ecmdata,
	struct xz *xz,		
	struct ed *e)		
{
	int	stop_reason;
	bool	gen_edwards = !ecmdata->montg_stage1;	
	ASSERTG (gen_edwards || e == NULL);		

	gwnum u, v, vPu, vMu, u2, vP3u, vM3u, u3, v3, An, Ad;
	u = gwalloc (&ecmdata->gwdata);	if (u == NULL) goto oom;
	v = gwalloc (&ecmdata->gwdata);	if (v == NULL) goto oom;
	u64togw (&ecmdata->gwdata, ecmdata->sigma, v);
	gwsquare2 (&ecmdata->gwdata, v, u, 0);					
	gwsmalladd (&ecmdata->gwdata, -5, u);					
	gwsmallmul (&ecmdata->gwdata, 4.0, v);					

	vPu = gwalloc (&ecmdata->gwdata); if (vPu == NULL) goto oom;
	vMu = gwalloc (&ecmdata->gwdata); if (vMu == NULL) goto oom;
	gwaddsub4o (&ecmdata->gwdata, v, u, vPu, vMu, GWADD_GUARANTEED_OK);	
	u2 = gwalloc (&ecmdata->gwdata); if (u2 == NULL) goto oom;
	gwadd3o (&ecmdata->gwdata, u, u, u2, GWADD_GUARANTEED_OK);		

	vP3u = gwalloc (&ecmdata->gwdata); if (vP3u == NULL) goto oom;
	gwadd3o (&ecmdata->gwdata, vPu, u2, vP3u, GWADD_GUARANTEED_OK);		
	vM3u = NULL;
	if (gen_edwards) {
		vM3u = gwalloc (&ecmdata->gwdata); if (vM3u == NULL) goto oom;
		gwsub3o (&ecmdata->gwdata, vMu, u2, vM3u, GWADD_GUARANTEED_OK);	
	}
	if (e != NULL) {
		gwnum twoV1 = u2;									
		gwsmalladd (&ecmdata->gwdata, 10, u);							
		gwmul3 (&ecmdata->gwdata, twoV1, u, twoV1, GWMUL_PRESERVE_S2 | GWMUL_STARTNEXTFFT);	
		gwsmalladd (&ecmdata->gwdata, -6, u);							
		gwmul3 (&ecmdata->gwdata, twoV1, u, twoV1, GWMUL_PRESERVE_S2 | GWMUL_STARTNEXTFFT);	
		gwsmalladd (&ecmdata->gwdata, -24, u);							
		gwmul3 (&ecmdata->gwdata, twoV1, u, twoV1, GWMUL_PRESERVE_S2 | GWMUL_STARTNEXTFFT);	
		gwsmalladd (&ecmdata->gwdata, 20, u);							
	}

	u3 = gwalloc (&ecmdata->gwdata); if (u3 == NULL) goto oom;
	v3 = gwalloc (&ecmdata->gwdata); if (v3 == NULL) goto oom;
	gwsquare2 (&ecmdata->gwdata, u, u3, GWMUL_FFT_S1 | GWMUL_STARTNEXTFFT);
	gwmul3 (&ecmdata->gwdata, u, u3, u3, 0);					
	gwsquare2 (&ecmdata->gwdata, v, v3, GWMUL_FFT_S1 | GWMUL_STARTNEXTFFT);
	gwmul3 (&ecmdata->gwdata, v, v3, v3, 0);					

	gwmul3 (&ecmdata->gwdata, vMu, vP3u, vP3u, GWMUL_FFT_S1 | GWMUL_STARTNEXTFFT);	
	gwsquare2 (&ecmdata->gwdata, vMu, vMu, GWMUL_STARTNEXTFFT);			
	gwmul3 (&ecmdata->gwdata, vMu, vP3u, vP3u, 0);					
	An = vP3u;									
	if (gen_edwards) {								
		ecmdata->ed_a = gwalloc (&ecmdata->gwdata); if (ecmdata->ed_a == NULL) goto oom;
		gwfft (&ecmdata->gwdata, An, ecmdata->ed_a);
	}
	Ad = vMu;									
	gwsetmulbyconst (&ecmdata->gwdata, 4);
	gwmul3 (&ecmdata->gwdata, u3, v, Ad, GWMUL_MULBYCONST);				

	stop_reason = normalize (ecmdata, Ad, An);					
	if (stop_reason) return (stop_reason);
	gwfree (&ecmdata->gwdata, An);

	gwsmallmul (&ecmdata->gwdata, 4.0, Ad);

	gwfft (&ecmdata->gwdata, Ad, Ad);
	ecmdata->Ad4 = Ad;

	if (gen_edwards) {
		gwmul3 (&ecmdata->gwdata, vPu, vM3u, vM3u, GWMUL_FFT_S1 | GWMUL_STARTNEXTFFT);	
		gwsquare2 (&ecmdata->gwdata, vPu, vPu, GWMUL_STARTNEXTFFT);			
		gwmul3 (&ecmdata->gwdata, vPu, vM3u, vM3u, GWMUL_STARTNEXTFFT);			
		ecmdata->ed_d = vM3u;
	}

	if (e != NULL) {
		gwnum yz = vPu;								
		gwnum twoV1 = u2;							

		ed_alloc (ecmdata, e);

		gwmul3 (&ecmdata->gwdata, u, v, e->x, GWMUL_STARTNEXTFFT);		
		gwaddsub4o (&ecmdata->gwdata, u3, v3, yz, e->y, GWADD_GUARANTEED_OK);	

		gwmul3 (&ecmdata->gwdata, e->x, yz, e->x, GWMUL_STARTNEXTFFT);		
		gwmul3 (&ecmdata->gwdata, e->y, twoV1, e->y, GWMUL_STARTNEXTFFT);	
		gwmul3 (&ecmdata->gwdata, twoV1, yz, e->z, GWMUL_STARTNEXTFFT);		

		ASSERTG (ed_check (ecmdata, e));
	}

	if (xz != NULL) {
		ASSERTG (xz->x == NULL && xz->z == NULL);
		gwswap (xz->x, u3);	
		gwswap (xz->z, v3);	
	}

	gwfree (&ecmdata->gwdata, u);
	gwfree (&ecmdata->gwdata, v);
	gwfree (&ecmdata->gwdata, vPu);
	gwfree (&ecmdata->gwdata, u2);
	gwfree (&ecmdata->gwdata, u3);
	gwfree (&ecmdata->gwdata, v3);
	return (0);

oom:	return (OutOfMemory (ecmdata->thread_num));
}

int choose_atkin_morain (
	ecmhandle *ecmdata,
	struct ed *e)		
{
	
	struct ws w;
	ws_alloc (ecmdata, &w);
	u64togw (&ecmdata->gwdata, 12, w.x);
	u64togw (&ecmdata->gwdata, 40, w.y);
	u64togw (&ecmdata->gwdata, 1, w.z);

	ws_mul (ecmdata, &w, ecmdata->sigma);

#ifdef STRAIGHT_FORWARD
	
	ecm_modinv (ecmdata, w.z);				
	gwmul3 (&ecmdata->gwdata, w.z, w.x, w.x, 0);		
	gwmul3 (&ecmdata->gwdata, w.z, w.y, w.y, 0);		
	gwnum s = w.x;
	gwnum t = w.y;

	gwnum a = gwalloc (&ecmdata->gwdata);
	gwnum tmp = gwalloc (&ecmdata->gwdata);
	gwsetmulbyconst (&ecmdata->gwdata, 2);
	gwcopy (&ecmdata->gwdata, s, a);
	gwsmalladd (&ecmdata->gwdata, -9, a);				
	gwcopy (&ecmdata->gwdata, t, tmp);
	gwsmalladd (&ecmdata->gwdata, 25, tmp);				
	gwadd (&ecmdata->gwdata, a, tmp);				
	normalize (ecmdata, a, tmp);					

	gwnum b = gwalloc (&ecmdata->gwdata);
	gwcopy (&ecmdata->gwdata, a, b);
	gwsmallmul (&ecmdata->gwdata, 4, b);				
	gwsmalladd (&ecmdata->gwdata, 1, b);				
	gwmul3 (&ecmdata->gwdata, a, b, b, GWMUL_MULBYCONST);		
	gwsquare2 (&ecmdata->gwdata, a, tmp, 0);			
	gwsmallmul (&ecmdata->gwdata, 8, tmp);				
	gwsmalladd (&ecmdata->gwdata, -1, tmp);				
	normalize (ecmdata, b, tmp);					

	ecmdata->ed_d = gwalloc (&ecmdata->gwdata);
	gwnum b2m1 = gwalloc (&ecmdata->gwdata);
	gwadd3o (&ecmdata->gwdata, b, b, b2m1, GWADD_FORCE_NORMALIZE);	
	gwsmalladd (&ecmdata->gwdata, -1, b2m1);			
	gwsquare2 (&ecmdata->gwdata, b2m1, tmp, GWMUL_PRESERVE_S1);	
	gwadd3 (&ecmdata->gwdata, tmp, tmp, ecmdata->ed_d);		
	gwsmalladd (&ecmdata->gwdata, -1, ecmdata->ed_d);		
	gwsquare2 (&ecmdata->gwdata, tmp, tmp, 0);			
	normalize (ecmdata, ecmdata->ed_d, tmp);			

	if (e != NULL) {
		ed_alloc (ecmdata, e);
		gwadd3o (&ecmdata->gwdata, b2m1, b2m1, e->x, GWADD_FORCE_NORMALIZE); 
		gwsmalladd (&ecmdata->gwdata, -1, e->x);			
		gwadd3o (&ecmdata->gwdata, b2m1, e->x, e->z, GWADD_FORCE_NORMALIZE); 
		gwsmalladd (&ecmdata->gwdata, -1, e->z);			
		gwmul3 (&ecmdata->gwdata, b2m1, e->x, e->x, GWMUL_STARTNEXTFFT);
		gwsquare2 (&ecmdata->gwdata, t, e->y, GWMUL_PRESERVE_S1);	
		gwcopy (&ecmdata->gwdata, t, tmp);
		gwsmallmul (&ecmdata->gwdata, 50, tmp);				
		gwadd3 (&ecmdata->gwdata, e->y, tmp, e->y);			
		gwsquare2 (&ecmdata->gwdata, s, tmp, GWMUL_PRESERVE_S1 | GWMUL_STARTNEXTFFT); 
		gwmul3 (&ecmdata->gwdata, s, tmp, tmp, GWMUL_PRESERVE_S1 | GWMUL_MULBYCONST); 
		gwsub3 (&ecmdata->gwdata, e->y, tmp, e->y);			
		gwsquare2 (&ecmdata->gwdata, s, tmp, GWMUL_PRESERVE_S1);	
		gwsmallmul (&ecmdata->gwdata, 27, tmp);				
		gwadd3 (&ecmdata->gwdata, e->y, tmp, e->y);			
		gwsmalladd (&ecmdata->gwdata, -104, e->y);			
		gwmul3 (&ecmdata->gwdata, b2m1, e->y, e->y, GWMUL_STARTNEXTFFT);
		gwnum z2 = b2m1;
		gwcopy (&ecmdata->gwdata, t, z2);				
		gwcopy (&ecmdata->gwdata, s, tmp);				
		gwsmallmul (&ecmdata->gwdata, 3, tmp);				
		gwadd3o (&ecmdata->gwdata, z2, tmp, z2, GWADD_FORCE_NORMALIZE); 
		gwsmalladd (&ecmdata->gwdata, -2, z2);				
		gwadd3 (&ecmdata->gwdata, t, s, tmp);				
		gwsmalladd (&ecmdata->gwdata, 16, tmp);				
		gwmul3 (&ecmdata->gwdata, z2, tmp, z2, GWMUL_STARTNEXTFFT);	

		gwmul3 (&ecmdata->gwdata, e->x, z2, e->x, 0);
		gwmul3 (&ecmdata->gwdata, e->y, e->z, e->y, 0);
		gwmul3 (&ecmdata->gwdata, e->z, z2, e->z, 0);
		ed_extend (ecmdata, e, 0);

		ASSERTG (ed_check (ecmdata, e));
	}

	ws_free (ecmdata, &w);
	gwfree (&ecmdata->gwdata, tmp);
	gwfree (&ecmdata->gwdata, b2m1);

#else

	gwnum sN = w.x, tN = w.y, sD = w.z;
	gwnum aN = gwalloc (&ecmdata->gwdata);
	gwnum aD = gwalloc (&ecmdata->gwdata);
	gwnum tmp = gwalloc (&ecmdata->gwdata);
	gwcopy (&ecmdata->gwdata, sD, tmp);
	gwsmallmul (&ecmdata->gwdata, 9, tmp);					
	gwsub3o (&ecmdata->gwdata, sN, tmp, aN, GWADD_FORCE_NORMALIZE);		
	gwcopy (&ecmdata->gwdata, sD, tmp);
	gwsmallmul (&ecmdata->gwdata, 16, tmp);					
	gwadd3o (&ecmdata->gwdata, sN, tmp, aD, GWADD_DELAY_NORMALIZE);		
	gwadd3o (&ecmdata->gwdata, aD, tN, aD, GWADD_FORCE_NORMALIZE);		

	gwnum bN = gwalloc (&ecmdata->gwdata);
	gwnum bD = gwalloc (&ecmdata->gwdata);
	gwsetmulbyconst (&ecmdata->gwdata, 8);
	gwsquare2 (&ecmdata->gwdata, aN, bD, GWMUL_FFT_S1 | GWMUL_MULBYCONST);	
	gwsetmulbyconst (&ecmdata->gwdata, 2);
	gwmul3 (&ecmdata->gwdata, aN, aD, tmp, GWMUL_FFT_S2 | GWMUL_MULBYCONST);
	gwadd3o (&ecmdata->gwdata, bD, tmp, bN, GWADD_DELAY_NORMALIZE);		
	gwsquare2 (&ecmdata->gwdata, aD, tmp, 0);				
	gwsub3o (&ecmdata->gwdata, bD, tmp, bD, GWADD_FORCE_NORMALIZE);		

	gwnum b2m1N, b2m1D;
	gwadd3o (&ecmdata->gwdata, bN, bN, bN, GWADD_DELAY_NORMALIZE);		
	gwsub3o (&ecmdata->gwdata, bN, bD, b2m1N = bN, GWADD_FORCE_NORMALIZE);	
	b2m1D = bD;

	gwnum dN = aN, dD = aD;
	gwsquare2 (&ecmdata->gwdata, b2m1D, tmp, GWMUL_PRESERVE_S1);		
	gwsquare2 (&ecmdata->gwdata, b2m1N, dD, GWMUL_PRESERVE_S1);		
	gwadd3o (&ecmdata->gwdata, dD, dD, dN, GWADD_DELAY_NORMALIZE);		
	gwsub3o (&ecmdata->gwdata, dN, tmp, dN, GWADD_FORCE_NORMALIZE);		
	gwmul3 (&ecmdata->gwdata, dN, tmp, dN, GWMUL_STARTNEXTFFT);		
	gwsquare2 (&ecmdata->gwdata, dD, dD, GWMUL_STARTNEXTFFT);		

	ecmdata->Ad4 = gwalloc (&ecmdata->gwdata);
	gwsub3o (&ecmdata->gwdata, dD, dN, ecmdata->Ad4, GWADD_MUL_INPUT);

	gwnum	inv_sD, inv_dD, inv_sDdD, s, t;
	gwmul3 (&ecmdata->gwdata, sD, dD, tmp, GWMUL_FFT_S12);						
	ecm_modinv (ecmdata, inv_sDdD = tmp);								
	gwmul3 (&ecmdata->gwdata, inv_sDdD, dD, inv_sD = dD, GWMUL_FFT_S1 | GWMUL_STARTNEXTFFT);	
	gwmul3 (&ecmdata->gwdata, inv_sDdD, sD, inv_dD = sD, GWMUL_STARTNEXTFFT);			
	gwmul3 (&ecmdata->gwdata, dN, inv_dD, ecmdata->ed_d = dN, GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);	
	gwfft (&ecmdata->gwdata, ecmdata->ed_d, ecmdata->ed_d);
	gwmul3 (&ecmdata->gwdata, ecmdata->Ad4, inv_dD, ecmdata->Ad4, GWMUL_STARTNEXTFFT);		
	gwfft (&ecmdata->gwdata, ecmdata->Ad4, ecmdata->Ad4);
	gwmul3 (&ecmdata->gwdata, sN, inv_sD, s = sN, 0);						
	gwmul3 (&ecmdata->gwdata, tN, inv_sD, t = tN, 0);						

	if (e != NULL) {
		gwnum xN, xD, yN, yD, tmp2;
		ed_alloc (ecmdata, e);
		xN = e->x; yN = e->y; xD = e->z; yD = sD; tmp2 = dD;
		
		gwadd3o (&ecmdata->gwdata, b2m1N, b2m1N, xN, GWADD_DELAY_NORMALIZE);		
		gwsub3o (&ecmdata->gwdata, xN, b2m1D, xN, GWADD_FORCE_NORMALIZE);		
		gwadd3o (&ecmdata->gwdata, b2m1N, xN, xD, GWADD_DELAY_NORMALIZE);		
		gwsub3o (&ecmdata->gwdata, xD, b2m1D, xD, GWADD_FORCE_NORMALIZE);		
		
		gwadd3 (&ecmdata->gwdata, t, s, tmp);						
		gwadd3o (&ecmdata->gwdata, tmp, s, yD, GWADD_DELAY_NORMALIZE);			
		gwadd3o (&ecmdata->gwdata, yD, s, yD, GWADD_FORCE_NORMALIZE);			
		gwsmalladd (&ecmdata->gwdata, -2, yD);						
		gwsmalladd (&ecmdata->gwdata, 16, tmp);						
		gwmul3 (&ecmdata->gwdata, yD, tmp, yD, GWMUL_STARTNEXTFFT);			
		
		gwsquare2 (&ecmdata->gwdata, t, yN, GWMUL_PRESERVE_S1);				
		gwsquare2 (&ecmdata->gwdata, s, tmp, GWMUL_FFT_S1);				
		gwmul3 (&ecmdata->gwdata, s, tmp, tmp2, GWMUL_PRESERVE_S2 | GWMUL_MULBYCONST);	
		gwsub3o (&ecmdata->gwdata, yN, tmp2, yN, GWADD_DELAY_NORMALIZE);		
		gwsmallmul (&ecmdata->gwdata, 50, t);						
		gwadd3o (&ecmdata->gwdata, yN, t, yN, GWADD_DELAY_NORMALIZE);			
		gwsmallmul (&ecmdata->gwdata, 27, tmp);						
		gwadd3o (&ecmdata->gwdata, yN, tmp, yN, GWADD_FORCE_NORMALIZE);			
		gwsmalladd (&ecmdata->gwdata, -104, yN);					
		
		gwmul3 (&ecmdata->gwdata, xN, yD, e->x = xN, GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
		gwmul3 (&ecmdata->gwdata, yN, xD, e->y = yN, GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
		gwmul3 (&ecmdata->gwdata, xD, yD, e->z = xD, GWMUL_STARTNEXTFFT);
		
		gwmul3 (&ecmdata->gwdata, b2m1N, e->x, e->x, GWMUL_FFT_S1);
		gwmul3 (&ecmdata->gwdata, b2m1N, e->y, e->y, 0);
		gwmul3 (&ecmdata->gwdata, b2m1D, e->z, e->z, 0);
		
		ASSERTG (ed_check (ecmdata, e));
	}

	ws_free (ecmdata, &w);
	gwfree (&ecmdata->gwdata, tmp);
	gwfree (&ecmdata->gwdata, b2m1N);
	gwfree (&ecmdata->gwdata, b2m1D);
	gwfree (&ecmdata->gwdata, dD);
#endif

	return (0);
}

int choose_gmp_ecm_param3 (
	ecmhandle *ecmdata)
{
	gwnum	An, Ad;
	int	stop_reason;

	An = gwalloc (&ecmdata->gwdata);	if (An == NULL) goto oom;
	Ad = gwalloc (&ecmdata->gwdata);	if (Ad == NULL) goto oom;
	u64togw (&ecmdata->gwdata, ecmdata->sigma, An);
	gwsmallmul (&ecmdata->gwdata, 4.0, An);					
	u64togw (&ecmdata->gwdata, 1ULL << 32, Ad);				

	stop_reason = normalize (ecmdata, Ad, An);				
	if (stop_reason) return (stop_reason);
	gwfree (&ecmdata->gwdata, An);

	gwsmallmul (&ecmdata->gwdata, 4.0, Ad);

	gwfft (&ecmdata->gwdata, Ad, Ad);
	ecmdata->Ad4 = Ad;

	return (0);

oom:	return (OutOfMemory (ecmdata->thread_num));
}

void ed_to_Montgomery (
	ecmhandle *ecmdata)
{

	ASSERTG (ecmdata->Ad4 != NULL);			
	
	if (ecmdata->e.x != NULL) {			
		ASSERTG (ecmdata->xz.x == NULL);	
		alloc_xz (ecmdata, &ecmdata->xz);
		gwaddsub4 (&ecmdata->gwdata, ecmdata->e.z, ecmdata->e.y, ecmdata->xz.x, ecmdata->xz.z);
	}

	ed_free (ecmdata, &ecmdata->dict_start);
	ed_free (ecmdata, &ecmdata->e);
	gwfree (&ecmdata->gwdata, ecmdata->ed_a), ecmdata->ed_a = NULL;
	gwfree (&ecmdata->gwdata, ecmdata->ed_d), ecmdata->ed_d = NULL;
	gwfree (&ecmdata->gwdata, ecmdata->ed_half), ecmdata->ed_half = NULL;
}

int init_curve (
	ecmhandle *ecmdata)
{
	int	stop_reason;

	if (ecmdata->sigma_type == 3) {
		stop_reason = choose_gmp_ecm_param3 (ecmdata);
	}
	
	else if (ecmdata->sigma_type == 1) {
		if (ecmdata->state != ECM_STATE_STAGE1_INIT)		
			stop_reason = choose12 (ecmdata, NULL, NULL);
		else if (ecmdata->montg_stage1)				
			stop_reason = choose12 (ecmdata, &ecmdata->xz, NULL);
		else							
			stop_reason = choose12 (ecmdata, NULL, &ecmdata->e);
	}
	
	else {
		if (ecmdata->state == ECM_STATE_STAGE1_INIT)		
			stop_reason = choose_atkin_morain (ecmdata, &ecmdata->e);
		else							
			stop_reason = choose_atkin_morain (ecmdata, NULL);
		if (ecmdata->montg_stage1)
			ed_to_Montgomery (ecmdata);			
	}
	return (stop_reason);
}

void ecm_calc_exp_internal (
	void	*sieve_info,	
	mpz_t	g,		
	uint64_t B1,		
	uint64_t *p,		
	uint64_t len)		
{

	if (len >= 1024) {
		mpz_t	x;
		ecm_calc_exp_internal (sieve_info, g, B1, p, len >> 1);
		mpz_init (x);
		mpz_set_ui (x, 1);
		ecm_calc_exp_internal (sieve_info, x, B1, p, len >> 1);
		mpz_mul (g, x, g);
		mpz_clear (x);
		return;
	}

	for ( ; *p <= B1 && mpz_sizeinbase (g, 2) < len; *p = sieve (sieve_info)) {
		uint64_t val, max;
		val = *p;
		max = B1 / *p;
		while (val <= max) val *= *p;
		if (sizeof (unsigned int) == 4 && val > 0xFFFFFFFF) {
			mpz_t	mpz_val;
			mpz_init_set_d (mpz_val, (double) val);		
			mpz_mul (g, g, mpz_val);
			mpz_clear (mpz_val);
		} else
			mpz_mul_ui (g, g, (unsigned int) val);
	}
}

void ecm_calc_exp (
	void	*sieve_info,	
	mpz_t	g,		
	uint64_t B1,		
	uint64_t *p,		
	uint64_t bufsize)	
{

	uint64_t start_prime = *p;
	uint64_t end_prime = B1;
	uint64_t expected_bits = (end_prime - start_prime) * 144/100;
	uint64_t bufsize_in_bits = bufsize * 8;
	
	if (expected_bits <= bufsize_in_bits) bufsize_in_bits = (end_prime - start_prime) * 150/100;
	
	else if (expected_bits <= 2*bufsize_in_bits) bufsize_in_bits = expected_bits / 2;

	if (start_prime != 2) mpz_set_ui (g, 1);
	else mpz_set_ui (g, 48);			

	ecm_calc_exp_internal (sieve_info, g, B1, p, bufsize_in_bits);
}

bool dict_normalize (
	ecmhandle *ecmdata,
	uint32_t first_index,		
	uint32_t last_index,		
	gwnum	**addr_of_next_available_gwnum)
{
	gwnum *next_available_gwnum = *addr_of_next_available_gwnum;

	if (first_index == 0 && ecmdata->NAF_dictionary[0].z == NULL) first_index = 1;

	gwarray temps = next_available_gwnum;
	ASSERTG (temps + last_index - first_index + 1 <= ecmdata->NAF_gwnums + 3 * ecmdata->NAF_dictionary_size);

	gwswap (ecmdata->NAF_dictionary[first_index].z, temps[0]);
	for (uint32_t i = 1; i <= last_index - first_index; i++) {
		int options = (i == last_index - first_index) ? 0 : GWMUL_STARTNEXTFFT;
		gwmul3 (&ecmdata->gwdata, temps[i-1], ecmdata->NAF_dictionary[first_index+i].z, temps[i], GWMUL_FFT_S12 | options);
		ASSERTG (ecmdata->NAF_dictionary[first_index+i].t == NULL);
	}

	gwnum big_inverse = temps[last_index - first_index];
	if (ecm_modinv (ecmdata, big_inverse)) return (FALSE);

	for (uint32_t i = last_index - first_index; i; i--) {
		gwmul3 (&ecmdata->gwdata, big_inverse, temps[i-1], temps[i-1], GWMUL_FFT_S1 | GWMUL_STARTNEXTFFT);			
		gwmul3 (&ecmdata->gwdata, big_inverse, ecmdata->NAF_dictionary[first_index+i].z, big_inverse, GWMUL_STARTNEXTFFT);	
		
		gwmul3 (&ecmdata->gwdata, ecmdata->NAF_dictionary[first_index+i].x, temps[i-1], ecmdata->NAF_dictionary[first_index+i].x, GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
		gwmul3 (&ecmdata->gwdata, ecmdata->NAF_dictionary[first_index+i].y, temps[i-1], ecmdata->NAF_dictionary[first_index+i].y, GWMUL_STARTNEXTFFT);
		
		*--next_available_gwnum = ecmdata->NAF_dictionary[first_index+i].z, ecmdata->NAF_dictionary[first_index+i].z = NULL;
	}
	
	gwmul3 (&ecmdata->gwdata, ecmdata->NAF_dictionary[first_index].x, big_inverse, ecmdata->NAF_dictionary[first_index].x, GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
	gwmul3 (&ecmdata->gwdata, ecmdata->NAF_dictionary[first_index].y, big_inverse, ecmdata->NAF_dictionary[first_index].y, GWMUL_STARTNEXTFFT);
	
	gwswap (ecmdata->NAF_dictionary[first_index].z, temps[0]);
	*--next_available_gwnum = ecmdata->NAF_dictionary[first_index].z, ecmdata->NAF_dictionary[first_index].z = NULL;

	*addr_of_next_available_gwnum = next_available_gwnum;
	return (TRUE);
}

bool NAF_dictionary_init (
	ecmhandle *ecmdata,
	int	*first_NAF_index,	
	uint64_t *num_doublings)	
{
	
	ecmdata->NAF_dictionary = (struct ed *) malloc (ecmdata->NAF_dictionary_size * sizeof (struct ed));
	if (ecmdata->NAF_dictionary == NULL) return (FALSE);
	ecmdata->NAF_gwnums = gwalloc_array (&ecmdata->gwdata, 3 * ecmdata->NAF_dictionary_size);
	if (ecmdata->NAF_gwnums == NULL) return (FALSE);
	memset (ecmdata->NAF_dictionary, 0, ecmdata->NAF_dictionary_size * sizeof (struct ed));
	gwnum	*next_available_gwnum = ecmdata->NAF_gwnums;

	ecmdata->NAF_dictionary[0].x = *next_available_gwnum++;
	ecmdata->NAF_dictionary[0].y = *next_available_gwnum++;
	ecmdata->NAF_dictionary[0].t = *next_available_gwnum++;
	gwcopy (&ecmdata->gwdata, ecmdata->dict_start.x, ecmdata->NAF_dictionary[0].x);
	gwcopy (&ecmdata->gwdata, ecmdata->dict_start.y, ecmdata->NAF_dictionary[0].y);
	if (ecmdata->dict_start.z != NULL) {
		ecmdata->NAF_dictionary[0].z = *next_available_gwnum++;
		gwcopy (&ecmdata->gwdata, ecmdata->dict_start.z, ecmdata->NAF_dictionary[0].z);
	}
	ed_free (ecmdata, &ecmdata->dict_start);
			
	struct ed *two = &ecmdata->NAF_dictionary[ecmdata->NAF_dictionary_size-1];
	two->x = *next_available_gwnum++;
	two->y = *next_available_gwnum++;
	two->z = *next_available_gwnum++;
	two->t = *next_available_gwnum++;
	ed_dbl (ecmdata, &ecmdata->NAF_dictionary[0], two, ED_NO_GWALLOC | ED_FFT_S1 | ED_XYZ | ED_RESULT_FOR_ADD | ED_STARTNEXTFFT);

	ed_extend (ecmdata, &ecmdata->NAF_dictionary[0], ED_FORCE_EXTEND);
	for (uint32_t i = 1; i < ecmdata->NAF_dictionary_size; i++) {
		
		if (i != ecmdata->NAF_dictionary_size-1) {
			ecmdata->NAF_dictionary[i].x = *next_available_gwnum++;
			ecmdata->NAF_dictionary[i].y = *next_available_gwnum++;
			ecmdata->NAF_dictionary[i].z = *next_available_gwnum++;
			ecmdata->NAF_dictionary[i].t = *next_available_gwnum++;
		}
		
		int options = (i == ecmdata->NAF_dictionary_size / 2 || i == ecmdata->NAF_dictionary_size - 1) ? 0 : ED_RESULT_FOR_ADD;
		ed_add (ecmdata, &ecmdata->NAF_dictionary[i-1], two, &ecmdata->NAF_dictionary[i], options | ED_NO_GWALLOC | ED_FFT_S1Z | ED_XYZ | ED_STARTNEXTFFT);
		
		*--next_available_gwnum = ecmdata->NAF_dictionary[i-1].t, ecmdata->NAF_dictionary[i-1].t = NULL;
		if (options == 0) *--next_available_gwnum = ecmdata->NAF_dictionary[i].t, ecmdata->NAF_dictionary[i].t = NULL;
		
		if (i == ecmdata->NAF_dictionary_size / 2) {
			*--next_available_gwnum = two->t, two->t = NULL;	
			ed_swap (*two, ecmdata->NAF_dictionary[i+1]);		
			if (!dict_normalize (ecmdata, 0, i+1, &next_available_gwnum)) return (FALSE);
			ed_swap (*two, ecmdata->NAF_dictionary[i+1]);		
			
			ecmdata->NAF_dictionary[i].t = *next_available_gwnum++;
			ed_extend (ecmdata, &ecmdata->NAF_dictionary[i], ED_FORCE_EXTEND);
			two->t = *next_available_gwnum++;
			ed_extend (ecmdata, two, ED_FORCE_EXTEND);
		}
	}

	if (!dict_normalize (ecmdata, ecmdata->NAF_dictionary_size / 2 + 1, ecmdata->NAF_dictionary_size - 1, &next_available_gwnum)) return (FALSE);

	for (uint32_t i = 0; i < ecmdata->NAF_dictionary_size; i++) {
		ASSERTG (ecmdata->NAF_dictionary[i].t == NULL && ecmdata->NAF_dictionary[i].z == NULL);
		ASSERTG (next_available_gwnum < ecmdata->NAF_gwnums + 3 * ecmdata->NAF_dictionary_size);
		ecmdata->NAF_dictionary[i].t = *next_available_gwnum++;
		ed_extend (ecmdata, &ecmdata->NAF_dictionary[i], ED_FORCE_EXTEND);
	}

	uint64_t len = mpz_sizeinbase (ecmdata->stage1_exp, 2);
	ecmdata->NAF_codes = (char *) malloc ((size_t) divide_rounding_up (len, 8));
	memset (ecmdata->NAF_codes, 0, (size_t) divide_rounding_up (len, 8));

	uint64_t bitnum = len - 1;
	int max_dictionary_value = 2 * ecmdata->NAF_dictionary_size - 1;
	int first_NAF_value;
	for (int tmp = 0; tmp * 2 + 1 <= max_dictionary_value; bitnum--) {
		tmp *= 2;
		if (mpz_tstbit64 (ecmdata->stage1_exp, bitnum)) {
			mpz_clrbit64 (ecmdata->stage1_exp, bitnum);	
			tmp++;
			first_NAF_value = tmp;
			*num_doublings = bitnum;
		}
		if (bitnum == 0) break;
	}

	int NAF_value_under_construction = 0;
	int NAF_value_addin = 1;
	int carry = 0;
	for (bitnum = 0; bitnum < *num_doublings; bitnum++) {
		
		int this_bit = carry + (mpz_tstbit64 (ecmdata->stage1_exp, bitnum) ? 1 : 0);
		carry = this_bit / 2;
		this_bit = this_bit & 1;
		
		if (NAF_value_under_construction == 0 && this_bit) mpz_setbit64 (ecmdata->stage1_exp, bitnum);
		else mpz_clrbit64 (ecmdata->stage1_exp, bitnum);
		
		if (this_bit) NAF_value_under_construction += NAF_value_addin;
		if (NAF_value_under_construction == 0) continue;
		NAF_value_addin *= 2;
		
		if (bitnum != *num_doublings - 1) {
			if (NAF_value_addin < max_dictionary_value) continue;
			if (NAF_value_under_construction <= max_dictionary_value && NAF_value_addin - NAF_value_under_construction <= max_dictionary_value) continue;
		}
		
		int NAF_code;
		if (NAF_value_under_construction <= max_dictionary_value) NAF_code = (NAF_value_under_construction - 1) / 2;
		else NAF_code = (NAF_value_addin - NAF_value_under_construction - 1) / 2 + ecmdata->NAF_dictionary_size, carry = 1;
		for (uint64_t write_bitnum = bitnum; NAF_value_addin >>= 1; write_bitnum--) {
			if (NAF_code & NAF_value_addin) bitset (ecmdata->NAF_codes, write_bitnum);
		}
		
		NAF_value_under_construction = 0;
		NAF_value_addin = 1;
	}
	
	if (carry) for (first_NAF_value = first_NAF_value + 1; (first_NAF_value & 1) == 0; first_NAF_value /= 2) *num_doublings = *num_doublings + 1;
	*first_NAF_index = (first_NAF_value - 1) / 2;
	
	return (TRUE);
}

void NAF_code (			
	ecmhandle *ecmdata,
	uint64_t i,		
	int	*NAF_index,	
	bool	*subtract)	
{
	int NAF_code = 0;
	for (int addin = 1; addin < (int) (2*ecmdata->NAF_dictionary_size); addin <<= 1, i++) {
		if (bittst (ecmdata->NAF_codes, i)) NAF_code += addin;
	}
	if (NAF_code < (int) ecmdata->NAF_dictionary_size) *NAF_index = NAF_code, *subtract = FALSE;
	else *NAF_index = NAF_code - ecmdata->NAF_dictionary_size, *subtract = TRUE;
	ASSERTG (*NAF_index < (int) ecmdata->NAF_dictionary_size);
}

void NAF_dictionary_free (
	ecmhandle *ecmdata)
{
	free (ecmdata->NAF_codes), ecmdata->NAF_codes = NULL;
	gwfree_array (&ecmdata->gwdata, ecmdata->NAF_gwnums), ecmdata->NAF_gwnums = NULL;
	free (ecmdata->NAF_dictionary), ecmdata->NAF_dictionary = NULL;
}

uint32_t NAF_best_size (	
	ecmhandle *ecmdata)
{
	uint64_t explen = mpz_sizeinbase (ecmdata->stage1_exp, 2);

#define evalcost(x,s)	{ x.size = s; x.totcost = x.size * single_init_cost + (double) explen / (log2 (x.size) + 3.0) * single_ed_add_cost; }
	double	single_init_cost = (ecmdata->ed_a == NULL) ? 27.0 : 29.0;
	double	single_ed_add_cost = (ecmdata->ed_a == NULL) ? 14.0 : 16.0;
	struct { uint32_t size; double totcost; } best[3], midpoint;
	evalcost (best[0], 1);
	evalcost (best[1], 1000);
	evalcost (best[2], 1000000);
	while (best[2].size - best[0].size > 2) {
		
		if (best[1].size - best[0].size > best[2].size - best[1].size) {	
			evalcost (midpoint, (best[0].size + best[1].size) / 2);
			if (midpoint.totcost < best[1].totcost) {			
				best[2] = best[1];
				best[1] = midpoint;
			} else {							
				best[0] = midpoint;
			}
		} else {								
			evalcost (midpoint, (best[1].size + best[2].size) / 2);
			if (midpoint.totcost < best[1].totcost) {			
				best[0] = best[1];
				best[1] = midpoint;
			} else {							
				best[2] = midpoint;
			}
		}
	}
#undef evalcost
	return (best[1].size);
}

void curve_start_msg (
	ecmhandle *ecmdata)
{
	char	buf[120];

	if (ecmdata->N_short_string_rep == NULL) {
		ecmdata->N_short_string_rep = ecmdata->N_full_string_rep = gwmodulo_as_string (&ecmdata->gwdata);
		if (ecmdata->N_short_string_rep[0] == 'A') {
			mpz_t	N;
			int	len;
			mpz_init (N);
			gtompz (ecmdata->N, N);
			ecmdata->N_full_string_rep = (char *) malloc (mpz_sizeinbase (N, 10) + 2);
			mpz_get_str (ecmdata->N_full_string_rep, 10, N);
			len = (int) strlen (ecmdata->N_full_string_rep);
			ecmdata->N_short_string_rep = (char *) malloc (50);
			if (len <= 20) strcpy (ecmdata->N_short_string_rep, ecmdata->N_full_string_rep);
			else sprintf (ecmdata->N_short_string_rep, "%.10s...%s (%d digits)", ecmdata->N_full_string_rep, ecmdata->N_full_string_rep + len - 10, len);
			mpz_clear (N);
		}
	}

	if (ecmdata->curve != 1) OutputStr (ecmdata->thread_num, "\n");
	sprintf (buf, "%s ECM curve #%" PRIu32, ecmdata->N_short_string_rep, ecmdata->curve);
	title (ecmdata->thread_num, buf);

	sprintf (buf, "ECM on %s: %s curve #%" PRIu32 " with s=%" PRIu64 ", B1=%" PRIu64 ", ",
		 ecmdata->N_short_string_rep, ecmdata->sigma_type == 3 ? "GMP-ECM-param3" : ecmdata->sigma_type == 1 ? "Montgomery" : "Edwards",
		 ecmdata->curve, ecmdata->sigma, ecmdata->B);
	if (!ecmdata->optimal_B2 || ecmdata->state >= ECM_STATE_STAGE2) sprintf (buf + strlen (buf), "B2=%" PRIu64 "\n", ecmdata->C);
	else sprintf (buf + strlen (buf), "B2=TBD\n");
	OutputStr (ecmdata->thread_num, buf);
}

int mQ_preinit (
	ecmhandle *ecmdata,
	Dmultiple_data_map &Dmultiple_map)	
{
	uint64_t m;				
	Dmultiple_data_map::iterator it_Dmult;

	m = divide_rounding_up (ecmdata->B2_start, ecmdata->D) + ecmdata->Dsection;
	ASSERTG (m > 1); 

	if (!alloc_xz (ecmdata, &ecmdata->Qprevm)) goto oom;
	it_Dmult = Dmultiple_map.find (m);
	if (it_Dmult == Dmultiple_map.end()) it_Dmult = Dmultiple_map.insert({m, {0, m}}).first;
	it_Dmult->second.set_val_buffers (&ecmdata->gwdata, ecmdata->Qprevm.x, ecmdata->Qprevm.z, TRUE);

	if (!alloc_xz (ecmdata, &ecmdata->Qm)) goto oom;
	it_Dmult = Dmultiple_map.find (m+1);
	if (it_Dmult == Dmultiple_map.end()) it_Dmult = Dmultiple_map.insert({m+1, {0, m+1}}).first;
	it_Dmult->second.set_val_buffers (&ecmdata->gwdata, ecmdata->Qm.x, ecmdata->Qm.z, TRUE);

	if (ecmdata->TWO_FFT_STAGE2 && ecmdata->Dsection + ecmdata->E < ecmdata->numDsections) {
		if (!alloc_xz (ecmdata, &ecmdata->QD_Eover2)) goto oom;
		it_Dmult = Dmultiple_map.find ((uint64_t)(ecmdata->E/2));
		if (it_Dmult == Dmultiple_map.end()) it_Dmult = Dmultiple_map.insert({(uint64_t)(ecmdata->E/2), {0,(uint64_t)(ecmdata->E/2)}}).first;
		it_Dmult->second.set_val_buffers (&ecmdata->gwdata, ecmdata->QD_Eover2.x, ecmdata->QD_Eover2.z, TRUE);
	}

	return (0);

oom:	return (OutOfMemory (ecmdata->thread_num));
}
int mQ_init (
	ecmhandle *ecmdata)
{
	int	stop_reason;

	if (ecmdata->TWO_FFT_STAGE2) {

		ecmdata->mQx = (gwnum *) malloc (ecmdata->E * sizeof (gwnum));
		if (ecmdata->mQx == NULL) goto oom;
		for (uint32_t i = 0; i < ecmdata->E; i++) ecmdata->mQx[i] = NULL;

		for (ecmdata->mQx_count = 0; ecmdata->mQx_count < ecmdata->E; ecmdata->mQx_count++) {
			struct xz nextQm;

			if (ecmdata->mQx_count + 2 >= ecmdata->E ||
			    ecmdata->Dsection + ecmdata->mQx_count + 2 >= ecmdata->numDsections) {
				if (ecmdata->Qprevm.x == NULL) break;
				nextQm.x = NULL;
				nextQm.z = NULL;
			}

			else {
				nextQm.x = gwalloc (&ecmdata->gwdata);
				if (nextQm.x == NULL) return (OutOfMemory (ecmdata->thread_num));
				nextQm.z = gwalloc (&ecmdata->gwdata);
				if (nextQm.z == NULL) return (OutOfMemory (ecmdata->thread_num));
				
				ell_add_xz (ecmdata, &ecmdata->Qm, &ecmdata->QD, &ecmdata->Qprevm, &nextQm);
			}

			ecmdata->mQx[ecmdata->mQx_count] = ecmdata->Qprevm.x;
			stop_reason = add_to_normalize_pool_ro (ecmdata, ecmdata->mQx[ecmdata->mQx_count], ecmdata->Qprevm.z);
			if (stop_reason) return (stop_reason);

			ecmdata->Qprevm = ecmdata->Qm;
			ecmdata->Qm = nextQm;

			stop_reason = stopCheck (ecmdata->thread_num);
			if (stop_reason) return (stop_reason);
		}

		stop_reason = normalize_pool (ecmdata);
		if (stop_reason) return (stop_reason);
		if (ecmdata->factor != NULL) return (0);
		gwfree_cached (&ecmdata->gwdata);
		mallocFreeForOS ();

		free_xz (ecmdata, &ecmdata->QD);

		ecmdata->mQx_retcount = 0;
	}

	if (ecmdata->TWO_FFT_STAGE2 && ecmdata->Dsection + ecmdata->mQx_count < ecmdata->numDsections) {
		if (is_gwsetaddin_emulated (&ecmdata->gwdata)) {
			gwuser_init_FFT1 (&ecmdata->gwdata);
		} else {
			gwsetaddin (&ecmdata->gwdata, -1);
		}
	}

	ecmdata->Qm_state = 0;

	return (0);

oom:	return (OutOfMemory (ecmdata->thread_num));
}
int mQ_next (
	ecmhandle *ecmdata,
	gwnum	*retx,
	gwnum	*retz)
{
	int	stop_reason;

	if (!ecmdata->TWO_FFT_STAGE2) {
		
		if (ecmdata->Qm_state == 0) {
			ecmdata->Qm_state = 1;
			*retx = ecmdata->Qprevm.x;
			*retz = ecmdata->Qprevm.z;
		}
		
		else if (ecmdata->Qm_state == 1) {
			ecmdata->Qm_state = 2;
			*retx = ecmdata->Qm.x;
			*retz = ecmdata->Qm.z;
		}
		
		else {
			stop_reason = ell_add_xz_noscr (ecmdata, &ecmdata->Qm, &ecmdata->QD, &ecmdata->Qprevm, &ecmdata->Qprevm);
			if (stop_reason) return (stop_reason);
			xzswap (ecmdata->Qm, ecmdata->Qprevm);
			*retx = ecmdata->Qm.x;
			*retz = ecmdata->Qm.z;
		}
		
		return (0);
	}

	if (ecmdata->mQx_count == 0) {
		ASSERTG (ecmdata->E % 2 == 0);

		if (ecmdata->numDsections - ecmdata->Dsection > ecmdata->E) ecmdata->mQx_count = ecmdata->E;
		else ecmdata->mQx_count = (int) (ecmdata->numDsections - ecmdata->Dsection);

		for (uint32_t i = 0; i < ecmdata->E/2; i++) {
			struct xz nextQm, nextQm2;

			if (i >= ecmdata->mQx_count) break;

			nextQm.x = gwalloc (&ecmdata->gwdata);
			if (nextQm.x == NULL) return (OutOfMemory (ecmdata->thread_num));
			nextQm.z = ecmdata->mQx[i];

			ell_add_xz_3norm (ecmdata, ecmdata->mQx[i+ecmdata->E/2], ecmdata->QD_Eover2.x, ecmdata->mQx[i], &nextQm);

			if (i + ecmdata->E/2 < ecmdata->mQx_count) {
				
				nextQm2.x = ecmdata->mQx[i+ecmdata->E/2];
				nextQm2.z = gwalloc (&ecmdata->gwdata);
				if (nextQm.z == NULL) return (OutOfMemory (ecmdata->thread_num));

				ell_add_xz_2norm (ecmdata, &nextQm, ecmdata->QD_Eover2.x, ecmdata->mQx[i+ecmdata->E/2], &nextQm2);

				ecmdata->mQx[i+ecmdata->E/2] = nextQm2.x;
				stop_reason = add_to_normalize_pool_ro (ecmdata, ecmdata->mQx[i+ecmdata->E/2], nextQm2.z);
				if (stop_reason) return (stop_reason);
			}

			ecmdata->mQx[i] = nextQm.x;
			stop_reason = add_to_normalize_pool_ro (ecmdata, ecmdata->mQx[i], nextQm.z);
			if (stop_reason) return (stop_reason);

			stop_reason = stopCheck (ecmdata->thread_num);
			if (stop_reason) return (stop_reason);
		}

		stop_reason = normalize_pool (ecmdata);
		if (stop_reason) return (stop_reason);
		if (ecmdata->factor != NULL) return (0);
		gwfree_cached (&ecmdata->gwdata);
		mallocFreeForOS ();

		ecmdata->mQx_retcount = 0;
	}

	*retx = ecmdata->mQx[ecmdata->mQx_retcount++];
	ecmdata->mQx_count--;
	return (0);
}
void mQ_term (
	ecmhandle *ecmdata)
{
	free_xz (ecmdata, &ecmdata->Qm);
	free_xz (ecmdata, &ecmdata->Qprevm);
	free_xz (ecmdata, &ecmdata->QD);
	free_xz (ecmdata, &ecmdata->QD_Eover2);
	if (ecmdata->mQx != NULL) {
		for (uint32_t i = 0; i < ecmdata->E; i++) gwfree (&ecmdata->gwdata, ecmdata->mQx[i]);
		free (ecmdata->mQx); ecmdata->mQx = NULL;
	}
}

int mQ_preinit_array (
	ecmhandle *ecmdata,
	Dmultiple_data_map &Dmultiple_map)	
{
	uint64_t m;				
	Dmultiple_data_map::iterator it_Dmult;

	m = divide_rounding_up (ecmdata->B2_start, ecmdata->D) + ecmdata->Dsection;
	ASSERTG (m > 1); 

	ecmdata->Qprevm.x = ecmdata->polyGaux[0];
	ecmdata->Qprevm.z = gwalloc (&ecmdata->gwdata);
	if (ecmdata->Qprevm.z == NULL) return (OutOfMemory (ecmdata->thread_num));
	it_Dmult = Dmultiple_map.find (m);
	if (it_Dmult == Dmultiple_map.end()) it_Dmult = Dmultiple_map.insert({m, {0, m}}).first;
	it_Dmult->second.set_val_buffers (&ecmdata->gwdata, ecmdata->Qprevm.x, ecmdata->Qprevm.z, TRUE);

	ecmdata->Qm.x = ecmdata->polyGaux[1];
	ecmdata->Qm.z = gwalloc (&ecmdata->gwdata);
	if (ecmdata->Qm.z == NULL) return (OutOfMemory (ecmdata->thread_num));
	it_Dmult = Dmultiple_map.find (m+1);
	if (it_Dmult == Dmultiple_map.end()) it_Dmult = Dmultiple_map.insert({m+1, {0, m+1}}).first;
	it_Dmult->second.set_val_buffers (&ecmdata->gwdata, ecmdata->Qm.x, ecmdata->Qm.z, TRUE);

	if (!alloc_xz (ecmdata, &ecmdata->QD_Eover2)) return (OutOfMemory (ecmdata->thread_num));
	it_Dmult = Dmultiple_map.find ((uint64_t)(ecmdata->polyGaux_size/2));
	if (it_Dmult == Dmultiple_map.end()) it_Dmult = Dmultiple_map.insert({(uint64_t)(ecmdata->polyGaux_size/2), {0,(uint64_t)(ecmdata->polyGaux_size/2)}}).first;
	it_Dmult->second.set_val_buffers (&ecmdata->gwdata, ecmdata->QD_Eover2.x, ecmdata->QD_Eover2.z, TRUE);

	return (0);
}
int mQ_init_array (
	ecmhandle *ecmdata)
{
	int	stop_reason;

	for (int i = 2; i < ecmdata->polyGaux_size; i++) {
		struct xz nextQm;

		nextQm.x = ecmdata->polyGaux[i];
		nextQm.z = gwalloc (&ecmdata->gwdata);
		if (nextQm.z == NULL) return (OutOfMemory (ecmdata->thread_num));

		ell_add_xz (ecmdata, &ecmdata->Qm, &ecmdata->QD, &ecmdata->Qprevm, &nextQm);
		
		stop_reason = add_to_normalize_pool_ro (ecmdata, ecmdata->Qprevm.x, ecmdata->Qprevm.z);
		if (stop_reason) return (stop_reason);

		ecmdata->Qprevm = ecmdata->Qm;
		ecmdata->Qm = nextQm;

		stop_reason = stopCheck (ecmdata->thread_num);
		if (stop_reason) return (stop_reason);
	}

	stop_reason = add_to_normalize_pool_ro (ecmdata, ecmdata->Qprevm.x, ecmdata->Qprevm.z);
	if (stop_reason) return (stop_reason);
	ecmdata->Qprevm.z = NULL;
	stop_reason = add_to_normalize_pool_ro (ecmdata, ecmdata->Qm.x, ecmdata->Qm.z);
	if (stop_reason) return (stop_reason);
	ecmdata->Qm.z = NULL;

	stop_reason = normalize_pool (ecmdata);
	if (stop_reason) return (stop_reason);
	if (ecmdata->factor != NULL) return (0);
	gwfree_cached (&ecmdata->gwdata);
	mallocFreeForOS ();

	ecmdata->use_preallocated_norm_temps = TRUE;

	free_xz (ecmdata, &ecmdata->QD);

	ecmdata->mQx_retcount = 0;

	if (is_gwsetaddin_emulated (&ecmdata->gwdata)) {
		gwuser_init_FFT1 (&ecmdata->gwdata);
	} else {
		gwsetaddin (&ecmdata->gwdata, -1);
	}

	ecmdata->Qm_state = 0;

	return (0);
}
int mQ_next_array (
	ecmhandle *ecmdata,
	gwnum	*mQx_array)		
{
	int	next_Gaux = 0;		
	int	stop_reason;

	if (ecmdata->Qm_state == 0) {
		for (uint64_t i = 0; i < ecmdata->polyGaux_size; i++) gwswap (mQx_array[i], ecmdata->polyGaux[i]);
	} else {
		for (uint64_t i = 0; i < ecmdata->polyGaux_size; i++) gwswap (mQx_array[i], ecmdata->polyGaux[(i + (ecmdata->polyGaux_size & 1)) % ecmdata->polyGaux_size]);
	}

	uint64_t initial_i = (ecmdata->Qm_state == 0 ? ecmdata->polyGaux_size : 0);
	for (uint64_t i = initial_i; i < initial_i + ecmdata->polyGaux_size / 2; i++) {
		struct xz prevQm, Qm;

		if (ecmdata->Qm_state == 0) {
			prevQm.x = mQx_array[i - 2 * (ecmdata->polyGaux_size / 2)];
			prevQm.z = NULL;
			Qm.x = mQx_array[i - (ecmdata->polyGaux_size / 2)];
			Qm.z = NULL;
		}
		
		else {
			prevQm.x = mQx_array[i];
			prevQm.z = NULL;
			Qm.x = mQx_array[i + (ecmdata->polyGaux_size / 2)];
			Qm.z = NULL;
		}

		for (uint64_t j = i; j < ecmdata->poly_size; j += ecmdata->polyGaux_size / 2) {
			struct xz nextQm;

			nextQm.x = mQx_array[j];
			if (ecmdata->norm_pool_temps_count) nextQm.z = ecmdata->norm_pool_temps[--ecmdata->norm_pool_temps_count];
			else nextQm.z = ecmdata->polyGaux[next_Gaux++];
			ASSERTG (next_Gaux <= ecmdata->polyGaux_size);

			if (Qm.z == NULL) ell_add_xz_3norm (ecmdata, Qm.x, ecmdata->QD_Eover2.x, prevQm.x, &nextQm);
			else if (prevQm.z == NULL) ell_add_xz_2norm (ecmdata, &Qm, ecmdata->QD_Eover2.x, prevQm.x, &nextQm);
			else ell_add_xz_1norm (ecmdata, &Qm, ecmdata->QD_Eover2.x, &prevQm, &nextQm);

			if (prevQm.z != NULL) {
				
				if (ecmdata->norm_pool_temps_count < 1 && next_Gaux < ecmdata->polyGaux_size)
					ecmdata->norm_pool_temps[ecmdata->norm_pool_temps_count++] = ecmdata->polyGaux[next_Gaux++];
				stop_reason = add_to_normalize_pool_ro (ecmdata, prevQm.x, prevQm.z);
				if (stop_reason) return (stop_reason);
			}

			prevQm = Qm;
			Qm = nextQm;
		}

		if (prevQm.z != NULL) {
			
			if (ecmdata->norm_pool_temps_count < 1 && next_Gaux < ecmdata->polyGaux_size)
				ecmdata->norm_pool_temps[ecmdata->norm_pool_temps_count++] = ecmdata->polyGaux[next_Gaux++];
			stop_reason = add_to_normalize_pool_ro (ecmdata, prevQm.x, prevQm.z);
			if (stop_reason) return (stop_reason);
		}
		if (Qm.z != NULL) {
			
			if (ecmdata->norm_pool_temps_count < 1 && next_Gaux < ecmdata->polyGaux_size)
				ecmdata->norm_pool_temps[ecmdata->norm_pool_temps_count++] = ecmdata->polyGaux[next_Gaux++];
			stop_reason = add_to_normalize_pool_ro (ecmdata, Qm.x, Qm.z);
			if (stop_reason) return (stop_reason);
		}
	}
   
	while (ecmdata->norm_pool_temps_count < 2 && next_Gaux < ecmdata->polyGaux_size)
		ecmdata->norm_pool_temps[ecmdata->norm_pool_temps_count++] = ecmdata->polyGaux[next_Gaux++];
	stop_reason = normalize_pool (ecmdata);
	if (stop_reason) return (stop_reason);
	if (ecmdata->factor != NULL) return (0);

	ecmdata->Qm_state = 1;

	return (0);
}
void mQ_term_array (
	ecmhandle *ecmdata)
{
	gwfree (&ecmdata->gwdata, ecmdata->Qprevm.z), ecmdata->Qprevm.z = NULL;
	gwfree (&ecmdata->gwdata, ecmdata->Qm.z), ecmdata->Qm.z = NULL;
	free_xz (ecmdata, &ecmdata->QD);
	free_xz (ecmdata, &ecmdata->QD_Eover2);
}

void ecm_stage1_memory_usage (
	int	thread_num,
	ecmhandle *ecmdata)
{
	if (ecmdata->montg_stage1) set_memory_usage (thread_num, 0, cvt_gwnums_to_mem (&ecmdata->gwdata, 9));
	else set_memory_usage (thread_num, 0, cvt_gwnums_to_mem (&ecmdata->gwdata, ecmdata->NAF_dictionary_size * 3 + 8));
}

uint64_t max_safe_poly2_size (
	gwhandle *gwdata,
	uint64_t poly1_size,
	uint64_t desired_poly2_size)
{
	
	double safety_adjust = IniGetFloat (INI_FILE, "PolySafetyMargin", (float) 0.0);
	
	if (gw_passes_safety_margin (gwdata, polymult_safety_margin (poly1_size, desired_poly2_size) + safety_adjust)) return (desired_poly2_size);
	
	if (!gw_passes_safety_margin (gwdata, polymult_safety_margin (poly1_size, poly1_size) + safety_adjust)) return (0);
	uint64_t known_safe_size = poly1_size;
	while (desired_poly2_size - known_safe_size >= 2) {
		uint64_t midpoint = (known_safe_size + desired_poly2_size) / 2;
		if (gw_passes_safety_margin (gwdata, polymult_safety_margin (poly1_size, midpoint) + safety_adjust)) known_safe_size = midpoint;
		else desired_poly2_size = midpoint;
	}
	return (known_safe_size);
}

struct ecm_stage2_cost_data {
	
	struct common_cost_data c;
	
	int	impl;		
	bool	saving_Ftree_to_disk; 
	
	int	stage2_type;	
	int	pool_type;	
	int	TWO_FFT_STAGE2;	
	int	E;		
	int	Ftree_polys_in_mem; 
};

double ecm_stage2_cost (
	void	*data)		
{
	struct ecm_stage2_cost_data *cost_data = (struct ecm_stage2_cost_data *) data;
	int	e;
	int	beste;		
	double	numgcdsections;
	double	cost, cost_modinv;

	if (cost_data->stage2_type == ECM_STAGE2_PAIRING) {

	    if (cost_data->c.B2_start <= cost_data->c.D) return (-1.0);

#define ELL_ADD_COST		12
#define ELL_ADD_2NORM_COST	10
#define ELL_ADD_3NORM_COST	8
#define ELL_DBL_COST		11
#define	AVG_ELL_ADD_NORM_COST	((ELL_ADD_3NORM_COST + ELL_ADD_2NORM_COST) / 2)
#define	AVG_ELL_DBLADD_COST	((ELL_DBL_COST + ELL_ADD_COST) / 2)
#define N_SQUARED_POOL_COST(n)	(((double)((n)+3)) * ((double)((n)+3)) - 13.0)
#define MULT3_POOL_COST(n)	((double)(n) * (9-2))
#define MULT3POINT44_POOL_COST(n) ((double)(n) * (10.3333333333333-2.0))
#define MULT3POINT57_POOL_COST(n) ((double)(n) * (10.7142857142857-2.0))
#define NQX_SETUP_COST(fft4)	(((fft4) || cost_data->c.D % 3 != 0) ? \
				 1.0 * ELL_ADD_COST + 2.0 * ELL_DBL_COST + ((double) (cost_data->c.D / 2 - 1)) * ELL_ADD_COST : \
				 5.0 * ELL_ADD_COST + 3.0 * ELL_DBL_COST + ((double) (cost_data->c.D / 6 - 2)) * 2.0 * ELL_ADD_COST)

	    if (cost_data->impl == 0) {
		cost_data->c.est_init_transforms = NQX_SETUP_COST (TRUE) + N_SQUARED_POOL_COST (cost_data->c.totrels);  
		cost_data->c.est_stage2_transforms = (double) cost_data->c.numDsections * ELL_ADD_COST;
		cost_modinv = cost_data->c.modinv_cost;
		cost_data->TWO_FFT_STAGE2 = FALSE;
		cost_data->pool_type = POOL_N_SQUARED;
		cost_data->E = 0;
		cost_data->c.stage2_numvals = 9 + cost_data->c.totrels;
	    }

	    if (cost_data->impl == 1) {
		
		if (8 + cost_data->c.totrels * 2 + cost_data->c.numvals_consumed_by_pairmap > cost_data->c.numvals) return (-1.0);
		cost_data->c.est_init_transforms = NQX_SETUP_COST (TRUE) + MULT3_POOL_COST (cost_data->c.totrels);  
		cost_data->c.est_stage2_transforms = (double) cost_data->c.numDsections * ELL_ADD_COST;
		cost_modinv = cost_data->c.modinv_cost;
		cost_data->TWO_FFT_STAGE2 = FALSE;
		cost_data->pool_type = POOL_3MULT;
		cost_data->E = 0;
		cost_data->c.stage2_numvals = 8 + cost_data->c.totrels * 2;
	    }

	    if (cost_data->impl == 2) {
		
		if (8 + cost_data->c.totrels * 4 / 3 + 2 + cost_data->c.numvals_consumed_by_pairmap > cost_data->c.numvals) return (-1.0);
		cost_data->c.est_init_transforms = NQX_SETUP_COST (TRUE) + MULT3POINT44_POOL_COST (cost_data->c.totrels);  
		cost_data->c.est_stage2_transforms = (double) cost_data->c.numDsections * ELL_ADD_COST;
		cost_modinv = cost_data->c.modinv_cost;
		cost_data->TWO_FFT_STAGE2 = FALSE;
		cost_data->pool_type = POOL_3POINT44MULT;
		cost_data->E = 0;
		cost_data->c.stage2_numvals = 8 + cost_data->c.totrels * 4 / 3 + 2;
	    }

	    if (cost_data->impl == 3) {
		
		if (8 + cost_data->c.totrels * 8 / 7 + 5 + cost_data->c.numvals_consumed_by_pairmap > cost_data->c.numvals) return (-1.0);
		cost_data->c.est_init_transforms = NQX_SETUP_COST (TRUE) + MULT3POINT57_POOL_COST (cost_data->c.totrels);  
		cost_data->c.est_stage2_transforms = (double) cost_data->c.numDsections * ELL_ADD_COST;
		cost_modinv = cost_data->c.modinv_cost;
		cost_data->TWO_FFT_STAGE2 = FALSE;
		cost_data->pool_type = POOL_3POINT57MULT;
		cost_data->E = 0;
		cost_data->c.stage2_numvals = 8 + cost_data->c.totrels * 8 / 7 + 5;
	    }

	    if (cost_data->impl >= 4) {
		numgcdsections = floor ((1.0 + sqrt (1.0 + 12.0 * (double) cost_data->c.numDsections / cost_data->c.modinv_cost)) / 2.0);
		beste = (int) ceil ((double) cost_data->c.numDsections / numgcdsections);
		if ((beste & 1) && numgcdsections > 1.0) beste++;
	    }

	    if (cost_data->impl == 4) {
		int	max_n_squared_e = (int) sqrt (2 * (cost_data->c.modinv_cost + 9)); 
		
		if (13 + cost_data->c.totrels + 1 + cost_data->c.numvals_consumed_by_pairmap > cost_data->c.numvals) return (-1.0);
		
		e = (cost_data->c.numvals - 5 - cost_data->c.totrels - 1) & ~1;
		if (e <= 0) return (-1.0);
		
		if (e > max_n_squared_e) e = max_n_squared_e;
		
		if (e >= beste) e = beste;
		else {
			numgcdsections = ceil ((double) cost_data->c.numDsections / (double) e);
			e = (int) ceil ((double) cost_data->c.numDsections / numgcdsections);
			if (e & 1) e++;
		}
		
		cost_data->c.est_init_transforms = NQX_SETUP_COST (FALSE) +					
						   N_SQUARED_POOL_COST (cost_data->c.totrels) +			
						   e * ELL_ADD_COST + N_SQUARED_POOL_COST (e+1);		
		cost_data->c.est_stage2_transforms = (double) cost_data->c.numDsections +			
			(numgcdsections - 1.0) * (e * AVG_ELL_ADD_NORM_COST + N_SQUARED_POOL_COST (e));		
		cost_modinv = cost_data->c.modinv_cost * (1 + numgcdsections);
		cost_data->TWO_FFT_STAGE2 = TRUE;
		cost_data->pool_type = POOL_N_SQUARED;
		cost_data->E = e;
		cost_data->c.stage2_numvals = _intmax (12 + cost_data->c.totrels + 1, 5 + cost_data->c.totrels + e + 1);
	    }

	    if (cost_data->impl == 5) {
		
		if (12 + cost_data->c.totrels * 2 + cost_data->c.numvals_consumed_by_pairmap > cost_data->c.numvals) return (-1.0);
		
		e = ((cost_data->c.numvals - 5 - cost_data->c.totrels) / 2) & ~1;
		
		if (e >= beste) e = beste;
		else {
			numgcdsections = ceil ((double) cost_data->c.numDsections / (double) e);
			e = (int) ceil ((double) cost_data->c.numDsections / numgcdsections);
			if (e & 1) e++;
		}
		
		cost_data->c.est_init_transforms = NQX_SETUP_COST (FALSE) +					
						   MULT3_POOL_COST (cost_data->c.totrels) +			
						   e * ELL_ADD_COST + MULT3_POOL_COST (e+1);			
		cost_data->c.est_stage2_transforms = (double) cost_data->c.numDsections +			
			(numgcdsections - 1.0) * (e * AVG_ELL_ADD_NORM_COST + MULT3_POOL_COST (e));		
		cost_modinv = cost_data->c.modinv_cost * (1 + numgcdsections);
		cost_data->TWO_FFT_STAGE2 = TRUE;
		cost_data->pool_type = POOL_3MULT;
		cost_data->E = e;
		cost_data->c.stage2_numvals = _intmax (12 + cost_data->c.totrels * 2, 5 + cost_data->c.totrels + e * 2);
	    }

	    if (cost_data->impl == 6) {
		
		if (12 + cost_data->c.totrels * 4 / 3 + 2 + cost_data->c.numvals_consumed_by_pairmap > cost_data->c.numvals) return (-1.0);
		
		e = ((cost_data->c.numvals - 5 - cost_data->c.totrels - 2) * 3 / 4) & ~1;
		
		if (e >= beste) e = beste;
		else {
			numgcdsections = ceil ((double) cost_data->c.numDsections / (double) e);
			e = (int) ceil ((double) cost_data->c.numDsections / numgcdsections);
			if (e & 1) e++;
		}
		
		cost_data->c.est_init_transforms = NQX_SETUP_COST (FALSE) +					
						   MULT3POINT44_POOL_COST (cost_data->c.totrels) +		
						   e * ELL_ADD_COST + MULT3POINT44_POOL_COST (e+1);		
		cost_data->c.est_stage2_transforms = (double) cost_data->c.numDsections +			
			(numgcdsections - 1.0) * (e * AVG_ELL_ADD_NORM_COST + MULT3POINT44_POOL_COST (e));	
		cost_modinv = cost_data->c.modinv_cost * (1 + numgcdsections);
		cost_data->TWO_FFT_STAGE2 = TRUE;
		cost_data->pool_type = POOL_3POINT44MULT;
		cost_data->E = e;
		cost_data->c.stage2_numvals = _intmax (12 + cost_data->c.totrels * 4 / 3 + 2, 5 + cost_data->c.totrels + e * 4 / 3 + 2);
	    }

	    if (cost_data->impl == 7) {
		
		if (12 + cost_data->c.totrels * 8 / 7 + 5 + cost_data->c.numvals_consumed_by_pairmap > cost_data->c.numvals) return (-1.0);
		
		e = ((cost_data->c.numvals - 5 - cost_data->c.totrels - 5) * 7 / 8) & ~1;
		
		if (e >= beste) e = beste;
		else {
			numgcdsections = ceil ((double) cost_data->c.numDsections / (double) e);
			e = (int) ceil ((double) cost_data->c.numDsections / numgcdsections);
			if (e & 1) e++;
		}
		
		cost_data->c.est_init_transforms = NQX_SETUP_COST (FALSE) +					
						   MULT3POINT57_POOL_COST (cost_data->c.totrels) +		
						   e * ELL_ADD_COST + MULT3POINT57_POOL_COST (e+1);		
		cost_data->c.est_stage2_transforms = (double) cost_data->c.numDsections +			
			(numgcdsections - 1.0) * (e * AVG_ELL_ADD_NORM_COST + MULT3POINT57_POOL_COST (e));	
		cost_modinv = cost_data->c.modinv_cost * (1 + numgcdsections);
		cost_data->TWO_FFT_STAGE2 = TRUE;
		cost_data->pool_type = POOL_3POINT57MULT;
		cost_data->E = e;
		cost_data->c.stage2_numvals = _intmax (12 + cost_data->c.totrels * 8 / 7 + 5, 5 + cost_data->c.totrels + e * 8 / 7 + 5);
	    }

	    cost_data->c.est_init_transforms += (cost_data->c.totrels - cost_data->c.numrels) * ELL_ADD_COST;		

	    cost_data->c.est_init_transforms +=
		cost_data->c.numrels * cost_data->c.relp_sets[1] * ELL_ADD_COST +					  
		one_based_modulo (cost_data->c.totrels, cost_data->c.numrels) * cost_data->c.relp_sets[2] * ELL_ADD_COST; 

	    cost_data->c.est_init_transforms += (2.0 * log2((double)((cost_data->c.B2_start + cost_data->c.D / 2)) / cost_data->c.D) + cost_data->c.multiplier) * AVG_ELL_DBLADD_COST;

	    cost_data->c.est_stage2_transforms += cost_data->c.totrels;

	    cost_data->c.est_stage2_transforms += (cost_data->c.est_numpairs + cost_data->c.est_numsingles) * (cost_data->impl >= 4 ? 2.0 : 4.0);

	    cost = (cost_data->c.est_init_transforms + cost_data->c.est_stage2_transforms) * IniGetFloat (INI_FILE, "EcmTransformCost", (float) 1.0);
	    cost += cost_data->c.est_pairing_runtime * IniGetFloat (INI_FILE, "EcmPairRatioAdjust", 1.0);
	    cost += cost_modinv;
	}

	else {
		uint64_t poly_size;
		double	num_polys, mem_used_by_a_gwnum, mem_saved_by_compression, mem_used_by_polymult;

		poly_size = cost_data->c.numrels;
		if (max_safe_poly2_size (cost_data->c.gwdata, poly_size, poly_size) < poly_size) return (-1.0);

		if (poly_size < 8) return (-1.0);
		
if (cost_data->c.numvals < 6*32) return (-1.0);

		uint64_t num_polyG = divide_rounding_up (cost_data->c.numDsections, poly_size);
		if (num_polyG < 2) num_polyG = 2; 
		cost_data->c.numDsections = num_polyG * poly_size;
		if (cost_data->c.gap_end == 0) {
			for ( ; ; ) {
				uint64_t B2_end = cost_data->c.B2_start + cost_data->c.numDsections * cost_data->c.D;
				uint64_t B2_start = round_down_to_multiple_of (B2_end / cost_data->c.first_missing_prime, cost_data->c.D);
				if (cost_data->c.B2_start >= B2_start) break;
				cost_data->c.B2_start = B2_start;
			}
		}

		cost_data->pool_type = POOL_3POINT57MULT;
		num_polys = (cost_data->saving_Ftree_to_disk ? 4.0 : 6.0) + 1.0 / 7.0;
		if (num_polyG == 1) num_polys -= 1.0;

		cost_data->c.stage2_numvals = (int) (num_polys * poly_size) + 1;		
		mem_used_by_a_gwnum = (double) array_gwnum_size (cost_data->c.gwdata);
		mem_saved_by_compression = 2 * (double) poly_size * mem_used_by_a_gwnum * (1.0 - cost_data->c.poly_compression);
		if (num_polyG == 1) mem_saved_by_compression = 0.0;
		mem_used_by_polymult = (double) polymult_mem_required (cost_data->c.polydata, poly_size, poly_size, POLYMULT_INVEC1_MONIC);
		cost_data->c.stage2_numvals += (int) floor ((mem_used_by_polymult - mem_saved_by_compression) / mem_used_by_a_gwnum + 0.5);
		if (cost_data->c.stage2_numvals > cost_data->c.numvals) return (-1.0);

		double	polymult_cost;		
		polymult_cost = 2.145 + 0.58 * log2 ((double) poly_size / 105.0);
		double L2_cache_size = (CPU_NUM_L2_CACHES > 0 ? CPU_TOTAL_L2_CACHE_SIZE / CPU_NUM_L2_CACHES : 0) * 1024.0;
		double polymult_mem_per_thread = mem_used_by_polymult / cost_data->c.stage2_threads;
		if (polymult_mem_per_thread >= 8 * L2_cache_size) polymult_cost *= 2.05;
		else if (polymult_mem_per_thread > L2_cache_size) polymult_cost *= 1.0 + 1.05 * (polymult_mem_per_thread - L2_cache_size) / (7 * L2_cache_size);
		polymult_cost *= IniGetFloat (INI_FILE, "EcmPolymultCostAdjust", 1.0);

		cost_data->c.est_pairing_runtime = 0.0;

		cost_data->c.est_init_transforms = NQX_SETUP_COST (FALSE) +					
						   MULT3POINT57_POOL_COST (cost_data->c.numrels + 1);		
		cost_data->c.est_stage2_transforms = (double) cost_data->c.numDsections +			
			num_polyG * (poly_size * AVG_ELL_ADD_NORM_COST + MULT3POINT57_POOL_COST (poly_size));	
		cost_modinv = cost_data->c.modinv_cost * (1 + num_polyG);

		cost_data->c.est_init_polymult = log2 ((double) poly_size) * poly_size * polymult_cost;
		cost_data->c.est_init_transforms += log2 ((double) poly_size) * poly_size * 2.0;

		cost_data->c.est_init_polymult += 2 * (poly_size*2) * polymult_cost;
		cost_data->c.est_init_transforms += 2 * (poly_size*2) * 2.0;

		cost_data->c.est_stage2_polymult = num_polyG * log2 ((double) poly_size) * poly_size * polymult_cost;
		cost_data->c.est_stage2_transforms += num_polyG * log2 ((double) poly_size) * poly_size * 2.0;

		cost_data->c.est_stage2_polymult += (num_polyG-1) * 3.0 * (double) (poly_size*2) * polymult_cost;
		cost_data->c.est_stage2_transforms += (num_polyG-1) * (double) (poly_size*2 + poly_size + poly_size) * 2;

		cost_data->c.est_stage2_polymult += (double) (poly_size*2) * polymult_cost;
		cost_data->c.est_stage2_transforms += (double) poly_size * 2.0;

		int	num_Ftree_polys, Ftree_polys_in_mem, more_Ftree_polys_in_mem, total_Ftree_polys_in_mem, needing_rebuild;
		num_Ftree_polys = (int) ceil (log2 ((double) poly_size));
		Ftree_polys_in_mem = (cost_data->saving_Ftree_to_disk ? 0 : 2);
		more_Ftree_polys_in_mem = (int) divide_rounding_down (cost_data->c.numvals - cost_data->c.stage2_numvals, poly_size);

if (Ftree_polys_in_mem + more_Ftree_polys_in_mem > num_Ftree_polys - 1) more_Ftree_polys_in_mem = num_Ftree_polys - Ftree_polys_in_mem - 1;
		total_Ftree_polys_in_mem = Ftree_polys_in_mem + more_Ftree_polys_in_mem;
		cost_data->Ftree_polys_in_mem = total_Ftree_polys_in_mem;
		cost_data->c.stage2_numvals += more_Ftree_polys_in_mem * poly_size;
		if (total_Ftree_polys_in_mem < 2) cost_modinv += (cost_data->c.gwdata->b == 2 ? 13.2 : 687.0) * cost_data->c.stage2_threads * (2 - total_Ftree_polys_in_mem) * poly_size;

		needing_rebuild = num_Ftree_polys - intmax (total_Ftree_polys_in_mem, 2);
		if (needing_rebuild > 0) {
			cost_data->c.est_stage2_polymult += needing_rebuild * poly_size * polymult_cost;
			cost_data->c.est_stage2_transforms += needing_rebuild * poly_size * 2.0;
		}

		cost_data->c.est_stage2_polymult += log2 ((double) poly_size) * (3 * poly_size) * polymult_cost;
		cost_data->c.est_stage2_transforms += log2 ((double) poly_size) * poly_size * 2.0;

		cost_data->c.est_stage2_transforms += poly_size * 2.0;

		cost = (cost_data->c.est_init_transforms + cost_data->c.est_stage2_transforms) * IniGetFloat (INI_FILE, "EcmTransformCost", (float) 1.0);
		cost += (cost_data->c.est_init_polymult + cost_data->c.est_stage2_polymult) * IniGetFloat (INI_FILE, "EcmPolyRatioAdjust", 1.0);
		cost *= (double) cost_data->c.stage2_fftlen / (double) cost_data->c.stage1_fftlen;	
		cost += cost_modinv;
	}

	cost *= IniGetFloat (INI_FILE, "EcmStage2RatioAdjust", 1.0);
	cost *= (double) cost_data->c.stage1_threads / (double) cost_data->c.stage2_threads;
	cost_data->c.stage2_cost = cost;						
	cost_data->c.est_stage2_stage1_ratio = cost / cost_data->c.stage1_cost;		
	cost_data->c.total_cost = cost_data->c.stage1_cost + cost_data->c.stage2_cost + cost_data->c.gcd_cost;

	cost_data->c.efficiency = kruppa_adjust (cost_data->c.B2_start + cost_data->c.numDsections * cost_data->c.D, cost_data->c.B1) / cost_data->c.total_cost;
	return (cost_data->c.efficiency);
}

double ecm_stage2_impl_given_numvals (
	ecmhandle *ecmdata,
	uint64_t numvals,				
	int	forced_stage2_type,			
	struct ecm_stage2_cost_data *return_cost_data)	
{
	double	efficiency, best_efficiency;	
	struct ecm_stage2_cost_data cost_data;	

	if (ecmdata->sigma_type) cost_data.c.stage1_cost = 25.55 * (double) ecmdata->B;
	else cost_data.c.stage1_cost = 21.95 * (double) ecmdata->B;

	cost_data.c.poly_compression = (double) gwfftlen (&ecmdata->gwdata) * (double) sizeof (double) / (double) array_gwnum_size (&ecmdata->gwdata);
	if (IniGetInt (INI_FILE, "ECMPolyCompress", 1) == 2) cost_data.c.poly_compression *= 0.875;
	if (IniGetInt (INI_FILE, "ECMPolyCompress", 1) == 0) cost_data.c.poly_compression = 1.0;

	cost_data.saving_Ftree_to_disk = IniGetInt (INI_FILE, "EcmSaveFtreeToDisk", 0);

	best_efficiency = -1.0;
	cost_data.c.numvals = numvals;
	cost_data.c.use_poly_D_data = FALSE;
	cost_data.c.centers_on_Dmultiple = TRUE;
	cost_data.c.required_missing = ecmdata->required_missing;
	cost_data.c.gwdata = &ecmdata->gwdata;
	cost_data.c.polydata = &ecmdata->polydata;
	cost_data.c.stage1_fftlen = ecmdata->stage1_fftlen;
	cost_data.c.stage2_fftlen = gwfftlen (&ecmdata->gwdata);
	cost_data.c.stage1_threads = ecmdata->stage1_threads;
	cost_data.c.stage2_threads = ecmdata->stage2_threads;
	init_gcd_costs (ecmdata->gwdata.bit_length, &cost_data);
	for (int stage2_type = 0; stage2_type <= 1; stage2_type++) {	

	    if (forced_stage2_type != 99 && forced_stage2_type != stage2_type) continue;

	    for (int impl = 0; impl <= 7; impl++) {			

		if (stage2_type == 1) impl = 7;

		if (QA_TYPE != 0 && QA_TYPE != impl + 1) continue;

		cost_data.stage2_type = (stage2_type == 0 ? ECM_STAGE2_PAIRING : ECM_STAGE2_POLYMULT);
		cost_data.impl = impl;
		cost_data.c.use_poly_D_data = (stage2_type == 1);
		efficiency = best_stage2_impl (ecmdata->B, ecmdata->first_relocatable, ecmdata->last_relocatable, ecmdata->C_done, ecmdata->C,
					       numvals - 9, &ecm_stage2_cost, &cost_data);
		if (efficiency > best_efficiency) {
			best_efficiency = efficiency;
			*return_cost_data = cost_data;
		}
	    }

	}

	return (best_efficiency);
}

int ecm_choose_B2 (
	ecmhandle *ecmdata,
	uint64_t numvals,
	int	forced_stage2_type,		
	char	*optimal_B2_msg)		
{
	int	max_B2mult;
	struct ecm_stage2_cost_data cost_data;	
	struct ecm_stage2_efficiency {
		int	i;
		int	actual_i;
		double	efficiency;
	} best[3];

	if (forced_stage2_type == 99 && numvals >= 200) forced_stage2_type = 1;

	max_B2mult = IniGetInt (INI_FILE, "MaxOptimalB2Multiplier", (int) (numvals < 100000 ? numvals * 100 : 10000000));
#define kruppa(x,B2mult)	x.i = B2mult; \
				if (x.i > max_B2mult) x.i = max_B2mult; \
				ecmdata->C = x.i * ecmdata->B; \
				x.efficiency = ecm_stage2_impl_given_numvals (ecmdata, numvals, forced_stage2_type, &cost_data); \
				if (x.efficiency < 0.0) x.actual_i = x.i; \
				else x.actual_i = (int) ((cost_data.c.B2_start + cost_data.c.numDsections * cost_data.c.D) / ecmdata->B);

	kruppa (best[0], 50);
	kruppa (best[1], best[0].actual_i * 5);
	kruppa (best[2], best[1].actual_i * 5);
	if (best[0].efficiency < 0.0 && best[1].efficiency < 0.0 && best[2].efficiency < 0.0) return (FALSE);

	while (best[0].efficiency > best[1].efficiency && best[0].i > 1) {
		best[2] = best[1];
		best[1] = best[0];
		kruppa (best[0], best[0].i / 2);
	}

	while (best[2].efficiency > best[1].efficiency && best[2].i < max_B2mult) {
		best[0] = best[1];
		best[1] = best[2];
		kruppa (best[2], best[1].i * 5);
	}

	for ( ; ; ) {
		struct ecm_stage2_efficiency midpoint;

		ASSERTG (best[1].efficiency >= best[0].efficiency);
		ASSERTG (best[1].efficiency >= best[2].efficiency);

		int	lower_gap = best[1].i - best[0].actual_i;
		int	upper_gap = best[2].i - best[1].actual_i;
		if (lower_gap < 2 && upper_gap < 2) break;

		if (lower_gap > upper_gap) {					
			kruppa (midpoint, best[0].actual_i + lower_gap / 2);
			if (midpoint.efficiency > best[1].efficiency) {		
				best[2] = best[1];
				best[1] = midpoint;
			} else {						
				best[0] = midpoint;
			}
		} else {							
			kruppa (midpoint, best[1].actual_i + upper_gap / 2);
			if (midpoint.efficiency > best[1].efficiency) {		
				best[0] = best[1];
				best[1] = midpoint;
			} else {						
				best[2] = midpoint;
			}
		}
	}

	ecmdata->C = best[1].actual_i * ecmdata->B;
	sprintf (optimal_B2_msg, "Optimal B2 is %d*B1 = %" PRIu64 ".  ", best[1].actual_i, ecmdata->C);

	return (TRUE);
}

double ecm_stage2_impl (
	ecmhandle *ecmdata,
	unsigned int memory,		
	int	forced_stage2_type,	
	char	*msgbuf)		
{
	uint64_t numvals;					
	struct ecm_stage2_cost_data cost_data;			

	numvals = cvt_mem_to_array_gwnums_adj (&ecmdata->gwdata, memory, 0.0);
	if (numvals < 13) numvals = 13;

	if (ecmdata->state == ECM_STATE_MIDSTAGE) {
		
		ecmdata->first_relocatable = ecmdata->C_done = ecmdata->B;
		ecmdata->last_relocatable = 0;
		if (ecmdata->optimal_B2 && !ecm_choose_B2 (ecmdata, numvals, forced_stage2_type, msgbuf + strlen (msgbuf))) return (0.0);
	}
	if (ecmdata->state >= ECM_STATE_STAGE2 && ecmdata->stage2_type == ECM_STAGE2_POLYMULT) {
		ecmdata->first_relocatable = ecmdata->C_done > ecmdata->B ? ecmdata->C_done : ecmdata->B;
		ecmdata->last_relocatable = 0;
	}

	if (ecmdata->state >= ECM_STATE_STAGE2 &&				
	    ecmdata->stage2_type == ECM_STAGE2_PAIRING &&			
	    ecmdata->pairmap != NULL &&						
	    numvals >= ecmdata->stage2_numvals &&				
	    forced_stage2_type != 1 &&						
	    (numvals < ecmdata->stage2_numvals * 2 ||				
	     ecmdata->Dsection >= ecmdata->numDsections / 2))			
		return (0);							

	if (ecmdata->state == ECM_STATE_STAGE2 && ecmdata->stage2_type == ECM_STAGE2_PAIRING) {
		int max_relp_set = get_max_relp_set (ecmdata->relp_sets);
		if (ecmdata->Dsection > max_relp_set) ecmdata->C_done = ecmdata->B2_start + (ecmdata->Dsection - max_relp_set) * ecmdata->D;
	}

	cost_data.c.total_cost = 0.0;						
	ecm_stage2_impl_given_numvals (ecmdata, numvals, forced_stage2_type, &cost_data);
	sprintf (msgbuf + strlen (msgbuf), "Actual B2 will be %" PRIu64 ".  Curve is worth %.2f B2=100*B1 curves.\n",
		 cost_data.c.B2_start + cost_data.c.numDsections * cost_data.c.D,
		 kruppa_adjust (cost_data.c.B2_start + cost_data.c.numDsections * cost_data.c.D, ecmdata->B));

	if (ecmdata->state >= ECM_STATE_STAGE2 &&				
	    ecmdata->stage2_type == ECM_STAGE2_PAIRING &&			
	    ecmdata->pairmap != NULL &&						
	    forced_stage2_type != 1 &&						
	    numvals >= ecmdata->stage2_numvals &&				
	    (cost_data.c.total_cost == 0.0 ||					
	     cost_data.c.stage2_numvals < ecmdata->stage2_numvals * 2))		
		return (1.0);							

	if (ecmdata->state >= ECM_STATE_STAGE2) {
		free (ecmdata->pairmap);
		ecmdata->pairmap = NULL;
		ecmdata->pairmap_size = 0;
	}

	if (cost_data.c.total_cost == 0.0) return (0.0);

	ecmdata->stage2_numvals = cost_data.c.stage2_numvals;
	ecmdata->numrels = cost_data.c.numrels;
	ecmdata->pool_type = cost_data.pool_type;
	ecmdata->D = cost_data.c.D;
	ecmdata->B2_start = cost_data.c.B2_start;
	ecmdata->numDsections = cost_data.c.numDsections;
	ecmdata->Dsection = 0;									
	ecmdata->stage2_type = cost_data.stage2_type;
	if (ecmdata->stage2_type == ECM_STAGE2_PAIRING) {
		ecmdata->TWO_FFT_STAGE2 = cost_data.TWO_FFT_STAGE2;
		ecmdata->E = cost_data.E;
		ecmdata->totrels = (int) cost_data.c.totrels;
		memcpy (ecmdata->relp_sets, cost_data.c.relp_sets, sizeof (ecmdata->relp_sets));
		ecmdata->max_pairmap_Dsections = cost_data.c.max_pairmap_Dsections;
		if (ecmdata->state < ECM_STATE_STAGE2 || ecmdata->last_relocatable > ecmdata->B2_start) ecmdata->last_relocatable = ecmdata->B2_start;
	} else {
		ecmdata->poly_size = cost_data.c.numrels;
		ecmdata->Ftree_polys_in_mem = cost_data.Ftree_polys_in_mem;
	}

	if (IniGetInt (INI_FILE, "Stage2Estimates", 0)) {
		if (ecmdata->stage2_type == ECM_STAGE2_PAIRING)
			sprintf (msgbuf + strlen (msgbuf), "Est. pair%%: %5.2f, init transforms: %.0f, main loop transforms: %.0f\n",
				 cost_data.c.est_pair_pct * 100.0, cost_data.c.est_init_transforms, cost_data.c.est_stage2_transforms);
		else
			sprintf (msgbuf + strlen (msgbuf), "Est. init transforms: %.0f, main loop transforms: %.0f, init poly cost: %.0f, main loop poly cost: %.0f\n",
				 cost_data.c.est_init_transforms, cost_data.c.est_stage2_transforms, cost_data.c.est_init_polymult, cost_data.c.est_stage2_polymult);
	}
	
	sprintf (msgbuf + strlen (msgbuf), "Estimated stage 2 vs. stage 1 runtime ratio: %.3f\n", cost_data.c.est_stage2_stage1_ratio);
	ecmdata->est_stage2_stage1_ratio = cost_data.c.est_stage2_stage1_ratio;

	return (cost_data.c.efficiency);
}

#define ECM_MAGICNUM	0x1725bcd9

#define ECM_VERSION	6			

void ecm_save (
	ecmhandle *ecmdata)
{
	int	fd;
	struct work_unit *w = ecmdata->w;
	uint32_t sum = 0;

	fd = openWriteSaveFile (&ecmdata->write_save_file_state);
	if (fd < 0) return;

	if (! write_header (fd, ECM_MAGICNUM, ECM_VERSION, w)) goto writeerr;

	if (! write_uint32 (fd, ecmdata->curve, &sum)) goto writeerr;
	if (! write_uint64 (fd, ecmdata->average_B2, NULL)) goto writeerr;
	if (! write_uint32 (fd, ecmdata->state, &sum)) goto writeerr;
	if (! write_uint64 (fd, ecmdata->sigma, NULL)) goto writeerr;
	if (! write_uint64 (fd, ecmdata->B, &sum)) goto writeerr;
	if (! write_uint64 (fd, ecmdata->C, &sum)) goto writeerr;
	if (! write_uint32 (fd, ecmdata->sigma_type, &sum)) goto writeerr;
	if (! write_uint32 (fd, ecmdata->montg_stage1, &sum)) goto writeerr;

	if (ecmdata->state == ECM_STATE_STAGE1) {
		if (ecmdata->montg_stage1) {
			if (! write_uint64 (fd, ecmdata->stage1_prime, &sum)) goto writeerr;
			if (! write_gwnum (fd, &ecmdata->gwdata, ecmdata->xz.x, &sum)) goto writeerr;
			if (! write_gwnum (fd, &ecmdata->gwdata, ecmdata->xz.z, &sum)) goto writeerr;
		} else {
			ASSERTG (!ecmdata->e.alternate_format);
			if (! write_uint64 (fd, ecmdata->stage1_start_prime, &sum)) goto writeerr;
			if (! write_uint32 (fd, ecmdata->stage1_exp_buffer_size, &sum)) goto writeerr;
			if (! write_uint32 (fd, ecmdata->stage1_bitnum, &sum)) goto writeerr;
			if (! write_uint32 (fd, ecmdata->NAF_dictionary_size, &sum)) goto writeerr;
			if (! write_gwnum (fd, &ecmdata->gwdata, ecmdata->NAF_dictionary[0].x, &sum)) goto writeerr;
			if (! write_gwnum (fd, &ecmdata->gwdata, ecmdata->NAF_dictionary[0].y, &sum)) goto writeerr;
			if (! write_gwnum (fd, &ecmdata->gwdata, ecmdata->e.x, &sum)) goto writeerr;
			if (! write_gwnum (fd, &ecmdata->gwdata, ecmdata->e.y, &sum)) goto writeerr;
			if (! write_gwnum (fd, &ecmdata->gwdata, ecmdata->e.z, &sum)) goto writeerr;
		}
	}

	else if (ecmdata->state == ECM_STATE_MIDSTAGE) {
		int32_t tmp;
		if (! write_giant (fd, ecmdata->Qx_binary, &sum)) goto writeerr;
		tmp = (ecmdata->Qz_binary != NULL);
		if (! write_int32 (fd, tmp, &sum)) goto writeerr;
		if (tmp && ! write_giant (fd, ecmdata->Qz_binary, &sum)) goto writeerr;
		tmp = (ecmdata->gg_binary != NULL);
		if (! write_int32 (fd, tmp, &sum)) goto writeerr;
		if (tmp && ! write_giant (fd, ecmdata->gg_binary, &sum)) goto writeerr;
	}

	else if (ecmdata->state == ECM_STATE_STAGE2) {
		if (! write_int32 (fd, ecmdata->stage2_type, &sum)) goto writeerr;
		if (ecmdata->stage2_type != ECM_STAGE2_POLYMULT) {	
			uint64_t remaining_pairmap_size;
			if (! write_uint64 (fd, ecmdata->stage2_numvals, &sum)) goto writeerr;
			if (! write_uint32 (fd, ecmdata->totrels, &sum)) goto writeerr;
			if (! write_uint32 (fd, ecmdata->D, &sum)) goto writeerr;
			if (! write_uint32 (fd, ecmdata->E, &sum)) goto writeerr;
			if (! write_int32 (fd, ecmdata->TWO_FFT_STAGE2, &sum)) goto writeerr;
			if (! write_int32 (fd, ecmdata->pool_type, &sum)) goto writeerr;
			if (! write_uint64 (fd, ecmdata->first_relocatable, &sum)) goto writeerr;
			if (! write_uint64 (fd, ecmdata->last_relocatable, &sum)) goto writeerr;
			if (! write_uint64 (fd, ecmdata->B2_start, &sum)) goto writeerr;
			if (! write_uint64 (fd, ecmdata->C_done, &sum)) goto writeerr;
			if (! write_uint64 (fd, ecmdata->numDsections, &sum)) goto writeerr;
			if (! write_uint64 (fd, ecmdata->Dsection, &sum)) goto writeerr;
			if (! write_uint64 (fd, ecmdata->max_pairmap_Dsections, &sum)) goto writeerr;
			if (! write_int32 (fd, ecmdata->relp, &sum)) goto writeerr;
			if (! write_array (fd, (char *) ecmdata->relp_sets, 32 * sizeof (int16_t), &sum)) goto writeerr;
			
			remaining_pairmap_size = ecmdata->pairmap_size - (ecmdata->pairmap_ptr - ecmdata->pairmap);
			if (! write_uint64 (fd, remaining_pairmap_size, &sum)) goto writeerr;
			if (! write_array (fd, (char *) ecmdata->pairmap_ptr, (size_t) remaining_pairmap_size, &sum)) goto writeerr;
			
			if (! write_giant (fd, ecmdata->Qx_binary, &sum)) goto writeerr;
			if (! write_gwnum (fd, &ecmdata->gwdata, ecmdata->gg, &sum)) goto writeerr;
		} else {						
			
			int32_t tmp;
			if (! write_int32 (fd, ecmdata->required_missing, &sum)) goto writeerr;
			if (! write_uint64 (fd, ecmdata->C_done, &sum)) goto writeerr;

			if (! write_giant (fd, ecmdata->Qx_binary, &sum)) goto writeerr;
			ASSERTG (ecmdata->gg_binary == NULL || ecmdata->gg == NULL);
			tmp = (ecmdata->gg_binary != NULL || ecmdata->gg != NULL);
			if (! write_int32 (fd, tmp, &sum)) goto writeerr;
			if (ecmdata->gg_binary != NULL && ! write_giant (fd, ecmdata->gg_binary, &sum)) goto writeerr;
			if (ecmdata->gg != NULL && ! write_gwnum (fd, &ecmdata->gwdata, ecmdata->gg, &sum)) goto writeerr;
		}
	}

	else if (ecmdata->state == ECM_STATE_GCD) {
		if (! write_gwnum (fd, &ecmdata->gwdata, ecmdata->gg, &sum)) goto writeerr;
	}

	if (! write_checksum (fd, sum)) goto writeerr;

	closeWriteSaveFile (&ecmdata->write_save_file_state, fd);
	return;

writeerr:
	deleteWriteSaveFile (&ecmdata->write_save_file_state, fd);
}

int ecm_old_restore (			
	ecmhandle *ecmdata,
	int	fd,
	uint32_t filesum)
{
	double	sigma;
	uint32_t state, sum = 0;
	uint64_t savefile_B, unused64;

	if (! read_uint32 (fd, &state, &sum)) goto readerr;
	if (! read_uint32 (fd, &ecmdata->curve, &sum)) goto readerr;
	if (! read_double (fd, &sigma, NULL)) goto readerr;
	ecmdata->sigma = (uint64_t) sigma;
	if (! read_uint64 (fd, &savefile_B, &sum)) goto readerr;
	if (! read_uint64 (fd, &ecmdata->stage1_prime, &sum)) goto readerr;
	if (! read_uint64 (fd, &unused64, &sum)) goto readerr;	

	if (savefile_B < ecmdata->B) {
		OutputStr (ecmdata->thread_num, "ECM save file created with smaller B1 value.  Save file cannot be used.\n");
		goto readerr;
	}

	if (state == 0) {
		ecmdata->state = ECM_STATE_STAGE1;
		if (! alloc_xz (ecmdata, &ecmdata->xz)) goto readerr;
		if (! read_gwnum (fd, &ecmdata->gwdata, ecmdata->xz.x, &sum)) goto readerr;
		if (! read_gwnum (fd, &ecmdata->gwdata, ecmdata->xz.z, &sum)) goto readerr;
	}
	if (state == 1) {
		ecmdata->state = ECM_STATE_MIDSTAGE;
		ecmdata->Qx_binary = allocgiant (((int) ecmdata->gwdata.bit_length >> 5) + 10);
		if (ecmdata->Qx_binary == NULL) goto readerr;
		if (! read_giant (fd, ecmdata->Qx_binary, &sum)) goto readerr;
		ecmdata->gg_binary = allocgiant (((int) ecmdata->gwdata.bit_length >> 5) + 10);
		if (ecmdata->gg_binary == NULL) goto readerr;
		if (! read_giant (fd, ecmdata->gg_binary, &sum)) goto readerr;
		OutputStr (ecmdata->thread_num, "Old ECM save file was in stage 2.  Restarting stage 2 from scratch.\n");
		free (ecmdata->gg_binary), ecmdata->gg_binary = NULL;
	}

	ecmdata->average_B2 = ecmdata->C;

	if (filesum != sum) goto readerr;
	_close (fd);
	return (TRUE);

readerr:
	_close (fd);
	return (FALSE);
}

int ecm_restore (			
	ecmhandle *ecmdata)
{
	int	fd;
	struct work_unit *w = ecmdata->w;
	uint32_t version, sum = 0, filesum;
	uint64_t savefile_B;

	fd = _open (ecmdata->read_save_file_state.current_filename, _O_BINARY | _O_RDONLY);
	if (fd < 0) goto err;

	if (! read_magicnum (fd, ECM_MAGICNUM)) goto readerr;
	if (! read_header (fd, &version, w, &filesum)) goto readerr;
	if (version == 0 || version > ECM_VERSION) goto readerr;
	if (version == 1) return (ecm_old_restore (ecmdata, fd, filesum));

	if (! read_uint32 (fd, &ecmdata->curve, &sum)) goto readerr;
	if (! read_uint64 (fd, &ecmdata->average_B2, NULL)) goto readerr;
	if (! read_uint32 (fd, &ecmdata->state, &sum)) goto readerr;
	if (version < 5) {
		double	sigma;
		if (! read_double (fd, &sigma, NULL)) goto readerr;
		ecmdata->sigma = (uint64_t) sigma;
	} else {
		if (! read_uint64 (fd, &ecmdata->sigma, NULL)) goto readerr;
	}
	if (! read_uint64 (fd, &savefile_B, &sum)) goto readerr;
	if (! read_uint64 (fd, &ecmdata->C, &sum)) goto readerr;
	if (version < 6) {
		ecmdata->sigma_type = 1;
		ecmdata->montg_stage1 = TRUE;
	} else {
		if (! read_uint32 (fd, &ecmdata->sigma_type, &sum)) goto readerr;
		uint32_t tmp;
		if (! read_uint32 (fd, &tmp, &sum)) goto readerr;
		ecmdata->montg_stage1 = tmp;
	}
		
	if (savefile_B < ecmdata->B) {
		OutputStr (ecmdata->thread_num, "ECM save file created with smaller B1 value.  Save file cannot be used.\n");
		goto readerr;
	}

	if (ecmdata->B == 0) ecmdata->B = savefile_B;

	if (version == 2 && ecmdata->state == ECM_STATE_STAGE2) {
		OutputStr (ecmdata->thread_num, "Old ECM save file was in stage 2.  Save file cannot be used.\n");
		goto readerr;
	}

	if (ecmdata->state == ECM_STATE_STAGE1) {
		if (ecmdata->montg_stage1) {
			if (! read_uint64 (fd, &ecmdata->stage1_prime, &sum)) goto readerr;
			if (! alloc_xz (ecmdata, &ecmdata->xz)) goto readerr;
			if (! read_gwnum (fd, &ecmdata->gwdata, ecmdata->xz.x, &sum)) goto readerr;
			if (! read_gwnum (fd, &ecmdata->gwdata, ecmdata->xz.z, &sum)) goto readerr;
		} else {
			if (! read_uint64 (fd, &ecmdata->stage1_start_prime, &sum)) goto readerr;
			if (! read_uint32 (fd, &ecmdata->stage1_exp_buffer_size, &sum)) goto readerr;
			if (! read_uint32 (fd, &ecmdata->stage1_bitnum, &sum)) goto readerr;
			if (! read_uint32 (fd, &ecmdata->NAF_dictionary_size, &sum)) goto readerr;
			if (! ed_alloc (ecmdata, &ecmdata->dict_start)) goto readerr;
			if (! read_gwnum (fd, &ecmdata->gwdata, ecmdata->dict_start.x, &sum)) goto readerr;
			if (! read_gwnum (fd, &ecmdata->gwdata, ecmdata->dict_start.y, &sum)) goto readerr;
			gwfree (&ecmdata->gwdata, ecmdata->dict_start.z), ecmdata->dict_start.z = NULL;
			if (! ed_alloc (ecmdata, &ecmdata->e)) goto readerr;
			if (! read_gwnum (fd, &ecmdata->gwdata, ecmdata->e.x, &sum)) goto readerr;
			if (! read_gwnum (fd, &ecmdata->gwdata, ecmdata->e.y, &sum)) goto readerr;
			if (! read_gwnum (fd, &ecmdata->gwdata, ecmdata->e.z, &sum)) goto readerr;
		}
	}

	else if (ecmdata->state == ECM_STATE_MIDSTAGE) {
		int32_t	tmp;

		ecmdata->Qx_binary = allocgiant (((int) ecmdata->gwdata.bit_length >> 5) + 10);
		if (ecmdata->Qx_binary == NULL) goto readerr;
		if (! read_giant (fd, ecmdata->Qx_binary, &sum)) goto readerr;

		if (! read_int32 (fd, &tmp, &sum)) goto readerr;
		if (tmp) {
			ecmdata->Qz_binary = allocgiant (((int) ecmdata->gwdata.bit_length >> 5) + 10);
			if (ecmdata->Qz_binary == NULL) goto readerr;
			if (! read_giant (fd, ecmdata->Qz_binary, &sum)) goto readerr;
		}

		if (! read_int32 (fd, &tmp, &sum)) goto readerr;
		if (tmp) {
			ecmdata->gg_binary = allocgiant (((int) ecmdata->gwdata.bit_length >> 5) + 10);
			if (ecmdata->gg_binary == NULL) goto readerr;
			if (! read_giant (fd, ecmdata->gg_binary, &sum)) goto readerr;
		}
	}

	else if (ecmdata->state == ECM_STATE_STAGE2) {
		int32_t	tmp;
		if (! read_int32 (fd, &ecmdata->stage2_type, &sum)) goto readerr;
		if (ecmdata->stage2_type != ECM_STAGE2_POLYMULT) {	
			if (! read_uint64 (fd, &ecmdata->stage2_numvals, &sum)) goto readerr;
			if (! read_uint32 (fd, &ecmdata->totrels, &sum)) goto readerr;
			if (! read_uint32 (fd, &ecmdata->D, &sum)) goto readerr;
			ecmdata->numrels = map_D_to_numrels (ecmdata->D);
			if (! read_uint32 (fd, &ecmdata->E, &sum)) goto readerr;
			if (! read_int32 (fd, &ecmdata->TWO_FFT_STAGE2, &sum)) goto readerr;
			if (! read_int32 (fd, &ecmdata->pool_type, &sum)) goto readerr;
			if (! read_uint64 (fd, &ecmdata->first_relocatable, &sum)) goto readerr;
			if (! read_uint64 (fd, &ecmdata->last_relocatable, &sum)) goto readerr;
			if (! read_uint64 (fd, &ecmdata->B2_start, &sum)) goto readerr;
			if (! read_uint64 (fd, &ecmdata->C_done, &sum)) goto readerr;
			if (! read_uint64 (fd, &ecmdata->numDsections, &sum)) goto readerr;
			if (! read_uint64 (fd, &ecmdata->Dsection, &sum)) goto readerr;
			if (! read_uint64 (fd, &ecmdata->max_pairmap_Dsections, &sum)) goto readerr;
			if (! read_int32 (fd, &ecmdata->relp, &sum)) goto readerr;
			if (! read_array (fd, (char *) ecmdata->relp_sets, 32 * sizeof (int16_t), &sum)) goto readerr;
			if (! read_uint64 (fd, &ecmdata->pairmap_size, &sum)) goto readerr;
			ecmdata->pairmap = (uint8_t *) malloc ((size_t) ecmdata->pairmap_size);
			if (ecmdata->pairmap == NULL) goto readerr;
			if (! read_array (fd, (char *) ecmdata->pairmap, (size_t) ecmdata->pairmap_size, &sum)) goto readerr;
			ecmdata->pairmap_ptr = ecmdata->pairmap;
			ecmdata->Qx_binary = allocgiant (((int) ecmdata->gwdata.bit_length >> 5) + 10);
			if (ecmdata->Qx_binary == NULL) goto readerr;
			if (! read_giant (fd, ecmdata->Qx_binary, &sum)) goto readerr;
			ecmdata->gg_binary = allocgiant (((int) ecmdata->gwdata.bit_length >> 5) + 10);
			if (ecmdata->gg_binary == NULL) goto readerr;
			if (! read_giant (fd, ecmdata->gg_binary, &sum)) goto readerr;
		} else {						
			if (! read_int32 (fd, &ecmdata->required_missing, &sum)) goto readerr;
			if (! read_uint64 (fd, &ecmdata->C_done, &sum)) goto readerr;
			ecmdata->Qx_binary = allocgiant (((int) ecmdata->gwdata.bit_length >> 5) + 10);
			if (ecmdata->Qx_binary == NULL) goto readerr;
			if (! read_giant (fd, ecmdata->Qx_binary, &sum)) goto readerr;
			if (! read_int32 (fd, &tmp, &sum)) goto readerr;
			if (tmp) {
				ecmdata->gg_binary = allocgiant (((int) ecmdata->gwdata.bit_length >> 5) + 10);
				if (ecmdata->gg_binary == NULL) goto readerr;
				if (! read_giant (fd, ecmdata->gg_binary, &sum)) goto readerr;
			}
		}
	}

	else if (ecmdata->state == ECM_STATE_GCD) {
		ecmdata->gg = gwalloc (&ecmdata->gwdata);
		if (ecmdata->gg == NULL) goto readerr;
		if (! read_gwnum (fd, &ecmdata->gwdata, ecmdata->gg, &sum)) goto readerr;
	}

	if (filesum != sum) goto readerr;
	_close (fd);
	return (TRUE);

readerr:
	free_xz (ecmdata, &ecmdata->xz);
	free (ecmdata->Qx_binary), ecmdata->Qx_binary = NULL;
	free (ecmdata->Qz_binary), ecmdata->Qz_binary = NULL;
	free (ecmdata->gg_binary), ecmdata->gg_binary = NULL;
	_close (fd);
err:
	return (FALSE);
}

#define ECM_POLYF_LEVEL_ZERO	1
#define ECM_POLYG_LEVEL_ZERO	2
#define ECM_BUILD_GCDVAL	3

void ecm_helper (
	int	helper_num,	
	gwhandle *gwdata,	
	void	*info)
{
	ecmhandle *ecmdata = (ecmhandle *) info;

	if (ecmdata->helper_work == ECM_POLYF_LEVEL_ZERO) {
		pmhandle pmdata;
		void	*plan2 = NULL;
		void	*plan3 = NULL;
		void	*plan4 = NULL;

		polymult_init (&pmdata, gwdata);

		gwarray source_poly = ecmdata->poly1;
		gwarray dest_poly = ecmdata->polyF;
		int	log2_num_polys = (int) ceil (log2 ((double) ecmdata->poly_size));	

		for ( ; ; ) {
			int	counter, poly_base, next_poly_base, num_polys;

			counter = (int) atomic_fetch_incr (ecmdata->polydata.helper_counter);
			poly_base = (int) ((counter * 4 * ecmdata->poly_size) >> log2_num_polys);
			if (poly_base >= ecmdata->poly_size) break;
			next_poly_base = (int) (((counter + 1) * 4 * ecmdata->poly_size) >> log2_num_polys);
			num_polys = next_poly_base - poly_base;

			ASSERTG (num_polys >= 2 && num_polys <= 4);
			if (num_polys == 2) {
				int options = POLYMULT_INVEC1_MONIC_NEGATE | POLYMULT_INVEC2_MONIC_NEGATE | POLYMULT_NEXTFFT | POLYMULT_SAVE_PLAN;
				if (plan2 != NULL) pmdata.plan = plan2, options |= POLYMULT_USE_PLAN;
				polymult (&pmdata, &source_poly[poly_base], 1, &source_poly[poly_base+1], 1, &dest_poly[poly_base], 2, options);
				plan2 = pmdata.plan;
			}
			else if (num_polys == 3) {
				int options = POLYMULT_INVEC1_MONIC_NEGATE | POLYMULT_INVEC2_MONIC_NEGATE | POLYMULT_NEXTFFT | POLYMULT_SAVE_PLAN;
				if (plan2 != NULL) pmdata.plan = plan2, options |= POLYMULT_USE_PLAN;
				polymult (&pmdata, &source_poly[poly_base], 1, &source_poly[poly_base+1], 1, &dest_poly[poly_base], 2, options);
				plan2 = pmdata.plan;
				options = POLYMULT_INVEC1_MONIC | POLYMULT_INVEC2_MONIC_NEGATE | POLYMULT_NEXTFFT | POLYMULT_SAVE_PLAN;
				if (plan3 != NULL) pmdata.plan = plan3, options |= POLYMULT_USE_PLAN;
				polymult (&pmdata, &dest_poly[poly_base], 2, &source_poly[poly_base+2], 1, &dest_poly[poly_base], 3, options);
				plan3 = pmdata.plan;
			} else {
				int options = POLYMULT_INVEC1_MONIC_NEGATE | POLYMULT_INVEC2_MONIC_NEGATE | POLYMULT_NEXTFFT | POLYMULT_SAVE_PLAN;
				if (plan2 != NULL) pmdata.plan = plan2, options |= POLYMULT_USE_PLAN;
				polymult (&pmdata, &source_poly[poly_base], 1, &source_poly[poly_base+1], 1, &dest_poly[poly_base], 2, options);
				options |= POLYMULT_USE_PLAN;
				polymult (&pmdata, &source_poly[poly_base+2], 1, &source_poly[poly_base+3], 1, &dest_poly[poly_base+2], 2, options);
				plan2 = pmdata.plan;
				options = POLYMULT_INVEC1_MONIC | POLYMULT_INVEC2_MONIC | POLYMULT_NEXTFFT | POLYMULT_SAVE_PLAN;
				if (plan4 != NULL) pmdata.plan = plan4, options |= POLYMULT_USE_PLAN;
				polymult (&pmdata, &dest_poly[poly_base], 2, &dest_poly[poly_base+2], 2, &dest_poly[poly_base], 4, options);
				plan4 = pmdata.plan;
			}
		}

		free (plan2);
		free (plan3);
		free (plan4);
	}

	if (ecmdata->helper_work == ECM_POLYG_LEVEL_ZERO) {
		pmhandle pmdata;
		void	*plan2 = NULL;
		void	*plan3 = NULL;
		void	*plan4 = NULL;

		polymult_init (&pmdata, gwdata);

		gwarray polyG = ecmdata->polyGH;
		uint64_t num_in_polyG = ecmdata->poly_size - ecmdata->polyGaux_size;		
		int	log2_num_polys = (int) ceil (log2 ((double) ecmdata->poly_size));	

		for ( ; ; ) {
			int	counter, poly_base, next_poly_base, num_polys;

			counter = (int) atomic_fetch_incr (ecmdata->polydata.helper_counter);
			poly_base = (int) ((counter * 4 * ecmdata->poly_size) >> log2_num_polys);
			if (poly_base >= ecmdata->poly_size) break;
			next_poly_base = (int) (((counter + 1) * 4 * ecmdata->poly_size) >> log2_num_polys);
			num_polys = next_poly_base - poly_base;

			gwnum	poly1, poly2, poly3, poly4;
			poly1 = polyG[poly_base];
			if (poly_base >= num_in_polyG) gwswap (polyG[poly_base], ecmdata->polyGaux[poly_base - num_in_polyG]);
			poly2 = polyG[poly_base+1];
			if (poly_base+1 >= num_in_polyG) gwswap (polyG[poly_base+1], ecmdata->polyGaux[poly_base+1 - num_in_polyG]);
			if (num_polys > 2) {
				poly3 = polyG[poly_base+2];
				if (poly_base+2 >= num_in_polyG) gwswap (polyG[poly_base+2], ecmdata->polyGaux[poly_base+2 - num_in_polyG]);
			}
			if (num_polys > 3) {
				poly4 = polyG[poly_base+3];
				if (poly_base+3 >= num_in_polyG) gwswap (polyG[poly_base+3], ecmdata->polyGaux[poly_base+3 - num_in_polyG]);
			}

			ASSERTG (num_polys >= 2 && num_polys <= 4);
			if (num_polys == 2) {
				int options = POLYMULT_INVEC1_MONIC_NEGATE | POLYMULT_INVEC2_MONIC_NEGATE | POLYMULT_NEXTFFT | POLYMULT_SAVE_PLAN;
				if (plan2 != NULL) pmdata.plan = plan2, options |= POLYMULT_USE_PLAN;
				polymult (&pmdata, &poly1, 1, &poly2, 1, &polyG[poly_base], 2, options);
				plan2 = pmdata.plan;
			}
			else if (num_polys == 3) {
				int options = POLYMULT_INVEC1_MONIC_NEGATE | POLYMULT_INVEC2_MONIC_NEGATE | POLYMULT_NEXTFFT | POLYMULT_SAVE_PLAN;
				if (plan2 != NULL) pmdata.plan = plan2, options |= POLYMULT_USE_PLAN;
				polymult (&pmdata, &poly1, 1, &poly2, 1, &polyG[poly_base], 2, options);
				plan2 = pmdata.plan;
				options = POLYMULT_INVEC1_MONIC | POLYMULT_INVEC2_MONIC_NEGATE | POLYMULT_NEXTFFT | POLYMULT_SAVE_PLAN;
				if (plan3 != NULL) pmdata.plan = plan3, options |= POLYMULT_USE_PLAN;
				polymult (&pmdata, &polyG[poly_base], 2, &poly3, 1, &polyG[poly_base], 3, options);
				plan3 = pmdata.plan;
			} else {
				int options = POLYMULT_INVEC1_MONIC_NEGATE | POLYMULT_INVEC2_MONIC_NEGATE | POLYMULT_NEXTFFT | POLYMULT_SAVE_PLAN;
				if (plan2 != NULL) pmdata.plan = plan2, options |= POLYMULT_USE_PLAN;
				polymult (&pmdata, &poly1, 1, &poly2, 1, &polyG[poly_base], 2, options);
				options |= POLYMULT_USE_PLAN;
				polymult (&pmdata, &poly3, 1, &poly4, 1, &polyG[poly_base+2], 2, options);
				plan2 = pmdata.plan;
				options = POLYMULT_INVEC1_MONIC | POLYMULT_INVEC2_MONIC | POLYMULT_NEXTFFT | POLYMULT_SAVE_PLAN;
				if (plan4 != NULL) pmdata.plan = plan4, options |= POLYMULT_USE_PLAN;
				polymult (&pmdata, &polyG[poly_base], 2, &polyG[poly_base+2], 2, &polyG[poly_base], 4, options);
				plan4 = pmdata.plan;
			}
		}

		free (plan2);
		free (plan3);
		free (plan4);
	}

	if (ecmdata->helper_work == ECM_BUILD_GCDVAL) {
		gwnum	accumulator = NULL;
		gwnum	*polyH = &ecmdata->polyGH[ecmdata->poly_size];

		for ( ; ; ) {
			
			int j = (int) atomic_fetch_incr (ecmdata->polydata.helper_counter);

			if (j >= ecmdata->poly_size) break;

			if (polymult_must_unfft (gwdata, polyH[j])) gwunfft2 (gwdata, polyH[j], polyH[j], GWMUL_STARTNEXTFFT);

			if (accumulator == NULL) accumulator = polyH[j];
			else gwmul3 (gwdata, polyH[j], accumulator, accumulator, GWMUL_STARTNEXTFFT);
		}

		if (accumulator != NULL) gwfft (gwdata, accumulator, accumulator);
		ecmdata->polyGH[helper_num] = accumulator;
	}
}

int ecm (
	int	thread_num,
	struct PriorityInfo *sp_info,	
	struct work_unit *w)
{
	ecmhandle ecmdata;
	uint64_t next_prime, last_output, last_output_t, dictionary_memory;
	double	one_over_B, output_frequency, output_title_frequency;
	double	base_pct_complete, one_relp_pct;
	int	i;
	unsigned int memused;
	char	filename[32], ftree_filename[40], buf[255], JSONbuf[4000], fft_desc[200];
	int	res, stop_reason, delayed_stop_reason, stage, first_iter_msg;
	int	msglen, continueECM, prpAfterEcmFactor;
	char	*str, *msg;
	double	stage1_timer, stage2_timer, timers[10];
	bool	near_fft_limit, saving, default_sigma_type;
	double	allowable_maxerr;
	int	maxerr_restart_count = 0;
	unsigned long maxerr_fftlen = 0;
	relp_set_data_map relp_set_map;
	Dmultiple_data_map Dmultiple_map;

	str = NULL;
	msg = NULL;

	memset (&ecmdata, 0, sizeof (ecmhandle));
	ecmdata.thread_num = thread_num;
	ecmdata.stage1_threads = get_worker_num_threads (thread_num, HYPERTHREAD_LL);
	ecmdata.stage2_threads = ecmdata.stage1_threads + IniGetInt (INI_FILE, "Stage2ExtraThreads", 0);
	ecmdata.w = w;
	ecmdata.B = (uint64_t) w->B1;
	ecmdata.C = (uint64_t) w->B2;
	if (ecmdata.B < 120 && w->gmp_ecm_file == NULL) {
		OutputStr (thread_num, "Using minimum bound #1 of 120\n");
		ecmdata.B = 120;
	}
	if (ecmdata.C == 0) ecmdata.C = ecmdata.B * 100;
	if (ecmdata.C <= ecmdata.B) ecmdata.C = ecmdata.B;
	if (IniGetInt (INI_FILE, "GmpEcmHook", 0)) ecmdata.C = ecmdata.B;
	ecmdata.pct_mem_to_use = 1.0;				

	ecmdata.optimal_B2 = (!QA_IN_PROGRESS && ecmdata.C == 100 * ecmdata.B && IniGetInt (INI_FILE, "ECMBestB2", 1));

	if (!QA_IN_PROGRESS && !IniGetInt (INI_FILE, "GmpEcmHook", 0)) {
		int	mult = IniGetInt (INI_FILE, "ECMBoundsMultiplier", 1);
		if (mult < 1) mult = 1;
		if (mult > 20) mult = 20;
		ecmdata.B *= mult;
		ecmdata.C *= mult;
	}

restart:
	if (w->gmp_ecm_file != NULL) {
		ecmdata.gmp_ecm_file = fopen (w->gmp_ecm_file, "r");
		if (ecmdata.gmp_ecm_file == NULL) {
			sprintf (buf, "File named '%s' could not be opened.\n", w->gmp_ecm_file);
			OutputStr (thread_num, buf);
			stop_reason = STOP_FILE_IO_ERROR;
			goto exit;
		}
		
		for (i = 0; i < (int) w->skip_curves; i++) skip_resumefile_line (ecmdata.gmp_ecm_file);
	}

	if (w->n) {
		stop_reason = setN (thread_num, w, &ecmdata.N);
		if (stop_reason) goto exit;
	} else {
		mpz_t n;
		mpz_init (n);
		if (!peek_resumefile_line (ecmdata.gmp_ecm_file, n)) {
			sprintf (buf, "Could not find line containing N= in file '%s'.\n", w->gmp_ecm_file);
			OutputStr (thread_num, buf);
			stop_reason = STOP_FILE_IO_ERROR;
			goto exit;
		}
		ecmdata.N = allocgiant ((int) divide_rounding_up (mpz_sizeinbase (n, 2), 32));
		if (ecmdata.N == NULL) goto oom;
		mpztog (n, ecmdata.N);
		mpz_clear (n);
	}

	PRAC_SEARCH = IniGetInt (INI_FILE, "PracSearch", 7);
	if (PRAC_SEARCH < 1) PRAC_SEARCH = 1;
	if (PRAC_SEARCH > 50) PRAC_SEARCH = 50;

#ifdef TIMING1
clear_timers (timers, sizeof (timers) / sizeof (timers[0]));
if (w->n == 598) {
int i, j;
giant	x, y, z, a, m;
#define TESTSIZE	200
RDTSC_TIMING = 12;
x = allocgiant(TESTSIZE); y = allocgiant (2*TESTSIZE);
z = allocgiant (2*TESTSIZE), a = allocgiant (2*TESTSIZE);
m = allocgiant (TESTSIZE);
srand ((unsigned) time (NULL));
for (i = 0; i < TESTSIZE; i++) {
	x->n[i] = (rand () << 17) + rand ();
	m->n[i] = (rand () << 17) + rand ();
}
x->n[TESTSIZE-1] &= 0x00FFFFFF;
m->n[TESTSIZE-1] &= 0x00FFFFFF;
for (i = TESTSIZE; i >= 10; i--) {
	x->sign = i;
	m->sign = i;
	setmulmode (GRAMMAR_MUL);
	for (j = 0; j < 10; j++) {
		gtog (x, y);
		start_timer (timers, 0);
		if (B&1) mulg (m, y);
		else squareg (y);
		end_timer (timers, 0);
		if (timers[1] == 0 || timers[1] > timers[0]) timers[1] = timers[0];
		timers[0] = 0;
	}
	setmulmode (KARAT_MUL);
	for (j = 0; j < 10; j++) {
		gtog (x, z);
		start_timer (timers, 0);
		if (B&1) mulg (m, z);
		else squareg (z);
		end_timer (timers, 0);
		if (timers[2] == 0 || timers[2] > timers[0]) timers[2] = timers[0];
		timers[0] = 0;
	}
	setmulmode (FFT_MUL);
	for (j = 0; j < 10; j++) {
		gtog (x, a);
		start_timer (timers, 0);
		if (B&1) mulg (m, a);
		else squareg (a);
		end_timer (timers, 0);
		if (timers[3] == 0 || timers[3] > timers[0]) timers[3] = timers[0];
		timers[0] = 0;
	}
	sprintf (buf, "Size: %ld  G: ", i);
	print_timer (timers, 1, buf, TIMER_MS | TIMER_CLR);
	strcat (buf, ", K: ");
	print_timer (timers, 2, buf, TIMER_MS | TIMER_CLR);
	strcat (buf, ", F: ");
	print_timer (timers, 3, buf, TIMER_MS | TIMER_NL | TIMER_CLR);
	OutputBoth (thread_num, buf);
	if (gcompg (y, z) != 0)
		i--;
	if (gcompg (y, a) != 0)
		i--;
	Sleep (100);
}
return 0;
}

if (w->n == 601) {
clear_timers (timers, sizeof (timers) / sizeof (timers[0]));
int i, j;
giant	x, a, m;
#define TESTSIZE2	260000
RDTSC_TIMING = 12;
x = allocgiant(TESTSIZE2);
a = allocgiant (2*TESTSIZE2);
m = allocgiant (TESTSIZE2);
srand ((unsigned) time (NULL));
for (i = 0; i < TESTSIZE2; i++) {
	x->n[i] = (rand () << 17) + rand ();
	m->n[i] = (rand () << 17) + rand ();
}
x->n[TESTSIZE2-1] &= 0x00FFFFFF;
m->n[TESTSIZE2-1] &= 0x00FFFFFF;
for (i = 30; i < TESTSIZE2/2; i<<=1) {
	x->sign = i;
	m->sign = i;
	setmulmode (FFT_MUL);
	for (j = 0; j < 10; j++) {
		gtog (x, a);
		start_timer (timers, 0);
		if (B&1) mulg (m, a);
		else squareg (a);
		end_timer (timers, 0);
		if (timers[3] == 0 || timers[3] > timers[0]) timers[3] = timers[0];
		timers[0] = 0;
	}
	sprintf (buf, "Size: %ld  , F: ", i);
	print_timer (timers, 3, buf, TIMER_NL | TIMER_CLR | TIMER_MS);
	OutputStr (thread_num, buf);
	Sleep (100);
}
return 0;
}
#endif

#ifdef GDEBUG
if (w->n == 600) {
gwhandle gwdata;
void *workbuf;
int j, min_test, max_test, test, cnt, NUM_X87_TESTS, NUM_SSE2_TESTS, NUM_AVX_TESTS, NUM_AVX512_TESTS;
#define timeit(a,n,w) (((void**)a)[0]=w,((uint32_t*)a)[2]=n,gwtimeit(a))

clear_timers (timers, sizeof (timers) / sizeof (timers[0]));
gwinit (&gwdata); gwdata.cpu_flags &= ~CPU_AVX512F;
gwsetup (&gwdata, 1.0, 2, 10000000, -1);
workbuf = (void *) aligned_malloc (400000000, 4096);
memset (workbuf, 0, 400000000);
RDTSC_TIMING = 2;
min_test = IniGetInt (INI_FILE, "MinTest", 0);
max_test = IniGetInt (INI_FILE, "MaxTest", min_test);
NUM_X87_TESTS = timeit (gwdata.asm_data, -1, NULL);
NUM_SSE2_TESTS = timeit (gwdata.asm_data, -2, NULL);
NUM_AVX_TESTS = timeit (gwdata.asm_data, -3, NULL);
NUM_AVX512_TESTS = timeit (gwdata.asm_data, -4, NULL);

for (j = 0; j < NUM_X87_TESTS + NUM_SSE2_TESTS + NUM_AVX_TESTS + NUM_AVX512_TESTS; j++) {
	cnt = 0;
	test = (j < NUM_X87_TESTS ? j :
		j < NUM_X87_TESTS + NUM_SSE2_TESTS ? 1000 + j - NUM_X87_TESTS :
		j < NUM_X87_TESTS + NUM_SSE2_TESTS + NUM_AVX_TESTS ? 2000 + j - NUM_X87_TESTS - NUM_SSE2_TESTS :
			3000 + j - NUM_X87_TESTS - NUM_SSE2_TESTS - NUM_AVX_TESTS);
	if (test == 3000 && CPU_FLAGS & CPU_AVX512F) {
		gwdone (&gwdata);
		gwinit (&gwdata);
		gwsetup (&gwdata, 1.0, 2, 10000000, -1);
	}
	if (min_test && (test < min_test || test > max_test)) continue;
	if (! (CPU_FLAGS & CPU_SSE2) && test >= 1000) break;
	if (! (CPU_FLAGS & CPU_AVX) && test >= 2000) break;
	if (! (CPU_FLAGS & CPU_AVX512F) && test >= 3000) break;
for (i = 1; i <= 50; i++) {
	start_timer (timers, 0);
	timeit (gwdata.asm_data, test, workbuf);
	end_timer (timers, 0);
	if (timers[1] == 0 || timers[1] > timers[0]) timers[1] = timers[0];
	if (i > 1 && timers[0] < 3.0 * timers[1]) {
		if (timers[0] > 1.5 * timers[1])
			i++;
		timers[2] += timers[0];
		cnt++;
	}
	timers[0] = 0;
}
sprintf (buf, "Test %d: ", test);
print_timer (timers, 1, buf, TIMER_CLR);
timers[2] /= cnt;
strcat (buf, ", avg: ");
print_timer (timers, 2, buf, TIMER_NL | TIMER_CLR);
OutputBoth (thread_num, buf);
}
aligned_free (workbuf);
gwdone (&gwdata);
if (min_test) exit (0);
return 0;
}
#endif

#ifdef TIMING604
if (w->n == 604) {
	gmp_randstate_t rstate;
	int	exp;

	clear_timers (timers, sizeof (timers) / sizeof (timers[0]));
	gmp_randinit_default (rstate);
	for (exp = 100000; exp < 300000000; exp *= 2) {
		gwhandle gwdata;
		giant	N;
		gwnum	a, b, c;
		int	j;
		mpz_t	r;

		gwinit (&gwdata);
		gwsetup (&gwdata, 1.0, 2, exp, -1);

		a = gwalloc (&gwdata);
		b = gwalloc (&gwdata);
		c = gwalloc (&gwdata);
		N = popg (&gwdata.gdata, ((int) gwdata.bit_length >> 5) + 10);
		mpz_init (r);
		mpz_urandomb (r, rstate, exp);
		mpztog (r, N);
		gianttogw (&gwdata, N, a);
		mpz_urandomb (r, rstate, exp);
		mpztog (r, N);
		gianttogw (&gwdata, N, b);
		mpz_urandomb (r, rstate, exp);
		mpztog (r, N);
		gianttogw (&gwdata, N, c);
		mpz_clear (r);

		start_timer (timers, 0);
		for (j = 0; j < 100; j++) {
			gwsquare2 (&gwdata, c, c, GWMUL_STARTNEXTFFT);
		}
		end_timer (timers, 0);
		sprintf (buf, "Exp: %d, 100 squares: ", exp);
		print_timer (timers, 0, buf, TIMER_NL | TIMER_CLR);
		OutputBoth (thread_num, buf);

		start_timer (timers, 0);
		for (j = 0; j < 1; j++) {
			mpz_t	__v, __N, __gcd;
			giant	v;
			v = popg (&gwdata.gdata, ((int) gwdata.bit_length >> 5) + 10);
			gwtogiant (&gwdata, b, v);
			
			mpz_init (__v);
			mpz_init (__N);
			mpz_init (__gcd);
			gtompz (v, __v);
			gtompz (N, __N);
			mpz_gcd (__gcd, __v, __N);
			mpz_clear (__gcd);
			mpz_clear (__v);
			mpz_clear (__N);
			pushg (&gwdata.gdata, 1);
		}
		end_timer (timers, 0);
		sprintf (buf, "Exp: %d, GCD: ", exp);
		print_timer (timers, 0, buf, TIMER_NL | TIMER_CLR);
		OutputBoth (thread_num, buf);

		start_timer (timers, 0);
		for (j = 0; j < 1; j++) {
			giant	v;
			mpz_t	__v, __N, __gcd, __inv;
			v = popg (&gwdata.gdata, ((int) gwdata.bit_length >> 5) + 10);
			gwtogiant (&gwdata, b, v);
			
			mpz_init (__v);
			mpz_init (__N);
			mpz_init (__gcd);
			mpz_init (__inv);
			gtompz (v, __v);
			gtompz (N, __N);
			mpz_gcdext (__gcd, __inv, NULL, __v, __N);
			mpz_clear (__v);
			if (mpz_sgn (__inv) < 0) mpz_add (__inv, __inv, __N);
			mpztog (__inv, v);
			gianttogw (&gwdata, v, b);
			mpz_clear (__gcd);
			mpz_clear (__inv);
			mpz_clear (__N);
			pushg (&gwdata.gdata, 1);
		}
		end_timer (timers, 0);
		sprintf (buf, "Exp: %d, modinv: ", exp);
		print_timer (timers, 0, buf, TIMER_NL | TIMER_CLR);
		OutputBoth (thread_num, buf);
		gwdone (&gwdata);
	}
	return (0);
}
#endif

	tempFileName (w, filename);
	uniquifySaveFile (thread_num, filename);
	sprintf (ftree_filename, "%s.ftree", filename);

	srand ((unsigned) time (NULL));

	gwinit (&ecmdata.gwdata);
	if (IniGetInt (INI_FILE, "UseLargePages", 0)) gwset_use_large_pages (&ecmdata.gwdata);
	if (HYPERTHREAD_LL) sp_info->normal_work_hyperthreading = TRUE, gwset_will_hyperthread (&ecmdata.gwdata, 2);
	gwset_bench_cores (&ecmdata.gwdata, HW_NUM_CORES);
	gwset_bench_workers (&ecmdata.gwdata, NUM_WORKERS);
	if (ERRCHK) gwset_will_error_check (&ecmdata.gwdata);
	gwset_num_threads (&ecmdata.gwdata, ecmdata.stage1_threads);
	gwset_thread_callback (&ecmdata.gwdata, SetAuxThreadPriority);
	gwset_thread_callback_data (&ecmdata.gwdata, sp_info);
	gwset_safety_margin (&ecmdata.gwdata, IniGetFloat (INI_FILE, "ExtraSafetyMargin", 0.0));
	gwset_larger_fftlen_count (&ecmdata.gwdata, maxerr_restart_count < 3 ? maxerr_restart_count : 3);
	gwset_minimum_fftlen (&ecmdata.gwdata, w->minimum_fftlen);
	gwset_use_spin_wait (&ecmdata.gwdata, IniGetInt (INI_FILE, "SpinWait", 0));
	if (w->n) res = gwsetup (&ecmdata.gwdata, w->k, w->b, w->n, w->c);
	else res = gwsetup_general_mod_giant (&ecmdata.gwdata, ecmdata.N);
	if (res) {
		sprintf (buf, "Cannot initialize FFT code, errcode=%d\n", res);
		OutputBoth (thread_num, buf);
		return (STOP_FATAL_ERROR);
	}
	gwerror_checking (&ecmdata.gwdata, ERRCHK);
	ecmdata.stage1_fftlen = gwfftlen (&ecmdata.gwdata);

	stage1_timer = stage2_timer = 0.0;
	last_output = last_output_t = ecmdata.modinv_count = 0;
	gw_clear_fft_count (&ecmdata.gwdata);
	first_iter_msg = TRUE;
	calc_output_frequencies (&ecmdata.gwdata, &output_frequency, &output_title_frequency);
	dictionary_memory = (uint64_t) IniGetInt (INI_FILE, "DictionaryMemory", 256) << 20;	

	if (IniGetInt (INI_FILE, "ProbablePrimeTest", 0) && isProbablePrime (&ecmdata.gwdata, ecmdata.N)) {
		sprintf (buf, "%s is a probable prime\n", ecmdata.N_short_string_rep);
		OutputStr (thread_num, buf);
	}

	gwfft_description (&ecmdata.gwdata, fft_desc);
	sprintf (buf, "\nUsing %s\n", fft_desc);
	OutputStr (thread_num, buf);
	sprintf (buf, "%5.3f bits-per-word below FFT limit\n", (ecmdata.gwdata.EXTRA_BITS - EB_GWMUL_SAVINGS) / 2.0);
	OutputStr (thread_num, buf);

	near_fft_limit = exponent_near_fft_limit (&ecmdata.gwdata);
	gwerror_checking (&ecmdata.gwdata, ERRCHK || near_fft_limit);

	allowable_maxerr = IniGetFloat (INI_FILE, "MaxRoundoffError", (float) 0.4375);

	readSaveFileStateInit (&ecmdata.read_save_file_state, thread_num, filename);
	writeSaveFileStateInit (&ecmdata.write_save_file_state, filename, 0);
	for ( ; ; ) {
		if (! saveFileExists (&ecmdata.read_save_file_state)) {
			
			if (ecmdata.read_save_file_state.a_non_bad_save_file_existed) {
				OutputBoth (thread_num, ALLSAVEBAD_MSG);
				return (0);
			}
			
			break;
		}

		if (! ecm_restore (&ecmdata)) {
			
			saveFileBad (&ecmdata.read_save_file_state);
			continue;
		}

		if (w->gmp_ecm_file != NULL) {
			fclose (ecmdata.gmp_ecm_file);
			ecmdata.gmp_ecm_file = fopen (w->gmp_ecm_file, "r");
			if (ecmdata.gmp_ecm_file == NULL) {
				sprintf (buf, "File named '%s' could not be opened.\n", w->gmp_ecm_file);
				OutputStr (thread_num, buf);
				stop_reason = STOP_FILE_IO_ERROR;
				goto exit;
			}
			
			for (i = 0; i < (int) (w->skip_curves + ecmdata.curve); i++) skip_resumefile_line (ecmdata.gmp_ecm_file);
		}

		curve_start_msg (&ecmdata);

		if (ecmdata.state == ECM_STATE_STAGE1) {
			stop_reason = init_curve (&ecmdata);
			if (stop_reason) goto exit;
			if (ecmdata.factor != NULL) goto bingo;				
			if (!ecmdata.montg_stage1 && !ed_check (&ecmdata, &ecmdata.e)) {
				saveFileBad (&ecmdata.read_save_file_state);		
				goto ed_err;
			}
			goto restart1;
		}

		if (ecmdata.state == ECM_STATE_MIDSTAGE) {
			goto restart3;
		}

		if (ecmdata.state == ECM_STATE_STAGE2) {
			goto restart3;
		}

		ASSERTG (ecmdata.state == ECM_STATE_GCD);
		goto restart4;
	}

	ecmdata.curve = 1;
	ecmdata.average_B2 = 0;

restart0:

	if (w->gmp_ecm_file != NULL) {
		mpz_t	x, n, sigma;
		int	param;
		double	b1;
		mpz_inits (x, n, sigma, 0);
		if (!read_resumefile_line (ecmdata.thread_num, ecmdata.gmp_ecm_file, x, n, sigma, &param, &b1)) {
			w->curves_to_do = ecmdata.curve - 1;
			goto no_more_curves;
		}
		if (param == 0) ecmdata.sigma_type = 1;
		else if (param == 3) ecmdata.sigma_type = 3;
		else {
			sprintf (buf, "Unsupported GMP-ECM param=%d\n", param);
			OutputStr (ecmdata.thread_num, buf);
			stop_reason = STOP_FILE_IO_ERROR;
			goto exit;
		}
		mpz_get_str (buf, 10, sigma);
		ecmdata.sigma = atoll (buf);
		ecmdata.B = (uint64_t) b1;
		ecmdata.state = ECM_STATE_MIDSTAGE;
		ecmdata.Qx_binary = allocgiant (((int) ecmdata.gwdata.bit_length >> 5) + 10);
		if (ecmdata.Qx_binary == NULL) goto oom;
		mpztog (x, ecmdata.Qx_binary);
		mpz_clears (x, n, sigma, 0);
		curve_start_msg (&ecmdata);
		stop_reason = init_curve (&ecmdata);
		if (stop_reason) goto exit;
		goto restart3;
	}

	ecmdata.state = ECM_STATE_STAGE1_INIT;
	ecmdata.C_done = 0;
	ecmdata.pct_mem_to_use = 1.0;				
	ecm_stage1_memory_usage (thread_num, &ecmdata);
	last_output = last_output_t = ecmdata.modinv_count = 0;
	gw_clear_fft_count (&ecmdata.gwdata);
	ASSERTG (ecmdata.xz.x == NULL && ecmdata.xz.z == NULL);
	ASSERTG (ecmdata.e.x == NULL && ecmdata.e.y == NULL && ecmdata.e.z == NULL);
	ASSERTG (ecmdata.gg == NULL);

	do {
		uint32_t hi, lo;
		ecmdata.sigma = ((uint64_t) (rand () & 0x1F)) << 48;
		ecmdata.sigma += ((uint64_t) (rand () & 0xFFFF)) << 32;
		if (CPU_FLAGS & CPU_RDTSC) rdtsc (&hi, &lo);
		ecmdata.sigma += lo ^ hi ^ ((uint32_t) rand () << 16);
	} while (ecmdata.sigma <= 5);
	if (w->curve > 5) {
		ecmdata.sigma = w->curve;
		w->curves_to_do = 1;
	}
	default_sigma_type = (IniGetInt (INI_FILE, "DictionarySize", (int) (dictionary_memory / (3 * gwnum_size (&ecmdata.gwdata)))) < 4) ? 1 : 0;
	ecmdata.sigma_type = IniGetInt (INI_FILE, "MontgSigma", default_sigma_type);
	ecmdata.montg_stage1 = IniGetInt(INI_FILE, "MontgStage1", ecmdata.sigma_type == 1);
	curve_start_msg (&ecmdata);
	stop_reason = init_curve (&ecmdata);
	if (stop_reason) goto exit;
	if (ecmdata.factor != NULL) goto bingo;
	ecmdata.stage1_prime = 0;				
	ecmdata.stage1_bitnum = 0;				
	ecmdata.state = ECM_STATE_STAGE1;
	sprintf (w->stage, "C%" PRIu32 "S1", ecmdata.curve);
	w->pct_complete = 0.0;
	start_timer_from_zero (&stage1_timer, 0);

restart1:
	one_over_B = 1.0 / (double) ecmdata.B;
	start_timer_from_zero (timers, 0);
	start_timer_from_zero (timers, 1);

	if (ecmdata.montg_stage1) {
		ecm_stage1_memory_usage (thread_num, &ecmdata);
		unsigned long SQRT_B = (unsigned long) sqrt ((double) ecmdata.B);
		
		stop_reason = start_sieve_with_limit (thread_num, ecmdata.stage1_prime + 1, 16 * SQRT_B, &ecmdata.sieve_info);
		if (stop_reason) goto exit;
		for (ecmdata.stage1_prime = sieve (ecmdata.sieve_info); ecmdata.stage1_prime <= ecmdata.B; ecmdata.stage1_prime = next_prime) {
			int	count;

			next_prime = sieve (ecmdata.sieve_info);

			stop_reason = stopCheck (thread_num);
			saving = testSaveFilesFlag (thread_num);

			if (ecmdata.stage1_prime <= SQRT_B) {
				uint64_t mult, max;
				count = 0;
				max = ecmdata.B / ecmdata.stage1_prime;
				for (mult = ecmdata.stage1_prime; ; mult *= ecmdata.stage1_prime) {
					count++;
					if (mult > max) break;
				}
			} else
				count = 1;

			if (ecmdata.stage1_prime <= 3) count += (ecmdata.stage1_prime == 2) ? 2 : 1;

			for ( ; count; count--) {
				int last_mul = (count == 1 && (saving || stop_reason || next_prime > ecmdata.B));
				int oom_stop_reason = ell_mul (&ecmdata, &ecmdata.xz, ecmdata.stage1_prime, last_mul);
				if (oom_stop_reason) {	
					stop_reason = oom_stop_reason;
					goto exit;
				}
			}

			w->pct_complete = ecmdata.stage1_prime * one_over_B;

			if (first_iter_msg ||
			    (ITER_OUTPUT != 999999999 && gw_get_fft_count (&ecmdata.gwdata) >= last_output_t + 2 * ITER_OUTPUT * output_title_frequency)) {
				sprintf (buf, "%.*f%% of %s ECM curve %" PRIu32 " stage 1",
					 (int) PRECISION, trunc_percent (w->pct_complete), ecmdata.N_short_string_rep, ecmdata.curve);
				title (thread_num, buf);
				last_output_t = gw_get_fft_count (&ecmdata.gwdata);
			}

			if (first_iter_msg ||
			    (ITER_OUTPUT != 999999999 && gw_get_fft_count (&ecmdata.gwdata) >= last_output + 2 * ITER_OUTPUT * output_frequency)) {
				sprintf (buf, "%s curve %" PRIu32 " stage 1 at prime %" PRIu64 " [%.*f%%].",
					 ecmdata.N_short_string_rep, ecmdata.curve, ecmdata.stage1_prime, (int) PRECISION, trunc_percent (w->pct_complete));
				end_timer (timers, 0);
				if (first_iter_msg) {
					strcat (buf, "\n");
				} else {
					strcat (buf, " Time: ");
					print_timer (timers, 0, buf, TIMER_NL);
				}
				if (ecmdata.stage1_prime != 2)
					OutputStr (thread_num, buf);
				start_timer_from_zero (timers, 0);
				last_output = gw_get_fft_count (&ecmdata.gwdata);
				first_iter_msg = FALSE;
			}

			if (gw_test_for_error (&ecmdata.gwdata) || gw_get_maxerr (&ecmdata.gwdata) > allowable_maxerr) goto err;

			if (stop_reason || saving) {
				ecm_save (&ecmdata);
				if (stop_reason) goto exit;
			}
		}
	}

	else {

		if (ecmdata.stage1_bitnum == 0) {
			ecmdata.stage1_start_prime = 2;
			ecmdata.stage1_exp_buffer_size = IniGetInt (INI_FILE, "ECMStage1ExpBufferSize", 40);
			if (ecmdata.stage1_exp_buffer_size < 1) ecmdata.stage1_exp_buffer_size = 1;
			if (ecmdata.stage1_exp_buffer_size > 128) ecmdata.stage1_exp_buffer_size = 128;
			ecmdata.stage1_exp_buffer_size <<= 20;
		}

		stop_reason = start_sieve_with_limit (thread_num, ecmdata.stage1_start_prime, (uint32_t) sqrt ((double) ecmdata.B), &ecmdata.sieve_info);
		if (stop_reason) goto exit;
		ecmdata.stage1_prime = sieve (ecmdata.sieve_info);	

		mpz_init (ecmdata.stage1_exp), ecmdata.stage1_exp_initialized = TRUE;

		for ( ; ; ) {
			int	NAF_index;
			uint32_t best_size;
			uint64_t num_doublings;
			double	chunk_size, chunk_size_over_num_doublings;

			ecmdata.stage1_start_prime = ecmdata.stage1_prime;	
			ecm_calc_exp (ecmdata.sieve_info, ecmdata.stage1_exp, ecmdata.B, &ecmdata.stage1_prime, ecmdata.stage1_exp_buffer_size);
			if (mpz_eq_ui (ecmdata.stage1_exp, 1)) break;
			best_size = NAF_best_size (&ecmdata);
			if (ecmdata.stage1_bitnum == 0) {			
				ecmdata.NAF_dictionary_size = (uint32_t) (dictionary_memory / (3 * gwnum_size (&ecmdata.gwdata)));
				if (ecmdata.NAF_dictionary_size > best_size) ecmdata.NAF_dictionary_size = best_size;
				ecmdata.NAF_dictionary_size = IniGetInt (INI_FILE, "DictionarySize", ecmdata.NAF_dictionary_size);  
				if (ecmdata.NAF_dictionary_size < 4) ecmdata.NAF_dictionary_size = 4;		
			}
			sprintf (buf, "%sxponent length is %" PRIu64 ", best dictionary size is %" PRIu32 ", actual dictionary size is %" PRIu32 "\n",
				 ecmdata.stage1_prime <= ecmdata.B ? "Partial e" : "E",
				 (uint64_t) mpz_sizeinbase (ecmdata.stage1_exp, 2), best_size, ecmdata.NAF_dictionary_size);
			OutputStr (thread_num, buf);
			ecm_stage1_memory_usage (thread_num, &ecmdata);
			if (ecmdata.stage1_bitnum == 0) ed_swap (ecmdata.e, ecmdata.dict_start);
			NAF_dictionary_init (&ecmdata, &NAF_index, &num_doublings);
			if (ecmdata.factor != NULL) goto bingo;			

			chunk_size = (double) ((ecmdata.stage1_prime < ecmdata.B ? ecmdata.stage1_prime : ecmdata.B) - ecmdata.stage1_start_prime);
			chunk_size_over_num_doublings = chunk_size / (double) num_doublings;

			while (ecmdata.stage1_bitnum < num_doublings) {

				struct ed *ed_dbl_src = &ecmdata.e;		
				if (ecmdata.stage1_bitnum == 0) {
					if (!ed_alloc (&ecmdata, &ecmdata.e)) goto oom;
					ed_dbl_src = &ecmdata.NAF_dictionary[NAF_index];
				}

				if (! mpz_tstbit64 (ecmdata.stage1_exp, num_doublings - ecmdata.stage1_bitnum - 1)) {
					stop_reason = 0;
					saving = 0;
					gwerror_checking (&ecmdata.gwdata, ERRCHK || near_fft_limit || ((ecmdata.stage1_bitnum & 127) == 64));
					int options = (ecmdata.stage1_bitnum+1 == num_doublings) ? ED_XYZ : ED_STARTNEXTFFT;
					ed_dbl (&ecmdata, ed_dbl_src, &ecmdata.e, ED_RESULT_FOR_DBL | options);
				} else {
					bool	subtract;
					gwerror_checking (&ecmdata.gwdata, ERRCHK || near_fft_limit || ((ecmdata.stage1_bitnum & 127) == 64));
					ed_dbl (&ecmdata, ed_dbl_src, &ecmdata.e, ED_RESULT_FOR_ADD | ED_STARTNEXTFFT);
					stop_reason = stopCheck (thread_num);
					saving = testSaveFilesFlag (thread_num);
					gwerror_checking (&ecmdata.gwdata, stop_reason || saving || ERRCHK || near_fft_limit);
					NAF_code (&ecmdata, num_doublings - ecmdata.stage1_bitnum - 1, &NAF_index, &subtract);
					int options = (stop_reason || saving || ecmdata.stage1_bitnum+1 == num_doublings) ? ED_XYZ : ED_STARTNEXTFFT;
					if (subtract) options |= ED_SUBTRACT;
					ed_add (&ecmdata, &ecmdata.e, &ecmdata.NAF_dictionary[NAF_index], &ecmdata.e, ED_RESULT_FOR_DBL | options);
				}

				ecmdata.stage1_bitnum++;
				w->pct_complete = (ecmdata.stage1_start_prime + ecmdata.stage1_bitnum * chunk_size_over_num_doublings) * one_over_B;

				if (first_iter_msg ||
				    (ITER_OUTPUT != 999999999 && gw_get_fft_count (&ecmdata.gwdata) >= last_output_t + 2 * ITER_OUTPUT * output_title_frequency)) {
					sprintf (buf, "%.*f%% of %s ECM curve %" PRIu32 " stage 1",
						 (int) PRECISION, trunc_percent (w->pct_complete), ecmdata.N_short_string_rep, ecmdata.curve);
					title (thread_num, buf);
					last_output_t = gw_get_fft_count (&ecmdata.gwdata);
				}

				if (first_iter_msg ||
				    (ITER_OUTPUT != 999999999 && gw_get_fft_count (&ecmdata.gwdata) >= last_output + 2 * ITER_OUTPUT * output_frequency)) {
					sprintf (buf, "%s curve %" PRIu32 " stage 1 at exponent bit %" PRIu32 " [%.*f%%].",
						 ecmdata.N_short_string_rep, ecmdata.curve, ecmdata.stage1_bitnum,
						 (int) PRECISION, trunc_percent (w->pct_complete));
					end_timer (timers, 0);
					if (first_iter_msg) {
						strcat (buf, "\n");
					} else {
						strcat (buf, " Time: ");
						print_timer (timers, 0, buf, TIMER_NL);
					}
					if (ecmdata.stage1_bitnum != 2) OutputStr (thread_num, buf);
					start_timer_from_zero (timers, 0);
					last_output = gw_get_fft_count (&ecmdata.gwdata);
					first_iter_msg = FALSE;
				}

				if (gw_test_for_error (&ecmdata.gwdata) || gw_get_maxerr (&ecmdata.gwdata) > allowable_maxerr) goto err;

				if (stop_reason || saving) {
					if (!ed_check (&ecmdata, &ecmdata.e)) goto ed_err;
					ecm_save (&ecmdata);
					if (stop_reason) goto exit;
				}
			}

			NAF_dictionary_free (&ecmdata);

			if (ecmdata.stage1_prime > ecmdata.B) break;
			ecmdata.stage1_bitnum = 0;
		}
		mpz_clear (ecmdata.stage1_exp), ecmdata.stage1_exp_initialized = FALSE;
		ecmdata.stage1_bitnum = 0;

		if (!ed_check (&ecmdata, &ecmdata.e)) goto ed_err;

		ed_to_Montgomery (&ecmdata);
		ecmdata.montg_stage1 = TRUE;		
	}

	end_timer (timers, 0);
	end_timer (timers, 1);
	if (stage1_timer != 0.0) end_timer (&stage1_timer, 0);
	sprintf (buf, "Stage 1 complete. %" PRIu64 " transforms, %lu modular inverses. Total time: ", gw_get_fft_count (&ecmdata.gwdata), ecmdata.modinv_count);
	print_timer (timers, 1, buf, TIMER_NL | TIMER_CLR);
	OutputStr (thread_num, buf);
	last_output = last_output_t = ecmdata.modinv_count = 0;
	gw_clear_fft_count (&ecmdata.gwdata);

	if (ERRCHK) {
		sprintf (buf, "Round off: %.10g\n", gw_get_maxerr (&ecmdata.gwdata));
		OutputStr (thread_num, buf);
		gw_clear_maxerr (&ecmdata.gwdata);
	}

	if (ecmdata.C <= ecmdata.B) {
skip_stage_2:	start_timer_from_zero (timers, 0);
		if (ecmdata.xz.z != NULL) stop_reason = gcd (&ecmdata.gwdata, thread_num, ecmdata.xz.z, ecmdata.N, &ecmdata.factor);
		else stop_reason = gcd (thread_num, ecmdata.Qz_binary, ecmdata.N, &ecmdata.factor);
		if (stop_reason) {
			ecm_save (&ecmdata);
			goto exit;
		}
		end_timer (timers, 0);
		strcpy (buf, "Stage 1 GCD complete. Time: ");
		print_timer (timers, 0, buf, TIMER_NL | TIMER_CLR);
		OutputStr (thread_num, buf);
		if (ecmdata.factor != NULL) goto bingo;

		if (IniGetInt (INI_FILE, "GmpEcmHook", 0)) {
			char	*msg, *buf;
			int	msglen, i, leadingzeroes;
			giant	gx;
			const char *hex = "0123456789ABCDEF";

			stop_reason = normalize (&ecmdata, ecmdata.xz.x, ecmdata.xz.z);
			if (stop_reason) goto exit;

			gx = popg (&ecmdata.gwdata.gdata, ((int) ecmdata.gwdata.bit_length >> 5) + 10);
			if (gx == NULL) goto oom;
			if (gwtogiant (&ecmdata.gwdata, ecmdata.xz.x, gx)) goto oom;  
			modgi (&ecmdata.gwdata.gdata, ecmdata.N, gx);

			msglen = ecmdata.N->sign * 8 + 5;
			buf = (char *) malloc (msglen + msglen + 80);
			if (buf == NULL) goto oom;
			strcpy (buf, "N=0x");
			msg = buf + strlen (buf);
			leadingzeroes = 1; 
			for (i = 0; i < ecmdata.N->sign * 8; i++) {
				char nibble = ( ecmdata.N->n[ecmdata.N->sign - 1 - i/8] >> ((7-i%8)*4)) & 0xF;
				if (nibble != 0) leadingzeroes = 0;
				if (!leadingzeroes) *msg++ = hex[nibble];
			}
			strcpy (msg, "; QX=0x");
			msg = msg + strlen (msg);
			leadingzeroes = 1;
			for (i = 0; i < gx->sign * 8; i++) {
				char nibble = ( gx->n[gx->sign - 1 - i/8] >> ((7-i%8)*4)) & 0xF;
				if (nibble != 0) leadingzeroes = 0;
				if (!leadingzeroes) *msg++ = hex[nibble];
			}
			strcpy (msg, "; SIGMA=");
			msg = msg + strlen (msg);
			sprintf (msg, "%" PRIu64 "\n", ecmdata.sigma);
			writeResults (buf);
			free (buf);
			pushg (&ecmdata.gwdata.gdata, 1);
		}

		goto more_curves;
	}

	ecmdata.Qx_binary = allocgiant (((int) ecmdata.gwdata.bit_length >> 5) + 10);
	if (ecmdata.Qx_binary == NULL) goto oom;
	gwtogiant (&ecmdata.gwdata, ecmdata.xz.x, ecmdata.Qx_binary);
	ecmdata.Qz_binary = allocgiant (((int) ecmdata.gwdata.bit_length >> 5) + 10);
	if (ecmdata.Qz_binary == NULL) goto oom;
	gwtogiant (&ecmdata.gwdata, ecmdata.xz.z, ecmdata.Qz_binary);
	free_xz (&ecmdata, &ecmdata.xz);

	ecmdata.state = ECM_STATE_MIDSTAGE;
	sprintf (w->stage, "C%" PRIu32 "S2", ecmdata.curve);
	w->pct_complete = 0.0;
	start_timer_from_zero (&stage2_timer, 0);

	if (IniGetInt (INI_FILE, "SaveBeforeStage2", 1)) ecm_save (&ecmdata);

	ecmdata.required_missing = 0;

restart3:
	ASSERTG (ecmdata.xz.x == NULL && ecmdata.xz.x == NULL && ecmdata.gg == NULL);
	Dmultiple_map.clear ();			
	relp_set_map.clear ();			

	unsigned long min_memory;
	min_memory = cvt_gwnums_to_mem (&ecmdata.gwdata, 13.0);
	if ((int) max_mem (thread_num) < min_memory) {
		sprintf (buf, "Skipping stage 2 due to insufficient memory -- %luMB needed.\n", min_memory);
		OutputStr (thread_num, buf);
		goto skip_stage_2;
	}

	start_timer_from_zero (timers, 0);
	sprintf (buf, "%s ECM curve %" PRIu32 " stage 2 init", ecmdata.N_short_string_rep, ecmdata.curve);
	title (thread_num, buf);

replan:	unsigned long best_fftlen;
	unsigned int memory;
	int	forced_stage2_type, best_stage2_type, best_fails;
	double	best_efficiency, best_poly_efficiency;
	char	msgbuf[2000];

	forced_stage2_type = IniGetInt (INI_FILE, "ForceECMStage2Type", 99);		
	memory = 0;
	best_fftlen = 0;
	best_fails = 0;
	best_poly_efficiency = 0.0;
	msgbuf[0] = 0;
	for (bool found_best = FALSE; ; ) {

		polymult_init (&ecmdata.polydata, &ecmdata.gwdata);
		polymult_set_max_num_threads (&ecmdata.polydata, ecmdata.stage2_threads);
		polymult_default_tuning (&ecmdata.polydata,
					 IniGetInt (INI_FILE, "PolymultCacheSize", CPU_NUM_L2_CACHES > 0 ? CPU_TOTAL_L2_CACHE_SIZE / CPU_NUM_L2_CACHES : 256),
					 IniGetInt (INI_FILE, "PolymultCacheSize2", CPU_NUM_L3_CACHES > 0 ? CPU_TOTAL_L3_CACHE_SIZE / CPU_NUM_L3_CACHES : 6144));
		ecmdata.gwdata.scramble_arrays = (char) IniGetInt (INI_FILE, "PolymultScramble", ecmdata.gwdata.scramble_arrays);
		ecmdata.polydata.strided_writes_end = IniGetInt64 (INI_FILE, "PolymultStridedWritesEnd", ecmdata.polydata.strided_writes_end);
		ecmdata.polydata.streamed_stores_start = IniGetInt64 (INI_FILE, "PolymultStreamStores", 0); 
		gwset_polymult_safety_margin (&ecmdata.gwdata, 0.0);		

		if (gwfftlen (&ecmdata.gwdata) > maxerr_fftlen) {
			double efficiency;

			if (memory == 0) {
				unsigned int min_memory, desired_memory;	
				min_memory = 1 + cvt_gwnums_to_mem (&ecmdata.gwdata, 13);
				if (ecmdata.state < ECM_STATE_STAGE2 || ecmdata.pairmap == NULL) desired_memory = 3 + cvt_gwnums_to_mem (&ecmdata.gwdata, 250);
				else desired_memory = (unsigned int) (ecmdata.pairmap_size >> 20) + cvt_gwnums_to_mem (&ecmdata.gwdata, ecmdata.stage2_numvals);
				stop_reason = avail_mem (thread_num, min_memory, desired_memory, &memory);
				if (stop_reason) {
					if (ecmdata.state == ECM_STATE_MIDSTAGE) ecm_save (&ecmdata);
					goto exit;
				}

				memory = (unsigned int) (ecmdata.pct_mem_to_use * (double) memory);
				if (memory < min_memory) {
					stop_reason = avail_mem_not_sufficient (thread_num, min_memory, desired_memory);
					if (ecmdata.state == ECM_STATE_MIDSTAGE) ecm_save (&ecmdata);
					goto exit;
				}

				sprintf (buf, "Available memory is %uMB.\n", memory);
				OutputStr (thread_num, buf);
			}

			efficiency = ecm_stage2_impl (&ecmdata, memory, found_best ? best_stage2_type : best_fftlen ? 1 : forced_stage2_type, msgbuf + strlen (msgbuf));
			
			if (ecmdata.pairmap != NULL) break;

			efficiency *= 1.0e13;
if (IniGetInt (INI_FILE, "PolyVerbose", 0)) {
if (efficiency == 0.0) sprintf (buf, "FFT: %d no stage 2 plan works\n", (int) gwfftlen (&ecmdata.gwdata));
else sprintf (buf, "FFT: %d, B2: %" PRIu64 "/%" PRIu64 ", numvals: %" PRIu64 "/%d, poly: %" PRIu64 ", efficiency: %.0f\n", (int) gwfftlen (&ecmdata.gwdata), ecmdata.C,
		ecmdata.B2_start + ecmdata.numDsections * ecmdata.D,
		ecmdata.stage2_numvals, (int) cvt_mem_to_gwnums_adj (&ecmdata.gwdata, memory, 0.0), ecmdata.poly_size, efficiency);
OutputStr (thread_num, buf); }

			if (found_best) break;

			if (efficiency != 0.0 && (best_fftlen == 0 || efficiency > best_efficiency)) {
				best_fftlen = gwfftlen (&ecmdata.gwdata);
				best_stage2_type = ecmdata.stage2_type;
				best_efficiency = efficiency;
			}

			if (ecmdata.stage2_type == ECM_STAGE2_POLYMULT && efficiency > best_poly_efficiency) {
				best_poly_efficiency = efficiency;
				best_fails = 0;
			}
		}

		if (best_fftlen && forced_stage2_type == 0) found_best = TRUE;

		if (best_fftlen && (double) memory * 1000000.0 / (double) array_gwnum_size (&ecmdata.gwdata) < 60.0) found_best = TRUE;

		if (best_poly_efficiency != 0.0) best_fails++;
		if (best_fails == 3) found_best = TRUE;

		for ( ; ; ) {
			unsigned long next_fftlen = gwfftlen (&ecmdata.gwdata) + 1;  
			if (next_fftlen < maxerr_fftlen) next_fftlen = maxerr_fftlen;
			if (found_best) next_fftlen = best_fftlen;
			msgbuf[0] = 0;
			
			if (next_fftlen == gwfftlen (&ecmdata.gwdata)) break;
			
			if (ecmdata.Ad4 != NULL) {
				ecmdata.Ad4_binary = allocgiant (((int) ecmdata.gwdata.bit_length >> 5) + 10);
				if (ecmdata.Ad4_binary == NULL) goto oom;
				gwtogiant (&ecmdata.gwdata, ecmdata.Ad4, ecmdata.Ad4_binary);
				ecmdata.Ad4 = NULL;
			}
			polymult_done (&ecmdata.polydata);
			gwdone (&ecmdata.gwdata);
			
			gwinit (&ecmdata.gwdata);
			if (next_fftlen != best_fftlen) gwset_information_only (&ecmdata.gwdata);
			if (IniGetInt (INI_FILE, "UseLargePages", 0)) gwset_use_large_pages (&ecmdata.gwdata);
			if (HYPERTHREAD_LL) sp_info->normal_work_hyperthreading = TRUE, gwset_will_hyperthread (&ecmdata.gwdata, 2);
			gwset_bench_cores (&ecmdata.gwdata, HW_NUM_CORES);
			gwset_bench_workers (&ecmdata.gwdata, NUM_WORKERS);
			if (ERRCHK) gwset_will_error_check (&ecmdata.gwdata);
			gwset_num_threads (&ecmdata.gwdata, ecmdata.stage2_threads);
			gwset_thread_callback (&ecmdata.gwdata, SetAuxThreadPriority);
			gwset_thread_callback_data (&ecmdata.gwdata, sp_info);
			gwset_safety_margin (&ecmdata.gwdata, IniGetFloat (INI_FILE, "ExtraSafetyMargin", 0.0));
			gwset_minimum_fftlen (&ecmdata.gwdata, next_fftlen);
			gwset_using_polymult (&ecmdata.gwdata);
			gwset_use_spin_wait (&ecmdata.gwdata, IniGetInt (INI_FILE, "SpinWait", 0));
			if (w->n) res = gwsetup (&ecmdata.gwdata, w->k, w->b, w->n, w->c);
			else res = gwsetup_general_mod_giant (&ecmdata.gwdata, ecmdata.N);
			if (res == GWERROR_TOO_LARGE && best_fftlen) {
				found_best = TRUE;
				continue;
			}
			if (res) {
				sprintf (buf, "Cannot initialize FFT code, errcode=%d\n", res);
				OutputBoth (thread_num, buf);
				stop_reason = STOP_FATAL_ERROR;
				goto exit;
			}
			gwerror_checking (&ecmdata.gwdata, ERRCHK || near_fft_limit);
			if (gwfftlen (&ecmdata.gwdata) != ecmdata.stage1_fftlen) {
				char	fft_desc[200];
				gwfft_description (&ecmdata.gwdata, fft_desc);
				sprintf (msgbuf, "Switching to %s\n", fft_desc);
			}
			break;
		}
	}

	OutputStr (thread_num, msgbuf);

	if (!alloc_xz (&ecmdata, &ecmdata.xz)) goto lowmem;
	gianttogw (&ecmdata.gwdata, ecmdata.Qx_binary, ecmdata.xz.x);
	if (ecmdata.Qz_binary == NULL) dbltogw (&ecmdata.gwdata, 1.0, ecmdata.xz.z);
	else gianttogw (&ecmdata.gwdata, ecmdata.Qz_binary, ecmdata.xz.z);
	if (ecmdata.Ad4 == NULL) {
		if (ecmdata.Ad4_binary != NULL) {
			ecmdata.Ad4 = gwalloc (&ecmdata.gwdata);
			if (ecmdata.Ad4 == NULL) goto lowmem;
			gianttogw (&ecmdata.gwdata, ecmdata.Ad4_binary, ecmdata.Ad4);
			free (ecmdata.Ad4_binary), ecmdata.Ad4_binary = NULL;
			gwfft (&ecmdata.gwdata, ecmdata.Ad4, ecmdata.Ad4);
		} else {
			stop_reason = init_curve (&ecmdata);
			if (stop_reason) goto possible_lowmem;
		}
	}

	xzswap (ecmdata.xz, ecmdata.QD);

	memused = cvt_array_gwnums_to_mem (&ecmdata.gwdata, ecmdata.stage2_numvals);
	memused += 2 * (int) ecmdata.gwdata.bit_length >> 23;
	memused += (int) (ecmdata.pairmap_size >> 20);
	
	if (set_memory_usage (thread_num, MEM_VARIABLE_USAGE, memused)) {
		ecmdata.pct_mem_to_use *= 0.99;
		free (ecmdata.pairmap); ecmdata.pairmap = NULL;
		goto replan;
	}

	if (ecmdata.stage2_type == ECM_STAGE2_POLYMULT) goto ecm_polymult;

	polymult_done (&ecmdata.polydata);

	sprintf (buf, "Using %uMB of memory.\n", memused);
	OutputStr (thread_num, buf);

	if (ecmdata.pairmap == NULL) {
		int fill_window = pair_window_size (ecmdata.gwdata.bit_length, ecmdata.relp_sets);
		gwfree_internal_memory (&ecmdata.gwdata);
		stop_reason = fill_pairmap (ecmdata.thread_num, &ecmdata.sieve_info, ecmdata.D, fill_window,0,0,0,
					    ecmdata.totrels, ecmdata.relp_sets+3, ecmdata.first_relocatable, ecmdata.last_relocatable,
					    ecmdata.B2_start, ecmdata.C, ecmdata.max_pairmap_Dsections, &ecmdata.pairmap, &ecmdata.pairmap_size);
		if (stop_reason) goto exit;
		ecmdata.pairmap_ptr = ecmdata.pairmap;
		ecmdata.Dsection = 0;
		ecmdata.relp = -1;			
	}

	process_relp_sets (ecmdata.totrels, ecmdata.numrels, ecmdata.relp_sets, relp_set_map, Dmultiple_map);

	if (ecmdata.TWO_FFT_STAGE2 && ecmdata.pool_type == POOL_3MULT)
		sprintf (buf, "Stage 2 uses %uMB of memory, 2 FFTs per prime pair, 3-mult modinv pooling, pool size %d.\n", memused, ecmdata.E);
	else if (ecmdata.TWO_FFT_STAGE2 && ecmdata.pool_type == POOL_3POINT44MULT)
		sprintf (buf, "Stage 2 uses %uMB of memory, 2 FFTs per prime pair, 3.44-mult modinv pooling, pool size %d.\n", memused, ecmdata.E);
	else if (ecmdata.TWO_FFT_STAGE2 && ecmdata.pool_type == POOL_3POINT57MULT)
		sprintf (buf, "Stage 2 uses %uMB of memory, 2 FFTs per prime pair, 3.57-mult modinv pooling, pool size %d.\n", memused, ecmdata.E);
	else if (ecmdata.TWO_FFT_STAGE2 && ecmdata.pool_type == POOL_N_SQUARED)
		sprintf (buf, "Stage 2 uses %uMB of memory, 2 FFTs per prime pair, N^2 modinv pooling, pool size %d.\n", memused, ecmdata.E);
	else if (!ecmdata.TWO_FFT_STAGE2 && ecmdata.pool_type == POOL_3MULT)
		sprintf (buf, "Stage 2 uses %uMB of memory, 4 FFTs per prime pair, 3-mult modinv pooling.\n", memused);
	else if (!ecmdata.TWO_FFT_STAGE2 && ecmdata.pool_type == POOL_3POINT44MULT)
		sprintf (buf, "Stage 2 uses %uMB of memory, 4 FFTs per prime pair, 3.44-mult modinv pooling.\n", memused);
	else if (!ecmdata.TWO_FFT_STAGE2 && ecmdata.pool_type == POOL_3POINT57MULT)
		sprintf (buf, "Stage 2 uses %uMB of memory, 4 FFTs per prime pair, 3.57-mult modinv pooling.\n", memused);
	else if (!ecmdata.TWO_FFT_STAGE2 && ecmdata.pool_type == POOL_N_SQUARED)
		sprintf (buf, "Stage 2 uses %uMB of memory, 4 FFTs per prime pair, N^2 modinv pooling.\n", memused);
	OutputStr (thread_num, buf);

	base_pct_complete = (double) (ecmdata.B2_start - ecmdata.last_relocatable) / (double) (ecmdata.C - ecmdata.last_relocatable);
	
	one_relp_pct = (1.0 - base_pct_complete) / (double) (ecmdata.numDsections * ecmdata.totrels);
	
	w->pct_complete = base_pct_complete + (double) (ecmdata.Dsection * ecmdata.totrels) * one_relp_pct;

	int	num_relp_sets;		
	num_relp_sets = (int) relp_set_map.size();
	ecmdata.nQx = (gwnum *) malloc (num_relp_sets * ecmdata.numrels * 2 * sizeof (gwnum));
	if (ecmdata.nQx == NULL) goto lowmem;

	stop_reason = normalize_pool_init (&ecmdata, ecmdata.stage2_numvals);
	if (stop_reason) goto possible_lowmem;

	int	z_offset;		
	int	set_minus1_nQx_index;	
	z_offset = num_relp_sets * ecmdata.numrels;
	set_minus1_nQx_index = relp_set_map.find(-1)->second.nQx_store;
	
	#define nqxmap(relp)	((relp) < ecmdata.numrels ? (relp) : set_minus1_nQx_index + (relp) - ecmdata.numrels)

	if (!ecmdata.TWO_FFT_STAGE2 || ecmdata.D % 3 != 0) {
		struct xz t1, t2, t3, t4;
		struct xz *Q1, *Q2, *Q3, *Qi, *Qiminus2, *Qscratch;
		int	have_QD;
		uint32_t totrels;

		if (!alloc_xz (&ecmdata, &t1)) goto lowmem;
		if (!alloc_xz (&ecmdata, &t2)) goto lowmem;
		if (!alloc_xz (&ecmdata, &t3)) goto lowmem;
		t4.x = NULL; t4.z = NULL;					

		xzswap (t1, ecmdata.QD);					
		Q1 = &t1;							
		Q2 = &ecmdata.QD;
		ell_dbl_xz_scr (&ecmdata, Q1, Q2, &t3);				
		ecmdata.nQx[0] = Q1->x; ecmdata.nQx[z_offset] = Q1->z; totrels = 1;

		Q3 = &t2;
		ell_add_xz_scr (&ecmdata, Q2, Q1, Q1, Q3, &t3);			
		ASSERTG (ecmdata.D % 3 == 0);					

		Qi = Q3;
		Qiminus2 = Q1;
		Qscratch = &t3;
		have_QD = FALSE;
		for (i = 3; ; i += 2) {

			if (i + i == ecmdata.D) {
				ASSERTG (Q2 == &ecmdata.QD);
				
				if (totrels != ecmdata.totrels) {
					t4 = ecmdata.QD; Q2 = &t4;				
					if (!alloc_xz (&ecmdata, &ecmdata.QD)) goto lowmem;	
				}
				ell_dbl_xz_scr (&ecmdata, Qi, &ecmdata.QD, Qscratch);		
				have_QD = TRUE;
			}
			if (i + i-2 == ecmdata.D) {
				ASSERTG (Q2 == &ecmdata.QD);
				
				if (totrels != ecmdata.totrels) {
					t4 = ecmdata.QD; Q2 = &t4;				
					if (!alloc_xz (&ecmdata, &ecmdata.QD)) goto lowmem;	
				}
				ell_add_xz (&ecmdata, Qi, Qiminus2, Q2, &ecmdata.QD);		
				have_QD = TRUE;
			}

			if (have_QD && (totrels == ecmdata.totrels || totrels == ecmdata.numrels * 2)) break;

			ell_add_xz (&ecmdata, Qi, Q2, Qiminus2, Qscratch);			
			if (relatively_prime (i-2, ecmdata.D)) {
				if (ecmdata.totrels <= ecmdata.numrels * 2) {
					stop_reason = add_to_normalize_pool_ro (&ecmdata, Qiminus2->x, Qiminus2->z);
					if (stop_reason) goto possible_lowmem;
				}
				if (!alloc_xz (&ecmdata, Qiminus2)) goto lowmem;
			}
			xzswap (*Qiminus2, *Qi);
			xzswap (*Qi, *Qscratch);
			if (relatively_prime (i+2, ecmdata.D)) {
				ecmdata.nQx[nqxmap(totrels)] = Qi->x;
				ecmdata.nQx[nqxmap(totrels) + z_offset] = Qi->z;
				totrels++;
			}

			if (gw_test_for_error (&ecmdata.gwdata) || gw_get_maxerr (&ecmdata.gwdata) > allowable_maxerr) goto err;
			stop_reason = stopCheck (thread_num);
			if (stop_reason) goto possible_lowmem;
		}

		free_xz (&ecmdata, &t4);

		for (i = 1; i <= 2; i++) {
			struct xz *needs_work;
			if (t1.x == ecmdata.nQx[nqxmap(totrels-i)]) needs_work = &t1;
			else if (t2.x == ecmdata.nQx[nqxmap(totrels-i)]) needs_work = &t2;
			else if (t3.x == ecmdata.nQx[nqxmap(totrels-i)]) needs_work = &t3;
			else continue;
			if (ecmdata.totrels <= ecmdata.numrels * 2) {
				stop_reason = add_to_normalize_pool_ro (&ecmdata, needs_work->x, needs_work->z);
				if (stop_reason) goto possible_lowmem;
			}
			needs_work->x = NULL;
			needs_work->z = NULL;
		}

		free_xz (&ecmdata, &t1);
		free_xz (&ecmdata, &t2);
		free_xz (&ecmdata, &t3);
	}

	else {
		struct xz t1, t2, t3, t4, t5, t6;
		struct xz *Q1, *Q2, *Q3, *Q5, *Q6, *Q7, *Q11, *Qnext_i;
		struct {
			struct xz *Qi;
			struct xz *Qi_minus6;
			int	Qi_is_relp;
			int	Qi_minus6_is_relp;
		} Q1mod6, Q5mod6, *curr, *notcurr;
		int	have_QD;
		uint32_t totrels;

		if (!alloc_xz (&ecmdata, &t1)) goto lowmem;
		if (!alloc_xz (&ecmdata, &t2)) goto lowmem;
		if (!alloc_xz (&ecmdata, &t3)) goto lowmem;
		if (!alloc_xz (&ecmdata, &t4)) goto lowmem;
		if (!alloc_xz (&ecmdata, &t5)) goto lowmem;

		ell_dbl_xz_scr (&ecmdata, &ecmdata.QD, &t1, &t5);		
		xzswap (t1, ecmdata.QD);					
		Q1 = &t1;							
		Q2 = &ecmdata.QD;

		Q3 = &t2;
		ell_add_xz (&ecmdata, Q2, Q1, Q1, Q3);				

		Q5 = &t3;
		ell_add_xz (&ecmdata, Q3, Q2, Q1, Q5);				

		Q6 = Q3;
		ell_dbl_xz_scr (&ecmdata, Q3, Q6, &t5);				

		Q7 = &t4;
		ell_add_xz (&ecmdata, Q6, Q1, Q5, Q7);				

		Q11 = &t5;
		ell_add_xz (&ecmdata, Q6, Q5, Q1, Q11);				

		Q1mod6.Qi_minus6 = Q1;
		Q1mod6.Qi_minus6_is_relp = 1;
		Q5mod6.Qi_minus6 = Q5;
		Q5mod6.Qi_minus6_is_relp = relatively_prime (5, ecmdata.D);
		Q1mod6.Qi = Q7;
		Q1mod6.Qi_is_relp = relatively_prime (7, ecmdata.D);
		Q5mod6.Qi = Q11;
		Q5mod6.Qi_is_relp = relatively_prime (11, ecmdata.D);

		ecmdata.nQx[0] = Q1->x; ecmdata.nQx[z_offset] = Q1->z; totrels = 1;
		if (Q5mod6.Qi_minus6_is_relp) { ecmdata.nQx[totrels] = Q5->x; ecmdata.nQx[totrels+z_offset] = Q5->z; totrels++; }
		if (Q1mod6.Qi_is_relp) { ecmdata.nQx[totrels] = Q7->x; ecmdata.nQx[totrels+z_offset] = Q7->z; totrels++; }
		if (Q5mod6.Qi_is_relp) { ecmdata.nQx[totrels] = Q11->x; ecmdata.nQx[totrels+z_offset] = Q11->z; totrels++; }

#define normalize_or_free(xz,is_relp)	{ if (is_relp) { if (ecmdata.totrels <= ecmdata.numrels * 2) { \
								stop_reason = add_to_normalize_pool_ro (&ecmdata, xz->x, xz->z); \
								if (stop_reason) goto possible_lowmem; } \
							 xz->x = NULL; xz->z = NULL; is_relp = FALSE; } \
					  else free_xz (&ecmdata, xz); }

		Qnext_i = &t6;
		have_QD = FALSE;
		for (uint32_t i = 7, i_gap = 4; ; i += i_gap, i_gap = 6 - i_gap) {
			int	next_i_is_relp;

			curr = (i_gap == 4) ? &Q1mod6 : &Q5mod6;

			next_i_is_relp = (relatively_prime (i+6, ecmdata.D) && totrels < ecmdata.totrels);

			if ((next_i_is_relp && have_QD && (totrels+1 == ecmdata.totrels || totrels+1 == ecmdata.numrels * 2)) ||
			    (i+6 > ecmdata.D/2 && ecmdata.totrels == ecmdata.numrels)) {
				notcurr = (curr == &Q5mod6) ? &Q1mod6 : &Q5mod6;
				if (have_QD) normalize_or_free (notcurr->Qi, notcurr->Qi_is_relp);
				normalize_or_free (notcurr->Qi_minus6, notcurr->Qi_minus6_is_relp);
			}

			if (!alloc_xz (&ecmdata, Qnext_i)) goto lowmem;
			ell_add_xz (&ecmdata, curr->Qi, Q6, curr->Qi_minus6, Qnext_i);		

			normalize_or_free (curr->Qi_minus6, curr->Qi_minus6_is_relp);

			{
				struct xz *shuffle;
				shuffle = curr->Qi_minus6;
				curr->Qi_minus6 = curr->Qi;
				curr->Qi_minus6_is_relp = curr->Qi_is_relp;
				curr->Qi = Qnext_i;
				curr->Qi_is_relp = next_i_is_relp;
				Qnext_i = shuffle;
			}

			if ((i+6) + (i+i_gap) == ecmdata.D) {
				struct xz tmp;
				struct xz *Qgap = Q2;
				ASSERTG (Qgap == &ecmdata.QD);
				if (!alloc_xz (&ecmdata, &tmp)) goto lowmem;
				if (6 - i_gap == 4) ell_dbl_xz_scr (&ecmdata, Q2, Qgap, &tmp);		  
				ell_add_xz_scr (&ecmdata, Q1mod6.Qi, Q5mod6.Qi, Qgap, &ecmdata.QD, &tmp); 
				free_xz (&ecmdata, &tmp);
				have_QD = TRUE;
			}

			if (next_i_is_relp) {
				ecmdata.nQx[nqxmap(totrels)] = curr->Qi->x;
				ecmdata.nQx[nqxmap(totrels) + z_offset] = curr->Qi->z;
				totrels++;
			}

			if (have_QD && (totrels == ecmdata.totrels || totrels == ecmdata.numrels * 2)) break;

			if (gw_test_for_error (&ecmdata.gwdata) || gw_get_maxerr (&ecmdata.gwdata) > allowable_maxerr) goto err;
			stop_reason = stopCheck (thread_num);
			if (stop_reason) goto possible_lowmem;
		}

		normalize_or_free (Q1mod6.Qi, Q1mod6.Qi_is_relp);
		normalize_or_free (Q1mod6.Qi_minus6, Q1mod6.Qi_minus6_is_relp);
		normalize_or_free (Q5mod6.Qi, Q5mod6.Qi_is_relp);
		normalize_or_free (Q5mod6.Qi_minus6, Q5mod6.Qi_minus6_is_relp);
		free_xz (&ecmdata, Q6);
	}
	#undef nqxmap
	#undef normalize_or_free

	{
		auto it_Dmult = Dmultiple_map.find (1);
		if (it_Dmult == Dmultiple_map.end()) it_Dmult = Dmultiple_map.insert({1, {0, 1}}).first;
		it_Dmult->second.set_val_buffers (&ecmdata.gwdata, ecmdata.QD.x, ecmdata.QD.z, TRUE);
	}

	stop_reason = mQ_preinit (&ecmdata, Dmultiple_map);
	if (stop_reason) goto possible_lowmem;

	process_Dmult_map (Dmultiple_map);
	for (auto this_Dmult = Dmultiple_map.begin(); this_Dmult != Dmultiple_map.end(); ++this_Dmult) {
		
		if (this_Dmult->first == 1) continue;
		
		if (this_Dmult->second.val.x == NULL) {
			struct xz tmp;
			if (!alloc_xz (&ecmdata, &tmp)) goto lowmem;
			this_Dmult->second.set_val_buffers (&ecmdata.gwdata, tmp.x, tmp.z, FALSE);
		}
		
		auto it_base = Dmultiple_map.find (this_Dmult->second.base);
		if (this_Dmult->second.diff == 0) {
			struct xz tmp;
			if (!alloc_xz (&ecmdata, &tmp)) goto lowmem;
			ell_dbl_xz_scr (&ecmdata, &it_base->second.val, &this_Dmult->second.val, &tmp);
			free_xz (&ecmdata, &tmp);
		}
		
		else {
			auto it_addin = Dmultiple_map.find (this_Dmult->second.addin);
			auto it_diff = Dmultiple_map.find (this_Dmult->second.diff);
			ell_add_xz (&ecmdata, &it_base->second.val, &it_addin->second.val, &it_diff->second.val, &this_Dmult->second.val);
			
			it_addin->second.free_if_Dmult_last_used_by (this_Dmult->first);
			it_diff->second.free_if_Dmult_last_used_by (this_Dmult->first);
		}
		
		it_base->second.free_if_Dmult_last_used_by (this_Dmult->first);
		
		if (gw_test_for_error (&ecmdata.gwdata) || gw_get_maxerr (&ecmdata.gwdata) > allowable_maxerr) goto err;
		stop_reason = stopCheck (thread_num);
		if (stop_reason) goto possible_lowmem;
	}

	if (ecmdata.QD_Eover2.x != NULL) {
		stop_reason = add_to_normalize_pool_ro (&ecmdata, ecmdata.QD_Eover2.x, ecmdata.QD_Eover2.z);
		if (stop_reason) goto possible_lowmem;
		ecmdata.QD_Eover2.z = NULL;
	}

	gwfree (&ecmdata.gwdata, ecmdata.Ad4);
	ecmdata.Ad4 = NULL;

	gwfree_internal_memory (&ecmdata.gwdata);

	int	num_partials;
	num_partials = one_based_modulo (ecmdata.totrels, ecmdata.numrels);
	
	for (auto it_relp_set = relp_set_map.begin (); it_relp_set != relp_set_map.end (); ++it_relp_set) {
		
		if (it_relp_set->first == 0 || it_relp_set->first == -1) continue;
		
		auto it_base_relp_set = relp_set_map.find (it_relp_set->second.base);
		auto it_diff_relp_set = relp_set_map.find (it_relp_set->second.diff);
		
		auto it_Dmultiple = Dmultiple_map.find (it_relp_set->second.Dmultiple);
		
		bool	diff_access_inverted = (it_base_relp_set->first < 0) ^ (it_diff_relp_set->first < 0);
		
		for (i = (it_relp_set->second.partial ? num_partials : ecmdata.numrels) - 1; i >= 0; i--) {
			struct xz Qbase, Qdiff, Qnew;
			int	ni;

			ni = it_base_relp_set->second.nQx_store + i;
			Qbase.x = ecmdata.nQx[ni];
			Qbase.z = ecmdata.nQx[ni + z_offset];
			
			ni = it_diff_relp_set->second.nQx_store;
			if (!diff_access_inverted) ni += i; else ni += (ecmdata.numrels-1)-i;
			Qdiff.x = ecmdata.nQx[ni];
			Qdiff.z = ecmdata.nQx[ni + z_offset];

			bool Qbase_last_use = it_base_relp_set->second.is_last_use (it_relp_set->first, i, num_partials, ecmdata.numrels, FALSE);
			bool Qdiff_last_use = it_diff_relp_set->second.is_last_use (it_relp_set->first, i, num_partials, ecmdata.numrels, diff_access_inverted);
			bool Dmultiple_to_be_freed = (i == 0 && it_Dmultiple->second.will_free_due_to_relp_last_used_by (it_relp_set->first));

			if (!alloc_xz (&ecmdata, &Qnew)) goto lowmem;

			ell_add_xz (&ecmdata, &Qbase, &it_Dmultiple->second.val, &Qdiff, &Qnew);	

			ni = it_relp_set->second.nQx_store + i;
			ecmdata.nQx[ni] = Qnew.x;
			ecmdata.nQx[ni + z_offset] = Qnew.z;

			if (it_relp_set->second.is_last_use (it_relp_set->first, i, num_partials, ecmdata.numrels, FALSE)) {
				stop_reason = add_to_normalize_pool_ro (&ecmdata, Qnew.x, Qnew.z);
				if (stop_reason) goto possible_lowmem;
			}

			if (Qbase_last_use) {
				if (it_base_relp_set->second.permanent) {
					stop_reason = add_to_normalize_pool_ro (&ecmdata, Qbase.x, Qbase.z);
					if (stop_reason) goto possible_lowmem;
				} else {
					free_xz (&ecmdata, &Qbase);
				}
			}

			if (Qdiff_last_use) {
				if (it_diff_relp_set->second.permanent) {
					stop_reason = add_to_normalize_pool_ro (&ecmdata, Qdiff.x, Qdiff.z);
					if (stop_reason) goto possible_lowmem;
				} else {
					free_xz (&ecmdata, &Qdiff);
				}
			}

			if (Dmultiple_to_be_freed) {
				it_Dmultiple->second.free_if_relp_last_used_by (it_relp_set->first);
			}

			if (gw_test_for_error (&ecmdata.gwdata) || gw_get_maxerr (&ecmdata.gwdata) > allowable_maxerr) goto err;
			stop_reason = stopCheck (thread_num);
			if (stop_reason) goto possible_lowmem;
		}
	}

	Dmultiple_map.clear ();
	relp_set_map.clear ();

	stop_reason = normalize_pool (&ecmdata);
	if (stop_reason) goto possible_lowmem;

	if (ecmdata.factor != NULL) goto bingo;

	if (ecmdata.Qz_binary != NULL) {
		ASSERTG (ecmdata.Qx_binary != NULL);
		gwtogiant (&ecmdata.gwdata, ecmdata.nQx[0], ecmdata.Qx_binary);
		free (ecmdata.Qz_binary), ecmdata.Qz_binary = NULL;
	}

	stop_reason = mQ_init (&ecmdata);
	if (stop_reason) goto possible_lowmem;

	ASSERTG (ecmdata.gg == NULL);
	ecmdata.gg = gwalloc (&ecmdata.gwdata);
	if (ecmdata.gg == NULL) goto lowmem;
	if (ecmdata.gg_binary != NULL) {
		gianttogw (&ecmdata.gwdata, ecmdata.gg_binary, ecmdata.gg);
		free (ecmdata.gg_binary), ecmdata.gg_binary = NULL;
	} else
		dbltogw (&ecmdata.gwdata, 1.0, ecmdata.gg);

	stop_reason = stopCheck (thread_num);
	if (stop_reason) goto possible_lowmem;

	sprintf (buf, "%.*f%% of %s ECM curve %" PRIu32 " stage 2 (using %uMB)",
		 (int) PRECISION, trunc_percent (w->pct_complete), ecmdata.N_short_string_rep, ecmdata.curve, memused);
	title (thread_num, buf);

	end_timer (timers, 0);
	sprintf (buf, "Stage 2 init complete. %" PRIu64 " transforms, %lu modular inverses. Time: ", gw_get_fft_count (&ecmdata.gwdata), ecmdata.modinv_count);
	print_timer (timers, 0, buf, TIMER_NL | TIMER_CLR);
	OutputStr (thread_num, buf);
	gw_clear_fft_count (&ecmdata.gwdata);
	ecmdata.modinv_count = 0;

	gwnum	mQx, mQz, t1;
	ecmdata.state = ECM_STATE_STAGE2;
	start_timer_from_zero (timers, 0);
	start_timer_from_zero (timers, 1);
	last_output = last_output_t = ecmdata.modinv_count = 0;
	ecmdata.Dsection--, ecmdata.relp += ecmdata.totrels;		
	t1 = NULL;
	for ( ; ; ) {

		ecmdata.relp += next_pair (&ecmdata.pairmap_ptr);

		if (ecmdata.relp >= (int32_t) ecmdata.totrels && ecmdata.Dsection + ecmdata.relp / ecmdata.totrels >= ecmdata.numDsections) break;

		while (ecmdata.relp >= (int32_t) ecmdata.totrels) {
			ecmdata.Dsection++;
			ecmdata.relp -= ecmdata.totrels;

			gwfree (&ecmdata.gwdata, t1);			
			stop_reason = mQ_next (&ecmdata, &mQx, &mQz);
			if (stop_reason) {
				
				mQ_term (&ecmdata);
				ecm_save (&ecmdata);
				goto exit;
			}
			if (ecmdata.factor != NULL) goto bingo;

			if (!ecmdata.TWO_FFT_STAGE2) {
				t1 = gwalloc (&ecmdata.gwdata);
				if (t1 == NULL) goto oom;
			}
		}

		if (ecmdata.pairmap_ptr == ecmdata.pairmap + ecmdata.pairmap_size) {
			ASSERTG (ecmdata.relp == 0);
			ecmdata.C_done = ecmdata.B2_start + ecmdata.Dsection * ecmdata.D;
			ecmdata.first_relocatable = calc_new_first_relocatable (ecmdata.D, ecmdata.C_done);
			int fill_window = pair_window_size (ecmdata.gwdata.bit_length, ecmdata.relp_sets);
			gwfree_internal_memory (&ecmdata.gwdata);
			stop_reason = fill_pairmap (ecmdata.thread_num, &ecmdata.sieve_info, ecmdata.D, fill_window,0,0,0,
						    ecmdata.totrels, ecmdata.relp_sets+3, ecmdata.first_relocatable, ecmdata.last_relocatable,
						    ecmdata.C_done, ecmdata.C, ecmdata.max_pairmap_Dsections, &ecmdata.pairmap, &ecmdata.pairmap_size);
			if (stop_reason) {

				ecm_save (&ecmdata);
				goto exit;
			}
			ecmdata.pairmap_ptr = ecmdata.pairmap;
			ecmdata.relp = -1;
			continue;
		}

		saving = testSaveFilesFlag (thread_num);
		stop_reason = stopCheck (thread_num);

		if (ecmdata.TWO_FFT_STAGE2) {
			gwsubmul4 (&ecmdata.gwdata, mQx, ecmdata.nQx[ecmdata.relp], ecmdata.gg, ecmdata.gg, (!stop_reason && !saving) ? GWMUL_STARTNEXTFFT : 0);
		}

		else {
			gwmul3 (&ecmdata.gwdata, ecmdata.nQx[ecmdata.relp], mQz, t1, GWMUL_FFT_S12 | GWMUL_STARTNEXTFFT);
			gwsubmul4 (&ecmdata.gwdata, mQx, t1, ecmdata.gg, ecmdata.gg, (!stop_reason && !saving) ? GWMUL_STARTNEXTFFT : 0);
		}

		w->pct_complete = base_pct_complete + (double) (ecmdata.Dsection * ecmdata.totrels + ecmdata.relp) * one_relp_pct;

		if (gw_test_for_error (&ecmdata.gwdata) || gw_get_maxerr (&ecmdata.gwdata) > allowable_maxerr) goto err;

		if (first_iter_msg ||
		    (ITER_OUTPUT != 999999999 && gw_get_fft_count (&ecmdata.gwdata) >= last_output_t + 2 * ITER_OUTPUT * output_title_frequency)) {
			sprintf (buf, "%.*f%% of %s ECM curve %" PRIu32 " stage 2 (using %uMB)",
				 (int) PRECISION, trunc_percent (w->pct_complete), ecmdata.N_short_string_rep, ecmdata.curve, memused);
			title (thread_num, buf);
			last_output_t = gw_get_fft_count (&ecmdata.gwdata);
		}

		if (first_iter_msg ||
		    (ITER_OUTPUT != 999999999 && gw_get_fft_count (&ecmdata.gwdata) >= last_output + 2 * ITER_OUTPUT * output_frequency)) {
			sprintf (buf, "%s curve %" PRIu32 " stage 2 at B2=%" PRIu64 " [%.*f%%].",
				 ecmdata.N_short_string_rep, ecmdata.curve, ecmdata.B2_start + ecmdata.Dsection * ecmdata.D + ecmdata.D / 2,
				 (int) PRECISION, trunc_percent (w->pct_complete));
			end_timer (timers, 0);
			if (first_iter_msg) {
				strcat (buf, "\n");
			} else {
				strcat (buf, " Time: ");
				print_timer (timers, 0, buf, TIMER_NL);
			}
			OutputStr (thread_num, buf);
			start_timer_from_zero (timers, 0);
			last_output = gw_get_fft_count (&ecmdata.gwdata);
			first_iter_msg = FALSE;
		}

		stop_reason = stopCheck (thread_num);
		if (stop_reason || testSaveFilesFlag (thread_num)) {
			ecm_save (&ecmdata);
			if (stop_reason) goto exit;
		}
	}
	gwfree (&ecmdata.gwdata, t1);

	mQ_term (&ecmdata);
	normalize_pool_term (&ecmdata);
	ecmdata.xz.x = ecmdata.nQx[0];
	for (uint32_t i = 1; i < ecmdata.totrels; i++) gwfree (&ecmdata.gwdata, ecmdata.nQx[i]);
	free (ecmdata.nQx); ecmdata.nQx = NULL;
	free (ecmdata.pairmap); ecmdata.pairmap = NULL;

	goto stage2_done;

ecm_polymult:
	start_timer_from_zero (timers, 1);

	end_sieve (ecmdata.sieve_info), ecmdata.sieve_info = NULL;

	sprintf (buf, "Using %uMB of memory.  D: %d, degree-%" PRIu64 " polynomials.  Ftree polys in memory: %d\n",
		 memused, ecmdata.D, ecmdata.poly_size, ecmdata.Ftree_polys_in_mem);
	OutputStr (thread_num, buf);

	ecmdata.nQx = gwalloc_array (&ecmdata.gwdata, ecmdata.numrels);
	if (ecmdata.nQx == NULL) goto lowmem;

	stop_reason = normalize_pool_init (&ecmdata, ecmdata.poly_size * 2);	
	if (stop_reason) goto possible_lowmem;

	start_timer_from_zero (timers, 0);
	ASSERTG (ecmdata.D % 3 == 0);
	{
		struct xz t1, *Q1, *Q2, *Q3, *Q5, *Q6, *Q7, *Q11;
		struct {
			struct xz Qi;
			struct xz Qi_minus6;
			int	Qi_is_relp;
			int	Qi_minus6_is_relp;
		} Q1mod6, Q5mod6, *curr;
		int	i_gap, have_QD;
		uint32_t totrels = 0;

#define Qmodhi_init(Qmod,xz,is_relp)	{ xz = &(Qmod)->Qi; \
					  if (!alloc_xz (&ecmdata, xz)) goto lowmem; \
					  (Qmod)->Qi_is_relp = is_relp; \
					  if ((Qmod)->Qi_is_relp) { gwfree (&ecmdata.gwdata, xz->x); xz->x = ecmdata.nQx[totrels++]; } }
#define Qmodlo_init(Qmod,xz,is_relp)	{ xz = &(Qmod)->Qi_minus6; \
					  if (!alloc_xz (&ecmdata, xz)) goto lowmem; \
					  (Qmod)->Qi_minus6_is_relp = is_relp; \
					  if ((Qmod)->Qi_minus6_is_relp) { gwfree (&ecmdata.gwdata, xz->x); xz->x = ecmdata.nQx[totrels++]; } }
#define normalize_or_free(xz,is_relp)	{ if (is_relp) { stop_reason = add_to_normalize_pool_ro (&ecmdata, (xz)->x, (xz)->z); \
							 if (stop_reason) goto possible_lowmem; \
							 (xz)->x = NULL; (xz)->z = NULL; is_relp = FALSE; } \
					  else free_xz (&ecmdata, xz); }

		Qmodlo_init (&Q1mod6, Q1, 1);
		Qmodhi_init (&Q1mod6, Q7, relatively_prime (7, ecmdata.D));
		Qmodlo_init (&Q5mod6, Q5, relatively_prime (5, ecmdata.D));
		Qmodhi_init (&Q5mod6, Q11, relatively_prime (11, ecmdata.D));
		if (!alloc_xz (&ecmdata, &t1)) goto lowmem;
		Q6 = Q3 = &t1;

		gwcopy_xz (&ecmdata, &ecmdata.QD, Q1);
		ell_dbl_xz_scr (&ecmdata, Q1, Q2 = &ecmdata.QD, Q11);		
		ell_add_xz (&ecmdata, Q2, Q1, Q1, Q3);				
		ell_add_xz (&ecmdata, Q3, Q2, Q1, Q5);				
		ell_dbl_xz_scr (&ecmdata, Q3, Q6, Q11);				
		ell_add_xz (&ecmdata, Q6, Q1, Q5, Q7);				
		ell_add_xz (&ecmdata, Q6, Q5, Q1, Q11);				

		have_QD = FALSE;
		for (i = 7, i_gap = 4; ; i += i_gap, i_gap = 6 - i_gap) {
			struct xz Qnext_i;
			int	next_i_is_relp;

			curr = (i_gap == 4) ? &Q1mod6 : &Q5mod6;

			if (!alloc_xz (&ecmdata, &Qnext_i)) goto lowmem;
			next_i_is_relp = (relatively_prime (i+6, ecmdata.D) && totrels < ecmdata.numrels);
			if (next_i_is_relp) { gwfree (&ecmdata.gwdata, Qnext_i.x); Qnext_i.x = ecmdata.nQx[totrels++]; }

			ell_add_xz (&ecmdata, &curr->Qi, Q6, &curr->Qi_minus6, &Qnext_i);	

			normalize_or_free (&curr->Qi_minus6, curr->Qi_minus6_is_relp);

			curr->Qi_minus6 = curr->Qi;
			curr->Qi_minus6_is_relp = curr->Qi_is_relp;
			curr->Qi = Qnext_i;
			curr->Qi_is_relp = next_i_is_relp;

			if ((i+6) + (i+i_gap) == ecmdata.D) {
				struct xz tmp;
				struct xz *Qgap = Q2;
				ASSERTG (Qgap == &ecmdata.QD);
				if (!alloc_xz (&ecmdata, &tmp)) goto lowmem;
				if (6 - i_gap == 4) ell_dbl_xz_scr (&ecmdata, Q2, Qgap, &tmp);		  
				ell_add_xz_scr (&ecmdata, &Q1mod6.Qi, &Q5mod6.Qi, Qgap, &ecmdata.QD, &tmp); 
				free_xz (&ecmdata, &tmp);
				have_QD = TRUE;
			}

			if (have_QD && totrels == ecmdata.numrels) break;

			if (gw_test_for_error (&ecmdata.gwdata) || gw_get_maxerr (&ecmdata.gwdata) > allowable_maxerr) goto err;
			stop_reason = stopCheck (thread_num);
			if (stop_reason) goto possible_lowmem;
		}

		normalize_or_free (&Q1mod6.Qi, Q1mod6.Qi_is_relp);
		normalize_or_free (&Q1mod6.Qi_minus6, Q1mod6.Qi_minus6_is_relp);
		normalize_or_free (&Q5mod6.Qi, Q5mod6.Qi_is_relp);
		normalize_or_free (&Q5mod6.Qi_minus6, Q5mod6.Qi_minus6_is_relp);
		free_xz (&ecmdata, Q6);
	}
	#undef Qmodhi_init
	#undef Qmodlo_init
	#undef normalize_or_free

	{
		auto it_Dmult = Dmultiple_map.find (1);
		if (it_Dmult == Dmultiple_map.end()) it_Dmult = Dmultiple_map.insert({1, {0, 1}}).first;
		it_Dmult->second.set_val_buffers (&ecmdata.gwdata, ecmdata.QD.x, ecmdata.QD.z, TRUE);
	}

	ecmdata.polyGaux_size = divide_rounding_up (ecmdata.poly_size - 1, 7) + 6;	
	ecmdata.polyGaux = gwalloc_array (&ecmdata.gwdata, ecmdata.polyGaux_size);
	if (ecmdata.polyGaux == NULL) goto lowmem;

	stop_reason = mQ_preinit_array (&ecmdata, Dmultiple_map);
	if (stop_reason) goto possible_lowmem;

	process_Dmult_map (Dmultiple_map);
	for (auto this_Dmult = Dmultiple_map.begin(); this_Dmult != Dmultiple_map.end(); ++this_Dmult) {
		
		if (this_Dmult->first == 1) continue;
		
		if (this_Dmult->second.val.x == NULL) {
			struct xz tmp;
			if (!alloc_xz (&ecmdata, &tmp)) goto lowmem;
			this_Dmult->second.set_val_buffers (&ecmdata.gwdata, tmp.x, tmp.z, FALSE);
		}
		
		auto it_base = Dmultiple_map.find (this_Dmult->second.base);
		if (this_Dmult->second.diff == 0) {
			struct xz tmp;
			if (!alloc_xz (&ecmdata, &tmp)) goto lowmem;
			ell_dbl_xz_scr (&ecmdata, &it_base->second.val, &this_Dmult->second.val, &tmp);
			free_xz (&ecmdata, &tmp);
		}
		
		else {
			auto it_addin = Dmultiple_map.find (this_Dmult->second.addin);
			auto it_diff = Dmultiple_map.find (this_Dmult->second.diff);
			ell_add_xz (&ecmdata, &it_base->second.val, &it_addin->second.val, &it_diff->second.val, &this_Dmult->second.val);
			
			it_addin->second.free_if_Dmult_last_used_by (this_Dmult->first);
			it_diff->second.free_if_Dmult_last_used_by (this_Dmult->first);
		}
		
		it_base->second.free_if_Dmult_last_used_by (this_Dmult->first);
		
		if (gw_test_for_error (&ecmdata.gwdata) || gw_get_maxerr (&ecmdata.gwdata) > allowable_maxerr) goto err;
		stop_reason = stopCheck (thread_num);
		if (stop_reason) goto possible_lowmem;
	}
	Dmultiple_map.clear ();

	if (ecmdata.QD_Eover2.x != NULL) {
		stop_reason = add_to_normalize_pool_ro (&ecmdata, ecmdata.QD_Eover2.x, ecmdata.QD_Eover2.z);
		if (stop_reason) goto possible_lowmem;
		ecmdata.QD_Eover2.z = NULL;
	}

	gwfree (&ecmdata.gwdata, ecmdata.Ad4);
	ecmdata.Ad4 = NULL;
if (IniGetInt (INI_FILE, "PolyVerbose", 0)) {
end_timer (timers, 0);
sprintf (buf, "nQx complete.  Time: ");
print_timer (timers, 0, buf, TIMER_NL);
OutputStr (thread_num, buf); gw_clear_maxerr (&ecmdata.gwdata);
start_timer_from_zero (timers, 0);
}

	gwfree_internal_memory (&ecmdata.gwdata);

	stop_reason = mQ_init_array (&ecmdata);
	if (stop_reason) goto possible_lowmem;
	if (ecmdata.factor != NULL) goto bingo;

	if (ecmdata.Qz_binary != NULL) {
		ASSERTG (ecmdata.Qx_binary != NULL);
		gwtogiant (&ecmdata.gwdata, ecmdata.nQx[0], ecmdata.Qx_binary);
		free (ecmdata.Qz_binary), ecmdata.Qz_binary = NULL;
	}

	stop_reason = stopCheck (thread_num);
	if (stop_reason) goto possible_lowmem;

	int	fd;
	if (ecmdata.Ftree_polys_in_mem < 2) {
		fd = _open (ftree_filename, _O_CREAT | _O_TRUNC | _O_BINARY | _O_RDWR, CREATE_FILE_ACCESS);

	}

	ecmdata.polyF = gwalloc_array (&ecmdata.gwdata, ecmdata.poly_size);
	if (ecmdata.polyF == NULL) goto lowmem;

	gwset_polymult_safety_margin (&ecmdata.gwdata, polymult_safety_margin (ecmdata.poly_size, ecmdata.poly_size));

	int num_tree_levels;
	num_tree_levels = (int) ceil(log2((double)ecmdata.poly_size));	
	ASSERTG (num_tree_levels >= 4);
	for (int tree_level = 0; tree_level < num_tree_levels; tree_level++) {
		int log2_num_polys = num_tree_levels - tree_level;

		gwarray	source_poly = (tree_level == 0 ? ecmdata.nQx : ecmdata.polyF);

		if (tree_level == 0 || log2_num_polys == 3 ||
		    (ecmdata.Ftree_polys_in_mem >= 3 && tree_level >= num_tree_levels - ecmdata.Ftree_polys_in_mem + 2) ||
		    (ecmdata.Ftree_polys_in_mem >= 5 && tree_level >= num_tree_levels - ecmdata.Ftree_polys_in_mem + 1)) {

			if (ecmdata.Ftree_polys_in_mem == 0 || (ecmdata.Ftree_polys_in_mem == 1 && tree_level == 0)) {
				int array_size = (int) ceil (ecmdata.gwdata.bit_length);
				array_size = divide_rounding_up (array_size, 32);
				uint32_t *array = (uint32_t *) malloc (array_size * sizeof (uint32_t));
				if (array == NULL) goto lowmem;
				gwnum tmp = gwalloc (&ecmdata.gwdata);
				if (tmp == NULL) { free (array); goto lowmem; }
				for (int i = 0; i < ecmdata.poly_size; i++) {
					long	size;

					gwunfft (&ecmdata.gwdata, source_poly[i], tmp);
					size = gwtobinary (&ecmdata.gwdata, tmp, array, array_size);

					memset (array + size, 0, (array_size - size) * sizeof (uint32_t));
					_write (fd, array, array_size * sizeof (uint32_t));
				}
				free (array);
				gwfree (&ecmdata.gwdata, tmp);

			}

			else {
				if (tree_level == 0) {		
					ecmdata.polyFtree[0] = ecmdata.nQx;
				} else {			
					ecmdata.polyFtree[tree_level] = ecmdata.polyF;
					ecmdata.polyF = gwalloc_array (&ecmdata.gwdata, ecmdata.poly_size);
					if (ecmdata.polyF == NULL) goto lowmem;
				}
			}
		}

start_timer_from_zero (timers, 0);
		
		if (tree_level == 0) {

			ecmdata.polydata.helper_callback = &ecm_helper;
			ecmdata.polydata.helper_callback_data = &ecmdata;
			ecmdata.helper_work = ECM_POLYF_LEVEL_ZERO;
			ecmdata.poly1 = source_poly;			

			polymult_launch_helpers (&ecmdata.polydata);	

			tree_level++;
		}

		else {
			uint64_t poly_base_calculator = 0;	
			int	poly_base = 0;			
			void	*plan1 = NULL;
			void	*plan2 = NULL;
			while (poly_base < ecmdata.poly_size) {
				int	poly1_base, poly1_size, poly2_base, poly2_size;

				poly1_base = poly_base;
				poly_base_calculator += ecmdata.poly_size;
				poly2_base = (int) (poly_base_calculator >> log2_num_polys);
				poly_base_calculator += ecmdata.poly_size;
				poly_base = (int) (poly_base_calculator >> log2_num_polys);
				poly1_size = poly2_base - poly1_base;
				poly2_size = poly_base - poly2_base;

				void **plan_addr = (poly1_size == poly2_size) ? &plan1 : &plan2;
				int options = POLYMULT_INVEC1_MONIC | POLYMULT_INVEC2_MONIC | POLYMULT_NO_UNFFT | POLYMULT_SAVE_PLAN;
				if (*plan_addr != NULL) ecmdata.polydata.plan = *plan_addr, options |= POLYMULT_USE_PLAN;
				if (poly1_size <= poly2_size)
					polymult (&ecmdata.polydata, &source_poly[poly1_base], poly1_size, &source_poly[poly2_base], poly2_size,
						  &ecmdata.polyF[poly1_base], poly1_size + poly2_size, options);
				else
					polymult (&ecmdata.polydata, &source_poly[poly2_base], poly2_size, &source_poly[poly1_base], poly1_size,
						  &ecmdata.polyF[poly1_base], poly2_size + poly1_size, options);
				*plan_addr = ecmdata.polydata.plan;
			}

			free (plan1);
			free (plan2);

			poly_unfft_fft_coefficients (&ecmdata.polydata, ecmdata.polyF, ecmdata.poly_size);
		}
if (IniGetInt (INI_FILE, "PolyVerbose", 0)) {
end_timer (timers, 0);
double poly_size = ecmdata.poly_size / pow (2.0, log2_num_polys);
if (poly_size < 2.0) sprintf (buf, "Round off: %.10g, avg poly_size: %g, EB: %g, SM: %g, Time:", gw_get_maxerr (&ecmdata.gwdata), poly_size, ecmdata.gwdata.EXTRA_BITS, ecmdata.gwdata.safety_margin);
else sprintf (buf, "Round off: %.10g, avg poly_size: %g, Time: ", gw_get_maxerr (&ecmdata.gwdata), poly_size);
print_timer (timers, 0, buf, TIMER_NL);
OutputStr (thread_num, buf); gw_clear_maxerr (&ecmdata.gwdata);
start_timer_from_zero (timers, 0);
}
		stop_reason = stopCheck (thread_num);
		if (stop_reason) goto possible_lowmem;
	}

	if (ecmdata.Ftree_polys_in_mem < 2) {
		gwfree_array (&ecmdata.gwdata, ecmdata.nQx); ecmdata.nQx = NULL;
	}

	ecmdata.polyR = gwalloc_array (&ecmdata.gwdata, ecmdata.poly_size);
	if (ecmdata.polyR == NULL) goto lowmem;

	gwcopy (&ecmdata.gwdata, ecmdata.polyF[ecmdata.poly_size-1], ecmdata.polyR[ecmdata.poly_size-1]);

	ecmdata.polydata.twiddle_cache_additions_disabled = TRUE;

	for (uint64_t terms = 2; terms < ecmdata.poly_size; ) {	
		
		uint64_t terms_to_compute;
		for (terms_to_compute = ecmdata.poly_size + 1; terms_to_compute > 2 * terms; terms_to_compute = (terms_to_compute + 1) / 2);
		terms_to_compute -= terms;
		
		polymult2 (&ecmdata.polydata, &ecmdata.polyF[ecmdata.poly_size-(terms+terms_to_compute-1)], terms+terms_to_compute-1,
			   &ecmdata.polyR[ecmdata.poly_size-(terms-1)], terms-1,
			   ecmdata.polyR, terms_to_compute,		
			   NULL, 0, terms-1,				
			   POLYMULT_INVEC1_MONIC | POLYMULT_INVEC2_MONIC_NEGATE | POLYMULT_MULMID | POLYMULT_NEXTFFT);
		
		polymult (&ecmdata.polydata, &ecmdata.polyR[ecmdata.poly_size-(terms-1)], terms-1,
			  ecmdata.polyR, terms_to_compute,
			  &ecmdata.polyR[ecmdata.poly_size-(terms-1)-terms_to_compute], terms_to_compute,	
			  POLYMULT_INVEC1_MONIC_NEGATE | POLYMULT_MULHI | POLYMULT_NEXTFFT);
		
		terms = terms + terms_to_compute;
	}
if (IniGetInt (INI_FILE, "PolyVerbose", 0)) {
end_timer (timers, 0);
sprintf (buf, "PolyR built.  Time: ");
print_timer (timers, 0, buf, TIMER_NL);
OutputStr (thread_num, buf); gw_clear_maxerr (&ecmdata.gwdata);
start_timer_from_zero (timers, 0);
}

	ecmdata.polydata.twiddle_cache_additions_disabled = FALSE;

	if (IniGetInt (INI_FILE, "ECMPolyCompress", 1)) {
		int options = POLYMULT_INVEC1_MONIC;
		if (IniGetInt (INI_FILE, "ECMPolyCompress", 1) == -1) options |= POLYMULT_PRE_FFT;			
		if (IniGetInt (INI_FILE, "ECMPolyCompress", 1) == -2) options |= POLYMULT_PRE_FFT | POLYMULT_PRE_COMPRESS;
		if (IniGetInt (INI_FILE, "ECMPolyCompress", 1) == 2) options |= POLYMULT_PRE_COMPRESS;
		gwarray tmp = polymult_preprocess (&ecmdata.polydata, ecmdata.polyF, ecmdata.poly_size, ecmdata.poly_size, ecmdata.poly_size*2, options);
		gwfree_array (&ecmdata.gwdata, ecmdata.polyF);
		ecmdata.polyF = tmp;
	}

	if (IniGetInt (INI_FILE, "ECMPolyCompress", 1)) {
		
		int options = POLYMULT_INVEC1_MONIC_NEGATE;
		if (IniGetInt (INI_FILE, "ECMPolyCompress", 1) == -1) options |= POLYMULT_PRE_FFT;			
		if (IniGetInt (INI_FILE, "ECMPolyCompress", 1) == -2) options |= POLYMULT_PRE_FFT | POLYMULT_PRE_COMPRESS;
		if (IniGetInt (INI_FILE, "ECMPolyCompress", 1) == 2) options |= POLYMULT_PRE_COMPRESS;
		gwarray tmp = polymult_preprocess (&ecmdata.polydata, ecmdata.polyR, ecmdata.poly_size, ecmdata.poly_size, ecmdata.poly_size*2, options);
		gwfree_array (&ecmdata.gwdata, ecmdata.polyR);
		ecmdata.polyR = tmp;
	}
if (IniGetInt (INI_FILE, "PolyVerbose", 0)) {
end_timer (timers, 0);
sprintf (buf, "Poly compress.  Time: ");
print_timer (timers, 0, buf, TIMER_NL);
OutputStr (thread_num, buf); gw_clear_maxerr (&ecmdata.gwdata);
start_timer_from_zero (timers, 0);
}

	gwfree_internal_memory (&ecmdata.gwdata);
	mallocFreeForOS ();

	ecmdata.polyGH = gwalloc_array (&ecmdata.gwdata, 2*ecmdata.poly_size);
	if (ecmdata.polyGH == NULL) goto lowmem;
	gwnum *polyG, *polyH;
	polyG = &ecmdata.polyGH[0];
	polyH = &ecmdata.polyGH[ecmdata.poly_size];

	sprintf (buf, "%.*f%% of %s ECM curve %" PRIu32 " stage 2 (using %uMB)",
		 (int) PRECISION, trunc_percent (w->pct_complete), ecmdata.N_short_string_rep, ecmdata.curve, memused);
	title (thread_num, buf);

	end_timer (timers, 1);
	sprintf (buf, "Stage 2 init complete. %" PRIu64 " transforms, %lu modular inverses. Time: ", gw_get_fft_count (&ecmdata.gwdata), ecmdata.modinv_count);
	print_timer (timers, 1, buf, TIMER_NL | TIMER_CLR);
	OutputStr (thread_num, buf);
	gw_clear_fft_count (&ecmdata.gwdata);
	ecmdata.modinv_count = 0;

	base_pct_complete = (double) (ecmdata.B2_start - ecmdata.last_relocatable) /
			    (double) (ecmdata.B2_start + ecmdata.numDsections * ecmdata.D - ecmdata.last_relocatable);

	ecmdata.state = ECM_STATE_STAGE2;
	start_timer_from_zero (timers, 0);
	start_timer_from_zero (timers, 1);
	last_output = last_output_t = 0;
	delayed_stop_reason = 0;
	for (int outer_loop_counter = 1; ; outer_loop_counter++) {

		stop_reason = mQ_next_array (&ecmdata, polyG);
		if (stop_reason) {
			
			if (stop_reason == STOP_OUT_OF_MEM || outer_loop_counter <= 2) {
				ecm_save (&ecmdata);
				goto exit;
			}
			if (delayed_stop_reason == 0) OutputStr (thread_num, "Stopping.  Please be patient.\n");
			delayed_stop_reason = stop_reason;
			goto polydown;
		}
		if (ecmdata.factor != NULL) goto bingo;

		ecmdata.polydata.helper_callback = &ecm_helper;
		ecmdata.polydata.helper_callback_data = &ecmdata;
		ecmdata.helper_work = ECM_POLYG_LEVEL_ZERO;

		polymult_launch_helpers (&ecmdata.polydata);	

		for (int log2_num_polys = num_tree_levels - 2; log2_num_polys >= 1; log2_num_polys--) {
			uint64_t poly_base_calculator = 0;	
			int	poly_base = 0;			
			void	*plan1 = NULL;
			void	*plan2 = NULL;

			while (poly_base < ecmdata.poly_size) {
				int	poly1_base, poly1_size, poly2_base, poly2_size;

				poly1_base = poly_base;
				poly_base_calculator += ecmdata.poly_size;
				poly2_base = (int) (poly_base_calculator >> log2_num_polys);
				poly_base_calculator += ecmdata.poly_size;
				poly_base = (int) (poly_base_calculator >> log2_num_polys);
				poly1_size = poly2_base - poly1_base;
				poly2_size = poly_base - poly2_base;

				if (outer_loop_counter == 1 && log2_num_polys == 1) {
					int options = POLYMULT_INVEC1_MONIC | POLYMULT_INVEC2_MONIC | POLYMULT_NO_UNFFT;
					polymult (&ecmdata.polydata, &polyG[poly1_base], poly1_size, &polyG[poly2_base], poly2_size,
						  polyH, poly1_size + poly2_size, options);
					break;
				}
				
				void **plan_addr = (poly1_size == poly2_size) ? &plan1 : &plan2;
				int options = POLYMULT_INVEC1_MONIC | POLYMULT_INVEC2_MONIC | POLYMULT_NO_UNFFT | POLYMULT_SAVE_PLAN;
				if (*plan_addr != NULL) ecmdata.polydata.plan = *plan_addr, options |= POLYMULT_USE_PLAN;
				if (poly1_size <= poly2_size)
					polymult (&ecmdata.polydata, &polyG[poly1_base], poly1_size, &polyG[poly2_base], poly2_size,
						  &polyG[poly1_base], poly1_size + poly2_size, options);
				else
					polymult (&ecmdata.polydata, &polyG[poly2_base], poly2_size, &polyG[poly1_base], poly1_size,
						  &polyG[poly1_base], poly2_size + poly1_size, options);
				*plan_addr = ecmdata.polydata.plan;
			}

			free (plan1);
			free (plan2);

			poly_unfft_fft_coefficients (&ecmdata.polydata, outer_loop_counter > 1 || log2_num_polys > 1 ? polyG : polyH, ecmdata.poly_size);

			stop_reason = stopCheck (thread_num);
			if (stop_reason) {
				
				if (stop_reason == STOP_OUT_OF_MEM || outer_loop_counter <= 2) {
					ecm_save (&ecmdata);
					goto exit;
				}
				if (delayed_stop_reason == 0) OutputStr (thread_num, "Stopping.  Please be patient.\n");
				delayed_stop_reason = stop_reason;
				goto polydown;
			}
		}
if (IniGetInt (INI_FILE, "PolyVerbose", 0)) {
strcpy (buf, "PolyG built.  Time: ");
end_timer (timers, 0);
print_timer (timers, 0, buf, TIMER_NL);
OutputStr (thread_num, buf);
start_timer_from_zero (timers, 0);
}

		if (outer_loop_counter > 1) {
			
 			polymult (&ecmdata.polydata, polyG, ecmdata.poly_size, polyH, ecmdata.poly_size, ecmdata.polyGH, 2*ecmdata.poly_size,
				  POLYMULT_INVEC1_MONIC | (outer_loop_counter == 2 ? POLYMULT_INVEC2_MONIC : 0) | POLYMULT_NEXTFFT);

			polymult (&ecmdata.polydata, polyH, ecmdata.poly_size, ecmdata.polyR, ecmdata.poly_size, polyH, ecmdata.poly_size,
				  (outer_loop_counter == 2 ? POLYMULT_INVEC1_MONIC : 0) | POLYMULT_INVEC2_MONIC_NEGATE | POLYMULT_MULHI | POLYMULT_NEXTFFT);

			polymult_fma (&ecmdata.polydata, polyH, ecmdata.poly_size, ecmdata.polyF, ecmdata.poly_size, polyH, ecmdata.poly_size, polyG,
				      POLYMULT_INVEC2_MONIC | POLYMULT_FNMADD | POLYMULT_MULLO | POLYMULT_NEXTFFT);
if (IniGetInt (INI_FILE, "PolyVerbose", 0)) {
strcpy (buf, "PolyH built.  Time: ");
end_timer (timers, 0);
print_timer (timers, 0, buf, TIMER_NL);
OutputStr (thread_num, buf);
start_timer_from_zero (timers, 0);
}
		}

if (ecmdata.Dsection==0 && IniGetInt (INI_FILE, "PolyVerbose", 0)) {
sprintf (buf, "Round off: %.10g\n", gw_get_maxerr (&ecmdata.gwdata));
OutputStr (thread_num, buf); gw_clear_maxerr (&ecmdata.gwdata);}

		if (gw_test_for_error (&ecmdata.gwdata) || gw_get_maxerr (&ecmdata.gwdata) > allowable_maxerr) goto err;

		ecmdata.Dsection += ecmdata.poly_size;
		if (ecmdata.Dsection >= ecmdata.numDsections && ecmdata.B2_start + ecmdata.Dsection * ecmdata.D >= ecmdata.C) break;

		w->pct_complete = base_pct_complete + (1.0 - base_pct_complete) * (double) ecmdata.Dsection / (double) ecmdata.numDsections;

#define ITER_FUDGE 3
		if (first_iter_msg ||
		    (ITER_OUTPUT != 999999999 && gw_get_fft_count (&ecmdata.gwdata) * ITER_FUDGE >= last_output_t + 2 * ITER_OUTPUT * output_title_frequency)) {
			sprintf (buf, "%.*f%% of %s ECM stage 2 (using %uMB)",
				 (int) PRECISION, trunc_percent (w->pct_complete), ecmdata.N_short_string_rep, memused);
			title (thread_num, buf);
			last_output_t = gw_get_fft_count (&ecmdata.gwdata) * ITER_FUDGE;
		}

		if (first_iter_msg ||
		    (ITER_OUTPUT != 999999999 && gw_get_fft_count (&ecmdata.gwdata) * ITER_FUDGE >= last_output + 2 * ITER_OUTPUT * output_frequency)) {
			sprintf (buf, "%s stage 2 at B2=%" PRIu64 " [%.*f%%]",
				 ecmdata.N_short_string_rep, ecmdata.B2_start + ecmdata.Dsection * ecmdata.D, (int) PRECISION, trunc_percent (w->pct_complete));
			end_timer (timers, 0);
			if (first_iter_msg) {
				strcat (buf, "\n");
			} else {
				strcat (buf, ".  Time: ");
				print_timer (timers, 0, buf, TIMER_NL);
			}
			OutputStr (thread_num, buf);
			start_timer_from_zero (timers, 0);
			last_output = gw_get_fft_count (&ecmdata.gwdata) * ITER_FUDGE;
			first_iter_msg = FALSE;
		}

		if (testSaveFilesFlag (thread_num)) {
			ecm_save (&ecmdata);
			saving = FALSE;
		}

		if (delayed_stop_reason) break;
	}

polydown:
	polymult (&ecmdata.polydata, polyH, ecmdata.poly_size, ecmdata.polyR, ecmdata.poly_size,
		  polyH, ecmdata.poly_size, POLYMULT_INVEC2_MONIC_NEGATE | POLYMULT_MULHI | POLYMULT_NEXTFFT);
if (IniGetInt (INI_FILE, "PolyVerbose", 0)) {
strcpy (buf, "H(X) scaled.  Time: ");
end_timer (timers, 0);
print_timer (timers, 0, buf, TIMER_NL);
OutputStr (thread_num, buf);
start_timer_from_zero (timers, 0);
}

	gwfree_array (&ecmdata.gwdata, ecmdata.polyF), ecmdata.polyF = NULL;
	gwfree_array (&ecmdata.gwdata, ecmdata.polyR), ecmdata.polyR = NULL;
	
	uint64_t gwnums_needed;
	gwnums_needed = (num_tree_levels - 3) * divide_rounding_up (ecmdata.poly_size, 8);	
	if (gwnums_needed < 3 * ecmdata.poly_size) gwnums_needed = 3 * ecmdata.poly_size;	

	ecmdata.polyF = gwalloc_array (&ecmdata.gwdata, gwnums_needed - (ecmdata.poly_size + ecmdata.polyGaux_size));
	if (ecmdata.polyF == NULL) goto lowmem;
	ecmdata.Ftree = (gwarray) malloc ((size_t) (gwnums_needed * sizeof (gwnum)));
	if (ecmdata.Ftree == NULL) goto lowmem;

	memcpy (ecmdata.Ftree, polyG, (size_t) (ecmdata.poly_size * sizeof (gwnum)));
	memcpy (ecmdata.Ftree + ecmdata.poly_size, ecmdata.polyGaux, (size_t) (ecmdata.polyGaux_size * sizeof (gwnum)));
	memcpy (ecmdata.Ftree + ecmdata.poly_size + ecmdata.polyGaux_size, ecmdata.polyF,
		(size_t) ((gwnums_needed - (ecmdata.poly_size + ecmdata.polyGaux_size)) * sizeof (gwnum)));

	for (int pass = 1; pass <= 2; pass++) {
	    int num_levels_this_pass, log2_num_slices;
	    if (pass == 1) num_levels_this_pass = 3, log2_num_slices = 0;
	    else num_levels_this_pass = num_tree_levels - 3, log2_num_slices = 3;

	    int	slice_base = 0;			
	    for (int slice = 0; slice < (1 << log2_num_slices); slice++) {
		int	slice_size, next_slice_base;

		next_slice_base = (int) (((slice + 1) * ecmdata.poly_size) >> log2_num_slices);
		slice_size = next_slice_base - slice_base;

		for (int level = 0; level < num_levels_this_pass; level++) {
			int	tree_level = level + (pass == 1 ? num_tree_levels - 3 : 0);
			gwarray polyF = ecmdata.Ftree + level * slice_size;

			if (level == 0 || (ecmdata.Ftree_polys_in_mem >= 3 && tree_level >= num_tree_levels - ecmdata.Ftree_polys_in_mem + 2) ||
					  (ecmdata.Ftree_polys_in_mem >= 5 && tree_level >= num_tree_levels - ecmdata.Ftree_polys_in_mem + 1)) {
				
				if (ecmdata.Ftree_polys_in_mem == 0 || (ecmdata.Ftree_polys_in_mem == 1 && tree_level == 0)) {
					int	array_size;
					uint32_t *array;
					int64_t	offset;
					array_size = (int) ceil (ecmdata.gwdata.bit_length);
					array_size = divide_rounding_up (array_size, 32);
					array = (uint32_t *) malloc (array_size * sizeof (uint32_t));
					if (array == NULL) goto lowmem;
					offset = (pass == 1 ? (int64_t) ecmdata.poly_size * array_size * sizeof (uint32_t) : 0);
					_lseeki64 (fd, offset + (int64_t) slice * slice_size * array_size * sizeof (uint32_t), SEEK_SET);
					for (int i = 0; i < slice_size; i++) {
						_read (fd, array, array_size * sizeof (uint32_t));
						binarytogw (&ecmdata.gwdata, array, array_size, polyF[i]);
					}
					free (array);

				}
				
				else {

					memcpy (polyF, &ecmdata.polyFtree[tree_level][slice_base], slice_size * sizeof (gwnum));
				}
				continue;
			}

			gwarray	prev_polyF = polyF - slice_size;
			int	log2_num_polys = num_levels_this_pass - (level-1) + log2_num_slices;		
			uint64_t poly_base_calculator;	
			int	poly_base = 0;
			void	*plan1 = NULL;
			void	*plan2 = NULL;
			poly_base_calculator = ((uint64_t) slice << (log2_num_polys - log2_num_slices)) * ecmdata.poly_size;
			while (poly_base < slice_size) {
				int	poly1_base, poly1_size, poly2_base, poly2_size;

				poly1_base = poly_base;
				poly_base_calculator += ecmdata.poly_size;
				poly2_base = (int) (poly_base_calculator >> log2_num_polys) - slice_base;
				poly_base_calculator += ecmdata.poly_size;
				poly_base = (int) (poly_base_calculator >> log2_num_polys) - slice_base;
				poly1_size = poly2_base - poly1_base;
				poly2_size = poly_base - poly2_base;

				if (poly1_size == 0 || poly2_size == 0) { gwswap (polyF[poly1_base], prev_polyF[poly1_base]); continue; }

				void **plan_addr = (poly1_size == poly2_size) ? &plan1 : &plan2;
				int options = POLYMULT_INVEC1_MONIC | POLYMULT_INVEC2_MONIC | POLYMULT_NO_UNFFT | POLYMULT_SAVE_PLAN;
				if (*plan_addr != NULL) ecmdata.polydata.plan = *plan_addr, options |= POLYMULT_USE_PLAN;
				if (poly1_size <= poly2_size) {
					if (poly1_size == 1) options |= POLYMULT_INVEC1_NEGATE;
					if (poly2_size == 1) options |= POLYMULT_INVEC2_NEGATE;
					polymult (&ecmdata.polydata, &prev_polyF[poly1_base], poly1_size, &prev_polyF[poly2_base], poly2_size,
						  &polyF[poly1_base], poly1_size + poly2_size, options);
				} else {
					if (poly2_size == 1) options |= POLYMULT_INVEC1_NEGATE;
					polymult (&ecmdata.polydata, &prev_polyF[poly2_base], poly2_size, &prev_polyF[poly1_base], poly1_size,
						  &polyF[poly1_base], poly2_size + poly1_size, options);
				}
				*plan_addr = ecmdata.polydata.plan;
			}
			free (plan1);
			free (plan2);
			poly_unfft_fft_coefficients (&ecmdata.polydata, polyF, slice_size);
		}
if (IniGetInt (INI_FILE, "PolyVerbose", 0)) {
if (slice == 0) {
strcpy (buf, "PolyF up.  Time: ");
end_timer (timers, 0); timers[0] *= 1 << log2_num_slices;
print_timer (timers, 0, buf, TIMER_NL);
OutputStr (thread_num, buf);
start_timer_from_zero (timers, 0);
}
}

		for (int level = num_levels_this_pass - 1; level >= 0; level--) {
			gwarray polyF = ecmdata.Ftree + level * slice_size;
			int	log2_num_polys = num_levels_this_pass - level + log2_num_slices;		
			uint64_t poly_base_calculator;	
			int	poly_base = 0;
			void	*plan1 = NULL;
			void	*plan2 = NULL;
			void	*plan3 = NULL;
			poly_base_calculator = ((uint64_t) slice << (log2_num_polys - log2_num_slices)) * ecmdata.poly_size;
			while (poly_base < slice_size) {
				int	poly1_base, poly1_size, poly2_base, poly2_size;

				poly1_base = poly_base;
				poly_base_calculator += ecmdata.poly_size;
				poly2_base = (int) (poly_base_calculator >> log2_num_polys) - slice_base;
				poly_base_calculator += ecmdata.poly_size;
				poly_base = (int) (poly_base_calculator >> log2_num_polys) - slice_base;
				poly1_size = poly2_base - poly1_base;
				poly2_size = poly_base - poly2_base;

				if (poly1_size == 0 || poly2_size == 0) continue;

				polymult_arg a[2];		
				a[0].invec2 = &polyF[poly2_base];		
				a[0].invec2_size = poly2_size;			
				a[0].outvec = &polyH[slice_base+poly1_base];	
				a[0].outvec_size = poly1_size;			
				a[0].fmavec = NULL;
				a[0].circular_size = poly1_size + poly2_size;	
				a[0].first_mulmid = 0;
				a[0].options = (poly2_size == 1 ? POLYMULT_INVEC2_NEGATE : 0) + POLYMULT_INVEC2_MONIC | POLYMULT_CIRCULAR | POLYMULT_MULHI;

				a[1].invec2 = &polyF[poly1_base];		
				a[1].invec2_size = poly1_size;			
				a[1].outvec = &polyH[slice_base+poly2_base];	
				a[1].outvec_size = poly2_size;			
				a[1].fmavec = NULL;
				a[1].circular_size = poly1_size + poly2_size;	
				a[1].first_mulmid = 0;
				a[1].options = (poly1_size == 1 ? POLYMULT_INVEC2_NEGATE : 0) + POLYMULT_INVEC2_MONIC | POLYMULT_CIRCULAR | POLYMULT_MULHI;

				void **plan_addr = (poly1_size == poly2_size) ? &plan1 : (poly1_size < poly2_size) ? &plan2 : &plan3;
				int options = POLYMULT_NO_UNFFT | POLYMULT_SAVE_PLAN;
				if (*plan_addr != NULL) ecmdata.polydata.plan = *plan_addr, options |= POLYMULT_USE_PLAN;
				polymult_several (&ecmdata.polydata, &polyH[slice_base+poly1_base], poly1_size + poly2_size, a, 2, options);
				*plan_addr = ecmdata.polydata.plan;
			}
			
			free (plan1);
			free (plan2);
			free (plan3);
			
			if (pass == 1 || level > 0) poly_unfft_fft_coefficients (&ecmdata.polydata, &polyH[slice_base], slice_size);
		}
if (IniGetInt (INI_FILE, "PolyVerbose", 0)) {
if (slice == 0) {
strcpy (buf, "PolyF down.  Time: ");
end_timer (timers, 0); timers[0] *= 1 << log2_num_slices;
print_timer (timers, 0, buf, TIMER_NL);
OutputStr (thread_num, buf);
start_timer_from_zero (timers, 0);
}
}
		slice_base = next_slice_base;
	    }
	}

	if (ecmdata.Ftree_polys_in_mem < 2) {
		_chsize_s (fd, 0);		
		_close (fd);
		_unlink (ftree_filename);
	}
	free (ecmdata.Ftree), ecmdata.Ftree = NULL;

start_timer_from_zero (timers, 0);
	ecmdata.polydata.helper_callback = &ecm_helper;
	ecmdata.polydata.helper_callback_data = &ecmdata;
	ecmdata.helper_work = ECM_BUILD_GCDVAL;
	memset (&ecmdata.polyGH[0], 0, ecmdata.stage2_threads * sizeof (gwnum));	
	polymult_launch_helpers (&ecmdata.polydata);	

	for (int i = 1; i < ecmdata.stage2_threads; i++) {
		if (ecmdata.polyGH[i] == NULL) continue;
		if (ecmdata.polyGH[0] == NULL) ecmdata.polyGH[0] = ecmdata.polyGH[i];
		else gwmul3 (&ecmdata.gwdata, ecmdata.polyGH[0], ecmdata.polyGH[i], ecmdata.polyGH[0],
			     (i != ecmdata.stage2_threads - 1 || ecmdata.gg_binary != NULL) ? GWMUL_STARTNEXTFFT : 0);
	}

ASSERTG (ecmdata.gg == NULL);
	if (ecmdata.gg_binary != NULL) {

		gwnum tmp = gwalloc (&ecmdata.gwdata);
		if (tmp == NULL) goto oom;
		gianttogw (&ecmdata.gwdata, ecmdata.gg_binary, tmp);
		free (ecmdata.gg_binary), ecmdata.gg_binary = NULL;
		gwmul3 (&ecmdata.gwdata, ecmdata.polyGH[0], tmp, ecmdata.polyGH[0], 0);
		gwfree (&ecmdata.gwdata, tmp);
	}
	ecmdata.gg = ecmdata.polyGH[0];
if (IniGetInt (INI_FILE, "PolyVerbose", 0)) {
strcpy (buf, "gg = mul H(X).  Time: ");
end_timer (timers, 0);
print_timer (timers, 0, buf, TIMER_NL);
OutputStr (thread_num, buf);
start_timer_from_zero (timers, 0);
}

	if (ecmdata.B2_start > ecmdata.C_done) ecmdata.required_missing = map_polyD_to_first_missing_prime (ecmdata.D);
	ecmdata.C_done = ecmdata.B2_start + ecmdata.Dsection * ecmdata.D;
	ecmdata.C = ecmdata.B2_start + ecmdata.numDsections * ecmdata.D;

	if (delayed_stop_reason) {
		ecm_save (&ecmdata);
		stop_reason = delayed_stop_reason;
		goto exit;
	}

	polymult_done (&ecmdata.polydata);
	gwfree_array (&ecmdata.gwdata, ecmdata.polyF), ecmdata.polyF = NULL;
	gwfree_array (&ecmdata.gwdata, ecmdata.polyR), ecmdata.polyR = NULL;

	mQ_term_array (&ecmdata);
	normalize_pool_term (&ecmdata);

stage2_done:
	{
		double total_B2 = 0.0;
		if (ecmdata.average_B2 > 0) total_B2 = (ecmdata.curve - 1) * kruppa_adjust (ecmdata.average_B2, ecmdata.B);
		total_B2 += kruppa_adjust (ecmdata.C, ecmdata.B);
		ecmdata.average_B2 = kruppa_unadjust (total_B2 / ecmdata.curve, ecmdata.B);
	}

	gwfree_internal_memory (&ecmdata.gwdata);
	mallocFreeForOS ();
	ecm_stage1_memory_usage (thread_num, &ecmdata);					
	end_timer (timers, 1);
	end_timer (&stage2_timer, 0);
	sprintf (buf, "Stage 2 complete. %" PRIu64 " transforms, %lu modular inverses. Total time: ", gw_get_fft_count (&ecmdata.gwdata), ecmdata.modinv_count);
	print_timer (timers, 1, buf, TIMER_NL | TIMER_CLR);
	OutputStr (thread_num, buf);
	last_output = last_output_t = ecmdata.modinv_count = 0;
	gw_clear_fft_count (&ecmdata.gwdata);

	if (stage1_timer != 0.0 && stage2_timer != 0.0 && IniGetInt (INI_FILE, "EcmStage2AutoAdjust", 1)) {
		double correct_ratio = stage2_timer / stage1_timer;
		double current_adjustment = IniGetFloat (INI_FILE, "EcmStage2RatioAdjust", 1.0);
		double correct_adjustment = correct_ratio / ecmdata.est_stage2_stage1_ratio * current_adjustment;
		
		if (correct_adjustment > 3.0) correct_adjustment = 3.0;
		if (correct_adjustment < 0.333) correct_adjustment = 0.333;
		
		IniWriteFloat (INI_FILE, "EcmStage2RatioAdjust", (float) (0.9 * current_adjustment + 0.1 * correct_adjustment));
	}

	if (ERRCHK) {
		sprintf (buf, "Round off: %.10g\n", gw_get_maxerr (&ecmdata.gwdata));
		OutputStr (thread_num, buf);
		gw_clear_maxerr (&ecmdata.gwdata);
	}

restart4:
	ecmdata.state = ECM_STATE_GCD;
	sprintf (w->stage, "C%" PRIu32 "S2", ecmdata.curve);
	w->pct_complete = 1.0;
	start_timer_from_zero (timers, 0);
	stop_reason = gcd (&ecmdata.gwdata, thread_num, ecmdata.gg, ecmdata.N, &ecmdata.factor);
	if (stop_reason) {
		ecm_save (&ecmdata);
		goto exit;
	}
	end_timer (timers, 0);
	strcpy (buf, "Stage 2 GCD complete. Time: ");
	print_timer (timers, 0, buf, TIMER_NL | TIMER_CLR);
	OutputStr (thread_num, buf);
	if (ecmdata.factor != NULL) goto bingo;

more_curves:
	ecm_mini_cleanup (&ecmdata);		
	mallocFreeForOS ();
	ecmdata.curve++;
	if (w->curves_to_do == 0 || ecmdata.curve <= w->curves_to_do) {
		
		if (gwfftlen (&ecmdata.gwdata) != ecmdata.stage1_fftlen || ecmdata.stage1_threads != ecmdata.stage2_threads) {
			char	fft_desc[200];
			gwdone (&ecmdata.gwdata);
			gwinit (&ecmdata.gwdata);
			if (IniGetInt (INI_FILE, "UseLargePages", 0)) gwset_use_large_pages (&ecmdata.gwdata);
			if (HYPERTHREAD_LL) sp_info->normal_work_hyperthreading = TRUE, gwset_will_hyperthread (&ecmdata.gwdata, 2);
			gwset_bench_cores (&ecmdata.gwdata, HW_NUM_CORES);
			gwset_bench_workers (&ecmdata.gwdata, NUM_WORKERS);
			if (ERRCHK) gwset_will_error_check (&ecmdata.gwdata);
			gwset_num_threads (&ecmdata.gwdata, ecmdata.stage1_threads);
			gwset_thread_callback (&ecmdata.gwdata, SetAuxThreadPriority);
			gwset_thread_callback_data (&ecmdata.gwdata, sp_info);
			gwset_safety_margin (&ecmdata.gwdata, IniGetFloat (INI_FILE, "ExtraSafetyMargin", 0.0));
			gwset_minimum_fftlen (&ecmdata.gwdata, w->minimum_fftlen);
			gwset_using_polymult (&ecmdata.gwdata);
			gwset_use_spin_wait (&ecmdata.gwdata, IniGetInt (INI_FILE, "SpinWait", 0));
			if (w->n) res = gwsetup (&ecmdata.gwdata, w->k, w->b, w->n, w->c);
			else res = gwsetup_general_mod_giant (&ecmdata.gwdata, ecmdata.N);
			if (res) {
				sprintf (buf, "Cannot initialize FFT code, errcode=%d\n", res);
				OutputBoth (thread_num, buf);
				stop_reason = STOP_FATAL_ERROR;
				goto exit;
			}
			gwerror_checking (&ecmdata.gwdata, ERRCHK || near_fft_limit);
			gwfft_description (&ecmdata.gwdata, fft_desc);
			sprintf (msgbuf, "Switching back to %s\n", fft_desc);
			OutputStr (thread_num, msgbuf);
		} else
			gwsetaddin (&ecmdata.gwdata, 0);	
		goto restart0;
	}

no_more_curves:
	if (w->curves_to_do == 0) w->curves_to_do = ecmdata.curve - 1;

	sprintf (buf, "%s completed %u ECM %s curve%s, B1=%" PRIu64 ",%s B2=%" PRIu64 ", Wi%d: %08lX\n",
		 ecmdata.N_short_string_rep, w->curves_to_do, ecmdata.sigma_type == 3 ? "GMP-ECM param3" : ecmdata.sigma_type == 1 ? "Montgomery" : "Edwards",
		 w->curves_to_do == 1 ? "" : "s", ecmdata.B, ecmdata.optimal_B2 ? " average" : "", ecmdata.average_B2,
		 PORT, SEC5 (w->n, ecmdata.B, ecmdata.average_B2));
	OutputStr (thread_num, buf);
	formatMsgForResultsFile (buf, w);
	writeResults (buf);

	strcpy (JSONbuf, "{\"status\":\"NF\"");
	if (w->n) JSONaddExponent (JSONbuf, w);
	else if (strlen (ecmdata.N_full_string_rep) < 2000) sprintf (JSONbuf+strlen(JSONbuf), ", \"N\":%s", ecmdata.N_full_string_rep);
	else sprintf (JSONbuf+strlen(JSONbuf), ", \"N\":\"%s\"", ecmdata.N_short_string_rep);
	strcat (JSONbuf, ", \"worktype\":\"ECM\"");
	sprintf (JSONbuf+strlen(JSONbuf), ", \"b1\":%" PRIu64 ", \"b2\":%" PRIu64, ecmdata.B, ecmdata.average_B2);
	sprintf (JSONbuf+strlen(JSONbuf), ", \"curves\":%u", w->curves_to_do);
	if (ecmdata.sigma_type == 3) sprintf (JSONbuf+strlen(JSONbuf), ", \"GMP-ECM-param3\":{}");
	if (ecmdata.sigma_type == 0) sprintf (JSONbuf+strlen(JSONbuf), ", \"Edwards\":{}");
	sprintf (JSONbuf+strlen(JSONbuf), ", \"fft-length\":%lu", ecmdata.stage1_fftlen);
	sprintf (JSONbuf+strlen(JSONbuf), ", \"security-code\":\"%08lX\"", SEC5 (w->n, ecmdata.B, ecmdata.average_B2));
	JSONaddProgramTimestamp (JSONbuf);
	JSONaddExponentKnownFactors (JSONbuf, w);
	JSONaddUserComputerAID (JSONbuf, w);
	strcat (JSONbuf, "}");
	if (IniGetInt (INI_FILE, "OutputJSON", 1)) writeResultsJSON (JSONbuf);

	if (!QA_IN_PROGRESS && (ecmdata.B >= 50000 || IniGetInt (INI_FILE, "SendAllFactorData", 0))) {
		struct primenetAssignmentResult pkt;
		memset (&pkt, 0, sizeof (pkt));
		strcpy (pkt.computer_guid, COMPUTER_GUID);
		strcpy (pkt.assignment_uid, w->assignment_uid);
		truncated_strcpy (pkt.message, sizeof (pkt.message), buf);
		pkt.result_type = PRIMENET_AR_ECM_NOFACTOR;
		pkt.k = w->k;
		pkt.b = w->b;
		pkt.n = w->n;
		pkt.c = w->c;
		pkt.B1 = ecmdata.B;
		pkt.B2 = ecmdata.average_B2;
		pkt.curves = w->curves_to_do;
		pkt.fftlen = ecmdata.stage1_fftlen;
		pkt.done = TRUE;
		strcpy (pkt.JSONmessage, JSONbuf);
		spoolMessage (PRIMENET_ASSIGNMENT_RESULT, &pkt);
	}

	unlinkSaveFiles (&ecmdata.write_save_file_state);

	stop_reason = STOP_WORK_UNIT_COMPLETE;
exit:	Dmultiple_map.clear ();
	relp_set_map.clear ();
	ecm_cleanup (&ecmdata);
	free (str);
	free (msg);
	return (stop_reason);

lowmem:	stop_reason = OutOfMemory (thread_num);
possible_lowmem:
	if (ecmdata.state == ECM_STATE_MIDSTAGE) ecm_save (&ecmdata);
	if (stop_reason != STOP_OUT_OF_MEM) goto exit;
	Dmultiple_map.clear ();
	relp_set_map.clear ();
	ecm_cleanup (&ecmdata);
	OutputBoth (thread_num, "Memory allocation error.  Trying again using less memory.\n");
	ecmdata.pct_mem_to_use *= 0.8;
	goto restart;

oom:	stop_reason = OutOfMemory (thread_num);
	goto exit;

bingo:	stage = (ecmdata.state > ECM_STATE_MIDSTAGE) ? 2 : (ecmdata.state > ECM_STATE_STAGE1_INIT) ? 1 : 0;
	sprintf (buf, "ECM found a factor in curve #%" PRIu32 ", stage #%d\n", ecmdata.curve, stage);
	writeResults (buf);
	sprintf (buf, "Sigma=%" PRIu64 ", B1=%" PRIu64 ", B2=%" PRIu64 ".\n", ecmdata.sigma, ecmdata.B, ecmdata.C);
	writeResults (buf);

	msglen = ecmdata.factor->sign * 10 + (int) strlen (ecmdata.N_full_string_rep) + 400;
	str = (char *) malloc (msglen);
	if (str == NULL) goto oom;
	msg = (char *) malloc (msglen);
	if (msg == NULL) goto oom;
	gtoc (ecmdata.factor, str, msglen);

	if (!testFactor (&ecmdata.gwdata, w, ecmdata.factor)) {
		sprintf (msg, "ERROR: Bad factor for %s found: %s\n", ecmdata.N_short_string_rep, str);
		OutputBoth (thread_num, msg);
		OutputStr (thread_num, "Restarting ECM curve from scratch.\n");
		continueECM = TRUE;
		ecmdata.curve--;
		goto error_restart;
	}

	sprintf (msg, "%s has a factor: %s (ECM curve %" PRIu32 ", B1=%" PRIu64 ", B2=%" PRIu64 ")\n",
		 ecmdata.N_full_string_rep, str, ecmdata.curve, ecmdata.B, ecmdata.C);
	OutputBoth (thread_num, msg);

	strcpy (JSONbuf, "{\"status\":\"F\"");
	if (w->n) JSONaddExponent (JSONbuf, w);
	else if (strlen (ecmdata.N_full_string_rep) < 2000) sprintf (JSONbuf+strlen(JSONbuf), ", \"N\":%s", ecmdata.N_full_string_rep);
	else sprintf (JSONbuf+strlen(JSONbuf), ", \"N\":\"%s\"", ecmdata.N_short_string_rep);
	strcat (JSONbuf, ", \"worktype\":\"ECM\"");
	sprintf (JSONbuf+strlen(JSONbuf), ", \"factors\":[\"%s\"]", str);
	sprintf (JSONbuf+strlen(JSONbuf), ", \"b1\":%" PRIu64 ", \"b2\":%" PRIu64, ecmdata.B, ecmdata.C);
	if (ecmdata.sigma_type == 3) sprintf (JSONbuf+strlen(JSONbuf), ", \"GMP-ECM-param3\":{\"sigma\":%" PRIu64 "}", ecmdata.sigma);
	else if (ecmdata.sigma_type == 0) sprintf (JSONbuf+strlen(JSONbuf), ", \"Edwards\":{\"sigma\":%" PRIu64 "}", ecmdata.sigma);
	else sprintf (JSONbuf+strlen(JSONbuf), ", \"sigma\":%" PRIu64, ecmdata.sigma);
	sprintf (JSONbuf+strlen(JSONbuf), ", \"stage\":%d", stage);
	sprintf (JSONbuf+strlen(JSONbuf), ", \"curves\":%" PRIu32, ecmdata.curve);
	if (ecmdata.optimal_B2 && ecmdata.average_B2 != ecmdata.C) sprintf (JSONbuf+strlen(JSONbuf), ", \"average-b2\":%" PRIu64, ecmdata.average_B2);
	sprintf (JSONbuf+strlen(JSONbuf), ", \"fft-length\":%lu", ecmdata.stage1_fftlen);
	sprintf (JSONbuf+strlen(JSONbuf), ", \"security-code\":\"%08lX\"", SEC5 (w->n, ecmdata.B, ecmdata.C));
	JSONaddProgramTimestamp (JSONbuf);
	JSONaddExponentKnownFactors (JSONbuf, w);
	JSONaddUserComputerAID (JSONbuf, w);
	strcat (JSONbuf, "}");
	if (IniGetInt (INI_FILE, "OutputJSON", 1)) writeResultsJSON (JSONbuf);

	continueECM = IniGetInt (INI_FILE, "ContinueECM", 0);
	if (ecmdata.curve == w->curves_to_do) continueECM = FALSE;
	prpAfterEcmFactor = IniGetInt (INI_FILE, "PRPAfterECMFactor", bitlen (ecmdata.N) < 100000 && w->n && (w->k != 1.0 || w->b != 2 || w->c != -1));
	if (prpAfterEcmFactor || continueECM) divg (ecmdata.factor, ecmdata.N);
	if (prpAfterEcmFactor && isProbablePrime (&ecmdata.gwdata, ecmdata.N)) {
		OutputBoth (thread_num, "Cofactor is a probable prime!\n");
		continueECM = FALSE;
	}

	if (!QA_IN_PROGRESS && (strlen (str) >= 21 || IniGetInt (INI_FILE, "SendAllFactorData", 0))) {
		struct primenetAssignmentResult pkt;
		memset (&pkt, 0, sizeof (pkt));
		strcpy (pkt.computer_guid, COMPUTER_GUID);
		strcpy (pkt.assignment_uid, w->assignment_uid);
		truncated_strcpy (pkt.message, sizeof (pkt.message), msg);
		pkt.result_type = PRIMENET_AR_ECM_FACTOR;
		pkt.k = w->k;
		pkt.b = w->b;
		pkt.n = w->n;
		pkt.c = w->c;
		truncated_strcpy (pkt.factor, sizeof (pkt.factor), str);
		pkt.B1 = ecmdata.B;
		pkt.B2 = ecmdata.average_B2;
		pkt.curves = ecmdata.curve;
		pkt.stage = stage;
		pkt.fftlen = ecmdata.stage1_fftlen;
		pkt.done = !continueECM;
		strcpy (pkt.JSONmessage, JSONbuf);
		spoolMessage (PRIMENET_ASSIGNMENT_RESULT, &pkt);
	}

	if (continueECM) {
		w->curves_to_do -= ecmdata.curve;
		char *new_known_factors = addNewKnownFactor (w, str);
		free (w->known_factors);
		w->known_factors = new_known_factors;
		stop_reason = updateWorkToDoLine (thread_num, w);
		if (stop_reason) goto exit;
		unlinkSaveFiles (&ecmdata.write_save_file_state);
		ecmdata.curve = 0;
		ecmdata.average_B2 = 0;
	}

	if (USE_PRIMENET && w->k == 1.0 && w->b == 2 && w->c == -1 && strlen (str) >= 21 && (int) w->n < IniGetInt (INI_FILE, "MaxAutoPRPExponent", 5000000)) {
		struct work_unit w_prp;
		generatePRPWorkUnit (w, continueECM ? NULL : str, &w_prp);
		
		w_prp.next = w;
		stop_reason = addWorkToDoLine (thread_num, &w_prp, continueECM ? ADD_BEFORE_SPECIFIC : ADD_AFTER_SPECIFIC);
		if (stop_reason) goto exit;
		
		if (continueECM) {
			stop_reason = STOP_PREV_WORK_UNIT;
			goto exit;
		}
	}

	free (str); str = NULL;
	free (msg); msg = NULL;
	free (ecmdata.factor); ecmdata.factor = NULL;

	clear_timer (timers, 0);

	if (!continueECM) {
		unlinkSaveFiles (&ecmdata.write_save_file_state);
		stop_reason = STOP_WORK_UNIT_COMPLETE;
		invalidateNextRollingAverageUpdate ();
		goto exit;
	}

	goto more_curves;

ed_err:	OutputBoth (thread_num, "Hardware error?  Point no longer on the Edwards curve.  Restarting from last save file.\n");
	goto error_restart;

err:	if (gw_get_maxerr (&ecmdata.gwdata) > allowable_maxerr) {
		sprintf (buf, "Possible roundoff error (%.8g), backtracking to last save file and using larger FFT.\n", gw_get_maxerr (&ecmdata.gwdata));
		OutputStr (thread_num, buf);
		maxerr_restart_count++;
		maxerr_fftlen = gwfftlen (&ecmdata.gwdata);
	} else {
		OutputBoth (thread_num, "SUMOUT error occurred.\n");
		stop_reason = SleepFive (thread_num);
		if (stop_reason) goto exit;
	}

error_restart:
	Dmultiple_map.clear ();
	relp_set_map.clear ();
	ecm_cleanup (&ecmdata);
	free (str); str = NULL;
	free (msg); msg = NULL;
	goto restart;
}

int ecm_QA (
	int	thread_num,
	struct PriorityInfo *sp_info)	
{
	FILE	*fd;

	title (thread_num, "QA");

	fd = fopen ("qa_ecm", "r");
	if (fd == NULL) {
		OutputStr (thread_num, "File named 'qa_ecm' could not be opened.\n");
		return (STOP_FILE_IO_ERROR);
	}

	QA_TYPE = 0;
	for ( ; ; ) {
		struct work_unit w;
		double	k;
		unsigned long b, n, B1, B2;
		signed long c;
		char	fac_str[80];
		uint64_t sigma;
		int	stop_reason;

		n = 0;
		(void) fscanf (fd, "%lf,%lu,%lu,%ld,%" PRIu64 ",%lu,%lu,%s\n", &k, &b, &n, &c, &sigma, &B1, &B2, fac_str);
		if (n == 0) break;

		if (b == 1) {
			QA_TYPE = c;
			continue;
		}

		QA_FACTOR = allocgiant ((int) strlen (fac_str));
		ctog (fac_str, QA_FACTOR);

		memset (&w, 0, sizeof (w));
		w.work_type = WORK_ECM;
		w.k = k;
		w.b = b;
		w.n = n;
		w.c = c;
		w.B1 = B1;
		w.B2 = B2;
		w.curves_to_do = 1;
		w.curve = sigma;
		QA_IN_PROGRESS = TRUE;
		stop_reason = ecm (0, sp_info, &w);
		QA_IN_PROGRESS = FALSE;
		free (QA_FACTOR);
		if (stop_reason != STOP_WORK_UNIT_COMPLETE) {
			fclose (fd);
			return (stop_reason);
		}
	}

	fclose (fd);
	return (0);
}

#define luc_dbl(h,s,d)			gwsquare2 (h, s, d, GWMUL_FFT_S1 | GWMUL_ADDINCONST | GWMUL_STARTNEXTFFT)
#define luc_add(h,s1,s2,diff,d)		gwmulsub4 (h, s1, s2, diff, d, GWMUL_FFT_S12 | GWMUL_STARTNEXTFFT)
#define luc_add_last(h,s1,s2,diff,d)	gwmulsub4 (h, s1, s2, diff, d, GWMUL_FFT_S12)

#define PM1_STATE_STAGE0	0	
#define PM1_STATE_STAGE1	1	
#define PM1_STATE_MIDSTAGE	2	
#define PM1_STATE_STAGE2	3	
#define PM1_STATE_GCD		4	
#define PM1_STATE_DONE		5	

#define PM1_STAGE2_PAIRING	0	
#define PM1_STAGE2_POLYMULT	1	

typedef struct {
	gwhandle gwdata;	
	int	thread_num;	
	int	stage1_threads;	
	int	stage2_threads;	
	struct work_unit *w;	
	int	state;		
	uint64_t B;		
	uint64_t C;		
	uint64_t interim_B;	
	uint64_t interim_C;	
	uint64_t B_done;	
	uint64_t C_done;	
	uint64_t first_C_start;	
	int	optimal_B2;	
	uint64_t max_stage0_prime; 
	uint64_t stage0_bitnum;	
	readSaveFileState read_save_file_state;	
	writeSaveFileState write_save_file_state; 
	void	*sieve_info;	
	uint64_t stage1_prime;	
	unsigned long stage1_fftlen; 

	gwnum	x;		
	gwnum	gg;		

	gwnum	invx;		
	giant	x_binary;	
	giant	invx_binary;	
	giant	gg_binary;	
	int	stage2_type;	
	uint64_t stage2_numvals; 
	int	D;		
	int	numrels;	
	gwnum	*nQx;		
	double	est_stage2_stage1_ratio; 
	double	pct_mem_to_use;	
	uint64_t B2_start;	
	uint64_t numDsections;	
	uint64_t Dsection;	
	uint64_t first_relocatable; 
	uint64_t last_relocatable; 

	gwnum	V;		
	gwnum	Vn;		
	gwnum	Vn1;		
	int	totrels;	
	uint64_t max_pairmap_Dsections;	
	uint8_t	*pairmap;	
	uint64_t pairmap_size;	
	uint8_t *pairmap_ptr;	
	int16_t relp_sets[32];	
	int	relp;		

	int32_t	required_missing; 
				 
	pmhandle polydata;	
	uint64_t poly1_size;	
	uint64_t poly2_size;	
	uint64_t num_points;	
	gwnum	diff1;		
	gwnum	r_squared;	
	gwnum	r_2helper;	
	gwnum	r_2helper2;	
	int	helper_work;	
	int	helper_count;	
	gwnum	*poly2;		
	uint64_t remaining_poly2_size; 
	gwnum	*points;	
} pm1handle;

void pm1_cleanup (
	pm1handle *pm1data)
{

	free (pm1data->x_binary), pm1data->x_binary = NULL;
	free (pm1data->invx_binary), pm1data->invx_binary = NULL;
	free (pm1data->gg_binary), pm1data->gg_binary = NULL;
	free (pm1data->nQx), pm1data->nQx = NULL;
	free (pm1data->pairmap), pm1data->pairmap = NULL;
	polymult_done (&pm1data->polydata);
	gwdone (&pm1data->gwdata);
	pm1data->x = NULL;
	pm1data->gg = NULL;
	end_sieve (pm1data->sieve_info), pm1data->sieve_info = NULL;
}

int pm1_modinv (
	pm1handle *pm1data,
	gwnum	b,
	giant	N,
	giant	&factor)
{
	giant	v;

	v = popg (&pm1data->gwdata.gdata, ((int) pm1data->gwdata.bit_length >> 5) + 10);
	if (v == NULL) goto oom;
	if (gwtogiant (&pm1data->gwdata, b, v)) {
		
		goto oom;
	}

	{
	mpz_t	__v, __N, __gcd, __inv;

	mpz_init (__v);
	mpz_init (__N);
	mpz_init (__gcd);
	mpz_init (__inv);
	gtompz (v, __v);
	gtompz (N, __N);
	mpz_gcdext (__gcd, __inv, NULL, __v, __N);
	mpz_clear (__v);

	if (mpz_cmp_ui (__gcd, 1) && mpz_cmp (__gcd, __N)) {
		factor = allocgiant ((int) divide_rounding_up (mpz_sizeinbase (__gcd, 2), 32));
		if (factor == NULL) goto oom;
		mpztog (__gcd, factor);
	}

	else {
		factor = NULL;
		if (mpz_sgn (__inv) < 0) mpz_add (__inv, __inv, __N);
		mpztog (__inv, v);
		gianttogw (&pm1data->gwdata, v, b);
	}

	mpz_clear (__gcd);
	mpz_clear (__inv);
	mpz_clear (__N);
	}

	pushg (&pm1data->gwdata.gdata, 1);

	return (0);

oom:	return (OutOfMemory (pm1data->thread_num));
}

#define PM1_MAGICNUM	0x317a394b

#define PM1_VERSION	8				

void pm1_save (
	pm1handle *pm1data)
{
	int	fd;
	struct work_unit *w = pm1data->w;
	uint32_t sum = 0;

	fd = openWriteSaveFile (&pm1data->write_save_file_state);
	if (fd < 0) return;

	if (!write_header (fd, PM1_MAGICNUM, PM1_VERSION, w)) goto writeerr;

	if (! write_int (fd, pm1data->state, &sum)) goto writeerr;

	if (pm1data->state == PM1_STATE_STAGE0) {
		if (! write_uint64 (fd, pm1data->interim_B, &sum)) goto writeerr;
		if (! write_uint64 (fd, pm1data->max_stage0_prime, &sum)) goto writeerr;
		if (! write_uint64 (fd, pm1data->stage0_bitnum, &sum)) goto writeerr;
	}

	else if (pm1data->state == PM1_STATE_STAGE1) {
		if (! write_uint64 (fd, pm1data->B_done, &sum)) goto writeerr;
		if (! write_uint64 (fd, pm1data->interim_B, &sum)) goto writeerr;
		if (! write_uint64 (fd, pm1data->stage1_prime, &sum)) goto writeerr;
	}

	else if (pm1data->state == PM1_STATE_MIDSTAGE) {
		if (! write_uint64 (fd, pm1data->B_done, &sum)) goto writeerr;
		if (! write_uint64 (fd, pm1data->C_done, &sum)) goto writeerr;
	}

	else if (pm1data->state == PM1_STATE_STAGE2) {
		uint64_t remaining_pairmap_size;
		if (! write_uint64 (fd, pm1data->B_done, &sum)) goto writeerr;
		if (! write_uint64 (fd, pm1data->C_done, &sum)) goto writeerr;
		if (! write_uint64 (fd, pm1data->interim_C, &sum)) goto writeerr;
		if (! write_int (fd, pm1data->stage2_type, &sum)) goto writeerr;

		if (! write_int (fd, pm1data->D, &sum)) goto writeerr;
		if (! write_int (fd, pm1data->numrels, &sum)) goto writeerr;
		if (! write_uint64 (fd, pm1data->B2_start, &sum)) goto writeerr;
		if (! write_uint64 (fd, pm1data->numDsections, &sum)) goto writeerr;
		if (! write_uint64 (fd, pm1data->Dsection, &sum)) goto writeerr;
		if (! write_uint64 (fd, pm1data->first_relocatable, &sum)) goto writeerr;
		if (! write_uint64 (fd, pm1data->last_relocatable, &sum)) goto writeerr;
		if (pm1data->stage2_type == PM1_STAGE2_PAIRING) {
			if (! write_int (fd, (int) pm1data->stage2_numvals, &sum)) goto writeerr;
			if (! write_int (fd, pm1data->totrels, &sum)) goto writeerr;
			if (! write_uint64 (fd, pm1data->max_pairmap_Dsections, &sum)) goto writeerr;
			if (! write_int (fd, pm1data->relp, &sum)) goto writeerr;
			if (! write_array (fd, (char *) pm1data->relp_sets, 32 * sizeof (int16_t), &sum)) goto writeerr;
			
			remaining_pairmap_size = pm1data->pairmap_size - (pm1data->pairmap_ptr - pm1data->pairmap);
			if (! write_uint64 (fd, remaining_pairmap_size, &sum)) goto writeerr;
			if (! write_array (fd, (char *) pm1data->pairmap_ptr, (size_t) remaining_pairmap_size, &sum)) goto writeerr;
		} else {
			if (! write_int32 (fd, pm1data->required_missing, &sum)) goto writeerr;
		}
	}

	else if (pm1data->state == PM1_STATE_GCD) {
		if (! write_uint64 (fd, pm1data->B_done, &sum)) goto writeerr;
		if (! write_uint64 (fd, pm1data->C_done, &sum)) goto writeerr;
	}

	else if (pm1data->state == PM1_STATE_DONE) {
		if (! write_uint64 (fd, pm1data->B_done, &sum)) goto writeerr;
		if (! write_uint64 (fd, pm1data->C_done, &sum)) goto writeerr;
	}

	{
		int have_x = TRUE;
		if (! write_int (fd, have_x, &sum)) goto writeerr;
		if (pm1data->x_binary != NULL) { if (! write_giant (fd, pm1data->x_binary, &sum)) goto writeerr; }
		else { if (! write_gwnum (fd, &pm1data->gwdata, pm1data->x, &sum)) goto writeerr; }
	}

	if (pm1data->state >= PM1_STATE_MIDSTAGE && pm1data->state <= PM1_STATE_STAGE2) {
		if (! write_giant (fd, pm1data->invx_binary, &sum)) goto writeerr;
	}

	if (pm1data->state >= PM1_STATE_MIDSTAGE && pm1data->state <= PM1_STATE_GCD) {
		int have_gg = (pm1data->gg != NULL || pm1data->gg_binary != NULL);
		if (! write_int (fd, have_gg, &sum)) goto writeerr;
		if (have_gg && pm1data->gg_binary != NULL && !write_giant (fd, pm1data->gg_binary, &sum)) goto writeerr;
		if (have_gg && pm1data->gg != NULL && !write_gwnum (fd, &pm1data->gwdata, pm1data->gg, &sum)) goto writeerr;
	}

	gwfree_cached (&pm1data->gwdata);
	mallocFreeForOS ();

	if (! write_checksum (fd, sum)) goto writeerr;

	closeWriteSaveFile (&pm1data->write_save_file_state, fd);
	return;

writeerr:
	deleteWriteSaveFile (&pm1data->write_save_file_state, fd);
}

int pm1_old_restore (
	pm1handle *pm1data,
	int	fd,
	unsigned long version,
	uint32_t filesum)
{
	uint32_t sum = 0;
	unsigned long state, bitarray_len, unused;
	uint64_t processed, B_done, B, C_done, unused64;

	if (! read_long (fd, &state, &sum)) goto readerr;
	if (version == 2) pm1data->max_stage0_prime = 13333333;		
	else {
		unsigned long tmp;
		if (! read_long (fd, &tmp, &sum)) goto readerr;
		pm1data->max_stage0_prime = tmp;
	}
	if (! read_uint64 (fd, &B_done, &sum)) goto readerr;
	if (! read_uint64 (fd, &B, &sum)) goto readerr;
	if (! read_uint64 (fd, &C_done, &sum)) goto readerr;
	if (! read_uint64 (fd, &unused64, &sum)) goto readerr;		
	if (! read_uint64 (fd, &unused64, &sum)) goto readerr;		
	if (! read_uint64 (fd, &processed, &sum)) goto readerr;
	if (! read_long (fd, &unused, &sum)) goto readerr;		
	if (! read_long (fd, &unused, &sum)) goto readerr;		
	if (! read_long (fd, &unused, &sum)) goto readerr;		
	if (! read_long (fd, &bitarray_len, &sum)) goto readerr;
	if (bitarray_len) {
		char *bitarray = (char *) malloc (bitarray_len);
		if (bitarray == NULL) goto readerr;
		if (! read_array (fd, bitarray, (size_t) bitarray_len, &sum)) goto readerr;
		free (bitarray);
	}
	if (! read_uint64 (fd, &unused64, &sum)) goto readerr;		
	if (! read_long (fd, &unused, &sum)) goto readerr;		
	if (! read_long (fd, &unused, &sum)) goto readerr;		

	if (state == 3) {				
		pm1data->state = PM1_STATE_STAGE0;
		pm1data->interim_B = B;
		pm1data->stage0_bitnum = (unsigned long) processed;
		
		if (version == 1) {
			OutputBoth (pm1data->thread_num, "P-1 save file incompatible with this program version.  Restarting stage 1 from the beginning.\n");
			goto readerr;
		}
	} else if (state == 0) {			
		pm1data->state = PM1_STATE_STAGE1;
		pm1data->B_done = B_done;			
		pm1data->interim_B = B;
		pm1data->stage1_prime = processed;
	} else if (state == 1) {			
		pm1data->state = PM1_STATE_STAGE1;
		pm1data->stage1_prime = B_done;
		pm1data->interim_B = B_done;
		pm1data->B_done = B_done;
		pm1data->C_done = B_done;
		
		OutputBoth (pm1data->thread_num, "Cannot continue stage 2 from old P-1 save file.  Restarting stage 2 from the beginning.\n");
	} else if (state == 2) {			
		pm1data->state = PM1_STATE_DONE;
		pm1data->B_done = B_done;
		pm1data->C_done = C_done;
	}

	pm1data->x = gwalloc (&pm1data->gwdata);
	if (pm1data->x == NULL) goto readerr;
	if (! read_gwnum (fd, &pm1data->gwdata, pm1data->x, &sum)) goto readerr;

	pm1data->gg = NULL;
	if (state == 1) {
		pm1data->gg = gwalloc (&pm1data->gwdata);
		if (pm1data->gg == NULL) goto readerr;
		if (! read_gwnum (fd, &pm1data->gwdata, pm1data->gg, &sum)) goto readerr;
	}

	if (filesum != sum) goto readerr;
	_close (fd);

	return (TRUE);

readerr:
	_close (fd);
	return (FALSE);
}

int pm1_restore (			
	pm1handle *pm1data)
{
	int	fd;
	struct work_unit *w = pm1data->w;
	uint32_t version, sum = 0, filesum;

	fd = _open (pm1data->read_save_file_state.current_filename, _O_BINARY | _O_RDONLY);
	if (fd < 0) goto err;

	if (! read_magicnum (fd, PM1_MAGICNUM)) goto readerr;
	if (! read_header (fd, &version, w, &filesum)) goto readerr;
	if (version < 1 || version > PM1_VERSION) goto readerr;
	if (version < 4) return (pm1_old_restore (pm1data, fd, version, filesum));

	if (! read_int (fd, &pm1data->state, &sum)) goto readerr;

	if (pm1data->state == PM1_STATE_STAGE0) {
		if (! read_uint64 (fd, &pm1data->interim_B, &sum)) goto readerr;
		if (version <= 5) {
			unsigned long tmp;
			if (! read_long (fd, &tmp, &sum)) goto readerr;
			pm1data->max_stage0_prime = tmp;
			if (! read_long (fd, &tmp, &sum)) goto readerr;
			pm1data->stage0_bitnum = tmp;
		} else {
			if (! read_uint64 (fd, &pm1data->max_stage0_prime, &sum)) goto readerr;
			if (! read_uint64 (fd, &pm1data->stage0_bitnum, &sum)) goto readerr;
		}
	}

	else if (pm1data->state == PM1_STATE_STAGE1) {
		if (! read_uint64 (fd, &pm1data->B_done, &sum)) goto readerr;
		if (! read_uint64 (fd, &pm1data->interim_B, &sum)) goto readerr;
		if (! read_uint64 (fd, &pm1data->stage1_prime, &sum)) goto readerr;
		
		if (pm1data->interim_B < pm1data->B_done) {
			pm1data->state = PM1_STATE_DONE;
			if (pm1data->C_done < pm1data->B) pm1data->C_done = pm1data->B;
		}
	}

	else if (pm1data->state == PM1_STATE_MIDSTAGE) {
		if (version <= 6) {
			OutputBoth (pm1data->thread_num, "P-1 save file incompatible with this program version.  Restarting stage 1 from the beginning.\n");
			goto readerr;
		}
		if (! read_uint64 (fd, &pm1data->B_done, &sum)) goto readerr;
		if (! read_uint64 (fd, &pm1data->C_done, &sum)) goto readerr;
	}

	else if (pm1data->state == PM1_STATE_STAGE2) {
		if (version <= 6) {
			OutputBoth (pm1data->thread_num, "P-1 save file incompatible with this program version.  Restarting stage 1 from the beginning.\n");
			goto readerr;
		}
		if (! read_uint64 (fd, &pm1data->B_done, &sum)) goto readerr;
		if (! read_uint64 (fd, &pm1data->C_done, &sum)) goto readerr;
		if (! read_uint64 (fd, &pm1data->interim_C, &sum)) goto readerr;
		if (! read_int (fd, &pm1data->stage2_type, &sum)) goto readerr;
		if (! read_int (fd, &pm1data->D, &sum)) goto readerr;
		if (! read_int (fd, &pm1data->numrels, &sum)) goto readerr;
		if (! read_uint64 (fd, &pm1data->B2_start, &sum)) goto readerr;
		if (! read_uint64 (fd, &pm1data->numDsections, &sum)) goto readerr;
		if (! read_uint64 (fd, &pm1data->Dsection, &sum)) goto readerr;
		if (! read_uint64 (fd, &pm1data->first_relocatable, &sum)) goto readerr;
		if (! read_uint64 (fd, &pm1data->last_relocatable, &sum)) goto readerr;
		if (pm1data->stage2_type == PM1_STAGE2_PAIRING) {
			int	temp;
			if (! read_int (fd, &temp, &sum)) goto readerr;
			pm1data->stage2_numvals = temp;
			if (! read_int (fd, &pm1data->totrels, &sum)) goto readerr;
			if (! read_uint64 (fd, &pm1data->max_pairmap_Dsections, &sum)) goto readerr;
			if (! read_int (fd, &pm1data->relp, &sum)) goto readerr;
			if (! read_array (fd, (char *) pm1data->relp_sets, 32 * sizeof (int16_t), &sum)) goto readerr;
			if (! read_uint64 (fd, &pm1data->pairmap_size, &sum)) goto readerr;
			pm1data->pairmap = (uint8_t *) malloc ((size_t) pm1data->pairmap_size);
			if (pm1data->pairmap == NULL) goto readerr;
			if (! read_array (fd, (char *) pm1data->pairmap, (size_t) pm1data->pairmap_size, &sum)) goto readerr;
			pm1data->pairmap_ptr = pm1data->pairmap;
		} else {
			if (version <= 7) {		
				pm1data->required_missing = map_polyD_to_first_missing_prime (pm1data->D);
			} else {
				if (! read_int32 (fd, &pm1data->required_missing, &sum)) goto readerr;
			}
		}
	}

	else if (pm1data->state == PM1_STATE_GCD) {
		if (! read_uint64 (fd, &pm1data->B_done, &sum)) goto readerr;
		if (! read_uint64 (fd, &pm1data->C_done, &sum)) goto readerr;
	}

	else if (pm1data->state == PM1_STATE_DONE) {
		if (! read_uint64 (fd, &pm1data->B_done, &sum)) goto readerr;
		if (! read_uint64 (fd, &pm1data->C_done, &sum)) goto readerr;
	}

	{
		int	have_x;
		if (version == 4) have_x = TRUE;				
		else if (! read_int (fd, &have_x, &sum)) goto readerr;		
		if (have_x) {
			pm1data->x_binary = allocgiant (((int) pm1data->gwdata.bit_length >> 5) + 10);
			if (pm1data->x_binary == NULL) goto readerr;
			if (! read_giant (fd, pm1data->x_binary, &sum)) goto readerr;
			if (pm1data->state < PM1_STATE_MIDSTAGE || pm1data->state == PM1_STATE_DONE) {
				pm1data->x = gwalloc (&pm1data->gwdata);
				if (pm1data->x == NULL) goto readerr;
				gianttogw (&pm1data->gwdata, pm1data->x_binary, pm1data->x);
				free (pm1data->x_binary), pm1data->x_binary = NULL;
			}
		}
	}

	if (version >= 6 && pm1data->state >= PM1_STATE_MIDSTAGE && pm1data->state <= PM1_STATE_STAGE2) {
			pm1data->invx_binary = allocgiant (((int) pm1data->gwdata.bit_length >> 5) + 10);
			if (pm1data->invx_binary == NULL) goto readerr;
			if (! read_giant (fd, pm1data->invx_binary, &sum)) goto readerr;
	}

	if (pm1data->state >= PM1_STATE_MIDSTAGE && pm1data->state <= PM1_STATE_GCD) {
		int	have_gg;
		if (version == 4) have_gg = TRUE;				
		else if (! read_int (fd, &have_gg, &sum)) goto readerr;		
		if (have_gg) {
			pm1data->gg_binary = allocgiant (((int) pm1data->gwdata.bit_length >> 5) + 10);
			if (pm1data->gg_binary == NULL) goto readerr;
			if (! read_giant (fd, pm1data->gg_binary, &sum)) goto readerr;
		}
	}

	if (filesum != sum) goto readerr;
	_close (fd);

	return (TRUE);

readerr:
	_close (fd);
	free (pm1data->x_binary), pm1data->x_binary = NULL;
	gwfree (&pm1data->gwdata, pm1data->x), pm1data->x = NULL;
	free (pm1data->invx_binary), pm1data->invx_binary = NULL;
	free (pm1data->gg_binary), pm1data->gg_binary = NULL;
err:
	return (FALSE);
}

struct pm1_stage2_cost_data {
	
	struct common_cost_data c;
	
	int	stage2_type;			
	double	sieve_depth;			
	double	takeAwayBits;			
	
	uint64_t poly2_size;			
	double	factor_probability;		
};

double pm1_stage2_cost (
	void	*data)		
{
	struct pm1_stage2_cost_data *cost_data = (struct pm1_stage2_cost_data *) data;
	double	cost;

	cost_data->c.est_init_transforms = 4.0;

	if (cost_data->stage2_type == PM1_STAGE2_PAIRING) {

		cost_data->c.est_init_polymult = 0.0;
		cost_data->c.est_stage2_polymult = 0.0;

		cost_data->c.est_init_transforms += 6.0 * 2.0;						
		cost_data->c.est_init_transforms += (double) (cost_data->c.D / 6 - 2) * 2.0 * 2.0;	
		cost_data->c.est_init_transforms += 2.0 * 2.0;						
		cost_data->c.est_init_transforms += (cost_data->c.totrels - cost_data->c.numrels) * 2.0; 

		cost_data->c.est_init_transforms +=
			cost_data->c.numrels * cost_data->c.relp_sets[1] * 2.0 +					 
			one_based_modulo (cost_data->c.totrels, cost_data->c.numrels) * cost_data->c.relp_sets[2] * 2.0; 

		cost_data->c.est_init_transforms += (2.0 * log2((double)((cost_data->c.B2_start + cost_data->c.D / 2) / cost_data->c.D)) + cost_data->c.multiplier) * 2.0;

		cost_data->c.est_stage2_transforms = (double) cost_data->c.numDsections * 2.0;

		cost_data->c.est_stage2_transforms += cost_data->c.numrels;

		cost_data->c.est_stage2_transforms += (cost_data->c.est_numpairs + cost_data->c.est_numsingles) * 2.0;

		cost = cost_data->c.est_init_transforms + cost_data->c.est_stage2_transforms;
		cost *= IniGetFloat (INI_FILE, "Pm1TransformCost", (float) 1.502);

		cost += cost_data->c.est_pairing_runtime;

		cost *= IniGetFloat (INI_FILE, "Pm1PairRatioAdjust", 1.0);

		cost_data->c.stage2_numvals = cost_data->c.totrels + 4;
	}

	else {
		uint64_t poly1_size, poly2_size, min_poly2_size, max_poly2_size, acceptable_poly2_size, unacceptable_poly2_size;
		double	mem_used_by_a_gwnum, mem_saved_by_compression, mem_used_by_polymult;

		cost_data->c.est_pairing_runtime = 0.0;

		poly1_size = cost_data->c.numrels;
		if (cost_data->c.gwdata != NULL) {
			mem_used_by_a_gwnum = (double) array_gwnum_size (cost_data->c.gwdata);
			max_poly2_size = max_safe_poly2_size (cost_data->c.gwdata, poly1_size*2, cost_data->c.numvals);
		} else {
			mem_used_by_a_gwnum = (double) cost_data->c.stage2_fftlen * sizeof (double) * 1.016;
			max_poly2_size = cost_data->c.numvals;
		}
		min_poly2_size = 2*poly1_size;
		if (min_poly2_size > max_poly2_size) return (-1.0);

		mem_saved_by_compression = (double) poly1_size * mem_used_by_a_gwnum * (1.0 - cost_data->c.poly_compression);
		acceptable_poly2_size = min_poly2_size - 1;
		unacceptable_poly2_size = max_poly2_size + 1;
		while (unacceptable_poly2_size - acceptable_poly2_size >= 2) {
			uint64_t midpoint = (acceptable_poly2_size + unacceptable_poly2_size) / 2;
			mem_used_by_polymult = (double) polymult_mem_required (cost_data->c.polydata, poly1_size, midpoint, POLYMULT_INVEC1_MONIC_RLP | POLYMULT_CIRCULAR);
			if (poly1_size + midpoint + floor ((mem_used_by_polymult - mem_saved_by_compression) / mem_used_by_a_gwnum + 0.5) <= cost_data->c.numvals)
				acceptable_poly2_size = midpoint;		
			else
				unacceptable_poly2_size = midpoint;		
		}
		if (acceptable_poly2_size < min_poly2_size) return (-1.0); 
		poly2_size = acceptable_poly2_size;
		mem_used_by_polymult = (double) polymult_mem_required (cost_data->c.polydata, poly1_size, poly2_size, POLYMULT_INVEC1_MONIC_RLP | POLYMULT_CIRCULAR);

		uint64_t num_points = poly2_size - (2*poly1_size+1) + 1;
		if (num_points < 1) return (-1.0);

		uint64_t num_polyevals = divide_rounding_up (cost_data->c.numDsections, num_points);
		cost_data->c.numDsections = num_polyevals * num_points;
		if (cost_data->c.gap_end == 0) {
			for ( ; ; ) {
				uint64_t B2_end = cost_data->c.B2_start + cost_data->c.numDsections * cost_data->c.D;
				uint64_t B2_start = round_down_to_multiple_of (B2_end / cost_data->c.first_missing_prime, cost_data->c.D);
				if (cost_data->c.B2_start >= B2_start) break;
				cost_data->c.B2_start = B2_start;
			}
		}

		double	polymult_cost;		
		polymult_cost = 2.145 + 0.58 * log2 ((double) poly2_size / 105.0);

		double L2_cache_size = (CPU_NUM_L2_CACHES > 0 ? CPU_TOTAL_L2_CACHE_SIZE / CPU_NUM_L2_CACHES : 0) * 1024.0;
		double polymult_mem_per_thread = mem_used_by_polymult / cost_data->c.stage2_threads;
		if (polymult_mem_per_thread >= 8 * L2_cache_size) polymult_cost *= 2.05;
		else if (polymult_mem_per_thread > L2_cache_size) polymult_cost *= 1.0 + 1.05 * (polymult_mem_per_thread - L2_cache_size) / (7 * L2_cache_size);
		polymult_cost *= IniGetFloat (INI_FILE, "Pm1PolymultCostAdjust", 1.0);

		cost_data->c.est_init_transforms += 4.0 * 2.0;						
		cost_data->c.est_init_transforms += (double) (cost_data->c.D / 4.0 / 6.0) * 2.0 * 2.0;	
		cost_data->c.est_init_transforms += (cost_data->c.numrels) * 2.0;			

		cost_data->c.est_init_polymult = log2 ((double) cost_data->c.numrels) * cost_data->c.numrels * polymult_cost;
		cost_data->c.est_init_transforms += log2 ((double) cost_data->c.numrels) * cost_data->c.numrels * 2.0;

		cost_data->c.est_init_transforms += log2 ((double)(cost_data->c.D / 2)) * 3.0;		
		cost_data->c.est_init_transforms += 2.0;						
		cost_data->c.est_init_transforms += log2 ((double)(cost_data->c.D / 2)) * 3.0;		
		cost_data->c.est_init_transforms += 2.0;						
		cost_data->c.est_init_transforms += log2 ((double) cost_data->c.numrels * (double) cost_data->c.numrels / 2.0) * 3.0; 
		cost_data->c.est_init_transforms += (double) cost_data->c.numrels * 3.0 * 2.0;		
		cost_data->c.est_init_transforms += cost_data->c.numrels * 1.0;				
		cost_data->c.est_init_transforms += log2 ((double) cost_data->c.B2_start / ((double) cost_data->c.D / 2.0)) * 3.0; 
		cost_data->c.est_init_transforms += log2 ((double) cost_data->c.numrels * 2.0) * 3.0 + 2.0; 

		cost_data->c.est_stage2_transforms = (double) cost_data->c.numDsections * 4.0 - 6.0;

		cost_data->c.est_stage2_polymult = (double) num_polyevals * (double) (poly1_size + poly2_size) * polymult_cost;

		cost_data->c.est_stage2_transforms += (double) cost_data->c.numDsections * 4.0;

		cost_data->c.stage2_numvals = poly1_size + poly2_size + (int) floor ((mem_used_by_polymult - mem_saved_by_compression) / mem_used_by_a_gwnum + 0.5) + 3;
		cost_data->poly2_size = poly2_size;

		cost = (cost_data->c.est_init_transforms + cost_data->c.est_stage2_transforms) * IniGetFloat (INI_FILE, "Pm1TransformCost", (float) 1.502);

		cost += (cost_data->c.est_init_polymult + cost_data->c.est_stage2_polymult) * IniGetFloat (INI_FILE, "Pm1PolyRatioAdjust", 1.0);
		cost *= (double) cost_data->c.stage2_fftlen / (double) cost_data->c.stage1_fftlen;	
	}

	cost += cost_data->c.modinv_cost;

	cost *= IniGetFloat (INI_FILE, "Pm1Stage2RatioAdjust", 1.0);
	cost *= (double) cost_data->c.stage1_threads / (double) cost_data->c.stage2_threads;
	cost_data->c.stage2_cost = cost;						
	cost_data->c.est_stage2_stage1_ratio = cost / cost_data->c.stage1_cost;		
	cost_data->c.total_cost = cost_data->c.stage1_cost + cost_data->c.stage2_cost + cost_data->c.gcd_cost;

	if (cost_data->sieve_depth != 0.0) {
		cost_data->factor_probability = pm1prob (cost_data->takeAwayBits, cost_data->sieve_depth, cost_data->c.B1,
							 cost_data->c.B2_start + cost_data->c.numDsections * cost_data->c.D);
		cost_data->c.efficiency = cost_data->factor_probability;
	} else {
		cost_data->factor_probability =	0.0;		
		cost_data->c.efficiency = (double) kruppa_adjust (cost_data->c.B2_start + cost_data->c.numDsections * cost_data->c.D, cost_data->c.B1);
	}
	cost_data->c.efficiency /= cost_data->c.total_cost;
	return (cost_data->c.efficiency);
}

double pm1_stage2_impl_given_numvals (
	pm1handle *pm1data,
	uint64_t numvals,				
	int	forced_stage2_type,			
	struct pm1_stage2_cost_data *return_cost_data)	
{
	double	efficiency, best_efficiency;	
	struct pm1_stage2_cost_data cost_data;	

	cost_data.c.stage1_cost = (double) pm1data->B * 1.44 * 2.0;

	best_efficiency = -1.0;
	cost_data.c.required_missing = pm1data->required_missing;
	cost_data.c.gwdata = &pm1data->gwdata;
	cost_data.c.polydata = &pm1data->polydata;
	cost_data.c.stage1_fftlen = pm1data->stage1_fftlen;
	cost_data.c.stage2_fftlen = gwfftlen (&pm1data->gwdata);
	cost_data.c.stage1_threads = pm1data->stage1_threads;
	cost_data.c.stage2_threads = pm1data->stage2_threads;
	init_gcd_costs (pm1data->gwdata.bit_length, &cost_data);
	cost_data.sieve_depth = pm1data->w->sieve_depth;
	cost_data.takeAwayBits = isMersenne (pm1data->w->k, pm1data->w->b, pm1data->w->n, pm1data->w->c) ? log2 (pm1data->w->n) + 1.0 :
				 isGeneralizedFermat (pm1data->w->k, pm1data->w->b, pm1data->w->n, pm1data->w->c) ? log2 (pm1data->w->n) : 0.0;

	cost_data.c.poly_compression = (double) gwfftlen (&pm1data->gwdata) * (double) sizeof (double) / (double) array_gwnum_size (&pm1data->gwdata);
	if (IniGetInt (INI_FILE, "Poly1Compress", 2) == 2) cost_data.c.poly_compression *= 0.875;
	if (IniGetInt (INI_FILE, "Poly1Compress", 2) == 0) cost_data.c.poly_compression = 1.0;

	for (int stage2_type = 0; stage2_type <= 1; stage2_type++) {	

		if (QA_TYPE != 0 && QA_TYPE != stage2_type + 1) continue;

		if (forced_stage2_type != 99 && forced_stage2_type != stage2_type) continue;

		cost_data.stage2_type = (stage2_type == 0 ? PM1_STAGE2_PAIRING : PM1_STAGE2_POLYMULT);
		cost_data.c.use_poly_D_data = (stage2_type == 1);
		cost_data.c.centers_on_Dmultiple = (stage2_type != 1);
		efficiency = best_stage2_impl (pm1data->B, pm1data->first_relocatable, pm1data->last_relocatable, pm1data->C_done, pm1data->C,
					       numvals - (stage2_type == 0 ? 4 : 3), &pm1_stage2_cost, &cost_data);
		if (efficiency > best_efficiency) {
			best_efficiency = efficiency;
			*return_cost_data = cost_data;
		}
	}

	return (best_efficiency);
}

int pm1_choose_B2 (
	pm1handle *pm1data,
	uint64_t numvals,
	int	forced_stage2_type,		
	char	*optimal_B2_msg)		
{
	int	max_B2mult;
	struct pm1_stage2_cost_data cost_data;	
	struct pm1_stage2_efficiency {
		int	i;
		int	actual_i;
		int	D;			
		uint64_t B2;			
		double	B2_cost;		
		double	fac_pct;		
		double	efficiency;
		double	compared_to_next_i;	
	} best[3];
	double	saved_sieve_depth;
	double	takeAwayBits;			

	if (forced_stage2_type == 99 && numvals >= 150) forced_stage2_type = 1;

	saved_sieve_depth = pm1data->w->sieve_depth;
	if (saved_sieve_depth == 0.0) pm1data->w->sieve_depth = 67.0;

	max_B2mult = IniGetInt (INI_FILE, "MaxOptimalB2Multiplier", (int) (numvals < 100000 ? numvals * 100 : 10000000));
#define p1eval(x,B2mult)	x.i = B2mult; if (x.i > max_B2mult) x.i = max_B2mult; \
				pm1data->C = x.i * pm1data->B; \
				x.efficiency = pm1_stage2_impl_given_numvals (pm1data, numvals, forced_stage2_type, &cost_data); \
				if (x.efficiency > 0.0) { \
				  x.B2 = cost_data.c.B2_start + cost_data.c.numDsections * cost_data.c.D; \
				  x.actual_i = (int) (x.B2 / pm1data->B); \
				  x.B2_cost = cost_data.c.stage2_cost; \
				  x.fac_pct = cost_data.factor_probability; \
				  x.D = (cost_data.stage2_type == PM1_STAGE2_POLYMULT) ? cost_data.c.D : 0; \
				   \
				   \
				  uint64_t altB1 = pm1data->B + (uint64_t) (0.01 * cost_data.c.stage2_cost / (1.44 * 2.0)); \
				  uint64_t altB2 = altB1 + (uint64_t) (0.99 * (x.B2 - pm1data->B)); \
				  x.compared_to_next_i = pm1prob (takeAwayBits, pm1data->w->sieve_depth, altB1,altB2) - cost_data.factor_probability; \
				  x.compared_to_next_i = - fabs (x.compared_to_next_i); \
				} else x.actual_i = x.i, x.compared_to_next_i = -100.0;

#define p1compare(x,y)	(((x.D && y.D) && (x.D > y.D || (x.D == y.D && x.compared_to_next_i > y.compared_to_next_i))) ||	\
			 ((x.D == 0 || y.D == 0) && (x.compared_to_next_i > y.compared_to_next_i)))

	takeAwayBits = isMersenne (pm1data->w->k, pm1data->w->b, pm1data->w->n, pm1data->w->c) ? log2 (pm1data->w->n) + 1.0 :
		       isGeneralizedFermat (pm1data->w->k, pm1data->w->b, pm1data->w->n, pm1data->w->c) ? log2 (pm1data->w->n) : 0.0;
	p1eval (best[0], 5);
	p1eval (best[1], best[0].actual_i * 50);
	p1eval (best[2], best[1].actual_i * 50);
	if (best[0].efficiency < 0.0 && best[1].efficiency < 0.0 && best[2].efficiency < 0.0) return (FALSE);

	while (p1compare (best[0], best[1]) && best[0].i + 2 != best[2].i) {
		best[2] = best[1];
		p1eval (best[1], (best[0].i + best[2].i) / 2);
	}

	while (!p1compare (best[1], best[2]) && best[2].i < max_B2mult) {
		best[0] = best[1];
		best[1] = best[2];
		p1eval (best[2], best[1].i * 2);
	}

	for ( ; ; ) {
		struct pm1_stage2_efficiency midpoint;

		int	lower_gap = best[1].i - best[0].actual_i;
		int	upper_gap = best[2].i - best[1].actual_i;
		if (lower_gap < 2 && upper_gap < 2) break;

		if (lower_gap > upper_gap) {					
			p1eval (midpoint, best[0].actual_i + lower_gap / 2);
			if (p1compare (midpoint, best[1])) {			
				best[2] = best[1];
				best[1] = midpoint;
			} else {						
				best[0] = midpoint;
			}
		} else {							
			p1eval (midpoint, best[1].actual_i + upper_gap / 2);
			if (!p1compare (best[1], midpoint)) {			
				best[0] = best[1];
				best[1] = midpoint;
			} else {						
				best[2] = midpoint;
			}
		}
	}

	pm1data->C = best[1].actual_i * pm1data->B;
	if (saved_sieve_depth == 0.0) {
		sprintf (optimal_B2_msg, "With unknown amount of trial factoring done, guessing optimal B2 is %d*B1 = %" PRIu64 ".\n", best[1].actual_i, pm1data->C);
	} else {
		sprintf (optimal_B2_msg, "With trial factoring done to 2^%g, optimal B2 is %d*B1 = %" PRIu64 ".\n", saved_sieve_depth, best[1].actual_i, pm1data->C);
		sprintf (optimal_B2_msg + strlen (optimal_B2_msg), "If no prior P-1, chance of a new factor is %.3g%%\n", best[1].fac_pct * 100.0);
	}

	pm1data->w->sieve_depth = saved_sieve_depth;

	return (TRUE);
}
#undef p1eval
#undef B1increase
#undef p1compare

double pm1_stage2_impl (
	pm1handle *pm1data,
	unsigned int memory,		
	int	forced_stage2_type,	
	char	*msgbuf)		
{
	uint64_t numvals;		
	struct pm1_stage2_cost_data cost_data;
	bool	B2_was_chosen = FALSE;

	numvals = cvt_mem_to_array_gwnums_adj (&pm1data->gwdata, memory, -0.5);
	if (numvals < 8) numvals = 8;
	if (QA_TYPE) numvals = QA_TYPE;			

	if (pm1data->state == PM1_STATE_MIDSTAGE) {
		if (pm1data->C_done == pm1data->B) {
			pm1data->first_relocatable = pm1data->first_C_start;
			pm1data->last_relocatable = 0;
			if (pm1data->optimal_B2) {
				if (!pm1_choose_B2 (pm1data, numvals, forced_stage2_type, msgbuf + strlen (msgbuf))) return (0.0);
				B2_was_chosen = TRUE;
			}
		} else {
			pm1data->first_relocatable = pm1data->C_done;
			pm1data->last_relocatable = 0;
		}
	}
	if (pm1data->state >= PM1_STATE_STAGE2 && pm1data->stage2_type == PM1_STAGE2_POLYMULT) {
		pm1data->first_relocatable = pm1data->C_done > pm1data->B ? pm1data->C_done : pm1data->B;
		pm1data->last_relocatable = 0;
	}

	if (pm1data->state >= PM1_STATE_STAGE2 &&				
	    pm1data->stage2_type == PM1_STAGE2_PAIRING &&			
	    pm1data->pairmap != NULL &&						
	    numvals >= pm1data->stage2_numvals &&				
	    forced_stage2_type != 1 &&						
	    (numvals < pm1data->stage2_numvals * 2 ||				
	     pm1data->Dsection >= pm1data->numDsections / 2))			
		return (1.0);							

	if (pm1data->state == PM1_STATE_STAGE2 && pm1data->Dsection) {
		int max_relp_set = (pm1data->stage2_type == PM1_STAGE2_PAIRING) ? get_max_relp_set (pm1data->relp_sets) : 0;
		if (pm1data->Dsection > max_relp_set) pm1data->C_done = pm1data->B2_start + (pm1data->Dsection - max_relp_set) * pm1data->D;
	}

	cost_data.c.total_cost = 0.0;						
	pm1_stage2_impl_given_numvals (pm1data, numvals, forced_stage2_type, &cost_data);

	if (pm1data->state >= PM1_STATE_STAGE2 &&				
	    pm1data->stage2_type == PM1_STAGE2_PAIRING &&			
	    pm1data->pairmap != NULL &&						
	    forced_stage2_type != 1 &&						
	    numvals >= pm1data->stage2_numvals &&				
	    (cost_data.c.total_cost == 0.0 ||					
	     cost_data.c.stage2_numvals < pm1data->stage2_numvals * 2))		
		return (1.0);							

	if (pm1data->state >= PM1_STATE_STAGE2) {
		free (pm1data->pairmap);
		pm1data->pairmap = NULL;
		pm1data->pairmap_size = 0;
	}

	if (cost_data.c.total_cost == 0.0) return (0.0);

	pm1data->stage2_numvals = cost_data.c.stage2_numvals;
	pm1data->D = cost_data.c.D;
	pm1data->numrels = cost_data.c.numrels;
	pm1data->B2_start = cost_data.c.B2_start;
	pm1data->numDsections = cost_data.c.numDsections;
	pm1data->Dsection = 0;
	if (pm1data->state == PM1_STATE_MIDSTAGE || pm1data->last_relocatable > pm1data->B2_start)
		pm1data->last_relocatable = (pm1data->B2_start > pm1data->C_done ? pm1data->B2_start : pm1data->C_done);
	pm1data->stage2_type = cost_data.stage2_type;
	if (pm1data->stage2_type == PM1_STAGE2_PAIRING) {
		pm1data->totrels = cost_data.c.totrels;
		memcpy (pm1data->relp_sets, cost_data.c.relp_sets, sizeof (pm1data->relp_sets));
		pm1data->max_pairmap_Dsections = cost_data.c.max_pairmap_Dsections;
	} else {
		pm1data->poly1_size = cost_data.c.numrels;
		pm1data->poly2_size = cost_data.poly2_size;
	}

	if (IniGetInt (INI_FILE, "Stage2Estimates", 0)) {
		if (pm1data->stage2_type == PM1_STAGE2_PAIRING)
			sprintf (msgbuf + strlen (msgbuf), "Est. pair%%: %5.2f, init transforms: %.0f, main loop transforms: %.0f\n",
				 cost_data.c.est_pair_pct * 100.0, cost_data.c.est_init_transforms, cost_data.c.est_stage2_transforms);
		else
			sprintf (msgbuf + strlen (msgbuf), "Est. init transforms: %.0f, main loop transforms: %.0f, init poly cost: %.0f, main loop poly cost: %.0f\n",
				 cost_data.c.est_init_transforms, cost_data.c.est_stage2_transforms, cost_data.c.est_init_polymult, cost_data.c.est_stage2_polymult);
	}
	
	sprintf (msgbuf + strlen (msgbuf), "Estimated stage 2 vs. stage 1 runtime ratio: %.3f\n", cost_data.c.est_stage2_stage1_ratio);
	pm1data->est_stage2_stage1_ratio = cost_data.c.est_stage2_stage1_ratio;

	return (B2_was_chosen ? cost_data.factor_probability : cost_data.c.efficiency);
}

void calc_exp (
	void	*sieve_info,	
	double	k,		
	unsigned long b,	
	unsigned long n,	
	signed long c,		
	mpz_t	g,		
	uint64_t B1,		
	uint64_t *p,		
	uint64_t lower,
	uint64_t upper)
{
	uint64_t len;

	len = upper - lower;

	if (len >= 1024) {
		mpz_t	x;
		calc_exp (sieve_info, k, b, n, c, g, B1, p, lower, lower + (len >> 1));
		mpz_init (x);
		calc_exp (sieve_info, k, b, n, c, x, B1, p, lower + (len >> 1), upper);
		mpz_mul (g, x, g);
		mpz_clear (x);
		return;
	}

	if (lower == 0) mpz_set_ui (g, 2*n);
	else mpz_set_ui (g, 1);

	for ( ; *p <= B1 && mpz_sizeinbase (g, 2) < len; *p = sieve (sieve_info)) {
		uint64_t val, max;
		val = *p;
		max = B1 / *p;
		while (val <= max) val *= *p;
		if (sizeof (unsigned int) == 4 && val > 0xFFFFFFFF) {
			mpz_t	mpz_val;
			mpz_init_set_d (mpz_val, (double) val);		
			mpz_mul (g, g, mpz_val);
			mpz_clear (mpz_val);
		} else
			mpz_mul_ui (g, g, (unsigned int) val);
	}
}

void calc_exp2 (
	void	*sieve_info,	
	mpz_t	g,		
	uint64_t B1_completed,	
	uint64_t B1,		
	uint64_t *p,		
	uint64_t len)		
{

	if (len >= 256) {
		mpz_t	x;
		calc_exp2 (sieve_info, g, B1_completed, B1, p, len >> 1);
		mpz_init (x);
		calc_exp2 (sieve_info, x, B1_completed, B1, p, len >> 1);
		mpz_mul (g, x, g);
		mpz_clear (x);
		return;
	}

	mpz_set_ui (g, 1);
	for ( ; *p <= B1 && mpz_sizeinbase (g, 2) < len; *p = sieve (sieve_info)) {
		uint64_t no_more_vals_above_here = B1 / *p;
		for (uint64_t val = *p; ; val *= *p) {
			if (val > B1_completed) {
				if (sizeof (unsigned int) == 4 && *p > 0xFFFFFFFF) {
					mpz_t	mpz_val;
					mpz_init_set_d (mpz_val, (double) *p);		
					mpz_mul (g, g, mpz_val);
					mpz_clear (mpz_val);
				} else
					mpz_mul_ui (g, g, (unsigned int) *p);
			}
			if (val > no_more_vals_above_here) break;
		}
	}
}

#define PM1_COMPUTE_POLY2	1
#define PM1_ACCUMULATING_GG	2

void pm1_helper (
	int	helper_num,	
	gwhandle *gwdata,	
	void	*info)
{
	pm1handle *pm1data = (pm1handle *) info;

	if (pm1data->helper_work == PM1_COMPUTE_POLY2) {
	    
	    for ( ; ; ) {
		helper_num = (int) atomic_fetch_incr (pm1data->polydata.helper_counter);
		if (helper_num >= pm1data->helper_count) break;

		gwnum diff1 = pm1data->poly2[(pm1data->remaining_poly2_size - 1 - helper_num) % pm1data->helper_count];	
		gwnum diff2 = pm1data->r_2helper2;		

		for (uint64_t j = pm1data->remaining_poly2_size - 1 - helper_num; ; j -= pm1data->helper_count) {
			
			gwmul3 (gwdata, pm1data->poly2[j+pm1data->helper_count], diff1, pm1data->poly2[j], GWMUL_FFT_S12 | GWMUL_STARTNEXTFFT);
			if (j < pm1data->helper_count) break;
			
			if (j == pm1data->helper_count) gwmul3 (gwdata, diff1, diff2, pm1data->diff1, GWMUL_STARTNEXTFFT), diff1 = pm1data->diff1;
			else gwmul3 (gwdata, diff1, diff2, diff1, GWMUL_STARTNEXTFFT);
		}
	    }
	}

	else if (pm1data->helper_work == PM1_ACCUMULATING_GG) {
		gwnum	accumulator = NULL;

		for ( ; ; ) {
			
			uint64_t point = atomic_fetch_incr (pm1data->polydata.helper_counter);

			if (point >= pm1data->num_points) break;

			if (polymult_must_unfft (gwdata, pm1data->points[point]))
				gwunfft2 (gwdata, pm1data->points[point], pm1data->points[point], GWMUL_STARTNEXTFFT);

			if (accumulator == NULL) accumulator = pm1data->points[point];
			else gwmul3 (gwdata, pm1data->points[point], accumulator, accumulator, GWMUL_STARTNEXTFFT);
		}

		if (accumulator != NULL) {
			gwfft (gwdata, accumulator, accumulator);
			gwmutex_lock (&pm1data->polydata.poly_mutex);
			gwmul3 (gwdata, accumulator, pm1data->gg, pm1data->gg, GWMUL_STARTNEXTFFT);

			gwmutex_unlock (&pm1data->polydata.poly_mutex);
		}
	}
}

int pminus1 (
	int	thread_num,
	struct PriorityInfo *sp_info,	
	struct work_unit *w)
{
	pm1handle pm1data;
	giant	N;		
	giant	factor;		
	mpz_t	exp;
	int	exp_initialized;
	uint64_t stage0_limit;
	unsigned int memused;
	int	i, stage1_batch_size, stage1_mem, stage1_temps;
	unsigned long len;
	int	maxerr_restart_count = 0;
	unsigned long maxerr_fftlen = 0;
	char	filename[32], buf[255], JSONbuf[4000], testnum[100];
	int	pminus1_base, res, stop_reason, saving, near_fft_limit, echk;
	double	one_over_len, one_over_B, base_pct_complete, one_relp_pct;
	uint64_t last_output, last_output_t, last_output_r;
	double	allowable_maxerr, output_frequency, output_title_frequency;
	int	first_iter_msg;
	int	msglen;
	char	*str, *msg;
	double	stage1_timer, stage2_timer, timers[2];
	relp_set_data_map relp_set_map;
	Dmultiple_data_map Dmultiple_map;

	OutputStr (thread_num, "\n");

	N = NULL;
	factor = NULL;
	str = NULL;
	msg = NULL;
	exp_initialized = FALSE;

	pminus1_base = (w->b != 3 ? 3 : 5);
	memset (&pm1data, 0, sizeof (pm1handle));
	pm1data.thread_num = thread_num;
	pm1data.stage1_threads = get_worker_num_threads (thread_num, HYPERTHREAD_LL);
	pm1data.stage2_threads = pm1data.stage1_threads + IniGetInt (INI_FILE, "Stage2ExtraThreads", 0);
	pm1data.w = w;
	pm1data.B = (uint64_t) w->B1;
	pm1data.C = (uint64_t) w->B2;
	if (pm1data.B < 90) {
		OutputStr (thread_num, "Using minimum bound #1 of 90\n");
		pm1data.B = 90;
	}
	if (pm1data.C == 0) pm1data.C = pm1data.B * 100;
	if (pm1data.C < pm1data.B) pm1data.C = pm1data.B;
	pm1data.first_C_start = (uint64_t) w->B2_start;
	if (pm1data.first_C_start < pm1data.B) pm1data.first_C_start = pm1data.B;
	pm1data.pct_mem_to_use = 1.0;				

	pm1data.optimal_B2 = (!QA_IN_PROGRESS && w->work_type == WORK_PMINUS1 && pm1data.first_C_start == pm1data.B && IniGetInt (INI_FILE, "Pminus1BestB2", 1) &&
			      (w->sieve_depth > 50 || (w->sieve_depth == 0 && pm1data.C == 100 * pm1data.B)));
	if (pm1data.optimal_B2 && pm1data.C <= pm1data.B) pm1data.C = 100 * pm1data.B;	

	stop_reason = setN (thread_num, w, &N);
	if (stop_reason) goto exit;

	gw_as_string (testnum, w->k, w->b, w->n, w->c);
	sprintf (buf, "%s P-1", testnum);
	title (thread_num, buf);
	if (w->work_type == WORK_PMINUS1) {
		if (pm1data.C <= pm1data.B)
			sprintf (buf, "P-1 on %s with B1=%" PRIu64 "\n", testnum, pm1data.B);
		else if (pm1data.optimal_B2)
			sprintf (buf, "P-1 on %s with B1=%" PRIu64 ", B2=TBD\n", testnum, pm1data.B);
		else
			sprintf (buf, "P-1 on %s with B1=%" PRIu64 ", B2=%" PRIu64 "\n", testnum, pm1data.B, pm1data.C);
		OutputStr (thread_num, buf);
		if (w->sieve_depth > 0.0 && !pm1data.optimal_B2) {
			double prob = guess_pminus1_probability (w);
			sprintf (buf, "Chance of finding a new factor is an estimated %.3g%%\n", prob * 100.0);
			OutputStr (thread_num, buf);
		}
	}

restart:
	tempFileName (w, filename);
	filename[0] = 'm';

	gwinit (&pm1data.gwdata);
	gwset_maxmulbyconst (&pm1data.gwdata, pminus1_base);
	if (IniGetInt (INI_FILE, "UseLargePages", 0)) gwset_use_large_pages (&pm1data.gwdata);
	if (HYPERTHREAD_LL) sp_info->normal_work_hyperthreading = TRUE, gwset_will_hyperthread (&pm1data.gwdata, 2);
	gwset_bench_cores (&pm1data.gwdata, HW_NUM_CORES);
	gwset_bench_workers (&pm1data.gwdata, NUM_WORKERS);
	if (ERRCHK) gwset_will_error_check (&pm1data.gwdata);
	else gwset_will_error_check_near_limit (&pm1data.gwdata);
	gwset_num_threads (&pm1data.gwdata, pm1data.stage1_threads);
	gwset_thread_callback (&pm1data.gwdata, SetAuxThreadPriority);
	gwset_thread_callback_data (&pm1data.gwdata, sp_info);
	gwset_safety_margin (&pm1data.gwdata, IniGetFloat (INI_FILE, "ExtraSafetyMargin", 0.0));
	gwset_larger_fftlen_count (&pm1data.gwdata, maxerr_restart_count < 3 ? maxerr_restart_count : 3);
	gwset_minimum_fftlen (&pm1data.gwdata, w->minimum_fftlen);
	gwset_using_polymult (&pm1data.gwdata);
	gwset_use_spin_wait (&pm1data.gwdata, IniGetInt (INI_FILE, "SpinWait", 0));
	res = gwsetup (&pm1data.gwdata, w->k, w->b, w->n, w->c);
	if (res) {
		sprintf (buf, "Cannot initialize FFT code, errcode=%d\n", res);
		OutputBoth (thread_num, buf);
		return (STOP_FATAL_ERROR);
	}
	gwerror_checking (&pm1data.gwdata, ERRCHK);
	pm1data.stage1_fftlen = gwfftlen (&pm1data.gwdata);

	stage1_timer = stage2_timer = 0.0;
	last_output = last_output_t = last_output_r = 0;
	gw_clear_fft_count (&pm1data.gwdata);
	first_iter_msg = TRUE;
	calc_output_frequencies (&pm1data.gwdata, &output_frequency, &output_title_frequency);
	clear_timers (timers, 2);

	{
		char	fft_desc[200];
		gwfft_description (&pm1data.gwdata, fft_desc);
		sprintf (buf, "Using %s\n", fft_desc);
		OutputStr (thread_num, buf);
	}

	near_fft_limit = exponent_near_fft_limit (&pm1data.gwdata);
	gwerror_checking (&pm1data.gwdata, ERRCHK || near_fft_limit);

	allowable_maxerr = IniGetFloat (INI_FILE, "MaxRoundoffError", (float) 0.4375);

	readSaveFileStateInit (&pm1data.read_save_file_state, thread_num, filename);
	writeSaveFileStateInit (&pm1data.write_save_file_state, filename, 0);
	for ( ; ; ) {
		if (! saveFileExists (&pm1data.read_save_file_state)) {
			
			if (pm1data.read_save_file_state.a_non_bad_save_file_existed) {
				OutputBoth (thread_num, ALLSAVEBAD_MSG);
				return (0);
			}
			
			break;
		}

		if (!pm1_restore (&pm1data)) {
			
			saveFileBad (&pm1data.read_save_file_state);
			continue;
		}

		if (pm1data.state == PM1_STATE_STAGE0) {
			if (pm1data.interim_B > pm1data.B) {
				pm1data.B = pm1data.interim_B;
				sprintf (buf, "Ignoring suggested B1 value, using B1=%" PRIu64 " from the save file\n", pm1data.B);
				OutputStr (thread_num, buf);
			}
			goto restart0;
		}

		if (pm1data.state == PM1_STATE_STAGE1) {
			if (pm1data.interim_B > pm1data.B) pm1data.interim_B = pm1data.B;
			goto restart1;
		}

		if (pm1data.state == PM1_STATE_MIDSTAGE) {
			if (pm1data.B > pm1data.B_done) {
				free (pm1data.gg_binary), pm1data.gg_binary = NULL;
				goto more_B;
			}
			goto restart3b;
		}

		if (pm1data.state == PM1_STATE_STAGE2) {

			if (pm1data.B > pm1data.B_done && w->work_type == WORK_PMINUS1) {
				free (pm1data.gg_binary), pm1data.gg_binary = NULL;
				free (pm1data.pairmap), pm1data.pairmap = NULL;
				goto more_B;
			}

			if (pm1data.B != pm1data.B_done) {
				pm1data.B = pm1data.B_done;
				sprintf (buf, "Ignoring suggested B1 value, using B1=%" PRIu64 " from the save file\n", pm1data.B);
				OutputStr (thread_num, buf);
			}

			if ((w->work_type != WORK_PMINUS1 && pm1data.C != pm1data.interim_C) ||
			    (w->work_type == WORK_PMINUS1 && pm1data.C < pm1data.interim_C)) {
				pm1data.C = pm1data.interim_C;
				sprintf (buf, "Ignoring suggested B2 value, using B2=%" PRIu64 " from the save file\n", pm1data.C);
				OutputStr (thread_num, buf);
			}

			if (pm1data.optimal_B2) {
				pm1data.C = pm1data.interim_C;
				sprintf (buf, "Resuming P-1 in stage 2 with B2=%" PRIu64 "\n", pm1data.interim_C);
				OutputStr (thread_num, buf);
			}
			goto restart3b;
		}

		if (pm1data.state == PM1_STATE_GCD) {
			if (pm1data.optimal_B2) pm1data.C = pm1data.C_done;
			goto restart4;
		}

		ASSERTG (pm1data.state == PM1_STATE_DONE);
		if (pm1data.B > pm1data.B_done) goto more_B;
		pm1data.B = pm1data.B_done;
		if (pm1data.C > pm1data.C_done) {
			pm1data.state = PM1_STATE_STAGE1;		
			if (pm1data.first_C_start == pm1data.B) pm1data.first_C_start = pm1data.C_done;
			if (pm1data.C_done > pm1data.B) {
				sprintf (buf, "Resuming P-1 in stage 2 with B2 from %" PRIu64 " to %" PRIu64 "\n", pm1data.C_done, pm1data.C);
				OutputStr (thread_num, buf);
			}
			goto restart3a;
		}

		sprintf (buf, "%s already tested to B1=%" PRIu64 " and B2=%" PRIu64 ".\n",
			 gwmodulo_as_string (&pm1data.gwdata), pm1data.B_done, pm1data.C_done);
		OutputBoth (thread_num, buf);
		goto done;
	}

	strcpy (w->stage, "S1");
	w->pct_complete = 0.0;
	start_timer_from_zero (&stage1_timer, 0);
	pm1data.state = PM1_STATE_STAGE0;
	pm1data.interim_B = pm1data.B;
	pm1data.stage0_bitnum = 0;
	pm1data.x = gwalloc (&pm1data.gwdata);
	if (pm1data.x == NULL) goto oom;
	dbltogw (&pm1data.gwdata, pminus1_base, pm1data.x);

	pm1data.max_stage0_prime = (uint64_t) IniGetInt (INI_FILE, "MaxStage0Prime", 250);
	if (pm1data.max_stage0_prime < 1) pm1data.max_stage0_prime = 1;
	if (pm1data.max_stage0_prime > 2000) pm1data.max_stage0_prime = 2000;
	pm1data.max_stage0_prime *= 1000000;

restart0:
	pm1data.B_done = 0;
	set_memory_usage (thread_num, 0, cvt_gwnums_to_mem (&pm1data.gwdata, 1));
	start_timer_from_zero (timers, 0);
	start_timer_from_zero (timers, 1);
	stop_reason = start_sieve_with_limit (thread_num, 2, (uint32_t) sqrt ((double) pm1data.B), &pm1data.sieve_info);
	if (stop_reason) goto exit;
	pm1data.stage1_prime = sieve (pm1data.sieve_info);
	stage0_limit = (pm1data.interim_B > pm1data.max_stage0_prime) ? pm1data.max_stage0_prime : pm1data.interim_B;
	mpz_init (exp);  exp_initialized = TRUE;
	calc_exp (pm1data.sieve_info, w->k, w->b, w->n, w->c, exp, pm1data.interim_B, &pm1data.stage1_prime, 0, stage0_limit * 3 / 2);

	len = (unsigned long) mpz_sizeinbase (exp, 2) - 1;
	one_over_len = 1.0 / (double) len;
	if (pm1data.stage1_prime < pm1data.B) one_over_len *= (double) pm1data.stage1_prime / (double) pm1data.B;

	gwsetmulbyconst (&pm1data.gwdata, pminus1_base);
	while (pm1data.stage0_bitnum < len) {

		stop_reason = stopCheck (thread_num);
		saving = testSaveFilesFlag (thread_num);
		echk = stop_reason || saving || ERRCHK || near_fft_limit || ((pm1data.stage0_bitnum & 127) == 64);
		gwerror_checking (&pm1data.gwdata, echk);

#ifndef SERVER_TESTING
		int options = (!stop_reason && !saving && pm1data.stage0_bitnum+1 != len) ? GWMUL_STARTNEXTFFT : 0;
		if (mpz_tstbit64 (exp, len - pm1data.stage0_bitnum - 1)) options |= GWMUL_MULBYCONST;
		int option = (!stop_reason && !saving && pm1data.stage0_bitnum+1 != len) ? GWMUL_STARTNEXTFFT : 0;
		if (pm1data.stage0_bitnum < 30) gwmul3_carefully (&pm1data.gwdata, pm1data.x, pm1data.x, pm1data.x, options);
		else gwsquare2 (&pm1data.gwdata, pm1data.x, pm1data.x, options);
#endif

		if (gw_test_for_error (&pm1data.gwdata) || gw_get_maxerr (&pm1data.gwdata) > allowable_maxerr) goto err;

		pm1data.stage0_bitnum++;
		w->pct_complete = (double) pm1data.stage0_bitnum * one_over_len;

		if (first_iter_msg ||
		    (ITER_OUTPUT != 999999999 && gw_get_fft_count (&pm1data.gwdata) >= last_output_t + 2 * ITER_OUTPUT * output_title_frequency)) {
			sprintf (buf, "%.*f%% of %s P-1 stage 1",
				 (int) PRECISION, trunc_percent (w->pct_complete), gwmodulo_as_string (&pm1data.gwdata));
			title (thread_num, buf);
			last_output_t = gw_get_fft_count (&pm1data.gwdata);
		}

		if (first_iter_msg ||
		    (ITER_OUTPUT != 999999999 && gw_get_fft_count (&pm1data.gwdata) >= last_output + 2 * ITER_OUTPUT * output_frequency)) {
			sprintf (buf, "%s stage 1 is %.*f%% complete.",
				 gwmodulo_as_string (&pm1data.gwdata), (int) PRECISION, trunc_percent (w->pct_complete));
			end_timer (timers, 0);
			if (first_iter_msg) {
				strcat (buf, "\n");
			} else {
				strcat (buf, " Time: ");
				print_timer (timers, 0, buf, TIMER_NL);
			}
			if (pm1data.stage0_bitnum > 1)
				OutputStr (thread_num, buf);
			start_timer_from_zero (timers, 0);
			last_output = gw_get_fft_count (&pm1data.gwdata);
			first_iter_msg = FALSE;
		}

		if ((ITER_OUTPUT_RES != 999999999 && gw_get_fft_count (&pm1data.gwdata) >= last_output_r + 2 * ITER_OUTPUT_RES) ||
		    (NO_GUI && stop_reason)) {
			sprintf (buf, "%s stage 1 is %.*f%% complete.\n",
				 gwmodulo_as_string (&pm1data.gwdata), (int) PRECISION, trunc_percent (w->pct_complete));
			writeResults (buf);
			last_output_r = gw_get_fft_count (&pm1data.gwdata);
		}

		if (stop_reason || saving) {
			pm1_save (&pm1data);
			if (stop_reason) goto exit;
		}
	}

	gwerror_checking (&pm1data.gwdata, ERRCHK || near_fft_limit);
	mpz_clear (exp), exp_initialized = FALSE;
	end_timer (timers, 0);
	end_timer (timers, 1);

	pm1data.stage1_prime--;				

restart1:
	strcpy (w->stage, "S1");
	one_over_B = 1.0 / (double) pm1data.B;
	w->pct_complete = (double) pm1data.stage1_prime * one_over_B;
	start_timer (timers, 0);
	start_timer (timers, 1);
	pm1data.state = PM1_STATE_STAGE1;
	stop_reason = start_sieve_with_limit (thread_num, pm1data.stage1_prime, (uint32_t) sqrt((double) pm1data.B), &pm1data.sieve_info);
	if (stop_reason) goto exit;

	stage1_batch_size = (int) ((double) IniGetInt (INI_FILE, "Stage1Batch4KMults", 150000) * pm1data.stage1_threads / (pm1data.gwdata.FFTLEN / 4096.0));
	if (stage1_batch_size < IniGetInt (INI_FILE, "Stage1BatchSizeMin", 40)) stage1_batch_size = IniGetInt (INI_FILE, "Stage1BatchSizeMin", 40);

	stage1_mem = IniGetInt (INI_FILE, "Stage1Memory", 100);		
	stage1_temps = (int) cvt_mem_to_gwnums (&pm1data.gwdata, stage1_mem);
	if (stage1_temps > IniGetInt (INI_FILE, "Stage1MaxTemps", 512)) stage1_temps = IniGetInt (INI_FILE, "Stage1MaxTemps", 512);
	set_memory_usage (thread_num, 0, cvt_gwnums_to_mem (&pm1data.gwdata, stage1_temps));

	for (pm1data.stage1_prime = sieve (pm1data.sieve_info); pm1data.stage1_prime < pm1data.interim_B; ) {

		stop_reason = stopCheck (thread_num);
		saving = testSaveFilesFlag (thread_num);
		echk = stop_reason || saving || ERRCHK || near_fft_limit || ((pm1data.stage1_prime & 127) == 127);
		gwerror_checking (&pm1data.gwdata, echk);

		mpz_init (exp);  exp_initialized = TRUE;
		calc_exp2 (pm1data.sieve_info, exp, pm1data.B_done, pm1data.interim_B, &pm1data.stage1_prime, stop_reason ? stage1_batch_size/10 : stage1_batch_size);

		exponentiate_mpz_limited_temps (&pm1data.gwdata, pm1data.x, exp, stage1_temps);
		mpz_clear (exp), exp_initialized = FALSE;

		if (gw_test_for_error (&pm1data.gwdata) || gw_get_maxerr (&pm1data.gwdata) > allowable_maxerr) goto err;

		w->pct_complete = (double) pm1data.stage1_prime * one_over_B;

		if (first_iter_msg ||
		    (ITER_OUTPUT != 999999999 && gw_get_fft_count (&pm1data.gwdata) >= last_output_t + 2 * ITER_OUTPUT * output_title_frequency)) {
			sprintf (buf, "%.*f%% of %s P-1 stage 1",
				 (int) PRECISION, trunc_percent (w->pct_complete), gwmodulo_as_string (&pm1data.gwdata));
			title (thread_num, buf);
			last_output_t = gw_get_fft_count (&pm1data.gwdata);
		}

		if (first_iter_msg ||
		    (ITER_OUTPUT != 999999999 && gw_get_fft_count (&pm1data.gwdata) >= last_output + 2 * ITER_OUTPUT * output_frequency)) {
			sprintf (buf, "%s stage 1 is %.*f%% complete.",
				 gwmodulo_as_string (&pm1data.gwdata), (int) PRECISION, trunc_percent (w->pct_complete));
			end_timer (timers, 0);
			if (first_iter_msg) {
				strcat (buf, "\n");
			} else {
				strcat (buf, " Time: ");
				print_timer (timers, 0, buf, TIMER_NL);
			}
			OutputStr (thread_num, buf);
			start_timer_from_zero (timers, 0);
			last_output = gw_get_fft_count (&pm1data.gwdata);
			first_iter_msg = FALSE;
		}

		if ((ITER_OUTPUT_RES != 999999999 && gw_get_fft_count (&pm1data.gwdata) >= last_output_r + 2 * ITER_OUTPUT_RES) ||
		    (NO_GUI && stop_reason)) {
			sprintf (buf, "%s stage 1 is %.*f%% complete.\n",
				 gwmodulo_as_string (&pm1data.gwdata), (int) PRECISION, trunc_percent (w->pct_complete));
			writeResults (buf);
			last_output_r = gw_get_fft_count (&pm1data.gwdata);
		}

		if (stop_reason || saving) {
			pm1_save (&pm1data);
			if (stop_reason) goto exit;
		}
	}
	pm1data.B_done = pm1data.interim_B;
	end_timer (timers, 0);

	if (pm1data.B > pm1data.B_done) {
more_B:		if (pm1data.x_binary != NULL) {			
			ASSERTG (pm1data.x == NULL);
			pm1data.x = gwalloc (&pm1data.gwdata);
			if (pm1data.x == NULL) goto oom;
			gianttogw (&pm1data.gwdata, pm1data.x_binary, pm1data.x);
			free (pm1data.x_binary), pm1data.x_binary = NULL;
		}
		pm1data.interim_B = pm1data.B;
		pm1data.stage1_prime = 1;
		
		goto restart1;
	}
	pm1data.C_done = pm1data.B;

	gwunfft (&pm1data.gwdata, pm1data.x, pm1data.x);

	end_timer (timers, 1);
	if (stage1_timer != 0.0) end_timer (&stage1_timer, 0);
	sprintf (buf, "%s stage 1 complete. %" PRIu64 " transforms. Total time: ", gwmodulo_as_string (&pm1data.gwdata), gw_get_fft_count (&pm1data.gwdata));
	print_timer (timers, 1, buf, TIMER_NL | TIMER_CLR);
	OutputStr (thread_num, buf);
	gw_clear_fft_count (&pm1data.gwdata);

	if (ERRCHK) {
		sprintf (buf, "Round off: %.10g\n", gw_get_maxerr (&pm1data.gwdata));
		OutputStr (thread_num, buf);
		gw_clear_maxerr (&pm1data.gwdata);
	}

	strcpy (w->stage, "S1");
	w->pct_complete = 1.0;
	if (pm1data.C <= pm1data.B) {
		if (w->work_type != WORK_PMINUS1) OutputStr (thread_num, "Starting stage 1 GCD - please be patient.\n");
		start_timer_from_zero (timers, 0);
		gwsmalladd (&pm1data.gwdata, -1, pm1data.x);
		stop_reason = gcd (&pm1data.gwdata, thread_num, pm1data.x, N, &factor);
		gwsmalladd (&pm1data.gwdata, 1, pm1data.x);
		if (stop_reason) {
			pm1_save (&pm1data);
			goto exit;
		}
		end_timer (timers, 0);
		strcpy (buf, "Stage 1 GCD complete. Time: ");
		print_timer (timers, 0, buf, TIMER_NL | TIMER_CLR);
		OutputStr (thread_num, buf);
		if (factor != NULL) goto bingo;
		
		pm1data.state = PM1_STATE_DONE;
		goto msg_and_exit;
	}

restart3a:
	strcpy (w->stage, "S1");
	w->pct_complete = 1.0;
	start_timer_from_zero (timers, 0);

	pm1data.invx = gwalloc (&pm1data.gwdata);
	if (pm1data.invx == NULL) goto oom;

	if (!QA_IN_PROGRESS && IniGetInt (INI_FILE, "Stage1GCD", 1) == -1) {
		gwcopy (&pm1data.gwdata, pm1data.x, pm1data.invx);
		stop_reason = pm1_modinv (&pm1data, pm1data.invx, N, factor);
		if (factor != NULL) goto bingo;
		if (stop_reason) goto exit;
	}

	else {
		gwnum x_minus_1 = gwalloc (&pm1data.gwdata);
		if (x_minus_1 == NULL) goto oom;
		gwcopy (&pm1data.gwdata, pm1data.x, x_minus_1);
		gwsmalladd (&pm1data.gwdata, -1, x_minus_1);
		gwmul3 (&pm1data.gwdata, pm1data.x, x_minus_1, pm1data.invx, GWMUL_PRESERVE_S1 | GWMUL_FFT_S2);	
		stop_reason = pm1_modinv (&pm1data, pm1data.invx, N, factor);					
		if (factor != NULL) goto bingo;
		if (stop_reason) goto exit;
		gwmul3 (&pm1data.gwdata, x_minus_1, pm1data.invx, pm1data.invx, 0);				
		gwfree (&pm1data.gwdata, x_minus_1);
	}

	end_timer (timers, 0);
	sprintf (buf, "Inversion of stage 1 result complete. %" PRIu64 " transforms, 1 modular inverse. Time: ", gw_get_fft_count (&pm1data.gwdata));
	print_timer (timers, 0, buf, TIMER_NL | TIMER_CLR);
	OutputStr (thread_num, buf);
	gw_clear_fft_count (&pm1data.gwdata);

	pm1data.x_binary = allocgiant (((int) pm1data.gwdata.bit_length >> 5) + 10);
	if (pm1data.x_binary == NULL) goto oom;
	gwtogiant (&pm1data.gwdata, pm1data.x, pm1data.x_binary);
	pm1data.invx_binary = allocgiant (((int) pm1data.gwdata.bit_length >> 5) + 10);
	if (pm1data.invx_binary == NULL) goto oom;
	gwtogiant (&pm1data.gwdata, pm1data.invx, pm1data.invx_binary);

	pm1data.state = PM1_STATE_MIDSTAGE;
	strcpy (w->stage, "S2");
	w->pct_complete = 0.0;
	start_timer_from_zero (&stage2_timer, 0);

	if (IniGetInt (INI_FILE, "SaveBeforeStage2", 1)) pm1_save (&pm1data);

	pm1data.required_missing = 0;

restart3b:
	ASSERTG (pm1data.gg == NULL);
	if (pm1data.gg_binary == NULL) {
		pm1data.gg_binary = allocgiant (((int) pm1data.gwdata.bit_length >> 5) + 10);
		if (pm1data.gg_binary == NULL) goto oom;
		gtog (pm1data.x_binary, pm1data.gg_binary);
		sladdg (-1, pm1data.gg_binary);
	}

	unsigned long min_memory;
	min_memory = cvt_gwnums_to_mem (&pm1data.gwdata, 8.5);
	if (max_mem (thread_num) < min_memory) {
		sprintf (buf, "Insufficient memory to ever run stage 2 -- %luMB needed.\n", min_memory);
		OutputStr (thread_num, buf);
		pm1data.C = pm1data.B_done;
		goto restart4;
	}

more_C:	start_timer_from_zero (timers, 0);
	sprintf (buf, "%s P-1 stage 2 init", gwmodulo_as_string (&pm1data.gwdata));
	title (thread_num, buf);

	clear_restart_if_max_memory_change (thread_num);

replan:	unsigned long best_fftlen;
	unsigned int memory;
	int	forced_stage2_type, best_stage2_type, best_fails;
	double	best_efficiency, best_poly_efficiency;
	char	msgbuf[2000];

	forced_stage2_type = IniGetInt (INI_FILE, "ForceP1Stage2Type", 99);		
	memory = 0;
	best_fftlen = 0;
	best_fails = 0;
	best_poly_efficiency = 0.0;
	msgbuf[0] = 0;

	for (bool found_best = FALSE; ; ) {

		polymult_init (&pm1data.polydata, &pm1data.gwdata);
		polymult_set_max_num_threads (&pm1data.polydata, pm1data.stage2_threads);
		polymult_default_tuning (&pm1data.polydata,
					 IniGetInt (INI_FILE, "PolymultCacheSize", CPU_NUM_L2_CACHES > 0 ? CPU_TOTAL_L2_CACHE_SIZE / CPU_NUM_L2_CACHES : 256),
					 IniGetInt (INI_FILE, "PolymultCacheSize2", CPU_NUM_L3_CACHES > 0 ? CPU_TOTAL_L3_CACHE_SIZE / CPU_NUM_L3_CACHES : 6144));
		pm1data.gwdata.scramble_arrays = (char) IniGetInt (INI_FILE, "PolymultScramble", pm1data.gwdata.scramble_arrays);
		pm1data.polydata.strided_writes_end = IniGetInt64 (INI_FILE, "PolymultStridedWritesEnd", pm1data.polydata.strided_writes_end);
		pm1data.polydata.streamed_stores_start = IniGetInt64 (INI_FILE, "PolymultStreamStores", 0); 
		gwset_polymult_safety_margin (&pm1data.gwdata, 0.0);		

		if (gwfftlen (&pm1data.gwdata) > maxerr_fftlen) {
			double efficiency;

			if (memory == 0) {
				unsigned int min_memory, desired_memory;	
				min_memory = 1 + cvt_gwnums_to_mem (&pm1data.gwdata, 8.5);
				if (pm1data.state < PM1_STATE_STAGE2 || pm1data.pairmap == NULL) desired_memory = 3 + cvt_gwnums_to_mem (&pm1data.gwdata, 144);
				else desired_memory = (unsigned int) (pm1data.pairmap_size >> 20) + cvt_gwnums_to_mem (&pm1data.gwdata, pm1data.stage2_numvals);
				stop_reason = avail_mem (thread_num, min_memory, desired_memory, &memory);
				if (stop_reason) {
					if (pm1data.state == PM1_STATE_MIDSTAGE) pm1_save (&pm1data);
					goto exit;
				}

				memory = (unsigned int) (pm1data.pct_mem_to_use * (double) memory);
				if (memory < min_memory) {
					stop_reason = avail_mem_not_sufficient (thread_num, min_memory, desired_memory);
					if (pm1data.state == PM1_STATE_MIDSTAGE) pm1_save (&pm1data);
					goto exit;
				}

				sprintf (buf, "Available memory is %uMB.\n", memory);
				OutputStr (thread_num, buf);
			}

			efficiency = pm1_stage2_impl (&pm1data, memory, found_best ? best_stage2_type : best_fftlen ? 1 : forced_stage2_type, msgbuf + strlen (msgbuf));

			if (pm1data.pairmap != NULL) break;

			efficiency *= 1.0e13;
if (IniGetInt (INI_FILE, "PolyVerbose", 0)) {
if (efficiency == 0.0) sprintf (buf, "FFT: %d no stage 2 plan works\n", (int) gwfftlen (&pm1data.gwdata));
else sprintf (buf, "FFT: %d, B2: %" PRIu64 "/%" PRIu64 ", numvals: %" PRIu64 "/%d, poly: %" PRIu64 "x%" PRIu64 ", efficiency: %.0f\n", (int) gwfftlen (&pm1data.gwdata),
	      pm1data.C, pm1data.B2_start + pm1data.numDsections * pm1data.D,
	      pm1data.stage2_numvals, (int) cvt_mem_to_gwnums_adj (&pm1data.gwdata, memory, -0.5), pm1data.poly1_size, pm1data.poly2_size, efficiency);
OutputStr (thread_num, buf); }

			if (found_best) break;

			if (efficiency != 0.0 && (best_fftlen == 0 || efficiency > best_efficiency)) {
				best_fftlen = gwfftlen (&pm1data.gwdata);
				best_stage2_type = pm1data.stage2_type;
				best_efficiency = efficiency;
			}

			if (pm1data.stage2_type == PM1_STAGE2_POLYMULT && efficiency > best_poly_efficiency) {
				best_poly_efficiency = efficiency;
				best_fails = 0;
			}
		}

		if (best_fftlen && forced_stage2_type == 0) found_best = TRUE;

		if (best_fftlen && (double) memory * 1000000.0 / (double) array_gwnum_size (&pm1data.gwdata) < 60.0) found_best = TRUE;

		if (best_poly_efficiency != 0.0) best_fails++;
		if (best_fails == 3) found_best = TRUE;

		for ( ; ; ) {
			unsigned long next_fftlen = gwfftlen (&pm1data.gwdata) + 1;  
			if (next_fftlen < maxerr_fftlen) next_fftlen = maxerr_fftlen;
			if (found_best) next_fftlen = best_fftlen;
			msgbuf[0] = 0;
			
			if (next_fftlen == gwfftlen (&pm1data.gwdata)) break;
			
			polymult_done (&pm1data.polydata);
			gwdone (&pm1data.gwdata);
			pm1data.x = NULL;
			pm1data.invx = NULL;
			
			gwinit (&pm1data.gwdata);
			if (next_fftlen != best_fftlen) gwset_information_only (&pm1data.gwdata);
			if (IniGetInt (INI_FILE, "UseLargePages", 0)) gwset_use_large_pages (&pm1data.gwdata);
			if (HYPERTHREAD_LL) sp_info->normal_work_hyperthreading = TRUE, gwset_will_hyperthread (&pm1data.gwdata, 2);
			gwset_bench_cores (&pm1data.gwdata, HW_NUM_CORES);
			gwset_bench_workers (&pm1data.gwdata, NUM_WORKERS);
			if (ERRCHK) gwset_will_error_check (&pm1data.gwdata);
			else gwset_will_error_check_near_limit (&pm1data.gwdata);
			gwset_num_threads (&pm1data.gwdata, pm1data.stage2_threads);
			gwset_thread_callback (&pm1data.gwdata, SetAuxThreadPriority);
			gwset_thread_callback_data (&pm1data.gwdata, sp_info);
			gwset_minimum_fftlen (&pm1data.gwdata, next_fftlen);
			gwset_using_polymult (&pm1data.gwdata);
			gwset_use_spin_wait (&pm1data.gwdata, IniGetInt (INI_FILE, "SpinWait", 0));
			res = gwsetup (&pm1data.gwdata, w->k, w->b, w->n, w->c);
			if (res == GWERROR_TOO_LARGE && best_fftlen) {
				found_best = TRUE;
				continue;
			}
			if (res) {
				sprintf (buf, "Cannot initialize FFT code, errcode=%d\n", res);
				OutputBoth (thread_num, buf);
				stop_reason = STOP_FATAL_ERROR;
				goto exit;
			}
			gwerror_checking (&pm1data.gwdata, ERRCHK || near_fft_limit);
			if (gwfftlen (&pm1data.gwdata) != pm1data.stage1_fftlen) {
				char	fft_desc[200];
				gwfft_description (&pm1data.gwdata, fft_desc);
				sprintf (msgbuf, "Switching to %s\n", fft_desc);
			}
			break;
		}
	}

	OutputStr (thread_num, msgbuf);

	if (pm1data.x == NULL) {
		pm1data.x = gwalloc (&pm1data.gwdata);
		if (pm1data.x == NULL) goto oom;
		gianttogw (&pm1data.gwdata, pm1data.x_binary, pm1data.x);
	}
	if (pm1data.invx == NULL) {
		pm1data.invx = gwalloc (&pm1data.gwdata);
		if (pm1data.invx == NULL) goto oom;
		gianttogw (&pm1data.gwdata, pm1data.invx_binary, pm1data.invx);
	}

	pm1data.V = gwalloc (&pm1data.gwdata);
	if (pm1data.V == NULL) goto oom;
	gwadd3o (&pm1data.gwdata, pm1data.x, pm1data.invx, pm1data.V, GWADD_SQUARE_INPUT);

	gwsetaddin (&pm1data.gwdata, -2);

	memused = cvt_array_gwnums_to_mem (&pm1data.gwdata, pm1data.stage2_numvals);
	memused += 2 * (int) pm1data.gwdata.bit_length >> 23;
	memused += (int) (pm1data.pairmap_size >> 20);
	
	if (set_memory_usage (thread_num, MEM_VARIABLE_USAGE, memused)) {
		pm1data.pct_mem_to_use *= 0.99;
		free (pm1data.pairmap); pm1data.pairmap = NULL;
		goto replan;
	}

	if (pm1data.stage2_type == PM1_STAGE2_POLYMULT) goto pm1_polymult;

	polymult_done (&pm1data.polydata);

	sprintf (buf, "Using %uMB of memory.\n", memused);
	OutputStr (thread_num, buf);

	gwfree (&pm1data.gwdata, pm1data.x), pm1data.x = NULL;
	gwfree (&pm1data.gwdata, pm1data.invx), pm1data.invx = NULL;

	if (pm1data.pairmap == NULL) {
		int fill_window = pair_window_size (pm1data.gwdata.bit_length, pm1data.relp_sets);
		gwfree_internal_memory (&pm1data.gwdata);
		stop_reason = fill_pairmap (pm1data.thread_num, &pm1data.sieve_info, pm1data.D, fill_window,0,0,0,
					    pm1data.totrels, pm1data.relp_sets+3, pm1data.first_relocatable, pm1data.last_relocatable,
					    pm1data.B2_start, pm1data.C, pm1data.max_pairmap_Dsections, &pm1data.pairmap, &pm1data.pairmap_size);
		if (stop_reason) goto exit;
		pm1data.pairmap_ptr = pm1data.pairmap;
		pm1data.Dsection = 0;
		pm1data.relp = -1;			
	}

	process_relp_sets (pm1data.totrels, pm1data.numrels, pm1data.relp_sets, relp_set_map, Dmultiple_map);

	base_pct_complete = ((double) pm1data.B2_start - (double) pm1data.last_relocatable) / ((double) pm1data.C - (double) pm1data.last_relocatable);
	
	one_relp_pct = (1.0 - base_pct_complete) / (double) (pm1data.numDsections * pm1data.totrels);
	
	w->pct_complete = base_pct_complete + (double) (pm1data.Dsection * pm1data.totrels) * one_relp_pct;

	int	num_relp_sets;		
	num_relp_sets = (int) relp_set_map.size();
	pm1data.nQx = (gwnum *) malloc (num_relp_sets * pm1data.numrels * sizeof (gwnum));
	if (pm1data.nQx == NULL) goto lowmem;

	int	set_minus1_nQx_index;	
	set_minus1_nQx_index = relp_set_map.find(-1)->second.nQx_store;
	
	#define nqxmap(relp)	((relp) < pm1data.numrels ? (relp) : set_minus1_nQx_index + (relp) - pm1data.numrels)

	ASSERTG (pm1data.D % 3 == 0);
	{
		gwnum	t1, t2, t3, t4, t5;
		gwnum	V1, V2, V3, V5, V6, V7, V11;
		struct {
			gwnum	Vi;
			gwnum	Vi_minus6;
			int	Vi_is_relp;
			int	Vi_minus6_is_relp;
		} V1mod6, V5mod6, *curr, *notcurr;
		int	i_gap, totrels, have_VD;

		t1 = gwalloc (&pm1data.gwdata);
		if (t1 == NULL) goto lowmem;
		t2 = gwalloc (&pm1data.gwdata);
		if (t2 == NULL) goto lowmem;
		t3 = gwalloc (&pm1data.gwdata);
		if (t3 == NULL) goto lowmem;
		t4 = gwalloc (&pm1data.gwdata);
		if (t4 == NULL) goto lowmem;
		t5 = gwalloc (&pm1data.gwdata);
		if (t5 == NULL) goto lowmem;

		luc_dbl (&pm1data.gwdata, pm1data.V, t1);			
		gwswap (t1, pm1data.V);
		V1 = t1;
		V2 = pm1data.V;

		V3 = t2;
		luc_add (&pm1data.gwdata, V2, V1, V1, V3);			

		V5 = t3;
		luc_add (&pm1data.gwdata, V3, V2, V1, V5);			

		V6 = V3;
		luc_dbl (&pm1data.gwdata, V3, V6);				

		V7 = t4;
		luc_add (&pm1data.gwdata, V6, V1, V5, V7);			

		V11 = t5;
		luc_add (&pm1data.gwdata, V6, V5, V1, V11);			

		V1mod6.Vi_minus6 = V1;
		V1mod6.Vi_minus6_is_relp = 1;
		V5mod6.Vi_minus6 = V5;
		V5mod6.Vi_minus6_is_relp = relatively_prime (5, pm1data.D);
		V1mod6.Vi = V7;
		V1mod6.Vi_is_relp = relatively_prime (7, pm1data.D);
		V5mod6.Vi = V11;
		V5mod6.Vi_is_relp = relatively_prime (11, pm1data.D);
		pm1data.nQx[0] = V1; totrels = 1;
		if (V5mod6.Vi_minus6_is_relp) pm1data.nQx[totrels++] = V5;
		if (V1mod6.Vi_is_relp) pm1data.nQx[totrels++] = V7;
		if (V5mod6.Vi_is_relp) pm1data.nQx[totrels++] = V11;

		have_VD = FALSE;
		for (i = 7, i_gap = 4; ; i += i_gap, i_gap = 6 - i_gap) {
			gwnum	next_i;
			int	next_i_is_relp;

			curr = (i_gap == 4) ? &V1mod6 : &V5mod6;

			if (i + (i + i_gap) == pm1data.D) {
				gwnum	Vgap = V2;
				ASSERTG (Vgap == pm1data.V);
				if (i_gap == 4) luc_dbl (&pm1data.gwdata, Vgap, Vgap);			
				luc_add (&pm1data.gwdata, V1mod6.Vi, V5mod6.Vi, Vgap, pm1data.V);	
				have_VD = TRUE;
			}

			if (have_VD && (totrels == pm1data.totrels || totrels == pm1data.numrels * 2)) break;

			next_i_is_relp = (relatively_prime (i+6, pm1data.D) && totrels < pm1data.totrels);

			if ((next_i_is_relp && have_VD && (totrels+1 == pm1data.totrels || totrels+1 == pm1data.numrels * 2)) ||
			    (!have_VD && i+6 > pm1data.D/2 && pm1data.totrels == pm1data.numrels)) {
				notcurr = (curr == &V5mod6) ? &V1mod6 : &V5mod6;
				if (!notcurr->Vi_is_relp) gwfree (&pm1data.gwdata, notcurr->Vi), notcurr->Vi = NULL;
				if (!notcurr->Vi_minus6_is_relp) gwfree (&pm1data.gwdata, notcurr->Vi_minus6), notcurr->Vi_minus6 = NULL;
			}			

			if (curr->Vi_minus6_is_relp) {
				next_i = gwalloc (&pm1data.gwdata);
				if (next_i == NULL) goto lowmem;
			} else
				next_i = curr->Vi_minus6;

			luc_add (&pm1data.gwdata, curr->Vi, V6, curr->Vi_minus6, next_i);	
			if (next_i_is_relp) {
				pm1data.nQx[nqxmap(totrels)] = next_i;
				totrels++;
			}

			curr->Vi_minus6 = curr->Vi;
			curr->Vi_minus6_is_relp = curr->Vi_is_relp;
			curr->Vi = next_i;
			curr->Vi_is_relp = next_i_is_relp;

			if (gw_test_for_error (&pm1data.gwdata)) goto err;
			stop_reason = stopCheck (thread_num);
			if (stop_reason) goto possible_lowmem;
		}

		if (!V1mod6.Vi_is_relp) gwfree (&pm1data.gwdata, V1mod6.Vi);
		if (!V1mod6.Vi_minus6_is_relp) gwfree (&pm1data.gwdata, V1mod6.Vi_minus6);
		if (!V5mod6.Vi_is_relp) gwfree (&pm1data.gwdata, V5mod6.Vi);
		if (!V5mod6.Vi_minus6_is_relp) gwfree (&pm1data.gwdata, V5mod6.Vi_minus6);
		gwfree (&pm1data.gwdata, V6);
	}
	#undef nqxmap

	{
		auto it_Dmult = Dmultiple_map.find (1);
		if (it_Dmult == Dmultiple_map.end()) it_Dmult = Dmultiple_map.insert({1, {0, 1}}).first;
		it_Dmult->second.set_val_buffers (&pm1data.gwdata, pm1data.V, TRUE);

		uint64_t D_multiplier = (pm1data.B2_start + pm1data.D / 2) / pm1data.D + pm1data.Dsection;
		ASSERTG (D_multiplier > 2); 
		it_Dmult = Dmultiple_map.find (D_multiplier - 1);
		if (it_Dmult == Dmultiple_map.end()) it_Dmult = Dmultiple_map.insert({D_multiplier - 1, {0, D_multiplier - 1}}).first;
		pm1data.Vn = gwalloc (&pm1data.gwdata);
		if (pm1data.Vn == NULL) goto lowmem;
		it_Dmult->second.set_val_buffers (&pm1data.gwdata, pm1data.Vn, TRUE);

		it_Dmult = Dmultiple_map.find (D_multiplier);
		if (it_Dmult == Dmultiple_map.end()) it_Dmult = Dmultiple_map.insert({D_multiplier, {0, D_multiplier}}).first;
		pm1data.Vn1 = gwalloc (&pm1data.gwdata);
		if (pm1data.Vn1 == NULL) goto lowmem;
		it_Dmult->second.set_val_buffers (&pm1data.gwdata, pm1data.Vn1, TRUE);
	}

	process_Dmult_map (Dmultiple_map);
	for (auto this_Dmult = Dmultiple_map.begin(); this_Dmult != Dmultiple_map.end(); ++this_Dmult) {
		
		if (this_Dmult->first == 1) continue;
		
		if (this_Dmult->second.val.x == NULL) {
			gwnum	tmp = gwalloc (&pm1data.gwdata);
			if (tmp == NULL) goto lowmem;
			this_Dmult->second.set_val_buffers (&pm1data.gwdata, tmp, FALSE);
		}
		
		auto it_base = Dmultiple_map.find (this_Dmult->second.base);
		if (this_Dmult->second.diff == 0) {
			luc_dbl (&pm1data.gwdata, it_base->second.val.x, this_Dmult->second.val.x);
		}
		
		else {
			auto it_addin = Dmultiple_map.find (this_Dmult->second.addin);
			auto it_diff = Dmultiple_map.find (this_Dmult->second.diff);
			luc_add (&pm1data.gwdata, it_base->second.val.x, it_addin->second.val.x, it_diff->second.val.x, this_Dmult->second.val.x);
			
			it_addin->second.free_if_Dmult_last_used_by (this_Dmult->first);
			it_diff->second.free_if_Dmult_last_used_by (this_Dmult->first);
		}
		
		it_base->second.free_if_Dmult_last_used_by (this_Dmult->first);
		
		if (gw_test_for_error (&pm1data.gwdata)) goto err;
		stop_reason = stopCheck (thread_num);
		if (stop_reason) goto possible_lowmem;
	}

	gwfree_internal_memory (&pm1data.gwdata);

	int	num_partials;
	num_partials = one_based_modulo (pm1data.totrels, pm1data.numrels);
	
	for (auto it_relp_set = relp_set_map.begin (); it_relp_set != relp_set_map.end (); ++it_relp_set) {
		
		if (it_relp_set->first == 0 || it_relp_set->first == -1) continue;
		
		auto it_base_relp_set = relp_set_map.find (it_relp_set->second.base);
		auto it_diff_relp_set = relp_set_map.find (it_relp_set->second.diff);
		
		auto it_Dmultiple = Dmultiple_map.find (it_relp_set->second.Dmultiple);
		
		bool	diff_access_inverted = (it_base_relp_set->first < 0) ^ (it_diff_relp_set->first < 0);
		
		for (i = (it_relp_set->second.partial ? num_partials : pm1data.numrels) - 1; i >= 0; i--) {
			gwnum	*Vbase, *Vdiff, Vnew;
			int	ni;

			ni = it_base_relp_set->second.nQx_store + i;
			Vbase = &pm1data.nQx[ni];
			
			ni = it_diff_relp_set->second.nQx_store;
			if (!diff_access_inverted) ni += i; else ni += (pm1data.numrels-1)-i;
			Vdiff = &pm1data.nQx[ni];

			bool Vbase_to_be_freed = (!it_base_relp_set->second.permanent &&
						  it_base_relp_set->second.is_last_use (it_relp_set->first, i, num_partials, pm1data.numrels, FALSE));
			bool Vdiff_to_be_freed = (!it_diff_relp_set->second.permanent &&
						  it_diff_relp_set->second.is_last_use (it_relp_set->first, i, num_partials, pm1data.numrels, diff_access_inverted));
			bool Dmultiple_to_be_freed = (i == 0 && it_Dmultiple->second.will_free_due_to_relp_last_used_by (it_relp_set->first));

			if (Vbase_to_be_freed) Vnew = *Vbase;
			else if (Vdiff_to_be_freed) Vnew = *Vdiff;
			else if (Dmultiple_to_be_freed) Vnew = it_Dmultiple->second.val.x;
			else {
				Vnew = gwalloc (&pm1data.gwdata);
				if (Vnew == NULL) goto lowmem;
			}

			luc_add (&pm1data.gwdata, *Vbase, it_Dmultiple->second.val.x, *Vdiff, Vnew);	

			ni = it_relp_set->second.nQx_store + i;
			pm1data.nQx[ni] = Vnew;

			if (Vbase_to_be_freed && Vnew != *Vbase) {
				gwfree (&pm1data.gwdata, *Vbase);
				*Vbase = NULL;
			}

			if (Vdiff_to_be_freed && Vnew != *Vdiff) {
				gwfree (&pm1data.gwdata, *Vdiff);
				*Vdiff = NULL;
			}

			if (Dmultiple_to_be_freed) {
				it_Dmultiple->second.do_not_free_val = (Vnew == it_Dmultiple->second.val.x);
				it_Dmultiple->second.free_if_relp_last_used_by (it_relp_set->first);
			}

			if (gw_test_for_error (&pm1data.gwdata)) goto err;
			stop_reason = stopCheck (thread_num);
			if (stop_reason) goto possible_lowmem;
		}
	}

	Dmultiple_map.clear ();
	relp_set_map.clear ();

	ASSERTG (pm1data.gg == NULL);
	pm1data.gg = gwalloc (&pm1data.gwdata);
	if (pm1data.gg == NULL) goto lowmem;
	gianttogw (&pm1data.gwdata, pm1data.gg_binary, pm1data.gg);
	free (pm1data.gg_binary), pm1data.gg_binary = NULL;

	gwfree_cached (&pm1data.gwdata);
	mallocFreeForOS ();

	sprintf (buf, "%.*f%% of %s P-1 stage 2 (using %uMB)",
		 (int) PRECISION, trunc_percent (w->pct_complete), gwmodulo_as_string (&pm1data.gwdata), memused);
	title (thread_num, buf);

	end_timer (timers, 0);
	sprintf (buf, "Stage 2 init complete. %" PRIu64 " transforms. Time: ", gw_get_fft_count (&pm1data.gwdata));
	print_timer (timers, 0, buf, TIMER_NL | TIMER_CLR);
	OutputStr (thread_num, buf);
	gw_clear_fft_count (&pm1data.gwdata);

	ASSERTG (pm1data.interim_C <= pm1data.B2_start + pm1data.numDsections * pm1data.D);
	pm1data.interim_C = pm1data.B2_start + pm1data.numDsections * pm1data.D;

	pm1data.state = PM1_STATE_STAGE2;
	start_timer_from_zero (timers, 0);
	start_timer_from_zero (timers, 1);
	last_output = last_output_t = last_output_r = 0;
	for ( ; ; ) {

		pm1data.relp += next_pair (&pm1data.pairmap_ptr);

		if (pm1data.relp >= pm1data.totrels && pm1data.Dsection + pm1data.relp / pm1data.totrels >= pm1data.numDsections) break;

		while (pm1data.relp >= pm1data.totrels) {
			pm1data.Dsection++;
			pm1data.relp -= pm1data.totrels;
			luc_add (&pm1data.gwdata, pm1data.Vn1, pm1data.V, pm1data.Vn, pm1data.Vn);	
			gwswap (pm1data.Vn, pm1data.Vn1);						

		}

		if (pm1data.pairmap_ptr == pm1data.pairmap + pm1data.pairmap_size) {
			ASSERTG (pm1data.relp == 0);
			pm1data.C_done = pm1data.B2_start + pm1data.Dsection * pm1data.D;
			pm1data.first_relocatable = calc_new_first_relocatable (pm1data.D, pm1data.C_done);
			int fill_window = pair_window_size (pm1data.gwdata.bit_length, pm1data.relp_sets);
			gwfree_internal_memory (&pm1data.gwdata);
			stop_reason = fill_pairmap (pm1data.thread_num, &pm1data.sieve_info, pm1data.D, fill_window,0,0,0,
						    pm1data.totrels, pm1data.relp_sets+3, pm1data.first_relocatable, pm1data.last_relocatable,
						    pm1data.C_done, pm1data.C, pm1data.max_pairmap_Dsections, &pm1data.pairmap, &pm1data.pairmap_size);
			if (stop_reason) {

				pm1_save (&pm1data);
				goto exit;
			}
			pm1data.pairmap_ptr = pm1data.pairmap;
			pm1data.relp = -1;
			continue;
		}

		saving = testSaveFilesFlag (thread_num);
		stop_reason = stopCheck (thread_num);
		gwsubmul4 (&pm1data.gwdata, pm1data.Vn1, pm1data.nQx[pm1data.relp], pm1data.gg, pm1data.gg, (!stop_reason && !saving) ? GWMUL_STARTNEXTFFT : 0);

		w->pct_complete = base_pct_complete + (double) (pm1data.Dsection * pm1data.totrels + pm1data.relp) * one_relp_pct;

		if (gw_test_for_error (&pm1data.gwdata) || gw_get_maxerr (&pm1data.gwdata) > allowable_maxerr) goto err;

		if (first_iter_msg ||
		    (ITER_OUTPUT != 999999999 && gw_get_fft_count (&pm1data.gwdata) >= last_output_t + 2 * ITER_OUTPUT * output_title_frequency)) {
			sprintf (buf, "%.*f%% of %s P-1 stage 2 (using %uMB)",
				 (int) PRECISION, trunc_percent (w->pct_complete), gwmodulo_as_string (&pm1data.gwdata), memused);
			title (thread_num, buf);
			last_output_t = gw_get_fft_count (&pm1data.gwdata);
		}

		if (first_iter_msg ||
		    (ITER_OUTPUT != 999999999 && gw_get_fft_count (&pm1data.gwdata) >= last_output + 2 * ITER_OUTPUT * output_frequency)) {
			sprintf (buf, "%s stage 2 is %.*f%% complete.",
				 gwmodulo_as_string (&pm1data.gwdata), (int) PRECISION, trunc_percent (w->pct_complete));
			end_timer (timers, 0);
			if (first_iter_msg) {
				strcat (buf, "\n");
			} else {
				strcat (buf, " Time: ");
				print_timer (timers, 0, buf, TIMER_NL);
			}
			OutputStr (thread_num, buf);
			start_timer_from_zero (timers, 0);
			last_output = gw_get_fft_count (&pm1data.gwdata);
			first_iter_msg = FALSE;
		}

		if ((ITER_OUTPUT_RES != 999999999 && gw_get_fft_count (&pm1data.gwdata) >= last_output_r + 2 * ITER_OUTPUT_RES) ||
		    (NO_GUI && stop_reason)) {
			sprintf (buf, "%s stage 2 is %.*f%% complete.\n",
				 gwmodulo_as_string (&pm1data.gwdata), (int) PRECISION, trunc_percent (w->pct_complete));
			writeResults (buf);
			last_output_r = gw_get_fft_count (&pm1data.gwdata);
		}

		if (stop_reason || saving) {
			pm1_save (&pm1data);
			if (stop_reason) goto exit;
		}
	}
	pm1data.C_done = pm1data.interim_C;

	for (i = 0; i < pm1data.totrels; i++) gwfree (&pm1data.gwdata, pm1data.nQx[i]);
	free (pm1data.nQx), pm1data.nQx = NULL;
	free (pm1data.pairmap), pm1data.pairmap = NULL;

	if (pm1data.C > pm1data.C_done) {
		pm1data.state = PM1_STATE_MIDSTAGE;
		goto more_C;
	}

	goto stage2_done;

pm1_polymult:
	uint64_t outpoly_size;			
	gwnum	*poly1, *poly2, *outpoly, r, invr, e;

	ASSERTG (pm1data.poly2_size > pm1data.poly1_size*2);
	
	outpoly_size = polymult_fft_size (pm1data.poly2_size);
	
	pm1data.num_points = pm1data.poly2_size - (2*pm1data.poly1_size+1) + 1;
	ASSERTG (pm1data.numDsections % pm1data.num_points == 0);

	end_sieve (pm1data.sieve_info), pm1data.sieve_info = NULL;

	sprintf (buf, "Using %uMB of memory.  D: %d, %" PRIu64 "x%" PRIu64 " polynomial multiplication.\n", memused, pm1data.D, pm1data.poly1_size, pm1data.poly2_size);
	OutputStr (thread_num, buf);

	poly1 = gwalloc_array (&pm1data.gwdata, pm1data.poly1_size);	
	if (poly1 == NULL) goto lowmem;
	
	pm1data.nQx = (gwnum *) malloc ((size_t) (outpoly_size * sizeof (gwnum)));
	if (pm1data.nQx == NULL) goto lowmem;
	outpoly = pm1data.nQx;					
	memset (outpoly, 0, (size_t) (outpoly_size * sizeof (gwnum)));

	ASSERTG (pm1data.D % 3 == 0);
	ASSERTG (pm1data.D % 4 == 2);			
	{
		gwnum	t1, t2, V1, V2, V3, V5, V6;
		struct {
			gwnum	Vi;
			gwnum	Vi_minus6;
		} V1mod6, V5mod6, *curr;
		int	i_gap, totrels;

		t1 = gwalloc (&pm1data.gwdata);
		if (t1 == NULL) goto lowmem;
		t2 = gwalloc (&pm1data.gwdata);
		if (t2 == NULL) goto lowmem;

		V1 = pm1data.V, pm1data.V = NULL;
		V2 = t1;
		luc_dbl (&pm1data.gwdata, V1, V2);				

		V3 = t2;
		luc_add (&pm1data.gwdata, V2, V1, V1, V3);			

		V5 = V2;
		luc_add (&pm1data.gwdata, V3, V2, V1, V5);			

		V6 = V3;
		luc_dbl (&pm1data.gwdata, V3, V6);				

		V1mod6.Vi = V1;
		V1mod6.Vi_minus6 = V5;
		V5mod6.Vi = V5;
		V5mod6.Vi_minus6 = V1;
		totrels = 0;

		for (i = 1, i_gap = 4; ; i += i_gap, i_gap = 6 - i_gap) {
			gwnum	next_i;

			curr = (i_gap == 4) ? &V1mod6 : &V5mod6;

			if (relatively_prime (i, pm1data.D)) {
				gwnum	V_to_double = curr->Vi;
				for (int k = 1; i * k <= pm1data.D / 4; k *= 2) {
					luc_dbl (&pm1data.gwdata, V_to_double, poly1[totrels]);
					V_to_double = poly1[totrels];
					totrels++;
				}
				if (totrels >= pm1data.numrels) break;
			}

			if (i < 6) {
				next_i = gwalloc (&pm1data.gwdata);
				if (next_i == NULL) goto lowmem;
			} else
				next_i = curr->Vi_minus6;

			luc_add (&pm1data.gwdata, curr->Vi, V6, curr->Vi_minus6, next_i);	

			curr->Vi_minus6 = curr->Vi;
			curr->Vi = next_i;

			if (gw_test_for_error (&pm1data.gwdata)) goto err;
			stop_reason = stopCheck (thread_num);
			if (stop_reason) goto possible_lowmem;
		}
		ASSERTG (totrels == pm1data.numrels && i <= pm1data.D/4);

		gwfree (&pm1data.gwdata, V1mod6.Vi);
		gwfree (&pm1data.gwdata, V1mod6.Vi_minus6);
		gwfree (&pm1data.gwdata, V5mod6.Vi);
		gwfree (&pm1data.gwdata, V5mod6.Vi_minus6);
		gwfree (&pm1data.gwdata, V6);
	}

		pm1data.helper_count = pm1data.stage2_threads;

	gwset_polymult_safety_margin (&pm1data.gwdata, polymult_safety_margin (pm1data.poly1_size, pm1data.poly2_size));

	{
		int	num_polys = pm1data.numrels;	
		int	poly_size = 1;			
		int	last_poly_size = 1;		
		while (num_polys > 1) {
			for (int i = 0; i < num_polys-1; i += 2) {
				int poly2_size = (i+1 < num_polys-1) ? poly_size : last_poly_size;
				polymult (&pm1data.polydata, &poly1[i*poly_size], poly_size, &poly1[(i+1)*poly_size], poly2_size,
					  &poly1[i*poly_size], poly_size + poly2_size,
					  (poly_size == 1 ? POLYMULT_INVEC1_NEGATE : 0) +
					  (poly2_size == 1 ? POLYMULT_INVEC2_NEGATE : 0) +
					  POLYMULT_INVEC1_MONIC_RLP | POLYMULT_INVEC2_MONIC_RLP | POLYMULT_NO_UNFFT);
			}
			
			poly_unfft_fft_coefficients (&pm1data.polydata, poly1, pm1data.poly1_size);
			
			if ((num_polys & 1) == 0) last_poly_size += poly_size;
			poly_size *= 2;
			num_polys = (num_polys + 1) / 2;
if (IniGetInt (INI_FILE, "PolyVerbose", 0)) {
if (poly_size==2)sprintf (buf, "Round off: %.10g, poly_size: %d, EB: %g, SM: %g\n", gw_get_maxerr (&pm1data.gwdata), poly_size, pm1data.gwdata.EXTRA_BITS, pm1data.gwdata.safety_margin);
else sprintf (buf, "Round off: %.10g, poly_size: %d\n", gw_get_maxerr (&pm1data.gwdata), poly_size);
OutputStr (thread_num, buf); gw_clear_maxerr (&pm1data.gwdata);
}
			stop_reason = stopCheck (thread_num);
			if (stop_reason) goto possible_lowmem;
		}
	}

	r = pm1data.x, pm1data.x = NULL;
	exponentiate (&pm1data.gwdata, r, pm1data.D / 2);
	
	pm1data.r_squared = gwalloc (&pm1data.gwdata);
	if (pm1data.r_squared == NULL) goto lowmem;
	gwsquare2 (&pm1data.gwdata, r, pm1data.r_squared, GWMUL_FFT_S1 | GWMUL_STARTNEXTFFT);
	gwfft (&pm1data.gwdata, pm1data.r_squared, pm1data.r_squared);
	
	invr = pm1data.invx, pm1data.invx = NULL;
	exponentiate (&pm1data.gwdata, invr, pm1data.D / 2);
	{
		
		gwnum diff2 = gwalloc (&pm1data.gwdata);
		if (diff2 == NULL) goto lowmem;
		gwsquare2 (&pm1data.gwdata, invr, diff2, GWMUL_FFT_S1 | GWMUL_STARTNEXTFFT);				
		
		gwnum diff1 = gwalloc (&pm1data.gwdata);
		if (diff1 == NULL) goto lowmem;
		gwcopy (&pm1data.gwdata, invr, diff1);									
		
		gwnum multiplier = gwalloc (&pm1data.gwdata);
		if (multiplier == NULL) goto lowmem;
		if (pm1data.numrels & 1) {		
			gwcopy (&pm1data.gwdata, r, multiplier);
			exponentiate (&pm1data.gwdata, multiplier, (uint64_t) pm1data.numrels * (uint64_t) pm1data.numrels);	
		} else {
			gwcopy (&pm1data.gwdata, pm1data.r_squared, multiplier);
			exponentiate (&pm1data.gwdata, multiplier, (uint64_t) pm1data.numrels * (uint64_t) pm1data.numrels / 2);
		}
		
		gwmul3 (&pm1data.gwdata, poly1[0], multiplier, poly1[0], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);		
		
		gwmul3 (&pm1data.gwdata, multiplier, diff1, multiplier, GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);		
		gwmul3 (&pm1data.gwdata, poly1[1], multiplier, poly1[1], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);		
		
		for (int j = 2; j < pm1data.poly1_size; j++) {
			gwmul3 (&pm1data.gwdata, diff1, diff2, diff1, GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			gwmul3 (&pm1data.gwdata, multiplier, diff1, multiplier, GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			gwmul3 (&pm1data.gwdata, poly1[j], multiplier, poly1[j], GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);
			stop_reason = stopCheck (thread_num);
			if (stop_reason) goto possible_lowmem;
		}
		gwfree (&pm1data.gwdata, diff2);
		gwfree (&pm1data.gwdata, diff1);
		gwfree (&pm1data.gwdata, multiplier);
	}

	if (IniGetInt (INI_FILE, "Poly1Compress", 2)) {
		int options = POLYMULT_INVEC1_MONIC_RLP | POLYMULT_CIRCULAR;
		if (IniGetInt (INI_FILE, "Poly1Compress", 2) == -1) options |= POLYMULT_PRE_FFT;				
		if (IniGetInt (INI_FILE, "Poly1Compress", 2) == -2) options |= POLYMULT_PRE_FFT | POLYMULT_PRE_COMPRESS;	
		if (IniGetInt (INI_FILE, "Poly1Compress", 2) == 2) options |= POLYMULT_PRE_COMPRESS;
		gwarray tmp = polymult_preprocess (&pm1data.polydata, poly1, pm1data.poly1_size, pm1data.poly2_size, outpoly_size, options);
		gwfree_array (&pm1data.gwdata, poly1);
		poly1 = tmp;

	}

	e = r, r = NULL;
	ASSERTG (pm1data.B2_start % pm1data.D == 0);
	exponentiate (&pm1data.gwdata, e, (pm1data.B2_start + pm1data.D / 2) / (pm1data.D / 2));

	exponentiate (&pm1data.gwdata, invr, pm1data.numrels * 2 - 1);
   	pm1data.diff1 = e, gwmul3 (&pm1data.gwdata, e, invr, pm1data.diff1, GWMUL_STARTNEXTFFT), e = NULL;
	gwfree (&pm1data.gwdata, invr), invr = NULL;

	if (pm1data.helper_count == 1) {
		pm1data.r_2helper = pm1data.r_squared;
		pm1data.r_2helper2 = pm1data.r_squared;
	} else {
		pm1data.r_2helper = gwalloc (&pm1data.gwdata);
		if (pm1data.r_2helper == NULL) goto lowmem;
		gwcopy (&pm1data.gwdata, pm1data.r_squared, pm1data.r_2helper);
		exponentiate (&pm1data.gwdata, pm1data.r_2helper, pm1data.helper_count);
		gwfft (&pm1data.gwdata, pm1data.r_2helper, pm1data.r_2helper);
		pm1data.r_2helper2 = gwalloc (&pm1data.gwdata);
		if (pm1data.r_2helper2 == NULL) goto lowmem;
		gwcopy (&pm1data.gwdata, pm1data.r_2helper, pm1data.r_2helper2);
		exponentiate (&pm1data.gwdata, pm1data.r_2helper2, pm1data.helper_count);
		gwfft (&pm1data.gwdata, pm1data.r_2helper2, pm1data.r_2helper2);
	}

	if (pm1data.helper_count == 1) {
		ASSERTG (pm1data.gg == NULL);
		pm1data.gg = gwalloc (&pm1data.gwdata);
		if (pm1data.gg == NULL) goto lowmem;
		gianttogw (&pm1data.gwdata, pm1data.gg_binary, pm1data.gg);
		free (pm1data.gg_binary), pm1data.gg_binary = NULL;
	}

	gwfree_internal_memory (&pm1data.gwdata);
	mallocFreeForOS ();

	poly2 = gwalloc_array (&pm1data.gwdata, pm1data.poly2_size);
	if (poly2 == NULL) goto lowmem;

	sprintf (buf, "%.*f%% of %s P-1 stage 2 (using %uMB)", (int) PRECISION, trunc_percent (w->pct_complete), gwmodulo_as_string (&pm1data.gwdata), memused);
	title (thread_num, buf);

	end_timer (timers, 0);
	sprintf (buf, "Stage 2 init complete. %" PRIu64 " transforms. Time: ", gw_get_fft_count (&pm1data.gwdata));
	print_timer (timers, 0, buf, TIMER_NL | TIMER_CLR);
	OutputStr (thread_num, buf);
	gw_clear_fft_count (&pm1data.gwdata);

	ASSERTG (pm1data.C <= pm1data.B2_start + pm1data.numDsections * pm1data.D);
	ASSERTG (pm1data.interim_C <= pm1data.B2_start + pm1data.numDsections * pm1data.D);
	pm1data.interim_C = pm1data.B2_start + pm1data.numDsections * pm1data.D;

	base_pct_complete = (double) (pm1data.B2_start - pm1data.last_relocatable) /
			    (double) (pm1data.B2_start + pm1data.numDsections * pm1data.D - pm1data.last_relocatable);

	gwsquare2 (&pm1data.gwdata, pm1data.diff1, poly2[pm1data.poly2_size - 2], GWMUL_FFT_S1 | GWMUL_STARTNEXTFFT);
	gwcopy (&pm1data.gwdata, pm1data.diff1, poly2[pm1data.poly2_size - 1]);

	if (pm1data.helper_count > 1) {
		gwnum	small_diff1, big_diff1;
		gwcopy (&pm1data.gwdata, pm1data.diff1, small_diff1 = poly2[0]);	
		big_diff1 = pm1data.diff1;						
		for (int j = 1; j < pm1data.helper_count; j++) {
			
			gwmul3 (&pm1data.gwdata, small_diff1, pm1data.r_squared, small_diff1, GWMUL_STARTNEXTFFT);
			
			gwmul3 (&pm1data.gwdata, poly2[pm1data.poly2_size-1-j], small_diff1, poly2[pm1data.poly2_size-2-j], GWMUL_FFT_S12 | GWMUL_STARTNEXTFFT);
			
			gwmul3 (&pm1data.gwdata, big_diff1, small_diff1, big_diff1, GWMUL_STARTNEXTFFT);
		}
		
		pm1data.gg = pm1data.r_squared, pm1data.r_squared = NULL;
		gianttogw (&pm1data.gwdata, pm1data.gg_binary, pm1data.gg);
		free (pm1data.gg_binary), pm1data.gg_binary = NULL;
	}

	gwfree_cached (&pm1data.gwdata);
	mallocFreeForOS ();

	if (pm1data.B2_start > pm1data.C_done) pm1data.required_missing = map_polyD_to_first_missing_prime (pm1data.D);

	pm1data.state = PM1_STATE_STAGE2;
	start_timer_from_zero (timers, 0);
	start_timer_from_zero (timers, 1);
	last_output = last_output_t = last_output_r = 0;
	for (bool first_polymult = TRUE; ; first_polymult = FALSE) {

		if (first_polymult) {
			
			pm1data.remaining_poly2_size = pm1data.poly2_size - (pm1data.helper_count + 1);
		}

		else {
			
			pm1data.remaining_poly2_size = pm1data.num_points;
		}
		
		gwnum	prev_diff;
		for (int j = 0; j < pm1data.helper_count; j++) {
			gwnum this_diff = poly2[(pm1data.remaining_poly2_size - 1 - j) % pm1data.helper_count];
			if (j == 0) gwmul3 (&pm1data.gwdata, pm1data.diff1, pm1data.r_2helper, this_diff, GWMUL_STARTNEXTFFT);
			else gwmul3 (&pm1data.gwdata, prev_diff, pm1data.r_2helper, this_diff, GWMUL_FFT_S1 | GWMUL_STARTNEXTFFT);
			prev_diff = this_diff;
		}

		pm1data.polydata.helper_callback = &pm1_helper;
		pm1data.polydata.helper_callback_data = &pm1data;
		pm1data.helper_work = PM1_COMPUTE_POLY2;				
		pm1data.poly2 = poly2;							

		polymult_launch_helpers (&pm1data.polydata);

		stop_reason = stopCheck (thread_num);
		if (stop_reason) { pm1_save (&pm1data); goto exit; }

		memcpy (outpoly + 2*pm1data.numrels, poly2 + 2*pm1data.numrels, (size_t) (pm1data.num_points * sizeof (gwnum)));

		polymult (&pm1data.polydata, poly1, pm1data.poly1_size, poly2, pm1data.poly2_size, outpoly, outpoly_size,
			  POLYMULT_INVEC1_MONIC_RLP | POLYMULT_CIRCULAR | POLYMULT_NO_UNFFT);
if (pm1data.Dsection==0 && IniGetInt (INI_FILE, "PolyVerbose", 0)) {
sprintf (buf, "Round off: %.10g\n", gw_get_maxerr (&pm1data.gwdata));
OutputStr (thread_num, buf); gw_clear_maxerr (&pm1data.gwdata);}

		if (gw_test_for_error (&pm1data.gwdata)) goto err;
		stop_reason = stopCheck (thread_num);
		saving = testSaveFilesFlag (thread_num);

		pm1data.polydata.helper_callback = &pm1_helper;
		pm1data.polydata.helper_callback_data = &pm1data;
		pm1data.helper_work = PM1_ACCUMULATING_GG;		
		pm1data.points = poly2 + 2*pm1data.numrels;		

			polymult_launch_helpers (&pm1data.polydata);

		pm1data.Dsection += pm1data.num_points;
		if (pm1data.Dsection >= pm1data.numDsections && pm1data.B2_start + pm1data.Dsection * pm1data.D >= pm1data.C) break;

		w->pct_complete = base_pct_complete + (1.0 - base_pct_complete) * (double) pm1data.Dsection / (double) pm1data.numDsections;

		if (gw_test_for_error (&pm1data.gwdata) || gw_get_maxerr (&pm1data.gwdata) > allowable_maxerr) goto err;

#define ITER_FUDGE 3
		if (first_iter_msg ||
		    (ITER_OUTPUT != 999999999 && gw_get_fft_count (&pm1data.gwdata) * ITER_FUDGE >= last_output_t + 2 * ITER_OUTPUT * output_title_frequency)) {
			sprintf (buf, "%.*f%% of %s P-1 stage 2 (using %uMB)",
				 (int) PRECISION, trunc_percent (w->pct_complete), gwmodulo_as_string (&pm1data.gwdata), memused);
			title (thread_num, buf);
			last_output_t = gw_get_fft_count (&pm1data.gwdata) * ITER_FUDGE;
		}

		if (first_iter_msg ||
		    (ITER_OUTPUT != 999999999 && gw_get_fft_count (&pm1data.gwdata) * ITER_FUDGE >= last_output + 2 * ITER_OUTPUT * output_frequency)) {
			sprintf (buf, "%s stage 2 at B2=%" PRIu64 " [%.*f%%]",
				 gwmodulo_as_string (&pm1data.gwdata), pm1data.B2_start + pm1data.Dsection * pm1data.D,
				 (int) PRECISION, trunc_percent (w->pct_complete));
			end_timer (timers, 0);
			if (first_iter_msg) {
				strcat (buf, "\n");
			} else {
				strcat (buf, ".  Time: ");
				print_timer (timers, 0, buf, TIMER_NL);
			}
			OutputStr (thread_num, buf);
			start_timer_from_zero (timers, 0);
			last_output = gw_get_fft_count (&pm1data.gwdata) * ITER_FUDGE;
			first_iter_msg = FALSE;
		}

		if ((ITER_OUTPUT_RES != 999999999 && gw_get_fft_count (&pm1data.gwdata) * ITER_FUDGE >= last_output_r + 2 * ITER_OUTPUT_RES) ||
		    (NO_GUI && stop_reason)) {
			sprintf (buf, "%s stage 2 at B2=%" PRIu64 " [%.*f%%]\n",
				 gwmodulo_as_string (&pm1data.gwdata), pm1data.B2_start + pm1data.Dsection * pm1data.D,
				 (int) PRECISION, trunc_percent (w->pct_complete));
			writeResults (buf);
			last_output_r = gw_get_fft_count (&pm1data.gwdata) * ITER_FUDGE;
		}

		if (stop_reason || saving) {
			pm1_save (&pm1data);
			if (stop_reason) goto exit;
		}

		memmove (poly2 + pm1data.num_points, poly2, 2*pm1data.numrels * sizeof (gwnum));
		memcpy (poly2, outpoly + 2*pm1data.numrels, (size_t) (pm1data.num_points * sizeof (gwnum)));
	}

	pm1data.C = pm1data.C_done = pm1data.B2_start + pm1data.Dsection * pm1data.D;

	gwfree (&pm1data.gwdata, pm1data.r_squared), pm1data.r_squared = NULL;
	gwfree (&pm1data.gwdata, pm1data.diff1), pm1data.diff1 = NULL;
	polymult_done (&pm1data.polydata);
	gwfree_array (&pm1data.gwdata, poly1);
	gwfree_array (&pm1data.gwdata, poly2);

stage2_done:
	gwfree_internal_memory (&pm1data.gwdata);
	mallocFreeForOS ();
	set_memory_usage (thread_num, 0, cvt_gwnums_to_mem (&pm1data.gwdata, 1));	
	end_timer (timers, 1);
	end_timer (&stage2_timer, 0);
	sprintf (buf, "%s stage 2 complete. %" PRIu64 " transforms. Total time: ", gwmodulo_as_string (&pm1data.gwdata), gw_get_fft_count (&pm1data.gwdata));
	print_timer (timers, 1, buf, TIMER_NL | TIMER_CLR);
	OutputStr (thread_num, buf);

	if (stage1_timer != 0.0 && stage2_timer != 0.0 && IniGetInt (INI_FILE, "Pm1Stage2AutoAdjust", 1)) {
		double correct_ratio = stage2_timer / stage1_timer;
		double current_adjustment = IniGetFloat (INI_FILE, "Pm1Stage2RatioAdjust", 1.0);
		double correct_adjustment = correct_ratio / pm1data.est_stage2_stage1_ratio * current_adjustment;
		
		if (correct_adjustment > 3.0) correct_adjustment = 3.0;
		if (correct_adjustment < 0.333) correct_adjustment = 0.333;
		
		IniWriteFloat (INI_FILE, "Pm1Stage2RatioAdjust", (float) (0.9 * current_adjustment + 0.1 * correct_adjustment));
	}

	if (ERRCHK) {
		sprintf (buf, "Round off: %.10g\n", gw_get_maxerr (&pm1data.gwdata));
		OutputStr (thread_num, buf);
		gw_clear_maxerr (&pm1data.gwdata);
	}

restart4:
	if (pm1data.gg != NULL) {			
		ASSERTG (pm1data.gg_binary == NULL);
		pm1data.gg_binary = allocgiant (((int) pm1data.gwdata.bit_length >> 5) + 10);
		if (pm1data.gg_binary == NULL) goto oom;
		gwtogiant (&pm1data.gwdata, pm1data.gg, pm1data.gg_binary);
		gwfree (&pm1data.gwdata, pm1data.gg), pm1data.gg = NULL;
	}
	pm1data.state = PM1_STATE_GCD;
	strcpy (w->stage, "S2");
	w->pct_complete = 1.0;
	if (w->work_type != WORK_PMINUS1) OutputStr (thread_num, "Starting stage 2 GCD - please be patient.\n");
	start_timer_from_zero (timers, 0);
	stop_reason = gcd (thread_num, pm1data.gg_binary, N, &factor);
	if (stop_reason) {
		pm1_save (&pm1data);
		goto exit;
	}
	pm1data.state = PM1_STATE_DONE;
	end_timer (timers, 0);
	strcpy (buf, "Stage 2 GCD complete. Time: ");
	print_timer (timers, 0, buf, TIMER_NL | TIMER_CLR);
	OutputStr (thread_num, buf);
	if (factor != NULL) goto bingo;

msg_and_exit:
	sprintf (buf, "%s completed P-1, B1=%" PRIu64, gwmodulo_as_string (&pm1data.gwdata), pm1data.B);
	if (pm1data.C > pm1data.B) sprintf (buf+strlen(buf), ", B2=%" PRIu64, pm1data.C);
	sprintf (buf+strlen(buf), ", Wi%d: %08lX\n", PORT, SEC5 (w->n, pm1data.B, pm1data.C));
	OutputStr (thread_num, buf);
	formatMsgForResultsFile (buf, w);
	writeResults (buf);

	strcpy (JSONbuf, "{\"status\":\"NF\"");
	JSONaddExponent (JSONbuf, w);
	strcat (JSONbuf, ", \"worktype\":\"P-1\"");
	sprintf (JSONbuf+strlen(JSONbuf), ", \"b1\":%" PRIu64, pm1data.B);
	if (pm1data.C > pm1data.B) {
		sprintf (JSONbuf+strlen(JSONbuf), ", \"b2\":%" PRIu64, pm1data.C);
		sprintf (JSONbuf+strlen(JSONbuf), ", \"d\":%d", pm1data.D);
		if (pm1data.stage2_type == PM1_STAGE2_POLYMULT) {
			sprintf (JSONbuf+strlen(JSONbuf), ", \"poly1-size\":%" PRIu64, pm1data.poly1_size);
			sprintf (JSONbuf+strlen(JSONbuf), ", \"poly2-size\":%" PRIu64, pm1data.poly2_size);
			sprintf (JSONbuf+strlen(JSONbuf), ", \"stage2-fft-length\":%lu", gwfftlen (&pm1data.gwdata));
		}
	}
	sprintf (JSONbuf+strlen(JSONbuf), ", \"fft-length\":%lu", pm1data.stage1_fftlen);
	sprintf (JSONbuf+strlen(JSONbuf), ", \"security-code\":\"%08lX\"", SEC5 (w->n, pm1data.B, pm1data.C > pm1data.B ? pm1data.C : pm1data.B));
	JSONaddProgramTimestamp (JSONbuf);
	JSONaddExponentKnownFactors (JSONbuf, w);
	JSONaddUserComputerAID (JSONbuf, w);
	strcat (JSONbuf, "}");
	if (IniGetInt (INI_FILE, "OutputJSON", 1)) writeResultsJSON (JSONbuf);

	if (!QA_IN_PROGRESS && (pm1data.B >= 50000 || IniGetInt (INI_FILE, "SendAllFactorData", 0))) {
		struct primenetAssignmentResult pkt;
		memset (&pkt, 0, sizeof (pkt));
		strcpy (pkt.computer_guid, COMPUTER_GUID);
		strcpy (pkt.assignment_uid, w->assignment_uid);
		strcpy (pkt.message, buf);
		pkt.result_type = PRIMENET_AR_P1_NOFACTOR;
		pkt.k = w->k;
		pkt.b = w->b;
		pkt.n = w->n;
		pkt.c = w->c;
		pkt.B1 = pm1data.B;
		pkt.B2 = pm1data.C;
		pkt.fftlen = pm1data.stage1_fftlen;
		pkt.done = (w->work_type == WORK_PMINUS1 || w->work_type == WORK_PFACTOR);
		strcpy (pkt.JSONmessage, JSONbuf);
		spoolMessage (PRIMENET_ASSIGNMENT_RESULT, &pkt);
	}

	unlinkSaveFiles (&pm1data.write_save_file_state);
	if (!QA_IN_PROGRESS && w->work_type == WORK_PMINUS1 && IniGetInt (INI_FILE, "KeepPminus1SaveFiles", 1))
		pm1_save (&pm1data);

done:	if (w->work_type == WORK_PMINUS1 || w->work_type == WORK_PFACTOR)
		stop_reason = STOP_WORK_UNIT_COMPLETE;
	else {
		w->pminus1ed = 1;		
		w->tests_saved = 0.0;		
		stop_reason = updateWorkToDoLine (thread_num, w);
		if (stop_reason) goto exit;
	}

exit:	Dmultiple_map.clear ();
	relp_set_map.clear ();
	pm1_cleanup (&pm1data);
	free (N);
	free (factor);
	free (str);
	free (msg);
	if (exp_initialized) mpz_clear (exp);
	return (stop_reason);

lowmem:	stop_reason = OutOfMemory (thread_num);
possible_lowmem:
	if (pm1data.state == PM1_STATE_MIDSTAGE) pm1_save (&pm1data);
	if (stop_reason != STOP_OUT_OF_MEM) goto exit;

	pm1_save (&pm1data);
	Dmultiple_map.clear ();
	relp_set_map.clear ();
	pm1_cleanup (&pm1data);
	OutputBoth (thread_num, "Memory allocation error.  Trying again using less memory.\n");
	pm1data.pct_mem_to_use *= 0.8;
	goto restart;

oom:	stop_reason = OutOfMemory (thread_num);
	goto exit;

bingo:	if (pm1data.state < PM1_STATE_MIDSTAGE)
		sprintf (buf, "P-1 found a factor in stage #1, B1=%" PRIu64 ".\n", pm1data.B);
	else
		sprintf (buf, "P-1 found a factor in stage #2, B1=%" PRIu64 ", B2=%" PRIu64 ".\n", pm1data.B, pm1data.C);
	OutputBoth (thread_num, buf);

	msglen = factor->sign * 10 + 400;
	str = (char *) malloc (msglen);
	if (str == NULL) {
		stop_reason = OutOfMemory (thread_num);
		goto exit;
	}
	msg = (char *) malloc (msglen);
	if (msg == NULL) {
		stop_reason = OutOfMemory (thread_num);
		goto exit;
	}
	gtoc (factor, str, msglen);

	if (!testFactor (&pm1data.gwdata, w, factor)) {
		sprintf (msg, "ERROR: Bad factor for %s found: %s\n", gwmodulo_as_string (&pm1data.gwdata), str);
		OutputBoth (thread_num, msg);
		unlinkSaveFiles (&pm1data.write_save_file_state);
		OutputStr (thread_num, "Restarting P-1 from scratch.\n");
		stop_reason = 0;
		goto error_restart;
	}

	if (pm1data.state < PM1_STATE_MIDSTAGE)
		sprintf (msg, "%s has a factor: %s (P-1, B1=%" PRIu64 ")\n",
			 gwmodulo_as_string (&pm1data.gwdata), str, pm1data.B);
	else
		sprintf (msg, "%s has a factor: %s (P-1, B1=%" PRIu64 ", B2=%" PRIu64 ")\n",
			 gwmodulo_as_string (&pm1data.gwdata), str, pm1data.B, pm1data.C);
	OutputStr (thread_num, msg);
	formatMsgForResultsFile (msg, w);
	writeResults (msg);

	strcpy (JSONbuf, "{\"status\":\"F\"");
	JSONaddExponent (JSONbuf, w);
	strcat (JSONbuf, ", \"worktype\":\"P-1\"");
	sprintf (JSONbuf+strlen(JSONbuf), ", \"factors\":[\"%s\"]", str);
	sprintf (JSONbuf+strlen(JSONbuf), ", \"b1\":%" PRIu64, pm1data.B);
	if (pm1data.state > PM1_STATE_MIDSTAGE) {
		sprintf (JSONbuf+strlen(JSONbuf), ", \"b2\":%" PRIu64, pm1data.C);
		sprintf (JSONbuf+strlen(JSONbuf), ", \"d\":%d", pm1data.D);
		if (pm1data.stage2_type == PM1_STAGE2_POLYMULT) {
			sprintf (JSONbuf+strlen(JSONbuf), ", \"poly1-size\":%" PRIu64, pm1data.poly1_size);
			sprintf (JSONbuf+strlen(JSONbuf), ", \"poly2-size\":%" PRIu64, pm1data.poly2_size);
			sprintf (JSONbuf+strlen(JSONbuf), ", \"stage2-fft-length\":%lu", gwfftlen (&pm1data.gwdata));
		}
	}
	sprintf (JSONbuf+strlen(JSONbuf), ", \"fft-length\":%lu", pm1data.stage1_fftlen);
	sprintf (JSONbuf+strlen(JSONbuf), ", \"security-code\":\"%08lX\"", SEC5 (w->n, pm1data.B, pm1data.state > PM1_STATE_MIDSTAGE ? pm1data.C : pm1data.B));
	JSONaddProgramTimestamp (JSONbuf);
	JSONaddExponentKnownFactors (JSONbuf, w);
	JSONaddUserComputerAID (JSONbuf, w);
	strcat (JSONbuf, "}");
	if (IniGetInt (INI_FILE, "OutputJSON", 1)) writeResultsJSON (JSONbuf);

	if (!QA_IN_PROGRESS && (strlen (str) >= 21 || IniGetInt (INI_FILE, "SendAllFactorData", 0))) {
		struct primenetAssignmentResult pkt;
		memset (&pkt, 0, sizeof (pkt));
		strcpy (pkt.computer_guid, COMPUTER_GUID);
		strcpy (pkt.assignment_uid, w->assignment_uid);
		strcpy (pkt.message, msg);
		pkt.result_type = PRIMENET_AR_P1_FACTOR;
		pkt.k = w->k;
		pkt.b = w->b;
		pkt.n = w->n;
		pkt.c = w->c;
		truncated_strcpy (pkt.factor, sizeof (pkt.factor), str);
		pkt.B1 = pm1data.B;
		pkt.B2 = (pm1data.state < PM1_STATE_MIDSTAGE ? 0 : pm1data.C);
		pkt.fftlen = gwfftlen (&pm1data.gwdata);
		pkt.done = TRUE;
		strcpy (pkt.JSONmessage, JSONbuf);
		spoolMessage (PRIMENET_ASSIGNMENT_RESULT, &pkt);
	}

	unlinkSaveFiles (&pm1data.write_save_file_state);
	if (w->work_type != WORK_PMINUS1) {
		pm1data.write_save_file_state.base_filename[0] = 'p';
		unlinkSaveFiles (&pm1data.write_save_file_state);
	}
	if (!QA_IN_PROGRESS && w->work_type == WORK_PMINUS1 && IniGetInt (INI_FILE, "KeepPminus1SaveFiles", 1)) {
		pm1data.state = PM1_STATE_DONE;
		ASSERTG (pm1data.C_done >= pm1data.B_done);
		pm1_save (&pm1data);
	}

	invalidateNextRollingAverageUpdate ();

	if (USE_PRIMENET && w->k == 1.0 && w->b == 2 && w->c == -1 && strlen (str) >= 21 && (int) w->n < IniGetInt (INI_FILE, "MaxAutoPRPExponent", 5000000)) {
		struct work_unit w_prp;
		generatePRPWorkUnit (w, str, &w_prp);
		
		w_prp.next = w;
		stop_reason = addWorkToDoLine (thread_num, &w_prp, ADD_AFTER_SPECIFIC);
		if (stop_reason) goto exit;
	}

	stop_reason = STOP_WORK_UNIT_COMPLETE;
	goto exit;

err:	if (gw_get_maxerr (&pm1data.gwdata) > allowable_maxerr) {
		sprintf (buf, "Possible roundoff error (%.8g), backtracking to last save file and using larger FFT.\n\n", gw_get_maxerr (&pm1data.gwdata));
		OutputStr (thread_num, buf);
		maxerr_restart_count++;
		maxerr_fftlen = gwfftlen (&pm1data.gwdata);
		pm1data.interim_C = 0;				
	} else {
		OutputBoth (thread_num, "SUMOUT error occurred.\n");
		stop_reason = SleepFive (thread_num);
		if (stop_reason) goto exit;
	}
error_restart:
	Dmultiple_map.clear ();
	relp_set_map.clear ();
	pm1_cleanup (&pm1data);
	free (factor), factor = NULL;
	free (str), str = NULL;
	free (msg), msg = NULL;
	if (exp_initialized) mpz_clear (exp), exp_initialized = FALSE;
	goto restart;
}

int pminus1_QA (
	int	thread_num,
	struct PriorityInfo *sp_info)	
{
	FILE	*fd;

	title (thread_num, "QA");

	fd = fopen ("qa_pm1", "r");
	if (fd == NULL) {
		OutputStr (thread_num, "File named 'qa_pm1' could not be opened.\n");
		return (STOP_FILE_IO_ERROR);
	}

	QA_TYPE = 0;
	for ( ; ; ) {
		struct work_unit w;
		double	k;
		unsigned long b, n, B1, B2;
		signed long c;
		char	fac_str[80];
		int	stop_reason;

		n = 0;
		(void) fscanf (fd, "%lf,%lu,%lu,%ld,%lu,%lu,%s\n", &k, &b, &n, &c, &B1, &B2, fac_str);
		if (n == 0) break;

		if (n == 1) {
			QA_TYPE = c;
			continue;
		}

		QA_FACTOR = allocgiant ((int) strlen (fac_str));
		ctog (fac_str, QA_FACTOR);

		memset (&w, 0, sizeof (w));
		w.work_type = WORK_PMINUS1;
		w.k = k;
		w.b = b;
		w.n = n;
		w.c = c;
		w.B1 = B1;
		w.B2 = B2;
		QA_IN_PROGRESS = TRUE;
		stop_reason = pminus1 (0, sp_info, &w);
		QA_IN_PROGRESS = FALSE;
		free (QA_FACTOR);
		if (stop_reason != STOP_WORK_UNIT_COMPLETE) {
			fclose (fd);
			return (stop_reason);
		}
	}

	fclose (fd);
	return (0);
}

struct global_pm1_savings_data {
	unsigned long n;				
	struct pm1_stage2_cost_data cost_data;		
	double	ll_testing_cost;			
	uint64_t vals;					
	pmhandle polydata;				
};

#define PFACTOR_BOUND1_ROUNDING	1000			
#define PFACTOR_BOUND2_ROUNDING	5000			

struct pm1_savings_data {
	uint64_t B1;
	uint64_t B2;
	uint64_t actual_B2;
	double	prob;
	double	total_cost;
	double	savings;
};

void pm1_savings (
	struct global_pm1_savings_data *g,
	struct pm1_savings_data *c)
{
	uint64_t B1 = c->B1 * PFACTOR_BOUND1_ROUNDING;
	uint64_t B2 = c->B2 * PFACTOR_BOUND2_ROUNDING;

	if (B2 <= B1) {
		c->prob = pm1prob (g->cost_data.takeAwayBits, g->cost_data.sieve_depth, B1, B1);
		c->total_cost = g->cost_data.c.stage1_cost + g->cost_data.c.gcd_cost;
		c->actual_B2 = B2;
	}

	else {
		double efficiency = 0.0;
		
		if (g->vals < 120) {
			g->cost_data.stage2_type = PM1_STAGE2_PAIRING;
			g->cost_data.c.use_poly_D_data = FALSE;
			g->cost_data.c.centers_on_Dmultiple = TRUE;
			g->cost_data.c.stage2_fftlen = g->cost_data.c.stage1_fftlen;
			efficiency = best_stage2_impl (B1, B1, 0, 0, B2, g->vals - 3, &pm1_stage2_cost, &g->cost_data);
			if (efficiency > 0.0) {
				c->prob = g->cost_data.factor_probability;
				c->total_cost = g->cost_data.c.total_cost;
				c->actual_B2 = g->cost_data.c.B2_start + g->cost_data.c.numDsections *  g->cost_data.c.D;
			}
		}
		
		if (g->vals > 40) {
			g->cost_data.stage2_type = PM1_STAGE2_POLYMULT;
			g->cost_data.c.use_poly_D_data = TRUE;
			g->cost_data.c.centers_on_Dmultiple = FALSE;
			
			g->cost_data.c.stage2_fftlen = (unsigned long) ((double) g->cost_data.c.stage1_fftlen * 1.10);
			double poly_efficiency = best_stage2_impl (B1, B1, 0, 0, B2, (int) (g->vals / 1.10) - 4, &pm1_stage2_cost, &g->cost_data);
			if (poly_efficiency > efficiency) {
				efficiency = poly_efficiency;
				c->prob = g->cost_data.factor_probability;
				c->total_cost = g->cost_data.c.total_cost;
				c->actual_B2 = g->cost_data.c.B2_start + g->cost_data.c.numDsections *  g->cost_data.c.D;
			}
		}
		
		if (efficiency == 0.0) {
			c->prob = 0.0;
			c->total_cost = g->cost_data.c.stage1_cost + g->cost_data.c.gcd_cost;
			c->actual_B2 = B2;
		}
	}

	c->savings = c->prob * g->ll_testing_cost - c->total_cost;
}

void pminus1_choose_B2 (
	struct global_pm1_savings_data *g,
	struct pm1_savings_data *c)
{
	struct pm1_savings_data best[3];

	g->cost_data.c.stage1_cost = ceil (1.44 * c->B1 * PFACTOR_BOUND1_ROUNDING) * 2.0;

	best[0] = *c;
	best[0].B2 = c->B1 * PFACTOR_BOUND1_ROUNDING / PFACTOR_BOUND2_ROUNDING;
	pm1_savings (g, &best[0]);
	best[1] = *c;
	best[1].B2 = best[0].actual_B2 * 5 / PFACTOR_BOUND2_ROUNDING;
	pm1_savings (g, &best[1]);
	best[2] = *c;
	best[2].B2 = best[1].actual_B2 * 5 / PFACTOR_BOUND2_ROUNDING;
	pm1_savings (g, &best[2]);

	while (best[0].savings > best[1].savings) {
		best[2] = best[1];
		best[1].B2 = (best[0].B2 + best[2].B2) / 2;
		pm1_savings (g, &best[1]);
	}

	uint64_t max_B2 = IniGetInt (INI_FILE, "MaxOptimalB2Multiplier", 10000) * c->B1 * PFACTOR_BOUND1_ROUNDING / PFACTOR_BOUND2_ROUNDING;
	while (best[2].savings > best[1].savings) {
		if (best[2].B2 == max_B2) {
			best[0] = best[2];
			best[1] = best[2];
			break;
		}
		best[0] = best[1];
		best[1] = best[2];
		best[2].B2 = best[1].actual_B2 * 5 / PFACTOR_BOUND2_ROUNDING;
		if (best[2].B2 > max_B2) best[2].B2 = max_B2;
		pm1_savings (g, &best[2]);
	}

	while (best[2].B2 - best[0].B2 > 2) {
		struct pm1_savings_data midpoint;

		ASSERTG (best[1].savings >= best[0].savings);
		ASSERTG (best[1].savings >= best[2].savings);

		if (best[1].B2 - best[0].B2 > best[2].B2 - best[1].B2) {	
			
			if (best[1].savings - best[0].savings < 100.0) {
				best[0] = best[1], best[0].B2--;
				continue;
			}
			midpoint = *c;
			midpoint.B2 = (best[0].B2 + best[1].B2) / 2;
			pm1_savings (g, &midpoint);
			if (midpoint.savings > best[1].savings) {		
				best[2] = best[1];
				best[1] = midpoint;
			} else {						
				best[0] = midpoint;
			}
		} else {							
			
			if (best[1].savings - best[2].savings < 100.0) {
				best[2] = best[1], best[2].B2++;
				continue;
			}
			midpoint = *c;
			midpoint.B2 = (best[1].B2 + best[2].B2) / 2;
			pm1_savings (g, &midpoint);
			if (midpoint.savings > best[1].savings) {		
				best[0] = best[1];
				best[1] = midpoint;
			} else {						
				best[2] = midpoint;
			}
		}
	}

	*c = best[1];
}

void guess_pminus1_bounds (
	int	thread_num,
	double	k,			
	unsigned long b,		
	unsigned long n,		
	signed long c,			
	double	how_far_factored,	
	double	tests_saved,		
	uint64_t *bound1,
	uint64_t *bound2,
	uint64_t *squarings,
	double	*success_rate)
{
	struct global_pm1_savings_data g;
	struct pm1_savings_data best[3];

	if (tests_saved > 100) tests_saved = 100;

	memset (&g, 0, sizeof (g));
	g.n = n;
	g.cost_data.takeAwayBits = isMersenne (k, b, n, c) ? log2 (n) + 1.0 : isGeneralizedFermat (k, b, n, c) ? log2 (n) : 0.0;
	g.cost_data.sieve_depth = how_far_factored;
	g.cost_data.c.polydata = &g.polydata;
	g.cost_data.c.poly_compression = 0.984;
	if (IniGetInt (INI_FILE, "Poly1Compress", 2) == 2) g.cost_data.c.poly_compression *= 0.875;
	if (IniGetInt (INI_FILE, "Poly1Compress", 2) == 0) g.cost_data.c.poly_compression = 1.0;
	g.cost_data.c.stage1_fftlen = gwmap_to_fftlen (k, b, n, c);
	g.cost_data.c.stage1_threads = CORES_PER_TEST[thread_num];
	g.cost_data.c.stage2_threads = g.cost_data.c.stage1_threads + IniGetInt (INI_FILE, "Stage2ExtraThreads", 0);
	init_gcd_costs ((n * log ((double) b) + log (k)) / log (2.0), &g.cost_data);

	g.ll_testing_cost = (tests_saved + 2.0 * ERROR_RATE) * n * 2.0;

	g.vals = cvt_mem_to_estimated_gwnums (max_mem (thread_num), k, b, n, c);
	if (g.vals < 1) g.vals = 1;

	gwhandle gwdata;
	memset (&gwdata, 0, sizeof (gwdata));
	gwdata.k = k;
	gwdata.b = b;
	gwdata.n = n;
	gwdata.c = c;
	gwdata.FFTLEN = (unsigned long) (g.cost_data.c.stage1_fftlen * 1.10);
	gwdata.cpu_flags = CPU_FLAGS;
	if (gwdata.FFTLEN < 1024 && gwdata.cpu_flags & CPU_AVX512F) gwdata.cpu_flags &= ~CPU_AVX512F;
	gwdata.num_threads = g.cost_data.c.stage2_threads;
	polymult_init (&g.polydata, &gwdata);
	polymult_default_tuning (&g.polydata,
				 IniGetInt (INI_FILE, "PolymultCacheSize", CPU_NUM_L2_CACHES > 0 ? CPU_TOTAL_L2_CACHE_SIZE / CPU_NUM_L2_CACHES : 256),
				 IniGetInt (INI_FILE, "PolymultCacheSize2", CPU_NUM_L3_CACHES > 0 ? CPU_TOTAL_L3_CACHE_SIZE / CPU_NUM_L3_CACHES : 6144));

	best[0].B1 = n / 3300 / PFACTOR_BOUND1_ROUNDING;
	if (best[0].B1 < 1) best[0].B1 = 1;
	pminus1_choose_B2 (&g, &best[0]);
	best[1].B1 = 125 * best[0].B1;
	pminus1_choose_B2 (&g, &best[1]);
	best[2].B1 = 250 * best[0].B1;
	pminus1_choose_B2 (&g, &best[2]);

	while (best[0].savings > best[1].savings) {
		best[2] = best[1];
		best[1].B1 = (best[0].B1 + best[2].B1) / 2;
		pminus1_choose_B2 (&g, &best[1]);
	}

	while (best[2].savings > best[1].savings) {
		best[0] = best[1];
		best[1] = best[2];
		best[2].B1 = best[1].B1 * 2;
		pminus1_choose_B2 (&g, &best[2]);
	}

	while (best[2].B1 - best[0].B1 > 2) {
		struct pm1_savings_data midpoint;

		ASSERTG (best[1].savings >= best[0].savings);
		ASSERTG (best[1].savings >= best[2].savings);

		if (best[1].B1 - best[0].B1 > best[2].B1 - best[1].B1) {	
			
			if (best[1].savings - best[0].savings < 100.0) {
				best[0] = best[1], best[0].B1--;
				continue;
			}
			midpoint.B1 = (best[0].B1 + best[1].B1) / 2;
			pminus1_choose_B2 (&g, &midpoint);
			if (midpoint.savings > best[1].savings) {		
				best[2] = best[1];
				best[1] = midpoint;
			} else {						
				best[0] = midpoint;
			}
		} else {							
			
			if (best[1].savings - best[2].savings < 100.0) {
				best[2] = best[1], best[2].B1++;
				continue;
			}
			midpoint.B1 = (best[1].B1 + best[2].B1) / 2;
			pminus1_choose_B2 (&g, &midpoint);
			if (midpoint.savings > best[1].savings) {		
				best[0] = best[1];
				best[1] = midpoint;
			} else {						
				best[2] = midpoint;
			}
		}
	}

	if (best[1].savings > 0.0) {
		*bound1 = best[1].B1 * PFACTOR_BOUND1_ROUNDING;
		*bound2 = best[1].B2 * PFACTOR_BOUND2_ROUNDING;
		*squarings = (uint64_t) (best[1].total_cost / 2.0);		
		*success_rate = best[1].prob;
	} else {
		*bound1 = 0;
		*bound2 = 0;
		*squarings = 0;
		*success_rate = 0.0;
	}
}

double guess_pminus1_probability (
	struct work_unit *w)
{
	double	takeAwayBits;
	takeAwayBits = isMersenne (w->k, w->b, w->n, w->c) ? log2 (w->n) + 1.0 :
		       isGeneralizedFermat (w->k, w->b, w->n, w->c) ? log2 (w->n) : 0.0;
	return (pm1prob (takeAwayBits, (unsigned int) w->sieve_depth, w->B1, w->B2));
}

int pfactor (
	int	thread_num,
	struct PriorityInfo *sp_info,	
	struct work_unit *w)
{
	uint64_t bound1, bound2, squarings;
	double	prob;
	char	buf[120], testnum[120];
	int	stop_reason;

	set_restart_if_max_memory_change (thread_num);

	gw_as_string (testnum, w->k, w->b, w->n, w->c);
	sprintf (buf, "Optimal P-1 factoring of %s using up to %luMB of memory.\n", testnum, max_mem (thread_num));
	OutputStr (thread_num, buf);
	if (w->known_factors != NULL) {
		OutputStr (thread_num, "Skipping known factors: ");
		OutputStr (thread_num, w->known_factors);
		OutputStr (thread_num, "\n");
	}
	sprintf (buf, "Assuming no%s factors below 2^%.2g and %.2g primality test%s saved if a factor is found.\n",
		 w->known_factors != NULL ? " other" : "", w->sieve_depth, w->tests_saved, w->tests_saved == 1.0 ? "" : "s");
	OutputStr (thread_num, buf);

	guess_pminus1_bounds (thread_num, w->k, w->b, w->n, w->c, w->sieve_depth, w->tests_saved, &bound1, &bound2, &squarings, &prob);
	if (bound1 == 0) {
		sprintf (buf, "%s does not need P-1 factoring.\n", testnum);
		OutputBoth (thread_num, buf);
		if (w->work_type == WORK_PFACTOR) {
			
			return (STOP_WORK_UNIT_COMPLETE);
		} else {
			w->pminus1ed = 1;		
			w->tests_saved = 0.0;		
			stop_reason = updateWorkToDoLine (thread_num, w);
			if (stop_reason) return (stop_reason);
			return (0);
		}
	}

	sprintf (buf, "Optimal bounds are B1=%" PRIu64 ", B2=%" PRIu64 "\n", bound1, bound2);
	OutputStr (thread_num, buf);
	sprintf (buf, "Chance of finding a new factor is an estimated %.3g%%\n", prob * 100.0);
	OutputStr (thread_num, buf);

	w->B1 = bound1;
	w->B2_start = bound1;
	w->B2 = bound2;
	return (pminus1 (thread_num, sp_info, w));
}

#define PP1_STATE_STAGE1	1	
#define PP1_STATE_MIDSTAGE	2	
#define PP1_STATE_STAGE2	3	
#define PP1_STATE_GCD		4	
#define PP1_STATE_DONE		5	

typedef struct {
	gwhandle gwdata;	
	int	thread_num;	
	int	stage1_threads;	
	int	stage2_threads;	
	struct work_unit *w;	
	int	state;		
	double	takeAwayBits;	
	double	success_rate;	
	uint32_t numerator;	
	uint32_t denominator;	
	uint64_t B;		
	uint64_t C;		
	uint64_t interim_B;	
	uint64_t interim_C;	
	uint64_t B_done;	
	int	optimal_B2;	

	readSaveFileState read_save_file_state;	
	writeSaveFileState write_save_file_state; 
	void	*sieve_info;	
	uint64_t stage1_prime;	
	unsigned long stage1_fftlen; 

	int	D;		
	int	numrels;	
	int	totrels;	
	uint64_t B2_start;	
	uint64_t numDsections;	
	uint64_t Dsection;	
	int	relp;		

	uint64_t max_pairmap_Dsections;	
	uint8_t	*pairmap;	
	uint64_t pairmap_size;	
	uint8_t *pairmap_ptr;	
	uint64_t first_relocatable; 
	uint64_t last_relocatable; 
	uint64_t C_done;	

	uint64_t stage2_numvals;
	int16_t relp_sets[32];	
	gwnum	*nQx;		

	double	pct_mem_to_use;	

	gwnum	V;		
	gwnum	Vn;		
	gwnum	Vn1;		
	gwnum	gg;		
} pp1handle;

void pp1_cleanup (
	pp1handle *pp1data)
{

	free (pp1data->nQx), pp1data->nQx = NULL;
	free (pp1data->pairmap), pp1data->pairmap = NULL;
	gwdone (&pp1data->gwdata);
	end_sieve (pp1data->sieve_info), pp1data->sieve_info = NULL;
}

#define PP1_MAGICNUM	0x912a374a

#define PP1_VERSION	2				

void pp1_save (
	pp1handle *pp1data)
{
	int	fd;
	struct work_unit *w = pp1data->w;
	uint32_t sum = 0;

	fd = openWriteSaveFile (&pp1data->write_save_file_state);
	if (fd < 0) return;

	if (!write_header (fd, PP1_MAGICNUM, PP1_VERSION, w)) goto writeerr;

	if (! write_int (fd, pp1data->state, &sum)) goto writeerr;
	if (! write_int (fd, pp1data->numerator, &sum)) goto writeerr;
	if (! write_int (fd, pp1data->denominator, &sum)) goto writeerr;

	if (pp1data->state == PP1_STATE_STAGE1) {
		if (! write_uint64 (fd, pp1data->B_done, &sum)) goto writeerr;
		if (! write_uint64 (fd, pp1data->interim_B, &sum)) goto writeerr;
		if (! write_uint64 (fd, pp1data->stage1_prime, &sum)) goto writeerr;
	}

	else if (pp1data->state == PP1_STATE_MIDSTAGE) {
		if (! write_uint64 (fd, pp1data->B_done, &sum)) goto writeerr;
		if (! write_uint64 (fd, pp1data->C_done, &sum)) goto writeerr;
	}

	else if (pp1data->state == PP1_STATE_STAGE2) {
		uint64_t remaining_pairmap_size;
		if (! write_uint64 (fd, pp1data->B_done, &sum)) goto writeerr;
		if (! write_uint64 (fd, pp1data->C_done, &sum)) goto writeerr;
		if (! write_uint64 (fd, pp1data->interim_C, &sum)) goto writeerr;
		if (! write_int (fd, (int) pp1data->stage2_numvals, &sum)) goto writeerr;
		if (! write_int (fd, pp1data->totrels, &sum)) goto writeerr;
		if (! write_int (fd, pp1data->D, &sum)) goto writeerr;
		if (! write_uint64 (fd, pp1data->first_relocatable, &sum)) goto writeerr;
		if (! write_uint64 (fd, pp1data->last_relocatable, &sum)) goto writeerr;
		if (! write_uint64 (fd, pp1data->B2_start, &sum)) goto writeerr;
		if (! write_uint64 (fd, pp1data->numDsections, &sum)) goto writeerr;
		if (! write_uint64 (fd, pp1data->Dsection, &sum)) goto writeerr;
		if (! write_uint64 (fd, pp1data->max_pairmap_Dsections, &sum)) goto writeerr;
		if (! write_int (fd, pp1data->relp, &sum)) goto writeerr;
		if (! write_array (fd, (char *) pp1data->relp_sets, 32 * sizeof (int16_t), &sum)) goto writeerr;
		
		remaining_pairmap_size = pp1data->pairmap_size - (pp1data->pairmap_ptr - pp1data->pairmap);
		if (! write_uint64 (fd, remaining_pairmap_size, &sum)) goto writeerr;
		if (! write_array (fd, (char *) pp1data->pairmap_ptr, (size_t) remaining_pairmap_size, &sum)) goto writeerr;
	}

	else if (pp1data->state == PP1_STATE_GCD) {
		if (! write_uint64 (fd, pp1data->B_done, &sum)) goto writeerr;
		if (! write_uint64 (fd, pp1data->C_done, &sum)) goto writeerr;
	}

	else if (pp1data->state == PP1_STATE_DONE) {
		if (! write_uint64 (fd, pp1data->B_done, &sum)) goto writeerr;
		if (! write_uint64 (fd, pp1data->C_done, &sum)) goto writeerr;
	}

	if (! write_gwnum (fd, &pp1data->gwdata, pp1data->V, &sum)) goto writeerr;
	if (pp1data->state >= PP1_STATE_MIDSTAGE && pp1data->state <= PP1_STATE_GCD) {
		int	have_gg = (pp1data->gg != NULL);
		if (! write_int (fd, have_gg, &sum)) goto writeerr;
		if (have_gg && !write_gwnum (fd, &pp1data->gwdata, pp1data->gg, &sum)) goto writeerr;
	}

	gwfree_cached (&pp1data->gwdata);
	mallocFreeForOS ();

	if (! write_checksum (fd, sum)) goto writeerr;

	closeWriteSaveFile (&pp1data->write_save_file_state, fd);
	return;

writeerr:
	deleteWriteSaveFile (&pp1data->write_save_file_state, fd);
}

int pp1_restore (
	pp1handle *pp1data)
{
	int	fd, numerator, denominator;
	struct work_unit *w = pp1data->w;
	uint32_t version, sum = 0, filesum;

	fd = _open (pp1data->read_save_file_state.current_filename, _O_BINARY | _O_RDONLY);
	if (fd < 0) goto err;

	if (! read_magicnum (fd, PP1_MAGICNUM)) goto readerr;
	if (! read_header (fd, &version, w, &filesum)) goto readerr;
	if (version < 1 || version > PP1_VERSION) goto readerr;

	if (! read_int (fd, &pp1data->state, &sum)) goto readerr;
	if (! read_int (fd, &numerator, &sum)) goto readerr;
	if (! read_int (fd, &denominator, &sum)) goto readerr;
	if (numerator != pp1data->numerator || denominator != pp1data->denominator) {
		if (pp1data->w->nth_run <= 2) goto bad_nth_run;			
		if (numerator == 2 && denominator == 7) goto bad_nth_run;	
		if (numerator == 6 && denominator == 5) goto bad_nth_run;	
		pp1data->numerator = numerator;					
		pp1data->denominator = denominator;
	}

	if (pp1data->state == PP1_STATE_STAGE1) {
		if (! read_uint64 (fd, &pp1data->B_done, &sum)) goto readerr;
		if (! read_uint64 (fd, &pp1data->interim_B, &sum)) goto readerr;
		if (! read_uint64 (fd, &pp1data->stage1_prime, &sum)) goto readerr;
	}

	else if (pp1data->state == PP1_STATE_MIDSTAGE) {
		if (! read_uint64 (fd, &pp1data->B_done, &sum)) goto readerr;
		if (! read_uint64 (fd, &pp1data->C_done, &sum)) goto readerr;
	}

	else if (pp1data->state == PP1_STATE_STAGE2) {
		if (! read_uint64 (fd, &pp1data->B_done, &sum)) goto readerr;
		if (! read_uint64 (fd, &pp1data->C_done, &sum)) goto readerr;
		if (! read_uint64 (fd, &pp1data->interim_C, &sum)) goto readerr;
		int temp;
		if (! read_int (fd, &temp, &sum)) goto readerr;
		pp1data->stage2_numvals = temp;
		if (! read_int (fd, &pp1data->totrels, &sum)) goto readerr;
		if (! read_int (fd, &pp1data->D, &sum)) goto readerr;
		pp1data->numrels = map_D_to_numrels (pp1data->D);
		if (! read_uint64 (fd, &pp1data->first_relocatable, &sum)) goto readerr;
		if (! read_uint64 (fd, &pp1data->last_relocatable, &sum)) goto readerr;
		if (! read_uint64 (fd, &pp1data->B2_start, &sum)) goto readerr;
		if (! read_uint64 (fd, &pp1data->numDsections, &sum)) goto readerr;
		if (! read_uint64 (fd, &pp1data->Dsection, &sum)) goto readerr;
		if (version == 1) {		
			uint64_t bitarraymaxDsections, bitarrayfirstDsection, bitarray_numDsections, bitarray_start, bitarray_len;
			char	*bitarray;
			if (! read_uint64 (fd, &bitarraymaxDsections, &sum)) goto readerr;
			if (! read_uint64 (fd, &bitarrayfirstDsection, &sum)) goto readerr;
			
			bitarray_numDsections = pp1data->numDsections - bitarrayfirstDsection;
			if (bitarray_numDsections > bitarraymaxDsections) bitarray_numDsections = bitarraymaxDsections;
			bitarray_len = divide_rounding_up (bitarray_numDsections * pp1data->totrels, 8);
			bitarray_start = divide_rounding_down ((pp1data->Dsection - bitarrayfirstDsection) * pp1data->totrels, 8);
			bitarray = (char *) malloc ((size_t) bitarray_len);
			if (bitarray == NULL) goto readerr;
			if (! read_array (fd, bitarray, (size_t) (bitarray_len - bitarray_start), &sum)) goto readerr;
			free (bitarray);
		} else {				
			if (! read_uint64 (fd, &pp1data->max_pairmap_Dsections, &sum)) goto readerr;
			if (! read_int (fd, &pp1data->relp, &sum)) goto readerr;
			if (! read_array (fd, (char *) pp1data->relp_sets, 32 * sizeof (int16_t), &sum)) goto readerr;
			if (! read_uint64 (fd, &pp1data->pairmap_size, &sum)) goto readerr;
			pp1data->pairmap = (uint8_t *) malloc ((size_t) pp1data->pairmap_size);
			if (pp1data->pairmap == NULL) goto readerr;
			if (! read_array (fd, (char *) pp1data->pairmap, (size_t) pp1data->pairmap_size, &sum)) goto readerr;
			pp1data->pairmap_ptr = pp1data->pairmap;
		}
	}

	else if (pp1data->state == PP1_STATE_GCD) {
		if (! read_uint64 (fd, &pp1data->B_done, &sum)) goto readerr;
		if (! read_uint64 (fd, &pp1data->C_done, &sum)) goto readerr;
	}

	else if (pp1data->state == PP1_STATE_DONE) {
		if (! read_uint64 (fd, &pp1data->B_done, &sum)) goto readerr;
		if (! read_uint64 (fd, &pp1data->C_done, &sum)) goto readerr;
	}

	if (! read_gwnum (fd, &pp1data->gwdata, pp1data->V, &sum)) goto readerr;

	if (pp1data->state >= PP1_STATE_MIDSTAGE && pp1data->state <= PP1_STATE_GCD) {
		int	have_gg;
		if (version == 4) have_gg = TRUE;				
		else if (! read_int (fd, &have_gg, &sum)) goto readerr;		
		if (have_gg) {
			pp1data->gg = gwalloc (&pp1data->gwdata);
			if (pp1data->gg == NULL) goto readerr;
			if (! read_gwnum (fd, &pp1data->gwdata, pp1data->gg, &sum)) goto readerr;
		}
	}

	if (version == 1 && pp1data->state == PP1_STATE_STAGE2) {
		pp1data->state = PP1_STATE_STAGE1;
		pp1data->stage1_prime = pp1data->B_done;
		pp1data->interim_B = pp1data->B_done;
		OutputStr (pp1data->thread_num, "Old P+1 save file was in stage 2.  Restarting stage 2 from scratch.\n");
		gwfree (&pp1data->gwdata, pp1data->gg);
		pp1data->gg = NULL;
	}

	if (filesum != sum) goto readerr;
	_close (fd);

	return (TRUE);

bad_nth_run:
	OutputBoth (pp1data->thread_num, "P+1 starting point in save file does not match nth_run parameter from worktodo.txt\n");
readerr:
	_close (fd);
	gwfree (&pp1data->gwdata, pp1data->gg), pp1data->gg = NULL;
err:
	return (FALSE);
}

struct pp1_stage2_cost_data {
	
	struct common_cost_data c;
	
};

double pp1_stage2_cost (
	void	*data)		
{
	struct pp1_stage2_cost_data *cost_data = (struct pp1_stage2_cost_data *) data;

	cost_data->c.est_init_transforms = 6.0 * 2.0;						
	cost_data->c.est_init_transforms += (double) (cost_data->c.D / 6 - 2) * 2.0 * 2.0;	
	cost_data->c.est_init_transforms += 2.0 * 2.0;						
	cost_data->c.est_init_transforms += (cost_data->c.totrels - cost_data->c.numrels) * 2.0; 

	cost_data->c.est_init_transforms +=
		cost_data->c.numrels * cost_data->c.relp_sets[1] * 2.0 +					 
		one_based_modulo (cost_data->c.totrels, cost_data->c.numrels) * cost_data->c.relp_sets[2] * 2.0; 

	cost_data->c.est_init_transforms += (2.0 * log2((double)((cost_data->c.B2_start + cost_data->c.D / 2) / cost_data->c.D)) + cost_data->c.multiplier) * 2.0;

	cost_data->c.est_stage2_transforms = (double) cost_data->c.numDsections * 2.0;

	cost_data->c.est_stage2_transforms += cost_data->c.numrels;

	cost_data->c.est_stage2_transforms += (cost_data->c.est_numpairs + cost_data->c.est_numsingles) * 2.0;

	cost_data->c.stage2_numvals = cost_data->c.totrels + 4;

	cost_data->c.stage2_cost = cost_data->c.est_pairing_runtime + cost_data->c.est_init_transforms + cost_data->c.est_stage2_transforms;
	cost_data->c.total_cost = cost_data->c.stage1_cost + cost_data->c.stage2_cost + cost_data->c.gcd_cost;
	cost_data->c.efficiency = 1.0 / cost_data->c.total_cost;
	return (cost_data->c.efficiency);
}

void pp1_choose_B2 (
	pp1handle *pp1data,
	uint64_t numvals)
{
	int	max_B2mult;
	struct pp1_stage2_cost_data cost_data;	
	struct pp1_stage2_efficiency {
		int	i;
		double	B2_cost;		
		double	fac_pct;		
	} best[3];
	int	sieve_depth;
	char	buf[160];

#define pp1prob(B1,B2)		(pm1prob (pp1data->takeAwayBits, sieve_depth, B1, B2) * pp1data->success_rate)

	max_B2mult = IniGetInt (INI_FILE, "MaxOptimalB2Multiplier", 1000);
	cost_data.c.numvals = numvals;
	cost_data.c.use_poly_D_data = FALSE;
	cost_data.c.centers_on_Dmultiple = TRUE;
	cost_data.c.gwdata = &pp1data->gwdata;
	cost_data.c.stage1_fftlen = pp1data->stage1_fftlen;
	cost_data.c.stage2_fftlen = gwfftlen (&pp1data->gwdata);
	cost_data.c.stage1_threads = pp1data->stage1_threads;
	cost_data.c.stage2_threads = pp1data->stage2_threads;
#define p1eval(x,B2mult)	x.i = B2mult; \
				if (x.i > max_B2mult) x.i = max_B2mult; \
				x.B2_cost = best_stage2_impl (pp1data->B, pp1data->B, 0, 0, x.i * pp1data->B, numvals - 4, &pp1_stage2_cost, &cost_data); \
				x.fac_pct = pp1prob (pp1data->B, x.i * pp1data->B);

#define B1increase(x,y)	(int) ((y.B2_cost - x.B2_cost) / (1.44*1.52))	
#define p1compare(x,y)	(pp1prob (pp1data->B + B1increase(x,y), x.i * pp1data->B + B1increase(x,y)) > y.fac_pct)

	sieve_depth = (int) pp1data->w->sieve_depth;
	p1eval (best[0], 10);
	p1eval (best[1], 40);
	p1eval (best[2], 80);

	while (p1compare (best[0], best[1])) {
		best[2] = best[1];
		p1eval (best[1], (best[0].i + best[2].i) / 2);
	}

	while (!p1compare (best[1], best[2]) && best[2].i < max_B2mult) {
		best[0] = best[1];
		best[1] = best[2];
		p1eval (best[2], best[1].i * 2);
	}

	while (best[2].i - best[0].i > 2) {
		struct pp1_stage2_efficiency midpoint;

		if (best[1].i - best[0].i > best[2].i - best[1].i) {		
			p1eval (midpoint, (best[0].i + best[1].i) / 2);
			if (p1compare (midpoint, best[1])) {			
				best[2] = best[1];
				best[1] = midpoint;
			} else {						
				best[0] = midpoint;
			}
		} else {							
			p1eval (midpoint, (best[1].i + best[2].i) / 2);
			if (!p1compare (best[1], midpoint)) {			
				best[0] = best[1];
				best[1] = midpoint;
			} else {						
				best[2] = midpoint;
			}
		}
	}

	pp1data->C = best[1].i * pp1data->B;
	sprintf (buf, "With trial factoring done to 2^%d, optimal B2 is %d*B1 = %" PRIu64 ".\n", sieve_depth, best[1].i, pp1data->C);
	OutputStr (pp1data->thread_num, buf);
	sprintf (buf, "Chance of finding a new factor assuming no ECM has been done is %.3g%%\n", best[1].fac_pct * 100.0);
	OutputStr (pp1data->thread_num, buf);
}
#undef p1eval
#undef B1increase
#undef p1compare

int pp1_stage2_impl (
	pp1handle *pp1data)
{
	unsigned int memory, min_memory, desired_memory;	
	uint64_t numvals;					
	struct pp1_stage2_cost_data cost_data;
	int	stop_reason;

	min_memory = 1 + cvt_gwnums_to_mem (&pp1data->gwdata, 8);
	if (pp1data->state < PP1_STATE_STAGE2) desired_memory = 3 + cvt_gwnums_to_mem (&pp1data->gwdata, 144);
	else desired_memory = (unsigned int) (pp1data->pairmap_size >> 20) + cvt_gwnums_to_mem (&pp1data->gwdata, pp1data->stage2_numvals);
	stop_reason = avail_mem (pp1data->thread_num, min_memory, desired_memory, &memory);
	if (stop_reason) return (stop_reason);

	memory = (unsigned int) (pp1data->pct_mem_to_use * (double) memory);
	if (memory < min_memory) return (avail_mem_not_sufficient (pp1data->thread_num, min_memory, desired_memory));
	if (memory < 8) memory = 8;

	if (NUM_WORKERS > 1) {
		char	buf[100];
		sprintf (buf, "Available memory is %dMB.\n", memory);
		OutputStr (pp1data->thread_num, buf);
	}

	numvals = cvt_mem_to_gwnums (&pp1data->gwdata, memory);
	if (numvals < 8) numvals = 8;

	if (numvals > 100000) numvals = 100000;
	if (QA_TYPE) numvals = QA_TYPE;			

	if (pp1data->state == PP1_STATE_MIDSTAGE) {
		if (pp1data->C_done == pp1data->B) {
			pp1data->first_relocatable = pp1data->B;
			pp1data->last_relocatable = 0;
			if (pp1data->optimal_B2) pp1_choose_B2 (pp1data, numvals);
		} else {
			pp1data->first_relocatable = pp1data->C_done;
			pp1data->last_relocatable = 0;
		}
	}

	if (pp1data->state >= PP1_STATE_STAGE2 &&				
	    numvals >= pp1data->stage2_numvals &&				
	    (numvals < pp1data->stage2_numvals * 2 ||				
	     pp1data->Dsection >= pp1data->numDsections / 2))			
		return (0);							

	if (pp1data->state == PP1_STATE_STAGE2) {
		int	max_relp_set = get_max_relp_set (pp1data->relp_sets);
		if (pp1data->Dsection > max_relp_set) pp1data->C_done = pp1data->B2_start + (pp1data->Dsection - max_relp_set) * pp1data->D;
		else pp1data->C_done = pp1data->B2_start;
	}

	cost_data.c.use_poly_D_data = FALSE;
	cost_data.c.centers_on_Dmultiple = TRUE;
	cost_data.c.gwdata = &pp1data->gwdata;
	cost_data.c.stage1_fftlen = pp1data->stage1_fftlen;
	cost_data.c.stage2_fftlen = gwfftlen (&pp1data->gwdata);
	cost_data.c.stage1_threads = pp1data->stage1_threads;
	cost_data.c.stage2_threads = pp1data->stage2_threads;
	best_stage2_impl (pp1data->B, pp1data->first_relocatable, pp1data->last_relocatable, pp1data->C_done, pp1data->C, numvals - 4, &pp1_stage2_cost, &cost_data);

	if (pp1data->state >= PP1_STATE_STAGE2 &&				
	    numvals >= pp1data->stage2_numvals &&				
	    cost_data.c.stage2_numvals < pp1data->stage2_numvals * 2)		
		return (0);							

	if (pp1data->state >= PP1_STATE_STAGE2) {
		free (pp1data->pairmap);
		pp1data->pairmap = NULL;
	}

	pp1data->interim_C = pp1data->C;
	pp1data->stage2_numvals = cost_data.c.stage2_numvals;
	pp1data->D = cost_data.c.D;
	pp1data->totrels = (int) cost_data.c.totrels;
	pp1data->numrels = cost_data.c.numrels;
	pp1data->B2_start = cost_data.c.B2_start;
	pp1data->numDsections = cost_data.c.numDsections;
	pp1data->max_pairmap_Dsections = cost_data.c.max_pairmap_Dsections;
	memcpy (pp1data->relp_sets, cost_data.c.relp_sets, sizeof (pp1data->relp_sets));
	if (pp1data->state < PP1_STATE_STAGE2 || pp1data->last_relocatable > pp1data->B2_start) pp1data->last_relocatable = pp1data->B2_start;

	if (IniGetInt (INI_FILE, "Stage2Estimates", 0)) {
		char	buf[120];
		sprintf (buf, "Est pair%%: %5.2f, init transforms: %.0f, main loop transforms: %.0f\n",
			 cost_data.c.est_pair_pct * 100.0, cost_data.c.est_init_transforms, cost_data.c.est_stage2_transforms);
		OutputStr (pp1data->thread_num, buf);
	}

	int fill_window = pair_window_size (pp1data->gwdata.bit_length, pp1data->relp_sets);
	gwfree_internal_memory (&pp1data->gwdata);
	stop_reason = fill_pairmap (pp1data->thread_num, &pp1data->sieve_info, pp1data->D, fill_window,0,0,0,
				    pp1data->totrels, pp1data->relp_sets+3, pp1data->first_relocatable, pp1data->last_relocatable,
				    pp1data->B2_start, pp1data->C, pp1data->max_pairmap_Dsections, &pp1data->pairmap, &pp1data->pairmap_size);
	if (stop_reason) return (stop_reason);
	pp1data->pairmap_ptr = pp1data->pairmap;
	pp1data->Dsection = 0;
	pp1data->relp = -1;

	return (0);
}

void pp1_calc_start (
	gwhandle *gwdata,
	int	numerator,
	int	denominator,
	giant	N,		
	gwnum	V)		
{
	mpz_t	__frac, __inv, __N;
	giant	g_frac;

	mpz_init (__N);
	gtompz (N, __N);

	mpz_init_set_ui (__inv, denominator);
	mpz_invert (__inv, __inv, __N);

	mpz_init (__frac);
	mpz_mul_ui (__frac, __inv, numerator);
	mpz_mod (__frac, __frac, __N);
	g_frac = allocgiant ((int) divide_rounding_up (mpz_sizeinbase (__frac, 2), 32));
	mpztog (__frac, g_frac);
	gianttogw (gwdata, g_frac, V);
	free (g_frac);

	mpz_clear (__N);
	mpz_clear (__inv);
	mpz_clear (__frac);
}

#define swap(a,b)	{t=a;a=b;b=t;}
#define PP1_ADD_COST	2
#define PP1_DBL_COST	2

int pp1_lucas_cost (
	uint64_t n,
	uint64_t d)
{
	uint64_t e, t;
	unsigned long c;

	if (d >= n || d <= n/2) return (999999999);		

	c = 0;
	e = n - d;
	d = d - e;

	c += PP1_ADD_COST;

	while (d != e) {
		if (d < e) {
			swap (d,e);
		}
		if (100 * d <= 296 * e) {
			d = d-e;
			c += PP1_ADD_COST;
		} else if ((d&1) == (e&1)) {
			d = (d-e) >> 1;
			c += PP1_DBL_COST + PP1_ADD_COST;
		} else if ((d&1) == 0) {
			d = d >> 1;
			c += PP1_DBL_COST + PP1_ADD_COST;

		} else {
			d = d-e;
			c += PP1_ADD_COST;

		}
	}

	return (c);
}

__inline int pp1_lucas_cost_several (uint64_t n, uint64_t *d) {
	int	i, c, min;
	uint64_t testd;
	for (i = 0, testd = *d - PRAC_SEARCH / 2; i < PRAC_SEARCH; i++, testd++) {
		c = pp1_lucas_cost (n, testd);
		if (i == 0 || c < min) min = c, *d = testd;
	}
	return (min);
}

void pp1_lucas_mul (
	pp1handle *pp1data,
	uint64_t n,
	uint64_t d,
	int	last_mul)	
{
	uint64_t e, t;
	gwnum	A, B, C;
	A = pp1data->V;
	B = pp1data->Vn;
	C = pp1data->Vn1;

	luc_dbl (&pp1data->gwdata, A, B);				
									
	e = n - d;
	d = d - e;

	if (e > d && (100 * e <= 296 * d)) {
		swap (d, e);
		gwswap (A, B);							
		luc_add (&pp1data->gwdata, A, B, B, C);				
		gwswap (B, C);							
		d = d-e;
	}
	else if (d > e && (100 * d <= 296 * e)) {
		luc_add (&pp1data->gwdata, A, B, A, C);				
		gwswap (B, C);							
		d = d-e;
	} else {
		gwcopy (&pp1data->gwdata, A, C);				
	}

	while (d != e) {
		if (d < e) {
			swap (d, e);
			gwswap (A, B);
		}
		if (100 * d <= 296 * e) {					
			luc_add (&pp1data->gwdata, A, B, C, C);			
			gwswap (B, C);						
			d = d-e;
		} else if ((d&1) == (e&1)) {
			luc_add (&pp1data->gwdata, A, B, C, B);			
			luc_dbl (&pp1data->gwdata, A, A);			
			d = (d-e) >> 1;
		} else if ((d&1) == 0) {
			luc_add (&pp1data->gwdata, A, C, B, C);			
			luc_dbl (&pp1data->gwdata, A, A);			
			d = d >> 1;

		} else {
			luc_add (&pp1data->gwdata, A, B, C, C);			
			gwswap (B, C);						
			d = d-e;

		}
	}

	if (!last_mul)
		luc_add (&pp1data->gwdata, A, B, C, A);				
	else
		luc_add_last (&pp1data->gwdata, A, B, C, A);			

	pp1data->V = A;
	pp1data->Vn = B;
	pp1data->Vn1 = C;
}
#undef swap
#undef PP1_ADD_COST
#undef PP1_DBL_COST

void pp1_mul (
	pp1handle *pp1data,
	uint64_t multiplier,
	int	last_mul)		
{
	ASSERTG (multiplier == 2 || (multiplier > 2 && (multiplier & 1)));

	if (multiplier > 12) {
		int	c, min;
		uint64_t n, d, mind;

		n = multiplier;
		mind = (uint64_t) ceil((double) 0.6180339887498948 * n);		
		min = pp1_lucas_cost_several (n, &mind);

		d = (uint64_t) ceil ((double) 0.7236067977499790 * n);			
		c = pp1_lucas_cost_several (n, &d);
		if (c < min) min = c, mind = d;

		d = (uint64_t) ceil ((double) 0.5801787282954641 * n);			
		c = pp1_lucas_cost_several (n, &d);
		if (c < min) min = c, mind = d;

		d = (uint64_t) ceil ((double) 0.6328398060887063 * n);			
		c = pp1_lucas_cost_several (n, &d);
		if (c < min) min = c, mind = d;

		d = (uint64_t) ceil ((double) 0.6124299495094950 * n);			
		c = pp1_lucas_cost_several (n, &d);
		if (c < min) min = c, mind = d;

		d = (uint64_t) ceil ((double) 0.6201819808074158 * n);			
		c = pp1_lucas_cost_several (n, &d);
		if (c < min) min = c, mind = d;

		d = (uint64_t) ceil ((double) 0.6172146165344039 * n);			
		c = pp1_lucas_cost_several (n, &d);
		if (c < min) min = c, mind = d;

		d = (uint64_t) ceil ((double) 0.6183471196562281 * n);			
		c = pp1_lucas_cost_several (n, &d);
		if (c < min) min = c, mind = d;

		d = (uint64_t) ceil ((double) 0.6179144065288179 * n);			
		c = pp1_lucas_cost_several (n, &d);
		if (c < min) min = c, mind = d;

		d = (uint64_t) ceil ((double) 0.6180796684698958 * n);			
		c = pp1_lucas_cost_several (n, &d);
		if (c < min) min = c, mind = d;

		pp1_lucas_mul (pp1data, n, mind, last_mul);
	}

	else {
		gwnum	V, Vn, Vn1;
		uint64_t mask;
		int	last_mul_options = last_mul ? GWMUL_STARTNEXTFFT : 0;

		V = pp1data->V;
		Vn = pp1data->Vn;
		Vn1 = pp1data->Vn1;

		if (multiplier == 2) {
			gwsquare2 (&pp1data->gwdata, V, V, GWMUL_ADDINCONST | last_mul_options);
			return;
		}
		if (multiplier == 3) {
			gwsquare2 (&pp1data->gwdata, V, Vn, GWMUL_FFT_S1 | GWMUL_ADDINCONST | GWMUL_STARTNEXTFFT);	
			gwmulsub4 (&pp1data->gwdata, V, Vn, V, V, last_mul_options);					
			return;
		}

		for (mask = 1ULL << 63; (multiplier & mask) == 0; mask >>= 1);

		mask >>= 1;
		if (multiplier & mask) {
			gwsquare2 (&pp1data->gwdata, V, Vn1, GWMUL_FFT_S1 | GWMUL_ADDINCONST | GWMUL_STARTNEXTFFT);	
			gwmulsub4 (&pp1data->gwdata, V, Vn1, V, Vn, GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);			
			gwsquare2 (&pp1data->gwdata, Vn1, Vn1, GWMUL_ADDINCONST | GWMUL_STARTNEXTFFT);			
		}
		else {
			gwsquare2 (&pp1data->gwdata, V, Vn, GWMUL_FFT_S1 | GWMUL_ADDINCONST | GWMUL_STARTNEXTFFT);	
			gwmulsub4 (&pp1data->gwdata, V, Vn, V, Vn1, GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);			
		}

		gwfft_for_fma (&pp1data->gwdata, V, V);	
		for (mask >>= 1; ; mask >>= 1) {
			if (mask == 1) {
				gwmulsub4 (&pp1data->gwdata, Vn, Vn1, V, V, last_mul_options);				
				break;
			}
			if (multiplier & mask) {
				gwmulsub4 (&pp1data->gwdata, Vn, Vn1, V, Vn, GWMUL_FFT_S2 | GWMUL_STARTNEXTFFT);	
				gwsquare2 (&pp1data->gwdata, Vn1, Vn1, GWMUL_ADDINCONST | GWMUL_STARTNEXTFFT);		
			}
			else {
				gwmulsub4 (&pp1data->gwdata, Vn, Vn1, V, Vn1, GWMUL_FFT_S1 | GWMUL_STARTNEXTFFT);	
				gwsquare2 (&pp1data->gwdata, Vn, Vn, GWMUL_ADDINCONST | GWMUL_STARTNEXTFFT);		
			}
		}
	}
}

int pplus1 (
	int	thread_num,
	struct PriorityInfo *sp_info,	
	struct work_unit *w)
{
	pp1handle pp1data;
	giant	N;		
	giant	factor;		
	unsigned int memused;
	int	i, res, stop_reason, first_iter_msg, saving, near_fft_limit, echk;
	char	filename[32], buf[255], JSONbuf[4000], testnum[100];
	uint64_t sieve_start, next_prime;
	double	one_over_B, base_pct_complete, one_relp_pct;
	uint64_t last_output, last_output_t, last_output_r;
	double	allowable_maxerr, output_frequency, output_title_frequency;
	int	maxerr_restart_count = 0;
	char	*str, *msg;
	int	msglen;
	double	timers[2];
	relp_set_data_map relp_set_map;
	Dmultiple_data_map Dmultiple_map;

	OutputStr (thread_num, "\n");

	N = NULL;
	factor = NULL;
	str = NULL;
	msg = NULL;

	memset (&pp1data, 0, sizeof (pp1handle));
	pp1data.thread_num = thread_num;
	pp1data.stage1_threads = get_worker_num_threads (thread_num, HYPERTHREAD_LL);
	pp1data.stage2_threads = pp1data.stage1_threads + IniGetInt (INI_FILE, "Stage2ExtraThreads", 0);
	pp1data.w = w;
	pp1data.B = (uint64_t) w->B1;
	pp1data.C = (uint64_t) w->B2;
	if (pp1data.B < 90) {
		OutputStr (thread_num, "Using minimum bound #1 of 90\n");
		pp1data.B = 90;
	}
	if (pp1data.C == 0) pp1data.C = pp1data.B * 100;
	if (pp1data.C < pp1data.B) pp1data.C = pp1data.B;
	pp1data.pct_mem_to_use = 1.0;				

	if (w->nth_run <= 1) {				
		pp1data.numerator = 2;
		pp1data.denominator = 7;
		pp1data.takeAwayBits = log2 (6.0);	
		pp1data.success_rate = 1.0;
	} else if (w->nth_run == 2) {			
		pp1data.numerator = 6;
		pp1data.denominator = 5;
		pp1data.takeAwayBits = log2 (4.0);	
		pp1data.success_rate = 0.5;		
	} else {
		unsigned char small_primes[16] = {11,13,17,19,23,29,31,37,41,43,47,53,59,61,67,71};
		srand ((unsigned) time (NULL));
		pp1data.numerator = 72 + (rand () & 0x7F);
		pp1data.denominator = small_primes[rand() & 0xF];
		pp1data.takeAwayBits = log2 (2.0);	
		pp1data.success_rate = pow (0.5, (double) w->nth_run); 
	}

	pp1data.optimal_B2 = (!QA_IN_PROGRESS && w->sieve_depth > 50 && IniGetInt (INI_FILE, "Pplus1BestB2", 1));
	if (pp1data.optimal_B2 && pp1data.C <= pp1data.B) pp1data.C = 100 * pp1data.B;	

	stop_reason = setN (thread_num, w, &N);
	if (stop_reason) goto exit;

	PRAC_SEARCH = IniGetInt (INI_FILE, "PracSearch", 7);
	if (PRAC_SEARCH < 1) PRAC_SEARCH = 1;
	if (PRAC_SEARCH > 50) PRAC_SEARCH = 50;

	gw_as_string (testnum, w->k, w->b, w->n, w->c);
	sprintf (buf, "%s P+1", testnum);
	title (thread_num, buf);
	
	sprintf (buf, "P+1 on %s, ", testnum);
	
	if (w->nth_run <= 2) sprintf (buf+strlen(buf), "start=%" PRIu32 "/%" PRIu32 ", ", pp1data.numerator, pp1data.denominator);
	else sprintf (buf+strlen(buf), "random start, ");
	if (pp1data.C <= pp1data.B) sprintf (buf+strlen(buf), "B1=%" PRIu64 "\n", pp1data.B);
	else if (pp1data.optimal_B2) sprintf (buf+strlen(buf), "B1=%" PRIu64 ", B2=TBD\n", pp1data.B);
	else sprintf (buf+strlen(buf), "B1=%" PRIu64 ", B2=%" PRIu64 "\n", pp1data.B, pp1data.C);
	OutputStr (thread_num, buf);
	if (w->sieve_depth > 0.0 && !pp1data.optimal_B2) {
		double prob = pm1prob (pp1data.takeAwayBits, (unsigned int) w->sieve_depth, pp1data.B, pp1data.C) * pp1data.success_rate;
		sprintf (buf, "Chance of finding a new factor assuming no ECM has been done is an estimated %.3g%%\n", prob * 100.0);
		OutputStr (thread_num, buf);
	}

restart:
	tempFileName (w, filename);
	filename[0] = 'n';

	gwinit (&pp1data.gwdata);
	if (IniGetInt (INI_FILE, "UseLargePages", 0)) gwset_use_large_pages (&pp1data.gwdata);
	if (HYPERTHREAD_LL) sp_info->normal_work_hyperthreading = TRUE, gwset_will_hyperthread (&pp1data.gwdata, 2);
	gwset_bench_cores (&pp1data.gwdata, HW_NUM_CORES);
	gwset_bench_workers (&pp1data.gwdata, NUM_WORKERS);
	if (ERRCHK) gwset_will_error_check (&pp1data.gwdata);
	else gwset_will_error_check_near_limit (&pp1data.gwdata);
	gwset_num_threads (&pp1data.gwdata, pp1data.stage1_threads);
	gwset_thread_callback (&pp1data.gwdata, SetAuxThreadPriority);
	gwset_thread_callback_data (&pp1data.gwdata, sp_info);
	gwset_safety_margin (&pp1data.gwdata, IniGetFloat (INI_FILE, "ExtraSafetyMargin", 0.0));
	gwset_larger_fftlen_count (&pp1data.gwdata, maxerr_restart_count < 3 ? maxerr_restart_count : 3);
	gwset_minimum_fftlen (&pp1data.gwdata, w->minimum_fftlen);
	gwset_use_spin_wait (&pp1data.gwdata, IniGetInt (INI_FILE, "SpinWait", 0));
	res = gwsetup (&pp1data.gwdata, w->k, w->b, w->n, w->c);
	if (res) {
		sprintf (buf, "Cannot initialize FFT code, errcode=%d\n", res);
		OutputBoth (thread_num, buf);
		return (STOP_FATAL_ERROR);
	}
	gwerror_checking (&pp1data.gwdata, ERRCHK);
	pp1data.stage1_fftlen = gwfftlen (&pp1data.gwdata);
	gwsetaddin (&pp1data.gwdata, -2);

	pp1data.V = gwalloc (&pp1data.gwdata);
	if (pp1data.V == NULL) goto oom;
	pp1data.Vn = gwalloc (&pp1data.gwdata);
	if (pp1data.Vn == NULL) goto oom;
	pp1data.Vn1 = gwalloc (&pp1data.gwdata);
	if (pp1data.Vn1 == NULL) goto oom;

	last_output = last_output_t = last_output_r = 0;
	gw_clear_fft_count (&pp1data.gwdata);
	first_iter_msg = TRUE;
	calc_output_frequencies (&pp1data.gwdata, &output_frequency, &output_title_frequency);

	{
		char	fft_desc[200];
		gwfft_description (&pp1data.gwdata, fft_desc);
		sprintf (buf, "Using %s\n", fft_desc);
		OutputStr (thread_num, buf);
	}

	near_fft_limit = exponent_near_fft_limit (&pp1data.gwdata);
	gwerror_checking (&pp1data.gwdata, ERRCHK || near_fft_limit);

	allowable_maxerr = IniGetFloat (INI_FILE, "MaxRoundoffError", (float) 0.4375);

	readSaveFileStateInit (&pp1data.read_save_file_state, thread_num, filename);
	writeSaveFileStateInit (&pp1data.write_save_file_state, filename, 0);
	for ( ; ; ) {
		if (! saveFileExists (&pp1data.read_save_file_state)) {
			
			if (pp1data.read_save_file_state.a_non_bad_save_file_existed) {
				OutputBoth (thread_num, ALLSAVEBAD_MSG);
				return (0);
			}
			
			break;
		}

		if (!pp1_restore (&pp1data)) {
			
			saveFileBad (&pp1data.read_save_file_state);
			continue;
		}

		if (pp1data.state == PP1_STATE_STAGE1) {
			sieve_start = pp1data.stage1_prime + 1;
			if (pp1data.interim_B > pp1data.B) pp1data.interim_B = pp1data.B;
			goto restart1;
		}

		if (pp1data.state == PP1_STATE_MIDSTAGE) {
			if (pp1data.B > pp1data.B_done) {
				gwfree (&pp1data.gwdata, pp1data.gg), pp1data.gg = NULL;
				goto more_B;
			}
			goto restart3b;
		}

		if (pp1data.state == PP1_STATE_STAGE2) {

			if (pp1data.B > pp1data.B_done) {
				gwfree (&pp1data.gwdata, pp1data.gg), pp1data.gg = NULL;
				free (pp1data.pairmap), pp1data.pairmap = NULL;
				goto more_B;
			}

			if (pp1data.B != pp1data.B_done) {
				pp1data.B = pp1data.B_done;
				sprintf (buf, "Ignoring suggested B1 value, using B1=%" PRIu64 " from the save file\n", pp1data.B);
				OutputStr (thread_num, buf);
			}

			if (pp1data.C < pp1data.interim_C) {
				pp1data.C = pp1data.interim_C;
				sprintf (buf, "Ignoring suggested B2 value, using B2=%" PRIu64 " from the save file\n", pp1data.C);
				OutputStr (thread_num, buf);
			}

			if (pp1data.optimal_B2) {
				pp1data.C = pp1data.interim_C;
				sprintf (buf, "Resuming P+1 in stage 2 with B2=%" PRIu64 "\n", pp1data.interim_C);
				OutputStr (thread_num, buf);
			}
			goto restart3b;
		}

		if (pp1data.state == PP1_STATE_GCD) {
			if (pp1data.optimal_B2) pp1data.C = pp1data.C_done;
			goto restart4;
		}

		ASSERTG (pp1data.state == PP1_STATE_DONE);
		if (pp1data.B > pp1data.B_done) goto more_B;
		if (pp1data.C > pp1data.C_done) {
			pp1data.state = PP1_STATE_MIDSTAGE;
			goto restart3a;
		}

		sprintf (buf, "%s already tested to B1=%" PRIu64 " and B2=%" PRIu64 ".\n",
			 gwmodulo_as_string (&pp1data.gwdata), pp1data.B_done, pp1data.C_done);
		OutputBoth (thread_num, buf);
		goto done;
	}

	pp1_calc_start (&pp1data.gwdata, pp1data.numerator, pp1data.denominator, N, pp1data.V);	
	pp1data.B_done = 0;
	pp1data.interim_B = pp1data.B;
	sieve_start = 2;

restart1:
	strcpy (w->stage, "S1");
	pp1data.state = PP1_STATE_STAGE1;
	set_memory_usage (thread_num, 0, cvt_gwnums_to_mem (&pp1data.gwdata, 3));
	start_timer_from_zero (timers, 0);
	start_timer_from_zero (timers, 1);
	stop_reason = start_sieve_with_limit (thread_num, sieve_start, (uint32_t) sqrt ((double) pp1data.C), &pp1data.sieve_info);
	if (stop_reason) goto exit;
	pp1data.stage1_prime = sieve (pp1data.sieve_info);
	one_over_B = 1.0 / (double) pp1data.B;
	w->pct_complete = (double) pp1data.stage1_prime * one_over_B;
	for ( ; pp1data.stage1_prime <= pp1data.interim_B; pp1data.stage1_prime = next_prime) {
		uint64_t mult, max;
		int	count;

		next_prime = sieve (pp1data.sieve_info);

		stop_reason = stopCheck (thread_num);
		saving = testSaveFilesFlag (thread_num);
		echk = stop_reason || saving || ERRCHK || near_fft_limit || ((pp1data.stage1_prime & 255) == 255);
		gwerror_checking (&pp1data.gwdata, echk);

		count = 0;
		max = pp1data.interim_B / pp1data.stage1_prime;
		for (mult = pp1data.stage1_prime; ; mult *= pp1data.stage1_prime) {
			if (mult > pp1data.B_done) count++;
			if (mult > max) break;
		}

		if (pp1data.stage1_prime <= 3 && pp1data.B_done == 0) count += (pp1data.stage1_prime == 2) ? 2 : 1;

		if (pp1data.stage1_prime == 2 && pp1data.B_done == 0) {
			if (count < (int) log2 (w->n) + 3) count = (int) log2 (w->n) + 3;
			for ( ; count; count--) {
				gwmul3_carefully (&pp1data.gwdata, pp1data.V, pp1data.V, pp1data.V, GWMUL_ADDINCONST);
			}
			
			gwfree_internal_memory (&pp1data.gwdata);
			
			if (w->n > pp1data.B && (isMersenne (w->k, w->b, w->n, w->c) || isGeneralizedFermat (w->k, w->b, w->n, w->c)))
				pp1_mul (&pp1data, w->n, FALSE);
		}

		else {
			for ( ; count; count--) {
				int	last_mul = (count == 1 && (saving || stop_reason || next_prime > pp1data.B));
				pp1_mul (&pp1data, pp1data.stage1_prime, last_mul);
			}
		}

		if (gw_test_for_error (&pp1data.gwdata) || gw_get_maxerr (&pp1data.gwdata) > allowable_maxerr) goto err;

		w->pct_complete = (double) pp1data.stage1_prime * one_over_B;

		if (first_iter_msg ||
		    (ITER_OUTPUT != 999999999 && gw_get_fft_count (&pp1data.gwdata) >= last_output_t + 2 * ITER_OUTPUT * output_title_frequency)) {
			sprintf (buf, "%.*f%% of %s P+1 stage 1",
				 (int) PRECISION, trunc_percent (w->pct_complete), gwmodulo_as_string (&pp1data.gwdata));
			title (thread_num, buf);
			last_output_t = gw_get_fft_count (&pp1data.gwdata);
		}

		if (first_iter_msg ||
		    (ITER_OUTPUT != 999999999 && gw_get_fft_count (&pp1data.gwdata) >= last_output + 2 * ITER_OUTPUT * output_frequency)) {
			sprintf (buf, "%s stage 1 is %.*f%% complete.",
				 gwmodulo_as_string (&pp1data.gwdata), (int) PRECISION, trunc_percent (w->pct_complete));
			end_timer (timers, 0);
			if (first_iter_msg) {
				strcat (buf, "\n");
			} else {
				strcat (buf, " Time: ");
				print_timer (timers, 0, buf, TIMER_NL);
			}
			if (pp1data.stage1_prime != 2 || pp1data.B_done != 0) OutputStr (thread_num, buf);
			start_timer_from_zero (timers, 0);
			last_output = gw_get_fft_count (&pp1data.gwdata);
			first_iter_msg = FALSE;
		}

		if ((ITER_OUTPUT_RES != 999999999 && gw_get_fft_count (&pp1data.gwdata) >= last_output_r + 2 * ITER_OUTPUT_RES) ||
		    (NO_GUI && stop_reason)) {
			sprintf (buf, "%s stage 1 is %.*f%% complete.\n",
				 gwmodulo_as_string (&pp1data.gwdata), (int) PRECISION, trunc_percent (w->pct_complete));
			writeResults (buf);
			last_output_r = gw_get_fft_count (&pp1data.gwdata);
		}

		if (stop_reason || saving) {
			pp1_save (&pp1data);
			if (stop_reason) goto exit;
		}
	}

	pp1data.B_done = pp1data.interim_B;
	end_timer (timers, 0);
	end_timer (timers, 1);

	if (pp1data.B > pp1data.B_done) {
more_B:		pp1data.interim_B = pp1data.B;
		sieve_start = 1;

		goto restart1;
	}
	pp1data.C_done = pp1data.B;

	gwerror_checking (&pp1data.gwdata, ERRCHK || near_fft_limit);

	sprintf (buf, "%s stage 1 complete. %" PRIu64 " transforms. Total time: ", gwmodulo_as_string (&pp1data.gwdata), gw_get_fft_count (&pp1data.gwdata));
	print_timer (timers, 1, buf, TIMER_NL | TIMER_CLR);
	OutputStr (thread_num, buf);
	gw_clear_fft_count (&pp1data.gwdata);

	if (ERRCHK) {
		sprintf (buf, "Round off: %.10g\n", gw_get_maxerr (&pp1data.gwdata));
		OutputStr (thread_num, buf);
		gw_clear_maxerr (&pp1data.gwdata);
	}

	strcpy (w->stage, "S1");
	w->pct_complete = 1.0;
	if (pp1data.C <= pp1data.B || (!QA_IN_PROGRESS && IniGetInt (INI_FILE, "Stage1GCD", 1))) {
		start_timer_from_zero (timers, 0);
		gwsmalladd (&pp1data.gwdata, -2, pp1data.V);
		stop_reason = gcd (&pp1data.gwdata, thread_num, pp1data.V, N, &factor);
		gwsmalladd (&pp1data.gwdata, 2, pp1data.V);
		if (stop_reason) {
			pp1_save (&pp1data);
			goto exit;
		}
		end_timer (timers, 0);
		strcpy (buf, "Stage 1 GCD complete. Time: ");
		print_timer (timers, 0, buf, TIMER_NL);
		OutputStr (thread_num, buf);
		if (factor != NULL) goto bingo;
	}

	if (pp1data.C <= pp1data.B) goto msg_and_exit;

restart3a:

	pp1data.state = PP1_STATE_MIDSTAGE;
	strcpy (w->stage, "S2");
	w->pct_complete = 0.0;

	if (IniGetInt (INI_FILE, "SaveBeforeStage2", 1)) pp1_save (&pp1data);

restart3b:
more_C:
	start_timer_from_zero (timers, 0);
	sprintf (buf, "%s P+1 stage 2 init", gwmodulo_as_string (&pp1data.gwdata));
	title (thread_num, buf);

	clear_restart_if_max_memory_change (thread_num);

replan:	{
		unsigned long min_memory = cvt_gwnums_to_mem (&pp1data.gwdata, 8);
		if (max_mem (thread_num) < min_memory) {
			sprintf (buf, "Insufficient memory to ever run stage 2 -- %luMB needed.\n", min_memory);
			OutputStr (thread_num, buf);
			pp1data.C = pp1data.B_done;
			goto restart4;
		}
	}

	stop_reason = pp1_stage2_impl (&pp1data);
	if (stop_reason) {
		if (pp1data.state == PP1_STATE_MIDSTAGE) pp1_save (&pp1data);
		goto exit;
	}

	memused = cvt_gwnums_to_mem (&pp1data.gwdata, pp1data.stage2_numvals);
	memused += (int) (pp1data.pairmap_size >> 20);
	
	if (set_memory_usage (thread_num, MEM_VARIABLE_USAGE, memused)) {
		pp1data.pct_mem_to_use *= 0.99;
		free (pp1data.pairmap); pp1data.pairmap = NULL;
		goto replan;
	}

	sprintf (buf, "Using %uMB of memory.\n", memused);
	OutputStr (thread_num, buf);

	process_relp_sets (pp1data.totrels, pp1data.numrels, pp1data.relp_sets, relp_set_map, Dmultiple_map);

	base_pct_complete = (double) (pp1data.B2_start - pp1data.last_relocatable) / (double) (pp1data.C - pp1data.last_relocatable);
	
	one_relp_pct = (1.0 - base_pct_complete) / (double) (pp1data.numDsections * pp1data.totrels);
	
	w->pct_complete = base_pct_complete + (double) (pp1data.Dsection * pp1data.totrels) * one_relp_pct;

	int	num_relp_sets;		
	num_relp_sets = (int) relp_set_map.size();
	pp1data.nQx = (gwnum *) malloc (num_relp_sets * pp1data.numrels * sizeof (gwnum));
	if (pp1data.nQx == NULL) goto lowmem;

	int	set_minus1_nQx_index;	
	set_minus1_nQx_index = relp_set_map.find(-1)->second.nQx_store;
	
	#define nqxmap(relp)	((relp) < pp1data.numrels ? (relp) : set_minus1_nQx_index + (relp) - pp1data.numrels)

	ASSERTG (pp1data.D % 3 == 0);
	{
		gwnum	t1, t2, t3, t4, t5;
		gwnum	V1, V2, V3, V5, V6, V7, V11;
		struct {
			gwnum	Vi;
			gwnum	Vi_minus6;
			int	Vi_is_relp;
			int	Vi_minus6_is_relp;
		} V1mod6, V5mod6, *curr, *notcurr;
		int	i_gap, totrels, have_VD;

		t1 = gwalloc (&pp1data.gwdata);
		if (t1 == NULL) goto lowmem;
		t2 = gwalloc (&pp1data.gwdata);
		if (t2 == NULL) goto lowmem;
		t3 = gwalloc (&pp1data.gwdata);
		if (t3 == NULL) goto lowmem;
		t4 = pp1data.Vn;
		t5 = pp1data.Vn1;

		luc_dbl (&pp1data.gwdata, pp1data.V, t1);			
		gwswap (t1, pp1data.V);
		V1 = t1;
		V2 = pp1data.V;

		V3 = t2;
		luc_add (&pp1data.gwdata, V2, V1, V1, V3);			

		V5 = t3;
		luc_add (&pp1data.gwdata, V3, V2, V1, V5);			

		V6 = V3;
		luc_dbl (&pp1data.gwdata, V3, V6);				

		V7 = t4;
		luc_add (&pp1data.gwdata, V6, V1, V5, V7);			

		V11 = t5;
		luc_add (&pp1data.gwdata, V6, V5, V1, V11);			

		V1mod6.Vi_minus6 = V1;
		V1mod6.Vi_minus6_is_relp = 1;
		V5mod6.Vi_minus6 = V5;
		V5mod6.Vi_minus6_is_relp = relatively_prime (5, pp1data.D);
		V1mod6.Vi = V7;
		V1mod6.Vi_is_relp = relatively_prime (7, pp1data.D);
		V5mod6.Vi = V11;
		V5mod6.Vi_is_relp = relatively_prime (11, pp1data.D);
		pp1data.nQx[0] = V1; totrels = 1;
		if (V5mod6.Vi_minus6_is_relp) pp1data.nQx[totrels++] = V5;
		if (V1mod6.Vi_is_relp) pp1data.nQx[totrels++] = V7;
		if (V5mod6.Vi_is_relp) pp1data.nQx[totrels++] = V11;

		have_VD = FALSE;
		for (i = 7, i_gap = 4; ; i += i_gap, i_gap = 6 - i_gap) {
			gwnum	next_i;
			int	next_i_is_relp;

			curr = (i_gap == 4) ? &V1mod6 : &V5mod6;

			if (i + (i + i_gap) == pp1data.D) {
				gwnum	Vgap = V2;
				ASSERTG (Vgap == pp1data.V);
				if (i_gap == 4) luc_dbl (&pp1data.gwdata, Vgap, Vgap);			
				luc_add (&pp1data.gwdata, V1mod6.Vi, V5mod6.Vi, Vgap, pp1data.V);	
				have_VD = TRUE;
			}

			if (have_VD && (totrels == pp1data.totrels || totrels == pp1data.numrels * 2)) break;

			next_i_is_relp = (relatively_prime (i+6, pp1data.D) && totrels < pp1data.totrels);

			if ((next_i_is_relp && have_VD && (totrels+1 == pp1data.totrels || totrels+1 == pp1data.numrels * 2)) ||
			    (!have_VD && i+6 > pp1data.D/2 && pp1data.totrels == pp1data.numrels)) {
				notcurr = (curr == &V5mod6) ? &V1mod6 : &V5mod6;
				if (!notcurr->Vi_is_relp) gwfree (&pp1data.gwdata, notcurr->Vi), notcurr->Vi = NULL;
				if (!notcurr->Vi_minus6_is_relp) gwfree (&pp1data.gwdata, notcurr->Vi_minus6), notcurr->Vi_minus6 = NULL;
			}			

			if (curr->Vi_minus6_is_relp) {
				next_i = gwalloc (&pp1data.gwdata);
				if (next_i == NULL) goto lowmem;
			} else
				next_i = curr->Vi_minus6;

			luc_add (&pp1data.gwdata, curr->Vi, V6, curr->Vi_minus6, next_i);	
			if (next_i_is_relp) {
				pp1data.nQx[nqxmap(totrels)] = next_i;
				totrels++;
			}

			curr->Vi_minus6 = curr->Vi;
			curr->Vi_minus6_is_relp = curr->Vi_is_relp;
			curr->Vi = next_i;
			curr->Vi_is_relp = next_i_is_relp;

			if (gw_test_for_error (&pp1data.gwdata)) goto err;
			stop_reason = stopCheck (thread_num);
			if (stop_reason) goto possible_lowmem;
		}

		if (!V1mod6.Vi_is_relp) gwfree (&pp1data.gwdata, V1mod6.Vi);
		if (!V1mod6.Vi_minus6_is_relp) gwfree (&pp1data.gwdata, V1mod6.Vi_minus6);
		if (!V5mod6.Vi_is_relp) gwfree (&pp1data.gwdata, V5mod6.Vi);
		if (!V5mod6.Vi_minus6_is_relp) gwfree (&pp1data.gwdata, V5mod6.Vi_minus6);
		gwfree (&pp1data.gwdata, V6);

		pp1data.Vn = gwalloc (&pp1data.gwdata);
		pp1data.Vn1 = gwalloc (&pp1data.gwdata);
	}
	#undef nqxmap

	{
		auto it_Dmult = Dmultiple_map.find (1);
		if (it_Dmult == Dmultiple_map.end()) it_Dmult = Dmultiple_map.insert({1, {0, 1}}).first;
		it_Dmult->second.set_val_buffers (&pp1data.gwdata, pp1data.V, TRUE);

		uint64_t D_multiplier = (pp1data.B2_start + pp1data.D / 2) / pp1data.D + pp1data.Dsection;
		ASSERTG (D_multiplier > 2); 
		it_Dmult = Dmultiple_map.find (D_multiplier - 1);
		if (it_Dmult == Dmultiple_map.end()) it_Dmult = Dmultiple_map.insert({D_multiplier - 1, {0, D_multiplier - 1}}).first;
		it_Dmult->second.set_val_buffers (&pp1data.gwdata, pp1data.Vn, TRUE);

		it_Dmult = Dmultiple_map.find (D_multiplier);
		if (it_Dmult == Dmultiple_map.end()) it_Dmult = Dmultiple_map.insert({D_multiplier, {0, D_multiplier}}).first;
		it_Dmult->second.set_val_buffers (&pp1data.gwdata, pp1data.Vn1, TRUE);
	}

	process_Dmult_map (Dmultiple_map);
	for (auto this_Dmult = Dmultiple_map.begin(); this_Dmult != Dmultiple_map.end(); ++this_Dmult) {
		
		if (this_Dmult->first == 1) continue;
		
		if (this_Dmult->second.val.x == NULL) {
			gwnum	tmp = gwalloc (&pp1data.gwdata);
			if (tmp == NULL) goto lowmem;
			this_Dmult->second.set_val_buffers (&pp1data.gwdata, tmp, FALSE);
		}
		
		auto it_base = Dmultiple_map.find (this_Dmult->second.base);
		if (this_Dmult->second.diff == 0) {
			luc_dbl (&pp1data.gwdata, it_base->second.val.x, this_Dmult->second.val.x);
		}
		
		else {
			auto it_addin = Dmultiple_map.find (this_Dmult->second.addin);
			auto it_diff = Dmultiple_map.find (this_Dmult->second.diff);
			luc_add (&pp1data.gwdata, it_base->second.val.x, it_addin->second.val.x, it_diff->second.val.x, this_Dmult->second.val.x);
			
			it_addin->second.free_if_Dmult_last_used_by (this_Dmult->first);
			it_diff->second.free_if_Dmult_last_used_by (this_Dmult->first);
		}
		
		it_base->second.free_if_Dmult_last_used_by (this_Dmult->first);
		
		if (gw_test_for_error (&pp1data.gwdata)) goto err;
		stop_reason = stopCheck (thread_num);
		if (stop_reason) goto possible_lowmem;
	}

	gwfree_internal_memory (&pp1data.gwdata);

	int	num_partials;
	num_partials = one_based_modulo (pp1data.totrels, pp1data.numrels);
	
	for (auto it_relp_set = relp_set_map.begin (); it_relp_set != relp_set_map.end (); ++it_relp_set) {
		
		if (it_relp_set->first == 0 || it_relp_set->first == -1) continue;
		
		auto it_base_relp_set = relp_set_map.find (it_relp_set->second.base);
		auto it_diff_relp_set = relp_set_map.find (it_relp_set->second.diff);
		
		auto it_Dmultiple = Dmultiple_map.find (it_relp_set->second.Dmultiple);
		
		bool	diff_access_inverted = (it_base_relp_set->first < 0) ^ (it_diff_relp_set->first < 0);
		
		for (i = (it_relp_set->second.partial ? num_partials : pp1data.numrels) - 1; i >= 0; i--) {
			gwnum	*Vbase, *Vdiff, Vnew;
			int	ni;

			ni = it_base_relp_set->second.nQx_store + i;
			Vbase = &pp1data.nQx[ni];
			
			ni = it_diff_relp_set->second.nQx_store;
			if (!diff_access_inverted) ni += i; else ni += (pp1data.numrels-1)-i;
			Vdiff = &pp1data.nQx[ni];

			bool Vbase_to_be_freed = (!it_base_relp_set->second.permanent &&
						  it_base_relp_set->second.is_last_use (it_relp_set->first, i, num_partials, pp1data.numrels, FALSE));
			bool Vdiff_to_be_freed = (!it_diff_relp_set->second.permanent &&
						  it_diff_relp_set->second.is_last_use (it_relp_set->first, i, num_partials, pp1data.numrels, diff_access_inverted));
			bool Dmultiple_to_be_freed = (i == 0 && it_Dmultiple->second.will_free_due_to_relp_last_used_by (it_relp_set->first));

			if (Vbase_to_be_freed) Vnew = *Vbase;
			else if (Vdiff_to_be_freed) Vnew = *Vdiff;
			else if (Dmultiple_to_be_freed) Vnew = it_Dmultiple->second.val.x;
			else {
				Vnew = gwalloc (&pp1data.gwdata);
				if (Vnew == NULL) goto lowmem;
			}

			luc_add (&pp1data.gwdata, *Vbase, it_Dmultiple->second.val.x, *Vdiff, Vnew);	

			ni = it_relp_set->second.nQx_store + i;
			pp1data.nQx[ni] = Vnew;

			if (Vbase_to_be_freed && Vnew != *Vbase) {
				gwfree (&pp1data.gwdata, *Vbase);
				*Vbase = NULL;
			}

			if (Vdiff_to_be_freed && Vnew != *Vdiff) {
				gwfree (&pp1data.gwdata, *Vdiff);
				*Vdiff = NULL;
			}

			if (Dmultiple_to_be_freed) {
				it_Dmultiple->second.do_not_free_val = (Vnew == it_Dmultiple->second.val.x);
				it_Dmultiple->second.free_if_relp_last_used_by (it_relp_set->first);
			}

			if (gw_test_for_error (&pp1data.gwdata)) goto err;
			stop_reason = stopCheck (thread_num);
			if (stop_reason) goto possible_lowmem;
		}
	}

	Dmultiple_map.clear ();
	relp_set_map.clear ();

	if (pp1data.gg == NULL) {
		pp1data.gg = gwalloc (&pp1data.gwdata);
		if (pp1data.gg == NULL) goto oom;
		dbltogw (&pp1data.gwdata, 1.0, pp1data.gg);
		gwmul3 (&pp1data.gwdata, pp1data.Vn1, pp1data.gg, pp1data.gg, GWMUL_FFT_S1 | GWMUL_ADDINCONST | GWMUL_STARTNEXTFFT);
	}

	gwfree_cached (&pp1data.gwdata);
	mallocFreeForOS ();

	sprintf (buf, "%.*f%% of %s P+1 stage 2 (using %uMB)",
		 (int) PRECISION, trunc_percent (w->pct_complete), gwmodulo_as_string (&pp1data.gwdata), memused);
	title (thread_num, buf);

	end_timer (timers, 0);
	sprintf (buf, "Stage 2 init complete. %" PRIu64 " transforms. Time: ", gw_get_fft_count (&pp1data.gwdata));
	print_timer (timers, 0, buf, TIMER_NL | TIMER_CLR);
	OutputStr (thread_num, buf);
	gw_clear_fft_count (&pp1data.gwdata);

	pp1data.state = PP1_STATE_STAGE2;
	start_timer_from_zero (timers, 0);
	start_timer_from_zero (timers, 1);
	last_output = last_output_t = last_output_r = 0;
	for ( ; ; ) {

		pp1data.relp += next_pair (&pp1data.pairmap_ptr);

		if (pp1data.relp >= pp1data.totrels && pp1data.Dsection + pp1data.relp / pp1data.totrels >= pp1data.numDsections) break;

		while (pp1data.relp >= pp1data.totrels) {
			pp1data.Dsection++;
			pp1data.relp -= pp1data.totrels;
			luc_add (&pp1data.gwdata, pp1data.Vn1, pp1data.V, pp1data.Vn, pp1data.Vn);	
			gwswap (pp1data.Vn, pp1data.Vn1);						

		}

		if (pp1data.pairmap_ptr == pp1data.pairmap + pp1data.pairmap_size) {
			ASSERTG (pp1data.relp == 0);
			pp1data.C_done = pp1data.B2_start + pp1data.Dsection * pp1data.D;
			pp1data.first_relocatable = calc_new_first_relocatable (pp1data.D, pp1data.C_done);
			int fill_window = pair_window_size (pp1data.gwdata.bit_length, pp1data.relp_sets);
			stop_reason = fill_pairmap (pp1data.thread_num, &pp1data.sieve_info, pp1data.D, fill_window,0,0,0,
						    pp1data.totrels, pp1data.relp_sets+3, pp1data.first_relocatable, pp1data.last_relocatable,
						    pp1data.C_done, pp1data.C, pp1data.max_pairmap_Dsections, &pp1data.pairmap, &pp1data.pairmap_size);
			if (stop_reason) {

				pp1_save (&pp1data);
				goto exit;
			}
			pp1data.pairmap_ptr = pp1data.pairmap;
			pp1data.relp = -1;
			continue;
		}

		saving = testSaveFilesFlag (thread_num);
		stop_reason = stopCheck (thread_num);
		gwsubmul4 (&pp1data.gwdata, pp1data.Vn1, pp1data.nQx[pp1data.relp], pp1data.gg, pp1data.gg, (!stop_reason && !saving) ? GWMUL_STARTNEXTFFT : 0);

		w->pct_complete = base_pct_complete + (double) (pp1data.Dsection * pp1data.totrels + pp1data.relp) * one_relp_pct;

		if (gw_test_for_error (&pp1data.gwdata) || gw_get_maxerr (&pp1data.gwdata) > allowable_maxerr) goto err;

		if (first_iter_msg ||
		    (ITER_OUTPUT != 999999999 && gw_get_fft_count (&pp1data.gwdata) >= last_output_t + 2 * ITER_OUTPUT * output_title_frequency)) {
			sprintf (buf, "%.*f%% of %s P+1 stage 2 (using %uMB)",
				 (int) PRECISION, trunc_percent (w->pct_complete), gwmodulo_as_string (&pp1data.gwdata), memused);
			title (thread_num, buf);
			last_output_t = gw_get_fft_count (&pp1data.gwdata);
		}

		if (first_iter_msg ||
		    (ITER_OUTPUT != 999999999 && gw_get_fft_count (&pp1data.gwdata) >= last_output + 2 * ITER_OUTPUT * output_frequency)) {
			sprintf (buf, "%s stage 2 is %.*f%% complete.",
				 gwmodulo_as_string (&pp1data.gwdata), (int) PRECISION, trunc_percent (w->pct_complete));
			end_timer (timers, 0);
			if (first_iter_msg) {
				strcat (buf, "\n");
			} else {
				strcat (buf, " Time: ");
				print_timer (timers, 0, buf, TIMER_NL);
			}
			OutputStr (thread_num, buf);
			start_timer_from_zero (timers, 0);
			last_output = gw_get_fft_count (&pp1data.gwdata);
			first_iter_msg = FALSE;
		}

		if ((ITER_OUTPUT_RES != 999999999 && gw_get_fft_count (&pp1data.gwdata) >= last_output_r + 2 * ITER_OUTPUT_RES) ||
		    (NO_GUI && stop_reason)) {
			sprintf (buf, "%s stage 2 is %.*f%% complete.\n",
				 gwmodulo_as_string (&pp1data.gwdata), (int) PRECISION, trunc_percent (w->pct_complete));
			writeResults (buf);
			last_output_r = gw_get_fft_count (&pp1data.gwdata);
		}

		if (stop_reason || saving) {
			pp1_save (&pp1data);
			if (stop_reason) goto exit;
		}
	}
	pp1data.C_done = pp1data.interim_C;

	for (i = 0; i < pp1data.totrels; i++) gwfree (&pp1data.gwdata, pp1data.nQx[i]);
	free (pp1data.nQx), pp1data.nQx = NULL;
	free (pp1data.pairmap), pp1data.pairmap = NULL;

	if (pp1data.C > pp1data.C_done) {
		pp1data.state = PP1_STATE_MIDSTAGE;
		goto more_C;
	}

	gwfree_internal_memory (&pp1data.gwdata);
	mallocFreeForOS ();
	set_memory_usage (thread_num, 0, cvt_gwnums_to_mem (&pp1data.gwdata, 1));	

	end_timer (timers, 1);
	sprintf (buf, "%s stage 2 complete. %" PRIu64 " transforms. Total time: ", gwmodulo_as_string (&pp1data.gwdata), gw_get_fft_count (&pp1data.gwdata));
	print_timer (timers, 1, buf, TIMER_NL | TIMER_CLR);
	OutputStr (thread_num, buf);

	if (ERRCHK) {
		sprintf (buf, "Round off: %.10g\n", gw_get_maxerr (&pp1data.gwdata));
		OutputStr (thread_num, buf);
		gw_clear_maxerr (&pp1data.gwdata);
	}

restart4:
	pp1data.state = PP1_STATE_GCD;
	strcpy (w->stage, "S2");
	w->pct_complete = 1.0;
	start_timer_from_zero (timers, 0);
	stop_reason = gcd (&pp1data.gwdata, thread_num, pp1data.gg, N, &factor);
	if (stop_reason) {
		pp1_save (&pp1data);
		goto exit;
	}
	pp1data.state = PP1_STATE_DONE;
	end_timer (timers, 0);
	strcpy (buf, "Stage 2 GCD complete. Time: ");
	print_timer (timers, 0, buf, TIMER_NL);
	OutputStr (thread_num, buf);
	if (factor != NULL) goto bingo;

msg_and_exit:
	sprintf (buf, "%s completed P+1, B1=%" PRIu64, gwmodulo_as_string (&pp1data.gwdata), pp1data.B);
	if (pp1data.C > pp1data.B) sprintf (buf+strlen(buf), ", B2=%" PRIu64, pp1data.C);
	sprintf (buf+strlen(buf), ", Wi%d: %08lX\n", PORT, SEC5 (w->n, pp1data.B, pp1data.C));
	OutputStr (thread_num, buf);
	formatMsgForResultsFile (buf, w);
	writeResults (buf);

	strcpy (JSONbuf, "{\"status\":\"NF\"");
	JSONaddExponent (JSONbuf, w);
	strcat (JSONbuf, ", \"worktype\":\"P+1\"");
	sprintf (JSONbuf+strlen(JSONbuf), ", \"b1\":%" PRIu64, pp1data.B);
	if (pp1data.C > pp1data.B) sprintf (JSONbuf+strlen(JSONbuf), ", \"b2\":%" PRIu64, pp1data.C);
	sprintf (JSONbuf+strlen(JSONbuf), ", \"start\":\"%" PRIu32 "/%" PRIu32 "\"", pp1data.numerator, pp1data.denominator);
	sprintf (JSONbuf+strlen(JSONbuf), ", \"fft-length\":%lu", pp1data.stage1_fftlen);
	sprintf (JSONbuf+strlen(JSONbuf), ", \"security-code\":\"%08lX\"", SEC5 (w->n, pp1data.B, pp1data.C > pp1data.B ? pp1data.C : pp1data.B));
	JSONaddProgramTimestamp (JSONbuf);
	JSONaddExponentKnownFactors (JSONbuf, w);
	JSONaddUserComputerAID (JSONbuf, w);
	strcat (JSONbuf, "}");
	if (IniGetInt (INI_FILE, "OutputJSON", 1)) writeResultsJSON (JSONbuf);

	if (!QA_IN_PROGRESS && (pp1data.B >= 50000 || IniGetInt (INI_FILE, "SendAllFactorData", 0))) {
		struct primenetAssignmentResult pkt;
		memset (&pkt, 0, sizeof (pkt));
		strcpy (pkt.computer_guid, COMPUTER_GUID);
		strcpy (pkt.assignment_uid, w->assignment_uid);
		strcpy (pkt.message, buf);
		pkt.result_type = PRIMENET_AR_PP1_NOFACTOR;
		pkt.k = w->k;
		pkt.b = w->b;
		pkt.n = w->n;
		pkt.c = w->c;
		pkt.pp1_numerator = pp1data.numerator;
		pkt.pp1_denominator = pp1data.denominator;
		pkt.B1 = pp1data.B;
		pkt.B2 = pp1data.C;
		pkt.fftlen = pp1data.stage1_fftlen;
		pkt.done = TRUE;
		strcpy (pkt.JSONmessage, JSONbuf);
		spoolMessage (PRIMENET_ASSIGNMENT_RESULT, &pkt);
	}

	unlinkSaveFiles (&pp1data.write_save_file_state);
	if (!QA_IN_PROGRESS && IniGetInt (INI_FILE, "KeepPplus1SaveFiles", 0) && w->nth_run <= 2)
		pp1_save (&pp1data);

done:	stop_reason = STOP_WORK_UNIT_COMPLETE;

exit:	Dmultiple_map.clear ();
	relp_set_map.clear ();
	pp1_cleanup (&pp1data);
	free (N);
	free (factor);
	free (str);
	free (msg);
	return (stop_reason);

lowmem:	stop_reason = OutOfMemory (thread_num);
possible_lowmem:
	if (pp1data.state == PP1_STATE_MIDSTAGE) pp1_save (&pp1data);
	if (stop_reason != STOP_OUT_OF_MEM) goto exit;

	pp1_save (&pp1data);
	Dmultiple_map.clear ();
	relp_set_map.clear ();
	pp1_cleanup (&pp1data);
	OutputBoth (thread_num, "Memory allocation error.  Trying again using less memory.\n");
	pp1data.pct_mem_to_use *= 0.8;
	goto restart;

oom:	stop_reason = OutOfMemory (thread_num);
	goto exit;

bingo:	if (pp1data.state < PP1_STATE_MIDSTAGE)
		sprintf (buf, "P+1 found a factor in stage #1, B1=%" PRIu64 ".\n", pp1data.B);
	else
		sprintf (buf, "P+1 found a factor in stage #2, B1=%" PRIu64 ", B2=%" PRIu64 ".\n", pp1data.B, pp1data.C);
	OutputBoth (thread_num, buf);

	msglen = factor->sign * 10 + 400;
	str = (char *) malloc (msglen);
	if (str == NULL) {
		stop_reason = OutOfMemory (thread_num);
		goto exit;
	}
	msg = (char *) malloc (msglen);
	if (msg == NULL) {
		stop_reason = OutOfMemory (thread_num);
		goto exit;
	}
	gtoc (factor, str, msglen);

	if (!testFactor (&pp1data.gwdata, w, factor)) {
		sprintf (msg, "ERROR: Bad factor for %s found: %s\n", gwmodulo_as_string (&pp1data.gwdata), str);
		OutputBoth (thread_num, msg);
		unlinkSaveFiles (&pp1data.write_save_file_state);
		OutputStr (thread_num, "Restarting P+1 from scratch.\n");
		stop_reason = 0;
		goto error_restart;
	}

	if (pp1data.state < PP1_STATE_MIDSTAGE)
		sprintf (msg, "%s has a factor: %s (P+1, B1=%" PRIu64 ")\n",
			 gwmodulo_as_string (&pp1data.gwdata), str, pp1data.B);
	else
		sprintf (msg, "%s has a factor: %s (P+1, B1=%" PRIu64 ", B2=%" PRIu64 ")\n",
			 gwmodulo_as_string (&pp1data.gwdata), str, pp1data.B, pp1data.C);
	OutputStr (thread_num, msg);
	formatMsgForResultsFile (msg, w);
	writeResults (msg);

	strcpy (JSONbuf, "{\"status\":\"F\"");
	JSONaddExponent (JSONbuf, w);
	strcat (JSONbuf, ", \"worktype\":\"P+1\"");
	sprintf (JSONbuf+strlen(JSONbuf), ", \"factors\":[\"%s\"]", str);
	sprintf (JSONbuf+strlen(JSONbuf), ", \"b1\":%" PRIu64, pp1data.B);
	if (pp1data.state > PP1_STATE_MIDSTAGE) sprintf (JSONbuf+strlen(JSONbuf), ", \"b2\":%" PRIu64, pp1data.C);
	sprintf (JSONbuf+strlen(JSONbuf), ", \"start\":\"%" PRIu32 "/%" PRIu32 "\"", pp1data.numerator, pp1data.denominator);
	sprintf (JSONbuf+strlen(JSONbuf), ", \"fft-length\":%lu", pp1data.stage1_fftlen);
	sprintf (JSONbuf+strlen(JSONbuf), ", \"security-code\":\"%08lX\"", SEC5 (w->n, pp1data.B, pp1data.state > PP1_STATE_MIDSTAGE ? pp1data.C : pp1data.B));
	JSONaddProgramTimestamp (JSONbuf);
	JSONaddExponentKnownFactors (JSONbuf, w);
	JSONaddUserComputerAID (JSONbuf, w);
	strcat (JSONbuf, "}");
	if (IniGetInt (INI_FILE, "OutputJSON", 1)) writeResultsJSON (JSONbuf);

	if (!QA_IN_PROGRESS && (strlen (str) >= 21 || IniGetInt (INI_FILE, "SendAllFactorData", 0))) {
		struct primenetAssignmentResult pkt;
		memset (&pkt, 0, sizeof (pkt));
		strcpy (pkt.computer_guid, COMPUTER_GUID);
		strcpy (pkt.assignment_uid, w->assignment_uid);
		strcpy (pkt.message, msg);
		pkt.result_type = PRIMENET_AR_PP1_FACTOR;
		pkt.k = w->k;
		pkt.b = w->b;
		pkt.n = w->n;
		pkt.c = w->c;
		pkt.pp1_numerator = pp1data.numerator;
		pkt.pp1_denominator = pp1data.denominator;
		truncated_strcpy (pkt.factor, sizeof (pkt.factor), str);
		pkt.B1 = pp1data.B;
		pkt.B2 = (pp1data.state < PP1_STATE_MIDSTAGE ? 0 : pp1data.C);
		pkt.fftlen = pp1data.stage1_fftlen;
		pkt.done = TRUE;
		strcpy (pkt.JSONmessage, JSONbuf);
		spoolMessage (PRIMENET_ASSIGNMENT_RESULT, &pkt);
	}

	unlinkSaveFiles (&pp1data.write_save_file_state);
	if (!QA_IN_PROGRESS && IniGetInt (INI_FILE, "KeepPplus1SaveFiles", 0) && w->nth_run <= 2)
		pp1_save (&pp1data);

	invalidateNextRollingAverageUpdate ();

	if (USE_PRIMENET && w->k == 1.0 && w->b == 2 && w->c == -1 && strlen (str) >= 21 && (int) w->n < IniGetInt (INI_FILE, "MaxAutoPRPExponent", 5000000)) {
		struct work_unit w_prp;
		generatePRPWorkUnit (w, str, &w_prp);
		
		w_prp.next = w;
		stop_reason = addWorkToDoLine (thread_num, &w_prp, ADD_AFTER_SPECIFIC);
		if (stop_reason) goto exit;
	}

	stop_reason = STOP_WORK_UNIT_COMPLETE;
	goto exit;

err:	if (gw_get_maxerr (&pp1data.gwdata) > allowable_maxerr) {
		sprintf (buf, "Possible roundoff error (%.8g), backtracking to last save file and using larger FFT.\n", gw_get_maxerr (&pp1data.gwdata));
		OutputStr (thread_num, buf);
		maxerr_restart_count++;
	} else {
		OutputBoth (thread_num, "SUMOUT error occurred.\n");
		stop_reason = SleepFive (thread_num);
		if (stop_reason) goto exit;
	}
error_restart:
	Dmultiple_map.clear ();
	relp_set_map.clear ();
	pp1_cleanup (&pp1data);
	free (factor), factor = NULL;
	free (str), str = NULL;
	free (msg), msg = NULL;
	goto restart;
}

int pplus1_QA (
	int	thread_num,
	struct PriorityInfo *sp_info)	
{
	FILE	*fd;

	title (thread_num, "QA");

	fd = fopen ("qa_pp1", "r");
	if (fd == NULL) {
		OutputStr (thread_num, "File named 'qa_pp1' could not be opened.\n");
		return (STOP_FILE_IO_ERROR);
	}

	QA_TYPE = 0;
	for ( ; ; ) {
		struct work_unit w;
		double	k;
		unsigned long b, n, B1, B2, start;
		signed long c;
		char	fac_str[80];
		int	stop_reason;

		n = 0;
		(void) fscanf (fd, "%lf,%lu,%lu,%ld,%lu,%lu,%lu,%s\n", &k, &b, &n, &c, &B1, &B2, &start, fac_str);
		if (n == 0) break;

		if (n == 1) {
			QA_TYPE = c;
			continue;
		}

		QA_FACTOR = allocgiant ((int) strlen (fac_str));
		ctog (fac_str, QA_FACTOR);

		memset (&w, 0, sizeof (w));
		w.work_type = WORK_PPLUS1;
		w.k = k;
		w.b = b;
		w.n = n;
		w.c = c;
		w.B1 = B1;
		w.B2 = B2;
		w.nth_run = start;
		QA_IN_PROGRESS = TRUE;
		stop_reason = pplus1 (0, sp_info, &w);
		QA_IN_PROGRESS = FALSE;
		free (QA_FACTOR);
		if (stop_reason != STOP_WORK_UNIT_COMPLETE) {
			fclose (fd);
			return (stop_reason);
		}
	}

	fclose (fd);
	return (0);
}
