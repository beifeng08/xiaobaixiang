// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

#ifndef _GIANTS_H_
#define _GIANTS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "gwcommon.h"

#define GIANT_OUT_OF_MEMORY	749156

#define AUTO_MUL 0
#define GRAMMAR_MUL 1
#define FFT_MUL 2
#define KARAT_MUL 3

typedef struct
{
	int	maxsize;	
	int	sign;		
	uint32_t *n;		
} giantstruct;

typedef giantstruct *giant;

#ifdef GDEBUG
#define setmaxsize(g,s)	(g)->maxsize = s
#else
#define setmaxsize(g,s)
#endif

#define stackgiant(name,count) uint32_t name##_data[count]; giantstruct name##_struct = {count, 0, (uint32_t *) &name##_data}; const giant name = &name##_struct

giant 	allocgiant(int count);

int 	bitlen(giant n);

#define bitval(g,pos)	((g)->n[(pos)>>5] & (1 << ((pos)&31)))

#define isZero(g)	((g)->sign == 0)

#define isone(g)	(((g)->sign == 1) && ((g)->n[0] == 1))

#define istwo(g)	(((g)->sign == 1) && ((g)->n[0] == 2))

void 	gtog(giant src, giant dest);

#define setzero(g)	(g)->sign = 0
#define setone(g)	(g)->sign = (g)->n[0] = 1
void 	itog(int32_t n, giant g);
void 	ultog(uint32_t n, giant g);

void 	slltog(int64_t n, giant g);
void 	ulltog(uint64_t n, giant g);

void 	dbltog(double n, giant g);

void 	ctog (const char *s, giant g);
void 	gtoc (giant g, char *s, int sbufsize);

int	gsign(giant g);

int	gcompg(giant a, giant b);

void	setmulmode(int mode);

#define negg(g)		((g)->sign = -(g)->sign)

#define absg(g)		if ((g)->sign < 0) (g)->sign = -(g)->sign

#define	iaddg(i,g)	sladdg ((int32_t)(i), g)

void 	uladdg (uint32_t i, giant g);

void 	sladdg (int32_t i, giant g);

void 	addg(giant a, giant b);

void 	ulsubg (uint32_t i, giant g);

void 	subg(giant a, giant b);

int	squareg(giant g);

int	mulg(giant a, giant b);

void 	ulmulg (uint32_t i, giant g);
void 	imulg (int32_t i, giant g);

void 	ullmulg (uint64_t i, giant g);

void 	dblmulg (double n, giant g);

void 	modg(giant den, giant num);

void 	divg(giant den, giant num);
void 	dbldivg(double den, giant num);

void 	gmaskbits(int bits, giant g);

void 	gshiftleft(int bits, giant g);

#define	gshiftright(n,g)	{if (n) gtogshiftright (n, g, g);}
void 	gtogshiftright (int bits, giant src, giant dest);

void gtogshiftrightsplit (int bits, giant src, giant dest, int64_t accum);

void gtogshiftleftunsplit (int bits, giant src, giant dest);

int 	invg(giant n, giant x);

int 	gcdg(giant n, giant x);

void power (giant x, int n);
void powerg (giant x, giant n);

void 	powermod(giant x, int n, giant z);

void 	powermodg(giant x, giant n, giant z);

typedef struct {
	 void	*memblk;		
	 long	offset;			
	 giant	prev;			
} gstacknode;

typedef struct {
	 unsigned long blksize;		
	 void*	(*allocate)(void *);	
	 void	(*free)(void *,void *);	
	 void	(*deallocate)(void *);	
	 void*	handle;			
	 int	num_popgs;		
	 gstacknode stack;		
	 double	*ooura_sincos;
	 int	*ooura_ip;
	 int	ooura_fft_size;
	 giant	cur_recip;
	 giant	cur_den;
} ghandle;

void	init_ghandle (ghandle *);
void	term_ghandle (ghandle *);

giant	popg(ghandle *,int);	
void	pushg(ghandle *,int);

int	squaregi(ghandle *, giant g);
int	mulgi(ghandle *, giant a, giant b);
void 	modgi(ghandle *, giant den, giant num);
void 	divgi(ghandle *, giant den, giant num);
int 	invgi(ghandle *, int, giant n, giant x);
int 	gcdgi(ghandle *, int, giant n, giant x);

extern void addhlp (uint32_t *res, uint32_t *carry, uint32_t val);
extern void subhlp (uint32_t *res, uint32_t *carry, uint32_t val);
extern void muladdhlp (uint32_t *res, uint32_t *carryl, uint32_t *carryh, uint32_t val1, uint32_t val2);
extern void muladd2hlp (uint32_t *res, uint32_t *carryl, uint32_t *carryh, uint32_t val1, uint32_t val2);
extern void mulsubhlp (uint32_t *res, uint32_t *carryl, uint32_t *carryh, uint32_t val1, uint32_t val2);

extern int (*StopCheckRoutine)(int);

#define newgiant(numshorts)	allocgiant(((numshorts)+1)/2)

#ifdef __cplusplus
}
#endif

#define gtompz(g,m)	mpz_import (m, (g)->sign, -1, sizeof ((g)->n[0]), 0, 0, (g)->n)
#define mpztog(m,g)	{size_t	count; mpz_export ((g)->n, &count, -1, sizeof ((g)->n[0]), 0, 0, m); (g)->sign = (int) count;}

#endif
