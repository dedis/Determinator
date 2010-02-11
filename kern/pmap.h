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
// - The low 1.75GB contains fixed direct mappings of all physical memory,
//   representing the address space in which the kernel operates.
//   This way the kernel's address space effectively remains the same
//   both before and after it initializes the MMU and enables paging.
//
// - The next 2GB contains the running process's user-level address space.
//   Although user space starts at VM_LINUSER (1GB) in linear space,
//   we set up the user-mode segment registers to use this as their base,
//   so that user-mode code sees this as virtual address 0.
//
// - The top 256MB again contains direct mappings of physical memory,
//   giving the kernel access to the high I/O region, e.g., the local APIC.
//
// Kernel's linear address map: 	              Permissions
//                                                    kernel/user
//
//    4 Gig ---------> +==============================+
//                     |                              | RW/--
//                     |    High 32-bit I/O region    | RW/--
//                     |                              | RW/--
//    VM_LINHIGH ----> +==============================+ 0xf0000000
//                     |                              | RW/RW
//                     |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
//                     :              .               :
//                     :              .               :
//                     |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
//                     |                              | RW/RW
//                     |   User address space (2GB)   | RW/RW
//                     |        (see inc/vm.h)        | RW/RW
//                     |                              | RW/RW
//    VM_LINUSER ----> +==============================+ 0x70000000
//                     |                              | RW/--
//                     |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
//                     :              .               :
//                     :              .               :
//                     |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
//                     |                              | RW/--
//                     |    Physical memory (2GB)     | RW/--
//                     |    incl. I/O, kernel, ...    | RW/--
//                     |                              | RW/--
//    0 -------------> +==============================+
//
#define	PMAP_LINHIGH	0xf0000000
#define	PMAP_LINUSER	0x70000000


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
void pmap_invl(pde_t *pdir, uint32_t uva);


#endif /* !PIOS_KERN_PMAP_H */
#endif // LAB >= 3
