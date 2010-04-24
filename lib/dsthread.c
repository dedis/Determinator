// Deterministically scheduled "compatibility" implementation of pthreads,
// supporting nondeterministic constructs like mutexes and condition variables.

#define PIOS_DSCHED

#include <inc/string.h>
#include <inc/syscall.h>
#include <inc/pthread.h>
#include <inc/signal.h>
#include <inc/assert.h>
#include <inc/file.h>
#include <inc/x86.h>
#include <inc/mmu.h>
#include <inc/vm.h>


// Tunable scheduling policy parameters (could be made variables).
#define P_QUANTUM	100	// Number of instructions per thread quantum
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
	struct pthread *qnext;		// next on scheduler or wait queue
	pthread_mutex_t*reqs;		// mutexes req'd by other threads
	struct pthread *joiner;		// thread blocked joining this thread
	void *		exitval;	// value thread returned on exit

	// state save area for blocked (but not preempted) threads
	uint32_t	ebp;		// frame pointer
	uint32_t	esp;		// stack pointer
	uint32_t	eip;		// instruction pointer
} pthread;

// Per-thread state page, starting at VM_PRIVLO
typedef struct threadpriv {
	void *		brk;		// heap allocation pointer
	void *		tspec[MAXKEYS];	// thread-specific value for each key
} threadpriv;
#define THREADPRIV	((threadpriv*)VM_PRIVLO)

// Special "thread-private" page that mcall() uses to call the master process.
// In the master process, this page just contains a RET instruction;
// in all child processes representing threads, it does a sys_ret().
#define MCALLPAGE	((void*)VM_PRIVLO + PAGESIZE)

// Another special "thread-private" page,
// which contains the scheduler stack in the master process,
// but is unmapped in all child processes.
#define SCHEDSTACKHI	(VM_PRIVHI)
#define SCHEDSTACKLO	(VM_PRIVHI - PAGESIZE)

// Divide our total stack address space into equal-size per-thread chunks.
// Arrange them downward so that the initial stack is in the "thread 0" area.
#define TSTACKSIZE	((VM_STACKHI - VM_STACKLO) / MAXTHREADS)
#define TSTACKHI(tno)	(VM_STACKHI - (tno) * TSTACKSIZE)
#define TSTACKLO(tno)	(TSTACKHI(tno) - TSTACKSIZE)

// Use half of the general-purpose "shared" address space area as heap,
// and divide this heap space into equal-size per-thread chunks.
#define HEAPSIZE	((VM_SHAREHI - VM_SHARELO) / 2)
#define HEAPLO		(VM_SHAREHI - HEAPSIZE)
#define HEAPHI		(VM_SHAREHI)
#define THEAPSIZE	(HEAPSIZE / MAXTHREADS)
#define THEAPLO(tno)	(HEAPLO + (tno) * THEAPSIZE)
#define THEAPHI(tno)	(HEAPLO + (tno) * THEAPSIZE + THEAPSIZE)


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
static pthread *readyqhead;
static pthread **readyqtail = &readyqhead;
static pthread *runqhead;
static pthread **runqtail = &runqhead;

// List of condition variables we have signaled during this timeslice.
// Each thread effectively maintains its own list during its timeslice,
// but it gets consumed and zeroed the next time the thread syncs up,
// which is why all threads can use this (shared) location without conflict.
static pthread_cond_t *evconds;

typedef void (*destructor)(void *);
static pthread_key_t nextkey = 1;
static destructor dtors[MAXKEYS];


static void tinit(pthread_t t);
static void tpinit(void);
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

static void
init(void)
{
	assert(tlock < 0);
	tlock = 2;		// we start out as the master process

	// Create the master's version of the mcall-page.
	assert(sizeof(threadpriv) <= PAGESIZE);
	sys_get(SYS_PERM | SYS_RW, 0, NULL, NULL, MCALLPAGE, PAGESIZE);
	memcpy(MCALLPAGE, mmcalls, mmcalle - mmcalls);

	// Create the scheduler stack.
	sys_get(SYS_PERM | SYS_RW, 0, NULL, NULL,
		(void*)SCHEDSTACKLO, PAGESIZE);
	sys_get(SYS_PERM, 0, NULL, NULL,
		(void*)SCHEDSTACKLO-PAGESIZE, PAGESIZE);

	// We should have been started in thread 0's stack area.
	pthread_t t = &th[0];
	assert(t == self());
	tinit(t);	// initialize thread 0
	mret();		// drop into thread 0
	tpinit();	// initialize thread-private state
}

////////// Scheduling //////////

// Place a thread we're about to run on the run queue.
// This can be called on either the scheduler's or a thread's stack,
// though always in the context of the master thread.
static void
trun(pthread_t t)
{
	// Handle any wakeup/transfer events for this thread.
	if (evconds != NULL)
		condwakeups();	// process any queued condition var events
	if (t->reqs != NULL)
		mutexreqs(t);

	// Place this thread on the run queue,
	// so the scheduler will collect its results sometime later.
	assert(t->qnext == NULL); assert(*runqtail == NULL);
	*runqtail = t;
	runqtail = &t->qnext;
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
		if (runqhead == NULL)
			panic("sched: no running threads - deadlock?");
		t = runqhead;
		if ((runqhead = t->qnext) == NULL)	// dequeue first thread
			runqtail = &runqhead;		// for good measure
		t->qnext = NULL;

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

	// OK, just resume the thread from where it left off,
	// with tlock == 2 so it knows it's the master process.
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

	// Set new thread state
	assert(newstate != TH_RUN);
	t->state = newstate;

	// Save critical registers and invoke scheduler;
	// this thread will continue when it gets unblocked.
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
		"	popl	%%edi;"
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
static gcc_inline void
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
static gcc_inline char
munlock(void)
{
	assert(tlock != 0);

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
static gcc_inline void
mcall(void)
{
	if (tlock == 0)
		mlock();

	// Now actually invoking the master is a bit tricky.
	// We basically just want to do a sys_ret(),
	// but if we get preempted just before the INT instruction,
	// then we'll unintentionally return from the master process - bad!
	// So instead we make a call to a page in the thread-private area,
	// which contains an INT $48 in the child and a RET in the master;
	// that way we'll do the correct thing even if we get preempted
	// just the moment before that instruction is executed.
	asm volatile("call %1" : : "a" (SYS_RET), "m" (*(char*)MCALLPAGE));

	assert(tlock == 2);
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
		.pff = P_QUANTUM ? PFF_ICNT : 0,
		.icnt = 0,
		.imax = P_QUANTUM,
	};

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
	sys_get(SYS_PERM | SYS_RW, 0, NULL, NULL,
		(void*)THEAPLO(tno), THEAPSIZE);
	sys_get(SYS_PERM, 0, NULL, NULL,		// 1-page red zone
		(void*)TSTACKLO(tno), PAGESIZE);
	sys_get(SYS_PERM | SYS_RW, 0, NULL, NULL,
		(void*)TSTACKLO(tno) + PAGESIZE, TSTACKSIZE - PAGESIZE);
}

// Initialize a thread's thread-private state area,
// which can be done only from within the thread itself.
static void
tpinit(void)
{
	// Set up our thread-private storage page.
	assert(sizeof(threadpriv) <= PAGESIZE);
	sys_get(SYS_PERM | SYS_RW, 0, NULL, NULL, THREADPRIV, PAGESIZE);

	// Set up our child version of the mcall-page.
	sys_get(SYS_PERM | SYS_RW, 0, NULL, NULL, MCALLPAGE, PAGESIZE);
	memcpy(MCALLPAGE, cmcalls, cmcalle - cmcalls);
	assert((cmcalle - cmcalls) == (mmcalle - mmcalls));
}

// Initial entrypoint for threads started via pthread_create().
static gcc_noreturn void
tstart(void *(*start_routine)(void *), void *arg)
{
	assert(tlock == 2);
	mret();			// return to normal execution

	pthread_exit(start_routine(arg));
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
	if (tj->state != TH_EXIT) {	// hasn't yet exited - must wait for it
		mcall();

		assert(tj->state > TH_FREE);
		assert(tj->joiner == NULL);	// only one joiner allowed
		pthread_t t = self();
		tj->joiner = t;
		tblock(t, TH_MUTEX);	// block until the thread exits
	}
	assert(tj->state == TH_EXIT);

	// Retrieve the thread's exit value
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
	assert(t->state > TH_FREE && t->state < TH_EXIT);

	// wake up any thread trying to join with us
	if (t->joiner != NULL)
		tready(t->joiner);

	// save our exit status and block until joiner collects it
	t->exitval = exitval;
	tblock(t, TH_EXIT);
	panic("exited thread should not have unblocked");
}

////////// Mutexes //////////

int
pthread_mutex_init(pthread_mutex_t *m, const pthread_mutexattr_t *attr)
{
	memset(m, 0, sizeof(*m));
	m->owner = self();
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

int
pthread_mutex_lock(pthread_mutex_t *m)
{
	mlock();

	// XX can we make 'owner' just a flag in the mutex and avoid self()?
	pthread *t = self();
	if (m->owner != t || (P_MUTEXFAIR && m->qhead != NULL)) {
		mcall();	// give up timeslice, synchronize with master

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
		m->qtail = &t;
		assert(m->qhead != NULL);

		tblock(t, TH_MUTEX);	// block until we obtain the mutex
	}
	assert(m->owner == t);

	// Now we can grab the mutex
	if (m->locked)
		panic("pthread_mutex_lock: mutex %x already locked", m);
	m->locked = 1;

	mret();
	return 0;
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
	assert(t == self());
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
		m->qhead = tn->qnext;
		if (m->qhead != NULL) {
			// other threads are waiting,
			// so add mutex to new owner's reqs list.
			m->reqnext = tn->reqs;
			tn->reqs = m;
		} else {
			m->reqnext = NULL;
			m->qtail = &m->qhead;	// reset now-empty list
		}

		// Let the new owner thread run
		tready(tn);
	}
}

////////// Condition variables //////////

int
pthread_cond_init(pthread_cond_t *c, const pthread_condattr_t *attr)
{
	memset(c, 0, sizeof(*c));
	return 0;
}

int
pthread_cond_destroy(pthread_cond_t *c)
{
	assert(c->event == 0);
	assert(c->qhead == NULL);
	return 0;
}

static int condevent(pthread_cond_t *c, int ev)
{
	if (c->qhead == NULL)
		return 0;	// no one waiting, nothing to do

	mlock();

	pthread *t = self();
	if (c->event == 0) {	// add cond to our thread's signal chain
		assert(c->evnext == NULL);
		c->evnext = evconds;
		evconds = c;
	}
	c->event |= ev;

	// Now return to nonpreemptible execution.
	// If our timeslice ended and we've become the master by now,
	// this will handle and clear the evconds list.
	mret();
	return 0;
}

int
pthread_cond_signal(pthread_cond_t *c)
{
	return condevent(c, 1);
}

int
pthread_cond_broadcast(pthread_cond_t *c)
{
	return condevent(c, 3);
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

	mutexunlock(t, m);	// unlock mutex (while nonpreemptible)
	tblock(t, TH_COND);	// block until signaled
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
		assert(c->event != 0);

		// Wake up one or all waiting threads, depending on c->event
		do {
			pthread *tw = c->qhead;
			if (tw == NULL)
				break;
			c->qhead = tw->qnext;	// unlink thread from queue
			assert(tw->state == TH_COND);
			tw->qnext = NULL;
			tready(tw);		// let it run again

		} while (c->event > 1);
		if (c->qhead == NULL)	// reset tail if queue is now empty
			c->qtail = &c->qhead;

		evconds = c->evnext;
		c->event = 0;
		c->evnext = NULL;
	}
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
	return THREADPRIV->tspec[key];
}

int pthread_setspecific(pthread_key_t key, const void *val)
{
	assert(key > 0 && key < MAXKEYS);
	THREADPRIV->tspec[key] = (void*) val;
	return 0;
}

////////// Memory allocation //////////

void *
malloc(size_t size)
{
	void *ptr = THREADPRIV->brk;
	void *nbrk = ptr + ROUNDUP(size, 8);
	if (nbrk > (void*)THEAPHI(self()->tno))
		panic("malloc: out of memory");
	THREADPRIV->brk = nbrk;
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

