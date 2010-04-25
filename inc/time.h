#ifndef PIOS_INC_TIME_H
#define PIOS_INC_TIME_H

#include <inc/types.h>

struct timeval {
	time_t		tv_sec;		// Seconds
	suseconds_t	tv_usec;	// Microseconds;
};

int gettimeofday(struct timeval *tv, void *tzp);

#endif	// PIOS_INC_TIME_H
