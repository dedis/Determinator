#if LAB >= 4
#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include <inc/types.h>
#include <kern/trap.h>

/* system call numbers */
enum
{
	SYS_getenvid	= 0,
	SYS_cputs,
	SYS_yield,
	SYS_env_destroy,
	SYS_env_alloc,
	SYS_ipc_can_send,
	SYS_ipc_recv,
	SYS_set_pgfault_handler,
	SYS_set_env_status,
	SYS_mem_alloc,
	SYS_mem_map,
	SYS_mem_unmap,
	SYS_set_trapframe,
	SYS_panic,

	NSYSCALLS,
};

#endif /* !_SYSCALL_H_ */
#endif /* LAB >= 4 */
