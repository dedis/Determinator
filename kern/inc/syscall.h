///BEGIN 3

#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include <kern/inc/types.h>
#include <kern/inc/trap.h>

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
///BEGIN 200
#if 0
///END

#define MAX_SYSCALL SYS_set_env_status
///BEGIN 200
#endif

#define SYS_mem_unmap            11
#define SYS_mem_alloc            12
#define SYS_mem_remap            13
#define SYS_disk_read            14

#define MAX_SYSCALL SYS_disk_read
///END

/* system call error values */
#define E_UNSPECIFIED	1	/* Unspecified or unknown problem */
#define E_BAD_ENV       2       /* Environment doesn't exist or otherwise
				   cannot be used in requested action */
#define E_INVAL		3	/* Invalid parameter */
#define E_NO_MEM	4	/* Request failed due to memory shortage */
#define E_NO_FREE_ENV   5       /* Attempt to create a new environment beyond
				   the maximum allowed */
#define E_IPC_BLOCKED   6       /* Attempt to ipc to env blocking ipc's */

#ifdef KERNEL
// These prototypes are seen by the kernel.


///BEGIN 200
#if 0
///END
int dispatch_syscall (u_int, u_int, u_int, u_int);
///BEGIN 200
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


#else

// These functions are seen by user processes.

// If you don't know inline assembly, read the 2nd half of:
//   http://www.cs.princeton.edu/courses/archive/fall99/cs318/Files/djgpp.html

///BEGIN 200
#if 0
///END
static inline int
sys_call (int sn, u_int a1, u_int a2, u_int a3)
{
  int ret;


  __asm __volatile ("int %5\n"
                    : "=a" (ret)
		    : "a" (sn), "b" (a3), "c" (a2), "d" (a1),
		    "i" (T_SYSCALL)
		    : "cc", "memory");
  return (ret);
}
///BEGIN 200
#endif


static inline int
sys_call (int sn, u_int a1, u_int a2, u_int a3, u_int a4)
{
  int ret;
  // XXX isn't "cc" unnecessary? The kernel restores the eflags.
  __asm __volatile ("int %6\n"
                    : "=a" (ret)
		    : "a" (sn), "S" (a4), "b" (a3), "c" (a2), "d" (a1),
		    "i" (T_SYSCALL)
		    : "cc", "memory");
  return (ret);
}
///END

static inline u_int
sys_getenvid (void)
{
///BEGIN 200
#if 0
///END
   return sys_call (SYS_getenvid, 0, 0, 0);
///BEGIN 200
#endif
  return sys_call (SYS_getenvid, 0, 0, 0, 0);
///END
}

static inline void
sys_cputu (u_int a1)
{
///BEGIN 200
#if 0
///END
  (void) sys_call (SYS_cputu, a1, 0, 0);
///BEGIN 200
#endif
  (void) sys_call (SYS_cputu, a1, 0, 0, 0);
///END
}

static inline void
sys_cputs (char *a1)
{
///BEGIN 200
#if 0
///END
  (void) sys_call (SYS_cputs, (u_int) a1, 0, 0);
///BEGIN 200
#endif
  (void) sys_call (SYS_cputs, (u_int) a1, 0, 0, 0);
///END
}

static inline void
sys_yield (void)
{
///BEGIN 200
#if 0
///END
  (void) sys_call (SYS_yield, 0, 0, 0);
///BEGIN 200
#endif
  (void) sys_call (SYS_yield, 0, 0, 0, 0);
///END
}

static inline void
sys_env_destroy (void)
{
///BEGIN 200
#if 0
///END
  (void) sys_call (SYS_env_destroy, 0, 0, 0);
///BEGIN 200
#endif
  (void) sys_call (SYS_env_destroy, 0, 0, 0, 0);
///END
}

static inline int
sys_env_alloc (u_int a1, u_int a2)
{
///BEGIN 200
#if 0
///END
  return sys_call (SYS_env_alloc, a1, a2, 0);
///BEGIN 200
#endif
  return sys_call (SYS_env_alloc, a1, a2, 0, 0);
///END
}


static inline int
sys_ipc_send (u_int a1, u_int a2)
{
///BEGIN 200
#if 0
///END
  return sys_call (SYS_ipc_send, a1, a2, 0);
///BEGIN 200
#endif
  return sys_call (SYS_ipc_send, a1, a2, 0, 0);
///END
}

static inline void
sys_ipc_unblock (void)
{
///BEGIN 200
#if 0
///END
  (void) sys_call (SYS_ipc_unblock, 0, 0, 0);
///BEGIN 200
#endif
  (void) sys_call (SYS_ipc_unblock, 0, 0, 0, 0);
///END
}

static inline void
sys_set_pgfault_handler (u_int a1, u_int a2)
{
///BEGIN 200
#if 0
///END
  (void) sys_call (SYS_set_pgfault_handler, a1, a2, 0);
///BEGIN 200
#endif
  (void) sys_call (SYS_set_pgfault_handler, a1, a2, 0, 0);
///END
}

///BEGIN 200
static inline int
sys_mod_perms (u_int a1, u_int a2, u_int a3)
{
  return sys_call (SYS_mod_perms, a1, a2, a3, 0);
}


static inline int
sys_set_env_status (u_int a1, u_int a2)
{
  return sys_call (SYS_set_env_status, a1, a2, 0, 0);
}

static inline int
sys_mem_unmap (u_int a1, u_int a2)
{
  return sys_call (SYS_mem_unmap, a1, a2, 0, 0);
}

static inline int
sys_mem_alloc (u_int a1, u_int a2, u_int a3)
{
  return sys_call (SYS_mem_alloc, a1, a2, a3, 0);
}

static inline int
sys_mem_remap (u_int a1, u_int a2, u_int a3, u_int a4)
{
  return sys_call (SYS_mem_remap, a1, a2, a3, a4);
}

static inline int
sys_disk_read (u_int a1, u_int a2, u_int a3)
{
  return sys_call (SYS_disk_read, a1, a2, a3, 0);
}

///END


#endif /* KERNEL */

#endif /* !_SYSCALL_H_ */
///END
