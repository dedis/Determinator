#if LAB >= 3

#include <inc/assert.h>

#include <kern/env.h>
#include <kern/pmap.h>


// Choose a user environment to run and run it.
void
sched_yield(void)
{
#if LAB >= 4
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
#else	// not SOL >= 4
	// Your code here to implement simple round-robin scheduling.
	// Search through envs array for a runnable environment,
	// in circular fashion starting from the previously running env,
	// and switch to the first such environment found.
	// But never choose envs[0], the idle environment,
	// unless NOTHING else is runnable.
#endif	// SOL >= 4

	// Run the special idle environment when nothing else is runnable.
	assert(envs[0].env_status == ENV_RUNNABLE);
	env_run(&envs[0]);
#else	// not LAB >= 4
	// We only have one user environment for now, so just run it.
	// Panic if it's not runnable!
	assert(envs[0].env_status == ENV_RUNNABLE);
	env_run(&envs[0]);
#endif	// not LAB >= 4
}

#endif	// LAB >= 3
