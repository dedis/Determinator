#if LAB >= 2
/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/trap.h>
#include <inc/syscall.h>

#include <kern/cpu.h>
#include <kern/trap.h>
#include <kern/proc.h>
#include <kern/syscall.h>


#if SOL >= 3
static void sys_ret(trapframe *tf) gcc_noreturn;

static void gcc_noreturn
sys_recover(trapframe *ktf, void *recoverdata)
{
	trapframe *utf = (trapframe*)recoverdata;	// user trapframe

	cpu *c = cpu_cur();
	assert(c->recover == sys_recover);
	c->recover = NULL;

	proc *p = proc_cur();
	spinlock_release(&p->lock);	// release lock we were holding

	// Pretend that a trap caused this process to stop.
	utf->tf_trapno = ktf->tf_trapno;	// copy trap info
	utf->tf_err = ktf->tf_err;
	utf->tf_eip -= 2;	// back up before int 0x30 syscall instruction
	sys_ret(utf);
}
#endif	// SOL >= 3

static void
do_cputs(trapframe *tf, uint32_t cmd)
{
	// Print the string supplied by the user: pointer in EBX
	cprintf("%s", (char*)tf->tf_regs.reg_ebx);

	trap_return(tf);	// syscall completed
}
#if SOL >= 2

static void
do_put(trapframe *tf, uint32_t cmd)
{
	proc *p = proc_cur();
	assert(p->state == PROC_RUN && p->runcpu == cpu_cur());

	spinlock_acquire(&p->lock);

	// Find the named child process; create if it doesn't exist
	uint32_t cn = tf->tf_regs.reg_edx & 0xff;
	proc *cp = p->child[cn];
	if (!cp) {
		cp = proc_alloc(p, cn);
		if (!cp)	// XX handle more gracefully
			panic("sys_put: no memory for child");
	}

	// Synchronize with child if necessary.
	if (cp->state != PROC_STOP)
		proc_wait(p, cp, tf);

	// Put child's general register state
	if (cmd & SYS_REGS) {
#if SOL >= 3
		// Recover from traps caused by dereferencing user pointer
		cpu *c = cpu_cur();
		assert(c->recover == NULL);
		c->recover = sys_recover;
		c->recoverdata = tf;

#endif	// SOL >= 3
		// Copy user's trapframe into child process
		cpustate *cs = (cpustate*) tf->tf_regs.reg_ebx;
		cp->tf = cs->tf;
#if SOL >= 3

		c->recover = NULL;	// finished successfully
#endif	// SOL >= 3
	}

	// Start the child if requested
	if (cmd & SYS_START)
		proc_ready(cp);

	trap_return(tf);	// syscall completed
}

static void
do_get(trapframe *tf, uint32_t cmd)
{
	proc *p = proc_cur();
	assert(p->state == PROC_RUN && p->runcpu == cpu_cur());

	spinlock_acquire(&p->lock);

	// Find the named child process; DON'T create if it doesn't exist
	uint32_t cn = tf->tf_regs.reg_edx & 0xff;
	proc *cp = p->child[cn];
	if (!cp)
		cp = &proc_null;

	// Synchronize with child if necessary.
	if (cp->state != PROC_STOP)
		proc_wait(p, cp, tf);

	// Get child's general register state
	if (cmd & SYS_REGS) {
#if SOL >= 3
		// Recover from traps caused by dereferencing user pointer
		cpu *c = cpu_cur();
		assert(c->recover == NULL);
		c->recover = sys_recover;
		c->recoverdata = tf;

#endif	// SOL >= 3
		// Copy child process's trapframe into user space
		cpustate *cs = (cpustate*) tf->tf_regs.reg_ebx;
		cs->tf = cp->tf;
#if SOL >= 3

		c->recover = NULL;	// finished successfully
#endif	// SOL >= 3
	}

	trap_return(tf);	// syscall completed
}

static void gcc_noreturn
do_ret(trapframe *tf, uint32_t cmd)
{
	proc *cp = proc_cur();		// we're the child
	assert(cp->state == PROC_RUN && cp->runcpu == cpu_cur());
	proc *p = cp->parent;		// find our parent

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

}
#endif	// SOL >= 2

// Common function to handle all system calls -
// decode the system call type and call an appropriate handler function.
// Be sure to handle undefined system calls appropriately.
void
syscall(trapframe *tf)
{
#if SOL >= 2
	// EAX register holds system call command/flags
	uint32_t cmd = tf->tf_regs.reg_eax;
	switch (cmd & SYS_TYPE) {
	case SYS_CPUTS:	return do_cputs(tf, cmd);
	case SYS_PUT:	return do_put(tf, cmd);
	case SYS_GET:	return do_get(tf, cmd);
	case SYS_RET:	return do_ret(tf, cmd);
	default:	return;		// handle as a regular trap
	}
#else	// not SOL >= 2
	panic("syscall not implemented");
#endif	// not SOL >= 2
}

#endif	// LAB >= 2
