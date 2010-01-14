#if LAB >= 1
// Physical memory management.
// See COPYRIGHT for copyright information.

#include <inc/x86.h>
#include <inc/mmu.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/spinlock.h>
#include <kern/mem.h>

#include <dev/nvram.h>


size_t mem_max;			// Maximum physical address
size_t mem_npage;		// Total number of physical memory pages

pageinfo *mem_pageinfo;		// Metadata array indexed by page number

pageinfo *mem_freelist;		// Start of free page list
spinlock mem_freelock;		// Spinlock protecting the free page list


void mem_check(void);

void
mem_init(void)
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
	basemem = ROUNDDOWN(nvram_read16(NVRAM_BASELO)*1024, PAGESIZE);
	extmem = ROUNDDOWN(nvram_read16(NVRAM_EXTLO)*1024, PAGESIZE);

	// The maximum physical address is the top of extended memory.
	mem_max = MEM_EXT + extmem;

	// Compute the total number of physical pages (including I/O space)
	mem_npage = mem_max / PAGESIZE;

	cprintf("Physical memory: %dK available, ", (int)(mem_max/1024));
	cprintf("base = %dK, extended = %dK\n",
		(int)(basemem/1024), (int)(extmem/1024));


#if SOL >= 1
	// Now that we know the size of physical memory,
	// reserve enough space for the pageinfo array
	// just past our statically-assigned program code/data/bss,
	// which the linker placed at the start of extended memory.
	// Make sure the pageinfo entries are naturally aligned.
	mem_pageinfo = (pageinfo *) ROUNDUP((size_t) end, sizeof(pageinfo));

	// Initialize the entire pageinfo array to zero for good measure.
	memset(mem_pageinfo, 0, sizeof(pageinfo) * mem_npage);

	// Free extended memory starts just past the pageinfo array.
	freemem = &mem_pageinfo[mem_npage];


	// Align freemem to page boundary.
	freemem = ROUNDUP(freemem, PAGESIZE);

	// Chain all the available physical pages onto the free page list.
	spinlock_init(&mem_freelock, "mem_freelist lock");
	mem_freelist = 0;	// Initialize mem_freelist to empty.
	for (i = 0; i < mem_npage; i++) {
		// Off-limits until proven otherwise.
		inuse = 1;

		// The bottom basemem bytes are free except page 0.
		if (i != 0 && i < basemem / PAGESIZE)
			inuse = 0;

		// The IO hole and the kernel abut.

		// The memory past the kernel is free.
		if (i >= mem_phys(freemem) / PAGESIZE)
			inuse = 0;

		mem_pageinfo[i].refcount = inuse;
		if (!inuse) {
			// Insert page into the free page list.
			mem_pageinfo[i].free_next = mem_freelist;
			mem_freelist = &mem_pageinfo[i];
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
	mem_free = 0;		// Initialize mem_free list to empty.
	for (i = 0; i < npage; i++) {
		// A free page has no references to it.
		mem_pageinfo[i].refcount = 0;

		// Add the page to the mem_freelist.
		mem_pageinfo[i].free_next = mem_freelist;
		mem_freelist = &mem_pageinfo[i];
	}
#endif /* not SOL >= 1 */

	// Check to make sure the page allocator seems to work correctly.
	mem_check();
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
mem_alloc(void)
{
	// Fill this function in
#if SOL >= 1
	spinlock_acquire(&mem_freelock);

	pageinfo *pi = mem_freelist;
	if (pi != NULL) {
		mem_freelist = pi->free_next;	// Remove page from free list
		pi->free_next = NULL;		// Mark it not on the free list
	}

	spinlock_release(&mem_freelock);

	return pi;	// Return pageinfo pointer or NULL

#else
	// Fill this function in.
	panic("mem_alloc not implemented.");
#endif /* not SOL >= 1 */
}

//
// Return a page to the free list, given its pageinfo pointer.
// (This function should only be called when pp->pp_ref reaches 0.)
//
void
mem_free(pageinfo *pi)
{
#if SOL >= 1
	if (pi->refcount != 0)
		panic("mem_free: attempt to free in-use page");
	if (pi->free_next != NULL)
		panic("mem_free: attempt to free already free page!");

	spinlock_acquire(&mem_freelock);

	// Insert the page at the head of the free list.
	pi->free_next = mem_freelist;
	mem_freelist = pi;

	spinlock_release(&mem_freelock);
#else /* not SOL >= 1 */
	// Fill this function in.
	panic("mem_free not implemented.");
#endif /* not SOL >= 1 */
}

//
// Decrement the reference count on a page,
// freeing it if there are no more refs.
//
void
page_decref(pageinfo* pp)
{
	if (--pp->refcount == 0)
		mem_free(pp);
}


//
// Check the physical page allocator (mem_alloc(), mem_free())
// for correct operation after initialization via mem_init().
//
void
mem_check()
{
	pageinfo *pp, *pp0, *pp1, *pp2;
	pageinfo *fl;
	int i;

        // if there's a page that shouldn't be on
        // the free list, try to make sure it
        // eventually causes trouble.
	for (pp = mem_freelist; pp != 0; pp = pp->free_next)
		memset(mem_pi2ptr(pp), 0x97, 128);

	// should be able to allocate three pages
	pp0 = pp1 = pp2 = 0;
	pp0 = mem_alloc(); assert(pp0 != 0);
	pp1 = mem_alloc(); assert(pp1 != 0);
	pp2 = mem_alloc(); assert(pp2 != 0);

	assert(pp0);
	assert(pp1 && pp1 != pp0);
	assert(pp2 && pp2 != pp1 && pp2 != pp0);
        assert(mem_pi2phys(pp0) < mem_npage*PAGESIZE);
        assert(mem_pi2phys(pp1) < mem_npage*PAGESIZE);
        assert(mem_pi2phys(pp2) < mem_npage*PAGESIZE);

	// temporarily steal the rest of the free pages
	fl = mem_freelist;
	mem_freelist = 0;

	// should be no free memory
	assert(mem_alloc() == 0);

        // free and re-allocate?
        mem_free(pp0);
        mem_free(pp1);
        mem_free(pp2);
	pp0 = pp1 = pp2 = 0;
	pp0 = mem_alloc(); assert(pp0 != 0);
	pp1 = mem_alloc(); assert(pp1 != 0);
	pp2 = mem_alloc(); assert(pp2 != 0);
	assert(pp0);
	assert(pp1 && pp1 != pp0);
	assert(pp2 && pp2 != pp1 && pp2 != pp0);
	assert(mem_alloc() == 0);

	// give free list back
	mem_freelist = fl;

	// free the pages we took
	mem_free(pp0);
	mem_free(pp1);
	mem_free(pp2);

	cprintf("mem_check() succeeded!\n");
}

#endif /* LAB >= 1 */
