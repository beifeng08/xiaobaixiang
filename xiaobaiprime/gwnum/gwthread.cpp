/*----------------------------------------------------------------------
| gwthread.cpp
|
| This file contains the C++ routines and global variables that are used
| to implement atomics, multi-threading, mutexes, and locking.
| 
|  Copyright 2006-2023 Mersenne Research, Inc.  All rights reserved.
+---------------------------------------------------------------------*/

/* Include files */

#include <stdlib.h>
#ifdef _WIN32
#include "windows.h"
#include <process.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#include <pthread.h>
#endif

#include "gwcommon.h"
#include "gwthread.h"
#include <atomic>
#include <memory.h>

#define cast_as_atomic_int(a)	((std::atomic<int64_t> *)(a))

extern "C"
void	gwatomic_set (gwatomic *x, int64_t val) {

	cast_as_atomic_int(x)->store (val, std::memory_order_relaxed);
}

extern "C"
int64_t	gwatomic_get (gwatomic *x) {

	return (cast_as_atomic_int(x)->load (std::memory_order_relaxed));
}

extern "C"
int64_t	gwatomic_fetch_increment (gwatomic *x) {

	return (cast_as_atomic_int(x)->fetch_add (1, std::memory_order_relaxed));
}

extern "C"
int64_t	gwatomic_fetch_decrement (gwatomic *x) {

	return (cast_as_atomic_int(x)->fetch_sub (1, std::memory_order_relaxed));
}

extern "C"
int64_t	gwatomic_fetch_add (gwatomic *x, int64_t val) {

	return (cast_as_atomic_int(x)->fetch_add (val, std::memory_order_relaxed));
}

extern "C"
void	gwatomic_spinwait (gwatomic *x, int64_t val) {

	while (cast_as_atomic_int(x)->load (std::memory_order_relaxed) != val) {
#ifdef _MSC_VER
		_mm_pause();
#else
		__builtin_ia32_pause();
#endif
	}
}

extern "C"
void gwmutex_init (
	gwmutex	*mutex)			
{
#ifdef _WIN32
	*mutex = (gwmutex) malloc (sizeof (CRITICAL_SECTION));

	InitializeCriticalSection ((LPCRITICAL_SECTION) *mutex);
#else
	*mutex = (gwmutex) malloc (sizeof (pthread_mutex_t));

	pthread_mutex_init ((pthread_mutex_t *) *mutex, NULL);
#endif
}

extern "C"
void gwmutex_lock (
	gwmutex	*mutex)			
{
	if (*mutex == NULL) return;
#ifdef _WIN32
	EnterCriticalSection ((LPCRITICAL_SECTION) *mutex);
#else
	pthread_mutex_lock ((pthread_mutex_t *) *mutex);
#endif
}

extern "C"
void gwmutex_unlock (
	gwmutex	*mutex)			
{
	if (*mutex == NULL) return;
#ifdef _WIN32
	LeaveCriticalSection ((LPCRITICAL_SECTION) *mutex);
#else
	pthread_mutex_unlock ((pthread_mutex_t *) *mutex);
#endif
}

extern "C"
void gwmutex_destroy (
	gwmutex	*mutex)			
{
	if (*mutex == NULL) return;
#ifdef _WIN32
	DeleteCriticalSection ((LPCRITICAL_SECTION) *mutex);
#else
	pthread_mutex_destroy ((pthread_mutex_t *) *mutex);
#endif
	free (*mutex);
	*mutex = NULL;
}

#ifdef _WIN32
#else
struct event_data {
	pthread_mutex_t event_mutex;
	pthread_cond_t event_cond;
	int	signalled_state;
	int	threads_waiting;
	int	signalled_count;
};
#endif

extern "C"
void gwevent_init (
	gwevent	*event)			
{
#ifdef _WIN32
 	* (HANDLE *) event = CreateEvent (
				NULL,		
				TRUE,		
				FALSE,		
				NULL);		
	ASSERTG (*(HANDLE *) event != 0);
#else
	struct event_data *e;

	e = (struct event_data *) malloc (sizeof (struct event_data));

	pthread_mutex_init (&e->event_mutex, NULL);
	pthread_cond_init (&e->event_cond, NULL);
	e->signalled_state = FALSE;
	e->threads_waiting = 0;
	e->signalled_count = 0;
	*event = (gwevent) e;
#endif
}

extern "C"
int gwevent_wait (
	gwevent	*event,			
	int	seconds)		
{
#ifdef _WIN32
	DWORD	rc;

	if (seconds == 0)
		rc = WaitForSingleObject (
			* (HANDLE *) event,	
			INFINITE);		
	else
		rc = WaitForSingleObject (
			* (HANDLE *) event,	
			seconds * 1000);	
	if (rc == WAIT_TIMEOUT) return (GWEVENT_TIMED_OUT);
	ASSERTG (rc == 0);
	return (GWEVENT_SIGNALED);
#else
	struct event_data *e;
	struct timeval now;
	struct timespec timeout;
	int	rc;

	e = (struct event_data *) *event;
	rc = 0;

	if (seconds) {
		gettimeofday (&now, NULL);
		timeout.tv_sec = now.tv_sec + seconds;
		timeout.tv_nsec = now.tv_usec * 1000;
	}

	pthread_mutex_lock (&e->event_mutex);
	if (!e->signalled_state) {

		e->threads_waiting++;
		for ( ; ; ) {
			int	signalled_count;
			signalled_count = e->signalled_count;
			if (seconds) {
				rc = pthread_cond_timedwait (&e->event_cond,
							     &e->event_mutex,
							     &timeout);
				if (rc == ETIMEDOUT) break;
			} else
				rc = pthread_cond_wait (&e->event_cond,
							&e->event_mutex);
			if (signalled_count != e->signalled_count) break;
		}
		e->threads_waiting--;
	}
	pthread_mutex_unlock (&e->event_mutex);

	if (rc == ETIMEDOUT) return (GWEVENT_TIMED_OUT);
	ASSERTG (rc == 0);
	return (GWEVENT_SIGNALED);
#endif
}

extern "C"
void gwevent_signal (
	gwevent	*event)			
{
#ifdef _WIN32
	BOOL	rc;

	rc = SetEvent (* (HANDLE *) event);	
	ASSERTG (rc);
#else
	struct event_data *e;

	e = (struct event_data *) *event;
	pthread_mutex_lock (&e->event_mutex);
	if (!e->signalled_state) {
		e->signalled_state = TRUE;
		e->signalled_count++;
		if (e->threads_waiting)
			pthread_cond_broadcast (&e->event_cond);
	}
	pthread_mutex_unlock (&e->event_mutex);
#endif
}

extern "C"
void gwevent_reset (
	gwevent	*event)			
{
#ifdef _WIN32
	BOOL	rc;

	rc = ResetEvent (* (HANDLE *) event);	
	ASSERTG (rc);
#else
	struct event_data *e;

	e = (struct event_data *) *event;
	pthread_mutex_lock (&e->event_mutex);
	e->signalled_state = FALSE;
	pthread_mutex_unlock (&e->event_mutex);
#endif
}

extern "C"
void gwevent_destroy (
	gwevent	*event)			
{
	if (*event == NULL) return;

#ifdef _WIN32
	BOOL	rc;

	rc = CloseHandle (* (HANDLE *) event);	
	ASSERTG (rc);
#else
	struct event_data *e;

	e = (struct event_data *) *event;
	pthread_mutex_destroy (&e->event_mutex);
	pthread_cond_destroy (&e->event_cond);
	free (e);
#endif
	*event = NULL;
}

struct threaddata {
	void	(*proc)(void *);
	void	*arg;		
};

#ifdef _WIN32
extern "C"
void __cdecl ThreadStarter (
	void	*arg)
{
	struct threaddata *td;

	td = (struct threaddata *) arg;
	(*td->proc)(td->arg);
	free (arg);
}

extern "C"
unsigned __stdcall ThreadExStarter (
	void	*arg)
{
	struct threaddata *td;

	td = (struct threaddata *) arg;
	(*td->proc)(td->arg);
	free (arg);
	return (0);
}
#else
extern "C"
void *ThreadStarter (
	void	*arg)
{
	struct threaddata *td;

	td = (struct threaddata *) arg;
	(*td->proc)(td->arg);
	free (arg);
	return (NULL);
}

#endif

extern "C"
void gwthread_create (
	gwthread *thread_id,
	void	(*thread_proc)(void *),
	void	*arg)
{
	struct threaddata *td;
	td = (struct threaddata *) malloc (sizeof (struct threaddata));

	td->proc = thread_proc;
	td->arg = arg;

#ifdef _WIN32
	* (HANDLE *) thread_id = (HANDLE)
		_beginthread (
			&ThreadStarter, 
			0,		
			td);		
#else
	{
	pthread_attr_t attr;
	pthread_t tid;

	pthread_attr_init (&attr);
	pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
	pthread_create (&tid, &attr, &ThreadStarter, td);
	pthread_attr_destroy (&attr);
	*thread_id = (gwthread) tid;
	}
#endif
}

extern "C"
void gwthread_create_waitable (
	gwthread *thread_id,
	void	(*thread_proc)(void *),
	void	*arg)
{
	unsigned int thread_num;	
	struct threaddata *td;

	td = (struct threaddata *) malloc (sizeof (struct threaddata));

	td->proc = thread_proc;
	td->arg = arg;

#ifdef _WIN32
	* (HANDLE *) thread_id = (HANDLE)
		_beginthreadex (
			NULL,		
			0,		
			&ThreadExStarter, 
			td,		
			0,		
			&thread_num);	
#else
	{
	pthread_attr_t attr;
	pthread_t tid;

	pthread_attr_init (&attr);
	pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_JOINABLE);
	pthread_create (&tid, &attr, &ThreadStarter, td);
	pthread_attr_destroy (&attr);
	*thread_id = (gwthread) tid;
	}
#endif
}

extern "C"
void gwthread_wait_for_exit (
	gwthread *thread_id)
{
#ifdef _WIN32
	WaitForSingleObject (* (HANDLE *) thread_id, INFINITE);
	CloseHandle (* (HANDLE *) thread_id);
#else
	void	*returned_status;
	pthread_join ((pthread_t) *thread_id, &returned_status);
#endif
}

