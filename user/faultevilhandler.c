#if LAB >= 4
#elif LAB >= 3
// test evil pointer for user-level fault handler

#include <inc/lib.h>

void
umain(void)
{
	sys_set_pgfault_handler(0, 0xF0100020);
	*(int*)0 = 0;
}
#endif
