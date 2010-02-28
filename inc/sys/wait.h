#if LAB >= 4
// Unix compatibility API - process wait functions
#ifndef PIOS_INC_UNISTD_H
#define PIOS_INC_UNISTD_H 1

#include <inc/types.h>


pid_t	wait(int *status);
pid_t	waitpid(pid_t pid, int *status, int options);

#endif	// !PIOS_INC_UNISTD_H
#endif	// LAB >= 4
