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
static void gcc_noreturn do_ret(trapframe *tf);

static void gcc_noreturn
syscall_recover(trapframe *ktf, void *recoverdata)
{
	trapframe *utf = (trapframe*)recoverdata;	// user trapframe

	cpu *c = cpu_cur();
	assert(c->recover == syscall_recover);
	c->recover = NULL;

	proc *p = proc_cur();
	spinlock_release(&p->lock);	// release lock we were holding

	// Pretend that a trap caused this process to stop.
	utf->tf_trapno = ktf->tf_trapno;	// copy trap info
	utf->tf_err = ktf->tf_err;
	utf->tf_eip -= 2;	// back up before int 0x30 syscall instruction
	do_ret(utf);
}

// Copy data to/from user space,
// recovering from any traps this causes.
static void usercopy(trapframe *utf, bool copyout,
			void *kva, uint32_t uva, size_t size)
{
	// First range-check the user area to be copied.
	if (uva >= PMAP_LINUSER || size >= PMAP_LINUSER - uva) {
		// Outside of user address space!  Simulate a GP fault.
		utf->tf_trapno = T_GPFLT;
		utf->tf_eip -= 2;	// back up before int 0x30
		do_ret(utf);
	}

	// Now do the copy, but recover from page faults.
	cpu *c = cpu_cur();
	assert(c->recover == NULL);
	c->recover = syscall_recover;

	void *ukva = (void*)PMAP_LINUSER + uva;
	if (copyout)
		memmove(ukva, kva, size);
	else
		memmove(kva, ukva, size);

	c->recover = NULL;
}
#endif	// SOL >= 3

static void
do_cputs(trapframe *tf, uint32_t cmd)
{
	// Print the string supplied by the user: pointer in EBX
#if SOL >= 3
	char buf[SYS_CPUTS_MAX+1];
	usercopy(tf, 0, buf, tf->tf_regs.reg_ebx, SYS_CPUTS_MAX);
	buf[SYS_CPUTS_MAX] = 0;	// make sure it's null-terminated
	cprintf("%s", buf);
#else	// SOL < 3
	cprintf("%s", (char*)tf->tf_regs.reg_ebx);
#endif	// SOL < 3

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

	// Since the child is now stopped, it's ours to control;
	// we no longer need our process lock -
	// and we don't want to be holding it if usercopy() below aborts.
	spinlock_release(&p->lock);

	// Put child's general register state
	if (cmd & SYS_REGS) {

		// Copy user's trapframe into child process
#if SOL >= 3
		usercopy(tf, 0, &cp->tf, tf->tf_regs.reg_ebx,
				sizeof(trapframe));
#else
		cpustate *cs = (cpustate*) tf->tf_regs.reg_ebx;
		cp->tf = cs->tf;
#endif

		// Make sure process uses user-mode segments and eflag settings
		cp->tf.tf_ds = CPU_GDT_UDATA | 3;
		cp->tf.tf_es = CPU_GDT_UDATA | 3;
		cp->tf.tf_cs = CPU_GDT_UCODE | 3;
		cp->tf.tf_ss = CPU_GDT_UDATA | 3;
		cp->tf.tf_eflags &=
			FL_CF|FL_PF|FL_AF|FL_ZF|FL_SF|FL_TF|FL_DF|FL_OF;
		cp->tf.tf_eflags |= FL_IF;	// enable interrupts
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

		// Copy child process's trapframe into user space
#if SOL >= 3
		usercopy(tf, 1, &cp->tf, tf->tf_regs.reg_ebx,
				sizeof(trapframe));
#else
		cpustate *cs = (cpustate*) tf->tf_regs.reg_ebx;
		cs->tf = cp->tf;
#endif
	}

	spinlock_release(&p->lock);

	trap_return(tf);	// syscall completed
}

static void gcc_noreturn
do_ret(trapframe *tf)
{
	proc_ret(tf);
}
#endif	// SOL >= 2

// Common function to handle all system calls -
// decode the system call type and call an appropriate handler function.
// Be sure to handle undefined system calls appropriately.
void
syscall(trapframe *tf)
{
	// EAX register holds system call command/flags
	uint32_t cmd = tf->tf_regs.reg_eax;
	switch (cmd & SYS_TYPE) {
	case SYS_CPUTS:	return do_cputs(tf, cmd);
#if SOL >= 2
	case SYS_PUT:	return do_put(tf, cmd);
	case SYS_GET:	return do_get(tf, cmd);
	case SYS_RET:	return do_ret(tf);
#else	// not SOL >= 2
	// Your implementations of SYS_PUT, SYS_GET, SYS_RET here...
#endif	// not SOL >= 2
	default:	return;		// handle as a regular trap
	}
}

#endif	// LAB >= 2
