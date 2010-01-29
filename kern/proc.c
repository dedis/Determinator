#if LAB >= 2
// Process management.
// See COPYRIGHT for copyright information.

#include <inc/string.h>
#include <inc/syscall.h>

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
	spinlock_init(&readylock);
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
	spinlock_init(&cp->lock);
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

	if (p)
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
	spinlock_acquire(&readylock);
	while (!readyhead) {
		spinlock_release(&readylock);

		cprintf("cpu %d waiting for work\n", cpu_cur()->id);
		while (!readyhead)	// spin until something appears
			pause();	// let CPU know we're in a spin loop

		spinlock_acquire(&readylock);
		// now must recheck readyhead while holding readylock!
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
	cpu *c = cpu_cur();
	p->state = PROC_RUN;
	p->runcpu = c;
	c->proc = p;

	spinlock_release(&p->lock);

#if SOL >= 3
	XXX change PDBR
#endif

	trap_return(&p->tf);
#else	// SOL >= 2
	panic("proc_run not implemented");
#endif	// SOL >= 2
}

// Yield the current CPU to another ready process.
void gcc_noreturn
proc_yield(trapframe *tf)
{
#if SOL >= 2
	proc *p = proc_cur();
	assert(p->runcpu == cpu_cur());
	p->runcpu = NULL;	// this process no longer running
	p->tf = *tf;		// save this process's register state

	proc_ready(p);		// put it on tail of ready queue

	proc_sched();		// schedule a process from head of ready queue
#else
	panic("proc_yield not implemented");
#endif	// SOL >= 2
}

// Put the current process to sleep by "returning" to its parent process.
// Used both when a process calls the SYS_RET system call explicitly,
// and when a process causes an unhandled trap in user mode.
void gcc_noreturn
proc_ret(trapframe *tf)
{
#if SOL >= 2
	proc *cp = proc_cur();		// we're the child
	assert(cp->state == PROC_RUN && cp->runcpu == cpu_cur());
	proc *p = cp->parent;		// find our parent
	assert(p != NULL);		// root process shouldn't return!

	spinlock_acquire(&p->lock);	// lock both in proper order

	cp->state = PROC_STOP;		// we're becoming stopped
	cp->runcpu = NULL;		// no longer running
	cp->tf = *tf;			// save our register state

	// If parent is waiting to sync with us, wake it up.
	if (p->state == PROC_WAIT && p->waitchild == cp) {
		p->waitchild = NULL;
		proc_run(p);
	}

	spinlock_release(&p->lock);
	proc_sched();			// find and run someone else
#else
	panic("proc_ret not implemented");
#endif	// SOL >= 2
}

// Helper functions for proc_check()
static void child(int n);
static void grandchild(int n);

static struct cpustate child_state;
static char gcc_aligned(16) child_stack[4][PAGESIZE];

static volatile uint32_t pingpong = 0;
static void *recoveip;

void
proc_check(void)
{
	// Spawn 2 child processes, executing on statically allocated stacks.

	int i;
	for (i = 0; i < 4; i++) {
		// Setup register state for child
		uint32_t *esp = (uint32_t*) &child_stack[i][PAGESIZE];
		*--esp = i;	// push argument to child() function
		*--esp = 0;	// fake return address
		child_state.tf.tf_eip = (uint32_t) child;
		child_state.tf.tf_esp = (uint32_t) esp;

		// Use PUT syscall to create each child,
		// but only start the first 2 children for now.
		cprintf("spawning child %d\n", i);
		sys_put(SYS_REGS | (i < 2 ? SYS_START : 0), i, &child_state);
	}

	// Wait for both children to complete.
	// This should complete without preemptive scheduling
	// when we're running on a 2-processor machine.
	for (i = 0; i < 2; i++) {
		cprintf("waiting for child %d\n", i);
		sys_get(SYS_REGS, i, &child_state);
	}
	cprintf("proc_check() 2-child test succeeded\n");

	// (Re)start all four children, and wait for them.
	// This will require preemptive scheduling to complete
	// if we have less than 4 CPUs.
	cprintf("proc_check: spawning 4 children\n");
	for (i = 0; i < 4; i++) {
		cprintf("spawning child %d\n", i);
		sys_put(SYS_START, i, NULL);
	}

	// Wait for all 4 children to complete.
	for (i = 0; i < 4; i++)
		sys_get(0, i, NULL);
	cprintf("proc_check() 4-child test succeeded\n");

	// Now do a trap handling test using all 4 children -
	// but they'll _think_ they're all child 0!
	// (We'll lose the register state of the other children.)
	i = 0;
	sys_get(SYS_REGS, i, &child_state);	// get child 0's state
	assert(recoveip == NULL);
	do {
		sys_put(SYS_REGS | SYS_START, i, &child_state); // start child
		sys_get(SYS_REGS, i, &child_state);	// wait and get state
		if (recoveip) {	// set eip to the recovery point
			cprintf("recover from trap %d\n",
				child_state.tf.tf_trapno);
			child_state.tf.tf_eip = (uint32_t)recoveip;
		} else
			assert(child_state.tf.tf_trapno == T_SYSCALL);
		i = (i+1) % 4;	// rotate to next child proc
	} while (child_state.tf.tf_trapno != T_SYSCALL);

	cprintf("proc_check() trap reflection test succeeded\n");

	cprintf("proc_check() succeeded!\n");
}

static void child(int n)
{
	// Only first 2 children participate in first pingpong test
	if (n < 2) {
		int i;
		for (i = 0; i < 10; i++) {
			cprintf("in child %d count %d\n", n, i);
			while (pingpong != n)
				pause();
			xchg(&pingpong, !pingpong);
		}
		sys_ret();
	}

	// Second test, round-robin pingpong between all 4 children
	int i;
	for (i = 0; i < 10; i++) {
		cprintf("in child %d count %d\n", n, i);
		while (pingpong != n)
			pause();
		xchg(&pingpong, (pingpong + 1) % 4);
	}
	sys_ret();

	// Only "child 0" (or the proc that thinks it's child 0), trap check...
	if (n == 0) {
		trap_check(&child_state.tf, &recoveip);

		recoveip = NULL;	// return to parent cleanly
		sys_ret();
	}

	panic("child(): shouldn't have gotten here");
}

static void grandchild(int n)
{
	panic("grandchild(): shouldn't have gotten here");
}

#endif // LAB >= 2
