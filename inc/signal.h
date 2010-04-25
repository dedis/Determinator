#ifndef PIOS_INC_SIGNAL_H
#define PIOS_INC_SIGNAL_H

#include <types.h>

#define SIGHUP		1
#define SIGINT		2
#define SIGQUIT		3
#define SIGILL		4
#define SIGTRAP		5
#define SIGABRT		6
#define SIGFPE		8
#define SIGKILL		9
#define SIGBUS		10
#define SIGSEGV		11

typedef void (*sighandler_t)(int);

sighandler_t signal(int sig, sighandler_t newhandler);

#endif /* not PIOS_INC_SIGNAL_H */
