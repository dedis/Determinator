#ifndef _BOCHS_H_
#define _BOCHS_H_ 1

#include <inc/x86.h>

// This function performs a magic sequence of I/O operations
// that makes Bochs break into the debugger
// (only if we're running under Bochs, of course).
static inline void
bochs(void)
{
	outw(0x8A00, 0x8A00);
	outw(0x8A00, 0x8AE0);
}

#endif

