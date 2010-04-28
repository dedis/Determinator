#if LAB >= 3

#include <inc/file.h>
#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/syscall.h>
#include <inc/x86.h>
#include <inc/mmu.h>
#include <inc/vm.h>
#include <inc/errno.h>

typedef int	KEY_T;

//#define USE_MALLOC

/*
 * These are the COMPARISON macros
 * Replace these macros by YOUR comparison operations.
 * e.g. if you are sorting an array of pointers to strings
 * you should define:
 *
 *	GT(x, y)  as   (strcmp((x),(y)) > 0) 	Greater than
 *	LT(x, y)  as   (strcmp((x),(y)) < 0) 	Less than
 *	GE(x, y)  as   (strcmp((x),(y)) >= 0) 	Greater or equal
 *	LE(x, y)  as   (strcmp((x),(y)) <= 0) 	Less or equal
 *	EQ(x, y)  as   (strcmp((x),(y)) == 0) 	Equal
 *	NE(x, y)  as   (strcmp((x),(y)) != 0) 	Not Equal
 */
#define GT(x, y) ((x) > (y))
#define LT(x, y) ((x) < (y))
#define GE(x, y) ((x) >= (y))
#define LE(x, y) ((x) <= (y))
#define EQ(x, y) ((x) == (y))
#define NE(x, y) ((x) != (y))

/*
 * This is the SWAP macro to swap between two keys.
 * Replace these macros by YOUR swap macro.
 * e.g. if you are sorting an array of pointers to strings
 * You can define it as:
 *
 *	#define SWAP(x, y) temp = (x); (x) = (y); (y) = temp
 *
 * Bug: 'insort()' doesn't use the SWAP macro.
 */
#define SWAP(x, y) temp = (x); (x) = (y); (y) = temp

void  insort (array, len)
register KEY_T  array[];
register int    len;
{
	register int	i, j;
	register KEY_T	temp;

	for (i = 1; i < len; i++) {
		/* invariant:  array[0..i-1] is sorted */
		j = i;
		/* customization bug: SWAP is not used here */
		temp = array[j];
		while (j > 0 && GT(array[j-1], temp)) {
			array[j] = array[j-1];
			j--;
		}
		array[j] = temp;
	}
}

#ifndef CUTOFF
#  define CUTOFF 15
#endif

/*
 |  void  partial_quickersort (array, lower, upper)
 |  KEY_T  array[];
 |  int    lower, upper;
 |
 |  Abstract:
 |	Sort array[lower..upper] into a partial order
 |     	leaving segments which are CUTOFF elements long
 |     	unsorted internally.
 |
 |  Efficiency:
 |	Could be made faster for _worst_ cases by selecting
 |	a pivot using median-of-3. I don't do it because
 |	in practical cases my pivot selection is arbitrary and
 |	thus pretty random, your mileage may vary.
 |
 |  Method:
 |	Partial Quicker-sort using a sentinel (ala Robert Sedgewick)
 |
 |  BIG NOTE:
 |	Precondition: array[upper+1] holds the maximum possible key.
 |	with a cutoff value of CUTOFF.
 */

void  partial_quickersort (array, lower, upper)
register KEY_T  array[];
register int    lower, upper;
{
    register int	i, j;
    register KEY_T	temp, pivot;

    if (upper - lower > CUTOFF) {
	SWAP(array[lower], array[(upper+lower)/2]);
	i = lower;  j = upper + 1;  pivot = array[lower];
	while (1) {
	    /*
	     * ------------------------- NOTE --------------------------
	     * ignoring BIG NOTE above may lead to an infinite loop here
	     * ---------------------------------------------------------
	     */
	    do i++; while (LT(array[i], pivot));
	    do j--; while (GT(array[j], pivot));
	    if (j < i) break;
	    SWAP(array[i], array[j]);
	}
	SWAP(array[lower], array[j]);
	partial_quickersort (array, lower, j - 1);
	partial_quickersort (array, i, upper);
    }
}
/*
 |  void  sedgesort (array, len)
 |  KEY_T  array[];
 |  int    len;
 |
 |  Abstract:
 |	Sort array[0..len-1] into increasing order.
 |
 |  Method:
 |	Use partial_quickersort() with a sentinel (ala Sedgewick)
 |	to reach a partial order, leave the unsorted segments of
 |	length == CUTOFF to a simpler low-overhead, insertion sort.
 |
 |	This method is the fastest in this collection according
 |	to my experiments.
 |
 |  BIG NOTE:
 |	precondition: array[len] must hold a sentinel (largest
 |	possible value) in order for this to work correctly.
 |	An easy way to do this is to declare an array that has
 | 	len+1 elements [0..len], and assign MAXINT or some such
 |	to the last location before starting the sort (see sorttest.c)
 */
void  sedgesort (array, len)
register KEY_T  array[];
register int    len;
{
    /*
     * ------------------------- NOTE --------------------------
     * ignoring BIG NOTE above may lead to an infinite loop here
     * ---------------------------------------------------------
     */
    partial_quickersort (array, 0, len - 1);
    insort (array, len);
}


#define STACKSIZE	PAGESIZE

#define ALLVA		((void*) VM_USERLO)
#define ALLSIZE		(VM_USERHI - VM_USERLO)

#define SHAREVA		((void*) VM_SHARELO)
#define SHARESIZE		(VM_SHAREHI - VM_SHARELO)

extern uint8_t start[], etext[], edata[], end[];

uint8_t stack[2][STACKSIZE];


// Fork a child process/thread, returning 0 in the child and tid in the parent.
int
tfork()
{	
	//find a free slot for child thread
	uint8_t tid;
	for (tid = 1; tid < 256; tid++)
		if (files->child[tid].state == PROC_FREE)
			break;
	if (tid == 256) {
		warn("tfork: no child thread available");
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
	if (!isparent) {
		files->thself = tid;
		// Clear our child state array, since we have no children yet.
		memset(&files->child, 0, sizeof(files->child));
		files->child[0].state = PROC_RESERVED;
		return 0;	// in the child
	}

	// Fork the child, copying our entire user address space into it.
	cs.tf.tf_regs.reg_eax = 0;	// isparent == 0 in the child
	sys_put(SYS_SNAP | SYS_START | SYS_REGS | SYS_COPY, tid, &cs, ALLVA, ALLVA, ALLSIZE);
	// Record the inode generation numbers of all inodes at fork time,
	// so that we can reconcile them later when we synchronize with it.
	memset(&files->child[tid], 0, sizeof(files->child[tid]));
	files->child[tid].state = PROC_FORKED;

	return tid;
}

void
tjoin(uint8_t child)
{
	// Wait for the child and retrieve its CPU state.
	// If merging, leave the highest 4MB containing the stack unmerged,
	// so that the stack acts as a "thread-private" memory area.
	struct cpustate cs;
	sys_get(SYS_MERGE | SYS_REGS, child, &cs, SHAREVA, SHAREVA, SHARESIZE-PTSIZE);

	// Make sure the child exited with the expected trap number
	if (cs.tf.tf_trapno != T_SYSCALL) {
		cprintf("  eip  0x%08x\n", cs.tf.tf_eip);
		cprintf("  esp  0x%08x\n", cs.tf.tf_esp);
		panic("join: unexpected trap %d, expecting %d\n",
			cs.tf.tf_trapno, T_SYSCALL);
	}
}

#define swapints(a,b) ({ int t = (a); (a) = (b); (b) = t; })

uint64_t
get_time(void)
{
	return sys_time();
}

void
pqsort(uint32_t *lo, uint32_t *hi, int nthread)
{
	//cprintf("starting thread %d...\n", files->thself);
	//cprintf("lo = %x, hi = %x, num = %d\n", lo, hi, hi-lo + 1);
	if (lo >= hi)
		return;
	
	if(/*hi <= lo + MIN_STRIDE ||*/ nthread <= 1) { // now, MIN_STRIDE is controlled by nthread...
		sedgesort(lo, hi - lo + 1);
		//insort(lo, hi-lo + 1);
		return;
	}

	int pivot = *lo;	// yeah, bad way to choose pivot...
	uint32_t *l = lo+1, *h = hi;
	while (l <= h) {
		if (*l < pivot)
			l++;
		else if (*h > pivot)
			h--;
		else
			swapints(*h, *l), l++, h--;
	}
	swapints(*lo, l[-1]);

	// Now recursively sort the two halves in parallel subprocesses
	int lchild, rchild;
	if (!(lchild = tfork())) {
		pqsort(lo, l-2, (nthread>>1));
		sys_ret();
	}
	if (!(rchild = tfork())) {
		pqsort(h+1, hi, (nthread>>1));
		sys_ret();
	}
	//cprintf("to join children...l=%d, r=%d\n", lchild, rchild);
	tjoin((uint8_t)lchild);
	tjoin((uint8_t)rchild);
}

#define RAND_INT_NUM 1000000
int niter = 10;
#define MAX_ARRAY_SIZE 1000000

#ifdef MAXTHREADS
#define MAX_THREAD MAXTHREADS/2
#else
#define MAX_THREAD 8
#endif 

#ifndef USE_MALLOC
uint32_t randints[MAX_ARRAY_SIZE];
#else
#include <inc/dlmalloc.h>
uint32_t *randints = NULL;
#endif

#include <inc/rngs.h>
#define MAXINT (0xffffffff)

void
gen_randints(uint32_t *randints, size_t n, int seed) {
	SelectStream(0);
	PutSeed(seed); //use a negative seed to activate sys_time dependent seed!
	int i;
	for(i = 0; i < n; ++i) {
		randints[i] = (uint32_t)(MAXINT * Random());
		//cprintf("%d ", randints[i]);
	}
	//cprintf("\n");
}

void
testpqsort(int array_size, int nthread)
{	
#ifdef USE_MALLOC
	randints = malloc(array_size*4);
#endif
	assert(nthread <= MAX_THREAD);
	int iter = 0;
	uint64_t tt = 0ull;
	for(; iter < niter; ++iter) {
		gen_randints(randints, array_size, 1);
		uint64_t ts = get_time();
		pqsort(randints, randints + array_size - 1, nthread);
		uint64_t td = get_time();
		cprintf("test %d uses time: %lld\n", iter, td-ts);
		tt += (td - ts);
		//int i;
		//for(i = 0; i < array_size; ++i)
		//	cprintf("%d ", randints[i]);
	}
	
	cprintf("array_size: %d\tave. time: %lld\n", array_size, tt/niter);
#ifdef USE_MALLOC
	free(randints);
#endif
}

int
main()
{
	files->thself = 0;
	cprintf("start...\n");
	//testpqsort(10, 2);
	//testpqsort(700000, 8);
	//testpqsort(800000, 8);
	//testpqsort(900000, 8);
	testpqsort(500000, MAX_THREAD);
	testpqsort(1000000, MAX_THREAD);
	return 0;
}

#endif // LAB >= 3
