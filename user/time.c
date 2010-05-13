#if LAB >= 9
#include <inc/stdio.h>
#include <inc/stdlib.h>
#include <inc/string.h>
#include <inc/unistd.h>
#include <inc/time.h>

int
main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "Usage: time <command>\n");
		exit(1);
	}

	struct timeval starttime, endtime;
	if (gettimeofday(&starttime, NULL) < 0)
		perror("time: gettimeofday");
	pid_t pid = fork();
	if (pid < 0)
		perror("time: fork");
	if (pid == 0) {	// in the child
		if (execv(argv[1], &argv[1]) < 0)
			perror("time: exec");
		abort();
	}
	if (waitpid(pid, NULL, 0) < 0)
		perror("time: wait");
	if (gettimeofday(&endtime, NULL) < 0)
		perror("time: gettimeofday");

	double startsecs = (double)starttime.tv_sec
			+ (double)starttime.tv_usec / 1000000.0;
	double endsecs = (double)endtime.tv_sec
			+ (double)endtime.tv_usec / 1000000.0;
	printf("real %10.3fs\n", endsecs - startsecs);

	return 0;
}

#endif	// LAB >= 9
