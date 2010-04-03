#if LAB >= 3
/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/mmu.h>
#include <inc/gcc.h>
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
pde_t pmap_bootpdir[1024] gcc_aligned(PAGESIZE);

// Statically allocated page that we always keep set to all zeros.
uint8_t pmap_zero[PAGESIZE] gcc_aligned(PAGESIZE);


// --------------------------------------------------------------
// Set up initial memory mappings and turn on MMU.
// --------------------------------------------------------------



// Set up a two-level page table:
// pmap_bootpdir is its linear (virtual) address of the root
// Then turn on paging.
// 
// This function only creates mappings in the kernel part of the address space
// (addresses outside of the range between VM_USERLO and VM_USERHI).
// The user part of the address space remains all PTE_ZERO until later.
//
void
pmap_init(void)
{
	if (cpu_onboot()) {
		// Initialize pmap_bootpdir, the bootstrap page directory.
		// Page directory entries (PDEs) corresponding to the 
		// user-mode address space between VM_USERLO and VM_USERHI
		// should all be initialized to PTE_ZERO (see kern/pmap.h).
		// All virtual addresses below and above this user area
		// should be identity-mapped to the same physical addresses,
		// but only accessible in kernel mode (not in user mode).
		// The easiest way to do this is to use 4MB page mappings.
		// Since these page mappings never change on context switches,
		// we can also mark them global (PTE_G) so the processor
		// doesn't flush these mappings when we reload the PDBR.
#if SOL >= 3
		int i;
		for (i = 0; i < NPDENTRIES; i++)
			pmap_bootpdir[i] = (i << PDXSHIFT)
				| PTE_P | PTE_W | PTE_PS | PTE_G;
		for (i = PDX(VM_USERLO); i < PDX(VM_USERHI); i++)
			pmap_bootpdir[i] = PTE_ZERO;	// clear user area
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

	// If we survived the lcr0, we're running with paging enabled.
	// Now check the page table management functions below.
	if (cpu_onboot())
		pmap_check();
}

//
// Allocate a new page directory, initialized from the bootstrap pdir.
// Returns the new pdir with a reference count of 1.
//
pte_t *
pmap_newpdir(void)
{
	pageinfo *pi = mem_alloc();
	if (pi == NULL)
		return NULL;
	mem_incref(pi);
	pte_t *pdir = mem_pi2ptr(pi);

	// Initialize it from the bootstrap page directory
	assert(sizeof(pmap_bootpdir) == PAGESIZE);
	memmove(pdir, pmap_bootpdir, PAGESIZE);

	return pdir;
}

//
// Free a page directory, and all page tables and mappings it may contain.
//
void
pmap_freepdir(pte_t *pdir)
{
	pmap_remove(pdir, VM_USERLO, VM_USERHI-VM_USERLO);
	mem_free(mem_ptr2pi(pdir));
}


// Given 'pdir', a pointer to a page directory, pmap_walk returns
// a pointer to the page table entry (PTE) for user virtual address 'va'.
// This requires walking the two-level page table structure.
//
// If the relevant page table doesn't exist in the page directory, then:
//    - If writing == 0, pmap_walk returns NULL.
//    - Otherwise, pmap_walk tries to allocate a new page table
//	with mem_alloc.  If this fails, pmap_walk returns NULL.
//    - The new page table is cleared and its refcount set to 1.
//    - Finally, pmap_walk returns a pointer to the requested entry
//	within the new page table.
//
// If the relevant page table does already exist in the page directory,
// but it is read shared and writing != 0, then copy the page table
// to obtain an exclusive copy of it and write-enable the PDE.
//
// Hint: you can turn a pageinfo pointer into the physical address of the
// page it refers to with mem_pi2phys() from kern/mem.h.
//
// Hint 2: the x86 MMU checks permission bits in both the page directory
// and the page table, so it's safe to leave some page permissions
// more permissive than strictly necessary.
pte_t *
pmap_walk(pde_t *pdir, uint32_t va, bool writing)
{
#if SOL >= 3
	assert(va >= VM_USERLO && va < VM_USERHI);

	uint32_t la = va;			// linear = virtual address
	pde_t *pde = &pdir[PDX(la)];		// find PDE
	pte_t *ptab;
	if (*pde & PTE_P) {			// ptab already exist?
		ptab = mem_ptr(PGADDR(*pde));
	} else {				// no - create?
		assert(*pde == PTE_ZERO);
		pageinfo *pi;
		if (!writing || (pi = mem_alloc()) == NULL)
			return NULL;
		mem_incref(pi);
		ptab = mem_pi2ptr(pi);

		// Clear all mappings in the page table
		int i;
		for (i = 0; i < NPTENTRIES; i++)
			ptab[i] = PTE_ZERO;

		// The permissions here are overly generous, but they can
		// be further restricted by the permissions in the page table 
		// entries, if necessary.
		*pde = mem_pi2phys(pi) | PTE_A | PTE_P | PTE_W | PTE_U;
	}

	// If the page table is shared and we're writing, copy it first.
	// Must propagate the read-only status down to the page mappings.
	if (writing && !(*pde & PTE_W)) {
		pageinfo *pi = mem_alloc();
		if (pi == NULL)
			return NULL;
		mem_incref(pi);
		pte_t *nptab = mem_pi2ptr(pi);
		//cprintf("pdir_walk %x %x: copy ptab %x->%x\n",
		//	pdir, va, ptab, nptab);

		// Copy all page table entries, incrementing each's refcount
		int i;
		for (i = 0; i < NPTENTRIES; i++) {
			uint32_t pte = ptab[i];
			nptab[i] = pte & ~PTE_W;
			if (PGADDR(pte) != PTE_ZERO)
				mem_incref(mem_phys2pi(PGADDR(pte)));
		}

		mem_decref(mem_ptr2pi(ptab));	// release old ptab reference
		*pde = (uint32_t)nptab | PTE_A | PTE_P | PTE_W | PTE_U;
		ptab = nptab;
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
// What if this is the only reference to that page?
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
//     the pdir/ptab.
//   - If the region to remove covers a whole 4MB page table region,
//     then unmap and free the page table after unmapping all its contents.
//
// Hint: The TA solution is implemented using pmap_lookup,
// 	pmap_inval, and mem_decref.
//
void
pmap_remove(pde_t *pdir, uint32_t va, size_t size)
{
#if SOL >= 3
	assert(PGOFF(size) == 0);	// must be page-aligned
	assert(va >= VM_USERLO && va < VM_USERHI);
	assert(size <= VM_USERHI - va);

	pmap_inval(pdir, va, size);	// invalidate region we're removing

	uint32_t vahi = va + size;
	while (va < vahi) {
		pde_t *pde = &pdir[PDX(va)];		// find PDE
		if (*pde == PTE_ZERO) {	// no page table - skip 4MB region
			va = PTADDR(va + PTSIZE);	// start of next ptab
			continue;
		}

		// Are we removing the entire page table?
		bool wholeptab = (PTX(va) == 0 && vahi-va >= PTSIZE);

		pte_t *pte = pmap_walk(pdir, va, 1);	// find & unshare PTE
		assert(pte != NULL);	// XXX

		// Remove page mappings up to end of region or page table
		do {
			uint32_t pgaddr = PGADDR(*pte);
			if (pgaddr != PTE_ZERO)
				mem_decref(mem_phys2pi(pgaddr));
			*pte++ = PTE_ZERO;
			va += PAGESIZE;
		} while (va < vahi && PTX(va) != 0);

		// Free the page table too if appropriate
		if (wholeptab) {
			mem_decref(mem_phys2pi(PGADDR(*pde)));
			*pde = PTE_ZERO;
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
	if (p == NULL || p->pdir == pdir) {
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
	assert(sva >= VM_USERLO && sva < VM_USERHI);
	assert(dva >= VM_USERLO && dva < VM_USERHI);
	assert(size <= VM_USERHI - sva);
	assert(size <= VM_USERHI - dva);

	// Invalidate both regions we may be modifying
	pmap_inval(spdir, sva, size);
	pmap_inval(dpdir, dva, size);

	uint32_t svahi = sva + size;
	pde_t *spde = &spdir[PDX(sva)];
	pde_t *dpde = &dpdir[PDX(dva)];
	while (sva < svahi) {

		if (*dpde & PTE_P)	// remove old ptable first
			pmap_remove(dpdir, dva, PTSIZE);
		assert(*dpde == PTE_ZERO);

		*spde &= ~PTE_W;	// remove write permission

		*dpde = *spde;		// copy ptable mapping
		if (*spde != PTE_ZERO)
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
	// It can't be our problem unless it's a write fault in user space!
	if (fva < VM_USERLO || fva >= VM_USERHI || !(tf->tf_err & PFE_WR))
		return;

	proc *p = proc_cur();
	pde_t *pde = &p->pdir[PDX(fva)];
	if (!(*pde & PTE_P))
		return;		// ptab doesn't exist at all - blame user

	// Find the page table entry, copying the page table if it's shared.
	pte_t *pte = pmap_walk(p->pdir, fva, 1);
	if ((*pte & (SYS_READ | SYS_WRITE | PTE_P)) !=
			(SYS_READ | SYS_WRITE | PTE_P))
		return;		// page doesn't exist at all - blame user
	assert(!(*pte & PTE_W));

	// Find the "shared" page.  If refcount is 1, we have the only ref!
	uint32_t pg = PGADDR(*pte);
	if (pg == PTE_ZERO || mem_phys2pi(pg)->refcount > 1) {
		pageinfo *npi = mem_alloc(); assert(npi);
		mem_incref(npi);
		uint32_t npg = mem_pi2phys(npi);
		memmove((void*)npg, (void*)pg, PAGESIZE); // copy the page
		if (pg != PTE_ZERO)
			mem_decref(mem_phys2pi(pg));	// release the old ref
		//cprintf("pmap_pflt %x %x: copy page %x->%x\n",
		//	p->pdir, fva, pg, npg);
		pg = npg;
	}
	*pte = pg | SYS_RW | PTE_A | PTE_D | PTE_W | PTE_U | PTE_P;

	// Make sure the old mapping doesn't get used anymore
	pmap_inval(p->pdir, PGADDR(fva), PAGESIZE);

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
pmap_mergepage(pte_t *rpte, pte_t *spte, pte_t *dpte, uint32_t dva)
{
#if SOL >= 3
	uint8_t *rpg = (uint8_t*)PGADDR(*rpte);
	uint8_t *spg = (uint8_t*)PGADDR(*spte);
	uint8_t *dpg = (uint8_t*)PGADDR(*dpte);
	if (dpg == pmap_zero) return;	// Conflict - just leave dest unmapped

	// Make sure the destination page isn't shared
	if (dpg == (uint8_t*)PTE_ZERO || mem_ptr2pi(dpg)->refcount > 1) {
		pageinfo *npi = mem_alloc(); assert(npi);
		mem_incref(npi);
		uint8_t *npg = mem_pi2ptr(npi);
		memmove(npg, dpg, PAGESIZE); // copy the page
		if (dpg != (uint8_t*)PTE_ZERO)
			mem_decref(mem_ptr2pi(dpg));	// release the old ref
		dpg = npg;
		*dpte = (uint32_t)npg |
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
		mem_decref(mem_phys2pi(PGADDR(*dpte)));
		*dpte = PTE_ZERO;
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
	assert(sva >= VM_USERLO && sva < VM_USERHI);
	assert(dva >= VM_USERLO && dva < VM_USERHI);
	assert(size <= VM_USERHI - sva);
	assert(size <= VM_USERHI - dva);

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
		pte_t *rpte = mem_ptr(PGADDR(*rpde));	// OK if PTE_ZERO
		pte_t *spte = mem_ptr(PGADDR(*spde));	// OK if PTE_ZERO
		pte_t *dpte = pmap_walk(dpdir, dva, 1);	// must exist, unshared
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
				if (PGADDR(*dpte) != PTE_ZERO)
					mem_decref(mem_phys2pi(PGADDR(*dpte)));
				*spte &= ~PTE_W;
				*dpte = *spte;		// copy ptable mapping
				mem_incref(mem_phys2pi(PGADDR(*spte)));
				continue;
			}

			// changed in both spaces - must merge word-by-word
			pmap_mergepage(rpte, spte, dpte, dva);
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
pmap_setperm(pde_t *pdir, uint32_t va, uint32_t size, int perm)
{
#if SOL >= 3
	assert(PGOFF(va) == 0);
	assert(PGOFF(size) == 0);
	assert(va >= VM_USERLO && va < VM_USERHI);
	assert(size <= VM_USERHI - va);
	assert((perm & ~(SYS_RW)) == 0);

	pmap_inval(pdir, va, size);	// invalidate region we're modifying

	// Determine the nominal and actual bits to set or clear
	uint32_t pteand, pteor;
	if (!(perm & SYS_READ))		// clear all permissions
		pteand = ~(SYS_RW | PTE_W | PTE_P), pteor = 0;
	else if (!(perm & SYS_WRITE))	// read-only permission
		pteand = ~(SYS_WRITE | PTE_W),
		pteor = (SYS_READ | PTE_U | PTE_P | PTE_A);
	else	// nominal read/write (but don't add PTE_W to shared mappings!)
		pteand = ~0, pteor = (SYS_RW | PTE_U | PTE_P | PTE_A | PTE_D);

	uint32_t vahi = va + size;
	while (va < vahi) {
		pde_t *pde = &pdir[PDX(va)];		// find PDE
		if (*pde == PTE_ZERO && pteor == 0) {
			// clearing perms, but no page table - skip 4MB region
			va = PTADDR(va + PTSIZE);	// start of next ptab
			continue;
		}

		pte_t *pte = pmap_walk(pdir, va, 1);	// find & unshare PTE
		if (pte == NULL)
			return 0;	// page table alloc failed

		// Adjust page mappings up to end of region or page table
		do {
			*pte = (*pte & pteand) | pteor;
			pte++;
			va += PAGESIZE;
		} while (va < vahi && PTX(va) != 0);
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
static uint32_t
va2pa(pde_t *pdir, uintptr_t va)
{
	pdir = &pdir[PDX(va)];
	if (!(*pdir & PTE_P))
		return ~0;
	pte_t *ptab = mem_ptr(PGADDR(*pdir));
	if (!(ptab[PTX(va)] & PTE_P))
		return ~0;
	return PGADDR(ptab[PTX(va)]);
}

// check pmap_insert, pmap_remove, &c
void
pmap_check(void)
{
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
	assert(pmap_insert(pmap_bootpdir, pi1, VM_USERLO, 0) == NULL);

	// free pi0 and try again: pi0 should be used for page table
	mem_free(pi0);
	assert(pmap_insert(pmap_bootpdir, pi1, VM_USERLO, 0) != NULL);
	assert(PGADDR(pmap_bootpdir[PDX(VM_USERLO)]) == mem_pi2phys(pi0));
	assert(va2pa(pmap_bootpdir, VM_USERLO) == mem_pi2phys(pi1));
	assert(pi1->refcount == 1);
	assert(pi0->refcount == 1);

	// should be able to map pi2 at VM_USERLO+PAGESIZE
	// because pi0 is already allocated for page table
	assert(pmap_insert(pmap_bootpdir, pi2, VM_USERLO+PAGESIZE, 0));
	assert(va2pa(pmap_bootpdir, VM_USERLO+PAGESIZE) == mem_pi2phys(pi2));
	assert(pi2->refcount == 1);

	// should be no free memory
	assert(mem_alloc() == NULL);

	// should be able to map pi2 at VM_USERLO+PAGESIZE
	// because it's already there
	assert(pmap_insert(pmap_bootpdir, pi2, VM_USERLO+PAGESIZE, 0));
	assert(va2pa(pmap_bootpdir, VM_USERLO+PAGESIZE) == mem_pi2phys(pi2));
	assert(pi2->refcount == 1);

	// pi2 should NOT be on the free list
	// could hapien in ref counts are handled slopiily in pmap_insert
	assert(mem_alloc() == NULL);

	// check that pmap_walk returns a pointer to the pte
	ptep = mem_ptr(PGADDR(pmap_bootpdir[PDX(VM_USERLO+PAGESIZE)]));
	assert(pmap_walk(pmap_bootpdir, VM_USERLO+PAGESIZE, 0)
		== ptep+PTX(VM_USERLO+PAGESIZE));

	// should be able to change permissions too.
	assert(pmap_insert(pmap_bootpdir, pi2, VM_USERLO+PAGESIZE, PTE_U));
	assert(va2pa(pmap_bootpdir, VM_USERLO+PAGESIZE) == mem_pi2phys(pi2));
	assert(pi2->refcount == 1);
	assert(*pmap_walk(pmap_bootpdir, VM_USERLO+PAGESIZE, 0) & PTE_U);
	assert(pmap_bootpdir[PDX(VM_USERLO)] & PTE_U);
	
	// should not be able to map at VM_USERLO+PTSIZE
	// because we need a free page for a page table
	assert(pmap_insert(pmap_bootpdir, pi0, VM_USERLO+PTSIZE, 0) == NULL);

	// insert pi1 at VM_USERLO+PAGESIZE (replacing pi2)
	assert(pmap_insert(pmap_bootpdir, pi1, VM_USERLO+PAGESIZE, 0));
	assert(!(*pmap_walk(pmap_bootpdir, VM_USERLO+PAGESIZE, 0) & PTE_U));

	// should have pi1 at both +0 and +PAGESIZE, pi2 nowhere, ...
	assert(va2pa(pmap_bootpdir, VM_USERLO+0) == mem_pi2phys(pi1));
	assert(va2pa(pmap_bootpdir, VM_USERLO+PAGESIZE) == mem_pi2phys(pi1));
	// ... and ref counts should reflect this
	assert(pi1->refcount == 2);
	assert(pi2->refcount == 0);

	// pi2 should be returned by mem_alloc
	assert(mem_alloc() == pi2);

	// unmapping pi1 at VM_USERLO+0 should keep pi1 at +PAGESIZE
	pmap_remove(pmap_bootpdir, VM_USERLO+0, PAGESIZE);
	assert(va2pa(pmap_bootpdir, VM_USERLO+0) == ~0);
	assert(va2pa(pmap_bootpdir, VM_USERLO+PAGESIZE) == mem_pi2phys(pi1));
	assert(pi1->refcount == 1);
	assert(pi2->refcount == 0);
	assert(mem_alloc() == NULL);	// still should have no pages free

	// unmapping pi1 at VM_USERLO+PAGESIZE should free it
	pmap_remove(pmap_bootpdir, VM_USERLO+PAGESIZE, PAGESIZE);
	assert(va2pa(pmap_bootpdir, VM_USERLO+0) == ~0);
	assert(va2pa(pmap_bootpdir, VM_USERLO+PAGESIZE) == ~0);
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
	pmap_insert(pmap_bootpdir, pi1, VM_USERLO, 0);
	assert(pi1->refcount == 1);
	assert(*(int*)VM_USERLO == 0x01010101);
	pmap_insert(pmap_bootpdir, pi2, VM_USERLO, 0);
	assert(*(int*)VM_USERLO == 0x02020202);
	assert(pi2->refcount == 1);
	assert(pi1->refcount == 0);
	assert(mem_alloc() == pi1);
	pmap_remove(pmap_bootpdir, VM_USERLO, PAGESIZE);
	assert(pi2->refcount == 0);
	assert(mem_alloc() == pi2);

	// now use a pmap_remove on a large region to take pi0 back
	pmap_remove(pmap_bootpdir, VM_USERLO, VM_USERHI-VM_USERLO);
	assert(pmap_bootpdir[PDX(VM_USERLO)] == PTE_ZERO);
	assert(pi0->refcount == 0);
	assert(mem_alloc() == pi0);
	assert(mem_freelist == NULL);

	// test pmap_remove with large, non-ptable-aligned regions
	mem_free(pi1);
	uintptr_t va = VM_USERLO;
	assert(pmap_insert(pmap_bootpdir, pi0, va, 0));
	assert(pmap_insert(pmap_bootpdir, pi0, va+PAGESIZE, 0));
	assert(pmap_insert(pmap_bootpdir, pi0, va+PTSIZE-PAGESIZE, 0));
	assert(PGADDR(pmap_bootpdir[PDX(VM_USERLO)]) == mem_pi2phys(pi1));
	assert(mem_freelist == NULL);
	mem_free(pi2);
	assert(pmap_insert(pmap_bootpdir, pi0, va+PTSIZE, 0));
	assert(pmap_insert(pmap_bootpdir, pi0, va+PTSIZE+PAGESIZE, 0));
	assert(pmap_insert(pmap_bootpdir, pi0, va+PTSIZE*2-PAGESIZE, 0));
	assert(PGADDR(pmap_bootpdir[PDX(VM_USERLO+PTSIZE)])
		== mem_pi2phys(pi2));
	assert(mem_freelist == NULL);
	mem_free(pi3);
	assert(pmap_insert(pmap_bootpdir, pi0, va+PTSIZE*2, 0));
	assert(pmap_insert(pmap_bootpdir, pi0, va+PTSIZE*2+PAGESIZE, 0));
	assert(pmap_insert(pmap_bootpdir, pi0, va+PTSIZE*3-PAGESIZE*2, 0));
	assert(pmap_insert(pmap_bootpdir, pi0, va+PTSIZE*3-PAGESIZE, 0));
	assert(PGADDR(pmap_bootpdir[PDX(VM_USERLO+PTSIZE*2)])
		== mem_pi2phys(pi3));
	assert(mem_freelist == NULL);
	assert(pi0->refcount == 10);
	assert(pi1->refcount == 1);
	assert(pi2->refcount == 1);
	assert(pi3->refcount == 1);
	pmap_remove(pmap_bootpdir, va+PAGESIZE, PTSIZE*3-PAGESIZE*2);
	assert(pi0->refcount == 2);
	assert(pi2->refcount == 0); assert(mem_alloc() == pi2);
	assert(mem_freelist == NULL);
	pmap_remove(pmap_bootpdir, va, PTSIZE*3-PAGESIZE);
	assert(pi0->refcount == 1);
	assert(pi1->refcount == 0); assert(mem_alloc() == pi1);
	assert(mem_freelist == NULL);
	pmap_remove(pmap_bootpdir, va+PTSIZE*3-PAGESIZE, PAGESIZE);
	assert(pi0->refcount == 0);	// pi3 might or might not also be freed
	pmap_remove(pmap_bootpdir, va+PAGESIZE, PTSIZE*3);
	assert(pi3->refcount == 0);
	mem_alloc(); mem_alloc();	// collect pi0 and pi3
	assert(mem_freelist == NULL);

	// check pointer arithmetic in pmap_walk
	mem_free(pi0);
	va = VM_USERLO + PAGESIZE*NPTENTRIES + PAGESIZE;
	ptep = pmap_walk(pmap_bootpdir, va, 1);
	ptep1 = mem_ptr(PGADDR(pmap_bootpdir[PDX(va)]));
	assert(ptep == ptep1 + PTX(va));
	pmap_bootpdir[PDX(va)] = PTE_ZERO;
	pi0->refcount = 0;

	// check that new page tables get cleared
	memset(mem_pi2ptr(pi0), 0xFF, PAGESIZE);
	mem_free(pi0);
	pmap_walk(pmap_bootpdir, VM_USERHI-PAGESIZE, 1);
	ptep = mem_pi2ptr(pi0);
	for(i=0; i<NPTENTRIES; i++)
		assert(ptep[i] == PTE_ZERO);
	pmap_bootpdir[PDX(VM_USERHI-PAGESIZE)] = PTE_ZERO;
	pi0->refcount = 0;

	// give free list back
	mem_freelist = fl;

	// free the pages we filched
	mem_free(pi0);
	mem_free(pi1);
	mem_free(pi2);
	mem_free(pi3);

	cprintf("pmap_check() succeeded!\n");
#if LAB >= 99
	// More things we should test:
	// - does trap() call pmap_fault() before recovery?
	// - does syscall_checkva avoid wraparound issues?
#endif
}

#endif /* LAB >= 3 */
