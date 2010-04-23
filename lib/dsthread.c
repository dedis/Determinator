// Deterministically scheduled "compatibility" implementation of pthreads,
// supporting nondeterministic constructs like mutexes and condition variables.

#define PIOS_DSCHED

#include <inc/string.h>
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
	struct pthread *waitnext;	// next on wait queue
	pthread_cond_t *conds;		// condition variables signaled
} pthread;

// Per-thread state page, starting at VM_PRIVLO
typedef struct perthread {
	pthread_t	self;		// current thread, NULL if in master
	int		tno;		// current thread number
	void *		brk;		// heap allocation pointer
	void *		tspec[MAXKEYS];	// thread-specific value for each key
} perthread;

#define PERTHREAD	((perthread*)VM_PRIVLO)
#define SELF		(PERTHREAD->self)

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

typedef void (*destructor)(void *);
static pthread_key_t nextkey = 1;
static destructor dtors[MAXKEYS];

static schedstack[PAGESIZE];


static void tinit(int tno);
static void ptinit(int tno);

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

	// Create the thread-specific state page for the master
	assert(sizeof(perthread) <= PAGESIZE);
	sys_get(SYS_PERM | SYS_RW, 0, NULL, NULL, PERTHREAD, PAGESIZE);
	assert(PERTHREAD->self == NULL);

	// We should have been started in thread 0's stack area.
	assert(selfno() == 0);
	tinit(0);
	become(0);
	ptinit(0);
}

// Initialize a given thread slot.
static void
tinit(int tno)
{
	assert(tno >= 0 && tno < MAXTHREADS);
	pthread *self = &th[tno];
	assert(self->tno == 0);
	self->tno = tno;

	// Set up the new thread's stack and heap.
	// Leave a 1-page redzone at the bottom of each stack.
	sys_get(SYS_PERM | SYS_RW, 0, NULL, NULL, THEAPLO(tno), THEAPSIZE);
	sys_get(SYS_PERM, 0, NULL, NULL, TSTACKLO(tno), PAGESIZE);
	sys_get(SYS_PERM | SYS_RW, 0, NULL, NULL,
		TSTACKLO(tno) + PAGESIZE, TSTACKSIZE - PAGESIZE);
}

// Initialize a thread's per-thread state,
// which must be done within the thread itself.
static void
ptinit(int tno)
{
	assert(tno > 0 && tno < MAXTHREADS);
	PERTHREAD->tno = tno;
	PERTHREAD->self = &th[tno];
}

static void
callmaster(void (*fun)(void *dat), void *dat)
{
	
}

////////// Threads //////////

static gcc_noreturn void
newthread(

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
	mutex->owner = SELF;
	return 0;
}

int
pthread_mutex_destroy(pthread_mutex_t *mutex)
{
}

int
pthread_mutex_lock(pthread_mutex_t *mutex)
{
	if (mutex->owner == SELF) {
		if (mutex->locked)
			panic("pthread_mutex_lock: mutex %x already locked",
				mutex);
		mutex->locked = 1;
		return 0;
	}
}

int
pthread_mutex_unlock(pthread_mutex_t *mutex)
{
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
		cond->evnext = t->conds;
		t->conds = cond;
	}
	cond->event |= ev;

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
	assert(t->waitnext == NULL);
	t->waitnext = *cond->waittail;
	*cond->waittail = t;
	cond->waittail = &t->waitnext;

	mblock(TH_COND);	// Block
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
	assert(PERTHREAD->self != NULL);
	return PERTHREAD->tspec[key];
}

int pthread_setspecific(pthread_key_t key, const void *val)
{
	assert(key > 0 && key < MAXKEYS);
	assert(PERTHREAD->self != NULL);
	PERTHREAD->tspec[key] = (void*) val;
	return 0;
}

////////// Memory allocation //////////

void *
malloc(size_t size)
{
	void *ptr = PERTHREAD->brk;
	void *nbrk = ptr + ROUNDUP(size, 8);
	if (nbrk > THEAPHI(PERTHREAD->tno))
		panic("malloc: out of memory");
	PERTHREAD->brk = nbrk;
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

