#if LAB >= 5
#elif LAB >= 3
// buggy program - faults with a read from location zero

#include <inc/lib.h>

void
umain(void)
{
	printf("I read %08x from location 0!\n", *(u_int*)0);
}

#endif
