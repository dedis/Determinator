#if LAB >= 5
#elif LAB >= 4
// test user fault handler being called with no exception stack mapped

#include <inc/lib.h>

void _pgfault_entry();

void
umain(void)
{
	sys_env_set_pgfault_upcall(0, (u_long)_pgfault_entry);
	*(int*)0 = 0;
}
#endif
