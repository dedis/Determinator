#if LAB >= 2
#ifndef PIOS_INC_SYSCALL_H
#define PIOS_INC_SYSCALL_H

/* system call numbers */
enum
{
	SYS_cputs = 0,
	SYS_cgetc,
	SYS_fork,
	SYS_wait,
	SYS_exit,
#if LAB >= 3
	SYS_map,
#endif	// LAB >= 3
	NSYSCALLS
};

#endif /* !PIOS_INC_SYSCALL_H */
#endif /* LAB >= 2 */
