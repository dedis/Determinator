#if LAB >= 5
/*
 * Minimal PIO-based (non-interrupt-driven) IDE driver code.
 * For information about what all this IDE/ATA magic means,
 * see the materials available on the class references page.
 */

#include "fs.h"
#include <inc/x86.h>

void
ide_read(uint32_t diskno, uint32_t secno, void *dst, size_t nsecs)
{
	assert(nsecs <= 256);

	while ((inb(0x1F7) & 0xC0) != 0x40)
		/* do nothing */;

	outb(0x1F2, nsecs);
	outb(0x1F3, secno & 0xFF);
	outb(0x1F4, (secno >> 8) & 0xFF);
	outb(0x1F5, (secno >> 16) & 0xFF);
	outb(0x1F6, 0xE0 | ((diskno&1)<<4) | ((secno>>24)&0x0F));
	outb(0x1F7, 0x20);	// CMD 0x20 means read sector

	for (; nsecs > 0; nsecs--, dst += SECTSIZE) {
		while ((inb(0x1F7) & 0xC0) != 0x40)
			/* do nothing */;
		insl(0x1F0, dst, SECTSIZE/4);
	}
}

void
ide_write(uint32_t diskno, uint32_t secno, const void *src, size_t nsecs)
{
	assert(nsecs <= 256);

	while ((inb(0x1F7) & 0xC0) != 0x40)
		/* do nothing */;

	outb(0x1F2, nsecs);
	outb(0x1F3, secno & 0xFF);
	outb(0x1F4, (secno >> 8) & 0xFF);
	outb(0x1F5, (secno >> 16) & 0xFF);
	outb(0x1F6, 0xE0 | ((diskno&1)<<4) | ((secno>>24)&0x0F));
	outb(0x1F7, 0x30);	// CMD 0x30 means write sector

	for (; nsecs > 0; nsecs--, src += SECTSIZE) {
		while ((inb(0x1F7) & 0xC0) != 0x40)
			/* do nothing */;
		outsl(0x1F0, src, SECTSIZE/4);
	}
}

#endif
