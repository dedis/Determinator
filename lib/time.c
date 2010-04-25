#if LAB >= 9

#include <inc/syscall.h>
#include <inc/time.h>


int gettimeofday(struct timeval *tv, struct timezone *tzp)
{
	uint64_t t = sys_time() / 1000;		// get time in microseconds
	tv->tv_sec = t / 1000000;
	tv->tv_usec = t % 1000000;
	return 0;
}

#endif	// LAB >= 9
