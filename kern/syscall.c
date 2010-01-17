#if LAB >= 2
/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/trap.h>

#include <kern/syscall.h>


static void
sys_put(trapframe *tf)
{
	panic("sys_put not implemented");
}

static void
sys_get(trapframe *tf)
{
	panic("sys_get not implemented");
}

static void
sys_ret(trapframe *tf)
{
	panic("sys_ret not implemented");
}

// Common function to handle all system calls -
// decode the system call type and call the appropriate function.
// Be sure to handle undefined system calls appropriately.
void
syscall(trapframe *tf)
{
#if SOL >= 2
	// EAX register holds system call command/flags
	uint32_t cmd = tf->tf_regs.reg_eax;
	switch (cmd & SYS_TYPE) {
	case SYS_PUT:	return sys_put(tf);
	case SYS_GET:	return sys_get(tf);
	case SYS_RET:	return sys_ret(tf);
	default:	return;		// handle as a regular trap
	}
#else	// not SOL >= 2
	panic("syscall not implemented");
#endif	// not SOL >= 2
}

#endif	// LAB >= 2
