#if LAB >= 3

#include <inc/assert.h>

#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/picirq.h>

#if LAB >= 4
#else
// Trivial temporary clock interrupt handler,
// called from clock_interrupt in locore.S
void
clock(void)
{
	printf("*");
}
#endif


// The real clock interrupt handler,
// implementing round-robin scheduling
void
sched_yield(void)
{
#if SOL >= 4
	int i, j;

	if (curenv)
		i = curenv-envs;
	else
		i = NENV-1;
	for (j=1; j<=NENV; j++) {
		if (j+i == NENV)
			continue;
		if (envs[(j+i)%NENV].env_status == ENV_RUNNABLE)
			env_run(&envs[(j+i)%NENV]);
	}
	assert(envs[0].env_status == ENV_RUNNABLE);
	env_run(&envs[0]);
#elif SOL >= 3
	int i, j;

	if (curenv)
		i = curenv-envs;
	else
		i = -1;
	for (j=1; j<=NENV; j++)
		if(envs[(j+i)%NENV].env_status == ENV_RUNNABLE)
			env_run(&envs[(j+i)%NENV]);
	panic("no runnable envs");
#else
	assert(envs[0].env_status == ENV_RUNNABLE);
	env_run(&envs[0]);
#endif // SOL 4, SOL 3
}

#endif
