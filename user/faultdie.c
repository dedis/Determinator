#if LAB >= 5
#elif LAB >= 4
// test user-level fault handler -- just exit when we fault

#include <inc/lib.h>

void
handler(u_int va, u_int err)
{
	printf("i faulted at va %x, err %x\n", va, err&7);
	sys_env_destroy(sys_getenvid());
}

void
umain(void)
{
	set_pgfault_handler(handler);
	*(int*)0xDeadBeef = 0;
}
#endif
