/*----------------------------------------------------------------------
| gwutil.c
|
| This file contains various utility routines that may be used by gwnum
| routines, prime95, or other consumers of gwnum.
| 
|  Copyright 2004-2020 Mersenne Research, Inc.  All rights reserved.
+---------------------------------------------------------------------*/

/* Include files */

#include <stdlib.h>
#include <memory.h>
#include <string.h>
#ifdef _WIN32
#include "windows.h"
#endif
#include <stdlib.h>
#if defined (__linux__) || defined (__HAIKU__)
#include <malloc.h>
#include <sys/mman.h>
#endif
#if defined (__APPLE__)
#include <sys/mman.h>
#include <mach/vm_statistics.h>
#endif
#if defined (__FreeBSD__)
#include <sys/mman.h>
#endif
#include "gwcommon.h"
#include "gwutil.h"

void * aligned_offset_malloc (
	size_t	size,
	size_t	alignment,
	size_t	mod)
{

	char	*p, *q;
	p = (char *) malloc (sizeof (void *) + size + alignment);
	if (p == NULL) return (NULL);
	q = (char *) (((intptr_t) p + sizeof (void *) + mod + alignment - 1) & ~(alignment - 1)) - mod;
	* (void **) ((char *) q - sizeof (void *)) = p;
	return (q);
}

void * aligned_malloc (
	size_t	size,
	size_t	alignment)
{

	return (aligned_offset_malloc (size, alignment, 0));
}

void aligned_free (
	void	*ptr)
{

	if (ptr == NULL) return;
	free (* (void **) ((char *) ptr - sizeof (void *)));
}

#define TWO_MEGABYTES	2*1024*1024

static int large_pages_are_supported = 0;
#if defined (_WIN32)
static size_t windows_large_page_size = 0;
#endif

void large_pages_init ()
{
static	int large_pages_first_call = 1;
	void	*p;

	if (!large_pages_first_call) return;
	large_pages_first_call = 0;

	large_pages_are_supported = TRUE;

#if defined (_WIN32) && defined (MEM_LARGE_PAGES)
	{
	DWORD	lasterr;
	HANDLE	hToken;
	LUID	luid;
	TOKEN_PRIVILEGES tp;
	HINSTANCE hDll;      
	int (*pGetLargePageMinimum)(void);

	OpenProcessToken (GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken);
	lasterr = GetLastError ();
	LookupPrivilegeValue (NULL, "SeLockMemoryPrivilege", &luid);
	lasterr = GetLastError ();

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	AdjustTokenPrivileges (hToken, FALSE, &tp, sizeof (TOKEN_PRIVILEGES), (PTOKEN_PRIVILEGES) NULL, (PDWORD) NULL);
	lasterr = GetLastError ();

	hDll = LoadLibrary (TEXT ("kernel32.dll"));
	pGetLargePageMinimum = (int (*)(void)) GetProcAddress (hDll, "GetLargePageMinimum");
	if (pGetLargePageMinimum != NULL) windows_large_page_size = (*pGetLargePageMinimum)();
	FreeLibrary (hDll);

	large_pages_are_supported = (windows_large_page_size == TWO_MEGABYTES || windows_large_page_size == 2*TWO_MEGABYTES);
	}
#endif

	p = large_pages_malloc (TWO_MEGABYTES/2);
	if (p == NULL) large_pages_are_supported = FALSE;
	else large_pages_free (p);
}

int large_pages_supported ()
{
	large_pages_init ();
	return (large_pages_are_supported);
}

void * large_pages_malloc (
	size_t	size)
{
	void	*p;
	uint64_t *q;

	large_pages_init ();
	if (!large_pages_are_supported) return (NULL);

	size = round_up_to_multiple_of (size + 4096, TWO_MEGABYTES);

#if defined (_WIN32) && defined (MEM_LARGE_PAGES)
	{
	DWORD lasterr;
	size = round_up_to_multiple_of (size, windows_large_page_size);
	p = VirtualAlloc (NULL, size, MEM_RESERVE | MEM_COMMIT | MEM_LARGE_PAGES, PAGE_READWRITE);
	lasterr = GetLastError ();
	if (p == NULL) return (NULL);
	}
#elif defined (__linux__)
	p = mmap (NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
	if (p == MAP_FAILED) return (NULL);
#elif defined (__APPLE__)
	p = mmap (NULL, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, VM_FLAGS_SUPERPAGE_SIZE_2MB, 0);
	if (p == MAP_FAILED) return (NULL);
#elif defined (__FreeBSD__)
	p = mmap (NULL, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE | MAP_ALIGNED_SUPER, -1, 0);
	if (p == MAP_FAILED) return (NULL);
#else
	return (NULL);
#endif

	q = (uint64_t *) p;
	*q++ = size;
	return (q);
}

void large_pages_free (
	void	*ptr)
{
	uint64_t *q, size;

	if (ptr == NULL) return;

	q = (uint64_t *) ptr;
	size = *--q;

#ifdef _WIN32
	VirtualFree (q, 0, MEM_RELEASE);
#elif defined (__linux__) || defined (__APPLE__) || defined (__FreeBSD__)
	munmap (q, size);
#endif
}

void * aligned_offset_large_pages_malloc (
	size_t	size,
	size_t	alignment,
	size_t	mod)
{
	char	*p, *q;
	p = (char *) large_pages_malloc (sizeof (void *) + size + alignment);
	if (p == NULL) return (NULL);
	q = (char *) (((intptr_t) p + sizeof (void *) + mod + alignment - 1) & ~(alignment - 1)) - mod;
	* (void **) ((char *) q - sizeof (void *)) = p;
	return (q);
}

void * aligned_large_pages_malloc (
	size_t	size,
	size_t	alignment)
{
	return (aligned_offset_large_pages_malloc (size, alignment, 0));
}

void aligned_large_pages_free (
	void	*ptr)
{
	if (ptr == NULL) return;
	large_pages_free (* (void **) ((char *) ptr - sizeof (void *)));
}

void truncated_strcpy_with_len (
	char	*buf,
	unsigned int bufsize,
	const char *val,
	unsigned int valsize)
{
	if (valsize >= bufsize) valsize = bufsize - 1;
	memcpy (buf, val, valsize);
	buf[valsize] = 0;
}

void truncated_strcpy (
	char	*buf,
	unsigned int bufsize,
	const char *val)
{
	truncated_strcpy_with_len (buf, bufsize, val, (unsigned int) strlen (val));
}

#ifdef _WIN32
int gettimeofday (
	struct timeval *tp,
	void	*tzp)
{
	
	static const uint64_t EPOCH = ((uint64_t) 116444736000000000ULL);

	SYSTEMTIME  system_time;
	FILETIME    file_time;
	uint64_t    time;

	GetSystemTime (&system_time);
	SystemTimeToFileTime (&system_time, &file_time);
	time =  ((uint64_t) file_time.dwLowDateTime);
	time += ((uint64_t) file_time.dwHighDateTime) << 32;

	tp->tv_sec  = (long) ((time - EPOCH) / 10000000L);
	tp->tv_usec = (long) (system_time.wMilliseconds * 1000);
	return 0;
}
#endif

