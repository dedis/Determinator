#if LAB >= 2
/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/mmu.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/pmap.h>
#include <kern/kclock.h>
#if LAB >= 3
#include <kern/env.h>
#endif

u_long boot_cr3; /* Physical address of boot time pg dir */
Pde* boot_pgdir;
struct Page *pages;

/* These variables are set by i386_detect_memory() */
u_long maxpa;            /* Maximum physical address */
u_long npage;            /* Amount of memory(in pages) */
u_long basemem;          /* Amount of base memory(in bytes) */
u_long extmem;           /* Amount of extended memory(in bytes) */

static u_long freemem;    /* Pointer to next byte of free mem */
static struct Page_list page_free_list;	/* Free list of physical pages */

// Global descriptor table.
//
// The kernel and user segments are identical(except for the DPL).
// To load the SS register, the CPL must equal the DPL.  Thus,
// we must duplicate the segments for the user and the kernel.
//
struct Segdesc gdt[] =
{
	// 0x0 - unused (always faults -- for trapping NULL far pointers)
	SEG_NULL,

	// 0x8 - kernel code segment
	[GD_KT >> 3] = SEG(STA_X | STA_R, 0x0, 0xffffffff, 0),

	// 0x10 - kernel data segment
	[GD_KD >> 3] = SEG(STA_W, 0x0, 0xffffffff, 0),

	// 0x18 - user code segment
	[GD_UT >> 3] = SEG(STA_X | STA_R, 0x0, 0xffffffff, 3),

	// 0x20 - user data segment
	[GD_UD >> 3] = SEG(STA_W, 0x0, 0xffffffff, 3),

	// 0x40 - tss, initialized in idt_init()
	[GD_TSS >> 3] =  SEG_NULL
};

struct Pseudodesc gdt_pd =
{
	0, sizeof(gdt) - 1, (unsigned long) gdt,
};

static int
nvram_read(int r)
{
	return mc146818_read(NULL, r) | (mc146818_read(NULL, r+1)<<8);
}

void
i386_detect_memory(void)
{
	// CMOS tells us how many kilobytes there are
	basemem = ROUNDDOWN(nvram_read(NVRAM_BASELO)*1024, BY2PG);
	extmem = ROUNDDOWN(nvram_read(NVRAM_EXTLO)*1024, BY2PG);

	// Calculate the maxmium physical address based on whether
	// or not there is any extended memory.  See comment in ../inc/mmu.h.
	if (extmem)
		maxpa = EXTPHYSMEM + extmem;
	else
		maxpa = basemem;

	npage = maxpa / BY2PG;

	printf("Physical memory: %dK available, ", (int)(maxpa/1024));
	printf("base = %dK, extended = %dK\n", (int)(basemem/1024), (int)(extmem/1024));
}

// --------------------------------------------------------------
// Set up initial memory mappings and turn on MMU.
// --------------------------------------------------------------

static void check_boot_pgdir(void);

//
// Allocate n bytes of physical memory aligned on an 
// align-byte boundary.  Align must be a power of two.
// Return kernel virtual address.  If clear != 0, zero
// the memory.
//
// If we're out of memory, alloc should panic.
// It's too early to run out of memory.
// 
static void *
alloc(u_int n, u_int align, int clear)
{
	extern char end[];
	void *v;

	// initialize freemem if this is the first time
	if (freemem == 0)
		freemem = (u_long)end;

	// Your code here:
	//	Step 1: round freemem up to be aligned properly
	//	Step 2: save current value of freemem as allocated chunk
	//	Step 3: increase freemem to record allocation
	//	Step 4: clear allocated chunk if necessary
	//	Step 5: return allocated chunk

#if SOL >= 2
	freemem = ROUND(freemem, align);
	if (freemem+n < freemem || freemem+n > KERNBASE+maxpa)
		panic("out of memory during i386_vm_init");
	v = (void*)freemem;
	freemem += n;
	if (clear)
		memset(v, 0, n);
	return v;
#endif /* SOL >= 2 */
}

//
// Given pgdir, the current page directory base,
// walk the 2-level page table structure to find
// the Pte for virtual address va.  Return a pointer to it.
//
// If there is no such page table page:
//	- if create == 0, return 0.
//	- otherwise allocate a new page table and install it.
//
// This function is abstracting away the 2-level nature of
// the page directory for us by allocating new page tables
// as needed.
// 
// Boot_pgdir_walk cannot fail.  It's too early to fail.
// 
static Pte*
boot_pgdir_walk(Pde *pgdir, u_long va, int create)
{
#if SOL >= 2
	Pde *pde;
	Pte *pgtab;

	pde = &pgdir[PDX(va)];
	if (*pde & PTE_P)
		pgtab = (Pte*)KADDR(PTE_ADDR(*pde));
	else {
		if (!create)
			return 0;

		// Must set PTE_U in directory entry so that we can define
		// user segments with boot_map_segment.  If the
		// pages really aren't meant to be user readable,
		// the page table entries will reflect this.

		pgtab = alloc(BY2PG, BY2PG, 1);
		*pde = PADDR(pgtab)|PTE_P|PTE_U|PTE_W;
	}
	return &pgtab[PTX(va)];
#endif /* SOL >= 2 */
}

//
// Map [va, va+size) of virtual address space to physical [pa, pa+size)
// in the page table rooted at pgdir.  Size is a multiple of BY2PG.
// Use permission bits perm|PTE_P for the entries.
//
static void
boot_map_segment(Pde *pgdir, u_long va, u_long size, u_long pa, int perm)
{
#if SOL >= 2
	u_long i;

	for (i=0; i<size; i+=BY2PG)
		*boot_pgdir_walk(pgdir, va+i, 1) = (pa+i)|perm|PTE_P;
#endif /* SOL >= 2 */
}

// Set up a two-level page table:
//    boot_pgdir is its virtual address of the root
//    boot_cr3 is the physical adresss of the root
// Then turn on paging.  Then effectively turn off segmentation.
// (i.e., the segment base addrs are set to zero).
// 
// This function only sets up the kernel part of the address space
// (ie. addresses >= UTOP).  The user part of the address space
// will be setup later.
//
// From UTOP to ULIM, the user is allowed to read but not write.
// Above ULIM the user cannot read (or write). 
void
i386_vm_init(void)
{
	Pde *pgdir;
	u_int cr0, n;

#if SOL >= 2
#else
	panic("i386_vm_init: This function is not finished\n");
#endif

	//////////////////////////////////////////////////////////////////////
	// create initial page directory.
	pgdir = alloc(BY2PG, BY2PG, 1);
	boot_pgdir = pgdir;
	boot_cr3 = PADDR(pgdir);

	//////////////////////////////////////////////////////////////////////
	// Recursively insert PD in itself as a page table, to form
	// a virtual page table at virtual address VPT.
	// (For now, you don't have understand the greater purpose of the
	// following two lines.)

	// Permissions: kernel RW, user NONE
	pgdir[PDX(VPT)] = PADDR(pgdir)|PTE_W|PTE_P;

	// same for UVPT
	// Permissions: kernel R, user R 
	pgdir[PDX(UVPT)] = PADDR(pgdir)|PTE_U|PTE_P;

	//////////////////////////////////////////////////////////////////////
	// Map the kernel stack (symbol name "bootstack"):
	//   [KSTACKTOP-PDMAP, KSTACKTOP)  -- the complete VA range of the stack
	//     * [KSTACKTOP-KSTKSIZE, KSTACKTOP) -- backed by physical memory
	//     * [KSTACKTOP-PDMAP, KSTACKTOP-KSTKSIZE) -- not backed => faults
	//   Permissions: kernel RW, user NONE
	// Your code goes here:
#if SOL >= 2
	boot_map_segment(pgdir, KSTACKTOP-KSTKSIZE, KSTKSIZE,
		PADDR(bootstack), PTE_W);
#endif

	//////////////////////////////////////////////////////////////////////
	// Map all of physical memory at KERNBASE. 
	// Ie.  the VA range [KERNBASE, 2^32 - 1] should map to
	//      the PA range [0, 2^32 - 1 - KERNBASE]   
	// We might not have that many(ie. 2^32 - 1 - KERNBASE)    
	// bytes of physical memory.  But we just set up the mapping anyway.
	// Permissions: kernel RW, user NONE
	// Your code goes here: 
#if SOL >= 2
	boot_map_segment(pgdir, KERNBASE, -KERNBASE, 0, PTE_W);
#endif

	//////////////////////////////////////////////////////////////////////
	// Make 'pages' point to an array of size 'npage' of 'struct Page'.   
	// You must allocate this array yourself.
	// Map this array read-only by the user at virtual address UPAGES
	// (ie. perm = PTE_U | PTE_P)
	// Permissions:
	//    - pages -- kernel RW, user NONE
	//    - the image mapped at UPAGES  -- kernel R, user R
	// Your code goes here: 
#if SOL >= 2
	n = npage*sizeof(struct Page);
	pages = alloc(n, BY2PG, 1 /* clear it */);
	boot_map_segment(pgdir, UPAGES, n, PADDR(pages), PTE_U);
#endif

#if SOL >= 3
	//////////////////////////////////////////////////////////////////////
	// Make 'envs' point to an array of size 'NENV' of 'struct Env'.
	// Map this array read-only by the user at virtual address UENVS
	// (ie. perm = PTE_U | PTE_P)
	// Permissions:
	//    - envs itself -- kernel RW, user NONE
	//    - the image of envs mapped at UENVS  -- kernel R, user R
	n = NENV*sizeof(struct Env);
	envs = alloc(n, BY2PG, 1 /* clear it */);
	boot_map_segment(pgdir, UENVS, n, PADDR(envs), PTE_U);
#endif	// SOL >= 3

	check_boot_pgdir();

	//////////////////////////////////////////////////////////////////////
	// On x86, segmentation maps a VA to a LA (linear addr) and
	// paging maps the LA to a PA.  I.e. VA => LA => PA.  If paging is
	// turned off the LA is used as the PA.  Note: there is no way to
	// turn off segmentation.  The closest thing is to set the base
	// address to 0, so the VA => LA mapping is the identity.

	// Current mapping: VA KERNBASE+x => PA x.
	//     (segmentation base=-KERNBASE and paging is off)

	// From here on down we must maintain this VA KERNBASE + x => PA x
	// mapping, even though we are turning on paging and reconfiguring
	// segmentation.

	// Map VA 0:4MB same as VA KERNBASE, i.e. to PA 0:4MB.
	// (Limits our kernel to <4MB)
	pgdir[0] = pgdir[PDX(KERNBASE)];

	// Install page table.
	lcr3(boot_cr3);

	// Turn on paging.
	cr0 = rcr0();
	cr0 |= CR0_PE|CR0_PG|CR0_AM|CR0_WP|CR0_NE|CR0_TS|CR0_EM|CR0_MP;
	cr0 &= ~(CR0_TS|CR0_EM);
	lcr0(cr0);

	// Current mapping: KERNBASE+x => x => x.
	// (x < 4MB so uses paging pgdir[0])

	// Reload all segment registers.
	asm volatile("lgdt gdt_pd+2");
	asm volatile("movw %%ax,%%gs" :: "a" (GD_UD|3));
	asm volatile("movw %%ax,%%fs" :: "a" (GD_UD|3));
	asm volatile("movw %%ax,%%es" :: "a" (GD_KD));
	asm volatile("movw %%ax,%%ds" :: "a" (GD_KD));
	asm volatile("movw %%ax,%%ss" :: "a" (GD_KD));
	asm volatile("ljmp %0,$1f\n 1:\n" :: "i" (GD_KT));  // reload cs
	asm volatile("lldt %%ax" :: "a" (0));

	// Final mapping: KERNBASE+x => KERNBASE+x => x.

	// This mapping was only used after paging was turned on but
	// before the segment registers were reloaded.
	pgdir[0] = 0;

	// Flush the TLB for good measure, to kill the pgdir[0] mapping.
	lcr3(boot_cr3);
}

//
// Checks that the kernel part of virtual address space
// has been setup roughly correctly(by i386_vm_init()).
//
// This function doesn't test every corner case,
// in fact it doesn't test the permission bits at all,
// but it is a pretty good sanity check. 
//
static u_long va2pa(Pde *pgdir, u_long va);

static void
check_boot_pgdir(void)
{
	u_long i, n;
	Pde *pgdir;

	pgdir = boot_pgdir;

	// check pages array
	n = ROUND(npage*sizeof(struct Page), BY2PG);
	for(i=0; i<n; i+=BY2PG)
		assert(va2pa(pgdir, UPAGES+i) == PADDR(pages)+i);
	
#if LAB >= 3
	// check envs array
	n = ROUND(NENV*sizeof(struct Env), BY2PG);
	for(i=0; i<n; i+=BY2PG)
		assert(va2pa(pgdir, UENVS+i) == PADDR(envs)+i);
#endif

	// check phys mem
	for(i=0; KERNBASE+i != 0; i+=BY2PG)
		assert(va2pa(pgdir, KERNBASE+i) == i);

	// check kernel stack
	for(i=0; i<KSTKSIZE; i+=BY2PG)
		assert(va2pa(pgdir, KSTACKTOP-KSTKSIZE+i) == PADDR(bootstack)+i);

	// check for zero/non-zero in PDEs
	for (i = 0; i < PDE2PD; i++) {
		switch (i) {
		case PDX(VPT):
		case PDX(UVPT):
		case PDX(KSTACKTOP-1):
		case PDX(UPAGES):
#if LAB >= 3
		case PDX(UENVS):
#endif
			assert(pgdir[i]);
			break;
		default:
			if(i >= PDX(KERNBASE))
				assert(pgdir[i]);
			else
				assert(pgdir[i]==0);
			break;
		}
	}
	printf("check_boot_pgdir() succeeded!\n");
}

static u_long
va2pa(Pde *pgdir, u_long va)
{
	Pte *p;

	pgdir = &pgdir[PDX(va)];
	if (!(*pgdir&PTE_P))
		return ~0;
	p = (Pte*)KADDR(PTE_ADDR(*pgdir));
	if (!(p[PTX(va)]&PTE_P))
		return ~0;
	return PTE_ADDR(p[PTX(va)]);
}
		
// --------------------------------------------------------------
// Tracking of physical pages.
// --------------------------------------------------------------

static void page_initpp(struct Page *pp);

//  
// Initialize page structure and memory free list.
//
void
page_init(void)
{
#if SOL >= 2
	int i, inuse;

	LIST_INIT (&page_free_list);

	// Align freemem to page boundary.
	alloc(0, BY2PG, 0);

	for (i=0; i<npage; i++) {
		// Off-limits until proven otherwise.
		inuse = 1;

		// The bottom basemem bytes are free except page 0.
		if (i!=0 && i<basemem/BY2PG)
			inuse = 0;

		// The IO hole and the kernel abut.

		// The memory past the kernel is free.
		if (i >= (freemem-KERNBASE)/BY2PG)
			inuse = 0;

		pages[i].pp_ref = inuse;
		if (!inuse)
			LIST_INSERT_HEAD(&page_free_list, &pages[i], pp_link);
	}
	
#else /* not SOL >= 2 */
	// The exaple code here marks all pages as free.
	// However this is not truly the case.  What memory is free?
	//  1) Mark page 0 as in use(for good luck) 
	//  2) Mark the rest of base memory as free.
	//  3) Then comes the IO hole [IOPHYSMEM, EXTPHYSMEM) => mark it as in use
	//     So that it can never be allocated.      
	//  4) Then extended memory(ie. >= EXTPHYSMEM):
	//     ==> some of it's in use some is free. Where is the kernel?
	//     Which pages are used for page tables and other data structures?    
	//
	// Change the code to reflect this.
	int i;
	LIST_INIT (&page_free_list);
	for (i = 0; i < npage; i++) {
		pages[i].pp_ref = 0;
		LIST_INSERT_HEAD(&page_free_list, &pages[i], pp_link);
	}
#endif /* not SOL >= 2 */
}

//
// Initialize a Page structure.
//
static void
page_initpp(struct Page *pp)
{
	memset(pp, 0, sizeof(*pp));
}

//
// Allocates a physical page.
//
// *pp -- is set to point to the Page struct of the newly allocated
// page
//
// RETURNS 
//   0 -- on success
//   -E_NO_MEM -- otherwise 
//
// Hint: use LIST_FIRST, LIST_REMOVE, page_initpp()
// Hint: pp_ref should not be incremented 
int
page_alloc(struct Page **pp)
{
	// Fill this function in
#if SOL >= 2
	*pp = LIST_FIRST (&page_free_list);
	if (*pp) {
		LIST_REMOVE(*pp, pp_link);
		page_initpp(*pp);
		return 0;
	}

	//warn("page_alloc() can't find memory");
#endif /* not SOL >= 2 */
	return -E_NO_MEM;
}

//
// Return a page to the free list.
// (This function should only be called when pp->pp_ref reaches 0.)
//
void
page_free(struct Page *pp)
{
#if SOL >= 2
	if (pp->pp_ref) {
		warn("page_free: attempt to free mapped page");
		return;		/* be conservative and assume page is still used */
	}
	LIST_INSERT_HEAD(&page_free_list, pp, pp_link);
	pp->pp_ref = 0;
#else /* not SOL >= 2 */
	// Fill this function in
#endif /* not SOL >= 2 */
}

//
// Decrement the reference count on a page, freeing it if there are no more refs.
//
void
page_decref(struct Page *pp)
{
	if (--pp->pp_ref == 0)
		page_free(pp);
}

//
// This is boot_pgdir_walk with a different allocate function.
// Unlike boot_pgdir_walk, pgdir_walk can fail, so we have to
// return pte via a pointer parameter.
//
// Stores address of page table entry in *ppte.
// Stores 0 if there is no such entry or on error.
// 
// RETURNS: 
//   0 on success
//   -E_NO_MEM, if page table couldn't be allocated
//
int
pgdir_walk(Pde *pgdir, u_long va, int create, Pte **ppte)
{
#if SOL >= 2
	int r;
	struct Page *pp;
	Pde *pde;
	Pte *pgtab;

	*ppte = 0;
	pde = &pgdir[PDX(va)];
	if (*pde & PTE_P)
		pgtab = (Pte*)KADDR(PTE_ADDR(*pde));
	else {
		if (!create) {
			return 0;
		}
		if ((r = page_alloc(&pp)) < 0) {
			//warn("pgdir_walk: could not allocate page for va %lx", va);
			return r;
		}
		pp->pp_ref++;

		pgtab = (Pte*)page2kva(pp);

		// Make sure all those PTE_P bits are zero.
		memset(pgtab, 0, BY2PG);

		// The permissions here are overly generous, but they can
		// be further restricted by the permissions in the page table 
		// entries, if necessary.
		*pde = page2pa(pp) | PTE_P | PTE_W | PTE_U;
	}

	*ppte = &pgtab[PTX(va)];
	return 0;
#else /* not SOL >= 2 */
	// Fill this function in
#endif /* not SOL >= 2 */
}

//
// Map the physical page 'pp' at virtual address 'va'.
// The permissions (the low 12 bits) of the page table
//  entry should be set to 'perm|PTE_P'.
//
// Details
//   - If there is already a page mapped at 'va', it is page_remove()d.
//   - If necesary, on demand, allocates a page table and inserts it into 'pgdir'.
//   - pp->pp_ref should be incremented if the insertion succeeds
//
// RETURNS: 
//   0 on success
//   -E_NO_MEM, if page table couldn't be allocated
//
// Hint: The TA solution is implemented using
//   pgdir_walk() and and page_remove().
//
int
page_insert(Pde *pgdir, struct Page *pp, u_long va, u_int perm) 
{
#if SOL >= 2
	int r;
	Pte *pte;

	if ((r = pgdir_walk(pgdir, va, 1, &pte)) < 0)
		return r;

	// We must increment pp_ref before page_remove, so that
	// if pp is already mapped at va (we're just changing perm),
	// we don't lose the page when we decref in page_remove.
	pp->pp_ref++;

	if (*pte & PTE_P)
		page_remove(pgdir, va);

	*pte = page2pa(pp) | perm | PTE_P;
	return 0;
#else /* not SOL >= 2 */
	// Fill this function in
#endif /* not SOL >= 2 */
}

//
// Return the page mapped at virtual address 'va'.
// If ppte is not zero, then we store in it the address
// of the pte for this page.  This is used by page_remove
// but should not be used by other callers.
//
// Return 0 if there is no page mapped at va.
//
// Hint: the TA solution uses pgdir_walk and pa2page.
//
struct Page*
page_lookup(Pde *pgdir, u_long va, Pte **ppte)
{
#if SOL >= 2
	int r;
	Pte *pte;

	if ((r = pgdir_walk(pgdir, va, 0, &pte)) < 0)
		panic("pgdir_walk cannot fail now: %e", r);

	if (pte == 0 || *pte == 0)
		return 0;
	if (ppte)
		*ppte = pte;
	if (!(*pte & PTE_P) || PPN(PTE_ADDR(*pte)) >= npage) {
		warn("page_lookup: found bogus PTE 0x%08lx at pgdir %p va 0x%08lx",
			*pte, pgdir, va);
		return 0;
	}

	return pa2page(PTE_ADDR(*pte));
#else /* not SOL >= 2 */
	// Fill this function in
#endif /* not SOL >= 2 */
}

//
// Unmaps the physical page at virtual address 'va'.
//
// Details:
//   - The ref count on the physical page should decrement.
//   - The physical page should be freed if the refcount reaches 0.
//   - The pg table entry corresponding to 'va' should be set to 0.
//     (if such a PTE exists)
//   - The TLB must be invalidated if you remove an entry from
//	   the pg dir/pg table.
//
// Hint: The TA solution is implemented using page_lookup,
// 	tlb_invalidate, and page_decref.
//
void
page_remove(Pde *pgdir, u_long va) 
{
#if SOL >= 2
	struct Page *p;
	Pte *pte;

	if ((p = page_lookup(pgdir, va, &pte)) == 0)
		return;
	*pte = 0;
	tlb_invalidate(pgdir, va);
	page_decref(p);
#else /* not SOL >= 2 */
	// Fill this function in
#endif /* not SOL >= 2 */
}

//
// Invalidate a TLB entry, but only if the page tables being
// edited are the ones currently in use by the processor.
//
void
tlb_invalidate(Pde *pgdir, u_long va)
{
	// Flush the entry only if we're modifying the current address space.
#if LAB >= 4
	if (!curenv || curenv->env_pgdir == pgdir)
		invlpg(va);
#else
	// For now, there is only one address space, so always invalidate.
	invlpg(va);
#endif
}

void
page_check(void)
{
	struct Page *pp, *pp0, *pp1, *pp2;
	struct Page_list fl;

	// should be able to allocate three pages
	pp0 = pp1 = pp2 = 0;
	assert(page_alloc(&pp0) == 0);
	assert(page_alloc(&pp1) == 0);
	assert(page_alloc(&pp2) == 0);

	assert(pp0);
	assert(pp1 && pp1 != pp0);
	assert(pp2 && pp2 != pp1 && pp2 != pp0);

	// temporarily steal the rest of the free pages
	fl = page_free_list;
	LIST_INIT(&page_free_list);

	// should be no free memory
	assert(page_alloc(&pp) == -E_NO_MEM);

	// there is no free memory, so we can't allocate a page table 
	assert(page_insert(boot_pgdir, pp1, 0x0, 0) < 0);

	// free pp0 and try again: pp0 should be used for page table
	page_free(pp0);
	assert(page_insert(boot_pgdir, pp1, 0x0, 0) == 0);
	assert(PTE_ADDR(boot_pgdir[0]) == page2pa(pp0));
	assert(va2pa(boot_pgdir, 0x0) == page2pa(pp1));
	assert(pp1->pp_ref == 1);
	assert(pp0->pp_ref == 1);

	// should be able to map pp2 at BY2PG because pp0 is already allocated for page table
	assert(page_insert(boot_pgdir, pp2, BY2PG, 0) == 0);
	assert(va2pa(boot_pgdir, BY2PG) == page2pa(pp2));
	assert(pp2->pp_ref == 1);

	// should be no free memory
	assert(page_alloc(&pp) == -E_NO_MEM);

	// should be able to map pp2 at BY2PG because it's already there
	assert(page_insert(boot_pgdir, pp2, BY2PG, 0) == 0);
	assert(va2pa(boot_pgdir, BY2PG) == page2pa(pp2));
	assert(pp2->pp_ref == 1);

	// pp2 should NOT be on the free list
	// could happen in ref counts are handled sloppily in page_insert
	assert(page_alloc(&pp) == -E_NO_MEM);

	// should not be able to map at PDMAP because need free page for page table
	assert(page_insert(boot_pgdir, pp0, PDMAP, 0) < 0);

	// insert pp1 at BY2PG (replacing pp2)
	assert(page_insert(boot_pgdir, pp1, BY2PG, 0) == 0);

	// should have pp1 at both 0 and BY2PG, pp2 nowhere, ...
	assert(va2pa(boot_pgdir, 0x0) == page2pa(pp1));
	assert(va2pa(boot_pgdir, BY2PG) == page2pa(pp1));
	// ... and ref counts should reflect this
	assert(pp1->pp_ref == 2);
	assert(pp2->pp_ref == 0);

	// pp2 should be returned by page_alloc
	assert(page_alloc(&pp) == 0 && pp == pp2);

	// unmapping pp1 at 0 should keep pp1 at BY2PG
	page_remove(boot_pgdir, 0x0);
	assert(va2pa(boot_pgdir, 0x0) == ~0);
	assert(va2pa(boot_pgdir, BY2PG) == page2pa(pp1));
	assert(pp1->pp_ref == 1);
	assert(pp2->pp_ref == 0);

	// unmapping pp1 at BY2PG should free it
	page_remove(boot_pgdir, BY2PG);
	assert(va2pa(boot_pgdir, 0x0) == ~0);
	assert(va2pa(boot_pgdir, BY2PG) == ~0);
	assert(pp1->pp_ref == 0);
	assert(pp2->pp_ref == 0);

	// so it should be returned by page_alloc
	assert(page_alloc(&pp) == 0 && pp == pp1);

	// should be no free memory
	assert(page_alloc(&pp) == -E_NO_MEM);

	// forcibly take pp0 back
	assert(PTE_ADDR(boot_pgdir[0]) == page2pa(pp0));
	boot_pgdir[0] = 0;
	assert(pp0->pp_ref == 1);
	pp0->pp_ref = 0;

	// give free list back
	page_free_list = fl;

	// free the pages we took
	page_free(pp0);
	page_free(pp1);
	page_free(pp2);

	printf("page_check() succeeded!\n");
}

#endif /* LAB >= 2 */
