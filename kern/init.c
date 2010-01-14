/* See COPYRIGHT for copyright information. */

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/init.h>
#include <kern/console.h>
#include <kern/debug.h>
#include <kern/mem.h>
#include <kern/cpu.h>
#include <kern/trap.h>
#if LAB >= 2
#include <kern/mp.h>
#endif	// LAB >= 2

#if LAB >= 2
#include <dev/lapic.h>
#endif	// LAB >= 2



// Called from entry.S, only on the bootstrap processor.
// PIOS conventional: all '_init' functions get called
// only at bootstrap and only on the bootstrap processor.
void
init(void)
{
	extern char edata[], end[];

	// Before doing anything else, complete the ELF loading process.
	// Clear the uninitialized global data (BSS) section of our program.
	// This ensures that all static/global variables start out zero.
	memset(edata, 0, end - edata);

	// Initialize the console.
	// Can't call cprintf until after we do this!
	cons_init();

	// Lab 1: test cprintf and debug_trace
	cprintf("1234 decimal is %o octal!\n", 1234);
	debug_check();

	// Initialize and load the bootstrap CPU's GDT, TSS, and IDT.
	cpu_init(&bootcpu);
	assert(cpu_cur() == &bootcpu);	// cpu_cur() should now work
	trap_init();

	cpu_setup();		// load GDT, TSS
	trap_setup();		// load IDT
#if LAB >= 2
	lapic_setup();		// setup boot CPU's local APIC
#endif	// LAB >= 2

	// Physical memory detection/initialization.
	// Can't call mem_alloc until after we do this!
	mem_init();

#if LAB >= 2
	// Find and init other processors in a multiprocessor system
	mp_init();

	cpu_bootothers();	// Get other processors started

#endif	// LAB >= 2
	startup();		// Continue with per-CPU startup
}


// Called after bootstrap initialization on ALL processors,
// to initialize each CPU's private state and start it doing work.
// PIOS convention: all '_setup' functions get called on every processor.
void
startup(void)
{
	if (cpu_cur() != &bootcpu) {	// already done by init() on boot CPU
		cpu_setup();
		trap_setup();
#if LAB >= 2
		lapic_setup();
#endif	// LAB >= 2
	}

	cprintf("CPU %d (%s) has booted\n", cpu_cur()->id,
		cpu_cur() == &bootcpu ? "BP" : "AP");
	done();
}

void
done()
{
	asm volatile("hlt");	// Halt the processor
}

