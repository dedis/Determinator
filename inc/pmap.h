#ifndef _PMAP_H_
#define _PMAP_H_

#include <inc/types.h>
#include <inc/queue.h>
#include <inc/mmu.h>

LIST_HEAD(Page_list, Page);
typedef LIST_ENTRY(Page) Page_LIST_entry_t;

struct Page {
	Page_LIST_entry_t pp_link;	/* free list link */

	//
	// ref is set to the number of address
	// spaces this physical page is mapped.
	//
	// N.B.: the physical pages which are used
	// either as a page directory or a page table
	// have pp_refcnt = 0.
	//

	u_short pp_ref;
};

#endif /* _PMAP_H_ */

