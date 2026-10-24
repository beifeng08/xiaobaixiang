// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <memory.h>
#include <string.h>
#include <time.h>
#include "cpuid.h"
#include "gwnum.h"
#include "gwtables.h"
#include "gwini.h"
#include "gwutil.h"
#define DBLDBL_INLINES
#include "gwdbldbl.h"
#include "gwbench.h"
#include "radix.h"

#define cyclic_gwnum(h,g)	(g)
#define negacyclic_gwnum(h,g)	((gwnum) ((char *)(g) + round_up_to_multiple_of (gwnum_datasize((h)->cyclic_gwdata) + GW_HEADER_SIZE((h)->negacyclic_gwdata), 128)))

#ifndef GDEBUG_MEM
#define SHARE_SINCOS_DATA
#endif

#include "mt19937ar.c"

gwmutex	gwclone_lock;				
int	gwclone_lock_initialized = FALSE;	

void (*OutputBothRoutine)(int, const char *) = NULL;
#define OutputBoth(t,x)	(*OutputBothRoutine)(t,x)

struct gwinfo1_data {
	const struct gwasm_jmptab *sse2_cyclic_fft_info; 
	const struct gwasm_jmptab *sse2_negacyclic_fft_info; 
	const struct gwasm_jmptab *x86_cyclic_fft_info; 
	const struct gwasm_jmptab *x86_negacyclic_fft_info; 
	const struct gwasm_jmptab *avx_cyclic_fft_info; 
	const struct gwasm_jmptab *avx_negacyclic_fft_info; 
	const struct gwasm_jmptab *avx512_cyclic_fft_info; 
	const struct gwasm_jmptab *avx512_negacyclic_fft_info; 
	uint32_t version;			
};

void gwinfo1 (struct gwinfo1_data *);
void prefetchL2 (void *addr, int count);
void pause_for_count (int count);

#define gw_fft(h,a)	(*(h)->GWPROCPTRS[0])(a)
#define gw_add(h,a)	(*(h)->GWPROCPTRS[1])(a)
#define gw_addq(h,a)	(*(h)->GWPROCPTRS[2])(a)
#define gw_addf(h,a)	(*(h)->GWPROCPTRS[2])(a)
#define gw_sub(h,a)	(*(h)->GWPROCPTRS[3])(a)
#define gw_subq(h,a)	(*(h)->GWPROCPTRS[4])(a)
#define gw_subf(h,a)	(*(h)->GWPROCPTRS[4])(a)
#define gw_addsub(h,a)	(*(h)->GWPROCPTRS[5])(a)
#define gw_addsubq(h,a)	(*(h)->GWPROCPTRS[6])(a)
#define gw_addsubf(h,a)	(*(h)->GWPROCPTRS[6])(a)
#define gw_copyzero(h,a) (*(h)->GWPROCPTRS[7])(a)
#define gw_muls(h,a)	(*(h)->GWPROCPTRS[8])(a)
#define norm_routines	9
#define zerohigh_routines 13

#define addr_gw_fft(h)		((h)->GWPROCPTRS[0])
#define addr_gw_add(h)		((h)->GWPROCPTRS[1])
#define addr_gw_addq(h)		((h)->GWPROCPTRS[2])
#define addr_gw_addf(h)		((h)->GWPROCPTRS[2])
#define addr_gw_sub(h)		((h)->GWPROCPTRS[3])
#define addr_gw_subq(h)		((h)->GWPROCPTRS[4])
#define addr_gw_subf(h)		((h)->GWPROCPTRS[4])
#define addr_gw_addsub(h)	((h)->GWPROCPTRS[5])
#define addr_gw_addsubq(h)	((h)->GWPROCPTRS[6])
#define addr_gw_addsubf(h)	((h)->GWPROCPTRS[6])
#define addr_gw_copyzero(h)	((h)->GWPROCPTRS[7])
#define addr_gw_muls(h)		((h)->GWPROCPTRS[8])
#define call_op(p,a)		(*p)(a)

#define extern_decl(name)		extern void (*name)(void);
#define array_entry(name)		&name,

#define aux_decl(name)			extern_decl(name##1)	extern_decl(name##2)
#define aux_decl3(name)			extern_decl(name##1)	extern_decl(name##2)	extern_decl(name##3)
#define aux_decl3y(name)		aux_decl3(name)	aux_decl3(name##r) aux_decl3(name##zp) aux_decl3(name##rzp) aux_decl3(name##n) aux_decl3(name##nr) aux_decl3(name##nzp) aux_decl3(name##nrzp)
#define aux_decl3z(name)		aux_decl3(name)	aux_decl3(name##r) aux_decl3(name##zp) aux_decl3(name##rzp)

#ifndef X86_64
aux_decl(gwadd)		aux_decl(gwaddq)
aux_decl(gwsub)		aux_decl(gwsubq)
aux_decl(gwaddsub)	aux_decl(gwaddsubq)
aux_decl(gwcopyzero)	aux_decl(gwmuls)

void *x87_aux_prctab[] = {
	&gwadd1, &gwaddq1, &gwsub1, &gwsubq1, &gwaddsub1, &gwaddsubq1, &gwcopyzero1, &gwmuls1,
	&gwadd2, &gwaddq2, &gwsub2, &gwsubq2, &gwaddsub2, &gwaddsubq2, &gwcopyzero2, &gwmuls2};
#endif

aux_decl3(gwxadd)	aux_decl(gwxaddq)
aux_decl3(gwxsub)	aux_decl(gwxsubq)
aux_decl3(gwxaddsub)	aux_decl(gwxaddsubq)
aux_decl(gwxcopyzero)	aux_decl3(gwxadds)	aux_decl3(gwxmuls)

void *sse2_aux_prctab[] = {
	&gwxadd1, &gwxaddq1, &gwxsub1, &gwxsubq1, &gwxaddsub1, &gwxaddsubq1, &gwxcopyzero1, &gwxmuls1,
	&gwxadd2, &gwxaddq2, &gwxsub2, &gwxsubq2, &gwxaddsub2, &gwxaddsubq2, &gwxcopyzero2, &gwxmuls2,
	&gwxadd3, &gwxaddq2, &gwxsub3, &gwxsubq2, &gwxaddsub3, &gwxaddsubq2, &gwxcopyzero2, &gwxmuls3};

aux_decl3y(gwyadd)	aux_decl3(gwyaddq)
aux_decl3y(gwysub)	aux_decl3(gwysubq)
aux_decl3y(gwyaddsub)	aux_decl3(gwyaddsubq)
aux_decl3(gwycopyzero)	aux_decl3y(gwyadds)	aux_decl3y(gwymuls)

void *avx_aux_prctab[] = {
	&gwyaddq1, &gwysubq1, &gwyaddsubq1, &gwycopyzero1,
	&gwyadd1, &gwysub1, &gwyaddsub1, &gwymuls1,
	&gwyaddr1, &gwysubr1, &gwyaddsubr1, &gwymulsr1,
	&gwyaddzp1, &gwysubzp1, &gwyaddsubzp1, &gwymulszp1,
	&gwyaddrzp1, &gwysubrzp1, &gwyaddsubrzp1, &gwymulsrzp1,
	&gwyaddn1, &gwysubn1, &gwyaddsubn1, &gwymulsn1,
	&gwyaddnr1, &gwysubnr1, &gwyaddsubnr1, &gwymulsnr1,
	&gwyaddnzp1, &gwysubnzp1, &gwyaddsubnzp1, &gwymulsnzp1,
	&gwyaddnrzp1, &gwysubnrzp1, &gwyaddsubnrzp1, &gwymulsnrzp1,

	&gwyaddq3, &gwysubq3, &gwyaddsubq3, &gwycopyzero3,
	&gwyadd3, &gwysub3, &gwyaddsub3, &gwymuls3,
	&gwyaddr3, &gwysubr3, &gwyaddsubr3, &gwymulsr3,
	&gwyaddzp3, &gwysubzp3, &gwyaddsubzp3, &gwymulszp3,
	&gwyaddrzp3, &gwysubrzp3, &gwyaddsubrzp3, &gwymulsrzp3,
	&gwyaddn3, &gwysubn3, &gwyaddsubn3, &gwymulsn3,
	&gwyaddnr3, &gwysubnr3, &gwyaddsubnr3, &gwymulsnr3,
	&gwyaddnzp3, &gwysubnzp3, &gwyaddsubnzp3, &gwymulsnzp3,
	&gwyaddnrzp3, &gwysubnrzp3, &gwyaddsubnrzp3, &gwymulsnrzp3};

aux_decl3y(ygw_carries_wpn)
void gwy3_apply_carries (void *);

void *avx_carries_prctab[] = {
	&ygw_carries_wpn3, &ygw_carries_wpnr3, &ygw_carries_wpnzp3, &ygw_carries_wpnrzp3,
	&ygw_carries_wpnn3, &ygw_carries_wpnnr3, &ygw_carries_wpnnzp3, &ygw_carries_wpnnrzp3};

#ifdef X86_64
aux_decl3z(gwzadd)	aux_decl3(gwzaddq)
aux_decl3z(gwzsub)	aux_decl3(gwzsubq)
aux_decl3z(gwzaddsub)	aux_decl3(gwzaddsubq)
aux_decl3(gwzcopyzero)	aux_decl3y(gwzadds)	aux_decl3y(gwzmuls)

void *avx512_aux_prctab[] = {
	&gwzaddq3, &gwzsubq3, &gwzaddsubq3, &gwzcopyzero3,
	&gwzadd3, &gwzsub3, &gwzaddsub3, &gwzmuls3,
	&gwzaddr3, &gwzsubr3, &gwzaddsubr3, &gwzmulsr3,
	&gwzaddzp3, &gwzsubzp3, &gwzaddsubzp3, &gwzmulszp3,
	&gwzaddrzp3, &gwzsubrzp3, &gwzaddsubrzp3, &gwzmulsrzp3};

aux_decl3z(zgw_carries_wpn)
extern_decl(zgw_carries_op_wpnzp3)
extern_decl(zgw_carries_op_wpnrzp3)
void gwz3_apply_carries (void *);

void *avx512_carries_prctab[] = {
	&zgw_carries_wpn3, &zgw_carries_wpnr3, &zgw_carries_wpnzp3, &zgw_carries_wpnrzp3, &zgw_carries_op_wpnzp3, &zgw_carries_op_wpnrzp3};
#else
#define gwz3_apply_carries(a)
#endif

#ifdef X86_64
#define avx512_explode(macro)			avx512_explode1(macro,zr)		avx512_explode1(macro,zi)
#define avx512_explode1(macro,name)		avx512_explode2(macro,name##3,SKX)
#define avx512_explode2(macro,name,suff)	avx512_zero_explode(macro,name##z,suff)	avx512_explode3(macro,name,suff,notzp)	avx512_explode3(macro,name##zp,suff,zp)
#define avx512_zero_explode(macro,name,suff)	avx512_explode9##suff(macro,name)	avx512_explode9##suff(macro,name##e)
#define avx512_explode3(macro,name,suff,zp)	avx512_explode4(macro,name,suff,zp)	avx512_explode4(macro,name##e,suff,zp)
#define avx512_explode4(macro,name,suff,zp)	avx512_explode5(macro,name,suff,zp,notc) avx512_explode5(macro,name##c,suff,zp,c)
#define avx512_explode5(macro,name,suff,zp,c)	avx512_explode6(macro,name,suff,zp,c)
#define avx512_explode6(macro,name,suff,zp,c)	avx512_explode7##zp(macro,name,suff,c)
#define avx512_explode7notzp(macro,name,suff,c)	avx512_explode9##suff(macro,name)
#define avx512_explode7zp(macro,name,suff,c)	avx512_explode8##c(macro,name,suff)	avx512_explode8##c(macro,name##k,suff)
#define avx512_explode8c(macro,name,suff)	avx512_explode9##suff(macro,name)
#define avx512_explode8notc(macro,name,suff)	avx512_explode9##suff(macro,name)
#define avx512_explode9SKX(macro,name)		macro(name##SKX)

avx512_explode(extern_decl)
void *avx512_prctab[] = { avx512_explode(array_entry) NULL };

int avx512_prctab_index (gwhandle *gwdata, int z, int e, int c)
{
	int	index = 0;

	if (! gwdata->RATIONAL_FFT) index += 14;		

	if (z) {
		if (e) index += 1;
		return (index);
	}
	index += 2;
	if (! gwdata->ZERO_PADDED_FFT) {
		if (e) index += 2;
		if (c) index += 1;
	} else {
		struct gwasm_data *asm_data = (struct gwasm_data *) gwdata->asm_data;
		index += 4;
		if (e) index += 4;
		if (!c) {
			if (asm_data->u.zmm.ZMM_K_HI_OVER_SMALL_BASE == 0.0) index += 1;
		} else {
			index += 2;
			if (asm_data->u.zmm.ZMM_K_TIMES_MULCONST_HI_OVER_SMALL_BASE == 0.0) index += 1;
		}
	}
	return (index);
}
#endif

#define avx_explode(macro)			avx_explode1(macro,yr)			avx_explode1(macro,yi)
#define avx_explode1(macro,name)		avx_explode2(macro,name##1,CORE)	avx_explode2(macro,name##1,FMA3)	avx_explode2(macro,name##3,CORE)	avx_explode2(macro,name##3,FMA3)
#define avx_explode2(macro,name,suff)		avx_zero_explode(macro,name##z,suff)	avx_explode3(macro,name,suff,notzp)	avx_explode3(macro,name##zp,suff,zp)
#define avx_zero_explode(macro,name,suff)	avx_explode9##suff(macro,name)		avx_explode9##suff(macro,name##e)
#define avx_explode3(macro,name,suff,zp)	avx_explode4(macro,name,suff,zp)	avx_explode4(macro,name##e,suff,zp)
#define avx_explode4(macro,name,suff,zp)	avx_explode5(macro,name,suff,zp,notc)	avx_explode5(macro,name##c,suff,zp,c)
#define avx_explode5(macro,name,suff,zp,c)	avx_explode6(macro,name,suff,zp,c)	avx_explode6(macro,name##b,suff,zp,c)
#define avx_explode6(macro,name,suff,zp,c)	avx_explode7##zp(macro,name,suff,c)
#define avx_explode7notzp(macro,name,suff,c)	avx_explode9##suff(macro,name)
#define avx_explode7zp(macro,name,suff,c)	avx_explode8##c(macro,name,suff)	avx_explode8##c(macro,name##k,suff)
#define avx_explode8c(macro,name,suff)		avx_explode9##suff(macro,name)
#define avx_explode8notc(macro,name,suff)	avx_explode9##suff(macro,name)		avx_explode9##suff(macro,name##c1)	avx_explode9##suff(macro,name##cm1)
#define avx_explode9BLEND(macro,name)		macro(name##BLEND)
#define avx_explode9CORE(macro,name)		macro(name##CORE)
#ifdef X86_64
#define avx_explode9FMA3(macro,name)		macro(name##FMA3)
#else
#define avx_explode9FMA3(macro,name)		macro(name##CORE)			
#endif

avx_explode(extern_decl)
void *avx_prctab[] = { avx_explode(array_entry) NULL };

int avx_prctab_index (gwhandle *gwdata, int z, int e, int c)
{
	int	index = 0;

	if (! gwdata->RATIONAL_FFT) index += 168;		
	if (gwdata->PASS2_SIZE) index += 84;			
	if (gwdata->cpu_flags & CPU_FMA3) index += 42;		

	if (z) {
		if (e) index += 1;
		return (index);
	}
	index += 2;
	if (! gwdata->ZERO_PADDED_FFT) {
		if (e) index += 4;
		if (c) index += 2;
		if (gwdata->b > 2) index += 1;
	} else {
		struct gwasm_data *asm_data = (struct gwasm_data *) gwdata->asm_data;
		index += 8;
		if (e) index += 16;
		if (!c) {
			if (gwdata->b > 2) index += 6;
			if (asm_data->u.ymm.YMM_K_HI[0] == 0.0) index += 3;
			if (gwdata->c == 1) index += 1;  if (gwdata->c == -1) index += 2;
		} else {
			index += 12;
			if (gwdata->b > 2) index += 2;
			if (asm_data->u.ymm.YMM_K_TIMES_MULCONST_HI[0] == 0.0) index += 1;
		}
	}
	return (index);
}				    

#define sse2_explode(macro)			sse2_explode1(macro,xr)			sse2_explode1(macro,xi)
#define sse2_explode1(macro,name)		sse2_explode2(macro,name##1,BLEND)	sse2_explode2(macro,name##2,CORE)	sse2_explode2(macro,name##2,K8)		sse2_explode2(macro,name##3,CORE)	sse2_explode2(macro,name##3,K8)
#define sse2_explode2(macro,name,suff)		sse2_zero_explode(macro,name##z,suff)	sse2_explode3(macro,name,suff,notzp)	sse2_explode3(macro,name##zp,suff,zp)
#define sse2_zero_explode(macro,name,suff)	sse2_explode9##suff(macro,name)		sse2_explode9##suff(macro,name##e)
#define sse2_explode3(macro,name,suff,zp)	sse2_explode4(macro,name,suff,zp)	sse2_explode4(macro,name##e,suff,zp)
#define sse2_explode4(macro,name,suff,zp)	sse2_explode5(macro,name,suff,zp,notc)	sse2_explode5(macro,name##c,suff,zp,c)
#define sse2_explode5(macro,name,suff,zp,c)	sse2_explode6(macro,name,suff,zp,c)	sse2_explode6(macro,name##b,suff,zp,c)
#define sse2_explode6(macro,name,suff,zp,c)	sse2_explode7##zp(macro,name,suff,c)	sse2_explode7##zp(macro,name##s4,suff,c)
#define sse2_explode7notzp(macro,name,suff,c)	sse2_explode9##suff(macro,name)
#define sse2_explode7zp(macro,name,suff,c)	sse2_explode8##c(macro,name,suff)	sse2_explode8##c(macro,name##k,suff)
#define sse2_explode8c(macro,name,suff)		sse2_explode9##suff(macro,name)
#define sse2_explode8notc(macro,name,suff)	sse2_explode9##suff(macro,name)		sse2_explode9##suff(macro,name##c1)	sse2_explode9##suff(macro,name##cm1)
#define sse2_explode9BLEND(macro,name)		macro(name##BLEND)
#define sse2_explode9CORE(macro,name)		macro(name##CORE)
#ifndef __APPLE__
#define sse2_explode9K8(macro,name)		macro(name##K8)
#else
#define sse2_explode9K8(macro,name)		macro(name##CORE)		
#endif

sse2_explode(extern_decl)
void *sse2_prctab[] = { sse2_explode(array_entry) NULL };

int sse2_prctab_index (gwhandle *gwdata, int z, int e, int c)
{
	int	index = 0;

	if (! gwdata->RATIONAL_FFT) index += 410;
	if (gwdata->PASS2_SIZE) {
		index += 82;
		if (gwdata->FFT_TYPE == FFT_TYPE_RADIX_4_DWPN) index += 164;
		if (gwdata->cpu_flags & CPU_3DNOW_PREFETCH) index += 82;
	}
	if (z) {
		if (e) index += 1;
		return (index);
	}
	index += 2;
	if (! gwdata->ZERO_PADDED_FFT) {
		if (e) index += 8;
		if (c) index += 4;
		if (gwdata->b > 2) index += 2;
		if (gwdata->cpu_flags & CPU_SSE41) index += 1;
	} else {
		struct gwasm_data *asm_data = (struct gwasm_data *) gwdata->asm_data;
		index += 16;
		if (e) index += 32;
		if (!c) {
			if (gwdata->b > 2) index += 12;
			if (gwdata->cpu_flags & CPU_SSE41) index += 6;
			if (asm_data->u.xmm.XMM_K_HI[0] == 0.0) index += 3;
			if (gwdata->c == 1) index += 1;  if (gwdata->c == -1) index += 2;
		} else {
			index += 24;
			if (gwdata->b > 2) index += 4;
			if (gwdata->cpu_flags & CPU_SSE41) index += 2;
			if (asm_data->u.xmm.XMM_K_TIMES_MULCONST_HI[0] == 0.0) index += 1;
		}
	}
	return (index);
}				    

#ifndef X86_64
#define x87_explode(macro)		x87_explode1(macro,r)		x87_explode1(macro,i)
#define x87_explode1(macro,name)	x87_explode2(macro,name##1)	x87_explode2(macro,name##2)
#define x87_explode2(macro,name)	x87_zero_explode(macro,name##z)	x87_explode3(macro,name)	x87_explode3(macro,name##zp)
#define x87_zero_explode(macro,name)	macro(name)			macro(name##e)
#define x87_explode3(macro,name)	x87_explode4(macro,name)	x87_explode4(macro,name##e)
#define x87_explode4(macro,name)	macro(name)			macro(name##c)

x87_explode(extern_decl)
void *x87_prctab[] = { x87_explode(array_entry) NULL };

#define	x87_prctab_index(gwdata, z, e, c)  \
	    ((gwdata)->RATIONAL_FFT ? 0 : 20) + \
	    ((gwdata)->PASS2_SIZE ? 10 : 0) + \
	    (z ? (e ? 1 : 0) : 2 + \
	     ((gwdata)->ZERO_PADDED_FFT ? 4 : 0) + \
	     (e ? 2 : 0) + \
	     (c ? 1 : 0))
#endif

inline int is_valid_double (double x) {
	union uint64_double { uint64_t x64; double xdbl; } xcast;
	xcast.xdbl = x;
	return ((xcast.x64 & 0x7ff0000000000000ULL) != 0x7ff0000000000000ULL);
}
#define is_valid_double_addr(x)	(((* (uint64_t *) x) & 0x7ff0000000000000ULL) != 0x7ff0000000000000ULL)

#define	GWINIT_WAS_CALLED_VALUE	0xAA12BB34	
#define GWFREEABLE		0x80000000	
#define GWFREE_INTERNAL		0x40000000	
#define GWFREE_LARGE_PAGES	0x20000000	
#define GWFREEALLOC_INDEX	0x1FFFFFFF	

int convert_giant_to_k2ncd (
	giant	g,		
	double	*k,		
	unsigned long *n,	
	signed long *c,		
	unsigned long *d);	
int internal_gwsetup (
	gwhandle *gwdata,	
	double	k,		
	unsigned long b,	
	unsigned long n,	
	signed long c);		
void raw_gwsetaddin (gwhandle *gwdata, unsigned long word, double *ptr, double val);
int multithread_init (gwhandle *gwdata);
void multithread_term (gwhandle *gwdata);
void do_multithread_op_work (gwhandle *gwdata, struct gwasm_data *asm_data);
void pass1_aux_entry_point (void*);
void pass2_aux_entry_point (void*);
void gw_fixed_random_number (gwhandle *gwdata, gwnum x);
gwnum gwalloc_internal (gwhandle *gwdata);

unsigned long dwpn_col (
	gwhandle *gwdata,	
	unsigned long word)
{
	unsigned long high, low;

	ASSERTG (! (gwdata->cpu_flags & CPU_AVX512F));	

	if (gwdata->cpu_flags & CPU_AVX) {
		low = word % gwdata->PASS2_SIZE;
		high = (((word / gwdata->PASS2_SIZE) & ~3) % gwdata->wpn_count) * gwdata->PASS2_SIZE;
	} else {
		unsigned long upper_sse2_word = gwdata->PASS2_SIZE >> 1;
		high = word / upper_sse2_word;
		low = word - high * upper_sse2_word;

		high = high >> 1;
		high = high % gwdata->wpn_count;
		high = high * gwdata->PASS2_SIZE;
	}
	return (high + low);
}

int match_pathological_pattern (
	unsigned long num_big_words,
	unsigned long total_length,
	double	pathological_fraction)
{
	double	half_length;
	unsigned long pathological_base, actual_base;

	half_length = (double) total_length * 0.5;
	pathological_base = (unsigned long) ceil (half_length * pathological_fraction);

	actual_base = (unsigned long) ceil (half_length * (double) num_big_words / (double) total_length);

	return (actual_base >= pathological_base && actual_base <= pathological_base + 1);
}

int is_pathological_distribution (
	unsigned long num_big_words,
	unsigned long num_small_words)
{
	unsigned long total_length;

	if (num_big_words == 0 || num_small_words == 0) return (FALSE);

	total_length = num_big_words + num_small_words;
	while ((num_big_words & 1) == 0 && (total_length & 1) == 0) {
		num_big_words >>= 1;
		total_length >>= 1;
	}

	if (match_pathological_pattern (num_big_words, total_length, 1.0 / 2.0)) return (TRUE);
	if (match_pathological_pattern (num_big_words, total_length, 1.0 / 4.0)) return (TRUE);
	if (match_pathological_pattern (num_big_words, total_length, 3.0 / 4.0)) return (TRUE);
	if (match_pathological_pattern (num_big_words, total_length, 1.0 / 8.0)) return (TRUE);
	if (match_pathological_pattern (num_big_words, total_length, 3.0 / 8.0)) return (TRUE);
	if (match_pathological_pattern (num_big_words, total_length, 5.0 / 8.0)) return (TRUE);
	if (match_pathological_pattern (num_big_words, total_length, 7.0 / 8.0)) return (TRUE);
	if (total_length % 3 == 0) {
		if (match_pathological_pattern (num_big_words, total_length, 1.0 / 3.0)) return (TRUE);
		if (match_pathological_pattern (num_big_words, total_length, 2.0 / 3.0)) return (TRUE);
		if (match_pathological_pattern (num_big_words, total_length, 1.0 / 6.0)) return (TRUE);
		if (match_pathological_pattern (num_big_words, total_length, 5.0 / 6.0)) return (TRUE);
	}
	if (total_length % 5 == 0) {
		if (match_pathological_pattern (num_big_words, total_length, 1.0 / 5.0)) return (TRUE);
		if (match_pathological_pattern (num_big_words, total_length, 2.0 / 5.0)) return (TRUE);
		if (match_pathological_pattern (num_big_words, total_length, 3.0 / 5.0)) return (TRUE);
		if (match_pathological_pattern (num_big_words, total_length, 4.0 / 5.0)) return (TRUE);
	}
	if (total_length % 7 == 0) {
		if (match_pathological_pattern (num_big_words, total_length, 1.0 / 7.0)) return (TRUE);
		if (match_pathological_pattern (num_big_words, total_length, 2.0 / 7.0)) return (TRUE);
		if (match_pathological_pattern (num_big_words, total_length, 3.0 / 7.0)) return (TRUE);
		if (match_pathological_pattern (num_big_words, total_length, 4.0 / 7.0)) return (TRUE);
		if (match_pathological_pattern (num_big_words, total_length, 5.0 / 7.0)) return (TRUE);
		if (match_pathological_pattern (num_big_words, total_length, 6.0 / 7.0)) return (TRUE);
	}

	return (FALSE);
}

#define BIF_CORE2		0	
#define BIF_CORE2_512		1	
#define BIF_I7			2	
#define BIF_FMA3		3	
#define BIF_P4_1024		4	
#define BIF_P4TP_512		5	
#define BIF_P4TP_256		6	
#define BIF_SKX			7	
#define BIF_K8			8	
#define BIF_K10			9	
#define BIF_RYZEN		11	

#define ARCH_P4TP		1	
#define ARCH_P4			2
#define ARCH_CORE		3
#define ARCH_FMA3		4
#define ARCH_K8			5
#define ARCH_K10		6
#define ARCH_SKX		8
#define ARCH_BLEND		0

int calculate_bif (
	gwhandle *gwdata,	
	unsigned long fftlen,
	int	negacyclic)
{
	int	cpu_arch, retval;

	cpu_arch = CPU_ARCHITECTURE;
	if (cpu_arch == CPU_ARCHITECTURE_AMD_K8 && ! (gwdata->cpu_flags & CPU_3DNOW_PREFETCH))
		cpu_arch = CPU_ARCHITECTURE_CORE_2;
	if (cpu_arch == CPU_ARCHITECTURE_AMD_K10 && ! (gwdata->cpu_flags & CPU_3DNOW_PREFETCH))
		cpu_arch = CPU_ARCHITECTURE_CORE_2;
	if (cpu_arch == CPU_ARCHITECTURE_AMD_BULLDOZER && ! (gwdata->cpu_flags & CPU_3DNOW_PREFETCH))
		cpu_arch = CPU_ARCHITECTURE_CORE_2;

	switch (cpu_arch) {
	case CPU_ARCHITECTURE_PENTIUM_M:	
	case CPU_ARCHITECTURE_CORE:
	case CPU_ARCHITECTURE_ATOM:
		retval = BIF_P4_1024;		
		break;
	case CPU_ARCHITECTURE_PENTIUM_4:
		if (CPU_L2_CACHE_SIZE <= 128)	
			retval = BIF_P4TP_256;	
		else if (CPU_L2_CACHE_SIZE <= 256)
			retval = BIF_P4TP_256;	
		else if (CPU_L2_CACHE_SIZE <= 512)
			retval = BIF_P4TP_512;	
		else if (gwdata->cpu_flags & CPU_TLB_PRIMING)
			retval = BIF_P4TP_512;	
		else
			retval = BIF_P4_1024;	
		break;
	case CPU_ARCHITECTURE_CORE_2:
		if (CPU_L2_CACHE_SIZE <= 1024)
			retval = BIF_CORE2_512;	
		else
			retval = BIF_CORE2;	
		break;
	case CPU_ARCHITECTURE_CORE_I7:
		if (gwdata->cpu_flags & CPU_AVX512F)
			retval = BIF_SKX;	
		else if (gwdata->cpu_flags & CPU_FMA3)
			retval = BIF_FMA3;	
		else
			retval = BIF_I7;	
		break;
	case CPU_ARCHITECTURE_PHI:		
		if (gwdata->cpu_flags & CPU_AVX512F)
			retval = BIF_SKX;	
		else if (gwdata->cpu_flags & CPU_FMA3)
			retval = BIF_FMA3;	
		else
			retval = BIF_I7;	
		break;
	case CPU_ARCHITECTURE_INTEL_OTHER:	
		if (gwdata->cpu_flags & CPU_AVX512F)
			retval = BIF_SKX;	
		else if (gwdata->cpu_flags & CPU_FMA3)
			retval = BIF_FMA3;	
		else
			retval = BIF_I7;	
		break;
	case CPU_ARCHITECTURE_AMD_K8:
		retval = BIF_K8;		
		break;
	case CPU_ARCHITECTURE_AMD_K10:
		retval = BIF_K10;		
		break;
	case CPU_ARCHITECTURE_AMD_BULLDOZER:	
		if (gwdata->cpu_flags & CPU_FMA3)  
			retval = BIF_FMA3;	
		else if (gwdata->cpu_flags & CPU_AVX)  
			retval = BIF_I7;	
		else
			retval = BIF_K10;	
		break;
	case CPU_ARCHITECTURE_AMD_ZEN:		
	case CPU_ARCHITECTURE_AMD_OTHER:	
		if (gwdata->cpu_flags & CPU_AVX512F)
			retval = BIF_RYZEN;	
		else if (gwdata->cpu_flags & CPU_FMA3)
			retval = BIF_RYZEN;
		else
			retval = BIF_I7;	
		break;
	case CPU_ARCHITECTURE_OTHER:		
		if (gwdata->cpu_flags & CPU_AVX512F)
			retval = BIF_SKX;	
		else if (gwdata->cpu_flags & CPU_FMA3)
			retval = BIF_FMA3;	
		else if (gwdata->cpu_flags & CPU_AVX)
			retval = BIF_I7;	
		else
			retval = BIF_CORE2;	
		break;
	case CPU_ARCHITECTURE_PRE_SSE2:		
	default:				
		retval = BIF_CORE2;
		break;
	}

#ifndef X86_64
	if (retval == BIF_FMA3 || retval == BIF_RYZEN) retval = BIF_I7;
#endif

	if (retval == BIF_P4TP_256)
		retval = BIF_P4TP_512;	
	if (fftlen > 4194304 && retval == BIF_P4TP_512)
		retval = BIF_P4_1024;	
	if (fftlen > 6291456 && retval == BIF_P4_1024)
		retval = BIF_CORE2;	
	if (fftlen > 6291456 && retval == BIF_K8)
		retval = BIF_K10;	
	if (fftlen > 4194304 && retval == BIF_CORE2_512)
		retval = BIF_CORE2;	
	if (fftlen > 7864320 && (negacyclic || fftlen < 18874368 || fftlen > 19660800) && (retval == BIF_CORE2 || retval == BIF_K10))
		retval = BIF_I7;	

	return (retval);
}

static __inline const struct gwasm_jmptab *INC_JMPTAB (const struct gwasm_jmptab *x)
{
	if (x->flags & 0x40000000) return ((const struct gwasm_jmptab *) ((const char *)(x) + sizeof(uint32_t) + 2*sizeof(void*) + sizeof(uint32_t)));
	return ((const struct gwasm_jmptab *) ((const char *)(x) + sizeof(uint32_t) + sizeof(void*) + sizeof(uint32_t)));
}
static __inline const struct gwasm_jmptab *LAST_JMPTAB (const struct gwasm_jmptab *x)
{
	const struct gwasm_jmptab *next;
	for ( ; (next = INC_JMPTAB (x))->flags & 0x80000000; x = next);
	return (x);
}
static __inline const struct gwasm_jmptab *NEXT_SET_OF_JMPTABS (const struct gwasm_jmptab *x)
{
	
	if (x->flags == 0) return ((const struct gwasm_jmptab *) &x->proc_ptr);
	
	x = LAST_JMPTAB (x);
	
	if (x->flags & 0x40000000) x = (const struct gwasm_jmptab *) ((const char *)(x) + sizeof(void*));
	
	while (x->counts[0]) x = (const struct gwasm_jmptab *) ((const char *)(x) + sizeof(int32_t));
	
	return ((const struct gwasm_jmptab *) &x->counts[1]);
}

int is_fft_implemented (
	gwhandle *gwdata,		
	int	negacyclic,		
	const struct gwasm_jmptab *jmptab, 
	int	*best_impl_id)		
{
	int	desired_bif;		
					
	if (! (gwdata->cpu_flags & CPU_3DNOW_PREFETCH)) {
		for ( ; jmptab->flags & 0x80000000; jmptab = INC_JMPTAB (jmptab)) {
			int	arch = (jmptab->flags >> 17) & 0xF;
			if (arch != ARCH_K8 && arch != ARCH_K10) break;
		}
	}
	if (!(jmptab->flags & 0x80000000)) return (FALSE);

	desired_bif = calculate_bif (gwdata, jmptab->fftlen, negacyclic);

	*best_impl_id = -1;

	if (! (gwdata->cpu_flags & (CPU_AVX512F | CPU_AVX))) {
		if (gwdata->cpu_flags & CPU_SSE2) {
			if (jmptab->fftlen < 7168) return (TRUE);
		} else
			return (TRUE);
	}

	if (gwdata->bench_pick_nth_fft || gwdata->qa_pick_nth_fft) return (TRUE);

	if (gwdata->use_benchmarks) {
		int	arch, num_cores, num_workers, num_hyperthreads, i;

		switch (desired_bif) {			
		case BIF_RYZEN:
			arch = (gwdata->cpu_flags & CPU_AVX512F) ? ARCH_SKX : ARCH_FMA3;
			break;
		case BIF_SKX:
			arch = ARCH_SKX;
			break;
		case BIF_FMA3:
			arch = ARCH_FMA3;
			break;
		case BIF_I7:
		case BIF_CORE2:
		case BIF_CORE2_512:
			arch = ARCH_CORE;
			break;
		case BIF_K10:
			arch = ARCH_K10;
			break;
		case BIF_K8:
			arch = ARCH_K8;
			break;
		default:
			arch = ARCH_P4;
		}
		if (gwdata->will_hyperthread) {
			num_hyperthreads = gwdata->will_hyperthread;		
			if (num_hyperthreads <= 1) num_hyperthreads = 2;	
		} else
			num_hyperthreads = 1;
		num_cores = gwdata->bench_num_cores;				
		if (num_cores == 0) num_cores = CPU_CORES;			
		num_workers = gwdata->bench_num_workers;			
		if (num_workers == 0) num_workers = num_cores * num_hyperthreads / gwdata->num_threads; 

		for (i = 0; i <= 1; i++) {
			const struct gwasm_jmptab *next_jmptab;
			int	error_check, impl, next_impl;
			double	throughput, next_throughput;

			if (gwdata->will_error_check == 0) error_check = i;	
			if (gwdata->will_error_check == 1) error_check = !i;	
			if (gwdata->will_error_check == 2) error_check = i;	
			gwbench_get_max_throughput (jmptab->fftlen, arch, num_cores, num_workers, num_hyperthreads,
						    negacyclic, error_check, &impl, &throughput);
			if (throughput <= 0.0) continue;

			for (next_jmptab = NEXT_SET_OF_JMPTABS(jmptab); ; next_jmptab = NEXT_SET_OF_JMPTABS(next_jmptab)) {
				if (next_jmptab->fftlen == 0 ||				
				    next_jmptab->fftlen > 1.03 * jmptab->fftlen) {	
					*best_impl_id = impl;
					return (TRUE);
				}
				gwbench_get_max_throughput (next_jmptab->fftlen, arch, num_cores, num_workers, num_hyperthreads,
							    negacyclic, error_check, &next_impl, &next_throughput);
				if (next_throughput <= 0.0) continue;			
				if (next_throughput > throughput) return (FALSE);	
			}
		}
	}

	while (jmptab->flags & 0x80000000) {
		if (((jmptab->flags >> 13) & 0xF) == desired_bif) return (TRUE);
		if (gwdata->required_pass2_size && ((jmptab->flags >> 13) & 0xF) == 0) return (TRUE);
		jmptab = INC_JMPTAB (jmptab);
	}

	return (FALSE);
}

const struct gwasm_jmptab *choose_one_pass_or_two_pass_impl (
	gwhandle *gwdata,	
	const struct gwasm_jmptab *jmptab,
	int	negacyclic)
{
	const struct gwasm_jmptab *orig_jmptab;	
	int	desired_bif;		
					
	if (! (gwdata->cpu_flags & (CPU_AVX512F | CPU_AVX))) {
		if (gwdata->cpu_flags & CPU_SSE2) {
			if (jmptab->fftlen < 7168) return (jmptab);
		} else
			return (jmptab);
	}

	if (gwdata->bench_pick_nth_fft) return (jmptab);

	if ((jmptab->flags & 0x1FF) != 0) return (jmptab);

	desired_bif = calculate_bif (gwdata, jmptab->fftlen, negacyclic);

	orig_jmptab = jmptab;
	while (jmptab->flags & 0x80000000) {
		if ((jmptab->flags & 0x1FF) != 0) return (jmptab);
		if (((jmptab->flags >> 13) & 0xF) == desired_bif) return (orig_jmptab);
		jmptab = INC_JMPTAB (jmptab);
	}

	return (orig_jmptab);
}

unsigned long adjusted_max_exponent (
	const gwhandle *gwdata,			
	const struct gwasm_jmptab *jmptab)	
{
	unsigned long max_exp;

	max_exp = jmptab->max_exp;
	if (max_exp == 0) return (0);
	if (!(gwdata->cpu_flags & CPU_AVX512F) && gwdata->cpu_flags & CPU_FMA3) {
		if (jmptab->fftlen >= 1048576) max_exp += (int) (0.035635 * (double) jmptab->fftlen);
		else if (jmptab->fftlen >= 32768) max_exp += (int) (0.017656 * (double) jmptab->fftlen);
	}
	return (max_exp);
}

int gwinfo (			
	gwhandle *gwdata,	
	double	k,		
	unsigned long b,	
	unsigned long n,	
	signed long c)		
{
	struct gwinfo1_data asm_info;
	const struct gwasm_jmptab *jmptab, *zpad_jmptab, *generic_jmptab, *impl_jmptab;
	int	best_impl_id, zpad_best_impl_id;
	double	log2k, logbk, log2b, log2c, log2maxmulbyconst;
	double	max_bits_per_input_word, max_bits_per_output_word;
	double	max_weighted_bits_per_output_word;
	int	num_b_in_big_word, num_small_words, num_big_words;
	double	b_per_input_word, bits_per_output_word;
	double	weighted_bits_per_output_word;
	unsigned long max_exp;
	char	buf[20];
	int	larger_fftlen_count, qa_nth_fft, desired_bif;
	void	*prev_proc_ptrs[5];
	uint32_t flags;
	float	safety_margin;
	struct gwasm_data *asm_data;

	gwinfo1 (&asm_info);

	sprintf (buf, "%d.%d", asm_info.version / 100, asm_info.version % 100);
	if (strcmp (buf, GWNUM_VERSION)) return (GWERROR_VERSION);

	log2k = log2 (k);
	log2b = log2 (b);
	logbk = log2k / log2b;
	log2c = log2 (labs (c));
	log2maxmulbyconst = log2 (gwdata->maxmulbyconst);
	safety_margin = gwdata->safety_margin + gwdata->polymult_safety_margin;

	if (n && log2b * (double) n < 5.0 * 1024.0)
		gwdata->cpu_flags &= ~CPU_AVX512F;

again:	zpad_jmptab = NULL;
	generic_jmptab = NULL;
	if (! gwdata->force_general_mod && (k > 1.0 || (n > 0 && n < 500) || labs (c) > 1) && gwdata->qa_pick_nth_fft < 1000) {
		larger_fftlen_count = gwdata->larger_fftlen_count;

		if (gwdata->cpu_flags & CPU_AVX512F)
			zpad_jmptab = asm_info.avx512_cyclic_fft_info;
		else if (gwdata->cpu_flags & CPU_AVX)
			zpad_jmptab = asm_info.avx_cyclic_fft_info;
		else if (gwdata->cpu_flags & CPU_SSE2)
			zpad_jmptab = asm_info.sse2_cyclic_fft_info;
		else
			zpad_jmptab = asm_info.x86_cyclic_fft_info;

		if (zpad_jmptab == NULL) return (GWERROR_TOO_LARGE);

		while ((max_exp = adjusted_max_exponent (gwdata, zpad_jmptab)) != 0) {

			if ((double) (n + n) * log2b / (double) zpad_jmptab->fftlen > 27.0 - 0.25 * log2 (zpad_jmptab->fftlen)) goto next1;
			if (zpad_jmptab->fftlen < gwdata->minimum_fftlen) goto next1;

			if (generic_jmptab != NULL && zpad_jmptab->timing > 3.0 * generic_jmptab->timing) goto next1;

			if (! is_fft_implemented (gwdata, FALSE, zpad_jmptab, &zpad_best_impl_id)) goto next1;

			if (generic_jmptab == NULL && 2.0 * (log2k + n * log2b) + 128.0 < max_exp + 0.15 * zpad_jmptab->fftlen)
				generic_jmptab = zpad_jmptab;

			max_bits_per_input_word = (double) max_exp / zpad_jmptab->fftlen;
			max_bits_per_input_word -= safety_margin;

			num_b_in_big_word = (int) ceil (max_bits_per_input_word);
			num_small_words = (int) ((num_b_in_big_word - max_bits_per_input_word) * zpad_jmptab->fftlen);
			num_big_words = zpad_jmptab->fftlen - num_small_words;
			max_bits_per_output_word =
				2 * (num_b_in_big_word - 1) +
				0.6 * log2 (num_big_words + num_small_words / 3.174802103936252);

			b_per_input_word = (double) (n + n) / zpad_jmptab->fftlen;
			if (logbk > 4.0 * b_per_input_word) goto next1;
			num_b_in_big_word = (int) ceil (b_per_input_word);
			num_small_words = (int) ((num_b_in_big_word - b_per_input_word) * (zpad_jmptab->fftlen / 2 + 4));
			num_big_words = (zpad_jmptab->fftlen / 2 + 4) - num_small_words;
			bits_per_output_word =
				2.0 * (num_b_in_big_word * log2b - 1.0) +
				0.6 * log2 (num_big_words + num_small_words / pow (2.0, log2b / 0.6));

			max_weighted_bits_per_output_word = 2.0 * max_bits_per_input_word + 0.6 * log2 (zpad_jmptab->fftlen / 2 + 4);
			weighted_bits_per_output_word = 2.0 * ((b_per_input_word + 1.0) * log2b - 1.0) + 0.6 * log2 (zpad_jmptab->fftlen / 2 + 4);
			if ((n + n) % zpad_jmptab->fftlen == 0)
				weighted_bits_per_output_word -= ((log2b <= 4.0) ? log2b : 1.4 * log2b);
			else if (! is_pathological_distribution (num_big_words, num_small_words))
				weighted_bits_per_output_word -=
					((log2b <= 3.0) ? (log2b - 1.0) / 2.0 :
					 (log2b <= 6.0) ? 1.0 + (log2b - 3.0) / 3.0 :
					 (log2b <= 12.5) ? 2.0 + (log2b - 6.0) / 6.5 : 3.0);

			if ((weighted_bits_per_output_word <= max_weighted_bits_per_output_word) &&

			    bits_per_output_word + log2k + log2maxmulbyconst <= floor (7.0 * b_per_input_word) * log2b &&

			    bits_per_output_word - floor (b_per_input_word) * log2b + log2c + log2maxmulbyconst <= 51.0) {

				if (larger_fftlen_count--) goto next1;

				break;
			}

next1:			zpad_jmptab = NEXT_SET_OF_JMPTABS (zpad_jmptab);
		}

		if (zpad_jmptab->max_exp == 0 && gwdata->cpu_flags & CPU_AVX512F && b == 2 && n < 100000) {
			gwdata->cpu_flags &= ~CPU_AVX512F;
			goto again;
		}
	}

	if (c < 0) {
		if (gwdata->cpu_flags & CPU_AVX512F)
			jmptab = asm_info.avx512_cyclic_fft_info;
		else if (gwdata->cpu_flags & CPU_AVX)
			jmptab = asm_info.avx_cyclic_fft_info;
		else if (gwdata->cpu_flags & CPU_SSE2)
			jmptab = asm_info.sse2_cyclic_fft_info;
		else
			jmptab = asm_info.x86_cyclic_fft_info;
	} else {
		if (gwdata->cpu_flags & CPU_AVX512F)
			jmptab = asm_info.avx512_negacyclic_fft_info;
		else if (gwdata->cpu_flags & CPU_AVX)
			jmptab = asm_info.avx_negacyclic_fft_info;
		else if (gwdata->cpu_flags & CPU_SSE2)
			jmptab = asm_info.sse2_negacyclic_fft_info;
		else
			jmptab = asm_info.x86_negacyclic_fft_info;
	}

	if (jmptab == NULL) return (GWERROR_TOO_LARGE);

	larger_fftlen_count = gwdata->larger_fftlen_count;
	while ((max_exp = adjusted_max_exponent (gwdata, jmptab)) != 0) {

		if ((double) n * log2b / (double) jmptab->fftlen > 26.0 - 0.25 * log2 (jmptab->fftlen)) goto next2;
		if (jmptab->fftlen < gwdata->minimum_fftlen) goto next2;

		if (log2k >= 34.0) goto next2;

		if (! is_fft_implemented (gwdata, c > 0, jmptab, &best_impl_id)) goto next2;

		if (gwdata->minimum_fftlen && n == 0) break;

		impl_jmptab = choose_one_pass_or_two_pass_impl (gwdata, jmptab, c > 0);

		max_bits_per_input_word = (double) max_exp / jmptab->fftlen;
		max_bits_per_input_word -= safety_margin;

		num_b_in_big_word = (int) ceil (max_bits_per_input_word);
		num_small_words = (int) ((num_b_in_big_word - max_bits_per_input_word) * jmptab->fftlen);
		num_big_words = jmptab->fftlen - num_small_words;
		max_bits_per_output_word =
				2 * (num_b_in_big_word - 1) +
				0.6 * log2 (num_big_words + num_small_words / 3.174802103936252);

		b_per_input_word = (logbk + n) / jmptab->fftlen;
		num_b_in_big_word = (int) ceil (b_per_input_word);
		num_small_words = (int) ((num_b_in_big_word - b_per_input_word) * jmptab->fftlen);
		num_big_words = jmptab->fftlen - num_small_words;
		if (gwdata->radix_bigwords && num_big_words > gwdata->radix_bigwords) num_big_words = gwdata->radix_bigwords;
		if (k == 1.0 && n % jmptab->fftlen == 0)
			bits_per_output_word = 
				2.0 * (num_b_in_big_word * log2b - 1.0) +
				0.6 * log2 (num_big_words + num_small_words / pow (2.0, log2b / 0.6)) +
				log2k + 1.7 * log2c;
		else
			bits_per_output_word = 
				floor (2.0 * (b_per_input_word + 1.0)) * log2b - 2.0 +
				0.6 * log2 (num_big_words + num_small_words / pow (2.0, log2b / 0.6)) +
				log2k + 1.7 * log2c;

		max_weighted_bits_per_output_word = 2.0 * max_bits_per_input_word + 0.6 * log2 (jmptab->fftlen);
		if (k == 1.0 && n % jmptab->fftlen == 0) {
			
			weighted_bits_per_output_word =	bits_per_output_word + 1.0;
		} else {
			weighted_bits_per_output_word =
				2.0 * ((b_per_input_word + 1.0) * log2b - 1.0) +
				0.6 * log2 (jmptab->fftlen) + log2k + 1.7 * log2c;

			if (num_big_words == 1 && k > 1.0)
				weighted_bits_per_output_word += log2b;

			else if (! is_pathological_distribution (num_big_words, num_small_words)) {
				weighted_bits_per_output_word -=
					((log2b <= 3.0) ? (log2b - 1.0) / 2.0 :
					 (log2b <= 6.0) ? 1.0 + (log2b - 3.0) / 3.0 :
					 (log2b <= 12.5) ? 2.0 + (log2b - 6.0) / 6.5 : 3.0);
				if (b != 2) weighted_bits_per_output_word += 0.3;
			}
		}

		if (weighted_bits_per_output_word <= max_weighted_bits_per_output_word) {
			double	carries_spread_over;

			if (! (gwdata->cpu_flags & (CPU_AVX512F | CPU_AVX | CPU_SSE2)))
				carries_spread_over = 4.0;
			else if (jmptab->fftlen == 80 || jmptab->fftlen == 112)
				carries_spread_over = 4.0;
			else if ((impl_jmptab->flags & 0x1FF) == 0)	
				carries_spread_over = 6.0;
			else						
				carries_spread_over = 6.0;

			if (!gwdata->radix_bigwords &&			
			    bits_per_output_word + log2maxmulbyconst > (floor (b_per_input_word) + ceil (carries_spread_over * b_per_input_word)) * log2b) {

				goto next2;
			}

			if ((impl_jmptab->flags & 0x1FF) == 0 &&
			    logbk > ceil (jmptab->fftlen * b_per_input_word) - ceil ((jmptab->fftlen-3) * b_per_input_word)) {

				goto next2;
			}

			if ((impl_jmptab->flags & 0x1FF) != 0 &&
			    logbk > ceil (jmptab->fftlen * b_per_input_word) - ceil ((jmptab->fftlen-2) * b_per_input_word)) {

				goto next2;
			}

			if (larger_fftlen_count--) goto next2;

			break;
		}

next2:		jmptab = NEXT_SET_OF_JMPTABS (jmptab);
	}

	if (zpad_jmptab != NULL && zpad_jmptab->max_exp &&
	    (jmptab->max_exp == 0 || zpad_jmptab->fftlen < jmptab->fftlen || gwdata->qa_pick_nth_fft)) {
		gwdata->ZERO_PADDED_FFT = TRUE;
		gwdata->NEGACYCLIC_FFT = FALSE;
		jmptab = zpad_jmptab;
		best_impl_id = zpad_best_impl_id;
	}

	else if (jmptab->max_exp) {
		gwdata->ZERO_PADDED_FFT = FALSE;
		gwdata->NEGACYCLIC_FFT = (c > 0);
	}

	else
		return (GWERROR_TOO_LARGE);

	gwdata->jmptab = jmptab;
	gwdata->FFTLEN = jmptab->fftlen;

	qa_nth_fft = gwdata->ZERO_PADDED_FFT ? 100 : 1000;
	desired_bif = calculate_bif (gwdata, gwdata->FFTLEN, gwdata->NEGACYCLIC_FFT);
	prev_proc_ptrs[0] = NULL;
	prev_proc_ptrs[1] = NULL;
	prev_proc_ptrs[2] = NULL;
	prev_proc_ptrs[3] = NULL;
	prev_proc_ptrs[4] = NULL;
	for ( ; ; ) {
		int	arch;		
		int	best_impl_for;	
		int	fft_type;	

		if (! (jmptab->flags & 0x80000000)) {

			if (gwdata->qa_pick_nth_fft && gwdata->qa_pick_nth_fft < 1000) {
				gwdata->qa_pick_nth_fft = 1000;
				goto again;
			}

			return (GWERROR_TOO_LARGE);
		}

		arch = (jmptab->flags >> 17) & 0xF;
		if ((arch == ARCH_K8 || arch == ARCH_K10) && ! (gwdata->cpu_flags & CPU_3DNOW_PREFETCH))
			goto next3;

		if (arch == ARCH_FMA3 && ! (gwdata->cpu_flags & CPU_FMA3))
			goto next3;

		if (gwdata->bench_pick_nth_fft) {
			if (jmptab->proc_ptr == prev_proc_ptrs[0]) goto next3;
			if (jmptab->proc_ptr == prev_proc_ptrs[1]) goto next3;
			if (jmptab->proc_ptr == prev_proc_ptrs[2]) goto next3;
			if (jmptab->proc_ptr == prev_proc_ptrs[3]) goto next3;
			if (jmptab->proc_ptr == prev_proc_ptrs[4]) goto next3;
			if (CPU_ARCHITECTURE == CPU_ARCHITECTURE_AMD_K8 &&
			    (jmptab->flags & 0x1FF) != 0 &&
			    (arch == ARCH_P4 || arch == ARCH_P4TP || arch == ARCH_CORE))
				goto next3;
			if (gwdata->cpu_flags & CPU_FMA3 && (arch == ARCH_P4 || arch == ARCH_P4TP || arch == ARCH_CORE))
				goto next3;
			gwdata->bench_pick_nth_fft--;
			if (gwdata->bench_pick_nth_fft) goto next3;
			break;
		}

		if (gwdata->qa_pick_nth_fft) {
			if (jmptab->proc_ptr == prev_proc_ptrs[0]) goto next3;
			if (jmptab->proc_ptr == prev_proc_ptrs[1]) goto next3;
			if (jmptab->proc_ptr == prev_proc_ptrs[2]) goto next3;
			if (jmptab->proc_ptr == prev_proc_ptrs[3]) goto next3;
			if (jmptab->proc_ptr == prev_proc_ptrs[4]) goto next3;
			if (CPU_ARCHITECTURE == CPU_ARCHITECTURE_AMD_K8 &&
			    (arch == ARCH_P4 || arch == ARCH_P4TP || arch == ARCH_CORE))
				goto next3;
			qa_nth_fft++;
			if (qa_nth_fft <= gwdata->qa_pick_nth_fft) goto next3;
			gwdata->qa_picked_nth_fft = qa_nth_fft;
			break;
		}

		if (! (gwdata->cpu_flags & (CPU_AVX512F | CPU_AVX))) {
			if (gwdata->cpu_flags & CPU_SSE2) {
				if (gwdata->FFTLEN < 7168) break;
			} else
				break;
		}

		if (gwdata->required_pass2_size) {
			int	flags, p2size, arch;
			flags = jmptab->flags;
			if ((flags & 0x0000001FF) == 511)
				p2size = 48;
			else if ((flags & 0x0000001FF) == 510)
				p2size = 80;
			else if ((flags & 0x0000001FF) == 509)
				p2size = 32768;
			else
				p2size = (flags & 0x0000001FF) << 6;
			arch = (flags >> 17) & 0x0000000F;
			if (p2size + arch == gwdata->required_pass2_size) break;
			goto next3;
		}

		fft_type = (jmptab->flags >> 21) & 0xF;
		if (best_impl_id != -1) {
			int	flags, fft_type, arch, clm, p2size, no_prefetch, in_place;
			flags = jmptab->flags;
			no_prefetch = (flags >> 27) & 0x00000001;
			in_place = (flags >> 26) & 0x00000001;
			fft_type = (flags >> 21) & 0x0000000F;
			arch = (flags >> 17) & 0x0000000F;
			clm = (flags >> 9) & 0x0000000F;
			if ((flags & 0x0000001FF) == 511)
				p2size = 48;
			else if ((flags & 0x0000001FF) == 510)
				p2size = 80;
			else if ((flags & 0x0000001FF) == 509)
				p2size = 32768;
			else
				p2size = (flags & 0x0000001FF) << 6;
			if (internal_implementation_ids_match (best_impl_id, gwdata->FFTLEN, fft_type, no_prefetch, in_place, p2size, arch, clm)) break;
		}

		else {
			best_impl_for = (jmptab->flags >> 13) & 0xF;
			if (best_impl_for == desired_bif) break;
		}

next3:		prev_proc_ptrs[4] = prev_proc_ptrs[3];
		prev_proc_ptrs[3] = prev_proc_ptrs[2];
		prev_proc_ptrs[2] = prev_proc_ptrs[1];
		prev_proc_ptrs[1] = prev_proc_ptrs[0];
		prev_proc_ptrs[0] = jmptab->proc_ptr;
		jmptab = INC_JMPTAB (jmptab);
	}

	flags = jmptab->flags;
	gwdata->GWPROCPTRS[0] = jmptab->proc_ptr;
	if (jmptab->flags & 0x40000000) {
		struct gwasm_alt_jmptab *altjmptab = (struct gwasm_alt_jmptab *) jmptab;
		gwdata->mem_needed = altjmptab->mem_needed;
	} else
		gwdata->mem_needed = jmptab->mem_needed;

	gwdata->NO_PREFETCH_FFT = (flags >> 27) & 0x00000001;
	gwdata->IN_PLACE_FFT = (flags >> 26) & 0x00000001;
	gwdata->FFT_TYPE = (flags >> 21) & 0x0000000F;
	gwdata->ARCH = (flags >> 17) & 0x0000000F;
	if (gwdata->cpu_flags & CPU_AVX512F) gwdata->PASS1_CACHE_LINES = ((flags >> 9) & 0x0000000F) * 8;
	else if (gwdata->cpu_flags & CPU_AVX) gwdata->PASS1_CACHE_LINES = ((flags >> 9) & 0x0000000F) * 4;
	else gwdata->PASS1_CACHE_LINES = ((flags >> 9) & 0x0000000F) * 2;
	if ((flags & 0x0000001FF) == 511) gwdata->PASS2_SIZE = 48;
	else if ((flags & 0x0000001FF) == 510) gwdata->PASS2_SIZE = 80;
	else if ((flags & 0x0000001FF) == 509) gwdata->PASS2_SIZE = 32768;
	else gwdata->PASS2_SIZE = (flags & 0x0000001FF) << 6;
	if (gwdata->PASS2_SIZE) gwdata->PASS1_SIZE = gwdata->FFTLEN / gwdata->PASS2_SIZE; 
	else gwdata->PASS1_SIZE = 0;				
	if (gwdata->PASS1_SIZE == 2) gwdata->PASS1_SIZE = 0;	

	if (gwdata->PASS1_SIZE && !gwdata->IN_PLACE_FFT) {
		if (gwdata->cpu_flags & CPU_AVX512F) {		
			int	pass1_size, num_clmblks, clmblkdst, gaps;
			
			pass1_size = gwdata->PASS1_SIZE;
			num_clmblks = pass1_size >> 4;
			clmblkdst = gwdata->PASS1_CACHE_LINES * 128;
			if (pass1_size == 192 || pass1_size == 640 || pass1_size == 768 || pass1_size == 896 || pass1_size == 960 ||
			    pass1_size == 1152 || pass1_size == 1280 || pass1_size == 1344 || pass1_size == 1536 || pass1_size == 1920 ||
			    pass1_size == 2304) { 
				gwdata->SCRATCH_SIZE = num_clmblks * (clmblkdst + 64) - 64;
			} else if (gwdata->PASS1_CACHE_LINES == 8) {	
				gaps = num_clmblks / 8 - 1;
				gwdata->SCRATCH_SIZE = num_clmblks * clmblkdst + gaps * 128;
			} else if (gwdata->PASS1_CACHE_LINES == 16) {	
				gaps = num_clmblks / 8 - 1;
				gwdata->SCRATCH_SIZE = num_clmblks * clmblkdst + gaps * 192;
			} else {					
				gaps = num_clmblks / 8;
				gwdata->SCRATCH_SIZE = num_clmblks * (clmblkdst + 64) + gaps * -64;
			}
		}
		else if (gwdata->cpu_flags & CPU_AVX) {		
			int	pass1_size, pass1_chunks, gaps;
			
			pass1_size = gwdata->PASS1_SIZE;
			pass1_chunks = pass1_size >> 3;
			if ((pass1_size == 384 && gwdata->NEGACYCLIC_FFT) ||
			    (pass1_size == 640 && gwdata->NEGACYCLIC_FFT) ||
			    (pass1_size == 1536 && gwdata->NEGACYCLIC_FFT)) { 
				gwdata->SCRATCH_SIZE = pass1_chunks * (gwdata->PASS1_CACHE_LINES * 64 + 64) - 64;
			} else if (gwdata->PASS1_CACHE_LINES == 4) {	
				gaps = pass1_chunks / 8 - 1;
				gwdata->SCRATCH_SIZE = pass1_chunks * gwdata->PASS1_CACHE_LINES * 64 + gaps * 64;
			} else if (gwdata->PASS1_CACHE_LINES == 8) {	
				gaps = pass1_chunks / 8 - 1;
				gwdata->SCRATCH_SIZE = pass1_chunks * gwdata->PASS1_CACHE_LINES * 64 + gaps * 192;
			} else {					
				gaps = pass1_chunks / 8;
				gwdata->SCRATCH_SIZE = pass1_chunks * (gwdata->PASS1_CACHE_LINES * 64 + 64) + gaps * -64;
			}
		} else if (gwdata->cpu_flags & CPU_SSE2) {	
			int	pass1_chunks, gaps;
			
			pass1_chunks = gwdata->PASS1_SIZE >> 2;
			gaps = pass1_chunks / 8 - 1;
			gwdata->SCRATCH_SIZE = pass1_chunks * gwdata->PASS1_CACHE_LINES * 64 + gaps * 128;
		} else {					
			int	pass1_chunks, gaps;
			
			pass1_chunks = gwdata->PASS1_SIZE >> 1;
			gaps = pass1_chunks / 32 - 1;
			if (gaps < 3) gaps = 0;
			gwdata->SCRATCH_SIZE = pass1_chunks * gwdata->PASS1_CACHE_LINES * 32 + gaps * 64;
		}
	} else
		gwdata->SCRATCH_SIZE = 0;

	if (gwdata->cpu_flags & CPU_AVX512F) {
		gwdata->FOURKBGAPSIZE = 0;		
		gwdata->PASS2GAPSIZE = 64;		
		if (gwdata->PASS2_SIZE == 3072 || gwdata->PASS2_SIZE == 3584 || gwdata->PASS2_SIZE == 3840 || gwdata->PASS2_SIZE == 4096 ||
		    gwdata->PASS2_SIZE == 5120 || gwdata->PASS2_SIZE == 5376 || gwdata->PASS2_SIZE == 6144 || gwdata->PASS2_SIZE >= 7168) {
			gwdata->FOURKBGAPSIZE = 64;	
			gwdata->PASS2GAPSIZE = -64;	
							
		}
	}
	else if (gwdata->cpu_flags & CPU_AVX) {
		
		if (gwdata->PASS2_SIZE == 48 || gwdata->PASS2_SIZE == 64 || gwdata->PASS2_SIZE == 80 || gwdata->PASS2_SIZE == 192 || gwdata->PASS2_SIZE == 320)
			gwdata->FOURKBGAPSIZE = 0;
		if (gwdata->PASS2_SIZE == 256 || gwdata->PASS2_SIZE == 768 || gwdata->PASS2_SIZE == 2048 || gwdata->PASS2_SIZE == 2304 ||
		    gwdata->PASS2_SIZE == 3072 || gwdata->PASS2_SIZE == 3840 || gwdata->PASS2_SIZE == 4096 || gwdata->PASS2_SIZE == 5120 ||
		    gwdata->PASS2_SIZE == 6144 || gwdata->PASS2_SIZE == 6400 || gwdata->PASS2_SIZE == 8192 || gwdata->PASS2_SIZE == 9216 ||
		    gwdata->PASS2_SIZE == 10240 || gwdata->PASS2_SIZE == 12288 || gwdata->PASS2_SIZE == 15360 || gwdata->PASS2_SIZE == 16384 ||
		    gwdata->PASS2_SIZE == 20480 || gwdata->PASS2_SIZE == 25600)
			gwdata->FOURKBGAPSIZE = 64;
		if (gwdata->PASS2_SIZE == 1024 || gwdata->PASS2_SIZE == 1280 || gwdata->PASS2_SIZE == 1536 || gwdata->PASS2_SIZE == 2560 ||
		    gwdata->PASS2_SIZE == 4608 || gwdata->PASS2_SIZE == 7680 || gwdata->PASS2_SIZE == 12800)
			gwdata->FOURKBGAPSIZE = 128;
		
		if (gwdata->FOURKBGAPSIZE == 0)
			gwdata->PASS2GAPSIZE = 0;
		else if (gwdata->PASS2_SIZE == 2304 || gwdata->PASS2_SIZE == 9216)
			gwdata->PASS2GAPSIZE = 4*64;
		else if (gwdata->PASS2_SIZE * 2 * 8 / 4096 * gwdata->FOURKBGAPSIZE % 128 == 0)
			gwdata->PASS2GAPSIZE = -64;
		else
			gwdata->PASS2GAPSIZE = 0;
		
		if (gwdata->PASS2_SIZE == 0) {
			if (gwdata->FFTLEN == 1536 || gwdata->FFTLEN == 2560)
				gwdata->FOURKBGAPSIZE = 16;		
			else if (gwdata->FFTLEN == 1024 || gwdata->FFTLEN == 3072 || gwdata->FFTLEN == 5120)
				gwdata->FOURKBGAPSIZE = 32;		
			else if (gwdata->FFTLEN == 2048 || gwdata->FFTLEN == 4096 || gwdata->FFTLEN == 6144 || gwdata->FFTLEN == 8192 ||
				 gwdata->FFTLEN == 10240 || gwdata->FFTLEN == 12288 || gwdata->FFTLEN == 16384 || gwdata->FFTLEN == 18432 ||
				 gwdata->FFTLEN == 20480 || gwdata->FFTLEN == 24576 || gwdata->FFTLEN == 32768)
				gwdata->FOURKBGAPSIZE = 64;		
			else
				gwdata->FOURKBGAPSIZE = 0;		
		}
	} else {
		if (gwdata->PASS2_SIZE * 2 * 2 * 8 / 8192 * 128 % 256 == 0)
			gwdata->PASS2GAPSIZE = -128;
		else
			gwdata->PASS2GAPSIZE = 0;
	}

	asm_data = (struct gwasm_data *) gwdata->asm_data;
	if (asm_data != NULL) {
		if (gwdata->cpu_flags & CPU_AVX512F) {
			
			struct gwasm_alt_jmptab *altjmptab = (struct gwasm_alt_jmptab *) jmptab;
			asm_data->u.zmm.ZMM_PASS2_ROUTINE = altjmptab->pass2_proc_ptr;

			if (gwdata->PASS1_SIZE == 0) {
				
				asm_data->addcount1 = gwdata->FFTLEN / 128;
			}
			else {
				
				asm_data->addcount1 = gwdata->PASS1_SIZE / 16;
			}
		}
		else if (gwdata->cpu_flags & CPU_AVX) {
			if (gwdata->PASS2_SIZE == 0) {
				if (gwdata->FOURKBGAPSIZE) {				
					asm_data->count1 = gwdata->FOURKBGAPSIZE;	
					asm_data->normcount1 = gwdata->FFTLEN / 32 / gwdata->FOURKBGAPSIZE; 
					asm_data->count2 = asm_data->count1 / 2;	
					asm_data->addcount1 = asm_data->normcount1 * 4;	
				} else {						
					asm_data->count1 = gwdata->FFTLEN / 32;		
					asm_data->normcount1 = 0;			
					asm_data->count2 = gwdata->FFTLEN / 16;		
					asm_data->addcount1 = 1;			
				}
			}
			else {
				
				asm_data->addcount1 = (gwdata->FFTLEN / 2) / (gwdata->PASS2_SIZE * 4);
				
				if (jmptab->flags & 0x40000000) {
					struct gwasm_alt_jmptab *altjmptab = (struct gwasm_alt_jmptab *) jmptab;
					asm_data->u.ymm.YMM_PASS2_ROUTINE = altjmptab->pass2_proc_ptr;
				}
			}
		} else if (gwdata->cpu_flags & CPU_SSE2) {
			if (gwdata->PASS2_SIZE == 0) {
				const struct gwasm_jmptab *last_jmptab;
				
				last_jmptab = LAST_JMPTAB (jmptab);
				asm_data->addcount1 = last_jmptab->counts[0];
				asm_data->normcount1 = last_jmptab->counts[1];
				asm_data->count1 = last_jmptab->counts[2];
				asm_data->count2 = last_jmptab->counts[3];
				asm_data->count3 = last_jmptab->counts[4];
				asm_data->count4 = last_jmptab->counts[5];
				asm_data->count5 = last_jmptab->counts[6];
			} else if (gwdata->FFT_TYPE == FFT_TYPE_HOME_GROWN) {
				int	pfa, pfa_shift;
				
				asm_data->addcount1 = gwdata->PASS1_SIZE / 4;
				
				asm_data->count1 = asm_data->addcount1;
				if (gwdata->ZERO_PADDED_FFT || c < 0) asm_data->count1--;
				
				for (pfa = asm_data->addcount1, pfa_shift = 0;
				     pfa > 8;
				     pfa >>= 1, pfa_shift++);
				if (pfa == 5)
					asm_data->count3 = ((4 << 11) + 1) << pfa_shift;
				else if (pfa == 7)
					asm_data->count3 = ((4 << 22) + (2 << 11) + 1) << pfa_shift;
				else
					asm_data->count3 = asm_data->addcount1;
			} else {
				
				asm_data->addcount1 = gwdata->PASS1_SIZE / 4;
				
				asm_data->count1 = asm_data->addcount1;
				if (gwdata->ZERO_PADDED_FFT || c < 0) asm_data->count1--;
				
				asm_data->count3 = asm_data->addcount1;
			}
		} else {
			if (gwdata->PASS2_SIZE == 0) {
				
				asm_data->count1 = jmptab->counts[0];
				asm_data->count2 = jmptab->counts[1];
				asm_data->count3 = jmptab->counts[2];
				asm_data->count4 = jmptab->counts[3];
				asm_data->count5 = jmptab->counts[4];
			} else {
				
				asm_data->addcount1 = gwdata->PASS1_SIZE / 2;
				
				asm_data->count1 = asm_data->addcount1;
				if (gwdata->ZERO_PADDED_FFT || c < 0) asm_data->count1--;
			}
		}
	}

	if (gwdata->cpu_flags & CPU_AVX512F) {
		if (gwdata->PASS1_SIZE == 0) {		
			gwdata->GW_ALIGNMENT = 128;	
			gwdata->GW_ALIGNMENT_MOD = 0;
		} else {				
			gwdata->GW_ALIGNMENT = 1024;	
			gwdata->GW_ALIGNMENT_MOD = 0;	
		}
	}
	else if (gwdata->cpu_flags & CPU_AVX) {
		if (gwdata->PASS2_SIZE == 0) {		
			gwdata->GW_ALIGNMENT = 64;	
			gwdata->GW_ALIGNMENT_MOD = 0;
		} else if (gwdata->SCRATCH_SIZE == 0) {	
			gwdata->GW_ALIGNMENT = 4096;	
			gwdata->GW_ALIGNMENT_MOD = 0;
		} else {				
			gwdata->GW_ALIGNMENT = 1024;	
			gwdata->GW_ALIGNMENT_MOD = 64;	
		}
	}
	else if (gwdata->cpu_flags & CPU_SSE2) {
		if (gwdata->PASS2_SIZE == 0) {		
			gwdata->GW_ALIGNMENT = 128;	
			gwdata->GW_ALIGNMENT_MOD = 0;
		} else if (gwdata->SCRATCH_SIZE == 0) {	
			gwdata->GW_ALIGNMENT = 4096;	
			gwdata->GW_ALIGNMENT_MOD = 0;
		} else {				
			gwdata->GW_ALIGNMENT = 1024;	
			gwdata->GW_ALIGNMENT_MOD = 128; 
		}
	} else {
		if (gwdata->PASS2_SIZE == 0)		
			gwdata->GW_ALIGNMENT = 128;	
		else					
			gwdata->GW_ALIGNMENT = 4096;	
		gwdata->GW_ALIGNMENT_MOD = 0;
	}

	gwdata->datasize = addr_offset (gwdata, gwdata->FFTLEN - 1) + sizeof (double);

	return (0);
}

#define FIXED_PASS1_SINCOS_DATA			1
#define PASS2_REAL_SINCOS_DATA			2
#define PASS2_COMPLEX_SINCOS_DATA		3

struct shareable_sincos_data {
	struct shareable_sincos_data *next;	
	double	*data;				
	size_t	data_size;			
	int	use_count;			
};
struct shareable_sincos_data *shareable_data = NULL;  
gwmutex	shareable_lock;				
int	shareable_lock_initialized = FALSE;	

double *share_sincos_data (
	gwhandle *gwdata,	
	int	table_type,	
	double *table,		
	size_t	table_size)	
{
#ifdef SHARE_SINCOS_DATA
	struct shareable_sincos_data *p;	

	if (table_size == 0) return (table);

	if (!shareable_lock_initialized) {
		gwmutex_init (&shareable_lock);
		shareable_lock_initialized = TRUE;
	}
	gwmutex_lock (&shareable_lock);

	for (p = shareable_data; p != NULL; p = p->next) {
		if (p->data_size == table_size && !memcmp (p->data, table, table_size)) {
			p->use_count++;
			gwmutex_unlock (&shareable_lock);
			return (p->data);
		}
	}

	p = (struct shareable_sincos_data *) malloc (sizeof (struct shareable_sincos_data));
	if (p == NULL) {
		gwmutex_unlock (&shareable_lock);
		return (table);
	}
	p->data = (double *) aligned_malloc (table_size, 4096);
	if (p->data == NULL) {
		free (p);
		gwmutex_unlock (&shareable_lock);
		return (table);
	}
	memcpy (p->data, table, table_size);
	p->data_size = table_size;
	p->use_count = 1;
	p->next = shareable_data;
	shareable_data = p;
	gwmutex_unlock (&shareable_lock);
	return (p->data);
#else
	return (table);
#endif
}

void unshare_sincos_data (
	double *table)		
{
#ifdef SHARE_SINCOS_DATA
	struct shareable_sincos_data *p;		
	struct shareable_sincos_data **ptr_to_p;	

	if (table == NULL) return;

	if (!shareable_lock_initialized) return;
	gwmutex_lock (&shareable_lock);

	for (ptr_to_p = &shareable_data; (p = *ptr_to_p) != NULL; ptr_to_p = &p->next) {
		if (p->data != table) continue;
		if (--p->use_count == 0) {
			aligned_free (p->data);
			*ptr_to_p = p->next;
			free (p);
		}
		break;
	}

	gwmutex_unlock (&shareable_lock);
#endif
}

void gwinit2 (
	gwhandle *gwdata,	
	int	struct_size,	
	const char *version_string)
{

	if (strcmp (version_string, GWNUM_VERSION)) {
		gwdata->GWERROR = GWERROR_VERSION_MISMATCH;
		return;
	}
	if (struct_size != sizeof (gwhandle)) {
		gwdata->GWERROR = GWERROR_STRUCT_SIZE_MISMATCH;
		return;
	}

	memset (gwdata, 0, sizeof (gwhandle));
	gwdata->safety_margin = 0.0f;
	gwdata->maxmulbyconst = 3;
	gwdata->minimum_fftlen = 0;
	gwdata->larger_fftlen_count = 0;
	gwdata->num_threads = 1;
	gwdata->force_general_mod = 0;
	gwdata->use_irrational_general_mod = 0;
	gwdata->use_large_pages = 0;
	gwdata->use_benchmarks = 1;
	gwdata->gwnum_max_free_count = 10;
	gwdata->scramble_arrays = 1;
	gwdata->mem_needed = GWINIT_WAS_CALLED_VALUE;	

	init_ghandle (&gwdata->gdata);
	gwdata->gdata.allocate = &gwgiantalloc;
	gwdata->gdata.free = &gwgiantfree;
	gwdata->gdata.deallocate = &gwgiantdealloc;
	gwdata->gdata.handle = (void *) gwdata;

	if (CPU_FLAGS == 0 && CPU_SPEED == 0.0) {
		guessCpuType ();
		guessCpuSpeed ();
	}
	gwdata->cpu_flags = CPU_FLAGS;

#ifndef X86_64
	gwdata->cpu_flags &= ~(CPU_AVX512F | CPU_FMA3);
#endif

	if (! (gwdata->cpu_flags & CPU_AVX)) gwdata->cpu_flags &= ~CPU_FMA3;

	if (CPU_ARCHITECTURE == CPU_ARCHITECTURE_AMD_BULLDOZER && ((CPU_SIGNATURE >> 16) & 0xF) < 3)
		gwdata->cpu_flags &= ~(CPU_AVX512F | CPU_AVX | CPU_FMA3);

	gwbench_read_data (gwdata->cpu_flags);
}

int gwsetup (
	gwhandle *gwdata,	
	double	k,		
	unsigned long b,	
	unsigned long n,	
	signed long c)		
{
	int	gcd, error_code, setup_completed;
	double	orig_k;
	unsigned long orig_n;

	if (gwdata->mem_needed != GWINIT_WAS_CALLED_VALUE) return (GWERROR_NO_INIT);

	if (gwdata->GWERROR) return (gwdata->GWERROR);

	if (k < 1.0) return (GWERROR_K_TOO_SMALL);
	if (k > 9007199254740991.0) return (GWERROR_K_TOO_LARGE);
	if (gwdata->minimum_fftlen == 0) {
		if (gwdata->cpu_flags & CPU_AVX512F) {
			if (log2(b) * (double) n > MAX_PRIME_AVX512) return (GWERROR_TOO_LARGE);
		} else if (gwdata->cpu_flags & CPU_FMA3) {
			if (log2(b) * (double) n > MAX_PRIME_FMA3) return (GWERROR_TOO_LARGE);
		} else if (gwdata->cpu_flags & CPU_AVX) {
			if (log2(b) * (double) n > MAX_PRIME_AVX) return (GWERROR_TOO_LARGE);
		} else if (gwdata->cpu_flags & CPU_SSE2) {
			if (log2(b) * (double) n > MAX_PRIME_SSE2) return (GWERROR_TOO_LARGE);
		} else {
			if (log2(b) * (double) n > MAX_PRIME) return (GWERROR_TOO_LARGE);
		}
	}
	if ((k == 1.0 && n == 0 && c == 0) || (c < 0 && n * log ((double) b) + log (k) <= log ((double) 1-c)))
		return (GWERROR_TOO_SMALL);

	setup_completed = FALSE;
	orig_k = k;
	orig_n = n;

	while (k > 1.0 && b > 1 && fmod (k, (double) b) == 0.0) {
		k = k / (double) b;
		n = n + 1;
	}

	if (c == 0)
		gcd = 0;
	else if (k == 1.0 || labs (c) == 1)
		gcd = 1;
	else {
		stackgiant(kg,2);
		stackgiant(cg,2);
		dbltog (k, kg);
		itog (labs (c), cg);
		gcdg (kg, cg);
		gcd = cg->n[0];
	}

	if (gcd == 1 &&
	    k * gwdata->maxmulbyconst <= MAX_ZEROPAD_K &&
	    labs (c) * gwdata->maxmulbyconst <= MAX_ZEROPAD_C &&
	    log2(b) * (double) n >= 350.0 &&
	    (b == 2 || (gwdata->cpu_flags & (CPU_AVX512F | CPU_AVX | CPU_SSE2))) &&
	    !gwdata->force_general_mod) {
		error_code = internal_gwsetup (gwdata, k, b, n, c);
		if (error_code == 0) setup_completed = TRUE;
		else if (b == 2) return (error_code);
		gwdata->GENERAL_MOD = FALSE;
		gwdata->GENERAL_MMGW_MOD = FALSE;
	}

	if (!setup_completed) {
		
		if (gwdata->GW_MODULUS != NULL) {
			if (gwdata->force_general_mod == 0) gwdata->force_general_mod = 1;
			error_code = gwsetup_general_mod_giant (gwdata, gwdata->GW_MODULUS);
			if (error_code) return (error_code);
		} else {
			double	bits;
			giant	g;
			bits = (double) n * log2 (b);
			g = allocgiant (((unsigned long) bits >> 5) + 4);
			if (g == NULL) return (GWERROR_MALLOC);
			ultog (b, g);
			power (g, n);
			dblmulg (k, g);
			iaddg (c, g);
			if (gwdata->force_general_mod == 0) gwdata->force_general_mod = 1;
			error_code = gwsetup_general_mod_giant (gwdata, g);
			free (g);
			if (error_code) return (error_code);
		}
	}

	gw_as_string (gwdata->GWSTRING_REP, orig_k, b, orig_n, c);

	return (0);
}

int gwsetup_general_mod (
	gwhandle *gwdata,	
	const uint32_t *array,	
	uint32_t arraylen)	
{
	giantstruct tmp;
	tmp.sign = arraylen;
	tmp.n = (uint32_t *) array;
	while (tmp.sign && tmp.n[tmp.sign-1] == 0) tmp.sign--;
	return (gwsetup_general_mod_giant (gwdata, &tmp));
}

int gwsetup_general_mod_64 (
	gwhandle *gwdata,	
	const uint64_t *array,	
	uint64_t arraylen)	
{
	giantstruct tmp;
	tmp.sign = (int) arraylen * 2;
	tmp.n = (uint32_t *) array;
	while (tmp.sign && tmp.n[tmp.sign-1] == 0) tmp.sign--;
	return (gwsetup_general_mod_giant (gwdata, &tmp));
}

int gwsetup_general_Barrett_mod_giant (
	gwhandle *gwdata,	
	giant	g,		
	unsigned long d)	
{
	unsigned long bits;	
	unsigned long n;
	unsigned long safety_bits;
	const struct gwasm_jmptab *info;
	int	error_code;
	unsigned long fftlen, max_exponent, desired_n;
	giant	modified_modulus, tmp;

	bits = bitlen (g);

	if (d != 1) {
		modified_modulus = allocgiant ((bits >> 5) + 2);
		if (modified_modulus == NULL) {
			gwdone (gwdata);
			return (GWERROR_MALLOC);
		}
		gtog (g, modified_modulus);
		ulmulg (d, modified_modulus);
		g = modified_modulus;
		bits = bitlen (g);
	} else
		modified_modulus = NULL;

	n = bits + bits + 128;

	gwdata->safety_margin -= 0.15f;
	error_code = gwinfo (gwdata, 1.0, 2, n, -1);
	gwdata->safety_margin += 0.15f;
	if (error_code) return (error_code);
	info = gwdata->jmptab;
	fftlen = info->fftlen;
	max_exponent = adjusted_max_exponent (gwdata, info);

	if (n < 12 * fftlen) n = 12 * fftlen;

	if (!gwdata->use_irrational_general_mod) {
		desired_n = round_up_to_multiple_of (n, fftlen);
		if (desired_n < max_exponent) n = desired_n;
	}

	else {
		double	prime_number, bits_per_word;

		prime_number = 53.0;
		bits_per_word = (double) n / (double) fftlen;
		bits_per_word = (ceil (bits_per_word * prime_number) + 0.5)/ prime_number;

		desired_n = (unsigned long) ceil (bits_per_word * (double) fftlen);
		if (desired_n < max_exponent) n = desired_n;
	}

	if (n + fftlen < max_exponent) n = n + fftlen;
	else if (gwdata->use_irrational_general_mod && n + fftlen / 2 < max_exponent) n = n + fftlen / 2;

	gwdata->safety_margin -= 0.15f;
	int saved_minimum_fftlen = gwdata->minimum_fftlen;		
	int saved_larger_fftlen_count = gwdata->larger_fftlen_count;	
	gwdata->minimum_fftlen = fftlen;				
	gwdata->larger_fftlen_count = 0;				
	error_code = internal_gwsetup (gwdata, 1.0, 2, n, -1);
	gwdata->larger_fftlen_count = saved_larger_fftlen_count;	
	gwdata->minimum_fftlen = saved_minimum_fftlen;			
	gwdata->safety_margin += 0.15f;
	if (error_code) return (error_code);

	if (!gwdata->information_only) {
		gwdata->GW_MODULUS_FFT = gwalloc_internal (gwdata);
		if (gwdata->GW_MODULUS_FFT == NULL) {
			gwdone (gwdata);
			return (GWERROR_MALLOC);
		}
		gianttogw (gwdata, g, gwdata->GW_MODULUS_FFT);
		gwfft (gwdata, gwdata->GW_MODULUS_FFT, gwdata->GW_MODULUS_FFT);

		gwdata->GW_ZEROWORDSLOW = (unsigned long) floor ((double) bits / gwdata->avg_num_b_per_word);

		safety_bits = bits - (unsigned long) ((double) gwdata->GW_ZEROWORDSLOW * gwdata->avg_num_b_per_word) + 3;

		gwdata->GW_RECIP_FFT = gwalloc_internal (gwdata);
		if (gwdata->GW_RECIP_FFT == NULL) {
			gwdone (gwdata);
			return (GWERROR_MALLOC);
		}
		tmp = allocgiant ((gwdata->n >> 5) + 1);
		if (tmp == NULL) {
			gwdone (gwdata);
			return (GWERROR_MALLOC);
		}
		itog (1, tmp);
		gshiftleft (bits + gwdata->n / 2 - safety_bits, tmp);
		divg (g, tmp);		
		gshiftleft (gwdata->n - (bits + gwdata->n / 2 - safety_bits), tmp);
					
		gianttogw (gwdata, tmp, gwdata->GW_RECIP_FFT);
		gwfft (gwdata, gwdata->GW_RECIP_FFT, gwdata->GW_RECIP_FFT);
		free (tmp);
		free (modified_modulus);

		gwdata->GW_GEN_MOD_MAX = (unsigned long) floor ((double)((gwdata->n/2-safety_bits+bits-8)/2) / gwdata->avg_num_b_per_word);
		gwdata->GW_GEN_MOD_MAX_OFFSET = addr_offset (gwdata, gwdata->GW_GEN_MOD_MAX-1);

		gwdata->GENERAL_MOD = TRUE;
		gwdata->GENERAL_MMGW_MOD = FALSE;
	}

	sprintf (gwdata->GWSTRING_REP, "A %ld-bit number", bits);

	return (0);
}

int gwsetup_general_mod_giant (
	gwhandle *gwdata,	
	giant	N)		
{
	giant	safe_N;			
	unsigned long bits, safe_bits;	
	int	convertible;		
	double	k;
	unsigned long n;
	signed long c;
	unsigned long d;
	int	error_code;
	unsigned long fftlen;
	gwhandle *cyclic_gwdata, *negacyclic_gwdata;

	if (gwdata->mem_needed != GWINIT_WAS_CALLED_VALUE) return (GWERROR_NO_INIT);

	if (gwdata->GWERROR) return (gwdata->GWERROR);

	bits = bitlen (N);

	d = 1;
	convertible = (!gwdata->force_general_mod && bits > 300 && convert_giant_to_k2ncd (N, &k, &n, &c, &d));

	if (!convertible || d != 1) {
		
		if (gwdata->GW_MODULUS == NULL) {
			gwdata->GW_MODULUS = allocgiant ((bits >> 5) + 1);
			if (gwdata->GW_MODULUS == NULL) {
				gwdone (gwdata);
				return (GWERROR_MALLOC);
			}
			gtog (N, gwdata->GW_MODULUS);
		}
	}

	if (convertible) {
		error_code = gwsetup (gwdata, k, 2, n, c);
		if (error_code) return (error_code);
		if (d != 1) {
			char	buf[60];
			strcpy (buf, gwdata->GWSTRING_REP);
			if (isdigit (buf[0])) sprintf (gwdata->GWSTRING_REP, "(%s)/%lu", buf, d);
			else sprintf (gwdata->GWSTRING_REP, "%s/%lu", buf, d);
		}
		return (0);
	}

	if (1) {			
		safe_N = allocgiant ((bits >> 5) + 2);
		gtog (N, safe_N);
		imulg (400000009, safe_N);
		safe_bits = bitlen (safe_N);
	} else {
		safe_N = N;
		safe_bits = bits;
	}

	if (gwdata->force_general_mod == 2 || (N->n[0] & 1) == 0 || safe_bits <= 308) {
		if (safe_N != N) free (safe_N);
		return (gwsetup_general_Barrett_mod_giant (gwdata, N, d));
	}

	gwdata->use_benchmarks = FALSE;					
	int saved_minimum_fftlen = gwdata->minimum_fftlen;		
	int saved_larger_fftlen_count = gwdata->larger_fftlen_count;	
	int larger_fftlen_count = saved_larger_fftlen_count;		
	gwdata->larger_fftlen_count = 0;
	for ( ; ; gwdata->minimum_fftlen = fftlen + 1) {
		
		n = safe_bits + 128;

		gwdata->safety_margin -= 0.15f;
		gwdata->required_pass2_size = 0;			
		error_code = gwinfo (gwdata, 1.0, 2, n, 1);
		gwdata->safety_margin += 0.15f;
		if (error_code) return (error_code);

		const struct gwasm_jmptab *info = gwdata->jmptab;
		fftlen = info->fftlen;
		unsigned long max_exponent = adjusted_max_exponent (gwdata, info);

		if (n < 12 * fftlen) n = 12 * fftlen;

		n = round_up_to_multiple_of (n, fftlen);
		if (n > max_exponent) continue;

		gwdata->minimum_fftlen = fftlen;					
		gwdata->required_pass2_size = gwdata->PASS2_SIZE + gwdata->ARCH;	
		error_code = gwinfo (gwdata, 1.0, 2, n, 1);
		if (error_code) return (error_code);
		if (gwdata->jmptab->fftlen != fftlen) continue;
		error_code = gwinfo (gwdata, 1.0, 2, n, -1);
		if (error_code == GWERROR_TOO_LARGE) continue;				
		if (error_code) return (error_code);
		if (gwdata->jmptab->fftlen != fftlen) continue;

		if (larger_fftlen_count == 0) break;
		larger_fftlen_count--;
	}

	giant R = allocgiant (2 * (n >> 5) + 2);
	giant Np = allocgiant ((n >> 5) + 2);
	if (R == NULL || Np == NULL) { gwdone (gwdata); return (GWERROR_MALLOC); }
	itog (1, R); gshiftleft (n, R); sladdg (-1, R);			
	gtog (safe_N, Np);
	invg (R, Np);							

	if (Np->sign <= 0) {
		Np->sign = -Np->sign; modg (Np, R); if (!isZero(R)) { gwdone (gwdata); return (GWERROR_INTERNAL+12); } 
		free (Np); free (R);
		gwdata->larger_fftlen_count = saved_larger_fftlen_count;	
		gwdata->minimum_fftlen = saved_minimum_fftlen;			
		gwdata->required_pass2_size = 0;				
		if (safe_N != N) free (safe_N);
		return (gwsetup_general_Barrett_mod_giant (gwdata, N, d));
	}

	gwdata->cyclic_gwdata = cyclic_gwdata = (gwhandle *) malloc (sizeof (gwhandle));
	gwdata->negacyclic_gwdata = negacyclic_gwdata = (gwhandle *) malloc (sizeof (gwhandle));
	if (cyclic_gwdata == NULL || negacyclic_gwdata == NULL) {
		gwdone (gwdata);
		return (GWERROR_MALLOC);
	}
	memcpy (cyclic_gwdata, gwdata, sizeof (gwhandle));
	memcpy (negacyclic_gwdata, gwdata, sizeof (gwhandle));
	cyclic_gwdata->GW_MODULUS = negacyclic_gwdata->GW_MODULUS = NULL;
	cyclic_gwdata->parent_gwdata = negacyclic_gwdata->parent_gwdata = gwdata;

	error_code = internal_gwsetup (cyclic_gwdata, 1.0, 2, n, -1);
	if (error_code) return (error_code);
	error_code = internal_gwsetup (negacyclic_gwdata, 1.0, 2, n, 1);
	if (error_code) return (error_code);
	ASSERTG (cyclic_gwdata->FFTLEN == fftlen && negacyclic_gwdata->FFTLEN == fftlen);
	ASSERTG (fftlen == 32 || addr_offset (cyclic_gwdata, fftlen/2-17) == addr_offset (negacyclic_gwdata, fftlen/2-17));
	ASSERTG (addr_offset (cyclic_gwdata, fftlen/2+13) == addr_offset (negacyclic_gwdata, fftlen/2+13));
	ASSERTG (cyclic_gwdata->PASS2_SIZE == negacyclic_gwdata->PASS2_SIZE);
	ASSERTG (cyclic_gwdata->num_threads == negacyclic_gwdata->num_threads);

	gwdata->larger_fftlen_count = saved_larger_fftlen_count;	
	gwdata->minimum_fftlen = saved_minimum_fftlen;			

	gwdata->k = 1.0;
	gwdata->b = 2;
	gwdata->n = n;
	gwdata->c = 1;
	gwdata->bit_length = n;
	gwdata->GENERAL_MMGW_MOD = TRUE;		
	gwdata->GENERAL_MOD = FALSE;
	gwdata->FFTLEN = cyclic_gwdata->FFTLEN;
	gwdata->RATIONAL_FFT = TRUE; ASSERTG (cyclic_gwdata->RATIONAL_FFT && negacyclic_gwdata->RATIONAL_FFT);
	gwdata->avg_num_b_per_word = cyclic_gwdata->avg_num_b_per_word;
	gwdata->NUM_B_PER_SMALL_WORD = cyclic_gwdata->NUM_B_PER_SMALL_WORD;
	gwdata->fft_max_bits_per_word = negacyclic_gwdata->fft_max_bits_per_word;
	gwdata->EXTRA_BITS = cyclic_gwdata->EXTRA_BITS < negacyclic_gwdata->EXTRA_BITS ? cyclic_gwdata->EXTRA_BITS : negacyclic_gwdata->EXTRA_BITS;
	cyclic_gwdata->EXTRA_BITS = negacyclic_gwdata->EXTRA_BITS = gwdata->EXTRA_BITS;	
	gwdata->datasize = round_up_to_multiple_of (cyclic_gwdata->datasize + GW_HEADER_SIZE(negacyclic_gwdata), 128) + negacyclic_gwdata->datasize;
	gwdata->num_threads = intmin (cyclic_gwdata->num_threads, negacyclic_gwdata->num_threads);

	if (!gwdata->information_only) {
		gwdata->FFT1_state = 0;				
		atomic_set (gwdata->clone_count, 0);
		gwdata->gwnum_alloc = (gwnum *) malloc (50 * sizeof (gwnum));
		if (gwdata->gwnum_alloc == NULL) { gwdone (gwdata); return (GWERROR_MALLOC); }
		gwdata->gwnum_free = (gwnum *) malloc (50 * sizeof (gwnum));
		if (gwdata->gwnum_free == NULL) { gwdone (gwdata); return (GWERROR_MALLOC); }
		gwdata->gwnum_alloc_count = 0;
		gwdata->gwnum_alloc_array_size = 50;
		gwdata->gwnum_free_count = 0;
		gwmutex_init (&gwdata->alloc_lock);
		gwdata->GW_ALIGNMENT = cyclic_gwdata->GW_ALIGNMENT;
		gwdata->GW_ALIGNMENT_MOD = cyclic_gwdata->GW_ALIGNMENT_MOD;
		gwdata->gdata.blksize = gwnum_datasize (gwdata);

		gwdata->Np_R = gwalloc_internal (cyclic_gwdata);
		gwdata->N_Q = gwalloc_internal (negacyclic_gwdata);
		gwdata->R2_4 = gwalloc_internal (gwdata);
		if (gwdata->Np_R == NULL || gwdata->N_Q == NULL || gwdata->R2_4 == NULL) {
			gwdone (gwdata);
			return (GWERROR_MALLOC);
		}

		multithread_init (gwdata);

		gianttogw (cyclic_gwdata, Np, gwdata->Np_R);
		gwfft (cyclic_gwdata, gwdata->Np_R, gwdata->Np_R);

		gianttogw (negacyclic_gwdata, safe_N, gwdata->N_Q);
		if (1) {						
			gwnum tmp = gwdata->R2_4;
			gwmul3_carefully (cyclic_gwdata, gwdata->N_Q, gwdata->Np_R, tmp, GWMUL_PRESERVE_S1);
			gwtogiant (cyclic_gwdata, tmp, Np);
			if (!isone (Np)) { gwdone (gwdata); return (GWERROR_INTERNAL+13); }
			gwfree_internal_memory (cyclic_gwdata);
		}
		gwfft (negacyclic_gwdata, gwdata->N_Q, gwdata->N_Q);

		if (R->n[0] & 1) addg (safe_N, R);				
		gshiftright (1, R);						
		modg (safe_N, R);						
		squareg (R);							
		modg (safe_N, R);						
		gianttogw (gwdata, R, gwdata->R2_4);
		gwfft (gwdata, gwdata->R2_4, gwdata->R2_4);
	}

	free (Np);
	free (R);
	if (safe_N != N) free (safe_N);

	sprintf (gwdata->GWSTRING_REP, "A %ld-bit number", bits);

	return (0);
}

int gwsetup_without_mod (
	gwhandle *gwdata,	
	unsigned long n)	
{
	const struct gwasm_jmptab *info;
	unsigned long fftlen, max_exponent, desired_n;
	int	error_code;

	if (gwdata->mem_needed != GWINIT_WAS_CALLED_VALUE) return (GWERROR_NO_INIT);

	if (gwdata->GWERROR) return (gwdata->GWERROR);

	gwdata->safety_margin -= 0.15f;
	error_code = gwinfo (gwdata, 1.0, 2, n, -1);
	gwdata->safety_margin += 0.15f;
	info = gwdata->jmptab;
	if (error_code) return (error_code);

	max_exponent = adjusted_max_exponent (gwdata, info);
	fftlen = info->fftlen;

	desired_n = round_up_to_multiple_of (n, fftlen);
	if (desired_n < max_exponent) n = desired_n;

	if (n < 12 * fftlen) n = 12 * fftlen;

	gwdata->safety_margin -= 0.15f;
	error_code = internal_gwsetup (gwdata, 1.0, 2, n, -1);
	gwdata->safety_margin += 0.15f;
	if (error_code) return (error_code);

	gwdata->GENERAL_MOD = FALSE;
	gwdata->GENERAL_MMGW_MOD = FALSE;

	strcpy (gwdata->GWSTRING_REP, "No modulus");

	return (0);
}

int asm_data_carries_size (		
	gwhandle *gwdata)
{
	if (gwdata->cpu_flags & CPU_AVX512F) return (gwdata->PASS1_SIZE == 0 ? gwdata->FFTLEN / 8 : gwdata->PASS1_SIZE);
	if (gwdata->cpu_flags & CPU_AVX) return (gwdata->PASS1_SIZE);
	if (gwdata->cpu_flags & CPU_SSE2) return (gwdata->PASS1_SIZE * 2);
	return (gwdata->PASS1_SIZE);
}

void init_asm_data_carries (
	gwhandle *gwdata,
	struct gwasm_data *asm_data)	
{
	int	carry_table_size = asm_data_carries_size (gwdata);
	double	*table = asm_data->carries;
	if (gwdata->cpu_flags & CPU_AVX512F) {
		for (int i = 0; i < carry_table_size; i++) table[i] = asm_data->u.zmm.ZMM_RNDVAL;
	}
	else if (gwdata->cpu_flags & CPU_AVX) {
		double	carryval = (gwdata->b == 2) ? 3.0 * 131072.0 * 131072.0 * 131072.0 : 0.0;
		for (int i = 0; i < carry_table_size; i++) table[i] = (gwdata->ZERO_PADDED_FFT && (i & 4)) ? 0.0 : carryval;
	}
	else if (gwdata->cpu_flags & CPU_SSE2) {
		double	xmm_bigval = 3.0 * 131072.0 * 131072.0 * 131072.0;
		for (int i = 0; i < carry_table_size; i++) table[i] = xmm_bigval;
	}
	else {
		for (int i = 0; i < carry_table_size; i++) table[i] = 0.0;
	}
}

int gwclone (
	gwhandle *cloned_gwdata,	
	gwhandle *gwdata)		
{
	int	retcode;

	if (gwdata->clone_of != NULL) gwdata = gwdata->clone_of;

	atomic_incr (gwdata->clone_count);

	gwmutex_lock (&gwclone_lock);		
	memcpy (cloned_gwdata, gwdata, sizeof (struct gwhandle_struct));
	gwmutex_unlock (&gwclone_lock);
	cloned_gwdata->clone_of = gwdata;

	init_ghandle (&cloned_gwdata->gdata);
	cloned_gwdata->gdata.allocate = &gwgiantalloc;
	cloned_gwdata->gdata.free = &gwgiantfree;
	cloned_gwdata->gdata.deallocate = &gwgiantdealloc;
	cloned_gwdata->gdata.handle = (void *) cloned_gwdata;
	cloned_gwdata->gdata.blksize = gwnum_datasize (gwdata);

	cloned_gwdata->to_radix_gwdata = NULL;
	cloned_gwdata->from_radix_gwdata = NULL;
	cloned_gwdata->cyclic_gwdata = NULL;
	cloned_gwdata->negacyclic_gwdata = NULL;

	cloned_gwdata->GW_ADDIN = NULL;
	cloned_gwdata->GW_POSTADDIN = NULL;

	cloned_gwdata->asm_data = NULL;

	if (gwdata->asm_data != NULL) {			
		void	*cloned_asm_data_alloc;
		struct gwasm_data *asm_data, *cloned_asm_data;
		cloned_asm_data_alloc = aligned_malloc (sizeof (struct gwasm_data) + NEW_STACK_SIZE, 4096);
		if (cloned_asm_data_alloc == NULL) {
			gwdone (cloned_gwdata);
			return (GWERROR_MALLOC);
		}
		cloned_gwdata->asm_data = (char *) cloned_asm_data_alloc + NEW_STACK_SIZE;

		asm_data = (struct gwasm_data *) gwdata->asm_data;
		cloned_asm_data = (struct gwasm_data *) cloned_gwdata->asm_data;
		memcpy (cloned_asm_data, asm_data, sizeof (struct gwasm_data));
		cloned_asm_data->gwdata = cloned_gwdata;
		cloned_asm_data->thread_num = 0;
		cloned_asm_data->scratch_area = NULL;
		cloned_asm_data->carries = NULL;
		if (gwdata->SCRATCH_SIZE) {
			cloned_asm_data->scratch_area = aligned_malloc (gwdata->SCRATCH_SIZE, 64);
			if (cloned_asm_data->scratch_area == NULL) {
				gwdone (cloned_gwdata);
				return (GWERROR_MALLOC);
			}
		}
		if (gwdata->PASS2_SIZE) {
			cloned_asm_data->carries = (double *) aligned_malloc (asm_data_carries_size (gwdata) * sizeof (double), 64);
			if (cloned_asm_data->carries == NULL) {
				gwdone (cloned_gwdata);
				return (GWERROR_MALLOC);
			}
			init_asm_data_carries (gwdata, cloned_asm_data);
		}
		cloned_asm_data->MAXERR = 0.0;
	}

	cloned_gwdata->GWERROR = 0;
	cloned_gwdata->fft_count = 0;
	cloned_gwdata->read_count = 0;
	cloned_gwdata->write_count = 0;
	cloned_gwdata->saved_copyz_n = 0;

	cloned_gwdata->num_threads = 1;
	cloned_gwdata->thread_callback = NULL;
	cloned_gwdata->thread_callback_data = NULL;
	cloned_gwdata->thread_ids = NULL;
	retcode = multithread_init (cloned_gwdata);
	if (retcode) {
		gwdone (cloned_gwdata);
		return (retcode);
	}

	if (gwdata->GENERAL_MMGW_MOD) {
		cloned_gwdata->cyclic_gwdata = (gwhandle *) malloc (sizeof (gwhandle));
		cloned_gwdata->negacyclic_gwdata = (gwhandle *) malloc (sizeof (gwhandle));
		if (cloned_gwdata->cyclic_gwdata == NULL || cloned_gwdata->negacyclic_gwdata == NULL) {
			gwdone (cloned_gwdata);
			return (GWERROR_MALLOC);
		}
		retcode = gwclone (cloned_gwdata->cyclic_gwdata, gwdata->cyclic_gwdata);
		if (retcode) {
			gwdone (cloned_gwdata);
			return (retcode);
		}
		retcode = gwclone (cloned_gwdata->negacyclic_gwdata, gwdata->negacyclic_gwdata);
		if (retcode) {
			gwdone (cloned_gwdata);
			return (retcode);
		}
		cloned_gwdata->cyclic_gwdata->parent_gwdata = cloned_gwdata;
		cloned_gwdata->negacyclic_gwdata->parent_gwdata = cloned_gwdata;
	}

	return (0);
}

void gwclone_merge_stats (
	gwhandle *dest_gwdata,		
	gwhandle *cloned_gwdata)	
{
	struct gwasm_data *cloned_asm_data = (struct gwasm_data *) cloned_gwdata->asm_data;
	struct gwasm_data *dest_asm_data = (struct gwasm_data *) dest_gwdata->asm_data;

	dest_gwdata->GWERROR = intmax (dest_gwdata->GWERROR, cloned_gwdata->GWERROR), cloned_gwdata->GWERROR = 0;
	dest_gwdata->fft_count += cloned_gwdata->fft_count, cloned_gwdata->fft_count = 0;
	dest_gwdata->read_count += cloned_gwdata->read_count, cloned_gwdata->read_count = 0;
	dest_gwdata->write_count += cloned_gwdata->write_count, cloned_gwdata->write_count = 0;
	if (dest_asm_data != NULL) dest_asm_data->MAXERR = fltmax (dest_asm_data->MAXERR, cloned_asm_data->MAXERR), cloned_asm_data->MAXERR = 0.0;
}

int convert_giant_to_k2ncd (
	giant	g,		
	double	*k,		
	unsigned long *n,	
	signed long *c,		
	unsigned long *d)	
{
	unsigned long less_nasty_d;
	int	i;
	uint32_t quick_test;
	giant	test_g, alloc_g;
	stackgiant(tmp,3);

	alloc_g = NULL;
	less_nasty_d = 1;
	for (*d = 1; *d <= 999; *d += 2) {

		quick_test = (g->n[1] * *d) & 0xFFFFFC00;
		if (quick_test != 0 && quick_test != 0xFFFFFC00) continue;

		if (*d == 1) {
			test_g = g;
		} else {
			if (alloc_g == NULL) {
				alloc_g = allocgiant (((bitlen (g) + 10) >> 5) + 1);
				if (alloc_g == NULL) return (FALSE);
			}
			ultog (*d, alloc_g);
			mulg (g, alloc_g);
			test_g = alloc_g;
		}

		if (*d >= 3 && less_nasty_d == 1) {
			int	count = 0;
			for (i = 0; i < test_g->sign; i++)
				if (test_g->n[i] == 0 || test_g->n[i] == -1) count++;
			if (count >= (test_g->sign >> 2)) less_nasty_d = *d;
		}

		*c = (int32_t) test_g->n[0];

		if (test_g->n[1] == 0 && test_g->n[0] <= MAX_ZEROPAD_C);
		else if (test_g->n[1] == 0xFFFFFFFF && *c < 0 && *c >= -MAX_ZEROPAD_C);
		else continue;

		for (i = 2; i < test_g->sign - 1; i++)
			if (test_g->n[i] != test_g->n[1]) break;

		tmp->n[0] = test_g->n[i]; tmp->sign = 1;
		if (test_g->sign - i >= 2) { tmp->n[1] = test_g->n[i+1]; tmp->sign = 2; }
		if (test_g->sign - i >= 3) { tmp->n[2] = test_g->n[i+2]; tmp->sign = 3; }
		if (test_g->sign - i >= 4) continue;
		if (test_g->n[1] == 0xFFFFFFFF) iaddg (1, tmp);

		*n = i * 32;
		while ((tmp->n[0] & 0x1) == 0) {
			gshiftright(1, tmp);
			(*n)++;
		}
		if (bitlen (tmp) > 51) continue;

		*k = tmp->n[0];
		if (tmp->sign == 2) *k += (double) tmp->n[1] * 4294967296.0;
		free (alloc_g);
		return (TRUE);
	}

	*d = less_nasty_d;
	free (alloc_g);
	return (FALSE);
}

int internal_gwsetup (
	gwhandle *gwdata,	
	double	k,		
	unsigned long b,	
	unsigned long n,	
	signed long c)		
{
	const struct gwasm_jmptab *info;
	void	*asm_data_alloc;
	struct gwasm_data *asm_data;
	int	error_code;
	unsigned long mem_needed;
	double	*tables;		
	unsigned long pass1_size;
	double	small_word, big_word, temp, asm_values[50];

	if (gwdata->num_threads == 0) return (GWERROR_ZERO_THREADS);

	gwdata->k = k;
	gwdata->b = b;
	gwdata->n = n;
	gwdata->c = c;

	fpu_init ();

	if (!gwclone_lock_initialized) {
		gwmutex_init (&gwclone_lock);
		gwclone_lock_initialized = TRUE;
	}

	asm_data_alloc = aligned_malloc (sizeof (struct gwasm_data) + NEW_STACK_SIZE, 4096);
	if (asm_data_alloc == NULL) {
		gwdone (gwdata);
		return (GWERROR_MALLOC);
	}
	gwdata->asm_data = (char *) asm_data_alloc + NEW_STACK_SIZE;
	asm_data = (struct gwasm_data *) gwdata->asm_data;
	memset (asm_data, 0, sizeof (struct gwasm_data));

	error_code = gwinfo (gwdata, k, b, n, c);
	if (error_code) {
		gwdone (gwdata);
		return (error_code);
	}
	info = gwdata->jmptab;

	if (!gwdata->information_only) {
		tables = NULL;
		gwdata->large_pages_ptr = NULL;
		gwdata->large_pages_gwnum = NULL;
		mem_needed = gwdata->mem_needed + gwdata->SCRATCH_SIZE;
		if (gwdata->use_large_pages) {
			tables = (double *) large_pages_malloc (mem_needed + gwnum_size (gwdata) + 4096);
			if (tables != NULL) {
				
				gwdata->large_pages_ptr = tables;
				tables = align_ptr (tables, 128);
				
				gwdata->large_pages_gwnum = align_ptr ((char *) tables + mem_needed, 128);
			}
		}
		if (tables == NULL) {
			tables = (double *) aligned_malloc (mem_needed, 4096);
			if (tables == NULL) return (GWERROR_MALLOC);
		}
		gwdata->gwnum_memory = tables;

		memset (tables, 0, mem_needed);
	}

	asm_data->FFTLEN = gwdata->FFTLEN;
	asm_data->ZERO_PADDED_FFT = gwdata->ZERO_PADDED_FFT;
	asm_data->NEGACYCLIC_FFT = gwdata->NEGACYCLIC_FFT;
	asm_data->B_IS_2 = (b == 2);

	if (!gwdata->information_only) {
		gwdata->dd_data = gwdbldbl_data_alloc ();
		if (gwdata->dd_data == NULL) {
			gwdone (gwdata);
			return (GWERROR_MALLOC);
		}
		gwfft_weight_setup (gwdata->dd_data, gwdata->ZERO_PADDED_FFT, k, b, n, c, gwdata->FFTLEN);
	}

	gwdata->bit_length = log2 (k) + n * log2 (b);

	gwdata->avg_num_b_per_word = (gwdata->ZERO_PADDED_FFT ? n * 2.0 : (log (k) / log (b) + n)) / gwdata->FFTLEN;

	gwdata->NUM_B_PER_SMALL_WORD = (unsigned long) gwdata->avg_num_b_per_word;
	small_word = pow ((double) b, gwdata->NUM_B_PER_SMALL_WORD);
	big_word = (double) b * small_word;

	gwdata->RATIONAL_FFT = asm_data->RATIONAL_FFT =
		((double) gwdata->NUM_B_PER_SMALL_WORD == gwdata->avg_num_b_per_word) && (gwdata->ZERO_PADDED_FFT || labs (c) == 1);

	gwdata->fft_max_bits_per_word = (double) adjusted_max_exponent (gwdata, info) / (double) gwdata->FFTLEN;
	if (gwdata->ZERO_PADDED_FFT) gwdata->fft_max_bits_per_word += 0.3;

	gwdata->EXTRA_BITS = (float) ((gwdata->fft_max_bits_per_word - gwdata->safety_margin - gwdata->polymult_safety_margin - virtual_bits_per_word (gwdata)) * 2.0);
	if (gwdata->EXTRA_BITS < 0.0f) gwdata->EXTRA_BITS = 0.0f;
	gwdata->EXTRA_BITS += EB_GWMUL_SAVINGS;

	if (gwdata->PASS2_SIZE)
		pass1_size = gwdata->PASS1_SIZE;
	else
		pass1_size = gwdata->FFTLEN;

	if (!gwdata->information_only) {

	    if (gwdata->cpu_flags & CPU_AVX512F) {
		double	rndval_base;
		int	cnt;

		asm_data->u.zmm.ZMM_LARGE_BASE = big_word;				
		asm_data->u.zmm.ZMM_LARGE_BASE_INVERSE = 1.0 / big_word;		
		asm_data->u.zmm.ZMM_SMALL_BASE = small_word;				
		asm_data->u.zmm.ZMM_SMALL_BASE_INVERSE = 1.0 / small_word;		

		rndval_base = ((double) gwdata->NUM_B_PER_SMALL_WORD == gwdata->avg_num_b_per_word ? small_word : big_word);
		ASSERTG (rndval_base < 67108864.0);
		cnt = (int) log2 (rndval_base) + 1;
		asm_data->u.zmm.ZMM_RNDVAL = 3.0 * 131072.0 * 131072.0 * 131072.0;	
		asm_data->u.zmm.ZMM_RNDVAL /= pow (2.0, cnt);
		asm_data->u.zmm.ZMM_RNDVAL += fltmod (rndval_base - fltmod (asm_data->u.zmm.ZMM_RNDVAL, rndval_base), rndval_base);
		asm_data->u.zmm.ZMM_RNDVAL *= pow (2.0, cnt);

		asm_data->u.zmm.ZMM_RNDVAL_TIMES_LARGE_BASE = asm_data->u.zmm.ZMM_RNDVAL * big_word - asm_data->u.zmm.ZMM_RNDVAL;
		asm_data->u.zmm.ZMM_RNDVAL_TIMES_SMALL_BASE = asm_data->u.zmm.ZMM_RNDVAL * small_word - asm_data->u.zmm.ZMM_RNDVAL;
		asm_data->u.zmm.ZMM_RNDVAL_OVER_LARGE_BASE = asm_data->u.zmm.ZMM_RNDVAL / big_word - asm_data->u.zmm.ZMM_RNDVAL;
		asm_data->u.zmm.ZMM_RNDVAL_OVER_SMALL_BASE = asm_data->u.zmm.ZMM_RNDVAL / small_word - asm_data->u.zmm.ZMM_RNDVAL;

		ASSERTG (((tables - gwdata->gwnum_memory) & 7) == 0);
		gwdata->pass1_var_data = tables;
		tables = zr4dwpn_build_pass1_table (gwdata, tables);
		tables = round_to_cache_line (tables);
		
		if (gwdata->PASS1_SIZE == 0) {
			asm_data->sincos2 = gwdata->pass1_var_data;
		} else {
			ASSERTG (((tables - gwdata->gwnum_memory) & 7) == 0);
			asm_data->sincos2 = tables;
			tables = zr4dwpn_build_fixed_pass1_table (gwdata, tables);
			asm_data->sincos2 = share_sincos_data (gwdata, FIXED_PASS1_SINCOS_DATA, asm_data->sincos2, (char *) tables - (char *) asm_data->sincos2);
			tables = round_to_cache_line (tables);
		}

		ASSERTG (((tables - gwdata->gwnum_memory) & 7) == 0);
		asm_data->xsincos_complex = tables;
		tables = zr4_build_pass2_complex_table (gwdata, tables);
		asm_data->xsincos_complex = share_sincos_data (gwdata, PASS2_COMPLEX_SINCOS_DATA, asm_data->xsincos_complex, (char *) tables - (char *) asm_data->xsincos_complex);
		tables = round_to_cache_line (tables);
		ASSERTG (((tables - gwdata->gwnum_memory) & 7) == 0);
		asm_data->sincos3 = tables;
		tables = zr4_build_pass2_real_table (gwdata, tables);
		asm_data->sincos3 = share_sincos_data (gwdata, PASS2_REAL_SINCOS_DATA, asm_data->sincos3, (char *) tables - (char *) asm_data->sincos3);
		tables = round_to_cache_line (tables);

		ASSERTG (((tables - gwdata->gwnum_memory) & 7) == 0);
		asm_data->carries = tables;
		if (gwdata->PASS1_SIZE == 0) tables += gwdata->FFTLEN / 8;	
		else tables += gwdata->PASS1_SIZE;				
		tables = round_to_cache_line (tables);

		ASSERTG (((tables - gwdata->gwnum_memory) & 7) == 0);
		asm_data->norm_grp_mults = tables;
		tables = zr4dwpn_build_norm_table (gwdata, tables);
		tables = round_to_cache_line (tables);

		if (gwdata->SCRATCH_SIZE) {
			ASSERTG (((tables - gwdata->gwnum_memory) & 7) == 0);
			asm_data->scratch_area = tables;
			tables = (double *) ((char *) tables + gwdata->SCRATCH_SIZE);
			tables = round_to_cache_line (tables);
		}

		ASSERTG (((tables - gwdata->gwnum_memory) & 7) == 0);
		tables = zr4dwpn_build_biglit_table (gwdata, tables);
		tables = round_to_cache_line (tables);
		tables = zr4dwpn_build_fudge_table (gwdata, tables);
		tables = round_to_cache_line (tables);

#ifdef GDEBUG_MEM
		{
		char buf[80];
		sprintf (buf, "FFTlen: %d, clm: %d\n", (int) gwdata->FFTLEN, (int) gwdata->PASS1_CACHE_LINES); OutputBoth (0, buf);
		sprintf (buf, "FFTlen: %d, scratch area: %d\n", (int) gwdata->FFTLEN, (int) gwdata->SCRATCH_SIZE); OutputBoth (0, buf);
		sprintf (buf, "FFTlen: %d, pass1 var s/c data: %d\n", (int) gwdata->FFTLEN, (int) ((intptr_t) asm_data->sincos2 - (intptr_t) gwdata->pass1_var_data)); OutputBoth (0, buf);
		sprintf (buf, "FFTlen: %d, pass1 fixed s/c data: %d\n", (int) gwdata->FFTLEN, (int) ((intptr_t) asm_data->xsincos_complex - (intptr_t) asm_data->sincos2)); OutputBoth (0, buf);
		sprintf (buf, "FFTlen: %d, pass2 complex s/c data: %d\n", (int) gwdata->FFTLEN, (int) ((intptr_t) asm_data->sincos3 - (intptr_t) asm_data->xsincos_complex)); OutputBoth (0, buf);
		sprintf (buf, "FFTlen: %d, pass2 real s/c data: %d\n", (int) gwdata->FFTLEN, (int) ((intptr_t) asm_data->carries - (intptr_t) asm_data->sincos3)); OutputBoth (0, buf);
		sprintf (buf, "FFTlen: %d, carries: %d\n", (int) gwdata->FFTLEN, (int) ((intptr_t) asm_data->norm_grp_mults - (intptr_t) asm_data->carries)); OutputBoth (0, buf);
		sprintf (buf, "FFTlen: %d, norm grp mults: %d\n", (int) gwdata->FFTLEN, (int) ((intptr_t) asm_data->scratch_area - (intptr_t) asm_data->norm_grp_mults)); OutputBoth (0, buf);
		}
#endif

		asm_data->u.zmm.ZMM_SRC_INCR = (intptr_t) addr_offset (gwdata, 1) - (intptr_t) addr_offset (gwdata, 0);
		ASSERTG (asm_data->u.zmm.ZMM_SRC_INCR == (intptr_t) addr_offset (gwdata, 2) - (intptr_t) addr_offset (gwdata, 1));
		ASSERTG (asm_data->u.zmm.ZMM_SRC_INCR == (intptr_t) addr_offset (gwdata, 3) - (intptr_t) addr_offset (gwdata, 2));
		ASSERTG (asm_data->u.zmm.ZMM_SRC_INCR == (intptr_t) addr_offset (gwdata, 4) - (intptr_t) addr_offset (gwdata, 3));
		ASSERTG (asm_data->u.zmm.ZMM_SRC_INCR == (intptr_t) addr_offset (gwdata, 5) - (intptr_t) addr_offset (gwdata, 4));
		ASSERTG (asm_data->u.zmm.ZMM_SRC_INCR == (intptr_t) addr_offset (gwdata, 6) - (intptr_t) addr_offset (gwdata, 5));
		ASSERTG (asm_data->u.zmm.ZMM_SRC_INCR == (intptr_t) addr_offset (gwdata, 7) - (intptr_t) addr_offset (gwdata, 6));
	    }

	    else if (gwdata->cpu_flags & CPU_AVX) {

		if (gwdata->PASS2_SIZE == 0) {

			ASSERTG (((tables - gwdata->gwnum_memory) & 7) == 0);
			asm_data->sincos1 = tables;
			tables = yr4_build_onepass_sincos_table (gwdata, tables);

			ASSERTG (((tables - gwdata->gwnum_memory) & 7) == 0);
			asm_data->norm_col_mults = tables;
			tables = yr4_build_onepass_norm_table (gwdata, tables);

			ASSERTG (((tables - gwdata->gwnum_memory) & 7) == 0);
			asm_data->norm_biglit_array = tables;
			tables = yr4_build_onepass_biglit_table (gwdata, tables);
		}

		else if (gwdata->FFT_TYPE == FFT_TYPE_RADIX_4_DWPN) {

			ASSERTG (((tables - gwdata->gwnum_memory) & 7) == 0);
			tables = yr4dwpn_build_pass1_table (gwdata, tables);
			ASSERTG (((tables - gwdata->gwnum_memory) & 7) == 0);
			asm_data->sincos2 = tables;
			tables = yr4dwpn_build_fixed_pass1_table (gwdata, tables);
			asm_data->sincos2 = share_sincos_data (gwdata, FIXED_PASS1_SINCOS_DATA, asm_data->sincos2, (char *) tables - (char *) asm_data->sincos2);

			ASSERTG (((tables - gwdata->gwnum_memory) & 7) == 0);
			asm_data->xsincos_complex = tables;
			tables = yr4_build_pass2_complex_table (gwdata, tables);
			asm_data->xsincos_complex = share_sincos_data (gwdata, PASS2_COMPLEX_SINCOS_DATA, asm_data->xsincos_complex, (char *) tables - (char *) asm_data->xsincos_complex);
			ASSERTG (((tables - gwdata->gwnum_memory) & 7) == 0);
			asm_data->sincos3 = tables;
			tables = yr4_build_pass2_real_table (gwdata, tables);
			asm_data->sincos3 = share_sincos_data (gwdata, PASS2_REAL_SINCOS_DATA, asm_data->sincos3, (char *) tables - (char *) asm_data->sincos3);

			if (gwdata->PASS2_SIZE) {
				ASSERTG (((tables - gwdata->gwnum_memory) & 7) == 0);
				asm_data->carries = tables;
				tables += gwdata->PASS1_SIZE;
				tables += (8 - (tables - gwdata->gwnum_memory)) & 7;
			}

			ASSERTG (((tables - gwdata->gwnum_memory) & 7) == 0);
			asm_data->norm_grp_mults = tables;
			tables = yr4dwpn_build_norm_table (gwdata, tables);

			if (gwdata->SCRATCH_SIZE) {
				ASSERTG (((tables - gwdata->gwnum_memory) & 7) == 0);
				asm_data->scratch_area = tables;
				tables = (double *) ((char *) tables + gwdata->SCRATCH_SIZE);
			}

			ASSERTG (((tables - gwdata->gwnum_memory) & 7) == 0);
			asm_data->norm_biglit_array = tables;
			tables = yr4dwpn_build_biglit_table (gwdata, tables);
			tables += (8 - (tables - gwdata->gwnum_memory)) & 7;

			asm_data->norm_col_mults = NULL;
		}

#ifdef GDEBUG_MEM
	if (gwdata->PASS2_SIZE) {
		char buf[80];
		sprintf (buf, "FFTlen: %d, clm: %d, count3: %d, count2: %d\n", (int) gwdata->FFTLEN, (int) gwdata->PASS1_CACHE_LINES, (int) asm_data->count3, (int) asm_data->count2); OutputBoth (0, buf);
		sprintf (buf, "FFTlen: %d, scratch area: %d\n", (int) gwdata->FFTLEN, (int) gwdata->SCRATCH_SIZE); OutputBoth (0, buf);
		sprintf (buf, "FFTlen: %d, pass1 var s/c data: %d\n", (int) gwdata->FFTLEN, (int) ((intptr_t) asm_data->sincos2 - (intptr_t) gwdata->pass1_var_data)); OutputBoth (0, buf);
		sprintf (buf, "FFTlen: %d, pass1 fixed s/c data: %d\n", (int) gwdata->FFTLEN, (int) ((intptr_t) asm_data->xsincos_complex - (intptr_t) asm_data->sincos2)); OutputBoth (0, buf);
		sprintf (buf, "FFTlen: %d, pass2 complex s/c data: %d\n", (int) gwdata->FFTLEN, (int) ((intptr_t) asm_data->sincos3 - (intptr_t) asm_data->xsincos_complex)); OutputBoth (0, buf);
		sprintf (buf, "FFTlen: %d, pass2 real s/c data: %d\n", (int) gwdata->FFTLEN, (int) ((intptr_t) asm_data->carries - (intptr_t) asm_data->sincos3)); OutputBoth (0, buf);
		sprintf (buf, "FFTlen: %d, carries: %d\n", (int) gwdata->FFTLEN, (int) ((intptr_t) asm_data->norm_grp_mults - (intptr_t) asm_data->carries)); OutputBoth (0, buf);
		sprintf (buf, "FFTlen: %d, norm grp mults: %d\n", (int) gwdata->FFTLEN, (int) ((intptr_t) asm_data->scratch_area - (intptr_t) asm_data->norm_grp_mults)); OutputBoth (0, buf);
		sprintf (buf, "FFTlen: %d, norm biglit: %d\n", (int) gwdata->FFTLEN, (int) ((intptr_t) tables - (intptr_t) asm_data->norm_biglit_array)); OutputBoth (0, buf);
	}
#endif

		asm_data->u.ymm.YMM_SRC_INCR7 = (intptr_t) addr_offset (gwdata, 7) - (intptr_t) addr_offset (gwdata, 6);
		asm_data->u.ymm.YMM_SRC_INCR6 = (intptr_t) addr_offset (gwdata, 6) - (intptr_t) addr_offset (gwdata, 5);
		asm_data->u.ymm.YMM_SRC_INCR5 = (intptr_t) addr_offset (gwdata, 5) - (intptr_t) addr_offset (gwdata, 4);
		asm_data->u.ymm.YMM_SRC_INCR4 = (intptr_t) addr_offset (gwdata, 4) - (intptr_t) addr_offset (gwdata, 3);
		asm_data->u.ymm.YMM_SRC_INCR3 = (intptr_t) addr_offset (gwdata, 3) - (intptr_t) addr_offset (gwdata, 2);
		asm_data->u.ymm.YMM_SRC_INCR2 = (intptr_t) addr_offset (gwdata, 2) - (intptr_t) addr_offset (gwdata, 1);
		asm_data->u.ymm.YMM_SRC_INCR1 = (intptr_t) addr_offset (gwdata, 1) - (intptr_t) addr_offset (gwdata, 0);
	    }

	    else if (gwdata->cpu_flags & CPU_SSE2) {

		if (gwdata->FFT_TYPE == FFT_TYPE_RADIX_4) {

			ASSERTG (((tables - gwdata->gwnum_memory) & 15) == 0);
			tables = r4_build_pass1_table (gwdata, tables);

			ASSERTG (((tables - gwdata->gwnum_memory) & 15) == 0);
			asm_data->xsincos_complex = tables;
			tables = r4_build_pass2_complex_table (gwdata, tables);
			ASSERTG (((tables - gwdata->gwnum_memory) & 15) == 0);
			asm_data->sincos3 = tables;
			tables = r4_build_pass2_real_table (gwdata, tables);

			if (gwdata->PASS2_SIZE) {
				ASSERTG (((tables - gwdata->gwnum_memory) & 15) == 0);
				asm_data->carries = tables;
				tables += gwdata->PASS1_SIZE * 2;
				tables += (16 - (tables - gwdata->gwnum_memory)) & 15;
			}

			ASSERTG (((tables - gwdata->gwnum_memory) & 15) == 0);
			asm_data->norm_grp_mults = tables;
			tables = r4_build_norm_table (gwdata, tables, 0);

			if (gwdata->SCRATCH_SIZE) {
				ASSERTG (((tables - gwdata->gwnum_memory) & 15) == 0);
				asm_data->scratch_area = tables;
				tables = (double *) ((char *) tables + gwdata->SCRATCH_SIZE);
			}

			ASSERTG (((tables - gwdata->gwnum_memory) & 15) == 0);
			asm_data->norm_biglit_array = tables;
			tables = r4_build_biglit_table (gwdata, tables);
			tables += (16 - (tables - gwdata->gwnum_memory)) & 15;

			ASSERTG (((tables - gwdata->gwnum_memory) & 15) == 0);
			asm_data->norm_col_mults = tables;
			tables = r4_build_norm_table (gwdata, tables, 1);
		}

		else if (gwdata->FFT_TYPE == FFT_TYPE_RADIX_4_DELAYED) {

			ASSERTG (((tables - gwdata->gwnum_memory) & 15) == 0);
			tables = r4delay_build_pass1_table (gwdata, tables);
			ASSERTG (((tables - gwdata->gwnum_memory) & 15) == 0);
			asm_data->sincos1 = tables;
			tables = r4delay_build_fixed_premult_table (gwdata, tables);
			ASSERTG (((tables - gwdata->gwnum_memory) & 15) == 0);
			asm_data->sincos2 = tables;
			tables = r4delay_build_fixed_pass1_table (gwdata, tables);

			ASSERTG (((tables - gwdata->gwnum_memory) & 15) == 0);
			asm_data->xsincos_complex = tables;
			tables = r4_build_pass2_complex_table (gwdata, tables);
			ASSERTG (((tables - gwdata->gwnum_memory) & 15) == 0);
			asm_data->sincos3 = tables;
			tables = r4_build_pass2_real_table (gwdata, tables);

			if (gwdata->PASS2_SIZE) {
				ASSERTG (((tables - gwdata->gwnum_memory) & 15) == 0);
				asm_data->carries = tables;
				tables += gwdata->PASS1_SIZE * 2;
				tables += (16 - (tables - gwdata->gwnum_memory)) & 15;
			}

			ASSERTG (((tables - gwdata->gwnum_memory) & 15) == 0);
			asm_data->norm_grp_mults = tables;
			tables = r4_build_norm_table (gwdata, tables, 0);

			if (gwdata->SCRATCH_SIZE) {
				ASSERTG (((tables - gwdata->gwnum_memory) & 15) == 0);
				asm_data->scratch_area = tables;
				tables = (double *) ((char *) tables + gwdata->SCRATCH_SIZE);
			}

			ASSERTG (((tables - gwdata->gwnum_memory) & 15) == 0);
			asm_data->norm_biglit_array = tables;
			tables = r4_build_biglit_table (gwdata, tables);
			tables += (16 - (tables - gwdata->gwnum_memory)) & 15;

			ASSERTG (((tables - gwdata->gwnum_memory) & 15) == 0);
			asm_data->norm_col_mults = tables;
			tables = r4_build_norm_table (gwdata, tables, 1);
		}

		else if (gwdata->FFT_TYPE == FFT_TYPE_RADIX_4_DWPN) {

			ASSERTG (((tables - gwdata->gwnum_memory) & 15) == 0);
			tables = r4dwpn_build_pass1_table (gwdata, tables);
			ASSERTG (((tables - gwdata->gwnum_memory) & 15) == 0);
			asm_data->sincos1 = tables;
			tables = r4delay_build_fixed_premult_table (gwdata, tables);
			asm_data->sincos1 = share_sincos_data (gwdata, FIXED_PASS1_SINCOS_DATA, asm_data->sincos1, (char *) tables - (char *) asm_data->sincos1);
			ASSERTG (((tables - gwdata->gwnum_memory) & 15) == 0);
			asm_data->sincos2 = tables;
			tables = r4delay_build_fixed_pass1_table (gwdata, tables);
			asm_data->sincos2 = share_sincos_data (gwdata, FIXED_PASS1_SINCOS_DATA, asm_data->sincos2, (char *) tables - (char *) asm_data->sincos2);

			ASSERTG (((tables - gwdata->gwnum_memory) & 15) == 0);
			asm_data->xsincos_complex = tables;
			tables = r4_build_pass2_complex_table (gwdata, tables);
			asm_data->xsincos_complex = share_sincos_data (gwdata, PASS2_COMPLEX_SINCOS_DATA, asm_data->xsincos_complex, (char *) tables - (char *) asm_data->xsincos_complex);
			ASSERTG (((tables - gwdata->gwnum_memory) & 15) == 0);
			asm_data->sincos3 = tables;
			tables = r4_build_pass2_real_table (gwdata, tables);
			asm_data->sincos3 = share_sincos_data (gwdata, PASS2_REAL_SINCOS_DATA, asm_data->sincos3, (char *) tables - (char *) asm_data->sincos3);

			if (gwdata->PASS2_SIZE) {
				ASSERTG (((tables - gwdata->gwnum_memory) & 15) == 0);
				asm_data->carries = tables;
				tables += gwdata->PASS1_SIZE * 2;
				tables += (16 - (tables - gwdata->gwnum_memory)) & 15;
			}

			ASSERTG (((tables - gwdata->gwnum_memory) & 15) == 0);
			asm_data->norm_grp_mults = tables;
			tables = r4dwpn_build_norm_table (gwdata, tables);

			if (gwdata->SCRATCH_SIZE) {
				ASSERTG (((tables - gwdata->gwnum_memory) & 15) == 0);
				asm_data->scratch_area = tables;
				tables = (double *) ((char *) tables + gwdata->SCRATCH_SIZE);
			}

			ASSERTG (((tables - gwdata->gwnum_memory) & 15) == 0);
			asm_data->norm_biglit_array = tables;
			tables = r4dwpn_build_biglit_table (gwdata, tables);
			tables += (16 - (tables - gwdata->gwnum_memory)) & 15;

			asm_data->norm_col_mults = NULL;
		}

		else {

			if (gwdata->PASS2_SIZE) {
				ASSERTG (((tables - gwdata->gwnum_memory) & 15) == 0);
				asm_data->u.xmm.pass2_premults = tables;
				tables = hg_build_premult_table (gwdata, tables);

				ASSERTG (((tables - gwdata->gwnum_memory) & 15) == 0);
				asm_data->xsincos_complex = tables;
				tables = hg_build_sin_cos_table (tables, gwdata->PASS2_SIZE/2, 0, 1);

				if (!gwdata->NEGACYCLIC_FFT) {
					ASSERTG (((tables - gwdata->gwnum_memory) & 15) == 0);
					asm_data->u.xmm.sincos6 = tables;
					tables = hg_build_sin_cos_table (tables, gwdata->PASS2_SIZE * 4, 1, 2);
					asm_data->u.xmm.sincos7 = tables;
					tables = hg_build_sin_cos_table (tables, gwdata->PASS2_SIZE, 1, 1);
					tables += (16 - (tables - gwdata->gwnum_memory)) & 15;
				}

				asm_data->u.xmm.sincos8 = asm_data->u.xmm.sincos7;
				asm_data->u.xmm.sincos9 = asm_data->u.xmm.sincos7;
				asm_data->u.xmm.sincos10 = asm_data->u.xmm.sincos7;
				asm_data->u.xmm.sincos11 = asm_data->u.xmm.sincos7;
			}

			if (gwdata->PASS2_SIZE) {
				ASSERTG (((tables - gwdata->gwnum_memory) & 15) == 0);
				asm_data->carries = tables;
				tables += gwdata->PASS1_SIZE * 2;
				tables += (16 - (tables - gwdata->gwnum_memory)) & 15;
			}

			ASSERTG (((tables - gwdata->gwnum_memory) & 15) == 0);
			asm_data->norm_grp_mults = tables;
			tables = hg_build_norm_table (gwdata, tables, 0);

			if (gwdata->NEGACYCLIC_FFT) {
				ASSERTG (((tables - gwdata->gwnum_memory) & 15) == 0);
				asm_data->plus1_premults = tables;
				tables = hg_build_plus1_table (gwdata, tables);
			}

			if (gwdata->SCRATCH_SIZE) {
				ASSERTG (((tables - gwdata->gwnum_memory) & 15) == 0);
				asm_data->scratch_area = tables;
				tables = (double *) ((char *) tables + gwdata->SCRATCH_SIZE);
			}

			ASSERTG (((tables - gwdata->gwnum_memory) & 15) == 0);
			asm_data->sincos1 = tables;
			tables = hg_build_sin_cos_table (tables, pass1_size, !gwdata->NEGACYCLIC_FFT, gwdata->PASS2_SIZE == 0 ? 2 : 1);

			if (gwdata->PASS2_SIZE && pass1_size == pow_two_above_or_equal (pass1_size))
				asm_data->sincos2 = asm_data->sincos1;
			else {
				asm_data->sincos2 = tables;
				tables = hg_build_sin_cos_table (tables, pass1_size/4, !gwdata->NEGACYCLIC_FFT, 1);
			}

			if (pass1_size == pow_two_above_or_equal (pass1_size)) {
				asm_data->sincos3 = asm_data->sincos2;
				asm_data->sincos4 = asm_data->sincos2;
				asm_data->sincos5 = asm_data->sincos2;
			} else {
				asm_data->sincos3 = tables;
				tables = hg_build_sin_cos_table (tables, pass1_size/16, !gwdata->NEGACYCLIC_FFT, 1);
				asm_data->sincos4 = tables;
				tables = hg_build_sin_cos_table (tables, pass1_size/64, !gwdata->NEGACYCLIC_FFT, 1);
				asm_data->sincos5 = tables;
				tables = hg_build_sin_cos_table (tables, pass1_size/256, !gwdata->NEGACYCLIC_FFT, 1);
			}
			tables += (16 - (tables - gwdata->gwnum_memory)) & 15;

			ASSERTG (((tables - gwdata->gwnum_memory) & 15) == 0);
			asm_data->norm_biglit_array = tables;
			tables = hg_build_biglit_table (gwdata, tables);
			tables += (16 - (tables - gwdata->gwnum_memory)) & 15;

			ASSERTG (((tables - gwdata->gwnum_memory) & 15) == 0);
			asm_data->norm_col_mults = tables;
			tables = hg_build_norm_table (gwdata, tables, 1);
		}
	    }

#ifndef X86_64

	    else {

		if (gwdata->PASS2_SIZE) {
			asm_data->carries = tables;
			tables += gwdata->PASS1_SIZE;
		}

		asm_data->scratch_area = tables;
		if (gwdata->SCRATCH_SIZE)
			tables = (double *) ((char *) tables + gwdata->SCRATCH_SIZE);

		asm_data->norm_grp_mults = tables;
		tables = x87_build_norm_table (gwdata, tables, 0);

		asm_data->sincos1 = tables;
		tables = x87_build_sin_cos_table (tables, pass1_size, !gwdata->NEGACYCLIC_FFT);

		if (pass1_size == pow_two_above_or_equal (pass1_size)) {
			asm_data->sincos2 = asm_data->sincos1;
			asm_data->sincos3 = asm_data->sincos1;
			asm_data->sincos4 = asm_data->sincos1;
			asm_data->sincos5 = asm_data->sincos1;
		} else {
			asm_data->sincos2 = tables;
			tables = x87_build_sin_cos_table (tables, pass1_size/4, !gwdata->NEGACYCLIC_FFT);
			asm_data->sincos3 = tables;
			tables = x87_build_sin_cos_table (tables, pass1_size/16, !gwdata->NEGACYCLIC_FFT);
			asm_data->sincos4 = tables;
			tables = x87_build_sin_cos_table (tables, pass1_size/64, !gwdata->NEGACYCLIC_FFT);
			asm_data->sincos5 = tables;
			tables = x87_build_sin_cos_table (tables, pass1_size/256, !gwdata->NEGACYCLIC_FFT);
		}

		if (gwdata->PASS2_SIZE) {
			asm_data->u.x87.pass2_premults = tables;
			tables = x87_build_premult_table (gwdata, tables);
			asm_data->xsincos_complex = tables;
			tables = x87_build_sin_cos_table (tables, gwdata->PASS2_SIZE*2, 0);

			if (!gwdata->NEGACYCLIC_FFT) {
				asm_data->u.x87.sincos6 = tables;
				tables = x87_build_sin_cos_table (tables, gwdata->PASS2_SIZE*2, 1);
				asm_data->u.x87.sincos7 = asm_data->u.x87.sincos6;
				asm_data->u.x87.sincos8 = asm_data->u.x87.sincos6;
				asm_data->u.x87.sincos9 = asm_data->u.x87.sincos6;
				asm_data->u.x87.sincos10 = asm_data->u.x87.sincos6;
			}
		}

		if (gwdata->NEGACYCLIC_FFT) {
			asm_data->plus1_premults = tables;
			tables = x87_build_plus1_table (gwdata, tables);
		}

		asm_data->norm_col_mults = tables;
		tables = x87_build_norm_table (gwdata, tables, 1);

		asm_data->norm_biglit_array = tables;
		tables = x87_build_biglit_table (gwdata, tables);
	    }

#endif

	    if (gwdata->GWERROR) return (gwdata->GWERROR);

#ifdef GDEBUG
	    if (!gwdata->ZERO_PADDED_FFT && !gwdata->RATIONAL_FFT) {
		char buf[80];
		intptr_t mem_used = (intptr_t) tables - (intptr_t) gwdata->gwnum_memory;
		if (mem_used != mem_needed) {
			
			if (gwdata->cpu_flags & CPU_AVX512F && gwdata->PASS1_CACHE_LINES > 8) {
				if ((int) mem_used > (int) mem_needed || (int) mem_used < (int) (mem_needed * .98)) {
					sprintf (buf, "FFTlen: %d, mem needed should be approximately: %d\n",
						 (int) gwdata->FFTLEN, (int) (mem_used - gwdata->SCRATCH_SIZE));
					OutputBoth (0, buf);
				}
			} else {
				sprintf (buf, "FFTlen: %d, mem needed should be: %d\n",
					 (int) gwdata->FFTLEN, (int) (mem_used - gwdata->SCRATCH_SIZE));
				OutputBoth (0, buf);
			}
		}
	    }
#endif

	    init_asm_data_carries (gwdata, asm_data);
	}

	asm_data->cache_line_multiplier = gwdata->PASS1_CACHE_LINES;

	asm_data->ttmp_ff_inv = 2.0 / (double) gwdata->FFTLEN;

	if (!gwdata->information_only) {
	    gwasm_constants ((double *) &asm_values);

	    if (gwdata->cpu_flags & CPU_AVX512F) {
		asm_data->u.zmm.ZMM_HALF = 0.5;
		asm_data->u.zmm.ZMM_TWO = 2.0;
		asm_data->u.zmm.ZMM_ONE = 1.0;
		asm_data->u.zmm.ZMM_SQRTHALF = sqrt (0.5);
		asm_data->u.zmm.ZMM_SQRT2 = sqrt (2.0);
		asm_data->u.zmm.ZMM_ABSVAL[0] = 0xFFFFFFFF;
		asm_data->u.zmm.ZMM_ABSVAL[1] = 0x7FFFFFFF;
		asm_data->u.zmm.ZMM_B = (double) b;
		asm_data->u.zmm.ZMM_ONE_OVER_B = 1.0 / asm_data->u.zmm.ZMM_B;

		asm_data->u.zmm.ZMM_MINUS_C = (double) -c;

		if (k * big_word < 562949953421312.0) {
			asm_data->u.zmm.ZMM_K_LO = k;
			asm_data->u.zmm.ZMM_K_HI_OVER_SMALL_BASE = 0.0;
			asm_data->u.zmm.ZMM_K_HI_OVER_LARGE_BASE = 0.0;
		} else {
			asm_data->u.zmm.ZMM_K_HI_OVER_LARGE_BASE = floor (k / big_word);
			asm_data->u.zmm.ZMM_K_HI_OVER_SMALL_BASE = floor (k / big_word) * (double) b;
			asm_data->u.zmm.ZMM_K_LO = k - asm_data->u.zmm.ZMM_K_HI_OVER_LARGE_BASE * big_word;
		}
		gwsetmulbyconst (gwdata, gwdata->maxmulbyconst);

		asm_data->u.zmm.ZMM_P951 = asm_values[0];
		asm_data->u.zmm.ZMM_P588_P951 = asm_values[1];
		asm_data->u.zmm.ZMM_P309 = asm_values[2];
		asm_data->u.zmm.ZMM_P809 = -asm_values[6];
		asm_data->u.zmm.ZMM_P866 = asm_values[8];
		asm_data->u.zmm.ZMM_P383 = asm_values[21];
		asm_data->u.zmm.ZMM_P383_1 = asm_values[21];
		asm_data->u.zmm.ZMM_P434_P975 = asm_values[12];
		asm_data->u.zmm.ZMM_P782_P975 = asm_values[42];
		asm_data->u.zmm.ZMM_P901_P975 = asm_values[38];
		asm_data->u.zmm.ZMM_P623_P975 = asm_values[39];
		asm_data->u.zmm.ZMM_P223_P975 = asm_values[40];
		asm_data->u.zmm.ZMM_P1_P975 = asm_values[41];
		asm_data->u.zmm.ZMM_P259_P707 = asm_values[28];
		asm_data->u.zmm.ZMM_P966_P707 = asm_values[29];
		asm_data->u.zmm.ZMM_P924_P383 = asm_values[30];
		asm_data->u.zmm.ZMM_P981_P195 = asm_values[31];
		asm_data->u.zmm.ZMM_P195 = asm_values[33];
		asm_data->u.zmm.ZMM_P831_P556 = asm_values[34];
		asm_data->u.zmm.ZMM_P556_P195 = asm_values[37];

		asm_data->u.zmm.ZMM_PERMUTE1 = 0x0c040e0608000a02ULL;
		asm_data->u.zmm.ZMM_PERMUTE2 = 0x0d050f0709010b03ULL;
	    }

	    else if (gwdata->cpu_flags & CPU_AVX) {
		int	i;

		asm_data->u.ymm.YMM_HALF[0] = asm_data->u.ymm.YMM_HALF[1] =
		asm_data->u.ymm.YMM_HALF[2] = asm_data->u.ymm.YMM_HALF[3] = 0.5;
		asm_data->u.ymm.YMM_ONE[0] = asm_data->u.ymm.YMM_ONE[1] =
		asm_data->u.ymm.YMM_ONE[2] = asm_data->u.ymm.YMM_ONE[3] = 1.0;
		asm_data->u.ymm.YMM_TWO[0] = asm_data->u.ymm.YMM_TWO[1] =
		asm_data->u.ymm.YMM_TWO[2] = asm_data->u.ymm.YMM_TWO[3] = 2.0;
		asm_data->u.ymm.YMM_SQRTHALF[0] = asm_data->u.ymm.YMM_SQRTHALF[1] =
		asm_data->u.ymm.YMM_SQRTHALF[2] = asm_data->u.ymm.YMM_SQRTHALF[3] = sqrt (0.5);
		asm_data->u.ymm.YMM_ABSVAL[0] = asm_data->u.ymm.YMM_ABSVAL[2] =
		asm_data->u.ymm.YMM_ABSVAL[4] = asm_data->u.ymm.YMM_ABSVAL[6] = 0xFFFFFFFF;
		asm_data->u.ymm.YMM_ABSVAL[1] = asm_data->u.ymm.YMM_ABSVAL[3] =
		asm_data->u.ymm.YMM_ABSVAL[5] = asm_data->u.ymm.YMM_ABSVAL[7] = 0x7FFFFFFF;

		asm_data->u.ymm.YMM_BIGVAL[0] = asm_data->u.ymm.YMM_BIGVAL[1] =
		asm_data->u.ymm.YMM_BIGVAL[2] = asm_data->u.ymm.YMM_BIGVAL[3] = 3.0 * pow (2.0, 51.0);
		asm_data->u.ymm.YMM_BIGBIGVAL[0] = asm_data->u.ymm.YMM_BIGBIGVAL[1] =
		asm_data->u.ymm.YMM_BIGBIGVAL[2] = asm_data->u.ymm.YMM_BIGBIGVAL[3] = big_word * asm_data->u.ymm.YMM_BIGVAL[0];

		asm_data->u.ymm.YMM_MINUS_C[0] = asm_data->u.ymm.YMM_MINUS_C[1] =
		asm_data->u.ymm.YMM_MINUS_C[2] = asm_data->u.ymm.YMM_MINUS_C[3] = (double) -c;

		asm_data->u.ymm.YMM_NORM012_FF[0] = asm_data->u.ymm.YMM_NORM012_FF[1] =
		asm_data->u.ymm.YMM_NORM012_FF[2] = asm_data->u.ymm.YMM_NORM012_FF[3] =
			(gwdata->ZERO_PADDED_FFT) ?
				(double) (gwdata->FFTLEN / 2) :
				(double) (gwdata->FFTLEN / 2) / k;

		if (k * big_word < 562949953421312.0) {
			asm_data->u.ymm.YMM_K_HI[0] = asm_data->u.ymm.YMM_K_HI[1] =
			asm_data->u.ymm.YMM_K_HI[2] = asm_data->u.ymm.YMM_K_HI[3] = 0.0;
			asm_data->u.ymm.YMM_K_LO[0] = asm_data->u.ymm.YMM_K_LO[1] =
			asm_data->u.ymm.YMM_K_LO[2] = asm_data->u.ymm.YMM_K_LO[3] = k;
		} else {
			asm_data->u.ymm.YMM_K_HI[0] = asm_data->u.ymm.YMM_K_HI[1] =
			asm_data->u.ymm.YMM_K_HI[2] = asm_data->u.ymm.YMM_K_HI[3] = floor (k / big_word) * big_word;
			asm_data->u.ymm.YMM_K_LO[0] = asm_data->u.ymm.YMM_K_LO[1] =
			asm_data->u.ymm.YMM_K_LO[2] = asm_data->u.ymm.YMM_K_LO[3] = k - asm_data->u.ymm.YMM_K_HI[0];
		}
		gwsetmulbyconst (gwdata, gwdata->maxmulbyconst);

		for (i = 0; i <= 63; i++) {
			int	ymmword, bit;
			ymmword = i >> 2;
			bit = i & 3;
			if (! (ymmword & (1 << bit))) {		
				asm_data->u.ymm.YMM_LIMIT_INVERSE[i] = 1.0 / small_word;	
				if (b > 2) temp = small_word;					
				else temp = small_word * asm_data->u.ymm.YMM_BIGVAL[0] - asm_data->u.ymm.YMM_BIGVAL[0];
				asm_data->u.ymm.YMM_LIMIT_BIGMAX[i] = temp;
			} else {				
				asm_data->u.ymm.YMM_LIMIT_INVERSE[i] = 1.0 / big_word;		
				if (b > 2) temp = big_word;					
				else temp = big_word * asm_data->u.ymm.YMM_BIGVAL[0] - asm_data->u.ymm.YMM_BIGVAL[0];
				asm_data->u.ymm.YMM_LIMIT_BIGMAX[i] = temp;
			}
		}

		if (gwdata->FFT_TYPE == FFT_TYPE_RADIX_4_DWPN) {
			double	inv1, inv2, bmax1, bmax2;

			inv1 = asm_data->u.ymm.YMM_LIMIT_INVERSE[0];
			inv2 = asm_data->u.ymm.YMM_LIMIT_INVERSE[63];
			bmax1 = asm_data->u.ymm.YMM_LIMIT_BIGMAX[0];
			bmax2 = asm_data->u.ymm.YMM_LIMIT_BIGMAX[63];
			for (i = 0; i < 48; i++) {
				asm_data->u.ymm.YMM_LIMIT_INVERSE[i*4] = (((char *)gwdata->ASM_TIMERS)[i] & 1) ? inv2 : inv1;
				asm_data->u.ymm.YMM_LIMIT_INVERSE[i*4+1] = (((char *)gwdata->ASM_TIMERS)[i] & 2) ? inv2 : inv1;
				asm_data->u.ymm.YMM_LIMIT_INVERSE[i*4+2] = (((char *)gwdata->ASM_TIMERS)[i] & 4) ? inv2 : inv1;
				asm_data->u.ymm.YMM_LIMIT_INVERSE[i*4+3] = (((char *)gwdata->ASM_TIMERS)[i] & 8) ? inv2 : inv1;
				asm_data->u.ymm.YMM_LIMIT_BIGMAX[i*4] = (((char *)gwdata->ASM_TIMERS)[i] & 1) ? bmax2 : bmax1;
				asm_data->u.ymm.YMM_LIMIT_BIGMAX[i*4+1] = (((char *)gwdata->ASM_TIMERS)[i] & 2) ? bmax2 : bmax1;
				asm_data->u.ymm.YMM_LIMIT_BIGMAX[i*4+2] = (((char *)gwdata->ASM_TIMERS)[i] & 4) ? bmax2 : bmax1;
				asm_data->u.ymm.YMM_LIMIT_BIGMAX[i*4+3] = (((char *)gwdata->ASM_TIMERS)[i] & 8) ? bmax2 : bmax1;
			}
		}

		asm_data->u.ymm.YMM_P951[0] = asm_data->u.ymm.YMM_P951[1] =
		asm_data->u.ymm.YMM_P951[2] = asm_data->u.ymm.YMM_P951[3] = asm_values[0];
		asm_data->u.ymm.YMM_P618[0] = asm_data->u.ymm.YMM_P618[1] =
		asm_data->u.ymm.YMM_P618[2] = asm_data->u.ymm.YMM_P618[3] = asm_values[1];
		asm_data->u.ymm.YMM_P309[0] = asm_data->u.ymm.YMM_P309[1] =
		asm_data->u.ymm.YMM_P309[2] = asm_data->u.ymm.YMM_P309[3] = asm_values[2];
		asm_data->u.ymm.YMM_P588[0] = asm_data->u.ymm.YMM_P588[1] =
		asm_data->u.ymm.YMM_P588[2] = asm_data->u.ymm.YMM_P588[3] = asm_values[4];
		asm_data->u.ymm.YMM_P809[0] = asm_data->u.ymm.YMM_P809[1] =
		asm_data->u.ymm.YMM_P809[2] = asm_data->u.ymm.YMM_P809[3] = -asm_values[6];
		asm_data->u.ymm.YMM_P866[0] = asm_data->u.ymm.YMM_P866[1] =
		asm_data->u.ymm.YMM_P866[2] = asm_data->u.ymm.YMM_P866[3] = asm_values[8];
		asm_data->u.ymm.YMM_P975[0] = asm_data->u.ymm.YMM_P975[1] =
		asm_data->u.ymm.YMM_P975[2] = asm_data->u.ymm.YMM_P975[3] = asm_values[11];
		asm_data->u.ymm.YMM_P623[0] = asm_data->u.ymm.YMM_P623[1] =
		asm_data->u.ymm.YMM_P623[2] = asm_data->u.ymm.YMM_P623[3] = asm_values[14];
		asm_data->u.ymm.YMM_P223[0] = asm_data->u.ymm.YMM_P223[1] =
		asm_data->u.ymm.YMM_P223[2] = asm_data->u.ymm.YMM_P223[3] = -asm_values[17];
		asm_data->u.ymm.YMM_P901[0] = asm_data->u.ymm.YMM_P901[1] =
		asm_data->u.ymm.YMM_P901[2] = asm_data->u.ymm.YMM_P901[3] = -asm_values[18];
		asm_data->u.ymm.YMM_P924[0] = asm_data->u.ymm.YMM_P924[1] =
		asm_data->u.ymm.YMM_P924[2] = asm_data->u.ymm.YMM_P924[3] = asm_values[20];
		asm_data->u.ymm.YMM_P383[0] = asm_data->u.ymm.YMM_P383[1] =
		asm_data->u.ymm.YMM_P383[2] = asm_data->u.ymm.YMM_P383[3] = asm_values[21];
		asm_data->u.ymm.YMM_P782[0] = asm_data->u.ymm.YMM_P782[1] =
		asm_data->u.ymm.YMM_P782[2] = asm_data->u.ymm.YMM_P782[3] = asm_values[22];
		asm_data->u.ymm.YMM_P434[0] = asm_data->u.ymm.YMM_P434[1] =
		asm_data->u.ymm.YMM_P434[2] = asm_data->u.ymm.YMM_P434[3] = asm_values[23];
		asm_data->u.ymm.YMM_P975_P434[0] = asm_data->u.ymm.YMM_P975_P434[1] =
		asm_data->u.ymm.YMM_P975_P434[2] = asm_data->u.ymm.YMM_P975_P434[3] = asm_values[24];
		asm_data->u.ymm.YMM_P782_P434[0] = asm_data->u.ymm.YMM_P782_P434[1] =
		asm_data->u.ymm.YMM_P782_P434[2] = asm_data->u.ymm.YMM_P782_P434[3] = asm_values[25];
		asm_data->u.ymm.YMM_P259[0] = asm_data->u.ymm.YMM_P259[1] =
		asm_data->u.ymm.YMM_P259[2] = asm_data->u.ymm.YMM_P259[3] = asm_values[26];
		asm_data->u.ymm.YMM_P966[0] = asm_data->u.ymm.YMM_P966[1] =
		asm_data->u.ymm.YMM_P966[2] = asm_data->u.ymm.YMM_P966[3] = asm_values[27];
		asm_data->u.ymm.YMM_P259_P707[0] = asm_data->u.ymm.YMM_P259_P707[1] =
		asm_data->u.ymm.YMM_P259_P707[2] = asm_data->u.ymm.YMM_P259_P707[3] = asm_values[28];
		asm_data->u.ymm.YMM_P966_P707[0] = asm_data->u.ymm.YMM_P966_P707[1] =
		asm_data->u.ymm.YMM_P966_P707[2] = asm_data->u.ymm.YMM_P966_P707[3] = asm_values[29];
		asm_data->u.ymm.YMM_P924_P383[0] = asm_data->u.ymm.YMM_P924_P383[1] =
		asm_data->u.ymm.YMM_P924_P383[2] = asm_data->u.ymm.YMM_P924_P383[3] = asm_values[30];
	    }

	    else if (gwdata->cpu_flags & CPU_SSE2) {
		asm_data->u.xmm.XMM_TWO[0] = asm_data->u.xmm.XMM_TWO[1] = 2.0;
		asm_data->u.xmm.XMM_HALF[0] = asm_data->u.xmm.XMM_HALF[1] = 0.5;
		asm_data->u.xmm.XMM_SQRTHALF[0] = asm_data->u.xmm.XMM_SQRTHALF[1] = sqrt (0.5);
		asm_data->u.xmm.XMM_ABSVAL[0] = asm_data->u.xmm.XMM_ABSVAL[2] = 0xFFFFFFFF;
		asm_data->u.xmm.XMM_ABSVAL[1] = asm_data->u.xmm.XMM_ABSVAL[3] = 0x7FFFFFFF;

		asm_data->u.xmm.XMM_TTP_FUDGE[0] = asm_data->u.xmm.XMM_TTP_FUDGE[1] =
		asm_data->u.xmm.XMM_TTP_FUDGE[2] = asm_data->u.xmm.XMM_TTP_FUDGE[3] =
		asm_data->u.xmm.XMM_TTP_FUDGE[4] = asm_data->u.xmm.XMM_TTP_FUDGE[5] =
		asm_data->u.xmm.XMM_TTP_FUDGE[6] = asm_data->u.xmm.XMM_TTP_FUDGE[7] =
		asm_data->u.xmm.XMM_TTP_FUDGE[9] = asm_data->u.xmm.XMM_TTP_FUDGE[11] =
		asm_data->u.xmm.XMM_TTP_FUDGE[13] = asm_data->u.xmm.XMM_TTP_FUDGE[15] =
		asm_data->u.xmm.XMM_TTP_FUDGE[16] = asm_data->u.xmm.XMM_TTP_FUDGE[18] =
		asm_data->u.xmm.XMM_TTP_FUDGE[20] = asm_data->u.xmm.XMM_TTP_FUDGE[22] = 1.0;
		asm_data->u.xmm.XMM_TTP_FUDGE[8] = asm_data->u.xmm.XMM_TTP_FUDGE[10] =
		asm_data->u.xmm.XMM_TTP_FUDGE[12] = asm_data->u.xmm.XMM_TTP_FUDGE[14] =
		asm_data->u.xmm.XMM_TTP_FUDGE[17] = asm_data->u.xmm.XMM_TTP_FUDGE[19] =
		asm_data->u.xmm.XMM_TTP_FUDGE[21] = asm_data->u.xmm.XMM_TTP_FUDGE[23] =
		asm_data->u.xmm.XMM_TTP_FUDGE[24] = asm_data->u.xmm.XMM_TTP_FUDGE[25] =
		asm_data->u.xmm.XMM_TTP_FUDGE[26] = asm_data->u.xmm.XMM_TTP_FUDGE[27] =
		asm_data->u.xmm.XMM_TTP_FUDGE[28] = asm_data->u.xmm.XMM_TTP_FUDGE[29] =
		asm_data->u.xmm.XMM_TTP_FUDGE[30] = asm_data->u.xmm.XMM_TTP_FUDGE[31] = 1.0 / (double) b;

		asm_data->u.xmm.XMM_TTMP_FUDGE[0] = asm_data->u.xmm.XMM_TTMP_FUDGE[1] =
		asm_data->u.xmm.XMM_TTMP_FUDGE[2] = asm_data->u.xmm.XMM_TTMP_FUDGE[3] =
		asm_data->u.xmm.XMM_TTMP_FUDGE[4] = asm_data->u.xmm.XMM_TTMP_FUDGE[5] =
		asm_data->u.xmm.XMM_TTMP_FUDGE[6] = asm_data->u.xmm.XMM_TTMP_FUDGE[7] =
		asm_data->u.xmm.XMM_TTMP_FUDGE[9] = asm_data->u.xmm.XMM_TTMP_FUDGE[11] =
		asm_data->u.xmm.XMM_TTMP_FUDGE[13] = asm_data->u.xmm.XMM_TTMP_FUDGE[15] =
		asm_data->u.xmm.XMM_TTMP_FUDGE[16] = asm_data->u.xmm.XMM_TTMP_FUDGE[18] =
		asm_data->u.xmm.XMM_TTMP_FUDGE[20] = asm_data->u.xmm.XMM_TTMP_FUDGE[22] = 1.0;
		asm_data->u.xmm.XMM_TTMP_FUDGE[8] = asm_data->u.xmm.XMM_TTMP_FUDGE[10] =
		asm_data->u.xmm.XMM_TTMP_FUDGE[12] = asm_data->u.xmm.XMM_TTMP_FUDGE[14] =
		asm_data->u.xmm.XMM_TTMP_FUDGE[17] = asm_data->u.xmm.XMM_TTMP_FUDGE[19] =
		asm_data->u.xmm.XMM_TTMP_FUDGE[21] = asm_data->u.xmm.XMM_TTMP_FUDGE[23] =
		asm_data->u.xmm.XMM_TTMP_FUDGE[24] = asm_data->u.xmm.XMM_TTMP_FUDGE[25] =
		asm_data->u.xmm.XMM_TTMP_FUDGE[26] = asm_data->u.xmm.XMM_TTMP_FUDGE[27] =
		asm_data->u.xmm.XMM_TTMP_FUDGE[28] = asm_data->u.xmm.XMM_TTMP_FUDGE[29] =
		asm_data->u.xmm.XMM_TTMP_FUDGE[30] = asm_data->u.xmm.XMM_TTMP_FUDGE[31] = (double) b;

		asm_data->u.xmm.XMM_BIGVAL[0] = asm_data->u.xmm.XMM_BIGVAL[1] = 3.0 * pow (2.0, 51.0);
		asm_data->u.xmm.XMM_BIGVAL_NEG[0] = asm_data->u.xmm.XMM_BIGVAL_NEG[1] = -asm_data->u.xmm.XMM_BIGVAL[0];
		asm_data->u.xmm.XMM_BIGBIGVAL[0] = asm_data->u.xmm.XMM_BIGBIGVAL[1] = big_word * asm_data->u.xmm.XMM_BIGVAL[0];

		asm_data->u.xmm.XMM_MINUS_C[0] = asm_data->u.xmm.XMM_MINUS_C[1] = (double) -c;

		asm_data->u.xmm.XMM_NORM012_FF[0] = asm_data->u.xmm.XMM_NORM012_FF[1] =
			(gwdata->ZERO_PADDED_FFT) ?
				(double) (gwdata->FFTLEN / 2) :
				(double) (gwdata->FFTLEN / 2) / k;

		if (k * big_word < 562949953421312.0) {
			asm_data->u.xmm.XMM_K_HI[0] = asm_data->u.xmm.XMM_K_HI[1] = 0.0;
			asm_data->u.xmm.XMM_K_LO[0] = asm_data->u.xmm.XMM_K_LO[1] = k;
		} else {
			asm_data->u.xmm.XMM_K_HI[0] = asm_data->u.xmm.XMM_K_HI[1] = floor (k / big_word) * big_word;
			asm_data->u.xmm.XMM_K_LO[0] = asm_data->u.xmm.XMM_K_LO[1] = k - asm_data->u.xmm.XMM_K_HI[0];
		}
		gwsetmulbyconst (gwdata, gwdata->maxmulbyconst);

		asm_data->u.xmm.XMM_LIMIT_INVERSE[0] =
		asm_data->u.xmm.XMM_LIMIT_INVERSE[1] =
		asm_data->u.xmm.XMM_LIMIT_INVERSE[3] =
		asm_data->u.xmm.XMM_LIMIT_INVERSE[4] = 1.0 / small_word;	
		asm_data->u.xmm.XMM_LIMIT_INVERSE[2] =
		asm_data->u.xmm.XMM_LIMIT_INVERSE[5] =
		asm_data->u.xmm.XMM_LIMIT_INVERSE[6] =
		asm_data->u.xmm.XMM_LIMIT_INVERSE[7] = 1.0 / big_word;		

		if (b > 2)
			temp = small_word;
		else
			temp = small_word * asm_data->u.xmm.XMM_BIGVAL[0] - asm_data->u.xmm.XMM_BIGVAL[0];
		asm_data->u.xmm.XMM_LIMIT_BIGMAX[0] =
		asm_data->u.xmm.XMM_LIMIT_BIGMAX[1] =
		asm_data->u.xmm.XMM_LIMIT_BIGMAX[3] =
		asm_data->u.xmm.XMM_LIMIT_BIGMAX[4] = temp;

		asm_data->u.xmm.XMM_LIMIT_BIGMAX_NEG[0] =
		asm_data->u.xmm.XMM_LIMIT_BIGMAX_NEG[1] =
		asm_data->u.xmm.XMM_LIMIT_BIGMAX_NEG[3] =
		asm_data->u.xmm.XMM_LIMIT_BIGMAX_NEG[4] = -temp;	

		if (b > 2)
			temp = big_word;
		else
			temp = big_word * asm_data->u.xmm.XMM_BIGVAL[0] - asm_data->u.xmm.XMM_BIGVAL[0];
		asm_data->u.xmm.XMM_LIMIT_BIGMAX[2] =
		asm_data->u.xmm.XMM_LIMIT_BIGMAX[5] =
		asm_data->u.xmm.XMM_LIMIT_BIGMAX[6] =
		asm_data->u.xmm.XMM_LIMIT_BIGMAX[7] = temp;

		asm_data->u.xmm.XMM_LIMIT_BIGMAX_NEG[2] =
		asm_data->u.xmm.XMM_LIMIT_BIGMAX_NEG[5] =
		asm_data->u.xmm.XMM_LIMIT_BIGMAX_NEG[6] =
		asm_data->u.xmm.XMM_LIMIT_BIGMAX_NEG[7] = -temp;	

		memcpy (asm_data->u.xmm.XMM_LIMIT_INVERSE+8, asm_data->u.xmm.XMM_LIMIT_INVERSE, 8 * sizeof (double));
		memcpy (asm_data->u.xmm.XMM_LIMIT_INVERSE+16, asm_data->u.xmm.XMM_LIMIT_INVERSE, 8 * sizeof (double));
		memcpy (asm_data->u.xmm.XMM_LIMIT_INVERSE+24, asm_data->u.xmm.XMM_LIMIT_INVERSE, 8 * sizeof (double));
		memcpy (asm_data->u.xmm.XMM_LIMIT_BIGMAX+8, asm_data->u.xmm.XMM_LIMIT_BIGMAX, 8 * sizeof (double));
		memcpy (asm_data->u.xmm.XMM_LIMIT_BIGMAX+16, asm_data->u.xmm.XMM_LIMIT_BIGMAX, 8 * sizeof (double));
		memcpy (asm_data->u.xmm.XMM_LIMIT_BIGMAX+24, asm_data->u.xmm.XMM_LIMIT_BIGMAX, 8 * sizeof (double));
		memcpy (asm_data->u.xmm.XMM_LIMIT_BIGMAX_NEG+8, asm_data->u.xmm.XMM_LIMIT_BIGMAX_NEG, 8 * sizeof (double));
		memcpy (asm_data->u.xmm.XMM_LIMIT_BIGMAX_NEG+16, asm_data->u.xmm.XMM_LIMIT_BIGMAX_NEG, 8 * sizeof (double));
		memcpy (asm_data->u.xmm.XMM_LIMIT_BIGMAX_NEG+24, asm_data->u.xmm.XMM_LIMIT_BIGMAX_NEG, 8 * sizeof (double));

		if (gwdata->FFT_TYPE == FFT_TYPE_RADIX_4_DWPN) {
			int	i;
			double	inv1, inv2, bmax1, bmax2;

			inv1 = asm_data->u.xmm.XMM_LIMIT_INVERSE[0];
			inv2 = asm_data->u.xmm.XMM_LIMIT_INVERSE[7];
			bmax1 = asm_data->u.xmm.XMM_LIMIT_BIGMAX[0];
			bmax2 = asm_data->u.xmm.XMM_LIMIT_BIGMAX[7];
			for (i = 0; i < 48; i++) {
				asm_data->u.xmm.XMM_LIMIT_INVERSE[i+i] = (((char *)gwdata->ASM_TIMERS)[i] & 1) ? inv2 : inv1;
				asm_data->u.xmm.XMM_LIMIT_INVERSE[i+i+1] = (((char *)gwdata->ASM_TIMERS)[i] & 2) ? inv2 : inv1;
				asm_data->u.xmm.XMM_LIMIT_BIGMAX[i+i] = (((char *)gwdata->ASM_TIMERS)[i] & 1) ? bmax2 : bmax1;
				asm_data->u.xmm.XMM_LIMIT_BIGMAX[i+i+1] = (((char *)gwdata->ASM_TIMERS)[i] & 2) ? bmax2 : bmax1;
				asm_data->u.xmm.XMM_LIMIT_BIGMAX_NEG[i+i] = (((char *)gwdata->ASM_TIMERS)[i] & 1) ? -bmax2 : -bmax1;
				asm_data->u.xmm.XMM_LIMIT_BIGMAX_NEG[i+i+1] = (((char *)gwdata->ASM_TIMERS)[i] & 2) ? -bmax2 : -bmax1;
			}
		}

		asm_data->u.xmm.XMM_P951[0] = asm_data->u.xmm.XMM_P951[1] = asm_values[0];
		asm_data->u.xmm.XMM_P618[0] = asm_data->u.xmm.XMM_P618[1] = asm_values[1];
		asm_data->u.xmm.XMM_P309[0] = asm_data->u.xmm.XMM_P309[1] = asm_values[2];
		asm_data->u.xmm.XMM_M262[0] = asm_data->u.xmm.XMM_M262[1] = asm_values[3];
		asm_data->u.xmm.XMM_P588[0] = asm_data->u.xmm.XMM_P588[1] = asm_values[4];
		asm_data->u.xmm.XMM_M162[0] = asm_data->u.xmm.XMM_M162[1] = asm_values[5];
		asm_data->u.xmm.XMM_P809[0] = asm_data->u.xmm.XMM_P809[1] = -asm_values[6];
		asm_data->u.xmm.XMM_M809[0] = asm_data->u.xmm.XMM_M809[1] = asm_values[6];
		asm_data->u.xmm.XMM_M382[0] = asm_data->u.xmm.XMM_M382[1] = asm_values[7];
		asm_data->u.xmm.XMM_P866[0] = asm_data->u.xmm.XMM_P866[1] = asm_values[8];
		asm_data->u.xmm.XMM_P975[0] = asm_data->u.xmm.XMM_P975[1] = asm_values[11];
		asm_data->u.xmm.XMM_P445[0] = asm_data->u.xmm.XMM_P445[1] = asm_values[12];
		asm_data->u.xmm.XMM_P180[0] = asm_data->u.xmm.XMM_P180[1] = asm_values[13];
		asm_data->u.xmm.XMM_P623[0] = asm_data->u.xmm.XMM_P623[1] = asm_values[14];
		asm_data->u.xmm.XMM_M358[0] = asm_data->u.xmm.XMM_M358[1] = asm_values[15];
		asm_data->u.xmm.XMM_P404[0] = asm_data->u.xmm.XMM_P404[1] = asm_values[16];
		asm_data->u.xmm.XMM_P223[0] = asm_data->u.xmm.XMM_P223[1] = -asm_values[17];
		asm_data->u.xmm.XMM_P901[0] = asm_data->u.xmm.XMM_P901[1] = -asm_values[18];
		asm_data->u.xmm.XMM_P924[0] = asm_data->u.xmm.XMM_P924[1] = asm_values[20];
		asm_data->u.xmm.XMM_P383[0] = asm_data->u.xmm.XMM_P383[1] = asm_values[21];
		asm_data->u.xmm.XMM_P782[0] = asm_data->u.xmm.XMM_P782[1] = asm_values[22];
		asm_data->u.xmm.XMM_P434[0] = asm_data->u.xmm.XMM_P434[1] = asm_values[23];
	    }

	    else {
		asm_data->u.x87.HALF = 0.5;
		asm_data->u.x87.SQRTHALF = sqrt (0.5);
		asm_data->u.x87.P25 = 0.25;
		asm_data->u.x87.P75 = 0.75;
		asm_data->u.x87.P3 = 3.0;

		asm_data->u.x87.TTP_FUDGE[0] = asm_data->u.x87.TTP_FUDGE[1] =
		asm_data->u.x87.TTP_FUDGE[2] = asm_data->u.x87.TTP_FUDGE[3] =
		asm_data->u.x87.TTP_FUDGE[4] = asm_data->u.x87.TTP_FUDGE[5] =
		asm_data->u.x87.TTP_FUDGE[6] = asm_data->u.x87.TTP_FUDGE[7] =
		asm_data->u.x87.TTP_FUDGE[9] = asm_data->u.x87.TTP_FUDGE[11] =
		asm_data->u.x87.TTP_FUDGE[13] = asm_data->u.x87.TTP_FUDGE[15] =
		asm_data->u.x87.TTP_FUDGE[16] = asm_data->u.x87.TTP_FUDGE[18] =
		asm_data->u.x87.TTP_FUDGE[20] = asm_data->u.x87.TTP_FUDGE[22] = 1.0;
		asm_data->u.x87.TTP_FUDGE[8] = asm_data->u.x87.TTP_FUDGE[10] =
		asm_data->u.x87.TTP_FUDGE[12] = asm_data->u.x87.TTP_FUDGE[14] =
		asm_data->u.x87.TTP_FUDGE[17] = asm_data->u.x87.TTP_FUDGE[19] =
		asm_data->u.x87.TTP_FUDGE[21] = asm_data->u.x87.TTP_FUDGE[23] =
		asm_data->u.x87.TTP_FUDGE[24] = asm_data->u.x87.TTP_FUDGE[25] =
		asm_data->u.x87.TTP_FUDGE[26] = asm_data->u.x87.TTP_FUDGE[27] =
		asm_data->u.x87.TTP_FUDGE[28] = asm_data->u.x87.TTP_FUDGE[29] =
		asm_data->u.x87.TTP_FUDGE[30] = asm_data->u.x87.TTP_FUDGE[31] = 1.0 / (double) b;

		asm_data->u.x87.TTMP_FUDGE[0] = asm_data->u.x87.TTMP_FUDGE[1] =
		asm_data->u.x87.TTMP_FUDGE[2] = asm_data->u.x87.TTMP_FUDGE[3] =
		asm_data->u.x87.TTMP_FUDGE[4] = asm_data->u.x87.TTMP_FUDGE[5] =
		asm_data->u.x87.TTMP_FUDGE[6] = asm_data->u.x87.TTMP_FUDGE[7] =
		asm_data->u.x87.TTMP_FUDGE[9] = asm_data->u.x87.TTMP_FUDGE[11] =
		asm_data->u.x87.TTMP_FUDGE[13] = asm_data->u.x87.TTMP_FUDGE[15] =
		asm_data->u.x87.TTMP_FUDGE[16] = asm_data->u.x87.TTMP_FUDGE[18] =
		asm_data->u.x87.TTMP_FUDGE[20] = asm_data->u.x87.TTMP_FUDGE[22] = 1.0;
		asm_data->u.x87.TTMP_FUDGE[8] = asm_data->u.x87.TTMP_FUDGE[10] =
		asm_data->u.x87.TTMP_FUDGE[12] = asm_data->u.x87.TTMP_FUDGE[14] =
		asm_data->u.x87.TTMP_FUDGE[17] = asm_data->u.x87.TTMP_FUDGE[19] =
		asm_data->u.x87.TTMP_FUDGE[21] = asm_data->u.x87.TTMP_FUDGE[23] =
		asm_data->u.x87.TTMP_FUDGE[24] = asm_data->u.x87.TTMP_FUDGE[25] =
		asm_data->u.x87.TTMP_FUDGE[26] = asm_data->u.x87.TTMP_FUDGE[27] =
		asm_data->u.x87.TTMP_FUDGE[28] = asm_data->u.x87.TTMP_FUDGE[29] =
		asm_data->u.x87.TTMP_FUDGE[30] = asm_data->u.x87.TTMP_FUDGE[31] = (double) b;

		asm_data->u.x87.BIGVAL = (float) (3.0 * pow (2.0, 62.0));
		asm_data->u.x87.BIGBIGVAL = (float) (big_word * asm_data->u.x87.BIGVAL);

		asm_data->u.x87.MINUS_C = (double) -c;

		asm_data->u.x87.NORM012_FF =
			(gwdata->ZERO_PADDED_FFT) ?
				(double) (gwdata->FFTLEN / 2) :
				(double) (gwdata->FFTLEN / 2) / k;

		asm_data->u.x87.K_HI = floor (k / big_word) * big_word;
		asm_data->u.x87.K_LO = k - asm_data->u.x87.K_HI;
		asm_data->u.x87.K_HI_2 = floor (k / big_word / big_word) * big_word * big_word;
		asm_data->u.x87.K_HI_1 = asm_data->u.x87.K_HI - asm_data->u.x87.K_HI_2;
		gwsetmulbyconst (gwdata, gwdata->maxmulbyconst);

		asm_data->u.x87.LIMIT_INVERSE[0] =
		asm_data->u.x87.LIMIT_INVERSE[1] =
		asm_data->u.x87.LIMIT_INVERSE[3] =
		asm_data->u.x87.LIMIT_INVERSE[4] = 1.0 / small_word;	
		asm_data->u.x87.LIMIT_INVERSE[2] =
		asm_data->u.x87.LIMIT_INVERSE[5] =
		asm_data->u.x87.LIMIT_INVERSE[6] =
		asm_data->u.x87.LIMIT_INVERSE[7] = 1.0 / big_word;	

		temp = small_word * asm_data->u.x87.BIGVAL - asm_data->u.x87.BIGVAL;
		asm_data->u.x87.LIMIT_BIGMAX[0] =
		asm_data->u.x87.LIMIT_BIGMAX[1] =
		asm_data->u.x87.LIMIT_BIGMAX[3] =
		asm_data->u.x87.LIMIT_BIGMAX[4] = temp;

		asm_data->u.x87.LIMIT_BIGMAX_NEG[0] =
		asm_data->u.x87.LIMIT_BIGMAX_NEG[1] =
		asm_data->u.x87.LIMIT_BIGMAX_NEG[3] =
		asm_data->u.x87.LIMIT_BIGMAX_NEG[4] = -temp;	

		temp = big_word * asm_data->u.x87.BIGVAL - asm_data->u.x87.BIGVAL;
		asm_data->u.x87.LIMIT_BIGMAX[2] =
		asm_data->u.x87.LIMIT_BIGMAX[5] =
		asm_data->u.x87.LIMIT_BIGMAX[6] =
		asm_data->u.x87.LIMIT_BIGMAX[7] = temp;

		asm_data->u.x87.LIMIT_BIGMAX_NEG[2] =
		asm_data->u.x87.LIMIT_BIGMAX_NEG[5] =
		asm_data->u.x87.LIMIT_BIGMAX_NEG[6] =
		asm_data->u.x87.LIMIT_BIGMAX_NEG[7] = -temp;	

		memcpy (asm_data->u.x87.LIMIT_INVERSE+8, asm_data->u.x87.LIMIT_INVERSE, 8 * sizeof (double));
		memcpy (asm_data->u.x87.LIMIT_INVERSE+16, asm_data->u.x87.LIMIT_INVERSE, 8 * sizeof (double));
		memcpy (asm_data->u.x87.LIMIT_INVERSE+24, asm_data->u.x87.LIMIT_INVERSE, 8 * sizeof (double));
		memcpy (asm_data->u.x87.LIMIT_BIGMAX+8, asm_data->u.x87.LIMIT_BIGMAX, 8 * sizeof (double));
		memcpy (asm_data->u.x87.LIMIT_BIGMAX+16, asm_data->u.x87.LIMIT_BIGMAX, 8 * sizeof (double));
		memcpy (asm_data->u.x87.LIMIT_BIGMAX+24, asm_data->u.x87.LIMIT_BIGMAX, 8 * sizeof (double));
		memcpy (asm_data->u.x87.LIMIT_BIGMAX_NEG+8, asm_data->u.x87.LIMIT_BIGMAX_NEG, 8 * sizeof (double));
		memcpy (asm_data->u.x87.LIMIT_BIGMAX_NEG+16, asm_data->u.x87.LIMIT_BIGMAX_NEG, 8 * sizeof (double));
		memcpy (asm_data->u.x87.LIMIT_BIGMAX_NEG+24, asm_data->u.x87.LIMIT_BIGMAX_NEG, 8 * sizeof (double));

		asm_data->u.x87.P951 = asm_values[0];
		asm_data->u.x87.P618 = asm_values[1];
		asm_data->u.x87.P309 = asm_values[2];
		asm_data->u.x87.M262 = asm_values[3];
		asm_data->u.x87.P588 = asm_values[4];
		asm_data->u.x87.M162 = asm_values[5];
		asm_data->u.x87.M809 = asm_values[6];
		asm_data->u.x87.M382 = asm_values[7];
		asm_data->u.x87.P866 = asm_values[8];
		asm_data->u.x87.P433 = asm_values[9];
		asm_data->u.x87.P577 = asm_values[10];
		asm_data->u.x87.P975 = asm_values[11];
		asm_data->u.x87.P445 = asm_values[12];
		asm_data->u.x87.P180 = asm_values[13];
		asm_data->u.x87.P623 = asm_values[14];
		asm_data->u.x87.M358 = asm_values[15];
		asm_data->u.x87.P404 = asm_values[16];
		asm_data->u.x87.M223 = asm_values[17];
		asm_data->u.x87.M901 = asm_values[18];
		asm_data->u.x87.M691 = asm_values[19];
	    }
	}

	if (gwdata->cpu_flags & CPU_AVX512F) {

		if (gwdata->PASS1_SIZE == 0) {
			asm_data->fourKBgapsize = gwdata->FOURKBGAPSIZE;
			asm_data->normblkdst = 0;		
			asm_data->normblkdst4 = gwdata->FOURKBGAPSIZE;
			asm_data->normblkdst8 = 0;
			asm_data->pass1blkdst = 8*128;		
			asm_data->pass2blkdst = 128;		
			asm_data->normval4 = 1;			
			asm_data->normval2 = 1;			
			asm_data->normval1 = 1;			
			asm_data->pass2gapsize = 0;		
		}

		else {
			int	pass1_size;

			asm_data->fourKBgapsize = gwdata->FOURKBGAPSIZE;
			asm_data->pass2gapsize = gwdata->PASS2GAPSIZE;
			asm_data->pass2blkdst = gwdata->PASS2_SIZE * 16;
			asm_data->pass2blkdst += (asm_data->pass2blkdst >> 12) * asm_data->fourKBgapsize + asm_data->pass2gapsize;
			asm_data->pass1blkdst = asm_data->pass2blkdst * 8;

			pass1_size = gwdata->PASS1_SIZE;
			if (pass1_size == 192 || pass1_size == 640 || pass1_size == 768 || pass1_size == 896 || pass1_size == 960 ||
			    pass1_size == 1152 || pass1_size == 1280 || pass1_size == 1344 || pass1_size == 1536 || pass1_size == 1920 ||
			    pass1_size == 2304) {
				asm_data->normblkdst = 64;		
				asm_data->normblkdst8 = 0;
			} else if (gwdata->PASS1_CACHE_LINES == 8) {	
				asm_data->normblkdst = 0;		
				asm_data->normblkdst8 = 128;		
			} else if (gwdata->PASS1_CACHE_LINES == 16) {	
				asm_data->normblkdst = 0;		
				asm_data->normblkdst8 = 192;		
			} else {					
				asm_data->normblkdst = 64;		
				asm_data->normblkdst8 = -64;		
			}
			asm_data->normblkdst4 = 0;

			if (gwdata->FOURKBGAPSIZE) {
				asm_data->normval4 = (gwdata->PASS2_SIZE * 16) / 4096;
				asm_data->normval1 = 4096 / 128;
			} else {
				asm_data->normval4 = 1;
				asm_data->normval1 = gwdata->PASS2_SIZE * 16 / 128;
			}
			
			asm_data->normval2 = asm_data->normval1 / (asm_data->cache_line_multiplier / 8);
			if (gwdata->ZERO_PADDED_FFT) asm_data->normval3 = gwdata->pass1_var_data_size -  asm_data->cache_line_multiplier / 2;
			else asm_data->normval3 = gwdata->pass1_var_data_size - asm_data->cache_line_multiplier;
		}
	}

	else if (gwdata->cpu_flags & CPU_AVX) {

		if (gwdata->PASS2_SIZE) {
			int	pass1_size;

			asm_data->fourKBgapsize = gwdata->FOURKBGAPSIZE;
			asm_data->pass2gapsize = gwdata->PASS2GAPSIZE;
			asm_data->pass2blkdst = gwdata->PASS2_SIZE * 16;
			asm_data->pass2blkdst += (asm_data->pass2blkdst >> 12) * asm_data->fourKBgapsize + asm_data->pass2gapsize;
			asm_data->pass1blkdst = asm_data->pass2blkdst * 4;

			pass1_size = gwdata->PASS1_SIZE;
			if ((pass1_size == 384 && gwdata->NEGACYCLIC_FFT) ||
			    (pass1_size == 640 && gwdata->NEGACYCLIC_FFT) ||
			    (pass1_size == 1536 && gwdata->NEGACYCLIC_FFT)) {
				asm_data->normblkdst = 64;		
				asm_data->normblkdst8 = 0;
			} else if (gwdata->PASS1_CACHE_LINES == 4) {	
				asm_data->normblkdst = 0;		
				asm_data->normblkdst8 = 64;		
			} else if (gwdata->PASS1_CACHE_LINES == 8) {	
				asm_data->normblkdst = 0;		
				asm_data->normblkdst8 = 192;		
			} else {					
				asm_data->normblkdst = 64;		
				asm_data->normblkdst8 = -64;		
			}

			if (gwdata->FOURKBGAPSIZE) {
				asm_data->normval4 = (gwdata->PASS2_SIZE * 16) >> 12;
				asm_data->normval1 = (4096 / 64) / (asm_data->cache_line_multiplier / 4);
			} else {
				asm_data->normval4 = 1;
				asm_data->normval1 = (gwdata->PASS2_SIZE * 16 / 64) / (asm_data->cache_line_multiplier / 4);
			}

			asm_data->normval2 = asm_data->cache_line_multiplier * 2 * (asm_data->addcount1 - 1);

			asm_data->normval3 = asm_data->cache_line_multiplier * 2;
		}
	}

	else if (gwdata->cpu_flags & CPU_SSE2) {

		asm_data->normval4 = (gwdata->PASS2_SIZE * 16 * 2) >> 13;

		asm_data->pass2gapsize = gwdata->PASS2GAPSIZE;
		asm_data->pass2blkdst = gwdata->PASS2_SIZE * 2 * 8 * 2;
		asm_data->pass2blkdst += (asm_data->pass2blkdst >> 13) * 128 + asm_data->pass2gapsize;
		asm_data->pass1blkdst = asm_data->pass2blkdst;

		if (gwdata->SCRATCH_SIZE == 0) {
			
			asm_data->normblkdst = asm_data->pass1blkdst - asm_data->cache_line_multiplier * 64;
			asm_data->normblkdst8 = 0;
		} else if ((gwdata->FFT_TYPE == FFT_TYPE_RADIX_4 ||
			    gwdata->FFT_TYPE == FFT_TYPE_RADIX_4_DELAYED ||
			    gwdata->FFT_TYPE == FFT_TYPE_RADIX_4_DWPN) &&
			   (gwdata->PASS1_SIZE == 80 ||
			    gwdata->PASS1_SIZE == 96 ||
			    gwdata->PASS1_SIZE == 112 ||
			    gwdata->PASS1_SIZE == 224)) {
			
			asm_data->normblkdst = 0;
			asm_data->normblkdst8 = 0;
		} else {
			
			asm_data->normblkdst = 0;
			asm_data->normblkdst8 = 128;
		}

		if (gwdata->PASS2_SIZE) {
			
			asm_data->normval1 = (8192/64) / asm_data->cache_line_multiplier;

			asm_data->normval2 = asm_data->cache_line_multiplier * 4 * (asm_data->addcount1 - 1);

			asm_data->normval3 = asm_data->cache_line_multiplier * 4 -
					     (asm_data->cache_line_multiplier * 4 +
					      asm_data->normval2) *
					     asm_data->normval1 * asm_data->normval4;

			if (gwdata->FFT_TYPE == FFT_TYPE_RADIX_4_DWPN) {
				asm_data->normval2 /= 2;
				asm_data->normval3 /= 2;
			}
		}
	}

	else {
		if (gwdata->PASS2_SIZE) {
			
			asm_data->normval4 = (gwdata->PASS2_SIZE * 16) >> 12;

			asm_data->pass2blkdst =
		        asm_data->pass1blkdst =
			asm_data->normblkdst = asm_data->normval4 * (4096+64) + 64;
			asm_data->normblkdst8 = 0;

			if (gwdata->SCRATCH_SIZE) {
				
				asm_data->normblkdst = asm_data->cache_line_multiplier * 32;

				if (asm_data->addcount1 >= 128) asm_data->normblkdst8 = 64;
			}

			asm_data->normval1 = (4096/32) / asm_data->cache_line_multiplier;

			asm_data->normval2 = asm_data->cache_line_multiplier * 2 * (asm_data->addcount1 - 1);

			asm_data->normval3 = asm_data->cache_line_multiplier * 2 -
					     ((asm_data->cache_line_multiplier * 2 +
					       asm_data->normval2) *
					      asm_data->normval1 * asm_data->normval4);
		}
	}

	if (!gwdata->information_only) {
	    if (! (gwdata->cpu_flags & (CPU_AVX512F | CPU_AVX))) {
		if (gwdata->ZERO_PADDED_FFT ||
		    3.0 * gwdata->NUM_B_PER_SMALL_WORD * log2 (b) >
				2.0 * ((gwdata->NUM_B_PER_SMALL_WORD + 1) * log2 (b) - 1) +
				0.6 * log2 (gwdata->FFTLEN) + log2 (k) + 1.7 * log2 (labs (c)))
			asm_data->SPREAD_CARRY_OVER_EXTRA_WORDS = FALSE;
		else
			asm_data->SPREAD_CARRY_OVER_EXTRA_WORDS = TRUE;
	    }

	    asm_data->TOP_CARRY_NEEDS_ADJUSTING = (k > 1.0 && !gwdata->ZERO_PADDED_FFT);
	    if (asm_data->TOP_CARRY_NEEDS_ADJUSTING) {
		unsigned long num_b_in_top_word, num_b_in_second_top_word, num_b_in_third_top_word, num_b_in_k;
		double	carry_adjust_1_mod_k;

		asm_data->K = k;
		asm_data->INVERSE_K = 1.0 / k;

		num_b_in_top_word = gwdata->NUM_B_PER_SMALL_WORD;
		if (is_big_word (gwdata, gwdata->FFTLEN-1)) num_b_in_top_word++;
		num_b_in_second_top_word = gwdata->NUM_B_PER_SMALL_WORD;
		if (is_big_word (gwdata, gwdata->FFTLEN-2)) num_b_in_second_top_word++;
		num_b_in_third_top_word = gwdata->NUM_B_PER_SMALL_WORD;
		if (is_big_word (gwdata, gwdata->FFTLEN-3)) num_b_in_third_top_word++;

		num_b_in_k = (unsigned long) ceil (log (k) / log (b));
		asm_data->CARRY_ADJUST1 = pow ((double) b, num_b_in_k);
		carry_adjust_1_mod_k = fltmod (asm_data->CARRY_ADJUST1, k);
		asm_data->CARRY_ADJUST1_HI = floor (carry_adjust_1_mod_k / 131072.0);
		asm_data->CARRY_ADJUST1_LO = carry_adjust_1_mod_k - asm_data->CARRY_ADJUST1_HI * 131072.0;
		asm_data->TWO_TO_17 = 131072.0;
		asm_data->CARRY_ADJUST2 = pow ((double) b, num_b_in_top_word) / asm_data->CARRY_ADJUST1;
		asm_data->CARRY_ADJUST4 = pow ((double) b, num_b_in_second_top_word);
		asm_data->CARRY_ADJUST6 = pow ((double) b, num_b_in_third_top_word);
		if (! (gwdata->cpu_flags & CPU_AVX512F)) {
			if (gwdata->FFT_TYPE == FFT_TYPE_RADIX_4_DWPN) {
				asm_data->CARRY_ADJUST3 = gwfft_partial_weight (gwdata->dd_data, gwdata->FFTLEN-1, dwpn_col (gwdata, gwdata->FFTLEN-1));
				asm_data->CARRY_ADJUST5 = gwfft_partial_weight (gwdata->dd_data, gwdata->FFTLEN-2, dwpn_col (gwdata, gwdata->FFTLEN-2));
				asm_data->CARRY_ADJUST7 = gwfft_partial_weight (gwdata->dd_data, gwdata->FFTLEN-3, dwpn_col (gwdata, gwdata->FFTLEN-3));
			} else {
				asm_data->CARRY_ADJUST3 = gwfft_weight (gwdata->dd_data, gwdata->FFTLEN-1);
				asm_data->CARRY_ADJUST5 = gwfft_weight (gwdata->dd_data, gwdata->FFTLEN-2);
				asm_data->CARRY_ADJUST7 = gwfft_weight (gwdata->dd_data, gwdata->FFTLEN-3);
			}
		}

		if (! (gwdata->cpu_flags & (CPU_AVX512F | CPU_AVX | CPU_SSE2))) {
			unsigned long kbits, kbits_lo;
			kbits = (unsigned long) ceil (log2 (k));
			kbits_lo = kbits / 2;
			asm_data->u.x87.ALT_K_HI = ((unsigned long) k) & ~((1 << kbits_lo) - 1);
			asm_data->u.x87.ALT_K_LO = ((unsigned long) k) &  ((1 << kbits_lo) - 1);
			asm_data->CARRY_ADJUST1_HI = ((unsigned long) asm_data->CARRY_ADJUST1) & ~((1 << kbits_lo) - 1);
			asm_data->CARRY_ADJUST1_LO = ((unsigned long) asm_data->CARRY_ADJUST1) &  ((1 << kbits_lo) - 1);
			if (gwdata->PASS2_SIZE)
				asm_data->CARRY_ADJUST4 *= asm_data->CARRY_ADJUST5;
			else
				asm_data->CARRY_ADJUST6 *= asm_data->CARRY_ADJUST7;
		}

		ASSERTG ((gwdata->PASS2_SIZE &&
			  num_b_in_k <= num_b_in_top_word + num_b_in_second_top_word) ||
			 (!gwdata->PASS2_SIZE &&
			  num_b_in_k <= num_b_in_top_word + num_b_in_second_top_word + num_b_in_third_top_word));
		if (!	((gwdata->PASS2_SIZE &&
			  num_b_in_k <= num_b_in_top_word + num_b_in_second_top_word) ||
			 (!gwdata->PASS2_SIZE &&
			  num_b_in_k <= num_b_in_top_word + num_b_in_second_top_word + num_b_in_third_top_word))) return (GWERROR_INTERNAL + 8);

		asm_data->HIGH_WORD1_OFFSET = addr_offset (gwdata, gwdata->FFTLEN-1);
		asm_data->HIGH_WORD2_OFFSET = addr_offset (gwdata, gwdata->FFTLEN-2);
		asm_data->HIGH_WORD3_OFFSET = addr_offset (gwdata, gwdata->FFTLEN-3);

		raw_gwsetaddin (gwdata, gwdata->FFTLEN-1, NULL, 0.0);
		asm_data->HIGH_SCRATCH1_OFFSET = asm_data->ADDIN_OFFSET;
		raw_gwsetaddin (gwdata, gwdata->FFTLEN-2, NULL, 0.0);
		asm_data->HIGH_SCRATCH2_OFFSET = asm_data->ADDIN_OFFSET;
		raw_gwsetaddin (gwdata, gwdata->FFTLEN-3, NULL, 0.0);
		asm_data->HIGH_SCRATCH3_OFFSET = asm_data->ADDIN_OFFSET;
	    }

	    if (gwdata->ZERO_PADDED_FFT) {
		unsigned long num_b_in_k, num_b_in_word_0, num_b_in_word_1;
		unsigned long num_b_in_word_2, num_b_in_word_3, num_b_in_word_4, num_b_in_word_5;

		asm_data->ZPAD_LSW_ADJUST = (double) (c < 0 ? c + 1 : c - 1);

		if (gwdata->cpu_flags & CPU_SSE2 && ! (gwdata->cpu_flags & (CPU_AVX512F | CPU_AVX))) {
			asm_data->u.xmm.ZPAD_WORD5_OFFSET = addr_offset (gwdata, 4);
			if (asm_data->u.xmm.ZPAD_WORD5_OFFSET == 8) {  
				asm_data->u.xmm.ZPAD_WORD5_RBP_OFFSET = 8;
			} else {
				asm_data->u.xmm.ZPAD_WORD5_RBP_OFFSET = 256;
			}
		}

		asm_data->HIGH_WORD1_OFFSET = addr_offset (gwdata, gwdata->FFTLEN/2-1);
		asm_data->HIGH_WORD2_OFFSET = addr_offset (gwdata, gwdata->FFTLEN/2-2);
		asm_data->HIGH_WORD3_OFFSET = addr_offset (gwdata, gwdata->FFTLEN/2-3);

		raw_gwsetaddin (gwdata, gwdata->FFTLEN/2-1, NULL, 0.0);
		asm_data->HIGH_SCRATCH1_OFFSET = asm_data->ADDIN_OFFSET;
		raw_gwsetaddin (gwdata, gwdata->FFTLEN/2-2, NULL, 0.0);
		asm_data->HIGH_SCRATCH2_OFFSET = asm_data->ADDIN_OFFSET;
		raw_gwsetaddin (gwdata, gwdata->FFTLEN/2-3, NULL, 0.0);
		asm_data->HIGH_SCRATCH3_OFFSET = asm_data->ADDIN_OFFSET;

		num_b_in_k = (unsigned long) ceil (log (k) / log (b));
		num_b_in_word_0 = gwdata->NUM_B_PER_SMALL_WORD; if (is_big_word (gwdata, 0)) num_b_in_word_0++;
		num_b_in_word_1 = gwdata->NUM_B_PER_SMALL_WORD; if (is_big_word (gwdata, 1)) num_b_in_word_1++;
		num_b_in_word_2 = gwdata->NUM_B_PER_SMALL_WORD; if (is_big_word (gwdata, 2)) num_b_in_word_2++;
		num_b_in_word_3 = gwdata->NUM_B_PER_SMALL_WORD; if (is_big_word (gwdata, 3)) num_b_in_word_3++;
		num_b_in_word_4 = gwdata->NUM_B_PER_SMALL_WORD; if (is_big_word (gwdata, 4)) num_b_in_word_4++;
		num_b_in_word_5 = gwdata->NUM_B_PER_SMALL_WORD; if (is_big_word (gwdata, 5)) num_b_in_word_5++;

		asm_data->ZPAD_SHIFT1 = pow ((double) b, (int) num_b_in_word_0);
		asm_data->ZPAD_SHIFT2 = pow ((double) b, (int) num_b_in_word_1);
		asm_data->ZPAD_SHIFT3 = pow ((double) b, (int) num_b_in_word_2);
		asm_data->ZPAD_SHIFT4 = pow ((double) b, (int) num_b_in_word_3);
		asm_data->ZPAD_SHIFT5 = pow ((double) b, (int) num_b_in_word_4);
		asm_data->ZPAD_SHIFT6 = pow ((double) b, (int) num_b_in_word_5);

		if (num_b_in_k <= num_b_in_word_0) asm_data->ZPAD_TYPE = 1;
		else if (num_b_in_k <= num_b_in_word_0 + num_b_in_word_1) asm_data->ZPAD_TYPE = 2;
		else asm_data->ZPAD_TYPE = 3;

		if (asm_data->ZPAD_TYPE == 1) {
			asm_data->ZPAD_K1_LO = k;
			asm_data->ZPAD_INVERSE_K1 = 1.0 / k;
		}

		if (asm_data->ZPAD_TYPE == 2) {
			asm_data->ZPAD_K1_HI = floor (k / asm_data->ZPAD_SHIFT1);
			asm_data->ZPAD_K1_LO = k - asm_data->ZPAD_K1_HI * asm_data->ZPAD_SHIFT1;
			asm_data->ZPAD_INVERSE_K1 = asm_data->ZPAD_SHIFT1 / k;
			asm_data->ZPAD_K2_HI = floor (k / asm_data->ZPAD_SHIFT2);
			asm_data->ZPAD_K2_LO = k - asm_data->ZPAD_K2_HI * asm_data->ZPAD_SHIFT2;
			asm_data->ZPAD_INVERSE_K2 = asm_data->ZPAD_SHIFT2 / k;
			asm_data->ZPAD_K3_HI = floor (k / asm_data->ZPAD_SHIFT3);
			asm_data->ZPAD_K3_LO = k - asm_data->ZPAD_K3_HI * asm_data->ZPAD_SHIFT3;
			asm_data->ZPAD_INVERSE_K3 = asm_data->ZPAD_SHIFT3 / k;
			asm_data->ZPAD_K4_HI = floor (k / asm_data->ZPAD_SHIFT4);
			asm_data->ZPAD_K4_LO = k - asm_data->ZPAD_K4_HI * asm_data->ZPAD_SHIFT4;
			asm_data->ZPAD_INVERSE_K4 = asm_data->ZPAD_SHIFT4 / k;
			asm_data->ZPAD_K5_HI = floor (k / asm_data->ZPAD_SHIFT5);
			asm_data->ZPAD_K5_LO = k - asm_data->ZPAD_K5_HI * asm_data->ZPAD_SHIFT5;
			asm_data->ZPAD_INVERSE_K5 = asm_data->ZPAD_SHIFT5 / k;
			asm_data->ZPAD_K6_HI = floor (k / asm_data->ZPAD_SHIFT6);
			asm_data->ZPAD_K6_LO = k - asm_data->ZPAD_K6_HI * asm_data->ZPAD_SHIFT6;
			asm_data->ZPAD_INVERSE_K6 = asm_data->ZPAD_SHIFT6 / k;
		}

		if (asm_data->ZPAD_TYPE == 3) {
			double	powb, bigpowb;
			powb = pow ((double) b, (int) num_b_in_word_0);
			bigpowb = pow ((double) b, (int) (num_b_in_word_0 + num_b_in_word_1));
			asm_data->ZPAD_K2_HI = floor (k / bigpowb);
			asm_data->ZPAD_K2_MID = floor ((k - asm_data->ZPAD_K2_HI*bigpowb) / powb);
			asm_data->ZPAD_K2_LO = k - asm_data->ZPAD_K2_HI*bigpowb - asm_data->ZPAD_K2_MID*powb;
			asm_data->ZPAD_INVERSE_K2 = powb / k;
			powb = pow ((double) b, (int) num_b_in_word_1);
			bigpowb = pow ((double) b, (int) (num_b_in_word_1 + num_b_in_word_2));
			asm_data->ZPAD_K3_HI = floor (k / bigpowb);
			asm_data->ZPAD_K3_MID = floor ((k - asm_data->ZPAD_K3_HI*bigpowb) / powb);
			asm_data->ZPAD_K3_LO = k - asm_data->ZPAD_K3_HI*bigpowb - asm_data->ZPAD_K3_MID*powb;
			asm_data->ZPAD_INVERSE_K3 = powb / k;
			powb = pow ((double) b, (int) num_b_in_word_2);
			bigpowb = pow ((double) b, (int) (num_b_in_word_2 + num_b_in_word_3));
			asm_data->ZPAD_K4_HI = floor (k / bigpowb);
			asm_data->ZPAD_K4_MID = floor ((k - asm_data->ZPAD_K4_HI*bigpowb) / powb);
			asm_data->ZPAD_K4_LO = k - asm_data->ZPAD_K4_HI*bigpowb - asm_data->ZPAD_K4_MID*powb;
			asm_data->ZPAD_INVERSE_K4 = powb / k;
			powb = pow ((double) b, (int) num_b_in_word_3);
			bigpowb = pow ((double) b, (int) (num_b_in_word_3 + num_b_in_word_4));
			asm_data->ZPAD_K5_HI = floor (k / bigpowb);
			asm_data->ZPAD_K5_MID = floor ((k - asm_data->ZPAD_K5_HI*bigpowb) / powb);
			asm_data->ZPAD_K5_LO = k - asm_data->ZPAD_K5_HI*bigpowb - asm_data->ZPAD_K5_MID*powb;
			asm_data->ZPAD_INVERSE_K5 = powb / k;
			powb = pow ((double) b, (int) num_b_in_word_4);
			bigpowb = pow ((double) b, (int) (num_b_in_word_4 + num_b_in_word_5));
			asm_data->ZPAD_K6_HI = floor (k / bigpowb);
			asm_data->ZPAD_K6_MID = floor ((k - asm_data->ZPAD_K6_HI*bigpowb) / powb);
			asm_data->ZPAD_K6_LO = k - asm_data->ZPAD_K6_HI*bigpowb - asm_data->ZPAD_K6_MID*powb;
			asm_data->ZPAD_INVERSE_K6 = bigpowb / k;
		}

		if (gwdata->cpu_flags & CPU_AVX512F) {
			int	i;
			for (i = 0; i < 7; i++) {
				
				gwdata->ZPAD_COPY7_ADJUST[i] = gwfft_weight (gwdata->dd_data, gwdata->FFTLEN / 2 - 3 + i);
				
				gwdata->ZPAD_0_6_ADJUST[i] = gwfft_weight_inverse (gwdata->dd_data, i);
			}
		} else if (gwdata->FFT_TYPE == FFT_TYPE_RADIX_4_DWPN) {
			int	i;
			for (i = 0; i < 7; i++) {
				
				gwdata->ZPAD_COPY7_ADJUST[i] = gwfft_weight (gwdata->dd_data, dwpn_col (gwdata, gwdata->FFTLEN / 2 - 3 + i));
				
				gwdata->ZPAD_0_6_ADJUST[i] = gwfft_weight_inverse (gwdata->dd_data, dwpn_col (gwdata, i));
			}
		} else {
			int	i;
			for (i = 0; i < 7; i++) gwdata->ZPAD_COPY7_ADJUST[i] = 1.0;
		}
	    }

#ifdef X86_64
	    if (gwdata->cpu_flags & CPU_AVX512F) {
		int	index;
		index = 0;
		if (gwdata->ZERO_PADDED_FFT) index += 2;
		if (gwdata->RATIONAL_FFT) index += 1;
		asm_data->u.zmm.ZMM_CARRIES_ROUTINE = avx512_carries_prctab[index]; 
		asm_data->u.zmm.ZMM_OP_CARRIES_ROUTINE = avx512_carries_prctab[gwdata->ZERO_PADDED_FFT ? index + 2 : index]; 
		index = 4 + 4 * index;
		gwdata->GWPROCPTRS[1] = avx512_aux_prctab[index];	
		gwdata->GWPROCPTRS[2] = avx512_aux_prctab[0];		
		gwdata->GWPROCPTRS[3] = avx512_aux_prctab[index+1];	
		gwdata->GWPROCPTRS[4] = avx512_aux_prctab[1];		
		gwdata->GWPROCPTRS[5] = avx512_aux_prctab[index+2];	
		gwdata->GWPROCPTRS[6] = avx512_aux_prctab[2];		
		gwdata->GWPROCPTRS[7] = avx512_aux_prctab[3];		
		gwdata->GWPROCPTRS[8] = avx512_aux_prctab[index+3];	
		gwdata->GWPROCPTRS[norm_routines] = avx512_prctab[avx512_prctab_index (gwdata, 0, 0, 0)];  
		gwdata->GWPROCPTRS[norm_routines+1] = avx512_prctab[avx512_prctab_index (gwdata, 0, 1, 0)];  
		gwdata->GWPROCPTRS[norm_routines+2] = avx512_prctab[avx512_prctab_index (gwdata, 0, 0, 1)];  
		gwdata->GWPROCPTRS[norm_routines+3] = avx512_prctab[avx512_prctab_index (gwdata, 0, 1, 1)];  
		gwdata->GWPROCPTRS[zerohigh_routines] = avx512_prctab[avx512_prctab_index (gwdata, 1, 0, 0)];  
		gwdata->GWPROCPTRS[zerohigh_routines+1] = avx512_prctab[avx512_prctab_index (gwdata, 1, 1, 0)];  
	    } else
#endif
	    if (gwdata->cpu_flags & CPU_AVX) {
		int	index, aux_base;
		index = 0;
		if (b != 2) index += 4;
		if (gwdata->ZERO_PADDED_FFT) index += 2;
		if (gwdata->RATIONAL_FFT) index += 1;
		asm_data->u.ymm.YMM_CARRIES_ROUTINE = avx_carries_prctab[index]; 
		aux_base = (gwdata->PASS2_SIZE == 0) ? 0 : 36;
		index = aux_base + 4 + 4 * index;
		gwdata->GWPROCPTRS[1] = avx_aux_prctab[index];		
		gwdata->GWPROCPTRS[2] = avx_aux_prctab[aux_base];	
		gwdata->GWPROCPTRS[3] = avx_aux_prctab[index+1];	
		gwdata->GWPROCPTRS[4] = avx_aux_prctab[aux_base+1];	
		gwdata->GWPROCPTRS[5] = avx_aux_prctab[index+2];	
		gwdata->GWPROCPTRS[6] = avx_aux_prctab[aux_base+2];	
		gwdata->GWPROCPTRS[7] = avx_aux_prctab[aux_base+3];	
		gwdata->GWPROCPTRS[8] = avx_aux_prctab[index+3];	
		gwdata->GWPROCPTRS[norm_routines] = avx_prctab[avx_prctab_index (gwdata, 0, 0, 0)];  
		gwdata->GWPROCPTRS[norm_routines+1] = avx_prctab[avx_prctab_index (gwdata, 0, 1, 0)];  
		gwdata->GWPROCPTRS[norm_routines+2] = avx_prctab[avx_prctab_index (gwdata, 0, 0, 1)];  
		gwdata->GWPROCPTRS[norm_routines+3] = avx_prctab[avx_prctab_index (gwdata, 0, 1, 1)];  
		gwdata->GWPROCPTRS[zerohigh_routines] = avx_prctab[avx_prctab_index (gwdata, 1, 0, 0)];  
		gwdata->GWPROCPTRS[zerohigh_routines+1] = avx_prctab[avx_prctab_index (gwdata, 1, 1, 0)];  
	    }
	    else if (gwdata->cpu_flags & CPU_SSE2) {
		memcpy (gwdata->GWPROCPTRS+1, &sse2_aux_prctab[gwdata->FFT_TYPE == FFT_TYPE_RADIX_4_DWPN ? 16 : gwdata->PASS2_SIZE ? 8 : 0], 8 * sizeof (void *));
		gwdata->GWPROCPTRS[norm_routines] = sse2_prctab[sse2_prctab_index (gwdata, 0, 0, 0)];  
		gwdata->GWPROCPTRS[norm_routines+1] = sse2_prctab[sse2_prctab_index (gwdata, 0, 1, 0)];  
		gwdata->GWPROCPTRS[norm_routines+2] = sse2_prctab[sse2_prctab_index (gwdata, 0, 0, 1)];  
		gwdata->GWPROCPTRS[norm_routines+3] = sse2_prctab[sse2_prctab_index (gwdata, 0, 1, 1)];  
		gwdata->GWPROCPTRS[zerohigh_routines] = sse2_prctab[sse2_prctab_index (gwdata, 1, 0, 0)];  
		gwdata->GWPROCPTRS[zerohigh_routines+1] = sse2_prctab[sse2_prctab_index (gwdata, 1, 1, 0)];  
	    }
#ifndef X86_64
	    else {
		memcpy (gwdata->GWPROCPTRS+1, &x87_aux_prctab[gwdata->PASS2_SIZE ? 8 : 0], 8 * sizeof (void *));
		gwdata->GWPROCPTRS[norm_routines] = x87_prctab[x87_prctab_index (gwdata, 0, 0, 0)];  
		gwdata->GWPROCPTRS[norm_routines+1] = x87_prctab[x87_prctab_index (gwdata, 0, 1, 0)];  
		gwdata->GWPROCPTRS[norm_routines+2] = x87_prctab[x87_prctab_index (gwdata, 0, 0, 1)];  
		gwdata->GWPROCPTRS[norm_routines+3] = x87_prctab[x87_prctab_index (gwdata, 0, 1, 1)];  
		gwdata->GWPROCPTRS[zerohigh_routines] = x87_prctab[x87_prctab_index (gwdata, 1, 0, 0)];  
		gwdata->GWPROCPTRS[zerohigh_routines+1] = x87_prctab[x87_prctab_index (gwdata, 1, 1, 0)];  
	    }
#endif

	    gwsetnormroutine (gwdata, 0, 0, 0);
	    gwstartnextfft (gwdata, 0);
	    gwdata->asm_addin_value = 0.0;
	    gwdata->asm_postaddin_value = 0.0;
	    if (gwdata->careful_count == -1) gwset_carefully_count (gwdata, -1);
	}

	asm_data->MAXERR = 0.0;
	gwdata->saved_copyz_n = 0;
	gwdata->GWERROR = 0;
	gwdata->GW_RANDOM = NULL;
	gwdata->GW_RANDOM_SQUARED = NULL;
	gwdata->GW_RANDOM_FFT = NULL;
	gwdata->GW_ADDIN = NULL;
	gwdata->GW_POSTADDIN = NULL;

	gwdata->GW_FFT1 = NULL;
	gwdata->FFT1_state = 0;
	gwdata->FFT1_user_allocated = 0;
#ifdef X86_64
	if (gwdata->cpu_flags & (CPU_AVX512F | CPU_AVX)) { if (gwdata->k == 1.0 || gwdata->GENERAL_MMGW_MOD) gwdata->FFT1_state = 2; }
#else
	gwdata->FFT1_state = 2;
#endif

	gwdata->cyclic_gwdata = NULL;
	gwdata->negacyclic_gwdata = NULL;

	if (!gwdata->information_only) {
		atomic_set (gwdata->clone_count, 0);
		gwdata->fft_count = 0;
		gwdata->read_count = gwdata->write_count = 0;
		asm_data->ASM_TIMERS = (uint32_t *) &gwdata->ASM_TIMERS;

		gwdata->gwnum_alloc = (gwnum *) malloc (50 * sizeof (gwnum));
		if (gwdata->gwnum_alloc == NULL) { gwdone (gwdata); return (GWERROR_MALLOC); }
		gwdata->gwnum_free = (gwnum *) malloc (50 * sizeof (gwnum));
		if (gwdata->gwnum_free == NULL) { gwdone (gwdata); return (GWERROR_MALLOC); }
		gwdata->gwnum_alloc_count = 0;
		gwdata->gwnum_alloc_array_size = 50;
		gwdata->gwnum_free_count = 0;
		gwmutex_init (&gwdata->alloc_lock);

		if (gwdata->large_pages_gwnum != NULL) gwfree (gwdata, gwalloc (gwdata));

		gwdata->gdata.blksize = gwnum_datasize (gwdata);

		int retcode = multithread_init (gwdata);
		if (retcode) { gwdone (gwdata); return (retcode); }
	}

#ifdef GDEBUG
	if (!gwdata->information_only) {
	gwiter iter;
	gwnum g = gwalloc (gwdata);
	int32_t val, val2;
	gwiter_init_write_only (gwdata, &iter, g);
	for (unsigned long i = 0; i < gwdata->FFTLEN; i++, gwiter_next (&iter)) {
		ASSERTG (gwiter_index (&iter) == i);
		ASSERTG (gwiter_addr_offset (&iter) == addr_offset (gwdata, i));
		ASSERTG (gwiter_addr (&iter) == addr (gwdata, g, i));
		ASSERTG (gwiter_is_big_word (&iter) == is_big_word (gwdata, i));
		set_fft_value (gwdata, g, i, 987654321);
		gwiter_get_fft_value (&iter, &val);
		ASSERTG (val == 987654321);
		gwiter_set_fft_value (&iter, -123456789);
		gwiter_get_fft_value (&iter, &val2);
		ASSERTG (val2 == -123456789);
	}
	unsigned long rand_start = ((rand () << 16) + rand ()) % gwdata->FFTLEN;
	gwiter_init (gwdata, &iter, g, rand_start);
	for (unsigned long i = rand_start; i < gwdata->FFTLEN; i++, gwiter_next (&iter)) {
		ASSERTG (gwiter_index (&iter) == i);
		ASSERTG (gwiter_addr_offset (&iter) == addr_offset (gwdata, i));
		ASSERTG (gwiter_addr (&iter) == addr (gwdata, g, i));
		ASSERTG (gwiter_is_big_word (&iter) == is_big_word (gwdata, i));
		set_fft_value (gwdata, g, i, 897654321);
		gwiter_get_fft_value (&iter, &val);
		ASSERTG (val == 897654321);
		gwiter_set_fft_value (&iter, -213456789);
		gwiter_get_fft_value (&iter, &val2);
		ASSERTG (val2 == -213456789);
	}
	gwfree (gwdata, g);
	}
#endif

	return (0);
}

void xcopy_7_words (
	struct gwasm_data *asm_data)
{
	gwhandle *gwdata;
	double	*srcarg, *destarg;

	gwdata = asm_data->gwdata;
	destarg = (double *) asm_data->DESTARG;
	srcarg = (double *) ((char *) destarg + asm_data->DIST_TO_FFTSRCARG);
	if (gwdata->cpu_flags & CPU_AVX512F) {
		char	*srcp;
		srcp = (char *) srcarg + 64;
		destarg[-8] = * (double *) srcp * gwdata->ZPAD_COPY7_ADJUST[3];	
		srcp += asm_data->u.zmm.ZMM_SRC_INCR;
		destarg[-7] = * (double *) srcp * gwdata->ZPAD_COPY7_ADJUST[4]; 
		srcp += asm_data->u.zmm.ZMM_SRC_INCR;
		destarg[-6] = * (double *) srcp * gwdata->ZPAD_COPY7_ADJUST[5]; 
		srcp += asm_data->u.zmm.ZMM_SRC_INCR;
		destarg[-5] = * (double *) srcp * gwdata->ZPAD_COPY7_ADJUST[6]; 
	}
	else if (gwdata->cpu_flags & CPU_AVX) {
		char	*srcp;
		srcp = (char *) srcarg + 32;
		destarg[-8] = * (double *) srcp * gwdata->ZPAD_COPY7_ADJUST[3];	
		srcp += asm_data->u.ymm.YMM_SRC_INCR1;
		destarg[-7] = * (double *) srcp * gwdata->ZPAD_COPY7_ADJUST[4]; 
		srcp += asm_data->u.ymm.YMM_SRC_INCR2;
		destarg[-6] = * (double *) srcp * gwdata->ZPAD_COPY7_ADJUST[5]; 
		srcp += asm_data->u.ymm.YMM_SRC_INCR3;
		destarg[-5] = * (double *) srcp * gwdata->ZPAD_COPY7_ADJUST[6]; 
	} else {
		destarg[-8] = srcarg[4] * gwdata->ZPAD_COPY7_ADJUST[3];	
		destarg[-7] = srcarg[12] * gwdata->ZPAD_COPY7_ADJUST[4]; 
		destarg[-6] = srcarg[20] * gwdata->ZPAD_COPY7_ADJUST[5]; 
		destarg[-5] = srcarg[28] * gwdata->ZPAD_COPY7_ADJUST[6]; 
	}
	destarg[-9] = * (double *)  ((char *) srcarg + asm_data->HIGH_WORD1_OFFSET) * gwdata->ZPAD_COPY7_ADJUST[2];	
	destarg[-10] = * (double *) ((char *) srcarg + asm_data->HIGH_WORD2_OFFSET) * gwdata->ZPAD_COPY7_ADJUST[1];	
	destarg[-11] = * (double *) ((char *) srcarg + asm_data->HIGH_WORD3_OFFSET) * gwdata->ZPAD_COPY7_ADJUST[0];	
}

void xcopy_4_words (
	struct gwasm_data *asm_data)
{
	gwhandle *gwdata;
	double	*srcarg;

	gwdata = asm_data->gwdata;
	srcarg = (double *) asm_data->DESTARG;
	if (gwdata->cpu_flags & CPU_AVX512F) {
		char	*srcp;
		srcp = (char *) srcarg + 64;
		srcarg[-8] = * (double *) srcp * gwdata->ZPAD_COPY7_ADJUST[3];	
		srcp += asm_data->u.zmm.ZMM_SRC_INCR;
		srcarg[-7] = * (double *) srcp * gwdata->ZPAD_COPY7_ADJUST[4]; 
		srcp += asm_data->u.zmm.ZMM_SRC_INCR;
		srcarg[-6] = * (double *) srcp * gwdata->ZPAD_COPY7_ADJUST[5]; 
		srcp += asm_data->u.zmm.ZMM_SRC_INCR;
		srcarg[-5] = * (double *) srcp * gwdata->ZPAD_COPY7_ADJUST[6]; 
	}
	else if (gwdata->cpu_flags & CPU_AVX) {
		char	*srcp;
		srcp = (char *) srcarg + 32;
		srcarg[-8] = * (double *) srcp * gwdata->ZPAD_COPY7_ADJUST[3];	
		srcp += asm_data->u.ymm.YMM_SRC_INCR1;
		srcarg[-7] = * (double *) srcp * gwdata->ZPAD_COPY7_ADJUST[4]; 
		srcp += asm_data->u.ymm.YMM_SRC_INCR2;
		srcarg[-6] = * (double *) srcp * gwdata->ZPAD_COPY7_ADJUST[5]; 
		srcp += asm_data->u.ymm.YMM_SRC_INCR3;
		srcarg[-5] = * (double *) srcp * gwdata->ZPAD_COPY7_ADJUST[6]; 
	} else {
		srcarg[-8] = srcarg[4] * gwdata->ZPAD_COPY7_ADJUST[3]; 
		srcarg[-7] = srcarg[12] * gwdata->ZPAD_COPY7_ADJUST[4]; 
		srcarg[-6] = srcarg[20] * gwdata->ZPAD_COPY7_ADJUST[5]; 
		srcarg[-5] = srcarg[28] * gwdata->ZPAD_COPY7_ADJUST[6]; 
	}
}

void xcopy_3_words (
	struct gwasm_data *asm_data)
{
	gwhandle *gwdata;
	double	*srcarg;

	gwdata = asm_data->gwdata;

	srcarg = (double *) asm_data->DESTARG;
	if (asm_data->this_block == asm_data->last_pass1_block) {
		if (gwdata->SCRATCH_SIZE) {
			srcarg[-9] = * (double *)  ((char *) asm_data->scratch_area + asm_data->HIGH_SCRATCH1_OFFSET) * gwdata->ZPAD_COPY7_ADJUST[2];
			srcarg[-10] = * (double *) ((char *) asm_data->scratch_area + asm_data->HIGH_SCRATCH2_OFFSET) * gwdata->ZPAD_COPY7_ADJUST[1];
		} else {
			srcarg[-9] = * (double *)  ((char *) srcarg + asm_data->HIGH_WORD1_OFFSET) * gwdata->ZPAD_COPY7_ADJUST[2];
			srcarg[-10] = * (double *) ((char *) srcarg + asm_data->HIGH_WORD2_OFFSET) * gwdata->ZPAD_COPY7_ADJUST[1];
		}
	}

	if (asm_data->this_block <= gwdata->num_pass1_blocks - 3 &&
	    asm_data->this_block + asm_data->cache_line_multiplier > gwdata->num_pass1_blocks - 3) {
		if (gwdata->SCRATCH_SIZE) {
			srcarg[-11] = * (double *) ((char *) asm_data->scratch_area + asm_data->HIGH_SCRATCH3_OFFSET) * gwdata->ZPAD_COPY7_ADJUST[0];
		} else {
			srcarg[-11] = * (double *) ((char *) srcarg + asm_data->HIGH_WORD3_OFFSET) * gwdata->ZPAD_COPY7_ADJUST[0];
		}
	}
}

void xcopy_3_words_after_gwcarries (
	struct gwasm_data *asm_data)
{
	gwhandle *gwdata;
	double	*srcarg;

	gwdata = asm_data->gwdata;

	srcarg = (double *) asm_data->DESTARG;
	srcarg[-9] = * (double *)  ((char *) srcarg + asm_data->HIGH_WORD1_OFFSET) * gwdata->ZPAD_COPY7_ADJUST[2];
	srcarg[-10] = * (double *) ((char *) srcarg + asm_data->HIGH_WORD2_OFFSET) * gwdata->ZPAD_COPY7_ADJUST[1];
	srcarg[-11] = * (double *) ((char *) srcarg + asm_data->HIGH_WORD3_OFFSET) * gwdata->ZPAD_COPY7_ADJUST[0];
}

#define	PASS1_STATE_PASS2		999		
#define	PASS1_STATE_MULTITHREAD_OP	4000		

static __inline void *pass1_data_addr (
	gwhandle *gwdata,
	struct gwasm_data *asm_data,
	unsigned long block)
{
	if (gwdata->cpu_flags & CPU_AVX512F) {
		block >>= 3;
		return ((char *) asm_data->DESTARG + (block << 7) + ((block >> 5) * gwdata->FOURKBGAPSIZE));
	} else if (gwdata->cpu_flags & CPU_AVX) {
		block >>= 2;
		return ((char *) asm_data->DESTARG + (block << 6) + ((block >> 6) * gwdata->FOURKBGAPSIZE));
	} else
		return ((char *) asm_data->DESTARG + (block << 6) + ((block >> 7) << 7));
}

static __inline void *pass1_premult_addr (
	gwhandle *gwdata,
	unsigned long block)
{
	if (gwdata->FFT_TYPE == FFT_TYPE_RADIX_4 ||
	    gwdata->FFT_TYPE == FFT_TYPE_RADIX_4_DELAYED ||
	    gwdata->FFT_TYPE == FFT_TYPE_RADIX_4_DWPN)
		return ((char *) gwdata->pass1_var_data + block / gwdata->PASS1_CACHE_LINES * gwdata->pass1_var_data_size);
	return (NULL);
}

static __inline void pass1_state1_norm_addrs (
	gwhandle *gwdata,
	struct gwasm_data *asm_data)
{
	if (gwdata->cpu_flags & CPU_AVX512F) {
		
	} else if (gwdata->cpu_flags & CPU_AVX) {
		asm_data->norm_ptr1 = (char *) asm_data->norm_biglit_array + (asm_data->addcount1 * asm_data->this_block * 2);
	} else {
		if (gwdata->FFT_TYPE == FFT_TYPE_RADIX_4_DWPN)
			asm_data->norm_ptr1 = (char *) asm_data->norm_biglit_array + (asm_data->addcount1 * asm_data->this_block * 2);
		else {
			asm_data->norm_ptr1 = (char *) asm_data->norm_biglit_array + (asm_data->addcount1 * asm_data->this_block * 4);
			asm_data->norm_ptr2 = (char *) asm_data->norm_col_mults + (asm_data->this_block * 32);
		}
	}
}

static __inline void pass1_state1_carry_addrs (
	gwhandle *gwdata,
	struct gwasm_data *asm_data)
{
	if (gwdata->cpu_flags & CPU_AVX512F) {
		
		asm_data->norm_ptr1 = (char *) gwdata->pass1_var_data +
				      gwdata->pass1_var_data_size * asm_data->this_block / gwdata->PASS1_CACHE_LINES + gwdata->biglit_data_offset;
	} else if (gwdata->cpu_flags & CPU_AVX) {
		asm_data->norm_ptr1 = (char *) asm_data->norm_biglit_array + (asm_data->addcount1 * asm_data->this_block * 2);
	} else {
		if (gwdata->FFT_TYPE == FFT_TYPE_RADIX_4_DWPN)
			asm_data->norm_ptr1 = (char *) asm_data->norm_biglit_array + (asm_data->addcount1 * asm_data->this_block * 2);
		else {
			asm_data->norm_ptr1 = (char *) asm_data->norm_biglit_array + (asm_data->addcount1 * asm_data->this_block * 4);
			asm_data->norm_ptr2 = (char *) asm_data->norm_col_mults + (asm_data->this_block * 32);
		}
	}
}

static __inline void *pass2_data_addr (
	gwhandle *gwdata,
	struct gwasm_data *asm_data,
	unsigned long block)
{
	return ((char *) asm_data->DESTARG + block * (unsigned long) asm_data->pass2blkdst);
}

static __inline void * pass2_premult_addr (
	gwhandle *gwdata,
	unsigned long block)
{
	return ((char *) gwdata->adjusted_pass2_premults + block * gwdata->pass2_premult_block_size);
}

static __inline void pass1_state0_assign_first_block (
	gwhandle *gwdata,
	struct gwasm_data *asm_data)
{
	asm_data->this_block = (int) atomic_fetch_addin (gwdata->next_block, asm_data->cache_line_multiplier);
	asm_data->data_addr = pass1_data_addr (gwdata, asm_data, asm_data->this_block);
	asm_data->premult_addr = pass1_premult_addr (gwdata, asm_data->this_block);
}

static __inline void pass1_state0_assign_next_block (
	gwhandle *gwdata,
	struct gwasm_data *asm_data)
{
	int next_block = (int) atomic_fetch_addin (gwdata->next_block, asm_data->cache_line_multiplier);
	if (next_block < (int) gwdata->num_pass1_blocks) {
		asm_data->next_block = next_block;
		
		asm_data->data_prefetch = pass1_data_addr (gwdata, asm_data, asm_data->next_block);
		asm_data->premult_prefetch = pass1_premult_addr (gwdata, asm_data->next_block);
	} else {
		asm_data->next_block = asm_data->this_block;
		asm_data->data_prefetch = asm_data->data_addr;
		asm_data->premult_prefetch = asm_data->premult_addr;
	}
}

void __inline lock_carry_section (gwhandle *gwdata, int i) {
	while (atomic_fetch_incr (gwdata->pass1_carry_sections[i].change_in_progress) != 0) {
		atomic_decr (gwdata->pass1_carry_sections[i].change_in_progress);
		atomic_spinwait (gwdata->pass1_carry_sections[i].change_in_progress, 0);
	}
}

void __inline unlock_carry_section (gwhandle *gwdata, int i) {
	atomic_decr (gwdata->pass1_carry_sections[i].change_in_progress);
}

int pass1_state1_assign_first_block (
	gwhandle *gwdata,
	struct gwasm_data *asm_data)
{
	
	int i = asm_data->thread_num;

	if (gwdata->pass1_carry_sections[i].next_block == gwdata->pass1_carry_sections[i].last_block) {
		unsigned int min_split_size;

		min_split_size = gwdata->num_postfft_blocks;
		if (((gwdata->cpu_flags & CPU_AVX) && gwdata->ZERO_PADDED_FFT) || !(gwdata->cpu_flags & (CPU_AVX512F | CPU_AVX)))
			min_split_size = round_up_to_multiple_of (min_split_size, 8);
		
		min_split_size += asm_data->cache_line_multiplier;
		
		min_split_size += asm_data->cache_line_multiplier;

		for ( ; ; ) {
			unsigned int j, size, target, target_size, new_size;

			target_size = 0;
			for (j = 0; j < gwdata->num_threads; j++) {
				
				if (gwdata->pass1_carry_sections[j].section_state != 1) continue;
				
				size = gwdata->pass1_carry_sections[j].last_block - gwdata->pass1_carry_sections[j].next_block;
				
				if (size > target_size && size >= min_split_size) {
					target = j;
					target_size = size;
					
					if (gwdata->pass1_carry_sections_unallocated > 1 && j == 0) break;
				}
			}

			if (target_size == 0) return (FALSE);

			lock_carry_section (gwdata, target);

			if (gwdata->pass1_carry_sections[target].section_state == 1)
				target_size = gwdata->pass1_carry_sections[target].last_block - gwdata->pass1_carry_sections[target].next_block;
			else
				target_size = 0;
			if (target_size < min_split_size) {
				unlock_carry_section (gwdata, target);
				continue;
			}

			if (gwdata->pass1_carry_sections_unallocated > 1) {
				
				new_size = divide_rounding_up (target_size + asm_data->cache_line_multiplier, gwdata->pass1_carry_sections_unallocated + 1);
				new_size = round_up_to_multiple_of (new_size, asm_data->cache_line_multiplier);
				gwdata->pass1_carry_sections_unallocated--;
			}

			else {
				new_size = (target_size + asm_data->cache_line_multiplier) / 2;
				if (new_size > (unsigned int) (4 * asm_data->cache_line_multiplier))
					new_size = round_up_to_multiple_of (new_size, asm_data->cache_line_multiplier);
				else
					new_size = round_down_to_multiple_of (new_size, asm_data->cache_line_multiplier);
			}

			if (((gwdata->cpu_flags & CPU_AVX512F) && gwdata->ZERO_PADDED_FFT) ||
			    (!(gwdata->cpu_flags & CPU_AVX512F) && (gwdata->cpu_flags & CPU_AVX) && gwdata->ZERO_PADDED_FFT) ||
			    (!(gwdata->cpu_flags & (CPU_AVX512F | CPU_AVX))))
				new_size = round_up_to_multiple_of (new_size, 8);
			if (new_size < gwdata->num_postfft_blocks) new_size = gwdata->num_postfft_blocks;

			new_size = round_up_to_multiple_of (new_size, gwdata->num_postfft_blocks);

			gwdata->pass1_carry_sections[i].start_block =
			gwdata->pass1_carry_sections[i].next_block = gwdata->pass1_carry_sections[target].last_block - new_size;
			gwdata->pass1_carry_sections[i].last_block = gwdata->pass1_carry_sections[target].last_block;
			gwdata->pass1_carry_sections[i].carry_out_section = gwdata->pass1_carry_sections[target].carry_out_section;
			gwdata->pass1_carry_sections[i].carry_in_blocks_finished = FALSE;	

			gwdata->pass1_carry_sections[target].last_block -= new_size;
			gwdata->pass1_carry_sections[target].carry_out_section = i;

			unlock_carry_section (gwdata, target);
			break;
		}
	}

	asm_data->this_block = gwdata->pass1_carry_sections[i].next_block;
	gwdata->pass1_carry_sections[i].next_block += asm_data->cache_line_multiplier;
	gwdata->pass1_carry_sections[i].section_state = 1;

	asm_data->data_addr = pass1_data_addr (gwdata, asm_data, asm_data->this_block);
	asm_data->premult_addr = pass1_premult_addr (gwdata, asm_data->this_block);
	pass1_state1_norm_addrs (gwdata, asm_data);

	if (gwdata->ZERO_PADDED_FFT && ! (gwdata->cpu_flags & (CPU_AVX512F | CPU_AVX))) {
		int	i, carry_table_size;
		double	*table;
		carry_table_size = (gwdata->PASS1_SIZE) << 1;
		table = (double *) asm_data->carries;
		for (i = 0; i < carry_table_size; i += 8, table += 8) {
			table[4] = 0.0;
			table[5] = 0.0;
			table[6] = 0.0;
			table[7] = 0.0;
		}
	}

	return (TRUE);
}

void pass1_state1_assign_next_block (
	gwhandle *gwdata,
	struct gwasm_data *asm_data)
{
	
	int i = asm_data->thread_num;

	asm_data->next_block = gwdata->pass1_carry_sections[i].next_block;
	if (asm_data->next_block == gwdata->pass1_carry_sections[i].last_block && gwdata->pass1_carry_sections[i].section_state == 3)
		asm_data->next_block = asm_data->this_block;
	else if (asm_data->next_block == gwdata->num_pass1_blocks)
		asm_data->next_block = 0;

	asm_data->data_prefetch = pass1_data_addr (gwdata, asm_data, asm_data->next_block);
	asm_data->premult_prefetch = pass1_premult_addr (gwdata, asm_data->next_block);
}

static __inline void pass2_assign_first_block (
	gwhandle *gwdata,
	struct gwasm_data *asm_data)
{
	asm_data->this_block = (int) atomic_fetch_incr (gwdata->next_block);
	asm_data->data_addr = pass2_data_addr (gwdata, asm_data, asm_data->this_block);
	asm_data->premult_addr = pass2_premult_addr (gwdata, asm_data->this_block);
}

static __inline void pass2_assign_next_block (
	gwhandle *gwdata,
	struct gwasm_data *asm_data)
{
	int next_block = (int) atomic_fetch_incr (gwdata->next_block);
	if (next_block < (int) gwdata->num_pass2_blocks) {
		asm_data->next_block = next_block;
		
		asm_data->data_prefetch = pass2_data_addr (gwdata, asm_data, asm_data->next_block);
		asm_data->premult_prefetch = pass2_premult_addr (gwdata, asm_data->next_block);
	} else {
		asm_data->next_block = asm_data->this_block;
		asm_data->data_prefetch = asm_data->data_addr;
		asm_data->premult_prefetch = asm_data->premult_addr;
	}
}

void signal_auxiliary_threads (
	gwhandle *gwdata)
{
	
	gwhandle *parent_gwdata = (gwdata->parent_gwdata != NULL) ? gwdata->parent_gwdata : gwdata;
	parent_gwdata->active_child_gwdata = gwdata;			
	parent_gwdata->all_work_assigned = FALSE;			
	gwevent_signal (&parent_gwdata->work_to_do);			
	if (parent_gwdata->use_spin_wait >= 2) atomic_set (parent_gwdata->alt_work_to_do, 1);
}

void wait_on_auxiliary_threads (
	gwhandle *gwdata)
{
	
	if (gwdata->parent_gwdata != NULL) gwdata = gwdata->parent_gwdata;
	
	gwdata->all_work_assigned = TRUE;
	gwevent_reset (&gwdata->work_to_do);
	if (gwdata->use_spin_wait >= 2) atomic_set (gwdata->alt_work_to_do, 0);
	
	if (gwdata->use_spin_wait) atomic_spinwait (gwdata->num_active_helpers, 0);
	else while (atomic_get (gwdata->num_active_helpers)) {
		gwevent_reset (&gwdata->all_helpers_done);
		if (atomic_get (gwdata->num_active_helpers)) gwevent_wait (&gwdata->all_helpers_done, 0);
	}
}

struct thread_data {
	gwhandle *gwdata;
	int	thread_num;
};

void auxiliary_thread (void *arg)
{
	gwhandle *gwdata;
	int	thread_num;

	struct thread_data *info = (struct thread_data *) arg;
	gwdata = info->gwdata;
	thread_num = info->thread_num;
	free (arg);

	if (gwdata->thread_callback != NULL)
		(*gwdata->thread_callback) (thread_num, 0, gwdata->thread_callback_data);

	for ( ; ; ) {

	    if (gwdata->use_spin_wait > thread_num) atomic_spinwait (gwdata->alt_work_to_do, 1);
	    else gwevent_wait (&gwdata->work_to_do, 0);

	    if (gwdata->helpers_must_exit) break;

	    if (gwdata->all_work_assigned) continue;
	    atomic_incr (gwdata->num_active_helpers);
	    if (!gwdata->all_work_assigned) {			
		struct gwasm_data *asm_data, *main_thread_asm_data;

		gwdata = gwdata->active_child_gwdata;
		main_thread_asm_data = (struct gwasm_data *) gwdata->asm_data;
		asm_data = (struct gwasm_data *) ((char *) gwdata->thread_allocs[thread_num-1] + NEW_STACK_SIZE);

		if (!asm_data->aux_initialized) {
			memcpy (asm_data, main_thread_asm_data, sizeof (struct gwasm_data));

			asm_data->thread_num = thread_num;

			asm_data->scratch_area = align_ptr ((char *) asm_data + sizeof (struct gwasm_data), 128);

			asm_data->carries = align_ptr ((char *) asm_data->scratch_area + gwdata->SCRATCH_SIZE, 128);
			init_asm_data_carries (gwdata, asm_data);

			asm_data->aux_initialized = TRUE;
		}
		    
		if (gwdata->pass1_state == PASS1_STATE_MULTITHREAD_OP) {
			asm_data->DBLARG = main_thread_asm_data->DBLARG;
			do_multithread_op_work (gwdata, asm_data);
			goto aux_out_of_work;
		}

		asm_data->DESTARG = main_thread_asm_data->DESTARG;

		if (gwdata->pass1_state == 0) {
			pass1_state0_assign_first_block (gwdata, asm_data);
			if (asm_data->this_block >= gwdata->num_pass1_blocks) goto aux_out_of_work;
			pass1_state0_assign_next_block (gwdata, asm_data);
		} else if (gwdata->pass1_state == 1) {
			if (! pass1_state1_assign_first_block (gwdata, asm_data)) goto aux_out_of_work;
			pass1_state1_assign_next_block (gwdata, asm_data);
		} else {
			pass2_assign_first_block (gwdata, asm_data);
			if (asm_data->this_block >= gwdata->num_pass2_blocks) goto aux_out_of_work;
			pass2_assign_next_block (gwdata, asm_data);
		}

		asm_data->DIST_TO_FFTSRCARG = main_thread_asm_data->DIST_TO_FFTSRCARG;
		asm_data->DIST_TO_MULSRCARG = main_thread_asm_data->DIST_TO_MULSRCARG;
		asm_data->SRC2ARG = main_thread_asm_data->SRC2ARG;
		asm_data->SRC3ARG = main_thread_asm_data->SRC3ARG;
		asm_data->DEST2ARG = main_thread_asm_data->DEST2ARG;
		asm_data->ffttype = main_thread_asm_data->ffttype;
		asm_data->mul4_opcode = main_thread_asm_data->mul4_opcode;
		asm_data->thread_work_routine = main_thread_asm_data->thread_work_routine;
		if (gwdata->pass1_state == 1) {
			asm_data->NORMRTN = main_thread_asm_data->NORMRTN;
			asm_data->zero_fft = main_thread_asm_data->zero_fft;
			asm_data->const_fft = main_thread_asm_data->const_fft;
			asm_data->add_sub_smallmul_op = main_thread_asm_data->add_sub_smallmul_op;
			asm_data->ADDIN_ROW = main_thread_asm_data->ADDIN_ROW;
			asm_data->ADDIN_OFFSET = main_thread_asm_data->ADDIN_OFFSET;
			asm_data->ADDIN_VALUE = main_thread_asm_data->ADDIN_VALUE;
			asm_data->POSTADDIN_VALUE = main_thread_asm_data->POSTADDIN_VALUE;
			if (gwdata->cpu_flags & CPU_AVX512F) {
				asm_data->u.zmm.ZMM_MULCONST = main_thread_asm_data->u.zmm.ZMM_MULCONST;
				asm_data->u.zmm.ZMM_K_TIMES_MULCONST_HI_OVER_LARGE_BASE = main_thread_asm_data->u.zmm.ZMM_K_TIMES_MULCONST_HI_OVER_LARGE_BASE;
				asm_data->u.zmm.ZMM_K_TIMES_MULCONST_HI_OVER_SMALL_BASE = main_thread_asm_data->u.zmm.ZMM_K_TIMES_MULCONST_HI_OVER_SMALL_BASE;
				asm_data->u.zmm.ZMM_K_TIMES_MULCONST_LO = main_thread_asm_data->u.zmm.ZMM_K_TIMES_MULCONST_LO;
				asm_data->u.zmm.ZMM_MINUS_C_TIMES_MULCONST = main_thread_asm_data->u.zmm.ZMM_MINUS_C_TIMES_MULCONST;
			} else if (gwdata->cpu_flags & CPU_AVX) {
				memcpy (asm_data->u.ymm.YMM_MULCONST, main_thread_asm_data->u.ymm.YMM_MULCONST, 4 * sizeof (double));
				memcpy (asm_data->u.ymm.YMM_K_TIMES_MULCONST_HI, main_thread_asm_data->u.ymm.YMM_K_TIMES_MULCONST_HI, 4 * sizeof (double));
				memcpy (asm_data->u.ymm.YMM_K_TIMES_MULCONST_LO, main_thread_asm_data->u.ymm.YMM_K_TIMES_MULCONST_LO, 4 * sizeof (double));
				memcpy (asm_data->u.ymm.YMM_MINUS_C_TIMES_MULCONST, main_thread_asm_data->u.ymm.YMM_MINUS_C_TIMES_MULCONST, 4 * sizeof (double));
			} else if (gwdata->cpu_flags & CPU_SSE2) {
				memcpy (asm_data->u.xmm.XMM_MULCONST, main_thread_asm_data->u.xmm.XMM_MULCONST, 2 * sizeof (double));
				memcpy (asm_data->u.xmm.XMM_K_TIMES_MULCONST_HI, main_thread_asm_data->u.xmm.XMM_K_TIMES_MULCONST_HI, 2 * sizeof (double));
				memcpy (asm_data->u.xmm.XMM_K_TIMES_MULCONST_LO, main_thread_asm_data->u.xmm.XMM_K_TIMES_MULCONST_LO, 2 * sizeof (double));
				memcpy (asm_data->u.xmm.XMM_MINUS_C_TIMES_MULCONST, main_thread_asm_data->u.xmm.XMM_MINUS_C_TIMES_MULCONST, 2 * sizeof (double));
			}
			
			if (gwdata->cpu_flags & CPU_AVX512F) {
				asm_data->MAXERR = main_thread_asm_data->MAXERR;
			} else if (gwdata->cpu_flags & CPU_AVX) {
				memcpy (asm_data->u.ymm.YMM_MAXERR, &main_thread_asm_data->u.ymm.YMM_MAXERR, 4 * sizeof (double));
			} else {
				asm_data->u.xmm.XMM_SUMOUT[0] = 0.0;
				asm_data->u.xmm.XMM_SUMOUT[1] = 0.0;
				memcpy (asm_data->u.xmm.XMM_MAXERR, &main_thread_asm_data->u.xmm.XMM_MAXERR, 2 * sizeof (double));
			}
		}

		if (gwdata->pass1_state < PASS1_STATE_PASS2)
			pass1_aux_entry_point (asm_data);
		else
			pass2_aux_entry_point (asm_data);

aux_out_of_work:
		
		if (gwdata->parent_gwdata != NULL) gwdata = gwdata->parent_gwdata;

		gwdata->all_work_assigned = TRUE;		
		gwevent_reset (&gwdata->work_to_do);		
		if (gwdata->use_spin_wait >= 2) atomic_set (gwdata->alt_work_to_do, 0);
	    }

	    if (atomic_decr_fetch (gwdata->num_active_helpers) == 0) gwevent_signal (&gwdata->all_helpers_done);
	}

	if (gwdata->thread_callback != NULL)
		(*gwdata->thread_callback) (thread_num, 1, gwdata->thread_callback_data);
}

void pass1_wake_up_threads (
	struct gwasm_data *asm_data)
{
	gwhandle *gwdata = asm_data->gwdata;

	gwdata->pass1_state = asm_data->next_block;

	if (gwdata->pass1_state == 0) {

		if (gwdata->ZERO_PADDED_FFT) xcopy_7_words (asm_data);

		atomic_set (gwdata->next_block, 0);
		pass1_state0_assign_first_block (gwdata, asm_data);
		pass1_state0_assign_next_block (gwdata, asm_data);
	}

	if (gwdata->pass1_state == 1) {

		asm_data->DESTARG = (char *) asm_data->DESTARG + (intptr_t) asm_data->DEST2ARG;

		if (gwdata->cpu_flags & CPU_AVX512F) {
		}
		else if (gwdata->cpu_flags & CPU_AVX) {
			asm_data->u.ymm.YMM_MAXERR[0] =
			asm_data->u.ymm.YMM_MAXERR[1] =
			asm_data->u.ymm.YMM_MAXERR[2] =
			asm_data->u.ymm.YMM_MAXERR[3] = asm_data->MAXERR;
		} else {
			((double *) asm_data->DESTARG)[-3] = 0.0; 
			asm_data->u.xmm.XMM_SUMOUT[0] =
			asm_data->u.xmm.XMM_SUMOUT[1] = 0.0;
			asm_data->u.xmm.XMM_MAXERR[0] =
			asm_data->u.xmm.XMM_MAXERR[1] = asm_data->MAXERR;
		}

		memset (gwdata->pass1_carry_sections, 0, gwdata->num_threads * sizeof (struct pass1_carry_sections));
		gwdata->pass1_carry_sections[0].last_block = gwdata->num_pass1_blocks;
		gwdata->pass1_carry_sections_unallocated = gwdata->num_threads - 1;

		pass1_state1_assign_first_block (gwdata, asm_data);
		pass1_state1_assign_next_block (gwdata, asm_data);
	}

	if (gwdata->num_threads > 1) signal_auxiliary_threads (gwdata);
}

void pass1_pre_carries (
	struct gwasm_data *asm_data)
{
	gwhandle *gwdata = asm_data->gwdata;

	if (asm_data->ADDIN_VALUE != 0.0 && !gwdata->ZERO_PADDED_FFT && asm_data->this_block == asm_data->ADDIN_ROW) {
		double *fft_data = gwdata->SCRATCH_SIZE ? (double *) asm_data->scratch_area : (double *) asm_data->data_addr;
		fft_data = (double *) ((char *) fft_data + asm_data->ADDIN_OFFSET);
		*fft_data += asm_data->ADDIN_VALUE;
	}

	if (gwdata->ZERO_PADDED_FFT && asm_data->this_block <= 6) {
		struct gwasm_data *main_thread_asm_data = (struct gwasm_data *) gwdata->asm_data;

		double *fft_data = gwdata->SCRATCH_SIZE ? (double *) asm_data->scratch_area : (double *) asm_data->DESTARG;
		for (unsigned int i = asm_data->this_block; i <= 6 && i < asm_data->this_block + asm_data->cache_line_multiplier; i++) {
			if (gwdata->FFT_TYPE == FFT_TYPE_RADIX_4_DWPN) {
				main_thread_asm_data->ZPAD0_6[i] *= gwdata->ZPAD_0_6_ADJUST[i];
				*fft_data -= main_thread_asm_data->ZPAD0_6[i];
			} else {
				*fft_data -= main_thread_asm_data->ZPAD0_6[i] * asm_data->u.xmm.XMM_NORM012_FF[0];
				asm_data->u.xmm.XMM_SUMOUT[0] += main_thread_asm_data->ZPAD0_6[i] * asm_data->u.xmm.XMM_NORM012_FF[0];
			}
			if (gwdata->cpu_flags & CPU_AVX512F)
				fft_data += 16;		
			else
				fft_data += 8;		
		}
	}
}

#define PASS1_CARRIES_NO_FORWARD_FFT	0
#define PASS1_CARRIES_FORWARD_FFT	1

int pass1_post_carries (
	struct gwasm_data *asm_data)
{
	gwhandle *gwdata = asm_data->gwdata;
	int	i;

	if (asm_data->POSTADDIN_VALUE != 0.0 && !gwdata->ZERO_PADDED_FFT && asm_data->this_block == asm_data->ADDIN_ROW) {
		double *fft_data = gwdata->SCRATCH_SIZE ? (double *) asm_data->scratch_area : (double *) asm_data->data_addr;
		fft_data = (double *) ((char *) fft_data + asm_data->ADDIN_OFFSET);
		*fft_data += asm_data->POSTADDIN_VALUE;
	}

	if (!gwdata->POSTFFT) return (PASS1_CARRIES_NO_FORWARD_FFT);

	i = asm_data->thread_num;
	if (asm_data->this_block < gwdata->pass1_carry_sections[i].start_block + gwdata->num_postfft_blocks)
		return (PASS1_CARRIES_NO_FORWARD_FFT);

	if (gwdata->ZERO_PADDED_FFT && asm_data->this_block + asm_data->cache_line_multiplier >= gwdata->num_pass1_blocks - 3)
		xcopy_3_words (asm_data);

	return (PASS1_CARRIES_FORWARD_FFT);
}

#define PASS1_DO_MORE_INVERSE_FFT	0
#define PASS1_DO_MORE_FORWARD_FFT	1
#define PASS1_COMPLETE			2
#define PASS1_EXIT_THREAD		3
#define PASS1_START_PASS2		4
#define PASS1_DO_GWCARRIES		5

int pass1_get_next_block (
	struct gwasm_data *asm_data)
{
	gwhandle *gwdata = asm_data->gwdata;

	if (gwdata->pass1_state == 0) {

		if (asm_data->this_block == asm_data->next_block) {
			asm_data->DIST_TO_FFTSRCARG = 0;
			return (PASS1_START_PASS2);
		}

		asm_data->this_block = asm_data->next_block;
		asm_data->data_addr = asm_data->data_prefetch;
		asm_data->premult_addr = asm_data->premult_prefetch;
		pass1_state0_assign_next_block (gwdata, asm_data);
		return (PASS1_DO_MORE_FORWARD_FFT);
	}

	ASSERTG (gwdata->pass1_carry_sections[0].section_state >= 1 && gwdata->pass1_carry_sections[0].section_state <= 3);
	if (gwdata->pass1_carry_sections[0].section_state == 1) {

		if (gwdata->pass1_carry_sections[0].next_block != gwdata->pass1_carry_sections[0].last_block) {
			asm_data->this_block = gwdata->pass1_carry_sections[0].next_block;
			gwdata->pass1_carry_sections[0].next_block += asm_data->cache_line_multiplier;
			asm_data->data_addr = pass1_data_addr (gwdata, asm_data, asm_data->this_block);
			asm_data->premult_addr = pass1_premult_addr (gwdata, asm_data->this_block);
			pass1_state1_norm_addrs (gwdata, asm_data);
			pass1_state1_assign_next_block (gwdata, asm_data);
			return (PASS1_DO_MORE_INVERSE_FFT);
		}

		gwdata->pass1_carry_sections[0].section_state = 2;
		asm_data->this_block = 0;
		asm_data->data_addr = pass1_data_addr (gwdata, asm_data, asm_data->this_block);
		pass1_state1_carry_addrs (gwdata, asm_data);
		return (PASS1_DO_GWCARRIES);
	}

	if (gwdata->pass1_carry_sections[0].section_state == 2 && gwdata->POSTFFT) {
		gwdata->pass1_carry_sections[0].section_state = 3;

		if (gwdata->ZERO_PADDED_FFT) xcopy_4_words (asm_data);

		gwdata->pass1_carry_sections[0].start_block = 0;
		gwdata->pass1_carry_sections[0].last_block = gwdata->num_postfft_blocks;
		gwdata->pass1_carry_sections[0].next_block = 0;
	}

	if (gwdata->pass1_carry_sections[0].section_state == 3 &&
	    gwdata->pass1_carry_sections[0].next_block != gwdata->pass1_carry_sections[0].last_block) {
		asm_data->this_block = gwdata->pass1_carry_sections[0].next_block;
		gwdata->pass1_carry_sections[0].next_block += asm_data->cache_line_multiplier;
		asm_data->data_addr = pass1_data_addr (gwdata, asm_data, asm_data->this_block);
		asm_data->premult_addr = pass1_premult_addr (gwdata, asm_data->this_block);
		pass1_state1_assign_next_block (gwdata, asm_data);
		return (PASS1_DO_MORE_FORWARD_FFT);
	}

	if (gwdata->cpu_flags & CPU_AVX512F) {
	} else if (gwdata->cpu_flags & CPU_AVX) {
		if (asm_data->u.ymm.YMM_MAXERR[0] > asm_data->MAXERR) asm_data->MAXERR = asm_data->u.ymm.YMM_MAXERR[0];
		if (asm_data->u.ymm.YMM_MAXERR[1] > asm_data->MAXERR) asm_data->MAXERR = asm_data->u.ymm.YMM_MAXERR[1];
		if (asm_data->u.ymm.YMM_MAXERR[2] > asm_data->MAXERR) asm_data->MAXERR = asm_data->u.ymm.YMM_MAXERR[2];
		if (asm_data->u.ymm.YMM_MAXERR[3] > asm_data->MAXERR) asm_data->MAXERR = asm_data->u.ymm.YMM_MAXERR[3];
	} else if (gwdata->cpu_flags & CPU_SSE2) {
		if (asm_data->u.xmm.XMM_MAXERR[0] > asm_data->MAXERR) asm_data->MAXERR = asm_data->u.xmm.XMM_MAXERR[0];
		if (asm_data->u.xmm.XMM_MAXERR[1] > asm_data->MAXERR) asm_data->MAXERR = asm_data->u.xmm.XMM_MAXERR[1];
		
		((double *) asm_data->DESTARG)[-3] = asm_data->u.xmm.XMM_SUMOUT[0] + asm_data->u.xmm.XMM_SUMOUT[1];
		
		((double *) asm_data->DESTARG)[-3] *= asm_data->ttmp_ff_inv;
	}

	return (PASS1_COMPLETE);
}

int pass1_get_next_block_mt (
	struct gwasm_data *asm_data)
{
	gwhandle *gwdata = asm_data->gwdata;
	int	i;

	if (gwdata->pass1_state == 0) {

		if (asm_data->this_block == asm_data->next_block) {
			if (asm_data->thread_num) return (PASS1_EXIT_THREAD);
			wait_on_auxiliary_threads (gwdata);
			asm_data->DIST_TO_FFTSRCARG = 0;
			return (PASS1_START_PASS2);
		}

		asm_data->this_block = asm_data->next_block;
		asm_data->data_addr = asm_data->data_prefetch;
		asm_data->premult_addr = asm_data->premult_prefetch;
		pass1_state0_assign_next_block (gwdata, asm_data);
		return (PASS1_DO_MORE_FORWARD_FFT);
	}

	i = asm_data->thread_num;
	ASSERTG (gwdata->pass1_carry_sections[i].section_state >= 1 && gwdata->pass1_carry_sections[i].section_state <= 3);
	if (gwdata->pass1_carry_sections[i].section_state == 1) {

		if (asm_data->this_block + asm_data->cache_line_multiplier == gwdata->pass1_carry_sections[i].start_block + gwdata->num_postfft_blocks) {
			gwdata->pass1_carry_sections[i].carry_in_blocks_finished = TRUE;
			gwevent_signal (&gwdata->can_carry_into);
		}

		lock_carry_section (gwdata, i);

		if (gwdata->pass1_carry_sections[i].next_block != gwdata->pass1_carry_sections[i].last_block) {
			
			asm_data->this_block = gwdata->pass1_carry_sections[i].next_block;
			gwdata->pass1_carry_sections[i].next_block += asm_data->cache_line_multiplier;
			
			unlock_carry_section (gwdata, i);
			asm_data->data_addr = pass1_data_addr (gwdata, asm_data, asm_data->this_block);
			asm_data->premult_addr = pass1_premult_addr (gwdata, asm_data->this_block);
			pass1_state1_norm_addrs (gwdata, asm_data);
			pass1_state1_assign_next_block (gwdata, asm_data);
			return (PASS1_DO_MORE_INVERSE_FFT);
		}

		gwdata->pass1_carry_sections[i].section_state = 2;
		unlock_carry_section (gwdata, i);

		int carry_out_section = gwdata->pass1_carry_sections[i].carry_out_section;
		int last_block = gwdata->pass1_carry_sections[i].last_block != gwdata->num_pass1_blocks ? gwdata->pass1_carry_sections[i].last_block : 0;
		while (last_block == gwdata->pass1_carry_sections[carry_out_section].start_block &&
		       !gwdata->pass1_carry_sections[carry_out_section].carry_in_blocks_finished) {
			gwmutex_lock (&gwdata->thread_lock);
			if (atomic_fetch_incr (gwdata->can_carry_into_counter) == 0) gwevent_reset (&gwdata->can_carry_into);
			gwmutex_unlock (&gwdata->thread_lock);
			if (last_block != gwdata->pass1_carry_sections[carry_out_section].start_block ||
			    gwdata->pass1_carry_sections[carry_out_section].carry_in_blocks_finished) { atomic_decr (gwdata->can_carry_into_counter); break; }
			gwevent_wait (&gwdata->can_carry_into, 0);
			atomic_decr (gwdata->can_carry_into_counter);
		}

		asm_data->this_block = gwdata->pass1_carry_sections[i].last_block;
		if (asm_data->this_block == gwdata->num_pass1_blocks) asm_data->this_block = 0;
		
		if (gwdata->ZERO_PADDED_FFT && asm_data->this_block == 0) {
			struct gwasm_data *main_thread_asm_data = (struct gwasm_data *) gwdata->asm_data;
			memcpy (asm_data->ZPAD0_6, main_thread_asm_data->ZPAD0_6, 7 * sizeof (double));
		}
		asm_data->data_addr = pass1_data_addr (gwdata, asm_data, asm_data->this_block);
		pass1_state1_carry_addrs (gwdata, asm_data);
		return (PASS1_DO_GWCARRIES);
	}

	if (gwdata->pass1_carry_sections[i].section_state == 2 && gwdata->POSTFFT) {
		gwdata->pass1_carry_sections[i].section_state = 3;

		if (gwdata->ZERO_PADDED_FFT) {
			if (asm_data->this_block == 0) xcopy_4_words (asm_data);

			if (asm_data->this_block + gwdata->num_postfft_blocks >= gwdata->num_pass1_blocks - 3)
				xcopy_3_words_after_gwcarries (asm_data);
		}

		gwdata->pass1_carry_sections[i].start_block = gwdata->pass1_carry_sections[i].next_block;
		if (gwdata->pass1_carry_sections[i].start_block == gwdata->num_pass1_blocks) gwdata->pass1_carry_sections[i].start_block = 0;
		gwdata->pass1_carry_sections[i].last_block = gwdata->pass1_carry_sections[i].start_block + gwdata->num_postfft_blocks;
		gwdata->pass1_carry_sections[i].next_block = gwdata->pass1_carry_sections[i].start_block;
	}

	if (gwdata->pass1_carry_sections[i].section_state == 3 &&
	    gwdata->pass1_carry_sections[i].next_block != gwdata->pass1_carry_sections[i].last_block) {
		asm_data->this_block = gwdata->pass1_carry_sections[i].next_block;
		gwdata->pass1_carry_sections[i].next_block += asm_data->cache_line_multiplier;
		asm_data->data_addr = pass1_data_addr (gwdata, asm_data, asm_data->this_block);
		asm_data->premult_addr = pass1_premult_addr (gwdata, asm_data->this_block);
		pass1_state1_assign_next_block (gwdata, asm_data);
		return (PASS1_DO_MORE_FORWARD_FFT);
	}

	gwdata->pass1_carry_sections[i].section_state = 4;

	if (pass1_state1_assign_first_block (gwdata, asm_data)) {
		pass1_state1_assign_next_block (gwdata, asm_data);
		return (PASS1_DO_MORE_INVERSE_FFT);
	}

#define replace_with_larger(a,b)	if ((a)<(b)) (a)=(b)
#define locked_replace_with_larger(a,b) if ((a)<(b)) { gwmutex_lock(&gwdata->thread_lock); replace_with_larger(a,b); gwmutex_unlock(&gwdata->thread_lock);}
	if (gwdata->cpu_flags & CPU_AVX512F) {
		struct gwasm_data *main_thread_asm_data = (struct gwasm_data *) gwdata->asm_data;
		locked_replace_with_larger (main_thread_asm_data->MAXERR, asm_data->MAXERR);
	} else if (gwdata->cpu_flags & CPU_AVX) {
		struct gwasm_data *main_thread_asm_data = (struct gwasm_data *) gwdata->asm_data;
		replace_with_larger (asm_data->u.ymm.YMM_MAXERR[0], asm_data->u.ymm.YMM_MAXERR[1]);
		replace_with_larger (asm_data->u.ymm.YMM_MAXERR[0], asm_data->u.ymm.YMM_MAXERR[2]);
		replace_with_larger (asm_data->u.ymm.YMM_MAXERR[0], asm_data->u.ymm.YMM_MAXERR[3]);
		locked_replace_with_larger (main_thread_asm_data->MAXERR, asm_data->u.ymm.YMM_MAXERR[0]);
	} else if (gwdata->cpu_flags & CPU_SSE2) {
		struct gwasm_data *main_thread_asm_data = (struct gwasm_data *) gwdata->asm_data;
		replace_with_larger (asm_data->u.xmm.XMM_MAXERR[0], asm_data->u.xmm.XMM_MAXERR[1]);
		locked_replace_with_larger (main_thread_asm_data->MAXERR, asm_data->u.xmm.XMM_MAXERR[0]);
		
		gwmutex_lock (&gwdata->thread_lock);
		((double *) asm_data->DESTARG)[-3] += asm_data->u.xmm.XMM_SUMOUT[0] + asm_data->u.xmm.XMM_SUMOUT[1];
		gwmutex_unlock (&gwdata->thread_lock);
	}

	if (asm_data->thread_num) return (PASS1_EXIT_THREAD);

	wait_on_auxiliary_threads (gwdata);

	((double *) asm_data->DESTARG)[-3] *= asm_data->ttmp_ff_inv;

	return (PASS1_COMPLETE);
}

void pass2_wake_up_threads (
	struct gwasm_data *asm_data)
{
	gwhandle *gwdata = asm_data->gwdata;

	gwdata->pass1_state = PASS1_STATE_PASS2;
	atomic_set (gwdata->next_block, 0);
	pass2_assign_first_block (gwdata, asm_data);
	pass2_assign_next_block (gwdata, asm_data);

	if (gwdata->num_threads > 1 && gwdata->PASS1_SIZE) signal_auxiliary_threads (gwdata);
}

int pass2_get_next_block (
	struct gwasm_data *asm_data)
{
	gwhandle *gwdata;

	if (asm_data->this_block == asm_data->next_block) {
		asm_data->DIST_TO_FFTSRCARG = 0;
		return (TRUE);
	}

	gwdata = asm_data->gwdata;

	asm_data->this_block = asm_data->next_block;
	asm_data->data_addr = asm_data->data_prefetch;
	asm_data->premult_addr = asm_data->premult_prefetch;
	pass2_assign_next_block (gwdata, asm_data);

	return (FALSE);
}

int pass2_get_next_block_mt (
	struct gwasm_data *asm_data)
{
	gwhandle *gwdata = asm_data->gwdata;

	if (asm_data->this_block == asm_data->next_block) {
		if (asm_data->thread_num) return (TRUE);
		wait_on_auxiliary_threads (gwdata);
		asm_data->DIST_TO_FFTSRCARG = 0;
		return (TRUE);
	}

	asm_data->this_block = asm_data->next_block;
	asm_data->data_addr = asm_data->data_prefetch;
	asm_data->premult_addr = asm_data->premult_prefetch;
	pass2_assign_next_block (gwdata, asm_data);

	return (FALSE);
}

int multithread_init (
	gwhandle *gwdata)
{
	struct gwasm_data *asm_data;
	unsigned int i;

	if (gwdata->PASS2_SIZE == 0 || !(gwdata->cpu_flags & (CPU_AVX512F | CPU_AVX | CPU_SSE2))) {
		gwdata->num_threads = 1;
		return (0);
	}

	if (!gwdata->GENERAL_MMGW_MOD) {

		asm_data = (struct gwasm_data *) gwdata->asm_data;

		asm_data->gwdata = gwdata;
		asm_data->thread_num = 0;

		if (gwdata->cpu_flags & CPU_AVX512F) {
			if (gwdata->PASS1_SIZE == 0) {
				gwdata->num_pass1_blocks = 1;
				gwdata->num_pass2_blocks = 1;
				asm_data->last_pass1_block = 0;
			} else {
				gwdata->num_pass1_blocks = gwdata->PASS2_SIZE;
				gwdata->num_pass2_blocks = gwdata->PASS1_SIZE >> 1;
				asm_data->last_pass1_block = gwdata->num_pass1_blocks - asm_data->cache_line_multiplier;
			}
		}
		else if (gwdata->cpu_flags & CPU_AVX) {
			gwdata->num_pass1_blocks = gwdata->PASS2_SIZE;
			gwdata->num_pass2_blocks = gwdata->PASS1_SIZE >> 1;
			asm_data->last_pass1_block = gwdata->num_pass1_blocks - asm_data->cache_line_multiplier;
		} else {
			gwdata->num_pass1_blocks = gwdata->PASS2_SIZE >> 1;
			gwdata->num_pass2_blocks = gwdata->PASS1_SIZE >> 2;
			asm_data->last_pass1_block = gwdata->num_pass1_blocks - asm_data->cache_line_multiplier;
		}

		if (gwdata->num_threads > gwdata->num_pass1_blocks / asm_data->cache_line_multiplier)
			gwdata->num_threads = gwdata->num_pass1_blocks / asm_data->cache_line_multiplier;
		if (gwdata->num_threads > gwdata->num_pass1_blocks / 8)
			gwdata->num_threads = gwdata->num_pass1_blocks / 8;
		if (gwdata->num_threads == 0) gwdata->num_threads = 1;

		if (gwdata->ZERO_PADDED_FFT)
			gwdata->num_postfft_blocks = 8;
		else if (gwdata->cpu_flags & CPU_AVX512F) {
			gwdata->num_postfft_blocks = (int) floor (53.0 / (gwdata->avg_num_b_per_word * log2 (gwdata->b)));
			ASSERTG (gwdata->PASS1_SIZE == 0 || gwdata->num_postfft_blocks <= 8);
		} else if (gwdata->cpu_flags & CPU_AVX) {
			gwdata->num_postfft_blocks = (int) floor (53.0 / (gwdata->avg_num_b_per_word * log2 (gwdata->b)));
			ASSERTG (gwdata->num_postfft_blocks <= 8);
			asm_data->SPREAD_CARRY_OVER_EXTRA_WORDS = (gwdata->num_postfft_blocks > 4);
		} else {
			if (asm_data->SPREAD_CARRY_OVER_EXTRA_WORDS) gwdata->num_postfft_blocks = 6;
			else gwdata->num_postfft_blocks = 2;
		}
		gwdata->num_postfft_blocks = round_up_to_multiple_of (gwdata->num_postfft_blocks, asm_data->cache_line_multiplier);

		if (gwdata->FFT_TYPE == FFT_TYPE_HOME_GROWN) {
			gwdata->pass2_premult_block_size =
				(gwdata->PASS2_SIZE == 256) ? 32 * 128 :
				(gwdata->PASS2_SIZE == 1024) ? 64 * 128 :
				(gwdata->PASS2_SIZE == 2048) ? 96 * 128 :
				(gwdata->PASS2_SIZE == 4096) ? 128 * 128 :
				(gwdata->PASS2_SIZE == 8192) ? 192 * 128 : 0;
			if (gwdata->NEGACYCLIC_FFT)
				gwdata->adjusted_pass2_premults = asm_data->u.xmm.pass2_premults;
			else
				gwdata->adjusted_pass2_premults = (char *) asm_data->u.xmm.pass2_premults - gwdata->pass2_premult_block_size;
		}

		gwdata->pass1_carry_sections = (struct pass1_carry_sections *) malloc (gwdata->num_threads * sizeof (struct pass1_carry_sections));
		if (gwdata->pass1_carry_sections == NULL) return (GWERROR_MALLOC);

		if (gwdata->num_threads <= 1) {
			asm_data->pass1_wake_up_threads = pass1_wake_up_threads;
			asm_data->pass1_pre_carries = pass1_pre_carries;
			asm_data->pass1_post_carries = pass1_post_carries;
			asm_data->pass1_get_next_block = pass1_get_next_block;
			asm_data->pass2_wake_up_threads = pass2_wake_up_threads;
			asm_data->pass2_get_next_block = pass2_get_next_block;
			return (0);
		}

		gwmutex_init (&gwdata->thread_lock);
		gwevent_init (&gwdata->can_carry_into);
		atomic_set (gwdata->can_carry_into_counter, 0);		

		asm_data->pass1_wake_up_threads = pass1_wake_up_threads;
		asm_data->pass1_pre_carries = pass1_pre_carries;
		asm_data->pass1_post_carries = pass1_post_carries;
		asm_data->pass1_get_next_block = pass1_get_next_block_mt;
		asm_data->pass2_wake_up_threads = pass2_wake_up_threads;
		asm_data->pass2_get_next_block = pass2_get_next_block_mt;

		gwdata->thread_allocs = (gwthread *) malloc ((gwdata->num_threads - 1) * sizeof (void *));
		if (gwdata->thread_allocs == NULL) return (GWERROR_MALLOC);
		memset (gwdata->thread_allocs, 0, (gwdata->num_threads - 1) * sizeof (void *));

		for (i = 0; i < gwdata->num_threads - 1; i++) {
			
			gwdata->thread_allocs[i] = aligned_malloc (NEW_STACK_SIZE + sizeof (struct gwasm_data) + 128 + gwdata->SCRATCH_SIZE + 128 + asm_data_carries_size (gwdata) * sizeof (double), 4096);
			if (gwdata->thread_allocs[i] == NULL) return (GWERROR_MALLOC);
			
			struct gwasm_data *thread_asm_data = (struct gwasm_data *) ((char *) gwdata->thread_allocs[i] + NEW_STACK_SIZE);
			thread_asm_data->aux_initialized = FALSE;
		}
	}

	if (gwdata->num_threads > 1 && gwdata->parent_gwdata == NULL) {

		gwevent_init (&gwdata->work_to_do);
		gwevent_init (&gwdata->all_helpers_done);
		atomic_set (gwdata->alt_work_to_do, 0);			
		gwdata->helpers_must_exit = FALSE;

		gwdata->thread_ids = (gwthread *) malloc ((gwdata->num_threads - 1) * sizeof (gwthread));
		if (gwdata->thread_ids == NULL) return (GWERROR_MALLOC);
		memset (gwdata->thread_ids, 0, (gwdata->num_threads - 1) * sizeof (gwthread));

		for (i = 0; i < gwdata->num_threads - 1; i++) {
			struct thread_data *info;

			info = (struct thread_data *) malloc (sizeof (struct thread_data));
			if (info == NULL) return (GWERROR_MALLOC);
			info->gwdata = gwdata;
			info->thread_num = i+1;

			gwthread_create_waitable (&gwdata->thread_ids[i], &auxiliary_thread, info);
		}
	}

	return (0);
}

void multithread_term (
	gwhandle *gwdata)
{
	struct gwasm_data *asm_data = (struct gwasm_data *) gwdata->asm_data;
	unsigned int i;

	if (gwdata->num_threads > 1 && gwdata->parent_gwdata == NULL) {

		if (gwdata-> thread_ids != NULL) {
			gwdata->helpers_must_exit = TRUE;
			gwevent_signal (&gwdata->work_to_do);
			atomic_set (gwdata->alt_work_to_do, 1);

			for (i = 0; i < gwdata->num_threads - 1; i++)
				if (gwdata->thread_ids[i]) gwthread_wait_for_exit (&gwdata->thread_ids[i]);
			free (gwdata->thread_ids), gwdata->thread_ids = NULL;
		}

		gwevent_destroy (&gwdata->work_to_do);
		gwevent_destroy (&gwdata->all_helpers_done);
	}

	if (gwdata->num_threads > 1 && !gwdata->GENERAL_MMGW_MOD) {
		if (gwdata->thread_allocs != NULL) {
			for (i = 0; i < gwdata->num_threads - 1; i++) aligned_free (gwdata->thread_allocs[i]);
			free (gwdata->thread_allocs), gwdata->thread_allocs = NULL;
		}
		gwmutex_destroy (&gwdata->thread_lock);
		gwevent_destroy (&gwdata->can_carry_into);
	}

	free (gwdata->pass1_carry_sections);
	gwdata->pass1_carry_sections = NULL;
}

void gwdone (
	gwhandle *gwdata)	
{
	
	multithread_term (gwdata);

	if (gwdata->clone_of == NULL) {
		free (gwdata->gwnum_free); gwdata->gwnum_free = NULL;
		if (gwdata->gwnum_alloc != NULL) {
			for (unsigned int i = 0; i < gwdata->gwnum_alloc_count; i++) {
				char	*p;
				int32_t	freeable;
				p = (char *) gwdata->gwnum_alloc[i];
				freeable = * (int32_t *) (p - 32);
				if (freeable & GWFREEABLE) {
					if (freeable & GWFREE_LARGE_PAGES) aligned_large_pages_free ((char *) p - GW_HEADER_SIZE (gwdata));
					else aligned_free ((char *) p - GW_HEADER_SIZE (gwdata));
				}
			}
			free (gwdata->gwnum_alloc); gwdata->gwnum_alloc = NULL;
		}
		while (gwdata->array_list != NULL) gwfree_array (gwdata, (gwnum *) gwdata->array_list);
		if (gwdata->large_pages_ptr != NULL) large_pages_free (gwdata->large_pages_ptr), gwdata->large_pages_ptr = NULL;
		else aligned_free (gwdata->gwnum_memory), gwdata->gwnum_memory = NULL;
	}

	term_ghandle (&gwdata->gdata);

	if (gwdata->clone_of == NULL) {
		free (gwdata->GW_MODULUS); gwdata->GW_MODULUS = NULL;
		free (gwdata->dd_data); gwdata->dd_data = NULL;
		gwmutex_destroy (&gwdata->alloc_lock);
	}

	if (gwdata->clone_of == NULL) {
		if (gwdata->asm_data != NULL) {
			struct gwasm_data *asm_data = (struct gwasm_data *) gwdata->asm_data;
			unshare_sincos_data (asm_data->sincos1);			
			unshare_sincos_data (asm_data->sincos2);			
			unshare_sincos_data (asm_data->xsincos_complex);		
			unshare_sincos_data (asm_data->sincos3);			
			aligned_free ((char *) gwdata->asm_data - NEW_STACK_SIZE), gwdata->asm_data = NULL;
		}
		
		ASSERTG (gwdata->clone_count == 0);
	}
	else {
		if (gwdata->asm_data != NULL) {
			struct gwasm_data *asm_data = (struct gwasm_data *) gwdata->asm_data;
			
			if (gwdata->SCRATCH_SIZE) aligned_free (asm_data->scratch_area), asm_data->scratch_area = NULL;
			aligned_free (asm_data->carries), asm_data->carries = NULL;
			aligned_free ((char *) gwdata->asm_data - NEW_STACK_SIZE), gwdata->asm_data = NULL;
		}
		
		atomic_decr (gwdata->clone_of->clone_count);
	}

	if (gwdata->to_radix_gwdata != NULL) {
		gwdone (gwdata->to_radix_gwdata);
		free (gwdata->to_radix_gwdata), gwdata->to_radix_gwdata = NULL;
	}
	if (gwdata->from_radix_gwdata != NULL) {
		gwdone (gwdata->from_radix_gwdata);
		free (gwdata->from_radix_gwdata), gwdata->from_radix_gwdata = NULL;
	}

	if (gwdata->cyclic_gwdata != NULL) {
		gwdone (gwdata->cyclic_gwdata);
		free (gwdata->cyclic_gwdata), gwdata->cyclic_gwdata = NULL;
	}
	if (gwdata->negacyclic_gwdata != NULL) {
		gwdone (gwdata->negacyclic_gwdata);
		free (gwdata->negacyclic_gwdata), gwdata->negacyclic_gwdata = NULL;
	}
}

gwnum real_gwalloc (
	gwhandle *gwdata,
	int32_t	additional_freeable_flags)
{
	unsigned long header_size, size, aligned_size;
	char	*p, *q;
	int32_t	freeable;

	if (gwdata->clone_of) gwdata = gwdata->clone_of;

	if (gwdata->gwnum_free_count) {
		gwmutex_lock (&gwdata->alloc_lock);			
		if (gwdata->gwnum_free_count) {
			gwnum r = gwdata->gwnum_free[--gwdata->gwnum_free_count];
			gwmutex_unlock (&gwdata->alloc_lock);
			
			* (uint32_t *) ((char *) r - 28) = 0;		
			* (double *) ((char *) r - 16) = 0.0;		
			* (double *) ((char *) r - 24) = 0.0;		
			* (int32_t *) ((char *) r - 32) |= additional_freeable_flags;
			unnorms (r) = 0.0f;				
			return (r);
		}
		gwmutex_unlock (&gwdata->alloc_lock);
	}

	header_size = GW_HEADER_SIZE (gwdata);
	size = gwnum_datasize (gwdata);
	aligned_size = round_up_to_multiple_of (header_size, 128) + round_up_to_multiple_of (size, 128);
	p = NULL;

	if (gwdata->large_pages_gwnum != NULL) {
		p = (char *) gwdata->large_pages_gwnum;
		gwdata->large_pages_gwnum = NULL;
		p += round_up_to_multiple_of (header_size, 128) - header_size;
		freeable = 0;
	}

	else if (gwdata->GW_BIGBUF_SIZE >= aligned_size) {
		p = gwdata->GW_BIGBUF;
		gwdata->GW_BIGBUF += aligned_size;
		gwdata->GW_BIGBUF_SIZE -= aligned_size;
		p += round_up_to_multiple_of (header_size, 128) - header_size;
		freeable = 0;
	}

	else if (gwdata->use_large_pages) {
		
		int	alignment = gwdata->GW_ALIGNMENT;
		int	alignment_mod = (header_size - gwdata->GW_ALIGNMENT_MOD) & (gwdata->GW_ALIGNMENT - 1);
		if (alignment < 65536 && 65536 % alignment == 0) {
			alignment_mod += (rand () % (65536 / alignment)) * alignment;
			alignment = 65536;
		}
		p = (char *) aligned_offset_large_pages_malloc (size + header_size, alignment, alignment_mod);
		freeable = GWFREEABLE + GWFREE_LARGE_PAGES;
	}

#ifdef __FreeBSD__
	if (p == NULL && size >= 0x400000 && gwdata->gwnum_alloc_count == 0) {
		p = (char *) aligned_offset_malloc (size + header_size, 0x400000, (header_size - gwdata->GW_ALIGNMENT_MOD) & 0x3FFFFF);
		freeable = GWFREEABLE;
	}
#endif
	if (p == NULL) {
		p = (char *) aligned_offset_malloc (size + header_size, gwdata->GW_ALIGNMENT, (header_size - gwdata->GW_ALIGNMENT_MOD) & (gwdata->GW_ALIGNMENT - 1));
		if (p == NULL) return (NULL);
		freeable = GWFREEABLE;
	}

	q = p + header_size;
	
	* (uint32_t *) (q - 28) = 0;					
	* (double *) (q - 16) = 0.0;					
	* (double *) (q - 24) = 0.0;					
	unnorms (q) = 0.0f;						

	memset (q, 0, size);

	gwmutex_lock (&gwdata->alloc_lock);			
	if (gwdata->gwnum_alloc_count == gwdata->gwnum_alloc_array_size) {
		int new_gwnum_alloc_array_size = gwdata->gwnum_alloc_array_size + (gwdata->gwnum_alloc_array_size >> 1);
		gwnum *new_gwnum_alloc = (gwnum *) realloc (gwdata->gwnum_alloc, new_gwnum_alloc_array_size * sizeof (gwnum));
		gwnum *new_gwnum_free = (gwnum *) realloc (gwdata->gwnum_free, new_gwnum_alloc_array_size * sizeof (gwnum));
		if (new_gwnum_alloc == NULL || new_gwnum_free == NULL) {
			gwmutex_unlock (&gwdata->alloc_lock);
			if (freeable & GWFREE_LARGE_PAGES) aligned_large_pages_free (p);
			else if (freeable & GWFREEABLE) aligned_free (p);
			return (NULL);
		}
		gwdata->gwnum_alloc = new_gwnum_alloc;
		gwdata->gwnum_free = new_gwnum_free;
		gwdata->gwnum_alloc_array_size = new_gwnum_alloc_array_size;
	}

	* (int32_t *) (q - 32) = freeable + additional_freeable_flags + gwdata->gwnum_alloc_count;	
	gwdata->gwnum_alloc[gwdata->gwnum_alloc_count++] = (gwnum) q;
	gwmutex_unlock (&gwdata->alloc_lock);

	return ((gwnum) q);
}

gwnum gwalloc (
	gwhandle *gwdata)
{
	return (real_gwalloc (gwdata, 0));
}

gwnum gwalloc_internal (
	gwhandle *gwdata)
{
	return (real_gwalloc (gwdata, GWFREE_INTERNAL));
}

void gwfree_freeable (
	gwhandle *gwdata,
	gwnum	q)
{
	gwnum	moved_gwnum;
	int32_t	freeable, moved_freeable, alloc_index;

	ASSERTG (gwdata->clone_of == NULL);

	freeable = * (int32_t *) ((char *) q - 32);
	ASSERTG (freeable & GWFREEABLE);

	alloc_index = freeable & GWFREEALLOC_INDEX;
	moved_gwnum = gwdata->gwnum_alloc[--gwdata->gwnum_alloc_count];
	gwdata->gwnum_alloc[alloc_index] = moved_gwnum;
	
	moved_freeable = * (int32_t *) ((char *) moved_gwnum - 32);
	moved_freeable &= ~GWFREEALLOC_INDEX;
	moved_freeable |= alloc_index;
	* (int32_t *) ((char *) moved_gwnum - 32) = moved_freeable;
	
	if (freeable & GWFREE_LARGE_PAGES) aligned_large_pages_free ((char *) q - GW_HEADER_SIZE (gwdata));
	else aligned_free ((char *) q - GW_HEADER_SIZE (gwdata));
}

void gwfree (
	gwhandle *gwdata,	
	gwnum	q)		
{
	if (q == NULL) return;

	if (gwdata->clone_of) gwdata = gwdata->clone_of;
	gwmutex_lock (&gwdata->alloc_lock);

	* (int32_t *) ((char *) q - 32) &= ~GWFREE_INTERNAL;

	ASSERTG (gwdata->gwnum_alloc[* (int32_t *) ((char *) q - 32) & GWFREEALLOC_INDEX] == q);	
	if (gwdata->gwnum_free_count >= gwdata->gwnum_max_free_count && * (int32_t *) ((char *) q - 32) & GWFREEABLE)
		gwfree_freeable (gwdata, q);

	else
		gwdata->gwnum_free[gwdata->gwnum_free_count++] = q;
	gwmutex_unlock (&gwdata->alloc_lock);
}

void gwfree_cached (
	gwhandle *gwdata)	
{
	int	i;

	if (gwdata->clone_of) gwdata = gwdata->clone_of;
	gwmutex_lock (&gwdata->alloc_lock);

	for (i = 0; i < (int) gwdata->gwnum_free_count; i++) {
		gwnum	q = gwdata->gwnum_free[i];
		int32_t	freeable = * (int32_t *) ((char *) q - 32);
		if (freeable & GWFREEABLE) {
			gwfree_freeable (gwdata, q);
			gwdata->gwnum_free[i--] = gwdata->gwnum_free[--gwdata->gwnum_free_count];
		}
	}
	gwmutex_unlock (&gwdata->alloc_lock);
}

void gwfree_internal_memory (
	gwhandle *gwdata)	
{
	
	if (gwdata->clone_count == 0 && gwdata->clone_of == NULL) {
		gwmutex_lock (&gwclone_lock);		
		gwfree (gwdata, gwdata->GW_RANDOM), gwdata->GW_RANDOM = NULL;
		gwfree (gwdata, gwdata->GW_RANDOM_SQUARED), gwdata->GW_RANDOM_SQUARED = NULL;
		gwfree (gwdata, gwdata->GW_RANDOM_FFT), gwdata->GW_RANDOM_FFT = NULL;
		if (gwdata->FFT1_state == 1 && !gwdata->FFT1_user_allocated) gwfree (gwdata, gwdata->GW_FFT1), gwdata->GW_FFT1 = NULL, gwdata->FFT1_state = 0;
		gwmutex_unlock (&gwclone_lock);
	}
	if (gwdata->to_radix_gwdata != NULL) gwdone (gwdata->to_radix_gwdata), free (gwdata->to_radix_gwdata), gwdata->to_radix_gwdata = NULL;
	if (gwdata->from_radix_gwdata != NULL) gwdone (gwdata->from_radix_gwdata), free (gwdata->from_radix_gwdata), gwdata->from_radix_gwdata = NULL;
	gwfree_cached (gwdata);
}

gwarray gwalloc_array (		
	gwhandle *gwdata,	
	uint64_t n)		
{
	size_t	array_header_size, gwnum_header_size, gwnum_size, aligned_gwnum_size, array_size;
	int	pad_frequency, pad_amount, pad_direction;
	char	*p = NULL;			
	int	freeable;			

	if (gwdata->clone_of) gwdata = gwdata->clone_of;

	gwnum_header_size = GW_HEADER_SIZE (gwdata);
	gwnum_size = gwnum_datasize (gwdata) + gwnum_header_size;
	aligned_gwnum_size = round_up_to_multiple_of (gwnum_size, 64);

	if (gwdata->cpu_flags & CPU_AVX512F) {
		if (aligned_gwnum_size % 128 == 64) aligned_gwnum_size += 64;
		if (aligned_gwnum_size % 256 == 0) aligned_gwnum_size += 128;
		pad_amount = 128;							
	} else {
		if (aligned_gwnum_size % 128 == 0) aligned_gwnum_size += 64;
		pad_amount = 64;							
	}

	array_header_size = sizeof (gwarray_header);
	array_size = (size_t) (array_header_size + n * sizeof (gwnum) + 128 + n * aligned_gwnum_size);

	if (gwdata->scramble_arrays) {
		pad_frequency = 0;
	}
	
	else {
		pad_frequency = 4096 / pad_amount;							
		if ((int) (aligned_gwnum_size - gwnum_size) >= pad_amount) pad_direction = -1;		
		else pad_direction = 1;									
		
		array_size += (size_t) (divide_rounding_down (n, pad_frequency) * pad_direction * pad_amount -
					divide_rounding_down (n, pad_frequency * pad_frequency) * pad_direction * pad_amount +
					divide_rounding_down (n, pad_frequency * pad_frequency  * pad_frequency) * pad_direction * pad_amount);
	}

	if (gwdata->use_large_pages) {
		p = (char *) large_pages_malloc (array_size);
		freeable = GWFREE_LARGE_PAGES;
	}
	
	if (p == NULL) {
		p = (char *) malloc (array_size);
		freeable = 0;
	}
	
	if (p == NULL) return (NULL);

	gwarray array = (gwarray) (p + array_header_size);
	array = (gwarray) round_up_to_multiple_of ((intptr_t) array, sizeof (gwnum));
	gwnum next_gwnum = (gwnum) ((char *) array + n * sizeof (gwnum) + gwnum_header_size);
	next_gwnum = (gwnum) round_up_to_multiple_of ((intptr_t) next_gwnum, pad_amount);
	gwarray_header *array_header = (gwarray_header *) ((char *) array - array_header_size);

	for (int i = 0; i < n; i++) {
		
		if (pad_frequency && i && i % pad_frequency == 0) {
			
			next_gwnum = (gwnum) ((char *) next_gwnum + pad_direction * pad_amount);
			
			if (i % (pad_frequency * pad_frequency) == 0) {
				next_gwnum = (gwnum) ((char *) next_gwnum - pad_direction * pad_amount);
				if (i % (pad_frequency * pad_frequency * pad_frequency) == 0) {
					next_gwnum = (gwnum) ((char *) next_gwnum + pad_direction * pad_amount);
				}
			}
		}
		
		array[i] = next_gwnum;
		memset ((char *) next_gwnum - gwnum_header_size, 0, gwnum_header_size);
		next_gwnum = (gwnum) ((char *) next_gwnum + aligned_gwnum_size);
	}

	if (gwdata->scramble_arrays) {
		
		uint64_t max_block_size = 64;
		if (gwdata->scramble_arrays == 2) max_block_size = n;					
		else if (gwdata->scramble_arrays > 2) max_block_size = gwdata->scramble_arrays;		
		for (uint64_t i = 0; i < n; i += max_block_size) {
			uint64_t block_size = (n - i < max_block_size) ? n - i : max_block_size;
			for (uint64_t j = 0; j < block_size; j++) {
				int k = rand () % block_size;
				gwswap (array[i+j], array[i+k]);
			}
		}
	}

	gwmutex_lock (&gwdata->alloc_lock);

	array_header->flags = freeable;			
	array_header->next = gwdata->array_list;	
	array_header->prev = &gwdata->array_list;	
	if (array_header->next != NULL) {		
		gwarray_header *next_header = (gwarray_header *) ((char *) array_header->next - sizeof (gwarray_header));
		next_header->prev = &array_header->next;
	}
	gwdata->array_list = array;			

	gwmutex_unlock (&gwdata->alloc_lock);
	return (array);
}

void gwfree_array (
	gwhandle *gwdata,	
	gwarray	array)		
{
	
	if (array == NULL) return;

	if (gwdata->clone_of) gwdata = gwdata->clone_of;
	gwmutex_lock (&gwdata->alloc_lock);

	gwarray_header *array_header = (gwarray_header *) ((char *) array - sizeof (gwarray_header));
	*array_header->prev = array_header->next;	
	if (array_header->next != NULL) {		
		gwarray_header *next_header = (gwarray_header *) ((char *) array_header->next - sizeof (gwarray_header));
		next_header->prev = array_header->prev;
	}
	gwmutex_unlock (&gwdata->alloc_lock);

	int freeable = (int) (intptr_t) array[-3];		
	size_t array_header_size = 3 * sizeof (void *);
	if (freeable & GWFREE_LARGE_PAGES) large_pages_free ((char *) array - array_header_size);
	else free ((char *) array - array_header_size);
}

void *gwgiantalloc (
	void	*gwdata)
{
	return ((void *) gwalloc ((gwhandle *) gwdata));
}
void gwgiantfree (
	void	*gwdata,
	void	*q)
{
	gwfree ((gwhandle *) gwdata, (gwnum) q);
}

void gwgiantdealloc (
	void	*gwdata_arg)
{
	gwhandle *gwdata = (gwhandle *) gwdata_arg;

	if (gwdata->clone_of) gwdata = gwdata->clone_of;
	gwmutex_lock (&gwdata->alloc_lock);

	for (unsigned int i = 0; i < gwdata->gwnum_free_count; i++) {
		gwnum q = gwdata->gwnum_free[i];
		int32_t	freeable = * (int32_t *) ((char *) q - 32);
		if (!(freeable & GWFREEABLE)) continue;

		gwdata->gwnum_free[i] = gwdata->gwnum_free[--gwdata->gwnum_free_count];

		gwfree_freeable (gwdata, q);
		break;
	}
	gwmutex_unlock (&gwdata->alloc_lock);
}

void gwfreeall (
	gwhandle *gwdata)	
{

	if (gwdata->clone_of) gwdata = gwdata->clone_of;
	gwmutex_lock (&gwdata->alloc_lock);

	gwdata->gwnum_free_count = 0;
	for (unsigned int i = 0; i < gwdata->gwnum_alloc_count; i++) {
		gwnum	q = gwdata->gwnum_alloc[i];
		if (* (int32_t *) ((char *) q - 32) & GWFREE_INTERNAL) continue;
		if (* (int32_t *) ((char *) q - 32) & GWFREEABLE) {
			
			gwfree_freeable (gwdata, q);
			
			i--;
		}
		else
			gwdata->gwnum_free[gwdata->gwnum_free_count++] = q;
	}

	while (gwdata->array_list != NULL) {
		gwarray array = (gwarray) gwdata->array_list;

		gwarray_header *array_header = (gwarray_header *) ((char *) array - sizeof (gwarray_header));
		*array_header->prev = array_header->next;	
		if (array_header->next != NULL) {		
			gwarray_header *next_header = (gwarray_header *) ((char *) array_header->next - sizeof (gwarray_header));
			next_header->prev = array_header->prev;
		}

		int freeable = (int) (intptr_t) array[-3];		
		size_t array_header_size = 3 * sizeof (void *);
		if (freeable & GWFREE_LARGE_PAGES) large_pages_free ((char *) array - array_header_size);
		else free ((char *) array - array_header_size);
	}

	gwmutex_unlock (&gwdata->alloc_lock);
}

unsigned long addr_offset (
	gwhandle *gwdata,	
	unsigned long i)
{
	unsigned long fftlen;
	unsigned long addr, i1, i2, i3, i6;

	ASSERTG (i < gwdata->FFTLEN);

	fftlen = gwdata->FFTLEN;

	if (gwdata->cpu_flags & CPU_AVX512F) {

		if (gwdata->PASS1_SIZE == 0) {
			unsigned long top_bit, over64;
			top_bit = i / (fftlen >> 1); i -= top_bit * (fftlen >> 1); 
			over64 = i / 64; i -= over64 * 64;
			addr = (over64 << 7) + ((i & 7) << 4) + (top_bit << 3) + (i >> 3);
			addr = addr * sizeof (double);
			
			if (gwdata->FOURKBGAPSIZE) addr = addr + (addr / (gwdata->FOURKBGAPSIZE << 6)) * 64;
		}

		else {
			unsigned long top_bit, grp, row;
			top_bit = i / (fftlen >> 1); i -= top_bit * (fftlen >> 1); 
			i1 = i % gwdata->PASS2_SIZE;	
			i = i / gwdata->PASS2_SIZE;	
			grp = (i >> 3) * 8 + (i1 & 7);	
			row = grp * (gwdata->PASS2_SIZE >> 3) + (i1 >> 3); 
			addr = (row << 4) + (top_bit << 3) + (i & 7);
			addr = addr * sizeof (double);
			
			addr = addr + (addr >> 12) * gwdata->FOURKBGAPSIZE + grp * gwdata->PASS2GAPSIZE;
		}
	}

	else if (gwdata->cpu_flags & CPU_AVX) {

		if (gwdata->PASS2_SIZE == 0) {
			unsigned long top5bits;
			top5bits = i / (fftlen >> 5); i -= top5bits * (fftlen >> 5);
			addr = ((top5bits >> 2) & 3) * (fftlen >> 2) + (i << 3) + ((top5bits >> 4) << 2) + (top5bits & 3);
			addr = addr * sizeof (double);
			
			if (gwdata->FOURKBGAPSIZE)
				addr = addr + (addr / (gwdata->FOURKBGAPSIZE << 6)) * 64;
		}

		else {
			unsigned long top_bit, grps, row;
			top_bit = (i << 1) / fftlen; i -= (top_bit * fftlen) >> 1;
			i1 = i % gwdata->PASS2_SIZE; i = i / gwdata->PASS2_SIZE;
			grps = (i >> 2) * 4 + (i1 & 3);
			row = grps * (gwdata->PASS2_SIZE >> 2) + (i1 >> 2);
			addr = (row << 3) + (top_bit << 2) + (i & 3);
			addr = addr * sizeof (double);
			
			addr = addr + (addr >> 12) * gwdata->FOURKBGAPSIZE + grps * gwdata->PASS2GAPSIZE;
		}
	}

	else if (gwdata->cpu_flags & CPU_SSE2) {
		unsigned long sets, pfa, temp;

		if (gwdata->PASS2_SIZE == 0) {
			sets = fftlen >> 3;
			if (i >= (fftlen >> 1)) {
				i6 = 1;
				i -= (fftlen >> 1);
			} else
				i6 = 0;
			i1 = i & 1; i >>= 1;
			i3 = 0;
			for (pfa = sets; pfa > 8; pfa >>= 1);
			if (pfa == 5) {
				temp = sets / 5;
				if (i < temp * 2) {
					sets = temp;
				} else {
					i3 = temp; i -= temp * 2;
					sets = temp * 4;
				}
			} else if (pfa == 7) {
				temp = sets / 7;
				if (i < temp * 2) {
					sets = temp;
				} else if (i < temp * 6) {
					i3 = temp; i -= temp * 2;
					sets = temp * 2;
				} else {
					i3 = temp * 3; i -= temp * 6;
					sets = temp * 4;
				}
			}
			i3 += i % sets; i /= sets;
			addr = (((((i3 << 1) + i6) << 1) + i1) << 1) + i;
			addr = addr * sizeof (double);
		}

		else if (gwdata->FFT_TYPE == FFT_TYPE_HOME_GROWN) {
			sets = (fftlen / gwdata->PASS2_SIZE) >> 2;
			if (i >= (fftlen >> 1)) {
				i6 = 1;
				i -= (fftlen >> 1);
			} else
				i6 = 0;
			i1 = i % (gwdata->PASS2_SIZE >> 1);
			i = i / (gwdata->PASS2_SIZE >> 1);
			i2 = i & 1; i >>= 1;
			i3 = 0;
			for (pfa = sets; pfa > 8; pfa >>= 1);
			if (pfa == 5) {
				temp = sets / 5;
				if (i < temp * 2) {
					sets = temp;
				} else {
					i3 = temp; i -= temp * 2;
					sets = temp * 4;
				}
			} else if (pfa == 7) {
				temp = sets / 7;
				if (i < temp * 2) {
					sets = temp;
				} else if (i < temp * 6) {
					i3 = temp; i -= temp * 2;
					sets = temp * 2;
				} else {
					i3 = temp * 3; i -= temp * 6;
					sets = temp * 4;
				}
			}
			i3 += i % sets; i /= sets;
			addr = i3 * (gwdata->PASS2_SIZE >> 1);
			addr = ((((((addr + i1) << 1) + i6) << 1) + i) << 1) + i2;
			addr = addr * sizeof (double);
			
			addr = addr + (addr >> 13) * 128 + i3 * gwdata->PASS2GAPSIZE;
		}

		else {
			unsigned long top2, row;
			top2 = (i << 2) / fftlen; i -= (top2 * fftlen) >> 2;
			i1 = i % (gwdata->PASS2_SIZE >> 1); i = i / (gwdata->PASS2_SIZE >> 1);
			i2 = i & 1; i >>= 1;
			row = i * (gwdata->PASS2_SIZE >> 1) + i1;
			addr = (((row << 2) + top2) << 1) + i2;
			addr = addr * sizeof (double);
			
			addr = addr + (addr >> 13) * 128 + i * gwdata->PASS2GAPSIZE;
		}
	}

	else if (gwdata->PASS2_SIZE == 0) {
		if (i >= (fftlen >> 1)) {
			i2 = 1;
			i -= (fftlen >> 1);
		} else
			i2 = 0;
		addr = i * 16 + i2 * 8;
	}

	else {
		if (i >= (fftlen >> 1)) {
			i2 = 1;
			i -= (fftlen >> 1);
		} else
			i2 = 0;
		addr = i * 16 + i2 * 8 + (i >> 8) * 64 + (i / gwdata->PASS2_SIZE) * 64;
	}

	return (addr);
}

double *addr (
	gwhandle *gwdata,	
	gwnum	g,
	unsigned long i)
{
	return ((double *) ((char *) g + addr_offset (gwdata, i)));
}

#define AVX512_ONE_PASS		0
#define AVX512_TWO_PASS		1
#define AVX_ONE_PASS		2
#define AVX_TWO_PASS		3
#define	SSE2_ONE_PASS		4
#define SSE2_HG_TWO_PASS	5
#define SSE2_TWO_PASS		6
#define X87_ONE_OR_TWO_PASS	7

void gwiter_init_write_only (	
	gwhandle *gwdata,	
	gwiter	*iter,		
	gwnum	g)		
{
	FFT_state (g) = NOT_FFTed;	
	gwiter_init_zero (gwdata, iter, g);
}

void gwiter_init_zero (		
	gwhandle *gwdata,	
	gwiter	*iter,		
	gwnum	g)		
{

	if (FFT_state (g) != NOT_FFTed) gwunfft (gwdata, g, g);

	if (gwdata->GENERAL_MMGW_MOD) gwdata = gwdata->cyclic_gwdata;
	iter->gwdata = gwdata;
	iter->g = g;
	iter->index = 0;
	gwcached_init_zero (iter->cached_data);
	iter->addr_offset = 0;
	iter->big_word = ((double) gwdata->NUM_B_PER_SMALL_WORD != gwdata->avg_num_b_per_word);

	set_cached_dwpncol_counter (iter->cached_data, 0);		
	set_cached_dwpncol_value (iter->cached_data, 0);
	clear_cached_partial_weights (iter->cached_data);		

	if (gwdata->cpu_flags & CPU_AVX512F) {
		if (gwdata->PASS1_SIZE == 0) iter->switcher = AVX512_ONE_PASS;
		else iter->switcher = AVX512_TWO_PASS;
	} else if (gwdata->cpu_flags & CPU_AVX) {
		if (gwdata->PASS1_SIZE == 0) iter->switcher = AVX_ONE_PASS;
		else iter->switcher = AVX_TWO_PASS;
	} else if (gwdata->cpu_flags & CPU_SSE2) {
		if (gwdata->PASS2_SIZE == 0) iter->switcher = SSE2_ONE_PASS;
		else if (gwdata->FFT_TYPE == FFT_TYPE_HOME_GROWN) iter->switcher = SSE2_HG_TWO_PASS;
		else iter->switcher = SSE2_TWO_PASS;
	} else 
		iter->switcher = X87_ONE_OR_TWO_PASS;

	uint32_t *ao_values = iter->ao_values;
	memset (ao_values, 0, 9 * sizeof (uint32_t));
	switch (iter->switcher) {
	case AVX512_TWO_PASS:
		
		ao_values[0] = gwdata->PASS2_SIZE*16 + (gwdata->PASS2_SIZE >> 8) * gwdata->FOURKBGAPSIZE + gwdata->PASS2GAPSIZE;
		break;
	case AVX_ONE_PASS:
		ao_values[4] = gwdata->FOURKBGAPSIZE ? gwdata->FOURKBGAPSIZE : gwdata->FFTLEN/2/4/4;
		break;
	case AVX_TWO_PASS:
		
		ao_values[0] = gwdata->PASS2_SIZE*16 + (gwdata->PASS2_SIZE >> 8) * gwdata->FOURKBGAPSIZE + gwdata->PASS2GAPSIZE;
		break;
	case SSE2_ONE_PASS: {
		uint32_t num_cache_lines = gwdata->FFTLEN/8;
		uint32_t pfa;
		for (pfa = num_cache_lines; pfa >= 8; pfa >>= 1);	
		if (pfa == 4 || pfa == 6) ao_values[4] = 1, ao_values[5] = num_cache_lines;
		if (pfa == 5) ao_values[4] = 2, ao_values[5] = num_cache_lines/5, ao_values[6] = 4*num_cache_lines/5;
		if (pfa == 7) ao_values[4] = 3, ao_values[5] = num_cache_lines/7, ao_values[6] = 2*num_cache_lines/7, ao_values[7] = 4*num_cache_lines/7;
		break; }
	case SSE2_HG_TWO_PASS: {
		uint32_t num_cache_lines = gwdata->FFTLEN/gwdata->PASS2_SIZE/4;
		uint32_t pfa;
		for (pfa = num_cache_lines; pfa >= 8; pfa >>= 1);	
		if (pfa == 4 || pfa == 6) ao_values[5] = 1, ao_values[6] = num_cache_lines;
		if (pfa == 5) ao_values[5] = 2, ao_values[6] = num_cache_lines/5, ao_values[7] = 4*num_cache_lines/5;
		if (pfa == 7) ao_values[5] = 3, ao_values[6] = num_cache_lines/7, ao_values[7] = 2*num_cache_lines/7, ao_values[8] = 4*num_cache_lines/7;
		break; }
	}
}

void gwiter_init (		
	gwhandle *gwdata,	
	gwiter	*iter,		
	gwnum	g,			  
	uint32_t n)		
{
	ASSERTG (n < gwdata->FFTLEN);

	gwiter_init_zero (gwdata, iter, g); 
	gwdata = iter->gwdata;

	iter->index = n;
	gwcached_init (gwdata->dd_data, iter->cached_data, n);
	iter->addr_offset = addr_offset (gwdata, n);
	iter->big_word = gwcached_is_big_word (gwdata->dd_data, iter->cached_data);

	uint32_t *ao_values = iter->ao_values;
	uint32_t ntmp = n;
	switch (iter->switcher) {
	case AVX512_ONE_PASS:
		ao_values[0] = ntmp % 8; ntmp /= 8;
		ao_values[1] = ntmp % 8; ntmp /= 8;
		ao_values[2] = ntmp % (gwdata->FFTLEN/2/8/8);
		break;
	case AVX512_TWO_PASS:
		ao_values[1] = ntmp % 8; ntmp /= 8;
		ao_values[2] = ntmp % (gwdata->PASS2_SIZE/8); ntmp /= (gwdata->PASS2_SIZE/8);
		ao_values[3] = ntmp % 8; ntmp /= 8;
		ao_values[4] = ntmp % (gwdata->FFTLEN/2/(gwdata->PASS2_SIZE*8));
		break;
	case AVX_ONE_PASS:
		ao_values[0] = ntmp % ao_values[4]; ntmp /= ao_values[4];
		ao_values[1] = ntmp % (gwdata->FFTLEN/2/4/4/ao_values[4]); ntmp /= (gwdata->FFTLEN/2/4/4/ao_values[4]);
		ao_values[2] = ntmp % 4; ntmp /= 4;
		ao_values[3] = ntmp % 4;
		break;
	case AVX_TWO_PASS:
		if (gwdata->FFT_TYPE == FFT_TYPE_RADIX_4_DWPN) {
			set_cached_dwpncol_counter (iter->cached_data, n % gwdata->PASS2_SIZE);
			set_cached_dwpncol_value (iter->cached_data, dwpn_col (gwdata, n));
			clear_cached_partial_weights (iter->cached_data);		
		}
		ao_values[1] = ntmp % 4; ntmp /= 4;
		ao_values[2] = ntmp % (gwdata->PASS2_SIZE/4); ntmp /= (gwdata->PASS2_SIZE/4);
		ao_values[3] = ntmp % 4; ntmp /= 4;
		ao_values[4] = ntmp % (gwdata->FFTLEN/2/(gwdata->PASS2_SIZE*4));
		break;
	case SSE2_ONE_PASS:
		ntmp = ntmp % (gwdata->FFTLEN/2);
		ao_values[0] = ntmp % 2; ntmp /= 2;
		if (ntmp < 2*ao_values[5]) {		
			ao_values[1] = ntmp % ao_values[5]; ntmp = ntmp / ao_values[5];
			ao_values[3] = 0;
		} else {
			ntmp -= 2*ao_values[5];
			if (ntmp < 2*ao_values[6]) {	
				ao_values[1] = ntmp % ao_values[6]; ntmp = ntmp / ao_values[6];
				ao_values[3] = 1;
			} else {			
				ntmp -= 2*ao_values[6];
				ao_values[1] = ntmp % ao_values[7]; ntmp = ntmp / ao_values[7];
				ao_values[3] = 2;
			}
		}
		ao_values[2] = ntmp;
		break;
	case SSE2_HG_TWO_PASS: {
		uint32_t num_cache_lines = gwdata->FFTLEN/gwdata->PASS2_SIZE/4;
		uint32_t pfa;
		for (pfa = num_cache_lines; pfa >= 8; pfa >>= 1);	
		if (pfa == 4 || pfa == 6) ao_values[5] = 1, ao_values[6] = num_cache_lines;
		if (pfa == 5) ao_values[5] = 2, ao_values[6] = num_cache_lines/5, ao_values[7] = 4*num_cache_lines/5;
		if (pfa == 7) ao_values[5] = 3, ao_values[6] = num_cache_lines/7, ao_values[7] = 2*num_cache_lines/7, ao_values[8] = 4*num_cache_lines/7;
		ntmp = ntmp % (gwdata->FFTLEN/2);
		ao_values[0] = ntmp % (gwdata->PASS2_SIZE/2); ntmp /= (gwdata->PASS2_SIZE/2);
		ao_values[1] = ntmp % 2; ntmp /= 2;
		if (ntmp < 2*ao_values[6]) {		
			ao_values[2] = ntmp % ao_values[6]; ntmp = ntmp / ao_values[6];
			ao_values[4] = 0;
		} else {
			ntmp -= 2*ao_values[6];
			if (ntmp < 2*ao_values[7]) {	
				ao_values[2] = ntmp % ao_values[7]; ntmp = ntmp / ao_values[7];
				ao_values[4] = 1;
			} else {			
				ntmp -= 2*ao_values[7];
				ao_values[2] = ntmp % ao_values[8]; ntmp = ntmp / ao_values[8];
				ao_values[4] = 2;
			}
		}
		ao_values[3] = ntmp;
		break; }
	case SSE2_TWO_PASS:
		if (gwdata->FFT_TYPE == FFT_TYPE_RADIX_4_DWPN) {
			set_cached_dwpncol_counter (iter->cached_data, n % (gwdata->PASS2_SIZE/2));
			set_cached_dwpncol_value (iter->cached_data, dwpn_col (gwdata, n));
			clear_cached_partial_weights (iter->cached_data);		
		}
		ao_values[0] = ntmp % 128; ntmp /= 128;
		ao_values[1] = ntmp % (gwdata->PASS2_SIZE/2/128); ntmp /= (gwdata->PASS2_SIZE/2/128);
		ao_values[2] = ntmp % 2; ntmp /= 2;
		ao_values[3] = ntmp % (gwdata->FFTLEN/4/gwdata->PASS2_SIZE);
		break;
	}
}

void gwiter_next (		
	gwiter	*iter)		
{
	gwhandle *gwdata = iter->gwdata;	
	ASSERTG (iter->index < gwdata->FFTLEN);

	if (++iter->index < gwdata->FFTLEN) {

		gwcached_next (gwdata->dd_data, iter->cached_data, iter->index);
		iter->big_word = gwcached_is_big_word (gwdata->dd_data, iter->cached_data);

		uint32_t *ao_values = iter->ao_values;
		switch (iter->switcher) {
		case AVX512_ONE_PASS:
			iter->addr_offset += 128;
			if (++ao_values[0] == 8) { ao_values[0] = 0; iter->addr_offset -= 8*128;
			iter->addr_offset += 8;
			if (++ao_values[1] == 8) { ao_values[1] = 0; iter->addr_offset -= 8*8;
			iter->addr_offset += 1024 + ((++ao_values[2] & 3) == 0 ? gwdata->FOURKBGAPSIZE : 0);
			if (ao_values[2] == gwdata->FFTLEN/2/8/8) { ao_values[2] = 0; iter->addr_offset = 64;}}}
			break;
		case AVX512_TWO_PASS: {
			uint32_t dist1 = ao_values[0];			
			iter->addr_offset += dist1;
			if (++ao_values[1] == 8) { ao_values[1] = 0; iter->addr_offset -= 8*dist1;
			iter->addr_offset += 128 + ((++ao_values[2] & 31) == 0 ? gwdata->FOURKBGAPSIZE : 0);
			if (ao_values[2] == gwdata->PASS2_SIZE/8) { ao_values[2] = 0; iter->addr_offset -= gwdata->PASS2_SIZE/8*128 + gwdata->PASS2_SIZE/8/32*gwdata->FOURKBGAPSIZE;
			iter->addr_offset += 8;
			if (++ao_values[3] == 8) { ao_values[3] = 0; iter->addr_offset -= 8*8;
			iter->addr_offset += 8*dist1;
			if (++ao_values[4] == gwdata->FFTLEN/2/(gwdata->PASS2_SIZE*8)) { ao_values[4] = 0; iter->addr_offset = 64;}}}}
			break; }
		case AVX_ONE_PASS: {
			uint32_t count1 = ao_values[4];			
			iter->addr_offset += 64;
			if (++ao_values[0] == count1) { ao_values[0] = 0;
			iter->addr_offset += 64;			
			uint32_t count2 = gwdata->FOURKBGAPSIZE ? gwdata->FFTLEN/2/4/4/count1 : 1;
			if (++ao_values[1] == count2) { ao_values[1] = 0; iter->addr_offset -= gwdata->FFTLEN/2/4/4*64 + count2*64;
			iter->addr_offset += 8;
			if (++ao_values[2] == 4) { ao_values[2] = 0; iter->addr_offset -= 4*8;
			iter->addr_offset += gwdata->FFTLEN/2/4/4*64 + (gwdata->FOURKBGAPSIZE ? gwdata->FFTLEN/2/4/4/gwdata->FOURKBGAPSIZE*64 : 0);
			if (++ao_values[3] == 4) { ao_values[2] = 0; iter->addr_offset = 32;}}}}
			break; }
		case AVX_TWO_PASS: {
			
			if (gwdata->FFT_TYPE == FFT_TYPE_RADIX_4_DWPN) {
				inc_cached_dwpncol_counter (iter->cached_data);
				inc_cached_dwpncol_value (iter->cached_data);
				if (get_cached_dwpncol_counter (iter->cached_data) == gwdata->PASS2_SIZE) {
					set_cached_dwpncol_counter (iter->cached_data, 0);
					set_cached_dwpncol_value (iter->cached_data, dwpn_col (gwdata, iter->index));
					clear_cached_partial_weights (iter->cached_data);		
				}
			}
			
			uint32_t dist1 = ao_values[0];			
			iter->addr_offset += dist1;
			if (++ao_values[1] == 4) { ao_values[1] = 0; iter->addr_offset -= 4*dist1;
			iter->addr_offset += 64 + ((++ao_values[2] & 63) == 0 ? gwdata->FOURKBGAPSIZE : 0);
			if (ao_values[2] == gwdata->PASS2_SIZE/4) { ao_values[2] = 0; iter->addr_offset -= gwdata->PASS2_SIZE/4*64 + gwdata->PASS2_SIZE/4/64*gwdata->FOURKBGAPSIZE;
			iter->addr_offset += 8;
			if (++ao_values[3] == 4) { ao_values[3] = 0; iter->addr_offset -= 4*8;
			iter->addr_offset += 4*dist1;
			if (++ao_values[4] == gwdata->FFTLEN/2/(gwdata->PASS2_SIZE*4)) { ao_values[4] = 0; iter->addr_offset = 32;}}}}
			break; }
		case SSE2_ONE_PASS: {
			iter->addr_offset += 16;
			if (++ao_values[0] == 2) { ao_values[0] = 0; iter->addr_offset -= 2*16;
			iter->addr_offset += 64;
			uint32_t pfa_section_number = ao_values[3];
			uint32_t count = ao_values[5+pfa_section_number];
			if (++ao_values[1] == count) { ao_values[1] = 0; iter->addr_offset -= count*64;
			iter->addr_offset += 8;
			if (++ao_values[2] == 2) { ao_values[2] = 0; iter->addr_offset -= 2*8;
			iter->addr_offset += count*64;
			uint32_t num_pfa_sections = ao_values[4];
			if (++ao_values[3] == num_pfa_sections) { ao_values[3] = 0; iter->addr_offset = 32;}}}}
			break; }
		case SSE2_HG_TWO_PASS: {					
			iter->addr_offset += 64 + ((++ao_values[0] & 127) == 0 ? 128 : 0);	
			if (ao_values[0] == gwdata->PASS2_SIZE/2) { ao_values[0] = 0; iter->addr_offset -= gwdata->PASS2_SIZE/2*64 + gwdata->PASS2_SIZE/2/128*128;
			iter->addr_offset += 8;
			if (++ao_values[1] == 2) { ao_values[1] = 0; iter->addr_offset -= 2*8;
			uint32_t dist = gwdata->PASS2_SIZE/2*64 + gwdata->PASS2_SIZE/2/128*128 + gwdata->PASS2GAPSIZE;
			iter->addr_offset += dist;
			uint32_t pfa_section_number = ao_values[4];
			uint32_t count = ao_values[6+pfa_section_number];
			if (++ao_values[2] == count) { ao_values[2] = 0; iter->addr_offset -= count*dist;
			iter->addr_offset += 16;
			if (++ao_values[3] == 2) { ao_values[3] = 0; iter->addr_offset -= 2*16;
			iter->addr_offset += count*dist;
			uint32_t num_pfa_sections = ao_values[5];
			if (++ao_values[4] == num_pfa_sections) { ao_values[4] = 0; iter->addr_offset = 32;}}}}}
			break; }
		case SSE2_TWO_PASS: {
			
			if (gwdata->FFT_TYPE == FFT_TYPE_RADIX_4_DWPN) {
				inc_cached_dwpncol_counter (iter->cached_data);
				inc_cached_dwpncol_value (iter->cached_data);
				if (get_cached_dwpncol_counter (iter->cached_data) == gwdata->PASS2_SIZE / 2) {
					set_cached_dwpncol_counter (iter->cached_data, 0);
					set_cached_dwpncol_value (iter->cached_data, dwpn_col (gwdata, iter->index));
					clear_cached_partial_weights (iter->cached_data);		
				}
			}
			
			ASSERTG ((gwdata->PASS2_SIZE/2) % 128 == 0);
			iter->addr_offset += 64;
			if (++ao_values[0] == 128) { ao_values[0] = 0;
			iter->addr_offset += 128;						
			uint32_t count1 = gwdata->PASS2_SIZE/2/128;
			if (++ao_values[1] == count1) { ao_values[1] = 0; iter->addr_offset -= gwdata->PASS2_SIZE/2*64 + count1*128;
			iter->addr_offset += 8;
			if (++ao_values[2] == 2) { ao_values[2] = 0; iter->addr_offset -= 2*8;
			uint32_t dist = gwdata->PASS2_SIZE/2*64 + gwdata->PASS2_SIZE/2/128*128 + gwdata->PASS2GAPSIZE;
			iter->addr_offset += dist;
			uint32_t count3 = gwdata->FFTLEN/4/gwdata->PASS2_SIZE;
			if (++ao_values[3] == count3) { ao_values[3] = 0; iter->addr_offset -= count3*dist;
			iter->addr_offset += 16;}}}}
			break; }
		case X87_ONE_OR_TWO_PASS:
			
			iter->addr_offset = addr_offset (gwdata, iter->index);
			break;
		}
	}
}

unsigned long gwmemused (
	gwhandle *gwdata)	
{
	if (!gwdata->GENERAL_MMGW_MOD) return (gwdata->mem_needed + gwdata->SCRATCH_SIZE);
	return (gwmemused (gwdata->cyclic_gwdata) + gwmemused (gwdata->negacyclic_gwdata));
}

unsigned long gwnum_size (
	gwhandle *gwdata)	
{
	return (gwnum_datasize (gwdata) + GW_HEADER_SIZE (gwdata) + sizeof (void *) + gwdata->GW_ALIGNMENT + 2 * sizeof (void *));
}

int get_fft_value (
	gwhandle *gwdata,	
	gwnum	g,
	unsigned long i,
	long	*retval)
{
	double	val, *valaddr;

	if (FFT_state (g) != NOT_FFTed) gwunfft (gwdata, g, g);

	valaddr = addr (gwdata, g, i);
	if (! is_valid_double_addr (valaddr)) return (GWERROR_BAD_FFT_DATA);
	val = *valaddr;

	if (!gwdata->RATIONAL_FFT && !(gwdata->cpu_flags & CPU_AVX512F)) {

		if (gwdata->FFT_TYPE == FFT_TYPE_RADIX_4_DWPN)
			val = val * gwfft_partial_weight_inverse_sloppy (gwdata->dd_data, i, dwpn_col (gwdata, i));

		else
			val = val * gwfft_weight_inverse_sloppy (gwdata->dd_data, i);
	}

	round_double_to_int32 (val, *retval);

	return (0);
}

int gwiter_get_fft_value (			
	gwiter	*iter,
	int32_t	*retval)
{
	gwhandle *gwdata = iter->gwdata;	
	gwnum	g = iter->g;
	double	val, *valaddr;

	ASSERTG (FFT_state (g) == NOT_FFTed);
	ASSERTG (iter->index < gwdata->FFTLEN);

	valaddr = gwiter_addr (iter);
	if (! is_valid_double_addr (valaddr)) return (GWERROR_BAD_FFT_DATA);
	val = *valaddr;

	if (!gwdata->RATIONAL_FFT && !(gwdata->cpu_flags & CPU_AVX512F)) {

		if (gwdata->FFT_TYPE == FFT_TYPE_RADIX_4_DWPN)
			val = val * gwcached_partial_weight_inverse_sloppy (gwdata->dd_data, iter->cached_data, iter->index);

		else
			val = val * gwcached_weight_inverse_sloppy (gwdata->dd_data, iter->cached_data, iter->index);
	}

	round_double_to_int32 (val, *retval);

	return (0);
}

void set_fft_value (
	gwhandle *gwdata,	
	gwnum	g,
	unsigned long i,
	long	val)
{

	if (gwdata->RATIONAL_FFT || (gwdata->cpu_flags & CPU_AVX512F) || val == 0) {
		* addr (gwdata, g, i) = val;
		return;
	}

	if (gwdata->FFT_TYPE == FFT_TYPE_RADIX_4_DWPN) {
		* addr (gwdata, g, i) = val * gwfft_partial_weight_sloppy (gwdata->dd_data, i, dwpn_col (gwdata, i));
		return;
	}

	* addr (gwdata, g, i) = val * gwfft_weight_sloppy (gwdata->dd_data, i);
}

void gwiter_set_fft_value (		
	gwiter	*iter,
	int32_t	val)
{
	gwhandle *gwdata = iter->gwdata;
	gwnum	g = iter->g;
	double *valaddr = gwiter_addr (iter);

	ASSERTG (iter->index < gwdata->FFTLEN);

	if (gwdata->RATIONAL_FFT || (gwdata->cpu_flags & CPU_AVX512F) || val == 0)
		*valaddr = val;

	else if (gwdata->FFT_TYPE == FFT_TYPE_RADIX_4_DWPN)
		*valaddr = val * gwcached_partial_weight_sloppy (gwdata->dd_data, iter->cached_data, iter->index);

	else
		*valaddr = gwcached_weight_sloppy (gwdata->dd_data, iter->cached_data, iter->index, val);
}

int is_big_word (
	gwhandle *gwdata,	
	unsigned long i)
{
	unsigned long base, next_base;

	base = gwfft_base (gwdata->dd_data, i);
	next_base = gwfft_base (gwdata->dd_data, i+1);
	return ((next_base - base) > gwdata->NUM_B_PER_SMALL_WORD);
}

void bitaddr (
	gwhandle *gwdata,	
	unsigned long bit,
	unsigned long *word,
	unsigned long *bit_in_word)
{

	*word = (unsigned long) ((double) bit / gwdata->avg_num_b_per_word);
	if (*word >= gwdata->FFTLEN) *word = gwdata->FFTLEN - 1;

	unsigned long base = gwfft_base (gwdata->dd_data, *word);
	if (base > bit) *word -= 1, base = gwfft_base (gwdata->dd_data, *word);

	*bit_in_word = bit - base;
}

void gwerror_text (
	gwhandle *gwdata,	
	int	error_code,	
	char	*buf,		
	int	buflen)		
{
	char	localbuf[512];

	switch (error_code) {
	case GWERROR_VERSION:
		strcpy (localbuf, "Improperly compiled and linked.  Gwnum.h and FFT assembly code version numbers do not match.");
		break;
	case GWERROR_TOO_SMALL:
		strcpy (localbuf, "Number sent to gwsetup is less than or equal to one.");
		break;
	case GWERROR_TOO_LARGE:
		strcpy (localbuf, "Number sent to gwsetup is too large for the FFTs to handle.");
		break;
	case GWERROR_K_TOO_SMALL:
		strcpy (localbuf, "Value of k in k*b^n+c is too small.  Values less than one are not supported.");
		break;
	case GWERROR_K_TOO_LARGE:
		strcpy (localbuf, "Value of k in k*b^n+c is too large.  Values greater than 2251799813685247 are not supported.");
		break;
	case GWERROR_MALLOC:
		strcpy (localbuf, "Unable to allocate memory.  One possible cause is the operating system's swap area is too small.");
		break;
	case GWERROR_VERSION_MISMATCH:
		strcpy (localbuf, "GWNUM_VERSION from gwinit call doesn't match GWNUM_VERSION when gwnum.c was compiled.  Recompile and relink.");
		break;
	case GWERROR_STRUCT_SIZE_MISMATCH:
		strcpy (localbuf, "Gwhandle structure size from gwinit call doesn't match size when gwnum.c was compiled.  Check compiler alignment switches, recompile and relink.");
		break;
	case GWERROR_BAD_FFT_DATA:
		strcpy (localbuf, "Gwnum had corrupted data such as Nan or Inf.");
		break;
	default:
		if (error_code >= GWERROR_INTERNAL && error_code <= GWERROR_INTERNAL+100)
			sprintf (localbuf, "Internal error #%d.  Please contact the program's author.", error_code - GWERROR_INTERNAL);
		else
			sprintf (localbuf, "Unknown gwnum error code: %d", error_code);
		break;
	}

	if ((int) strlen (localbuf) >= buflen) localbuf[buflen-1] = 0;
	strcpy (buf, localbuf);
}

void gwfft_description (
	gwhandle *gwdata,	
	char	*buf)		
{
	char	*arch, *ffttype;

	gwhandle *fft_gwdata = (gwdata->GENERAL_MMGW_MOD ? gwdata->cyclic_gwdata : gwdata);

	arch = "";
	if (fft_gwdata->cpu_flags & CPU_AVX512F) {
		
	}
	else if (fft_gwdata->cpu_flags & CPU_AVX) {
		
	}
	else if (fft_gwdata->cpu_flags & CPU_SSE2) {
		
		if (fft_gwdata->ARCH == ARCH_P4 || fft_gwdata->ARCH == ARCH_P4TP) arch = "Pentium4 ";
		if (fft_gwdata->ARCH == ARCH_K8) arch = "AMD K8 ";
		if (fft_gwdata->ARCH == ARCH_K10) arch = "AMD K10 ";
	}

	ffttype = "";
	if (fft_gwdata->PASS2_SIZE) {
		if (fft_gwdata->cpu_flags & (CPU_AVX512F | CPU_AVX)) {
			
		}
		else if (fft_gwdata->cpu_flags & CPU_SSE2) {
			
			if (fft_gwdata->FFT_TYPE == 0) ffttype = "type-0 ";
			if (fft_gwdata->FFT_TYPE == 1) ffttype = "type-1 ";
			if (fft_gwdata->FFT_TYPE == 2) ffttype = "type-2 ";
		}
	}

	sprintf (buf, "%s%s%s%s%sFFT length %s%lu%s",
		 gwdata->GENERAL_MOD ? "Barrett reduction " :
		 gwdata->GENERAL_MMGW_MOD ? "Montgomery reduction " : "",
		 fft_gwdata->NEGACYCLIC_FFT ? "negacyclic " :
		 fft_gwdata->ZERO_PADDED_FFT ? "zero-padded " : "",
		 (fft_gwdata->cpu_flags & CPU_AVX512F) ? "AVX-512 " :
		 (fft_gwdata->ARCH == ARCH_FMA3) ? "FMA3 " :
		 (fft_gwdata->cpu_flags & CPU_AVX) ? "AVX " :
		 (fft_gwdata->cpu_flags & CPU_SSE2) ? "" : "x87 ",
		 arch, ffttype,
		 gwdata->GENERAL_MMGW_MOD ? "2x" : "",
		 fft_gwdata->FFTLEN >= 1048576 && (fft_gwdata->FFTLEN & 0xFFFFF) == 0 ? fft_gwdata->FFTLEN / 1048576 :
		 fft_gwdata->FFTLEN >= 1024 && (fft_gwdata->FFTLEN & 0x3FF) == 0 ? fft_gwdata->FFTLEN / 1024 : fft_gwdata->FFTLEN,
		 fft_gwdata->FFTLEN >= 1048576 && (fft_gwdata->FFTLEN & 0xFFFFF) == 0 ? "M" :
		 fft_gwdata->FFTLEN >= 1024 && (fft_gwdata->FFTLEN & 0x3FF) == 0 ? "K" : "");

	if (fft_gwdata->PASS1_SIZE) {
		int	clm;
		char	p1buf[20], p2buf[20];

		if (fft_gwdata->cpu_flags & CPU_AVX512F) clm = fft_gwdata->PASS1_CACHE_LINES / 8;
		else if (fft_gwdata->cpu_flags & CPU_AVX) clm = fft_gwdata->PASS1_CACHE_LINES / 4;
		else clm = fft_gwdata->PASS1_CACHE_LINES / 2;
		if (fft_gwdata->PASS1_SIZE % 1024) sprintf (p1buf, "%d", (int) fft_gwdata->PASS1_SIZE);
		else sprintf (p1buf, "%dK", (int) (fft_gwdata->PASS1_SIZE / 1024));
		if (fft_gwdata->PASS2_SIZE % 1024) sprintf (p2buf, "%d", (int) fft_gwdata->PASS2_SIZE);
		else sprintf (p2buf, "%dK", (int) (fft_gwdata->PASS2_SIZE / 1024));
		sprintf (buf + strlen (buf), ", Pass1=%s, Pass2=%s, clm=%d", p1buf, p2buf, clm);
	}

	if (fft_gwdata->num_threads > 1)
		sprintf (buf + strlen (buf), ", %d threads", (int) fft_gwdata->num_threads);

	if (gw_using_large_pages (gwdata))
		strcat (buf, " using large pages");
}

void gw_as_string (
	char	*buf,		
	double	k,		
	unsigned long b,	
	unsigned long n,	
	signed long c)		
{
	if (n == 0)
		sprintf (buf, "%.0f", k + c);
	else if (k != 1.0)
		sprintf (buf, "%.0f*%lu^%lu%c%lu", k, b, n, c < 0 ? '-' : '+', (unsigned long) labs (c));
	else if (b == 2 && c == -1)
		sprintf (buf, "M%lu", n);
	else {
		unsigned long cnt, temp_n;
		for (cnt = 0, temp_n = n; !(temp_n & 1); temp_n >>= 1, cnt++);
		if (b == 2 && temp_n == 1 && c == 1)
			sprintf (buf, "F%lu", cnt);
		else
			sprintf (buf, "%lu^%lu%c%lu", b, n, c < 0 ? '-' : '+', (unsigned long) labs (c));
	}
}

void gwerror_checking (
	gwhandle *gwdata,
	int	e)		
{
	if (gwdata->GENERAL_MMGW_MOD) {
		gwerror_checking (gwdata->cyclic_gwdata, e);
		gwerror_checking (gwdata->negacyclic_gwdata, e);
	}
	gwdata->ERROR_CHECKING = e ? 1 : 0;
}
double gw_get_maxerr (
	gwhandle *gwdata)
{
	if (!gwdata->GENERAL_MMGW_MOD) return (((struct gwasm_data *) gwdata->asm_data)->MAXERR);
	double err1 = gw_get_maxerr (gwdata->cyclic_gwdata);
	double err2 = gw_get_maxerr (gwdata->negacyclic_gwdata);
	return (err1 > err2 ? err1 : err2);
}
void gw_clear_maxerr (
	gwhandle *gwdata)
{
	if (!gwdata->GENERAL_MMGW_MOD) ((struct gwasm_data *) gwdata->asm_data)->MAXERR = 0.0;
	else gw_clear_maxerr (gwdata->cyclic_gwdata), gw_clear_maxerr (gwdata->negacyclic_gwdata);
}

uint64_t gw_get_fft_count (
	gwhandle *gwdata)
{
	if (gwdata->GENERAL_MMGW_MOD) return (gw_get_fft_count (gwdata->cyclic_gwdata) + gw_get_fft_count (gwdata->negacyclic_gwdata));
	return (gwdata->fft_count);
}
void gw_clear_fft_count (
	gwhandle *gwdata)
{
	if (gwdata->GENERAL_MMGW_MOD) {
		gw_clear_fft_count (gwdata->cyclic_gwdata);
		gw_clear_fft_count (gwdata->negacyclic_gwdata);
	} else
		gwdata->fft_count = 0;
}

int gwnear_fft_limit (
	gwhandle *gwdata,	
	double	pct)
{

	if (!gwdata->GENERAL_MMGW_MOD) return (virtual_bits_per_word (gwdata) > (100.0 - pct) / 100.0 * gwdata->fft_max_bits_per_word);
	return (gwnear_fft_limit (gwdata->negacyclic_gwdata, pct));
}

double virtual_bits_per_word (
	gwhandle *gwdata)	
{
	double	log2b, b_per_input_word, weighted_bits_per_output_word;
	double	max_weighted_bits_per_output_word;
	int	num_b_in_big_word, num_small_words, num_big_words;

	log2b = log2 (gwdata->b);

	if (gwdata->ZERO_PADDED_FFT) {
		b_per_input_word = (double) (gwdata->n + gwdata->n) / gwdata->FFTLEN;
		num_b_in_big_word = (int) ceil (b_per_input_word);
		num_small_words = (int) ((num_b_in_big_word - b_per_input_word) * (gwdata->FFTLEN / 2 + 4));
		num_big_words = (gwdata->FFTLEN / 2 + 4) - num_small_words;
		max_weighted_bits_per_output_word =
			2.0 * gwdata->fft_max_bits_per_word + 0.6 * log2 (gwdata->FFTLEN / 2 + 4);
		weighted_bits_per_output_word =
		       2.0 * ((b_per_input_word + 1.0) * log2b - 1.0) +
		       0.6 * log2 (gwdata->FFTLEN / 2 + 4);
		if ((gwdata->n + gwdata->n) % gwdata->FFTLEN == 0)
			weighted_bits_per_output_word -= ((log2b <= 4.0) ? log2b : 1.4 * log2b);
		else if (! is_pathological_distribution (num_big_words, num_small_words))
			weighted_bits_per_output_word -=
				((log2b <= 3.0) ? (log2b - 1.0) / 2.0 :
				 (log2b <= 6.0) ? 1.0 + (log2b - 3.0) / 3.0 :
						  2.0 + (log2b - 6.0) / 6.0);
	}

	else {
		b_per_input_word = gwdata->avg_num_b_per_word;
		num_b_in_big_word = (int) ceil (b_per_input_word);
		num_small_words = (int) ((num_b_in_big_word - b_per_input_word) * gwdata->FFTLEN);
		num_big_words = gwdata->FFTLEN - num_small_words;
		max_weighted_bits_per_output_word = 2.0 * gwdata->fft_max_bits_per_word + 0.6 * log2 (gwdata->FFTLEN);
		weighted_bits_per_output_word =
			2.0 * ((b_per_input_word + 1.0) * log2b - 1.0) +
			0.6 * log2 (gwdata->FFTLEN) +
			log2 (gwdata->k) + 1.7 * log2 (labs (gwdata->c));
		if (gwdata->k == 1.0 && gwdata->n % gwdata->FFTLEN == 0)
			weighted_bits_per_output_word -= ((log2b <= 4.0) ? log2b : 1.4 * log2b);
		else if (num_big_words == 1 && gwdata->k > 1.0)
			weighted_bits_per_output_word += log2b;
		else if (! is_pathological_distribution (num_big_words, num_small_words))
			weighted_bits_per_output_word -=
				((log2b <= 3.0) ? (log2b - 1.0) / 2.0 :
				 (log2b <= 6.0) ? 1.0 + (log2b - 3.0) / 3.0 :
						  2.0 + (log2b - 6.0) / 6.0);
	}

	return (gwdata->fft_max_bits_per_word - (max_weighted_bits_per_output_word - weighted_bits_per_output_word) / 2.0);
}

void gwset_polymult_safety_margin (
	gwhandle *gwdata,	
	float	safetyval)	
{
	gwdata->polymult_safety_margin = safetyval;
	gwdata->EXTRA_BITS = (float) ((gwdata->fft_max_bits_per_word - gwdata->safety_margin - gwdata->polymult_safety_margin - virtual_bits_per_word (gwdata)) * 2.0);
	if (gwdata->EXTRA_BITS < 0.0f) gwdata->EXTRA_BITS = 0.0f;
	gwdata->EXTRA_BITS += EB_GWMUL_SAVINGS;
}

unsigned long gwmap_to_fftlen (
	double	k,		
	unsigned long b,	
	unsigned long n,	
	signed long c)		
{
	return (gwmap_with_cpu_flags_to_fftlen (0, k, b, n, c));
}

unsigned long gwmap_with_cpu_flags_to_fftlen (
	int	cpu_flags,
	double	k,		
	unsigned long b,	
	unsigned long n,	
	signed long c)		
{
	gwhandle gwdata;	

	gwinit (&gwdata);
	if (cpu_flags) gwdata.cpu_flags = cpu_flags;
	gwclear_use_benchmarks (&gwdata);
	if (gwinfo (&gwdata, k, b, n, c)) return (0);
	return (gwdata.jmptab->fftlen);
}

unsigned long gwmap_fftlen_to_max_exponent (
	unsigned long fftlen)
{
	return (gwmap_with_cpu_flags_fftlen_to_max_exponent (0, fftlen));
}

unsigned long gwmap_with_cpu_flags_fftlen_to_max_exponent (
	int	cpu_flags,
	unsigned long fftlen)
{
	gwhandle gwdata;	

	gwinit (&gwdata);
	if (cpu_flags) gwdata.cpu_flags = cpu_flags;
	gwclear_use_benchmarks (&gwdata);
	gwset_minimum_fftlen (&gwdata, fftlen);
	if (gwinfo (&gwdata, 1.0, 2, 0, -1)) return (0);
	return (adjusted_max_exponent (&gwdata, gwdata.jmptab));
}

unsigned long gwmap_to_memused (
	double	k,		
	unsigned long b,	
	unsigned long n,	
	signed long c)		
{
	gwhandle gwdata;	

	gwinit (&gwdata);
	gwclear_use_benchmarks (&gwdata);
	if (gwinfo (&gwdata, k, b, n, c)) return (100000000L);
	return (gwdata.mem_needed + gwdata.SCRATCH_SIZE);
}

unsigned long gwmap_to_estimated_size (
	double	k,		
	unsigned long b,	
	unsigned long n,	
	signed long c)		
{
	gwhandle gwdata;	

	gwinit (&gwdata);
	gwclear_use_benchmarks (&gwdata);
	if (gwinfo (&gwdata, k, b, n, c)) return (100000000L);
	return (gwnum_datasize (&gwdata));
}

#define REL_486_SPEED	8.4	
#define REL_K6_SPEED	3.0	
#define REL_P3_SPEED	0.8	
#define REL_K7_SPEED	0.6	

#define REL_AMD64_SPEED	1.1	
#define REL_PM_SPEED	1.4	
#define REL_ATOM_SPEED	5.0	
#define REL_CORE2_SPEED	0.625	
#define REL_I7_SPEED	0.59	
#define REL_PHENOM_SPEED 0.67	
#define REL_FMA3_SPEED	0.95	

#define REL_BULLDOZER_SPEED	1.9	
#define REL_ZEN_SPEED		0.8	

double gwmap_to_timing (
	double	k,		
	unsigned long b,	
	unsigned long n,	
	signed long c)		
{
	gwhandle gwdata;	
	double	timing;

	gwinit (&gwdata);
	gwclear_use_benchmarks (&gwdata);
	if (gwinfo (&gwdata, k, b, n, c)) return (100.0);

	timing = gwdata.jmptab->timing;

	if (gwdata.cpu_flags & CPU_AVX512F) {
		timing = 0.10 * timing + 0.90 * timing * 4100.0 / CPU_SPEED;		
		if (CPU_ARCHITECTURE == CPU_ARCHITECTURE_AMD_OTHER) timing *= REL_ZEN_SPEED;  
	} else if (gwdata.cpu_flags & CPU_AVX) {
		timing = 0.10 * timing + 0.90 * timing * 4100.0 / CPU_SPEED;	
		if (CPU_ARCHITECTURE == CPU_ARCHITECTURE_AMD_OTHER) timing *= REL_ZEN_SPEED;  
		else if (CPU_ARCHITECTURE == CPU_ARCHITECTURE_AMD_ZEN) timing *= REL_ZEN_SPEED;
		else if (strstr (CPU_BRAND, "AMD")) timing *= REL_BULLDOZER_SPEED;
		if (gwdata.cpu_flags & CPU_FMA3) timing *= REL_FMA3_SPEED;
	} else if (gwdata.cpu_flags & CPU_SSE2) {
		timing = 0.10 * timing + 0.90 * timing * 1400.0 / CPU_SPEED;
		if (strstr (CPU_BRAND, "Phenom")) timing *= REL_PHENOM_SPEED;
		else if (strstr (CPU_BRAND, "AMD")) timing *= REL_AMD64_SPEED;
		else if (strstr (CPU_BRAND, "Atom")) timing *= REL_ATOM_SPEED;
		else if (strstr (CPU_BRAND, "Core 2")) timing *= REL_CORE2_SPEED;
		else if (strstr (CPU_BRAND, "Core(TM)2")) timing *= REL_CORE2_SPEED;
		else if (strstr (CPU_BRAND, "Core(TM) i7")) timing *= REL_I7_SPEED;
		else if (strstr (CPU_BRAND, "Pentium(R) M")) timing *= REL_PM_SPEED;
		else if (strstr (CPU_BRAND, "Core")) timing *= REL_PM_SPEED;
	} else {
		timing = 0.10 * timing + 0.90 * timing * 400.0 / CPU_SPEED;
		if (strstr (CPU_BRAND, "486")) timing *= REL_486_SPEED;
		else if (strstr (CPU_BRAND, "Intel")) {
			if (gwdata.cpu_flags & CPU_PREFETCH) timing *= REL_P3_SPEED;
		} else if (strstr (CPU_BRAND, "AMD")) {
			if (strstr (CPU_BRAND, "Unknown")) timing *= REL_486_SPEED;
			else if (strstr (CPU_BRAND, "K5")) timing *= REL_486_SPEED;
			else if (strstr (CPU_BRAND, "K6")) timing *= REL_K6_SPEED;
			else timing *= REL_K7_SPEED;
		} else
			timing *= REL_486_SPEED;
	}
	return (timing);
}

void calc8ptrs (
	gwhandle *gwdata,	
	unsigned long n,	
	uint32_t *ptrs)
{
	unsigned long i, j, k;

	if (gwdata->cpu_flags & CPU_AVX512F) {
		int	block_size, upper_avx512_word;

		if (gwdata->PASS1_SIZE == 0) {
			upper_avx512_word = 8;
			block_size = 64;
		} else {
			upper_avx512_word = gwdata->PASS2_SIZE;
			block_size = upper_avx512_word * 8;
		}

		ptrs[0] = 0xFF00;
		ptrs[1] = 0xFFFFFFFF;
		
		i = n % block_size;				
		j = i / upper_avx512_word;			
		ptrs[2] = 0xFFFF << (j+1);			
		ptrs[3] = i - j * upper_avx512_word;		
		ptrs[4] = 0xFFFF << j;				
		ptrs[5] = 0xFFFFFFFF;				
		
		ptrs[6] = 0xFFFF;
		ptrs[7] = 0xFFFFFFFF;
		
		if (n >= gwdata->FFTLEN/2) ptrs[0] <<= 8, ptrs[2] <<= 8, ptrs[4] <<= 8, ptrs[6] <<= 8;
		
		if (ptrs[3] == 0) ptrs[2] = ptrs[4], ptrs[3] = ptrs[5];
	}

	else if (gwdata->cpu_flags & CPU_AVX) {
		for (i = 0; i < 8; i++) ptrs[i] = 0xFFFFFFFF;	
		for (i = n; i < gwdata->FFTLEN; i++) {
			j = addr_offset (gwdata, i);
			k = (j & 63) >> 3;
			if (ptrs[k] == 0xFFFFFFFF) ptrs[k] = j - (k << 3);
		}
	}

	else {
		for (i = 0; i < 8; i++) ptrs[i] = 0;	
		for (i = 0; i < n; i++) {
			j = addr_offset (gwdata, i);
			k = (j & 63) >> 3;
			if (j >= ptrs[k]) ptrs[k] = j - (k << 3) + 64;
		}
	}
}

void gwcopyzero (
	gwhandle *gwdata,	
	gwnum	s,
	gwnum	d,
	unsigned long n)
{
	struct gwasm_data *asm_data = (struct gwasm_data *) gwdata->asm_data;

	ASSERTG (FFT_state (s) == NOT_FFTed);		

	if (n == 0) {
		gwcopy (gwdata, s, d);
		return;
	}

	if (n != gwdata->saved_copyz_n) {
		if (gwdata->cpu_flags & (CPU_AVX512F | CPU_AVX | CPU_SSE2))
			calc8ptrs (gwdata, n, asm_data->COPYZERO);
		gwdata->saved_copyz_n = n;
	}

	if (gwdata->cpu_flags & CPU_AVX512F) {
		
		int	i, num_blks, blk_containing_n;

		if (gwdata->PASS1_SIZE == 0)
			blk_containing_n = (n % (gwdata->FFTLEN / 2)) / 64;
		else
			blk_containing_n = (n % (gwdata->FFTLEN / 2)) / (gwdata->PASS2_SIZE * 8);
		num_blks = asm_data->addcount1;
		for (i = 0; i < num_blks; i++) {
			asm_data->SRCARG = (char *) s + i * asm_data->pass1blkdst + (i/4) * asm_data->normblkdst4;
			asm_data->DESTARG = (char *) d + i * asm_data->pass1blkdst + (i/4) * asm_data->normblkdst4;
			if (i < blk_containing_n) asm_data->SRC2ARG = (char *) asm_data->COPYZERO;		
			else if (i == blk_containing_n) asm_data->SRC2ARG = (char *) asm_data->COPYZERO + 8;	
			else asm_data->SRC2ARG = (char *) asm_data->COPYZERO + 24;				
			gw_copyzero (gwdata, asm_data);
		}
	}

	else {
		asm_data->SRCARG = s;
		asm_data->DESTARG = d;
		asm_data->NUMARG = n;
		gw_copyzero (gwdata, asm_data);
	}

	unnorms (d) = unnorms (s);
	FFT_state (d) = NOT_FFTed;
	gwdata->read_count += 1;
	gwdata->write_count += 1;
}

void gwsetmulbyconst (
	gwhandle *gwdata,	
	long	val)
{
	struct gwasm_data *asm_data;
	double	ktimesval, big_word;

	if (gwdata->mulbyconst == val) return;
	gwdata->mulbyconst = (int) val;

	if (gwdata->GENERAL_MMGW_MOD) {
		gwsetmulbyconst (gwdata->cyclic_gwdata, val);
		gwsetmulbyconst (gwdata->negacyclic_gwdata, val);
		return;
	}

	asm_data = (struct gwasm_data *) gwdata->asm_data;
	ktimesval = gwdata->k * (double) val;
	big_word = pow ((double) gwdata->b, gwdata->NUM_B_PER_SMALL_WORD + 1);

	if (gwdata->cpu_flags & CPU_AVX512F) {

		asm_data->u.zmm.ZMM_MULCONST = (double) val;
		asm_data->u.zmm.ZMM_MINUS_C_TIMES_MULCONST = (double) -gwdata->c * (double) val;

		if (fabs (ktimesval) * big_word < 562949953421312.0)
			asm_data->u.zmm.ZMM_K_TIMES_MULCONST_HI_OVER_LARGE_BASE = 0.0;
		else if (ktimesval > 0.0)
			asm_data->u.zmm.ZMM_K_TIMES_MULCONST_HI_OVER_LARGE_BASE = floor (ktimesval / big_word);
		else
			asm_data->u.zmm.ZMM_K_TIMES_MULCONST_HI_OVER_LARGE_BASE = -floor (-ktimesval / big_word);
		asm_data->u.zmm.ZMM_K_TIMES_MULCONST_HI_OVER_SMALL_BASE = asm_data->u.zmm.ZMM_K_TIMES_MULCONST_HI_OVER_LARGE_BASE * (double) gwdata->b;
		asm_data->u.zmm.ZMM_K_TIMES_MULCONST_LO = ktimesval - asm_data->u.zmm.ZMM_K_TIMES_MULCONST_HI_OVER_LARGE_BASE * big_word;
	}

	else if (gwdata->cpu_flags & CPU_AVX) {

		asm_data->u.ymm.YMM_MULCONST[0] = asm_data->u.ymm.YMM_MULCONST[1] =
		asm_data->u.ymm.YMM_MULCONST[2] = asm_data->u.ymm.YMM_MULCONST[3] = (double) val;
		asm_data->u.ymm.YMM_MINUS_C_TIMES_MULCONST[0] = asm_data->u.ymm.YMM_MINUS_C_TIMES_MULCONST[1] =
		asm_data->u.ymm.YMM_MINUS_C_TIMES_MULCONST[2] = asm_data->u.ymm.YMM_MINUS_C_TIMES_MULCONST[3] = (double) -gwdata->c * (double) val;

		if (fabs (ktimesval) * big_word < 562949953421312.0)
			asm_data->u.ymm.YMM_K_TIMES_MULCONST_HI[0] = asm_data->u.ymm.YMM_K_TIMES_MULCONST_HI[1] =
			asm_data->u.ymm.YMM_K_TIMES_MULCONST_HI[2] = asm_data->u.ymm.YMM_K_TIMES_MULCONST_HI[3] = 0.0;
		else if (ktimesval > 0.0)
			asm_data->u.ymm.YMM_K_TIMES_MULCONST_HI[0] = asm_data->u.ymm.YMM_K_TIMES_MULCONST_HI[1] =
			asm_data->u.ymm.YMM_K_TIMES_MULCONST_HI[2] = asm_data->u.ymm.YMM_K_TIMES_MULCONST_HI[3] =
				floor (ktimesval / big_word) * big_word;
		else
			asm_data->u.ymm.YMM_K_TIMES_MULCONST_HI[0] = asm_data->u.ymm.YMM_K_TIMES_MULCONST_HI[1] =
			asm_data->u.ymm.YMM_K_TIMES_MULCONST_HI[2] = asm_data->u.ymm.YMM_K_TIMES_MULCONST_HI[3] =
				-floor (-ktimesval / big_word) * big_word;
		asm_data->u.ymm.YMM_K_TIMES_MULCONST_LO[0] = asm_data->u.ymm.YMM_K_TIMES_MULCONST_LO[1] =
		asm_data->u.ymm.YMM_K_TIMES_MULCONST_LO[2] = asm_data->u.ymm.YMM_K_TIMES_MULCONST_LO[3] =
			ktimesval - asm_data->u.ymm.YMM_K_TIMES_MULCONST_HI[0];
	}

	else if (gwdata->cpu_flags & CPU_SSE2) {

		asm_data->u.xmm.XMM_MULCONST[0] = asm_data->u.xmm.XMM_MULCONST[1] = (double) val;
		asm_data->u.xmm.XMM_MINUS_C_TIMES_MULCONST[0] = asm_data->u.xmm.XMM_MINUS_C_TIMES_MULCONST[1] = (double) -gwdata->c * (double) val;

		if (fabs (ktimesval) * big_word < 562949953421312.0)
			asm_data->u.xmm.XMM_K_TIMES_MULCONST_HI[0] = asm_data->u.xmm.XMM_K_TIMES_MULCONST_HI[1] = 0.0;
		else if (ktimesval > 0.0)
			asm_data->u.xmm.XMM_K_TIMES_MULCONST_HI[0] = asm_data->u.xmm.XMM_K_TIMES_MULCONST_HI[1] =
				floor (ktimesval / big_word) * big_word;
		else
			asm_data->u.xmm.XMM_K_TIMES_MULCONST_HI[0] = asm_data->u.xmm.XMM_K_TIMES_MULCONST_HI[1] =
				-floor (-ktimesval / big_word) * big_word;
		asm_data->u.xmm.XMM_K_TIMES_MULCONST_LO[0] =
		asm_data->u.xmm.XMM_K_TIMES_MULCONST_LO[1] = ktimesval - asm_data->u.xmm.XMM_K_TIMES_MULCONST_HI[0];
	}

	else {

		asm_data->u.x87.MULCONST = (double) val;
		asm_data->u.x87.MINUS_C_TIMES_MULCONST = (double) -gwdata->c * (double) val;

		asm_data->u.x87.K_TIMES_MULCONST_HI = floor (ktimesval / big_word) * big_word;
		asm_data->u.x87.K_TIMES_MULCONST_LO = ktimesval - asm_data->u.x87.K_TIMES_MULCONST_HI;
		big_word = big_word * big_word;
		asm_data->u.x87.K_TIMES_MULCONST_HI_2 = floor (asm_data->u.x87.K_TIMES_MULCONST_HI / big_word) * big_word;
		asm_data->u.x87.K_TIMES_MULCONST_HI_1 = asm_data->u.x87.K_TIMES_MULCONST_HI - asm_data->u.x87.K_TIMES_MULCONST_HI_2;
	}
}

void gwsetaddinatpowerofb (
	gwhandle *gwdata,	
	long	value,
	unsigned long power_of_b)
{
	unsigned long word, b_in_word;

	ASSERTG (gwdata->k == 1.0 || value == 0);
	ASSERTG (!gwdata->GENERAL_MOD && !gwdata->GENERAL_MMGW_MOD);

	if (value == 0) {
		gwdata->asm_addin_value = 0.0;
		return;
	}

	bitaddr (gwdata, power_of_b, &word, &b_in_word);
	raw_gwsetaddin (gwdata, word, &gwdata->asm_addin_value, value * pow ((double) gwdata->b, b_in_word));
}

void gwsetaddin (
	gwhandle *gwdata,	
	long	value)
{
	unsigned long word, b_in_word;

	ASSERTG (labs(value) <= 2147483647);		

	if ((gwdata->k != 1.0 && labs (gwdata->c) != 1) || gwdata->GENERAL_MMGW_MOD) {
		if (value == gwdata->emulate_addin_value) return;
		gwdata->emulate_addin_value = value;
		gwfree (gwdata, gwdata->GW_ADDIN);
		gwdata->GW_ADDIN = NULL;
		return;
	}

	if (value == 0) {
		gwdata->asm_addin_value = 0.0;
		return;
	}

	if (gwdata->ZERO_PADDED_FFT) {
		
		gwdata->asm_addin_value = (double) (gwdata->c < 0 ? value : -value);
		return;
	}

	if (gwdata->k == 1.0) {
		raw_gwsetaddin (gwdata, 0, &gwdata->asm_addin_value, value);
		return;
	}

	b_in_word = 0;

	if (gwdata->c == -1) {
		bitaddr (gwdata, gwdata->n + b_in_word, &word, &b_in_word);
	}
	else if (gwdata->c == 1) {
		bitaddr (gwdata, gwdata->n + b_in_word, &word, &b_in_word);
		value = -value;
	}

	raw_gwsetaddin (gwdata, word, &gwdata->asm_addin_value, value * pow ((double) gwdata->b, b_in_word));
}

void gwsetpostmulbyconstaddin (
	gwhandle *gwdata,	
	int	value)
{
	ASSERTG (gwdata->k == 1.0 || abs(value) <= (int) (2*gwdata->b));	

	if ((gwdata->k != 1.0 && labs (gwdata->c) != 1) || gwdata->GENERAL_MMGW_MOD) {
		if (value == gwdata->emulate_postaddin_value) return;
		gwdata->emulate_postaddin_value = value;
		gwfree (gwdata, gwdata->GW_POSTADDIN);
		gwdata->GW_POSTADDIN = NULL;
		return;
	}

	if (value == 0) {
		gwdata->asm_postaddin_value = 0.0;
		return;
	}

	if (gwdata->ZERO_PADDED_FFT) {
		
		gwdata->asm_postaddin_value = (double) (gwdata->c < 0 ? value : -value);
		return;
	}

	if (gwdata->k == 1.0) {
		raw_gwsetaddin (gwdata, 0, &gwdata->asm_postaddin_value, value);
	}

	else {
		unsigned long word, b_in_word = 0;
		
		if (gwdata->c == -1) {
			bitaddr (gwdata, gwdata->n + b_in_word, &word, &b_in_word);
		}
		else if (gwdata->c == 1) {
			bitaddr (gwdata, gwdata->n + b_in_word, &word, &b_in_word);
			value = -value;
		}

		raw_gwsetaddin (gwdata, word, &gwdata->asm_postaddin_value, value * pow ((double) gwdata->b, b_in_word));
	}
}

void raw_gwsetaddin (
	gwhandle *gwdata,	
	unsigned long word,
	double	*ptr,	     
	double	val)
{
	struct gwasm_data *asm_data = (struct gwasm_data *) gwdata->asm_data;
	unsigned long row;

	if (gwdata->clone_of == NULL) gwmutex_lock (&gwclone_lock);

	if (gwdata->cpu_flags & CPU_AVX512F) {

		if (gwdata->PASS1_SIZE == 0) {
			asm_data->ADDIN_ROW = 0;
			asm_data->ADDIN_OFFSET = addr_offset (gwdata, word);
		}

		else {
			row = word % gwdata->PASS2_SIZE;
			asm_data->ADDIN_ROW = row & ~(gwdata->PASS1_CACHE_LINES - 1);
			asm_data->ADDIN_OFFSET = (row - asm_data->ADDIN_ROW) * 128;
			if (word >= gwdata->FFTLEN / 2) asm_data->ADDIN_OFFSET += 64;
			asm_data->ADDIN_OFFSET += ((word / gwdata->PASS2_SIZE) & 7) * 8;
			row = (word % (gwdata->FFTLEN / 2)) / (8 * gwdata->PASS2_SIZE);
			asm_data->ADDIN_OFFSET += row * (gwdata->PASS1_CACHE_LINES * 128 + (uint32_t) asm_data->normblkdst);
			asm_data->ADDIN_OFFSET += (row >> 3) * (uint32_t) asm_data->normblkdst8;
		}
	}

	else if (gwdata->cpu_flags & CPU_AVX) {

		if (gwdata->PASS2_SIZE) {
			row = word % gwdata->PASS2_SIZE;
			asm_data->ADDIN_ROW = row & ~(gwdata->PASS1_CACHE_LINES - 1);
			asm_data->ADDIN_OFFSET = (row - asm_data->ADDIN_ROW) * 64;
			if (word >= gwdata->FFTLEN / 2) asm_data->ADDIN_OFFSET += 32;
			asm_data->ADDIN_OFFSET += ((word / gwdata->PASS2_SIZE) & 3) * 8;
			row = (word % (gwdata->FFTLEN / 2)) / (4 * gwdata->PASS2_SIZE);
			asm_data->ADDIN_OFFSET += row * (gwdata->PASS1_CACHE_LINES * 64 + (uint32_t) asm_data->normblkdst);
			asm_data->ADDIN_OFFSET += (row >> 3) * (uint32_t) asm_data->normblkdst8;
		}

		else
			asm_data->ADDIN_OFFSET = addr_offset (gwdata, word);

	}

	else if (gwdata->cpu_flags & CPU_SSE2) {

		asm_data->ADDIN_OFFSET = addr_offset (gwdata, word);

		if (gwdata->PASS2_SIZE == 0) {
			row = asm_data->ADDIN_OFFSET & 31;
			if (row == 8) asm_data->ADDIN_OFFSET += 8;
			if (row == 16) asm_data->ADDIN_OFFSET -= 8;
		}

		else {
			unsigned long num_rows;

			num_rows = gwdata->PASS2_SIZE >> 1;
			row = word % num_rows;
			asm_data->ADDIN_ROW = row & ~(gwdata->PASS1_CACHE_LINES - 1);
			asm_data->ADDIN_OFFSET -= (row >> 7) * 128 + (row / gwdata->PASS1_CACHE_LINES) * gwdata->PASS1_CACHE_LINES * 64;

			if (gwdata->SCRATCH_SIZE) {
				unsigned long blkdst;

				blkdst = addr_offset (gwdata, gwdata->PASS2_SIZE);
				row = asm_data->ADDIN_OFFSET / blkdst;
				asm_data->ADDIN_OFFSET -= row * blkdst;
				asm_data->ADDIN_OFFSET += row * (gwdata->PASS1_CACHE_LINES * 64);
				asm_data->ADDIN_OFFSET += (row >> 3) * (uint32_t) asm_data->normblkdst8;
			}
		}
	}

	else {

		asm_data->ADDIN_OFFSET = addr_offset (gwdata, word);

		if (gwdata->PASS2_SIZE) {
			unsigned long num_cache_lines, cache_line;

			num_cache_lines = gwdata->PASS2_SIZE >> 1;
			cache_line = ((word >> 1) & (num_cache_lines - 1));

			asm_data->ADDIN_ROW = ((num_cache_lines>>7) - (cache_line>>7)) * 65536 +
					(128 / gwdata->PASS1_CACHE_LINES -
					(cache_line & 127) / gwdata->PASS1_CACHE_LINES) * 256;
			asm_data->ADDIN_OFFSET -= (cache_line >> 7) * 64 +
					(cache_line / gwdata->PASS1_CACHE_LINES) *
					gwdata->PASS1_CACHE_LINES * 32;

			if (gwdata->SCRATCH_SIZE) {
				unsigned long blkdst;

				blkdst = addr_offset (gwdata, gwdata->PASS2_SIZE);
				row = asm_data->ADDIN_OFFSET / blkdst;
				asm_data->ADDIN_OFFSET -= row * blkdst;
				asm_data->ADDIN_OFFSET += row * (gwdata->PASS1_CACHE_LINES * 32);

				if (((gwdata->FFTLEN / gwdata->PASS2_SIZE) >> 1) >= 128)
					asm_data->ADDIN_OFFSET += (row >> 5) * 64;
			}
		}
	}

	if (ptr != NULL) {

	    if (gwdata->cpu_flags & CPU_AVX512F) {

		if (ptr == &gwdata->asm_postaddin_value || (gwdata->RATIONAL_FFT && !(gwdata->PASS1_SIZE && gwdata->NEGACYCLIC_FFT))) {
			*ptr = val;
		}

		else {
			unsigned long upper_avx512_word, grp;
			double	ttmp;
			
			upper_avx512_word = (gwdata->PASS1_SIZE == 0) ? 8 : gwdata->PASS2_SIZE;
			grp = word - word % upper_avx512_word;
			if (gwdata->PASS1_SIZE && gwdata->NEGACYCLIC_FFT) {
				unsigned long sine_word;
				sine_word = grp % (gwdata->FFTLEN / 2);
				gwfft_weights_times_sine (gwdata->dd_data, grp, sine_word, gwdata->FFTLEN * 2, NULL, &ttmp);
			} else
				ttmp = gwfft_weight_inverse (gwdata->dd_data, grp);
			*ptr = val / ttmp;
		}
	    }

	    else if (gwdata->FFT_TYPE == FFT_TYPE_RADIX_4_DWPN) {
		*ptr = val * gwfft_partial_weight_sloppy (gwdata->dd_data, word, dwpn_col (gwdata, word));
	    }

	    else {
		*ptr = val * gwfft_weight_sloppy (gwdata->dd_data, word);
		if (ptr == &gwdata->asm_addin_value) *ptr *= gwdata->FFTLEN * 0.5 / gwdata->k;
	    }
	}

	if (gwdata->clone_of == NULL) gwmutex_unlock (&gwclone_lock);
}

void init_GW_ADDIN (
	gwhandle *gwdata)	
{
	if (gwdata->GW_ADDIN == NULL && gwdata->emulate_addin_value) {
		gwdata->GW_ADDIN = gwalloc_internal (gwdata);
		dbltogw (gwdata, (double) gwdata->emulate_addin_value, gwdata->GW_ADDIN);
	}
	if (gwdata->GW_POSTADDIN == NULL && gwdata->emulate_postaddin_value) {
		gwdata->GW_POSTADDIN = gwalloc_internal (gwdata);
		dbltogw (gwdata, (double) gwdata->emulate_postaddin_value, gwdata->GW_POSTADDIN);
	}
}

void MMGW_redc (gwhandle *gwdata, gwnum T_R, gwnum T_Q)
{
	gwmul3 (gwdata->cyclic_gwdata, T_R, gwdata->Np_R, T_R, 0);			
	*T_R += 2.0;									
	gwmul3 (gwdata->negacyclic_gwdata, T_R, gwdata->N_Q, T_R, 0);			
	gwsub3o (gwdata->negacyclic_gwdata, T_R, T_Q, T_R, GWADD_FORCE_NORMALIZE);	
}

void MMGW_redc4 (gwhandle *gwdata, gwnum T_R, gwnum T_Q)
{
	gwmul3 (gwdata->cyclic_gwdata, T_R, gwdata->Np_R, T_R, 0);			
	*T_R += 2.0;									
	gwmulsub4 (gwdata->negacyclic_gwdata, T_R, gwdata->N_Q, T_Q, T_R, 0);		
}

void MMGW_redc5 (gwhandle *gwdata, gwnum T_R, gwnum s1_Q, gwnum s2_Q)
{
	gwmul3 (gwdata->cyclic_gwdata, T_R, gwdata->Np_R, T_R, 0);			
	*T_R += 2.0;									
	gwmulmulsub5 (gwdata->negacyclic_gwdata, T_R, gwdata->N_Q, s1_Q, s2_Q, T_R, 0);	
}

void MMGW_redc1 (gwhandle *gwdata, gwnum T, gwnum T_R)
{
	ASSERTG (T != T_R);
	gwmul3 (gwdata->cyclic_gwdata, T, gwdata->Np_R, T_R, GWMUL_PRESERVE_S1);	
	*T_R += 2.0;									
	if (FFT_state (T) != FULLY_FFTed) {
		gwmul3 (gwdata->negacyclic_gwdata, T_R, gwdata->N_Q, T_R, 0);		
		gwsub3quick (gwdata->negacyclic_gwdata, T_R, T, T_R);			
	} else {
		gwnum T_Q = negacyclic_gwnum (gwdata, T);
		ASSERTG (muladd_safe (gwdata->negacyclic_gwdata, 0, 0, unnorms (T_Q)));
		gwmulsub4 (gwdata->negacyclic_gwdata, T_R, gwdata->N_Q, T_Q, T_R, 0);	
	}
}

void dbltogw (
	gwhandle *gwdata,	
	double	d,		
	gwnum	g)		
{
	stackgiant(tmp, 2);

	dbltog (d, tmp);
	gianttogw (gwdata, tmp, g);
}

void s64togw (
	gwhandle *gwdata,	
	int64_t d,		
	gwnum	g)		
{
	stackgiant(tmp, 2);

	slltog (d, tmp);
	gianttogw (gwdata, tmp, g);
}

void u64togw (
	gwhandle *gwdata,	
	uint64_t d,		
	gwnum	g)		
{
	stackgiant(tmp, 2);

	ulltog (d, tmp);
	gianttogw (gwdata, tmp, g);
}

void binarytogw (
	gwhandle *gwdata,	
	const uint32_t *array,	
	uint32_t arraylen,	
	gwnum	n)		
{
	giantstruct tmp;
	tmp.sign = arraylen;
	tmp.n = (uint32_t *) array;
	while (tmp.sign && tmp.n[tmp.sign-1] == 0) tmp.sign--;
	gianttogw (gwdata, &tmp, n);
}

void binary64togw (
	gwhandle *gwdata,	
	const uint64_t *array,	
	uint64_t arraylen,	
	gwnum	n)		
{
	giantstruct tmp;
	tmp.sign = (int) arraylen * sizeof (uint64_t) / sizeof (uint32_t);
	tmp.n = (uint32_t *) array;
	while (tmp.sign && tmp.n[tmp.sign-1] == 0) tmp.sign--;
	gianttogw (gwdata, &tmp, n);
}

void binarylongstogw (
	gwhandle *gwdata,	
	const unsigned long *array, 
	unsigned long arraylen,	
	gwnum	n)		
{
	giantstruct tmp;
	tmp.sign = arraylen * sizeof (unsigned long) / sizeof (uint32_t);
	tmp.n = (uint32_t *) array;
	while (tmp.sign && tmp.n[tmp.sign-1] == 0) tmp.sign--;
	gianttogw (gwdata, &tmp, n);
}

void gianttogw (
	gwhandle *gwdata,	
	giant	a,
	gwnum	g)
{

	if (gwdata->GENERAL_MMGW_MOD) {
		gianttogw (gwdata->negacyclic_gwdata, a, g);
		if (g != gwdata->R2_4) {
			if (labs (a->sign) > 4) gwmul3_carefully (gwdata, g, gwdata->R2_4, g, 0); 
			else gwmul3 (gwdata, g, gwdata->R2_4, g, 0);				  
		}
		return;
	}

	if (a->sign == 0) {
		memset (g, 0, gwnum_datasize (gwdata));
	}

	else if (labs (a->sign) <= 2 && (gwdata->k == 1.0 || labs (gwdata->c) == 1)) {
		uint64_t value_to_convert, low_addin;
		int	i;

		memset (g, 0, gwnum_datasize (gwdata));
		value_to_convert = a->n[0];
		if (labs (a->sign) == 2) value_to_convert += ((uint64_t) a->n[1]) << 32;

		uint64_t small_maxval = intpow (gwdata->b, gwdata->NUM_B_PER_SMALL_WORD);
		if (gwdata->k > 1.0) {
			uint64_t quotient, remainder, high_addin;
			unsigned long word, bit_in_word;

			quotient = value_to_convert / (uint64_t) gwdata->k;
			remainder = value_to_convert - quotient * (uint64_t) gwdata->k;
			if (gwdata->c == -1) {
				low_addin = quotient;
				high_addin = remainder;
			} else {
				low_addin = quotient + 1;
				high_addin = (uint64_t) gwdata->k - remainder;
			}

			bitaddr (gwdata, gwdata->n, &word, &bit_in_word);
			for (i = word; high_addin; i++, bit_in_word = 0) {
				long	value;

				uint64_t maxval = small_maxval;
				if (is_big_word (gwdata, i)) maxval *= gwdata->b;

				if (bit_in_word == 0) {
					quotient = high_addin / maxval;
					remainder = high_addin - quotient * maxval;
					value = (long) remainder;
					high_addin = quotient;
				} else {
					uint64_t pow_fudge, fudged_maxval;
					pow_fudge = intpow (gwdata->b, bit_in_word);
					fudged_maxval = maxval / pow_fudge;
					quotient = high_addin / fudged_maxval;
					remainder = high_addin - quotient * fudged_maxval;
					value = (long) (remainder * pow_fudge);
					high_addin = quotient;
				}

				if ((uint64_t) value > (maxval >> 1)) {
					value = value - (long) maxval;
					high_addin += 1;
				}
				if (i == gwdata->FFTLEN - 1) {
					value = value + (long) high_addin * (long) maxval;
					high_addin = 0;
				}
				set_fft_value (gwdata, g, i, a->sign > 0 ? value : -value);
			}
		}

		else
			low_addin = value_to_convert;

		for (i = 0; low_addin; i++) {
			uint64_t maxval = small_maxval;
			if (is_big_word (gwdata, i)) maxval *= gwdata->b;

			long value = (long) (low_addin % maxval);
			low_addin = low_addin / maxval;
			if (value > (long) (maxval >> 1)) {
				value = value - (long) maxval;
				low_addin++;
			}
			set_fft_value (gwdata, g, i, a->sign > 0 ? value : -value);
		}
	}

	else {
		giant	newg = NULL;

		if (gwdata->k > 1.0) {
			newg = popg (&gwdata->gdata, (((unsigned long) gwdata->bit_length >> 5) + 2) * 2);

			if (gwdata->c == -1) {
				if (gwdata->b == 2) {
					gtog (a, newg);
					gshiftleft (gwdata->n, newg);
				} else {
					itog (gwdata->b, newg);
					power (newg, gwdata->n);
					mulgi (&gwdata->gdata, a, newg);
				}
			}

			else if (gwdata->c == 1) {
				if (gwdata->b == 2) {
					gtog (a, newg);
					gshiftleft (gwdata->n, newg);
					negg (newg);
				} else {
					itog (gwdata->b, newg);
					power (newg, gwdata->n);
					negg (newg);
					mulgi (&gwdata->gdata, a, newg);
				}
			}

			else {				
				giant	n;
				n = popg (&gwdata->gdata, ((unsigned long) gwdata->bit_length >> 5) + 2);
				ultog (gwdata->b, n);		
				power (n, gwdata->n);
				dblmulg (gwdata->k, n);
				iaddg (gwdata->c, n);
				dbltog (gwdata->k, newg);	
				invg (n, newg);
				ASSERTG (newg->sign > 0);	
				mulgi (&gwdata->gdata, a, newg); 
				pushg (&gwdata->gdata, 1);
			}

			specialmodg (gwdata, newg);
			a = newg;
		}

		if (gwdata->b == 2) {
			unsigned long limit;
			int	bits, bits1, bits2, e1len, accumbits;
			int64_t	accum, value;
			uint32_t *e1;

			limit = (unsigned long) ceil ((double) bitlen (a) / (gwdata->avg_num_b_per_word * log2 (gwdata->b)));
			if (limit > gwdata->FFTLEN) limit = gwdata->FFTLEN;
			if (gwdata->ZERO_PADDED_FFT && limit > gwdata->FFTLEN / 2 + 4) limit = gwdata->FFTLEN / 2 + 4;

			e1len = abs (a->sign);
			e1 = a->n;

			bits1 = gwdata->NUM_B_PER_SMALL_WORD;
			bits2 = bits1 + 1;

			accum = 0;
			accumbits = 0;

			gwiter iter;
			for (gwiter_init_write_only (gwdata, &iter, g); gwiter_index (&iter) < limit; gwiter_next (&iter)) {

				if (accumbits < 28) {
					if (e1len > 0) {
						if (a->sign > 0) accum += ((int64_t) *e1) << accumbits;
						else accum -= ((int64_t) *e1) << accumbits;
						e1++, e1len--;
					}
					accumbits += 32;
				}

				bits = (gwiter_index (&iter) == limit - 1) ? 32 : gwiter_is_big_word (&iter) ? bits2 : bits1;
				value = bits ? (accum << (64 - bits)) >> (64 - bits) : 0;
				gwiter_set_fft_value (&iter, (int32_t) value);
				accum = (accum - value) >> bits;
				accumbits -= bits;
			}

			for ( ; gwiter_index (&iter) < gwdata->FFTLEN; gwiter_next (&iter)) gwiter_set_fft_value (&iter, 0);
		}

		else {
			if (a->sign < 0) {
				newg = popg (&gwdata->gdata, (((unsigned long) gwdata->bit_length >> 5) + 2) * 2);
				gtog (a, newg);
				specialmodg (gwdata, newg);
				a = newg;
			}
			nonbase2_gianttogw (gwdata, a, g);
		}

		if (a == newg) pushg (&gwdata->gdata, 1);
	}

	unnorms (g) = 0.0f;		
	FFT_state (g) = NOT_FFTed;	
	gwdata->write_count += 1;
}

long gwtobinary (
	gwhandle *gwdata,	
	gwnum	n,		
	uint32_t *array,	
	uint32_t arraylen)	
{
	int	err_code;

	ASSERTG (arraylen * 32 > gwdata->bit_length);

	if (FFT_state (n) != NOT_FFTed) gwunfft (gwdata, n, n);

	if (arraylen * 32 >= gwdata->bit_length + log2 (gwdata->k) + unnorms (n) + 32.0) {
		giantstruct tmp;
		tmp.n = (uint32_t *) array;
		setmaxsize (&tmp, arraylen);
		err_code = gwtogiant (gwdata, n, &tmp);
		if (err_code < 0) return (err_code);
		gwdata->read_count += 1;
		return (tmp.sign);
	}

	else {
		giant tmp = popg (&gwdata->gdata, ((unsigned long) gwdata->bit_length >> 5) + 5);
		err_code = gwtogiant (gwdata, n, tmp);
		if (err_code < 0) { pushg (&gwdata->gdata, 1); return (err_code); }
		if ((uint32_t) tmp->sign > arraylen) tmp->sign = arraylen;			
		memcpy (array, tmp->n, tmp->sign * sizeof (uint32_t));
		pushg (&gwdata->gdata, 1);
		gwdata->read_count += 1;
		return (tmp->sign);
	}
}

long gwtobinary64 (
	gwhandle *gwdata,	
	gwnum	n,		
	uint64_t *array,	
	uint32_t arraylen)	
{

	long count = gwtobinary (gwdata, n, (uint32_t *) array, arraylen * 2);
	if (count > 0) {
		bool count_is_odd = (count & 1);			
		count = (count + 1) / 2;				
		if (count_is_odd) array[count-1] &= 0xFFFFFFFFULL;	
	}
	return (count);
}

long gwtobinarylongs (
	gwhandle *gwdata,	
	gwnum	n,		
	unsigned long *array,	
	unsigned long arraylen)	
{
	if (sizeof (unsigned long) == sizeof (uint32_t)) return gwtobinary (gwdata, n, (uint32_t *) array, arraylen);
	else return gwtobinary64 (gwdata, n, (uint64_t *) array, arraylen);
}

int gwtogiant (
	gwhandle *gwdata,	
	gwnum	gg,
	giant	v)
{
	stackgiant(k,2);
	int	err_code;
	unsigned long limit;

	if (gwdata->GENERAL_MMGW_MOD) {
		gwnum newgg = gwalloc (gwdata);
		MMGW_redc1 (gwdata, gg, newgg);
		err_code = gwtogiant (gwdata->negacyclic_gwdata, newgg, v);
		gwfree (gwdata, newgg);
		specialmodg (gwdata, v);
		ASSERTG (v->sign >= 0);
		return (err_code);
	}

	if (FFT_state (gg) != NOT_FFTed) gwunfft (gwdata, gg, gg);

	v->sign = 0;

	dbltog (gwdata->k, k);
	bool k_is_small = (k->sign <= 1);
	bool k_is_one = (k_is_small && k->n[0] == 1);
#define klo	(k->n[0])
#define khi	(k->n[1])
#define k64	(*(uint64_t *)k->n)
	if (k_is_small) khi = 0;	

	if (gwdata->GENERAL_MOD) limit = gwdata->GW_GEN_MOD_MAX + 3;
	else if (gwdata->ZERO_PADDED_FFT) limit = gwdata->FFTLEN / 2 + 4;
	else limit = gwdata->FFTLEN;

	if (gwdata->GENERAL_MOD) {
		long	val, prev_val;
		while (limit < gwdata->FFTLEN) {
			err_code = get_fft_value (gwdata, gg, limit, &val);
			if (err_code) return (err_code);
			if (val == -1 || val == 0) break;
			limit++;
			ASSERTG (limit <= gwdata->FFTLEN / 2 + 2);
			if (limit > gwdata->FFTLEN / 2 + 2) return (GWERROR_INTERNAL + 9);
		}
		while (limit > 1) {		
			err_code = get_fft_value (gwdata, gg, limit-1, &prev_val);
			if (err_code) return (err_code);
			if (val != prev_val || val < -1 || val > 0) break;
			limit--;
		}
		limit++;
	}

	if (gwdata->b == 2) {
		gwiter	iter;
		int32_t	val;
		int64_t accum;
		int	bits, accumbits;
		uint32_t *outptr, outval;
		uint64_t outcarry;
		uint32_t will_output, cached_zeros;

		accum = 0;
		accumbits = 0;
		outptr = v->n;
		outcarry = 0;
		cached_zeros = 0;
		for (gwiter_init_zero (gwdata, &iter, gg); ; ) {
			
			if (gwiter_index (&iter) < limit) {
				err_code = gwiter_get_fft_value (&iter, &val);
				if (err_code) return (err_code);
				bits = gwdata->NUM_B_PER_SMALL_WORD;
				if (gwiter_is_big_word (&iter)) bits++;
				accum += ((int64_t) val) << accumbits;
				accumbits += bits;
				gwiter_next (&iter);
				
				if (accumbits < 32) continue;
			}

			outval = (uint32_t) accum;
			accum >>= 32;
			accumbits -= 32;

			if (k_is_one)
				will_output = outval;
			else if (k_is_small) {
				uint64_t tmp = (uint64_t) outval * (uint64_t) klo + outcarry;
				will_output = (uint32_t) tmp;
				outcarry = tmp >> 32;
			} else {
				uint64_t tmp = (uint64_t) outval * (uint64_t) klo;
				uint64_t next_outcarry = tmp >> 32;
				tmp = (tmp & 0xFFFFFFFFULL) + outcarry;
				will_output = (uint32_t) tmp;
				outcarry = next_outcarry + (tmp >> 32) + (uint64_t) outval * (uint64_t) khi;
			}
			if (will_output) {
				while (cached_zeros) { *outptr++ = 0; cached_zeros--; }
				*outptr++ = will_output;
			} else {
				cached_zeros++;
			}

			if (gwiter_index (&iter) >= limit && ((accum == 0 && outcarry == 0) || (accum == -1 && outcarry == k64 - 1))) break;
		}

		ASSERTG (v->maxsize >= (int) (outptr - v->n));
		v->sign = (int) (outptr - v->n);
		ASSERTG (v->sign == 0 || v->n[v->sign-1] != 0);		

		stackgiant(upper,5);					
		gtogshiftrightsplit (gwdata->n, v, upper, accum);	
		if (!isZero (upper)) {
			if (k_is_one) {
				imulg (gwdata->c, upper);	
				subg (upper, v);		
			} else {
				stackgiant(quot,5);
				gtog (upper, quot);
				divg (k, quot);			
				stackgiant(tmp,5);
				gtog (k, tmp);
				mulg (quot, tmp);
				subg (tmp, upper);		
				if (upper->sign < 0) iaddg (-1, quot), addg (k, upper); 
				gtogshiftleftunsplit (gwdata->n, upper, v);	
				imulg (gwdata->c, quot);	
				subg (quot, v);
			}
		}
	}

	else {
		err_code = nonbase2_gwtogiant (gwdata, gg, v);
		if (err_code) return (err_code);

		if (!k_is_one) mulgi (&gwdata->gdata, k, v);
	}

#undef klo
#undef khi
#undef k64

	ASSERTG (v->maxsize >= labs (v->sign));

	specialmodg (gwdata, v);
	ASSERTG (v->sign >= 0);

	gwdata->read_count += 1;
	return (0);
}

void specialmodg (
	gwhandle *gwdata,	
	giant	g)
{
	int	neg, count;
	giant	n;

	if (gwdata->GW_MODULUS != NULL) {
		modgi (&gwdata->gdata, gwdata->GW_MODULUS, g);
		return;
	}

	if (g->sign == 0) return;
	if (g->sign == 1 && log2 (1.00001 * (double) g->n[0]) < gwdata->bit_length) return;
	if (g->sign >= 2 && log2 (1.00001 * (g->n[g->sign-1] * 4294967296.0 + g->n[g->sign-2])) + 32 * (g->sign - 2) < gwdata->bit_length) return;

	n = popg (&gwdata->gdata, ((unsigned long) gwdata->bit_length >> 5) + 2);
	ultog (gwdata->b, n);
	power (n, gwdata->n);
	dblmulg (gwdata->k, n);
	iaddg (gwdata->c, n);

	if (gwdata->b != 2) {
		modgi (&gwdata->gdata, n, g);
		pushg (&gwdata->gdata, 1);
		return;
	}

	neg = FALSE;
	for (count = 0; count < 2; count++) {

	    neg ^= (g->sign < 0);
	    g->sign = abs (g->sign);

	    if (gcompg (g, n) > 0) {
		giant	tmp1;

		tmp1 = popg (&gwdata->gdata, ((unsigned long) gwdata->bit_length >> 5) + 5);

		gtogshiftright (gwdata->n, g, tmp1);	
		gmaskbits (gwdata->n, g);		

		if (gwdata->k == 1.0) {
			imulg (gwdata->c, tmp1);	
			subg (tmp1, g);
		} else {
			giant	tmp2, tmp3;

			tmp2 = popg (&gwdata->gdata, (((unsigned long) gwdata->bit_length >> 5) + 5) * 2);
			tmp3 = popg (&gwdata->gdata, ((unsigned long) gwdata->bit_length >> 5) + 5);

			gtog (tmp1, tmp2);
			dbltog (gwdata->k, tmp3);
			divgi (&gwdata->gdata, tmp3, tmp1);	
			mulgi (&gwdata->gdata, tmp1, tmp3);
			subg (tmp3, tmp2);	

			gshiftleft (gwdata->n, tmp2);
			addg (tmp2, g);		

			imulg (gwdata->c, tmp1);
			subg (tmp1, g);
			pushg (&gwdata->gdata, 2);
		}

		pushg (&gwdata->gdata, 1);
	    }
	}

	while (g->sign < 0) addg (n, g);
	while (gcompg (g, n) >= 0) subg (n, g);

	if (neg && g->sign) {
		g->sign = -g->sign;
		addg (n, g);
	}

	pushg (&gwdata->gdata, 1);
}

int gwiszero (
	gwhandle *gwdata,
	gwnum 	gg)
{
	unsigned long j;
	int 	result, count;

	ASSERTG (unnorms (gg) >= 0.0f);
	ASSERTG (FFT_state (gg) == NOT_FFTed);

	if (unnorms (gg) > 0.0f) {
		struct gwasm_data *asm_data;
		gwnum	gwnorm;

		gwnorm = gwalloc (gwdata);
		if (gwnorm == NULL) return (-GWERROR_MALLOC);
		dbltogw (gwdata, 0.0, gwnorm);
		asm_data = (struct gwasm_data *) gwdata->asm_data;
		asm_data->SRCARG = gg;
		asm_data->SRC2ARG = gwnorm;
		asm_data->DESTARG = gwnorm;
		gw_add (gwdata, asm_data);
		result = gwiszero (gwdata, gwnorm);
		gwfree (gwdata, gwnorm);
		return (result);
	}

#define	MAX_NZ_COUNT 16
	count = 0;
	for (j = 0; j < gwdata->FFTLEN; j++) {
		double	*valaddr = addr (gwdata, gg, j);
		if (! is_valid_double_addr (valaddr)) return (GWERROR_BAD_FFT_DATA);
		if (*valaddr == 0.0) continue;
		if (++count > MAX_NZ_COUNT) return (FALSE);	
	}
	if (count) {			
		giant	gtest;
		gtest = popg (&gwdata->gdata, ((unsigned long) gwdata->bit_length >> 5) + 5);
		if (gtest == NULL) return (-GWERROR_MALLOC);
		gwtogiant (gwdata, gg, gtest);
		result = isZero (gtest);
		pushg (&gwdata->gdata, 1);
		return (result);
	}
	else
		return (TRUE);			
}

int gwequal (
	gwhandle *gwdata,
	gwnum	gw1,
	gwnum	gw2)
{
	gwnum	gwdiff;
	int	result;

	ASSERTG (unnorms (gw1) >= 0.0f);
	ASSERTG (unnorms (gw2) >= 0.0f);
	ASSERTG (FFT_state (gw1) == NOT_FFTed);
	ASSERTG (FFT_state (gw2) == NOT_FFTed);

	gwdiff = gwalloc (gwdata);

	gwsub3o (gwdata, gw1, gw2, gwdiff, GWADD_FORCE_NORMALIZE);

	result = gwiszero (gwdata, gwdiff);

	gwfree (gwdata, gwdiff);
	return (result);
}

void init_FFT1 (		
	gwhandle *gwdata)	
{

	if (gwdata->clone_of != NULL && gwdata->GW_FFT1 != gwdata->clone_of->GW_FFT1 && gwdata->clone_of->GW_FFT1 != NULL) {
		gwfree (gwdata, gwdata->GW_FFT1);
		gwdata->GW_FFT1 = gwdata->clone_of->GW_FFT1;
	}

	else if (gwdata->FFT1_state != 0) return;

	else if (gwdata->GW_FFT1 == NULL) {		
		gwnum fft1 = gwalloc_internal (gwdata);
		dbltogw (gwdata, 1.0, fft1);
		gwfft (gwdata, fft1, fft1);
		gwdata->GW_FFT1 = fft1;			
	}
	gwdata->FFT1_state = 1;
}

void gwuser_init_FFT1 (		
	gwhandle *gwdata)	
{
	init_FFT1 (gwdata);
	gwdata->FFT1_user_allocated = 1;
}

void asm_mul (
	gwhandle *gwdata,	
	gwnum	s1,		
	gwnum	s2,		
	gwnum	d,		
	int	options)	
{
	struct gwasm_data *asm_data = (struct gwasm_data *) gwdata->asm_data;

	gwdata->POSTFFT = ((options & GWMUL_STARTNEXTFFT) && !gwdata->GENERAL_MOD && gwdata->PASS1_SIZE);
	
#ifndef X86_64
	if (! (gwdata->cpu_flags & (CPU_AVX512F | CPU_AVX | CPU_SSE2))) asm_data->u.x87.POSTFFT = gwdata->POSTFFT;
#endif	

	asm_data->ADDIN_VALUE = (options & GWMUL_ADDINCONST) ? gwdata->asm_addin_value : 0.0;
	asm_data->POSTADDIN_VALUE = (options & GWMUL_ADDINCONST) ? gwdata->asm_postaddin_value : 0.0;

	asm_data->DESTARG = d;
	asm_data->DIST_TO_FFTSRCARG = (intptr_t) s1 - (intptr_t) d;
	asm_data->DIST_TO_MULSRCARG = (intptr_t) s2 - (intptr_t) d;

	asm_data->zero_fft = (asm_data->NORMRTN == gwdata->GWPROCPTRS[zerohigh_routines + gwdata->ERROR_CHECKING]);
	asm_data->const_fft = !!(options & GWMUL_MULBYCONST);
	asm_data->add_sub_smallmul_op = 0;

	gw_fft (gwdata, asm_data);

	if (! is_valid_double_addr (&gwsumout (gwdata, d))) gwdata->GWERROR |= 1;
	gwdata->fft_count += (asm_data->ffttype <= 3) ? 2 : 1;

	if (asm_data->DEST2ARG) {
		FFT_state (s1) = FULLY_FFTed;
		d = (gwnum) ((char *) d + (intptr_t) asm_data->DEST2ARG);
	}

	unnorms (d) = 0.0f;
	FFT_state (d) = gwdata->POSTFFT ? PARTIALLY_FFTed : NOT_FFTed;
}

void raw_gwmul3 (
	gwhandle *gwdata,	
	gwnum	s1,		
	gwnum	s2,		
	gwnum	d,		
	int	options)	
{
#define swaps1s2() { gwswap (s1, s2); options = (options & ~0xF) + ((options & 0x3) << 2) + ((options & 0xC) >> 2); }
	struct gwasm_data *asm_data = (struct gwasm_data *) gwdata->asm_data;
	gwnum	tmp1, tmp2;

	tmp1 = tmp2 = NULL;

	asm_data->mul4_opcode = 0;
	asm_data->DEST2ARG = 0;

#ifdef X86_64

if ((gwdata->cpu_flags & (CPU_AVX512F | CPU_FMA3 | CPU_AVX)) &&
    ! (gwdata->ZERO_PADDED_FFT && FFT_state (s1) == PARTIALLY_FFTed && s2 == d) &&
    ! (gwdata->ZERO_PADDED_FFT && FFT_state (s2) == PARTIALLY_FFTed && s1 == d)) {
#else
	if (0) {
#endif

		if (s1 == s2) {
			if (FFT_state (s1) == FULLY_FFTed) asm_data->ffttype = 4;
			else if (!(options & GWMUL_FFT_S12)) asm_data->ffttype = 2;
			else {	
				asm_data->ffttype = (gwdata->cpu_flags & CPU_AVX512F) ? 2 : 3;
				asm_data->DEST2ARG = d;
				asm_data->mul4_opcode = 0x80;
			}
		}

		else {
			gwnum	preferred_FFT_arg;	

			bool s1_can_be_FFTed_and_discarded = (s2 != d);
			bool s2_can_be_FFTed_and_discarded = (s1 != d);

			if (FFT_state (s1) == FULLY_FFTed && FFT_state (s2) == FULLY_FFTed) preferred_FFT_arg = s1;
			
			else if (FFT_state (s1) != FULLY_FFTed && (options & GWMUL_PRESERVE_S1) && s1_can_be_FFTed_and_discarded) preferred_FFT_arg = s1;
			else if (FFT_state (s2) != FULLY_FFTed && (options & GWMUL_PRESERVE_S2) && s2_can_be_FFTed_and_discarded) preferred_FFT_arg = s2;
			
			else if (FFT_state (s1) != FULLY_FFTed && !(options & GWMUL_FFT_S1) && s1_can_be_FFTed_and_discarded) preferred_FFT_arg = s1;
			else if (FFT_state (s2) != FULLY_FFTed && !(options & GWMUL_FFT_S2) && s2_can_be_FFTed_and_discarded) preferred_FFT_arg = s2;
			
			else if (FFT_state (s1) != FULLY_FFTed && !(options & GWMUL_PRESERVE_S1)) preferred_FFT_arg = s1, asm_data->DEST2ARG = d;
			else if (FFT_state (s2) != FULLY_FFTed && !(options & GWMUL_PRESERVE_S2)) preferred_FFT_arg = s2, asm_data->DEST2ARG = d;
			
			else if (FFT_state (s1) == FULLY_FFTed) preferred_FFT_arg = s1;
			else if (FFT_state (s2) == FULLY_FFTed) preferred_FFT_arg = s2;
			else ASSERTG (FALSE);

			if (preferred_FFT_arg != s1 && FFT_state (s1) != FULLY_FFTed) {
				if (options & GWMUL_PRESERVE_S1) { tmp1 = gwalloc (gwdata); gwfft (gwdata, s1, tmp1); s1 = tmp1; }
				else gwfft (gwdata, s1, s1);
			}
			if (preferred_FFT_arg != s2 && FFT_state (s2) != FULLY_FFTed) {
				if (options & GWMUL_PRESERVE_S2) { tmp2 = gwalloc (gwdata); gwfft (gwdata, s2, tmp2); s2 = tmp2; }
				else gwfft (gwdata, s2, s2);
			}

			if (FFT_state (preferred_FFT_arg) == FULLY_FFTed) asm_data->ffttype = 4;
			else {
				asm_data->ffttype = 3;
				if (preferred_FFT_arg == s2) {
					s2 = s1;
					s1 = preferred_FFT_arg;
				}
			}
		}
	}

	else {

		if (s1 == s2) {
			if (FFT_state (s1) == FULLY_FFTed) asm_data->ffttype = 4;
			else if (options & GWMUL_FFT_S12) gwfft (gwdata, s1, s1), asm_data->ffttype = 4;
			else asm_data->ffttype = 2;
		}

		else {
			if (s2 == d) { swaps1s2 (); }						
			else if (s1 == d);							
			else if (FFT_state (s2) == FULLY_FFTed || (options & GWMUL_FFT_S2));	
			else if (FFT_state (s1) == FULLY_FFTed || (options & GWMUL_FFT_S1)) { swaps1s2 (); } 
			
			else if	(!(options & GWMUL_PRESERVE_S2));				
			else if	(!(options & GWMUL_PRESERVE_S1)) { swaps1s2 (); }		
			
			if (FFT_state (s2) != FULLY_FFTed) {
				if (options & GWMUL_PRESERVE_S2) { tmp2 = gwalloc (gwdata); gwfft (gwdata, s2, tmp2); s2 = tmp2; }
				else gwfft (gwdata, s2, s2);
			}

			if (FFT_state (s1) == FULLY_FFTed) asm_data->ffttype = 4;
			else if (options & GWMUL_FFT_S1) gwfft (gwdata, s1, s1), asm_data->ffttype = 4;
			else asm_data->ffttype = 3;
		}
	}

	if (gwdata->ZERO_PADDED_FFT && FFT_state (s1) == PARTIALLY_FFTed && s1 != d) memcpy (&d[-11], &s1[-11], 7 * sizeof (double));

	if (FFT_state (s1) == NOT_FFTed) gwdata->read_count += 1, gwdata->write_count += 1;

	if (asm_data->DEST2ARG) {
		asm_data->mul4_opcode = 0x80;
		asm_data->DEST2ARG = (void *) ((intptr_t) asm_data->DEST2ARG - (intptr_t) s1);  
		asm_mul (gwdata, s1, s2, s1, options);
	} else {
		asm_data->mul4_opcode = 0;
		asm_mul (gwdata, s1, s2, d, options);
	}

	gwdata->read_count += 3, gwdata->write_count += 2;
	if (s1 == s2) gwdata->read_count -= 1;
	if (asm_data->DEST2ARG) gwdata->write_count += 1;

	if (tmp1 != NULL) gwfree (gwdata, tmp1);
	if (tmp2 != NULL) gwfree (gwdata, tmp2);
}

void emulate_mod (
	gwhandle *gwdata,	
	gwnum	s)		
{
	struct gwasm_data *asm_data = (struct gwasm_data *) gwdata->asm_data;
	gwnum	tmp;

	ASSERTG (* addr (gwdata, s, gwdata->FFTLEN-1) > -2.0 && * addr (gwdata, s, gwdata->FFTLEN-1) <= 0.0);
	if (* addr (gwdata, s, gwdata->FFTLEN-1) <= -2.0 || * addr (gwdata, s, gwdata->FFTLEN-1) > 0.0)
		gwdata->GWERROR = GWERROR_INTERNAL + 10;

	tmp = gwalloc (gwdata);
	gwcopyzero (gwdata, s, tmp, gwdata->GW_ZEROWORDSLOW);

	asm_data->NORMRTN = gwdata->GWPROCPTRS[zerohigh_routines + gwdata->ERROR_CHECKING];
	raw_gwmul3 (gwdata, gwdata->GW_RECIP_FFT, tmp, tmp, GWMUL_PRESERVE_S1);
	ASSERTG (* addr (gwdata, tmp, gwdata->FFTLEN/2-1) > -2.0 && * addr (gwdata, tmp, gwdata->FFTLEN/2-1) <= 0.0);
	if (* addr (gwdata, tmp, gwdata->FFTLEN/2-1) <= -2.0 || * addr (gwdata, tmp, gwdata->FFTLEN/2-1) > 0.0)
		gwdata->GWERROR = GWERROR_INTERNAL + 11;

	asm_data->NORMRTN = gwdata->GWPROCPTRS[norm_routines + gwdata->ERROR_CHECKING];
	raw_gwmul3 (gwdata, gwdata->GW_MODULUS_FFT, tmp, tmp, GWMUL_PRESERVE_S1);

		gwsub3 (gwdata, s, tmp, s);
		unnorms (s) = 0.0f;
	
	ASSERTG (* addr (gwdata, s, gwdata->FFTLEN-1) == 0.0);
	gwfree (gwdata, tmp);
}

void gwunfft2 (
	gwhandle *gwdata,	
	gwnum	s,		
	gwnum	d,		
	int	options)	
{
	int	save_count;

	ASSERTG (unnorms (s) >= 0.0f);

	if (FFT_state (s) == NOT_FFTed) {
		ASSERTG (! (options & (GWMUL_ADDINCONST | GWMUL_MULBYCONST)));		
		if (s != d) gwcopy (gwdata, s, d);
		return;
	}

	if (gwdata->GENERAL_MMGW_MOD) {
		gwunfft2 (gwdata->cyclic_gwdata, cyclic_gwnum (gwdata, s), cyclic_gwnum (gwdata, d), options);
		return;
	}

	save_count = gwdata->careful_count;
	gwdata->careful_count = 0;

	unnorms (s) = 0.0f;

	if (FFT_state (s) == PARTIALLY_FFTed || FFT_state (s) == FULLY_FFTed) {

		init_FFT1 (gwdata);
		if (gwdata->GW_FFT1 != NULL) {
			gwmul3 (gwdata, s, gwdata->GW_FFT1, d, options);
		}

#ifdef X86_64
		else if (gwdata->cpu_flags & (CPU_AVX512F | CPU_FMA3 | CPU_AVX)) {
			ASSERTG (gwdata->k == 1.0);
			struct gwasm_data *asm_data = (struct gwasm_data *) gwdata->asm_data;
			if (options & GWMUL_MULBYCONST) asm_data->NORMRTN = gwdata->GWPROCPTRS[norm_routines + 2 + gwdata->ERROR_CHECKING];
			else asm_data->NORMRTN = gwdata->GWPROCPTRS[norm_routines + gwdata->ERROR_CHECKING];
			asm_data->mul4_opcode = 5;
			asm_data->SRC3ARG = (void *) 1;			
			asm_data->DEST2ARG = 0;
			if (FFT_state (s) == FULLY_FFTed) asm_data->ffttype = 4;
			else asm_data->ffttype = 3;
			asm_mul (gwdata, s, s, d, options);
			gwdata->read_count += 2, gwdata->write_count += 2;
			if (gwdata->GENERAL_MOD) emulate_mod (gwdata, d);
		}
#endif

		else if (s != d) {
			dbltogw (gwdata, 1.0, d);
			gwmul3 (gwdata, s, d, d, options);
		} else {
			gwnum	tmp = gwalloc (gwdata);
			dbltogw (gwdata, 1.0, tmp);
			gwmul3 (gwdata, s, tmp, d, options);
			gwfree (gwdata, tmp);
		}
	}

	if (FFT_state (s) == FFTed_FOR_FMA) {
		ASSERTG (gwdata->GENERAL_MMGW_MOD || gwdata->k != 1.0);

		if (gwdata->GENERAL_MMGW_MOD) {
			if (s != d) gwcopy (gwdata, s, d);				
			FFT_state (cyclic_gwnum (gwdata, d)) = FULLY_FFTed;		
			FFT_state (negacyclic_gwnum (gwdata, d)) = FULLY_FFTed;
			unnorms (negacyclic_gwnum (gwdata, d)) = unnorms (cyclic_gwnum (gwdata, d));
			ASSERTG (! (options & (GWMUL_MULBYCONST | GWMUL_ADDINCONST)));
			gwunfft2 (gwdata->cyclic_gwdata, cyclic_gwnum (gwdata, s), cyclic_gwnum (gwdata, d), GWMUL_STARTNEXTFFT);
			MMGW_redc4 (gwdata, cyclic_gwnum (gwdata, d), negacyclic_gwnum (gwdata, d));
		}

#ifdef X86_64
		else if (gwdata->cpu_flags & (CPU_AVX512F | CPU_FMA3 | CPU_AVX)) {
			struct gwasm_data *asm_data = (struct gwasm_data *) gwdata->asm_data;
			if (options & GWMUL_MULBYCONST) asm_data->NORMRTN = gwdata->GWPROCPTRS[norm_routines + 2 + gwdata->ERROR_CHECKING];
			else asm_data->NORMRTN = gwdata->GWPROCPTRS[norm_routines + gwdata->ERROR_CHECKING];
			asm_data->mul4_opcode = 5;
			asm_data->SRC3ARG = (void *) 2;			
			asm_data->DEST2ARG = 0;
			asm_data->ffttype = 4;
			asm_mul (gwdata, s, s, d, options);
			gwdata->read_count += 2, gwdata->write_count += 2;
			if (gwdata->GENERAL_MOD) emulate_mod (gwdata, d);
		}
#endif
		
		else {
			gwnum	tmp = gwalloc (gwdata);
			dbltogw (gwdata, gwdata->k, tmp);		
			gwfft (gwdata, tmp, tmp);
			if (gwdata->ZERO_PADDED_FFT) {	
				if (gwdata->cpu_flags & CPU_AVX512F) tmp[-5] = 1.0;
				else tmp[-8] = 1.0;
			}

			struct gwasm_data *asm_data = (struct gwasm_data *) gwdata->asm_data;
			if (options & GWMUL_MULBYCONST) asm_data->NORMRTN = gwdata->GWPROCPTRS[norm_routines + 2 + gwdata->ERROR_CHECKING];
			else asm_data->NORMRTN = gwdata->GWPROCPTRS[norm_routines + gwdata->ERROR_CHECKING];
			asm_data->mul4_opcode = 0;
			asm_data->DEST2ARG = 0;
			asm_data->ffttype = 4;
			asm_mul (gwdata, tmp, s, d, options);
			gwdata->read_count += 2, gwdata->write_count += 2;
			if (gwdata->GENERAL_MOD) emulate_mod (gwdata, d);

			gwfree (gwdata, tmp);
		}
	}

	gwdata->careful_count = save_count;
}

void gwunfft (
	gwhandle *gwdata,	
	gwnum	s,		
	gwnum	d)		
{
	gwunfft2 (gwdata, s, d, 0);	
}

void cmn_gwopmul4 (		
	gwhandle *gwdata,	
	gwnum	s1,		
	gwnum	s2,		
	gwnum	s3,		
	gwnum	d,		
	int	options,	
	int	opcode)		
{
	gwnum	tmp1, tmp2, tmp3;

	tmp1 = tmp2 = tmp3 = NULL;

	if (s1 == d) options &= ~(GWMUL_FFT_S1 | GWMUL_PRESERVE_S1);
	if (s2 == d) options &= ~(GWMUL_FFT_S2 | GWMUL_PRESERVE_S2);
	if (s3 == d) options &= ~(GWMUL_FFT_S3 | GWMUL_PRESERVE_S3);

	if (s1 == s3) options |= ((options & (GWMUL_FFT_S1 | GWMUL_PRESERVE_S1)) << 4) | ((options & (GWMUL_FFT_S3 | GWMUL_PRESERVE_S3)) >> 4);
	if (s2 == s3) options |= ((options & (GWMUL_FFT_S2 | GWMUL_PRESERVE_S2)) << 2) | ((options & (GWMUL_FFT_S3 | GWMUL_PRESERVE_S3)) >> 2);

	if ((options & GWMUL_ADDINCONST) && (gwdata->emulate_addin_value != 0 || gwdata->emulate_postaddin_value != 0)) {
		cmn_gwopmul4 (gwdata, s1, s2, s3, d, options & ~(GWMUL_MULBYCONST | GWMUL_ADDINCONST | GWMUL_STARTNEXTFFT), opcode);
		init_GW_ADDIN (gwdata);
		if (gwdata->GW_ADDIN != NULL) gwadd3o (gwdata, d, gwdata->GW_ADDIN, d, GWADD_FORCE_NORMALIZE);
		if (options & GWMUL_MULBYCONST) gwsmallmul (gwdata, gwdata->mulbyconst, d);
		if (gwdata->GW_POSTADDIN != NULL) gwadd3o (gwdata, d, gwdata->GW_POSTADDIN, d, GWADD_FORCE_NORMALIZE);
	}

	else if (gwdata->GENERAL_MMGW_MOD) {
		
		if (FFT_state (s1) != FULLY_FFTed) {
			if (options & GWMUL_PRESERVE_S1) { tmp1 = gwalloc (gwdata); gwfft (gwdata, s1, tmp1); s1 = tmp1; }
			else gwfft (gwdata, s1, s1);
		}
		if (FFT_state (s2) != FULLY_FFTed) {
			if (options & GWMUL_PRESERVE_S2) { tmp2 = gwalloc (gwdata); gwfft (gwdata, s2, tmp2); s2 = tmp2; }
			else gwfft (gwdata, s2, s2);
		}
		if (FFT_state (s3) != FULLY_FFTed && options & GWMUL_FFT_S3) gwfft (gwdata, s3, s3);

		gwnum s1R = cyclic_gwnum (gwdata, s1);
		gwnum s1Q = negacyclic_gwnum (gwdata, s1);
		gwnum s2R = cyclic_gwnum (gwdata, s2);
		gwnum s2Q = negacyclic_gwnum (gwdata, s2);
		gwnum s3R = cyclic_gwnum (gwdata, s3);
		gwnum s3Q = negacyclic_gwnum (gwdata, s3);
		gwnum T_R = cyclic_gwnum (gwdata, d);	
		gwnum T_Q = negacyclic_gwnum (gwdata, d);

		if (FFT_state (s3R) != FULLY_FFTed) {					
			if (s3 != d && !(options & GWMUL_PRESERVE_S3) && (s1 == d || s2 == d))
				gwfft (gwdata, s3, s3);					
			else
				s3Q = s3R;						
		}

		cmn_gwopmul4 (gwdata->negacyclic_gwdata, s1Q, s2Q, s3Q, T_Q, options & ~GWMUL_STARTNEXTFFT, opcode);

		cmn_gwopmul4 (gwdata->cyclic_gwdata, s1R, s2R, s3R, T_R, options | GWMUL_STARTNEXTFFT, opcode);

		MMGW_redc (gwdata, T_R, T_Q);
	}

#ifdef X86_64
	else if (!(gwdata->cpu_flags & (CPU_AVX512F | CPU_FMA3 | CPU_AVX)) || gwdata->careful_count > 0) {
#else
	else if (1) {
#endif
		tmp1 = gwalloc (gwdata);
		if (opcode == 1) gwadd3o (gwdata, s1, s2, tmp1, GWADD_DELAY_NORMALIZE);
		else gwsub3o (gwdata, s1, s2, tmp1, GWADD_DELAY_NORMALIZE);
		gwmul3 (gwdata, tmp1, s3, d, (options & ~0x3F) + ((options & 0x30) >> 2));	
	}

	else {
		struct gwasm_data *asm_data = (struct gwasm_data *) gwdata->asm_data;
		gwnum	preferred_FFT_arg;

		asm_data->DEST2ARG = 0;

		bool s1_can_be_FFTed_and_discarded = (s2 != d && s3 != d);
		bool s2_can_be_FFTed_and_discarded = (s1 != d && s3 != d);
		bool s3_can_be_FFTed_and_discarded = (s1 != d && s2 != d);

		if ((FFT_state (s1) == FULLY_FFTed && FFT_state (s2) == FULLY_FFTed && FFT_state (s3) == FULLY_FFTed) || s1 == s2) preferred_FFT_arg = s3;
		
		else if (FFT_state (s3) != FULLY_FFTed && (options & GWMUL_PRESERVE_S3) && s3_can_be_FFTed_and_discarded) preferred_FFT_arg = s3;
		else if (FFT_state (s1) != FULLY_FFTed && (options & GWMUL_PRESERVE_S1) && s1_can_be_FFTed_and_discarded) preferred_FFT_arg = s1;
		else if (FFT_state (s2) != FULLY_FFTed && (options & GWMUL_PRESERVE_S2) && s2_can_be_FFTed_and_discarded) preferred_FFT_arg = s2;
		
		else if (FFT_state (s3) != FULLY_FFTed && !(options & GWMUL_FFT_S3) && s3_can_be_FFTed_and_discarded) preferred_FFT_arg = s3;
		else if (FFT_state (s1) != FULLY_FFTed && !(options & GWMUL_FFT_S1) && s1_can_be_FFTed_and_discarded) preferred_FFT_arg = s1;
		else if (FFT_state (s2) != FULLY_FFTed && !(options & GWMUL_FFT_S2) && s2_can_be_FFTed_and_discarded) preferred_FFT_arg = s2;
		
		else if (FFT_state (s1) != FULLY_FFTed && !(options & GWMUL_PRESERVE_S1)) preferred_FFT_arg = s1, asm_data->DEST2ARG = d;
		else if (FFT_state (s2) != FULLY_FFTed && !(options & GWMUL_PRESERVE_S2)) preferred_FFT_arg = s2, asm_data->DEST2ARG = d;
		else if (FFT_state (s3) != FULLY_FFTed && !(options & GWMUL_PRESERVE_S3)) preferred_FFT_arg = s3, asm_data->DEST2ARG = d;
		
		else if (FFT_state (s1) == FULLY_FFTed) preferred_FFT_arg = s1;
		else if (FFT_state (s2) == FULLY_FFTed) preferred_FFT_arg = s2;
		else if (FFT_state (s3) == FULLY_FFTed) preferred_FFT_arg = s3;
		else ASSERTG (FALSE);

		if (preferred_FFT_arg != s1 && FFT_state (s1) != FULLY_FFTed) {
			if (options & GWMUL_PRESERVE_S1) { tmp1 = gwalloc (gwdata); gwfft (gwdata, s1, tmp1); s1 = tmp1; }
			else gwfft (gwdata, s1, s1);
		}
		if (preferred_FFT_arg != s2 && FFT_state (s2) != FULLY_FFTed) {
			if (options & GWMUL_PRESERVE_S2) { tmp2 = gwalloc (gwdata); gwfft (gwdata, s2, tmp2); s2 = tmp2; }
			else gwfft (gwdata, s2, s2);
		}
		if (preferred_FFT_arg != s3 && FFT_state (s3) != FULLY_FFTed) {
			if (options & GWMUL_PRESERVE_S3) { tmp3 = gwalloc (gwdata); gwfft (gwdata, s3, tmp3); s3 = tmp3; }
			else gwfft (gwdata, s3, s3);
		}

		if (FFT_state (preferred_FFT_arg) == FULLY_FFTed) asm_data->ffttype = 4;
		else {
			asm_data->ffttype = 3;
			if (preferred_FFT_arg == s1) {
				opcode = (opcode-1) * 4 + ((preferred_FFT_arg == s3) ? 7 : 6);		
				s1 = s2;
				s2 = s3;
				s3 = preferred_FFT_arg;
			}
			else if (preferred_FFT_arg == s2) {
				opcode = (opcode-1) * 4 + ((preferred_FFT_arg == s3) ? 9: 8);		
				s2 = s3;
				s3 = preferred_FFT_arg;
			}
		}

		if (FFT_state (s3) == NOT_FFTed) gwdata->read_count += 1, gwdata->write_count += 1;

		if (options & GWMUL_MULBYCONST) asm_data->NORMRTN = gwdata->GWPROCPTRS[norm_routines + 2 + gwdata->ERROR_CHECKING];
		else asm_data->NORMRTN = gwdata->GWPROCPTRS[norm_routines + gwdata->ERROR_CHECKING];
		if (asm_data->DEST2ARG) {
			asm_data->mul4_opcode = 0x80 | opcode;
			asm_data->SRC2ARG = (void *) ((intptr_t) s2 - (intptr_t) s3);
			asm_data->DEST2ARG = (void *) ((intptr_t) asm_data->DEST2ARG - (intptr_t) s3);
			asm_mul (gwdata, s3, s1, s3, options);
		} else {
			asm_data->mul4_opcode = opcode;
			asm_data->SRC2ARG = (void *) ((intptr_t) s2 - (intptr_t) d);
			asm_mul (gwdata, s3, s1, d, options);
		}

		gwdata->read_count += 4, gwdata->write_count += 2;
		if (s1 == s2 || s1 == s3) gwdata->read_count -= 1;
		if (s2 == s3) gwdata->read_count -= 1;
		if (asm_data->DEST2ARG) gwdata->write_count += 1;

		if (gwdata->GENERAL_MOD) emulate_mod (gwdata, d);
	}

	if (tmp1 != NULL) gwfree (gwdata, tmp1);
	if (tmp2 != NULL) gwfree (gwdata, tmp2);
	if (tmp3 != NULL) gwfree (gwdata, tmp3);
}

void cmn_gwmulmulop5 (		
	gwhandle *gwdata,	
	gwnum	s1,		
	gwnum	s2,		
	gwnum	s3,		
	gwnum	s4,		
	gwnum	d,		
	int	options,	
	int	opcode)		
{
	gwnum	tmp1, tmp2, tmp3, tmp4;

	tmp1 = tmp2 = tmp3 = tmp4 = NULL;

	if (s1 == d) options &= ~(GWMUL_FFT_S1 | GWMUL_PRESERVE_S1);
	if (s2 == d) options &= ~(GWMUL_FFT_S2 | GWMUL_PRESERVE_S2);
	if (s3 == d) options &= ~(GWMUL_FFT_S3 | GWMUL_PRESERVE_S3);
	if (s4 == d) options &= ~(GWMUL_FFT_S4 | GWMUL_PRESERVE_S4);

	if (opcode == 3 && FFT_state (s1) == FULLY_FFTed && FFT_state (s2) == FULLY_FFTed) {
		gwswap (s1, s3);
		gwswap (s2, s4);
	}

	if (FFT_state (s3) != FULLY_FFTed) {
		if (options & GWMUL_PRESERVE_S3) { tmp3 = gwalloc (gwdata); gwfft (gwdata, s3, tmp3); s3 = tmp3; }
		else gwfft (gwdata, s3, s3);
	}
	if (FFT_state (s4) != FULLY_FFTed) {
		if (options & GWMUL_PRESERVE_S4) { tmp4 = gwalloc (gwdata); gwfft (gwdata, s4, tmp4); s4 = tmp4; }
		else gwfft (gwdata, s4, s4);
	}

	if ((options & GWMUL_ADDINCONST) && (gwdata->emulate_addin_value != 0 || gwdata->emulate_postaddin_value != 0)) {
		cmn_gwmulmulop5 (gwdata, s1, s2, s3, s4, d, options & ~(GWMUL_MULBYCONST | GWMUL_ADDINCONST | GWMUL_STARTNEXTFFT), opcode);
		init_GW_ADDIN (gwdata);
		if (gwdata->GW_ADDIN != NULL) gwadd3o (gwdata, d, gwdata->GW_ADDIN, d, GWADD_FORCE_NORMALIZE);
		if (options & GWMUL_MULBYCONST) gwsmallmul (gwdata, gwdata->mulbyconst, d);
		if (gwdata->GW_POSTADDIN != NULL) gwadd3o (gwdata, d, gwdata->GW_POSTADDIN, d, GWADD_FORCE_NORMALIZE);
	}

	else if (gwdata->GENERAL_MMGW_MOD) {
		int T_Q_options = options;

		gwnum s1R = cyclic_gwnum (gwdata, s1);
		gwnum s1Q = negacyclic_gwnum (gwdata, s1);
		gwnum s2R = cyclic_gwnum (gwdata, s2);
		gwnum s2Q = negacyclic_gwnum (gwdata, s2);
		gwnum s3R = cyclic_gwnum (gwdata, s3);
		gwnum s3Q = negacyclic_gwnum (gwdata, s3);
		gwnum s4R = cyclic_gwnum (gwdata, s4);
		gwnum s4Q = negacyclic_gwnum (gwdata, s4);
		gwnum T_R = cyclic_gwnum (gwdata, d);	
		gwnum T_Q = negacyclic_gwnum (gwdata, d);

		if (options & GWMUL_FFT_S1) gwfft (gwdata, s1, s1);
		if (options & GWMUL_FFT_S2) gwfft (gwdata, s2, s2);

		if (s1 == s2) {									
			
			if (FFT_state (s1R) != FULLY_FFTed) {					
				if (s1 != d && !(options & GWMUL_PRESERVE_S1) && (!(gwdata->cpu_flags & CPU_AVX512F) || s3 == d || s4 == d))
					gwfft (gwdata, s1, s1);					
				else if (s3 != d && s4 != d) {					
					s1Q = s2Q = s1R;					
					T_Q_options |= GWMUL_PRESERVE_S1 | GWMUL_PRESERVE_S2;
				} else
					gwfft (gwdata->negacyclic_gwdata, s1R, s1Q);		
			}
		} else {									
			
			if (FFT_state (s1R) != FULLY_FFTed) {					
				if (s1 != d && !(options & GWMUL_PRESERVE_S1) && (FFT_state (s2R) != FULLY_FFTed || s2 == d || s3 == d || s4 == d))
					gwfft (gwdata, s1, s1);					
				else if (s2 != d && s3 != d && s4 != d)				
					s1Q = s1R, T_Q_options |= GWMUL_PRESERVE_S1;		
				else
					gwfft (gwdata->negacyclic_gwdata, s1R, s1Q);		
			}

			if (FFT_state (s2R) != FULLY_FFTed) {					
				if (s2 != d && !(options & GWMUL_PRESERVE_S2) && (FFT_state (s1R) != FULLY_FFTed || s1 == d || s3 == d || s4 == d))
					gwfft (gwdata, s2, s2);					
				else if (s1 != d && s3 != d && s4 != d && s1Q != s1R)		
					s2Q = s2R, T_Q_options |= GWMUL_PRESERVE_S2;		
				else
					gwfft (gwdata->negacyclic_gwdata, s2R, s2Q);		
			}
		}

		cmn_gwmulmulop5 (gwdata->negacyclic_gwdata, s1Q, s2Q, s3Q, s4Q, T_Q, T_Q_options & ~GWMUL_STARTNEXTFFT, opcode);

		cmn_gwmulmulop5 (gwdata->cyclic_gwdata, s1R, s2R, s3R, s4R, T_R, options | GWMUL_STARTNEXTFFT, opcode);

		MMGW_redc (gwdata, T_R, T_Q);
	}

#ifdef X86_64
	else if (!(gwdata->cpu_flags & (CPU_AVX512F | CPU_FMA3 | CPU_AVX)) || gwdata->careful_count > 0) {
#else
	else if (1) {
#endif

		tmp1 = gwalloc (gwdata);
		gwmul3 (gwdata, s3, s4, tmp1, 0);
		if ((options & GWMUL_MULBYCONST) && (options & GWMUL_ADDINCONST) && gwdata->asm_postaddin_value != 0.0) {
			
			double saved_postaddin_value = gwdata->asm_postaddin_value;
			gwdata->asm_postaddin_value = 0.0;
			gwmul3 (gwdata, s1, s2, d, options & ~(GWMUL_MULBYCONST | GWMUL_STARTNEXTFFT));
			gwdata->asm_postaddin_value = saved_postaddin_value;
			
			if (opcode == 3) gwadd3o (gwdata, d, tmp1, d, GWADD_GUARANTEED_OK | GWADD_DELAY_NORMALIZE);
			else gwsub3o (gwdata, d, tmp1, d, GWADD_GUARANTEED_OK | GWADD_DELAY_NORMALIZE);
			
			dbltogw (gwdata, gwdata->mulbyconst, tmp1);
			double saved_addin_value = gwdata->asm_addin_value;
			gwdata->asm_addin_value = 0.0;
			gwmul3 (gwdata, d, tmp1, d, options & ~GWMUL_MULBYCONST);
			gwdata->asm_addin_value = saved_addin_value;
		} else if (options & GWMUL_MULBYCONST) {
			gwmul3 (gwdata, s1, s2, d, options & ~(GWMUL_MULBYCONST | GWMUL_STARTNEXTFFT));
			if (opcode == 3) gwadd3o (gwdata, d, tmp1, d, GWADD_GUARANTEED_OK | GWADD_DELAY_NORMALIZE);
			else gwsub3o (gwdata, d, tmp1, d, GWADD_GUARANTEED_OK | GWADD_DELAY_NORMALIZE);
			gwsmallmul (gwdata, gwdata->mulbyconst, d);
		} else {
			gwmul3 (gwdata, s1, s2, d, options & ~GWMUL_STARTNEXTFFT);
			if (opcode == 3) gwadd3o (gwdata, d, tmp1, d, GWADD_FORCE_NORMALIZE);
			else gwsub3o (gwdata, d, tmp1, d, GWADD_FORCE_NORMALIZE);
		}
	}

	else {
		struct gwasm_data *asm_data = (struct gwasm_data *) gwdata->asm_data;

		asm_data->DEST2ARG = 0;

		if (s1 == s2 && !(gwdata->cpu_flags & CPU_AVX512F) && FFT_state (s1) != FULLY_FFTed) {
			if (options & GWMUL_PRESERVE_S1) { tmp1 = gwalloc (gwdata); gwfft (gwdata, s1, tmp1); s1 = tmp1; }
			else gwfft (gwdata, s1, s1);
		}

		if (s1 == s2) {
			if (FFT_state (s1) == FULLY_FFTed) asm_data->ffttype = 4;
			else if (!(options & GWMUL_FFT_S12) && s3 != d && s4 != d) asm_data->ffttype = 2;
			else if (!(gwdata->cpu_flags & (CPU_AVX512F))) gwfft (gwdata, s1, s1), asm_data->ffttype = 4;
			else {	
				asm_data->ffttype = 2;
				asm_data->DEST2ARG = d;
			}
		}

		else {
			gwnum	preferred_FFT_arg;	

			bool s1_can_be_FFTed_and_discarded = (s2 != d && s3 != d && s4 != d);
			bool s2_can_be_FFTed_and_discarded = (s1 != d && s3 != d && s4 != d);

			if (FFT_state (s1) == FULLY_FFTed && FFT_state (s2) == FULLY_FFTed) preferred_FFT_arg = s1;
			
			else if (FFT_state (s1) != FULLY_FFTed && (options & GWMUL_PRESERVE_S1) && s1_can_be_FFTed_and_discarded) preferred_FFT_arg = s1;
			else if (FFT_state (s2) != FULLY_FFTed && (options & GWMUL_PRESERVE_S2) && s2_can_be_FFTed_and_discarded) preferred_FFT_arg = s2;
			
			else if (FFT_state (s1) != FULLY_FFTed && !(options & GWMUL_FFT_S1) && s1_can_be_FFTed_and_discarded) preferred_FFT_arg = s1;
			else if (FFT_state (s2) != FULLY_FFTed && !(options & GWMUL_FFT_S2) && s2_can_be_FFTed_and_discarded) preferred_FFT_arg = s2;
			
			else if (FFT_state (s1) != FULLY_FFTed && !(options & GWMUL_PRESERVE_S1)) preferred_FFT_arg = s1, asm_data->DEST2ARG = d;
			else if (FFT_state (s2) != FULLY_FFTed && !(options & GWMUL_PRESERVE_S2)) preferred_FFT_arg = s2, asm_data->DEST2ARG = d;
			
			else if (FFT_state (s1) == FULLY_FFTed) preferred_FFT_arg = s1;
			else if (FFT_state (s2) == FULLY_FFTed) preferred_FFT_arg = s2;
			else ASSERTG (FALSE);

			if (preferred_FFT_arg != s1 && FFT_state (s1) != FULLY_FFTed) {
				if (options & GWMUL_PRESERVE_S1) { tmp1 = gwalloc (gwdata); gwfft (gwdata, s1, tmp1); s1 = tmp1; }
				else gwfft (gwdata, s1, s1);
			}
			if (preferred_FFT_arg != s2 && FFT_state (s2) != FULLY_FFTed) {
				if (options & GWMUL_PRESERVE_S2) { tmp2 = gwalloc (gwdata); gwfft (gwdata, s2, tmp2); s2 = tmp2; }
				else gwfft (gwdata, s2, s2);
			}

			if (FFT_state (preferred_FFT_arg) == FULLY_FFTed) asm_data->ffttype = 4;
			else {
				asm_data->ffttype = 3;
				if (preferred_FFT_arg == s2) {
					s2 = s1;
					s1 = preferred_FFT_arg;
				}
			}
		}

		if (FFT_state (s1) == NOT_FFTed) gwdata->read_count += 1, gwdata->write_count += 1;

		if (options & GWMUL_MULBYCONST) asm_data->NORMRTN = gwdata->GWPROCPTRS[norm_routines + 2 + gwdata->ERROR_CHECKING];
		else asm_data->NORMRTN = gwdata->GWPROCPTRS[norm_routines + gwdata->ERROR_CHECKING];
		if (asm_data->DEST2ARG) {
			asm_data->mul4_opcode = 0x80 | opcode;
			asm_data->SRC2ARG = (void *) ((intptr_t) s3 - (intptr_t) s1);
			asm_data->SRC3ARG = (void *) ((intptr_t) s4 - (intptr_t) s1);
			asm_data->DEST2ARG = (void *) ((intptr_t) asm_data->DEST2ARG - (intptr_t) s1);
			asm_mul (gwdata, s1, s2, s1, options);
		} else {
			asm_data->mul4_opcode = opcode;
			asm_data->SRC2ARG = (void *) ((intptr_t) s3 - (intptr_t) d);
			asm_data->SRC3ARG = (void *) ((intptr_t) s4 - (intptr_t) d);
			asm_mul (gwdata, s1, s2, d, options);
		}

		gwdata->read_count += 5, gwdata->write_count += 2;
		if (s1 == s2 || s1 == s3 || s1 == s4) gwdata->read_count -= 1;
		if (s2 == s3 || s2 == s4) gwdata->read_count -= 1;
		if (s3 == s4) gwdata->read_count -= 1;
		if (asm_data->DEST2ARG) gwdata->write_count += 1;

		if (gwdata->GENERAL_MOD) emulate_mod (gwdata, d);
	}

	if (tmp1 != NULL) gwfree (gwdata, tmp1);
	if (tmp2 != NULL) gwfree (gwdata, tmp2);
	if (tmp3 != NULL) gwfree (gwdata, tmp3);
	if (tmp4 != NULL) gwfree (gwdata, tmp4);
}

void cmn_gwmulop4 (		
	gwhandle *gwdata,	
	gwnum	s1,		
	gwnum	s2,		
	gwnum	s3,		
	gwnum	d,		
	int	options,	
	int	opcode)		
{
	gwnum	tmp1, tmp2, tmp3;

	tmp1 = tmp2 = tmp3 = NULL;

	if (s1 == d) options &= ~(GWMUL_FFT_S1 | GWMUL_PRESERVE_S1);
	if (s2 == d) options &= ~(GWMUL_FFT_S2 | GWMUL_PRESERVE_S2);
	if (s3 == d) options &= ~(GWMUL_FFT_S3 | GWMUL_PRESERVE_S3);

	if (FFT_state (s3) != FFTed_FOR_FMA) {
		init_FFT1 (gwdata);
		if (gwdata->GW_FFT1 != NULL) {
			cmn_gwmulmulop5 (gwdata, s1, s2, s3, gwdata->GW_FFT1, d, options, opcode);
			return;
		}
	}

	if ((options & GWMUL_ADDINCONST) && (gwdata->emulate_addin_value != 0 || gwdata->emulate_postaddin_value != 0)) {
		cmn_gwmulop4 (gwdata, s1, s2, s3, d, options & ~(GWMUL_MULBYCONST | GWMUL_ADDINCONST | GWMUL_STARTNEXTFFT), opcode);
		init_GW_ADDIN (gwdata);
		if (gwdata->GW_ADDIN != NULL) gwadd3o (gwdata, d, gwdata->GW_ADDIN, d, GWADD_FORCE_NORMALIZE);
		if (options & GWMUL_MULBYCONST) gwsmallmul (gwdata, gwdata->mulbyconst, d);
		if (gwdata->GW_POSTADDIN != NULL) gwadd3o (gwdata, d, gwdata->GW_POSTADDIN, d, GWADD_FORCE_NORMALIZE);
	}

	else if (gwdata->GENERAL_MMGW_MOD) {
		int T_Q_options = options;

		gwnum s1R = cyclic_gwnum (gwdata, s1);
		gwnum s1Q = negacyclic_gwnum (gwdata, s1);
		gwnum s2R = cyclic_gwnum (gwdata, s2);
		gwnum s2Q = negacyclic_gwnum (gwdata, s2);
		gwnum s3R = cyclic_gwnum (gwdata, s3);
		gwnum s3Q = negacyclic_gwnum (gwdata, s3);
		gwnum T_R = cyclic_gwnum (gwdata, d);	
		gwnum T_Q = negacyclic_gwnum (gwdata, d);

		if (options & GWMUL_FFT_S1) gwfft (gwdata, s1, s1);
		if (options & GWMUL_FFT_S2) gwfft (gwdata, s2, s2);

		if (s1 == s2) {									
			
			if (FFT_state (s1R) != FULLY_FFTed) {					
				if (s1 != d && !(options & GWMUL_PRESERVE_S1) && (!(gwdata->cpu_flags & CPU_AVX512F) || s3 == d))
					gwfft (gwdata, s1, s1);					
				else if (s3 != d) {						
					s1Q = s2Q = s1R;					
					T_Q_options |= GWMUL_PRESERVE_S1 | GWMUL_PRESERVE_S2;
				} else
					gwfft (gwdata->negacyclic_gwdata, s1R, s1Q);		
			}
		} else {									
			
			if (FFT_state (s1R) != FULLY_FFTed) {					
				if (s1 != d && !(options & GWMUL_PRESERVE_S1) && (FFT_state (s2R) != FULLY_FFTed || s2 == d || s3 == d))
					gwfft (gwdata, s1, s1);					
				else if (s2 != d && s3 != d)					
					s1Q = s1R, T_Q_options |= GWMUL_PRESERVE_S1;		
				else
					gwfft (gwdata->negacyclic_gwdata, s1R, s1Q);		
			}

			if (FFT_state (s2R) != FULLY_FFTed) {					
				if (s2 != d && !(options & GWMUL_PRESERVE_S2) && (FFT_state (s1R) != FULLY_FFTed || s1 == d || s3 == d))
					gwfft (gwdata, s2, s2);					
				else if (s1 != d && s3 != d && s1Q != s1R)			
					s2Q = s2R, T_Q_options |= GWMUL_PRESERVE_S2;		
				else
					gwfft (gwdata->negacyclic_gwdata, s2R, s2Q);		
			}
		}

		cmn_gwmulop4 (gwdata->negacyclic_gwdata, s1Q, s2Q, s3Q, T_Q, T_Q_options & ~GWMUL_STARTNEXTFFT, opcode);

		cmn_gwmulop4 (gwdata->cyclic_gwdata, s1R, s2R, s3R, T_R, options | GWMUL_STARTNEXTFFT, opcode);

		MMGW_redc (gwdata, T_R, T_Q);
	}

#ifdef X86_64
	else if (!(gwdata->cpu_flags & (CPU_AVX512F | CPU_FMA3 | CPU_AVX)) || gwdata->careful_count > 0) {
#else
	else if (1) {
#endif

		if (FFT_state (s3) != NOT_FFTed || s3 == d) {
			tmp3 = gwalloc (gwdata);
			gwunfft (gwdata, s3, tmp3);
			s3 = tmp3;
		}

		if ((options & GWMUL_MULBYCONST) && (options & GWMUL_ADDINCONST) && gwdata->asm_postaddin_value != 0.0) {
			
			double saved_postaddin_value = gwdata->asm_postaddin_value;
			gwdata->asm_postaddin_value = 0.0;
			gwmul3 (gwdata, s1, s2, d, options & ~(GWMUL_MULBYCONST | GWMUL_STARTNEXTFFT));
			gwdata->asm_postaddin_value = saved_postaddin_value;
			
			if (opcode == 3) gwadd3o (gwdata, d, s3, d, GWADD_GUARANTEED_OK | GWADD_DELAY_NORMALIZE);
			else gwsub3o (gwdata, d, s3, d, GWADD_GUARANTEED_OK | GWADD_DELAY_NORMALIZE);
			
			tmp1 = gwalloc (gwdata);
			dbltogw (gwdata, gwdata->mulbyconst, tmp1);
			double saved_addin_value = gwdata->asm_addin_value;
			gwdata->asm_addin_value = 0.0;
			gwmul3 (gwdata, d, tmp1, d, options & ~GWMUL_MULBYCONST);
			gwdata->asm_addin_value = saved_addin_value;
		} else if (options & GWMUL_MULBYCONST) {
			gwmul3 (gwdata, s1, s2, d, options & ~(GWMUL_MULBYCONST | GWMUL_STARTNEXTFFT));
			if (opcode == 3) gwadd3o (gwdata, d, s3, d, GWADD_GUARANTEED_OK | GWADD_DELAY_NORMALIZE);
			else gwsub3o (gwdata, d, s3, d, GWADD_GUARANTEED_OK | GWADD_DELAY_NORMALIZE);
			gwsmallmul (gwdata, gwdata->mulbyconst, d);
		} else {
			gwmul3 (gwdata, s1, s2, d, options & ~GWMUL_STARTNEXTFFT);
			if (opcode == 3) gwadd3o (gwdata, d, s3, d, GWADD_FORCE_NORMALIZE);
			else gwsub3o (gwdata, d, s3, d, GWADD_FORCE_NORMALIZE);
		}
	}

	else {
		struct gwasm_data *asm_data = (struct gwasm_data *) gwdata->asm_data;

		asm_data->DEST2ARG = 0;

		if (FFT_state (s3) != FULLY_FFTed && FFT_state (s3) != FFTed_FOR_FMA) {
			if (options & GWMUL_PRESERVE_S3) { tmp3 = gwalloc (gwdata); gwfft (gwdata, s3, tmp3); s3 = tmp3; }
			else gwfft (gwdata, s3, s3);
		}

		if (s1 == s2 && !(gwdata->cpu_flags & CPU_AVX512F) && FFT_state (s1) != FULLY_FFTed) {
			if (options & GWMUL_PRESERVE_S1) { tmp1 = gwalloc (gwdata); gwfft (gwdata, s1, tmp1); s1 = tmp1; }
			else gwfft (gwdata, s1, s1);
		}

		if (s1 == s2) {
			if (FFT_state (s1) == FULLY_FFTed) asm_data->ffttype = 4;
			else if (!(options & GWMUL_FFT_S12) && s3 != d) asm_data->ffttype = 2;
			else if (!(gwdata->cpu_flags & (CPU_AVX512F))) gwfft (gwdata, s1, s1), asm_data->ffttype = 4;
			else {	
				asm_data->ffttype = 2;
				asm_data->DEST2ARG = d;
			}
		}

		else {
			gwnum	preferred_FFT_arg;	

			bool s1_can_be_FFTed_and_discarded = (s2 != d && s3 != d);
			bool s2_can_be_FFTed_and_discarded = (s1 != d && s3 != d);

			if (FFT_state (s1) == FULLY_FFTed && FFT_state (s2) == FULLY_FFTed) preferred_FFT_arg = s1;
			
			else if (FFT_state (s1) != FULLY_FFTed && (options & GWMUL_PRESERVE_S1) && s1_can_be_FFTed_and_discarded) preferred_FFT_arg = s1;
			else if (FFT_state (s2) != FULLY_FFTed && (options & GWMUL_PRESERVE_S2) && s2_can_be_FFTed_and_discarded) preferred_FFT_arg = s2;
			
			else if (FFT_state (s1) != FULLY_FFTed && !(options & GWMUL_FFT_S1) && s1_can_be_FFTed_and_discarded) preferred_FFT_arg = s1;
			else if (FFT_state (s2) != FULLY_FFTed && !(options & GWMUL_FFT_S2) && s2_can_be_FFTed_and_discarded) preferred_FFT_arg = s2;
			
			else if (FFT_state (s1) != FULLY_FFTed && !(options & GWMUL_PRESERVE_S1)) preferred_FFT_arg = s1, asm_data->DEST2ARG = d;
			else if (FFT_state (s2) != FULLY_FFTed && !(options & GWMUL_PRESERVE_S2)) preferred_FFT_arg = s2, asm_data->DEST2ARG = d;
			
			else if (FFT_state (s1) == FULLY_FFTed) preferred_FFT_arg = s1;
			else if (FFT_state (s2) == FULLY_FFTed) preferred_FFT_arg = s2;
			else ASSERTG (FALSE);

			if (preferred_FFT_arg != s1 && FFT_state (s1) != FULLY_FFTed) {
				if (options & GWMUL_PRESERVE_S1) { tmp1 = gwalloc (gwdata); gwfft (gwdata, s1, tmp1); s1 = tmp1; }
				else gwfft (gwdata, s1, s1);
			}
			if (preferred_FFT_arg != s2 && FFT_state (s2) != FULLY_FFTed) {
				if (options & GWMUL_PRESERVE_S2) { tmp2 = gwalloc (gwdata); gwfft (gwdata, s2, tmp2); s2 = tmp2; }
				else gwfft (gwdata, s2, s2);
			}

			if (FFT_state (preferred_FFT_arg) == FULLY_FFTed) asm_data->ffttype = 4;
			else {
				asm_data->ffttype = 3;
				if (preferred_FFT_arg == s2) {
					s2 = s1;
					s1 = preferred_FFT_arg;
				}
			}
		}

		if (FFT_state (s1) == NOT_FFTed) gwdata->read_count += 1, gwdata->write_count += 1;

		if (options & GWMUL_MULBYCONST) asm_data->NORMRTN = gwdata->GWPROCPTRS[norm_routines + 2 + gwdata->ERROR_CHECKING];
		else asm_data->NORMRTN = gwdata->GWPROCPTRS[norm_routines + gwdata->ERROR_CHECKING];
		if (gwdata->FFT1_state == 2) asm_data->SRC3ARG = (void *) (intptr_t) 1;
		else asm_data->SRC3ARG = (void *) (intptr_t) 2;
		if (asm_data->DEST2ARG) {
			asm_data->mul4_opcode = 0x80 | opcode;
			asm_data->SRC2ARG = (void *) ((intptr_t) s3 - (intptr_t) s1);
			asm_data->DEST2ARG = (void *) ((intptr_t) asm_data->DEST2ARG - (intptr_t) s1);
			asm_mul (gwdata, s1, s2, s1, options);
		} else {
			asm_data->mul4_opcode = opcode;
			asm_data->SRC2ARG = (void *) ((intptr_t) s3 - (intptr_t) d);
			asm_mul (gwdata, s1, s2, d, options);
		}

		gwdata->read_count += 4, gwdata->write_count += 2;
		if (s1 == s2 || s1 == s3) gwdata->read_count -= 1;
		if (s2 == s3) gwdata->read_count -= 1;
		if ((intptr_t) asm_data->SRC3ARG != 1) gwdata->read_count += 1;
		if (asm_data->DEST2ARG) gwdata->write_count += 1;

		if (gwdata->GENERAL_MOD) emulate_mod (gwdata, d);
	}

	if (tmp1 != NULL) gwfree (gwdata, tmp1);
	if (tmp2 != NULL) gwfree (gwdata, tmp2);
	if (tmp3 != NULL) gwfree (gwdata, tmp3);
}

void gwfft (			
	gwhandle *gwdata,	
	gwnum	s,		
	gwnum	d)		
{
	struct gwasm_data *asm_data;

	ASSERTG (unnorms (s) >= 0.0f);

	if (FFT_state (s) == FULLY_FFTed) {
		if (s != d) gwcopy (gwdata, s, d);
		return;
	}

	if (FFT_state (s) == FFTed_FOR_FMA) {
		gwunfft (gwdata, s, d);
		s = d;
	}

	if (gwdata->GENERAL_MMGW_MOD) {
		gwunfft (gwdata, s, s);			
		gwfft (gwdata->negacyclic_gwdata, cyclic_gwnum (gwdata, s), negacyclic_gwnum (gwdata, d));
		gwfft (gwdata->cyclic_gwdata, cyclic_gwnum (gwdata, s), cyclic_gwnum (gwdata, d));
		return;
	}

	asm_data = (struct gwasm_data *) gwdata->asm_data;
	if (asm_data->ZERO_PADDED_FFT && FFT_state (s) == PARTIALLY_FFTed && s != d) {
		d[-5] = s[-5];
		d[-6] = s[-6];
		d[-7] = s[-7];
		d[-8] = s[-8];
		d[-9] = s[-9];
		d[-10] = s[-10];
		d[-11] = s[-11];
	}

	asm_data->DESTARG = d;
	asm_data->DIST_TO_FFTSRCARG = (intptr_t) s - (intptr_t) d;
	asm_data->DIST_TO_MULSRCARG = 0;
	asm_data->ffttype = 1;
	gw_fft (gwdata, asm_data);

	gwdata->fft_count += 1;
	if (FFT_state (s) == PARTIALLY_FFTed) {
		gwdata->read_count += 1;
		gwdata->write_count += 1;
	} else {
		gwdata->read_count += 2;
		gwdata->write_count += 2;
	}

	unnorms (d) = unnorms (s);
	FFT_state (d) = FULLY_FFTed;
}

void gwfft_for_fma (		
	gwhandle *gwdata,	
	gwnum	s,		
	gwnum	d)		
{
	int	i, j, doubles_per_vector, pass2_size;

	ASSERTG (unnorms (s) >= 0.0f);

	if (FFT_state (s) == FFTed_FOR_FMA) {
		if (s != d) gwcopy (gwdata, s, d);
		return;
	}

	if (gwdata->GENERAL_MMGW_MOD) {
		gwunfft (gwdata, s, s);			
		gwfft_for_fma (gwdata->negacyclic_gwdata, cyclic_gwnum (gwdata, s), negacyclic_gwnum (gwdata, d));
		gwfft_for_fma (gwdata->cyclic_gwdata, cyclic_gwnum (gwdata, s), cyclic_gwnum (gwdata, d));
		return;
	}

#ifdef X86_64
	if (!(gwdata->cpu_flags & (CPU_AVX512F | CPU_FMA3 | CPU_AVX))) {
#else
	if (1) {
#endif
		gwunfft (gwdata, s, d);
		return;
	}

	gwfft (gwdata, s, d);

	init_FFT1 (gwdata);
	if (gwdata->FFT1_state == 2) return;

	for (int which = 0; which == 0 || (gwdata->GENERAL_MMGW_MOD && which == 1); which++) {
		gwhandle *fft_gwdata;
		gwnum fft_d, GW_FFT1;

		if (!gwdata->GENERAL_MMGW_MOD) {
			fft_gwdata = gwdata;
			fft_d = d;
			GW_FFT1 = gwdata->GW_FFT1;
		} else if (which == 0) {
			fft_gwdata = gwdata->cyclic_gwdata;
			fft_d = cyclic_gwnum (gwdata, d);
			GW_FFT1 = cyclic_gwnum (gwdata, gwdata->GW_FFT1);
		} else {
			fft_gwdata = gwdata->negacyclic_gwdata;
			fft_d = negacyclic_gwnum (gwdata, d);
			GW_FFT1 = negacyclic_gwnum (gwdata, gwdata->GW_FFT1);
		}

		if (fft_gwdata->cpu_flags & CPU_AVX512F) {
			doubles_per_vector = 8;
			pass2_size = fft_gwdata->PASS2_SIZE ? fft_gwdata->PASS2_SIZE * 2 : fft_gwdata->FFTLEN;
			for (i = 0; i < (int) fft_gwdata->FFTLEN; i += doubles_per_vector * 2) {
			    for (j = 0; j < doubles_per_vector; j++) {
				int	k = i + j + (i / pass2_size) * (fft_gwdata->PASS2GAPSIZE / sizeof (double)) + (i / 512) * (fft_gwdata->FOURKBGAPSIZE / sizeof (double));
				double	real1 = fft_d[k];
				double	imag1 = fft_d[k + doubles_per_vector];
				double	real2 = GW_FFT1[k];
				double	imag2 = GW_FFT1[k + doubles_per_vector];
				if (k == 0 && !fft_gwdata->NEGACYCLIC_FFT) {
					fft_d[0] = real1 * real2;
					fft_d[doubles_per_vector] = imag1 * imag2;
				} else {
					fft_d[k] = real1 * real2 - imag1 * imag2;
					fft_d[k + doubles_per_vector] = real1 * imag2 + imag1 * real2;
				}
			    }
			}
		} else {
			doubles_per_vector = 4;
			for (i = 0; i < (int) (gwnum_datasize (fft_gwdata) / sizeof (double)); i += doubles_per_vector * 2) {
			    for (j = 0; j < doubles_per_vector; j++) {
				double	real1 = fft_d[i + j];
				double	imag1 = fft_d[i + j + doubles_per_vector];
				double	real2 = GW_FFT1[i + j];
				double	imag2 = GW_FFT1[i + j + doubles_per_vector];
				if (i == 0 && j == 0 && !fft_gwdata->NEGACYCLIC_FFT) {
					fft_d[0] = real1 * real2;
					fft_d[doubles_per_vector] = imag1 * imag2;
				} else {
					fft_d[i + j] = real1 * real2 - imag1 * imag2;
					fft_d[i + j + doubles_per_vector] = real1 * imag2 + imag1 * real2;
				}
			    }
			}
		}

		if (fft_gwdata->ZERO_PADDED_FFT) {
			double	wm3, wm2, wm1, w0, w1, w2, w3;
			double	xm3, xm2, xm1, x0, x1, x2, x3;
			wm3 = d[-11];			
			wm2 = d[-10];			
			wm1 = d[-9];			
			xm3 = gwdata->GW_FFT1[-11];	
			xm2 = gwdata->GW_FFT1[-10];	
			xm1 = gwdata->GW_FFT1[-9];	
			w0 = d[-8];			
			w1 = d[-7];			
			w2 = d[-6];			
			w3 = d[-5];			
			x0 = gwdata->GW_FFT1[-8];	
			x1 = gwdata->GW_FFT1[-7];	
			x2 = gwdata->GW_FFT1[-6];	
			x3 = gwdata->GW_FFT1[-5];	
			d[-11] = wm3 * x3 + wm2 * x2 + wm1 * x1 +		
				 w0 * x0 + w1 * xm1 + w2 * xm2 + w3 * xm3;	
			d[-10] = wm2 * x3 + wm1 * x2 + w0 * x1 +		
				 w1 * x0 + w2 * xm1 + w3 * xm2;			
			d[-9] = wm1 * x3 + w0 * x2 + w1 * x1 +			
				w2 * x0 + w3 * xm1;				
			d[-8] = w0 * x3 + w1 * x2 + w2 * x1 + w3 * x0;		
			d[-7] = w1 * x3 + w2 * x2 + w3 * x1;			
			d[-6] = w2 * x3 + w3 * x2;				
			d[-5] = w3 * x3;					
		}

		FFT_state (fft_d) = FFTed_FOR_FMA;
	}
	gwdata->read_count += 1;
	gwdata->write_count += 1;
}

void gwmul3 (			
	gwhandle *gwdata,	
	gwnum	s1,		
	gwnum	s2,		
	gwnum	d,		
	int	options)	
{
	struct gwasm_data *asm_data;

	ASSERTG (unnorms (s1) >= 0.0f);
	ASSERTG (unnorms (s2) >= 0.0f);
	ASSERTG (FFT_state (s1) != FFTed_FOR_FMA && FFT_state (s2) != FFTed_FOR_FMA);
	
	ASSERTG ((s1 == s2 && square_safe (gwdata, unnorms (s1))) ||
		 (s1 != s2 && mul_safe (gwdata, unnorms (s1), unnorms (s2))));

	if (s1 == d) options &= ~(GWMUL_FFT_S1 | GWMUL_PRESERVE_S1);
	if (s2 == d) options &= ~(GWMUL_FFT_S2 | GWMUL_PRESERVE_S2);

	if (gwdata->careful_count > 0) {
		gwdata->careful_count--;
		gwmul3_carefully (gwdata, s1, s2, d, options);
		return;
	}

	if ((options & GWMUL_ADDINCONST) && (gwdata->emulate_addin_value != 0 || gwdata->emulate_postaddin_value != 0)) {
		gwmul3 (gwdata, s1, s2, d, options & ~(GWMUL_MULBYCONST | GWMUL_ADDINCONST | GWMUL_STARTNEXTFFT));
		init_GW_ADDIN (gwdata);
		if (gwdata->GW_ADDIN != NULL) gwadd3o (gwdata, d, gwdata->GW_ADDIN, d, GWADD_FORCE_NORMALIZE);	
		if (options & GWMUL_MULBYCONST) gwsmallmul (gwdata, gwdata->mulbyconst, d);
		if (gwdata->GW_POSTADDIN != NULL) gwadd3o (gwdata, d, gwdata->GW_POSTADDIN, d, GWADD_FORCE_NORMALIZE);
		return;
	}

	if (gwdata->GENERAL_MMGW_MOD) {
		bool mulmulsub5able = (!(options & GWMUL_MULBYCONST) && !(options & GWMUL_ADDINCONST));
		int T_Q_options = options;

		gwnum s1R = cyclic_gwnum (gwdata, s1);
		gwnum s1Q = negacyclic_gwnum (gwdata, s1);
		gwnum s2R = cyclic_gwnum (gwdata, s2);
		gwnum s2Q = negacyclic_gwnum (gwdata, s2);
		gwnum T_R = cyclic_gwnum (gwdata, d);
		gwnum T_Q = negacyclic_gwnum (gwdata, d);

		if (options & GWMUL_FFT_S1) gwfft (gwdata, s1, s1);
		if (options & GWMUL_FFT_S2) gwfft (gwdata, s2, s2);

		if (s1 == s2) {									
			mulmulsub5able = mulmulsub5able && mulsquareadd_safe (gwdata->negacyclic_gwdata, 0, 0, unnorms (s1), unnorms (s1));

			if (FFT_state (s1R) != FULLY_FFTed) {					
				if (!mulmulsub5able) {
					s1Q = s2Q = s1R;					
					T_Q_options |= GWMUL_PRESERVE_S1 | GWMUL_PRESERVE_S2;
				} else
					gwfft (gwdata->negacyclic_gwdata, s1R, s1Q);		
			}
		} else {									
			mulmulsub5able = mulmulsub5able && mulmuladd_safe (gwdata->negacyclic_gwdata, 0, 0, unnorms (s1), unnorms (s2));

			if (FFT_state (s1R) != FULLY_FFTed) {					
				if (s1 != d && !(options & GWMUL_PRESERVE_S1) && (FFT_state (s2R) != FULLY_FFTed || s2 == d))
					gwfft (gwdata, s1, s1);					
				else if (!mulmulsub5able && s2 != d)				
					s1Q = s1R, T_Q_options |= GWMUL_PRESERVE_S1;		
				else
					gwfft (gwdata->negacyclic_gwdata, s1R, s1Q);		
			}

			if (FFT_state (s2R) != FULLY_FFTed) {					
				if (s2 != d && !(options & GWMUL_PRESERVE_S2) && (FFT_state (s1R) != FULLY_FFTed || s1 == d))
					gwfft (gwdata, s2, s2);					
				else if (!mulmulsub5able && s1 != d && s1Q != s1R)		
					s2Q = s2R, T_Q_options |= GWMUL_PRESERVE_S2;		
				else
					gwfft (gwdata->negacyclic_gwdata, s2R, s2Q);		
			}
		}

		if (!mulmulsub5able) gwmul3 (gwdata->negacyclic_gwdata, s1Q, s2Q, T_Q, T_Q_options & ~GWMUL_STARTNEXTFFT);

		gwmul3 (gwdata->cyclic_gwdata, s1R, s2R, T_R, options | GWMUL_STARTNEXTFFT);

		if (!mulmulsub5able) MMGW_redc (gwdata, T_R, T_Q);
		else MMGW_redc5 (gwdata, T_R, s1Q, s2Q);

		return;
	}

	asm_data = (struct gwasm_data *) gwdata->asm_data;
	if (options & GWMUL_MULBYCONST) asm_data->NORMRTN = gwdata->GWPROCPTRS[norm_routines + 2 + gwdata->ERROR_CHECKING];
	else asm_data->NORMRTN = gwdata->GWPROCPTRS[norm_routines + gwdata->ERROR_CHECKING];
	raw_gwmul3 (gwdata, s1, s2, d, options);

	if (gwdata->GENERAL_MOD) emulate_mod (gwdata, d);
}

void gwmul3_carefully (		
	gwhandle *gwdata,	
	gwnum	s1,		
	gwnum	s2,		
	gwnum	d,		
	int	options)	
{
	struct gwasm_data *asm_data = (struct gwasm_data *) gwdata->asm_data;
	int	saved_careful_count;

	ASSERTG (unnorms (s1) >= 0.0f);
	ASSERTG (unnorms (s2) >= 0.0f);
	ASSERTG (FFT_state (s1) != FFTed_FOR_FMA && FFT_state (s2) != FFTed_FOR_FMA);
	
	ASSERTG ((s1 == s2 && square_safe (gwdata, unnorms (s1))) ||
		 (s1 != s2 && mul_safe (gwdata, unnorms (s1), unnorms (s2))));

	if (s1 == d) options &= ~(GWMUL_FFT_S1 | GWMUL_PRESERVE_S1);
	if (s2 == d) options &= ~(GWMUL_FFT_S2 | GWMUL_PRESERVE_S2);

	options &= ~GWMUL_FFT_S12;

	saved_careful_count = gwdata->careful_count;
	gwdata->careful_count = 0;

	if (gwdata->clone_of != NULL && gwdata->GW_RANDOM != gwdata->clone_of->GW_RANDOM && gwdata->clone_of->GW_RANDOM != NULL) {
		gwfree (gwdata, gwdata->GW_RANDOM); gwdata->GW_RANDOM = gwdata->clone_of->GW_RANDOM;
		gwfree (gwdata, gwdata->GW_RANDOM_SQUARED); gwdata->GW_RANDOM_SQUARED = gwdata->clone_of->GW_RANDOM_SQUARED;
		gwfree (gwdata, gwdata->GW_RANDOM_FFT); gwdata->GW_RANDOM_FFT = gwdata->clone_of->GW_RANDOM_FFT;
	}
	
	else if (gwdata->GW_RANDOM == NULL) {
		gwnum random = gwalloc_internal (gwdata);
		gw_fixed_random_number (gwdata, random);
		gwdata->GW_RANDOM = random;				
		ASSERTG (gwdata->GW_RANDOM_SQUARED == NULL && gwdata->GW_RANDOM_FFT == NULL);
	}

	if (s1 == s2) {
		gwnum tmp = gwalloc (gwdata);
		if (FFT_state (s1) == NOT_FFTed);							
		else if (options & (GWMUL_PRESERVE_S1 | GWMUL_PRESERVE_S2)) gwunfft (gwdata, s1, tmp), s1 = tmp;
		else gwunfft (gwdata, s1, s1);

		gwaddsub4o (gwdata, s1, gwdata->GW_RANDOM, tmp, d, GWADD_FORCE_NORMALIZE);		

		if (gwdata->GW_RANDOM_SQUARED == NULL) {
			
			if (gwdata->clone_of != NULL && gwdata->GW_RANDOM == gwdata->clone_of->GW_RANDOM && gwdata->clone_of->GW_RANDOM_SQUARED != NULL) {
				gwdata->GW_RANDOM_SQUARED = gwdata->clone_of->GW_RANDOM_SQUARED;
			}
			
			else {
				gwnum random_squared = gwalloc_internal (gwdata);
				gwmul3 (gwdata, gwdata->GW_RANDOM, gwdata->GW_RANDOM, random_squared, GWMUL_PRESERVE_S1 | GWMUL_PRESERVE_S2 | GWMUL_STARTNEXTFFT);
				gwfft_for_fma (gwdata, random_squared, random_squared);
				gwdata->GW_RANDOM_SQUARED = random_squared;	
			}
		}

		gwmuladd4 (gwdata, tmp, d, gwdata->GW_RANDOM_SQUARED, d, options & (GWMUL_ADDINCONST | GWMUL_MULBYCONST | GWMUL_STARTNEXTFFT));

		gwfree (gwdata, tmp);
	}

	else {
		gwnum	tmp1, tmp2, tmp3;

		tmp1 = gwalloc (gwdata);
		if (FFT_state (s1) == NOT_FFTed);					
		else if (options & GWMUL_PRESERVE_S1) gwunfft (gwdata, s1, tmp1), s1 = tmp1;
		else gwunfft (gwdata, s1, s1);

		gwadd3o (gwdata, s1, gwdata->GW_RANDOM, tmp1, GWADD_FORCE_NORMALIZE);	

		tmp2 = NULL;
		if (FFT_state (s2) == NOT_FFTed);					
		else if (options & GWMUL_PRESERVE_S2) tmp2 = gwalloc (gwdata), gwunfft (gwdata, s2, tmp2), s2 = tmp2;
		else gwunfft (gwdata, s2, s2);

		tmp3 = gwalloc (gwdata);
		gwsub3o (gwdata, s2, gwdata->GW_RANDOM, tmp3, GWADD_FORCE_NORMALIZE);	
		gwsub3o (gwdata, tmp1, s2, d, GWADD_FORCE_NORMALIZE);			
		gwfree (gwdata, tmp2);

		if (gwdata->GW_RANDOM_FFT == NULL) {
			
			if (gwdata->clone_of != NULL && gwdata->GW_RANDOM == gwdata->clone_of->GW_RANDOM && gwdata->clone_of->GW_RANDOM_FFT != NULL) {
				gwdata->GW_RANDOM_FFT = gwdata->clone_of->GW_RANDOM_FFT;
			}
			
			else {
				gwnum random_fft = gwalloc_internal (gwdata);
				gwfft (gwdata, gwdata->GW_RANDOM, random_fft);
				gwdata->GW_RANDOM_FFT = random_fft;		
			}
		}
		
		if (!gwdata->paranoid_mul_careful || mulsquareadd_safe (gwdata, 0, 0, 0, 0)) {
			gwmulmuladd5 (gwdata, tmp1, tmp3, gwdata->GW_RANDOM_FFT, d, d, options & (GWMUL_ADDINCONST | GWMUL_MULBYCONST | GWMUL_STARTNEXTFFT));
		} else {
			
			gwmul3 (gwdata, gwdata->GW_RANDOM_FFT, d, d, GWMUL_STARTNEXTFFT);
			gwmuladd4 (gwdata, tmp1, tmp3, d, d, options & (GWMUL_ADDINCONST | GWMUL_MULBYCONST | GWMUL_STARTNEXTFFT));
		}

		gwfree (gwdata, tmp1);
		gwfree (gwdata, tmp3);
	}

	gwdata->careful_count = saved_careful_count;
}

void gwaddmul4 (		
	gwhandle *gwdata,	
	gwnum	s1,		
	gwnum	s2,		
	gwnum	s3,		
	gwnum	d,		
	int	options)	
{
	ASSERTG (unnorms (s1) >= 0.0f);
	ASSERTG (unnorms (s2) >= 0.0f);
	ASSERTG (unnorms (s3) >= 0.0f);
	ASSERTG (addmul_safe (gwdata, unnorms (s1), unnorms (s2), unnorms (s3)));
	ASSERTG (FFT_state (s1) != FFTed_FOR_FMA && FFT_state (s2) != FFTed_FOR_FMA && FFT_state (s3) != FFTed_FOR_FMA);

	cmn_gwopmul4 (gwdata, s1, s2, s3, d, options, 1);	
}

void gwsubmul4 (		
	gwhandle *gwdata,	
	gwnum	s1,		
	gwnum	s2,		
	gwnum	s3,		
	gwnum	d,		
	int	options)	
{
	ASSERTG (unnorms (s1) >= 0.0f);
	ASSERTG (unnorms (s2) >= 0.0f);
	ASSERTG (unnorms (s3) >= 0.0f);
	ASSERTG (addmul_safe (gwdata, unnorms (s1), unnorms (s2), unnorms (s3)));
	ASSERTG (FFT_state (s1) != FFTed_FOR_FMA && FFT_state (s2) != FFTed_FOR_FMA && FFT_state (s3) != FFTed_FOR_FMA);

	cmn_gwopmul4 (gwdata, s1, s2, s3, d, options, 2);	
}

void gwmuladd4 (		
	gwhandle *gwdata,	
	gwnum	s1,		
	gwnum	s2,		
	gwnum	s3,		
	gwnum	d,		
	int	options)	
{
	ASSERTG (unnorms (s1) >= 0.0f);
	ASSERTG (unnorms (s2) >= 0.0f);
	ASSERTG (unnorms (s3) >= 0.0f);
	ASSERTG (FFT_state (s1) != FFTed_FOR_FMA && FFT_state (s2) != FFTed_FOR_FMA);
	
	ASSERTG ((s1 == s2 && squareadd_safe (gwdata, unnorms (s1), unnorms (s2), unnorms (s3))) ||
		 (s1 != s2 && muladd_safe (gwdata, unnorms (s1), unnorms (s2), unnorms (s3))));

	cmn_gwmulop4 (gwdata, s1, s2, s3, d, options, 3);	
}

void gwmulsub4 (		
	gwhandle *gwdata,	
	gwnum	s1,		
	gwnum	s2,		
	gwnum	s3,		
	gwnum	d,		
	int	options)	
{
	ASSERTG (unnorms (s1) >= 0.0f);
	ASSERTG (unnorms (s2) >= 0.0f);
	ASSERTG (unnorms (s3) >= 0.0f);
	ASSERTG (FFT_state (s1) != FFTed_FOR_FMA && FFT_state (s2) != FFTed_FOR_FMA);
	
	ASSERTG ((s1 == s2 && squareadd_safe (gwdata, unnorms (s1), unnorms (s2), unnorms (s3))) ||
		 (s1 != s2 && muladd_safe (gwdata, unnorms (s1), unnorms (s2), unnorms (s3))));

	cmn_gwmulop4 (gwdata, s1, s2, s3, d, options, 4);	
}

void gwmulmuladd5 (		
	gwhandle *gwdata,	
	gwnum	s1,		
	gwnum	s2,		
	gwnum	s3,		
	gwnum	s4,		
	gwnum	d,		
	int	options)	
{
	ASSERTG (unnorms (s1) >= 0.0f);
	ASSERTG (unnorms (s2) >= 0.0f);
	ASSERTG (unnorms (s3) >= 0.0f);
	ASSERTG (unnorms (s4) >= 0.0f);
	ASSERTG (s1 != s2 || s1 != s3 || s1 != s4);  
	ASSERTG (FFT_state (s1) != FFTed_FOR_FMA && FFT_state (s2) != FFTed_FOR_FMA);
	ASSERTG (FFT_state (s3) != FFTed_FOR_FMA && FFT_state (s4) != FFTed_FOR_FMA);
	ASSERTG (mulmuladd_safe (gwdata, unnorms (s1), unnorms (s2), unnorms (s3), unnorms (s4)));

	cmn_gwmulmulop5 (gwdata, s1, s2, s3, s4, d, options, 3);	
}

void gwmulmulsub5 (		
	gwhandle *gwdata,	
	gwnum	s1,		
	gwnum	s2,		
	gwnum	s3,		
	gwnum	s4,		
	gwnum	d,		
	int	options)	
{
	ASSERTG (unnorms (s1) >= 0.0f);
	ASSERTG (unnorms (s2) >= 0.0f);
	ASSERTG (unnorms (s3) >= 0.0f);
	ASSERTG (unnorms (s4) >= 0.0f);
	ASSERTG (mulmuladd_safe (gwdata, unnorms (s1), unnorms (s2), unnorms (s3), unnorms (s4)));
	ASSERTG (FFT_state (s1) != FFTed_FOR_FMA && FFT_state (s2) != FFTed_FOR_FMA);
	ASSERTG (FFT_state (s3) != FFTed_FOR_FMA && FFT_state (s4) != FFTed_FOR_FMA);

	cmn_gwmulmulop5 (gwdata, s1, s2, s3, s4, d, options, 4);	
}

void gw_random_number_internal (
	gwhandle *gwdata,	
	gwnum	x,
	struct mt_state *rand_info)
{
	stackgiant(g,4);

	g->sign = 4;
	g->n[0] = (uint32_t) genrand_int32 (rand_info) | 1;
	g->n[1] = (uint32_t) genrand_int32 (rand_info);
	g->n[2] = (uint32_t) genrand_int32 (rand_info);
	g->n[3] = (uint32_t) genrand_int32 (rand_info) | 0x80000000;
	if (gwdata->GW_MODULUS != NULL) {
		modg (gwdata->GW_MODULUS, g);
		if (isZero (g) || isone (g)) itog (3, g);
	}
	gianttogw (gwdata, g, x);
	
	for (unsigned int bitlen = 128 >> 4; bitlen < (unsigned int) gwdata->bit_length; bitlen <<= 1) {
		char	saved_error_checking = gwdata->ERROR_CHECKING;
		int	saved_careful_count = gwdata->careful_count;
		gwerror_checking (gwdata, 0);			
		gwdata->careful_count = 0;			
		gwsquare2 (gwdata, x, x, 0);
		gwerror_checking (gwdata, saved_error_checking);
		gwdata->careful_count = saved_careful_count;
	}
}

void gw_random_number (
	gwhandle *gwdata,	
	gwnum	x)
{
static	int	genrand_initialized = FALSE;
static	struct mt_state rand_info;

	if (!genrand_initialized) {
		init_genrand (&rand_info, (unsigned long) time (NULL));
		genrand_initialized = TRUE;
	}

	gw_random_number_internal (gwdata, x, &rand_info);
}

void gw_fixed_random_number (
	gwhandle *gwdata,	
	gwnum	x)
{
	struct mt_state rand_info;

	init_genrand (&rand_info, 5489);

	gw_random_number_internal (gwdata, x, &rand_info);
}

void gwset_carefully_count (
	gwhandle *gwdata,	
	int	n)		
				
{

	if (n == -1 && gwdata->FFTLEN) n = (int) ceil (log2 (gwdata->bit_length)) + 12;

	gwdata->careful_count = n;
}

static __inline void *biglit_data_ptr (
	gwhandle *gwdata,
	int	i)
{
	char	*p;

	ASSERTG (gwdata->cpu_flags & CPU_AVX512F && gwdata->PASS2_SIZE);

	p = (char *) gwdata->pass1_var_data + gwdata->biglit_data_offset;

	if (gwdata->ZERO_PADDED_FFT) return (p + (i * gwdata->PASS1_CACHE_LINES * 2 / 8 / 2));
	else return (p + (i * gwdata->PASS1_CACHE_LINES * 2 / 8));
}

struct multithread_op_data {
	gwnum	s1;			
	gwnum	s2;			
	gwnum	d1;			
	gwnum	d2;			
	void	(*asm_proc)(void*);	
	int	is_quick;		
	int	num_blks;		
	void	*d1_carries;		
	void	*d2_carries;		
};

void multithread_op (
	gwhandle *gwdata,		
	gwnum	s1,			
	gwnum	s2,			
	gwnum	d1,			
	gwnum	d2,			
	void	(*asm_proc)(void*),	
	int	is_quick)		
{
	struct gwasm_data *asm_data = (struct gwasm_data *) gwdata->asm_data;
	struct multithread_op_data data;

	data.s1 = s1;
	data.s2 = s2;
	data.d1 = d1;
	data.d2 = d2;
	data.asm_proc = asm_proc;
	data.is_quick = is_quick;

	if (asm_proc == NULL) {
		int	size, movesize;

		size = 7 * sizeof (uint32_t) + gwnum_datasize (gwdata);
		data.s1 = (gwnum) ((char *) &(((uint32_t *) data.s1)[-7]));
		data.d1 = (gwnum) ((char *) &(((uint32_t *) data.d1)[-7]));

		if ((intptr_t) data.s1 & 127) {
			movesize = 128 - (int) ((intptr_t) data.s1 & 127);
			memcpy (data.d1, data.s1, movesize);
			size -= movesize;
			data.s1 = (gwnum) ((char *) data.s1 + movesize);
			data.d1 = (gwnum) ((char *) data.d1 + movesize);
		}

		data.num_blks = size >> 12;

		if (size & 4095) {
			memcpy ((char *) data.d1 + data.num_blks * 4096, (char *) data.s1 + data.num_blks * 4096, size & 4095);
		}
	}

	else if (gwdata->cpu_flags & CPU_AVX512F) {
		data.num_blks = asm_data->addcount1;
		data.d1_carries = asm_data->carries;
		
		if (d2 != NULL && !is_quick) data.d2_carries = aligned_malloc (data.num_blks * 16 * sizeof (double), 128);
	}

	else {
		data.num_blks = asm_data->addcount1;
		data.d1_carries = asm_data->carries;
		
		if (d2 != NULL && !is_quick) data.d2_carries = aligned_malloc (data.num_blks * 8 * sizeof (double), 128);
	}

	gwdata->pass1_state = PASS1_STATE_MULTITHREAD_OP;
	gwdata->multithread_op_data = &data;
	atomic_set (gwdata->next_block, 0);
	if (gwdata->num_threads > 1) signal_auxiliary_threads (gwdata);

	do_multithread_op_work (gwdata, asm_data);

	if (gwdata->num_threads > 1) wait_on_auxiliary_threads (gwdata);

	if (is_quick) return;

	asm_data->zero_fft = 0;
	asm_data->const_fft = 0;
	asm_data->add_sub_smallmul_op = 1;

	if (gwdata->cpu_flags & CPU_AVX512F) {

		asm_data->DESTARG = d1;		
		asm_data->data_addr = d1;	
		asm_data->norm_ptr1 = biglit_data_ptr (gwdata, 0);
		asm_data->this_block = 0;
		gwz3_apply_carries (asm_data);

		if (d2 != NULL) {
			asm_data->carries = data.d2_carries;
			asm_data->DESTARG = d2;		
			asm_data->data_addr = d2;	
			asm_data->norm_ptr1 = biglit_data_ptr (gwdata, 0);
			asm_data->this_block = 0;
			gwz3_apply_carries (asm_data);
			asm_data->carries = data.d1_carries;
			aligned_free (data.d2_carries);
		}
	}

	else {

		asm_data->DESTARG = d1;		
 		asm_data->data_addr = d1;	
		asm_data->norm_ptr1 = asm_data->norm_biglit_array;
		asm_data->this_block = 0;
		gwy3_apply_carries (asm_data);

		if (d2 != NULL) {
			asm_data->carries = data.d2_carries;
			asm_data->DESTARG = d2;		
			asm_data->data_addr = d2;	
			asm_data->norm_ptr1 = asm_data->norm_biglit_array;
			asm_data->this_block = 0;
			gwy3_apply_carries (asm_data);
			asm_data->carries = data.d1_carries;
			aligned_free (data.d2_carries);
		}
	}
}

void do_multithread_op_work (
	gwhandle *gwdata,		
	struct gwasm_data *asm_data)	
{
	struct multithread_op_data *data = (struct multithread_op_data *) gwdata->multithread_op_data;

	if (data->asm_proc == NULL) {
		for ( ; ; ) {

			int i = (int) atomic_fetch_incr (gwdata->next_block);
			if (i >= data->num_blks) break;

			memcpy ((char *) data->d1 + i * 4096, (char *) data->s1 + i * 4096, 4096);
		}
	}

	else if (gwdata->cpu_flags & CPU_AVX512F) {
		for ( ; ; ) {

			int i = (int) atomic_fetch_addin (gwdata->next_block, (data->is_quick ? 1 : 4));
			if (i >= data->num_blks) break;

			asm_data->SRCARG = (char *) data->s1 + i * asm_data->pass1blkdst + (i/4) * asm_data->normblkdst4;
			asm_data->SRC2ARG = (char *) data->s2 + i * asm_data->pass1blkdst + (i/4) * asm_data->normblkdst4;
			asm_data->DESTARG = (char *) data->d1 + i * asm_data->pass1blkdst + (i/4) * asm_data->normblkdst4;
			asm_data->DEST2ARG = (char *) data->d2 + i * asm_data->pass1blkdst + (i/4) * asm_data->normblkdst4;
			asm_data->data_addr = (char *) data->d1_carries + i * 128;	
			asm_data->premult_addr = (char *) data->d2_carries + i * 128;	
			asm_data->norm_ptr1 = biglit_data_ptr (gwdata, i);
			call_op (data->asm_proc, asm_data);
		}
	}

	else {
		for ( ; ; ) {

			int i = (int) atomic_fetch_addin (gwdata->next_block, 1);
			if (i >= data->num_blks) break;

			asm_data->SRCARG = (char *) data->s1 + i * asm_data->pass1blkdst;
			asm_data->SRC2ARG = (char *) data->s2 + i * asm_data->pass1blkdst;
			asm_data->DESTARG = (char *) data->d1 + i * asm_data->pass1blkdst;
			asm_data->DEST2ARG = (char *) data->d2 + i * asm_data->pass1blkdst;
			asm_data->data_addr = (char *) data->d1_carries + i * 64;	
			asm_data->premult_addr = (char *) data->d2_carries + i * 64;	
			asm_data->premult_prefetch = (char *) asm_data->norm_grp_mults +
				(i / asm_data->count2) * (gwdata->ZERO_PADDED_FFT ? 320 : 640);			
			asm_data->norm_ptr1 = (char *) asm_data->norm_biglit_array + i * asm_data->normval3;	
			call_op (data->asm_proc, asm_data);
		}
	}
}

void gwcopy (			
	gwhandle *gwdata,	
	gwnum	s,		
	gwnum	d)		
{

	if (gwdata->GENERAL_MMGW_MOD) {
		gwcopy (gwdata->cyclic_gwdata, s, d);
		if (FFT_state (s) != NOT_FFTed) gwcopy (gwdata->negacyclic_gwdata, negacyclic_gwnum (gwdata, s), negacyclic_gwnum (gwdata, d));
		return;
	}

	memmove ((char *) d - GW_HEADER_SIZE (gwdata), (char *) s - GW_HEADER_SIZE (gwdata), GW_HEADER_SIZE (gwdata) - 8 * sizeof (uint32_t));

	if (gwdata->num_threads > 1)
		multithread_op (gwdata, s, NULL, d, NULL, NULL, TRUE);
	else
		memmove ((char *) &(((uint32_t *) d)[-7]), (char *) &(((uint32_t *) s)[-7]), 7 * sizeof (uint32_t) + gwnum_datasize (gwdata));

	gwdata->read_count += 1;
	gwdata->write_count += 1;
}

void gwadd3o (			
	gwhandle *gwdata,	
	gwnum	s1,		
	gwnum	s2,		
	gwnum	d,		
	int	options)      
{
	struct gwasm_data *asm_data = (struct gwasm_data *) gwdata->asm_data;
	float	new_normcnt;

	ASSERTG (unnorms (s1) >= 0.0f);
	ASSERTG (unnorms (s2) >= 0.0f);
	ASSERTG (FFT_state (s1) != FFTed_FOR_FMA && FFT_state (s2) != FFTed_FOR_FMA);

	ASSERTG (!(options & GWADD_DELAY_NORMALIZE) || !(options & GWADD_FORCE_NORMALIZE));	
	
	ASSERTG (options & (GWADD_FORCE_NORMALIZE | GWADD_DELAY_NORMALIZE | GWADD_GUARANTEED_OK | GWADD_SQUARE_INPUT | GWADD_MUL_INPUT | GWADD_ADD_INPUT));

	if (options & GWADD_FORCE_NORMALIZE) {
		gwunfft (gwdata, s1, s1);
		gwunfft (gwdata, s2, s2);
	}
	else if (FFT_state (s1) != FFT_state (s2)) {
		gwfft (gwdata, s1, s1);
		gwfft (gwdata, s2, s2);
	}

	if (gwdata->GENERAL_MMGW_MOD) {
		gwadd3o (gwdata->cyclic_gwdata, s1, s2, d, options);
		if (FFT_state (s1) != NOT_FFTed)
			gwadd3o (gwdata->negacyclic_gwdata, negacyclic_gwnum (gwdata, s1), negacyclic_gwnum (gwdata, s2), negacyclic_gwnum (gwdata, d), options);
		return;
	}

	if (options & GWADD_NON_RANDOM_DATA) new_normcnt = eb_to_numadds (numadds_to_eb (unnorms (s1)) + numadds_to_eb (unnorms (s2)) + 1.0f);
	else new_normcnt = unnorms (s1) + unnorms (s2) + 1.0f;
	if (options & GWADD_GUARANTEED_OK) new_normcnt = 0.0f;

	if (FFT_state (s1) != NOT_FFTed || FFT_state (s2) != NOT_FFTed) {

		if (gwdata->ZERO_PADDED_FFT) {
			d[-5] = s1[-5] + s2[-5];
			d[-6] = s1[-6] + s2[-6];
			d[-7] = s1[-7] + s2[-7];
			d[-8] = s1[-8] + s2[-8];
			d[-9] = s1[-9] + s2[-9];
			d[-10] = s1[-10] + s2[-10];
			d[-11] = s1[-11] + s2[-11];
		}

		if ((gwdata->cpu_flags & CPU_AVX512F) || (gwdata->cpu_flags & CPU_AVX && gwdata->PASS2_SIZE)) {
			multithread_op (gwdata, s1, s2, d, NULL, addr_gw_addf (gwdata), TRUE);
		}

		else {
			asm_data->SRCARG = s1;
			asm_data->SRC2ARG = s2;
			asm_data->DESTARG = d;
			gw_addf (gwdata, asm_data);
		}

		unnorms (d) = new_normcnt;
		FFT_state (d) = FFT_state (s1);
	}

	else if ((options & GWADD_DELAY_NORMALIZE) ||
		 (!(options & GWADD_FORCE_NORMALIZE) &&
		  ((options & GWADD_SQUARE_INPUT) ? square_safe (gwdata, new_normcnt) :
		   (options & GWADD_ADD_INPUT) ? addmul_safe (gwdata, new_normcnt, 1, 1) :
		   mul_safe (gwdata, new_normcnt, 1)))) {

		if ((gwdata->cpu_flags & CPU_AVX512F) || (gwdata->cpu_flags & CPU_AVX && gwdata->PASS2_SIZE)) {
			multithread_op (gwdata, s1, s2, d, NULL, addr_gw_addq (gwdata), TRUE);
		}

		else {
			asm_data->SRCARG = s1;
			asm_data->SRC2ARG = s2;
			asm_data->DESTARG = d;
			gw_addq (gwdata, asm_data);
		}

		unnorms (d) = new_normcnt;
		FFT_state (d) = NOT_FFTed;
	}

	else {

		if ((gwdata->cpu_flags & CPU_AVX512F) || (gwdata->cpu_flags & CPU_AVX && gwdata->PASS2_SIZE)) {
			multithread_op (gwdata, s1, s2, d, NULL, addr_gw_add (gwdata), FALSE);
		}

		else {
			asm_data->SRCARG = s1;
			asm_data->SRC2ARG = s2;
			asm_data->DESTARG = d;
			gw_add (gwdata, asm_data);
		}

		unnorms (d) = 0.0f;
		FFT_state (d) = NOT_FFTed;
	}

	gwdata->read_count += (s1 == s2) ? 1 : 2;
	gwdata->write_count += 1;
}

void gwsub3o (			
	gwhandle *gwdata,	
	gwnum	s1,		
	gwnum	s2,		
	gwnum	d,		
	int	options)      
{
	struct gwasm_data *asm_data = (struct gwasm_data *) gwdata->asm_data;
	float	new_normcnt;

	ASSERTG (unnorms (s1) >= 0.0f);
	ASSERTG (unnorms (s2) >= 0.0f);
	ASSERTG (FFT_state (s1) != FFTed_FOR_FMA && FFT_state (s2) != FFTed_FOR_FMA);

	ASSERTG (!(options & GWADD_DELAY_NORMALIZE) || !(options & GWADD_FORCE_NORMALIZE));	
	
	ASSERTG (options & (GWADD_FORCE_NORMALIZE | GWADD_DELAY_NORMALIZE | GWADD_GUARANTEED_OK | GWADD_SQUARE_INPUT | GWADD_MUL_INPUT | GWADD_ADD_INPUT));

	if (options & GWADD_FORCE_NORMALIZE) {
		gwunfft (gwdata, s1, s1);
		gwunfft (gwdata, s2, s2);
	}
	else if (FFT_state (s1) != FFT_state (s2)) {
		gwfft (gwdata, s1, s1);
		gwfft (gwdata, s2, s2);
	}

	if (gwdata->GENERAL_MMGW_MOD) {
		gwsub3o (gwdata->cyclic_gwdata, s1, s2, d, options);
		if (FFT_state (s1) != NOT_FFTed)
			gwsub3o (gwdata->negacyclic_gwdata, negacyclic_gwnum (gwdata, s1), negacyclic_gwnum (gwdata, s2), negacyclic_gwnum (gwdata, d), options);
		return;
	}

	if (options & GWADD_NON_RANDOM_DATA) new_normcnt = eb_to_numadds (numadds_to_eb (unnorms (s1)) + numadds_to_eb (unnorms (s2)) + 1.0f);
	else new_normcnt = unnorms (s1) + unnorms (s2) + 1.0f;
	if (options & GWADD_GUARANTEED_OK) new_normcnt = 0.0f;

	if (FFT_state (s1) != NOT_FFTed || FFT_state (s2) != NOT_FFTed) {

		if (gwdata->ZERO_PADDED_FFT) {
			d[-5] = s1[-5] - s2[-5];
			d[-6] = s1[-6] - s2[-6];
			d[-7] = s1[-7] - s2[-7];
			d[-8] = s1[-8] - s2[-8];
			d[-9] = s1[-9] - s2[-9];
			d[-10] = s1[-10] - s2[-10];
			d[-11] = s1[-11] - s2[-11];
		}

		if ((gwdata->cpu_flags & CPU_AVX512F) || (gwdata->cpu_flags & CPU_AVX && gwdata->PASS2_SIZE)) {
			multithread_op (gwdata, s2, s1, d, NULL, addr_gw_subf (gwdata), TRUE);
		}

		else {
			asm_data->SRCARG = s2;
			asm_data->SRC2ARG = s1;
			asm_data->DESTARG = d;
			gw_subf (gwdata, asm_data);
		}

		unnorms (d) = new_normcnt;
		FFT_state (d) = FFT_state (s1);
	}

	else if ((options & GWADD_DELAY_NORMALIZE) ||
		 (!(options & GWADD_FORCE_NORMALIZE) &&
		  ((options & GWADD_SQUARE_INPUT) ? square_safe (gwdata, new_normcnt) :
		   (options & GWADD_ADD_INPUT) ? addmul_safe (gwdata, new_normcnt, 1, 1) :
		   mul_safe (gwdata, new_normcnt, 1)))) {

		if ((gwdata->cpu_flags & CPU_AVX512F) || (gwdata->cpu_flags & CPU_AVX && gwdata->PASS2_SIZE)) {
			multithread_op (gwdata, s2, s1, d, NULL, addr_gw_subq (gwdata), TRUE);
		}

		else {
			asm_data->SRCARG = s2;
			asm_data->SRC2ARG = s1;
			asm_data->DESTARG = d;
			gw_subq (gwdata, asm_data);
		}

		unnorms (d) = new_normcnt;
		FFT_state (d) = NOT_FFTed;
	}

	else {

		if ((gwdata->cpu_flags & CPU_AVX512F) || (gwdata->cpu_flags & CPU_AVX && gwdata->PASS2_SIZE)) {
			multithread_op (gwdata, s2, s1, d, NULL, addr_gw_sub (gwdata), FALSE);
		}

		else {
			asm_data->SRCARG = s2;
			asm_data->SRC2ARG = s1;
			asm_data->DESTARG = d;
			gw_sub (gwdata, asm_data);
		}

		unnorms (d) = 0.0f;
		FFT_state (d) = NOT_FFTed;
	}

	gwdata->read_count += 2;
	gwdata->write_count += 1;
}

void gwaddsub4o (		
	gwhandle *gwdata,	
	gwnum	s1,		
	gwnum	s2,		
	gwnum	d1,		
	gwnum	d2,		
	int	options)
{
	struct gwasm_data *asm_data = (struct gwasm_data *) gwdata->asm_data;
	float	new_normcnt;

	ASSERTG (unnorms (s1) >= 0.0f);
	ASSERTG (unnorms (s2) >= 0.0f);
	ASSERTG (FFT_state (s1) != FFTed_FOR_FMA && FFT_state (s2) != FFTed_FOR_FMA);

	ASSERTG (!(options & GWADD_DELAY_NORMALIZE) || !(options & GWADD_FORCE_NORMALIZE));	
	
	ASSERTG (options & (GWADD_FORCE_NORMALIZE | GWADD_DELAY_NORMALIZE | GWADD_GUARANTEED_OK | GWADD_SQUARE_INPUT | GWADD_MUL_INPUT | GWADD_ADD_INPUT));

	if (options & GWADD_FORCE_NORMALIZE) {
		gwunfft (gwdata, s1, s1);
		gwunfft (gwdata, s2, s2);
	}
	else if (FFT_state (s1) != FFT_state (s2)) {
		gwfft (gwdata, s1, s1);
		gwfft (gwdata, s2, s2);
	}

	if (gwdata->GENERAL_MMGW_MOD) {
		gwaddsub4o (gwdata->cyclic_gwdata, s1, s2, d1, d2, options);
		if (FFT_state (s1) != NOT_FFTed)
			gwaddsub4o (gwdata->negacyclic_gwdata, negacyclic_gwnum (gwdata, s1), negacyclic_gwnum (gwdata, s2),
				    negacyclic_gwnum (gwdata, d1), negacyclic_gwnum (gwdata, d2), options);
		return;
	}

	if (options & GWADD_NON_RANDOM_DATA) new_normcnt = eb_to_numadds (numadds_to_eb (unnorms (s1)) + numadds_to_eb (unnorms (s2)) + 1.0f);
	else new_normcnt = unnorms (s1) + unnorms (s2) + 1.0f;
	if (options & GWADD_GUARANTEED_OK) new_normcnt = 0.0f;

	if (FFT_state (s1) != NOT_FFTed || FFT_state (s2) != NOT_FFTed) {

		if (gwdata->ZERO_PADDED_FFT) {
			double	v1, v2;
			v1 = s1[-5]; v2 = s2[-5]; d1[-5] = v1 + v2; d2[-5] = v1 - v2;
			v1 = s1[-6]; v2 = s2[-6]; d1[-6] = v1 + v2; d2[-6] = v1 - v2;
			v1 = s1[-7]; v2 = s2[-7]; d1[-7] = v1 + v2; d2[-7] = v1 - v2;
			v1 = s1[-8]; v2 = s2[-8]; d1[-8] = v1 + v2; d2[-8] = v1 - v2;
			v1 = s1[-9]; v2 = s2[-9]; d1[-9] = v1 + v2; d2[-9] = v1 - v2;
			v1 = s1[-10]; v2 = s2[-10]; d1[-10] = v1 + v2; d2[-10] = v1 - v2;
			v1 = s1[-11]; v2 = s2[-11]; d1[-11] = v1 + v2; d2[-11] = v1 - v2;
		}

		if ((gwdata->cpu_flags & CPU_AVX512F) || (gwdata->cpu_flags & CPU_AVX && gwdata->PASS2_SIZE)) {
			multithread_op (gwdata, s1, s2, d1, d2, addr_gw_addsubf (gwdata), TRUE);
		}

		else {
			asm_data->SRCARG = s1;
			asm_data->SRC2ARG = s2;
			asm_data->DESTARG = d1;
			asm_data->DEST2ARG = d2;
			gw_addsubf (gwdata, asm_data);
		}

		unnorms (d1) = unnorms (d2) = new_normcnt;
		FFT_state (d1) = FFT_state (d2) = FFT_state (s1);
	}

	else if ((options & GWADD_DELAY_NORMALIZE) ||
		 (!(options & GWADD_FORCE_NORMALIZE) &&
		  ((options & GWADD_SQUARE_INPUT) ? square_safe (gwdata, new_normcnt) :
		   (options & GWADD_ADD_INPUT) ? addmul_safe (gwdata, new_normcnt, 1, 1) :
		   mul_safe (gwdata, new_normcnt, 1)))) {

		if ((gwdata->cpu_flags & CPU_AVX512F) || (gwdata->cpu_flags & CPU_AVX && gwdata->PASS2_SIZE)) {
			multithread_op (gwdata, s1, s2, d1, d2, addr_gw_addsubq (gwdata), TRUE);
		}

		else {
			asm_data->SRCARG = s1;
			asm_data->SRC2ARG = s2;
			asm_data->DESTARG = d1;
			asm_data->DEST2ARG = d2;
			gw_addsubq (gwdata, asm_data);
		}

		unnorms (d1) = unnorms (d2) = new_normcnt;
		FFT_state (d1) = FFT_state (d2) = NOT_FFTed;
	}

	else {

		if ((gwdata->cpu_flags & CPU_AVX512F) || (gwdata->cpu_flags & CPU_AVX && gwdata->PASS2_SIZE)) {
			multithread_op (gwdata, s1, s2, d1, d2, addr_gw_addsub (gwdata), FALSE);
		}

		else {
			asm_data->SRCARG = s1;
			asm_data->SRC2ARG = s2;
			asm_data->DESTARG = d1;
			asm_data->DEST2ARG = d2;
			gw_addsub (gwdata, asm_data);
		}

		unnorms (d1) = 0.0f;
		unnorms (d2) = 0.0f;
		FFT_state (d1) = NOT_FFTed;
		FFT_state (d2) = NOT_FFTed;
	}

	gwdata->read_count += 2;
	gwdata->write_count += 2;
}

void gwsmalladd (
	gwhandle *gwdata,	
	int64_t	addin,		
	gwnum	g)		
{

	ASSERTG (unnorms (g) >= 0.0f);
	ASSERTG (FFT_state (g) == NOT_FFTed);

	if (!gwdata->GENERAL_MMGW_MOD && (gwdata->k == 1.0 || labs (gwdata->c) == 1)) {
		int64_t	low_addin;
		gwiter iter;
		int32_t maxval1 = intpow (gwdata->b, gwdata->NUM_B_PER_SMALL_WORD);
		int32_t maxval2 = maxval1 * gwdata->b;

		if (gwdata->k > 1.0) {
			int64_t quotient, remainder, high_addin;

			quotient = addin / (int64_t) gwdata->k;
			remainder = addin - quotient * (int64_t) gwdata->k;
			low_addin = quotient;
			high_addin = (gwdata->c == -1) ? remainder : -remainder;

			unsigned long word, bit_in_word;
			bitaddr (gwdata, gwdata->n, &word, &bit_in_word);
			for (gwiter_init (gwdata, &iter, g, word); high_addin && gwiter_index (&iter) < gwdata->FFTLEN; gwiter_next (&iter), bit_in_word = 0) {

				int32_t maxval = gwiter_is_big_word (&iter) ? maxval2 : maxval1;
				if (maxval == 1) continue;

				int32_t	value;
				gwiter_get_fft_value (&iter, &value);

				int32_t scale = bit_in_word ? intpow (gwdata->b, bit_in_word) : 1;
				int32_t scaledmaxval = maxval / scale;
				quotient = high_addin / scaledmaxval;
				value += (int32_t) (high_addin - quotient * scaledmaxval) * scale;
				high_addin = quotient;

				if (gwiter_index (&iter) != gwdata->FFTLEN - 1) {
					if (value > (maxval >> 1)) {
						value = value - maxval;
						high_addin++;
					}
					if (value < -(maxval >> 1)) {
						value = value + maxval;
						high_addin--;
					}
				}

				gwiter_set_fft_value (&iter, value);
			}
		}

		else
			low_addin = addin;

		for (gwiter_init_zero (gwdata, &iter, g); low_addin; gwiter_next (&iter)) {

			int32_t maxval = gwiter_is_big_word (&iter) ? maxval2 : maxval1;
			if (maxval == 1) continue;

			int32_t	value;
			gwiter_get_fft_value (&iter, &value);

			if (low_addin >= -7 && low_addin <= 7) {
				gwiter_set_fft_value (&iter, value + (int32_t) low_addin);
				break;
			}

			int64_t quotient = low_addin / maxval;
			value += (int32_t) (low_addin - quotient * maxval);
			low_addin = quotient;

			if (value > (maxval >> 1)) {
				value = value - maxval;
				low_addin++;
			}
			if (value < -(maxval >> 1)) {
				value = value + maxval;
				low_addin--;
			}

			gwiter_set_fft_value (&iter, value);
		}
	}

	else {
		gwnum tmp = gwalloc (gwdata);
		s64togw (gwdata, addin, tmp);
		gwadd3o (gwdata, g, tmp, g, GWADD_FORCE_NORMALIZE);
		gwfree (gwdata, tmp);
	}
}

void gwsmallmul (
	gwhandle *gwdata,	
	double	mult,		
	gwnum	g)		
{
	struct gwasm_data *asm_data = (struct gwasm_data *) gwdata->asm_data;

	ASSERTG (unnorms (g) >= 0.0f);
	ASSERTG (FFT_state (g) == NOT_FFTed);

	if (gwdata->GENERAL_MMGW_MOD) {
		if (FFT_state (g) != NOT_FFTed) gwsmallmul (gwdata->negacyclic_gwdata, mult, negacyclic_gwnum (gwdata, g));
		gwsmallmul (gwdata->cyclic_gwdata, mult, g);
		return;
	}

	if ((! (gwdata->cpu_flags & (CPU_AVX512F | CPU_AVX | CPU_SSE2)) && (mult > 1024.0 || gwdata->ZERO_PADDED_FFT)) ||
	    (! (gwdata->cpu_flags & (CPU_AVX512F | CPU_AVX)) && gwdata->avg_num_b_per_word < 0.5 && mult > gwdata->b) ||
	    (gwdata->ZERO_PADDED_FFT && gwdata->k * mult > pow (gwdata->b, floor (4 * gwdata->avg_num_b_per_word)) / 2.0)) {
		gwnum	tmp;
		tmp = gwalloc (gwdata);
		if (mult == 1.0);
		else if (mult == 2.0) {
			gwadd3o (gwdata, g, g, g, GWADD_NON_RANDOM_DATA | GWADD_SQUARE_INPUT);
		}
		else if (mult == 3.0) {
			gwadd3o (gwdata, g, g, tmp, GWADD_NON_RANDOM_DATA | GWADD_DELAY_NORMALIZE);
			gwadd3o (gwdata, g, tmp, g, GWADD_NON_RANDOM_DATA | GWADD_SQUARE_INPUT);
		}
		else if (mult == 4.0) {
			gwadd3o (gwdata, g, g, g, GWADD_NON_RANDOM_DATA | GWADD_DELAY_NORMALIZE);
			gwadd3o (gwdata, g, g, g, GWADD_NON_RANDOM_DATA | GWADD_SQUARE_INPUT);
		}
		else if (mult == 5.0) {
			gwadd3o (gwdata, g, g, tmp, GWADD_NON_RANDOM_DATA | GWADD_DELAY_NORMALIZE);
			gwadd3o (gwdata, tmp, tmp, tmp, GWADD_NON_RANDOM_DATA | GWADD_DELAY_NORMALIZE);
			gwadd3o (gwdata, g, tmp, g, GWADD_NON_RANDOM_DATA | GWADD_SQUARE_INPUT);
		}
		else if (mult == 6.0) {
			gwadd3o (gwdata, g, g, tmp, GWADD_NON_RANDOM_DATA | GWADD_DELAY_NORMALIZE);
			gwadd3o (gwdata, g, tmp, g, GWADD_NON_RANDOM_DATA | GWADD_DELAY_NORMALIZE);
			gwadd3o (gwdata, g, g, g, GWADD_NON_RANDOM_DATA | GWADD_SQUARE_INPUT);
		}
		else if (mult == 8.0) {
			gwadd3o (gwdata, g, g, g, GWADD_NON_RANDOM_DATA | GWADD_DELAY_NORMALIZE);
			gwadd3o (gwdata, g, g, g, GWADD_NON_RANDOM_DATA | GWADD_DELAY_NORMALIZE);
			gwadd3o (gwdata, g, g, g, GWADD_NON_RANDOM_DATA | GWADD_SQUARE_INPUT);
		}
		else if (mult == 9.0) {
			gwadd3o (gwdata, g, g, tmp, GWADD_NON_RANDOM_DATA | GWADD_DELAY_NORMALIZE);
			gwadd3o (gwdata, tmp, tmp, tmp, GWADD_NON_RANDOM_DATA | GWADD_DELAY_NORMALIZE);
			gwadd3o (gwdata, tmp, tmp, tmp, GWADD_NON_RANDOM_DATA | GWADD_DELAY_NORMALIZE);
			gwadd3o (gwdata, g, tmp, g, GWADD_NON_RANDOM_DATA | GWADD_SQUARE_INPUT);
		}
		else {
			dbltogw (gwdata, mult, tmp);
			gwmul3 (gwdata, tmp, g, g, 0);
		}
		gwfree (gwdata, tmp);
	}

	else if ((gwdata->cpu_flags & CPU_AVX512F) || (gwdata->cpu_flags & CPU_AVX && gwdata->PASS2_SIZE)) {
		asm_data->DBLARG = mult;
		multithread_op (gwdata, g, NULL, g, NULL, addr_gw_muls (gwdata), FALSE);
	}

	else {
		asm_data->DESTARG = g;
		asm_data->DBLARG = mult;
		gw_muls (gwdata, asm_data);
	}

	unnorms (g) = 0.0f;
	gwdata->read_count += 1;
	gwdata->write_count += 1;

	if (gwdata->GENERAL_MOD &&
	    (* (double *) ((char *) g + gwdata->GW_GEN_MOD_MAX_OFFSET) <= -2.0 ||
	     * (double *) ((char *) g + gwdata->GW_GEN_MOD_MAX_OFFSET) > 0.0))
		emulate_mod (gwdata, g);
}
