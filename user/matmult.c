#if SOL >= 4

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/syscall.h>
#include <inc/mmu.h>
#include <inc/vm.h>

#define ALLVA		((void*) VM_USERLO)
#define ALLSIZE		(VM_USERHI - VM_USERLO)

// Fork a child process, returning 0 in the child and 1 in the parent.
int
tfork(int cmd, uint8_t child)
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
	sys_put(cmd | SYS_REGS | SYS_COPY, child, &cs, ALLVA, ALLVA, ALLSIZE);

	return 1;
}

void
tjoin(int cmd, uint8_t child, int trapexpect)
{
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


#define MAXDIM	256

typedef int elt;

elt a[MAXDIM*MAXDIM], b[MAXDIM*MAXDIM], r[MAXDIM*MAXDIM];

void
blkmult(int i, int j, int blk, int dim)
{
	int ih = i+blk, jh = j+blk;
	int ii, jj, kk;
	for (ii = i; ii < ih; ii++)
		for (jj = j; jj < jh; jj++) {
			elt sum = 0;
			for (kk = 0; kk < dim; kk++)
				sum += a[ii*dim+kk] * b[kk*dim+jj];
			r[ii*dim+jj] = sum;
		}
}

void
matmult(int blk, int dim)
{
	assert(dim >= 1 && dim <= MAXDIM);
	assert(blk >= 1 && blk <= dim);
	assert(dim % blk == 0);

	int nbl = dim/blk;
	int nth = nbl*nbl;
	assert(nth <= 256);

	int i,j,k;

	// Fork off a thread to compute each cell in the result matrix
	for (i = 0; i < nbl; i++)
		for (j = 0; j < nbl; j++) {
			int child = i*nth + j;
			if (!tfork(SYS_START | SYS_SNAP, child)) {
				blkmult(i*blk, j*blk, blk, dim);
				sys_ret();
			}
		}

	// Now go back and merge in the results of all our children
	for (i = 0; i < nbl; i++)
		for (j = 0; j < nbl; j++) {
			int child = i*nth + j;
			tjoin(SYS_MERGE, child, T_SYSCALL);
		}
}

int main()
{
	matmult(16, 256);
	return 0;
}

#endif
