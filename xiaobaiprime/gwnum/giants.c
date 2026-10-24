// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include "giants.h"
#include "gwutil.h"
#include "fftsg.c"

#define TWOPI (double)(2*3.1415926535897932384626433)
#define SQRT2 (double)(1.414213562373095048801688724209)
#define SQRTHALF (double)(0.707106781186547524400844362104)
#define TWO_TO_MINUS_19 (double)(0.0000019073486328125)
#define ONE_MINUS_TWO_TO_MINUS_19 (double)(0.9999980926513671875)

#define KARAT_BREAK_SQUARE	36
#define KARAT_BREAK_MULT	39

#define FFT_BREAK_SQUARE	47
#define FFT_BREAK_MULT1		40
#define FFT_BREAK_MULT2		100
#define FFT_BREAK_MULT3		99

#define GCDLIMIT 150

#define	STACK_GROW	100

#define CHGCD_BREAK	24

typedef struct gmatrixstruct {
	 giant	ul;			
	 giant	ur;			
	 giant	ll;			
	 giant	lr;			
} gmatrixstruct;
typedef gmatrixstruct *gmatrix;

#define stackstart(g)	(g)->num_popgs
#define pushall(g,x)	pushg(g,(g)->num_popgs - x)

int	mulmode = AUTO_MUL;

void	make_recip(ghandle *, giant d, giant r);

void	divg_via_recip(ghandle *, giant d, giant r, giant n);

void		normal_addg(giant, giant);
void		normal_subg(giant, giant);
void		reverse_subg(giant, giant, int);
int 		automulg(ghandle *, giant a, giant b);
void 		grammarmulg(ghandle *, giant a, giant b);
void		grammarsquareg(ghandle *, giant b);
void 		karatmulg(ghandle *, giant a, giant b);
void 		karatsquareg(ghandle *, giant b);

int		init_sincos(ghandle *,int);
int 		lpt(int);
void 		addsignal(giant, int, double *, int);
int 		FFTsquareg(ghandle *, giant x);
int 		FFTmulg(ghandle *, giant y, giant x);
void 		giant_to_double(giant x, int sizex, double *z, int L);

#define gswap(p,q)  {giant tgq; tgq = *(p); *(p) = *(q); *(q) = tgq;}
#define ulswap(p,q)  {unsigned long tgq; tgq = p; p = q; q = tgq;}

int		invg_common (ghandle *, giant, giant, int);
int		gcdg_common (ghandle *, giant, giant, int);
int	 	cextgcdg (ghandle *, giant *, giant *, gmatrix A, int);
int		ggcd (ghandle *, giant *, giant *, gmatrix, int);
void 		onestep (ghandle *, giant *, giant *, gmatrix);
int 		mulvM (ghandle *, gmatrix, giant, giant);
int 		mulmM (ghandle *, gmatrix, gmatrix);
int 		mulmMsp (ghandle *, gmatrix, gmatrix, int);
void		punch (ghandle *, giant, gmatrix);
int		hgcd (ghandle *, int, giant *, giant *, gmatrix, int);
int		rhgcd (ghandle *, giant *, giant *, gmatrix, int);

#define sintstackg(i,g) if(i<0){i=-i;g.sign=-1;}else g.sign=1;g.n=(uint32_t*)&i;setmaxsize(&g,1);
#define uintstackg(i,g) g.sign=1;g.n=(uint32_t*)&i;setmaxsize(&g,1);
#define ullstackg(i,g) g.n=(uint32_t*)&i;g.sign=(g.n[1]?2:1);setmaxsize(&g,2);

int gcdhlp (uint32_t, uint32_t *, uint32_t, uint32_t *, void *);

int (*StopCheckRoutine)(int) = NULL;
#define stopCheck(t)	((*StopCheckRoutine)(t-0x80000000))

giant allocgiant (		
	int 	count)
{
	int 	size;
	giant 	thegiant;

	ASSERTG (count > 0);

	size = sizeof (giantstruct) + count * sizeof (uint32_t);
	thegiant = (giant) malloc (size);
	thegiant->sign = 0;
	thegiant->n = (uint32_t *) ((char *) thegiant + sizeof (giantstruct));
	setmaxsize (thegiant, count);
	return (thegiant);
}

void itog (		
	int32_t	i,
	giant	g)
{
	if (i > 0) {
		g->sign = 1;
		g->n[0] = i;
	} else if (i == 0) {
		g->sign = 0;
		g->n[0] = 0;
	} else {
		g->sign = -1;
		g->n[0] = -i;
	}
}

void ultog (		
	uint32_t i,
	giant	g)
{
	if (i == 0) {
		g->sign = 0;
		g->n[0] = 0;
	} else {
		g->sign = 1;
		g->n[0] = i;
	}
}

void slltog (		
	int64_t i,
	giant	g)
{
	if (i >= 0) {
		ulltog (i, g);
	} else {
		ulltog (-i, g);
		g->sign = -g->sign;
	}
}

void ulltog (		
	uint64_t i,
	giant	g)
{
	if (i == 0) {
		g->sign = 0;
		g->n[0] = 0;
		return;
	}
	g->n[0] = (uint32_t) i;
	g->n[1] = (uint32_t) (i >> 32);
	g->sign = g->n[1] ? 2 : 1;
}

void dbltog (		
	double	i,
	giant	g)
{
	if (i < 0.0) {
		dbltog (-i, g);
		negg (g);
	} else if (i < 4294967296.0) {
		ultog ((uint32_t) i, g);
	} else {
		g->sign = 2;
		g->n[1] = (uint32_t) (i / 4294967296.0);
		g->n[0] = (uint32_t) (i - g->n[1] * 4294967296.0);
	}
}

void ctog (		
	const char *s,
	giant	g)
{
	uint32_t multiplier = 1;
	uint32_t addin = 0;
	for (g->sign = 0; isdigit (*s); s++) {
		multiplier = multiplier * 10;
		addin = addin * 10 + (*s - '0');
		if (multiplier == 1000000000) {
			ulmulg (multiplier, g);
			uladdg (addin, g);
			multiplier = 1;
			addin = 0;
		}
	}
	if (multiplier != 1) {
		ulmulg (multiplier, g);
		uladdg (addin, g);
	}
}

void gtoc (		
	giant	g,
	char	*s,
	int	sbufsize)
{
	ghandle gdata;
	giant	x, y, ten;
	int	i, len;
	char	c;

	ASSERTG (g->sign >= 0);

	init_ghandle (&gdata);

	x = popg (&gdata, g->sign); gtog (g, x);
	y = popg (&gdata, g->sign);
	ten = popg (&gdata, 1); itog (10, ten);
	sbufsize--;
	for (len = 0; len < sbufsize && x->sign; len++) {
		gtog (x, y);
		modgi (&gdata, ten, y);
		s[len] = (char) (y->n[0] + '0');
		divgi (&gdata, ten, x);
	}
	for (i = 0; i < len / 2; i++) {
		c = s[i];
		s[i] = s[len-1-i];
		s[len-1-i] = c;
	}
	s[len] = 0;
	pushg (&gdata, 3);

	term_ghandle (&gdata);
}

void gtog (			
	giant	srcgiant,
	giant	destgiant)
{
	destgiant->sign = srcgiant->sign;
	memmove (destgiant->n, srcgiant->n, abs (srcgiant->sign) * sizeof (uint32_t));
	ASSERTG (abs (destgiant->sign) <= destgiant->maxsize);
}

int gcompg (		
	giant	a,
	giant	b)
{
	int	sa = a->sign, j, sb = b->sign, sgn;
	uint32_t va, vb;

	if (sa > sb)
		return (1);
	if (sa < sb)
		return (-1);
	if (sa < 0) {
		sa = -sa; 
		sgn = -1;
	} else {
		sgn = 1;
	}
	for (j = sa-1; j >= 0; j--) {
		va = a->n[j];
		vb = b->n[j];
		if (va > vb) return (sgn);
		if (va < vb) return (-sgn);
	}
	return (0);
}

void normal_addg (	
	giant	a,
	giant	b)
{
	int 	j, asize = a->sign, bsize = b->sign;
	uint32_t *aptr = a->n, *bptr = b->n;
	uint32_t res, carry;

	ASSERTG (asize >= 0 && bsize >= 0);
	ASSERTG (a->sign == 0 || a->n[abs(a->sign)-1] != 0);
	ASSERTG (b->sign == 0 || b->n[abs(b->sign)-1] != 0);

	res = carry = 0;
	for (j = 0; j < asize || res; j++) {
		if (j < asize) addhlp (&res, &carry, *aptr++);
		if (j < bsize) addhlp (&res, &carry, *bptr);
		*bptr++ = res;
		res = carry;
		carry = 0;
	}
	if (j > bsize) b->sign = j;

	ASSERTG (b->sign == 0 || b->n[abs(b->sign)-1] != 0);
}

void normal_subg (	
	giant	a,
	giant	b)
{
	int 	j, asize = a->sign, bsize = b->sign;
	uint32_t *aptr = a->n, *bptr = b->n;
	uint32_t res, carry;

	ASSERTG (asize >= 0 && bsize >= asize);

	res = carry = 0;
	for (j = 0; j < asize; j++) {
		addhlp (&res, &carry, *bptr);
		subhlp (&res, &carry, *aptr++);
		*bptr++ = res;
		res = carry;
		carry = (int) carry >> 4;	
	}
	for ( ; res; j++)
	{
		if (j < bsize) addhlp (&res, &carry, *bptr);
		*bptr++ = res;
		res = carry;
		carry = (int) carry >> 4;	
	}
	if (j < bsize) return;

	while (j > 0 && b->n[j-1] == 0) j--;
	b->sign = j;

	ASSERTG (b->sign == 0 || b->n[abs(b->sign)-1] != 0);
}

void reverse_subg (	
	giant	a,
	giant	b,
	int	azeros)
{
	int 	j, asize = a->sign + azeros, bsize = b->sign;
	uint32_t *aptr = a->n, *bptr = b->n;
	uint32_t res, carry;

	ASSERTG (bsize >= 0 && asize >= bsize);

	res = carry = 0;
	for (j = 0; j < asize; j++) {
		if (azeros)
			azeros--;
		else
			addhlp (&res, &carry, *aptr++);
		if (j < bsize)
			subhlp (&res, &carry, *bptr);
		*bptr++ = res;
		res = carry;
		carry = (int) carry >> 4;	
	}

	while (j > 0 && b->n[j-1] == 0) j--;
	b->sign = j;

	ASSERTG (b->sign == 0 || b->n[abs(b->sign)-1] != 0);
}

void addg (		
	giant	a,
	giant	b)
{
	int 	asgn = a->sign, bsgn = b->sign;

	ASSERTG (a->sign == 0 || a->n[abs(a->sign)-1] != 0);
	ASSERTG (b->sign == 0 || b->n[abs(b->sign)-1] != 0);

	if (asgn == 0) return;
	if (bsgn == 0) {
		gtog (a, b);
		return;
	}
	if ((asgn < 0) == (bsgn < 0)) {
		if (bsgn > 0) {
			normal_addg (a, b);
		} else {
			absg (b);
			if (a != b) absg (a);
			normal_addg (a, b);
			negg (b);
			if (a != b) negg (a);
		}
	} else if (bsgn > 0) {
		negg (a);
		if (gcompg (b, a) >= 0) {
			normal_subg (a, b);
			negg (a);
		} else {
			reverse_subg (a, b, 0);
			negg (a);
			negg (b);
		}
	} else {
		negg (b);
		if (gcompg(b,a) < 0) {
			reverse_subg (a, b, 0);
		} else {
			normal_subg (a, b);
			negg (b);
		}
	}

	ASSERTG (b->sign == 0 || b->n[abs(b->sign)-1] != 0);
	ASSERTG (abs (b->sign) <= b->maxsize);
}

void sladdg (			
	int32_t i,
	giant	g)
{
	giantstruct tmp;

	if (i == 0) return;

	sintstackg (i, tmp);
	addg (&tmp, g);
}

void uladdg (			
	uint32_t i,
	giant	g)
{
	giantstruct tmp;

	if (i == 0) return;

	uintstackg (i, tmp);
	addg (&tmp, g);
}

void ulsubg (			
	uint32_t i,
	giant	g)
{
	giantstruct tmp;

	if (i == 0) return;

	uintstackg (i, tmp);
	subg (&tmp, g);
}

void subg (		
	giant	a,
	giant	b)
{
	int	asgn = a->sign, bsgn = b->sign;

	ASSERTG (a->sign == 0 || a->n[abs(a->sign)-1] != 0);
	ASSERTG (b->sign == 0 || b->n[abs(b->sign)-1] != 0);

	if (asgn == 0) return;
	if (bsgn == 0) {
		gtog (a, b);
		negg (b);
		return;
	}
	if ((asgn < 0) != (bsgn < 0)) {
		if (bsgn > 0) {
			negg (a);
			normal_addg (a, b);
			negg (a);
		} else {
			negg (b);
			normal_addg (a, b);
			negg (b);
		}
	} else if (bsgn > 0) {
		if (gcompg (b, a) >= 0) {
			normal_subg (a, b);
		} else {
			reverse_subg (a, b, 0);
			negg (b);
		}
	} else {
		negg (a);
		negg (b);
		if (gcompg (b, a) >= 0) {
			normal_subg (a, b);
			negg (a);
			negg (b);
		} else {
			reverse_subg (a, b, 0);
			negg (a);
		}
	}

	ASSERTG (b->sign == 0 || b->n[abs(b->sign)-1] != 0);
	ASSERTG (abs (b->sign) <= b->maxsize);
}

void setmulmode (
	int 	mode)
{
	mulmode = mode;
}

int squareg (			
	giant	b)
{
	ghandle gdata;
	int	stop_reason;

	init_ghandle (&gdata);
	stop_reason = squaregi (&gdata, b);
	term_ghandle (&gdata);
	return (stop_reason);
}

int squaregi (			
	ghandle *gdata,		
	giant	b)
{
	int 	bsize, stop_reason;

	ASSERTG (b->sign == 0 || b->n[abs(b->sign)-1] != 0);

	if (b->sign == 0) return (0);

	absg (b);
	switch (mulmode) {
	case GRAMMAR_MUL:
		grammarsquareg (gdata, b);
		break;
	case FFT_MUL:
		FFTsquareg (gdata, b);
		break;
	case KARAT_MUL:
		karatsquareg (gdata, b);
		break;
	case AUTO_MUL:
		bsize = b->sign;
		if (bsize >= FFT_BREAK_SQUARE) {
			stop_reason = FFTsquareg (gdata, b);
			if (stop_reason) return (stop_reason);
		}
		else if (bsize >= KARAT_BREAK_SQUARE)
			karatsquareg (gdata, b);
		else
			grammarsquareg (gdata, b);
		break;
	}

	ASSERTG (b->sign > 0 && b->n[b->sign-1] != 0);
	return (0);
}

int mulg (			
	giant	a,
	giant	b)
{
	ghandle gdata;
	int	stop_reason;

	init_ghandle (&gdata);
	stop_reason = mulgi (&gdata, a, b);
	term_ghandle (&gdata);
	return (stop_reason);
}

int mulgi (			
	ghandle *gdata,		
	giant	a,
	giant	b)

{
	int 	neg, asign, stop_reason;

	ASSERTG (a->sign == 0 || a->n[abs(a->sign)-1] != 0);
	ASSERTG (b->sign == 0 || b->n[abs(b->sign)-1] != 0);

	if (a == b) {
		return (squaregi (gdata, b));
	}

	if (a->sign == 0 || b->sign == 0) {
		b->sign = 0;
		return (0);
	}

	neg = 0;
	asign = a->sign;
	if (a->sign < 0) { neg = 1; a->sign = -a->sign; }
	if (b->sign < 0) { neg = !neg; b->sign = -b->sign; }

	switch (mulmode) {
	case GRAMMAR_MUL:
		grammarmulg (gdata, a, b);
		break;
	case FFT_MUL:
		FFTmulg (gdata, a, b);
		break;
	case KARAT_MUL:
		karatmulg (gdata, a, b);
		break;
	case AUTO_MUL:
		stop_reason = automulg (gdata, a, b);
		if (stop_reason) {
			if (asign < 0) a->sign = -a->sign;	
			return (stop_reason);
		}
		break;
	}

	if (asign < 0) a->sign = -a->sign;	
	if (neg) b->sign = -b->sign;
	ASSERTG (b->sign != 0 && b->n[abs(b->sign)-1] != 0);
	ASSERTG (abs (b->sign) <= b->maxsize);
	return (0);
}

void ulmulg (			
	uint32_t i,
	giant	g)
{
	giantstruct tmp;

	if (i == 0) {g->sign = 0; return;}
	if (i == 1) return;

	uintstackg (i, tmp);
	mulg (&tmp, g);
}

void imulg (			
	int32_t i,
	giant	g)
{
	giantstruct tmp;

	if (i == 0) {g->sign = 0; return;}
	if (i == 1) return;

	sintstackg (i, tmp);
	mulg (&tmp, g);
}

void ullmulg (			
	uint64_t i,
	giant	g)
{
	giantstruct tmp;

	if (i == 0) {g->sign = 0; return;}
	if (i == 1) return;

	ullstackg (i, tmp);
	mulg (&tmp, g);
}

void dblmulg (			
	double	i,
	giant	g)
{
	stackgiant(tmp,2);

	dbltog (i, tmp);
	mulg (tmp, g);
}

void modg (		
	giant 	d,
	giant 	n)
{
	ghandle gdata;

	init_ghandle (&gdata);
	modgi (&gdata, d, n);
	term_ghandle (&gdata);
}

void modgi (		
	ghandle *gdata,	
	giant 	d,
	giant 	n)
{
	giant	tmp;

	ASSERTG (d->sign > 0);
	ASSERTG (d->n[d->sign-1] != 0);
	ASSERTG (n->sign == 0 || n->n[abs(n->sign)-1] != 0);

	if (n->sign >= 0 && (n->sign < d->sign || (n->sign == d->sign && gcompg (n, d) < 0))) return;

	tmp = popg (gdata, abs(n->sign));
	gtog (n, tmp);
	divgi (gdata, d, tmp);
	mulgi (gdata, d, tmp);
	subg (tmp, n);
	if (n->sign < 0) addg (d, n);
	pushg (gdata, 1);

	ASSERTG (n->sign >= 0);
	ASSERTG (n->sign == 0 || n->n[n->sign-1] != 0);
}

void dbldivg (			
	double	i,
	giant	g)
{
	stackgiant(tmp,2);

	dbltog (i, tmp);
	divg (tmp, g);
}

void divg (		
	giant 	d,
	giant 	n)
{
	ghandle gdata;

	init_ghandle (&gdata);
	divgi (&gdata, d, n);
	term_ghandle (&gdata);
}

void divgi (		
	ghandle *gdata,	
	giant 	d,
	giant 	n)
{
	int	nsign, nsize, dsize;
	giant	r;

	ASSERTG (d->sign > 0);
	ASSERTG (d->n[d->sign-1] != 0);
	ASSERTG (n->sign == 0 || n->n[abs(n->sign)-1] != 0);

	if (isone (d)) return;
	nsign = n->sign;
	nsize = abs (nsign);
	dsize = d->sign;
	if (nsize < dsize) {
		n->sign = 0;
		return;
	}

	if (nsize - dsize <= 1) {
		double	dflt, nflt, qflt;
		uint32_t q;

		nflt = n->n[nsize-1];
		if (nsize > 1) nflt = nflt * 4294967296.0 + n->n[nsize-2];
		if (nsize > 2) nflt = nflt * 4294967296.0 + n->n[nsize-3];
		if (nsize > dsize && nsize > 3) nflt *= 4294967296.0;
		dflt = d->n[dsize-1];
		if (dsize > 1) dflt = dflt * 4294967296.0 + d->n[dsize-2];
		if (dsize > 2) dflt = dflt * 4294967296.0 + d->n[dsize-3];

		qflt = nflt / dflt;
		if (qflt < 4294967295.0) {
			q = (uint32_t) qflt;
			qflt = qflt - (double) q;
			if (nsize == 1 ||	
			    (qflt >= TWO_TO_MINUS_19 &&
			     qflt <= ONE_MINUS_TWO_TO_MINUS_19)) {
				n->sign = (q == 0) ? 0 : 1;
				n->n[0] = q;
				if (nsign < 0) negg (n);
				return;
			}
		}
	}

	if (nsize > dsize + dsize) {
		giant	tmp;
		int	i, chunks;

		r = popg (gdata, dsize + dsize);
		tmp = popg (gdata, dsize + dsize);

		chunks = nsize / dsize - 1;

		if (nsign < 0) negg (n);

		n->sign -= chunks * dsize;
		n->n += chunks * dsize;

		gtog (n, r);
		divgi (gdata, d, n);

		gtog (n, tmp);
		mulgi (gdata, d, tmp);
		subg (tmp, r);

		for (i = 0; i < chunks; i++) {
			gshiftleft (dsize << 5, r);
			n->n -= dsize;
			n->sign += dsize;
			memmove (r->n, n->n, dsize * sizeof (uint32_t));
			if (r->sign == 0) {
				r->sign = dsize;
				while (r->sign && r->n[r->sign-1] == 0) r->sign--;
			}

			gtog (r, tmp);
			divgi (gdata, d, tmp);
			memset ((char *) n->n, 0, dsize * sizeof (uint32_t));
			memmove (n->n, tmp->n, tmp->sign * sizeof (uint32_t));

			mulgi (gdata, d, tmp);
			subg (tmp, r);
		}

		if (nsign < 0) negg (n);

		pushg (gdata, 2);
	}

	else if (d->sign < 30) {
		r = popg (gdata, (d->sign << 1) + 1);
		make_recip (gdata, d, r);
		divg_via_recip (gdata, d, r, n);
		pushg (gdata, 1);
	}
	else {
		if (gdata->cur_recip == NULL || gcompg (d, gdata->cur_den)) {
			free (gdata->cur_recip);
			free (gdata->cur_den);
			gdata->cur_recip = allocgiant (d->sign + 1);
			gdata->cur_den = allocgiant (d->sign);
			gtog (d, gdata->cur_den);
			make_recip (gdata, d, gdata->cur_recip);
		}
		divg_via_recip (gdata, d, gdata->cur_recip, n);
	}

	ASSERTG (n->sign == 0 || n->n[abs(n->sign)-1] != 0);
}

void powerg (			
	giant	x,
	giant	n)
{
	ghandle gdata;
	int 	len;
	giant	scratch;

	ASSERTG (x->sign > 0);

	init_ghandle (&gdata);

	scratch = popg (&gdata, x->sign << 1);
	gtog (x, scratch);
	for (len = bitlen (n) - 1; len; len--) {
		squaregi (&gdata, x);
		if (bitval (n, len-1)) mulgi (&gdata, scratch, x);
	}
	pushg (&gdata, 1);

	term_ghandle (&gdata);
}

void power (			
	giant	x,
	int 	n)
{
	giantstruct ng;

	ASSERTG (n >= 0);

	if (n == 0) {itog (1, x); return;}
	if (n == 1) return;

	if (istwo (x)) {
		itog (1, x);
		gshiftleft (n, x);
	} else {
		uintstackg (n, ng);
		powerg (x, &ng);
	}
}

void powermodg (		
	giant	x,
	giant	n,
	giant	g)
{
	ghandle gdata;
	int 	len;
	giant	scratch;

	ASSERTG (x->sign > 0);
	ASSERTG (g->sign > 0);

	init_ghandle (&gdata);

	scratch = popg (&gdata, g->sign << 1);
	gtog (x, scratch);
	for (len = bitlen (n) - 1; len; len--) {
		squaregi (&gdata, scratch);
		modgi (&gdata, g, scratch);
		if (bitval (n, len-1)) {
			mulgi (&gdata, x, scratch);
			modgi (&gdata, g, scratch);
		}
	}
	gtog (scratch, x);
	pushg (&gdata, 1);

	term_ghandle (&gdata);
}

void powermod (			
	giant	x,
	int 	n,
	giant 	g)
{
	giantstruct ng;

	ASSERTG (n >= 0);

	if (n == 0) {itog (1, x); return;}
	if (n == 1) {modg (g, x); return;}

	uintstackg (n, ng);
	powermodg (x, &ng, g);
}

int automulg (			
	ghandle *gdata,		
	giant	a,
	giant	b)
{
	int 	asize, bsize, stop_reason;

	ASSERTG (a->sign >= 0);
	ASSERTG (b->sign >= 0);
	ASSERTG (a->sign == 0 || a->n[a->sign-1] != 0);
	ASSERTG (b->sign == 0 || b->n[b->sign-1] != 0);

	asize = a->sign;
	bsize = b->sign;
	if (asize < KARAT_BREAK_MULT || bsize < KARAT_BREAK_MULT) {
		grammarmulg (gdata, a, b);
	}

	else if (asize + asize <= bsize + 2) {
		giant	d;

		d = popg (gdata, bsize);	
		if (d == NULL) return (GIANT_OUT_OF_MEMORY);
		gtogshiftright (asize << 5, b, d);
		b->sign = asize;		
		while (b->sign && b->n[b->sign-1] == 0) b->sign--;

		stop_reason = automulg (gdata, a, d);	
		if (stop_reason) goto done;
		if (b->sign) {
			stop_reason = automulg (gdata, a, b);
			if (stop_reason) goto done;
		} else {
			memset (b->n, 0, asize * sizeof (uint32_t));
			b->sign = asize;
		}

		b->sign -= asize;	
		b->n += asize;
		normal_addg (d, b);
		b->sign += asize;	
		b->n -= asize;

done:		pushg (gdata, 1);
		return (stop_reason);
	}

	else if ((asize < FFT_BREAK_MULT1 && bsize < FFT_BREAK_MULT1) ||
	         (asize >= FFT_BREAK_MULT2 && bsize >= FFT_BREAK_MULT2 &&
	          asize < FFT_BREAK_MULT3 && bsize < FFT_BREAK_MULT3))
		karatmulg (gdata, a, b);
	else {
		stop_reason = FFTmulg (gdata, a, b);
		if (stop_reason) return (stop_reason);
	}

	ASSERTG (b->sign != 0 && b->n[abs(b->sign)-1] != 0);
	return (0);
}

void grammarsquareg (		
	ghandle *gdata,		
	giant	a)
{
	int	i, asize = a->sign, max = asize * 2 - 1;
	uint32_t *ptr, *ptr1, *ptr2;
	giant	scratch;
	uint32_t res, carryl, carryh;

	ASSERTG (a->sign >= 0);
	ASSERTG (a->sign == 0 || a->n[a->sign-1] != 0);

	if (asize == 0) return;

	scratch = popg (gdata, asize);
	gtog (a, scratch);
	ptr = scratch->n;

	asize--;
	res = carryl = carryh = 0;
	for (i = 0; i < max; i++) {
		if (i <= asize) {
			ptr1 = ptr;
			ptr2 = ptr + i;
		} else {
			ptr1 = ptr + i - asize;
			ptr2 = ptr + asize;
		}
		while (ptr1 < ptr2) {
			muladd2hlp (&res, &carryl, &carryh, *ptr1, *ptr2);
			ptr1++;
			ptr2--;
		}
		if (ptr1 == ptr2) {
			muladdhlp (&res, &carryl, &carryh, *ptr1, *ptr1);
		}
		a->n[i] = res;
		res = carryl;
		carryl = carryh;
		carryh = 0;
	}
	if (res) {
		a->n[i] = res;
		a->sign = i+1;
	} else
		a->sign = i;

	pushg (gdata, 1);
	ASSERTG (a->sign == 0 || a->n[a->sign-1] != 0);
}

void grammarmulg (		
	ghandle *gdata,		
	giant	a,
	giant	b)
{
	int 	i, max;
	int 	asize = a->sign, bsize = b->sign;
	uint32_t *aptr, *bptr, *destptr;
	giant	scratch;
	uint32_t res, carryl, carryh;

	ASSERTG (a->sign >= 0);
	ASSERTG (a->sign == 0 || a->n[a->sign-1] != 0);
	ASSERTG (b->sign >= 0);
	ASSERTG (b->sign == 0 || b->n[b->sign-1] != 0);

	if (bsize == 0) return;
	if (asize == 0) {b->sign = 0; return;}
	if (asize == 1 && a->n[0] == 1) return;

	scratch = popg (gdata, bsize);
	gtog (b, scratch);

	destptr = b->n;
	max = asize + bsize - 1;
	bsize--;
	res = carryl = carryh = 0;
	for (i = 0; i < max; i++) {
		if (i <= bsize) {
			aptr = a->n;
			bptr = scratch->n + i;
		} else {
			aptr = a->n + i - bsize;
			bptr = scratch->n + bsize;
		}
		while (aptr < a->n + asize && bptr >= scratch->n) {
			muladdhlp (&res, &carryl, &carryh, *aptr, *bptr);
			aptr++;
			bptr--;
		}
		*destptr++ = res;
		res = carryl;
		carryl = carryh;
		carryh = 0;
	}
	if (res) {
		*destptr++ = res;
		b->sign = i+1;
	} else
		b->sign = i;

	pushg (gdata, 1);
	ASSERTG (b->sign == 0 || b->n[b->sign-1] != 0);
}

void karatsquareg (		
	ghandle *gdata,		
	giant	x)
{
	int	s = x->sign, w;
 	giantstruct a, b;
	giant	c;

	ASSERTG (x->sign >= 0);
	ASSERTG (x->sign == 0 || x->n[x->sign-1] != 0);

	w = (s + 1) / 2;

	a.sign = w;		
	a.n = x->n;
	setmaxsize (&a, w);
	while (a.sign && a.n[a.sign-1] == 0) a.sign--;

	b.sign = s - w;		
	b.n = x->n + w + w;
	setmaxsize (&b, b.sign);
	memmove (b.n, a.n + w, b.sign * sizeof (uint32_t));

	c = popg (gdata, w + w + 3);	
	gtog (&a, c);
	normal_addg (&b, c);

	if (a.sign < KARAT_BREAK_SQUARE)
		grammarsquareg (gdata, &a);	
	else
		karatsquareg (gdata, &a);	
	if (b.sign < KARAT_BREAK_SQUARE)
		grammarsquareg (gdata, &b);
	else
		karatsquareg (gdata, &b);
	if (c->sign < KARAT_BREAK_SQUARE)
		grammarsquareg (gdata, c);
	else
		karatsquareg (gdata, c);

	subg (&a, c);		
	subg (&b, c);

	while (a.sign < w + w)	
		a.n[a.sign++] = 0;

	x->sign = b.sign + w;	
	x->n = a.n + w;
	normal_addg (c, x);
	x->sign += w;		
	x->n = a.n;

	pushg (gdata, 1);
	ASSERTG (x->sign == 0 || x->n[x->sign-1] != 0);
}

void karatmulg (		
	ghandle *gdata,		
	giant	x,
	giant	y)
{
	int	s = x->sign, t = y->sign, w;
	giantstruct a, b, c;
	giant	d, e, f;

	ASSERTG (x->sign >= 0);
	ASSERTG (x->sign == 0 || x->n[x->sign-1] != 0);
	ASSERTG (y->sign >= 0);
	ASSERTG (y->sign == 0 || y->n[y->sign-1] != 0);

	w = (s + t + 2) / 4;

	a.sign = (s < w) ? s : w;	
	a.n = x->n;
	setmaxsize (&a, a.sign);
	while (a.sign && a.n[a.sign-1] == 0) a.sign--;

	b.sign = (s > w) ? s - w : 0;	
	b.n = a.n + w;
	setmaxsize (&b, b.sign);

	c.sign = (t < w) ? t : w;	
	c.n = y->n;
	setmaxsize (&c, y->maxsize);
	while (c.sign && c.n[c.sign-1] == 0) c.sign--;

	d = popg (gdata, s + t);	
	d->sign = (t > w) ? t - w : 0;
	memmove (d->n, c.n + w, d->sign * sizeof (uint32_t));

	e = popg (gdata, s + t);	
	gtog (&a, e); normal_addg (&b, e);

	f = popg (gdata, s + t + 1);	
	gtog (&c, f); normal_addg (d, f);

	if (a.sign < KARAT_BREAK_MULT || c.sign < KARAT_BREAK_MULT)
		grammarmulg (gdata, &a, &c);	
	else
		karatmulg (gdata, &a, &c);	
	if (b.sign < KARAT_BREAK_MULT || d->sign < KARAT_BREAK_MULT)
		grammarmulg (gdata, &b, d);	
	else
		karatmulg (gdata, &b, d);	
	if (e->sign < KARAT_BREAK_MULT || f->sign < KARAT_BREAK_MULT)
		grammarmulg (gdata, e, f);	
	else
		karatmulg (gdata, e, f);	

	normal_subg (&c, f);		
	normal_subg (d, f);

	if (d->sign) {
		memmove (c.n + w + w, d->n, d->sign * sizeof (uint32_t)); 
		while (c.sign < w + w)	
			c.n[c.sign++] = 0;
		y->sign = d->sign + w;	
	} else {
		while (c.sign < w)	
			c.n[c.sign++] = 0;
		y->sign = c.sign - w;	
	}
	y->n = c.n + w;
	normal_addg (f, y);
	y->sign += w;			
	y->n = c.n;

	pushg (gdata, 3);
	ASSERTG (y->sign == 0 || y->n[y->sign-1] != 0);
	ASSERTG (y->sign <= y->maxsize);
}

void gmaskbits (	
	int	bits,
	giant	g)
{
	int 	rem = bits&31, words = bits>>5;
	int 	size = abs (g->sign);

	ASSERTG (bits >= 0);
	ASSERTG (g->sign >= 0 || g->n[abs(g->sign)-1] != 0);

	if (size <= words) return;

	if (rem) {
		g->n[words] &= ((1 << rem) - 1);
		words++;
	}
	g->sign = (g->sign < 0) ? -words : words;
	while (g->sign && g->n[g->sign-1] == 0) g->sign--;
}

void gshiftleft (	
	int	bits,
	giant	g)
{
	int 	rem = bits&31, crem = 32-rem, words = bits>>5;
	int 	size = abs (g->sign), j;
	uint32_t carry;

	ASSERTG (bits >= 0);
	ASSERTG (g->sign == 0 || g->n[abs(g->sign)-1] != 0);

	if (bits == 0) return;
	if (size == 0) return;

	if (rem == 0) {
		memmove (g->n + words, g->n, size * sizeof (uint32_t));
		memset (g->n, 0, words * sizeof (uint32_t));
		g->sign += (g->sign < 0) ? -words : words;
		return;
	}
	carry = g->n[size-1] >> crem;
	for (j = size-1; j > 0; j--) {
		g->n[j+words] = (g->n[j] << rem) | (g->n[j-1] >> crem);
	}
	g->n[words] = g->n[0] << rem;
	memset (g->n, 0, words * sizeof (uint32_t));
	size = size + words;
	if (carry) g->n[size++] = carry;
	g->sign = (g->sign < 0) ? -size : size;

	ASSERTG (g->sign == 0 || g->n[abs(g->sign)-1] != 0);
}

void gtogshiftright (	
	int	bits,
	giant	src,
	giant	dest)
{
	register int j, size = abs (src->sign);
	register uint32_t carry, *sptr, *dptr;
	int 	words = bits >> 5;
	int 	remain = bits & 31, cremain = (32-remain);

	ASSERTG (bits >= 0);
	ASSERTG (bits || src != dest);
	ASSERTG (src->sign == 0 || src->n[abs(src->sign)-1] != 0);

	if (words >= size) {
		dest->sign = 0;
		return;
	}
	if (remain == 0) {
		memmove (dest->n, src->n + words, (size - words) * sizeof (uint32_t));
		dest->sign = src->sign + (src->sign < 0 ? words : -words);
		return;
	}

	size -= words;
	sptr = src->n + words;
	dptr = dest->n;
	for (j = 0; j < size-1; j++, sptr++, dptr++) {
		carry = sptr[1] << cremain;
		*dptr = (sptr[0] >> remain) | carry;
	}
	*dptr = *sptr >> remain;
	if (*dptr == 0) size--;
	dest->sign = (src->sign > 0) ? size : -size;

	ASSERTG (dest->sign == 0 || dest->n[abs(dest->sign)-1] != 0);
}

void gtogshiftrightsplit (
	int	bits,
	giant	src,			
	giant	dest,
	int64_t	accum)			
{
	int	src_size = src->sign;
	int 	word = bits >> 5;
	int 	bit = bits & 31;
	uint32_t carry, *sptr, *dptr;

	ASSERTG (bits > 0);
	ASSERTG (src != dest);
	ASSERTG (src->sign >= 0 && (src->sign == 0 || src->n[src->sign-1] != 0));

	if (word >= src_size) {
		
		if (accum == -1) {
			for (int i = 0; i < src_size; i++) src->n[i] = ~src->n[i];
			while (src->sign && src->n[src->sign-1] == 0) src->sign--;
			iaddg (1, src);
			src->sign = -src->sign;
		}
		dest->sign = 0;
		return;
	}

	sptr = src->n + word;
	dptr = dest->n;

	if (bit) {
		carry = *sptr >> bit;
		*sptr -= (carry << bit);
		word++;
		sptr++;
	}
	src->sign = word;

	for ( ; word < src_size; word++, sptr++, dptr++) {
		if (bit == 0) *dptr = *sptr;
		else *dptr = (*sptr << (32 - bit)) + carry, carry = *sptr >> bit;
		if (accum == -1) *dptr = ~*dptr;
	}

	if (bit) {
		if (accum == 0) *dptr = carry;
		else *dptr = (~(carry << bit)) >> bit;		
		dptr++;
	}

	while (src->sign && src->n[src->sign-1] == 0) src->sign--;

	dest->sign = (int) (dptr - dest->n);
	while (dest->sign && dest->n[dest->sign-1] == 0) dest->sign--;
	if (accum == -1) iaddg (1, dest), dest->sign = -dest->sign;

	ASSERTG (src->sign == 0 || src->n[src->sign-1] != 0);
	ASSERTG (dest->sign == 0 || dest->n[abs(dest->sign)-1] != 0);
}

void gtogshiftleftunsplit (
	int	bits,
	giant	src,			
	giant	dest)			
{
	int	src_size = src->sign;
	int	dest_size = dest->sign;
	int 	word = bits >> 5;
	int 	bit = bits & 31;
	uint32_t carry, *sptr, *dptr;

	ASSERTG (bits > 0);
	ASSERTG (src != dest);
	ASSERTG (src->sign >= 0 && (src->sign == 0 || src->n[src->sign-1] != 0));
	ASSERTG (dest->sign >= 0 && (dest->sign == 0 || dest->n[dest->sign-1] != 0));

	if (src_size == 0) return;

	if (word > dest_size) memset (dest->n + dest_size, 0, (word - dest_size) * sizeof (uint32_t));

	sptr = src->n;
	dptr = dest->n + word;
	carry = (bit == 0) ? 0 : *dptr;
	for ( ; src_size; src_size--, sptr++, dptr++) {
		*dptr = (*sptr << bit) + carry;
		carry = (bit == 0) ? 0 : *sptr >> (32 - bit);
	}
	if (carry) *dptr++ = carry;

	dest->sign = (int) (dptr - dest->n);
	while (dest->sign && dest->n[dest->sign-1] == 0) dest->sign--;

	ASSERTG (dest->sign == 0 || dest->n[dest->sign-1] != 0);
}

int invg (		
			
	giant 	xx,
	giant 	yy)
{
	ghandle gdata;
	int	stop_reason;

	init_ghandle (&gdata);
	stop_reason = invg_common (&gdata, xx, yy, 0);
	term_ghandle (&gdata);
	return (stop_reason);
}

int invgi (		
	ghandle *gdata,	
	int	thread_num,
	giant 	xx,
	giant 	yy)
{
	return (invg_common (gdata, xx, yy, 0x80000000 + thread_num));
}

int invg_common (	
	ghandle *gdata,	
	giant 	xx,
	giant 	yy,
	int	interruptable)
{
	giant	x, y;
	gmatrixstruct A;
	giantstruct ul, ll;
	int	ss, stop_reason;

	ASSERTG (xx->sign > 0 && xx->n[xx->sign-1] != 0);
	ASSERTG (yy->sign != 0 && yy->n[abs(yy->sign)-1] != 0);

	ss = stackstart (gdata);
	x = popg (gdata, xx->sign);
	if (x == NULL) { stop_reason = GIANT_OUT_OF_MEMORY; goto done; }
	gtog (xx, x);
	y = yy;

	while (y->sign < 0) addg (x, y);
	modgi (gdata, x, y);

	A.ur = popg (gdata, x->sign + 1);			
	if (A.ur == NULL) { stop_reason = GIANT_OUT_OF_MEMORY; goto done; }
	setzero (A.ur);
	A.lr = popg (gdata, x->sign + 1);			
	if (A.lr == NULL) { stop_reason = GIANT_OUT_OF_MEMORY; goto done; }
	setone (A.lr);
	if (y->sign <= GCDLIMIT) {
		A.ul = &ul; setzero (A.ul); setmaxsize (A.ul, 1);
		A.ll = &ll; setzero (A.ll); setmaxsize (A.ll, 1);
		stop_reason = cextgcdg (gdata, &x, &y, &A, interruptable);
		if (stop_reason) goto done;
	} else {
		A.ul = popg (gdata, x->sign);
		if (A.ul == NULL) { stop_reason = GIANT_OUT_OF_MEMORY; goto done; }
		setone (A.ul);
		A.ll = popg (gdata, x->sign);
		if (A.ll == NULL) { stop_reason = GIANT_OUT_OF_MEMORY; goto done; }
		setzero (A.ll);
		stop_reason = ggcd (gdata, &x, &y, &A, interruptable);
		if (stop_reason) goto done;
		pushg (gdata, 2);
	}

	if (! isone (x)) {
		if (x != yy) gtog (x, yy);
		yy->sign = -yy->sign;
		ASSERTG (yy->sign < 0 && yy->n[-yy->sign-1] != 0);
	}

	else {
		gtog (A.ur, yy);
		if (A.ur->sign < 0) addg (xx, yy);
		ASSERTG (yy->sign > 0 && yy->n[yy->sign-1] != 0);
	}

done:	pushall (gdata, ss);
	return (stop_reason);
}

int gcdhlp_wrapper (
	ghandle *gdata,		
	giant	u,
	giant	v,
	gmatrix	A)
{
	int	i;
	uint32_t ures, ucarryl, ucarryh;
	uint32_t vres, vcarryl, vcarryh;
	struct {
		uint32_t A;
		uint32_t B;
		uint32_t C;
		uint32_t D;
		uint32_t ODD;
	} ret;

	if (!gcdhlp (u->sign, u->n, v->sign, v->n, &ret)) return (FALSE);
	if (A != NULL) {
		gmatrixstruct B;
		giantstruct ul, ur, ll, lr;
		setmaxsize (&ul, 1); setmaxsize (&ur, 1);
		setmaxsize (&ll, 1); setmaxsize (&lr, 1);
		B.ul = &ul; B.ur = &ur;
		B.ll = &ll; B.lr = &lr;
		ul.sign = ret.A ? 1 : 0; ul.n = &ret.A;
		ur.sign = ret.B ? 1 : 0; ur.n = &ret.B;
		ll.sign = ret.C ? 1 : 0; ll.n = &ret.C;
		lr.sign = ret.D ? 1 : 0; lr.n = &ret.D;
		if (ret.ODD) {
			ul.sign = -ul.sign;
			lr.sign = -lr.sign;
		} else {
			ur.sign = -ur.sign;
			ll.sign = -ll.sign;
		}
		mulmM (gdata, &B, A);
	}

	ures = ucarryl = ucarryh = 0;
	vres = vcarryl = vcarryh = 0;
	for (i = 0; i < v->sign; i++) {
		if (ret.ODD) {
			mulsubhlp (&ures, &ucarryl, &ucarryh, u->n[i], ret.A);
			muladdhlp (&ures, &ucarryl, &ucarryh, v->n[i], ret.B);
			muladdhlp (&vres, &vcarryl, &vcarryh, u->n[i], ret.C);
			mulsubhlp (&vres, &vcarryl, &vcarryh, v->n[i], ret.D);
		} else {
			muladdhlp (&ures, &ucarryl, &ucarryh, u->n[i], ret.A);
			mulsubhlp (&ures, &ucarryl, &ucarryh, v->n[i], ret.B);
			mulsubhlp (&vres, &vcarryl, &vcarryh, u->n[i], ret.C);
			muladdhlp (&vres, &vcarryl, &vcarryh, v->n[i], ret.D);
		}
		u->n[i] = ures;
		ures = ucarryl;
		ucarryl = ucarryh;
		ucarryh = (int) ucarryh >> 4;	
		v->n[i] = vres;
		vres = vcarryl;
		vcarryl = vcarryh;
		vcarryh = (int) vcarryh >> 4;	
	}
	u->sign = v->sign;
	while (u->sign > 0 && u->n[u->sign-1] == 0) u->sign--;
	while (v->sign > 0 && v->n[v->sign-1] == 0) v->sign--;
	ASSERTG (u->sign > v->sign ||
		 (u->sign == v->sign && u->n[u->sign-1] >= v->n[v->sign-1]));
	return (TRUE);
}

int gcdg (	
		
	giant	xx,
	giant	yy)
{
	ghandle gdata;
	int	stop_reason;

	init_ghandle (&gdata);
	stop_reason = gcdg_common (&gdata, xx, yy, 0);
	term_ghandle (&gdata);
	return (stop_reason);
}

int gcdgi (			
	ghandle *gdata,		
	int	thread_num,
	giant	xx,
	giant	yy)
{
	return (gcdg_common (gdata, xx, yy, 0x80000000 + thread_num));
}

int gcdg_common (		
	ghandle *gdata,		
	giant	xx,
	giant	yy,
	int	interruptable)
{
	giant	x, y;
	int	ss, stop_reason;

	ASSERTG (xx->sign > 0 && xx->n[xx->sign-1] != 0);
	ASSERTG (yy->sign > 0 && yy->n[yy->sign-1] != 0);

	ss = stackstart (gdata);
	x = popg (gdata, xx->sign + 1);
	if (x == NULL) { stop_reason = GIANT_OUT_OF_MEMORY; goto done; }
	gtog (xx, x);
	y = yy;

	if (gcompg (x, y) < 0) gswap (&x, &y);

	if (abs (y->sign) <= GCDLIMIT) {
		stop_reason = cextgcdg (gdata, &x, &y, NULL, interruptable);
		if (stop_reason) goto done;
	} else {
		stop_reason = ggcd (gdata, &x, &y, NULL, interruptable);
		if (stop_reason) goto done;
	}

	if (x != yy) gtog (x, yy);

	ASSERTG (yy->sign > 0 && yy->n[yy->sign-1] != 0);
done:	pushall (gdata, ss);
	return (stop_reason);
}

int cextgcdg (		
	ghandle *gdata,	
	giant 	*a,
	giant 	*b,
	gmatrix	A,
	int	interruptable)
{
	giant	u, v;
	int	stop_reason;

	ASSERTG ((*a)->sign > 0);
	ASSERTG ((*b)->sign >= 0);

	u = *a;
	v = *b;

	while (v->sign > 0) {
		unsigned int diff_length;

		if (interruptable && StopCheckRoutine != NULL) {
			stop_reason = stopCheck (interruptable);
			if (stop_reason) return (stop_reason);
		}

		diff_length = u->sign - v->sign;
		if (diff_length > 1 ||
		    (diff_length == 1 && u->n[u->sign-1] >= v->n[v->sign-1]) ||
		    ! gcdhlp_wrapper (gdata, u, v, A))
			onestep (gdata, &u, &v, A);
	}

	*a = u;
	*b = v;

	return (0);
}

void init_ghandle (
	ghandle *gdata)
{
	memset (gdata, 0, sizeof (ghandle));
}

void term_ghandle (
	ghandle *gdata)
{
	aligned_free (gdata->ooura_sincos);
	free (gdata->ooura_ip);
	free (gdata->cur_recip);
	free (gdata->cur_den);
}

giant popg (
	ghandle *gdata,		
	int	size)		
{
	giant	g;
	gstacknode *s;
	unsigned long memsize;

	ASSERTG (size >= 0);

	memsize = sizeof (gstacknode) + sizeof (giantstruct) + size * sizeof (uint32_t);
	if (memsize > gdata->blksize) {
		s = (gstacknode *) malloc (memsize);
		memmove (s, &gdata->stack, sizeof (gstacknode));
	} else if (gdata->stack.memblk == NULL ||
		   memsize > gdata->blksize - gdata->stack.offset) {
		s = (gstacknode *) ((*gdata->allocate) (gdata->handle));
		memmove (s, &gdata->stack, sizeof (gstacknode));
		gdata->stack.memblk = s;
		gdata->stack.offset = memsize;
	} else {
		s = (gstacknode *) ((char *) gdata->stack.memblk + gdata->stack.offset);
		memmove (s, &gdata->stack, sizeof (gstacknode));
		gdata->stack.offset += memsize;
	}
	gdata->num_popgs++;

	g = (giant) ((char *) s + sizeof (gstacknode));
	g->n = (uint32_t *) ((char *) g + sizeof (giantstruct));
	setmaxsize (g, size);

	gdata->stack.prev = g;
	return (g);
}

void pushg (
	ghandle *gdata,		
	int	a)
{
	gstacknode *s;

	while (a--) {
		s = (gstacknode *) ((char *) gdata->stack.prev - sizeof (gstacknode));
		if (gdata->stack.memblk == s->memblk &&
		    gdata->stack.offset == s->offset) {
			memmove (&gdata->stack, s, sizeof (gstacknode));
			free (s);
		} 
		else if (gdata->stack.memblk != s->memblk) {
			memmove (&gdata->stack, s, sizeof (gstacknode));
			(*gdata->free)(gdata->handle, s);
		} 
		else {
			memmove (&gdata->stack, s, sizeof (gstacknode));
		}
		gdata->num_popgs--;
	}
}

int bitlen (
	giant	g)
{
	int 	b = 32, c = 1<<31, w, size;

	ASSERTG (g->sign == 0 || g->n[abs(g->sign)-1] != 0);

	size = abs (g->sign);
	if (size == 0) return (0);
	w = g->n[size - 1];
	while ((w&c) == 0) {
		b--;
		c >>= 1;
	}
	return (32 * (size-1) + b);
}

int gsign (		
	giant 	g)
{
	if (isZero(g)) return (0);
	if (g->sign > 0) return (1);
	return (-1);
}

void make_recip (	
	ghandle *gdata,	
	giant 	d, 
	giant 	r)
{
	int	b;
	giant 	tmp, tmp2;

	ASSERTG (d->sign > 0);

	tmp = popg (gdata, (d->sign << 1) + 1);
	tmp2 = popg (gdata, (d->sign << 1) + 1);
	setone (r);
	normal_subg (r, d);
	b = bitlen (d);
	normal_addg (r, d);
	gshiftleft (b, r);
	do {
		gtog (r, tmp2);
		gtog (r, tmp);
		squaregi (gdata, tmp);
		gshiftright (b, tmp);
		mulgi (gdata, d, tmp);
		gshiftright (b, tmp);
		addg (r, r); 
		subg (tmp, r);
	} while (gcompg (r, tmp2) > 0);
	setone (tmp);
	gshiftleft (2*b, tmp);
	gtog (r, tmp2); 
	mulgi (gdata, d, tmp2);
	subg (tmp2, tmp);
	while (tmp->sign < 0) {
		ulsubg (1, r);
		addg (d, tmp);
	}
	pushg (gdata, 2);
}

void divg_via_recip (		
	ghandle *gdata,		
	giant 	d, 
	giant 	r, 
	giant 	n)
{
	int 	s = 2*(bitlen(r)-1), sign = gsign(n);
	giant 	tmp, tmp2;

	ASSERTG (d->sign > 0);

	tmp = popg (gdata, abs (n->sign) + r->sign);
	tmp2 = popg (gdata, abs (n->sign) + r->sign);

	n->sign = abs (n->sign);
	setzero (tmp2);
	while (1) {
		gtog (n, tmp);	
		mulgi (gdata, r, tmp);
		gshiftright (s, tmp);
		addg (tmp, tmp2);
		mulgi (gdata, d, tmp);
		subg (tmp, n);
		if (gcompg (n,d) >= 0) {
			subg (d,n);
			iaddg (1, tmp2);
		}
		if (gcompg (n,d) < 0) break;
	}
	gtog (tmp2, n);
	n->sign *= sign;
	pushg (gdata, 2);
}

int lpt (		
	int	n)
{
	register int	i = 1;

	while (i < n) i <<= 1;
	return (i);
}

void makewt (int nw, int *ip, double *w);
void makect (int nc, int *ip, double *c);

int init_sincos (
	ghandle	*gdata,
	int 	n)
{
	if (n <= gdata->ooura_fft_size) return (0);

	aligned_free (gdata->ooura_sincos);
	gdata->ooura_sincos = NULL;
	free (gdata->ooura_ip);
	gdata->ooura_ip = NULL;
	gdata->ooura_fft_size = 0;

	if (gdata->deallocate != NULL)
		(*gdata->deallocate) (gdata->handle);

	gdata->ooura_sincos = (double *) aligned_malloc ((n/2) * sizeof (double), sizeof (double));
	if (gdata->ooura_sincos == NULL) return (GIANT_OUT_OF_MEMORY);
	gdata->ooura_ip = (int *) malloc (((int) sqrt((double)(n/2)) + 2) * sizeof (int));
	if (gdata->ooura_ip == NULL) {
		aligned_free (gdata->ooura_sincos);
		gdata->ooura_sincos = NULL;
		return (GIANT_OUT_OF_MEMORY);
	}

	gdata->ooura_ip[0] = 0;
	gdata->ooura_fft_size = n;
	makewt (n >> 2, gdata->ooura_ip, gdata->ooura_sincos);
	makect (n >> 2, gdata->ooura_ip, gdata->ooura_sincos + (n >> 2));
	return (0);
}

void mp_squ_cmul (int nfft, double dinout[])
{
	int j;
	double xr, xi;
    
	dinout[0] *= dinout[0];
	dinout[1] *= dinout[1];
	for (j = 2; j < nfft; j += 2) {
		xr = dinout[j];
		xi = dinout[j + 1];
		dinout[j] = xr * xr - xi * xi;
		dinout[j + 1] = xr * xi * 2.0;
	}
}

void mp_mul_cmul (int nfft, double din[], double dinout[])
{
	int j;
	double xr, xi, yr, yi;
    
	dinout[0] *= din[0];
	dinout[1] *= din[1];
	for (j = 2; j < nfft; j += 2) {
		xr = din[j];
		xi = din[j + 1];
		yr = dinout[j];
		yi = dinout[j + 1];
		dinout[j] = xr * yr - xi * yi;
		dinout[j + 1] = xr * yi + xi * yr;
	}
}

void addsignal (
	giant	x,
	int	size,
	double 	*z,
	int 	n)
{
#define TWO24		((double)(16777216.0))
#define TWO24PLUSHALF	((double)(16777216.5))
#define TWOM24		(double)(0.000000059604644775390625)
	register int	j, m1, m2, value, carry;
	register double scale, f, fltcarry;

	scale = 2.0 / (double) n;

	carry = 0;
	fltcarry = 0.0;
	for (j = 0; ; ) {
		f = z[j+j] * scale + fltcarry * 256.0;
		fltcarry = (double) ((int) (f * TWOM24));
		f = f - fltcarry * TWO24;
		m1 = (int) (f + TWO24PLUSHALF) + carry;
		carry = (m1 >> 16) - 256;

		f = z[j+j+1] * scale + fltcarry * 256.0;
		fltcarry = (double) ((int) (f * TWOM24));
		f = f - fltcarry * TWO24;
		m2 = (int) (f + TWO24PLUSHALF) + carry;
		carry = (m2 >> 16) - 256;

		value = ((m2 & 0xFFFF) << 16) + (m1 & 0xFFFF);
		if (++j == size) break;
		x->n[j-1] = value;
	}
	if (value == 0) j--;
	else x->n[j-1] = value;
	x->sign = j;
	ASSERTG (x->n[x->sign-1] != 0);
}

int FFTsquareg (
	ghandle *gdata,	
	giant	x)
{
	int	size = x->sign;
	double	*z1;
	int 	ss, L, stop_reason;

	ASSERTG (x->sign >= 4 && x->n[x->sign-1] != 0);

	ss = stackstart (gdata);

	L = lpt (size+size) << 1;
	stop_reason = init_sincos (gdata, L);
	if (stop_reason) goto done;

	z1 = (double *) popg (gdata, (L+1) * sizeof (double) / sizeof (uint32_t));
	if (z1 == NULL) { stop_reason = GIANT_OUT_OF_MEMORY; goto done; }
	z1 = align_ptr (z1, sizeof (double));

	giant_to_double (x, size, z1, L);
	rdft (L, 1, z1, gdata->ooura_ip, gdata->ooura_sincos);
	mp_squ_cmul (L, z1);
	rdft (L, -1, z1, gdata->ooura_ip, gdata->ooura_sincos);
	addsignal (x, size+size, z1, L);

done:	pushall (gdata, ss);
	return (stop_reason);
}

int FFTmulg (			
	ghandle *gdata,		
	giant	y,
	giant	x)
{
	int	sizex = x->sign, sizey = y->sign;
	double	*z1, *z2;
	int	ss, L, stop_reason;

	ASSERTG (y->sign >= 4 && y->n[y->sign-1] != 0);
	ASSERTG (x->sign >= 4 && x->n[x->sign-1] != 0);

	ss = stackstart (gdata);

	L = lpt (sizex+sizey) << 1;
	stop_reason = init_sincos (gdata, L);
	if (stop_reason) goto done;

	z1 = (double *) popg (gdata, (L+1) * sizeof (double) / sizeof (uint32_t));
	if (z1 == NULL) { stop_reason = GIANT_OUT_OF_MEMORY; goto done; }
	z1 = align_ptr (z1, sizeof (double));

	z2 = (double *) popg (gdata, (L+1) * sizeof (double) / sizeof (uint32_t));
	if (z2 == NULL) { stop_reason = GIANT_OUT_OF_MEMORY; goto done; }
	z2 = align_ptr (z2, sizeof (double));

	giant_to_double (x, sizex, z1, L);
	giant_to_double (y, sizey, z2, L);
	rdft (L, 1, z1, gdata->ooura_ip, gdata->ooura_sincos);
	rdft (L, 1, z2, gdata->ooura_ip, gdata->ooura_sincos);
	mp_mul_cmul (L, z2, z1);
	rdft (L, -1, z1, gdata->ooura_ip, gdata->ooura_sincos);
	addsignal (x, sizex+sizey, z1, L);

	ASSERTG (y->sign > 0 && y->n[y->sign-1] != 0);
done:	pushall (gdata, ss);
	return (stop_reason);
}

void giant_to_double (
	giant 	x,
	int 	sizex,
	double 	*z,
	int 	L)
{
	register int j, w, carry;

	carry = 0;
	for (j = 0; j < sizex; j++) {
		w = (x->n[j] & 0xFFFF) + carry;
		if (w < 32768) {
			z[j+j] = w;
			carry = 0;
		} else {
			z[j+j] = w - 65536;
			carry = 1;
		}
		w = (x->n[j] >> 16) + carry;
		if (w < 32768) {
			z[j+j+1] = w;
			carry = 0;
		} else {
			z[j+j+1] = w - 65536;
			carry = 1;
		}
	}
	if (carry) z[j+j-1] += 65536;
	for (j = sizex + sizex; j < L; j++) {
		z[j] = 0.0;
	}
}

void gsetlength (	
	int	n,
	giant	g)
{
	int	size;

	ASSERTG (g->sign >= (n >> 5));

	size = (n + 31) >> 5;
	if (n & 31) g->n[size-1] &= (1 << (n & 31)) - 1;
	while (size && g->n[size-1] == 0) size--;
	g->sign = size;
}

void addshiftedg (	
			
	int	n,
	giant	x,	
	giant	g)
{
	ASSERTG (n >= 0 && x->sign >= 0);

	if (x->sign == 0) return;

	if (g->sign >= 0) {
		while (g->sign < n) g->n[g->sign++] = 0;
		g->sign -= n;
		g->n += n;
		normal_addg (x, g);
		g->sign += n;
		g->n -= n;
		while (g->sign && g->n[g->sign-1] == 0) g->sign--;
		return;
	}

	negg (g);
	if (g->sign < n) {
		reverse_subg (x, g, n);
		return;
	} 

	g->sign -= n;
	g->n += n;
	if (gcompg (g, x) >= 0) {
		normal_subg (x, g);
		g->sign += n;
		g->n -= n;
		while (g->sign && g->n[g->sign-1] == 0) g->sign--;
		negg (g);
	} else {
		g->sign += n;
		g->n -= n;
		reverse_subg (x, g, n);
	}
}

void onestep (		
	ghandle *gdata,	
	giant 	*x,
	giant 	*y,
	gmatrix A)
{
	giant q = popg (gdata, (*x)->sign);

	ASSERTG ((*x)->sign >= (*y)->sign);

	gtog (*x, q);		 
	divgi (gdata, *y, q);
	if (A != NULL) punch (gdata, q, A);
	mulgi (gdata, *y, q);		
	subg (q, *x);
	gswap (x, y);
	pushg (gdata, 1);
}

int mulvM (		
			
	ghandle *gdata,	
	gmatrix A,
	giant 	x,
	giant 	y)
{
	giant	s0, s1;
	int	ss, stop_reason;

	ss = stackstart (gdata);
	s0 = popg (gdata, abs(A->ll->sign) + x->sign);
	if (s0 == NULL) { stop_reason = GIANT_OUT_OF_MEMORY; goto done; }
	s1 = popg (gdata, abs(A->ur->sign) + y->sign);
	if (s1 == NULL) { stop_reason = GIANT_OUT_OF_MEMORY; goto done; }

	gtog (x, s0);
	gtog (y, s1);
	stop_reason = mulgi (gdata, A->ul, x);
	if (stop_reason) goto done;
	mulgi (gdata, A->ur, s1);
	if (stop_reason) goto done;
	addg (s1, x);

	stop_reason = mulgi (gdata, A->lr, y);
	if (stop_reason) goto done;
	stop_reason = mulgi (gdata, A->ll, s0);
	if (stop_reason) goto done;
	addg (s0, y);

done:	pushall (gdata, ss);
	return (stop_reason);
}

int mulmM (		
	ghandle *gdata,	
	gmatrix A,
	gmatrix B)
{
	giant	s0, s1;
	int	ss, stop_reason;

	ss = stackstart (gdata);
	s0 = popg (gdata, abs(A->ll->sign) + abs(B->ur->sign));
	if (s0 == NULL) { stop_reason = GIANT_OUT_OF_MEMORY; goto done; }
	s1 = popg (gdata, abs(A->ur->sign) + abs(B->lr->sign));
	if (s1 == NULL) { stop_reason = GIANT_OUT_OF_MEMORY; goto done; }

	gtog (B->ul, s0);
	gtog (B->ll, s1);
	stop_reason = mulgi (gdata, A->ul, B->ul);
	if (stop_reason) goto done;
	stop_reason = mulgi (gdata, A->ur, s1);
	if (stop_reason) goto done;
	addg (s1, B->ul);

	stop_reason = mulgi (gdata, A->lr, B->ll);
	if (stop_reason) goto done;
	stop_reason = mulgi (gdata, A->ll, s0);
	if (stop_reason) goto done;
	addg (s0, B->ll);

	gtog (B->ur, s0);
	gtog (B->lr, s1);
	stop_reason = mulgi (gdata, A->ul, B->ur);
	if (stop_reason) goto done;
	stop_reason = mulgi (gdata, A->ur, s1);
	if (stop_reason) goto done;
	addg (s1, B->ur);
	stop_reason = mulgi (gdata, A->lr, B->lr);
	if (stop_reason) goto done;
	stop_reason = mulgi (gdata, A->ll, s0);
	if (stop_reason) goto done;
	addg (s0, B->lr);

done:	pushall (gdata, ss);
	return (stop_reason);
}

int mulmMsp (		
			
	ghandle *gdata,	
	gmatrix A,
	gmatrix B,
	int	maxsize)
{
	giant	tmp0, tmp1, tmp2, tmp3;
	int	ss, stop_reason;

	ss = stackstart (gdata);
	tmp0 = popg (gdata, maxsize);
	if (tmp0 == NULL) { stop_reason = GIANT_OUT_OF_MEMORY; goto done; }
	tmp1 = popg (gdata, maxsize);
	if (tmp1 == NULL) { stop_reason = GIANT_OUT_OF_MEMORY; goto done; }
	tmp2 = popg (gdata, maxsize);
	if (tmp2 == NULL) { stop_reason = GIANT_OUT_OF_MEMORY; goto done; }
	tmp3 = popg (gdata, maxsize);
	if (tmp3 == NULL) { stop_reason = GIANT_OUT_OF_MEMORY; goto done; }

	gtog (A->ur, tmp0);	
	gtog (A->lr, tmp2);	
	gtog (B->ur, tmp1);	

	stop_reason = mulgi (gdata, A->ul, B->ur);	
	if (stop_reason) goto done;
	gtog (tmp0, tmp3);
	stop_reason = mulgi (gdata, B->lr, tmp3);	
	if (stop_reason) goto done;
	addg (tmp3, B->ur);				

	stop_reason = mulgi (gdata, A->ll, tmp1);	
	if (stop_reason) goto done;
	stop_reason = mulgi (gdata, tmp2, B->lr);	
	if (stop_reason) goto done;
	addg (tmp1, B->lr);				

	stop_reason = mulgi (gdata, B->ll, tmp0);	
	if (stop_reason) goto done;
	gtog (B->ul, tmp1);				
	gtog (A->ll, tmp3);				
	stop_reason = mulgi (gdata, A->ul, B->ul);	
	if (stop_reason) goto done;
	addg (tmp0, B->ul);				

	stop_reason = mulgi (gdata, tmp3, tmp1);	
	if (stop_reason) goto done;
	stop_reason = mulgi (gdata, tmp2, B->ll);	
	if (stop_reason) goto done;
	addg (tmp1, B->ll);				

done:	pushall (gdata, ss);
	return (stop_reason);
}

void punch (		
	ghandle *gdata,	
	giant 	q,
	gmatrix A)
{
	giant tmp = popg (gdata, abs(A->lr->sign) + q->sign);

	gtog (q, tmp);
	mulgi (gdata, A->ll, tmp);
	subg (tmp, A->ul);
	gswap (&A->ul, &A->ll);

	gtog (q, tmp);
	mulgi (gdata, A->lr, tmp);
	subg (tmp, A->ur);
	gswap (&A->ur, &A->lr);

	pushg (gdata, 1);
}

int ggcd (		
	ghandle *gdata,	
	giant 	*x,
	giant 	*y,
	gmatrix	R,
	int	interruptable)
{
	gmatrixstruct A;
	int	ss, stop_reason;

	ss = stackstart (gdata);

	if ((*x)->sign / 2 > FFT_BREAK_MULT1) {
		stop_reason = init_sincos (gdata, lpt ((*x)->sign / 2) << 1);
		if (stop_reason) return (stop_reason);
	}

	if (R != NULL) {
		stop_reason = hgcd (gdata, 0, x, y, R, interruptable);
		if (stop_reason) return (stop_reason);
		return (rhgcd (gdata, x, y, R, interruptable));
	}

	while ((*y)->sign > GCDLIMIT) {
		int 	quarter_size, third_size, a_size;

		quarter_size = ((*y)->sign - 1) >> 2;
		third_size = ((*y)->sign - 1) / 3;
		if (lpt (quarter_size) == lpt (third_size))
			a_size = third_size;
		else
			a_size = quarter_size;

		A.ul = popg (gdata, a_size);
		if (A.ul == NULL) { stop_reason = GIANT_OUT_OF_MEMORY; goto done; }
		setone (A.ul);
		A.ll = popg (gdata, a_size);
		if (A.ll == NULL) { stop_reason = GIANT_OUT_OF_MEMORY; goto done; }
		setzero (A.ll);
		A.ur = popg (gdata, a_size);
		if (A.ur == NULL) { stop_reason = GIANT_OUT_OF_MEMORY; goto done; }
		setzero (A.ur);
		A.lr = popg (gdata, a_size);
		if (A.lr == NULL) { stop_reason = GIANT_OUT_OF_MEMORY; goto done; }
		setone (A.lr);

		stop_reason = hgcd (gdata, (*y)->sign - (a_size + a_size + 1),
				    x, y, &A, interruptable);
		if (stop_reason) goto done;

		if (isone (A.lr)) {
			pushg (gdata, 4);		
			onestep (gdata, x, y, NULL);
			continue;
		}

		pushg (gdata, 4);		
	}

	return (cextgcdg (gdata, x, y, NULL, interruptable));

done:	pushall (gdata, ss);
	return (stop_reason);
}

int rhgcd (	
		
	ghandle *gdata,	
	giant	*x,
	giant 	*y,
	gmatrix R,
	int	interruptable)
{
	gmatrixstruct A;
	int	ss, stop_reason;

	ss = stackstart (gdata);
	A.ul = popg (gdata, (*x)->sign);
	if (A.ul == NULL) { stop_reason = GIANT_OUT_OF_MEMORY; goto done; }
	setone (A.ul);
	A.ll = popg (gdata, (*x)->sign);
	if (A.ll == NULL) { stop_reason = GIANT_OUT_OF_MEMORY; goto done; }
	setzero (A.ll);
	A.ur = popg (gdata, (*x)->sign + 1);					
	if (A.ur == NULL) { stop_reason = GIANT_OUT_OF_MEMORY; goto done; }
	setzero (A.ur);
	A.lr = popg (gdata, (*x)->sign + 1);					
	if (A.lr == NULL) { stop_reason = GIANT_OUT_OF_MEMORY; goto done; }
	setone (A.lr);

	if ((*y)->sign <= GCDLIMIT) {
		stop_reason = cextgcdg (gdata, x, y, &A, interruptable);
		if (stop_reason) goto done;
	} else {
		stop_reason = hgcd (gdata, 0, x, y, &A, interruptable);
		if (stop_reason) goto done;
		if (isone (A.lr)) onestep (gdata, x, y, &A);
		stop_reason = rhgcd (gdata, x, y, &A, interruptable);
		if (stop_reason) goto done;
	}
	stop_reason = mulmM (gdata, &A, R);
	if (stop_reason) goto done;

done:	pushall (gdata, ss);
	return (stop_reason);
}

int hgcd (	
	ghandle *gdata,	
	int 	n,
	giant	*xx,
	giant 	*yy,
	gmatrix A,
	int	interruptable)
{
	giant 	x, y;
	int	half_size, stop_reason;

	ASSERTG (n >= 0 && n < (*xx)->sign && n < (*yy)->sign);

#ifdef DEBUG_CODE
giant t1 = allocgiant (10000);
giant t2 = allocgiant (10000);
int A1 = (A->ur->sign == 0);
gtog (*xx, t1);
gtog (*yy, t2);
#endif

	if ((*yy)->sign <= n) return (0);

	x = *xx;
	x->sign -= n;
	x->n += n;
	setmaxsize (x, x->maxsize - n);
	y = *yy;
	y->sign -= n;
	y->n += n;
	setmaxsize (y, y->maxsize - n);

	half_size = (y->sign - 1) >> 1;
	if (half_size <= CHGCD_BREAK || x->sign - y->sign > half_size >> 1) {
		for ( ; ; ) {
			int	quot_length, a_length;

			quot_length = x->sign - y->sign;
			if (x->n[x->sign-1] >= y->n[y->sign-1]) quot_length++;
			a_length = abs (A->lr->sign);
			if (A->lr->n[a_length-1] & 0x80000000) a_length++;
			if (a_length + quot_length > half_size) break;

			if (quot_length > 1 || !gcdhlp_wrapper (gdata, x, y, A))
				onestep (gdata, &x, &y, A);
		}
	}

	else {
		gmatrixstruct B;
		giantstruct ul, ur, ll, lr;
		int	a_size, b_size;

		if (interruptable && StopCheckRoutine != NULL) {
			stop_reason = stopCheck (interruptable);
			if (stop_reason) return (stop_reason);
		}

		a_size = half_size >> 1;
		stop_reason = hgcd (gdata, y->sign - (a_size + a_size + 1), &x, &y, A, interruptable);
		if (stop_reason) return (stop_reason);
		a_size = intmax (abs (A->lr->sign), abs (A->ur->sign));
		ASSERTG (a_size >= abs (A->ll->sign) && a_size >= abs (A->ul->sign));

		b_size = half_size - a_size;
		if (A->lr->n[a_size-1] & 0x80000000) b_size--;
		ul.n = A->ul->n + a_size; setmaxsize (&ul, b_size);
		ur.n = A->ur->n + a_size; setmaxsize (&ur, b_size);
		ll.n = A->ll->n + a_size; setmaxsize (&ll, b_size);
		lr.n = A->lr->n + a_size; setmaxsize (&lr, b_size);
		B.ul = &ul; setone (&ul);
		B.ur = &ur; setzero (&ur);
		B.ll = &ll; setzero (&ll);
		B.lr = &lr; setone (&lr);
		if (b_size > ((y->sign - 1) >> 1)) b_size = (y->sign - 1) >> 1;

		if (x->sign >= 2 && (x->sign != y->sign || x->n[x->sign-1] != y->n[y->sign-1] || x->n[x->sign-2] != y->n[y->sign-2])) {
			stop_reason = hgcd (gdata, y->sign - (b_size + b_size + 1), &x, &y, &B, interruptable);
			if (stop_reason) return (stop_reason);
			stop_reason = mulmMsp (gdata, &B, A, half_size);
			if (stop_reason) return (stop_reason);
		}
	}

	if (n) {
		giant 	xinp, yinp, tmp;

		tmp = x; x = popg (gdata, x->sign); gtog (tmp, x);
		tmp = y; y = popg (gdata, y->sign); gtog (tmp, y);
		xinp = *xx;
		xinp->sign = n;
		xinp->n -= n;
		while (xinp->sign && xinp->n[xinp->sign-1] == 0) xinp->sign--;
		setmaxsize (xinp, xinp->maxsize + n);
		yinp = *yy;
		yinp->sign = n;
		yinp->n -= n;
		while (yinp->sign && yinp->n[yinp->sign-1] == 0) yinp->sign--;
		setmaxsize (yinp, yinp->maxsize + n);

		stop_reason = mulvM (gdata, A, xinp, yinp);
		if (stop_reason) {
			pushg (gdata, 2);
			return (stop_reason);
		}
		addshiftedg (n, x, xinp);
		addshiftedg (n, y, yinp);
		if (xinp->sign < 0) {
			negg (xinp); negg (A->ul); negg (A->ur);
		}
		if (yinp->sign < 0) {
			negg (yinp); negg (A->ll); negg (A->lr);
		}
		if (gcompg (xinp, yinp) < 0) {
			gswap (xx, yy);
			gswap (&A->ul, &A->ll);
			gswap (&A->ur, &A->lr);
		}
		pushg (gdata, 2);
	}

	else {
		if (*xx != x) gswap (xx, yy);
	}

#ifdef DEBUG_CODE
if (A1) {
giant t3 = allocgiant (10000);
giant t4 = allocgiant (10000);
gtog (t1, t3);  mulg (A->ul, t3);
gtog (t2, t4);  mulg (A->ur, t4);
addg (t3, t4);
ASSERTG (gcompg (*xx, t4) == 0);
gtog (t1, t3);  mulg (A->ll, t3);
gtog (t2, t4);  mulg (A->lr, t4);
addg (t3, t4);
ASSERTG (gcompg (*yy, t4) == 0);
free (t3);
free (t4);
}
free (t1);
free (t2);
#endif

	return (0);
}
