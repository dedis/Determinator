///LAB2
/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/mmu.h>
#include <inc/error.h>
#include <kern/pmap.h>
#include <kern/env.h>
#include <kern/printf.h>
#include <kern/kclock.h>

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
	// 0x0 - unused(always faults--for trapping NULL far pointers)
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
	basemem = ROUNDDOWN(nvram_read(NVRAM_BASELO)*1024, PGSIZE);
	extmem = ROUNDDOWN(nvram_read(NVRAM_EXTLO)*1024, PGSIZE);

	// Calculate the maxmium physical address based on whether
	// or not there is any extended memory.  See comment in ../inc/mmu.h.
	if (extmem)
		maxpa = EXTPHYSMEM + extmem;
	else
		maxpa = basemem;

	npage = maxpa / PGSIZE;

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

///SOL2
	freemem = ROUND(freemem, align);
	if (freemem+n < freemem || freemem+n > KERNBASE+maxpa)
		panic("out of memory during i386_vm_init");
	v = (void*)freemem;
	freemem += n;
	if (clear)
		bzero(v, n);
	return v;
///END
}

//
// Given pgdir, the current page directory base,
// walk the 2-level page table structure to find
// the Pte for virtual address va.  Return a pointer to it.
//
// If there is no such directory page:
//	- if create == 0, return 0.
//	- otherwise allocate a new directory page and install it.
//
// Boot_pgdir_walk cannot fail.  It's too early to fail.
// 
static Pte*
boot_pgdir_walk(Pde *pgdir, u_long va, int create)
{
///SOL2
	Pde *pde;
	Pte *pgtab;

	pde = &pgdir[PDX(va)];
	if (*pde & PTE_P)
		pgtab = (Pte*)KADDR(PTE_ADDR(*pde));
	else {
		if (!create)
			return 0;
		pgtab = alloc(PGSIZE, PGSIZE, 1);
		*pde = PADDR(pgtab)|PTE_P|PTE_W;
	}
	return &pgtab[PTX(va)];
///END
}

//
// Map [va, va+size) of virtual address space to physical [pa, pa+size)
// in the page table rooted at pgdir.  Size is a multiple of PGSIZE.
// Use permission bits perm|PTE_P for the entries.
//
static void
boot_map_segment(Pde *pgdir, u_long va, u_long size, u_long pa, int perm)
{
///SOL2
	u_long i;

	for (i=0; i<size; i+=PGSIZE)
		*boot_pgdir_walk(pgdir, va+i, 1) = (pa+i)|perm|PTE_P;
///END
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

	//////////////////////////////////////////////////////////////////////
	// create initial page directory.
	pgdir = alloc(PGSIZE, PGSIZE, 1);
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

///SOL2
///ELSE
	printf("\n");
	panic("i386_vm_init: This function is not finished\n");
///END

	//////////////////////////////////////////////////////////////////////
	// Map the kernel stack:
	//   [KSTACKTOP-PDMAP, KSTACKTOP)  -- the complete VA range of the stack
	//     * [KSTACKTOP-KSTKSIZE, KSTACKTOP) -- backed by physical memory
	//     * [KSTACKTOP-PDMAP, KSTACKTOP-KSTKSIZE) -- not backed => faults
	//   Permissions: kernel RW, user NONE
	// Your code goes here:
///SOL2
	boot_map_segment(pgdir, KSTACKTOP-KSTKSIZE, KSTKSIZE,
		PADDR(bootstack), PTE_W);
///END

	//////////////////////////////////////////////////////////////////////
	// Map all of physical memory at KERNBASE. 
	// Ie.  the VA range [KERNBASE, 2^32 - 1] should map to
	//      the PA range [0, 2^32 - 1 - KERNBASE]   
	// We might not have that many(ie. 2^32 - 1 - KERNBASE)    
	// bytes of physical memory.  But we just set up the mapping anyway.
	// Permissions: kernel RW, user NONE
	// Your code goes here: 
///SOL2
	boot_map_segment(pgdir, KERNBASE, -KERNBASE, 0, PTE_W);
///END

	//////////////////////////////////////////////////////////////////////
	// Make 'pages' point to an array of size 'npage' of 'struct Page'.   
	// Map this array read-only by the user at UPAGES (ie. perm = PTE_U | PTE_P)
	// Permissions:
	//    - pages -- kernel RW, user NONE
	//    - the image mapped at UPAGES  -- kernel R, user R
	// Your code goes here: 
///SOL2
	n = npage*sizeof(struct Page);
	pages = alloc(n, PGSIZE, 1);
	boot_map_segment(pgdir, UPAGES, n, PADDR(pages), PTE_U);
///END

	//////////////////////////////////////////////////////////////////////
	// Make '__envs' point to an array of size 'NENV' of 'struct Env'.
	// Map this array read-only by the user at UENVS (ie. perm = PTE_U | PTE_P)
	// Permissions:
	//    - __envs -- kernel RW, user NONE
	//    - the image mapped at UENVS  -- kernel R, user R
	// Your code goes here: 
///SOL2
	n = NENV*sizeof(struct Env);
	envs = alloc(n, PGSIZE, 1);
	boot_map_segment(pgdir, UENVS, n, PADDR(envs), PTE_U);
///END

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
	asm volatile("lgdt _gdt_pd+2");
	asm volatile("movw %%ax,%%gs" :: "a" (GD_UD|3));
	asm volatile("movw %%ax,%%fs" :: "a" (GD_UD|3));
	asm volatile("movw %%ax,%%es" :: "a" (GD_KD));
	asm volatile("movw %%ax,%%ds" :: "a" (GD_KD));
	asm volatile("movw %%ax,%%ss" :: "a" (GD_KD));
	asm volatile("ljmp %0,$1f\n 1:\n" :: "i" (GD_KT));  // reload cs
	asm volatile("lldt %0" :: "m" (0));

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
static Pte getent(Pde *pgdir, u_long va);

static void
check_boot_pgdir(void)
{
#if 0
	extern char bootstacktop;
	u_int nbytes;
	int i;
	Pte *pt;
	u_int ptr;
	
	// check final value of freemem
	assert((u_int)freemem == (u_int)&__envs[NENV]);
	
	// check __env array and its pg tbl
	ptr = (u_int)&__envs[0];
	pt = (Pte*)(ptr - NBPG); // __env's pgtbl resides right before it
	nbytes = PGROUNDUP (NENV * sizeof(struct Env));
	for (i = 0; i < nbytes / NBPG; i++, ptr += NBPG)
		assert((pt[i] & ~PGMASK) == kva2pa(ptr));

	// check page array and its pg tbl
	ptr = (u_int)&pages[0];
	pt = (Pte*)(ptr - NBPG); // page's pgtbl resides right before it
	nbytes = PGROUNDUP (npage * sizeof(struct Page));
	for (i = 0; i < nbytes / NBPG; i++, ptr += NBPG)
		assert((pt[i] & ~PGMASK) == kva2pa(ptr));

	// check the pgtbls that map phys memory
	pt = (Pte *)((u_int)pt - NBPG * NPPT);
	for (i = 0; i < NPPT * NLPG; i++)
		assert(pt[i] >> PGSHIFT == i);

	// check the pgtbl for the kernel stack
	for (i = 1; i <= KSTKSIZE/NBPG; i++) {
		ptr = (u_int)ptov(pt[-i] & ~PGMASK);
		assert(ptr < (u_int)&bootstacktop && ptr >= (u_int)&bootstacktop - KSTKSIZE);
	}

	// check the boot pgdir
	pt = (Pte *)((u_int)pt - 2*NBPG);
	assert((u_int)pt == (u_int)p0pgdir_boot);
	
	// check for zero in certain PDEs 
	for (i = 0; i < NLPG; i++) {
		if (i >= PDENO(KERNBASE))  assert(p0pgdir_boot[i]);
		else if (i == PDENO(VPT)) assert(p0pgdir_boot[i]);
		else if (i == PDENO(UVPT)) assert(p0pgdir_boot[i]);
		else if (i == PDENO(KSTACKTOP-NBPD)) assert(p0pgdir_boot[i]);
		else if (i == PDENO(UPPAGES)) assert(p0pgdir_boot[i]);
		else if (i == PDENO(UENVS)) assert(p0pgdir_boot[i]);
		else assert(p0pgdir_boot[i] == 0);
	}
#endif
	printf("check_boot_page_directory() succeeded!\n");
}

static Pte
getent(Pde *pgdir, u_long va)
{
	Pte *p;

	pgdir = &pgdir[PDX(va)];
	if (!(*pgdir&PTE_P))
		return 0;
	p = (Pte*)KADDR(PTE_ADDR(*pgdir));
	return p[PTX(va)];
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
///SOL2
	int i, inuse;

	LIST_INIT (&page_free_list);

	// Align freemem to page boundary.
	alloc(0, PGSIZE, 0);

	for (i=0; i<npage; i++) {
		// Off-limits until proven otherwise.
		inuse = 1;

		// The bottom basemem bytes are free except page 0.
		if (i!=0 && i<basemem/PGSIZE)
			inuse = 0;

		// The IO hole and the kernel abut.

		// The memory past the kernel is free.
		if (i >= (freemem-KERNBASE)/PGSIZE)
			inuse = 0;

		pages[i].pp_ref = inuse;
		if (!inuse)
			LIST_INSERT_HEAD(&page_free_list, &pages[i], pp_link);
	}
	
///ELSE
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
///END
}

//
// Initialize a Page structure.
//
static void
page_initpp(struct Page *pp)
{
	bzero(pp, sizeof(*pp));
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
///SOL2
	*pp = LIST_FIRST (&page_free_list);
	if (*pp) {
		LIST_REMOVE(*pp, pp_link);
		page_initpp(*pp);
		return 0;
	}

	warn("page_alloc() can't find memory");
///END
	return -E_NO_MEM;
}

//
// Return a page to the free list.
// (This function should only be called when pp->pp_ref reaches 0.)
//
void
page_free(struct Page *pp)
{
	// Fill this function in
///SOL2
	if (pp->pp_ref) {
		warn("page_free: attempt to free mapped page");
		return;		/* be conservative and assume page is still used */
	}
	LIST_INSERT_HEAD(&page_free_list, pp, pp_link);
	pp->pp_ref = 0;
///END
}

//
// This is boot_pgdir_walk with a different allocate function.
// Unlike boot_pgdir_walk, pgdir_walk can fail, so we have to
// return pte via a pointer parameter.
//
// Stores address of page table entry in *pte.
// Stores 0 if there is no such entry or on error.
// 
// RETURNS: 
//   0 on success
//   -E_NO_MEM, if page table couldn't be allocated
//
int
pgdir_walk(Pde *pgdir, u_long va, int create, Pte **ppte)
{
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
			warn("pgdir_walk: could not allocate page for va %lx", va);
			return r;
		}
		pgtab = (Pte*)page2kva(pp);

		// Make sure all those PTE_P bits are zero.
		bzero(pgtab, PGSIZE);

		// The permissions here are overly generous, but they can
		// be further restricted by the permissions in the page table 
		// entries, if necessary.
		*pde = page2pa(pp) | PTE_P | PTE_W | PTE_U;
	}

	*ppte = &pgtab[PTX(va)];
	return 0;
}

//
// Map the physical page 'pp' at virtual address 'va'.
// The permissions (the low 12 bits) of the page table
//  entry should be set to 'perm'.
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
///SOL2
	int r;
	Pte *pte;

	if ((r = pgdir_walk(pgdir, va, 1, &pte)) < 0)
		return r;

	if (*pte & PTE_P)
		page_remove(pgdir, va);

	pp->pp_ref++;
	*pte = page2pa(pp) | perm;
	return 0;
///ELSE
	// Fill this function in
///END
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
// Hint: The TA solution is implemented using
//  pgdir_walk(), page_free(), tlb_invalidate()
//
void
page_remove(Pde *pgdir, u_long va) 
{
///SOL2
	int r;
	struct Page *pp;
	Pte *pte;

	// Find the PTE and unmap the page.
	if ((r = pgdir_walk(pgdir, va, 0, &pte)) < 0)
		panic("pgdir_walk cannot fail when not creating");

	// Page is not mapped?  No work to do.
	if (pte == 0)
		return;

	// Sanity check: should be mapped and a valid entry.
	if (!(*pte & PTE_P) || PPN(PTE_ADDR(*pte)) >= npage) {
		warn("page_remove: found bogus PTE 0x%08lx at pgdir %p va 0x%08lx",
			*pte, pgdir, va);
		return;
	}

	// Decref page.
	pp = &pages[PPN(PTE_ADDR(*pte))];
	if (--pp->pp_ref == 0)
		page_free(pp);

	*pte = 0;
	tlb_invalidate(pgdir, va);
///ELSE	
	// Fill this function in
///END
}

//
// xxx
//
void
tlb_invalidate(Pde *pgdir, u_long va)
{
	// Flush the entry only if we're modifying the current address space.
	if (!curenv || curenv->env_pgdir == pgdir)
		invlpg(va);
}

void
page_check(void)
{
#if 0
	int r;
	struct Page *pp;
	struct Page *pp1;
	struct Page *pp2;
	// XXX
	// This alloc_list idea is dumb and it complicates
	// the code!
	//
	// Just reference page directly &pages[ppn].
	//

	struct Page_list alloc_list;
	LIST_INIT (&alloc_list);

	while (1) {
		r = page_alloc(&pp);
		if (r < 0)
			break;
		LIST_INSERT_HEAD(&alloc_list, pp, pp_link);

		// must not return an in use page
		assert(pp->pp_ref == 0);
		assert((u_int)pp2va(pp) < (u_int)IOPHYSMEM + KERNBASE
						|| (u_int)pp2va(pp) >= (u_int)freemem);
	}


	pp = LIST_FIRST (&alloc_list);   
	pp1 = LIST_NEXT (pp, pp_link);
	pp2 = LIST_NEXT (pp1, pp_link);
	LIST_REMOVE (pp, pp_link);
	LIST_REMOVE (pp1, pp_link);
	LIST_REMOVE (pp2, pp_link);

	// there is no free memory, so we can't allocate a page table 
	assert(page_insert(p0pgdir_boot, pp, 0x0, PTE_P) < -1);
	page_free(pp);

	assert(page_insert(p0pgdir_boot, pp1, 0x0, PTE_P) == 0);
	// expect: pp1 mapped 0x0
	assert(pgdir_get_ptep(p0pgdir_boot, 0x0));
	assert(*pgdir_get_ptep(p0pgdir_boot, 0x0) >> PGSHIFT == pp2ppn(pp1));

	assert(page_insert(p0pgdir_boot, pp2, NBPG, PTE_P) == 0);
	// expect: pp1 mapped 0x0, pp2 at NBPG
	assert(pgdir_get_ptep(p0pgdir_boot, 0x0));
	assert(*pgdir_get_ptep(p0pgdir_boot, 0x0) >> PGSHIFT == pp2ppn(pp1));
	assert(pgdir_get_ptep(p0pgdir_boot, NBPG));
	assert(*pgdir_get_ptep(p0pgdir_boot, NBPG) >> PGSHIFT == pp2ppn(pp2));
	assert(pp1->pp_ref == 1);
	assert(pp2->pp_ref == 1);

	// insert pp1 at VA NBPG (pp2 is already mapped there)
	assert(page_insert(p0pgdir_boot, pp1, NBPG, PTE_P) == 0);
	// expect: pp1 mapped at both 0x0 and NBPG, pp2 not mapped
	assert(pgdir_get_ptep(p0pgdir_boot, 0x0));
	assert(*pgdir_get_ptep(p0pgdir_boot, 0x0) >> PGSHIFT == pp2ppn(pp1));
	assert(pgdir_get_ptep(p0pgdir_boot, NBPG));
	assert(*pgdir_get_ptep(p0pgdir_boot, NBPG) >> PGSHIFT == pp2ppn(pp1));
	assert(pp1->pp_ref == 2);
	assert(pp2->pp_ref == 0);
	// alloc should return pp2, since it should be the only thing on the page_free_list
	assert(page_alloc(&pp) == 0);
	assert(pp == pp2);

	page_remove(p0pgdir_boot, 0x0);
	// expect: pp1 mapped at NBPG
	assert(pgdir_get_ptep(p0pgdir_boot, 0x0));
	assert(*pgdir_get_ptep(p0pgdir_boot, 0x0) == 0);
	assert(pgdir_get_ptep(p0pgdir_boot, NBPG));
	assert(*pgdir_get_ptep(p0pgdir_boot, NBPG) >> PGSHIFT == pp2ppn(pp1));
	assert(pp1->pp_ref == 1);
	assert(pp2->pp_ref == 0);

	page_remove(p0pgdir_boot, NBPG);
	// expect: pp1 not mapped
	assert(pgdir_get_ptep(p0pgdir_boot, 0x0));
	assert(*pgdir_get_ptep(p0pgdir_boot, 0x0) == 0);
	assert(pgdir_get_ptep(p0pgdir_boot, NBPG));
	assert(*pgdir_get_ptep(p0pgdir_boot, NBPG) == 0);
	assert(pp1->pp_ref == 0);
	assert(pp2->pp_ref == 0);

	// return all remaining memory to the free list
	page_free(pp);
	while (1) {
		pp = LIST_FIRST (&alloc_list);
		if (!pp)
			break;
		LIST_REMOVE (pp, pp_link);
		page_free(pp);
	}
#endif
	printf("page_check() succeeded!\n");
}

///END
