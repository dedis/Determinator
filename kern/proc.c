#if LAB >= 2
// Process management.
// See COPYRIGHT for copyright information.

#include <inc/string.h>

#include <kern/cpu.h>
#include <kern/mem.h>
#include <kern/trap.h>
#include <kern/proc.h>


proc proc_null;		// null process - just leave it initialized to 0

#if SOL >= 2
static spinlock readylock;	// Spinlock protecting ready queue
static proc *readyhead;		// Head of ready queue
static proc **readytail;	// Tail of ready queue
#else
// LAB 2: insert your scheduling data structure declarations here.
#endif


void
proc_init(void)
{
	if (!cpu_onboot())
		return;

#if SOL >= 2
	spinlock_init(&readylock, "readylock");
	readytail = &readyhead;
#else
	// your module initialization code here
#endif
}

// Allocate and initialize a new proc as child 'cn' of parent 'p'.
// Returns NULL if no physical memory available.
proc *
proc_alloc(proc *p, uint32_t cn)
{
	pageinfo *pi = mem_alloc();
	if (!pi)
		return NULL;

	proc *cp = (proc*)mem_pi2ptr(pi);
	memset(cp, 0, sizeof(proc));
	spinlock_init(&cp->lock, "proc lock");
	cp->parent = p;
	cp->state = PROC_STOP;

	// register state
	cp->tf.tf_ds = CPU_GDT_UDATA | 3;
	cp->tf.tf_es = CPU_GDT_UDATA | 3;
	cp->tf.tf_cs = CPU_GDT_UCODE | 3;
	cp->tf.tf_ss = CPU_GDT_UDATA | 3;

#if SOL >= 3
	// XXX alloc page directory
#endif	// SOL >= 3

	p->child[cn] = cp;
	return cp;
}

// Put process p in the ready state and add it to the ready queue.
void
proc_ready(proc *p)
{
#if SOL >= 2
	spinlock_acquire(&readylock);

	p->state = PROC_READY;
	p->readynext = NULL;
	*readytail = p;
	readytail = &p->readynext;

	spinlock_release(&readylock);
#else	// SOL >= 2
	panic("proc_ready not implemented");
#endif	// SOL >= 2
}

// Go to sleep waiting for a given child process to finish running.
// Parent process 'p' must be running and locked on entry.
// The supplied trapframe represents p's register state on syscall entry.
void gcc_noreturn
proc_wait(proc *p, proc *cp, trapframe *tf)
{
#if SOL >= 2
	assert(spinlock_holding(&p->lock));
	assert(cp && cp != &proc_null);	// null proc is always stopped
	assert(cp->state != PROC_STOP);

	p->state = PROC_WAIT;
	p->runcpu = NULL;
	p->waitchild = cp;	// remember what child we're waiting on
	p->tf = *tf;		// save our register state
	p->tf.tf_eip -= 2;	// back up to replay int instruction

	spinlock_release(&p->lock);

	proc_sched();
#else	// SOL >= 2
	panic("proc_wait not implemented");
#endif	// SOL >= 2
}

void gcc_noreturn
proc_sched(void)
{
#if SOL >= 2
	// Spin until something appears on the ready list.
	// Would be better to use the hlt instruction and really go idle,
	// but then we'd have to deal with inter-processor interrupts (IPIs).
	while (1) {
		while (!readyhead)
			;		// spin until something appears

		spinlock_acquire(&readylock);
		if (readyhead)
			break;		// while holding lock
		spinlock_release(&readylock);
	}

	// Remove the next proc from the ready queue
	proc *p = readyhead;
	readyhead = p->readynext;
	if (readytail == &p->readynext) {
		assert(readyhead == NULL);	// ready queue going empty
		readytail = &readyhead;
	}
	p->readynext = NULL;

	spinlock_acquire(&p->lock);
	spinlock_release(&readylock);

	proc_run(p);

#else	// SOL >= 2
	panic("proc_sched not implemented");
#endif	// SOL >= 2
}

// Switch to and run a specified process, which must already be locked.
void gcc_noreturn
proc_run(proc *p)
{
#if SOL >= 2
	assert(spinlock_holding(&p->lock));

	// Put it in running state
	p->state = PROC_RUN;
	p->runcpu = cpu_cur();

	spinlock_release(&p->lock);

#if SOL >= 3
	XXX change PDBR
#endif

	trap_return(&p->tf);
#else	// SOL >= 2
	panic("proc_run not implemented");
#endif	// SOL >= 2
}


#endif // LAB >= 2
