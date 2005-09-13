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
#include <kern/trap.h>
#include <kern/monitor.h>
#if LAB >= 4
#include <kern/sched.h>
#endif

struct Env *envs = NULL;		// All environments
struct Env *curenv = NULL;	        // The current env
static struct Env_list env_free_list;	// Free list

#define ENVGENSHIFT	12		// >= LOGNENV

//
// Converts an envid to an env pointer.
//
// RETURNS
//   0 on success, -E_BAD_ENV on error.
//   On success, sets *penv to the environment.
//   On error, sets *penv to NULL.
//
int
envid2env(envid_t envid, struct Env **env_store, bool checkperm)
{
	struct Env *e;

	// If envid is zero, return the current environment.
	if (envid == 0) {
		*env_store = curenv;
		return 0;
	}

	// Look up the Env structure via the index part of the envid,
	// then check the env_id field in that struct Env
	// to ensure that the envid is not stale
	// (i.e., does not refer to a _previous_ environment
	// that used the same slot in the envs[] array).
	e = &envs[ENVX(envid)];
	if (e->env_status == ENV_FREE || e->env_id != envid) {
		*env_store = 0;
		return -E_BAD_ENV;
	}

	// Check that the calling environment has legitimate permission
	// to manipulate the specified environment.
	// If checkperm is set, the specified environment
	// must be either the current environment
	// or an immediate child of the current environment.
	if (checkperm && e != curenv && e->env_parent_id != curenv->env_id) {
		*env_store = 0;
		return -E_BAD_ENV;
	}

	*env_store = e;
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
#else
	// LAB 3: Your code here.
#endif /* SOL >= 3 */
}

//
// Initializes the kernel virtual memory layout for environment e.
// Allocates a page directory and initializes
// the kernel portion of the new environment's address space.
// Also sets e->env_cr3 and e->env_pgdir accordingly.
// We do NOT (yet) map anything into the user portion
// of the environment's virtual address space.
//
// RETURNS
//   0 -- on sucess
//   <0 -- otherwise 
//
static int
env_setup_vm(struct Env *e)
{
	int i, r;
	struct Page *p = NULL;

	// Allocate a page for the page directory
	if ((r = page_alloc(&p)) < 0)
		return r;

	// Hint:
	//    - The VA space of all envs is identical above UTOP
	//      (except at VPT and UVPT, which we've set below).
	//	See inc/memlayout.h for permissions and layout.
	//	Can you use boot_pgdir as a template?  Hint: Yes.
	//	(Make sure you got the permissions right in Lab 2.)
	//    - The initial VA below UTOP is empty.
	//    - You do not need to make any more calls to page_alloc.
	//    - Note: pp_ref is not maintained for physical pages
	//	mapped above UTOP.

#if SOL >= 3
	e->env_cr3 = page2pa(p);
	e->env_pgdir = (pde_t*) page2kva(p);
	memset(e->env_pgdir, 0, PGSIZE);

	// The VA space of all envs is identical above UTOP...
	static_assert(UTOP % PTSIZE == 0);
	for (i = PDX(UTOP); i <= PDX(~0); i++)
		e->env_pgdir[i] = boot_pgdir[i];

#else
	// LAB 3: Your code here.
#endif /* SOL >= 3 */

	// VPT and UVPT map the env's own page table, with
	// different permissions.
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
env_alloc(struct Env **new, envid_t parent_id)
{
	int32_t generation;
	int r;
	struct Env *e;

	if (!(e = LIST_FIRST(&env_free_list)))
		return -E_NO_FREE_ENV;

	// Allocate and set up the page directory for this environment.
	if ((r = env_setup_vm(e)) < 0)
		return r;

	// Generate an env_id for this environment.
	generation = (e->env_id + (1 << ENVGENSHIFT)) & ~(NENV - 1);
	if (generation <= 0)	// Don't create a negative env_id.
		generation = 1 << ENVGENSHIFT;
	e->env_id = generation | (e - envs);
	
	// Set the basic status variables.
	e->env_parent_id = parent_id;
	e->env_status = ENV_RUNNABLE;
	e->env_runs = 0;

	// Clear out all the saved register state,
	// to prevent the register values
	// of a prior environment inhabiting this Env structure
	// from "leaking" into our new environment.
	memset(&e->env_tf, 0, sizeof(e->env_tf));

	// Set up appropriate initial values for the segment registers.
	// GD_UD is the user data segment selector in the GDT, and 
	// GD_UT is the user text segment selector (see inc/memlayout.h).
	// The low 2 bits of each segment register contains the
	// Requestor Privilege Level (RPL); 3 means user mode.
	e->env_tf.tf_ds = GD_UD | 3;
	e->env_tf.tf_es = GD_UD | 3;
	e->env_tf.tf_ss = GD_UD | 3;
	e->env_tf.tf_esp = USTACKTOP;
	e->env_tf.tf_cs = GD_UT | 3;
	// You will set e->env_tf.tf_eip later.

#if LAB >= 4
	// Enable interrupts while in user mode.
#if SOL >= 4
	e->env_tf.tf_eflags = FL_IF; // interrupts enabled
#else
	// LAB 4: Your code here.
#endif	// SOL >= 4
#endif	// LAB >= 4

#if LAB >= 4
	// Clear the page fault handler until user installs one.
	e->env_pgfault_upcall = 0;

	// Also clear the IPC receiving flag.
	e->env_ipc_recving = 0;
#endif

#if LAB >= 5
	// If this is the file server (e == &envs[1]) give it I/O privileges.
#if SOL >= 5
	if (e == &envs[1])
		e->env_tf.tf_eflags |= FL_IOPL_3;
#else
	// LAB 5: Your code here.
#endif
#endif

	// commit the allocation
	LIST_REMOVE(e, env_link);
	*new = e;

#if LAB >= 5
	// cprintf("[%08x] new env %08x\n", curenv ? curenv->env_id : 0, e->env_id);
#else
	cprintf("[%08x] new env %08x\n", curenv ? curenv->env_id : 0, e->env_id);
#endif
	return 0;
}

//
// Allocate and map all required pages into an env's address space
// to cover virtual addresses va through va+len-1 inclusive.
// Does not zero or otherwise initialize the mapped pages in any way.
// Pages should be writable by user and kernel.
// Panic if any allocation attempt fails.
//
// Warning: Neither va nor len are necessarily page-aligned!
// You may assume, however, that nothing is already mapped
// in the pages touched by the specified virtual address range.
//
static void
map_segment(struct Env *e, void *va, size_t len)
{
#if SOL >= 3
	int r;
	struct Page *pp;
	void *endva = (uint8_t*) va + len;

	while (va < endva) {
		// Allocate and map a page covering virtual address va.
		if ((r = page_alloc(&pp)) < 0)
			panic("map_segment: could not alloc page: %e\n", r);

		// Insert the page into the env's address space
		if ((r = page_insert(e->env_pgdir, pp, va, PTE_P|PTE_W|PTE_U)) < 0)
			panic("map_segment: could not insert page: %e\n", r);

		// Move on to start of next page
		va = ROUNDDOWN((uint8_t*) va + PGSIZE, PGSIZE);
	}	
#else
	// Your code here.
#endif
}

//
// Set up the initial program binary, stack, and processor flags
// for a user process.
// This function is ONLY called during kernel initialization,
// before running the first user-mode environment.
//
// This function loads all loadable segments from the ELF binary image
// into the environment's user memory, starting at the appropriate
// virtual addresses indicated in the ELF program header.
// At the same time it clears to zero any portions of these segments
// that are marked in the program header as being mapped
// but not actually present in the ELF file - i.e., the program's bss section.
//
// All this is very similar to what our boot loader does, except the boot
// loader also needs to read the code from disk.  Take a look at
// boot/main.c to get ideas.
//
// Finally, this function maps one page for the program's initial stack.
//
static void
load_icode(struct Env *e, uint8_t *binary, size_t size)
{
#if SOL >= 3
	int i;
	struct Elf *elf;
	struct Proghdr *ph;

	// Switch to this new environment's address space,
	// so that we can use virtual addresses to load the program segments.
	lcr3(e->env_cr3);

	// Check magic number on binary
	elf = (struct Elf*) binary;
	if (elf->e_magic != ELF_MAGIC)
		panic("load_icode: not an ELF binary");

	// Record entrypoint of binary in env's initial EIP.
	e->env_tf.tf_eip = elf->e_entry;

	// Map and load segments as directed.
	ph = (struct Proghdr*) (binary + elf->e_phoff);
	for (i = 0; i < elf->e_phnum; i++, ph++) {
		if (ph->p_type != ELF_PROG_LOAD)
			continue;
		if (ph->p_va + ph->p_memsz < ph->p_va)
			panic("load_icode: overflow in elf header segment");
		if (ph->p_va + ph->p_memsz >= UTOP)
			panic("load_icode: icode wants to be above UTOP");

		// Map pages for the segment
		map_segment(e, (void*) ph->p_va, ph->p_memsz);

		// Load/clear the segment
		memcpy((char*) ph->p_va, binary + ph->p_offset, ph->p_filesz);
		memset((char*) ph->p_va + ph->p_filesz, 0, ph->p_memsz - ph->p_filesz);
	}

	// Give environment a stack
	map_segment(e, (void*) (USTACKTOP - PGSIZE), PGSIZE);
#else /* not SOL >= 3 */
	// Hint: 
	//  Use map_segment() to map memory for each program segment.
	//  Load each program section into virtual memory
	//  at the address specified in the ELF section header.
	//  You should only load sections with ph->p_type == ELF_PROG_LOAD.
	//  Each section's virtual address can be found in ph->p_va
	//  and its size in memory can be found in ph->p_memsz.
	//  The ph->p_filesz bytes from the ELF binary, starting at
	//  'binary + ph->p_offset', should be copied to virtual address
	//  ph->p_va.  Any remaining memory bytes should be cleared to zero.
	//  (The ELF header should have ph->p_filesz <= ph->p_memsz.)
	//  Use functions from the previous lab to allocate and map pages.
	//
	//  All page protection bits should be user read/write for now.
	//  ELF sections are not necessarily page-aligned, but you can
	//  assume for this function that no two sections will touch
	//  the same virtual page.
	//
	// Hint:
	//  Loading the sections is much simpler if you can move data
	//  directly into the virtual addresses stored in the ELF binary!
	//  So which page directory should be in force during
	//  this function?
	//
	// Hint:
	//  You must also do something with the program's entry point.
	//  What?

	// LAB 3: Your code here.

	// Now map one page for the program's initial stack
	// at virtual address USTACKTOP - PGSIZE.

	// LAB 3: Your code here.
#endif /* not SOL >= 3 */
}

//
// Allocates a new env and loads the elf binary into it.
// This function is ONLY called during kernel initialization,
// before running the first user-mode environment.
// The new env's parent env id is set to 0.
//
void
env_create(uint8_t *binary, size_t size)
{
#if SOL >= 3
	int r;
	struct Env *e;
	if ((r = env_alloc(&e, 0)) < 0)
		panic("env_create: could not allocate env: %e\n", r);
	load_icode(e, binary, size);
#else
	// LAB 3: Your code here.
#endif /* not SOL >= 3 */
}

//
// Frees env e and all memory it uses.
// 
void
env_free(struct Env *e)
{
	pte_t *pt;
	uint32_t pdeno, pteno;
	physaddr_t pa;

	// Note the environment's demise.
#if LAB >= 5
	// cprintf("[%08x] free env %08x\n", curenv ? curenv->env_id : 0, e->env_id);
#else
	cprintf("[%08x] free env %08x\n", curenv ? curenv->env_id : 0, e->env_id);
#endif

	// Flush all mapped pages in the user portion of the address space
	static_assert(UTOP % PTSIZE == 0);
	for (pdeno = 0; pdeno < PDX(UTOP); pdeno++) {

		// only look at mapped page tables
		if (!(e->env_pgdir[pdeno] & PTE_P))
			continue;

		// find the pa and va of the page table
		pa = PTE_ADDR(e->env_pgdir[pdeno]);
		pt = (pte_t*) KADDR(pa);

		// unmap all PTEs in this page table
		for (pteno = 0; pteno <= PTX(~0); pteno++) {
			if (pt[pteno] & PTE_P)
				page_remove(e->env_pgdir, PGADDR(pdeno, pteno, 0));
		}

		// free the page table itself
		e->env_pgdir[pdeno] = 0;
		page_decref(pa2page(pa));
	}

	// free the page directory
	pa = e->env_cr3;
	e->env_pgdir = 0;
	e->env_cr3 = 0;
	page_decref(pa2page(pa));

	// return the environment to the free list
	e->env_status = ENV_FREE;
	LIST_INSERT_HEAD(&env_free_list, e, env_link);
}

//
// Frees env e.  And schedules a new env
// if e was the current env.
//
void
env_destroy(struct Env *e) 
{
	env_free(e);

#if LAB >= 4
	if (curenv == e) {
		curenv = NULL;
		sched_yield();
	}
#else
	cprintf("Destroyed the only environment - nothing more to do!\n");
	while (1)
		monitor(NULL);
#endif
}


//
// Restores the register values in the Trapframe
//  (does not return)
//
void
env_pop_tf(struct Trapframe *tf)
{
	__asm __volatile("movl %0,%%esp\n"
		"\tpopal\n"
		"\tpopl %%es\n"
		"\tpopl %%ds\n"
		"\taddl $0x8,%%esp\n" /* skip tf_trapno and tf_errcode */
		"\tiret"
		: : "g" (tf) : "memory");
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
	// Is this a context switch or just a return?
	if (curenv != e) {
		// keep track of which environment we're currently
		// running
		curenv = e;
		e->env_runs++;

		// restore e's address space
		lcr3(e->env_cr3);
	}

	// restore e's register state
	env_pop_tf(&e->env_tf);
#else /* not SOL >= 3 */
	// Step 1: Set 'curenv' to the new environment to be run,
	//	   and update the 'env_runs' counter if this is a
	//	   context switch.
	// Step 2: Use lcr3() to switch to the new environment's
	//         address space if this is a context switch.
	// Step 3: Use env_pop_tf() to restore the environment's
	//         registers and drop into user mode in the
	//         environment.

	// Hint: This function loads the new environment's state from
	//	e->env_tf.  Go back through the code you wrote above
	//	and make sure you have set the relevant parts of
	//	e->env_tf to sensible values, based on e's memory
	//	layout.
	
	// LAB 3: Your code here.
#endif /* not SOL >= 3 */
}

#endif /* LAB >= 3 */
