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
#include <sys/time.h>

char tforked[256];
pthread_t threads[256];

void
tfork(uint8_t child, void *(*fun)(void *), void *arg)
{
	assert(!tforked[child]);

	int rc = pthread_create(&threads[child], NULL, fun, arg);
	assert(rc == 0);
	tforked[child] = 1;
}

void
tjoin(uint8_t child)
{
	assert(tforked[child]);

	void *val;
	int rc = pthread_join(threads[child], &val);
	assert(rc == 0);
	assert(val == NULL);
	tforked[child] = 0;
}

static __attribute__((always_inline)) uint64_t
rdtsc(void)
{
#if 0
	uint32_t tsclo, tschi;
        asm volatile("rdtsc" : "=a" (tsclo), "=d" (tschi));
        return (uint64_t)tschi << 32 | tsclo;
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
#endif
}

#endif	// ! PIOS_USER


#define MAXDIM		1024

#define MAXTHREADS	8

typedef int elt;

elt a[MAXDIM*MAXDIM], b[MAXDIM*MAXDIM], r[MAXDIM*MAXDIM];

struct tharg {
	int bi, bj, nbi, nbj, dim;
};

void *
blkmult(void *varg)
{
	struct tharg *arg = varg;
	int dim = arg->dim;	// Total matrix size in each dimension
	int nbi = arg->nbi;	// Number of blocks in i dimension
	int nbj = arg->nbj;	// Number of blocks in j dimension
	int bsi = dim/nbi;	// Block size in i dimension
	int bsj = dim/nbj;	// Block size in j dimension

	int il = arg->bi * bsi, jl = arg->bj * bsj;
	int ih = il + bsi,	jh = jl + bsj;
	int ii, jj, kk;
	for (ii = il; ii < ih; ii++)
		for (jj = jl; jj < jh; jj++) {
			elt sum = 0;
			for (kk = 0; kk < dim; kk++)
				sum += a[ii*dim+kk] * b[kk*dim+jj];
			r[ii*dim+jj] = sum;
		}
	return NULL;
}

void
matmult(int nbi, int nbj, int dim)
{
	assert(dim >= 1 && dim <= MAXDIM);
	assert(nbi >= 1 && nbi <= dim); assert(dim % nbi == 0);
	assert(nbj >= 1 && nbj <= dim); assert(dim % nbj == 0);

	int nth = nbi*nbj;
	assert(nth >= 1 && nth <= 256);

	int bi,bj;
	struct tharg arg[256];

	// Fork off a thread to compute each cell in the result matrix
	for (bi = 0; bi < nbi; bi++)
		for (bj = 0; bj < nbj; bj++) {
			int child = bi*nbi + bj;
			arg[child].bi = bi;
			arg[child].bj = bj;
			arg[child].nbi = nbi;
			arg[child].nbj = nbj;
			arg[child].dim = dim;
			tfork(child, blkmult, &arg[child]);
		}

	// Now go back and merge in the results of all our children
	for (bi = 0; bi < nbi; bi++)
		for (bj = 0; bj < nbj; bj++) {
			int child = bi*nbi + bj;
			tjoin(child);
		}
}

int main(int argc, char **argv)
{
	int dim, nth, nbi, nbj, iter;
	for (dim = 16; dim <= MAXDIM; dim *= 2) {
		printf("matrix size: %dx%d = %d (%d bytes)\n",
			dim, dim, dim*dim, dim*dim*(int)sizeof(elt));
		for (nth = nbi = nbj = 1; nth <= MAXTHREADS; ) {
			assert(nth == nbi * nbj);
			int niter = MAXDIM/dim;
			niter = niter * niter; // * niter;	// MM = O(n^3)

			matmult(nbi, nbj, dim);	// once to warm up...

			uint64_t ts = rdtsc();
			for (iter = 0; iter < niter; iter++)
				matmult(nbi, nbj, dim);
			uint64_t td = (rdtsc() - ts) / niter;

			printf("blksize %dx%d threads %d iters %d: %lld\n",
				dim/nbi, dim/nbj, nth, niter, (long long)td);

			if (nbi == nbj)
				nbi *= 2;
			else
				nbj *= 2;
			nth *= 2;
		}
	}

	return 0;
}

//#endif
