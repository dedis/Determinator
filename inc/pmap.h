#if LAB >= 1
#ifndef _PMAP_H_
#define _PMAP_H_

#ifndef __ASSEMBLER__
#include <inc/types.h>
#include <inc/queue.h>
#include <inc/mmu.h>
#endif /* not __ASSEMBLER__ */

/*
 * This file contains definitions for memory management in our OS,
 * which are relevant to both the kernel and user-mode software.
 */

// Global descriptor numbers
#define GD_KT     0x08     // kernel text
#define GD_KD     0x10     // kernel data
#define GD_UT     0x18     // user text
#define GD_UD     0x20     // user data
#define GD_TSS    0x28     // Task segment selector

/*
 * Virtual memory map:                                Permissions
 *                                                    kernel/user
 *
 *    4 Gig -------->  +------------------------------+
 *                     |                              | RW/--
 *                     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *                     :              .               :
 *                     :              .               :
 *                     :              .               :
 *                     |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~| RW/--
 *                     |                              | RW/--
 *                     |  Physical Memory             | RW/--
 *                     |                              | RW/--
 *    KERNBASE ----->  +------------------------------+ 0xf0000000
 *                     |  Kernel Virtual Page Table   | RW/--    PDMAP
 *    VPT,KSTACKTOP--> +------------------------------+ 0xefc00000      --+
 *                     |        Kernel Stack          | RW/--  KSTKSIZE   |
 *                     | - - - - - - - - - - - - - - -|                 PDMAP
 *                     |       Invalid memory         | --/--             |
 *    ULIM     ------> +------------------------------+ 0xef800000      --+
 *                     |      R/O User VPT            | R-/R-    PDMAP
 *    UVPT      ---->  +------------------------------+ 0xef400000
 *                     |        R/O PAGES             | R-/R-    PDMAP
 *    UPAGES    ---->  +------------------------------+ 0xef000000
 *                     |        R/O ENVS              | R-/R-    PDMAP
 * UTOP,UENVS -------> +------------------------------+ 0xeec00000
 * UXSTACKTOP -/       |      user exception stack    | RW/RW   BY2PG  
 *                     +------------------------------+ 0xeebff000
 *                     |       Invalid memory         | --/--   BY2PG
 *    USTACKTOP  ----> +------------------------------+ 0xeebfe000
 *                     |     normal user stack        | RW/RW   BY2PG
 *                     +------------------------------+ 0xeebfd000
 *                     |                              |
 *                     |                              |
 *                     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *                     .                              .
 *                     .                              .
 *                     .                              .
 *                     |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
 *                     |                              |
 *    UTEXT ------->   +------------------------------+ 0x00800000
 *                     |                              |  2 * PDMAP
 *    0 ------------>  +------------------------------+
 */


// All physical memory mapped at this address
#define	KERNBASE	0xf0000000	// start of kernel virtual space

/*
 * Virtual page table.  Entry PDX[VPT] in the PD contains a pointer to
 * the page directory itself, thereby turning the PD into a page table,
 * which maps all the PTEs containing the page mappings for the entire
 * virtual address space into that 4 Meg region starting at VPT.
 */
#define VPT (KERNBASE - PDMAP)
#define KSTACKTOP VPT
#define KSTKSIZE (8 * BY2PG)   		// size of a kernel stack
#define ULIM (KSTACKTOP - PDMAP) 

/*
 * User read-only mappings! Anything below here til UTOP are readonly to user.
 * They are global pages mapped in at env allocation time.
 */

// Same as VPT but read-only for users
#define UVPT (ULIM - PDMAP)
// Read-only copies of all ppage structures
#define UPAGES (UVPT - PDMAP)
// Read only copy of the global env structures
#define UENVS (UPAGES - PDMAP)

/*
 * Top of user VM. User can manipulate VA from UTOP-1 and down!
 */
#define UTOP UENVS
#define UXSTACKTOP (UTOP)           // one page user exception stack
// leave top page invalid to guard against exception stack overflow 
#define USTACKTOP (UTOP - 2*BY2PG)   // top of the normal user stack
#define UTEXT (2*PDMAP)

/*
 * Page fault modes inside kernel.
 */
#define PFM_NONE 0x0     // No page faults expected.  Must be a kernel bug
#define PFM_KILL 0x1     // On fault kill user process.


#ifndef __ASSEMBLER__

/*
 * The page directory entry corresponding to the virtual address range
 * from VPT to (VPT+PDMAP) points to the page directory itself
 * (treating it as a page table as well as a page directory).  One
 * result of treating the page directory as a page table is that all
 * PTE's can be accessed through a "virtual page table" at virtual
 * address VPT (to which vpt is set in locore.S).  A second
 * consequence is that the contents of the current page directory will
 * always be available at virtual address(VPT+(VPT>>PGSHIFT)) to
 * which vpd is set in locore.S.
 */
typedef u_long Pte;
typedef u_long Pde;

extern volatile Pte vpt[];     // VA of "virtual page table"
extern volatile Pde vpd[];     // VA of current page directory


/*
 * Page descriptor structures, mapped at UPAGES.
 * Read/write to the kernel, read-only to user programs.
 */
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

#endif /* not __ASSEMBLER__ */
#endif /* not _PMAP_H_ */
#endif // LAB >= 1
