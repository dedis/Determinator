#if LAB >= 5
#elif LAB >= 4
// test bad stack for user-level fault handler

#include "lib.h"

void
loop(void)
{
	for(;;);
}

void
umain(void)
{
	sys_set_pgfault_handler(0, (u_int)loop, 0);
	*(int*)0 = 0;
}
#endif
