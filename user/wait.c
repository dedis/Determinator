#if LAB >= 6
#include "lib.h"

void
wait(u_int envid)
{
	struct Env *e;

	e = &envs[ENVX(envid)];
	while(e->env_id == envid && e->env_status != ENV_FREE)
		sys_yield();
}


#endif
