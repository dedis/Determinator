/* See COPYRIGHT for copyright information. */

#ifndef PIOS_KERN_CONSOLE_H_
#define PIOS_KERN_CONSOLE_H_
#ifndef PIOS_KERNEL
# error "This is a kernel header; user programs should not #include it"
#endif

#include <inc/types.h>


#define DEBUG_TRACEFRAMES	10

struct iocons;


void cons_init(void);

// Called by device interrupt routines to feed input characters
// into the circular console input buffer.
// Device-specific code supplies 'proc', which polls for a character
// and returns that character or 0 if no more available from device.
void cons_intr(int (*proc)(void));

// General console I/O functions - see kern/io.c
void cons_output(struct iocons *io);
bool cons_input(struct iocons *io);

#endif /* PIOS_KERN_CONSOLE_H_ */
