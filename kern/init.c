/* See COPYRIGHT for copyright information. */

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/gcc.h>
#include <inc/syscall.h>
#if LAB >= 3
#include <inc/elf.h>
#include <inc/vm.h>
#endif

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
#endif
#if LAB >= 4
#include <kern/file.h>
#endif

#if LAB >= 2
#include <dev/pic.h>
#include <dev/lapic.h>
#include <dev/ioapic.h>
#endif	// LAB >= 2


// User-mode stack for user(), below, to run on.
static char gcc_aligned(16) user_stack[PAGESIZE];

// Lab 3: ELF executable containing root process, linked into the kernel
#ifndef ROOTEXE_START
#if LAB == 3
#define ROOTEXE_START _binary_obj_user_testvm_start
#elif LAB >= 4
#define ROOTEXE_START _binary_obj_user_testfs_start
#endif // LAB >= 4
#endif
extern char ROOTEXE_START[];


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
	ioapic_init();		// prepare to handle external device interrupts
	lapic_init();		// setup this CPU's local APIC
	cpu_bootothers();	// Get other processors started
	cprintf("CPU %d (%s) has booted\n", cpu_cur()->id,
		cpu_onboot() ? "BP" : "AP");

#if LAB >= 4
	// Initialize the I/O system.
	file_init();		// Create root directory and console I/O files

#if SOL >= 4
	cons_intenable();	// Let the console start producing interrupts
#else
	// Lab 4: uncomment this when you can handle IRQ_SERIAL and IRQ_KBD.
	//cons_intenable();	// Let the console start producing interrupts
#endif

#endif
	// Initialize the process management code.
	proc_init();
#endif

#if SOL == 1
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
#elif SOL == 2
	if (!cpu_onboot())
		proc_sched();	// just jump right into the scheduler

	// Create our first actual user-mode process
	// (though it's still be sharing the kernel's address space for now).
	proc *root = proc_alloc(NULL, 0);
	root->tf.tf_eip = (uint32_t) user;
	root->tf.tf_esp = (uint32_t) &user_stack[PAGESIZE];

	proc_ready(root);	// make it ready
	proc_sched();		// run it
#elif SOL >= 3
	if (!cpu_onboot())
		proc_sched();	// just jump right into the scheduler

	// Create our first actual user-mode process
	proc *root = proc_root = proc_alloc(NULL, 0);

	elfhdr *eh = (elfhdr *)ROOTEXE_START;
	assert(eh->e_magic == ELF_MAGIC);

	// Load each program segment
	proghdr *ph = (proghdr *) ((void *) eh + eh->e_phoff);
	proghdr *eph = ph + eh->e_phnum;
	for (; ph < eph; ph++) {
		if (ph->p_type != ELF_PROG_LOAD)
			continue;
	
		void *fa = (void *) eh + ROUNDDOWN(ph->p_offset, PAGESIZE);
		uint32_t va = ROUNDDOWN(ph->p_va, PAGESIZE);
		uint32_t zva = ph->p_va + ph->p_filesz;
		uint32_t eva = ROUNDUP(ph->p_va + ph->p_memsz, PAGESIZE);

		uint32_t perm = SYS_READ | PTE_P | PTE_U;
		if (ph->p_flags & ELF_PROG_FLAG_WRITE)
			perm |= SYS_WRITE | PTE_W;

		for(; va < eva; va += PAGESIZE, fa += PAGESIZE) {
			pageinfo *pi = mem_alloc(); assert(pi != NULL);
			if (va < ROUNDDOWN(zva, PAGESIZE)) // complete page
				memmove(mem_pi2ptr(pi), fa, PAGESIZE);
			else if (va < zva && ph->p_filesz) {	// partial
				memset(mem_pi2ptr(pi), 0, PAGESIZE);
				memmove(mem_pi2ptr(pi), fa, zva-va);
			} else			// all-zero page
				memset(mem_pi2ptr(pi), 0, PAGESIZE);
			pte_t *pte = pmap_insert(root->pdir, pi, va, perm);
			assert(pte != NULL);
		}
	}

	// Start the process at the entry indicated in the ELF header
	root->tf.tf_eip = eh->e_entry;
	root->tf.tf_eflags |= FL_IF;	// enable interrupts

	// Give the process a 1-page stack in high memory
	// (the process can then increase its own stack as desired)
	pageinfo *pi = mem_alloc(); assert(pi != NULL);
	pte_t *pte = pmap_insert(root->pdir, pi, VM_STACKHI-PAGESIZE,
				SYS_READ | SYS_WRITE | PTE_P | PTE_U | PTE_W);
	assert(pte != NULL);
	root->tf.tf_esp = VM_STACKHI;

	// Give the root process an initial file system.
	file_initroot(root);

	proc_ready(root);	// make the root process ready
	proc_sched();		// run it
#else // SOL == 0
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

void gcc_noreturn
done()
{
	while (1)
		;	// just spin
}

