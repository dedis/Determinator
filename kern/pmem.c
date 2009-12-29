#if LAB >= 1
// Physical memory management.
// See COPYRIGHT for copyright information.

#include <inc/x86.h>
#include <inc/mmu.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/pmap.h>
#include <kern/kclock.h>
#if LAB >= 3
#include <kern/env.h>
#endif



// Global descriptor table.
//
// The kernel and user segments are identical (except for the DPL).
// To load the SS register, the CPL must equal the DPL.  Thus,
// we must duplicate the segments for the user and the kernel.
//
struct Segdesc gdt[] =
{
	// 0x0 - unused (always faults -- for trapping NULL far pointers)
	SEG_NULL,

	// 0x8 - kernel code segment
	[GD_KT >> 3] = SEG(STA_X | STA_R, 0x0, 0xffffffff, 0),

	// 0x10 - kernel data segment
	[GD_KD >> 3] = SEG(STA_W, 0x0, 0xffffffff, 0),

	// 0x18 - user code segment
	[GD_UT >> 3] = SEG(STA_X | STA_R, 0x0, 0xffffffff, 3),

	// 0x20 - user data segment
	[GD_UD >> 3] = SEG(STA_W, 0x0, 0xffffffff, 3),

	// 0x28 - tss, initialized in idt_init()
	[GD_TSS >> 3] = SEG_NULL
};

struct Pseudodesc gdt_pd = {
	sizeof(gdt) - 1, (unsigned long) gdt
};


size_t pmem_max;		// Maximum physical address
size_t pmem_npage;		// Total number of physical memory pages

pageinfo *pmem_pageinfo;	// Metadata array indexed by page number

pageinfo *pmem_free;		// Start of free page list
spinlock pmem_free_lock;	// Spinlock protecting the free page list



void
pmem_init(void)
{
	// The linker generates the special symbol 'end'
	// representing the end of the program's statically assigned
	// code, data, and bss (uninitialized data) sections:
	// this is where the dynamically allocated heap typically starts,
	// and in our case is where we'll put the pageinfo array.
	extern char end[];

	size_t basemem;		// Amount of base memory in bytes
	size_t extmem;		// Amount of extended memory in bytes
	void *freemem;		// Pointer used to assign free memory
	int i, inuse;


	// Determine how much base (<640K) and extended (>1MB) memory
	// is available in the system,
	// by reading the PC's BIOS-managed nonvolatile RAM (NVRAM).
	// The NVRAM tells us how many kilobytes there are.
	basemem = ROUNDDOWN(nvram_read(NVRAM_BASELO)*1024, PAGESIZE);
	extmem = ROUNDDOWN(nvram_read(NVRAM_EXTLO)*1024, PAGESIZE);

	// The maximum physical address is the top of extended memory.
	pmem_max = EXTPHYSMEM + extmem;

	// Compute the total number of physical pages (including I/O space)
	pmem_npage = pmem_max / PAGESIZE;

	cprintf("Physical memory: %dK available, ", (int)(maxpa/1024));
	cprintf("base = %dK, extended = %dK\n", (int)(basemem/1024), (int)(extmem/1024));


#if SOL >= 1
	// Now that we know the size of physical memory,
	// reserve enough space for the pageinfo array
	// just past our statically-assigned program code/data/bss,
	// which the linker placed at the start of extended memory.
	// Make sure the pageinfo entries are naturally aligned.
	pmem_pageinfo = (pageinfo *) ROUNDUP((size_t) end, sizeof(pageinfo));

	// Initialize the entire pageinfo array to zero for good measure.
	memset(pmem_pageinfo, 0, sizeof(pageinfo) * pmem_npage);

	// Free extended memory starts just past the pageinfo array.
	freemem = &pageinfo[pmem_npage];


	// Align freemem to page boundary.
	freemem = ROUNDUP(freemem, PAGESIZE);

	// Chain all the available physical pages onto the free page list.
	pmem_free = 0;		// Initialize pmem_free list to empty.
	for (i = 0; i < npage; i++) {
		// Off-limits until proven otherwise.
		inuse = 1;

		// The bottom basemem bytes are free except page 0.
		if (i != 0 && i < basemem / PAGESIZE)
			inuse = 0;

		// The IO hole and the kernel abut.

		// The memory past the kernel is free.
		if (i >= PADDR(boot_freemem) / PAGESIZE)
			inuse = 0;

		pmem_pageinfo[i].refcount = inuse;
		if (!inuse) {
			// Insert page into the free page list.
			pmem_pageinfo[i].free_next = pmem_free;
			pmem_free = &pmem_pageinfo[i];
		}
	}

#else /* not SOL >= 1 */
	// The example code here marks all pages as free.
	// However this is not truly the case.  What memory is free?
	//  1) Mark page 0 as in use.
	//     This way we preserve the real-mode IDT and BIOS structures
	//     in case we ever need them.  (Currently we don't, but...)
	//  2) Mark the rest of base memory as free.
	//  3) Then comes the IO hole [IOPHYSMEM, EXTPHYSMEM).
	//     Mark it as in use so that it can never be allocated.      
	//  4) Then extended memory [EXTPHYSMEM, ...).
	//     Some of it is in use, some is free. Where is the kernel?
	//     Which pages are used for page tables and other data structures?
	//
	// Change the code to reflect this.
	int i;
	pmem_free = 0;		// Initialize pmem_free list to empty.
	for (i = 0; i < npage; i++) {
		// A free page has no references to it.
		pmem_pageinfo[i].refcount = 0;

		// Add the page to the pmem_free list.
		pmem_pageinfo[i].free_next = pmem_free;
		pmem_free = &pmem_pageinfo[i];
	}
#endif /* not SOL >= 1 */

	// Check to make sure the page allocator seems to work correctly.
	pmem_check();
}

//
// Allocates a physical page from the page free list.
// Does NOT set the contents of the physical page to zero -
// the caller must do that if necessary.
//
// RETURNS 
//   - a pointer to the page's pageinfo struct if successful
//   - NULL if no available physical pages.
//
// Hint: pi->refs should not be incremented 
// Hint: be sure to use proper mutual exclusion for multiprocessor operation.
pageinfo *
pmem_alloc(void)
{
	// Fill this function in
#if SOL >= 2
	spinlock_acquire(&pmem_free_lock);

	pageinfo *pi = pmem_free;
	if (pi != NULL) {
		pmem_free = pi->free_next;	// Remove page from free list
		pi->free_next = NULL;		// Mark it not on the free list
	}

	spinlock_release(&pmem_free_lock);

	return pi;	// Return pageinfo pointer or NULL

#else
	// Fill this function in.
	panic("pmem_alloc not implemented.");
#endif /* not SOL >= 2 */
}

//
// Return a page to the free list, given its pageinfo pointer.
// (This function should only be called when pp->pp_ref reaches 0.)
//
void
pmem_free(pageinfo *pi)
{
#if SOL >= 2
	if (pi->refs != 0)
		panic("pmem_free: attempt to free in-use page");
	if (pi->free_next != NULL)
		panic("pmem_free: attempt to free already free page!");

	spinlock_acquire(&pmem_free_lock);

	// Insert the page at the head of the free list.
	pi->free_next = pmem_free;
	pmem_free = pi;

	spinlock_release(&pmem_free_lock);
#else /* not SOL >= 2 */
	// Fill this function in.
	panic("pmem_free not implemented.");
#endif /* not SOL >= 2 */
}

//
// Decrement the reference count on a page,
// freeing it if there are no more refs.
//
void
page_decref(pageinfo* pp)
{
	if (--pp->pp_ref == 0)
		page_free(pp);
}


//
// Check the physical page allocator (pmem_alloc(), pmem_free())
// for correct operation after initialization via pmem_init().
//
void
pmem_check()
{
	pageinfo *pp, *pp0, *pp1, *pp2;
	pageinfo fl;
	int i;

        // if there's a page that shouldn't be on
        // the free list, try to make sure it
        // eventually causes trouble.
	for (i = pmem_free; i != 0; i = pmem_pageinfo[i].free_next)
		memset((void*)(i * PAGESIZE), 0x97, 128);

	// should be able to allocate three pages
	pp0 = pp1 = pp2 = 0;
	assert(page_alloc(&pp0) == 0);
	assert(page_alloc(&pp1) == 0);
	assert(page_alloc(&pp2) == 0);

	assert(pp0);
	assert(pp1 && pp1 != pp0);
	assert(pp2 && pp2 != pp1 && pp2 != pp0);
        assert(page2pa(pp0) < npage*PAGESIZE);
        assert(page2pa(pp1) < npage*PAGESIZE);
        assert(page2pa(pp2) < npage*PAGESIZE);

	// temporarily steal the rest of the free pages
	fl = page_free_list;
	LIST_INIT(&page_free_list);

	// should be no free memory
	assert(page_alloc(&pp) == -E_NO_MEM);

        // free and re-allocate?
        page_free(pp0);
        page_free(pp1);
        page_free(pp2);
	pp0 = pp1 = pp2 = 0;
	assert(page_alloc(&pp0) == 0);
	assert(page_alloc(&pp1) == 0);
	assert(page_alloc(&pp2) == 0);
	assert(pp0);
	assert(pp1 && pp1 != pp0);
	assert(pp2 && pp2 != pp1 && pp2 != pp0);
	assert(page_alloc(&pp) == -E_NO_MEM);

	// give free list back
	page_free_list = fl;

	// free the pages we took
	page_free(pp0);
	page_free(pp1);
	page_free(pp2);

	cprintf("pmem_check() succeeded!\n");
}

#endif /* LAB >= 3 */
