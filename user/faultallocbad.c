#if LAB >= 5
#elif LAB >= 4
// test user-level fault handler -- alloc pages to fix faults
// doesn't work because we sys_cputs instead of printf (exercise: why?)

#include <inc/lib.h>

void
handler(void *va, uint32_t err)
{
	int r;

	printf("fault %x\n", va);
	if ((r = sys_page_alloc(0, va, PTE_P|PTE_U|PTE_W)) < 0)
		panic("allocating at %x in page fault handler: %e", va, r);
	snprintf((char*)va, 100, "this string was faulted in at %x", va);
}

void
umain(void)
{
	set_pgfault_handler(handler);
	sys_cputs((char*)0xDEADBEEF);
}
#endif
