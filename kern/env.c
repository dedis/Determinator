///LAB2
/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/mmu.h>
#include <inc/error.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/sched.h>
#include <kern/printf.h>

struct Env *envs = NULL;		// All environments
struct Env *curenv = NULL;	        // the current env
///END
///LAB3
static struct Env_list env_free_list;	// Free list

//
// Calculates the envid for env e.  
//
static u_int
mkenvid(struct Env *e)
{
	static u_long next_env_id = 0;
	// lower bits of envid hold e's position in the envs array
	u_int idx = e - envs;
	// high bits of envid hold an increasing number
	return(next_env_id++ << (1 + LOG2NENV)) | idx;
}

//
// Converts an envid to an env pointer.
//
// RETURNS
//   env pointer -- on success and sets *error = 0
//   NULL -- on failure, and sets *error = the error number
//
struct Env *
envid2env(u_int envid, int *error)
{
	struct Env *e = &envs[ENVX(envid)];
	if (e->env_status == ENV_FREE || e->env_id != envid) {
		*error = -E_BAD_ENV;
		return NULL;
	} else {
		*error = 0;
		return e;
	}
}

//
// Sets up the the stack and program binary for a user process.
//   This function loads the binary image at virtual address UTEXT,
//   and maps one page for the program's initial stack
//   at virtual address USTACKTOP - BY2PG.
//
void
load_aout(struct Env *e, u_char *binary, u_int size)
{
///SOL3
	int i, r;
	struct Page *pp;

	// Allocate and map physical pages
	for (i = 0; i < size; i += BY2PG) {
		if ((r = page_alloc(&pp)) < 0)
			panic("load_aout: could not alloc page. Errno %d\n", r);
		bcopy(&binary[i], (void*)page2kva(pp), MIN(BY2PG, size - i));
		if ((r = page_insert(e->env_pgdir, pp, UTEXT + i,
					PTE_P|PTE_W|PTE_U)) < 0)
			panic("load_aout: could not map page. Errno %d\n", r);
	}

	/* Give it a stack */
	if ((r = page_alloc(&pp)) < 0)
		panic("load_aout: could not alloc page. Errno %d\n", r);
	if ((r = page_insert(e->env_pgdir, pp, USTACKTOP - BY2PG,
				PTE_P|PTE_W|PTE_U)) < 0)
		panic("load_aout: could not map page. Errno %d\n", r);
///ELSE
	// Hint: 
	//  Use page_alloc, page_insert, page2kva and e->env_pgdir
	//  You must figure out which permissions you'll need
	//  for the different mappings you create.
	//  Remember that the binary image is an a.out format image,
	//  which contains both text and data.
///END
}







//
// Marks all environments in 'envs' as free and inserts them into 
// the env_free_list.  Insert in reverse order, so that
// the first call to env_alloc() returns envs[0].
//
void
env_init(void)
{
///SOL3
	int i;
	LIST_INIT (&env_free_list);
	for (i = NENV - 1; i >= 0; i--) {
		envs[i].env_status = ENV_FREE;    
		LIST_INSERT_HEAD (&env_free_list, &envs[i], env_link);
	}
///END
}



//
// Initializes the kernel virtual memory layout for environment e.
//
// Allocates a page directory and initializes it.  Sets
// e->env_cr3 and e->env_pgdir accordingly.
//
// RETURNS
//   0 -- on sucess
//   <0 -- otherwise 
//
static int
env_setup_vm(struct Env *e)
{
	// Hint:

	int i, r;
	struct Page *p = NULL;

	// Allocate a page for the page directory
	if ((r = page_alloc(&p)) < 0)
		return r;

	// Hint:
	//    - The VA space of all envs is identical above UTOP
	//      (except at VPT and UVPT) 
	//    - Use boot_pgdir
	//    - Do not make any calls to page_alloc 
	//    - Note: pp_refcnt is not maintained for physical pages mapped above UTOP.

///SOL3
	e->env_cr3 = page2pa(p);
	e->env_pgdir = page2kva(p);
	bzero(e->env_pgdir, BY2PG);

	// The VA space of all envs is identical above UTOP...
	static_assert(UTOP % PDMAP == 0);
	for (i = PDX(UTOP); i <= PDX(~0); i++)
		e->env_pgdir[i] = boot_pgdir[i];

///END

	// ...except at VPT and UVPT.  These map the env's own page table
	e->env_pgdir[PDX(VPT)]   = e->env_cr3 | PTE_P | PTE_W;
	e->env_pgdir[PDX(UVPT)]  = e->env_cr3 | PTE_P | PTE_U;

	return 0;
}

//
// Allocates and initializes a new env.
//
// RETURNS
//   0 -- on success, sets *new to point at the new env 
//   <0 -- on failure
//
int
env_alloc(struct Env **new, u_int parent_id)
{
	int r;
	struct Env *e;

	if (!(e = LIST_FIRST(&env_free_list)))
		return -E_NO_FREE_ENV;

	if ((r = env_setup_vm(e)) < 0)
		return r;

	e->env_id = mkenvid(e);
	e->env_parent_id = parent_id;
	e->env_status = ENV_RUNNABLE;

	// Set initial values of registers
	//  (lower 2 bits of the seg regs is the RPL -- 3 means user process)
	e->env_tf.tf_ds = GD_UD | 3;
	e->env_tf.tf_es = GD_UD | 3;
	e->env_tf.tf_ss = GD_UD | 3;
	e->env_tf.tf_esp = USTACKTOP;
	e->env_tf.tf_cs = GD_UT | 3;
	// You also need to set tf_eip to the correct value.
	// Hint: see load_aout

///SOL3
	e->env_tf.tf_eip = UTEXT + 0x20; // right past a.out header
	e->env_tf.tf_eflags = FL_IF; // interrupts enabled
///ELSE
	e->env_tf.tf_eflags = 0;
///END

	e->env_ipc_blocked = 0;
	e->env_ipc_value = 0;
	e->env_ipc_from = 0;

	e->env_pgfault_handler = 0;
	e->env_xstacktop = 0;

	// commit the allocation
	LIST_REMOVE(e, env_link);
	*new = e;

	return 0;
}




//
// Allocates a new env and loads the a.out binary into it.
//  - new env's  parent env id is 0
void
env_create(u_char *binary, int size)
{
////SOL3
	int r;
	struct Env *e;
	if ((r = env_alloc(&e, 0)) < 0)
		panic("env_create: could not allocate env.  Error %d\n", r);
	load_aout(e, binary, size);
////END
}


//
// Frees env e and all memory it uses.
// 
void
env_free(struct Env *e)
{
///SOL3
	Pte *pt;
	u_int pdeno, pteno;

	static_assert(UTOP%PDMAP == 0);
	// Flush all pages
	for (pdeno = 0; pdeno < PDX(UTOP); pdeno++) {
		if (!(e->env_pgdir[pdeno] & PTE_P))
			continue;
		pt = (Pte*)KADDR(PTE_ADDR(e->env_pgdir[pdeno]));
		for (pteno = 0; pteno <= PTX(~0); pteno++)
			if (pt[pteno] & PTE_P)
				page_remove(e->env_pgdir, (pdeno << PDSHIFT) | (pteno << PGSHIFT));
		page_free(pa2page(PADDR((u_long) pt)));
	}
	page_free(pa2page(PADDR((u_long) (e->env_pgdir))));
///END 

	// For lab 3, env_free() doesn't really do
	// anything(except leak memory).  We'll fix
	// this in later labs.
	e->env_status = ENV_FREE;
	LIST_INSERT_HEAD(&env_free_list, e, env_link);
}


//
// Frees env e.  And schedules a new env
// if e is the current env.
//
void
env_destroy(struct Env *e) 
{
	env_free(e);
	if (curenv == e) {
		curenv = NULL;
		sched_yield();
	}
}


//
// Restores the register values in the Trapframe
//  (does not return)
//
void
env_pop_tf(struct Trapframe *tf)
{
#if 0
	printf(" --> %d 0x%x\n", ENVX(curenv->env_id), tf->tf_eip);
#endif

	asm volatile("movl %0,%%esp\n"
		"\tpopal\n"
		"\tpopl %%es\n"
		"\tpopl %%ds\n"
		"\taddl $0x8,%%esp\n" /* skip tf_trapno and tf_errcode */
		"\tiret"
		:: "g" (tf) : "memory");
	panic("iret failed");  /* mostly to placate the compiler */
}


//
// Context switch from curenv to env e.
// Note: is this is the first call to env_run, curenv is NULL.
//  (This function does not return.)
void
env_run(struct Env *e)
{
	// step 1: save register state of curenv
	// step 2: set curenv
	// step 3: use lcr3
	// step 4: use env_pop_tf()

	// Hint: Skip step 1 until exercise 4.  You don't
	// need it for exercise 1, and in exercise 4 you'll better
	// understand what you need to do.
///SOL3
	// save register state of currently executing env
	if (curenv)
		curenv->env_tf = *UTF;
	curenv = e;
	// switch to e's addressing context
	lcr3(e->env_cr3);
	// restore e's register state
	env_pop_tf(&e->env_tf);
///END
}


///END
