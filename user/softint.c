#if LAB >= 4
#elif LAB >= 3
// buggy program - causes an illegal software interrupt

#include <inc/lib.h>

void
umain(void)
{
	asm volatile("int $14");	// page fault
}

#endif

