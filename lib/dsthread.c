// Deterministically scheduled "compatibility" implementation of pthreads,
// supporting nondeterministic constructs like mutexes and condition variables.

#define PIOS_DSCHED

#include <inc/string.h>
#include <inc/syscall.h>
#include <inc/pthread.h>
#include <inc/file.h>
#include <inc/vm.h>


#define MAXTHREADS	256		// must be power of two
#define MAXKEYS		1000

// Thread states
#define TH_RUN		1		// thread is runnable
#define TH_MUTEX	2		// blocked locking a mutex
#define TH_COND		3		// waiting on a condition variable

// Globally visible per-thread state - this is what pthread_t points to.
typedef struct pthread {
	int		tno;		// thread number in master process
	int		state;		// thread's current state
	struct pthread *next;		// next on scheduler or wait queue
	pthread_mutex_t*reqs;		// mutexes req'd by other threads
} pthread;

// Per-thread state page, starting at VM_PRIVLO
typedef struct threadpriv {
	void *		brk;		// heap allocation pointer
	void *		tspec[MAXKEYS];	// thread-specific value for each key
} threadpriv;
#define THREADPRIV	((threadpriv*)VM_PRIVLO)

// Divide our total stack address space into equal-size per-thread chunks.
// Arrange them downward so that the initial stack is in the "thread 0" area.
#define STACKSIZE	((VM_STACKHI - VM_STACKLO) / MAXTHREADS)
#define STACKHI(tno)	(VM_STACKHI - (tno) * STACKSIZE)
#define STACKLO(tno)	(STACKHI(tno) - STACKSIZE)

// Use half of the general-purpose "shared" address space area as heap,
// and divide this heap space into equal-size per-thread chunks.
#define HEAPSIZE	((VM_SHAREHI - VM_SHARELO) / 2)
#define HEAPLO		(VM_SHAREHI - HEAPSIZE)
#define HEAPHI		(VM_SHAREHI)
#define THEAPSIZE	(VM_HEAPSIZE / MAXTHREADS)
#define THEAPLO(tno)	(HEAPLO + (tno) * THEAPSIZE)
#define THEAPHI(tno)	(HEAPLO + (tno) * THEAPSIZE + THEAPSIZE)

static bool inited;
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
static int tlock = -1;

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
static pthread_mutex *wakemutexes;
static pthread_cond *evconds;

typedef void (*destructor)(void *);
static pthread_key_t nextkey = 1;
static destructor dtors[MAXKEYS];

static schedstack[PAGESIZE];


static void tinit(int tno);
static void condwakeups(void);

static gcc_inline int
selfno(void)
{
	int tno = (uint32_t)(VM_STACKHI - read_esp()) / STACKSIZE;
	assert(tno >= 0 && tno < MAXTHREADS);
	return tno;
}

static gcc_inline pthread *
self(void)
{
	return &th[selfno()];
}

static void
init(void)
{
	assert(!inited);
	inited = true;
	tlock = 2;		// we start out as the master process

	// Create the thread-private state page for the master
//	assert(sizeof(threadpriv) <= PAGESIZE);
//	sys_get(SYS_PERM | SYS_RW, 0, NULL, NULL, THREADPRIV, PAGESIZE);

	// We should have been started in thread 0's stack area.
	assert(selfno() == 0);
	tinit(0);
	mret();
}

////////// Scheduler //////////

// 
static void sched(void)
{
}

// Mark a thread ready and add it to the scheduler queue.
static gcc_inline void
tready(pthread *t)
{
	assert(tlock == 2);
	assert(t->state != TH_RUN);
	assert(t->next == NULL);

	t->state = TH_RUN;
	*readyqtail = t;
	readyqtail = &t->next;
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
	asm volatile("andl $2,%1; sz %0"
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
	sys_ret();
	assert(tlock == 2);
}

// Return a thread running as master to its proper child process,
// and invoke the scheduler in the master process.
static void
mret(void)
{
	assert(tlock == 2);

	if (evconds != NULL)
		condwakeups();	// process any queued condition var events

	int tno = selfno();
	...
}

////////// Threads //////////

// Initialize a given thread slot.
static void
tinit(int tno)
{
	assert(tno >= 0 && tno < MAXTHREADS);
	pthread *self = &th[tno];
	assert(self->tno == 0);
	self->tno = tno;
	if (tmax <= tno)
		tmax = tno+1;

	// Set up the new thread's stack and heap.
	// Leave a 1-page redzone at the bottom of each stack.
	sys_get(SYS_PERM | SYS_RW, 0, NULL, NULL, THEAPLO(tno), THEAPSIZE);
	sys_get(SYS_PERM, 0, NULL, NULL, TSTACKLO(tno), PAGESIZE);
	sys_get(SYS_PERM | SYS_RW, 0, NULL, NULL,
		TSTACKLO(tno) + PAGESIZE, TSTACKSIZE - PAGESIZE);
}

static void
callmaster(void (*fun)(void *dat), void *dat)
{
	
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
		if (th[tno].tno == 0)
			break;
	}

	tinit(tno);

	mret();
	return 0;
}

int
pthread_join(pthread_t th, void **out_exitval)
{
}

////////// Mutexes //////////

int
pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
	memset(mutex, 0, sizeof(*mutex));
	mutex->owner = self();
	return 0;
}

int
pthread_mutex_destroy(pthread_mutex_t *mutex)
{
	assert(!mutex->locked);
	assert(mutex->waithead == NULL);
	mutex->owner = NULL;
	return 0;
}

int
pthread_mutex_lock(pthread_mutex_t *mutex)
{
	mlock();

	pthread *t = self();
	if (mutex->owner != t || mutex->waithead != NULL) {
		mcall();	// give up timeslice, synchronize with master
		tblock(TH_MUTEX);	// block until we obtain mutex
	}
	assert(mutex->owner == t);

	if (mutex->locked)
		panic("pthread_mutex_lock: mutex %x already locked",
			mutex);
	mutex->locked = 1;
	return 0;
}

static int mutexunlock(pthread_t t, pthread_mutex_t *mutex)
{
	assert(tlock > 0);
	assert(mutex->owner == t);
	assert(mutex->locked);

	// If other threads are waiting on this mutex,
	// chain the mutex onto the mutexwake list to wake one up.
	if (mutex->waithead != NULL) {
		assert(mutex->wakenext == NULL);
		mutex->wakenext = wakemutexes;
		wakemutexes = mutex;
	}
	mutex->locked = 0;
}

int
pthread_mutex_unlock(pthread_mutex_t *mutex)
{
	pthread *t = self();
	mlock();
	mutexunlock();
	mret();
}

////////// Condition variables //////////

int
pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{
	memset(cond, 0, sizeof(*cond));
	return 0;
}

int
pthread_cond_destroy(pthread_cond_t *cond)
{
	assert(cond->event == 0);
	assert(cond->waiting == NULL);
	return 0;
}

static int condevent(pthread_cond_t *cond, int ev)
{
	if (cond->waiting == NULL)
		return 0;	// no one waiting, nothing to do

	mlock();

	pthread *t = self();
	if (cond->event == 0) {	// add cond to our thread's signal chain
		assert(cond->evnext == NULL);
		cond->evnext = evconds;
		evconds = cond;
	}
	cond->event |= ev;

	// Now return to nonpreemptible execution.
	// If our timeslice ended and we've become the master by now,
	// this will handle and clear the evconds list.
	mret();
}

int
pthread_cond_signal(pthread_cond_t *cond)
{
	return condevent(1);
}

int
pthread_cond_broadcast(pthread_cond_t *cond)
{
	return condevent(3);
}

int
pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
	mcall();

	// Enqueue us on the condition variable
	pthread *t = self();
	assert(t->state == TH_RUN);
	assert(t->waitnext == NULL);
	t->waitnext = *cond->waittail;
	*cond->waittail = t;
	cond->waittail = &t->waitnext;

	mutexunlock(t, mutex);	// unlock mutex (while nonpreemptible)
	tblock(TH_COND);	// block until signaled
	mret();			// resume normal execution
}

// Process any wakeups queued on the evconds list.
static void
condwakeups(void)
{
	assert(tlock == 2);
	while (evconds != NULL) {
		pthread_cond_t *c = evconds;
		assert(c->event != 0);

		// Wake up one or all waiting threads, depending on c->event
		do {
			pthread *t = c->waithead;
			if (t == NULL)
				break;
			c->waithead = t->next;
			assert(t->state == TH_COND);
			t->next = NULL;
			tready(t);		// let it run again
		} while (c->event > 1);
		if (c->waithead == NULL)	// reset tail too if it emptied
			c->waittail = &c->waithead;

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
	assert(THREADPRIV->self != NULL);
	return THREADPRIV->tspec[key];
}

int pthread_setspecific(pthread_key_t key, const void *val)
{
	assert(key > 0 && key < MAXKEYS);
	assert(THREADPRIV->self != NULL);
	THREADPRIV->tspec[key] = (void*) val;
	return 0;
}

////////// Memory allocation //////////

void *
malloc(size_t size)
{
	void *ptr = THREADPRIV->brk;
	void *nbrk = ptr + ROUNDUP(size, 8);
	if (nbrk > THEAPHI(THREADPRIV->tno))
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

void
free(void *ptr)
{
	// XXX
}

