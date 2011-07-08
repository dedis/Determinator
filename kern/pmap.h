#if LAB >= 3
/*
 * Page mapping and page directory/table management definitions.
 *
 * Copyright (C) 1997 Massachusetts Institute of Technology
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Derived from the MIT Exokernel and JOS.
 * Adapted for PIOS by Bryan Ford at Yale University.
 */

#ifndef PIOS_KERN_PMAP_H
#define PIOS_KERN_PMAP_H
#ifndef PIOS_KERNEL
# error "This is a kernel header; user programs should not #include it"
#endif

#include <inc/assert.h>
#include <inc/mmu.h>
#include <inc/vm.h>

#include <kern/mem.h>


// PML4/PDP/PD/PT entries are 64-bit integers.
typedef uintptr_t pte_t;


// Bootstrap page map root that identity-maps the kernel's address space.
// For 64-bit the root is a page map level-4 with NPRENTRIES entries.
extern pte_t pmap_bootpmap[NPRENTRIES];

// Statically allocated page that we always keep set to all zeros.
extern uint8_t pmap_zero[PAGESIZE];

// Memory mappings representing cleared (zero) memory
// always have a pointer to pmap_zero in the PGADDR part of their PTE.
// Such zero mappings do NOT increment the refcount on the pmap_zero page.
//
// A zero mapping containing PTE_ZERO in its PGADDR part
// but have SYS_READ and potentially SYS_WRITE nominal permissions.
// A zero mapping with SYS_READ also has PTE_P (present) set,
// but a zero mapping with SYS_WRITE never has PTE_W (writeable) set -
// instead the page fault handler creates copies of the zero page on demand.
#define PTE_ZERO	((intptr_t)pmap_zero)


void pmap_init(void);
pte_t *pmap_newpmap(void);
void pmap_freepmap(pageinfo *pml4pi);
void pmap_freepmap_level(pageinfo *pmap, int pmlevel);
void pmap_freeptab(pageinfo *ptabpi);
pte_t *pmap_walk(pte_t *pml4, intptr_t uva, bool writing);
pte_t *pmap_insert(pte_t *pml4, pageinfo *pi, intptr_t uva, int perm);
void pmap_remove(pte_t *pml4, intptr_t uva, size_t size);
void pmap_inval(pte_t *pml4, intptr_t uva, size_t size);
int pmap_copy(pte_t *spml4, intptr_t sva, pte_t *dpml4, intptr_t dva,
		size_t size);
int pmap_merge(pte_t *rpml4, pte_t *spdir, intptr_t sva,
		pte_t *dpml4, intptr_t dva, size_t size);
int pmap_setperm(pte_t *pml4, intptr_t va, size_t size, int perm);
void pmap_pagefault(trapframe *tf);
void pmap_check(void);


#endif /* !PIOS_KERN_PMAP_H */
#endif // LAB >= 3
