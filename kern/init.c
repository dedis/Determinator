/* See COPYRIGHT for copyright information. */

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/gcc.h>
#include <inc/syscall.h>

#include <kern/init.h>
#include <kern/console.h>
#include <kern/debug.h>
#include <kern/mem.h>
#include <kern/cpu.h>
#include <kern/trap.h>
#if LAB >= 2
#include <kern/spinlock.h>
#include <kern/mp.h>
#include <kern/proc.h>
#endif	// LAB >= 2

#if LAB >= 2
#include <dev/pic.h>
#include <dev/lapic.h>
#endif	// LAB >= 2


// User-mode stack for user(), below, to run on.
static char gcc_aligned(16) user_stack[PAGESIZE];


// Called first from entry.S on the bootstrap processor,
// and later from boot/bootother.S on all other processors.
// As a rule, "init" functions in PIOS are called once on EACH processor.
void
init(void)
{
	extern char start[], edata[], end[];

	// Before anything else, complete the ELF loading process.
	// Clear all uninitialized global data (BSS) in our program,
	// ensuring that all static/global variables start out zero.
	if (cpu_onboot())
		memset(edata, 0, end - edata);

	// Initialize the console.
	// Can't call cprintf until after we do this!
	cons_init();

#if LAB == 1
	// Lab 1: test cprintf and debug_trace
	cprintf("1234 decimal is %o octal!\n", 1234);
	debug_check();

#endif	// LAB == 1
	// Initialize and load the bootstrap CPU's GDT, TSS, and IDT.
	cpu_init();
	trap_init();

	// Physical memory detection/initialization.
	// Can't call mem_alloc until after we do this!
	mem_init();

#if LAB >= 2
	// Lab 2: check spinlock implementation
	if (cpu_onboot())
		spinlock_check();

#if LAB >= 3
	// Initialize the paged virtual memory system.
	pmap_init();

#endif
	// Find and start other processors in a multiprocessor system
	mp_init();		// Find info about processors in system
	pic_init();		// setup the legacy PIC (mainly to disable it)
	lapic_init();		// setup this CPU's local APIC
	cpu_bootothers();	// Get other processors started
	cprintf("CPU %d (%s) has booted\n", cpu_cur()->id,
		cpu_onboot() ? "BP" : "AP");

	// Initialize the process management code.
	proc_init();
#endif

#if SOL >= 2
	if (!cpu_onboot())
		proc_sched();	// just jump right into the scheduler

	// Create our first actual user-mode process
#if SOL == 2
	// (though it's still be sharing the kernel's address space for now).
#endif
	proc *root = proc_alloc(NULL, 0);
	root->tf.tf_eip = (uint32_t) user;
	root->tf.tf_esp = (uint32_t) &user_stack[PAGESIZE];

#if SOL >= 3
	// Copy the kernel into the first process's address space.
	uint32_t va;
	for (va = ROUNDDOWN((uint32_t)start, PAGESIZE);
			va < ROUNDUP((uint32_t)end, PAGESIZE);
			va += PAGESIZE) {
		pageinfo *pi = mem_alloc();
		assert(pi != NULL);
		memmove(mem_pi2ptr(pi), (void*)va, PAGESIZE);
		pte_t *pte = pmap_insert(root->pdir, pi, va,
				PTE_P | PTE_W | PTE_U);
		assert(pte != NULL);
	}

#endif	// SOL >= 3
	proc_ready(root);	// make it ready
	proc_sched();		// run it
#elif SOL >= 1
	// Conjure up a trapframe and "return" to it to enter user mode.
	static trapframe utf = {
		tf_ds: CPU_GDT_UDATA | 3,
		tf_es: CPU_GDT_UDATA | 3,
		tf_eip: (uint32_t) user,
		tf_cs: CPU_GDT_UCODE | 3,
		tf_eflags: FL_IOPL_3,	// let user() output to console
		tf_esp: (uint32_t) &user_stack[PAGESIZE],
		tf_ss: CPU_GDT_UDATA | 3,
	};
	trap_return(&utf);
#else
	// Lab 1: change this so it enters user() in user mode,
	// running on the user_stack declared above,
	// instead of just calling user() directly.
	user();
#endif
}

// This is the first function that gets run in user mode (ring 3).
// It acts as PIOS's "root process",
// of which all other processes are descendants.
void
user()
{
	cprintf("in user()\n");
	assert(read_esp() > (uint32_t) &user_stack[0]);
	assert(read_esp() < (uint32_t) &user_stack[sizeof(user_stack)]);

#if LAB == 1
	// Check that we're in user mode and can handle traps from there.
	trap_check_user();
#endif
#if LAB == 2
	// Check the system call and process scheduling code.
	proc_check();
#endif

	done();
}

void
done()
{
	while (1)
		;	// just spin
}

