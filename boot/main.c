#if LAB >= 1
/*
 *
 * N.B.  NOTE THAT THIS IS THE BOOT LOADER, NOT THE KERNEL.
 * YOU DO NOT NEED TO LOOK AT THIS FILE FOR THE ASSIGNMENT.
 */
#include <inc/x86.h>

/**********************************************************************
 * This a dirt simple boot loader, whose sole job is to boot
 * an a.out kernel image from the first IDE hard disk.
 *
 * DISK LAYOUT
 *  * This program(boot.S and main.c) is the bootloader.  It should
 *    be stored in the first sector of the disk.
 * 
 *  * The 2nd sector onward holds the kernel image.
 *	
 * BOOT UP STEPS	
 *  * when the CPU boots it loads the BIOS into memory and executes it
 *
 *  * the BIOS intializes devices, sets of the interrupt routines, and
 *    reads the first sector of the boot device(e.g., hard-drive) 
 *    into memory and jumps to it.
 *
 *  * Assuming this boot loader is stored in the first sector of the
 *    hard-drive, this code takes over...
 *
 *  * control starts in bootloader.S -- which sets up protected mode,
 *    and a stack so C code then run, then calls cmain()
 *
 *  * cmain() in this file takes over, reads in the kernel and jumps to it.
 **********************************************************************/

#define  SECTOR_SIZE    512

void read_sector(int sector, char *destination);

struct a_out_hdr {
	u_long	a_midmag;	/* flags<<26 | mid<<16 | magic */
	u_long	a_text;		/* text segment size */
	u_long	a_data;		/* initialized data size */
	u_long	a_bss;		/* uninitialized data size */
	u_long	a_syms;		/* symbol table size */
	u_long	a_entry;	/* entry point */
	u_long	a_trsize;	/* text relocation size */
	u_long	a_drsize;	/* data relocation size */
};

void
cmain(void)
{
	u_int8_t sector[SECTOR_SIZE];
	struct a_out_hdr *hdr;
	int i,nsectors;
	char *dest;
	u_int32_t kernel_entry_point;

	// read 2nd sector of disk
	read_sector(1, sector);

	// examine the a.out header
	hdr = (struct a_out_hdr *)sector;
	kernel_entry_point = hdr->a_entry & 0xffffff;
	// number of sectors to be read(rounded up)
	nsectors = sizeof(struct a_out_hdr) + hdr->a_text + hdr->a_data;
	nsectors = (nsectors + SECTOR_SIZE - 1)/SECTOR_SIZE;

	// copy disk sectors into memory
	dest = (unsigned char *)kernel_entry_point - sizeof(struct a_out_hdr);
	for (i = 1; i <= nsectors; i++, dest += SECTOR_SIZE)
		read_sector(i, dest);

	/* jump to the kernel entry point */
	asm volatile("jmp *%0" : : "a" (kernel_entry_point));

	/* NOT REACHED */
}


// located below cmain() to prevent code bloat caused by inlining.
//	(bootloader must fit in a disk sector)
void
read_sector(int sector, char *destination)
{
	unsigned char status;

	do {
		status = inb(0x1f7);
	} while (status & 0x80);

	outb(0x1f2, 1);		// sector count 
	outb(0x1f3, (sector >> 0) & 0xff);
	outb(0x1f4, (sector >> 8) & 0xff);
	outb(0x1f5, (sector >> 16) & 0xff);
	outb(0x1f6, 0xe0 | ((sector >> 24) & 0x0f));
	outb(0x1f7, 0x20);	// CMD 0x20 means read sector
	do {
		status = inb(0x1f7);
	} while (status & 0x80);

	insl(0x1f0, destination, SECTOR_SIZE / 4);
}
#endif /* LAB >= 1 */
