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
#if LAB >= 5
#include <kern/net.h>
#endif

#if SOL >= 4
#include <dev/timer.h>
#endif


#if SOL >= 3
static void gcc_noreturn do_ret(trapframe *tf);
#endif


// This bit mask defines the eflags bits user code is allowed to set.
#define FL_USER		(FL_CF|FL_PF|FL_AF|FL_ZF|FL_SF|FL_DF|FL_OF)


// During a system call, generate a specific processor trap -
// as if the user code's INT 0x30 instruction had caused it -
// and reflect the trap to the parent process as with other traps.
static void gcc_noreturn
systrap(trapframe *utf, int trapno, int err)
{
#if SOL >= 3
	//cprintf("systrap: reflect trap %d to parent process\n", trapno);
	utf->tf_trapno = trapno;
	utf->tf_err = err;
	proc_ret(utf, 0);	// abort syscall insn and return to parent
#else
	panic("systrap() not implemented.");
#endif
}

// Recover from a trap that occurs during a copyin or copyout,
// by aborting the system call and reflecting the trap to the parent process,
// behaving as if the user program's INT instruction had caused the trap.
// This uses the 'recover' pointer in the current cpu struct,
// and invokes systrap() above to blame the trap on the user process.
//
// Notes:
// - Be sure the parent gets the correct tf_trapno, tf_err, and tf_eip values.
// - Be sure to release any spinlocks you were holding during the copyin/out.
//
static void gcc_noreturn
sysrecover(trapframe *ktf, void *recoverdata)
{
#if SOL >= 3
	trapframe *utf = (trapframe*)recoverdata;	// user trapframe

	cpu *c = cpu_cur();
	assert(c->recover == sysrecover);
	c->recover = NULL;

	// Pretend that a trap caused this process to stop.
	systrap(utf, ktf->tf_trapno, ktf->tf_err);
#else
	panic("sysrecover() not implemented.");
#endif
}

// Check a user virtual address block for validity:
// i.e., make sure the complete area specified lies in
// the user address space between VM_USERLO and VM_USERHI.
// If not, abort the syscall by sending a T_GPFLT to the parent,
// again as if the user program's INT instruction was to blame.
//
// Note: Be careful that your arithmetic works correctly
// even if size is very large, e.g., if uva+size wraps around!
//
static void checkva(trapframe *utf, uint32_t uva, size_t size)
{
#if SOL >= 3
	if (uva < VM_USERLO || uva >= VM_USERHI
			|| size >= VM_USERHI - uva) {

		// Outside of user address space!  Simulate a page fault.
		systrap(utf, T_PGFLT, 0);
	}
#else
	panic("checkva() not implemented.");
#endif
}

// Copy data to/from user space,
// using checkva() above to validate the address range
// and using sysrecover() to recover from any traps during the copy.
void usercopy(trapframe *utf, bool copyout,
			void *kva, uint32_t uva, size_t size)
{
	checkva(utf, uva, size);

	// Now do the copy, but recover from page faults.
#if SOL >= 3
	cpu *c = cpu_cur();
	assert(c->recover == NULL);
	c->recover = sysrecover;

	//pmap_inval(proc_cur()->pdir, VM_USERLO, VM_USERHI-VM_USERLO);

	if (copyout)
		memmove((void*)uva, kva, size);
	else
		memmove(kva, (void*)uva, size);

	assert(c->recover == sysrecover);
	c->recover = NULL;
#else
	panic("syscall_usercopy() not implemented.");
#endif
}

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
cprintf("PUT proc %x eip %x esp %x cmd %x\n", p, tf->tf_eip, tf->tf_esp, cmd);

#if SOL >= 5
	// First migrate if we need to.
	uint8_t node = (tf->tf_regs.reg_edx >> 8) & 0xff;
	if (node == 0) node = RRNODE(p->home);		// Goin' home
	if (node != net_node)
		net_migrate(tf, node, 0);	// abort syscall and migrate

#endif // SOL >= 5
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
		int len = offsetof(cpustate, fx);	// just integer regs
		if (cmd & SYS_FPU) len = sizeof(cpustate); // whole shebang

		// Copy user's trapframe into child process
#if SOL >= 3
		usercopy(tf, 0, &cp->sv, tf->tf_regs.reg_ebx, len);
#else
		cpustate *cs = (cpustate*) tf->tf_regs.reg_ebx;
		memcpy(cp->sv, cs, len);
#endif

		// Make sure process uses user-mode segments and eflag settings
		cp->sv.tf.tf_ds = CPU_GDT_UDATA | 3;
		cp->sv.tf.tf_es = CPU_GDT_UDATA | 3;
		cp->sv.tf.tf_cs = CPU_GDT_UCODE | 3;
		cp->sv.tf.tf_ss = CPU_GDT_UDATA | 3;
		cp->sv.tf.tf_eflags &= FL_USER;
		cp->sv.tf.tf_eflags |= FL_IF;	// enable interrupts

		// Child gets to be nondeterministic only if parent is
		if (!(p->sv.pff & PFF_NONDET))
			cp->sv.pff &= ~PFF_NONDET;
	}

#if SOL >= 3
	uint32_t sva = tf->tf_regs.reg_esi;
	uint32_t dva = tf->tf_regs.reg_edi;
	uint32_t size = tf->tf_regs.reg_ecx;
	switch (cmd & SYS_MEMOP) {
	case 0:	// no memory operation
		break;
	case SYS_COPY:
		// validate source region
		if (PTOFF(sva) || PTOFF(size)
				|| sva < VM_USERLO || sva > VM_USERHI
				|| size > VM_USERHI-sva)
			systrap(tf, T_GPFLT, 0);
		// fall thru...
	case SYS_ZERO:
		// validate destination region
		if (PTOFF(dva) || PTOFF(size)
				|| dva < VM_USERLO || dva > VM_USERHI
				|| size > VM_USERHI-dva)
			systrap(tf, T_GPFLT, 0);

		switch (cmd & SYS_MEMOP) {
		case SYS_ZERO:	// zero memory and clear permissions
			pmap_remove(cp->pdir, dva, size);
			break;
		case SYS_COPY:	// copy from local src to dest in child
			pmap_copy(p->pdir, sva, cp->pdir, dva, size);
			break;
		}
		break;
	default:
		systrap(tf, T_GPFLT, 0);
	}

	if (cmd & SYS_PERM)
		if (!pmap_setperm(cp->pdir, dva, size, cmd & SYS_RW))
			panic("pmap_put: no memory to set permissions");

	if (cmd & SYS_SNAP)	// Snapshot child's state
		pmap_copy(cp->pdir, VM_USERLO, cp->rpdir, VM_USERLO,
				VM_USERHI-VM_USERLO);

#endif	// SOL >= 3
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
cprintf("GET proc %x eip %x esp %x cmd %x\n", p, tf->tf_eip, tf->tf_esp, cmd);

#if SOL >= 5
	// First migrate if we need to.
	uint8_t node = (tf->tf_regs.reg_edx >> 8) & 0xff;
	if (node == 0) node = RRNODE(p->home);		// Goin' home
	if (node != net_node)
		net_migrate(tf, node, 0);	// abort syscall and migrate

#endif // SOL >= 5
	spinlock_acquire(&p->lock);

	// Find the named child process; DON'T create if it doesn't exist
	uint32_t cn = tf->tf_regs.reg_edx & 0xff;
	proc *cp = p->child[cn];
	if (!cp)
		cp = &proc_null;

	// Synchronize with child if necessary.
	if (cp->state != PROC_STOP)
		proc_wait(p, cp, tf);

	// Since the child is now stopped, it's ours to control;
	// we no longer need our process lock -
	// and we don't want to be holding it if usercopy() below aborts.
	spinlock_release(&p->lock);

	// Get child's general register state
	if (cmd & SYS_REGS) {
		int len = offsetof(cpustate, fx);	// just integer regs
		if (cmd & SYS_FPU) len = sizeof(cpustate); // whole shebang

		// Copy child process's trapframe into user space
#if SOL >= 3
		usercopy(tf, 1, &cp->sv, tf->tf_regs.reg_ebx, len);
#else
		cpustate *cs = (cpustate*) tf->tf_regs.reg_ebx;
		memcpy(&cs, &cp->sv, len);
#endif
	}

#if SOL >= 3
	uint32_t sva = tf->tf_regs.reg_esi;
	uint32_t dva = tf->tf_regs.reg_edi;
	uint32_t size = tf->tf_regs.reg_ecx;
	switch (cmd & SYS_MEMOP) {
	case 0:	// no memory operation
		break;
	case SYS_COPY:
	case SYS_MERGE:
		// validate source region
		if (PTOFF(sva) || PTOFF(size)
				|| sva < VM_USERLO || sva > VM_USERHI
				|| size > VM_USERHI-sva)
			systrap(tf, T_GPFLT, 0);
		// fall thru...
	case SYS_ZERO:
		// validate destination region
		if (PTOFF(dva) || PTOFF(size)
				|| dva < VM_USERLO || dva > VM_USERHI
				|| size > VM_USERHI-dva)
			systrap(tf, T_GPFLT, 0);

		switch (cmd & SYS_MEMOP) {
		case SYS_ZERO:	// zero memory and clear permissions
			pmap_remove(p->pdir, dva, size);
			break;
		case SYS_COPY:	// copy from local src to dest in child
			pmap_copy(cp->pdir, sva, p->pdir, dva, size);
			break;
		case SYS_MERGE:	// merge from local src to dest in child
			pmap_merge(cp->rpdir, cp->pdir, sva,
					p->pdir, dva, size);
			break;
		}
		break;
	default:
		systrap(tf, T_GPFLT, 0);
	}

	if (cmd & SYS_PERM)
		if (!pmap_setperm(p->pdir, dva, size, cmd & SYS_RW))
			panic("pmap_get: no memory to set permissions");

	if (cmd & SYS_SNAP)
		systrap(tf, T_GPFLT, 0);	// only valid for PUT

#endif	// SOL >= 3
	trap_return(tf);	// syscall completed
}

static void gcc_noreturn
do_ret(trapframe *tf)
{
cprintf("RET proc %x eip %x esp %x\n", proc_cur(), tf->tf_eip, tf->tf_esp);
	proc_ret(tf, 1);	// Complete syscall insn and return to parent
}

#if SOL >= 4
static void gcc_noreturn
do_time(trapframe *tf)
{
	uint64_t t = timer_read();
	t = t * 1000000000 / TIMER_FREQ;	// convert to nanoseconds
	tf->tf_regs.reg_edx = t >> 32;
	tf->tf_regs.reg_eax = t;
	trap_return(tf);
}
#endif
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
#if SOL >= 4
	case SYS_TIME:	return do_time(tf);
#endif
#else	// not SOL >= 2
	// Your implementations of SYS_PUT, SYS_GET, SYS_RET here...
#endif	// not SOL >= 2
	default:	return;		// handle as a regular trap
	}
}

#endif	// LAB >= 2
