// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

#ifndef _GWTHREAD_H
#define _GWTHREAD_H

#ifdef __cplusplus
extern "C" {
#endif

typedef int64_t gwatomic;	

#define atomic_set(x,v)		gwatomic_set(&(x), v)				
#define atomic_get(x)		gwatomic_get(&(x))				
#define atomic_incr(x)		((void) gwatomic_fetch_increment(&(x)))		
#define atomic_decr(x)		((void) gwatomic_fetch_decrement(&(x)))		
#define atomic_fetch_incr(x)	(gwatomic_fetch_increment(&(x)))		
#define atomic_incr_fetch(x)	(gwatomic_fetch_increment(&(x)) + 1)		
#define atomic_fetch_decr(x)	(gwatomic_fetch_decrement(&(x)))		
#define atomic_decr_fetch(x)	(gwatomic_fetch_decrement(&(x)) - 1)		
#define atomic_fetch_addin(x,v)	(gwatomic_fetch_add(&(x), v))			
#define atomic_spinwait(x,v)	gwatomic_spinwait(&(x), v)			

void gwatomic_set (gwatomic *x, int64_t val);
int64_t gwatomic_get (gwatomic *x);
int64_t gwatomic_fetch_increment (gwatomic *x);
int64_t gwatomic_fetch_decrement (gwatomic *x);
int64_t gwatomic_fetch_add (gwatomic *x, int64_t val);
void gwatomic_spinwait (gwatomic *x, int64_t val);

typedef void *gwmutex;			

void gwmutex_init (gwmutex *mutex);	
void gwmutex_lock (gwmutex *mutex);	
void gwmutex_unlock (gwmutex *mutex);	
void gwmutex_destroy (gwmutex *mutex);	

typedef void *gwevent;			

void gwevent_init (gwevent *event);	
#define GWEVENT_TIMED_OUT	1
#define GWEVENT_SIGNALED	2
int gwevent_wait (gwevent *event, int seconds);
void gwevent_signal (gwevent *event);	
void gwevent_reset (gwevent *event);	
void gwevent_destroy (gwevent *event);	

typedef void *gwthread;			

void gwthread_create (gwthread *thread_id, void (*thread_proc)(void *), void *arg);
void gwthread_create_waitable (gwthread *thread_id, void (*thread_proc)(void *), void *arg);
void gwthread_wait_for_exit (gwthread *thread_id);

#ifdef __cplusplus
}
#endif

#endif
