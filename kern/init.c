#if LAB >= 2
/* See COPYRIGHT for copyright information. */

#include <inc/asm.h>
#if LAB >= 3
#include <kern/trap.h>
#endif
#include <kern/pmap.h>
#include <kern/env.h>
#include <kern/console.h>
#include <kern/printf.h>
#include <kern/picirq.h>
#include <kern/kclock.h>
#if LAB >= 3
#include <kern/sched.h>
#endif

void
i386_init(void)
{
	// can't call printf until after cons_init()
	cons_init();

	printf("6828 decimal is %o octal!\n", 6828);

	// Lab 2 initialization functions
	i386_detect_memory();
	i386_vm_init();
	page_init();
	page_check();

#if LAB >= 3
	// Lab 3 initialization functions
	idt_init();
	pic_init();
	kclock_init();
	env_init();

#if LAB >= 4
#else
	// Temporary test code specific to LAB 3
#if defined(TEST_START)
	{
		// Don't touch this!  Used by the grading script.
		extern u_char TEST_START, TEST_END;
		env_create(&TEST_START, &TEST_END - &TEST_START);
	}
#elif defined(TEST_ALICEBOB)
	{
		// Don't touch this!  Used by the grading script.
		extern u_char alice_start, alice_end, bob_start, bob_end;
		env_create(&alice_start, &alice_end - &alice_start);
		env_create(&bob_start, &bob_end - &bob_start);
	}
#else
	{
		// Do whatever you want here for your own testing purposes.
		extern u_char spin_start;
		extern u_char spin_end;
		env_create(&spin_start, &spin_end - &spin_start);
	}
#endif

#if SOL >= 4
	{
		/* the binary for the user prog */
		extern u_char binary_user_simple_simple_start[];
		extern u_char binary_user_simple_simple_size[];

		/* the binary for the idle loop */
		extern u_char binary_user_kenv0_kenv0_start[];
		extern u_char binary_user_kenv0_kenv0_size[];

		env_create(binary_user_kenv0_kenv0_start,
								(u_int)binary_user_kenv0_kenv0_size);
		env_create(binary_user_simple_simple_start,
								(u_int)binary_user_simple_simple_size);
	}
#endif /* SOL >= 4 */

	sched_yield();
#endif /* LAB >= 3 */

	panic("init.c: end of i386_init() reached!");
}


// like bcopy(but doesn't handle overlapping src/dst)
void
bcopy(const void *src, void *dst, size_t len)
{
	void *max;

	max = dst + len;
	// copy machine words while possible
	while (dst + 3 < max)
		*(int *)dst++ = *(int *)src++;
	// finish remaining 0-3 bytes
	while (dst < max)
		*(char *)dst++ = *(char *)src++;
}

void
bzero(void *b, size_t len)
{
	void *max;

	max = b + len;
	// zero machine words while possible
	while (b + 3 < max)
		*(int *)b++ = 0;
	// finish remaining 0-3 bytes
	while (b < max)
		*(char *)b++ = 0;
}

// Ignore from here on down.  The functions below here are never
// called.  They are just here to get around a linking problem.

void
abort(void)
{
	panic("abort");
}

void *
malloc(size_t size)
{
	panic("malloc: size %d", size);
}

void
free(void *ptr)
{
	panic("free: ptr %p", ptr);
}

int
atexit(void(*function)(void))
{
	panic("atexit: function %p", function);
}

#endif /* LAB >= 2 */
