#if LAB >= 2
/* See COPYRIGHT for copyright information. */

/* Support for two time-related hardware gadgets: 1) the run time
 * clock with its NVRAM access functions; 2) the 8253 timer, which
 * generates interrupts on IRQ 0.
 */

#include <inc/x86.h>

#include <dev/nvram.h>


unsigned
nvram_read(unsigned reg)
{
	outb(IO_RTC, reg);
	return inb(IO_RTC+1);
}

unsigned
nvram_read16(unsigned r)
{
	return nvram_read(r) | (nvram_read(r + 1) << 8);
}

void
nvram_write(unsigned reg, unsigned datum)
{
	outb(IO_RTC, reg);
	outb(IO_RTC+1, datum);
}


#endif /* LAB >= 2 */
