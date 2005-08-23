/* See COPYRIGHT for copyright information. */

#include <inc/asm.h>
#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/monitor.h>
#include <kern/console.h>
#if LAB >= 2
#include <kern/pmap.h>
#include <kern/kclock.h>
#if LAB >= 3
#include <kern/env.h>
#include <kern/trap.h>
#if LAB >= 4
#include <kern/sched.h>
#include <kern/picirq.h>
#endif	// LAB >= 4
#endif	// LAB >= 3
#endif	// LAB >= 2

#if LAB >= 2	// ...then leave this code out.
#elif LAB >= 1
// Test the stack backtrace function (lab 1 only)
void
test_backtrace(int x)
{
	cprintf("entering test_backtrace %d\n", x);
	if (x > 0)
		test_backtrace(x-1);
	else
		mon_backtrace(0, 0, 0);
	cprintf("leaving test_backtrace %d\n", x);
}
#endif

void
i386_init(void)
{
	extern char edata[], end[];

	// Before doing anything else,
	// clear the uninitialized global data (BSS) section of our program.
	// This ensures that all static/global variables start out zero.
	memset(edata, 0, end - edata);

	// Initialize the console.
	// Can't call cprintf until after we do this!
	cons_init();

#ifndef ENV_CLASS_NYU
	cprintf("6828 decimal is %o octal!\n", 6828);
#else /* !ENV_CLASS_NYU */
	cprintf("480 decimal is %o octal!\n", 480);
#endif /* !ENV_CLASS_NYU */

#if LAB >= 2
	// Lab 2 memory management initialization functions
	i386_detect_memory();
	i386_vm_init();
	page_init();
	page_check();
#endif

#if LAB >= 3
	// Lab 3 user environment initialization functions
	env_init();
	idt_init();
#endif

#if LAB >= 4
	// Lab 4 multitasking initialization functions
	pic_init();
	kclock_init();
#endif

#if LAB >= 6
	// Should always have an idle process as first one.
	ENV_CREATE(user_idle);

	// Start fs.
	ENV_CREATE(fs_fs);

	// Start init
#if defined(TEST)
	// Don't touch -- used by grading script!
	ENV_CREATE2(TEST, TESTSIZE);
#else
	// Touch all you want.
	ENV_CREATE(user_icode);
	// ENV_CREATE(user_pipereadeof);
	// ENV_CREATE(user_pipewriteeof);
#endif
#elif LAB >= 5
	// Should always have an idle process as first one.
	ENV_CREATE(user_idle);

	// Start fs.
	ENV_CREATE(fs_fs);

	// Start init
#if defined(TEST)
	// Don't touch -- used by grading script!
	ENV_CREATE2(TEST, TESTSIZE);
#else
	// Touch all you want.
	// ENV_CREATE(user_writemotd);
	// ENV_CREATE(user_testfsipc);
	// ENV_CREATE(user_icode);
#endif
#elif LAB >= 4
	// Should always have an idle process as first one.
	ENV_CREATE(user_idle);

#if defined(TEST)
	// Don't touch -- used by grading script!
	ENV_CREATE2(TEST, TESTSIZE)
#else
	// Touch all you want.
	ENV_CREATE(user_primes);
#endif // TEST*
#elif LAB >= 3
	// Temporary test code specific to LAB 3
#if defined(TEST)
	// Don't touch -- used by grading script!
	ENV_CREATE2(TEST, TESTSIZE);
#else
	// Touch all you want.
	ENV_CREATE(user_hello);
#endif // TEST*
#endif // LAB5, LAB4, LAB3

#if LAB >= 6
	// Should not be necessary - drain keyboard because interrupt has given up.
	kbd_intr();

#endif

#if LAB >= 4
	// Schedule and run the first user environment!
	sched_yield();
#else
#if LAB >= 3
	// We only have one user environment for now, so just run it.
	env_run(&envs[0]);
#endif
#endif

#if LAB >= 2
#else
	// Test the stack backtrace function (lab 1 only)
	test_backtrace(5);
#endif

	// Drop into the kernel monitor.
	while (1)
		monitor(NULL);
}


/*
 * Variable panicstr contains argument to first call to panic; used as flag
 * to indicate that the kernel has already called panic.
 */
static const char *panicstr;

/*
 * Panic is called on unresolvable fatal errors.
 * It prints "panic: mesg", and then enters the kernel monitor.
 */
void
_panic(const char *file, int line, const char *fmt,...)
{
	va_list ap;

	if (panicstr)
		goto dead;
	panicstr = fmt;

	va_start(ap, fmt);
	cprintf("kernel panic at %s:%d: ", file, line);
	vcprintf(fmt, ap);
	cprintf("\n");
	va_end(ap);

dead:
	/* break into the kernel monitor */
	while (1)
		monitor(NULL);
}

/* like panic, but don't */
void
_warn(const char *file, int line, const char *fmt,...)
{
	va_list ap;

	va_start(ap, fmt);
	cprintf("kernel warning at %s:%d: ", file, line);
	vcprintf(fmt, ap);
	cprintf("\n");
	va_end(ap);
}

