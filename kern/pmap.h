#if LAB >= 3
/* See COPYRIGHT for copyright information. */

#ifndef PIOS_KERN_PMAP_H
#define PIOS_KERN_PMAP_H
#ifndef PIOS_KERNEL
# error "This is a kernel header; user programs should not #include it"
#endif

#include <inc/assert.h>
#include <inc/vm.h>

#include <kern/mem.h>


// Page directory entries and page table entries are 32-bit integers.
typedef uint32_t pde_t;
typedef uint32_t pte_t;


// Bootstrap page directory that identity-maps the kernel's address space.
extern pde_t pmap_bootpdir[1024];


void pmap_init(void);
pte_t *pmap_newpdir(void);
void pmap_freepdir(pte_t *pdir);
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
