
/*
 * Copyright (C) 1997 Massachusetts Institute of Technology 
 *
 * This software is being provided by the copyright holders under the
 * following license. By obtaining, using and/or copying this software,
 * you agree that you have read, understood, and will comply with the
 * following terms and conditions:
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose and without fee or royalty is
 * hereby granted, provided that the full text of this NOTICE appears on
 * ALL copies of the software and documentation or portions thereof,
 * including modifications, that you make.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS," AND COPYRIGHT HOLDERS MAKE NO
 * REPRESENTATIONS OR WARRANTIES, EXPRESS OR IMPLIED. BY WAY OF EXAMPLE,
 * BUT NOT LIMITATION, COPYRIGHT HOLDERS MAKE NO REPRESENTATIONS OR
 * WARRANTIES OF MERCHANTABILITY OR FITNESS FOR ANY PARTICULAR PURPOSE OR
 * THAT THE USE OF THE SOFTWARE OR DOCUMENTATION WILL NOT INFRINGE ANY
 * THIRD PARTY PATENTS, COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS. COPYRIGHT
 * HOLDERS WILL BEAR NO LIABILITY FOR ANY USE OF THIS SOFTWARE OR
 * DOCUMENTATION.
 *
 * The name and trademarks of copyright holders may NOT be used in
 * advertising or publicity pertaining to the software without specific,
 * written prior permission. Title to copyright in this software and any
 * associated documentation will at all times remain with copyright
 * holders. See the file AUTHORS which should have accompanied this software
 * for a list of all copyright holders.
 *
 * This file may be derived from previously copyrighted software. This
 * copyright applies only to those changes made by the copyright
 * holders listed in the AUTHORS file. The rest of this file is covered by
 * the copyright notices, if any, listed below.
 */

#ifndef _PMAP_H_
#define _PMAP_H_

#include <kern/inc/types.h>
#include <kern/inc/queue.h>
#include <kern/inc/mmu.h>
#include <kern/inc/printf.h>
#include <kern/inc/trap.h>

struct Env;

LIST_HEAD(Ppage_list, Ppage);
typedef LIST_ENTRY(Ppage) Ppage_LIST_entry_t;

struct Ppage {
  Ppage_LIST_entry_t pp_link;     /* free list link */

  //
  // refcnt is set to the number of address
  // spaces this physical page is mapped.
  //
  // N.B.: the physical pages which are used
  // either as a page directory or a page table
  // have pp_refcnt = 0.
  //

  u_short pp_refcnt;              /* refcount */ 
};


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

#endif /* _PMAP_H_ */

