// System call stubs.

#include "lib.h"
#include <inc/syscall.h>

static inline int
syscall(int num, u_int a1, u_int a2, u_int a3, u_int a4, u_int a5)
{
	int ret;

	// Generic system call: pass
	// system call number in AX,
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
sys_cputu(u_int a1)
{
	syscall(SYS_cputu, a1, 0, 0, 0, 0);
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

void
sys_env_destroy(void)
{
	syscall(SYS_env_destroy, 0, 0, 0, 0, 0);
}

int
sys_set_env_status(u_int env, u_int status)
{
	return syscall(SYS_set_env_status, env, status, 0, 0, 0);
}

u_int
sys_getenvid(void)
{
	 return syscall(SYS_getenvid, 0, 0, 0, 0, 0);
}

// sys_env_alloc is inlined in lib.h

#if SOL >= 4
int
sys_ipc_can_send(u_int a1, u_int a2)
{
	return syscall(SYS_ipc_can_send, a1, a2, 0, 0, 0);
}

void
sys_ipc_recv(void)
{
	syscall(SYS_ipc_recv, 0, 0, 0, 0, 0);
}

int
sys_set_pgfault_handler(u_int envid, u_int a1, u_int a2)
{
	return syscall(SYS_set_pgfault_handler, envid, a1, a2, 0, 0);
}

int
sys_mem_alloc(u_int envid, u_int va, u_int perm)
{
	return syscall(SYS_mem_alloc, envid, va, perm, 0, 0);
}

int
sys_mem_map(u_int srcenv, u_int srcva, u_int dstenv, u_int dstva, u_int perm)
{
	return syscall(SYS_mem_map, srcenv, srcva, dstenv, dstva, perm);
}

int
sys_mem_unmap(u_int env, u_int va)
{
	return syscall(SYS_mem_unmap, env, va, 0, 0, 0);
}

#else

// Your code here: define 
//
//	sys_ipc_can_send
//	sys_ipc_recv
//	sys_set_pgfault_handler
//	sys_mem_alloc
//	sys_mem_map
//	sys_mem_unmap
//
// See lib.h for prototypes.

#endif

