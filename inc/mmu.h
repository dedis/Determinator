#ifndef _MMU_H_
#define _MMU_H_

/*
 * This file contains:
 *
 *	Part 1.  x86 definitions.
 *	Part 2.  Our conventions.
 *	Part 3.  Our helper functions.
 */

/*
 * Part 1.  x86 definitions.
 */

/*
 * An Address:
 *  +--------10------+-------10-------+---------12----------+
 *  | Page Directory |   Page Table   | Offset within Page  |
 *  |      Index     |      Index     |                     |
 *  +----------------+----------------+---------------------+
 */

/* page directory index */
#define PDX(va)		((((u_long)(va))>>22) & 0x03FF)

/* page table index */
#define PTX(va)		((((u_long)(va))>>12) & 0x03FF)

/* offset in page */
#define PGOFF(va)	(((u_long)(va)) & 0xFFF)

/* bytes in a page */
#define PGSIZE		4096

/* bytes mapped by a page directory entry */
#define PDSIZE		(4*1024*1024)

/* At IOPHYSMEM (640K) there is a 384K hole for I/O.  From the kernel,
 * IOPHYSMEM can be addressed at KERNBASE + IOPHYSMEM.  The hole ends
 * at physical address EXTPHYSMEM. */
#define IOPHYSMEM 0xa0000
#define EXTPHYSMEM 0x100000

/* Page Table/Directory Entry flags
 *   these are defined by the hardware
 */
#define PG_P 0x1               /* Present */
#define PG_W 0x2               /* Writeable */
#define PG_U 0x4               /* User */
#define PG_PWT 0x8             /* Write-Through */
#define PG_PCD 0x10            /* Cache-Disable */
#define PG_A 0x20              /* Accessed */
#define PG_D 0x40              /* Dirty */
#define PG_PS 0x80             /* Page Size */
#define PG_MBZ 0x180           /* Bits must be zero */
#define PG_USER 0xe00          /* Bits for user processes */
/*
 * The PG_USER bits are not used by the kernel and they are
 * not interpreted by the hardware.  The kernel allows 
 * user processes to set them arbitrarily.
 */

/* Control Register flags */
#define CR0_PE 0x1             /* Protection Enable */
#define CR0_MP 0x2             /* Monitor coProcessor */
#define CR0_EM 0x4             /* Emulation */
#define CR0_TS 0x8             /* Task Switched */
#define CR0_ET 0x10            /* Extension Type */
#define CR0_NE 0x20            /* Numeric Errror */
#define CR0_WP 0x10000         /* Write Protect */
#define CR0_AM 0x40000         /* Alignment Mask */
#define CR0_NW 0x20000000      /* Not Writethrough */
#define CR0_CD 0x40000000      /* Cache Disable */
#define CR0_PG 0x80000000      /* Paging */

#define CR4_PCE 0x100          /* Performance counter enable */
#define CR4_MCE 0x40           /* Machine Check Enable */
#define CR4_PSE 0x10           /* Page Size Extensions */
#define CR4_DE  0x08           /* Debugging Extensions */
#define CR4_TSD 0x04           /* Time Stamp Disable */
#define CR4_PVI 0x02           /* Protected-Mode Virtual Interrupts */
#define CR4_VME 0x01           /* V86 Mode Extensions */

/* Eflags register */
#define FL_CF 0x1              /* Carry Flag */
#define FL_PF 0x4              /* Parity Flag */
#define FL_AF 0x10             /* Auxiliary carry Flag */
#define FL_ZF 0x40             /* Zero Flag */
#define FL_SF 0x80             /* Sign Flag */
#define FL_TF 0x100            /* Trap Flag */
#define FL_IF 0x200            /* Interrupt Flag */
#define FL_DF 0x400            /* Direction Flag */
#define FL_OF 0x800            /* Overflow Flag */
#define FL_IOPL0 0x1000        /* Low bit of I/O Privilege Level */
#define FL_IOPL1 0x2000        /* High bit of I/O Privilege Level */
#define FL_NT 0x4000           /* Nested Task */
#define FL_RF 0x10000          /* Resume Flag */
#define FL_VM 0x20000          /* Virtual 8086 mode */
#define FL_AC 0x40000          /* Alignment Check */
#define FL_VIF 0x80000         /* Virtual Interrupt Flag */
#define FL_VIP 0x100000        /* Virtual Interrupt Pending */
#define FL_ID 0x200000         /* ID flag */

/* Page fault error codes */
#define FEC_PR 0x1             /* Page fault caused by protection violation */
#define FEC_WR 0x2             /* Page fault caused by a write */
#define FEC_U  0x4             /* Page fault occured while in user mode */


#ifdef __ASSEMBLER__

/*
 * Macros to build GDT entries in assembly.
 */
#define SEG_NULL           \
	.word 0, 0;        \
	.byte 0, 0, 0, 0
#define SEG(type,base,lim)                                    \
	.word((lim)&0xffff), ((base)&0xffff);                \
	.byte(((base)>>16)&0xff), (0x90|(type)),             \
		(0xc0|(((lim)>>16)&0xf)), (((base)>>24)&0xff)

#else

/* Segment Descriptors */
struct Segdesc {
	unsigned sd_lim_15_0 : 16;   /* low bits of segment limit */
	unsigned sd_base_15_0 : 16;  /* low bits of segment base address */
	unsigned sd_base_23_16 : 8;  /* middle bits of segment base */
	unsigned sd_type : 4;        /* segment type(see below) */
	unsigned sd_s : 1;           /* 0 = system, 1 = application */
	unsigned sd_dpl : 2;         /* Descriptor Privilege Level */
	unsigned sd_p : 1;           /* Present */
	unsigned sd_lim_19_16 : 4;   /* High bits of segment limit */
	unsigned sd_avl : 1;         /* Unused(available for software use) */
	unsigned sd_rsv1 : 1;        /* reserved */
	unsigned sd_db : 1;          /* 0 = 16-bit segment, 1 = 32-bit segment */
	unsigned sd_g : 1;           /* Granularity: limit scaled by 4K when set */
	unsigned sd_base_31_24 : 8;  /* Hight bits of base */
};
/* Null segment */
#define SEG_NULL (struct seg_desc){0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
/* Segment that is loadable but faults when used */
#define SEG_FAULT (struct seg_desc){0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0}
/* Normal segment */
#define SEG(type, base, lim, dpl) (struct seg_desc)			\
{ ((lim) >> 12) & 0xffff, (base) & 0xffff, ((base) >> 16) & 0xff,	\
    type, 1, dpl, 1, (unsigned) (unsigned) (lim) >> 28, 0, 0, 1, 1,	\
    (unsigned) (base) >> 24 }
#define SEG16(type, base, lim, dpl) (struct seg_desc)			\
{ (lim) & 0xffff, (base) & 0xffff, ((base) >> 16) & 0xff,		\
    type, 1, dpl, 1, (unsigned) (unsigned) (lim) >> 16, 0, 0, 1, 0,	\
    (unsigned) (base) >> 24 }


/* Task state segment format(as described by the Pentium architecture
 * book). */
struct Taskstate {
	u_int ts_link;         /* Old ts selector */
	u_int ts_esp0;         /* Stack pointers and segment selectors */
	u_short ts_ss0;        /*   after an increase in privilege level */
	u_int : 0;
	u_int ts_esp1;
	u_short ts_ss1;
	u_int : 0;
	u_int ts_esp2;
	u_short ts_ss2;
	u_int : 0;
	u_int ts_cr3;          /* Page directory base */
	u_int ts_eip;          /* Saved state from last task switch */
	u_int ts_eflags;
	u_int ts_eax;          /* More saved state(registers) */
	u_int ts_ecx;
	u_int ts_edx;
	u_int ts_ebx;
	u_int ts_esp;
	u_int ts_ebp;
	u_int ts_esi;
	u_int ts_edi;
	u_short ts_es;         /* Even more saved state(segment selectors) */
	u_int : 0;
	u_short ts_cs;
	u_int : 0;
	u_short ts_ss;
	u_int : 0;
	u_short ts_ds;
	u_int : 0;
	u_short ts_fs;
	u_int : 0;
	u_short ts_gs;
	u_int : 0;
	u_short ts_ldt;
	u_int : 0;
	u_short ts_iomb;       /* I/O map base address */
	u_short ts_t;          /* Trap on task switch */
};

/* Gate descriptors are slightly different */
struct Gatedesc {
	unsigned gd_off_15_0 : 16;   /* low 16 bits of offset in segment */
	unsigned gd_ss : 16;         /* segment selector */
	unsigned gd_args : 5;        /* # args, 0 for interrupt/trap gates */
	unsigned gd_rsv1 : 3;        /* reserved(should be zero I guess) */
	unsigned gd_type :4;         /* type(STS_{TG,IG32,TG32}) */
	unsigned gd_s : 1;           /* must be 0 (system) */
	unsigned gd_dpl : 2;         /* descriptor(meaning new) privilege level */
	unsigned gd_p : 1;           /* Present */
	unsigned gd_off_31_16 : 16;  /* high bits of offset in segment */
};

/* Normal gate descriptor */
#define SETGATE(gate, istrap, ss, off, dpl)			\
{								\
	(gate).gd_off_15_0 = (u_long) (off) & 0xffff;		\
	(gate).gd_ss = (ss);					\
	(gate).gd_args = 0;					\
	(gate).gd_rsv1 = 0;					\
	(gate).gd_type = (istrap) ? STS_TG32 : STS_IG32;	\
	(gate).gd_s = 0;					\
	(gate).gd_dpl = dpl;					\
	(gate).gd_p = 1;					\
	(gate).gd_off_31_16 = (u_long) (off) >> 16;		\
}

#define GATE(type, sel, off, dpl) (struct gate_desc)		\
	{ off & 0xffff, sel, 0, 0, type, 0, dpl, 1, off >> 16 }


/* Call gate descriptor */
#define SETCALLGATE(gate, ss, off, dpl)           	      \
{								\
	(gate).gd_off_15_0 = (u_long) (off) & 0xffff;		\
	(gate).gd_ss = (ss);					\
	(gate).gd_args = 0;					\
	(gate).gd_rsv1 = 0;					\
	(gate).gd_type = STS_CG32;				\
	(gate).gd_s = 0;					\
	(gate).gd_dpl = dpl;					\
	(gate).gd_p = 1;					\
	(gate).gd_off_31_16 = (u_long) (off) >> 16;		\
}

/* Pseudo-descriptors used for LGDT, LLDT and LIDT instructions */
struct Pseudodesc {
	u_short pd__garbage;         /* LGDT supposed to be from address 4N+2 */
	u_short pd_lim;              /* Limit */
	u_long pd_base __attribute__ ((packed));       /* Base address */
};
#define PD_ADDR(desc) (&(desc).pd_lim)

#endif /* !__ASSEMBLER__ */

/* Application segment type bits */
#define STA_X 0x8              /* Executable segment */
#define STA_E 0x4              /* Expand down(non-executable segments) */
#define STA_C 0x4              /* Conforming code segment(executable only) */
#define STA_W 0x2              /* Writeable(non-executable segments) */
#define STA_R 0x2              /* Readable(executable segments) */
#define STA_A 0x1              /* Accessed */

/* System segment type bits */
#define STS_T16A 0x1           /* Available 16-bit TSS */
#define STS_LDT 0x2            /* Local Descriptor Table */
#define STS_T16B 0x3           /* Busy 16-bit TSS */
#define STS_CG16 0x4           /* 16-bit Call Gate */
#define STS_TG 0x5             /* Task Gate / Coum Transmitions */
#define STS_IG16 0x6           /* 16-bit Interrupt Gate */
#define STS_TG16 0x6           /* 16-bit Trap Gate */
#define STS_T32A 0x9           /* Available 32-bit TSS */
#define STS_T32B 0xb           /* Busy 32-bit TSS */
#define STS_CG32 0xc           /* 32-bit Call Gate */
#define STS_IG32 0xe           /* 32-bit Interrupt Gate */
#define STS_TG32 0xf           /* 32-bit Task Gate */

/*
 * Part 2.  Our conventions.
 */

/* Global descriptor numbers */
#define GD_KT     0x08     /* kernel text */
#define GD_KD     0x10     /* kernel data */
#define GD_UT     0x18     /* user text */
#define GD_UD     0x20     /* user data */
#define GD_TSS    0x28     /* Task segment selector */

/*
 * Virtual memory map:                                Permissions
 *                                                    kernel/user
 *
 *    4 Gig -------->  +------------------------------+
 *		       |			      | RW/--
 *		       ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *                     :              .               :
 *                     :              .               :
 *                     :              .               :
 *  		       |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~| RW/--
 *		       |			      | RW/--
 *		       |  Physical Memory	      | RW/--
 *		       |			      | RW/--
 *    KERNBASE ----->  +------------------------------+
 *		       |  Kernel Virtual Page Table   | RW/--   PGSIZE
 *    VPT,KSTACKTOP--> +------------------------------+                 --+
 *             	       |        Kernel Stack          | RW/--  KSTKSIZE   |
 *                     | - - - - - - - - - - - - - - -|                 PDSIZE
 *		       |       Invalid memory 	      | --/--             |
 *    ULIM     ------> +------------------------------+                 --+
 *       	       |      R/O User VPT            | R-/R-   PGSIZE
 *    UVPT      ---->  +------------------------------+
 *                     |        R/O PPAGE             | R-/R-   PGSIZE
 *    UPPAGES   ---->  +------------------------------+
 *                     |        R/O UENVS             | R-/R-   PGSIZE
 * UTOP,UENVS -------> +------------------------------+
 * UXSTACKTOP -/       |      user exception stack    | RW/RW   PGSIZE  
 *                     +------------------------------+
 *                     |       Invalid memory         | --/--   PGSIZE
 *    USTACKTOP  ----> +------------------------------+
 *                     |     normal user stack        | RW/RW   PGSIZE
 *                     +------------------------------+
 *                     |                              |
 *                     |                              |
 *		       ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *                     .                              .
 *                     .                              .
 *                     .                              .
 *  		       |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
 *		       |                              |
 *    UTEXT ------->   +------------------------------+
 *                     |                              |  2 * PDSIZE
 *    0 ------------>  +------------------------------+
 */


/* All physical memory mapped at this address */
#define	KERNBASE	0xf0000000	/* start of kernel virtual space */

/*
 * Virtual page table.  Last entry of all PDEs contains a pointer to
 * the PD itself, thereby turning the PD into a page table which
 * maps all PTEs over the last 4 Megs of the virtual address space
 */
#define VPT (KERNBASE - PGSIZE)
#define KSTACKTOP VPT
#define KSTKSIZE (8 * PGSIZE)   		/* size of a kernel stack */
#define ULIM (KSTACKTOP - PDSIZE) 

/*
 * User read-only mappings! Anything below here til UTOP are readonly to user.
 * They are global pages mapped in at env allocation time.
 */

/* Same as VPT but read-only for users */
#define UVPT (ULIM - PDSIZE)
/* Read-only copies of all ppage structures */
#define UPPAGES (UVPT - PDSIZE)
/* Read only copy of the global env structures */
#define UENVS (UPPAGES - PDSIZE)

/*
 * Top of user VM. User can manipulate VA from UTOP-1 and down!
 */
#define UTOP UENVS
#define UXSTACKTOP (UTOP)           /* one page user exception stack */
/* leave top page invalid to guard against exception stack overflow */ 
#define USTACKTOP (UTOP - 2*PGSIZE)   /* top of the normal user stack */
#define UTEXT (2*PDSIZE)

/*
 * Page fault modes inside kernel.
 */
#define PFM_NONE 0x0     /* No page faults expected.  Must be a kernel bug */
#define PFM_KILL 0x1     /* On fault kill user process. */

/*
 * Part 3.  Our helper functions.
 */
#ifndef __ASSEMBLER__

#include <inc/types.h>

/* These macros take a user supplied address and turn it into
 * something that will cause a fault if it is a kernel address.  ULIM
 * itself is guaranteed never to contain a valid page.  
*/
#define TRUP(_p)  /* Translate User Pointer */			\
({								\
	register typeof((_p)) __m_p = (_p);			\
	(u_int) __m_p > ULIM ? (typeof(_p)) ULIM : __m_p;	\
})

/* translates from virtual address to physical address */
/* BUG case */
#define PADDR(kva)						\
({								\
	u_long a = (u_long) (kva);				\
	if ((u_long) a < KERNBASE)				\
		panic("%s:%d: PADDR called with invalid kva",	\
			__FILE__, __LINE__);			\
	a - KERNBASE;						\
})

/* translates from physical address to kernel virtual address */
/* BUG case */
#define KADDR(pa)						\
({								\
	u_long ppn = (u_long) (pa) >> PGSHIFT;			\
	if (ppn >= nppage)					\
		panic("%s:%d: KADDR called with invalid pa",	\
			__FILE__, __LINE__);			\
	(pa) + KERNBASE;					\
})

void bcopy(const void *, void *, size_t);
void bzero(void *, size_t);

extern u_long nppage;


typedef unsigned int Pte;
typedef unsigned int Pde;

/*
 * The page directory entry corresponding to the virtual address range
 * from VPT to (VPT+PDSIZE) points to the page directory itself
 * (treating it as a page table as well as a page directory).  One
 * result of treating the page directory as a page table is that all
 * PTE's can be accessed through a "virtual page table" at virtual
 * address VPT (to which vpt is set in locore.S).  A second
 * consequence is that the contents of the current page directory will
 * always be available at virtual address(VPT+(VPT>>PGSHIFT)) to
 * which vpd is set in locore.S.
 */
extern Pte vpt[];     /* VA of "virtual page table" */
extern Pde vpd[];     /* VA of current page directory */

#endif /* !__ASSEMBLER__ */

#endif /* !_MMU_H_ */
