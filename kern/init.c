///LAB2
/*
 * Copyright (C) 1997 Massachusetts Institute of Technology 
 *
 * This software is being provided by the copyright holders under the
 * following license. By obtaining, using and/or copying this software,
 * you agree that you have read, understood, and will comply with the
 * following terms and conditions:
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose and without fee or royalty is
 * hereby granted, provided that the full text of this NOTICE appears on
 * ALL copies of the software and documentation or portions thereof,
 * including modifications, that you make.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS," AND COPYRIGHT HOLDERS MAKE NO
 * REPRESENTATIONS OR WARRANTIES, EXPRESS OR IMPLIED. BY WAY OF EXAMPLE,
 * BUT NOT LIMITATION, COPYRIGHT HOLDERS MAKE NO REPRESENTATIONS OR
 * WARRANTIES OF MERCHANTABILITY OR FITNESS FOR ANY PARTICULAR PURPOSE OR
 * THAT THE USE OF THE SOFTWARE OR DOCUMENTATION WILL NOT INFRINGE ANY
 * THIRD PARTY PATENTS, COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS. COPYRIGHT
 * HOLDERS WILL BEAR NO LIABILITY FOR ANY USE OF THIS SOFTWARE OR
 * DOCUMENTATION.
 *
 * The name and trademarks of copyright holders may NOT be used in
 * advertising or publicity pertaining to the software without specific,
 * written prior permission. Title to copyright in this software and any
 * associated documentation will at all times remain with copyright
 * holders. See the file AUTHORS which should have accompanied this software
 * for a list of all copyright holders.
 *
 * This file may be derived from previously copyrighted software. This
 * copyright applies only to those changes made by the copyright
 * holders listed in the AUTHORS file. The rest of this file is covered by
 * the copyright notices, if any, listed below.
 */

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
