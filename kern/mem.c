#if LAB >= 1
/*
 * Physical memory management.
 *
 * Copyright (C) 1997 Massachusetts Institute of Technology
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Derived from the MIT Exokernel and JOS.
 * Adapted for PIOS by Bryan Ford at Yale University.
 */

#include <inc/x86.h>
#include <inc/mmu.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/cpu.h>
#include <kern/mem.h>
#if LAB >= 2
#include <kern/spinlock.h>
#endif
#if LAB >= 3
#include <kern/pmap.h>
#endif
#if LAB >= 5
#include <kern/net.h>
#endif

#include <dev/nvram.h>


size_t mem_max;			// Maximum physical address
size_t mem_npage;		// Total number of physical memory pages

pageinfo *mem_pageinfo;		// Metadata array indexed by page number

pageinfo *mem_freelist;		// Start of free page list
#if SOL >= 2
spinlock mem_freelock;		// Spinlock protecting the free page list
#endif


void mem_check(void);

void
mem_init(void)
{
	if (!cpu_onboot())	// only do once, on the boot CPU
		return;

	// now we have memory map info from 0x1000
	// 0x1000: number of entries
	// 0x1004-(every 20 bytes):
	//     64bit base address
	//     64bit length
	//     32bit type
	uint32_t mem_range_cnt = *(uint32_t *)mem_ptr(0x1000);
	uint32_t k;
	mem_addr_range *mem_ranges = mem_ptr(0x1004);
	size_t basemem = 0;
	size_t extmem = MEM_EXT;
	for (k = 0; k < mem_range_cnt; k++) {
		if (basemem == mem_ranges[k].base) {
			basemem += mem_ranges[k].size;
		}
		if (extmem == mem_ranges[k].base) {
			extmem += mem_ranges[k].size;
		}
		cprintf("range %u %llx - %llx (%llx) type %u\n", k, mem_ranges[k].base, mem_ranges[k].base + mem_ranges[k].size, mem_ranges[k].size, mem_ranges[k].type);
	}
/*
	// Determine how much base (<640K) and extended (>1MB) memory
	// is available in the system (in bytes),
	// by reading the PC's BIOS-managed nonvolatile RAM (NVRAM).
	// The NVRAM tells us how many kilobytes there are.
	// Since the count is 16 bits, this gives us up to 64MB of RAM;
	// additional RAM beyond that would have to be detected another way.
	size_t basemem = ROUNDDOWN(nvram_read16(NVRAM_BASELO)*1024, PAGESIZE);
	size_t extmem = ROUNDDOWN(nvram_read16(NVRAM_EXTLO)*1024, PAGESIZE);

	warn("Assuming we have 1GB of memory!");
	extmem = 1024*1024*1024 - MEM_EXT;	// assume 1GB total memory
*/

	// The maximum physical address is the top of extended memory.
	mem_max = MEM_EXT + extmem;

	// Compute the total number of physical pages (including I/O holes)
	mem_npage = mem_max / PAGESIZE;

	cprintf("Physical memory: %dK available, ", (int)(mem_max/1024));
	cprintf("base = %dK, extended = %dK\n",
		(int)(basemem/1024), (int)(extmem/1024));
	cprintf("%d pages in total\n", mem_npage);

#if SOL >= 1
	// Now that we know the size of physical memory,
	// reserve enough space for the pageinfo array
	// just past our statically-assigned program code/data/bss,
	// which the linker placed at the start of extended memory.
	// Make sure the pageinfo entries are naturally aligned.
	mem_pageinfo = mem_ptr(ROUNDUP((size_t) end, sizeof(pageinfo)));

	// Initialize the entire pageinfo array to zero for good measure.
	memset(mem_pageinfo, 0, sizeof(pageinfo) * mem_npage);

	// Free extended memory starts just past the pageinfo array.
	intptr_t freemem = mem_phys(&mem_pageinfo[mem_npage]);

	// Align freemem to page boundary.
	freemem = ROUNDUP(freemem, PAGESIZE);

	// Chain all the available physical pages onto the free page list.
#if SOL >= 2
	spinlock_init(&mem_freelock);
#endif
	pageinfo **freetail = &mem_freelist;
	int i;
	for (i = 0; i < mem_npage; i++) {
		// Off-limits until proven otherwise.
		int inuse = 1;

		// The bottom basemem bytes are free except page 0 and 1.
		if (i > 1 && i < basemem / PAGESIZE)
			inuse = 0;

		// The IO hole and the kernel abut.

		// The memory past the kernel is free.
		if (i >= (intptr_t)freemem / PAGESIZE)
			inuse = 0;

		mem_pageinfo[i].refcount = inuse;
		if (!inuse) {
			// Add the page to the end of the free list.
			*freetail = &mem_pageinfo[i];
			freetail = &mem_pageinfo[i].free_next;
		}
	}
	*freetail = NULL;	// null-terminate the freelist

#else /* not SOL >= 1 */
	// Insert code here to:
	// (1)	allocate physical memory for the mem_pageinfo array,
	//	making it big enough to hold mem_npage entries.
	// (2)	add all pageinfo structs in the array representing
	//	available memory that is not in use for other purposes.
	//
	// For step (2), here is some incomplete/incorrect example code
	// that simply marks all mem_npage pages as free.
	// Which memory is actually free?
	//  1) Reserve page 0 for the real-mode IDT and BIOS structures
	//     (do not allow this page to be used for anything else).
	//  2) Reserve page 1 for the AP bootstrap code (boot/bootother.S).
	//  3) Mark the rest of base memory as free.
	//  4) Then comes the IO hole [MEM_IO, MEM_EXT).
	//     Mark it as in-use so that it can never be allocated.      
	//  5) Then extended memory [MEM_EXT, ...).
	//     Some of it is in use, some is free.
	//     Which pages hold the kernel and the pageinfo array?
	//     Hint: the linker places the kernel (see start and end above),
	//     but YOU decide where to place the pageinfo array.
	// Change the code to reflect this.
	pageinfo **freetail = &mem_freelist;
	int i;
	for (i = 0; i < mem_npage; i++) {
		// A free page has no references to it.
		mem_pageinfo[i].refcount = 0;

		// Add the page to the end of the free list.
		*freetail = &mem_pageinfo[i];
		freetail = &mem_pageinfo[i].free_next;
	}
	*freetail = NULL;	// null-terminate the freelist

	// ...and remove this when you're ready.
	panic("mem_init() not implemented");
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
#if LAB >= 2
// Hint: be sure to use proper mutual exclusion for multiprocessor operation.
#endif
pageinfo *
mem_alloc(void)
{
	// Fill this function in
#if SOL >= 1
#if SOL >= 2
	spinlock_acquire(&mem_freelock);
#endif

	pageinfo *pi = mem_freelist;
	if (pi != NULL) {
		mem_freelist = pi->free_next;	// Remove page from free list
		pi->free_next = NULL;		// Mark it not on the free list
#if SOL >= 5
		pi->home = 0;			// Assume it originated here
		pi->shared = 0;			// Unshared initially
#endif
	}

#if SOL >= 2
	spinlock_release(&mem_freelock);
#endif

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

#if SOL >= 2
	spinlock_acquire(&mem_freelock);
#endif

	// Insert the page at the head of the free list.
	pi->free_next = mem_freelist;
	mem_freelist = pi;

#if SOL >= 2
	spinlock_release(&mem_freelock);
#endif
#else /* not SOL >= 1 */
	// Fill this function in.
	panic("mem_free not implemented.");
#endif /* not SOL >= 1 */
}

#if LAB >= 5
// When we receive a copy of a page or kernel object from a remote node,
// we call this function to keep track of the page's origin,
// so that we can later find it again given the same remote reference.
void mem_rrtrack(uint32_t rr, pageinfo *pi)
{
	assert(pi > &mem_pageinfo[1] && pi < &mem_pageinfo[mem_npage]);
	assert(pi != mem_ptr2pi(pmap_zero));	// Don't track zero page!
	assert(pi < mem_ptr2pi(start) || pi > mem_ptr2pi(end-1));

	// Change these to use whatever your memory spinlock is called.
	spinlock_acquire(&mem_freelock);

	uint8_t node = RRNODE(rr);
	assert(node > 0 && node <= NET_MAXNODES);

	// Look up our pageinfo struct containing our homelist
	// for the appropriate remote physical address.
	// This design assumes that our pageinfo array will be big enough,
	// which implies that all nodes must have the same amount of RAM.
	// This could easily be fixed by allocating a separate hash table
	// mapping remote references to local pages.
	uint32_t addr = RRADDR(rr);
	pageinfo *hpi = mem_phys2pi(addr);
	assert(hpi > &mem_pageinfo[1] && hpi < &mem_pageinfo[mem_npage]);

	// Quick scan just to make sure it's not already there - shouldn't be!
	pageinfo *spi;
	for (spi = hpi->homelist; spi != NULL; spi = spi->homenext) {
		assert(RRADDR(spi->home) == addr);
		assert(spi->home != rr);
	}

	// Insert the new page at the head of the appropriate homelist
	pi->home = rr;
	pi->homenext = hpi->homelist;
	hpi->homelist = pi;

	spinlock_release(&mem_freelock);
}

// Given a remote reference to a page on some other node,
// see if we already have a corresponding local page
// and return a pointer the beginning of that page if so.
// Otherwise, returns NULL.
pageinfo *
mem_rrlookup(uint32_t rr)
{
	// Change these to use whatever your memory spinlock is called.
	spinlock_acquire(&mem_freelock);

	uint8_t node = RRNODE(rr);
	assert(node > 0 && node <= NET_MAXNODES);
	uint32_t addr = RRADDR(rr);
	pageinfo *pi = mem_phys2pi(addr);
	assert(pi > &mem_pageinfo[1] && pi < &mem_pageinfo[mem_npage]);

	// Search for a page corresponding to this rr in the homelist
	for (pi = pi->homelist; pi != NULL; pi = pi->homenext) {
		assert(RRADDR(pi->home) == addr);
		if (pi->home == rr) {		// found it!
			// Take a reference while we still have
			// the pageinfo array locked, so it can't go away.
			mem_incref(pi);
			break;
		}
	}

	spinlock_release(&mem_freelock);
	return pi;
}

#endif	// LAB >= 5
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
	int freepages = 0;
	for (pp = mem_freelist; pp != 0; pp = pp->free_next) {
	//	memset(mem_pi2ptr(pp), 0x97, 128);
		freepages++;
	}
	cprintf("mem_check: %d free pages\n", freepages);
	assert(freepages < mem_npage);	// can't have more free than total!
	assert(freepages > 16000);	// make sure it's in the right ballpark

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

#if LAB >= 9
#else
	cprintf("mem_check() succeeded!\n");
#endif
}

#endif /* LAB >= 1 */
