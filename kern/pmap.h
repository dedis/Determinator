/* See COPYRIGHT for copyright information. */

#ifndef _KERN_PMAP_H_
#define _KERN_PMAP_H_

#include <inc/pmap.h>
#include <inc/trap.h>
#include <inc/assert.h>

extern struct Segdesc gdt[];
extern struct Pseudodesc gdt_pd;
extern struct Page *pages;
extern u_long npage;
extern u_long boot_cr3;
extern Pde *boot_pgdir;

void i386_vm_init();
void i386_detect_memory();
void page_init(void);
void page_check(void);
int  page_alloc(struct Page **);
void page_free(struct Page *);
int  page_insert(Pde *, struct Page *, u_long, u_int);
void page_remove(Pde *, u_long va);
struct Page *page_lookup(Pde*, u_long, Pte**);
void page_decref(struct Page*);
void tlb_invalidate(Pde *, u_long va);

static inline u_long
page2ppn(struct Page *pp)
{
	return pp - pages;
}

static inline u_long
page2pa(struct Page *pp)
{
	return page2ppn(pp)<<PGSHIFT;
}

static inline struct Page *
pa2page(u_long pa)
{
	if (PPN(pa) >= npage)
		panic("pa2page called with invalid pa");
	return &pages[PPN(pa)];
}

static inline u_long
page2kva(struct Page *pp)
{
	return KADDR(page2pa(pp));
}

int pgdir_walk(Pde *pgdir, u_long va, int create, Pte **ppte);

#endif /* _KERN_PMAP_H_ */

