/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/pmap.h>
#include <inc/kbdreg.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/console.h>
#if LAB >= 4
#include <kern/picirq.h>
#endif


void cons_intr(int (*proc)(void));


/***** Serial I/O code *****/

#define COM1 0x3F8
#define COMSTATUS 5
#define   COMDATA 0x01
#define COMREAD 0
#define COMWRITE 0

int
serial_proc_data(void)
{
	if (!(inb(COM1+COMSTATUS) & COMDATA))
		return -1;
	return inb(COM1+COMREAD);
}

void
serial_intr(void)
{
	cons_intr(serial_proc_data);
}

void
serial_init(void)
{
#if LAB >= 4
	irq_setmask_8259A(irq_mask_8259A & ~(1<<4));	
#endif
}



/***** Parallel port output code *****/
// For information on PC parallel port programming, see:

// Stupid I/O delay routine necessitated by historical PC design flaws
static void
delay(void)
{
	inb(0x84);
	inb(0x84);
	inb(0x84);
	inb(0x84);
}

static void
lpt_putc(int c)
{
	int i;

	for (i=0; !(inb(0x378+1)&0x80) && i<12800; i++)
		delay();
	outb(0x378+0, c);
	outb(0x378+2, 0x08|0x01);
	outb(0x378+2, 0x08);
}




/***** Text-mode CGA/VGA display output *****/

static unsigned addr_6845;
static u_short *crt_buf;
static short crt_pos;

void
cga_init(void)
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
cga_putc(int c)
{
	/* if no attribute given, then use black on white */
	if (!(c & ~0xff)) c |= 0x0700;

	switch (c & 0xff) {
	case '\b':
		if (crt_pos > 0) {
			crt_pos--;
			crt_buf[crt_pos] = (c&~0xff) | ' ';
		}
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

#if SOL >= 2
	// What is the purpose of this?
#else
	/* scroll if necessary */
#endif
	if (crt_pos >= CRT_SIZE) {
		int i;
		memcpy(crt_buf, crt_buf + CRT_COLS, CRT_SIZE << 1);
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



/***** Keyboard input code *****/

#define NO	0

#define SHIFT	(1<<0)
#define CTL	(1<<1)
#define ALT	(1<<2)

#define CAPSLOCK	(1<<3)
#define NUMLOCK		(1<<4)
#define SCROLLOCK	(1<<5)

static int shiftcode[256] = 
{
[29] CTL,
[42] SHIFT,
[54] SHIFT,
[56] ALT,
};

static int togglecode[256] = 
{
[58] CAPSLOCK,
[69] NUMLOCK,
[70] SCROLLOCK,
};

static char normalmap[256] =
{
	NO,    033,  '1',  '2',  '3',  '4',  '5',  '6',
	'7',  '8',  '9',  '0',  '-',  '=',  '\b', '\t',
	'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',
	'o',  'p',  '[',  ']',  '\n', NO,   'a',  's',
	'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',
	'\'', '`',  NO,   '\\', 'z',  'x',  'c',  'v',
	'b',  'n',  'm',  ',',  '.',  '/',  NO,   '*',
	NO,   ' ',   NO,   NO,   NO,   NO,   NO,   NO,
	NO,   NO,   NO,   NO,   NO,   NO,   NO,   '7',
	'8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
	'2',  '3',  '0',  '.',  
};

static char shiftmap[256] = 
{
	NO,   033,  '!',  '@',  '#',  '$',  '%',  '^',
	'&',  '*',  '(',  ')',  '_',  '+',  '\b', '\t',
	'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',
	'O',  'P',  '{',  '}',  '\n', NO,   'A',  'S',
	'D',  'F',  'G',  'H',  'J',  'K',  'L',  ';',
	'"',  '~',  NO,   '|',  'Z',  'X',  'C',  'V',
	'B',  'N',  'M',  '<',  '>',  '?',  NO,   '*',
	NO,   ' ',   NO,   NO,   NO,   NO,   NO,   NO,
	NO,   NO,   NO,   NO,   NO,   NO,   NO,   '7',
	'8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
	'2',  '3',  '0',  '.',  
};

#define C(x) (x-'@')

static char ctlmap[256] = 
{
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO, 
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO, 
	C('Q'),  C('W'),  C('E'),  C('R'),  C('T'),  C('Y'),  C('U'),  C('I'),
	C('O'),  C('P'),  NO,      NO,      '\r',    NO,      C('A'),  C('S'),
	C('D'),  C('F'),  C('G'),  C('H'),  C('J'),  C('K'),  C('L'),  NO, 
	NO,      NO,      NO,      C('\\'), C('Z'),  C('X'),  C('C'),  C('V'),
	C('B'),  C('N'),  C('M'),  NO,      NO,      C('/'),  NO,      NO, 
};

static char *charcode[4] = {
	normalmap,
	shiftmap,
	ctlmap,
	ctlmap,
};

/*
 * Get data from the keyboard.  If we finish a character, return it.  Else 0.
 * Return -1 if no data.
 */
static int
kbd_proc_data(void)
{
	int c;
	u_char data;
	static u_int shift;

	if ((inb(KBSTATP) & KBS_DIB) == 0)
		return -1;

	data = inb(KBDATAP);

	if (data & 0x80) {
		/* key up */
		shift &= ~shiftcode[data&~0x80];
		return 0;
	}

	/* key down */
	shift |= shiftcode[data];
	shift ^= togglecode[data];
	c = charcode[shift&(CTL|SHIFT)][data];

	if (shift&CAPSLOCK) {
		if ('a' <= c && c <= 'z')
			c += 'A' - 'a';
		else if ('A' <= c && c <= 'Z')
			c += 'a' - 'A';
	}
	
	return c;
}

void
kbd_intr(void)
{
	cons_intr(kbd_proc_data);
}

void
kbd_init(void)
{
#if LAB >= 4
	// Drain the kbd buffer so that Bochs generates interrupts.
	kbd_intr();
	irq_setmask_8259A(irq_mask_8259A & ~(1<<1));
#endif
}



/***** General device-independent console code *****/
// Here we manage the console input buffer,
// where we stash characters received from the keyboard or serial port
// whenever the corresponding interrupt occurs.

#define BY2CONS 512

static struct {
	u_char buf[BY2CONS];
	u_int rpos;
	u_int wpos;
} cons;

// called by device interrupt routines to feed input characters
// into the circular console input buffer.
void
cons_intr(int (*proc)(void))
{
	int c;

	while ((c = (*proc)()) != -1) {
		if (c == 0)
			continue;
		cons.buf[cons.wpos++] = c;
		if (cons.wpos == BY2CONS)
			cons.wpos = 0;
	}
}

// return the next input character from the console, or 0 if none waiting
int
cons_getc(void)
{
	int c;

	// poll for any pending input characters,
	// so that this function works even when interrupts are disabled
	// (e.g., when called from the kernel monitor).
	serial_intr();
	kbd_intr();

	// grab the next character from the input buffer.
	if (cons.rpos != cons.wpos) {
		c = cons.buf[cons.rpos++];
		if (cons.rpos == BY2CONS)
			cons.rpos = 0;
		return c;
	}
	return 0;
}

// output a character to the console
void
cons_putc(int c)
{
	lpt_putc(c);
	cga_putc(c);
}

// initialize the console devices
void
cons_init(void)
{
	cga_init();
	kbd_init();
	serial_init();
}



// `High'-level console I/O.  Used by readline and printf.

void
putchar(int c)
{
	cons_putc(c);
}

int
getchar(void)
{
	int c;

	while ((c = cons_getc()) == 0)
		; // spin
	return c;
}

int
iscons(int fdnum)
{
	// used by readline
	return 1;
}

