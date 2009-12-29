#if LAB >= 2
/*
 * Blocking synchronization abstractions for kernel threads.
 * See COPYRIGHT for copyright information.
 */

#include "sync.h"


////////// Mutexes //////////

void
mutex_init(mutex *m)
{
#if SOL >= 2
	spinlock_init(&m->lock);
	m->owner = NULL;
	waitqueue_init(&m->wait);
#else
	panic("mutex_init not implemented.");
#endif
}

void
mutex_acquire(mutex *m)
{
#if SOL >= 2
	thread *self = thread_self();

	spinlock_acquire(&m->lock);
	while (m->owner != NULL) {
		if (m->owner == thread_self())
			panic("Recursive mutex_acquire attempt!");

		// Lock the current thread 
		thread_lock(self);
		waitqueue_enqueue(&m->wait, self);
		
		spinlock_release(&m->lock);

		waitqueue_block(&m->wait, &m->lock);
		spinlock_acquire(&m->lock);
	}
	m->owner = thread_self();
	spinlock_release(&m->lock);
#else
	panic("mutex_acquire not implemented.");
#endif
}

void
mutex_release(mutex *m)
{
#if SOL >= 2
	spinlock_acquire(&m->lock);

	// Clear our ownership of the mutex
	assert(m->owner == thread_self());
	m->owner = NULL;

	// Unblock one waiting thread, if there are any
	waitqueue_unblock_one(&m->wait);

	spinlock_release(&m->lock);
#else
	panic("mutex_release not implemented.");
#endif
}


////////// Condition variables //////////

void
cond_init(cond *c)
{
#if SOL >= 2
	spinlock_init(&c->lock);
	waitqueue_init(&c->wait);
#else
	panic("cond_init not implemented.");
#endif
}

void
cond_wait(cond *c, mutex *m)
{
#if SOL >= 2
	// First acquire the condition variable's low-level spinlock.
	spinlock_acquire(&c->lock);

	// Now we can safely release the mutex:
	// if another thread tries to signal the condition variable now,
	// that thread will spin on c->lock until we're done here,
	// ensuring that we don't lose the signal before we get on the queue.
	mutex_release(m);

	// Get on the condition variable's wait queue
	// and atomically release c->lock.
	waitqueue_block(&c->wait);
#else
	panic("cond_wait not implemented.");
#endif
}

void
cond_signal(cond *c)
{
#if SOL >= 2
	// Unblock one thread waiting on c->wait, if there is one.
	// We must hold c->lock since it protects the wait queue.
	spinlock_acquire(&c->lock);
	waitqueue_unblock_one(&c->wait);
	spinlock_release(&c->lock);
#else
	panic("cond_signal not implemented.");
#endif
}

void
cond_broadcast(cond *c)
{
#if SOL >= 2
	// Unblock all threads waiting on c->wait, if there are any.
	// We must hold c->lock since it protects the wait queue.
	spinlock_acquire(&c->lock);
	waitqueue_unblock_all(&c->wait);
	spinlock_release(&c->lock);
#else
	panic("cond_broadcast not implemented.");
#endif
}


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

void
rwlock_init(rwlock *l)
{
#if SOL >= 2
	spinlock_init(&l->lock);
	l->state = RWLOCK_FREE;
	l->count = 0;
	l->owner = NULL;
	waitqueue_init(&l->wait);
#else
	panic("rwlock_init not implemented.");
#endif
}

void
rwlock_readlock(rwlock *l)
{
#if SOL >= 2
	spinlock_acquire(&l->lock);

	// If the lock is already write-locked, we must wait until it isn't.
	while (m->state == RWLOCK_WRITE) {
		if (l->owner == thread_self())
			panic("Recusrive read-lock after write-lock!");
		waitqueue_block(&l->wait, &l->lock);
		spinlock_acquire(&l->lock);
	}

	// Put the lock in the READ state and increment its counter.
	if (m->state == RWLOCK_FREE) {
		assert(m->count == 0);
		m->state = RWLOCK_READ);
	}

	assert(m->state == RWLOCK_READ);
	assert(m->owner == NULL);	// Read locks have no particular owner

	m->count++;
	assert(m->count > 0);	// panic if counter overflows

	spinlock_release(&m->lock);
#else
	panic("rwlock_readlock not implemented.");
#endif
}

void
rwlock_writelock(rwlock *l)
{
#if SOL >= 2
	spinlock_acquire(&l->lock);

	// If we already have a write lock, just increment the nesting counter.
	if (m->state == RWLOCK_WRITE && m->owner == thread_self()) {
		m->count++;
		assert(m->count > 0);	// panic if counter overflows
		spinlock_release(&l->lock);
		return;
	}

	// If the lock is already locked by anyone else, wait until it isn't.
	while (m->state != RWLOCK_FREE) {
		waitqueue_block(&l->wait, &l->lock);
		spinlock_acquire(&l->lock);
	}

	assert(m->state == RWLOCK_FREE);
	assert(m->owner == NULL);
	assert(m->count = 0);

	// Acquire the lock.
	m->state = RWLOCK_WRITE;
	m->count = 1;
	m->owner = thread_self();

	spinlock_release(&m->lock);
#else
	panic("rwlock_writelock not implemented.");
#endif
}

void
rwlock_unlock(rwlock *l)
{
#if SOL >= 2
	spinlock_acquire(&m->lock);

	// Remove one reference count,
	// truly releasing the lock only when count drops to zero.
	assert(m->state != RWLOCK_FREE);
	assert(m->count > 0);
	if (--m->count == 0) {
		l->state = RWLOCK_FREE;
		l->owner = NULL;

		// Unblock all waiting threads,
		// on the optimistic assumption they are all readers.
		// This implementation is far from ideal for efficiency,
		// since if many writers are waiting we'll wake them all up
		// and all but one will just have to go back to sleep again.
		// A smarter implementation might have separate queues
		// for readers and writers,
		// and either unblock all readers or just the next writers.
		// This would makes fairness a bit more tricky, however:
		// we must be careful to ensure that readers
		// can never completely starve writers or vice versa.
		waitqueue_unblock_all(&l->wait);
	}

	spinlock_release(&l->lock);
#else
	panic("rwlock_unlock not implemented.");
#endif
}


#endif // !PIOS_KERN_THREAD_H
#endif // LAB >= 2
