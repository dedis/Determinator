#if LAB >= 5
#elif LAB >= 4
// hello, world

#include "lib.h"

void
umain(void)
{
	sys_cputs("hello, world\n");
	printf("i am environment %08x\n", env->env_id);
}
#endif
