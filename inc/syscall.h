///LAB3

#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include <inc/types.h>
#include <kern/trap.h>

/* system call numbers */
#define SYS_getenvid              0
#define SYS_cputu                 1
#define SYS_cputs                 2
#define SYS_yield                 3
#define SYS_env_destroy           4
#define SYS_env_alloc             5
#define SYS_ipc_send              6
#define SYS_ipc_unblock           7
#define SYS_set_pgfault_handler   8
#define SYS_mod_perms             9
#define SYS_set_env_status       10

#define MAX_SYSCALL SYS_set_env_status


#ifdef INLINE_SYSCALLS

// These functions are seen by user processes.

// If you don't know inline assembly, read the 2nd half of:
//   http://www.cs.princeton.edu/courses/archive/fall99/cs318/Files/djgpp.html

static inline int
sys_call(int sn, u_int a1, u_int a2, u_int a3)
{
	int ret;

	__asm __volatile("int %5\n"
		: "=a" (ret)									: "a" (sn), "b" (a3), "c" (a2), "d" (a1), "i" (T_SYSCALL)
		: "cc", "memory");
	return(ret);
}

static inline u_int
sys_getenvid(void)
{
	 return sys_call(SYS_getenvid, 0, 0, 0);
}

static inline void
sys_cputu(u_int a1)
{
	sys_call(SYS_cputu, a1, 0, 0);
}

static inline void
sys_cputs(char *a1)
{
	sys_call(SYS_cputs, (u_int) a1, 0, 0);
}

static inline void
sys_yield(void)
{
	sys_call(SYS_yield, 0, 0, 0);
}

static inline void
sys_env_destroy(void)
{
	sys_call(SYS_env_destroy, 0, 0, 0);
}

static inline int
sys_env_alloc(u_int a1, u_int a2)
{
	return sys_call(SYS_env_alloc, a1, a2, 0);
}


static inline int
sys_ipc_send(u_int a1, u_int a2)
{
	return sys_call(SYS_ipc_send, a1, a2, 0);
}

static inline void
sys_ipc_unblock(void)
{
	sys_call(SYS_ipc_unblock, 0, 0, 0);
}

static inline void
sys_set_pgfault_handler(u_int a1, u_int a2)
{
	sys_call(SYS_set_pgfault_handler, a1, a2, 0);
}

#endif /* INLINE_SYSCALLS */
#endif /* !_SYSCALL_H_ */
///END
