#if LAB >= 2
/*
 * PIOS kernel thread definitions.
 * See COPYRIGHT for copyright information.
 */

#ifndef PIOS_KERN_THREAD_H
#define PIOS_KERN_THREAD_H


typedef enum thread_state {
	THREAD_READY,	= 1,	// Scheduled to run but not running
	THREAD_RUN,		// Running on some CPU
	THREAD_WAIT,		// Waiting on some waitqueue
	THREAD_DONE,		// Terminated and waiting to rejoin parent
} thread_state;

// Thread control block structure.
// Exactly 1 page (4096 bytes) in size, including kernel stack.
typedef struct thread {
	struct spinlock lock;

	// Kernel thread state save area.
	// Of the general-purpose registers, we only need
	uint32_t save_ebx;
	uint32_t save_esi;
	uint32_t save_edi;
	uint32_t save_ebp;
	uint32_t save_esp;

	struct thread *wait_next;

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


// Thread wait queues.
typedef struct waitqueue {
	thread *head;		// Head of list, next thread to dequeue
	thread **tail_next;	// Queue tail's next pointer, for enqueuing
} waitqueue;



void waitqueue_enqueue(waitqueue *w, thread *t);
thread *waitqueue_dequeue(waitqueue *w);

void waitqueue_block(waitqueue *w, spinlock *l);
void waitqueue_unblock_one(waitqueue *w);
void waitqueue_unblock_all(waitqueue *w);



#endif // !JOS_KERN_THREAD_H
#endif // LAB >= 2
