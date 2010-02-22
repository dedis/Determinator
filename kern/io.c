#if LAB >= 4
// Process management.
// See COPYRIGHT for copyright information.

#include <inc/vm.h>
#include <inc/gcc.h>
#include <inc/mmu.h>

#include <kern/cpu.h>
#include <kern/trap.h>
#include <kern/proc.h>
#include <kern/pmap.h>
#include <kern/spinlock.h>
#include <kern/io.h>
#include <kern/console.h>

#include <dev/ide.h>


static spinlock io_lock;
static proc *io_sleeper;		// currently waiting root process
static ioevent *io_event;		// where to put the next I/O event


void
io_init(void)
{
	spinlock_init(&io_lock);
}

// Called from proc_ret() when the root process "returns" -
// this function performs any output the root process requested,
// or if it didn't request output, puts the root process to sleep
// waiting for input to arrive from some I/O device.
void
io_sleep(trapframe *tf)
{
	proc *cp = proc_cur();
	assert(cp->parent == NULL);	// only root process should do this!

	// Find the root process's I/O page.
	pte_t *iopte = pmap_walk(cp->pdir, VM_IOLO, 1); assert(iopte);
	assert(PGADDR(*iopte) != PTE_ZERO);
	ioevent *ioev = mem_ptr(PGADDR(*iopte));

	// If output was requested, do it without putting the process to sleep
	switch (ioev->type) {
	case IO_NONE:
		break;			// no output - wait for input
	case IO_CONS:
		cons_startio(&ioev->cons);
		trap_return(tf);
	case IO_DISK:
		ide_startio(&ioev->disk);
		trap_return(tf);
	default:
		panic("root process requested unknown I/O %d", ioev->type);
	}

	// Now go to sleep waiting for input.
	spinlock_acquire(&io_lock);
	assert(io_sleeper == NULL);	// should only be one root process!
	cp->state = PROC_STOP;		// we're becoming stopped
	cp->runcpu = NULL;		// no longer running
	cp->tf = *tf;			// save our register state
	io_sleeper = cp;		// register the sleeping root process
	io_event = ioev;		// save the ioevent pointer too
	spinlock_release(&io_lock);

	io_check();			// check for I/O ready immediately
	proc_sched();			// go do something else
}

// Check to see if any input is available for the root process
// and if the root process is waiting for it, and if so, wake the process.
void
io_check(void)
{
	spinlock_acquire(&io_lock);
	if (io_sleeper == NULL)
		goto done;
	assert(io_event != NULL);

	if (!cons_checkio(&io_event->cons) && !ide_checkio(&io_event->disk))
		goto done;	// no drivers have anything to report

	// io_event filled in - start the root process again.
	proc_ready(io_sleeper);
	io_sleeper = NULL;
	io_event = NULL;

	done:
	spinlock_release(&io_lock);
}

#endif // LAB >= 4
