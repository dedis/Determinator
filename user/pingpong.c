#if LAB >= 4
// Ping-pong a counter between two processes.
// Only need to start one of these -- splits into two with fork.

#include "lib.h"

void
umain(void)
{
	u_int who, i;

	if ((who = fork()) != 0) {
		// get the ball rolling
		printf("send 0 from %x to %x\n", sys_getenvid(), who);
		ipc_send(who, 0);
	}

	for (;;) {
		i = ipc_recv(&who);
		printf("%x got %d from %x\n", sys_getenvid(), i, who);
		if (i == 10)
			return;
		i++;
		ipc_send(who, i);
		if (i == 10)
			return;
	}
		
}

#endif
