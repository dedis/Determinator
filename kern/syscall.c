///LAB4
/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/syscall.h>
#include <kern/console.h>
#include <kern/printf.h>

///LAB200
u_int(*sctab[MAX_SYSCALL + 1])() = {
	[SYS_getenvid] = (void *) sys_getenvid,
	[SYS_cputu] = (void *) sys_cputu,
	[SYS_cputs] = (void *) sys_cputs,
	[SYS_yield] = (void *) sys_yield,
	[SYS_env_destroy] = (void *) sys_env_destroy,
	[SYS_env_alloc] = (void *) sys_env_alloc,
	[SYS_ipc_send] = (void *) sys_ipc_send,
	[SYS_ipc_unblock] = (void *) sys_ipc_unblock,
	[SYS_set_pgfault_handler] = (void *) sys_set_pgfault_handler,
	[SYS_mod_perms] = (void *) sys_mod_perms,
	[SYS_set_env_status] = (void *) sys_set_env_status,
	[SYS_mem_unmap] = (void *) sys_mem_unmap,
	[SYS_mem_alloc] = (void *) sys_mem_alloc,
	[SYS_mem_remap] = (void *) sys_mem_remap,
	[SYS_disk_read] = (void *) sys_disk_read
};
///END


// Dispatches to the correct kernel function, passing the arguments.

///LAB200
#if 0
///END
int
dispatch_syscall(u_int sn, u_int a1, u_int a2, u_int a3)
{
}
///LAB200
#endif

int
dispatch_syscall(u_int sn, u_int a1, u_int a2, u_int a3, u_int a4)
{
	if (sn <= MAX_SYSCALL)
		return sctab[sn] (a1, a2, a3, a4);
	else {
		printf("Invalid system call %d\n", sn);
		return -E_INVAL;
	}
}
///END

// returns the current environment id
u_int
sys_getenvid(void)
{
	return curenv->env_id;
}

// prints the integer to the screen.
void
sys_cputu(u_int value)
{
	printf("%d", value);
}

// prints the string to the screen.
void
sys_cputs(char *s)
{
///LAB5
#if 0
///END
	printf("%s", s);
///LAB5
#endif
///END
///LAB5
	page_fault_mode = PFM_KILL;
	printf("%s", trup(s));
	page_fault_mode = PFM_NONE;
///END
}

// deschedule current environment
void
sys_yield(void)
{
	sched_yield();
}

// destroys the current environment
void
sys_env_destroy(void)
{
	env_destroy(curenv);
}

// Allocates a new(child) environment.
//
// The state of the child is initialized as follows:  
//    env_parent_id -- set the the curenv's id
//    env_status  -- set to ENV_NOTRUNNABLE
//    env_xstacktop -- inherited from parent
//    env_pgfault_handler -- ditto
//    env_tf -- set so that the child
//              resumes execution at the point where the parent 
//              made the call to sys_env_alloc
//    env_tf.tf_eax -- set to 0, so that the child observes a 0
//                     return value
//    env_* -- All other fields are left to the values to which env_alloc
//             initialized them.
//
// RETURNS
//  <0 on error
//  otherwise envid of the new environment
//
int
sys_env_alloc(u_int inherit, u_int initial_esp)
{
///LAB5
	struct Env *e;
	int r;

	if ((r = env_alloc(&e, curenv->env_id)) < 0)
		return r;

	e->env_status = ENV_NOTRUNNABLE;
	if (inherit) {
		e->env_pgfault_handler = curenv->env_pgfault_handler;
		e->env_xstacktop = curenv->env_xstacktop;

		e->env_tf = *UTF;
		e->env_tf.tf_eax = 0;	 // return 0 to child
	} else {
		e->env_tf.tf_esp = initial_esp;
	}
	return e->env_id;	// return child's envid to parent
///END
}


// clears curenv->env_ipc_blocked
void
sys_ipc_unblock(void)
{
///LAB5
	//printf("sys_ipc_unblock: old blocked %u\n", curenv->env_ipc_blocked);
	curenv->env_ipc_blocked = 0;
///END
}

// Sends the 'value' to the target env 'envid'.
//
// The send fails, with a return value of -E_IPC_BLOCKED if the
// target's env_ipc_blocked field is set.
//
// Otherwise, the send succeeds, and the target's ipc fields are
// updated as follows:
//    env_ipc_block is set to 1 to block further IPCs
//    env_ipc_from is set to the sending envid
//    env_ipc_value is set to the 'value' parameter
//
// RETURNS
//  0 -- on success
//  <0 -- on error
int
sys_ipc_send(u_int envid, u_int value)
{
///LAB5
	struct Env *e;
	int r;

	//printf("*** sys_ipc_send\n");

	if (!(e = envid2env(envid, &r))) {
		printf("sys_ipc_send: No env\n");
		return r;
	}

	if (e->env_ipc_blocked) {
		printf("sys_ipc_send: Blocked\n");
		return -E_IPC_BLOCKED;
	}

	e->env_ipc_blocked = 1;
	e->env_ipc_from = curenv->env_id;
	e->env_ipc_value = value;

#if 0
	printf("*** Delivering IPC: value %u(from %d, to %d)\n", 
		value, curenv->env_id, e->env_id);
#endif

#if 1
	UTF->tf_eax = 0; // caller sees return value of 0
	env_run(e);	 // switch to destination environment
	/* NOT REACHED */
#else
	// the alternative is return back to caller
	// who probably should call yield
	return 0;
#endif
///END
}

// Sets the current env's pagefault handler entry point and exception
// stack.
//
// xstacktop points one byte past exception stack
//
void
sys_set_pgfault_handler(u_int func, u_int xstacktop)
{
	curenv->env_pgfault_handler = func;
	curenv->env_xstacktop = xstacktop;
}


// Sets envid's env_status to status. 
//
// envid==0, means the current environment.
//
//RETURNS
// 0 -- on sucess
// <0 -- on error
//       envid must be 0, the current env's envid of a child of the
//       current environment => E_BAD_ENV

int
sys_set_env_status(u_int envid, u_int status)
{
	struct Env *env;
	int r;

	// 0 means self
	if (envid == 0)
		env = curenv;
	else if (!(env = envid2env(envid, &r)))
		return r;
	else if (env->env_parent_id != curenv->env_id)
		return -E_BAD_ENV;
	
	env->env_status = status;
	return 0;
}


// Modifies the permission for the page of memory at the virtual
// address 'va'.
//
// 'add' -- these permission bits are turned on
// 'del' -- these permission bits are turned off
//
// Upon the completion of this function the permission bits of the
// PTE must satifies:
// permission -- PTE_U|PTE_P are required,
//               PTE_AVAIL|PTE_W are optional,
//               but not other bits are allowed
//
// The tlb is invalidated for 'va'.
//
//RETURNS
//  0 -- on success
//  <0 -- on error
//     va must be less than UTOP => -E_INVAL
//     the is no PTE corresponding to va => -E_INVAL
//     permission violation => -E_INVAL
int
sys_mod_perms(u_int va, u_int add, u_int del)
{
///LAB5
	Pde pde = vpd[PDX(va)];
	if (!(pde & PTE_P))
		return -E_INVAL;
	if (va >= UTOP)
		return -E_INVAL;
	if (del & PTE_P)
		return -E_INVAL;
	vpt[PGNO(va)] |= add; // XXX check these
	vpt[PGNO(va)] &= ~del; // XXX ditto
	tlb_invalidate(va, curenv->env_pgdir);
	return 0;
///END
}

//
// Allocates a page of memory and maps it at 'va' with permission
// 'perm' in the address space of 'envid'.
//
// If a page is already mapped at 'va', it is unmapped as a
// side-effect.
//
// envid==0, means the current environment.
//
// perm -- PTE_U|PTE_P are required, 
//         PTE_AVAIL|PTE_W are optional,
//         but not other bits are allowed
//
//RETURNS
// 0 -- on sucess
// <0 -- on error:
//    - va >= UTOP
//    - A env may modify its own address space or the address space of
//      its children.
//    - permission violation
//    - plus others.
int
sys_mem_alloc(u_int envid, u_int va, u_int perm)
{
///LAB5
	struct Env *env;
	struct Page *pp;
	int r;

	if (va >= UTOP)
		return -E_INVAL;

	if (envid == 0)
		env = curenv;
	else if (!(env = envid2env(envid, &r)))
		return r;
	else if (env->env_parent_id != curenv->env_id)
		return -E_BAD_ENV;

	if ((r = page_alloc(&pp)) < 0)
		return r;

	// XXX -- check perms
	if ((r = page_insert(env->env_pgdir, pp, va, perm)) < 0) {
		page_free(pp);
		return r;
	}
	return 0;
///END
}


//
// Remaps the page of memory at 'srcva' (in the current address space)
// at the address 'va' in the address space of 'envid' with permission
// 'perm'.
// envid==0, means the current environment.
//
// perm -- PTE_U|PTE_P are required, 
//         PTE_AVAIL|PTE_W are optional,
//         but not other bits are allowed
//
//RETURNS
// 0 -- on sucess
// <0 -- on error:
//    - va >= UTOP
//    - srcva >= UTOP
//    - A env may modify its own address space or the address space of
//      its child.
//    - No page is mapped at 'srcva'
//    - permission violation
//    - plus others..

int
sys_mem_remap(u_int srcva, u_int envid, u_int va, u_int perm)
{
///LAB5
	Pte pte;
	int r;
	struct Env *env;

	if (srcva >= UTOP || va >= UTOP)
		return -E_INVAL;

	if (envid == 0)
		env = curenv;
	else if (!(env = envid2env(envid, &r)))
		return r;
	else if (env->env_parent_id != curenv->env_id)
		return -E_BAD_ENV;

	if (!(vpd[PDX(srcva)] & PTE_P))
		return -E_INVAL;
	pte = vpt[PGNO(srcva)];
	if (!(pte & PTE_P))
		return -E_INVAL;

	// XXX -- check perms
	return page_insert(env->env_pgdir, &ppages[PGNO(pte)], va, perm);
///END
}


//
// Unmaps the page of memory at 'va' in the address space of 'envid'
// envid==0, means the current environment.
// (if no page is mapped, the function silently succeeds)
//
//RETURNS
// 0 -- on sucess
// <0 -- on error:
//    - va >= UTOP
//    - srcva >= UTOP
//    - A env may modify its own address space or the address space of
//      its child.
//    - No page is mapped at 'srcva'
//    - permission violation
//    - plus others..
int
sys_mem_unmap(u_int envid, u_int va)
{
///LAB5
	struct Env *env;
	int r;

	if (envid == 0)
		env = curenv;
	else if (!(env = envid2env(envid, &r)))
		return r;
	else if (env->env_parent_id != curenv->env_id)
		return -E_BAD_ENV;

	page_remove(env->env_pgdir, va);
	return 0;
///END
}

#define SECTOR_SIZE 512

static void
read_block(u_int diskno, u_int blockno, char *destination)
{
	unsigned int sectors_per_block = NBPG/SECTOR_SIZE;
	unsigned int sectorno = sectors_per_block * blockno;
	unsigned char status;
	do {
		status = inb(0x1f7);
	} while (status & 0x80);

	outb(0x1f2, sectors_per_block);       // sector count 
	outb(0x1f3, (sectorno >> 0) & 0xff);
	outb(0x1f4, (sectorno >> 8) & 0xff);
	outb(0x1f5, (sectorno >> 16) & 0xff);
	outb(0x1f6, 0xe0 | (0x1 & diskno) << 4 | ((sectorno >> 24) & 0x0f));
	outb(0x1f7, 0x20);         // CMD 0x20 means read sector
	do {
		status = inb(0x1f7);
	} while (status & 0x80);

	insl(0x1f0, destination, NLPG);
}


int
sys_disk_read(u_int diskno, u_int blockno, u_int va)
{
	int ret;
#if 0
	warn("diskno %d, blockno %d, va 0x%x\n", diskno, blockno, va);
#endif

	if (va >= UTOP)
		return -E_INVAL;

	ret = sys_mem_alloc(0, va, PTE_U|PTE_W|PTE_P);
	if (ret < 0)
		return ret;

	read_block(diskno, blockno, (char *)va);
	return 0;
}

///END
