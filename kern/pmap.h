/* See COPYRIGHT for copyright information. */

#ifndef _KERN_PMAP_H_
#define _KERN_PMAP_H_

#include <inc/pmap.h>
#include <inc/trap.h>
#include <kern/printf.h>

extern struct seg_desc gdt[];
extern struct pseudo_desc gdt_pd;
extern struct Ppage *ppages;
extern u_long nppage;
extern unsigned int p0cr3_boot;
extern Pde *p0pgdir_boot;

void i386_vm_init ();
void i386_detect_memory ();
void ppage_init (void);
int  ppage_alloc (struct Ppage **);
void ppage_free (struct Ppage *);
int  ppage_insert (Pde *, struct Ppage *, u_int, u_int);
void ppage_remove (Pde *, u_int va);
void tlb_invalidate (u_int, Pde *);

/* translates from a Ppage structure to a physical address */
#define pp2pa(ppage_p) ((u_long) (pp2ppn (ppage_p) << PGSHIFT))

/* translates from a Ppage structure to a virtual address */
#define pp2va(ppage_p) ((void *) (KERNBASE + pp2pa (ppage_p)))

static inline int 
pp2ppn(struct Ppage* pp)
{
  return ((pp) - ppages);
}

static inline struct Ppage* 
kva2pp(u_long kva)
{
  u_long ppn = ((u_long) (kva) - KERNBASE) >> PGSHIFT;
  if (ppn >= nppage)
    panic ("%s:%d: kva2pp called with invalid kva",
	   __FUNCTION__, __LINE__);
  return &ppages[ppn];
}

int pgdir_check_and_alloc (Pde *pgdir, u_int va);
Pte *pgdir_get_ptep (Pde *pgdir, u_int va);
int pmap_insert_pte (Pde *pd, u_int va, Pte pte);

#endif /* _KERN_PMAP_H_ */

