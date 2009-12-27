#if LAB >= 2
/*
 * Blocking synchronization abstractions for kernel threads.
 * See COPYRIGHT for copyright information.
 */
#ifndef PIOS_KERN_THREAD_H
#define PIOS_KERN_THREAD_H

#include "thread.h"


// Blocking mutual exclusion (mutex) lock
typedef struct mutex {
#if SOL >= 2
	struct spinlock lock;		// Spinlock protecting this mutex
	struct thread *owner;		// Current owner, NULL if unlocked
	struct waitqueue wait;		// List of waiting threads
#else
	// Insert internal definitions to implement mutexes here
#endif
} mutex;

void mutex_init(mutex *m);		// Initialize mutex lock
void mutex_acquire(mutex *m);		// Acquire exclusive use
void mutex_release(mutex *m);		// Release mutex


// Condition variables for blocking inter-thread signaling
typedef struct cond {
#if SOL >= 2
	struct spinlock lock;
	struct waitqueue wait;
#else
	// Insert internal definitions to implement condition variables here
#endif
} cond;

void cond_init(cond *c);		// Initialize condition variable
void cond_wait(cond *c, mutex *m);	// Release m and wait for c
void cond_signal(cond *c);		// Wake up a thread waiting on c
void cond_broadcast(cond *c);		// Wake up all threads waiting on c


// Blocking read/write lock with recursive locking support
#if SOL >= 2
enum rwlock_state {
	RWLOCK_FREE	= 0,		// Not locked by anyone
	RWLOCK_READ	= 1,		// Locked by one or more writers
	RWLOCK_WRITE	= 2,		// Exclusively locked by a writer
};

#endif
typedef struct rwlock {
#if SOL >= 2
	struct spinlock lock;		// Spinlock protecting this mutex
	enum rwlock_state state;	// Current lock state
	int count;			// Number of outstanding lock calls
	struct thread *owner;		// Exclusive owner if in WRITE state
	struct waitqueue wait;		// List of waiting threads
#else
	// Insert internal definitions to implement mutexes here
#endif
} rwlock;

void rwlock_init(rwlock *l);		// Initialize mutex lock
void rwlock_readlock(rwlock *l);	// Acquire inclusive read lock
void rwlock_writelock(rwlock *l);	// Acquire exclusive write lock
void rwlock_unlock(rwlock *l);		// Release inclusive or exclusive lock


#endif // !JOS_KERN_THREAD_H
#endif // LAB >= 2
