#if LAB >= 1
// Definition's for the PC's nonvolatile RAM (NVRAM),
// part of the battery-backed real-timem clock.
// The kernel reads the NVRAM to detect how much memory we have.
// See COPYRIGHT for copyright information.
#ifndef PIOS_DEV_NVRAM_H
#define PIOS_DEV_NVRAM_H
#ifndef PIOS_KERNEL
# error "This is a JOS kernel header; user programs should not #include it"
#endif

#define	IO_RTC		0x070		/* RTC port */

#define	MC_NVRAM_START	0xe	/* start of NVRAM: offset 14 */
#define	MC_NVRAM_SIZE	50	/* 50 bytes of NVRAM */

/* NVRAM bytes 7 & 8: base memory size */
#define NVRAM_BASELO	(MC_NVRAM_START + 7)	/* low byte; RTC off. 0x15 */
#define NVRAM_BASEHI	(MC_NVRAM_START + 8)	/* high byte; RTC off. 0x16 */

/* NVRAM bytes 9 & 10: extended memory size */
#define NVRAM_EXTLO	(MC_NVRAM_START + 9)	/* low byte; RTC off. 0x17 */
#define NVRAM_EXTHI	(MC_NVRAM_START + 10)	/* high byte; RTC off. 0x18 */

/* NVRAM bytes 34 and 35: extended memory POSTed size */
#define NVRAM_PEXTLO	(MC_NVRAM_START + 34)	/* low byte; RTC off. 0x30 */
#define NVRAM_PEXTHI	(MC_NVRAM_START + 35)	/* high byte; RTC off. 0x31 */

/* NVRAM byte 36: current century.  (please increment in Dec99!) */
#define NVRAM_CENTURY	(MC_NVRAM_START + 36)	/* RTC offset 0x32 */


// Read NVRAM registers
unsigned nvram_read(unsigned reg);	// read an 8-bit byte from NVRAM
unsigned nvram_read16(unsigned reg);	// read a 16-bit word from NVRAM

// Write to NVRAM
void nvram_write(unsigned reg, unsigned datum);


#endif	// !PIOS_DEV_NVRAM_H
#endif	// LAB >= 1
