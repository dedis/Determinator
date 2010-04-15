#ifdef PIOS_USER

////////// PIOS Deterministic Threads //////////

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <syscall.h>
#include <x86.h>
#include <mmu.h>
#include <vm.h>

#include "bench.h"

#define ALLVA		((void*) VM_USERLO)
#define ALLSIZE		(VM_USERHI - VM_USERLO)

// Fork a child process, returning 0 in the child and 1 in the parent.
void
bench_fork(uint8_t child, void *(*fun)(void *), void *arg)
{
	int cmd = SYS_START | SYS_SNAP;

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
		fun(arg);
		sys_ret();
	}

	// Fork the child, copying our entire user address space into it.
	cs.tf.tf_regs.reg_eax = 0;	// isparent == 0 in the child
	sys_put(cmd | SYS_REGS | SYS_COPY, child, &cs, ALLVA, ALLVA, ALLSIZE);
}

void
bench_join(uint8_t child)
{
	int cmd = SYS_MERGE;
	int trapexpect = T_SYSCALL;

	// Wait for the child and retrieve its CPU state.
	// If merging, leave the highest 4MB containing the stack unmerged,
	// so that the stack acts as a "thread-private" memory area.
	struct cpustate cs;
	sys_get(cmd | SYS_REGS, child, &cs, ALLVA, ALLVA, ALLSIZE-PTSIZE);

	// Make sure the child exited with the expected trap number
	if (cs.tf.tf_trapno != trapexpect) {
		cprintf("  eip  0x%08x\n", cs.tf.tf_eip);
		cprintf("  esp  0x%08x\n", cs.tf.tf_esp);
		panic("join: unexpected trap %d, expecting %d\n",
			cs.tf.tf_trapno, trapexpect);
	}
}

uint64_t
bench_time(void)
{
	return sys_time();
}

#else	// ! PIOS_USER

////////// Conventional Unix/Pthreads Version //////////

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>

char tforked[256];
pthread_t threads[256];

void
bench_fork(uint8_t child, void *(*fun)(void *), void *arg)
{
	assert(!tforked[child]);

	int rc = pthread_create(&threads[child], NULL, fun, arg);
	assert(rc == 0);
	tforked[child] = 1;
}

void
bench_join(uint8_t child)
{
	assert(tforked[child]);

	void *val;
	int rc = pthread_join(threads[child], &val);
	assert(rc == 0);
	assert(val == NULL);
	tforked[child] = 0;
}

uint64_t
bench_time(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return ((uint64_t)tv.tv_sec * 1000000 + tv.tv_usec) * 1000; // ns
}

#endif	// ! PIOS_USER
