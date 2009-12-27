#if LAB >= 2
/*
 * PIOS kernel thread implementation.
 * See COPYRIGHT for copyright information.
 */

#include "thread.h"


////////// Threads //////////

typedef void (*thread_start_func)(void *start_data);

// Initialize a new thread structure and its kernel stack.
void
thread_init(thread *t, thread_start_func start_func, void *start_data)
{

	// Setup the new thread's stack so that
	// the first time some CPU context switches to it,
	// it will run the caller-specified function (start_func)
	// with the caller-specified data (start_data) as an argument.
	thread_stack_init(t, start_func, start_data);
}

thread *
thread_self()
{
	XXX
}

void
thread_switch(thread *cur, thread *next)
{
	// XXX save/restore FPU/SSE state?

	// Call our assembly language helper code
	// to switch to the new thread's stack
	// and then unlock the old thread's spinlock
	// just before resuming the new thread.
	thread_stack_switch(cur, next);
}


////////// Wait queues //////////

void
waitqueue_enqueue(waitqueue *w, thread *t)
{
#if SOL >= 2
	// Enqueue thread t at the tail of the designated wait queue,
	// by pointing the current tail's next pointer to it.
	// If the wait queue is currently empty,
	// then w->tail_next will point at w->head,
	// so we'll actually be setting the head pointer to point to t.
	assert(*w->tail_next == NULL);
	*w->tail_next = t;

	// Update w->tail_next to point to t's own wait_next pointer,
	// in peparation for the next thread enqueued after us.
	assert(t->wait_next == NULL);
	w->tail_next = &t->wait_next;
#else
	panic("waitqueue_enqueue not implemented.");
#endif
}

thread *
waitqueue_dequeue(waitqueue *w)
{
#if SOL >= 2
	thread *t = w->head;

	if (t == NULL)
		return NULL;		// Queue is empty.

	// Dequeue and return the next waiting thread.
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
	// Find the current thread, which is the one we'll be blocking!
	thread *self = thread_self();

	// Take the spinlock protecting the thread control block,
	// while we're still holding the spinlock protecting the wait queue.
	// Thus, to avoid deadlock, wait queue spinlocks
	// must ALWAYS be acquired before thread spinlocks.
	spinlock_acquire(&self->lock);

	// Enqueue ourselves at the tail of the designated wait queue,
	// by pointing the current tail's next pointer to us.
	// (If the wait queue is currently empty,
	// then w->tail_next will point at w->head,
	// so we'll actually be setting the head pointer to point to us.)
	assert(*w->tail_next == NULL);
	*w->tail_next = self;

	// Update w->tail_next to point to our own wait_next pointer,
	// in peparation for the next thread enqueued after us.
	assert(self->wait_next == NULL);
	w->tail_next = &self->wait_next;

	// We can now release the spinlock protecting the wait queue,
	// but we must continue holding our thread lock
	// until we complete the context switch to the next thread.
	spinlock_release(l);

	// Record that we're waiting on some waitqueue.
	self->state = THREAD_WAIT;

	// Find and dispatch another thread on this CPU.
	// Since we're still executing on the old thread's stack,
	// and another thread could try to wake us up at any time,
	// it would be bad if another CPU dispatched this thread
	// and started using its stack before we're totally off of it.
	// Therefore, we must keep holding the current thread's spinlock
	// until we've completely switched to the next thread's stack:
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
		thread *t = w->head;

		// Dequeue the thread from the waitqueue.
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
		thread *t = w->head;

		// Dequeue the thread from the waitqueue.
		w->head = t->wait_next;

		// Schedule it to be run sometime soon (possibly right now).
		sched_enqueue(t);
	}
#else
	panic("waitqueue_unblock_one not implemented.");
#endif
}


#endif // LAB >= 2
