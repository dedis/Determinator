#if LAB >= 2
/*
 * PIOS kernel thread definitions.
 * See COPYRIGHT for copyright information.
 */

#ifndef PIOS_KERN_THREAD_H
#define PIOS_KERN_THREAD_H


typedef enum thread_state {
	THREAD_READY,	= 1,	// Scheduled to run but not running now
	THREAD_RUN,		// Running on some CPU
	THREAD_WAIT,		// Waiting (blocked) for some event
	THREAD_DONE,		// Terminated and waiting to rejoin parent
} thread_state;

// Thread control block structure.
// Exactly 1 page (4096 bytes) in size, including kernel stack.
typedef struct thread {

	// Master spinlock protecting thread's state.
	spin_lock lock;

	// Kernel thread state save area.
	// Of the general-purpose registers,
	// we need to save only the callee-saved registers:
	// the C compiler will take care of the caller-saved registers
	// when it generates the function call to thread_stack_switch().
	uint32_t save_ebx;
	uint32_t save_esi;
	uint32_t save_edi;
	uint32_t save_ebp;
	uint32_t save_esp;

	// Thread queue this thread is currently sitting on, NULL if none.

	// Next pointer for any threadq this thread is sitting on.
	// Since each thread has only one queue_next pointer,
	// a thread can only sit on one threadq at a time.
	struct thread *queue_next;

	// Thread's current state, for debugging/informational purposes.
	// Only the thread itself can change its own state.
	thread_state state;

	// List of forked child threads, when there are any.
	// If non-NULL, implies parent thread is stopped.
	struct thread *children;
	struct thread *child_next;
	int child_count;

	// List of children that are done and waiting to be rejoined to parent.
	// When child_wait_count reaches child_count, resume the parent.
	struct thread *child_wait;
	int child_wait_count;
} thread;


void thread_init(thread *t);	// Initialize a new thread control block page

void thread_switch(thread *cur, thread *next);
thread *thread_self();		// Return a pointer to the current thread

inline void
thread_lock(thread *t) {
	return spinlock_acquire(t->lock);
}

inline void
thread_unlock(thread *t) {
	return spinlock_release(t->lock);
}


// All-purpose thread queues, implemented as a simple singly-linked list.
// These queues chain threads together via the threads' queue_next fields.
// Since each thread has only one queue_next pointer,
// a thread can only sit on one threadq at a time.
typedef struct threadq {
	thread *head;		// Head of list, next thread to dequeue
	thread **tail_next;	// Queue tail's next pointer, for enqueuing
} threadq;

void threadq_enqueue(threadq *w, thread *t);
thread *threadq_dequeue(threadq *w);


#endif // !JOS_KERN_THREAD_H
#endif // LAB >= 2
