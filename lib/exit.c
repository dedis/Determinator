#if LAB >= 3

#include <inc/lib.h>

void
exit(void)
{
#if LAB >= 5
	close_all();
#endif
	sys_env_destroy(0);
}

#endif	// LAB >= 3
