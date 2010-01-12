#if LAB >= 1
// Per-CPU kernel state.
// See COPYRIGHT for copyright information.
#ifndef PIOS_KERN_SEG_H
#define PIOS_KERN_SEG_H
#ifndef PIOS_KERNEL
# error "This is a PIOS kernel header; user programs should not #include it"
#endif

#include <inc/types.h>
#include <inc/x86.h>
#include <inc/mmu.h>


// Global segment descriptor numbers used by the kernel
#define CPU_GDT_NULL	0x00	// null descriptor (required by x86 processor)
#define CPU_GDT_TSS	0x08	// task state segment
#define CPU_GDT_KCODE	0x10	// kernel text
#define CPU_GDT_KDATA	0x18	// kernel data
#define CPU_GDT_UCODE	0x20	// user text
#define CPU_GDT_UDATA	0x28	// user data
#define CPU_GDT_NDESC	6	// number of GDT entries used, including null


// Per-CPU kernel state structure.
typedef struct cpu {

	// Each CPU has its own privileged (ring 0) interrupt stack
	uint32_t	istack;

	// Each CPU needs its own TSS,
	// because when the processor switches from lower to higher privilege,
	// it loads a new stack pointer (ESP) and stack segment (SS)
	// for the higher privilege level from this task state structure.
	taskstate	tss;

	// Since the x86 processor finds the TSS from a descriptor in the GDT,
	// each processor needs its own TSS segment descriptor.
	// We could have a single, "global" GDT with multiple TSS descriptors,
	// but it's easier just to have a separate fixed-size GDT per CPU.
	segdesc		gdt[CPU_GDT_NDESC];

} cpu;


// Find the CPU struct representing the current CPU.
// It always resides at the bottom of the page containing the CPU's stack.
#define cpu_cur()	((cpu*) ROUNDDOWN(read_esp(), PAGESIZE))


#endif // PIOS_KERN_CPU_H
#endif // LAB >= 1
