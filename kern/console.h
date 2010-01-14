/* See COPYRIGHT for copyright information. */

#ifndef PIOS_KERN_CONSOLE_H_
#define PIOS_KERN_CONSOLE_H_
#ifndef PIOS_KERNEL
# error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <inc/types.h>

void cons_init(void);
int cons_getc(void);

// Called by device interrupt routines to feed input characters
// into the circular console input buffer.
// Device-specific code supplies 'proc', which polls for a character
// and returns that character or 0 if no more available from device.
void cons_intr(int (*proc)(void));

void kbd_intr(void); // irq 1
void serial_intr(void); // irq 4

#endif /* PIOS_KERN_CONSOLE_H_ */
