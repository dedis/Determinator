#if LAB >= 6

#include <inc/lib.h>

void
umain(void)
{
	int p[2], r, i;
	struct Fd *fd;
	struct Env *kid;

	printf("testing for pipeisclosed race...\n");
	if ((r = pipe(p)) < 0)
		panic("pipe: %e", r);
	if ((r = fork()) < 0)
		panic("fork: %e", r);
	if (r == 0) {
		// child just dups and closes repeatedly, yielding so we can see
		// the fd state between the two.
		// p[1] is still open here -- the pipe is definitely open!
		for (i=0; i<200; i++) {
			if (i%10 == 0)
				printf("%d.", i);
			// dup, then close.  yield so that other guy will
			// see us while we're between them.
			dup(p[0], 10);
			sys_yield();
			close(10);
			sys_yield();
		}
		exit();
	}

	//
	// Now the ref count for p[0] will toggle between 2 and 3
	// as the child dups and closes it.
	// The ref count for p[1] is 1.
	// Thus the ref count for the underlying pipe structure 
	// will toggle between 3 and 4.
	//
	// If pipeisclosed checks pageref(p[0]) and gets 3, and
	// then the child closes, and then pipeisclosed checks
	// pageref(pipe structure) and gets 3, then we'll get a 1
	// when we shouldn't.
	//
	// If pipeisclosed checks pageref(pipe structure) and gets 3,
	// and then the child dups, and then pipeisclosed checks
	// pageref(p[0]) and gets 3, then we'll get a 1 when we
	// shouldn't.
	//
	// So either way, pipeisclosed is going give a wrong answer.
	//
	kid = &envs[ENVX(r)];
	while (kid->env_status == ENV_RUNNABLE)
		if (pipeisclosed(p[0]) != 0) {
			printf("\nRACE: pipe appears closed\n");
			exit();
		}
	printf("child done with loop\n");
	if (pipeisclosed(p[0]))
		panic("somehow the other end of p[0] got closed!");
	if ((r = fd_lookup(p[0], &fd)) < 0)
		panic("cannot look up p[0]: %e", r);
	(void) fd2data(fd);
	printf("\nrace didn't happen\n");
}
#endif
