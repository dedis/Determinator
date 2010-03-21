//#if SOL >= 4

#ifdef PIOS_USER

////////// PIOS Deterministic Threads //////////

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/syscall.h>
#include <inc/x86.h>
#include <inc/mmu.h>
#include <inc/vm.h>

#define ALLVA		((void*) VM_USERLO)
#define ALLSIZE		(VM_USERHI - VM_USERLO)

// Fork a child process, returning 0 in the child and 1 in the parent.
void
tfork(uint8_t child, void *(*fun)(void *), void *arg)
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
tjoin(uint8_t child)
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

#define printf cprintf	// use immediate console printing

#else	// ! PIOS_USER

////////// Conventional Unix/Pthreads Version //////////

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

pthread_t threads[256];

void
tfork(uint8_t child, void *(*fun)(void *), void *arg)
{
	assert(threads[child] == NULL);

	int rc = pthread_create(&threads[child], NULL, fun, arg);
	assert(rc == 0);
}

void
tjoin(uint8_t child)
{
	assert(threads[child] != NULL);

	void *val;
	int rc = pthread_join(threads[child], &val);
	assert(rc == 0);
	assert(val == NULL);

	threads[child] = NULL;
}

static __attribute__((always_inline)) uint64_t
rdtsc(void)
{
	uint32_t tsclo, tschi;
        asm volatile("rdtsc" : "=a" (tsclo), "=d" (tschi));
        return (uint64_t)tschi << 32 | tsclo;
}

#endif	// ! PIOS_USER


#define MAXDIM	1024

typedef int elt;

elt a[MAXDIM*MAXDIM], b[MAXDIM*MAXDIM], r[MAXDIM*MAXDIM];

struct tharg {
	int i, j, blk, dim;
};

void *
blkmult(void *varg)
{
	struct tharg *arg = varg;
	int i = arg->i;
	int j = arg->j;
	int blk = arg->blk;
	int dim = arg->dim;

	int ih = i+blk, jh = j+blk;
	int ii, jj, kk;
	for (ii = i; ii < ih; ii++)
		for (jj = j; jj < jh; jj++) {
			elt sum = 0;
			for (kk = 0; kk < dim; kk++)
				sum += a[ii*dim+kk] * b[kk*dim+jj];
			r[ii*dim+jj] = sum;
		}
	return NULL;
}

void
matmult(int blk, int dim)
{
	assert(dim >= 1 && dim <= MAXDIM);
	assert(blk >= 1 && blk <= dim);
	assert(dim % blk == 0);

	int nbl = dim/blk;
	int nth = nbl*nbl;
	assert(nth >= 1 && nth <= 256);

	int i,j,k;
	struct tharg arg[256];

	// Fork off a thread to compute each cell in the result matrix
	for (i = 0; i < nbl; i++)
		for (j = 0; j < nbl; j++) {
			int child = i*nbl + j;
			arg[child].i = i;
			arg[child].j = j;
			arg[child].blk = blk;
			arg[child].dim = dim;
			tfork(child, blkmult, &arg[child]);
		}

	// Now go back and merge in the results of all our children
	for (i = 0; i < nbl; i++)
		for (j = 0; j < nbl; j++) {
			int child = i*nbl + j;
			tjoin(child);
		}
}

int main(int argc, char **argv)
{
	int dim, blk;
	for (dim = 256; dim <= 1024; dim *= 2) {
		printf("matrix size: %dx%d = %d (%d bytes)\n",
			dim, dim, dim*dim, dim*dim*sizeof(elt));
		for (blk = dim/16; blk <= dim; blk *= 2) {
			uint64_t ts = rdtsc();
			matmult(blk, dim);
			uint64_t td = rdtsc() - ts;
			printf("blksize %d: %lld ticks\n", blk, td);
		}
	}

	return 0;
}

//#endif
