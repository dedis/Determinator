#if LAB >= 1
/*
 * Device driver code for accessing the PC's nonvolatile RAM (NVRAM),
 * which is part of the PC's real-time clock/calendar.
 *
 * Copyright (C) 1998 Exotec, Inc.
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Derived from the MIT Exokernel and JOS.
 * Adapted for PIOS by Bryan Ford at Yale University.
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


#endif /* LAB >= 1 */
