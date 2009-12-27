#if LAB >= 2
/* See COPYRIGHT for copyright information. */

#ifndef PIOS_KERN_SCHED_H
#define PIOS_KERN_SCHED_H
#ifndef JOS_KERNEL
# error "This is a JOS kernel header; user programs should not #include it"
#endif

void sched_init(void);			// Initialize the scheduler on startup
void sched_dispatch(struct thread *self); // Dispatch a ready thread
void sched_ready(struct thread *t);	// Schedule thread t to run
void sched_yield();			// Yield to some other ready thread

#endif	// !PIOS_KERN_SCHED_H
#endif	// LAB >= 2
