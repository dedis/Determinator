#if LAB >= 3
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/picirq.h>
#include <kern/printf.h>

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
#if LAB >= 5
	// marks current position in the round-robin sweep
	static int sched_idx = 0;
	int start = sched_idx;

	do {
		sched_idx++;
		sched_idx %= NENV; 
		// skip the idle env
		if (sched_idx && envs[sched_idx].env_status == ENV_OK)
			env_run(&envs[sched_idx]);
		// we fall out of the loop when we've
		// checked all envs except the idle env
	} while (start != sched_idx);

	// idle env must always be runnable
#elif SOL >= 3
	int i, j;

	i = curenv-envs;
	for (j=1; j<=NENV; j++)
		if(envs[(j+i)%NENV].env_status == ENV_RUNNABLE)
			env_run(&envs[(j+i)%NENV]);
	panic("no runnable envs");
#else
	assert(envs[0].env_status == ENV_RUNNABLE);
	env_run(&envs[0]);
#endif /* SOL >= 3 */
}

#endif
