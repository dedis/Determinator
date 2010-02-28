#if LAB >= 4
// A more-or-less Unix-compatible fork()

#include <inc/file.h>
#include <inc/unistd.h>
#include <inc/string.h>
#include <inc/syscall.h>
#include <inc/assert.h>
#include <inc/errno.h>
#include <inc/vm.h>


#define ALLVA		((void*) VM_USERLO)
#define ALLSIZE		(VM_USERHI - VM_USERLO)

pid_t fork(void)
{
	// Find a free child process slot.
	// We just use child process slot numbers as Unix PIDs,
	// even though child slots are process-local in PIOS
	// whereas PIDs are global in Unix.
	// This means that commands like 'ps' and 'kill'
	// have to be shell-builtin commands under PIOS.
	pid_t child;
	for (child = 1; child < 256; child++)
		if (files->child[child] == PROC_FREE)
			break;
	if (child == 256) {
		warn("fork: no child process available");
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
	if (!isparent)
		return 0;	// we're in the child

	// Fork the child, copying our entire user address space into it.
	cs.tf.tf_regs.reg_eax = 0;	// isparent == 0 in the child
	sys_put(SYS_REGS | SYS_COPY, child, &cs, ALLVA, ALLVA, ALLSIZE);

	files->child[child] = PROC_FORKED;
	return child;
}

#endif	// LAB >= 4
