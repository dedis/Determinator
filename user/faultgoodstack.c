#if LAB >= 5
#elif LAB >= 4
// test user-level fault handler -- just exit when we fault
// run the handlers on the user stack, just to make sure we can.

#include "lib.h"

void
handler(u_int va, u_int err)
{
	printf("i faulted at va %x, err %x, stack %x\n", va, err&7, (u_int)&va);
	sys_env_destroy();
}

void
umain(void)
{
	set_pgfault_handler(handler);
	sys_set_pgfault_handler(0, env->env_pgfault_handler, USTACKTOP);
	*(int*)0xDeadBeef = 0;
}
#endif
