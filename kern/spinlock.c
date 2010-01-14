// Mutual exclusion spin locks.
// Adapted from xv6.

#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/cpu.h>
#include <kern/spinlock.h>
#include <kern/console.h>


static int holding(struct spinlock *lock);

void
spinlock_init(struct spinlock *lk, char *name)
{
	lk->name = name;
	lk->locked = 0;
	lk->cpu = 0;
}

// Acquire the lock.
// Loops (spins) until the lock is acquired.
// Holding a lock for a long time may cause
// other CPUs to waste time spinning to acquire it.
void
spinlock_acquire(struct spinlock *lk)
{
	if(holding(lk))
		panic("spinlock_acquire");

	// The xchg is atomic.
	// It also serializes,
	// so that reads after acquire are not reordered before it. 
	while(xchg(&lk->locked, 1) != 0)
		;

	// Record info about lock acquisition for debugging.
	lk->cpu = cpu_cur();
	debug_trace(read_ebp(), lk->eips);
}

// Release the lock.
void
spinlock_release(struct spinlock *lk)
{
	if(!holding(lk))
		panic("spinlock_release");

	lk->eips[0] = 0;
	lk->cpu = 0;

	// The xchg serializes, so that reads before release are 
	// not reordered after it.  The 1996 PentiumPro manual (Volume 3,
	// 7.2) says reads can be carried out speculatively and in
	// any order, which implies we need to serialize here.
	// But the 2007 Intel 64 Architecture Memory Ordering White
	// Paper says that Intel 64 and IA-32 will not move a load
	// after a store. So lock->locked = 0 would work here.
	// The xchg being asm volatile ensures gcc emits it after
	// the above assignments (and after the critical section).
	xchg(&lk->locked, 0);
}

// Check whether this cpu is holding the lock.
static int
holding(struct spinlock *lock)
{
	return lock->locked && lock->cpu == cpu_cur();
}

