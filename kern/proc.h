#if LAB >= 2
// Process definitions.
// See COPYRIGHT for copyright information.
#ifndef PIOS_KERN_PROC_H
#define PIOS_KERN_PROC_H
#ifndef PIOS_KERNEL
# error "This is a kernel header; user programs should not #include it"
#endif


#define PROC_CHILDREN	256	// Max # of children a process can have

typedef enum proc_state {
	PROC_STOP	= 0,	// Passively waiting for parent to run it
	PROC_READY,		// Scheduled to run but not running now
	PROC_RUN,		// Running on some CPU
	PROC_EXIT,		// Waiting for parent to collect results
} proc_state;

// Thread control block structure.
// Consumes 1 physical memory page, though we don't use all of it.
typedef struct proc {

	// Master spinlock protecting proc's state.
	spin_lock	lock;

	// Process hierarchy information.
	struct proc	*parent;
	struct proc	*child[PROC_CHILDREN];

	// Scheduling state of this process.
	proc_state	state;		// current state
	struct proc	*readynext;	// chain on ready queue
	struct cpu	*runcpu;	// cpu we're running on if running

	// Save area for user-mode register state when proc is not running.
	trapframe	tf;
} proc;

#define proc_cur()	(cpu_cur()->proc)


void proc_init(proc *t);	// Initialize a new proc control block page

void proc_switch(proc *cur, proc *next);
proc *proc_self();		// Return a pointer to the current proc

void procq_enqueue(procq *w, proc *t);
proc *procq_dequeue(procq *w);


void sched_dispatch(struct proc *self); // Dispatch a ready proc
void sched_ready(struct proc *t);	// Schedule proc t to run
void proc_yield();			// Yield to some other ready proc



#endif // !PIOS_KERN_PROC_H
#endif // LAB >= 2
