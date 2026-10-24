/*----------------------------------------------------------------------
| This file contains various utility routines that may be used by gwnum
| routines, prime95, or other consumers of gwnum.
| 
|  Copyright 2004-2023 Mersenne Research, Inc.  All rights reserved.
+---------------------------------------------------------------------*/

#ifndef _GWUTIL_H
#define _GWUTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#define divide_rounding_up(a,b)		(((a) + (b) - 1) / (b))
#define divide_rounding_down(a,b)	((a) / (b))
#define round_up_to_multiple_of(a,b)	(divide_rounding_up (a, b) * (b))
#define round_down_to_multiple_of(a,b)	(divide_rounding_down (a, b) * (b))
#define fltmod(a,b)			((double)(a) - floor ((double)(a) / (double)(b)) * (double)(b))
#define round_to_cache_line(p)		(void *) (((intptr_t)(p) + 63) & ~63)
#define intmin(a,b)			(((int)(a) < (int)(b)) ? (int)(a) : (int)(b))
#define intmax(a,b)			(((int)(a) > (int)(b)) ? (int)(a) : (int)(b))
#define fltmax(a,b)			(((double)(a) > (double)(b)) ? (double)(a) : (double)(b))
#define round_double_to_int32(d,i)	{double t = ((d) + 6755399441055744.0); i = *((int32_t *)(&t));}

#define intpow(b,n)	((long) floor (pow ((double)(b), (double)(n)) + 0.1))

#define align_ptr(p,n)	(void *) (((intptr_t)(p) + (n)-1) & ~((n)-1))

void * aligned_offset_malloc (size_t size, size_t alignment, size_t mod);
void * aligned_malloc (size_t size, size_t alignment);
void  aligned_free (void *ptr);

int large_pages_supported ();
void * large_pages_malloc (size_t size);
void large_pages_free (void *ptr);
void * aligned_offset_large_pages_malloc (size_t size, size_t alignment, size_t mod);
void * aligned_large_pages_malloc (size_t size, size_t alignment);
void aligned_large_pages_free (void *ptr);

void truncated_strcpy (char *buf, unsigned int bufsize, const char *val);
void truncated_strcpy_with_len (char *buf, unsigned int bufsize, const char *val, unsigned int valsize);

#ifdef _WIN32
#if defined (__MINGW32__) || defined (__MINGW64__)
#include <sys/time.h>
#endif
int gettimeofday (struct timeval *tp, void *tzp);
#endif

#ifdef __cplusplus
}
#endif

#endif
