#if LAB >= 4
/*
 * Simple "thread" fork/join functions for PIOS.
 * Since the PIOS doesn't actually allow multiple threads
 * to run in one process and share memory directly,
 * these functions use the kernel's SNAP and MERGE features
 * to emulate threads in a deterministic consistency model.
 *
 * Copyright (C) 2010 Yale University.
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Primary author: Bryan Ford
 */

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
		: "=m" (cs.tf.regs.esi),
		  "=m" (cs.tf.regs.edi),
		  "=m" (cs.tf.regs.ebp),
		  "=m" (cs.tf.esp),
		  "=m" (cs.tf.eip),
		  "=a" (isparent)
		:
		: "ebx", "ecx", "edx");
	if (!isparent)
		return 0;	// in the child

	// Fork the child, copying our entire user address space into it.
	cs.tf.regs.eax = 0;	// isparent == 0 in the child
	sys_put(SYS_REGS | SYS_COPY | SYS_SNAP | SYS_START, child,
		&cs, ALLVA, ALLVA, ALLSIZE);

	return 1;
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
	if (cs.tf.trapno != T_SYSCALL) {
		cprintf("  eip  0x%08x\n", cs.tf.eip);
		cprintf("  esp  0x%08x\n", cs.tf.esp);
		panic("tjoin: unexpected trap %d, expecting %d\n",
			cs.tf.trapno, T_SYSCALL);
	}
}

#endif // LAB >= 4
