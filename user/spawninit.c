#if LAB >= 4
#include <inc/lib.h>

void
umain(void)
{
	int r;
	printf("i am parent environment %08x\n", env->env_id);
	if ((r = spawnl("init", "init", "one", "two", 0)) < 0)
		panic("spawnl(init) failed: %e", r);
}
#endif
