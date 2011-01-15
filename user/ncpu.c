#if LAB >= 9

#include <inc/stdio.h>
#include <inc/stdlib.h>
#include <inc/syscall.h>

int main(int argc, char **argv)
{
	int newlim;
	if (argc != 2 || (newlim = atoi(argv[1])) <= 0) {
		fprintf(stderr, "usage: ncpu <count>\n");
		exit(1);
	}
	sys_ncpu(newlim);
	return 0;
}

#endif	// LAB >= 9
