/* See COPYRIGHT for copyright information. */

#ifndef _KERN_PMAP_H_
#define _KERN_PMAP_H_

#include <inc/pmap.h>
#include <inc/trap.h>
#include <kern/printf.h>

extern struct Segdesc gdt[];
extern struct Pseudodesc gdt_pd;
extern struct Page *pages;
extern u_long npage;
extern unsigned int p0cr3_boot;
extern Pde *p0pgdir_boot;

void i386_vm_init();
void i386_detect_memory();
void page_init(void);
void page_check(void);
int  page_alloc(struct Page **);
void page_free(struct Page *);
int  page_insert(Pde *, struct Page *, u_int, u_int);
void page_remove(Pde *, u_int va);
void tlb_invalidate(u_int, Pde *);

/* translates from a Page structure to a physical address */
#define pp2pa(ppage_p) ((u_long) (pp2ppn(ppage_p) << PGSHIFT))

/* translates from a Page structure to a virtual address */
#define pp2va(ppage_p) ((void *) (KERNBASE + pp2pa(ppage_p)))

static inline int 
page2ppn(struct Page* pp)
{
	return((pp) - page);
}

static inline struct Page* 
kva2pp(u_long kva)
{
	u_long ppn = ((u_long) (kva) - KERNBASE) >> PGSHIFT;
	if (ppn >= nppage)
		panic("%s:%d: kva2pp called with invalid kva",
			__FUNCTION__, __LINE__);
	return &ppages[ppn];
}

int pgdir_check_and_alloc(Pde *pgdir, u_int va);
Pte *pgdir_get_ptep(Pde *pgdir, u_int va);
int pmap_insert_pte(Pde *pd, u_int va, Pte pte);

#endif /* _KERN_PMAP_H_ */

