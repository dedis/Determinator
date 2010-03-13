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
#include <kern/file.h>
#endif

#include <dev/video.h>
#include <dev/kbd.h>
#include <dev/serial.h>

void cons_intr(int (*proc)(void));
static void cons_putc(int c);

#if SOL >= 2
spinlock cons_lock;	// Spinlock to make console output atomic
#endif

/***** General device-independent console code *****/
// Here we manage the console input buffer,
// where we stash characters received from the keyboard or serial port
// whenever the corresponding interrupt occurs.

#define CONSBUFSIZE 512

static struct {
	uint8_t buf[CONSBUFSIZE];
	uint32_t rpos;
	uint32_t wpos;
} cons;

static int cons_outsize;	// Console output already written by root proc

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
		if (cons.wpos == CONSBUFSIZE)
			cons.wpos = 0;
	}
#if SOL >= 2
	spinlock_release(&cons_lock);

#if SOL >= 4	// XXX just give this code to the students!
	// Wake the root process
	file_wakeroot();
#endif
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
		if (cons.rpos == CONSBUFSIZE)
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

#if LAB >= 4
// Enable console interrupts.
void
cons_intenable(void)
{
	if (!cpu_onboot())	// only do once, on the boot CPU
		return;

	kbd_intenable();
	serial_intenable();
}
#endif // LAB >= 4

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

// Synchronize the root process's console special files
// with the actual console I/O device.
bool
cons_io(void)
{
	spinlock_acquire(&cons_lock);
	bool didio = 0;

	// Console output from the root process's console output file
	fileinode *outfi = &files->fi[FILEINO_CONSOUT];
	const char *outbuf = FILEDATA(FILEINO_CONSOUT);
	assert(cons_outsize <= outfi->size);
	while (cons_outsize < outfi->size) {
		cons_putc(outbuf[cons_outsize++]);
		didio = 1;
	}

	// Console input to the root process's console input file
	fileinode *infi = &files->fi[FILEINO_CONSIN];
	char *inbuf = FILEDATA(FILEINO_CONSIN);
	int amount = cons.wpos - cons.rpos;
	if (infi->size + amount > FILE_MAXSIZE)
		panic("cons_io: root process's console input file full!");
	assert(amount >= 0 && amount <= CONSBUFSIZE);
	if (amount > 0) {
		memmove(&inbuf[infi->size], &cons.buf[cons.rpos], amount);
		infi->size += amount;
		cons.rpos = cons.wpos = 0;
		didio = 1;
	}

	spinlock_release(&cons_lock);
	return didio;
}

