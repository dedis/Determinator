#if LAB >= 4
// User-level IPC library routines

#include <inc/lib.h>

// Send val to whom.  This function keeps trying until
// it succeeds.  It should panic() on any error other than
// -E_IPC_NOT_RECV.  
//
// Hint: use sys_yield() to be CPU-friendly.
void
ipc_send(envid_t whom, uint32_t val, void *srcva, int perm)
{
#if SOL >= 4
	int r;

	if (!srcva)
		srcva = (void*) UTOP;
	while ((r=sys_ipc_try_send(whom, val, srcva, perm)) == -E_IPC_NOT_RECV)
		sys_yield();
	if(r == 0)
		return;
	panic("error in ipc_send: %e", r);
#else
	// Your code here.
	panic("ipc_send not implemented");
#endif
}

// Receive a value.  Return the value and store the caller's envid
// in *whom.  
//
// Hint: use env to discover the value and who sent it.
uint32_t
ipc_recv(envid_t *whom, void *dstva, int *perm)
{
#if SOL >= 4
	if (!dstva)
		dstva = (void*) UTOP;
	sys_ipc_recv(dstva);
	if (whom)
		*whom = env->env_ipc_from;
	if (perm)
		*perm = env->env_ipc_perm;
	return env->env_ipc_value;
#else
	// Your code here
	panic("ipc_recv not implemented");
	return 0;
#endif
}

#endif
