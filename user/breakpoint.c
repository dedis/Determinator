#if LAB >= 4
#elif LAB >= 3
// program to cause a breakpoint trap

#include <inc/lib.h>

void
umain(void)
{
	asm volatile("int $3");
}

#endif
