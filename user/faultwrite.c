#if LAB >= 6
#elif LAB >= 3
// buggy program - faults with a write to location zero

#include <inc/lib.h>

void
umain(void)
{
	*(unsigned*)0 = 0;
}

#endif
