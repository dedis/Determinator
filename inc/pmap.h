#ifndef _PMAP_H_
#define _PMAP_H_

#include <inc/types.h>
#include <inc/queue.h>
#include <inc/mmu.h>

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

#endif /* _PMAP_H_ */

