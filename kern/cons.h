/*
 * Main console driver for PIOS, which manages lower-level console devices
 * such as video (dev/video.*), keyboard (dev/kbd.*), and serial (dev/serial.*)
 *
 * Copyright (c) 2010 Yale University.
 * Copyright (c) 1993, 1994, 1995 Charles Hannum.
 * Copyright (c) 1990 The Regents of the University of California.
 * See section "BSD License" in the file LICENSES for licensing terms.
 *
 * This code is derived from the NetBSD pcons driver, and in turn derived
 * from software contributed to Berkeley by William Jolitz and Don Ahn.
 * Adapted for PIOS by Bryan Ford at Yale University.
 */

#ifndef PIOS_KERN_CONSOLE_H_
#define PIOS_KERN_CONSOLE_H_
#ifndef PIOS_KERNEL
# error "This is a kernel header; user programs should not #include it"
#endif

#include <inc/types.h>


#define DEBUG_TRACEFRAMES	10

struct iocons;


#if LAB >= 2
extern struct spinlock cons_lock;
#endif

void cons_init(void);

// Called by device interrupt routines to feed input characters
// into the circular console input buffer.
// Device-specific code supplies 'proc', which polls for a character
// and returns that character or 0 if no more available from device.
void cons_intr(int (*proc)(void));

// Called by init() when the kernel is ready to receive console interrupts.
void cons_intenable(void);

// Called from file_io() in the context of the root process,
// to synchronize the root process's console special I/O files
// with the kernel's console I/O buffers.
// Returns true if I/O was done, false if no new I/O was ready.
bool cons_io(void);

#endif /* PIOS_KERN_CONSOLE_H_ */
