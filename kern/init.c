/* See COPYRIGHT for copyright information. */

#include <inc/asm.h>
#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/monitor.h>
#include <kern/console.h>
#include <kern/bochs.h>
#if LAB >= 2
#include <kern/pmap.h>
#include <kern/kclock.h>
#if LAB >= 3
#include <kern/env.h>
#include <kern/trap.h>
#include <kern/sched.h>
#include <kern/picirq.h>
#endif
#endif

#if LAB >= 2	// ...then leave this code out.
#elif LAB >= 1
void
test_backtrace(int x)
{
	printf("entering test_backtrace %d\n", x);
	if (x > 0)
		test_backtrace(x-1);
	else
		monitor(NULL);
	printf("leaving test_backtrace %d\n", x);
}
#endif

void
i386_init(void)
{
	extern char edata[], end[];

	// Before doing anything else,
	// clear the uninitialized global data (BSS) section of our program.
	// This ensures that all static/global variables start out zero.
	memset(edata, 0, end-edata);

	// Initialize the console.
	// Can't call printf until after we do this!
	cons_init();

	printf("6828 decimal is %o octal!\n", 6828);

#if LAB >= 2
	// Lab 2 initialization functions
	i386_detect_memory();
	i386_vm_init();
	page_init();
	page_check();
#endif

#if LAB >= 3
	// Lab 3 initialization functions
	idt_init();
	pic_init();
	kclock_init();
	env_init();
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
#elif defined(TEST_PINGPONG2)
	// Don't touch -- used by grading script!
	ENV_CREATE(user_pingpong2);
	ENV_CREATE(user_pingpong2);
#else
	// Touch all you want.
	ENV_CREATE(user_buggyhello);
#endif // TEST*
#elif LAB >= 3
	// Temporary test code specific to LAB 3
#if defined(TEST)
	// Don't touch -- used by grading script!
	ENV_CREATE(TEST)
#elif defined(TEST_ALICEBOB)
	// Don't touch -- used by grading script!
	ENV_CREATE(alice);
	ENV_CREATE(bob);
#else
	// Touch all you want.
	ENV_CREATE(spin);
#endif // TEST*
#endif // LAB5, LAB4, LAB3

#if LAB >= 6
	// Should not be necessary - drain keyboard because interrupt has given up.
	kbd_intr();

#endif
#if LAB >= 3
	sched_yield();
#endif

#if LAB >= 2
#else
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
 * It prints "panic: mesg", and then enters an infinite loop.
 * If executing on Bochs, drop into the debugger rather than chew CPU.
 */
void
_panic(const char *file, int line, const char *fmt,...)
{
	va_list ap;

	if (panicstr)
		goto dead;
	panicstr = fmt;

	va_start(ap, fmt);
	printf("kernel panic at %s:%d: ", file, line);
	vprintf(fmt, ap);
	printf("\n");
	va_end(ap);

dead:
	/* break into Bochs debugger */
	bochs();

	for(;;);
}

/* like panic, but don't */
void
_warn(const char *file, int line, const char *fmt,...)
{
	va_list ap;

	va_start(ap, fmt);
	printf("kernel warning at %s:%d: ", file, line);
	vprintf(fmt, ap);
	printf("\n");
	va_end(ap);
}

