#if LAB >= 5
#elif LAB >= 4
// test an evil stack for user-level fault handler

#include <inc/lib.h>

void
loop(void)
{
	for(;;);
}

void
umain(void)
{
	sys_set_pgfault_handler(0, (u_int)loop, 0xF0100020);
	*(int*)0 = 0;
}
#endif
