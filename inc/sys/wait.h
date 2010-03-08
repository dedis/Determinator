#if LAB >= 4
// Unix compatibility API - process wait functions
#ifndef PIOS_INC_SYS_WAIT_H
#define PIOS_INC_SYS_WAIT_H 1

#include <inc/types.h>


// Exit status codes from wait()
#define WEXITED			0x100	// Process exited via exit()
#define WSIGNALED		0x200	// Process exited via uncaught signal

#define WEXITSTATUS(x)		((x) & 0xff)
#define WTERMSIG(x)		((x) & 0xff)
#define WIFEXITED(x)		(((x) & 0xf00) == WEXITED)
#define WIFSIGNALED(x)		(((x) & 0xf00) == WSIGNALED)


pid_t	wait(int *status);
pid_t	waitpid(pid_t pid, int *status, int options);

#endif	// !PIOS_INC_SYS_WAIT_H
#endif	// LAB >= 4
