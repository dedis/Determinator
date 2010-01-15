#if LAB >= 2
// Process management.
// See COPYRIGHT for copyright information.

#include "proc.h"


#if SOL >= 2
static spinlock readylock;	// Spinlock protecting ready queue
static proc *ready;		// Queue of procs waiting to run
#else
// LAB 2: insert your scheduling data structure declarations here.
#endif


#if LAB > 99

typedef void (*proc_start_func)(void *start_data);

// Initialize a new proc structure and its kernel stack.
void
proc_init(proc *t, proc_start_func start_func, void *start_data)
{

	// Setup the new proc's stack so that
	// the first time some CPU context switches to it,
	// it will run the caller-specified function (start_func)
	// with the caller-specified data (start_data) as an argument.
	proc_stack_init(t, start_func, start_data);
}

proc *
proc_self()
{
	XXX
}

void
proc_switch(proc *cur, proc *next)
{
	// XXX save/restore FPU/SSE state?

	// Call our assembly language helper code
	// to switch to the new proc's stack
	// and then unlock the old proc's spinlock
	// just before resuming the new proc.
	proc_stack_switch(cur, next);
}


void
sched_init(void)
{
	spinlock_init(&readylock);
	waitqueue_init(&ready);
}

// Find and dispatch another proc to run on this CPU.
// On entry, assumes 'self' points to the current proc,
// self->lock is already held,
// and caller has already put self in some non-running state:
// e.g., enqueued onto a wait queue somewhere.
// This function doesn't appear to return until self gets scheduled again -
// possibly much later, possibly on a different processor.
// self->lock is released on return.
void
sched_dispatch(struct proc *self, proc_state newstate)
{
	proc *next, *last;

	// Set the old proc's state as specified by the caller.
	// Of course the caller could just do this itself,
	// but this way the caller won't forget to.
	self->state = newstate;

	// Dequeue a proc from the ready queue, if there is one.
	spinlock_acquire(&readylock);
	next = waitqueue_dequeue(&ready);
	spinlock_release(&readylock);

	// If there are no ready procs,
	// switch to the dedicated idle proc for this CPU.
	if (next == NULL)
		next = cpu_self()->idle_proc;

	// If we're just "context switching" back to ourself, we're done.
	// It's important in this case not to try to acquire next->lock,
	// since it's the same as self->lock and we're already holding it!
	if (next == self) {
		spinlock_release(&self->lock);
		return;
	}

	// Lock the proc we'll be switching to.
	// In general, acquiring locks on two of the same type of object
	// can be risky from a deadlock perspective and must be done carefully:
	// if one proc acquires A then B while another acquires B then A,
	// then the two procs can deadlock.
	//
	// In this specific case of switching from one proc to another,
	// we rely on the facts that:
	// (a) this is the only place in the kernel
	// where we ever acquire two procs' locks at the same time, and
	// (b) we always lock the currently running proc first,
	// followed by a non-running proc taken from a ready or wait queue.
	spinlock_acquire(&next->lock);

	// Mark the new proc currently running.
	next->state = PROC_RUN;

	// That's all we needed the next proc's lock for...
	spinlock_release(&next->lock);

	// Mark the new proc running and context switch to it.
	// The new proc will release the lock on 'self'.
	// When this proc eventually gets run again,
	// this proc_switch call will return
	// the proc that we're context switching _from_ -
	// i.e., the proc that needs us to unlock it
	// once we're totally done with it and not using its kernel stack.
	proc_switch(self, next);
}

// Make proc t ready (runnable) and enqueue it on the ready queue.
// Caller must have acquired t->lock, and it remains locked throughout.
void
sched_ready(struct proc *t)
{
	spinlock_acquire(&readylock);
	waitqueue_enqueue(&ready, t);
	spinlock_release(&readylock);
}

// Yield the current CPU to some other ready proc, if any is ready.
// Otherwise just returns and continues running the current proc.
void
sched_yield()
{
	proc *self = proc_self();

	spinlock_acquire(&self->lock);

	// Place ourselves on the ready queue
	sched_ready(self);

	// Dispatch another proc (could just be ourself again),
	// atomically releasing self->lock in the process.
	sched_dispatch(self, PROC_READY);
}


////////// Wait queues //////////

void
waitqueue_enqueue(waitqueue *w, proc *t)
{
#if SOL >= 2
	// Enqueue proc t at the tail of the designated wait queue,
	// by pointing the current tail's next pointer to it.
	// If the wait queue is currently empty,
	// then w->tail_next will point at w->head,
	// so we'll actually be setting the head pointer to point to t.
	assert(*w->tail_next == NULL);
	*w->tail_next = t;

	// Update w->tail_next to point to t's own wait_next pointer,
	// in peparation for the next proc enqueued after us.
	assert(t->wait_next == NULL);
	w->tail_next = &t->wait_next;
#else
	panic("waitqueue_enqueue not implemented.");
#endif
}

proc *
waitqueue_dequeue(waitqueue *w)
{
#if SOL >= 2
	proc *t = w->head;

	if (t == NULL)
		return NULL;		// Queue is empty.

	// Dequeue and return the next waiting proc.
	w->head = t->wait_next;
	return t;
#else
	panic("waitqueue_dequeue not implemented.");
#endif
}

void
waitqueue_block(waitqueue *w, spinlock *l)
{
#if SOL >= 2
	// Find the current proc, which is the one we'll be blocking!
	proc *self = proc_self();

	// Take the spinlock protecting the proc control block,
	// while we're still holding the spinlock protecting the wait queue.
	// Thus, to avoid deadlock, wait queue spinlocks
	// must ALWAYS be acquired before proc spinlocks.
	spinlock_acquire(&self->lock);

	// Enqueue ourselves at the tail of the designated wait queue,
	// by pointing the current tail's next pointer to us.
	// (If the wait queue is currently empty,
	// then w->tail_next will point at w->head,
	// so we'll actually be setting the head pointer to point to us.)
	assert(*w->tail_next == NULL);
	*w->tail_next = self;

	// Update w->tail_next to point to our own wait_next pointer,
	// in peparation for the next proc enqueued after us.
	assert(self->wait_next == NULL);
	w->tail_next = &self->wait_next;

	// We can now release the spinlock protecting the wait queue,
	// but we must continue holding our proc lock
	// until we complete the context switch to the next proc.
	spinlock_release(l);

	// Record that we're waiting on some waitqueue.
	self->state = PROC_WAIT;

	// Find and dispatch another proc on this CPU.
	// Since we're still executing on the old proc's stack,
	// and another proc could try to wake us up at any time,
	// it would be bad if another CPU dispatched this proc
	// and started using its stack before we're totally off of it.
	// Therefore, we must keep holding the current proc's spinlock
	// until we've completely switched to the next proc's stack:
	// thus we let sched_dispatch() release self->lock for us.
	sched_yield(self);
#else
	panic("waitqueue_block not implemented.");
#endif
}

void
waitqueue_unblock_one(waitqueue *w)
{
#if SOL >= 2
	if (w->head != NULL) {
		proc *t = w->head;

		// Dequeue the proc from the waitqueue.
		w->head = t->wait_next;

		// Schedule it to be run sometime soon (possibly right now).
		sched_enqueue(t);
	}
#else
	panic("waitqueue_unblock_one not implemented.");
#endif
}

void
waitqueue_unblock_all(waitqueue *w)
{
#if SOL >= 2
	while (w->head != NULL) {
		proc *t = w->head;

		// Dequeue the proc from the waitqueue.
		w->head = t->wait_next;

		// Schedule it to be run sometime soon (possibly right now).
		sched_enqueue(t);
	}
#else
	panic("waitqueue_unblock_one not implemented.");
#endif
}

#endif	// LAB > 99

#endif // LAB >= 2
