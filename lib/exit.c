#if LAB >= 3

#include <inc/lib.h>
#include <inc/syscall.h>

void
exit(void)
{
	sys_ret();
}

#endif	// LAB >= 3
