#if LAB >= 4
#include <inc/assert.h>

#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>


// Choose a user environment to run and run it.
void
sched_yield(void)
{
#if SOL >= 4
	int i, j;

	// Determine the starting point for the search.
	if (curenv)
		i = curenv-envs;
	else
		i = NENV-1;
	//cprintf("sched_yield searching from %d\n", i);

	// Loop through all the environments at most once.
	for (j = 1; j <= NENV; j++) {

		// Don't pick the idle environment.
		if (j + i == NENV)
			continue;

		// If this environment is runnable, run it.
		if (envs[(j+i) % NENV].env_status == ENV_RUNNABLE) {
			//cprintf("sched_yield picked %d\n", (j+i)%NENV);
			env_run(&envs[(j+i) % NENV]);
		}
	}
#else	// not SOL >= 4
	// Implement simple round-robin scheduling.
	// Search through 'envs' for a runnable environment,
	// in circular fashion starting after the previously running env,
	// and switch to the first such environment found.
	// It's OK to choose the previously running env if no other env
	// is runnable.
	// But never choose envs[0], the idle environment,
	// unless NOTHING else is runnable.

	// LAB 4: Your code here.
#endif	// SOL >= 4

	// Run the special idle environment when nothing else is runnable.
	if (envs[0].env_status == ENV_RUNNABLE)
		env_run(&envs[0]);
	else {
		cprintf("Destroyed all environments - nothing more to do!\n");
		while (1)
			monitor(NULL);
	}
}
#endif	// LAB >= 4
