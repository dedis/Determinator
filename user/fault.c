#if LAB >= 4
#elif LAB >= 3
// buggy program - faults

#include <inc/lib.h>

void
umain(void)
{
	*(u_int*)0 = 0;
}

#endif
