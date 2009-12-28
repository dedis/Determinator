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
#define PHYS_IOMEM	0x0A0000
#define PHYS_EXTMEM	0x100000


// A pageinfo struct holds metadata on how a particular physical page is used.
// On boot we allocate a big array of pageinfo structs, one per physical page.
// This could be a union instead of a struct,
// since only one member is used for a given page state (free, allocated) -
// but that might make debugging a bit more challenging.
typedef struct pageinfo {
	uint32_t	free_next;	// Next page number on free list
	uint32_t	refcount;	// Reference count on allocated pages
} pageinfo;


#endif /* !PIOS_KERN_MEM_H */
#endif // LAB >= 1
