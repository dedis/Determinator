#if LAB >= 3
#ifndef JOS_INC_SYSCALL_H
#define JOS_INC_SYSCALL_H

/* system call numbers */
enum
{
	SYS_cputs = 0,
	SYS_cgetc,
	SYS_getenvid,
	SYS_env_destroy,
#if LAB >= 4
	SYS_page_alloc,
	SYS_page_map,
	SYS_page_unmap,
	SYS_exofork,
	SYS_env_set_status,
	SYS_env_set_trapframe,
	SYS_env_set_pgfault_upcall,
	SYS_yield,
	SYS_ipc_try_send,
	SYS_ipc_recv,
#endif	// LAB >= 4
#if LAB >= 6
	SYS_time_msec,
#if SOL >= 6
	SYS_net_txbuf,
#endif  // SOL >= 6
#endif	// LAB >= 6
	NSYSCALLS
};

#endif /* !JOS_INC_SYSCALL_H */
#endif /* LAB >= 3 */
