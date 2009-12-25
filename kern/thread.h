#if LAB >= 2
/* See COPYRIGHT for copyright information. */

#ifndef PIOS_KERN_THREAD_H
#define PIOS_KERN_THREAD_H


enum thread_state {
	THREAD_RUN	= 1,
	THREAD_WAIT,
	THREAD_DONE,
};

// Thread control block structure.
// Exactly 1 page (4096 bytes) in size, including kernel stack.
struct tcb {

	struct tcb *wait_next;
	thread_state state;

	// List of forked child threads, when there are any.
	// If non-NULL, implies parent thread is stopped.
	struct tcb *children;
	struct tcb *child_next;
	int child_count;

	// List of children that are done and waiting to be rejoined to parent.
	// When child_wait_count reaches child_count, resume the parent.
	struct tcb *child_wait;
	int child_wait_count;
};


#endif // !JOS_KERN_THREAD_H
#endif // LAB >= 2
