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


/*-
 * Copyright (c) 1993, 1994, 1995 Charles Hannum.  All rights reserved.
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz and Don Ahn.
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
 */

/* This code isn't actually from NetBSD, but I consulted the NetBSD
 * pccons driver heavily and copied some chunks like the scan codes,
 * hence the above copyright probably applies.
 */

#include <kern/inc/x86.h>
#include <kern/inc/pmap.h>
#include <kern/inc/console.h>
#include <kern/inc/mmu.h>
#include <kern/inc/printf.h>
#include <kern/inc/picirq.h>
#include <kern/inc/kbdreg.h>

static unsigned addr_6845;
static unsigned short *crt_buf;
static short crt_pos;

void
cninit (void)
{
  unsigned short volatile *cp;
  unsigned short was;
  unsigned cursorat;

  cp = (short *) (KERNBASE + CGA_BUF);
  was = *cp;
  *cp = (unsigned short) 0xA55A;
  if (*cp != 0xA55A) {
    cp = (short *) (KERNBASE + MONO_BUF);
    addr_6845 = MONO_BASE;
  } else {
    *cp = was;
    addr_6845 = CGA_BASE;
  }
  
  /* Extract cursor location */
  outb(addr_6845, 14);
  cursorat = inb(addr_6845+1) << 8;
  outb(addr_6845, 15);
  cursorat |= inb(addr_6845+1);

  crt_buf = (unsigned short *)cp;
  crt_pos = cursorat;
}


void
cnputc(short int c)
{
  /* if no attribute given, then use black on white */
  if (!(c & ~0xff)) c |= 0x0700;

  switch (c & 0xff) {
  case '\b':
    if (crt_pos > 0)
      crt_pos--;
    break;
  case '\n':
    crt_pos += CRT_COLS;
    /* cascade  */
  case '\r':
    crt_pos -= (crt_pos % CRT_COLS);
    break;
  case '\t':
    cnputc (' ');
    cnputc (' ');
    cnputc (' ');
    cnputc (' ');
    cnputc (' ');
    break;
  default:
    crt_buf[crt_pos++] = c;   /* write the character */
    break;
  }

///BEGIN 3
  /* scroll if necessary */
///END
  // What is the purpose of this?
  if (crt_pos >= CRT_SIZE) {
    int i;
    bcopy (crt_buf + CRT_COLS, crt_buf, CRT_SIZE << 1);
    for (i = CRT_SIZE - CRT_COLS; i < CRT_SIZE; i++)
      crt_buf[i] = 0x0700 | ' ';
    crt_pos -= CRT_COLS;
  }

  /* move that little blinky thing */
  outb(addr_6845, 14);
  outb(addr_6845+1, crt_pos >> 8);
  outb(addr_6845, 15);
  outb(addr_6845+1, crt_pos);
}
///END





///BEGIN 2





/*
 *  Scan codes from NetBSD.
 */
#define	SCROLL		0x0001	/* stop output */
#define	NUM		0x0002	/* numeric shift  cursors vs. numeric */
#define	CAPS		0x0004	/* caps shift -- swaps case of letter */
#define	SHIFT		0x0008	/* keyboard shift */
#define	CTL		0x0010	/* control shift  -- allows ctl function */
#define	ASCII		0x0020	/* ascii code for this key */
#define	ALT		0x0080	/* alternate shift -- alternate chars */
#define	FUNC		0x0100	/* function key */
#define	KP		0x0200	/* Keypad keys */
#define	NONE		0x0400	/* no function */

#define	CODE_SIZE	4		/* Use a max of 4 for now... */
typedef struct {
  u_short type;
  char unshift[CODE_SIZE];
  char shift[CODE_SIZE];
  char ctl[CODE_SIZE];
} Scan_def;

static Scan_def scan_codes[] = {
  {NONE,  "",       "",       ""},       /* 0 unused */
  {ASCII, "\033",   "\033",   "\033"},   /* 1 ESCape */
  {ASCII, "1",      "!",      "!"},      /* 2 1 */
  {ASCII, "2",      "@",      "\000"},   /* 3 2 */
  {ASCII, "3",      "#",      "#"},      /* 4 3 */
  {ASCII, "4",      "$",      "$"},      /* 5 4 */
  {ASCII, "5",      "%",      "%"},      /* 6 5 */
  {ASCII, "6",      "^",      "\036"},   /* 7 6 */
  {ASCII, "7",      "&",      "&"},      /* 8 7 */
  {ASCII, "8",      "*",      "\010"},   /* 9 8 */
  {ASCII, "9",      "(",      "("},      /* 10 9 */
  {ASCII, "0",      ")",      ")"},      /* 11 0 */
  {ASCII, "-",      "_",      "\037"},   /* 12 - */
  {ASCII, "=",      "+",      "+"},      /* 13 = */
  {ASCII, "\177",   "\177",   "\010"},   /* 14 backspace */
  {ASCII, "\t",     "\177\t", "\t"},     /* 15 tab */
  {ASCII, "q",      "Q",      "\021"},   /* 16 q */
  {ASCII, "w",      "W",      "\027"},   /* 17 w */
  {ASCII, "e",      "E",      "\005"},   /* 18 e */
  {ASCII, "r",      "R",      "\022"},   /* 19 r */
  {ASCII, "t",      "T",      "\024"},   /* 20 t */
  {ASCII, "y",      "Y",      "\031"},   /* 21 y */
  {ASCII, "u",      "U",      "\025"},   /* 22 u */
  {ASCII, "i",      "I",      "\011"},   /* 23 i */
  {ASCII, "o",      "O",      "\017"},   /* 24 o */
  {ASCII, "p",      "P",      "\020"},   /* 25 p */
  {ASCII, "[",      "{",      "\033"},   /* 26 [ */
  {ASCII, "]",      "}",      "\035"},   /* 27 ] */
  {ASCII, "\r",     "\r",     "\n"},     /* 28 return */
  {CTL,   "",       "",       ""},       /* 29 control */
  {ASCII, "a",      "A",      "\001"},   /* 30 a */
  {ASCII, "s",      "S",      "\023"},   /* 31 s */
  {ASCII, "d",      "D",      "\004"},   /* 32 d */
  {ASCII, "f",      "F",      "\006"},   /* 33 f */
  {ASCII, "g",      "G",      "\007"},   /* 34 g */
  {ASCII, "h",      "H",      "\010"},   /* 35 h */
  {ASCII, "j",      "J",      "\n"},     /* 36 j */
  {ASCII, "k",      "K",      "\013"},   /* 37 k */
  {ASCII, "l",      "L",      "\014"},   /* 38 l */
  {ASCII, ";",      ":",      ";"},      /* 39 ; */
  {ASCII, "'",      "\"",     "'"},      /* 40 ' */
  {ASCII, "`",      "~",      "`"},      /* 41 ` */
  {SHIFT, "",       "",       ""},       /* 42 shift */
  {ASCII, "\\",     "|",      "\034"},   /* 43 \ */
  {ASCII, "z",      "Z",      "\032"},   /* 44 z */
  {ASCII, "x",      "X",      "\030"},   /* 45 x */
  {ASCII, "c",      "C",      "\003"},   /* 46 c */
  {ASCII, "v",      "V",      "\026"},   /* 47 v */
  {ASCII, "b",      "B",      "\002"},   /* 48 b */
  {ASCII, "n",      "N",      "\016"},   /* 49 n */
  {ASCII, "m",      "M",      "\r"},     /* 50 m */
  {ASCII, ",",      "<",      "<"},      /* 51 , */
  {ASCII, ".",      ">",      ">"},      /* 52 . */
  {ASCII, "/",      "?",      "\037"},   /* 53 / */
  {SHIFT, "",       "",       ""},       /* 54 shift */
  {KP,    "*",      "*",      "*"},      /* 55 kp * */
  {ALT,   "",       "",       ""},       /* 56 alt */
  {ASCII, " ",      " ",      "\000"},   /* 57 space */
  {CAPS,  "",       "",       ""},       /* 58 caps */
  {FUNC,  "\033[M", "\033[Y", "\033[k"}, /* 59 f1 */
  {FUNC,  "\033[N", "\033[Z", "\033[l"}, /* 60 f2 */
  {FUNC,  "\033[O", "\033[a", "\033[m"}, /* 61 f3 */
  {FUNC,  "\033[P", "\033[b", "\033[n"}, /* 62 f4 */
  {FUNC,  "\033[Q", "\033[c", "\033[o"}, /* 63 f5 */
  {FUNC,  "\033[R", "\033[d", "\033[p"}, /* 64 f6 */
  {FUNC,  "\033[S", "\033[e", "\033[q"}, /* 65 f7 */
  {FUNC,  "\033[T", "\033[f", "\033[r"}, /* 66 f8 */
  {FUNC,  "\033[U", "\033[g", "\033[s"}, /* 67 f9 */
  {FUNC,  "\033[V", "\033[h", "\033[t"}, /* 68 f10 */
  {NUM,   "",       "",       ""},       /* 69 num lock */
  {SCROLL,"",       "",       ""},       /* 70 scroll lock */
  {KP,    "7",      "\033[H", "7"},      /* 71 kp 7 */
  {KP,    "8",      "\033[A", "8"},      /* 72 kp 8 */
  {KP,    "9",      "\033[I", "9"},      /* 73 kp 9 */
  {KP,    "-",      "-",      "-"},      /* 74 kp - */
  {KP,    "4",      "\033[D", "4"},      /* 75 kp 4 */
  {KP,    "5",      "\033[E", "5"},      /* 76 kp 5 */
  {KP,    "6",      "\033[C", "6"},      /* 77 kp 6 */
  {KP,    "+",      "+",      "+"},      /* 78 kp + */
  {KP,    "1",      "\033[F", "1"},      /* 79 kp 1 */
  {KP,    "2",      "\033[B", "2"},      /* 80 kp 2 */
  {KP,    "3",      "\033[G", "3"},      /* 81 kp 3 */
  {KP,    "0",      "\033[L", "0"},      /* 82 kp 0 */
  {KP,    ".",      "\177",   "."},      /* 83 kp . */
  {NONE,  "",       "",       ""},       /* 84 0 */
  {NONE,  "100",    "",       ""},       /* 85 0 */
  {NONE,  "101",    "",       ""},       /* 86 0 */
  {FUNC,  "\033[W", "\033[i", "\033[u"}, /* 87 f11 */
  {FUNC,  "\033[X", "\033[j", "\033[v"}, /* 88 f12 */
  {NONE,  "102",    "",       ""},       /* 89 0 */
  {NONE,  "103",    "",       ""},       /* 90 0 */
  {NONE,  "",       "",       ""},       /* 91 0 */
  {NONE,  "",       "",       ""},       /* 92 0 */
  {NONE,  "",       "",       ""},       /* 93 0 */
  {NONE,  "",       "",       ""},       /* 94 0 */
  {NONE,  "",       "",       ""},       /* 95 0 */
  {NONE,  "",       "",       ""},       /* 96 0 */
  {NONE,  "",       "",       ""},       /* 97 0 */
  {NONE,  "",       "",       ""},       /* 98 0 */
  {NONE,  "",       "",       ""},       /* 99 0 */
  {NONE,  "",       "",       ""},       /* 100 */
  {NONE,  "",       "",       ""},       /* 101 */
  {NONE,  "",       "",       ""},       /* 102 */
  {NONE,  "",       "",       ""},       /* 103 */
  {NONE,  "",       "",       ""},       /* 104 */
  {NONE,  "",       "",       ""},       /* 105 */
  {NONE,  "",       "",       ""},       /* 106 */
  {NONE,  "",       "",       ""},       /* 107 */
  {NONE,  "",       "",       ""},       /* 108 */
  {NONE,  "",       "",       ""},       /* 109 */
  {NONE,  "",       "",       ""},       /* 110 */
  {NONE,  "",       "",       ""},       /* 111 */
  {NONE,  "",       "",       ""},       /* 112 */
  {NONE,  "",       "",       ""},       /* 113 */
  {NONE,  "",       "",       ""},       /* 114 */
  {NONE,  "",       "",       ""},       /* 115 */
  {NONE,  "",       "",       ""},       /* 116 */
  {NONE,  "",       "",       ""},       /* 117 */
  {NONE,  "",       "",       ""},       /* 118 */
  {NONE,  "",       "",       ""},       /* 119 */
  {NONE,  "",       "",       ""},       /* 120 */
  {NONE,  "",       "",       ""},       /* 121 */
  {NONE,  "",       "",       ""},       /* 122 */
  {NONE,  "",       "",       ""},       /* 123 */
  {NONE,  "",       "",       ""},       /* 124 */
  {NONE,  "",       "",       ""},       /* 125 */
  {NONE,  "",       "",       ""},       /* 126 */
  {NONE,  "",       "",       ""},       /* 127 */
};

static volatile u_char ack, nak;
static u_char lock_state;

static inline void
kbd_delay (void)
{
  inb(0x84);
  inb(0x84);
  inb(0x84);
  inb(0x84);
}



/*
 * Get characters from the keyboard.  If none are present, return NULL.
 */
static char *
sget (void)
{
  u_char dt;
  static u_char extended = 0, shift_state = 0;
  static u_char capchar[2];

top:
  kbd_delay ();
  dt = inb(KBDATAP);

  switch (dt) {
  case KBR_ACK:
    ack = 1;
    goto loop;
  case KBR_RESEND:
    nak = 1;
    goto loop;
  }

  switch (dt) {
  case KBR_EXTENDED:
    extended = 1;
    goto loop;
  }

  /*
   * Check for ctrl-alt-del.
   */
  if ((dt == 83) && (shift_state & (CTL | ALT)) == (CTL | ALT)) {
    printf ("Rebooting...");
    outb(KBCMDP, KBC_PULSE0);
    for (;;)
      ;
  }

  /*
   * Check for make/break.
   */
  if (dt & 0x80) {
    /*
     * break
     */
    dt &= 0x7f;
    switch (scan_codes[dt].type) {
    case NUM:
      shift_state &= ~NUM;
      break;
    case CAPS:
      shift_state &= ~CAPS;
      break;
    case SCROLL:
      shift_state &= ~SCROLL;
      break;
    case SHIFT:
      shift_state &= ~SHIFT;
      break;
    case ALT:
      shift_state &= ~ALT;
      break;
    case CTL:
      shift_state &= ~CTL;
      break;
    }
  } else {
    /*
     * make
     */
    switch (scan_codes[dt].type) {
      /*
       * locking keys
       */
    case NUM:
      if (shift_state & NUM)
	break;
      shift_state |= NUM;
      lock_state ^= NUM;
      break;
    case CAPS:
      if (shift_state & CAPS)
	break;
      shift_state |= CAPS;
      lock_state ^= CAPS;
      break;
    case SCROLL:
      if (shift_state & SCROLL)
	break;
      shift_state |= SCROLL;
      lock_state ^= SCROLL;
      break;
      /*
       * non-locking keys
       */
    case SHIFT:
      shift_state |= SHIFT;
      break;
    case ALT:
      shift_state |= ALT;
      break;
    case CTL:
      shift_state |= CTL;
      break;
    case ASCII:
      /* control has highest priority */
      if (shift_state & CTL)
	capchar[0] = scan_codes[dt].ctl[0];
      else if (shift_state & SHIFT)
	capchar[0] = scan_codes[dt].shift[0];
      else
	capchar[0] = scan_codes[dt].unshift[0];
      if ((lock_state & CAPS) && capchar[0] >= 'a' &&
	  capchar[0] <= 'z') {
	capchar[0] -= ('a' - 'A');
      }
      capchar[0] |= (shift_state & ALT);
      extended = 0;
      return capchar;
    case NONE:
      break;
    case FUNC: {
      char *more_chars;
      if (shift_state & SHIFT)
	more_chars = scan_codes[dt].shift;
      else if (shift_state & CTL)
	more_chars = scan_codes[dt].ctl;
      else
	more_chars = scan_codes[dt].unshift;
      extended = 0;
      return more_chars;
    }
    case KP: {
      char *more_chars;
      if (shift_state & (SHIFT | CTL) ||
	  (lock_state & NUM) == 0 || extended)
	more_chars = scan_codes[dt].shift;
      else
	more_chars = scan_codes[dt].unshift;
      extended = 0;
      return more_chars;
    }
    }
  }

  extended = 0;
loop:
  if ((inb(KBSTATP) & KBS_DIB) == 0)
    return ("");
  goto top;
}


static char kbd_buf[512];
u_int kbd_first;
u_int kbd_last;

void
kbd_intr () 
{
  char *ip;
  while (inb (KBSTATP) & KBS_DIB) {
    ip = sget ();
#if 1
    // For debugging only
    printf ("%s", ip);
#endif
    while (*ip && (kbd_last + 1) % sizeof (kbd_buf) != kbd_first) {
      kbd_buf[kbd_last++] = *ip++;
      kbd_last %= sizeof (kbd_buf);
    }
  }
}

void
kbd_init (void)
{
  // Drain the kbd's buffer, otherwise interrupts 
  // are not generate interrupts (under Bochs).
  kbd_intr ();

  irq_setmask_8259A (irq_mask_8259A & 0xfffd);
  printf ("  unmasked keyboard interrupts\n");
}


///END
