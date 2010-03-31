#if LAB >= 99
/*
 * Minimal PIO-based (non-interrupt-driven) IDE driver code.
 * For information about what all this IDE/ATA magic means,
 * see the materials available on the class references page.
 */

#include <inc/stdio.h>
#include <inc/assert.h>
#include <inc/x86.h>
#include <inc/trap.h>

#include <kern/spinlock.h>
#include <kern/mp.h>
#include <kern/io.h>

#include <dev/ide.h>
#include <dev/pic.h>
#include <dev/ioapic.h>

#define IDE_BSY		0x80
#define IDE_DRDY	0x40
#define IDE_DF		0x20
#define IDE_ERR		0x01

static int diskno = 1;

static spinlock idelock;
static bool pending;		// read/write I/O operation in progress
static bool write;		// pending operation is a write
static int secno;		// sector number of pending operation
static bool done;		// pending operation has completed

static int
ide_wait()
{
	int r;
	while (((r = inb(0x1F7)) & (IDE_BSY|IDE_DRDY)) != IDE_DRDY)
		pause();
	return (r & (IDE_DF|IDE_ERR)) == 0;
}

void
ide_init(void)
{
	int r, x;

	spinlock_init(&idelock);

	// Enable the IDE interrupt
	pic_enable(IRQ_IDE);
	ioapic_enable(IRQ_IDE, ncpu - 1);

	// wait for Device 0 to be ready
	ide_wait();

	// switch to Device 1
	outb(0x1F6, 0xE0 | (1<<4));

	// check for Device 1 to be ready for a while
	for (x = 0; 
           x < 1000 && ((r = inb(0x1F7)) & (IDE_BSY|IDE_DF|IDE_ERR)) != 0; 
           x++)
		pause();
	if (x == 1000)
		panic("Disk 1 not present!");

	// switch back to Device 0
	outb(0x1F6, 0xE0 | (0<<4));
}

// Output to disk device: initiate a sector read or write.
void ide_output(const struct iodisk *io)
{
	spinlock_acquire(&idelock);
	if(pending)
		panic("overlapped disk I/O not supported");

	ide_wait();

	outb(0x3f6, 0);		// generate interrupt
	outb(0x1f2, 1);		// number of sectors
	outb(0x1f3, io->secno & 0xff);
	outb(0x1f4, (io->secno >> 8) & 0xff);
	outb(0x1f5, (io->secno >> 16) & 0xff);
	outb(0x1f6, 0xe0 | ((diskno & 1)<<4) | ((io->secno>>24) & 0x0F));

	if (io->write) {
		outb(0x1f7, 0x30);	// CMD 0x30 means write sector
		outsl(0x1f0, io->data, IODISK_SECSIZE/4);
	} else {
		outb(0x1f7, 0x20);	// CMD 0x20 means read sector
	}

	pending = 1;
	write = io->write;
	secno = io->secno;
	done = 0;
	spinlock_release(&idelock);
}

// Input from disk device: collect the results of a read or write.
bool ide_input(struct iodisk *io)
{
	spinlock_acquire(&idelock);
	if (!pending || !done) {
		spinlock_release(&idelock);
		return 0;	// no results ready to return
	}

	// Fill in the iodisk struct with the completion information
	io->type = IO_DISK;
	io->write = write;
	io->secno = secno;

	// If it was a read operation, pull in the sector data
	if (!write && ide_wait())
		insl(0x1f0, io->data, IODISK_SECSIZE/4);
	pending = done = 0;

	spinlock_release(&idelock);
	return 1;
}

void
ide_intr(void)
{
	spinlock_acquire(&idelock);
	if (pending)
		done = 1;
	else
		warn("Suprious IDE interrupt\n");
	spinlock_release(&idelock);

	io_check();	// see if the user process is waiting for input
}

#endif	/* LAB >= 4 */
