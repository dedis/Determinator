///BEGIN 2
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

#include <kern/inc/pmap.h>
#include <kern/inc/env.h>
#include <kern/inc/trap.h>
#include <kern/inc/scheduler.h>
#include <kern/inc/console.h>
#include <kern/inc/printf.h>
#include <kern/inc/picirq.h>
#include <kern/inc/kclock.h>
#include <kern/inc/asm.h>
///END


///BEGIN 2
void
i386_init (void)
{
  // can't call printf until after cninit ()
  cninit ();
  i386_detect_memory ();
  i386_vm_init ();
  ppage_init ();

///BEGIN 2
#if 1
  {
    extern void ppage_check ();
    ppage_check ();
  }
#endif
///END

///END
///BEGIN 3
  idt_init ();
  pic_init ();
  clock_init ();
///BEGIN 6
  kbd_init ();
///END
  env_init ();
///BEGIN 4
#if 0
///END
  {
    extern u_char spin_start;
    extern u_char spin_end;
    env_create (&spin_start, &spin_end - &spin_start);
  }
///BEGIN 4
#endif
///END

///BEGIN 5
  {
    extern u_char simple_bin[];	/* the binary for the user prog */
    extern const u_int simple_bin_size;
    extern u_char kenv0_bin[];	/* the binary for the idle loop */
    extern const u_int kenv0_bin_size;
    env_create (kenv0_bin, kenv0_bin_size);
    env_create (simple_bin, simple_bin_size);
  }
///END

  yield ();
///END  
///BEGIN 2
  panic ("init.c: i386_init() yield returned");
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


