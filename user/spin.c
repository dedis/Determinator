#if LAB >= 5
#elif LAB >= 4
// Test preemption by forking off a child process that just spins forever.
// Let it run for a couple time slices, then kill it.

#include <inc/lib.h>

void
umain(void)
{
	u_int who;

	printf("I am the parent.  Forking the child...\n");
	if ((who = fork()) == 0) {
		printf("I am the child.  Spinning...\n");
		for (;;) { }
	}

	printf("I am the parent.  Running the child...\n");
	sys_yield();
	sys_yield();

	printf("I am the parent.  Killing the child...\n");
	sys_env_destroy(who);
}

#endif
