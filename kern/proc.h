#if LAB >= 2
// Process definitions.
// See COPYRIGHT for copyright information.
#ifndef PIOS_KERN_PROC_H
#define PIOS_KERN_PROC_H
#ifndef PIOS_KERNEL
# error "This is a kernel header; user programs should not #include it"
#endif

#include <inc/trap.h>

#include <kern/spinlock.h>
#if LAB >= 3
#include <kern/pmap.h>
#endif


#define PROC_CHILDREN	256	// Max # of children a process can have

typedef enum proc_state {
	PROC_STOP	= 0,	// Passively waiting for parent to run it
	PROC_READY,		// Scheduled to run but not running now
	PROC_RUN,		// Running on some CPU
	PROC_WAIT,		// Waiting to synchronize with child
} proc_state;

// Thread control block structure.
// Consumes 1 physical memory page, though we don't use all of it.
typedef struct proc {

	// Master spinlock protecting proc's state.
	spinlock	lock;

	// Process hierarchy information.
	struct proc	*parent;
	struct proc	*child[PROC_CHILDREN];

	// Scheduling state for this process.
	proc_state	state;		// current state
	struct proc	*readynext;	// chain on ready queue
	struct cpu	*runcpu;	// cpu we're running on if running
	struct proc	*waitchild;	// child proc if waiting for child

	// Save area for user-mode register state when proc is not running.
	trapframe	tf;		// general registers
	fxsave		fx;		// FPU/MMX/XMM state

#if LAB >= 3
	// Virtual memory state for this process.
	pde_t		*pdir;		// Working page directory
	pde_t		*oldpd;		// Snapshot from last Put, NULL if Got
#endif	// LAB >= 3
} proc;

#define proc_cur()	(cpu_cur()->proc)


// Special "null process" - always just contains zero in all fields.
extern proc proc_null;


void proc_init(void);	// Initialize process management code
proc *proc_alloc(proc *p, uint32_t cn);	// Allocate new child
void proc_ready(proc *p);	// Make process p ready
void proc_wait(proc *p, proc *cp, trapframe *tf) gcc_noreturn;
void proc_sched(void) gcc_noreturn;	// Find and run some ready process
void proc_run(proc *p) gcc_noreturn;	// Run a specific process
void proc_yield(trapframe *tf) gcc_noreturn;	// Yield to another process
void proc_ret(trapframe *tf) gcc_noreturn;	// Return to parent process
void proc_check(void);			// Check process code


#endif // !PIOS_KERN_PROC_H
#endif // LAB >= 2
