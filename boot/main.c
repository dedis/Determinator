#if LAB >= 1
/*
 *
 * N.B.  NOTE THAT THIS IS THE BOOT LOADER, NOT THE KERNEL.
 * YOU DO NOT NEED TO LOOK AT THIS FILE FOR THE ASSIGNMENT.
 */
#include <inc/x86.h>
#include <inc/elf.h>

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
 *  * The kernel image may be in a.out or ELF format.
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

static u_char *sect = (u_char*)0x10000;	// scratch space

void readsect(u_char*, u_int, u_int);
void readseg(u_int, u_int, u_int);

void
cmain(void)
{
	u_int entry, i;
	struct Elf *elf;
	struct Proghdr *ph;

	// read 1st page off disk
	readsect(sect, 8, 1);

	if(*(u_int*)sect != 0x464C457F)	// \x7F ELF in little endian
		goto bad;

	// look at ELF header - ignores ph flags
	elf = (struct Elf*)sect;
	entry = elf->e_entry;
	ph = (struct Proghdr*)(sect+elf->e_phoff);
	for(i=0; i<elf->e_phnum; i++, ph++)
		readseg(ph->p_va, ph->p_memsz, ph->p_offset);

	entry &= 0xFFFFFF;
	((void(*)(void))entry)();
	/* DOES NOT RETURN */

bad:
	outw(0x8A00, 0x8A00);
	outw(0x8A00, 0x8E00);
	for(;;);
	
}

// read count bytes at offset from kernel into addr dst
// might copy more than asked
void
readseg(u_int va, u_int count, u_int offset)
{
	u_int i;

	va &= 0xFFFFFF;

	// round down to sector boundary; offset will round later
	i = va&511;
	count += i;
	va -= i;

	// translate from bytes to sectors
	offset /= 512;
	count = (count+511)/512;

	// kernel starts at sector 1
	offset++;

	// if this is too slow, we could read lots of sectors at a time.
	// we'd write more to memory than asked, but it doesn't matter --
	// we load in increasing order.
	for(i=0; i<count; i++){
		readsect((u_char*)va, 1, offset+i);
		va += 512;
	}
}

void
notbusy(void)
{
	while(inb(0x1F7) & 0x80);	// wait for disk not busy
}

void
readsect(u_char *dst, u_int count, u_int offset)
{
	notbusy();

	outb(0x1F2, count);
	outb(0x1F3, offset);
	outb(0x1F4, offset>>8);
	outb(0x1F5, offset>>16);
	outb(0x1F6, (offset>>24)|0xE0);
	outb(0x1F7, 0x20);	// cmd 0x20 - read sectors

	notbusy();

	insl(0x1F0, dst, count*512/4);
}

#endif /* LAB >= 1 */
