#if LAB >= 3
// System call stubs.

#include <inc/syscall.h>
#include <inc/lib.h>

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

int
sys_cgetc(void)
{
	return syscall(SYS_cgetc, 0, 0, 0, 0, 0);
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

#if SOL >= 4
void
sys_yield(void)
{
	syscall(SYS_yield, 0, 0, 0, 0, 0);
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
sys_mem_unmap(u_int envid, u_int va)
{
	return syscall(SYS_mem_unmap, envid, va, 0, 0, 0);
}

// sys_env_alloc is inlined in lib.h

int
sys_set_trapframe(u_int envid, struct Trapframe *tf)
{
	return syscall(SYS_set_trapframe, envid, (u_int)tf, 0, 0, 0);
}

int
sys_set_status(u_int envid, u_int status)
{
	return syscall(SYS_set_status, envid, status, 0, 0, 0);
}

int
sys_set_pgfault_entry(u_int envid, u_int a1)
{
	return syscall(SYS_set_pgfault_entry, envid, a1, 0, 0, 0);
}

int
sys_ipc_can_send(u_int envid, u_int value, u_int srcva, u_int perm)
{
	return syscall(SYS_ipc_can_send, envid, value, srcva, perm, 0);
}

void
sys_ipc_recv(u_int dstva)
{
	syscall(SYS_ipc_recv, dstva, 0, 0, 0, 0);
}
#endif	// SOL >= 4

#endif	// LAB >= 3
