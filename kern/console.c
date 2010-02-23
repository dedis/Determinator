/* See COPYRIGHT for copyright information. */

#include <inc/stdio.h>
#include <inc/stdarg.h>
#include <inc/x86.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/syscall.h>

#include <kern/cpu.h>
#include <kern/console.h>
#include <kern/spinlock.h>
#include <kern/mem.h>
#if LAB >= 4
#include <kern/io.h>
#endif

#include <dev/video.h>
#include <dev/kbd.h>
#include <dev/serial.h>
#if LAB >= 4
#include <dev/pic.h>
#endif

void cons_intr(int (*proc)(void));
static void cons_putc(int c);

#if SOL >= 2
static spinlock cons_lock;	// Spinlock to make console output atomic
#endif

/***** General device-independent console code *****/
// Here we manage the console input buffer,
// where we stash characters received from the keyboard or serial port
// whenever the corresponding interrupt occurs.

static struct {
	uint8_t buf[IOCONS_BUFSIZE];
	uint32_t rpos;
	uint32_t wpos;
} cons;

// called by device interrupt routines to feed input characters
// into the circular console input buffer.
void
cons_intr(int (*proc)(void))
{
	int c;

#if SOL >= 2
	spinlock_acquire(&cons_lock);
#endif
	while ((c = (*proc)()) != -1) {
		if (c == 0)
			continue;
		cons.buf[cons.wpos++] = c;
		if (cons.wpos == IOCONS_BUFSIZE)
			cons.wpos = 0;
	}
#if SOL >= 2
	spinlock_release(&cons_lock);
#endif
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
		if (cons.rpos == IOCONS_BUFSIZE)
			cons.rpos = 0;
		return c;
	}
	return 0;
}

// output a character to the console
static void
cons_putc(int c)
{
	serial_putc(c);
	video_putc(c);
}

// initialize the console devices
void
cons_init(void)
{
	if (!cpu_onboot())	// only do once, on the boot CPU
		return;

#if SOL >= 2
	spinlock_init(&cons_lock);
#endif
	video_init();
	kbd_init();
	serial_init();

	if (!serial_exists)
		cprintf("Serial port does not exist!\n");
}

// Process a console output event a user process
void
cons_output(iocons *io)
{
	io->data[IOCONS_BUFSIZE] = 0;
	cputs(io->data);
}

// Collect any available console input for a waiting user process
bool
cons_input(iocons *io)
{
#if SOL >= 2
	spinlock_acquire(&cons_lock);
#endif
	int amount = cons.wpos - cons.rpos;
	assert(amount >= 0 && amount <= IOCONS_BUFSIZE);
	if (amount > 0) {
		io->type = IO_CONS;
		memmove(io->data, &cons.buf[cons.rpos], amount);
		io->data[amount] = 0;
		cons.rpos = cons.wpos = 0;
	}
#if SOL >= 2
	spinlock_release(&cons_lock);
#endif
	return amount > 0;
}


// `High'-level console I/O.  Used by readline and cprintf.

void
cputs(const char *str)
{
	if (read_cs() & 3)
		return sys_cputs(str);	// use syscall from user mode

#if SOL >= 2
	// Hold the console spinlock while printing the entire string,
	// so that the output of different cputs calls won't get mixed.
	spinlock_acquire(&cons_lock);

#endif
	char ch;
	while (*str)
		cons_putc(*str++);
#if SOL >= 2

	spinlock_release(&cons_lock);
#endif
}

