/* See COPYRIGHT for copyright information. */

#include <inc/asm.h>
#include <kern/trap.h>
#include <kern/pmap.h>
#include <kern/env.h>
#include <kern/console.h>
#include <kern/printf.h>
#include <kern/picirq.h>
#include <kern/kclock.h>
#include <kern/sched.h>

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

	// Lab 3 initialization functions
	idt_init();
	pic_init();
	kclock_init();
	env_init();

	// Temporary test code specific to LAB 3
	{
		extern u_char spin_start;
		extern u_char spin_end;
		env_create(&spin_start, &spin_end - &spin_start);
	}


	sched_yield();

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

