#if LAB >= 5
#elif LAB >= 4
// Ping-pong a counter between two processes.
// Start two instances of this program as envs 1 and 2
// (user/idle is env 0).

#include <inc/lib.h>

void
umain(void)
{
	u_int who, i;

	if (env == &envs[1]) {
		// get the ball rolling
		ipc_send(envs[2].env_id, 0, 0, 0);
	}

	for (;;) {
		i = ipc_recv(&who, 0, 0);
		printf("%x got %d from %x\n", sys_getenvid(), i, who);
		if (i == 10)
			return;
		i++;
		ipc_send(who, i, 0, 0);
		if (i == 10)
			return;
	}
}
#endif
