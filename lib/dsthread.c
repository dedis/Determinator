// Deterministically scheduled "compatibility" implementation of pthreads,
// supporting nondeterministic constructs like mutexes and condition variables.

#define PIOS_DSCHED

#include <inc/string.h>
#include <inc/stdlib.h>
#include <inc/syscall.h>
#include <inc/pthread.h>
#include <inc/signal.h>
#include <inc/assert.h>
#include <inc/errno.h>
#include <inc/file.h>
#include <inc/x86.h>
#include <inc/mmu.h>
#include <inc/vm.h>


// Tunable scheduling policy parameters (could be made variables).
#define P_QUANTUM	0	// Number of instructions per thread quantum
//#define P_QUANTUM	10000	// Number of instructions per thread quantum
#define P_MUTEXFAIR	0	// Mutex transfer in strict round-robin order
#define P_MUTEXIMMED	0	// Pass on mutex immediately on unlock


#define MAXTHREADS	256		// must be power of two
#define MAXKEYS		1000

// Thread states
#define TH_FREE		0		// unused thread slot
#define TH_RUN		1		// thread is runnable
#define TH_MUTEX	2		// blocked locking a mutex
#define TH_COND		3		// waiting on a condition variable
#define TH_EXIT		4		// thread has exited

// Globally visible per-thread state - this is what pthread_t points to.
typedef struct pthread {
	int		tno;		// thread number in master process
	int		state;		// thread's current state
	int		detached;	// true if thread can never join
	struct pthread *qnext;		// next on scheduler or wait queue
	pthread_mutex_t*reqs;		// mutexes req'd by other threads
	struct pthread *joiner;		// thread blocked joining this thread
	void *		exitval;	// value thread returned on exit

	// state save area for blocked (but not preempted) threads
	uint32_t	pff;		// process feature flags
	uint32_t	ebp;		// frame pointer
	uint32_t	esp;		// stack pointer
	uint32_t	eip;		// instruction pointer
} pthread;

// Per-thread state control block, at fixed address 0xeffff000.
// The kernel sets up the %gs register with this offset.
typedef struct threadpriv {
	// Pointer to thread-private global data laid out by the linker.
	// GCC-generated code reads this via %gs:0 to find thread-private data.
	void		*tlshi;
	void		*tlslo;

	// We place a 3-instruction sequence in this page,
	// which child processes use to make calls into the master process.
	// In child processes, this code fragment is INT $T_SYSCALL; RET,
	// whereas in the master process, it's just RET; RET; RET.
	// Using this code to call the master avoids the race condition
	// where we get preempted just after checking that we're a child,
	// and then mistakenly return to the master's parent instead of the master.
	char		mcall[3];
} threadpriv;
#define THREADPRIV	((threadpriv*)(VM_PRIVHI - PAGESIZE))

// Another special "thread-private" page,
// which contains the scheduler stack in the master process.
#define SCHEDSTACKLO	(VM_PRIVLO + PAGESIZE)
#define SCHEDSTACKHI	(VM_PRIVLO + PAGESIZE*2)

// Divide our total stack address space into equal-size per-thread chunks.
// Arrange them downward so that the initial stack is in the "thread 0" area.
#define TSTACKSIZE	((VM_STACKHI - VM_STACKLO) / MAXTHREADS)
#define TSTACKHI(tno)	(VM_STACKHI - (tno) * TSTACKSIZE)
#define TSTACKLO(tno)	(TSTACKHI(tno) - TSTACKSIZE)

#if 0
// Use half of the general-purpose "shared" address space area as heap,
// and divide this heap space into equal-size per-thread chunks.
#define HEAPSIZE	((VM_SHAREHI - VM_SHARELO) / 2)
#define HEAPLO		(VM_SHAREHI - HEAPSIZE)
#define HEAPHI		(VM_SHAREHI)
#define THEAPSIZE	(HEAPSIZE / MAXTHREADS)
#define THEAPLO(tno)	(HEAPLO + (tno) * THEAPSIZE)
#define THEAPHI(tno)	(HEAPLO + (tno) * THEAPSIZE + THEAPSIZE)
#endif


static pthread th[MAXTHREADS];
static int tmax;	// first unused thread

// This word serves as the main low-level "lock" for the thread system:
//	-1 = thread library not yet initialized
//	0 = running normally as a child process
//	1 = running nonpreemptively as a child process
//	2 = running nonpreemptively as the master process
// When the master process preempts a child and discovers tlock == 1,
// it just sets tlock = 2 and resumes execution of the thread (as the master),
// giving the pthread code running on behalf of the thread a chance to finish.
static volatile int tlock = -1;

// Round-robin scheduler queues.
// The ready queue contains threads that are definitely waiting to be run;
// the run queue contains threads whose results we need to collect in turn.
// We always process the ready queue before the run queue.
static pthread *readyqhead = NULL;
static pthread **readyqtail = &readyqhead;
static pthread *runqhead = NULL;
static pthread **runqtail = &runqhead;
static int runqlen;

// The scheduler runs on a thread-private stack in the master process.
static __thread char schedstack[PAGESIZE];

// List of condition variables we have signaled during this timeslice.
// Each thread effectively maintains its own list during its timeslice,
// but it gets consumed and zeroed the next time the thread syncs up,
// which is why all threads can use this (shared) location without conflict.
static pthread_cond_t *evconds;

// Thread-specific state via the pthreads API
typedef void (*destructor)(void *);
static pthread_key_t nextkey = 1;	// next key to be assigned
static destructor dtors[MAXKEYS];	// destructor for each key
static __thread void *tspec[MAXKEYS];	// thread-specific value for each key



static void tinit(pthread_t t);
static void tpinit(pthread_t t);
static void mret(void);
static void mutexreqs(pthread_t t);
static void condwakeups(void);

static gcc_inline int
selfno(void)
{
	int tno = (uint32_t)(VM_STACKHI - read_esp()) / TSTACKSIZE;
	assert(tno >= 0 && tno < MAXTHREADS);
	return tno;
}

static gcc_inline pthread *
self(void)
{
	return &th[selfno()];
}

// Child process code for the MCALLPAGE.
asm("cmcalls: int $48; ret; cmcalle:");
extern char cmcalls[], cmcalle[];

// Master process code for the MCALLPAGE.
asm("mmcalls: ret; ret; ret; mmcalle:");
extern char mmcalls[], mmcalle[];

static volatile __thread int testint = 0x12345678;

static void
init(void)
{
	assert(tlock < 0);
	tlock = 2;		// we start out as the master process

	// Create the scheduler stack.
	sys_get(SYS_PERM | SYS_RW, 0, NULL, NULL,
		(void*)SCHEDSTACKLO, SCHEDSTACKHI - SCHEDSTACKLO);

	// Create the master's version of the threadpriv page.
	static_assert((intptr_t)(THREADPRIV + 1) <= VM_PRIVHI);
	sys_get(SYS_PERM | SYS_RW, 0, NULL, NULL,
		(void*) VM_PRIVHI - PAGESIZE, PAGESIZE);
	assert(mmcalle - mmcalls == sizeof(THREADPRIV->mcall));
	memcpy(THREADPRIV->mcall, mmcalls, mmcalle - mmcalls);

	// We should have been started in thread 0's stack area.
	pthread_t t = &th[0];
	assert(t == self());
	memset(t, 0, sizeof(*t));
	t->state = TH_RUN;

	tinit(t);	// initialize thread 0
	mret();		// drop into thread 0
	tpinit(t);	// initialize thread-private state
}

////////// Scheduling //////////

// List all active threads and their current states, for deadlock debugging.
static void
tdump(void)
{
	int i;
	for (i = 0; i < MAXTHREADS; i++) {
		if (th[i].state == TH_FREE)
			continue;
		cprintf("thread %d: state %d eip %x esp %x\n",
			i, th[i].state, th[i].eip, th[i].esp);
	}
}

// Handle any events relevant to a thread
// before it either starts running again or blocks.
static gcc_inline void
tevents(pthread_t t)
{
	// Handle any wakeup/transfer events for this thread.
	if (evconds != NULL)
		condwakeups();	// process any queued condition var events
	if (t->reqs != NULL)
		mutexreqs(t);
}

// Place a thread we're about to run on the run queue.
// This can be called on either the scheduler's or a thread's stack,
// though always in the context of the master thread.
static void
trun(pthread_t t)
{
	// Handle any outstanding events first
	tevents(t);

	// Place this thread on the run queue,
	// so the scheduler will collect its results sometime later.
	assert(t->qnext == NULL); assert(*runqtail == NULL);
	*runqtail = t;
	runqtail = &t->qnext;
	assert(++runqlen <= MAXTHREADS);
}

// The scheduler runs on the special scheduler stack page,
// dispatching threads and collecting their results in round-robin order.
gcc_noreturn void
pthread_sched(void)
{
	assert(tlock == 2);
	assert(read_esp() > SCHEDSTACKLO);

	// First restart any threads that have been made ready after blocking.
	if (readyqhead != NULL) {
		pthread_t t = readyqhead;
		if ((readyqhead = t->qnext) == NULL)	// dequeue first thread
			readyqtail = &readyqhead;	// reset empty list
		t->qnext = NULL;

		// Resume the thread, still running in the master process.
		asm volatile(
			"	movl	%0,%%ebp;"	// restore thread's ebp
			"	movl	%1,%%esp;"	// restore thread's esp
			"	jmp	*%2;"		// restore thread's eip
			:
			: "r" (t->ebp),
			  "r" (t->esp),
			  "r" (t->eip));
		// (asm fragment does not return here)
	}

	// Now collect results from threads in the run queue.
	// Use a static cpustate struct so we can refer to it below
	// without referring to any registers we're trying to restore.
	static cpustate cs;
	pthread_t t;
	while (1) {
#if P_QUANTUM > 0
{ static int dispcnt, qlencum;
qlencum += runqlen;
if (++dispcnt >= 100000000/P_QUANTUM) {
	cprintf("sched: runqlen %d avg %d\n", runqlen, qlencum / dispcnt);
	qlencum = dispcnt = 0;
} }
#endif
		if (runqhead == NULL) {
			tdump();
			panic("sched: no running threads - deadlock?");
		}
		t = runqhead;
		if ((runqhead = t->qnext) == NULL)	// dequeue first thread
			runqtail = &runqhead;		// for good measure
		t->qnext = NULL;
		assert(--runqlen >= 0);

		// Merge the thread's memory changes and get its register state.
		tlock = 0;	// default state for user procs
		sys_get(SYS_REGS | SYS_MERGE, t->tno, &cs,
			(void*)VM_USERLO, (void*)VM_USERLO,
			VM_PRIVLO - VM_USERLO);
		int olock = tlock;
		tlock = 2;

		if (olock == 1)	// Was the thread running pthread code?
			break;	// if so, resume thread in master below.
		assert(olock == 0);

		// Pass on any traps/returns other than quantum expiration.
		if (cs.tf.tf_trapno == T_SYSCALL) {
			sys_ret();	// pass I/O on to our parent
		} else if (cs.tf.tf_trapno != T_ICNT) {
			panic("sched: thread %d trap %d eip %x",
				t->tno, cs.tf.tf_trapno, cs.tf.tf_eip);
		}

		// We preempted the thread while running normal user code.
		// Process events and return it to the tail of the run queue.
		trun(t);

		// Resume the thread's execution, with new memory state,
		// and the same register state except for a new quantum.
		cs.icnt = 0;
		cs.imax = P_QUANTUM;
		tlock = 0;	// tlock state expected by thread
		sys_put(SYS_REGS | SYS_COPY | SYS_SNAP | SYS_START, t->tno,
			&cs, (void*)VM_USERLO, (void*)VM_USERLO,
			VM_PRIVLO - VM_USERLO);
		tlock = 2;	// tlock state for master process
	}

	// Unexpected trap in pthreads code?
	if (cs.tf.tf_trapno != T_SYSCALL && cs.tf.tf_trapno != T_ICNT)
		panic("sched: thread %d trap %d while mlocked, eip %x",
			t->tno, cs.tf.tf_trapno, cs.tf.tf_eip);

	// If the thread started using the FPU, remember that.
	t->pff = cs.pff;

	// OK, just resume the thread from where it left off,
	// with tlock == 2 so it knows it's the master process.
#if 0
	asm volatile(
		"	pushl	%0;"
		"	popfl;"
		"	movl	%1,%%ebp;"
		"	movl	%2,%%esp;"
		"	jmp	*%3;"
		:
		: "m" (cs.tf.tf_eflags),
		  "m" (cs.tf.tf_regs.reg_ebp),
		  "m" (cs.tf.tf_esp),
		  "m" (cs.tf.tf_eip),
		  "a" (cs.tf.tf_regs.reg_eax),
		  "b" (cs.tf.tf_regs.reg_ebx),
		  "c" (cs.tf.tf_regs.reg_ecx),
		  "d" (cs.tf.tf_regs.reg_edx),
		  "S" (cs.tf.tf_regs.reg_esi),
		  "D" (cs.tf.tf_regs.reg_edi));
#else
	// XXX instead of using popfl to restore the flags,
	// connive to restore just the condition codes and not FL_TF,
	// to avoid interfering with the kernel's TF-based icnt stuff.
	// This should of course not be necessary;
	// the kernel instead needs to scan the instruction stream
	// and emulate any "unsafe" instructions like popfl.
	asm volatile(
		"	movl	%0,%%eax;"
		"	xchgb	%%al,%%ah;"
		"	shlb	$4,%%al;"	// get OF in high bit of AL
		"	shrb	$1,%%al;"	// set real OF with that bit
		"	sahf;"			// restore low 8 bits of eflags
		"	movl	%4,%%eax;"	// restore eax
		"	movl	%1,%%ebp;"
		"	movl	%2,%%esp;"
		"	jmp	*%3;"
		:
		: "m" (cs.tf.tf_eflags),
		  "m" (cs.tf.tf_regs.reg_ebp),
		  "m" (cs.tf.tf_esp),
		  "m" (cs.tf.tf_eip),
		  "m" (cs.tf.tf_regs.reg_eax),
		  "b" (cs.tf.tf_regs.reg_ebx),
		  "c" (cs.tf.tf_regs.reg_ecx),
		  "d" (cs.tf.tf_regs.reg_edx),
		  "S" (cs.tf.tf_regs.reg_esi),
		  "D" (cs.tf.tf_regs.reg_edi));
#endif
	// (asm fragment does not return here)
	while (1);
}

// Block the current thread in a given state.
static void
tblock(pthread_t t, int newstate)
{
	assert(tlock == 2);
	assert(t == self());
	assert(t->state == TH_RUN);

	// Handle any outstanding events first
	tevents(t);

	// Set new thread state
	assert(newstate != TH_RUN);
	t->state = newstate;

	// Save critical registers and invoke scheduler;
	// this thread will continue if/when it gets unblocked.
	asm volatile(
		"	pushl	%%esi;"		// save a few registers
		"	pushl	%%edi;"
		"	movl	%%ebp,%0;"	// save thread's frame pointer
		"	movl	%%esp,%1;"	// save thread's stack pointer
		"	movl	$1f,%2;"	// start address when unblocked
		"	movl	%3,%%esp;"	// switch to scheduler stack
		"	xorl	%%ebp,%%ebp;"	// clear frame pointer
		"	jmp	pthread_sched;"	// invoke the scheduler
		"	.p2align 4,0x90;"
		"1:	popl	%%edi;"
		"	popl	%%esi;"
		: "=m" (t->ebp),
		  "=m" (t->esp),
		  "=m" (t->eip)
		: "i" (SCHEDSTACKHI)
		: "eax", "ebx", "ecx", "edx", "cc", "memory");

	assert(tlock == 2);
}

// Mark a thread ready and add it to the scheduler queue.
static gcc_inline void
tready(pthread_t t)
{
	assert(tlock == 2);
	assert(t->state != TH_RUN);
	assert(t->qnext == NULL);

	t->state = TH_RUN;
	*readyqtail = t;
	readyqtail = &t->qnext;
}

////////// Master entry/exit and preemption control //////////

// Disable preemption.
static /*gcc_inline*/ void
mlock(void)
{
	if (tlock < 0)
		init();

	assert(tlock == 0);
	tlock = 1;
}

// Re-enable preemption.
// If our timeslice hasn't ended, sets tlock = 0 and returns nonzero.
// Otherwise, we're in the master process (tlock == 2), and returns false.
static /*gcc_inline*/ char
munlock(void)
{
	assert(tlock > 0);

	// We need an "atomic" fetch-and-op here
	// to guard against a race with preemption,
	// but it doesn't need to be an atomic RMW memory cycle
	// because normal instruction atomicity is all we need.
	char rc;
	asm volatile("andl $2,%1; setz %0"
		: "=r" (rc), "=m" (tlock) : : "memory", "cc");
	return rc;
}

// Disable preemption and synchronize with the master immediately,
// giving up the rest of our current execution quantum.
static /*gcc_inline*/ void
mcall(void)
{
	if (tlock <= 0)
		mlock();

	// Now actually invoking the master is a bit tricky.
	// We basically just want to do a sys_ret(),
	// but if we get preempted just before the INT instruction,
	// then we'll unintentionally return from the master process - bad!
	// So instead we make a call to a page in the thread-private area,
	// which contains an INT $48 in the child and a RET in the master;
	// that way we'll do the correct thing even if we get preempted
	// just the moment before that instruction is executed.
	asm volatile("call %1"
			: : "a" (SYS_RET), "m" (THREADPRIV->mcall[0])
			: "memory");

	assert(tlock == 2);
	assert(THREADPRIV->mcall[0] == mmcalls[0]);
}

// Return a thread running as master to its proper child process,
// and invoke the scheduler in the master process.
static void
mret(void)
{
	assert(tlock > 0);
	if (munlock())
		return;		// dropped back from tlock == 1 to tlock == 0
	assert(tlock == 2);	// running as master process

	// Handle wakeup events and put the thread on the run queue.
	pthread_t t = self();
	trun(t);

	// Making this cpustate static is safe since only the master uses it,
	// and means we don't have to clear it on each use.
	static cpustate cs = {
		.icnt = 0,
		.imax = P_QUANTUM,
	};
	cs.pff = t->pff | (P_QUANTUM ? PFF_ICNT : 0);

	// Copy our register state and address space to the child,
	// (re)start the child, and invoke the scheduler in the master process.
	asm volatile(
		"	movl	%%ebp,%0;"	// save thread's ebp
		"	movl	%%esp,%1;"	// save thread's esp
		"	movl	$1f,%2;"	// save thread's eip
		"	movl	$0,tlock;"	// child must see tlock == 0
		"	int	%3;"		// INT $T_SYSCALL
		"	movl	$2,tlock;"	// scheduler sees tlock == 2
		"	movl	%10,%%esp;"	// switch to scheduler stack
		"	xorl	%%ebp,%%ebp;"	// clear frame pointer
		"	jmp	pthread_sched;"	// invoke the scheduler
		"	.p2align 4,0x90;"
		"1:	"
		: "=m" (cs.tf.tf_regs.reg_ebp),
		  "=m" (cs.tf.tf_esp),
		  "=m" (cs.tf.tf_eip)
		: "i" (T_SYSCALL),
		  "a" (SYS_PUT | SYS_REGS | SYS_COPY | SYS_SNAP | SYS_START),
		  "d" (t->tno),
		  "b" (&cs),
		  "S" (VM_USERLO),
		  "D" (VM_USERLO),
		  "c" (VM_PRIVLO - VM_USERLO),
		  "i" (SCHEDSTACKHI)
		: "cc", "memory");

	assert(tlock == 0);
}

////////// Threads //////////

// Initialize a given thread slot.
static void
tinit(pthread_t t)
{
	int tno = t->tno;
	assert(t == &th[tno]);
	if (tmax <= tno)
		tmax = tno+1;

	// Set up the new thread's stack and heap.
	// Leave a 1-page redzone at the bottom of each stack.
//	sys_get(SYS_PERM | SYS_RW, 0, NULL, NULL,
//		(void*)THEAPLO(tno), THEAPSIZE);
	sys_get(SYS_PERM, 0, NULL, NULL,		// 1-page red zone
		(void*)TSTACKLO(tno), PAGESIZE);
	sys_get(SYS_PERM | SYS_RW, 0, NULL, NULL,
		(void*)TSTACKLO(tno) + PAGESIZE, TSTACKSIZE - PAGESIZE);

	// Create the child's thread-private area by cloning the master's.
	sys_put(SYS_COPY, tno, NULL,
		(void*)VM_PRIVLO, (void*)VM_PRIVLO, VM_PRIVHI - VM_PRIVLO);
}

// Find the size of the thread-private state area.
// This is only called from tpinit(), but needs to be separate and non-inlined,
// otherwise GCC assumes the (NULL) thread private pointer it reads here
// can be reused later in tpinit() for accessing (real) thread-private data.
static gcc_noinline void
tpgetsize(int *tdatasize, int *tlssize) {
	extern __thread char tdata_start[], tbss_start[];
	*tdatasize = tbss_start - tdata_start;	// size of init'd data

	assert(THREADPRIV->tlshi == NULL);
	*tlssize = -(intptr_t)tdata_start;	// complete tls area

	assert(*tlssize >= *tdatasize);
}

// Initialize a thread's thread-private state area,
// which can be done only from within the thread itself.
static void
tpinit(pthread_t t)
{
	assert(t == self());
	assert(tlock == 0);

	// Set up our child version of the mcall code sequence.
	assert(cmcalle - cmcalls == sizeof(THREADPRIV->mcall));
	memcpy(THREADPRIV->mcall, cmcalls, cmcalle - cmcalls);

	// Figure out how big our GCC thread-local state area needs to be.
	int tdatasize, tlssize;
	tpgetsize(&tdatasize, &tlssize);

	// Allocate a TLS area for this thread.
	// Of course, we could just place the TLS data area
	// at a fixed location in the space above VM_PRIVLO,
	// but then other threads wouldn't be able to refer to our
	// thread-private storage (e.g., via pointers we pass to them),
	// and pthreads-based code sometimes assumes it can do this.
	THREADPRIV->tlslo = malloc(tlssize);
	THREADPRIV->tlshi = THREADPRIV->tlslo + tlssize;

	// Initialize our TLS area from the linker-generated image.
	// We depend on tbss_start coinciding with __init_array_start
	// because of the way the linker lays out executables.  (XXX bad hack.)
	extern char __init_array_start[];
	memcpy(THREADPRIV->tlslo, __init_array_start - tdatasize, tdatasize);
	memset(THREADPRIV->tlslo + tdatasize, 0, tlssize - tdatasize);
	assert(testint == 0x12345678);	// make sure it worked!
}

// Initial entrypoint for threads started via pthread_create().
static gcc_noreturn void
tstart(void *(*start_routine)(void *), void *arg)
{
	assert(tlock == 2);
	mret();					// return to normal execution
	tpinit(self());				// init thread-private state
	pthread_exit(start_routine(arg));	// run the user start function
}

int
pthread_create(pthread_t *out_thread, const pthread_attr_t *attr,
		void *(*start_routine)(void *), void *arg)
{
	mcall();

	// Find an available thread slot
	int tno;
	for (tno = 1; ; tno++) {
		if (tno >= MAXTHREADS)
			panic("pthread_create: too many threads");
		if (th[tno].state == TH_FREE)
			break;
	}
	pthread_t t = &th[tno];
	memset(t, 0, sizeof(*t));
	t->tno = tno;
	if (attr != NULL)
		t->detached = *attr & PTHREAD_CREATE_DETACHED;

	tinit(t);	// initialize the thread's stack

	// Set up thread stack with arguments for tstart()
	uint32_t *esp = (uint32_t*) TSTACKHI(tno);
	*--esp = (uint32_t) arg;
	*--esp = (uint32_t) start_routine;
	--esp;	// space for return address

	t->ebp = 0;	// set register state to start at tstart()
	t->esp = (uint32_t) esp;
	t->eip = (uint32_t) tstart;

	tready(t);	// let it rock let it rock!

	mret();		// but resume current thread first

	*out_thread = t;
	return 0;
}

int
pthread_join(pthread_t tj, void **out_exitval)
{
	mlock();

	// Wait for target thread to exit
	assert(tj == &th[tj->tno]);
	assert(!tj->detached);		// can't join with detached threads!
	if (tj->state != TH_EXIT)	// hasn't exited as of last check -
		mcall();		// give up timeslice and check for sure
	if (tj->state != TH_EXIT) {	// hasn't yet exited - must wait for it
		assert(tj->state > TH_FREE);
		assert(tj->joiner == NULL);	// only one joiner allowed
		pthread_t t = self();
		tj->joiner = t;
		tblock(t, TH_MUTEX);	// block until the thread exits
	}
	assert(tj->state == TH_EXIT);

	// Retrieve the thread's exit value
	if (out_exitval != NULL)
		*out_exitval = tj->exitval;

	// Free the thread for subsequent reuse
	tj->state = TH_FREE;

	mret();
	return 0;
}

void gcc_noreturn
pthread_exit(void *exitval)
{
	mcall();

	pthread_t t = self();
	assert(t->state == TH_RUN);

	// wake up any thread trying to join with us
	if (t->joiner != NULL)
		tready(t->joiner);

	// save our exit status and block until joiner collects it,
	// or just go to TH_FREE immediately if we're detached.
	t->exitval = exitval;
	tblock(t, t->detached ? TH_FREE : TH_EXIT);
	panic("exited thread should not have unblocked");
}

int pthread_attr_init(pthread_attr_t *attr)
{
	*attr = PTHREAD_CREATE_JOINABLE;
	return 0;
}

int pthread_attr_destroy(pthread_attr_t *attr)
{
	return 0;
}

int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate)
{
	*detachstate = *attr;
	return 0;
}

int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate)
{
	*attr = detachstate;
	return 0;
}

int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize)
{
	*stacksize = TSTACKSIZE;
	return 0;
}

int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize)
{
	warn("pthread_attr_setstacksize: ignoring request for stack size %d",
		stacksize);
	return 0;
}

////////// Mutexes //////////

int
pthread_mutex_init(pthread_mutex_t *m, const pthread_mutexattr_t *attr)
{
	memset(m, 0, sizeof(*m));
	m->owner = self();
	m->qtail = &m->qhead;
	return 0;
}

int
pthread_mutex_destroy(pthread_mutex_t *m)
{
	assert(!m->locked);
	assert(m->qhead == NULL);
	m->owner = NULL;
	return 0;
}

static void
mutexlock(pthread_t t, pthread_mutex_t *m)
{
	assert(tlock > 0);
	assert(t == self());

	// XX can we make 'owner' just a flag in the mutex and avoid self()?
	if (m->owner != t || (P_MUTEXFAIR && m->qhead != NULL)) {
		mcall();	// give up timeslice, synchronize with master

		// If the current owner is blocked, steal the mutex.
		if (!m->locked && m->owner->state != TH_RUN) {
			assert(m->qhead == NULL); // else would have xfer'd
			assert(m->qtail == &m->qhead);
			assert(m->reqnext == NULL);
			m->owner = t;
		}
	}
	if (m->owner != t || (P_MUTEXFAIR && m->qhead != NULL)) {

		// Request mutex from its current owner
		if (m->qhead == NULL) {
			pthread_t tc = m->owner;
			assert(tc != NULL); assert(m->reqnext == NULL);
			m->reqnext = tc->reqs;
			tc->reqs = m;
		}

		// Enqueue us on mutex's thread queue
		assert(t->qnext == NULL); assert(*m->qtail == NULL);
		*m->qtail = t;
		m->qtail = &t->qnext;
		assert(m->qhead != NULL); assert(*m->qtail == NULL);

		tblock(t, TH_MUTEX);	// block until we obtain the mutex
	}
	assert(m->owner == t);

	// Now we can grab the mutex
	if (m->locked)
		panic("pthread_mutex_lock: mutex %x already locked", m);
	m->locked = 1;
}

int
pthread_mutex_lock(pthread_mutex_t *m)
{
	pthread *t = self();
	mlock();
	mutexlock(t, m);
	mret();
	return 0;
}

int
pthread_mutex_trylock(pthread_mutex_t *m)
{
	pthread *t = self();
	mlock();
	int rc;
	if (m->owner != t || (P_MUTEXFAIR && m->qhead != NULL)) {
		rc = EBUSY;	// already locked
	} else {
		assert(!m->locked);	// shouldn't already be locked
		m->locked = 1;
		rc = 0;
	}
	mret();
	return rc;
}

static gcc_inline void
mutexunlock(pthread_t t, pthread_mutex_t *m)
{
	assert(tlock > 0);
	assert(t == self());
	assert(m->owner == t);
	assert(m->locked);

	if (P_MUTEXIMMED && m->qhead != NULL)
		mcall();	// give up our quantum and pass the mutex now

	m->locked = 0;
}

int
pthread_mutex_unlock(pthread_mutex_t *m)
{
	pthread *t = self();
	mlock();
	mutexunlock(t, m);
	mret();
	return 0;
}

// Handle any requests other threads have made for mutexes we own,
// changing the ownership of any mutexes we're not currently holding.
// This function can run on either thread's or the scheduler's stack.
static void
mutexreqs(pthread_t t)
{
	assert(tlock == 2);

	pthread_mutex_t *m, **mp = &t->reqs;
	while ((m = *mp) != NULL) {
		assert(m->owner == t); assert(m->qhead != NULL);
		if (m->locked) {	// can't transfer a locked mutex
			mp = &m->reqnext;	// so just skip this one
			continue;
		}
		*mp = m->reqnext;	// un-chain this mutex

		// Transfer the mutex to its new owner
		pthread_t tn = m->qhead;
		assert(tn != NULL); assert(tn->state == TH_MUTEX);
		m->owner = tn;		// change ownership
		m->qhead = tn->qnext;	// remove new owner from mutex's queue
		if (m->qhead != NULL) {	// queue still not empty?
			// other threads are waiting,
			// so add mutex to new owner's reqs list.
			m->reqnext = tn->reqs;
			tn->reqs = m;
			tn->qnext = NULL;
		} else {		// mutex's queue now empty
			m->reqnext = NULL;
			m->qtail = &m->qhead;	// reset empty list
		}

		// Let the new owner thread run
		tready(tn);
	}
}

int pthread_mutexattr_init(pthread_mutexattr_t *attr)
{
	*attr = 0;
	return 0;
}

int pthread_mutexattr_destroy(pthread_mutexattr_t *attr)
{
	return 0;
}

////////// Condition variables //////////

int
pthread_cond_init(pthread_cond_t *c, const pthread_condattr_t *attr)
{
	memset(c, 0, sizeof(*c));
	c->qtail = &c->qhead;
	return 0;
}

int
pthread_cond_destroy(pthread_cond_t *c)
{
	assert(c->nwake == 0);
	assert(c->qhead == NULL);
	return 0;
}

static int condevent(pthread_cond_t *c, bool broadcast)
{
	if (c->qhead == NULL)
		return 0;	// no one waiting, nothing to do

	mlock();

	pthread *t = self();
	if (c->nwake == 0) {	// add cond to our thread's signal chain
		assert(c->evnext == NULL);
		c->evnext = evconds;
		evconds = c;
	}
	if (broadcast)
		c->nwake = INT_MAX;
	else
		c->nwake++;
	//cprintf("cond_event thread %d %x bcast %d nwake %d\n",
	//	t->tno, c, broadcast, c->nwake);

	// Now return to nonpreemptible execution.
	// If our timeslice ended and we've become the master by now,
	// this will handle and clear the evconds list.
	mret();
	return 0;
}

int
pthread_cond_signal(pthread_cond_t *c)
{
	return condevent(c, 0);
}

int
pthread_cond_broadcast(pthread_cond_t *c)
{
	return condevent(c, 1);
}

int
pthread_cond_wait(pthread_cond_t *c, pthread_mutex_t *m)
{
	mcall();

	// Enqueue us on the condition variable
	pthread *t = self();
	assert(t->state == TH_RUN);
	assert(t->qnext == NULL); assert(*c->qtail == NULL);
	t->qnext = *c->qtail;
	*c->qtail = t;
	c->qtail = &t->qnext;

	//cprintf("cond_wait thread %d cond %x\n", t->tno, c);
	mutexunlock(t, m);	// unlock mutex (while nonpreemptible)
	tblock(t, TH_COND);	// block until signaled
	mutexlock(t, m);	// re-lock the mutex
	mret();			// resume normal execution
	return 0;
}

// Process any wakeups queued on the evconds list.
// This function can run on either thread's or the scheduler's stack.
static void
condwakeups(void)
{
	assert(tlock == 2);
	while (evconds != NULL) {
		pthread_cond_t *c = evconds;
		assert(c->nwake > 0);

		// Wake up one or all waiting threads, depending on c->event
		do {
			pthread *tw = c->qhead;
			if (tw == NULL)
				break;
			c->qhead = tw->qnext;	// unlink thread from queue
			assert(tw->state == TH_COND);
			tw->qnext = NULL;
			tready(tw);		// let it run again

		} while (--c->nwake > 0);
		if (c->qhead == NULL)	// reset tail if queue is now empty
			c->qtail = &c->qhead;

		evconds = c->evnext;
		c->nwake = 0;
		c->evnext = NULL;
	}
}

int pthread_condattr_init(pthread_condattr_t *attr)
{
	*attr = 0;
	return 0;
}

int pthread_condattr_destroy(pthread_condattr_t *attr)
{
	return 0;
}


////////// Thread-specific data //////////

int pthread_key_create(pthread_key_t *keyp, destructor dtor)
{
	mcall();

	if (nextkey >= MAXKEYS)
		panic("pthread_key_create: out of thread-specific keys");
	dtors[nextkey] = dtor;
	*keyp = nextkey++;

	mret();
	return 0;
}

int pthread_key_delete(pthread_key_t key)
{
	assert(key > 0 && key < MAXKEYS);

	mcall();
	dtors[nextkey] = NULL;
	mret();
	return 0;
}

void *pthread_getspecific(pthread_key_t key)
{
	assert(key > 0 && key < MAXKEYS);
	return tspec[key];
}

int pthread_setspecific(pthread_key_t key, const void *val)
{
	assert(key > 0 && key < MAXKEYS);
	tspec[key] = (void*) val;
	return 0;
}

////////// Memory allocation //////////

extern char end[];
static void *brk = end;

void *
malloc(size_t size)
{
	mcall();

	// Allocate the requested memory
	void *ptr = brk;
	void *nbrk = ptr + ROUNDUP(size, 8);
	if (nbrk > (void*) VM_SHAREHI)
		panic("malloc: can't alloc chunk of size %d", size);
	brk = nbrk;

	// Make sure the new memory is accessible
	void *pgold = ROUNDUP(ptr, PAGESIZE);
	void *pgnew = ROUNDUP(nbrk, PAGESIZE);
	if (pgnew > pgold)
		sys_get(SYS_PERM | SYS_RW, 0, NULL, NULL, pgold, pgnew - pgold);

	mret();

	return ptr;
}

void *
calloc(size_t nelt, size_t eltsize)
{
	size_t size = nelt * eltsize;
	void *ptr = malloc(size);
	if (ptr == NULL)
		return ptr;
	memset(ptr, 0, size);
	return ptr;
}

void *
realloc(void *ptr, size_t newsize)
{
	if (ptr == NULL)
		return malloc(newsize);
	panic("realloc");
}

void
free(void *ptr)
{
	// XXX
}

////////// Signal handling //////////

sighandler_t
signal(int sig, sighandler_t newhandler)
{
	warn("signal() not implemented");
	return NULL;
}

