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
extern unsigned int boot_cr3;
extern Pde *boot_pgdir;

void i386_vm_init();
void i386_detect_memory();
void page_init(void);
void page_check(void);
int  page_alloc(struct Page **);
void page_free(struct Page *);
int  page_insert(Pde *, struct Page *, u_long, u_long);
void page_remove(Pde *, u_long va);
void tlb_invalidate(Pde *, u_long va);

static inline u_long
page2ppn(struct Page *pp)
{
	return pp - page;
}

static inline u_long
page2pa(struct Page *pp)
{
	return page2ppn(pp)<<PGSHIFT;
}

static inline struct Page *
pa2page(u_long pa)
{
	if (pa >= npage)
		panic("pa2page called with invalid pa");
	return &pages[PPN(pa)];
}

int pgdir_walk(Pde *pgdir, u_long va, int create, Pte **ppte);

#endif /* _KERN_PMAP_H_ */

