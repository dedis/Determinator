#ifndef _PMAP_H_
#define _PMAP_H_

#include <inc/types.h>
#include <inc/queue.h>
#include <inc/mmu.h>

LIST_HEAD(Page_list, Page);
typedef LIST_ENTRY(Page) Page_LIST_entry_t;

struct Page {
	Page_LIST_entry_t pp_link;	/* free list link */

	// Ref is the count of pointers (usually in page table entries)
	// to this page.  This only holds for pages allocated using 
	// page_alloc.  Pages allocated at boot time using pmap.c's "alloc"
	// do not have valid reference count fields.

	u_short pp_ref;
};

#endif /* _PMAP_H_ */

