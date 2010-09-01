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

	// Wait for the child and retrieve its final CPU state
	// and whatever changes it made to the shared memory area.
	struct cpustate cs;
	sys_get(cmd | SYS_REGS, child, &cs, SHAREVA, SHAREVA, SHARESIZE);

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

uint64_t
get_user_time(void)
{
	return bench_time();
}

uint64_t
get_system_time(void)
{
	return bench_time();
}
//			print_output(dim, nbi, nbj, nth, niter, td,
//				     user_time, system_time);

void
print_output(int dim, int nbi, int nbj, int nth, int niter, uint64_t td,
	     uint64_t user_time, uint64_t system_time)
{
	printf("blksize %dx%d thr %d itr %d: %lld.%09lld\n",
	       dim/nbi, dim/nbj, nth, niter,
	       (long long)td / 1000000000,
	       (long long)td % 1000000000);
}

#else	// ! PIOS_USER

////////// Conventional Unix/Pthreads Version //////////

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/resource.h>

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

uint64_t
get_user_time(void)
{
	struct rusage ru;
	if (getrusage(RUSAGE_SELF, &ru) < 0)
		perror("getrusage");
	return ((uint64_t)ru.ru_utime.tv_sec * 1000000 +
		ru.ru_utime.tv_usec) * 1000; // ns
}

uint64_t
get_system_time(void)
{
	struct rusage ru;
	if (getrusage(RUSAGE_SELF, &ru) < 0)
		perror("getrusage");
	return ((uint64_t)ru.ru_stime.tv_sec * 1000000 +
		ru.ru_stime.tv_usec) * 1000; // ns
}

void
print_output(int dim, int nbi, int nbj, int nth, int niter, uint64_t td,
	     uint64_t user_time, uint64_t system_time)
{
	printf("%dx%d\t%d\t%d\t%lld.%09lld\t"
	       "%lld.%09lld\t%lld.%09lld\n",
	       dim/nbi, dim/nbj, nth, niter,
	       (long long)td / 1000000000,
	       (long long)td % 1000000000,
	       (long long)user_time / 1000000000,
	       (long long)user_time % 1000000000,
	       (long long)system_time / 1000000000,
	       (long long)system_time % 1000000000);
}
#endif	// ! PIOS_USER
