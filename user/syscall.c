#if LAB >= 4
// System call stubs.

#include "lib.h"
#include <inc/syscall.h>

static inline int
syscall(int num, u_int a1, u_int a2, u_int a3, u_int a4, u_int a5)
{
	int ret;

	// Generic system call: pass system call number in AX,
	// up to five parameters in DX, CX, BX, DI, SI.
	// Interrupt kernel with T_SYSCALL.
	//
	// The "volatile" tells the assembler not to optimize
	// this instruction away just because we don't use the
	// return value.
	// 
	// The last clause tells the assembler that this can
	// potentially change the condition codes and arbitrary
	// memory locations.

	asm volatile("int %1\n"
		: "=a" (ret)
		: "i" (T_SYSCALL),
		  "a" (num),
		  "d" (a1),
		  "c" (a2),
		  "b" (a3),
		  "D" (a4),
		  "S" (a5)
		: "cc", "memory");
	return ret;
}

void
sys_cputs(char *a1)
{
	syscall(SYS_cputs, (u_int) a1, 0, 0, 0, 0);
}

void
sys_yield(void)
{
	syscall(SYS_yield, 0, 0, 0, 0, 0);
}

int
sys_env_destroy(u_int envid)
{
	return syscall(SYS_env_destroy, envid, 0, 0, 0, 0);
}

u_int
sys_getenvid(void)
{
	 return syscall(SYS_getenvid, 0, 0, 0, 0, 0);
}

int
sys_ipc_can_send(u_int envid, u_int value, u_int srcva, u_int perm)
{
#if LAB >= 5
	return syscall(SYS_ipc_can_send, envid, value, srcva, perm, 0);
#else
	// Your code here.
	panic("sys_ipc_can_send not implemented");
#endif
}

void
sys_ipc_recv(u_int dstva)
{
#if LAB >= 5
	syscall(SYS_ipc_recv, dstva, 0, 0, 0, 0);
#else
	// Your code here.
	panic("sys_ipc_recv not implemented");
#endif
}

int
sys_set_pgfault_handler(u_int envid, u_int a1, u_int a2)
{
#if SOL >= 4
	return syscall(SYS_set_pgfault_handler, envid, a1, a2, 0, 0);
#else
	// Your code here.
	panic("sys_set_pgfault_handler not implemented");
#endif
}

int
sys_mem_alloc(u_int envid, u_int va, u_int perm)
{
#if SOL >= 4
	return syscall(SYS_mem_alloc, envid, va, perm, 0, 0);
#else
	// Your code here.
	panic("sys_mem_alloc not implemented");
#endif
}

int
sys_mem_map(u_int srcenv, u_int srcva, u_int dstenv, u_int dstva, u_int perm)
{
#if SOL >= 4
	return syscall(SYS_mem_map, srcenv, srcva, dstenv, dstva, perm);
#else
	// Your code here.
	panic("sys_mem_map not implemented");
#endif
}

int
sys_mem_unmap(u_int envid, u_int va)
{
#if SOL >= 4
	return syscall(SYS_mem_unmap, envid, va, 0, 0, 0);
#else
	// Your code here.
	panic("sys_mem_unmap not implemented");
#endif
}

// sys_env_alloc is inlined in lib.h

int
sys_set_env_status(u_int envid, u_int status)
{
#if SOL >= 4
	return syscall(SYS_set_env_status, envid, status, 0, 0, 0);
#else
	// Your code here.
	panic("sys_set_env_status not implemented");
#endif
}

#if LAB >= 5
int
sys_set_trapframe(u_int envid, struct Trapframe *tf)
{
	return syscall(SYS_set_trapframe, envid, (u_int)tf, 0, 0, 0);
}

void
sys_panic(char *msg)
{
	syscall(SYS_panic, (u_int)msg, 0, 0, 0, 0);
}

#endif
#if LAB >= 6
int
sys_cgetc(void)
{
	return syscall(SYS_cgetc, 0, 0, 0, 0, 0);
}

#endif
#endif
