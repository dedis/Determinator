#if LAB >= 4
// implement fork from user space

#include "lib.h"

#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//

static void
pgfault(u_int va, u_int err)
{
	int r;
	u_char *tmp;

#if SOL >= 4
	if ((vpt[PPN(va)] & (PTE_P|PTE_U|PTE_W|PTE_COW)) != (PTE_P|PTE_U|PTE_COW))
		panic("fault at %x with pte %x, not copy-on-write", va, vpt[PPN(va)]);

	tmp = (u_char*)(UTEXT-BY2PG);	// should be available!

	// copy page
	if ((r=sys_mem_alloc(0, (u_int)tmp, PTE_P|PTE_U|PTE_W)) < 0)
		panic("sys_mem_alloc: %e", r);
	bcopy((u_char*)ROUNDDOWN(va, BY2PG), tmp, BY2PG);

	// remap over faulting page
	if ((r=sys_mem_map(0, (u_int)tmp, 0, va, PTE_P|PTE_U|PTE_W)) < 0)
		panic("sys_mem_map: %e", r);

	// unmap our work space
	if ((r=sys_mem_unmap(0, (u_int)tmp)) < 0)
		panic("sys_mem_unmap: %e", r);
#endif
}

//
// Map our virtual page pn (address pn*BY2PG) into the target envid
// at the same virtual address.  if the page is writable or copy-on-write,
// the new mapping must be created copy on write and then our mapping must be
// marked copy on write as well.  (Exercise: why mark ours copy-on-write again if
// it was already copy-on-write?)
// 
static void
duppage(u_int envid, u_int pn)
{
	int r;
	u_int addr;
	Pte pte;

#if SOL >= 4
	addr = pn<<PGSHIFT;
	pte = vpt[pn];

	// if the page is just read-only, just map it in.
	if ((pte&(PTE_W|PTE_COW)) == 0) {
		if ((r=sys_mem_map(0, addr, envid, addr, pte&PTE_FLAGS)) < 0)
			panic("sys_mem_map: %e", r);
		return;
	}

	// The order is VERY important here -- if we swap these and fault on addr
	// between them, then the child will see all the updates we make
	// to that page until it copies it.
	//
	// Probably this could only occur for the current stack page,
	// but if we managed to return from fork before the other env
	// got a chance to run, the stack frame would be lost and bad
	// things would happen when the child's fork returned.
	//
	// Even if we think the page is already copy-on-write in our
	// address space, we need to mark it copy-on-write again after
	// the first sys_mem_map, just in case a page fault has caused
	// us to copy the page in the interim.

	if ((r=sys_mem_map(0, addr, envid, addr, PTE_P|PTE_U|PTE_COW)) < 0)
		panic("sys_mem_map: %e", r);
	if ((r=sys_mem_map(0, addr, 0, addr, PTE_P|PTE_U|PTE_COW)) < 0)
		panic("sys_mem_map: %e", r);
#endif
}

//
// User-level fork.  Create a child and then copy our address space
// and page fault handler setup to the child.
//
// Hint: use vpd, vpt, and duppage.
// Hint: remember to fix "env" in the child process!
// 
int
fork(void)
{
#if SOL >= 4
	int envid, i, j, pn, addr, r;

	set_pgfault_handler(pgfault);

	// Create a child.
	envid = sys_env_alloc();
	if (envid < 0)
		return envid;
	if (envid == 0) {
		env = &envs[sys_getenvid()];
		return 0;
	}

	// Copy the address space.
	for (i=0; i<PDX(UTOP); i++) {
		if ((vpd[i]&PTE_P) == 0)
			continue;
		for (j=0; j<PTE2PT; j++) {
			pn = i*PTE2PT+j;
			if ((vpt[pn]&(PTE_P|PTE_U)) != (PTE_P|PTE_U))
				continue;
			if (pn == VPN(UXSTACKTOP-1))
				continue;
			duppage(envid, pn);
		}
	}

	// Copy the exception handler.
	if ((r=sys_mem_alloc(envid, env->env_xstacktop-1, PTE_P|PTE_U|PTE_W)) < 0)
		panic("allocating exception stack: %e", r);
	if ((r=sys_set_pgfault_handler(envid, env->env_pgfault_handler, env->env_xstacktop)) < 0)
		panic("set_pgfault_handler: %e", r);


	// Okay, the child is ready for life on its own.
	if ((r=sys_set_env_status(envid, ENV_RUNNABLE)) < 0)
		panic("sys_set_env_status: %e", r);

	return envid;
#endif
}

// Challenge!
int
sfork(void)
{
	return -E_INVAL;
}
#endif
