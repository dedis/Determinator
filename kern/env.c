#if LAB >= 2
/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/mmu.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/elf.h>

#include <kern/env.h>
#include <kern/pmap.h>
#if LAB >= 3
#include <kern/trap.h>
#include <kern/sched.h>
#endif

struct Env *envs = NULL;		// All environments
struct Env *curenv = NULL;	        // the current env

#if LAB >= 3
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
	return(++next_env_id << (1 + LOG2NENV)) | idx;
}

//
// Converts an envid to an env pointer.
//
// RETURNS
//   env pointer -- on success and sets *error = 0
//   NULL -- on failure, and sets *error = the error number
//
int
envid2env(u_int envid, struct Env **penv, int checkperm)
{
	struct Env *e;

	if (envid == 0) {
		*penv = curenv;
		return 0;
	}

	e = &envs[ENVX(envid)];
	if (e->env_status == ENV_FREE || e->env_id != envid) {
		*penv = 0;
		return -E_BAD_ENV;
	}

	if (checkperm) {
#if SOL >= 4
		if (e != curenv && e->env_parent_id != curenv->env_id) {
			*penv = 0;
			return -E_BAD_ENV;
		}
#else
		// Your code here in Lab 4
		return -E_BAD_ENV;
#endif
	}
	*penv = e;
	return 0;
}

//
// Marks all environments in 'envs' as free and inserts them into 
// the env_free_list.  Insert in reverse order, so that
// the first call to env_alloc() returns envs[0].
//
void
env_init(void)
{
#if SOL >= 3
	int i;
	LIST_INIT (&env_free_list);
	for (i = NENV - 1; i >= 0; i--) {
		envs[i].env_status = ENV_FREE;    
		LIST_INSERT_HEAD (&env_free_list, &envs[i], env_link);
	}
#endif /* SOL >= 3 */
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
	//    - Note: pp_ref is not maintained for physical pages mapped above UTOP.

#if SOL >= 3
	e->env_cr3 = page2pa(p);
	e->env_pgdir = (Pde*)page2kva(p);
	memset(e->env_pgdir, 0, BY2PG);

	// The VA space of all envs is identical above UTOP...
	static_assert(UTOP % PDMAP == 0);
	for (i = PDX(UTOP); i <= PDX(~0); i++)
		e->env_pgdir[i] = boot_pgdir[i];

#endif /* SOL >= 3 */

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
	// Hint: see load_icode

#if SOL >= 3
	e->env_tf.tf_eflags = FL_IF; // interrupts enabled
#else
	e->env_tf.tf_eflags = 0;
#endif
#if LAB >= 5
	// If this is the file server (e==&envs[1]) give it I/O privileges.
#if SOL >= 5
	if (e == &envs[1])
		e->env_tf.tf_eflags |= FL_IOPL0|FL_IOPL1;
#else
	//   (your code here)
#endif
#endif
	e->env_ipc_recving = 0;


	e->env_pgfault_handler = 0;
	e->env_xstacktop = 0;

	// commit the allocation
	LIST_REMOVE(e, env_link);
	*new = e;

#if LAB >= 5
	// printf("[%08x] new env %08x\n", curenv ? curenv->env_id : 0, e->env_id);
#else
	printf("[%08x] new env %08x\n", curenv ? curenv->env_id : 0, e->env_id);
#endif
	return 0;
}

// Map a segment at address va of size len bytes, rounded up to page size.
// Initialize the segment with src.
// Panic if it fails.
static void
map_segment(struct Env *e, u_int va, u_int len, u_char *src)
{
#if SOL >= 3
	int r;
	u_int i;
	struct Page *p;

	if (va&(BY2PG-1)) {
		i = va & (BY2PG-1);
		va -= i;
		len += i;
		src -= i;
	}

	for (i = 0; i < len; i += BY2PG) {
		if ((r = page_alloc(&p)) < 0)
			panic("map_segment: could not alloc page: %e\n", r);
		// printf("copy page %x to %x\n", src+i, va+i);
		memcpy((void*)page2kva(p), src+i, MIN(BY2PG, len-i));
		if ((r = page_insert(e->env_pgdir, p, va+i, PTE_P|PTE_W|PTE_U)) < 0)
			panic("map_segment: could not insert page: %e\n", r);
	}	
#else
	// Your code here.
#endif
}

//
// Set up the the initial stack and program binary for a user process.
//
// This function loads the complete binary image, including a.out header,
// into the environment's user memory starting at virtual address UTEXT,
// and maps one page for the program's initial stack
// at virtual address USTACKTOP - BY2PG.
// Since the a.out header from the binary is mapped at virtual address UTEXT,
// the actual program text starts at virtual address UTEXT+0x20.
//
// This function does not allocate or clear the bss of the loaded program,
// and all mappings are read/write including those of the text segment.
//
static void
load_icode(struct Env *e, u_char *binary, u_int size)
{
#if SOL >= 3
	int i, r;
	struct Elf *elf;
	struct Page *p;
	struct Proghdr *ph;

	// Check magic number on binary
	elf = (struct Elf*)binary;
	if (*(u_int*)elf != *(u_int*)"\x7F\x45\x4C\x46")
		panic("load_icode: not an ELF binary");

	// Record entry for binary.
	e->env_tf.tf_eip = elf->e_entry;
	printf("load_icode: entry is 0x%x\n", e->env_tf.tf_eip);

	// Map segments as directed.
	ph = (struct Proghdr*)(binary + elf->e_phoff);
	for (i = 0; i < elf->e_phnum; i++, ph++) {
		if(ph->p_type != ELF_PROG_LOAD)
			continue;
		map_segment(e, ph->p_va, ph->p_memsz, binary+ph->p_offset);	
	}

	// Give environment a stack
	if ((r = page_alloc(&p)) < 0)
		panic("load_icode: could not alloc page: %e\n", r);
	if ((r = page_insert(e->env_pgdir, p, USTACKTOP - BY2PG,
				PTE_P|PTE_W|PTE_U)) < 0)
		panic("load_icode: could not map page: %e\n", r);
#else /* not SOL >= 3 */
	// Hint: 
	//  Use page_alloc, page_insert, page2kva and e->env_pgdir
	//  You must figure out which permissions you'll need
	//  for the different mappings you create.
	//  Remember that the binary image is an a.out format image,
	//  which contains both text and data. XXX THIS IS WRONG
#endif /* not SOL >= 3 */
}

//
// Allocates a new env and loads the a.out binary into it.
//  - new env's parent env id is 0
void
env_create(u_char *binary, int size)
{
#if SOL >= 3
	int r;
	struct Env *e;
	if ((r = env_alloc(&e, 0)) < 0)
		panic("env_create: could not allocate env: %e\n", r);
	load_icode(e, binary, size);
#endif /* not SOL >= 3 */
}

//
// Frees env e and all memory it uses.
// 
void
env_free(struct Env *e)
{
#if LAB >= 4
	Pte *pt;
	u_int pdeno, pteno, pa;

	// Note the environment's demise.
#if LAB >= 5
	// printf("[%08x] free env %08x\n", curenv ? curenv->env_id : 0, e->env_id);
#else
	printf("[%08x] free env %08x\n", curenv ? curenv->env_id : 0, e->env_id);
#endif

	// Flush all pages
	static_assert(UTOP%PDMAP == 0);
	for (pdeno = 0; pdeno < PDX(UTOP); pdeno++) {
		if (!(e->env_pgdir[pdeno] & PTE_P))
			continue;
		pa = PTE_ADDR(e->env_pgdir[pdeno]);
		pt = (Pte*)KADDR(pa);
		for (pteno = 0; pteno <= PTX(~0); pteno++)
			if (pt[pteno] & PTE_P)
				page_remove(e->env_pgdir, (pdeno << PDSHIFT) | (pteno << PGSHIFT));
		e->env_pgdir[pdeno] = 0;
		page_decref(pa2page(pa));
	}
	pa = e->env_cr3;
	e->env_pgdir = 0;
	e->env_cr3 = 0;
	page_decref(pa2page(pa));

#else /* not LAB >= 4 */
	// For lab 3, env_free() doesn't really do
	// anything (except leak memory).  We'll fix
	// this in later labs.
#endif /* not LAB >= 4 */

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
// Note: if this is the first call to env_run, curenv is NULL.
//  (This function does not return.)
//
void
env_run(struct Env *e)
{
#if SOL >= 3
	// save register state of currently executing env
	if (curenv)
		curenv->env_tf = *UTF;
	curenv = e;
	// restore e's address space
	lcr3(e->env_cr3);
	// restore e's register state
	e->env_runs++;
	env_pop_tf(&e->env_tf);
#else /* not SOL >= 3 */
	// step 1: save register state of curenv
	// step 2: set curenv
	// step 3: use lcr3
	// step 4: use env_pop_tf()

	// Hint: Skip step 1 until exercise 4.  You don't
	// need it for exercise 1, and in exercise 4 you'll better
	// understand what you need to do.
#endif /* not SOL >= 3 */
}


#endif /* LAB >= 3 */
#endif /* LAB >= 2 */
