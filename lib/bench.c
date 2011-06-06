#if LAB >= 9
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

#define SHAREVA		((void*) VM_SHARELO)
#define SHARESIZE	(VM_SHAREHI - VM_SHARELO)

// Fork a child process, returning 0 in the child and 1 in the parent.
void
bench_fork(uint8_t child, void *(*fun)(void *), void *arg)
{
	int cmd = SYS_START | SYS_SNAP;

	// Set up the register state for the child
	struct procstate ps;
	memset(&ps, 0, sizeof(ps));

	// Use some assembly magic to propagate registers to child
	// and generate an appropriate starting eip
	int isparent;
	asm volatile(
		"	movq	%%rsi,%0;"
		"	movq	%%rdi,%1;"
		"	movq	%%rbp,%2;"
		"	movq	%%rsp,%3;"
		"	movq	$1f,%4;"
		"	movl	$1,%5;"
		"1:	"
		: "=m" (ps.tf.rsi),
		  "=m" (ps.tf.rdi),
		  "=m" (ps.tf.rbp),
		  "=m" (ps.tf.rsp),
		  "=m" (ps.tf.rip),
		  "=a" (isparent)
		:
		: "rbx", "rcx", "rdx");
	if (!isparent) {	// in the child
		fun(arg);
		sys_ret();
	}

	// Fork the child, copying our entire user address space into it.
	ps.tf.rax = 0;	// isparent == 0 in the child
	sys_put(cmd | SYS_REGS | SYS_COPY, child, &ps, ALLVA, ALLVA, ALLSIZE);
}

void
bench_join(uint8_t child)
{
	int cmd = SYS_MERGE;
	int trapexpect = T_SYSCALL;

	// Wait for the child and retrieve its final CPU state
	// and whatever changes it made to the shared memory area.
	struct procstate ps;
	sys_get(cmd | SYS_REGS, child, &ps, SHAREVA, SHAREVA, SHARESIZE);

	// Make sure the child exited with the expected trap number
	if (ps.tf.trapno != trapexpect) {
		cprintf("  rip  0x%08x\n", ps.tf.rip);
		cprintf("  rsp  0x%08x\n", ps.tf.rsp);
		panic("join: unexpected trap %d, expecting %d\n",
			ps.tf.trapno, trapexpect);
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
#endif	// LAB >= 9
