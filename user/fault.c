#if LAB >= 5
#elif LAB >= 4
// buggy program - faults

#include <inc/lib.h>

void
umain(void)
{
	*(u_int*)0 = 0;
}

#endif
