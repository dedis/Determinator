#if LAB >= 6
#include "lib.h"

void
umain(void)
{
	char buf[100];
	int i, p[2];

	if ((i=pipe(p)) < 0)
		panic("pipe: %e", i);

	if ((i=fork()) < 0)
		panic("fork: %e", i);

	if (i == 0) {
		printf("[%08x] pipereadeof readpipe\n", env->env_id);
		close(p[1]);
		i = readn(p[0], buf, sizeof buf-1);
		buf[i] = 0;
		if (strcmp(buf, "hello, world\n") == 0)
			printf("\npipe read closed\n");
	} else {
		close(p[0]);
		write(p[1], "hello, world\n", 13);
	}
}
#endif
