#if LAB >= 5
#elif LAB >= 4
// Ping-pong a counter between two processes.
// Only need to start one of these -- splits into two, crudely.

#include <inc/string.h>
#include <inc/lib.h>

int dumbfork(void);

void
umain(void)
{
	u_int who, i;

	// fork a child process
	who = dumbfork();

	// print a message and yield to the other a few times
	for (i = 0; i < (who ? 10 : 20); i++) {
		printf("%d: I am the %s!\n", i, who ? "parent" : "child");
		sys_yield();
	}
}

void
duppage(u_int dstenv, u_int addr)
{
	int r;
	u_char *tmp;

	tmp = (u_char*)(UTEXT-PGSIZE);	// should be available!

	// This is NOT what you should do in your fork.
	if ((r=sys_page_alloc(dstenv, addr, PTE_P|PTE_U|PTE_W)) < 0)
		panic("sys_mem_alloc: %e", r);
	if ((r=sys_page_map(dstenv, addr, 0, tmp, PTE_P|PTE_U|PTE_W)) < 0)
		panic("sys_mem_map: %e", r);
	memcpy(tmp, (u_char*)addr, PGSIZE);
	if ((r=sys_page_unmap(0, (u_int)tmp)) < 0)
		panic("sys_mem_unmap: %e", r);
}

int
dumbfork(void)
{
	int addr, envid, r;
	extern u_char end[];

	// Allocate a new child environment.
	// The kernel will initialize it with a copy of our register state,
	// so that the child will appear to have called sys_env_alloc() too -
	// except that in the child, this "fake" call to sys_env_alloc()
	// will return 0 instead of the envid of the child.
	envid = sys_exofork();
	if (envid < 0)
		panic("sys_env_fork: %e", envid);
	if (envid == 0) {
		// We're the child.
		// The copied value of the global variable 'env'
		// is no longer valid (it refers to the parent!).
		// Fix it and return 0.
		env = &envs[ENVX(sys_getenvid())];
		return 0;
	}

	// We're the parent.
	// Eagerly copy our entire address space into the child.
	// This is NOT what you should do in your fork implementation.
	for (addr=UTEXT; addr<(u_int)end; addr+=PGSIZE)
		duppage(envid, addr);

	// Also copy the stack we are currently running on.
	duppage(envid, ROUNDDOWN((u_int)&addr, PGSIZE));

	// Start the child environment running
	// (at the point above where the register state was copied).
	if ((r=sys_env_set_status(envid, ENV_RUNNABLE)) < 0)
		panic("sys_set_status: %e", r);

	return envid;
}

#endif
