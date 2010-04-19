#if SOL >= 4

////////// PIOS Deterministic Threads //////////

#include <inc/stdio.h>
#include <inc/stdlib.h>
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

// Barrier constants.
#define EXIT_BARRIER	0x80000000 // Highest bit set to indicate sys_ret at barrier.
#define BARRIER_MASK	0xff
#define BARRIER_READ(b)	((b) & BARRIER_MASK)
#define BARRIER_WRITE(b) (EXIT_BARRIER | (b))


static pthread_barrier_t barriers[BARRIER_MAX];

// Fork a child process, returning 0 in the child and 1 in the parent.
int
pthread_create(pthread_t *out_thread, const pthread_attr_t *attr,
		void *(*start_routine)(void *), void *arg)
{
	pthread_t th;
	int i;

	// Find a free child thread/process slot.
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
		// Clear our child state array, since we have no children yet.
		memset(&files->child, 0, sizeof(files->child));
		files->child[0].state = PROC_RESERVED;
		for (i = 1; i < FILE_INODES; i++)
			if (fileino_alloced(i)) {
				files->fi[i].rino = i;	// 1-to-1 mapping
				files->fi[i].rver = files->fi[i].ver;
				files->fi[i].rlen = files->fi[i].size;
			}

		files->thstat = (void *)th; // for pthread_self
 		files->thstat = start_routine(arg);
		asm volatile("	movl	%0, %%edx" : : "m" (files->thstat));
		sys_ret();
	}

	// Fork the child, copying our entire user address space into it.
	cs.tf.tf_regs.reg_eax = 0;	// isparent == 0 in the child
	sys_put(SYS_START | SYS_SNAP | SYS_REGS | SYS_COPY, th,
		&cs, ALLVA, ALLVA, ALLSIZE);

	// Record the inode generation numbers of all inodes at fork time,
	// so that we can reconcile them later when we synchronize with it.
	memset(&files->child[th], 0, sizeof(files->child[th]));
	files->child[th].state = PROC_FORKED;

	*out_thread = th;
	return 0;
}


int
wait_at_barrier(pthread_t first_child, pthread_barrier_t barrier)
{

	// Get the number of threads to wait at this barrier.
	// One has already arrived, so subtract 1.
	int count = barriers[barrier];
	int status, i;
	struct cpustate cs;
	pthread_t th;
	pthread_t threads[PROC_CHILDREN];
	i = 0;
	threads[i++] = first_child;
	for  (th = 1; th < PROC_CHILDREN && i < count; th++) {

		if (th == first_child || files->child[th].state != PROC_FORKED)
			continue;

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
		status = cs.tf.tf_regs.reg_edx;

		// This should be the same barrier;
		assert(barrier == BARRIER_READ(status));

		threads[i++] = th;
	}

	int j;
	cprintf("THREADS ");
	for (j = 0; j < count; j++)
		cprintf("%d ", threads[j]);
	cprintf("\n");

	// Wrong count or not enough forked threads: error.
	if (i < count)
		return i;

	// Synchronize memory with all children.
	// Restart all children.
	count++;
	for (i = 0; i < count; i++)
		sys_put( SYS_COPY | SYS_SNAP | SYS_START, threads[i],
			 NULL, SHAREVA, SHAREVA, SHARESIZE);
	return 0;
}


int
pthread_join(pthread_t th, void **out_exitval)
{

	// Wait for the child and retrieve its CPU state.
	// Merge, leaving the highest 4MB containing the stack unmerged,
	// so that the stack acts as a "thread-private" memory area.
	// If this is a barrier, wait until all children have arrived,
	// then restart them all.
	// If this is not a barrier, free the child.
	int status, ret;
	struct cpustate cs;
	while(true) {
		sys_get(SYS_MERGE | SYS_REGS, th, &cs, SHAREVA, SHAREVA, SHARESIZE);

		// Make sure the child exited with the expected trap number
		if (cs.tf.tf_trapno != T_SYSCALL) {
			cprintf("  eip  0x%08x\n", cs.tf.tf_eip);
			cprintf("  esp  0x%08x\n", cs.tf.tf_esp);
			cprintf("join: unexpected trap %d, expecting %d\n",
				cs.tf.tf_trapno, T_SYSCALL);
			status = -1;
			errno = EINVAL;
			ret = -1;
			goto done;
		}

		status = cs.tf.tf_regs.reg_edx;

		// At a barrier?
		if (status & EXIT_BARRIER) {
			ret = wait_at_barrier(th, BARRIER_READ(status));
			if (ret == -1) {
				errno = EINVAL;
				goto done;
			}
			if (ret > 0) {
				errno = EAGAIN;
				ret = -1;
				goto done;
			}
		}

		// If not, we're finished.
		else
			break;
	}
	ret = 0;
 done:
	if (out_exitval != NULL)
		*out_exitval = (void *)status;
	files->thstat = NULL;
	sys_put(SYS_ZERO, th, NULL, ALLVA, ALLVA, ALLSIZE);
	files->child[th].state = PROC_FREE;
	return ret;
}


int 
pthread_barrier_init(pthread_barrier_t * barrier, 
			 const pthread_barrierattr_t * attr,
			 unsigned int count)
{
	int b;
	if (count <= 0 || count >= PROC_CHILDREN) {
		errno = EINVAL;
		return -1;
	}

	for (b = 0; b < BARRIER_MAX; b++)
		if (barriers[b] == 0) 
			break;

	if (b == BARRIER_MAX) {
		errno = EAGAIN;
		return -1;
	}

	*barrier = b;
	barriers[b] = count;
	return 0;
}

int 
pthread_barrier_destroy(pthread_barrier_t * barrier)
{
	if (*barrier < 0 || *barrier >= BARRIER_MAX) {
		errno = EINVAL;
		return -1;
	}
	barriers[*barrier] = 0;
	return 0;
}

int
pthread_barrier_wait(pthread_barrier_t * barrier)
{
	// Reject if null or presumably uninitialized.

	if ((barrier == NULL) ||
	    (barriers[*barrier] <= 0)) {
		errno = EINVAL;
		return -1;
	}

	uint32_t b = BARRIER_WRITE(*barrier);
	asm volatile("	movl	%0, %%edx" : : "m" (b));
	sys_ret();
	return 0;
}


pthread_t pthread_self(void)
{
	return (int)files->thstat; 
}


#endif // SOL >= 4
