#if LAB >= 5
/*
 * Minimal PIO-based (non-interrupt-driven) IDE driver code.
 * For information about what all this IDE/ATA magic means,
 * see for example "The Guide to ATA/ATAPI documentation" at:
 *	http://www.stanford.edu/~csapuntz/ide.html
 */

#include "fs.h"

void
read_sectors(u_int diskno, u_int secno, void *dst, u_int nsecs)
{
	assert(nsecs <= 256);

	while(inb(0x1F7)&0x80);

	outb(0x1F2, nsecs);
	outb(0x1F3, secno & 0xFF);
	outb(0x1F4, (secno >> 8) & 0xFF);
	outb(0x1F5, (secno >> 16) & 0xFF);
	outb(0x1F6, 0xE0 | ((diskno&1)<<4) | ((n>>24)&0x0F));
	outb(0x1F7, 0x20);	// CMD 0x20 means read sector

	while(inb(0x1F7)&0x80);

	insl(0x1F0, dst, nsecs*BY2SECT/4);
}

void
write_sectors(u_int diskno, u_int secno, void *src, u_int nsecs)
{
	assert(nsecs <= 256);

	while(inb(0x1F7)&0x80);

	outb(0x1F2, nsecs);
	outb(0x1F3, secno & 0xFF);
	outb(0x1F4, (secno >> 8) & 0xFF);
	outb(0x1F5, (secno >> 16) & 0xFF);
	outb(0x1F6, 0xE0 | ((diskno&1)<<4) | ((n>>24)&0x0F));
	outb(0x1F7, 0x30);	// CMD 0x30 means write sector

	while(inb(0x1F7)&0x80);

	outsl(0x1F0, dst, nsecs*BY2SECT/4);
}

#endif /* LAB >= 5 */
