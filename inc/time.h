#if LAB >= 9
#ifndef PIOS_INC_TIME_H
#define PIOS_INC_TIME_H

#include <types.h>

struct timeval {
	time_t		tv_sec;		// Seconds
	suseconds_t	tv_usec;	// Microseconds;
};

struct tm {
	int		tm_sec;		// Seconds after the minute (0-60)
	int		tm_min;		// Minutes after the hour (0-59)
	int		tm_hour;	// Hours since midnight (0-23)
	int		tm_mday;	// Day of month (1-31)
	int		tm_mon;		// Months since January (0-11)
	int		tm_year;	// Years since 1900
	int		tm_wday;	// Days since Sunday (0-6)
	int		tm_yday;	// Days since January 1 (0-365)
	int		tm_isdst;	// Daylight Savings time flag
};

struct timezone {
	int		tz_minuteswest;	// Minutes west of GMT
	int		tz_dsttime;	// Type of daylight savings correction
};

// System time
int	gettimeofday(struct timeval *tv, struct timezone *tzp);
int	settimeofday(const struct timeval *tv, const struct timezone *tzp);

// Time conversion functions
struct tm *	gmtime(const time_t *);
struct tm *	localtime(const time_t *);
time_t		mktime(struct tm *);
char *		asctime(const struct tm *);

#endif	// PIOS_INC_TIME_H
#endif	// LAB >= 9
