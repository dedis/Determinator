#if LAB >= 4
// Ping-pong a counter between two shared-memory processes.
// Only need to start one of these -- splits into two with sfork.

#include "lib.h"

u_int val;

void
umain(void)
{
	u_int who, i;

	i = 0;
	if ((who = sfork()) != 0) {
		printf("i am %08x; env is %p\n", sys_getenvid(), env);
		// get the ball rolling
		printf("send 0 from %x to %x\n", sys_getenvid(), who);
		ipc_send(who, 0);
	}

	for (;;) {
		ipc_recv(&who);
		printf("%x got %d from %x (env is %p %x)\n", sys_getenvid(), val, who, env, env->env_id);
		if (val == 10)
			return;
		++val;
		ipc_send(who, 0);
		if (val == 10)
			return;
	}
		
}
#endif
