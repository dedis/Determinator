#if LAB >= 4
#elif LAB >= 3
// buggy program - causes a divide by zero exception

#include <inc/lib.h>

int zero;

void
umain(void)
{
	printf("1/0 is %08x!\n", 1/zero);
}

#endif
