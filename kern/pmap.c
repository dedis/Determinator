#if LAB >= 3
/*
 * Page mapping and page directory/table management.
 *
 * Copyright (C) 1997 Massachusetts Institute of Technology
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Derived from the MIT Exokernel and JOS.
 * Adapted for PIOS by Bryan Ford at Yale University.
 * Adapted for 64-bit PIOS by Yu Zhang.
 */


#include <inc/types.h>
#include <inc/x86.h>
#include <inc/mmu.h>
#include <inc/cdefs.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/syscall.h>
#include <inc/vm.h>

#include <kern/cpu.h>
#include <kern/trap.h>
#include <kern/proc.h>
#include <kern/pmap.h>


// Statically allocated page directory mapping the kernel's address space.
// We use this as a template for all pdirs for user-level processes.
// pte_t pmap_bootpmap[NPRENTRIES] gcc_aligned(PAGESIZE);
extern pte_t pmap_bootpmap[NPRENTRIES];

// Statically allocated page that we always keep set to all zeros.
uint8_t pmap_zero[PAGESIZE] gcc_aligned(PAGESIZE);


// --------------------------------------------------------------
// Set up initial memory mappings and turn on MMU.
// --------------------------------------------------------------



// Set up a four-level page table:
// pmap_bootpml4 is its linear (virtual) address of the root
// Then turn on paging.
// 
// This function only creates mappings in the kernel part of the address space
// (addresses outside of the range between VM_USERLO and VM_USERHI).
// The user part of the address space remains all PTE_ZERO until later.
//

int
pmap_init_boot(void) {
	// page table entries created in 32-bit mode in preparation
	// for entry into 64-bit mode
	// memory till 0x800000 exclusive (smallest page aligned address
	// greater than _end = 0x7090d0) mapped using 4KB global pages 
	// (non-TLB-flushed) only
	extern pte_t bootp3tab[NPTENTRIES];
	extern pte_t bootp2tab[NPTENTRIES];
	extern pte_t bootp1tab[4*NPTENTRIES];
	pmap_bootpmap[0] = (pte_t)bootp3tab + (PTE_P | PTE_W);
	bootp3tab[0] = (pte_t)bootp2tab + (PTE_P | PTE_W);
	int i;
	for (i = 0; i < 4; i++) {
		bootp2tab[i] = (pte_t)&bootp1tab[i*NPTENTRIES] + (PTE_P | PTE_W);
	}
        for (i = 0; i < 4*NPTENTRIES; i++) {
	        bootp1tab[i] = (pte_t)(i << PAGESHIFT) + (PTE_P | PTE_W | PTE_G);
	}
	return 0;
}

void
pmap_init(void)
{
	if (cpu_onboot()) {
		// Initialize pmap_bootpmap, the bootstrap page map.
		// Page map entries corresponding to the user-mode address 
		// space between VM_USERLO and VM_USERHI
		// should all be initialized to PTE_ZERO (see kern/pmap.h).
		// All virtual addresses below and above this user area
		// should be identity-mapped to the same physical addresses,
		// but only accessible in kernel mode (not in user mode).
#if SOL >= 3
		int i;
		for (i = 0; i < NPRENTRIES; i++)
			pmap_bootpmap[i] = ((intptr_t)i << PDSHIFT(NPTLVLS))
				| PTE_P | PTE_W | PTE_G;
		for (i = PDX(NPTLVLS, VM_USERLO); i < PDX(NPTLVLS, VM_USERHI); i++)
			pmap_bootpmap[i] = PTE_ZERO;	// clear user area
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

        uintptr_t cr4 = rcr4();
#if SOL >= 2
        cr4 |= CR4_OSFXSR | CR4_OSXMMEXCPT; // enable 128-bit XMM instructions
#endif
        lcr4(cr4);

        // Already done in kern/entry.S for boot CPU
	// and boot/bootothers.S for AP CPUs
	// Install the bootstrap page map level-4 into the PDBR.
        // lcr3(mem_phys(pmap_bootpmap));

        uintptr_t cr0 = rcr0();
        cr0 |= CR0_AM|CR0_NE|CR0_TS;
        cr0 &= ~(CR0_EM);
        lcr0(cr0);

        if (cpu_onboot())
                pmap_check();
}

//
// Allocate a new page map root, initialized from the bootstrap pmlap.
// Returns the new pmap with a reference count of 1.
//
pte_t *
pmap_newpmap(void)
{
	pageinfo *pi = mem_alloc();
	if (pi == NULL)
		return NULL;
	mem_incref(pi);
	pte_t *pmap = mem_pi2ptr(pi);

	// Initialize it from the bootstrap page map
	assert(sizeof(pmap_bootpmap) == PAGESIZE);
	memmove(pmap, pmap_bootpmap, PAGESIZE);

	return pmap;
}

// Free a page map, and all page map tables and mappings it may contain.
void
pmap_freepmap(pageinfo *pml4pi)
{
	pmap_remove(mem_pi2ptr(pml4pi), VM_USERLO, VM_USERHI-VM_USERLO);
	mem_free(pml4pi);
}

// Free a page table and all page mappings it may contain.
void
pmap_freeptab(pageinfo *ptabpi)
{
	pte_t *pte = mem_pi2ptr(ptabpi), *ptelim = pte + NPTENTRIES;
	for (; pte < ptelim; pte++) {
		intptr_t pgaddr = PTE_ADDR(*pte);
		if (pgaddr != PTE_ZERO)
			mem_decref(mem_phys2pi(pgaddr), mem_free);
	}
	mem_free(ptabpi);
}

// Given 'pml4', a pointer to a PML4 table, pmap_walk returns
// a pointer to the page table entry (PTE) for user virtual address 'va'.
// This requires walking the four-level page table structure.
//
// Assuming that pmtab points to the current page map table whose level
// is pmlevel, repeatedly do the following until the level equals 1.
// If the relevant lower level page map table doesn't exist in pmtab,
// then:
//    - If writing == 0, pmap_walk returns NULL.
//    - Otherwise, pmap_walk tries to allocate a new lower page map
//	table with mem_alloc.  If this fails, pmap_walk returns NULL.
//    - The new lower page map table is cleared and its refcount 
//      set to 1.
//    - If the relevant lower level page map table has already existed
//      in the table pointed-to by pmtab, but it is read shared and 
//      writing != 0, then copy the lower page map table to obtain an
//      exclusive copy of it and write-enable the entry in pmtab.
//    - If the lower table is a page table, pmap_walk returns a pointer
//      to the requested entry within the lower page table.
//    - Otherwise, let the lower table be pmtab, and do the above job 
//      repeatedly.
//
// Hint: you can turn a pageinfo pointer into the physical address of the
// page it refers to with mem_pi2phys() from kern/mem.h.
//
// Hint 2: the x86 MMU checks permission bits in all of the page map
// level-4, the page-directory-pointer, the page directory and 
// the page table, so it's safe to leave some page permissions
// more permissive than strictly necessary.
static pte_t *pmap_walk_level();

pte_t *
pmap_walk(pte_t *pml4, intptr_t va, bool writing)
{
	assert(va >= VM_USERLO && va < VM_USERHI);

#if SOL >= 3
	pte_t *pte = pmap_walk_level(NPTLVLS, pml4, va, writing);
	return pte;
#else /* not SOL >= 3 */
	// Fill in this function
	return NULL;
#endif /* not SOL >= 3 */
}

// Given a specified level table pointer 'pmtab',
// it returns a pointer to the PTE for user virtual address 'va'.
static pte_t *
pmap_walk_level(int pmlevel, pte_t *pmtab, intptr_t la, bool writing)
{
	pte_t *pmte = &pmtab[PDX(pmlevel, la)];	// find entry in the specified level tabke
	pte_t *plowtab;				// will point to lower page map table
	if (*pmte & PTE_P) {			// lower ptab already exist?
		plowtab = mem_ptr(PTE_ADDR(*pmte));
	} else {				// no - create?
		assert(*pmte == PTE_ZERO);
		pageinfo *pi;
		if (!writing || (pi = mem_alloc()) == NULL)
			return NULL;
		mem_incref(pi);
		plowtab = mem_pi2ptr(pi);

		// Clear all mappings in the page table
		int i;
		for (i = 0; i < NPTENTRIES; i++)
			plowtab[i] = PTE_ZERO;

		// The permissions here are overly generous, but they can
		// be further restricted by the permissions in the page table 
		// entries, if necessary.
		*pmte = mem_pi2phys(pi) | PTE_A | PTE_P | PTE_W | PTE_U;
	}

	// If the lower page map table is shared and we're writing, copy it first.
	// Must propagate the read-only status down to the page mappings.
	if (writing && !(*pmte & PTE_W)) {
		if (mem_ptr2pi(plowtab)->refcount == 1) {
			// Lower page map table isn't shared, so we can use in-place;
			// but must propagate the read-only status from the pmlevel
			// down to all individual entrys in lower level.
			int i;
			for (i = 0; i < NPTENTRIES; i++)
				plowtab[i] &= ~PTE_W;
		} else {
			// Lower page map table is or may still be shared - must copy.
			pageinfo *pi = mem_alloc();
			if (pi == NULL)
				return NULL;
			mem_incref(pi);
			pte_t *nplowtab = mem_pi2ptr(pi);
			//cprintf("pmap_walk_level %d %x %x: copy plowtab %x->%x\n",
			//	pmlevel, pmtab, va, plowtab, nplowtab);

			// Copy all page table entries,
			// incrementing each's refcount
			int i;
			for (i = 0; i < NPTENTRIES; i++) {
				intptr_t pte = plowtab[i];
				nplowtab[i] = pte & ~PTE_W;
				assert(PTE_ADDR(pte) != 0);
				if (PTE_ADDR(pte) != PTE_ZERO)
					mem_incref(mem_phys2pi(PTE_ADDR(pte)));
			}

			mem_decref(mem_ptr2pi(plowtab), pmap_freeptab); //TODO
			plowtab = nplowtab;
		}
		*pmte = (uintptr_t)plowtab | PTE_A | PTE_P | PTE_W | PTE_U;
	}

	if (pmlevel == 1)
		return &plowtab[PDX(pmlevel-1, la)];
	else
		return pmap_walk_level(pmlevel-1, pmte, la, writing);
}

//
// Map the physical page 'pi' at user virtual address 'va'.
// The permissions (the low 12 bits) of the page table
//  entry should be set to 'perm | PTE_P'.
//
// Requirements
//   - If there is already a page mapped at 'va', it should be pmap_removed().
//   - If necessary, allocate a page able on demand and insert into 'pml4'.
//   - pi->refcount should be incremented if the insertion succeeds.
//   - The TLB must be invalidated if a page was formerly present at 'va'.
//
// Corner-case hint: Make sure to consider what happens when the same 
// pi is re-inserted at the same virtual address in the same pml4.
// What if this is the only reference to that page?
//
// RETURNS: 
//   a pointer to the inserted PTE on success (same as pmap_walk)
//   NULL, if page table couldn't be allocated
//
// Hint: The reference solution uses pmap_walk, pmap_remove, and mem_pi2phys.
//
pte_t *
pmap_insert(pte_t *pml4, pageinfo *pi, intptr_t va, int perm)
{
#if SOL >= 3
	pte_t* pte = pmap_walk(pml4, va, 1);
	if (pte == NULL)
		return NULL;

	// We must increment pi->refcount before pmap_remove, so that
	// if pi is already mapped at va (we're just changing perm),
	// we don't lose the page when we decref in pmap_remove.
	mem_incref(pi);

	// Now remove the old mapping in this PTE.
	if (*pte & PTE_P)
		pmap_remove(pml4, va, PAGESIZE);

	*pte = mem_pi2phys(pi) | perm | PTE_P;
	return pte;
#else /* not SOL >= 3 */
	// Fill in this function
	return NULL;
#endif /* not SOL >= 3 */
}

//
// Unmap the physical pages starting at user virtual address 'va'
// and covering a virtual address region of 'size' bytes.
// The caller must ensure that both 'va' and 'size' are page-aligned.
// If there is no mapping at that address, pmap_remove silently does nothing.
// Clears nominal permissions (SYS_RW flags) as well as mappings themselves.
//
// Details:
//   - The refcount on mapped pages should be decremented atomically.
//   - The physical page should be freed if the refcount reaches 0.
//   - The page table entry corresponding to 'va' should be set to 0.
//     (if such a PTE exists)
//   - The TLB must be invalidated if you remove an entry from
//     the pml4/pdp/pdir/ptab.
//   - If the region to remove covers a whole 4MB page table region,
//     then unmap and free the page table after unmapping all its contents.
//
// Hint: The TA solution is implemented using pmap_lookup,
// 	pmap_inval, and mem_decref.
//
static intptr_t pmap_remove_level();

void
pmap_remove(pte_t *pml4, intptr_t va, size_t size)
{
	assert(PGOFF(size) == 0);	// must be page-aligned
	assert(va >= VM_USERLO && va < VM_USERHI);
	assert(size <= VM_USERHI - va);

#if SOL >= 3
	pmap_inval(pml4, va, size);	// invalidate region we're removing

	intptr_t vahi = va + size;
	while (va < vahi) {
		va = pmap_remove_level(NPTLVLS, pml4, vahi);
	}
#else /* not SOL >= 3 */
	// Fill in this function
#endif /* not SOL >= 3 */
}

// pmlevel == 3, pmtab contains a PML4 base address
// pmlevel == 2, pmtab contains a page-directory-pointer base address
// pmlevel == 1, pmtab contains a page-directory base address
// pmlevel == 0, pmtab contains a page table base address
static intptr_t
pmap_remove_level(int pmlevel, pte_t *pmtab, intptr_t va, intptr_t vahi)
{
	pte_t *pmte;
		
	assert(pmlevel >= 1);

	// find the entry in the specified level table
	pmte = &pmtab[PDX(pmlevel, va)];
	if (*pmte == PTE_ZERO) {
		// the entry does not points to a lower-level table
		// skip the entire lower-level table region
		va = PDADDR(pmlevel, va + PDSIZE(pmlevel));
		return va;
	}
		
	// Can we remove an entire lower-level table at once?
	if (PDOFF(pmlevel, va) == 0 && vahi-va >= PDSIZE(pmlevel)) {
		intptr_t plowtab = PTE_ADDR(*pmte);
		if (plowtab != PTE_ZERO) {// drop table ref
			if (pmlevel > 1) {
				size_t size = PDSIZE(pmlevel - 1);
				intptr_t tmpva = va; 
				int i;
				for (i = 0; i < NPTENTRIES; i++ ) {
					pmap_remove_level(pmlevel-1,
						pmte, tmpva, vahi);
					tmpva += size;
				}
				mem_decref(mem_phys2pi(plowtab),
					mem_free);
			} else
				mem_decref(mem_phys2pi(plowtab),
					pmap_freeptab);
		}
		*pmte = PTE_ZERO;
		va = PDADDR(pmlevel, va + PDSIZE(pmlevel));
		return va;
	} 
		
	if ( pmlevel > 1)
		return pmap_remove_level(pmlevel-1, pmte, va, vahi);
	
	assert( pmlevel == 1 );
	pte_t *pte = pmap_walk_level(1, pmtab, va, 1);	// find & unshare PTE
	assert(pte != NULL);	// XXX

	// Remove page mappings up to end of region or page table
	do {
		intptr_t pgaddr = PTE_ADDR(*pte);
		if (pgaddr != PTE_ZERO)
			mem_decref(mem_phys2pi(pgaddr), mem_free);
		*pte++ = PTE_ZERO;
		va += PAGESIZE;
	} while (va < vahi && PDX(0, va) != 0);
	return va;
}

//
// Invalidate the TLB entry or entries for a given virtual address range,
// but only if the page tables being edited are the ones
// currently in use by the processor.
//
void
pmap_inval(pte_t *pml4, intptr_t va, size_t size)
{
	// Flush the entry only if we're modifying the current address space.
	proc *p = proc_cur();
	if (p == NULL || p->pml4 == pml4) {
		if (size == PAGESIZE)
			invlpg(mem_ptr(va));	// invalidate one page
		else
			lcr3(mem_phys(pml4));	// invalidate everything
	}
}

//
// Virtually copy a range of pages from spml4 to dpml4 (could be the same).
// Uses copy-on-write to avoid the cost of immediate copying:
// instead just copies the mappings and makes both source and dest read-only.
// Returns true if successfull, false if not enough memory for copy.
//
static void pmap_copy_level();

int
pmap_copy(pte_t *spml4, intptr_t sva, pte_t *dpml4, intptr_t dva,
		size_t size)
{
	assert(PDOFF(1, sva) == 0);	// must be 4MB-aligned
	assert(PDOFF(1, dva) == 0);
	assert(PDOFF(1, size) == 0);
	assert(sva >= VM_USERLO && sva < VM_USERHI);
	assert(dva >= VM_USERLO && dva < VM_USERHI);
	assert(size <= VM_USERHI - sva);
	assert(size <= VM_USERHI - dva);

#if SOL >= 3
	// Invalidate both regions we may be modifying
	pmap_inval(spml4, sva, size);
	pmap_inval(dpml4, dva, size);

	intptr_t svahi = sva + size;
	pmap_copy_level(NPTLVLS, spml4, sva, dpml4, dva, svahi);
	return 1;
#else /* not SOL >= 3 */
	panic("pmap_copy() not implemented");
#endif /* not SOL >= 3 */
}

//
// pmlevel == 3, spmtab & dpmtab => source/destination pml4 table
// pmlevel == 2, spmtab & dpmtab => source/destination pdp table
// pmlevel == 1, spmtab & dpmtab => source/destination page directory table
// pmlevel == 0, spmtab & dpmtab => source/destination page table
//
static void
pmap_copy_level(int pmlevel, pte_t *spmtab, intptr_t sva, pte_t *dpmtab, 
		intptr_t dva, intptr_t svahi)
{
	if (sva >= svahi)
		return;

	assert(pmlevel < NPTLVLS && (svahi-sva) < PDSIZE(pmlevel+1));
	assert(PDOFF(pmlevel, sva) == PDOFF(pmlevel, dva)); 

	pte_t *spmte = &spmtab[PDX(pmlevel, sva)];
	pte_t *dpmte = &dpmtab[PDX(pmlevel, dva)];
	size_t size = PDSIZE(pmlevel);

	if (PDOFF(pmlevel, sva) != 0) {
		// copy a upper partial region mapped by *spmte to *dpmte
		pmap_copy_level(pmlevel-1, spmte, sva, dpmte, dva, 
				PDADDR(pmlevel, sva)+size);
		spmte++, dpmte++;
		sva = PDADDR(pmlevel, sva)+size;
		dva = PDADDR(pmlevel, dva)+size;
	}

	while ((sva + size) <= svahi) {
		// copy an entire region mapped by specfied level table entry
		if (*dpmte & PTE_P)	
			// remove old specified level pmap table first
			// TODO: need to remove the region mapped by *dpmte
			pmap_remove_level(pmlevel, dpmtab, dva, size);
		assert(*dpmte == PTE_ZERO);

		*spmte &= ~PTE_W;	// remove write permission

		*dpmte = *spmte;	// copy specified level page map table mapping
		if (*spmte != PTE_ZERO)
			mem_incref(mem_phys2pi(PTE_ADDR(*spmte)));

		spmte++, dpmte++;
		sva += size;
		dva += size;
	}
	if (sva < svahi && pmlevel > 0) {
		// copy a partial region mapped by specified level table entry
		pmap_copy_level(pmlevel-1, spmte, sva, dpmte, dva, svahi);
	}
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
	intptr_t fva = rcr2();
	//cprintf("pmap_pagefault fva %x eip %x\n", fva, tf->eip);

#if SOL >= 3
	// It can't be our problem unless it's a write fault in user space!
	if (fva < VM_USERLO || fva >= VM_USERHI || !(tf->err & PFE_WR)) {
		cprintf("pmap_pagefault: fva %x err %x\n", fva, tf->err);
		return;
	}

	proc *p = proc_cur();
	int pmlevel = NPTLVLS;
	pte_t *pmtab = p->pml4;
	while ( pmlevel >= 1) {
		pte_t *pmte = &pmtab[PDX(pmlevel, fva)];
		if (!(*pmte & PTE_P)) {
			cprintf("pmap_pagefault: %d-level pmte for fva \
				%x doesn't exist\n", pmlevel, fva);
			return;		// ptab doesn't exist at all - blame user
		}
		pmtab = pmte, pmlevel--;
	}

	// Find the page table entry, copying the page table if it's shared.
	pte_t *pte = pmap_walk(p->pml4, fva, 1);
	if ((*pte & (SYS_READ | SYS_WRITE | PTE_P)) !=
			(SYS_READ | SYS_WRITE | PTE_P)) {
		cprintf("pmap_pagefault: page for fva %x doesn't exist\n", fva);
		return;		// page doesn't exist at all - blame user
	}
	assert(!(*pte & PTE_W));

	// Find the "shared" page.  If refcount is 1, we have the only ref!
	intptr_t pg = PTE_ADDR(*pte);
	if (pg == PTE_ZERO || mem_phys2pi(pg)->refcount > 1) {
		pageinfo *npi = mem_alloc(); assert(npi);
		mem_incref(npi);
		intptr_t npg = mem_pi2phys(npi);
		memmove((void*)npg, (void*)pg, PAGESIZE); // copy the page
		if (pg != PTE_ZERO)
			mem_decref(mem_phys2pi(pg), mem_free); // drop old ref
		//cprintf("pmap_pflt %x %x: copy page %x->%x\n",
		//	p->pml4, fva, pg, npg);
		pg = npg;
	}
	*pte = pg | SYS_RW | PTE_A | PTE_D | PTE_W | PTE_U | PTE_P;

	// Make sure the old mapping doesn't get used anymore
	pmap_inval(p->pml4, PGADDR(fva), PAGESIZE);

	trap_return(tf);
#else /* not SOL >= 3 */
	// Fill in the rest of this code.
#endif /* not SOL >= 3 */
}

//
// Helper function for pmap_merge: merge a single memory page
// that has been modified in both the source and destination.
// If conflicting writes to a single byte are detected on the page,
// print a warning to the console and remove the page from the destination.
// If the destination page is read-shared, be sure to copy it before modifying!
//
void
pmap_mergepage(pte_t *rpte, pte_t *spte, pte_t *dpte, intptr_t dva)
{
#if SOL >= 3
	uint8_t *rpg = (uint8_t*)PTE_ADDR(*rpte);
	uint8_t *spg = (uint8_t*)PTE_ADDR(*spte);
	uint8_t *dpg = (uint8_t*)PTE_ADDR(*dpte);
	if (dpg == pmap_zero) return;	// Conflict - just leave dest unmapped

	// Make sure the destination page isn't shared
	if (dpg == (uint8_t*)PTE_ZERO || mem_ptr2pi(dpg)->refcount > 1) {
		pageinfo *npi = mem_alloc(); assert(npi);
		mem_incref(npi);
		uint8_t *npg = mem_pi2ptr(npi);
		memmove(npg, dpg, PAGESIZE); // copy the page
		if (dpg != (uint8_t*)PTE_ZERO)
			mem_decref(mem_ptr2pi(dpg), mem_free); // drop old ref
		dpg = npg;
		*dpte = (intptr_t)npg |
			SYS_RW | PTE_A | PTE_D | PTE_W | PTE_U | PTE_P;
	}

	// Do a byte-by-byte diff-and-merge into the destination
	int i;
	for (i = 0; i < PAGESIZE; i++) {
		if (spg[i] == rpg[i])
			continue;	// unchanged in source - leave dest
		if (dpg[i] == rpg[i]) {
			dpg[i] = spg[i];	// unchanged in dest - use src
			continue;
		}

		cprintf("pmap_mergepage: conflict at dva %x\n", dva);
		mem_decref(mem_phys2pi(PTE_ADDR(*dpte)), mem_free);
		*dpte = PTE_ZERO;
		return;
	}
#else /* not SOL >= 3 */
	panic("pmap_mergepage() not implemented");
#endif /* not SOL >= 3 */
}

// 
// Merge differences between a reference snapshot represented by rpml4
// and a source address space spml4 into a destination address space dpml4.
//
int
pmap_merge(pte_t *rpml4, pte_t *spml4, intptr_t sva,
		pte_t *dpml4, intptr_t dva, size_t size)
{
	assert(PDOFF(1, sva) == 0);	// must be 4MB-aligned
	assert(PDOFF(1, dva) == 0);
	assert(PDOFF(1, size) == 0);
	assert(sva >= VM_USERLO && sva < VM_USERHI);
	assert(dva >= VM_USERLO && dva < VM_USERHI);
	assert(size <= VM_USERHI - sva);
	assert(size <= VM_USERHI - dva);

#if SOL >= 3
	// Invalidate the source and destination regions we may be modifying.
	// (We may remove permissions from the source for copy-on-write.)
	// No need to invalidate rpdir since rpdirs are never loaded.
	pmap_inval(spml4, sva, size);
	pmap_inval(dpml4, dva, size);

	pte_t *rpmtab = rpml4, *spmtab = spml4, *dpmtab = dpml4;
	int pmlevel = NPTLVLS;
	pte_t *rpmte, *spmte, *dpmte;
	intptr_t svahi = sva + size;
	while (sva < svahi) {
		rpmte = &rpmtab[PDX(pmlevel, sva)]; // find PDE/PDPE/PML4Es
		spmte = &spmtab[PDX(pmlevel, sva)];
		dpmte = &dpmtab[PDX(pmlevel, dva)];
		if (*spmte == *rpmte) {	// unchanged in source - do nothing
			sva += PDSIZE(pmlevel), dva += PDSIZE(pmlevel);
			continue;
		}
		if (*dpmte == *rpmte) {	// unchanged in dest - copy from source
			if (!pmap_copy(spml4, sva, dpml4, dva, PDSIZE(pmlevel)))
				return 0;
			sva += PDSIZE(pmlevel), dva += PDSIZE(pmlevel);
			continue;
		}
		if (pmlevel > 1) {
			rpmtab = rpmte, spmtab = spmte, dpmtab = dpmte;
			pmlevel --;
			continue;
		}
		//cprintf("pmap_merge: merging page map table %d:%x-%x\n",
		//	pmlevel, sva, sva+PDSIZE(pmlevel));

		// Find each of the page tables from the corresponding PDEs
		pte_t *rpte = mem_ptr(PGADDR(*rpmte));	// OK if PTE_ZERO
		pte_t *spte = mem_ptr(PGADDR(*spmte));	// OK if PTE_ZERO
		pte_t *dpte = pmap_walk(dpml4, dva, 1);	// must exist, unshared
		if (dpte == NULL)
			return 0;

		// Loop through and merge the corresponding page table entries
		pte_t *erpte = &rpte[NPTENTRIES];
		for (; rpte < erpte; rpte++, spte++, dpte++,
				sva += PAGESIZE, dva += PAGESIZE) {

			if (*spte == *rpte)	// unchanged in source
				continue;		// nothing to do
			if (*dpte == *rpte) {	// unchanged in dest
				// just copy source page using COW
				if (PTE_ADDR(*dpte) != PTE_ZERO)
					mem_decref(mem_phys2pi(PTE_ADDR(*dpte)),
							mem_free);
				*spte &= ~PTE_W;
				*dpte = *spte;		// copy ptable mapping
				mem_incref(mem_phys2pi(PTE_ADDR(*spte)));
				continue;
			}
			//cprintf("pmap_merge: merging page %x-%x\n",
			//	sva, sva+PAGESIZE);

			// changed in both spaces - must merge word-by-word
			pmap_mergepage(rpte, spte, dpte, dva);
		}
		rpmte++, spmte++, dpmte++;
		while (spmte >= spmtab + NPTENTRIES) {
			pmlevel = NPTLVLS;
			rpmtab = rpml4, spmtab = spml4, dpmtab = dpml4;
		}
	}
	return 1;
#else /* not SOL >= 3 */
	panic("pmap_merge() not implemented");
#endif /* not SOL >= 3 */
}

//
// Set the nominal permission bits on a range of virtual pages to 'perm'.
// Adding permission to a nonexistent page maps zero-filled memory.
// It's OK to add SYS_READ and/or SYS_WRITE permission to a PTE_ZERO mapping;
// this causes the pmap_zero page to be mapped read-only (PTE_P but not PTE_W).
// If the user gives SYS_WRITE permission to a PTE_ZERO mapping,
// the page fault handler copies the zero page when the first write occurs.
//
int
pmap_setperm(pte_t *pml4, intptr_t va, size_t size, int perm)
{
	assert(PGOFF(va) == 0);
	assert(PGOFF(size) == 0);
	assert(va >= VM_USERLO && va < VM_USERHI);
	assert(size <= VM_USERHI - va);
	assert((perm & ~(SYS_RW)) == 0);

#if SOL >= 3
	pmap_inval(pml4, va, size);	// invalidate region we're modifying

	// Determine the nominal and actual bits to set or clear
	uint64_t pteand, pteor;
	if (!(perm & SYS_READ))		// clear all permissions
		pteand = ~(SYS_RW | PTE_W | PTE_P), pteor = 0;
	else if (!(perm & SYS_WRITE))	// read-only permission
		pteand = ~(SYS_WRITE | PTE_W),
		pteor = (SYS_READ | PTE_U | PTE_P | PTE_A);
	else	// nominal read/write (but don't add PTE_W to shared mappings!)
		pteand = ~0, pteor = (SYS_RW | PTE_U | PTE_P | PTE_A | PTE_D);

	intptr_t vahi = va + size;
	while (va < vahi) {
		int pmlevel = NPTLVLS;
		pte_t *pmtab = pml4;
		while ( pmlevel >= 1 ) {
			pte_t *pte = &pmtab[PDX(pmlevel, va)]; // find entry in pmtab
			if (PDOFF(pmlevel, va) != 0) {
				pmtab = pte, pmlevel--;
				continue;
			}
			if (*pte == PTE_ZERO && pteor == 0) { 
				// clearing perms, but no page table
				// - skip PDSIZE(pmlevel) region
				va = PDADDR(pmlevel, va + PDSIZE(pmlevel));
				// start of next pmtab
				break;
			}
		}
		if ( pmlevel >= 1)
			continue;

		pte_t *pte = pmap_walk(pml4, va, 1);	// find & unshare PTE
		if (pte == NULL)
			return 0;	// page table alloc failed

		// Adjust page mappings up to end of region or page table
		do {
			*pte = (*pte & pteand) | pteor;
			pte++;
			va += PAGESIZE;
		} while (va < vahi && PDX(0, va) != 0);
	}
	return 1;
#else /* not SOL >= 3 */
	panic("pmap_merge() not implemented");
#endif /* not SOL >= 3 */
}

//
// This function returns the physical address of the page containing 'va',
// defined by the page directory 'pdir'.  The hardware normally performs
// this functionality for us!  We define our own version to help check
// the pmap_check() function; it shouldn't be used elsewhere.
//
static intptr_t
va2pa(pte_t *pmtab, intptr_t va)
{
	int pmlevel = NPTLVLS;
	pte_t *ptab;
	while (pmlevel >= 1) {
		pmtab = &pmtab[PDX(pmlevel, va)];
		if (!(*pmtab & PTE_P))
			return ~0;
		ptab = mem_ptr(PTE_ADDR(*pmtab));
		pmlevel --;
		pmtab = ptab;
	}
	if (!(ptab[PDX(0, va)] & PTE_P))
		return ~0;
	return PTE_ADDR(ptab[PDX(0, va)]);
}

// check pmap_insert, pmap_remove, &c
void
pmap_check(void)
{
#if 0
	extern pageinfo *mem_freelist;

	pageinfo *pi, *pi0, *pi1, *pi2, *pi3;
	pageinfo *fl;
	pte_t *ptep, *ptep1;
	int i;

	// should be able to allocate three pages
	pi0 = pi1 = pi2 = 0;
	pi0 = mem_alloc();
	pi1 = mem_alloc();
	pi2 = mem_alloc();
	pi3 = mem_alloc();

	assert(pi0);
	assert(pi1 && pi1 != pi0);
	assert(pi2 && pi2 != pi1 && pi2 != pi0);

	// temporarily steal the rest of the free pages
	fl = mem_freelist;
	mem_freelist = NULL;

	// should be no free memory
	assert(mem_alloc() == NULL);

	// there is no free memory, so we can't allocate a page table 
	assert(pmap_insert(pmap_bootpmap, pi1, VM_USERLO, 0) == NULL);

	// free pi0 and try again: pi0 should be used for page table
	mem_free(pi0);
	assert(pmap_insert(pmap_bootpmap, pi1, VM_USERLO, 0) != NULL);
	assert(PTE_ADDR(pmap_bootpmap[PDX(3, VM_USERLO)]) == mem_pi2phys(pi0));
	assert(va2pa(pmap_bootpmap, VM_USERLO) == mem_pi2phys(pi1));
	assert(pi1->refcount == 1);
	assert(pi0->refcount == 1);

	// should be able to map pi2 at VM_USERLO+PAGESIZE
	// because pi0 is already allocated for page table
	assert(pmap_insert(pmap_bootpmap, pi2, VM_USERLO+PAGESIZE, 0));
	assert(va2pa(pmap_bootpmap, VM_USERLO+PAGESIZE) == mem_pi2phys(pi2));
	assert(pi2->refcount == 1);

	// should be no free memory
	assert(mem_alloc() == NULL);

	// should be able to map pi2 at VM_USERLO+PAGESIZE
	// because it's already there
	assert(pmap_insert(pmap_bootpmap, pi2, VM_USERLO+PAGESIZE, 0));
	assert(va2pa(pmap_bootpmap, VM_USERLO+PAGESIZE) == mem_pi2phys(pi2));
	assert(pi2->refcount == 1);

	// pi2 should NOT be on the free list
	// could hapien in ref counts are handled slopiily in pmap_insert
	assert(mem_alloc() == NULL);

	// check that pmap_walk returns a pointer to the pte
	ptep = mem_ptr(PGADDR(pmap_bootpmap[PDX(VM_USERLO+PAGESIZE)]));
	assert(pmap_walk(pmap_bootpmap, VM_USERLO+PAGESIZE, 0)
		== ptep+PTX(VM_USERLO+PAGESIZE));

	// should be able to change permissions too.
	assert(pmap_insert(pmap_bootpmap, pi2, VM_USERLO+PAGESIZE, PTE_U));
	assert(va2pa(pmap_bootpmap, VM_USERLO+PAGESIZE) == mem_pi2phys(pi2));
	assert(pi2->refcount == 1);
	assert(*pmap_walk(pmap_bootpmap, VM_USERLO+PAGESIZE, 0) & PTE_U);
	assert(pmap_bootpmap[PDX(VM_USERLO)] & PTE_U);
	
	// should not be able to map at VM_USERLO+PTSIZE
	// because we need a free page for a page table
	assert(pmap_insert(pmap_bootpmap, pi0, VM_USERLO+PTSIZE, 0) == NULL);

	// insert pi1 at VM_USERLO+PAGESIZE (replacing pi2)
	assert(pmap_insert(pmap_bootpmap, pi1, VM_USERLO+PAGESIZE, 0));
	assert(!(*pmap_walk(pmap_bootpmap, VM_USERLO+PAGESIZE, 0) & PTE_U));

	// should have pi1 at both +0 and +PAGESIZE, pi2 nowhere, ...
	assert(va2pa(pmap_bootpmap, VM_USERLO+0) == mem_pi2phys(pi1));
	assert(va2pa(pmap_bootpmap, VM_USERLO+PAGESIZE) == mem_pi2phys(pi1));
	// ... and ref counts should reflect this
	assert(pi1->refcount == 2);
	assert(pi2->refcount == 0);

	// pi2 should be returned by mem_alloc
	assert(mem_alloc() == pi2);

	// unmapping pi1 at VM_USERLO+0 should keep pi1 at +PAGESIZE
	pmap_remove(pmap_bootpmap, VM_USERLO+0, PAGESIZE);
	assert(va2pa(pmap_bootpmap, VM_USERLO+0) == ~0);
	assert(va2pa(pmap_bootpmap, VM_USERLO+PAGESIZE) == mem_pi2phys(pi1));
	assert(pi1->refcount == 1);
	assert(pi2->refcount == 0);
	assert(mem_alloc() == NULL);	// still should have no pages free

	// unmapping pi1 at VM_USERLO+PAGESIZE should free it
	pmap_remove(pmap_bootpmap, VM_USERLO+PAGESIZE, PAGESIZE);
	assert(va2pa(pmap_bootpmap, VM_USERLO+0) == ~0);
	assert(va2pa(pmap_bootpmap, VM_USERLO+PAGESIZE) == ~0);
	assert(pi1->refcount == 0);
	assert(pi2->refcount == 0);

	// so it should be returned by page_alloc
	assert(mem_alloc() == pi1);

	// should once again have no free memory
	assert(mem_alloc() == NULL);

	// should be able to pmap_insert to change a page
	// and see the new data immediately.
	memset(mem_pi2ptr(pi1), 1, PAGESIZE);
	memset(mem_pi2ptr(pi2), 2, PAGESIZE);
	pmap_insert(pmap_bootpmap, pi1, VM_USERLO, 0);
	assert(pi1->refcount == 1);
	assert(*(int*)VM_USERLO == 0x01010101);
	pmap_insert(pmap_bootpmap, pi2, VM_USERLO, 0);
	assert(*(int*)VM_USERLO == 0x02020202);
	assert(pi2->refcount == 1);
	assert(pi1->refcount == 0);
	assert(mem_alloc() == pi1);
	pmap_remove(pmap_bootpmap, VM_USERLO, PAGESIZE);
	assert(pi2->refcount == 0);
	assert(mem_alloc() == pi2);

	// now use a pmap_remove on a large region to take pi0 back
	pmap_remove(pmap_bootpmap, VM_USERLO, VM_USERHI-VM_USERLO);
	assert(pmap_bootpmap[PDX(VM_USERLO)] == PTE_ZERO);
	assert(pi0->refcount == 0);
	assert(mem_alloc() == pi0);
	assert(mem_freelist == NULL);

	// test pmap_remove with large, non-ptable-aligned regions
	mem_free(pi1);
	uintptr_t va = VM_USERLO;
	assert(pmap_insert(pmap_bootpmap, pi0, va, 0));
	assert(pmap_insert(pmap_bootpmap, pi0, va+PAGESIZE, 0));
	assert(pmap_insert(pmap_bootpmap, pi0, va+PTSIZE-PAGESIZE, 0));
	assert(PGADDR(pmap_bootpmap[PDX(VM_USERLO)]) == mem_pi2phys(pi1));
	assert(mem_freelist == NULL);
	mem_free(pi2);
	assert(pmap_insert(pmap_bootpmap, pi0, va+PTSIZE, 0));
	assert(pmap_insert(pmap_bootpmap, pi0, va+PTSIZE+PAGESIZE, 0));
	assert(pmap_insert(pmap_bootpmap, pi0, va+PTSIZE*2-PAGESIZE, 0));
	assert(PGADDR(pmap_bootpmap[PDX(VM_USERLO+PTSIZE)])
		== mem_pi2phys(pi2));
	assert(mem_freelist == NULL);
	mem_free(pi3);
	assert(pmap_insert(pmap_bootpmap, pi0, va+PTSIZE*2, 0));
	assert(pmap_insert(pmap_bootpmap, pi0, va+PTSIZE*2+PAGESIZE, 0));
	assert(pmap_insert(pmap_bootpmap, pi0, va+PTSIZE*3-PAGESIZE*2, 0));
	assert(pmap_insert(pmap_bootpmap, pi0, va+PTSIZE*3-PAGESIZE, 0));
	assert(PGADDR(pmap_bootpmap[PDX(VM_USERLO+PTSIZE*2)])
		== mem_pi2phys(pi3));
	assert(mem_freelist == NULL);
	assert(pi0->refcount == 10);
	assert(pi1->refcount == 1);
	assert(pi2->refcount == 1);
	assert(pi3->refcount == 1);
	pmap_remove(pmap_bootpmap, va+PAGESIZE, PTSIZE*3-PAGESIZE*2);
	assert(pi0->refcount == 2);
	assert(pi2->refcount == 0); assert(mem_alloc() == pi2);
	assert(mem_freelist == NULL);
	pmap_remove(pmap_bootpmap, va, PTSIZE*3-PAGESIZE);
	assert(pi0->refcount == 1);
	assert(pi1->refcount == 0); assert(mem_alloc() == pi1);
	assert(mem_freelist == NULL);
	pmap_remove(pmap_bootpmap, va+PTSIZE*3-PAGESIZE, PAGESIZE);
	assert(pi0->refcount == 0);	// pi3 might or might not also be freed
	pmap_remove(pmap_bootpmap, va+PAGESIZE, PTSIZE*3);
	assert(pi3->refcount == 0);
	mem_alloc(); mem_alloc();	// collect pi0 and pi3
	assert(mem_freelist == NULL);

	// check pointer arithmetic in pmap_walk
	mem_free(pi0);
	va = VM_USERLO + PAGESIZE*NPTENTRIES + PAGESIZE;
	ptep = pmap_walk(pmap_bootpmap, va, 1);
	ptep1 = mem_ptr(PGADDR(pmap_bootpmap[PDX(va)]));
	assert(ptep == ptep1 + PTX(va));
	pmap_bootpmap[PDX(va)] = PTE_ZERO;
	pi0->refcount = 0;

	// check that new page tables get cleared
	memset(mem_pi2ptr(pi0), 0xFF, PAGESIZE);
	mem_free(pi0);
	pmap_walk(pmap_bootpmap, VM_USERHI-PAGESIZE, 1);
	ptep = mem_pi2ptr(pi0);
	for(i=0; i<NPTENTRIES; i++)
		assert(ptep[i] == PTE_ZERO);
	pmap_bootpmap[PDX(VM_USERHI-PAGESIZE)] = PTE_ZERO;
	pi0->refcount = 0;

	// give free list back
	mem_freelist = fl;

	// free the pages we filched
	mem_free(pi0);
	mem_free(pi1);
	mem_free(pi2);
	mem_free(pi3);

#if LAB >= 9
#else
	cprintf("pmap_check() succeeded!\n");
#endif
#if LAB >= 99
	// More things we should test:
	// - does trap() call pmap_fault() before recovery?
	// - does syscall_checkva avoid wraparound issues?
#endif
#endif
}

#endif /* LAB >= 3 */
