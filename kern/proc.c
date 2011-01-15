#if LAB >= 2
/*
 * PIOS process management.
 *
 * Copyright (C) 2010 Yale University.
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Primary author: Bryan Ford
 */

#include <inc/string.h>
#include <inc/syscall.h>

#include <kern/cpu.h>
#include <kern/mem.h>
#include <kern/trap.h>
#include <kern/proc.h>
#include <kern/init.h>
#if LAB >= 4
#include <kern/file.h>
#endif
#if LAB >= 5
#include <kern/net.h>
#endif

#if SOL >= 2
#include <dev/pmc.h>
#endif


proc proc_null;		// null process - just leave it initialized to 0

proc *proc_root;	// root process, once it's created in init()

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
	mem_incref(pi);

	proc *cp = (proc*)mem_pi2ptr(pi);
	memset(cp, 0, sizeof(proc));
	spinlock_init(&cp->lock);
	cp->parent = p;
	cp->state = PROC_STOP;
#if LAB >= 5
	cp->home = RRCONS(net_node, mem_phys(cp), 0);
#endif	// LAB >= 5

	// Integer register state
#if LAB >= 9
	cp->sv.tf.gs = CPU_GDT_UDTLS | 3;
	cp->sv.tf.fs = 0;
#endif
	cp->sv.tf.ds = CPU_GDT_UDATA | 3;
	cp->sv.tf.es = CPU_GDT_UDATA | 3;
	cp->sv.tf.cs = CPU_GDT_UCODE | 3;
	cp->sv.tf.ss = CPU_GDT_UDATA | 3;
#if SOL >= 2

	// Floating-point register state
	cp->sv.fx.fcw = 0x037f;	// round-to-nearest, 80-bit prec, mask excepts
	cp->sv.fx.mxcsr = 0x00001f80;	// all MMX exceptions masked
#endif

#if SOL >= 3
	// Allocate a page directory for this process
	cp->pdir = pmap_newpdir();
	cp->rpdir = pmap_newpdir();
	if (!cp->pdir || !cp->rpdir) {
		if (cp->pdir) pmap_freepdir(mem_ptr2pi(cp->pdir));
		return NULL;
	}
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

// Save the current process's state before switching to another process.
// Copies trapframe 'tf' into the proc struct,
// and saves any other relevant state such as FPU state.
// The 'entry' parameter is one of:
//	-1	if we entered the kernel via a trap before executing an insn
//	0	if we entered via a syscall and must abort/rollback the syscall
//	1	if we entered via a syscall and are completing the syscall
void
proc_save(proc *p, trapframe *tf, int entry)
{
#if SOL >= 2
	assert(p == proc_cur());

	if (tf != &p->sv.tf)
		p->sv.tf = *tf;		// integer register state
	if (entry == 0)
		p->sv.tf.eip -= 2;	// back up to replay INT instruction
#if LAB >= 9

	if (p->sv.pff & PFF_USEFPU) {	// FPU state
		assert(sizeof(p->sv.fx) == 512);
		asm volatile("fxsave %0" : "=m" (p->sv.fx));
		lcr0(rcr0() | CR0_TS);	// re-disable FPU
	}

	if (p->sv.pff & PFF_ICNT) {	// Instruction counting/recovery
		//cprintf("proc_save tf %x -> proc %x\n", tf, &p->sv.tf);
		if (p->sv.tf.eflags & FL_TF) {	// single stepping
			if (entry > 0)
				p->sv.icnt++;	// executed the INT insn
			p->sv.tf.eflags &= ~FL_TF;
			p->sv.tf.eflags |= FL_IF;
		} else if (p->pmcmax > 0) {	// using performance counters
			assert(pmc_get != NULL);
			p->sv.icnt += pmc_get(p->pmcmax);
			if (entry == 0)
				p->sv.icnt--;	// don't count aborted INT insn
			p->pmcmax = 0;
			if (p->sv.icnt > p->sv.imax)
				panic("oops, perf ctr overshoot by %d insns\n",
					p->sv.icnt - p->sv.imax);
		}
		assert(p->sv.icnt <= p->sv.imax);
	}
	assert(!(p->sv.tf.eflags & FL_TF));
	assert(p->pmcmax == 0);
#endif	// LAB >= 9
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
	proc_save(p, tf, 0);	// save process state before INT instruction

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
	cpu *c = cpu_cur();
	spinlock_acquire(&readylock);
	while (!readyhead || cpu_disabled(c)) {
		spinlock_release(&readylock);

		//cprintf("cpu %d waiting for work\n", cpu_cur()->id);
		while (!readyhead || cpu_disabled(c)) {	// spin-wait for work
			sti();		// enable device interrupts briefly
			pause();	// let CPU know we're in a spin loop
			cli();		// disable interrupts again
		}
		//cprintf("cpu %d found work\n", cpu_cur()->id);

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

#if LAB >= 9
	if (p->sv.pff & PFF_USEFPU) {	// FPU state
		assert(sizeof(p->sv.fx) == 512);
		lcr0(rcr0() & ~CR0_TS);	// enable FPU
		asm volatile("fxrstor %0" : : "m" (p->sv.fx));
	}

	assert(!(p->sv.tf.eflags & FL_TF));
	assert(p->pmcmax == 0);
	if (p->sv.pff & PFF_ICNT) {	// Instruction counting/recovery
		//cprintf("proc_run proc %x\n", &p->sv.tf);
		if (p->sv.icnt >= p->sv.imax) {
			warn("proc_run: icnt expired");
			p->sv.tf.trapno = T_ICNT;
			proc_ret(&p->sv.tf, -1);	// can't run any insns!
		}
		assert(p->pmcmax == 0);
		int32_t pmax = p->sv.imax - p->sv.icnt - pmc_safety;
		if (pmc_set != NULL && pmax > 0) {
			assert(p->sv.tf.eflags & FL_IF);
			assert(!(p->sv.tf.eflags & FL_TF));
			pmc_set(pmax);
			p->pmcmax = pmax;
		} else {
			p->sv.tf.eflags |= FL_TF;	// just single-step
			// XXX taking hardware interrupts while tracing
			// messes up our ability to count properly.
			// Ideally we should poll for pending interrupts
			// after each instruction we trace.
			p->sv.tf.eflags &= ~FL_IF;
		}
	} else {
		assert(!(p->sv.tf.eflags & FL_TF));
		assert(p->pmcmax == 0);
	}
#endif	// LAB >= 9
#if SOL >= 3
	// Switch to the new process's address space.
	lcr3(mem_phys(p->pdir));

#endif
	trap_return(&p->sv.tf);
#else	// SOL >= 2
	panic("proc_run not implemented");
#endif	// SOL >= 2
}

// Yield the current CPU to another ready process.
// Called while handling a timer interrupt.
void gcc_noreturn
proc_yield(trapframe *tf)
{
#if SOL >= 2
	proc *p = proc_cur();
	assert(p->runcpu == cpu_cur());
	p->runcpu = NULL;	// this process no longer running
	proc_save(p, tf, -1);	// save this process's state
	proc_ready(p);		// put it on tail of ready queue

	proc_sched();		// schedule a process from head of ready queue
#else
	panic("proc_yield not implemented");
#endif	// SOL >= 2
}

// Put the current process to sleep by "returning" to its parent process.
// Used both when a process calls the SYS_RET system call explicitly,
// and when a process causes an unhandled trap in user mode.
// The 'entry' parameter is as in proc_save().
void gcc_noreturn
proc_ret(trapframe *tf, int entry)
{
#if SOL >= 2
	proc *cp = proc_cur();		// we're the child
	assert(cp->state == PROC_RUN && cp->runcpu == cpu_cur());

#if SOL >= 5
	// First migrate to our home node if we're not already there.
	if (net_node != RRNODE(cp->home)) {
		if (entry > 0) entry = 0;	// abort syscall
		net_migrate(tf, RRNODE(cp->home), entry);
	}

#endif
	proc *p = cp->parent;		// find our parent
	if (p == NULL) {		// "return" from root process!
		if (tf->trapno != T_SYSCALL) {
			trap_print(tf);
			panic("trap in root process");
		}
#if LAB >= 4
		// Allow the root process to do I/O via its special files.
		// May put the root process to sleep waiting for input.
		assert(entry == 1);
		file_io(tf);
#else	// LAB < 4
		cprintf("root process terminated\n");
		done();
#endif	// LAB < 4
	}

	spinlock_acquire(&p->lock);	// lock both in proper order

	cp->state = PROC_STOP;		// we're becoming stopped
	cp->runcpu = NULL;		// no longer running
	proc_save(cp, tf, entry);	// save process state after INT insn

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

static struct procstate child_state;
static char gcc_aligned(16) child_stack[4][PAGESIZE];

static volatile uint32_t pingpong = 0;
static void *recovargs;

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
		child_state.tf.eip = (uint32_t) child;
		child_state.tf.esp = (uint32_t) esp;

		// Use PUT syscall to create each child,
		// but only start the first 2 children for now.
		cprintf("spawning child %d\n", i);
		sys_put(SYS_REGS | (i < 2 ? SYS_START : 0), i, &child_state,
			NULL, NULL, 0);
	}

	// Wait for both children to complete.
	// This should complete without preemptive scheduling
	// when we're running on a 2-processor machine.
	for (i = 0; i < 2; i++) {
		cprintf("waiting for child %d\n", i);
		sys_get(SYS_REGS, i, &child_state, NULL, NULL, 0);
	}
	cprintf("proc_check() 2-child test succeeded\n");

	// (Re)start all four children, and wait for them.
	// This will require preemptive scheduling to complete
	// if we have less than 4 CPUs.
	cprintf("proc_check: spawning 4 children\n");
	for (i = 0; i < 4; i++) {
		cprintf("spawning child %d\n", i);
		sys_put(SYS_START, i, NULL, NULL, NULL, 0);
	}

	// Wait for all 4 children to complete.
	for (i = 0; i < 4; i++)
		sys_get(0, i, NULL, NULL, NULL, 0);
	cprintf("proc_check() 4-child test succeeded\n");

	// Now do a trap handling test using all 4 children -
	// but they'll _think_ they're all child 0!
	// (We'll lose the register state of the other children.)
	i = 0;
	sys_get(SYS_REGS, i, &child_state, NULL, NULL, 0);
		// get child 0's state
	assert(recovargs == NULL);
	do {
		sys_put(SYS_REGS | SYS_START, i, &child_state, NULL, NULL, 0);
		sys_get(SYS_REGS, i, &child_state, NULL, NULL, 0);
		if (recovargs) {	// trap recovery needed
			trap_check_args *args = recovargs;
			cprintf("recover from trap %d\n",
				child_state.tf.trapno);
			child_state.tf.eip = (uint32_t) args->reip;
			args->trapno = child_state.tf.trapno;
		} else
			assert(child_state.tf.trapno == T_SYSCALL);
		i = (i+1) % 4;	// rotate to next child proc
	} while (child_state.tf.trapno != T_SYSCALL);
	assert(recovargs == NULL);

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
		assert(recovargs == NULL);
		trap_check(&recovargs);
		assert(recovargs == NULL);
		sys_ret();
	}

	panic("child(): shouldn't have gotten here");
}

static void grandchild(int n)
{
	panic("grandchild(): shouldn't have gotten here");
}

#endif // LAB >= 2
