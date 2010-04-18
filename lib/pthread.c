#if SOL >= 4

////////// PIOS Deterministic Threads //////////

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/errno.h>
#include <inc/syscall.h>
#include <inc/x86.h>
#include <inc/mmu.h>
#include <inc/vm.h>
#include <inc/file.h>
#include <pthread.h>

#define ALLVA		((void*) VM_USERLO)
#define ALLSIZE		(VM_USERHI - VM_USERLO)

#define SHAREVA		((void*) VM_SHARELO)
#define SHARESIZE	(VM_SHAREHI - VM_SHARELO)

// Fork a child process, returning 0 in the child and 1 in the parent.
int
pthread_create(pthread_t *out_thread, const pthread_attr_t *attr,
		void *(*start_routine)(void *), void *arg)
{
	// Find a free child thread/process slot.
	pthread_t th;
	for (th = 1; th < PROC_CHILDREN; th++)
		if (files->child[th].state == PROC_FREE)
			break;
	if (th == 256) {
		warn("pthread_create: no child available");
		errno = EAGAIN;
		return -1;
	}

	// Set up the register state for the child
	struct cpustate cs;
	memset(&cs, 0, sizeof(cs));

	// Use some assembly magic to propagate registers to child
	// and generate an appropriate starting eip
	int isparent;
	asm volatile(
		"	movl	%%esi,%0;"
		"	movl	%%edi,%1;"
		"	movl	%%ebp,%2;"
		"	movl	%%esp,%3;"
		"	movl	$1f,%4;"
		"	movl	$1,%5;"
		"1:	"
		: "=m" (cs.tf.tf_regs.reg_esi),
		  "=m" (cs.tf.tf_regs.reg_edi),
		  "=m" (cs.tf.tf_regs.reg_ebp),
		  "=m" (cs.tf.tf_esp),
		  "=m" (cs.tf.tf_eip),
		  "=a" (isparent)
		:
		: "ebx", "ecx", "edx");
	if (!isparent) {	// in the child
 		files->thstat = start_routine(arg);
		asm volatile("	movl	%0, %%edx" : : "m" (files->thstat));
		sys_ret();
	}

	// Fork the child, copying our entire user address space into it.
	cs.tf.tf_regs.reg_eax = 0;	// isparent == 0 in the child
	sys_put(SYS_START | SYS_SNAP | SYS_REGS | SYS_COPY, th,
		&cs, ALLVA, ALLVA, ALLSIZE);

	*out_thread = th;
	return 0;
}

int
pthread_join(pthread_t th, void **out_exitval)
{
	// Wait for the child and retrieve its CPU state.
	// If merging, leave the highest 4MB containing the stack unmerged,
	// so that the stack acts as a "thread-private" memory area.
	struct cpustate cs;
	sys_get(SYS_MERGE | SYS_REGS, th, &cs, SHAREVA, SHAREVA, SHARESIZE);

	// Make sure the child exited with the expected trap number
	if (cs.tf.tf_trapno != T_SYSCALL) {
		cprintf("  eip  0x%08x\n", cs.tf.tf_eip);
		cprintf("  esp  0x%08x\n", cs.tf.tf_esp);
		cprintf("join: unexpected trap %d, expecting %d\n",
			cs.tf.tf_trapno, T_SYSCALL);
		errno = EINVAL;
		return -1;
	}
	if (out_exitval != NULL) {
		*out_exitval = (void *)cs.tf.tf_regs.reg_edx;
	}
	files->thstat = NULL;
	return 0;
}

#endif // SOL >= 4
