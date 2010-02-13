#if LAB >= 3
/* See COPYRIGHT for copyright information. */

#ifndef PIOS_KERN_PMAP_H
#define PIOS_KERN_PMAP_H
#ifndef PIOS_KERNEL
# error "This is a kernel header; user programs should not #include it"
#endif

#include <inc/assert.h>

#include <kern/mem.h>


//
// We divide our 4GB linear (post-segmentation) address space
// into three parts:
//
// - The low 1GB contains fixed direct mappings of physical memory,
//   representing the address space in which the kernel operates.
//   This way the kernel's address space effectively remains the same
//   both before and after it initializes the MMU and enables paging.
//   (It also means we can use at most 1GB of physical memory!)
//
// - The next 2.75GB contains the running process's user-level address space.
//   This is the only address range user-mode processes can access or map.
//
// - The top 256MB once again contains direct mappings of physical memory,
//   giving the kernel access to the high I/O region, e.g., the local APIC.
//
// Kernel's linear address map: 	              Permissions
//                                                    kernel/user
//
//    4 Gig ---------> +==============================+
//                     |                              | RW/--
//                     |    High 32-bit I/O region    | RW/--
//                     |                              | RW/--
//    PMAP_USERHI ---> +==============================+ 0xf0000000
//                     |                              | RW/RW
//                     |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
//                     :              .               :
//                     :              .               :
//                     |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
//                     |                              | RW/RW
//                     |  User address space (2.75GB) | RW/RW
//                     |        (see inc/vm.h)        | RW/RW
//                     |                              | RW/RW
//    PMAP_USERLO ---> +==============================+ 0x40000000
//                     |                              | RW/--
//                     |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
//                     :              .               :
//                     :              .               :
//                     |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
//                     |                              | RW/--
//                     |    Physical memory (1GB)     | RW/--
//                     |    incl. I/O, kernel, ...    | RW/--
//                     |                              | RW/--
//    0 -------------> +==============================+
//
#define	PMAP_USERHI	0xf0000000
#define	PMAP_USERLO	0x40000000


// Page directory entries and page table entries are 32-bit integers.
typedef uint32_t pde_t;
typedef uint32_t pte_t;


// Bootstrap page directory that identity-maps the kernel's address space.
extern pde_t pmap_bootpdir[1024];


void pmap_init(void);
pte_t *pmap_walk(pde_t *pdir, uint32_t uva, int create);
pte_t *pmap_insert(pde_t *pdir, pageinfo *pi, uint32_t uva, int perm);
pageinfo *pmap_lookup(pde_t *pdir, uint32_t uva, pte_t **pte_store);
void pmap_remove(pde_t *pdir, uint32_t uva);
void pmap_inval(pde_t *pdir, uint32_t uva, uint32_t size);


#endif /* !PIOS_KERN_PMAP_H */
#endif // LAB >= 3
