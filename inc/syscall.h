#if LAB >= 3
#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include <inc/types.h>

/* system call numbers */
enum
{
	SYS_cputs	= 0,
	SYS_cgetc,
	SYS_getenvid,
	SYS_env_destroy,
#if LAB >= 4
	SYS_yield,
	SYS_mem_alloc,
	SYS_mem_map,
	SYS_mem_unmap,
	SYS_env_alloc,
	SYS_set_trapframe,
	SYS_set_status,
	SYS_set_pgfault_entry,
	SYS_ipc_can_send,
	SYS_ipc_recv,
#endif	// LAB >= 4

	NSYSCALLS,
};

#endif /* !_SYSCALL_H_ */
#endif /* LAB >= 3 */
