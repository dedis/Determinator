/*
 * Text-mode CGA/VGA display output device driver.
 *
 * Copyright (c) 2010 Yale University.
 * Copyright (c) 1993, 1994, 1995 Charles Hannum.
 * Copyright (c) 1990 The Regents of the University of California.
 * See section "BSD License" in the file LICENSES for licensing terms.
 *
 * This code is derived from the NetBSD pcons driver, and in turn derived
 * from software contributed to Berkeley by William Jolitz and Don Ahn.
 * Adapted for PIOS by Bryan Ford at Yale University.
 */

#include <inc/string.h>

#include <kern/mem.h>

#include <dev/video.h>


static unsigned addr_6845;
static uint16_t *crt_buf;
static uint16_t crt_pos;

#if LAB >= 9
#if CRT_SAVEROWS > 0
static uint16_t crtsave_buf[CRT_SAVEROWS * CRT_COLS];
static uint16_t crtsave_pos;
static int16_t crtsave_backscroll;
static uint16_t crtsave_size;
#endif
#endif
void
video_init(void)
{
	volatile uint16_t *cp;
	uint16_t was;
	unsigned pos;

	/* Get a pointer to the memory-mapped text display buffer. */
	cp = (uint16_t*) mem_ptr(CGA_BUF);
	was = *cp;
	*cp = (uint16_t) 0xA55A;
	if (*cp != 0xA55A) {
		cp = (uint16_t*) mem_ptr(MONO_BUF);
		addr_6845 = MONO_BASE;
	} else {
		*cp = was;
		addr_6845 = CGA_BASE;
	}

	/* Extract cursor location */
	outb(addr_6845, 14);
	pos = inb(addr_6845 + 1) << 8;
	outb(addr_6845, 15);
	pos |= inb(addr_6845 + 1);

	crt_buf = (uint16_t*) cp;
	crt_pos = pos;
}


#if LAB >= 9
#if CRT_SAVEROWS > 0
// Copy one screen's worth of data to or from the save buffer,
// starting at line 'first_line'.
static void
video_savebuf_copy(int first_line, bool to_screen)
{
	uint16_t *pos;
	uint16_t *end;
	uint16_t *trueend;

	// Calculate the beginning & end of the save buffer area.
	pos = crtsave_buf + (first_line % CRT_SAVEROWS) * CRT_COLS;
	end = pos + CRT_ROWS * CRT_COLS;
	// Check for wraparound.
	trueend = MIN(end, crtsave_buf + CRT_SAVEROWS * CRT_COLS);

	// Copy the initial portion.
	if (to_screen)
		memmove(crt_buf, pos, (trueend - pos) * sizeof(uint16_t));
	else
		memmove(pos, crt_buf, (trueend - pos) * sizeof(uint16_t));

	// If there was wraparound, copy the second part of the screen.
	if (end == trueend)
		/* do nothing */;
	else if (to_screen)
		memmove(crt_buf + (trueend - pos), crtsave_buf, (end - trueend) * sizeof(uint16_t));
	else
		memmove(crtsave_buf, crt_buf + (trueend - pos), (end - trueend) * sizeof(uint16_t));
}

#endif
#endif

void
video_putc(int c)
{
#if LAB >= 9
#if CRT_SAVEROWS > 0
	// unscroll if necessary
	if (crtsave_backscroll > 0) {
		video_savebuf_copy(crtsave_pos + crtsave_size, 1);
		crtsave_backscroll = 0;
	}

#endif
#endif
	// if no attribute given, then use black on white
	if (!(c & ~0xFF))
		c |= 0x0700;

	switch (c & 0xff) {
	case '\b':
		if (crt_pos > 0) {
			crt_pos--;
			crt_buf[crt_pos] = (c & ~0xff) | ' ';
		}
		break;
	case '\n':
		crt_pos += CRT_COLS;
		/* fallthru */
	case '\r':
		crt_pos -= (crt_pos % CRT_COLS);
		break;
	case '\t':
		video_putc(' ');
		video_putc(' ');
		video_putc(' ');
		video_putc(' ');
		video_putc(' ');
		break;
	default:
		crt_buf[crt_pos++] = c;		/* write the character */
		break;
	}

#if SOL >= 1
	/* scroll if necessary */
#else
	// What is the purpose of this?
#endif
	if (crt_pos >= CRT_SIZE) {
		int i;

#if LAB >= 9
#if CRT_SAVEROWS > 0
		// Save the scrolled-back row
		if (crtsave_size == CRT_SAVEROWS - CRT_ROWS)
			crtsave_pos = (crtsave_pos + 1) % CRT_SAVEROWS;
		else
			crtsave_size++;
		memmove(crtsave_buf + ((crtsave_pos + crtsave_size - 1) % CRT_SAVEROWS) * CRT_COLS, crt_buf, CRT_COLS * sizeof(uint16_t));
#endif
#endif
		memmove(crt_buf, crt_buf + CRT_COLS,
			(CRT_SIZE - CRT_COLS) * sizeof(uint16_t));
		for (i = CRT_SIZE - CRT_COLS; i < CRT_SIZE; i++)
			crt_buf[i] = 0x0700 | ' ';
		crt_pos -= CRT_COLS;
	}

	/* move that little blinky thing */
	outb(addr_6845, 14);
	outb(addr_6845 + 1, crt_pos >> 8);
	outb(addr_6845, 15);
	outb(addr_6845 + 1, crt_pos);
}

#if LAB >= 9
#if CRT_SAVEROWS > 0
void
video_scroll(int delta)
{
	int new_backscroll = MAX(MIN(crtsave_backscroll - delta, crtsave_size), 0);

	if (new_backscroll == crtsave_backscroll)
		return;
	if (crtsave_backscroll == 0)
		// save current screen
		video_savebuf_copy(crtsave_pos + crtsave_size, 0);

	crtsave_backscroll = new_backscroll;
	video_savebuf_copy(crtsave_pos + crtsave_size - crtsave_backscroll, 1);
}
#endif
#endif

