#if LAB >= 4
#elif LAB >= 3
// buggy hello world -- unmapped pointer passed to kernel
// kernel should destroy user environment in response

#include <inc/lib.h>

void
umain(void)
{
	sys_cputs((char*)1);
}

#endif
