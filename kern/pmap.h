#if LAB >= 3
/* See COPYRIGHT for copyright information. */

#ifndef PIOS_KERN_PMAP_H
#define PIOS_KERN_PMAP_H
#ifndef PIOS_KERNEL
# error "This is a kernel header; user programs should not #include it"
#endif

#include <inc/assert.h>
#include <inc/mmu.h>
#include <inc/vm.h>

#include <kern/mem.h>


// Page directory entries and page table entries are 32-bit integers.
typedef uint32_t pde_t;
typedef uint32_t pte_t;


// Bootstrap page directory that identity-maps the kernel's address space.
extern pde_t pmap_bootpdir[1024];

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
#define PTE_ZERO	((uint32_t)pmap_zero)


void pmap_init(void);
pte_t *pmap_newpdir(void);
void pmap_freepdir(pageinfo *pdirpi);
void pmap_freeptab(pageinfo *ptabpi);
pte_t *pmap_walk(pde_t *pdir, uint32_t uva, bool writing);
pte_t *pmap_insert(pde_t *pdir, pageinfo *pi, uint32_t uva, int perm);
void pmap_remove(pde_t *pdir, uint32_t uva, size_t size);
void pmap_inval(pde_t *pdir, uint32_t uva, size_t size);
int pmap_copy(pde_t *spdir, uint32_t sva, pde_t *dpdir, uint32_t dva,
		size_t size);
int pmap_merge(pde_t *rpdir, pde_t *spdir, uint32_t sva,
		pde_t *dpdir, uint32_t dva, size_t size);
int pmap_setperm(pde_t *pdir, uint32_t va, uint32_t size, int perm);
void pmap_pagefault(trapframe *tf);
void pmap_check(void);


#endif /* !PIOS_KERN_PMAP_H */
#endif // LAB >= 3
