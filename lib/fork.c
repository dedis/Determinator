#if LAB >= 4
// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>
#if SOL >= 4

#define debug 0
#endif

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

#if SOL >= 4
	if (debug)
		cprintf("fault %08x %08x %d from %08x\n", addr, &vpt[VPN(addr)], err & 7, (&addr)[4]);

	if ((vpt[VPN(addr)] & (PTE_P|PTE_U|PTE_W|PTE_COW)) != (PTE_P|PTE_U|PTE_COW))
		panic("fault at %x with pte %x from %08x, not copy-on-write",
			addr, vpt[PPN(addr)], (&addr)[4]);
#else
	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at vpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
#endif

#if SOL >= 4
 	// copy page
	if ((r = sys_page_alloc(0, (void*) PFTEMP, PTE_P|PTE_U|PTE_W)) < 0)
		panic("sys_page_alloc: %e", r);
	memmove((void*) PFTEMP, ROUNDDOWN(addr, PGSIZE), PGSIZE);

	// remap over faulting page
	if ((r = sys_page_map(0, (void*) PFTEMP, 0, addr, PTE_P|PTE_U|PTE_W)) < 0)
		panic("sys_page_map: %e", r);

	// unmap our work space
	if ((r = sys_page_unmap(0, (void*) PFTEMP)) < 0)
		panic("sys_page_unmap: %e", r);
#else
	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	//   No need to explicitly delete the old page's mapping.

	// LAB 4: Your code here.

	panic("pgfault not implemented");
#endif
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why might we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
// 
static int
duppage(envid_t envid, unsigned pn)
{
	int r;

#if SOL >= 4
	void *addr;
	pte_t pte;

	addr = (void*) (pn << PGSHIFT);
	pte = vpt[pn];

#if SOL >= 6
	// if the page is just read-only or is library-shared, map it directly.
	if (!(pte & (PTE_W|PTE_COW)) || (pte & PTE_SHARE)) {
		if ((r = sys_page_map(0, addr, envid, addr, pte & PTE_USER)) < 0)
			panic("sys_page_map: %e", r);
		return 0;
	}
#else
	// if the page is just read-only, just map it in.
	if (!(pte & (PTE_W|PTE_COW))) {
		if ((r = sys_page_map(0, addr, envid, addr, pte & PTE_USER)) < 0)
			panic("sys_page_map: %e", r);
		return 0;
	}
#endif

	// The order is VERY important here -- if we swap these and fault on
	// addr between them, then the child will see all the updates we make
	// to that page until it copies it.
	//
	// Probably this could only occur for the current stack page,
	// but if we managed to return from fork before the other env
	// got a chance to run, the stack frame would be lost and bad
	// things would happen when the child's fork returned.
	//
	// Even if we think the page is already copy-on-write in our
	// address space, we need to mark it copy-on-write again after
	// the first sys_page_map, just in case a page fault has caused
	// us to copy the page in the interim.

	if ((r = sys_page_map(0, addr, envid, addr, PTE_P|PTE_U|PTE_COW)) < 0)
		panic("sys_page_map: %e", r);
	if ((r = sys_page_map(0, addr, 0, addr, PTE_P|PTE_U|PTE_COW)) < 0)
		panic("sys_page_map: %e", r);
	return r;
#else
	// LAB 4: Your code here.
	panic("duppage not implemented");
	return 0;
#endif
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use vpd, vpt, and duppage.
//   Remember to fix "env" and the user exception stack in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
#if SOL >= 4
	envid_t envid;
	int pn, end_pn, r;

	set_pgfault_handler(pgfault);

	// Create a child.
	envid = sys_exofork();
	if (envid < 0)
		return envid;
	if (envid == 0) {
		env = &envs[ENVX(sys_getenvid())];
		return 0;
	}

	// Copy the address space.
	for (pn = 0; pn < VPN(UTOP); ) {
		if (!(vpd[pn >> 10] & PTE_P)) {
			pn += NPTENTRIES;
			continue;
		}
		for (end_pn = pn + NPTENTRIES; pn < end_pn; pn++) {
			if ((vpt[pn] & (PTE_P|PTE_U)) != (PTE_P|PTE_U))
				continue;
			if (pn == VPN(UXSTACKTOP - 1))
				continue;
			duppage(envid, pn);
		}
	}

	// The child needs to start out with a valid exception stack.
	if ((r = sys_page_alloc(envid, (void*) (UXSTACKTOP - PGSIZE), PTE_P|PTE_U|PTE_W)) < 0)
		panic("allocating exception stack: %e", r);

	// Copy the user-mode exception entrypoint.
	if ((r = sys_env_set_pgfault_upcall(envid, env->env_pgfault_upcall)) < 0)
		panic("sys_env_set_pgfault_upcall: %e", r);


	// Okay, the child is ready for life on its own.
	if ((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0)
		panic("sys_env_set_status: %e", r);

	return envid;
#else
	// LAB 4: Your code here.
	panic("fork not implemented");
#endif
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
#endif
