#if LAB >= 6
#include "lib.h"

char buf[512], buf2[512];

int 
memcmp(char *a, char *b, int n)
{
	int i;

	for(i=0; i<n; i++)
		if(a[i] != b[i])
			return 1;
	return 0;
}

void
umain(void)
{
	int fd, r, n, n2;

	if ((fd = open("motd", O_RDONLY)) < 0)
		panic("open motd: %e", fd);
	seek(fd, 0);
	if ((n = readn(fd, buf, sizeof buf)) <= 0)
		panic("readn: %e", n);

	if ((r = fork()) < 0)
		panic("fork: %e", r);
	if (r == 0) {
		seek(fd, 0);
		printf("going to read in child (might page fault if your sharing is buggy)\n");
		if ((n2 = readn(fd, buf2, sizeof buf2)) != n2)
			panic("read in parent got %d, read in child got %d", n, n2);
		if(memcmp(buf, buf2, n) != 0)
			panic("read in parent got different bytes from read in child");
		printf("read in child succeeded\n");
		seek(fd, 0);
		close(fd);
		exit();
	}
	wait(r);
	if ((n2 = readn(fd, buf2, sizeof buf2)) != n)
		panic("read in parent got %d, then got %d", n, n2);
	printf("read in parent succeeded\n");		
}
#endif
