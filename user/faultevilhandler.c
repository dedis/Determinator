#if LAB >= 5
#elif LAB >= 4
// test evil pointer for user-level fault handler

#include <inc/lib.h>

void
umain(void)
{
	sys_mem_alloc(0, UXSTACKTOP-BY2PG, PTE_P|PTE_U|PTE_W);
	sys_set_pgfault_entry(0, 0xF0100020);
	*(int*)0 = 0;
}
#endif
