#if LAB >= 3
/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/syscall.h>
#include <kern/console.h>
#if LAB >= 4
#include <kern/sched.h>
#endif

// Print a string to the system console.
// The system call returns 0.
static void
sys_cputs(const char *s)
{
#if SOL >= 3
	page_fault_mode = PFM_KILL;
	printf("%s", TRUP(s));
	page_fault_mode = PFM_NONE;
#else
	printf("%s", s);
#endif
}

// Read a character from the system console.
// Returns the character.
static int
sys_cgetc(void)
{
	int c;

	// The cons_getc() primitive doesn't wait for a character,
	// but the sys_cgetc() system call does.
	while ((c = cons_getc()) == 0)
		/* do nothing */;

	return c;
}

// Returns the current environment's envid.
static envid_t
sys_getenvid(void)
{
	return curenv->env_id;
}

// Destroy a given environment (possibly the currently running environment).
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_destroy(envid_t envid)
{
	int r;
	struct Env *e;

	if ((r = envid2env(envid, &e, 1)) < 0)
		return r;
#if LAB >= 5
#else
	if (e == curenv)
		printf("[%08x] exiting gracefully\n", curenv->env_id);
	else
		printf("[%08x] destroying %08x\n", curenv->env_id, e->env_id);
#endif
	env_destroy(e);
	return 0;
}

#if LAB >= 4
// Deschedule current environment and pick a different one to run.
// The system call returns 0.
static void
sys_yield(void)
{
	sched_yield();
}

// Allocate a new environment.
// Returns envid of new environment, or < 0 on error.  Errors are:
//	-E_NO_FREE_ENV if no free environment is available.
static envid_t
sys_exofork(void)
{
#if SOL >= 4
	int r;
	struct Env *e;

	if ((r = env_alloc(&e, curenv->env_id)) < 0)
		return r;
	e->env_status = ENV_NOT_RUNNABLE;
	e->env_tf = *UTF;
	e->env_tf.tf_eax = 0;
	return e->env_id;
#else
	// Create the new environment with env_alloc(), from kern/env.c.
	// It should be left as env_alloc created it, except that
	// status is set to ENV_NOT_RUNNABLE, and the register set is copied
	// from the current environment -- but tweaked so sys_exofork
	// will appear to return 0.
	//
	// Hint: Your code in env_run() shows how to copy a register set.
	
	// LAB 4: Your code here.
	panic("sys_exofork not implemented");
#endif
}

// Set envid's env_status to status, which must be ENV_RUNNABLE
// or ENV_NOT_RUNNABLE.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if status is not a valid status for an environment.
static int
sys_env_set_status(envid_t envid, int status)
{
#if SOL >= 3
	struct Env *e;
	int r;

	if ((r = envid2env(envid, &e, 1)) < 0)
		return r;
	if (status != ENV_RUNNABLE && status != ENV_NOT_RUNNABLE)
		return -E_INVAL;
	e->env_status = status;
	return 0;
#else
  	// Hint: Use the 'envid2env' function from kern/env.c to translate an
  	// envid to a struct Env.
	// You should set envid2env's third argument to 1, which will
	// check whether the current environment has permission to set
	// envid's status.
	
	// LAB 4: Your code here.
	panic("sys_env_set_status not implemented");
#endif
}

// Set envid's trap frame to 'tf'.
// tf is modified to make sure that user environments always run at code
// protection level 3 (CPL 3) with interrupts enabled.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_set_trapframe(envid_t envid, struct Trapframe *tf)
{
#if SOL >= 4
	int r;
	struct Env *e;
	struct Trapframe ltf;

	page_fault_mode = PFM_KILL;
	ltf = *TRUP(tf);
	page_fault_mode = PFM_NONE;

	ltf.tf_eflags |= FL_IF;
	ltf.tf_cs |= 3;

	if ((r = envid2env(envid, &e, 1)) < 0)
		return r;
	if (e == curenv)
		*UTF = ltf;
	else
		e->env_tf = ltf;
	return 0;
#else
	// Hint: You must do something special if 'envid' is the current
	// environment!

	// LAB 4: Your code here.
	panic("sys_set_trapframe not implemented");
#endif
}

// Set the page fault upcall for 'envid' by modifying the corresponding struct
// Env's 'env_pgfault_upcall' field.  When 'envid' causes a page fault, the
// kernel will push a fault record onto the exception stack, then branch to
// 'func'.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_set_pgfault_upcall(envid_t envid, void *func)
{
#if SOL >= 4
	int r;
	struct Env *e;

	if ((r = envid2env(envid, &e, 1)) < 0)
		return r;
	e->env_pgfault_upcall = func;
	return 0;
#else
	// LAB 4: Your code here.
	panic("sys_env_set_pgfault_upcall not implemented");
#endif
}

// Allocate a page of memory and map it at 'va' with permission
// 'perm' in the address space of 'envid'.
// The page's contents are set to 0.
// If a page is already mapped at 'va', that page is unmapped as a
// side effect.
//
// perm -- PTE_U | PTE_P must be set, PTE_AVAIL | PTE_W may or may not be set,
//         but no other bits may be set.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if va >= UTOP, or va is not page-aligned.
//	-E_INVAL if perm is inappropriate (see above).
//	-E_NO_MEM if there's no memory to allocate the new page,
//		or to allocate any necessary page tables.
static int
sys_page_alloc(envid_t envid, void *va, int perm)
{
#if SOL >= 4
	int r;
	struct Env *e;
	struct Page *pp;

	if ((r = envid2env(envid, &e, 1)) < 0)
		return r;
	if ((~perm & (PTE_U|PTE_P)) || (perm & ~PTE_USER))
		return -E_INVAL;
	if (va >= (void*) UTOP)
		return -E_INVAL;
	if ((r = page_alloc(&pp)) < 0)
		return r;
	if ((r = page_insert(e->env_pgdir, pp, va, perm)) < 0) {
		page_free(pp);
		return r;
	}
	memset(page2kva(pp), 0, PGSIZE);
	return 0;
#else
	// Hint: This function is a wrapper around page_alloc() and
	//   page_insert() from kern/pmap.c.
	//   Most of the new code you write should be to check the
	//   parameters for correctness.
	//   If page_insert() fails, remember to free the page you
	//   allocated!

	// LAB 4: Your code here.
	panic("sys_page_alloc not implemented");
#endif
}

// Map the page of memory at 'srcva' in srcenvid's address space
// at 'dstva' in dstenvid's address space with permission 'perm'.
// Perm has the same restrictions as in sys_page_alloc, except
// that it also must not grant write access to a read-only
// page.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if srcenvid and/or dstenvid doesn't currently exist,
//		or the caller doesn't have permission to change one of them.
//	-E_INVAL if srcva >= UTOP or srcva is not page-aligned,
//		or dstva >= UTOP or dstva is not page-aligned.
//	-E_INVAL is srcva is not mapped in srcenvid's address space.
//	-E_INVAL if perm is inappropriate (see sys_page_alloc).
//	-E_INVAL if (perm & PTE_W), but srcva is read-only in srcenvid's
//		address space.
//	-E_NO_MEM if there's no memory to allocate the new page,
//		or to allocate any necessary page tables.
static int
sys_page_map(envid_t srcenvid, void *srcva,
	     envid_t dstenvid, void *dstva, int perm)
{
#if SOL >= 4
	int r;
	struct Env *es, *ed;
	struct Page *pp;
	pte_t *ppte;

	if (srcva >= (void*) UTOP || dstva >= (void*) UTOP)
		return -E_INVAL;
	if ((r = envid2env(srcenvid, &es, 1)) < 0
	    || (r = envid2env(dstenvid, &ed, 1)) < 0)
		return r;
	if ((~perm & (PTE_U|PTE_P)) || (perm & ~PTE_USER))
		return -E_INVAL;
	if ((pp = page_lookup(es->env_pgdir, srcva, &ppte)) == 0)
		return -E_INVAL;
	if ((perm & PTE_W) && !(*ppte & PTE_W))
		return -E_INVAL;
	if ((r = page_insert(ed->env_pgdir, pp, dstva, perm)) < 0)
		return r;
	return 0;
#else
	// Hint: This function is a wrapper around page_lookup() and
	//   page_insert() from kern/pmap.c.
	//   Again, most of the new code you write should be to check the
	//   parameters for correctness.
	//   Use the third argument to page_lookup() to
	//   check the current permissions on the page.

	// LAB 4: Your code here.
	panic("sys_page_map not implemented");
#endif
}

// Unmap the page of memory at 'va' in the address space of 'envid'.
// If no page is mapped, the function silently succeeds.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if va >= UTOP, or va is not page-aligned.
static int
sys_page_unmap(envid_t envid, void *va)
{
#if SOL >= 3
	int r;
	struct Env *e;

	if ((r = envid2env(envid, &e, 1)) < 0)
		return r;
	if (va >= (void*) UTOP)
		return -E_INVAL;
	page_remove(e->env_pgdir, va);
	return 0;
#else
	// Hint: This function is a wrapper around page_remove().
	
	// LAB 3 PART 3: Your code here.
	panic("sys_page_unmap not implemented");
#endif
}

// Try to send 'value' to the target env 'envid'.
// If va != 0, then also send page currently mapped at 'va',
// so that receiver gets a duplicate mapping of the same page.
//
// The send fails with a return value of -E_IPC_NOT_RECV if the
// target has not requested IPC with sys_ipc_recv.
//
// Otherwise, the send succeeds, and the target's ipc fields are
// updated as follows:
//    env_ipc_recving is set to 0 to block future sends;
//    env_ipc_from is set to the sending envid;
//    env_ipc_value is set to the 'value' parameter;
//    env_ipc_perm is set to 'perm' if a page was transferred, 0 otherwise.
// The target environment is marked runnable again.
//
// If the sender sends a page but the receiver isn't asking for one,
// then no page mapping is transferred, but no error occurs.
// The ipc doesn't happen unless no errors occur.
//
// Returns 0 on success where no page mapping occurs,
// 1 on success where a page mapping occurs, and < 0 on error.
// Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist.
//		(No need to check permissions.)
//	-E_IPC_NOT_RECV if envid is not currently blocked in sys_ipc_recv,
//		or another environment managed to send first.
//	-E_INVAL if srcva < UTOP but srcva is not page-aligned.
//	-E_INVAL if srcva < UTOP and perm is inappropriate
//		(see sys_page_alloc).
//	-E_INVAL if srcva < UTOP but srcva is not mapped in the caller's
//		address space.
//	-E_NO_MEM if there's not enough memory to map srcva in envid's
//		address space.
static int
sys_ipc_try_send(envid_t envid, uint32_t value, void *srcva, unsigned perm)
{
#if SOL >= 4
	int r;
	struct Env *e;
	struct Page *pp;
	int pgmap = 0;

	if ((r = envid2env(envid, &e, 0)) < 0)
		return r;
	if (!e->env_ipc_recving)
		return -E_IPC_NOT_RECV;

	if (srcva < (void*) UTOP && e->env_ipc_dstva < (void*) UTOP) {

		if ((~perm & (PTE_U|PTE_P)) || (perm & ~PTE_USER)) {
			printf("[%08x] bad perm %x in sys_ipc_try_send\n", curenv->env_id, perm);
			return -E_INVAL;
		}

		pp = page_lookup(curenv->env_pgdir, srcva, 0);
		if (pp == 0) {
			printf("[%08x] page_lookup %08x failed in sys_ipc_try_send\n", curenv->env_id, srcva);
			return -E_INVAL;
		}
		
		r = page_insert(e->env_pgdir, pp, e->env_ipc_dstva, perm);
		if (r < 0) {
			printf("[%08x] page_insert %08x failed in sys_ipc_try_send (%e)\n", curenv->env_id, srcva, r);
			return r;
		}
		
		pgmap = 1;
		e->env_ipc_perm = perm;
	} else {
		e->env_ipc_perm = 0;
	}

	e->env_ipc_recving = 0;
	e->env_ipc_from = curenv->env_id;
	e->env_ipc_value = value;
	e->env_status = ENV_RUNNABLE;
	return pgmap;
#else
	// LAB 4: Your code here.
	panic("sys_ipc_try_send not implemented");
#endif
}

// Block until a value is ready.  Record that you want to receive
// using the env_ipc_recving and env_ipc_dstva fields of struct Env,
// mark yourself not runnable, and then give up the CPU.
//
// If 'dstva' is < UTOP, then you are willing to receive a page of data.
// 'dstva' is the virtual address at which the sent page should be mapped.
//
// This function only returns on error, but the system call will eventually
// return 0 on success.
// Return < 0 on error.  Errors are:
//	-E_INVAL if dstva < UTOP but dstva is not page-aligned.
static int
sys_ipc_recv(void *dstva)
{
#if SOL >= 4
	if (curenv->env_ipc_recving)
		panic("already recving!");

	curenv->env_ipc_recving = 1;
	curenv->env_ipc_dstva = dstva;
	curenv->env_status = ENV_NOT_RUNNABLE;
	sched_yield();
	return 0;
#else
	// LAB 4: Your code here.
	panic("sys_ipc_recv not implemented");
	return 0;
#endif
}
#endif	// LAB >= 4


// Dispatches to the correct kernel function, passing the arguments.
uint32_t
syscall(uint32_t sn, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
{
	// printf("syscall %d %x %x %x from env %08x\n", sn, a1, a2, a3, curenv->env_id);

#if SOL >= 3
	switch (sn) {
	case SYS_cputs:
		sys_cputs((const char*) a1);
		return 0;
	case SYS_cgetc:
		return sys_cgetc();
	case SYS_getenvid:
		return sys_getenvid();
	case SYS_env_destroy:
		return sys_env_destroy(a1);
#if SOL >= 4
	case SYS_page_alloc:
		return sys_page_alloc(a1, (void*) a2, a3);
	case SYS_page_map:
		return sys_page_map(a1, (void*) a2, a3, (void*) a4, a5);
	case SYS_page_unmap:
		return sys_page_unmap(a1, (void*) a2);
	case SYS_exofork:
		return sys_exofork();
	case SYS_env_set_status:
		return sys_env_set_status(a1, a2);
	case SYS_env_set_trapframe:
		return sys_env_set_trapframe(a1, (struct Trapframe*) a2);
	case SYS_env_set_pgfault_upcall:
		return sys_env_set_pgfault_upcall(a1, (void*) a2);
	case SYS_yield:
		sys_yield();
		return 0;
	case SYS_ipc_try_send:
		return sys_ipc_try_send(a1, a2, (void*) a3, a4);
	case SYS_ipc_recv:
		sys_ipc_recv((void*) a1);
		return 0;
#endif	// SOL >= 4
	default:
		return -E_INVAL;
	}
#else	// not SOL >= 3
	// Your code here
	panic("syscall not implemented");
#endif	// not SOL >= 3
}

#endif	// LAB >= 3
