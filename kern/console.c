/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/mmu.h>
#include <inc/kbdreg.h>
#include <kern/pmap.h>
#include <kern/console.h>
#include <kern/printf.h>
#include <kern/picirq.h>

static unsigned addr_6845;
static u_short *crt_buf;
static short crt_pos;

/* Output to alternate parallel port console */
static void
delay(void)
{
	inb(0x84);
	inb(0x84);
	inb(0x84);
	inb(0x84);
}

static void
lptputc(int c)
{
	int i;

	for (i=0; !(inb(0x378+1)&0x80) && i<12800; i++)
		delay();
	outb(0x378+0, c);
	outb(0x378+2, 0x08|0x01);
	outb(0x378+2, 0x08);
}

/* Normal CGA-based console output */
void
cons_init(void)
{
	u_short volatile *cp;
	u_short was;
	u_int pos;

	cp = (short *) (KERNBASE + CGA_BUF);
	was = *cp;
	*cp = (u_short) 0xA55A;
	if (*cp != 0xA55A) {
		cp = (short *) (KERNBASE + MONO_BUF);
		addr_6845 = MONO_BASE;
	} else {
		*cp = was;
		addr_6845 = CGA_BASE;
	}
	
	/* Extract cursor location */
	outb(addr_6845, 14);
	pos = inb(addr_6845+1) << 8;
	outb(addr_6845, 15);
	pos |= inb(addr_6845+1);

	crt_buf = (u_short *)cp;
	crt_pos = pos;
}


void
cons_putc(short int c)
{
	lptputc(c);

	/* if no attribute given, then use black on white */
	if (!(c & ~0xff)) c |= 0x0700;

	switch (c & 0xff) {
	case '\b':
		if (crt_pos > 0)
			crt_pos--;
		break;
	case '\n':
		crt_pos += CRT_COLS;
		/* cascade	*/
	case '\r':
		crt_pos -= (crt_pos % CRT_COLS);
		break;
	case '\t':
		cons_putc(' ');
		cons_putc(' ');
		cons_putc(' ');
		cons_putc(' ');
		cons_putc(' ');
		break;
	default:
		crt_buf[crt_pos++] = c;		/* write the character */
		break;
	}

///SOL2
	// What is the purpose of this?
///ELSE
	/* scroll if necessary */
///END
	if (crt_pos >= CRT_SIZE) {
		int i;
		bcopy(crt_buf + CRT_COLS, crt_buf, CRT_SIZE << 1);
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
