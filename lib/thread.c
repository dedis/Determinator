#if LAB >= 4

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/unistd.h>
#include <inc/assert.h>
#include <inc/syscall.h>
#include <inc/vm.h>

#define ALLVA		((void*) VM_USERLO)
#define ALLSIZE		(VM_USERHI - VM_USERLO)

#define SHAREVA		((void*) VM_SHARELO)
#define SHARESIZE	(VM_SHAREHI - VM_SHARELO)

#define PROC_CHILDREN 256


// Fork a child process/thread, returning 0 in the child and 1 in the parent.
int
tfork(uint16_t child)
{
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
	if (!isparent)
		return 0;	// in the child

	// Fork the child, copying our entire user address space into it.
	cs.tf.tf_regs.reg_eax = 0;	// isparent == 0 in the child
	sys_put(SYS_REGS | SYS_COPY | SYS_SNAP | SYS_START, child,
 		&cs, ALLVA, ALLVA, ALLSIZE);

	return 1;
}


void
tresume(uint16_t child)
{
	// Restart a child after it has stopped.
	// Refresh child's memory state to match parent's.
	sys_put( SYS_COPY | SYS_SNAP | SYS_START, child,
		 NULL, SHAREVA, SHAREVA, SHARESIZE);
}

void
tjoin(uint16_t child)
{
	// Wait for the child and retrieve its CPU state.
	// If merging, leave the highest 4MB containing the stack unmerged,
	// so that the stack acts as a "thread-private" memory area.
	struct cpustate cs;
	sys_get(SYS_MERGE | SYS_REGS, child, &cs, SHAREVA, SHAREVA, SHARESIZE);

	// Make sure the child exited with the expected trap number
	if (cs.tf.tf_trapno != T_SYSCALL) {
		cprintf("  eip  0x%08x\n", cs.tf.tf_eip);
		cprintf("  esp  0x%08x\n", cs.tf.tf_esp);
		panic("tjoin: unexpected trap %d, expecting %d\n",
			cs.tf.tf_trapno, T_SYSCALL);
	}
}


void
tbarrier_merge(uint16_t * children, int num_children) 
{
	assert(num_children > 0);
	assert(num_children <= PROC_CHILDREN);
	int i;
	for (i = 0; i < num_children; i++)
		tjoin(children[i]);
	for (i = 0; i < num_children; i++)
		tresume(children[i]);
}


#endif // LAB >= 4
