#if LAB >= 6

#include <inc/lib.h>

char buf[1024];

void
umain(void)
{
	int r;

	close(0);
	if ((r = opencons()) < 0)
		panic("opencons: %e", r);
	if (r != 0)
		panic("first opencons used fd %d", r);
	if ((r = dup(0, 1)) < 0)
		panic("dup: %e", r);

	for(;;){
		printf("Type a line: ");
		readline(buf, sizeof buf);
		fprintf(1, "%s\n", buf);
	}
}
#endif
