#if LAB >= 1
// Physical memory management definitions.
// See COPYRIGHT for copyright information.
#ifndef PIOS_KERN_MEM_H
#define PIOS_KERN_MEM_H
#ifndef PIOS_KERNEL
# error "This is a PIOS kernel header; user programs should not #include it"
#endif


// At PHYS_IOMEM (640K) there is a 384K hole for I/O.  From the kernel,
// PHYS_IOMEM can be addressed at VM_KERNLO + PHYS_IOMEM.  The hole ends
// at physical address PHYS_EXTMEM.
#define PMEM_IOMEM	0x0A0000
#define PMEM_EXTMEM	0x100000


// A pageinfo struct holds metadata on how a particular physical page is used.
// On boot we allocate a big array of pageinfo structs, one per physical page.
// This could be a union instead of a struct,
// since only one member is used for a given page state (free, allocated) -
// but that might make debugging a bit more challenging.
typedef struct pageinfo {
	pageinfo	*free_next;	// Next page number on free list
	uint32_t	refs;		// Reference count on allocated pages
} pageinfo;


// The pmem module sets up the following globals during pmem_init().
extern size_t pmem_max;		// Maximum physical address
extern size_t pmem_npage;	// Total number of physical memory pages
extern pageinfo *pmem_pageinfo;	// Metadata array indexed by page number


// Detect available physical memory and initialize the pmem_pageinfo array.
void pmem_init(void);

// Allocate a physical page and return a pointer to its pageinfo struct.
// Returns NULL if no more physical pages are available.
pageinfo *pmem_alloc(void);

// Return a physical page to the free list.
void pmem_free(pageinfo *pi);

// Check the physical page allocator (pmem_alloc(), pmem_free())
// for correct operation after initialization via pmem_init().
void pmem_check(void);

#endif /* !PIOS_KERN_MEM_H */
#endif // LAB >= 1
