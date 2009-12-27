#if LAB >= 2
#include <inc/assert.h>

#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>


#if SOL >= 2
static spinlock readylock;	// Spinlock protecting ready queue
static waitqueue ready;		// Queue of threads waiting to run
#else
// LAB 2: insert your scheduling data structure definitions here.
#endif


void
sched_init(void)
{
	spinlock_init(&readylock);
	waitqueue_init(&ready);
}

// Find and dispatch another thread to run on this CPU.
// On entry, assumes 'self' points to the current thread,
// self->lock is already held,
// and caller has already put self in some non-running state:
// e.g., enqueued onto a wait queue somewhere.
// This function doesn't appear to return until self gets scheduled again -
// possibly much later, possibly on a different processor.
// self->lock is released on return.
void
sched_dispatch(struct thread *self, thread_state newstate)
{
	thread *next, *last;

	// Set the old thread's state as specified by the caller.
	// Of course the caller could just do this itself,
	// but this way the caller won't forget to.
	self->state = newstate;

	// Dequeue a thread from the ready queue, if there is one.
	spinlock_acquire(&readylock);
	next = waitqueue_dequeue(&ready);
	spinlock_release(&readylock);

	// If there are no ready threads,
	// switch to the dedicated idle thread for this CPU.
	if (next == NULL)
		next = cpu_self()->idle_thread;

	// If we're just "context switching" back to ourself, we're done.
	// It's important in this case not to try to acquire next->lock,
	// since it's the same as self->lock and we're already holding it!
	if (next == self) {
		spinlock_release(&self->lock);
		return;
	}

#if LAB > 99
	// Lock the thread we'll be switching to.
	// In general, acquiring locks on two of the same type of object
	// can be risky from a deadlock perspective and must be done carefully:
	// if one thread acquires A then B while another acquires B then A,
	// then the two threads can deadlock.
	//
	// In this specific case of switching from one thread to another,
	// we rely on the facts that:
	// (a) this is the only place in the kernel
	// where we ever acquire two threads' locks at the same time, and
	// (b) we always lock the currently running thread first,
	// followed by a non-running thread taken from a ready or wait queue.
	spinlock_acquire(&next->lock);
#endif

	// Mark the new thread currently running.
	next->state = THREAD_RUN;

	// Mark the new thread running and context switch to it.
	// The new thread will release the lock on 'self'.
	// When this thread eventually gets run again,
	// this thread_switch call will return
	// the thread that we're context switching _from_ -
	// i.e., the thread that needs us to unlock it
	// once we're totally done with it and not using its kernel stack.
	thread_switch(self, next);
}

// Make thread t ready (runnable) and enqueue it on the ready queue.
// Caller must have acquired t->lock, and it remains locked throughout.
void
sched_ready(struct thread *t)
{
	spinlock_acquire(&readylock);
	waitqueue_enqueue(&ready, t);
	spinlock_release(&readylock);
}

// Yield the current CPU to some other ready thread, if any is ready.
// Otherwise just returns and continues running the current thread.
void
sched_yield()
{
	thread *self = thread_self();

	spinlock_acquire(&self->lock);

	// Place ourselves on the ready queue
	sched_ready(self);

	// Dispatch another thread (could just be ourself again),
	// atomically releasing self->lock in the process.
	sched_dispatch(self, THREAD_READY);
}

#endif	// LAB >= 2
