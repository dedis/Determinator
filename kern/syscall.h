///LAB3

#ifndef _KERN_SYSCALL_H_
#define _KERN_SYSCALL_H_

#include <inc/syscall.h>

// These prototypes are private to the kernel.

///LAB200
#if 0
///END
int dispatch_syscall (u_int, u_int, u_int, u_int);
///LAB200
#endif
int dispatch_syscall (u_int, u_int, u_int, u_int, u_int);
///END
u_int sys_getenvid ();
void sys_cputu (u_int);
void sys_cputs (char *);
void sys_yield ();
void sys_env_destroy ();
int sys_env_alloc (u_int, u_int);
int sys_ipc_send (u_int, u_int);
void sys_ipc_unblock ();
void sys_set_pgfault_handler (u_int, u_int);
int sys_mod_perms (u_int, u_int, u_int);
int sys_set_env_status (u_int, u_int);
int sys_mem_unmap (u_int, u_int);
int sys_mem_alloc (u_int, u_int, u_int);
int sys_mem_remap (u_int, u_int, u_int, u_int);
int sys_disk_read (u_int, u_int, u_int);


#endif /* !_KERN_SYSCALL_H_ */
///END
