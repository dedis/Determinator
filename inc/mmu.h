#if LAB >= 1
/*
 * x86 memory management unit (MMU) hardware definitions.
 *
 * Copyright (C) 1997 Massachusetts Institute of Technology
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Derived from the MIT Exokernel and JOS.
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

// page number field of address
#define PPN(la)		(((uintptr_t) (la)) >> PTXSHIFT)
#define VPN(la)		PPN(la)		// used to index into vpt[]

// page directory index
#define PDX(la)		((((uintptr_t) (la)) >> PDXSHIFT) & 0x3FF)
#define VPD(la)		PDX(la)		// used to index into vpd[]

// page table index
#define PTX(la)		((((uintptr_t) (la)) >> PTXSHIFT) & 0x3FF)

// linear address components
#define PGADDR(la)	((uintptr_t) (la) & ~0xFFF)	// address of page
#define PGOFF(la)	((uintptr_t) (la) & 0xFFF)	// offset in page

#define PTADDR(la)	((uintptr_t) (la) & ~0x3FFFFF)	// address of page table
#define PTOFF(la)	((uintptr_t) (la) & 0x3FFFFF)	// offset in page table

// Page directory and page table constants.
#define NPDENTRIES	1024		// PDEs per page directory
#define NPTENTRIES	1024		// PTEs per page table

#define PAGESIZE	4096		// bytes mapped by a page
#define PAGESHIFT	12		// log2(PAGESIZE)

#define PTSIZE		(PAGESIZE*NPTENTRIES)	// bytes mapped by a PDE
#define PTSHIFT		22		// log2(PTSIZE)

#define PTXSHIFT	12		// offset of PTX in a linear address
#define PDXSHIFT	22		// offset of PDX in a linear address

/*
RAJAT has commented these out for now. Now he is fixing the page table stuff and then he will enable this
// An x86-64 address is split into six fields:
//	- the sign extension area in bits 63-48.
//	- four 9-bit page table index fields in bits 47-12.
//	- the page offset field in bits 11-0.
// The following macros aid in manipulating these addresses.

// Page directory and page table constants.

#define PAGESIZE	4096		// bytes mapped by a page
#define PAGESHIFT	12			// log2(PAGESIZE)

#define NPENTRIES	512

// Bit-shifts for each field
#define P4SHIFT		39
#define P3SHIFT		30
#define P2SHIFT		21
#define P1SHIFT		12

// Cover sizes for each mapping level
#define P4SIZE		(1L << P4SHIFT)
#define P3SIZE		(1L << P3SHIFT)
#define P2SIZE		(1L << P2SHIFT)
#define P1SIZE		(1L << P1SHIFT)

// Page table index extractors
#define P4X(va)		((((uintptr_t)(va)) >> P4SHIFT) & 0x1ff)
#define P3X(va)		((((uintptr_t)(va)) >> P3SHIFT) & 0x1ff)
#define P2X(va)		((((uintptr_t)(va)) >> P2SHIFT) & 0x1ff)
#define P1X(va)		((((uintptr_t)(va)) >> P1SHIFT) & 0x1ff)

// Offset extractors
#define P4OFF(va)	(((uintptr_t)(va)) & (P4SIZE - 1))
#define P3OFF(va)	(((uintptr_t)(va)) & (P3SIZE - 1))
#define P2OFF(va)	(((uintptr_t)(va)) & (P2SIZE - 1))
#define P1OFF(va)	(((uintptr_t)(va)) & (P1SIZE - 1))	// offset in physical page

// Truncate (downward) and round (upward) macros
#define P1TRUNC(va)	((uintptr_t)(va) & ~(P1SIZE - 1))
#define P2TRUNC(va)	((uintptr_t)(va) & ~(P2SIZE - 1))
#define P3TRUNC(va)	((uintptr_t)(va) & ~(P3SIZE - 1))
#define P4TRUNC(va)	((uintptr_t)(va) & ~(P4SIZE - 1))

#define P1ROUND(va)	P1TRUNC((uintptr_t)(va) + (P1SIZE - 1))
#define P2ROUND(va)	P2TRUNC((uintptr_t)(va) + (P2SIZE - 1))
#define P3ROUND(va)	P3TRUNC((uintptr_t)(va) + (P3SIZE - 1))
#define P4ROUND(va)	P4TRUNC((uintptr_t)(va) + (P4SIZE - 1))
*/

// Page table entry flags (both 4KB and 2MB page sizes)
#define PTE_P 		1<<0	// present bit
#define PTE_W 		1<<1	// read/write bit
#define PTE_U 		1<<2	// user/supervisor bit
#define PTE_PWT 	1<<3	// page-level writethrough bit
#define PTE_PCD 	1<<4	// page-level cache disable bit
#define PTE_A 		1<<5	// accessed bit
// The PTE_AVAIL bits aren't used by the kernel or interpreted by the
// hardware, so user processes are allowed to set them arbitrarily.
#define PTE_AVAIL	0xE00	// Available for software use
#define PTE_NX 		1<<63	// no execute bit
#define P2E_PS 		1<<7	// page size bit (0 for 4KB, 1 for 2MB)
#define P1E_D 		1<<6
#define P1E_PAT 	1<<7
#define P1E_G 		1<<8	// global page bit

// Only flags in P1E_USER may be used in system calls.
#define P1E_USER	(P1E_AVAIL | P1E_P | P1E_RW | P1E_US)

// CR0 register layout
#define CR0_PE		(1<<0)		// Protection Enabled
#define CR0_MP		(1<<1)		// Monitor Coprocessor
#define CR0_EM		(1<<2)		// Emulation
#define CR0_TS		(1<<3)		// Task Switched
#define CR0_ET		(1<<4)		// Extension Type
#define CR0_NE		(1<<5)		// Numeric Error
#define CR0_WP		(1<<16)		// Write Protect
#define CR0_AM		(1<<18)		// Alignment Mask
#define CR0_NW		(1<<29)		// Not Writethrough
#define CR0_CD		(1<<30)		// Cache Disable
#define CR0_PG		(1<<31)		// Paging

// CR3 register layout
#define CR3_PWT		(1<<3)		// Page_Level Writethrough
#define CR3_PCD		(1<<4)		// Page-level Cache Disable

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
#define EFER_SCE	(1<<0)		// System Call Extensions
#define EFER_LME	(1<<8)		// Long Mode Enable
#define EFER_LMA	(1<<10)		// Long Mode Active
#define EFER_NXE	(1<<11)		// No-Execute Enable

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
#define SEG32(base,limit,type,size) \
	.word	(limit) & 0xffff, (base) & 0xffff; \
	.byte	((base) >> 16) & 0xff, type, \
		((size) << 4) | ((limit) >> 16), ((base) >> 24) & 0xff
#define SEG64(base,limit,type,size) \
	SEG32(base,limit,type,size); \
	.long	(base >> 32), 0

#else	// not __ASSEMBLER__

#include <inc/types.h>

// Segment Descriptors
typedef struct segdesc {
	unsigned sd_lim_15_0 : 16;  // Low bits of segment limit
	unsigned sd_base_15_0 : 16; // Low bits of segment base address
	unsigned sd_base_23_16 : 8; // Middle bits of segment base address
	unsigned sd_type : 4;       // Segment type (see STS_ constants)
	unsigned sd_s : 1;          // 0 = system, 1 = application
	unsigned sd_dpl : 2;        // Descriptor Privilege Level
	unsigned sd_p : 1;          // Present
	unsigned sd_lim_19_16 : 4;  // High bits of segment limit
	unsigned sd_avl : 1;        // Unused (available for software use)
	unsigned sd_rsv1 : 1;       // Reserved
	unsigned sd_db : 1;         // 0 = 16-bit segment, 1 = 32-bit segment
	unsigned sd_g : 1;          // Granularity: limit scaled by 4K when set
	unsigned sd_base_31_24 : 8; // High bits of segment base address
	unsigned sd_base_63_32 : 32;
	unsigned sd_rsv2 : 32;
} segdesc;

// Null segment
#define SEGDESC_NULL	(struct segdesc){ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
// Segment that is loadable but faults when used
#define SEGDESC_FAULT	(struct segdesc){ 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0 }
// Normal segment
#define SEGDESC64(app, type, base, lim, dpl) (struct segdesc)		\
{ ((lim) >> 12) & 0xffff, (base) & 0xffff, ((base) >> 16) & 0xff,	\
    (type), (app), (dpl), 1, (unsigned) (lim) >> 28, 0, 0, 1, 1,	\
    (unsigned) ((base) >> 24) & 0xff, (base) >> 32, 0 }

#define SEGDESC32(app, type, base, lim, dpl) (struct segdesc)		\
{ ((lim) >> 12) & 0xffff, (base) & 0xffff, ((base) >> 16) & 0xff,	\
    (type), (app), (dpl), 1, (unsigned) (lim) >> 28, 0, 0, 1, 1,	\
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
	uint64_t ts_rsp0;
	uint64_t ts_rsp1;
	uint64_t ts_rsp2;
	uint64_t ts_padding2;
	uint64_t ts_ist1;
	uint64_t ts_ist2;
	uint64_t ts_ist3;
	uint64_t ts_ist4;
	uint64_t ts_ist5;
	uint64_t ts_ist6;
	uint64_t ts_ist7;
	uint64_t ts_padding3;
	uint16_t ts_padding4;
	uint16_t ts_iomb;	// I/O map base address
} taskstate;

// Gate descriptors for interrupts and traps
typedef struct gatedesc {
	unsigned gd_off_15_0 : 16;   // low 16 bits of offset in segment
	unsigned gd_ss : 16;         // target selector
	unsigned gd_ist : 3;		 // must be 0 for call gate
	unsigned gd_resv1 : 5;        // # args, 0 for interrupt/trap gates
	unsigned gd_type : 4;        // type(STS_{CG64,IG64,TG64})
	unsigned gd_s : 1;           // must be 0 (system)
	unsigned gd_dpl : 2;         // descriptor(meaning new) privilege level
	unsigned gd_p : 1;           // Present
	unsigned gd_off_31_16 : 16;	 // high bits of offset in segment
	unsigned gd_off_63_32 : 32;
	unsigned gd_resv2 : 32;
} gatedesc;

// Set up a normal interrupt/trap gate descriptor.
// - istrap: 1 for a trap (= exception) gate, 0 for an interrupt gate.
// - sel: Code segment selector for interrupt/trap handler
// - off: Offset in code segment for interrupt/trap handler
// - dpl: Descriptor Privilege Level -
//	  the privilege level required for software to invoke
//	  this interrupt/trap gate explicitly using an int instruction.
#define SETGATE(gate, istrap, sel, off, dpl,ist)			\
{								\
	(gate).gd_off_15_0 = (uintptr_t) (off) & 0xffff;		\
	(gate).gd_ss = (sel);					\
	(gate).gd_ist = (ist);						\
	(gate).gd_resv1 = 0;					\
	(gate).gd_type = (istrap) ? STS_TG64 : STS_IG64;	\
	(gate).gd_s = 0;					\
	(gate).gd_dpl = (dpl);					\
	(gate).gd_p = 1;					\
	(gate).gd_off_31_16 = ((uintptr_t) (off) >> 16) & 0xffff;		\
	(gate).gd_off_63_32 = (uintptr_t) (off) >> 32;		\
	(gate).gd_resv2 = 0;					\
}

// Set up a call gate descriptor.
#define SETCALLGATE(gate, ss, off, dpl)           	        \
{								\
	(gate).gd_off_15_0 = (uintptr_t) (off) & 0xffff;		\
	(gate).gd_ss = (ss);					\
	(gate).gd_ist = 0;					\
	(gate).gd_resv1 = 0;					\
	(gate).gd_type = STS_CG64;				\
	(gate).gd_s = 0;					\
	(gate).gd_dpl = (dpl);					\
	(gate).gd_p = 1;					\
	(gate).gd_off_31_16 = ((uintptr_t) (off) >> 16) & 0xffff;		\
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
