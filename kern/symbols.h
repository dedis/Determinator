//
// x86 hardware definitions
//
#ifndef MIK_X86_H
#define MIK_X86_H


// Processor-generated trap numbers
#define TRAP_DIVZERO	0		// Divide by zero error
#define TRAP_DEBUG	1		// Debug break-/watchpoint exception
#define TRAP_NMI	2		// External non-maskable interrupt
#define TRAP_BREAK	3		// Breakpoint - INT3 instruction
#define TRAP_OVERFLOW	4		// Overflow - INTO instruction
#define TRAP_BOUND	5		// Bounds check - BOUND instruction
#define TRAP_INVOP	6		// Invalid opcode
#define TRAP_DEVICE	7		// Device not available (x87)
#define TRAP_DOUBLE	8		// Double fault handling another fault
#define TRAP_TSS	10		// Invalid TSS on task switch
#define TRAP_SEGMENT	11		// Segment not present
#define TRAP_STACK	12		// Stack segment error
#define TRAP_GENERAL	13		// General protection fault
#define TRAP_PAGE	14		// Page fault
#define TRAP_FPU	16		// x87 floating-point exception
#define TRAP_ALIGN	17		// Misaligned memory access
#define TRAP_MCHECK	18		// Machine check exception
#define TRAP_SIMD	19		// SIMD floating-point exception


// CPUID standard feature support bits, Function 1, returned in EDX
#define CPUID_SD_FPU	(1<<0)		// x87 FPU
#define CPUID_SD_VME	(1<<1)		// Virtual-Mode Extensions
#define CPUID_SD_DE	(1<<2)		// Debugging Extensions
#define CPUID_SD_PSE	(1<<3)		// Page-Size Extensions
#define CPUID_SD_TSC	(1<<4)		// Time-Stamp Counter
#define CPUID_SD_MSR	(1<<5)		// Model-Specific Registers
#define CPUID_SD_PAE	(1<<6)		// Physical-Address Extensions
#define CPUID_SD_MCE	(1<<7)		// Machine Check Exception
#define CPUID_SD_CMPX8	(1<<8)		// CMPXCHG8B instruction
#define CPUID_SD_APIC	(1<<9)		// Advanced Programmable Int Controller
#define CPUID_SD_SYSEX	(1<<11)		// SYSENTER/SYSEXIT instructions
#define CPUID_SD_MTRR	(1<<12)		// Memory-Type Range Registers
#define CPUID_SD_PGE	(1<<13)		// Page Global Extension
#define CPUID_SD_MCA	(1<<14)		// Machine Check Architecture
#define CPUID_SD_CMOV	(1<<15)		// Conditional Move instructions
#define CPUID_SD_PAT	(1<<16)		// Page Attribute Table
#define CPUID_SD_PSE36	(1<<17)		// 36-bit Page-Size Extensions
#define CPUID_SD_CFLUSH	(1<<19)		// CFLUSH instruction
#define CPUID_SD_MMX	(1<<23)		// MMX instructions
#define CPUID_SD_FXSAVE	(1<<24)		// FXSAVE/FXRSTOR instructions
#define CPUID_SD_SSE	(1<<25)		// Streaming SIMD Extensions
#define CPUID_SD_SSE2	(1<<26)		// SSE2 instructions

// Intel-specific CPUID bits, Function 1, EDX
#define CPUID_SD_PSN	(1<<18)		// Processor Serial Number
#define CPUID_SD_DS	(1<<21)		// Debug Store
#define CPUID_SD_ACPI	(1<<22)		// Thermal Monitor and Clock Control
#define CPUID_SD_SS	(1<<27)		// Self Snoop
#define CPUID_SD_HTT	(1<<28)		// Hyper-Threading Technology
#define CPUID_SD_TM	(1<<29)		// Thermal Monitor
#define CPUID_SD_PBE	(1<<31)		// Pending Break Enable

// Intel-specific CPUID bits, Function 1, ECX
#define CPUID_SC_SSE3	(1<<0)		// SSE3 instructions
#define CPUID_SC_MON	(1<<3)		// MONITOR/MWAIT
#define CPUID_SC_DSCPL	(1<<4)		// CPL Qualified Debug Store
#define CPUID_SC_EST	(1<<7)		// Enhanced SpeedStep technology
#define CPUID_SC_TM2	(1<<8)		// Thermal Monitor 2 technology
#define CPUID_SC_CXTID	(1<<10)		// L1 Context ID

// AMD-specific extended CPUID bits (CPUID function 0x80000001)
#define CPUID_EF_NX	(1<<20)		// No-Execute page protection
#define CPUID_EF_AMDMMX	(1<<22)		// AMD extensions to MMX instructions
#define CPUID_EF_LONG	(1<<29)		// Long Mode
#define CPUID_EF_3DNOWX	(1<<30)		// Extensions to 3DNow! instructions
#define CPUID_EF_3DNOW	(1<<31)		// AMD 3DNow! instructions


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

// CR4 register layout
#define CR4_VME		(1<<0)		// Virtual-8086 Mode Extensions
#define CR4_PVI		(1<<1)		// Protected-Mode Virtual Interrupts
#define CR4_TSD		(1<<2)		// Time Stamp Disable
#define CR4_DE		(1<<3)		// Debugging Extensions
#define CR4_PSE		(1<<4)		// Page Size Extensions
#define CR4_PAE		(1<<5)		// Physical-Address Extension
#define CR4_MCE		(1<<6)		// Machine Check Enable
#define CR4_PGE		(1<<7)		// Page-Global Enable
#define CR4_PCE		(1<<8)		// Performance-Monitoring Counter Enable
#define CR4_OSFXSR	(1<<9)		// OS FXSAVE/FXRSTOR Support
#define CR4_OSX		(1<<10)		// OS Unmasked Exception Support


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


// 64-bit Task State Segment (TSS) layout
#define TSS_RSP0	0x04		// Stack pointer for privilege level 0
#define TSS_RSP1	0x0c		// Stack pointer for privilege level 1
#define TSS_RSP2	0x14		// Stack pointer for privilege level 2
#define TSS_IST1	0x24		// Interrupt stack pointer 1
#define TSS_IST2	0x2c		// Interrupt stack pointer 1
#define TSS_IST3	0x34		// Interrupt stack pointer 1
#define TSS_IST4	0x3c		// Interrupt stack pointer 1
#define TSS_IST5	0x44		// Interrupt stack pointer 1
#define TSS_IST6	0x4c		// Interrupt stack pointer 1
#define TSS_IST7	0x54		// Interrupt stack pointer 1
#define TSS_IOMBA	0x66		// I/O Map Base Address
#define TSS_SIZE	0x68


#ifndef __ASSEMBLER__

#include <mik/types.h>

// 64-bit FXSAVE/RXRSTOR structure
struct fxsave {
	u64_t	flags;
	u64_t	rip;
	u64_t	rdp;
	u32_t	mxcsr;
	u32_t	mxcsr_mask;

	// x87/MMX registers
	u64_t	mmx0[2];
	u64_t	mmx1[2];
	u64_t	mmx2[2];
	u64_t	mmx3[2];
	u64_t	mmx4[2];
	u64_t	mmx5[2];
	u64_t	mmx6[2];
	u64_t	mmx7[2];

	// SSE registers
	u64_t	xmm0[2];
	u64_t	xmm1[2];
	u64_t	xmm2[2];
	u64_t	xmm3[2];
	u64_t	xmm4[2];
	u64_t	xmm5[2];
	u64_t	xmm6[2];
	u64_t	xmm7[2];
	u64_t	xmm8[2];
	u64_t	xmm9[2];
	u64_t	xmm10[2];
	u64_t	xmm11[2];
	u64_t	xmm12[2];
	u64_t	xmm13[2];
	u64_t	xmm14[2];
	u64_t	xmm15[2];

	u64_t	reserved[6*2];
};


// Interrupt and trap gates for the IDT
struct idtgate {
	u16_t	ofs_15_0;
	u16_t	sel;
	u16_t	attrs;
	u16_t	ofs_31_16;
	u32_t	ofs_63_32;
	u32_t	reserved;
};


// C-level access to special x86 instructions
static inline void 
invlpg(u64_t va)
{ 
	asm volatile("invlpg (%0)" : : "r" (va) : "memory");
}  

#else	// __ASSEMBLER__

// Pad code to an appropriate boundary for speed using NOPs
#define TEXTALIGN	.p2align 4,,15

// Handy macro to declare a public assembly-language code entrypoint
#define ENTRY(name)	TEXTALIGN; .globl name; .type name,@function; name:

#endif	// __ASSEMBLER__

#endif // MIK_X86_H
