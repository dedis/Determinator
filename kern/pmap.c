#if LAB >= 3
/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/mmu.h>
#include <inc/gcc.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/syscall.h>

#include <kern/cpu.h>
#include <kern/trap.h>
#include <kern/proc.h>
#include <kern/pmap.h>


// Statically allocated page directory mapping the kernel's address space.
// We use this as a template for all pdirs for user-level processes.
pde_t pmap_bootpdir[1024] gcc_aligned(PAGESIZE);

// Statically allocated page that we always keep set to all zeros.
uint8_t pmap_zeros[PAGESIZE] gcc_aligned(PAGESIZE);


// --------------------------------------------------------------
// Set up initial memory mappings and turn on MMU.
// --------------------------------------------------------------



// Set up a two-level page table:
//    boot_pdir is its linear (virtual) address of the root
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
pmap_init(void)
{
	if (cpu_onboot()) {
		// Initialize the bootstrap page directory
		// to translate all virtual addresses from 0 to 2GB
		// directly to the same physical addresses,
		// representing the kernel's address space.
		// The easiest way to do this is to use 4MB page mappings.
		// Since these page mappings never change on context switches,
		// we can also mark them global (PTE_G) so the processor
		// doesn't flush these mappings when we reload the PDBR.
#if SOL >= 3
		int i;
		for (i = 0; i < NPDENTRIES; i++) {
			if (i == PDX(PMAP_USERLO))	// Skip user area
				i = PDX(PMAP_USERHI);
			pmap_bootpdir[i] = (i << PDXSHIFT)
				| PTE_P | PTE_W | PTE_PS | PTE_G;
		}
#else
		panic("pmap_init() not implemented");
#endif
	}

	// On x86, segmentation maps a VA to a LA (linear addr) and
	// paging maps the LA to a PA.  i.e., VA => LA => PA.  If paging is
	// turned off the LA is used as the PA.  There is no way to
	// turn off segmentation.  At the moment we turn on paging,
	// the code we're executing must be in an identity-mapped memory area
	// where LA == PA according to the page mapping structures.
	// In PIOS this is always the case for the kernel's address space,
	// so we don't have to play any special tricks as in other kernels.

	// Enable 4MB pages and global pages.
	uint32_t cr4 = rcr4();
	cr4 |= CR4_PSE | CR4_PGE;
	lcr4(cr4);

	// Install the bootstrap page directory into the PDBR.
	lcr3(mem_phys(pmap_bootpdir));

	// Turn on paging.
	uint32_t cr0 = rcr0();
	cr0 |= CR0_PE|CR0_PG|CR0_AM|CR0_WP|CR0_NE|CR0_TS|CR0_MP;
	cr0 &= ~(CR0_TS|CR0_EM);
	lcr0(cr0);
}


// Given 'pdir', a pointer to a page directory, pmap_walk returns
// a pointer to the page table entry (PTE) for user virtual address 'va'.
// This requires walking the two-level page table structure.
//
// If the relevant page table doesn't exist in the page directory, then:
//    - If create == 0, pmap_walk returns NULL.
//    - Otherwise, pmap_walk tries to allocate a new page table
//	with mem_alloc.  If this fails, pmap_walk returns NULL.
//    - The new page table is cleared and its refcount set to 1.
//    - Finally, pmap_walk returns a pointer to the requested entry
//	within the new page table.
//
// Hint: you can turn a pageinfo pointer into the physical address of the
// page it refers to with mem_pi2phys() from kern/mem.h.
//
// Hint 2: the x86 MMU checks permission bits in both the page directory
// and the page table, so it's safe to leave some page permissions
// more permissive than strictly necessary.
pte_t *
pmap_walk(pde_t *pdir, uint32_t va, int create)
{
#if SOL >= 3
	assert(va >= PMAP_USERLO && va < PMAP_USERHI);

	uint32_t la = va;			// linear = virtual address
	pde_t *pde = &pdir[PDX(la)];		// find PDE
	pte_t *ptab;
	if (*pde & PTE_P) {			// ptab already exist?
		ptab = mem_ptr(PGADDR(*pde));
	} else {				// no - create?
		pageinfo *pi;
		if (!create || (pi = mem_alloc()) == NULL)
			return NULL;
		mem_incref(pi);
		ptab = mem_pi2ptr(pi);

		// Make sure all those PTE_P bits are zero.
		memset(ptab, 0, PAGESIZE);

		// The permissions here are overly generous, but they can
		// be further restricted by the permissions in the page table 
		// entries, if necessary.
		*pde = mem_pi2phys(pi) | PTE_P | PTE_W | PTE_U;
	}

	return &ptab[PTX(la)];
#else /* not SOL >= 3 */
	// Fill in this function
	return NULL;
#endif /* not SOL >= 3 */
}

//
// Map the physical page 'pi' at user virtual address 'va'.
// The permissions (the low 12 bits) of the page table
//  entry should be set to 'perm | PTE_P'.
//
// Requirements
//   - If there is already a page mapped at 'va', it should be pmap_remove()d.
//   - If necessary, allocate a page able on demand and insert into 'pdir'.
//   - pi->refcount should be incremented if the insertion succeeds.
//   - The TLB must be invalidated if a page was formerly present at 'va'.
//
// Corner-case hint: Make sure to consider what happens when the same 
// pi is re-inserted at the same virtual address in the same pdir.
//
// RETURNS: 
//   a pointer to the inserted PTE on success (same as pmap_walk)
//   NULL, if page table couldn't be allocated
//
// Hint: The reference solution uses pmap_walk, pmap_remove, and mem_pi2phys.
//
pte_t *
pmap_insert(pde_t *pdir, pageinfo *pi, uint32_t va, int perm)
{
#if SOL >= 3
	pte_t* pte = pmap_walk(pdir, va, 1);
	if (pte == NULL)
		return NULL;

	// We must increment pi->refcount before pmap_remove, so that
	// if pi is already mapped at va (we're just changing perm),
	// we don't lose the page when we decref in pmap_remove.
	mem_incref(pi);

	// Now remove the old mapping in this PTE.
	if (*pte & PTE_P)
		pmap_remove(pdir, va, PAGESIZE);

	*pte = mem_pi2phys(pi) | perm | PTE_P;
	return pte;
#else /* not SOL >= 3 */
	// Fill in this function
	return NULL;
#endif /* not SOL >= 3 */
}

//
// Return the pageinfo for the page mapped at user virtual address 'va'.
// If pte_store is not zero, then also store in it the address
// of the pte for this page.  This is used by pmap_remove and
// can be used to verify page permissions for syscall arguments,
// but should not be used by most callers.
//
// Return NULL if there is no page mapped at va.
//
// Hint: the TA solution uses pmap_walk and mem_phys2pi.
//
pageinfo *
pmap_lookup(pde_t *pdir, uint32_t va, pte_t **pte_store)
{
#if SOL >= 3
	pte_t *pte = pmap_walk(pdir, va, 0);
	if (pte == 0 || *pte == 0)
		return NULL;
	if (pte_store)
		*pte_store = pte;
	if (!(*pte & PTE_P) || PPN(PGADDR(*pte)) >= mem_npage) {
		warn("pmap_lookup: found bogus PTE 0x%08lx at pdir %p va %p",
			*pte, pdir, va);
		return NULL;
	}

	return mem_phys2pi(PGADDR(*pte));
#else /* not SOL >= 3 */
	// Fill in this function
	return NULL;
#endif /* not SOL >= 3 */
}

//
// Unmap the physical page at user virtual address 'va'.
// If there is no mapping at that address, silently does nothing.
//
// Details:
//   - The ref count on the physical page should decrement.
//   - The physical page should be freed if the refcount reaches 0.
//   - The page table entry corresponding to 'va' should be set to 0.
//     (if such a PTE exists)
//   - The TLB must be invalidated if you remove an entry from
//     the pdir/ptab.
//
// Hint: The TA solution is implemented using pmap_lookup,
// 	pmap_tlbinvl, and mem_decref.
//
void
pmap_remove(pde_t *pdir, uint32_t va, size_t size)
{
#if SOL >= 3
	assert(PGOFF(size) == 0);	// must be page-aligned
	assert(va >= PMAP_USERLO && va < PMAP_USERHI);
	assert(size <= PMAP_USERHI - va);

	pmap_inval(pdir, va, size);	// invalidate region we're removing

	uint32_t vahi = va + size;
	while (va < vahi) {
		pde_t *pde = &pdir[PDX(va)];		// find PDE

		pte_t *pte = pmap_walk(pdir, va, 0);	// find PTE
		if (pte == NULL) {	// no page table - skip 4MB region
			va = PTADDR(va + PTSIZE);	// start of next ptab
			continue;
		}

		// Are we removing the entire page table?
		bool wholeptab = (PTX(va) == 0 && PTX(size) == 0);

		// Remove page mappings up to end of region or page table
		do {
			mem_decref(mem_phys2pi(PGADDR(*pte)));
			*pte++ = 0;
			va += PAGESIZE;
		} while (va < vahi && PTX(va) != 0);

		// Free the page table too if appropriate
		if (wholeptab) {
			mem_decref(mem_phys2pi(PGADDR(*pde)));
			*pde = 0;
		}
	}
#else /* not SOL >= 3 */
	// Fill in this function
#endif /* not SOL >= 3 */
}

//
// Invalidate the TLB entry or entries for a given virtual address range,
// but only if the page tables being edited are the ones
// currently in use by the processor.
//
void
pmap_inval(pde_t *pdir, uint32_t va, size_t size)
{
	// Flush the entry only if we're modifying the current address space.
	proc *p = proc_cur();
	if (p->pdir == pdir) {
		if (size == PAGESIZE)
			invlpg(mem_ptr(va));	// invalidate one page
		else
			lcr3(mem_phys(pdir));	// invalidate everything
	}
}

//
// Virtually copy a range of pages from spdir to dpdir (could be the same).
// Uses copy-on-write to avoid the cost of immediate copying:
// instead just copies the mappings and makes both source and dest read-only.
// Returns true if successfull, false if not enough memory for copy.
//
int
pmap_copy(pde_t *spdir, uint32_t sva, pde_t *dpdir, uint32_t dva,
		size_t size)
{
#if SOL >= 3
	assert(PTOFF(sva) == 0);	// must be 4MB-aligned
	assert(PTOFF(dva) == 0);
	assert(PTOFF(size) == 0);
	assert(sva >= PMAP_USERLO && sva < PMAP_USERHI);
	assert(dva >= PMAP_USERLO && dva < PMAP_USERHI);
	assert(size <= PMAP_USERHI - sva);
	assert(size <= PMAP_USERHI - dva);

	// Invalidate both regions we're modifying
	pmap_inval(spdir, sva, size);
	pmap_inval(dpdir, dva, size);

	uint32_t svahi = sva + size;
	pde_t *spde = &spdir[PDX(sva)];
	pde_t *dpde = &spdir[PDX(sva)];
	while (sva < svahi) {

		if (*dpde & PTE_P)	// remove old ptable first
			pmap_remove(dpdir, dva, PTSIZE);
		assert(*dpde == 0);

		if (*spde & PTE_W)	// remove write permission
			*spde &= ~PTE_W;

		*dpde = *spde;		// copy ptable mapping
		mem_incref(mem_phys2pi(PGADDR(*spde)));

		spde++, dpde++;
		sva += PTSIZE;
		dva += PTSIZE;
	}
	return 1;
#else /* not SOL >= 3 */
	panic("pmap_copy() not implemented");
#endif /* not SOL >= 3 */
}

//
// Helper function for pmap_merge: merge a single memory page
// that has been modified in both the source and destination.
// If conflicting writes to a single byte are detected on the page,
// print a warning to the console and remove the page from the destination.
//
static void
pmap_mergepage(pte_t *rpte, pte_t *spte, pte_t *dpte, uint32_t dva)
{
#if SOL >= 3
	uint8_t *rpg = (uint8_t*)PGADDR(*rpte);
	if (rpg == NULL) rpg = pmap_zeros;

	uint8_t *spg = (uint8_t*)PGADDR(*spte);
	if (spg == NULL) spg = pmap_zeros;

	uint8_t *dpg = (uint8_t*)PGADDR(*dpte);
	if (dpg == NULL) return;	// Conflict - just leave dest unmapped

	int i;
	for (i = 0; i < PAGESIZE; i++) {
		if (spg[i] == rpg[i])
			continue;	// unchanged in source - leave dest
		if (dpg[i] == rpg[i]) {
			dpg[i] = spg[i];	// unchanged in dest - use src
			continue;
		}

		cprintf("pmap_mergepage: conflict at dva %x\n", dva);
		mem_decref(mem_phys2pi(PGADDR(*dpte)));
		*dpte = 0;
		return;
	}
#else /* not SOL >= 3 */
	panic("pmap_mergepage() not implemented");
#endif /* not SOL >= 3 */
}

// 
// Merge differences between a reference snapshot represented by rpdir
// and a source address space spdir into a destination address space dpdir.
//
int
pmap_merge(pde_t *rpdir, pde_t *spdir, uint32_t sva,
		pde_t *dpdir, uint32_t dva, size_t size)
{
#if SOL >= 3
	assert(PTOFF(sva) == 0);	// must be 4MB-aligned
	assert(PTOFF(dva) == 0);
	assert(PTOFF(size) == 0);
	assert(sva >= PMAP_USERLO && sva < PMAP_USERHI);
	assert(dva >= PMAP_USERLO && dva < PMAP_USERHI);
	assert(size <= PMAP_USERHI - sva);
	assert(size <= PMAP_USERHI - dva);

	// Invalidate the destination region we'll be modifying
	pmap_inval(dpdir, dva, size);

	pde_t *rpde = &rpdir[PDX(sva)];		// find PDEs
	pde_t *spde = &spdir[PDX(sva)];
	pde_t *dpde = &dpdir[PDX(dva)];
	uint32_t svahi = sva + size;
	for (; sva < svahi; rpde++, spde++, dpde++,
				sva += PTSIZE, dva += PTSIZE) {

		if (*spde == *rpde)	// unchanged in source - do nothing
			continue;
		if (*dpde == *rpde) {	// unchanged in dest - copy from source
			if (!pmap_copy(spdir, sva, dpdir, dva, PTSIZE))
				return 0;
			continue;
		}

		// Find each of the page tables from the corresponding PDEs
		pte_t *rpte = (*rpde & PTE_P)
			? mem_ptr(PGADDR(*rpde)) : (pte_t *) pmap_zeros;
		pte_t *spte = (*spde & PTE_P)
			? mem_ptr(PGADDR(*spde)) : (pte_t *) pmap_zeros;
		pte_t *dpte = pmap_walk(dpdir, dva, 1);
		if (dpte == NULL)
			return 0;

		// Loop through and merge the corresponding page table entries
		int ptx;
		for (ptx = 0; ptx < NPTENTRIES; ptx++) {

			if (spte[ptx] == rpte[ptx])	// unchanged in source
				continue;		// nothing to do
			if (dpte[ptx] == rpte[ptx]) {	// unchanged in dest
				if (!pmap_copy(spdir, sva + ptx * PAGESIZE,
						dpdir, dva + ptx * PAGESIZE,
						PAGESIZE))
					return 0;
				continue;
			}

			pmap_mergepage(rpte, spte, dpte,
					dva + ptx * PAGESIZE);
		}
	}
	return 1;
#else /* not SOL >= 3 */
	panic("pmap_merge() not implemented");
#endif /* not SOL >= 3 */
}

//
// Transparently handle a page fault entirely in the kernel, if possible.
// If the page fault was caused by a write to a copy-on-write page,
// then performs the actual page copy on demand and calls trap_return().
// If the fault wasn't due to the kernel's copy on write optimization,
// however, this function just returns so the trap gets blamed on the user.
//
void
pmap_pagefault(trapframe *tf)
{
	// Read processor's CR2 register to find the faulting linear address.
	uint32_t fva = rcr2();

#if SOL >= 3
	// It can't be our problem unless it's a plain write fault!
	if ((tf->tf_err & (PFE_WR | PFE_PR)) != PFE_WR)
		return;

	proc *p = proc_cur();
	pde_t *pde = &p->pdir[PDX(fva)];

	// If the page table is shared, copy it first.
	if (!(*pde & PTE_P))
		return;		// ptab doesn't exist at all - blame user
	pte_t *ptab = (pte_t *) PGADDR(*pde);
	if (!(*pde & PTE_W)) {
		pageinfo *pi = mem_alloc(); assert(pi); mem_incref(pi);
		pte_t *nptab = mem_pi2ptr(pi);
		memmove(nptab, ptab, PAGESIZE);	// copy page table
		mem_decref(mem_ptr2pi(ptab));	// release the old reference
		*pde = (uint32_t)nptab | PTE_A | PTE_W | PTE_U | PTE_P;
		ptab = nptab;
	}

	pte_t *pte = &ptab[PTX(fva)];
	if ((*pte & (SYS_READ | SYS_WRITE | PTE_P)) !=
			(SYS_READ | SYS_WRITE | PTE_P))
		return;		// page doesn't exist at all - blame user
	assert(!(*pte & PTE_W));
	void *page = (void *) PGADDR(*pte);
	pageinfo *pi = mem_alloc(); assert(pi); mem_incref(pi);
	void *npage = mem_pi2ptr(pi);
	memmove(npage, page, PAGESIZE);	// copy the page
	mem_decref(mem_ptr2pi(page));	// release the old reference
	*pte = (uint32_t)npage | (*pte & (SYS_READ|SYS_WRITE))
			| PTE_A | PTE_D | PTE_W | PTE_U | PTE_P;

	trap_return(tf);
#else /* not SOL >= 3 */
	// Fill in the rest of this code.
#endif /* not SOL >= 3 */
}

#if LAB >= 99
// check pmap_insert, pmap_remove, &c
void
pmap_check(void)
{
	struct Page *pp, *pp0, *pp1, *pp2;
	struct Page_list fl;
	pte_t *ptep, *ptep1;
	void *va;
	int i;

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

	// there is no page allocated at address 0
	assert(page_lookup(boot_pdir, (void *) 0x0, &ptep) == NULL);

	// there is no free memory, so we can't allocate a page table 
	assert(pmap_insert(boot_pdir, pp1, 0x0, 0) < 0);

	// free pp0 and try again: pp0 should be used for page table
	page_free(pp0);
	assert(pmap_insert(boot_pdir, pp1, 0x0, 0) == 0);
	assert(PGADDR(boot_pdir[0]) == page2pa(pp0));
	assert(check_va2pa(boot_pdir, 0x0) == page2pa(pp1));
	assert(pp1->pp_ref == 1);
	assert(pp0->pp_ref == 1);

	// should be able to map pp2 at PAGESIZE because pp0 is already allocated for page table
	assert(pmap_insert(boot_pdir, pp2, (void*) PAGESIZE, 0) == 0);
	assert(check_va2pa(boot_pdir, PAGESIZE) == page2pa(pp2));
	assert(pp2->pp_ref == 1);

	// should be no free memory
	assert(page_alloc(&pp) == -E_NO_MEM);

	// should be able to map pp2 at PAGESIZE because it's already there
	assert(pmap_insert(boot_pdir, pp2, (void*) PAGESIZE, 0) == 0);
	assert(check_va2pa(boot_pdir, PAGESIZE) == page2pa(pp2));
	assert(pp2->pp_ref == 1);

	// pp2 should NOT be on the free list
	// could happen in ref counts are handled sloppily in pmap_insert
	assert(page_alloc(&pp) == -E_NO_MEM);

	// check that pdir_walk returns a pointer to the pte
	ptep = KADDR(PGADDR(boot_pdir[PDX(PAGESIZE)]));
	assert(pdir_walk(boot_pdir, (void*)PAGESIZE, 0) == ptep+PTX(PAGESIZE));

	// should be able to change permissions too.
	assert(pmap_insert(boot_pdir, pp2, (void*) PAGESIZE, PTE_U) == 0);
	assert(check_va2pa(boot_pdir, PAGESIZE) == page2pa(pp2));
	assert(pp2->pp_ref == 1);
	assert(*pdir_walk(boot_pdir, (void*) PAGESIZE, 0) & PTE_U);
	assert(boot_pdir[0] & PTE_U);
	
	// should not be able to map at PTSIZE because need free page for page table
	assert(pmap_insert(boot_pdir, pp0, (void*) PTSIZE, 0) < 0);

	// insert pp1 at PAGESIZE (replacing pp2)
	assert(pmap_insert(boot_pdir, pp1, (void*) PAGESIZE, 0) == 0);
	assert(!(*pdir_walk(boot_pdir, (void*) PAGESIZE, 0) & PTE_U));

	// should have pp1 at both 0 and PAGESIZE, pp2 nowhere, ...
	assert(check_va2pa(boot_pdir, 0) == page2pa(pp1));
	assert(check_va2pa(boot_pdir, PAGESIZE) == page2pa(pp1));
	// ... and ref counts should reflect this
	assert(pp1->pp_ref == 2);
	assert(pp2->pp_ref == 0);

	// pp2 should be returned by page_alloc
	assert(page_alloc(&pp) == 0 && pp == pp2);

	// unmapping pp1 at 0 should keep pp1 at PAGESIZE
	pmap_remove(boot_pdir, 0x0);
	assert(check_va2pa(boot_pdir, 0x0) == ~0);
	assert(check_va2pa(boot_pdir, PAGESIZE) == page2pa(pp1));
	assert(pp1->pp_ref == 1);
	assert(pp2->pp_ref == 0);

	// unmapping pp1 at PAGESIZE should free it
	pmap_remove(boot_pdir, (void*) PAGESIZE);
	assert(check_va2pa(boot_pdir, 0x0) == ~0);
	assert(check_va2pa(boot_pdir, PAGESIZE) == ~0);
	assert(pp1->pp_ref == 0);
	assert(pp2->pp_ref == 0);

	// so it should be returned by page_alloc
	assert(page_alloc(&pp) == 0 && pp == pp1);

	// should be no free memory
	assert(page_alloc(&pp) == -E_NO_MEM);
	
#if 0
	// should be able to pmap_insert to change a page
	// and see the new data immediately.
	memset(page2kva(pp1), 1, PAGESIZE);
	memset(page2kva(pp2), 2, PAGESIZE);
	pmap_insert(boot_pdir, pp1, 0x0, 0);
	assert(pp1->pp_ref == 1);
	assert(*(int*)0 == 0x01010101);
	pmap_insert(boot_pdir, pp2, 0x0, 0);
	assert(*(int*)0 == 0x02020202);
	assert(pp2->pp_ref == 1);
	assert(pp1->pp_ref == 0);
	pmap_remove(boot_pdir, 0x0);
	assert(pp2->pp_ref == 0);
#endif

	// forcibly take pp0 back
	assert(PGADDR(boot_pdir[0]) == page2pa(pp0));
	boot_pdir[0] = 0;
	assert(pp0->pp_ref == 1);
	pp0->pp_ref = 0;
	
	// check pointer arithmetic in pdir_walk
	page_free(pp0);
	va = (void*)(PAGESIZE * NPDENTRIES + PAGESIZE);
	ptep = pdir_walk(boot_pdir, va, 1);
	ptep1 = KADDR(PGADDR(boot_pdir[PDX(va)]));
	assert(ptep == ptep1 + PTX(va));
	boot_pdir[PDX(va)] = 0;
	pp0->pp_ref = 0;
	
	// check that new page tables get cleared
	memset(page2kva(pp0), 0xFF, PAGESIZE);
	page_free(pp0);
	pdir_walk(boot_pdir, 0x0, 1);
	ptep = page2kva(pp0);
	for(i=0; i<NPTENTRIES; i++)
		assert((ptep[i] & PTE_P) == 0);
	boot_pdir[0] = 0;
	pp0->pp_ref = 0;

	// give free list back
	page_free_list = fl;

	// free the pages we took
	page_free(pp0);
	page_free(pp1);
	page_free(pp2);
	
	cprintf("page_check() succeeded!\n");


	// More things we should test:
	// - does trap() call pmap_fault() before recovery?
	// - does syscall_checkva avoid wraparound issues?
}
#endif	// XXX

#endif /* LAB >= 3 */
