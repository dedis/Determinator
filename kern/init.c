/* See COPYRIGHT for copyright information. */

///LAB2
#include <inc/asm.h>
#include <inc/trap.h>
#include <kern/pmap.h>
#include <kern/env.h>
#include <kern/console.h>
#include <kern/printf.h>
#include <kern/picirq.h>
#include <kern/kclock.h>
///LAB3
#include <kern/sched.h>
///END


void
i386_init (void)
{
  // can't call printf until after cninit ()
  cninit ();
  i386_detect_memory ();
  i386_vm_init ();
  ppage_init ();

#if 1
  {
    extern void ppage_check ();
    ppage_check ();
  }
#endif

///LAB3
  idt_init ();
  pic_init ();
  clock_init ();
///LAB6
  kbd_init ();
///END
  env_init ();
///LAB4
#if 0
///END
  {
    extern u_char spin_start;
    extern u_char spin_end;
    env_create (&spin_start, &spin_end - &spin_start);
  }
///LAB4
#endif
///END

#if 0
///LAB5
  {
    /* the binary for the user prog */
    extern u_char binary_user_simple_simple_start[];
    extern u_char binary_user_simple_simple_size[];

    /* the binary for the idle loop */
    extern u_char binary_user_kenv0_kenv0_start[];
    extern u_char binary_user_kenv0_kenv0_size[];

    env_create (binary_user_kenv0_kenv0_start,
		(u_int)binary_user_kenv0_kenv0_size);
    env_create (binary_user_simple_simple_start,
		(u_int)binary_user_simple_simple_size);
  }
///END
#endif

  yield ();
///END

  panic ("init.c: end of i386_init() reached!");
}


// like bcopy (but doesn't handle overlapping src/dst)
void
bcopy (const void *src, void *dst, size_t len)
{
  void *max = dst + len;
  // copy 4bytes at a time while possible
  while (dst + 3 < max)
    *((u_int *)dst)++ = *((u_int *)src)++;
  // finish remaining 0-3 bytes
  while (dst < max)
    *(char *)dst++ = *(char *)src++;
}

void
bzero (void *b, size_t len)
{
  size_t i;
  for (i = 0; i < len; i++)
    *(char *)b++ = 0;
}



// Ignore from here on down.  The functions below here are never
// called.  They are just here to get around a linking problem.

void abort (void)
{
  panic ("abort");
}

void *
malloc (size_t size)
{
  panic ("malloc: size %d", size);
}

void
free (void *ptr)
{
  panic ("free: ptr %p", ptr);
}

int
atexit (void (*function)(void))
{
  panic ("atexit: function %p", function);
}
///END
