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


/*-
 * Copyright (c) 1986, 1988, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)subr_prf.c	8.3 (Berkeley) 1/21/94
 */

#include <inc/stdarg.h>
#include <inc/types.h>
#include <kern/picirq.h>
#include <kern/console.h>
#include <kern/printf.h>

/*
 * Put a number (base <= 16) in a buffer in reverse order; return an
 * optional length and a pointer to the NULL terminated (preceded?)
 * buffer.
 */
static char *
ksprintn (u_quad_t uq, int base, int *lenp)
{				/* A quad in binary, plus NULL. */
  static char buf[sizeof (u_quad_t) * 8 + 1];
  register char *p;

  p = buf;
  do {
    *++p = "0123456789abcdef"[uq % base];
  } while (uq /= base);
  if (lenp)
    *lenp = p - buf;
  return (p);
}

/*
 * Scaled down version of printf(3).
 *
 * Two additional formats:
 *
 * The format %b is supported to decode error registers.
 * Its usage is:
 *
 *      printf("reg=%b\n", regval, "<base><arg>*");
 *
 * where <base> is the output base expressed as a control character, e.g.
 * \10 gives octal; \20 gives hex.  Each arg is a sequence of characters,
 * the first of which gives the bit number to be inspected (origin 1), and
 * the next characters (up to a control character, i.e. a character <= 32),
 * give the name of the register.  Thus:
 *
 *      kprintf("reg=%b\n", 3, "\10\2BITTWO\1BITONE\n");
 *
 * would produce output:
 *
 *      reg=3<BITTWO,BITONE>
 *
 * The format %r passes an additional format string and argument list
 * recursively.  Its usage is:
 *
 * fn(char *fmt, ...)
 * {
 *      va_list ap;
 *      va_start(ap, fmt);
 *      printf("prefix: %r: suffix\n", fmt, ap);
 *      va_end(ap);
 * }
 *
 * Space or zero padding and a field width are supported for the numeric
 * formats only.
 */

void
kprintf (const char *fmt, va_list ap)
{
  register char *p, *q;
  register int ch, n;
  u_quad_t uq;
  int base, lflag, qflag, tmp, width;
  char padc;

  for (;;) {
    padc = ' ';
    width = 0;
    while ((ch = *(u_char *) fmt++) != '%') {
      if (ch == '\0')
	return;
      cnputc (ch);
    }
    lflag = 0;
    qflag = 0;
  reswitch:
    switch (ch = *(u_char *) fmt++) {
    case '0':
      padc = '0';
      goto reswitch;
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      for (width = 0;; ++fmt) {
	width = width * 10 + ch - '0';
	ch = *fmt;
	if (ch < '0' || ch > '9')
	  break;
      }
      goto reswitch;
    case 'l':
      lflag = 1;
      qflag = 0; /* this should be done better */
      goto reswitch;
    case 'q':
      qflag = 1;
      lflag = 0; /* this should be done better */
      goto reswitch;
    case 'b':
      uq = va_arg (ap, int);
      p = va_arg (ap, char *);
      for (q = ksprintn (uq, *p++, NULL); ch = *q--;)
	cnputc (ch);

      if (!uq)
	break;

      for (tmp = 0; n = *p++;) {
	if (uq & (1 << (n - 1))) {
	  cnputc (tmp ? ',' : '<');
	  for (; (n = *p) > ' '; ++p)
	    cnputc (n);
	  tmp = 1;
	}
	else
	  for (; *p > ' '; ++p)
	    continue;
      }
      if (tmp)
	cnputc ('>');
      break;
    case 'c':
      cnputc (va_arg (ap, int));
      break;
    case 'r':
      p = va_arg (ap, char *);
      kprintf (p, va_arg (ap, va_list));
      break;
    case 's':
      if ((p = va_arg (ap, char *)) == NULL)
	  p = "(null)";
      while (ch = *p++)
	cnputc (ch);
      break;
    case 'd':
      uq = lflag ? va_arg (ap, long)
	: qflag ? va_arg (ap, long long)
	: va_arg (ap, int);
      if ((quad_t) uq < 0) {
	cnputc ('-');
	uq = -(quad_t) uq;
      }
      base = 10;
      goto number;
    case 'u':
      uq = lflag ? va_arg (ap, u_long)
	: qflag ? va_arg (ap, long long)
	: va_arg (ap, u_int);
      base = 10;
      goto number;
    case 'o':
///SOL2
      uq = lflag ? va_arg (ap, u_long)
	: qflag ? va_arg (ap, long long)
	: va_arg (ap, u_int);
      base = 8;
      goto number;
///ELSE
      // Replace this with your code to print in octal.
      cnputc ('X');
      cnputc ('X');
      cnputc ('X');
      break;
///END
    case 'p':
      cnputc ('0');
      cnputc ('x');
      uq = (u_long) va_arg (ap, void *);
      base = 16;
      goto number;
    case 'x':
      uq = lflag ? va_arg (ap, u_long)
	: qflag ? va_arg (ap, long long)
	: va_arg (ap, u_int);
      base = 16;
    number:p = ksprintn (uq, base, &tmp);
      if (width && (width -= tmp) > 0)
	while (width--)
	  cnputc (padc);
      while (ch = *p--)
	cnputc (ch);
      break;
    default:
      cnputc ('%');
      if (lflag)
	cnputc ('l');
      /* FALLTHROUGH */
    case '%':
      cnputc (ch);
    }
  }
}


/*
 * Variable panicstr contains argument to first call to panic; used as flag
 * to indicate that the kernel has already called panic.
 */
static const char *panicstr;

/*
 * Panic is called on unresolvable fatal errors.  It prints "panic: mesg",
 * and then enters an infinite loop.
 */
void
panic (const char *fmt,...)
{
  va_list ap;

  if (panicstr)
    goto dead;
  panicstr = fmt;

  va_start (ap, fmt);
  printf ("panic: ");
  kprintf (fmt, ap);
  printf ("\n");
  va_end (ap);

dead:
  while (1)
    ;
}

/* like panic, but don't */
void
warn (const char *fmt,...)
{
  va_list ap;
  va_start (ap, fmt);
  kprintf (fmt, ap);
  printf ("\n");
  va_end (ap);
}

int
printf (const char *fmt,...)
{
  va_list ap;
  va_start (ap, fmt);
  kprintf (fmt, ap);
  va_end (ap);
  return 0;
}
///END

