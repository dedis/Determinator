#if LAB >= 4
// Ping-pong a counter between two processes.
// Only need to start one of these -- splits into two, crudely.

#include "lib.h"

int dumbfork(void);

void
umain(void)
{
	u_int who, i;

	if ((who = dumbfork()) != 0) {
		// get the ball rolling
		printf("send 0 from %x to %x\n", sys_getenvid(), who);
		ipc_send(who, 0, 0, 0);
	}

	for (;;) {
		i = ipc_recv(&who, 0, 0);
		printf("%x got %d from %x\n", sys_getenvid(), i, who);
		if (i == 10)
			return;
		i++;
		ipc_send(who, i, 0, 0);
		if (i == 10)
			return;
	}
		
}

void
duppage(u_int dstenv, u_int addr)
{
	int r;
	u_char *tmp;

	tmp = (u_char*)(UTEXT-BY2PG);	// should be available!

	// This is NOT what you should do in your fork.
	if ((r=sys_mem_alloc(dstenv, addr, PTE_P|PTE_U|PTE_W)) < 0)
		panic("sys_mem_alloc: %e", r);
	if ((r=sys_mem_map(dstenv, addr, 0, (u_int)tmp, PTE_P|PTE_U|PTE_W)) < 0)
		panic("sys_mem_map: %e", r);
	bcopy((u_char*)addr, tmp, BY2PG);
	if ((r=sys_mem_unmap(0, (u_int)tmp)) < 0)
		panic("sys_mem_unmap: %e", r);
}

int
dumbfork(void)
{
	int addr, envid, r;
	extern u_char end[];

	envid = sys_env_alloc();
	if (envid < 0)
		panic("sys_env_fork: %e", envid);
	if (envid == 0) {
		env = &envs[ENVX(sys_getenvid())];
		return 0;
	}
	
	// This is NOT what you should do in your fork.
	for (addr=UTEXT; addr<(u_int)end; addr+=BY2PG)
		duppage(envid, addr);
	duppage(envid, ROUNDDOWN((u_int)&addr, BY2PG));
	if ((r=sys_set_env_status(envid, ENV_RUNNABLE)) < 0)
		panic("sys_set_env_status: %e", r);
	return envid;
}

#endif
