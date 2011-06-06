#if LAB >= 1
/*
 * PIOS trap handling definitions.
 *
 * Copyright (C) 1997 Massachusetts Institute of Technology
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Derived from the MIT Exokernel and JOS.
 * Adapted for PIOS by Bryan Ford at Yale University.
 */

#ifndef PIOS_INC_TRAP_H
#define PIOS_INC_TRAP_H

#include <cdefs.h>


// Processor-generated trap numbers
#define T_DIVIDE	0	// divide error
#define T_DEBUG		1	// debug exception
#define T_NMI		2	// non-maskable interrupt
#define T_BRKPT		3	// breakpoint
#define T_OFLOW		4	// overflow
#define T_BOUND		5	// bounds check
#define T_ILLOP		6	// illegal opcode
#define T_DEVICE	7	// device not available
#define T_DBLFLT	8	// double fault
#define T_TSS		10	// invalid task switch segment
#define T_SEGNP		11	// segment not present
#define T_STACK		12	// stack exception
#define T_GPFLT		13	// general protection fault
#define T_PGFLT		14	// page fault
#define T_FPERR		16	// floating point error
#define T_ALIGN		17	// aligment check
#define T_MCHK		18	// machine check
#define T_SIMD		19	// SIMD floating point exception
#define T_SECEV		30	// Security-sensitive event

#define T_IRQ0		32	// Legacy ISA hardware interrupts: IRQ0-15.

// The rest are arbitrarily chosen, but with care not to overlap
// processor defined exceptions or ISA hardware interrupt vectors.
#define T_SYSCALL	48	// System call

// We use these vectors to receive local per-CPU interrupts
#define T_LTIMER	49	// Local APIC timer interrupt
#define T_LERROR	50	// Local APIC error interrupt
#if LAB >= 9
#define T_PERFCTR	51	// Performance counter overflow interrupt
#endif

#define T_DEFAULT	500	// Unused trap vectors produce this value
#define T_ICNT		501	// Child process instruction count expired

// ISA hardware IRQ numbers. We receive these as (T_IRQ0 + IRQ_WHATEVER)
#define IRQ_TIMER	0	// 8253 Programmable Interval Timer (PIT)
#define IRQ_KBD		1	// Keyboard interrupt
#define IRQ_SERIAL	4	// Serial (COM) interrupt
#define IRQ_SPURIOUS	7	// Spurious interrupt
#define IRQ_IDE		14	// IDE disk controller interrupt

#ifndef __ASSEMBLER__

#include <inc/types.h>


// This struct represents the format of the trap frames
// that get pushed on the kernel stack by the processor
// in conjunction with the interrupt/trap entry code in trapasm.S.
// All interrupts and traps use this same format,
// although not all fields are always used:
// e.g., the error code (err) applies only to some traps,
// and the processor pushes esp and ss
// only when taking a trap from user mode (privilege level >0).
typedef struct trapframe {
	uint16_t gs; uint16_t padding_gs1;  uint16_t padding_gs2;  uint16_t padding_gs3;
	uint16_t fs; uint16_t padding_fs1;  uint16_t padding_fs2;  uint16_t padding_fs3;
	uint16_t es; uint16_t padding_es1;  uint16_t padding_es2;  uint16_t padding_es3;
	uint16_t ds; uint16_t padding_ds1;  uint16_t padding_ds2;  uint16_t padding_ds3;
	uint64_t r15;
	uint64_t r14;
	uint64_t r13;
	uint64_t r12;
	uint64_t r11;
	uint64_t r10;
	uint64_t r9;
	uint64_t r8;
	uint64_t rbp;
	uint64_t rdi;
	uint64_t rsi;
	uint64_t rdx;
	uint64_t rcx;
	uint64_t rbx;
	uint64_t rax;
	uint64_t trapno;

	// format from here on determined by x86 hardware architecture
	uint64_t err;
	uintptr_t rip;
	uint16_t cs;  uint16_t padding_cs1;  uint16_t padding_cs2;  uint16_t padding_cs3;
	uint64_t rflags;

	// included whether or not crossing rings, e.g., user to kernel
	uintptr_t rsp;
	uint16_t ss;  uint16_t padding_ss1;  uint16_t padding_ss2;  uint16_t padding_ss3;
} trapframe;

// size of trapframe pushed when called from user and kernel mode, respectively
#define trapframe_usize sizeof(trapframe)	// full trapframe struct
#define trapframe_ksize sizeof(trapframe)	// full trapframe struct


// Floating-point/MMX/XMM register save area format,
// in the layout defined by the processor's FXSAVE/FXRSTOR instructions.
typedef gcc_aligned(16) struct fxsave {
	uint16_t	fcw;			// byte 0
	uint16_t	fsw;
	uint16_t	ftw;
	uint16_t	fop;
	uint32_t	fpu_ip;
	uint16_t	cs;
	uint16_t	reserved1;
	uint32_t	fpu_dp;			// byte 16
	uint16_t	ds;
	uint16_t	reserved2;
	uint32_t	mxcsr;
	uint32_t	mxcsr_mask;
	uint8_t		st_mm[8][16];		// byte 32: x87/MMX registers
	uint8_t		xmm[8][16];		// byte 160: XMM registers
	uint8_t		reserved3[11][16];	// byte 288: reserved area
	uint8_t		available[3][16];	// byte 464: available to OS
} fxsave;


#endif /* !__ASSEMBLER__ */

// Must equal 'sizeof(struct trapframe)'.
// A static_assert in kern/trap.c checks this.
#define SIZEOF_STRUCT_TRAPFRAME	0xD0

#endif /* !PIOS_INC_TRAP_H */
#endif /* LAB >= 1 */
