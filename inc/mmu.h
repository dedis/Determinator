#if LAB >= 1
/*
 * x86 memory management unit (MMU) hardware definitions.
 *
 * Copyright (C) 1997 Massachusetts Institute of Technology
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Derived from the MIT Exokernel and JOS.
 * Adapted for PIOS by Bryan Ford at Yale University.
 * Adapted for 64-bit PIOS by Yu Zhang and Rajat Goyal
 */

#ifndef PIOS_INC_MMU_H
#define PIOS_INC_MMU_H

#include <cdefs.h>

/*
 * This file contains definitions for the x86 memory management unit (MMU),
 * including paging- and segmentation-related data structures and constants,
 * the %cr0, %cr4, and %rflags registers, and traps.
 */

/*
 *
 *	Part 1.  Paging data structures and constants.
 *
 */

// An x86-64 address is split into six fields:
//      - the sign extension area in bits 63-48.
//      - four 9-bit page table index fields in bits 47-12.
//      - the page offset field in bits 11-0.
// The following macros aid in manipulating these addresses.

// Page directory and page table constants.
// entries per page table/page directory/page directory pointer/page map level-4
#define NPTENTRIES	512 
#define NPTBITS		9		// log2(NPTENTRIES)
#define NPTLVLS     	3           	// page table depth -1
// the entry in PML4 pointing to itself, the MSB is 1 (lower half) to avoid walking into user space, need to care about sign extension of canonical address form
#define PML4SELFOFFSET	0x1BF
#define CANONICALSIGNEXTENSION	0xFFFF000000000000ULL
// the unsigned linear address
#define ULA(la)		((la) & 0x0000FFFFFFFFFFFFULL)

// page size
#define PAGESIZE	4096		// bytes mapped by a page
#define PAGESHIFT	12		// log2(PAGESIZE)

//
// In 64-bit PIOS, the type of linear address is intptr_t.
// Currently the valid range of linear addresses is [0, 2^47-1].
// The layout of linear address space is shown in inc/vm.h.
// 
// A linear address 'la' has a six-part structure as follows:
// Index
// +---16---+-----9-----+-----9-----+-----9-----+-----9-----+----12----+
// |  Sign  |    PML4   |    PDP    | Page Dir  | Page Table|  Offset  |
// |   Ext. |   Index   |   Index   |   Index   |   Index   |  in Page |
// |        | PDX(3,la)/ \PDX(2,la)/ \PDX(1,la)/ \PDX(0,la)/ \PGOFF(la)/
// +--------+-----------+-----------+-----------+-----------+----------+
//           \----------------- PPN(la) -------------------/ 
//
// The PDX, PGOFF, and PPN macros decompose linear addresses as shown.
//
// +--------+-----------+-----------+-----------+-----------+----------+
//          \---------------- PDADDR(0,la) -----------------|PDOFF(0,la)
//          \------------- PDADDR(1,la) --------|----- PDOFF(1,la) ----/
//          \----- PDADDR(2,la) ----|---------- PDOFF(2,la) -----------/
//          PDADDR(3,la)|---------------- PDOFF(3,la) -----------------/
//
// The PDADDR and PDOFF macros are used to construct a page/PT/PD/PDP region
// -aligned linear address and an offset within a page/PT/PD/PDP region
// from la. 

// index info:         
//   n = 0 => page table 
//   n = 1 => page directory
//   n = 2 => page directory pointer
//   n = 3 => page map level 4
#define PDXMASK		((1 << NPTBITS) - 1)
// In 32-bit: PTXSHIFT, PDXSHIFT
#define PDSHIFT(n)	(12 + NPTBITS * (n))
// In 32-bit: PTX(la), PDX(la)
#define PDX(n, la)	((((uintptr_t) (la)) >> PDSHIFT(n)) & PDXMASK)
#define PGOFF(la)	((uintptr_t) (la) & 0xFFF)	// offset in page
// page number field of address
#define PPN(la)		(((uintptr_t) (la)) >> PAGESHIFT)
// bytes info:
//   n = 0 => bytes mapped by a PTE, == PAGESIZE
//   n = 1 => bytes mapped by a PDE, 32-bit: PTSIZE 
//   n = 2 => bytes mapped by a PDPE
//   n = 3 => bytes mapped by a PML4E 
#define PDSIZE(n)	(0x1ULL<<PDSHIFT(n))
#define PTSIZE		(1<<PDSHIFT(1))

// linear address components
#define PGADDR(la)	((uintptr_t) (la) & ~0xFFFULL)	// address of page
//   n = 0 => address/offset of page, 32-bit: PGADDR(la), PGOFF(la)
//   n = 1 => address/offset of page table, 32-bit: PTADDR(la), PTOFF(la) 
//   n = 2 => address/offset of page-directory table 
//   n = 3 => address/offset of page-directory-pointer table 
#define PDADDR(n, la)	((uintptr_t) (la) & ~((0x1ULL << PDSHIFT(n)) - 1))
#define PDOFF(n, la)	((uintptr_t) (la) & ((0x1ULL << PDSHIFT(n)) -1))
#define PTOFF(la)	PDOFF(1, (la))

// TODO: the following may be deleted after finishing 4 level page mapping
// page directory index
//#define VPD(la)		PDX(la)		// used to index into vpd[]
//#define VPN(la)		PPN(la)		// used to index into vpt[]
// end of TODO

// Page map tables (PML4/PDP/PD/PT) and entries (PML4E/PDPE/PDE/PTE)
// base address and flags in a PTE/PDE/PDPE/PML4E pte
#define PTE_ADDR(pmte) 	((uintptr_t) (pmte) & 0x000FFFFFFFFFF000)
#define PTE_FLAGS(pmte) ((uintptr_t) (pmte) & 0xFFF0000000000FFF)

// PML4/PDP/PD/PT entry flags.
#define PTE_P           0x001   // Present
#define PTE_W           0x002   // Writeable
#define PTE_U           0x004   // User-accessible
#define PTE_PWT         0x008   // Write-Through
#define PTE_PCD         0x010   // Cache-Disable
#define PTE_A           0x020   // Accessed
#define PTE_D           0x040   // Dirty
#define PTE_PS          0x080   // Page Size, in PD/PDP/PML4
#define PTE_PAT         0x080   // Page Attribute Table (only in PTEs)
#define PTE_G           0x100   // Global
// The PTE_AVAIL bits aren't used by the kernel or interpreted by the
// hardware, so user processes are allowed to set them arbitrarily.
// Mask for RWX permissions. Individual settings macros SYS_{PERM,READ,WRITE,RW,
// EXECUTE,RWX} in inc/syscall.h
#define PTE_AVAIL	0xE00	// Available for software use
#define PTE_NX 		(0x1ULL<<63)	// No execute

// Only flags in PTE_USER may be used in system calls.
#define PTE_USER	(PTE_AVAIL | PTE_P | PTE_W | PTE_U)

// Control Register flags
#define CR0_PE          0x00000001      // Protection Enable
#define CR0_MP          0x00000002      // Monitor coProcessor
#define CR0_EM          0x00000004      // Emulation
#define CR0_TS          0x00000008      // Task Switched
#define CR0_ET          0x00000010      // Extension Type
#define CR0_NE          0x00000020      // Numeric Errror
#define CR0_WP          0x00010000      // Write Protect
#define CR0_AM          0x00040000      // Alignment Mask
#define CR0_NW          0x20000000      // Not Writethrough
#define CR0_CD          0x40000000      // Cache Disable
#define CR0_PG          0x80000000      // Paging

#define CR3_PWT		0x8		// Page_Level Writethrough
#define CR3_PCD		0x10		// Page-level Cache Disable

#define CR4_VME		0x00000001	// V86 Mode Extensions
#define CR4_PVI		0x00000002	// Protected-Mode Virtual Interrupts
#define CR4_TSD		0x00000004	// Time Stamp Disable
#define CR4_DE		0x00000008	// Debugging Extensions
#define CR4_PSE		0x00000010	// Page Size Extensions
#define CR4_PAE		0x00000020	// Physical Address Extension
#define CR4_MCE		0x00000040	// Machine Check Enable
#define CR4_PGE		0x00000080	// Page Global Enable
#define CR4_PCE		0x00000100	// Performance counter enable
#define CR4_OSFXSR	0x00000200	// SSE and FXSAVE/FXRSTOR enable
#define CR4_OSXMMEXCPT	0x00000400	// Unmasked SSE FP exceptions

// Model-Specific Register (MSR) addresses
#define MSR_TSC		0x00000010	// Time-Stamp Counter
#define MSR_EFER	0xc0000080	// Extended Feature Enable Register
#define MSR_FSBASE	0xc0000100	// 64-bit Mode FS Base
#define MSR_GSBASE	0xc0000101	// 64-bit Mode FS Base
#define MSR_KGSBASE	0xc0000102	// Kernel GS Base for SWAPGS

// EFER register layout
#define EFER_SCE	0x1		// System-Call Extensions
#define EFER_LME	0x100		// Long Mode Enable
#define EFER_LMA	0x400		// Long Mode Active
#define EFER_NXE	0x800		// No-Execute Enable

// settings that have to be made to enter 64-bit mode
#define KERN_CR0	(CR0_PE|CR0_MP|CR0_ET|CR0_WP|CR0_PG)
#define KERN_CR4	(CR4_PSE|CR4_PAE|CR4_PGE)
#define KERN_EFER	(EFER_LME|EFER_SCE|EFER_NXE)

// Page fault error codes
#define PFE_PR		0x1	// Page fault caused by protection violation
#define PFE_WR		0x2	// Page fault caused by a write
#define PFE_U		0x4	// Page fault occured while in user mode



/*
 *
 *	Part 2.  Segmentation data structures and constants.
 *
 */

#ifdef __ASSEMBLER__

/*
 * Macros to build GDT entries in assembly.
 */

#define SEGNULL32	\
	.word 0,0,0,0
#define SEGNULL64	\
	.long 0,0,0,0
#define SEG32(app, type, base, limit, dpl, mode)                     \
	.word (((limit) >> 12) & 0xffff),                            \
	      ((base) & 0xffff);                                     \
	.byte (((base) >> 16) & 0xff),                               \
	      (0x80 | ((app) << 4) | (type) | ((dpl) << 5)),         \
	      (0x80 | ((~(mode)) & 0x60) | (((limit) >> 28) & 0xf)), \
	      (((base) >> 24) & 0xff)
#define SEG64(app, type, base, limit, dpl, mode)  \
	SEG32(app, type, base, limit, dpl, mode); \
	.long (base >> 32), 0

#else	// not __ASSEMBLER__

#include <inc/types.h>

// Segment Descriptors
typedef struct segdesc {
	unsigned sd_lim_15_0 : 16;   // Low bits of segment limit
	unsigned sd_base_15_0 : 16;  // Low bits of segment base address
	unsigned sd_base_23_16 : 8;  // Middle bits of segment base address
	unsigned sd_type : 4;        // Segment type (see STS_ constants)
	unsigned sd_s : 1;           // 0 = system, 1 = application
	unsigned sd_dpl : 2;         // Descriptor Privilege Level
	unsigned sd_p : 1;           // Present
	unsigned sd_lim_19_16 : 4;   // High bits of segment limit
	unsigned sd_avl : 1;         // Unused (available for software use)
	unsigned sd_l : 1;	     // Long mode bit (1 = 64 bit mode, 0 = legacy mdoe)
	unsigned sd_db : 1;          // 0 = 16-bit segment, 1 = 32-bit segment
	unsigned sd_g : 1;           // Granularity: limit scaled by 4K when set
	unsigned sd_base_31_24 : 8;  // High bits of segment base address
	unsigned sd_base_63_32 : 32; // Highest bits of segment base address
	unsigned sd_rsv : 32;
} segdesc;

// Null segment
#define SEGDESC_NULL	(struct segdesc){ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
// Segment that is loadable but faults when used
#define SEGDESC_FAULT	(struct segdesc){ 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0 }
// Normal segment
#define SEGDESC64(app, type, base, lim, dpl, mode) (struct segdesc)		\
{ ((lim) >> 12) & 0xffff, (base) & 0xffff, ((base) >> 16) & 0xff,	\
    (type), (app), (dpl), 1, (unsigned)(lim) >> 28, 0, (mode), ~(mode), 1,	\
    ((base) >> 24) & 0xff, (unsigned long long)(base) >> 32, 0 }
#define SEGDESC64_nogran(app, type, base, lim, dpl, mode) (struct segdesc)             \
{ ((lim)) & 0xffff, (base) & 0xffff, ((base) >> 16) & 0xff,       \
    (type), (app), (dpl), 1, ((lim) >> 16) & 0xf, 0, (mode), ~(mode), 0,     \
    ((base) >> 24) & 0xff, ((unsigned long long)(base)) >> 32, 0 }

#define SEGDESC32(app, type, base, lim, dpl, mode) (struct segdesc)		\
{ ((lim) >> 12) & 0xffff, (base) & 0xffff, ((base) >> 16) & 0xff,	\
    (type), (app), (dpl), 1, (unsigned) (lim) >> 28, 0, (mode), ~(mode), 1,	\
    (unsigned) (base) >> 24, 0, 0 }
#define SEGDESC16(app, type, base, lim, dpl) (struct segdesc)		\
{ (lim) & 0xffff, (base) & 0xffff, ((base) >> 16) & 0xff,		\
    (type), (app), (dpl), 1, (unsigned) (lim) >> 16, 0, 0, 1, 0,	\
    (unsigned) (base) >> 24, 0, 0 }

#endif /* !__ASSEMBLER__ */

// Application segment type bits ('app' bit = 1)
#define STA_X		0x8	    // Executable segment
#define STA_E		0x4	    // Expand down (non-executable segments)
#define STA_C		0x4	    // Conforming code segment (executable only)
#define STA_W		0x2	    // Writeable (non-executable segments)
#define STA_R		0x2	    // Readable (executable segments)
#define STA_A		0x1	    // Accessed

// System segment type bits ('app' bit = 0)
#define STS_LDT		0x2	    // 64 bit LDT
#define STS_T64A	0x9	    // Available 64-bit TSS
#define STS_T64B	0xB	    // Busy 64-bit TSS
#define STS_CG64	0xC	    // 64-bit Call Gate
#define STS_IG64	0xE	    // 64-bit Interrupt Gate
#define STS_TG64	0xF	    // 64-bit Trap Gate


/*
 *
 *	Part 3.  Traps.
 *
 */

#ifndef __ASSEMBLER__

// Task state segment format, as defined by the x86-64 architecture.
typedef struct taskstate {
	uint32_t ts_padding1;
	uint64_t gcc_packed ts_rsp[3];	// Stack pointer for CPL 0, 1, 2
	uint64_t gcc_packed ts_ist[8];	// Note: tss_ist[0] is ignored
	uint16_t ts_padding3[5];
	uint16_t ts_iomb;	// I/O map base address
} taskstate;

// Gate descriptors for interrupts and traps
typedef struct gatedesc {
	unsigned gd_off_15_0 : 16;   // low 16 bits of offset in segment
	unsigned gd_ss : 16;         // target selector
	unsigned gd_ist : 3;	     // must be 0 for call gate
	unsigned gd_resv1 : 5;       // # args, 0 for interrupt/trap gates
	unsigned gd_type : 4;        // type(STS_{CG64,IG64,TG64})
	unsigned gd_s : 1;           // must be 0 (system)
	unsigned gd_dpl : 2;         // descriptor(meaning new) privilege level
	unsigned gd_p : 1;           // Present
	unsigned gd_off_31_16 : 16;  // high bits of offset in segment
	unsigned gd_off_63_32 : 32;  // highest bits of offset in segment
	unsigned gd_resv2 : 32;	     // reserved
} gatedesc;

// Set up a normal interrupt/trap gate descriptor.
// - istrap: 1 for a trap (= exception) gate, 0 for an interrupt gate.
// - sel: Code segment selector for interrupt/trap handler
// - off: Offset in code segment for interrupt/trap handler
// - dpl: Descriptor Privilege Level -
//	  the privilege level required for software to invoke
//	  this interrupt/trap gate explicitly using an int instruction.
#define SETGATE(gate, istrap, sel, off, dpl,ist)		\
{								\
	(gate).gd_off_15_0 = (uintptr_t) (off) & 0xffff;	\
	(gate).gd_ss = (sel);					\
	(gate).gd_ist = (ist);					\
	(gate).gd_resv1 = 0;					\
	(gate).gd_type = (istrap) ? STS_TG64 : STS_IG64;	\
	(gate).gd_s = 0;					\
	(gate).gd_dpl = (dpl);					\
	(gate).gd_p = 1;					\
	(gate).gd_off_31_16 = ((uintptr_t) (off) >> 16) & 0xffff;	\
	(gate).gd_off_63_32 = (uintptr_t) (off) >> 32;		\
	(gate).gd_resv2 = 0;					\
}

// Set up a call gate descriptor.
#define SETCALLGATE(gate, ss, off, dpl)           	        \
{								\
	(gate).gd_off_15_0 = (uintptr_t) (off) & 0xffff;	\
	(gate).gd_ss = (ss);					\
	(gate).gd_ist = 0;					\
	(gate).gd_resv1 = 0;					\
	(gate).gd_type = STS_CG64;				\
	(gate).gd_s = 0;					\
	(gate).gd_dpl = (dpl);					\
	(gate).gd_p = 1;					\
	(gate).gd_off_31_16 = ((uintptr_t) (off) >> 16) & 0xffff;	\
	(gate).gd_off_63_32 = (uintptr_t) (off) >> 32;		\
	(gate).gd_resv2 = 0;					\
}

// Pseudo-descriptors used for LGDT, LLDT and LIDT instructions.
struct pseudodesc {
	uint16_t		pd_lim;		// Limit
	uintptr_t gcc_packed	pd_base;	// Base - NOT 4-byte aligned!
};
typedef struct pseudodesc pseudodesc;

#endif /* !__ASSEMBLER__ */

#endif /* !PIOS_INC_MMU_H */
#endif // LAB >= 1
