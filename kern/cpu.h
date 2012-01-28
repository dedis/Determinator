#if LAB >= 1
/*
 * Per-CPU kernel state structures.
 *
 * Copyright (C) 2010 Yale University.
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Primary author: Bryan Ford
 * Adapted for 64-bit PIOS by Rajat Goyal at IIT Delhi
 */
#ifndef PIOS_KERN_SEG_H
#define PIOS_KERN_SEG_H
#ifndef PIOS_KERNEL
# error "This is a kernel header; user programs should not #include it"
#endif


// Segment selectors in the kernel's GDT
// Except for SEG_TSS, one 8-byte code/data segment 
// is followed by one 8-byte garbage segment
#define SEG_NULL	0x00	// null descriptor (required by x86 processor)
#define SEG_KERN_CS_32	0x10	// 32-bit kernel code segment
#define SEG_KERN_DS_32	0x20	// 32-bit kernel data segment
#define SEG_KERN_CS_64	0x30	// 64-bit kernel code segment
#define SEG_KERN_DS_64	0x40	// 64-bit kernel data segment
#define SEG_USER_CS_64	0x50	// 64-bit user code segment
#define SEG_USER_DS_64	0x60	// 64-bit user data segment
#define SEG_USER_GS_64	0x70	// 64-bit user thread local storage data segment
#define SEG_TSS		0x80	// Task state segment
#define CPU_GDT_NDESC	9	// number of GDT entries used, including null

#define KSTACKSIZE 4*PAGESIZE

#ifndef __ASSEMBLER__

#include <inc/assert.h>
#include <inc/types.h>
#include <inc/x86.h>
#include <inc/mmu.h>
#include <inc/trap.h>

// Per-CPU kernel state structure.
// Exactly one page (4096 bytes) in size.
typedef struct cpu {
	// Since the x86 processor finds the TSS from a descriptor in the GDT,
	// each processor needs its own TSS segment descriptor in some GDT.
	// We could have a single, "global" GDT with multiple TSS descriptors,
	// but it's easier just to have a separate fixed-size GDT per CPU.
	segdesc		gdt[CPU_GDT_NDESC];

	// Each CPU needs its own TSS,
	// because when the processor switches from lower to higher privilege,
	// it loads a new stack pointer (ESP) for the higher privilege level
	// from this task state structure.
	taskstate	tss;

	// When non-NULL, all traps get diverted to this handler.
	gcc_noreturn void (*recover)(trapframe *tf, void *recoverdata);
	void		*recoverdata;

#if LAB >= 2
	// Next in list of all CPUs - cpu_boot (below) is the list head.
	struct cpu	*next;

	// Local APIC ID of this CPU, for inter-processor interrupts etc.
	uint8_t		id;
#if LAB >= 9
	// CPU number for this CPU, assigned sequentially from 0.
	uint8_t		num;
#endif

	// Flag used in cpu.c to serialize bootstrap of all CPUs
	volatile uint32_t booted;

	// Process currently running on this CPU.
	struct proc	*proc;

#endif
	// Magic verification tag (CPU_MAGIC) to help detect corruption,
	// e.g., if the CPU's ring 0 stack overflows down onto the cpu struct.
	uint32_t	magic;

	// Low end (growth limit) of the kernel stack.
	char		kstacklo[1];

	// High end (starting point) of the kernel stack.
	char gcc_aligned(KSTACKSIZE) kstackhi[0];
} cpu;

#define CPU_MAGIC	0x98765432	// cpu.magic should always = this


// We have one statically-allocated cpu struct representing the boot CPU;
// others get chained onto this via cpu_boot.next as we find them.
extern cpu cpu_boot;

#if LAB >= 9
// This variable artificially limits the number of running CPUs;
// those above this count just spin in the idle loop.
extern int cpu_limit;
#define cpu_disabled(c)		((c)->num >= cpu_limit)
#else
#define cpu_disabled(c)		0
#endif

// Find the CPU struct representing the current CPU.
// It always resides at the bottom of the page containing the CPU's stack.
static inline cpu *
cpu_cur() {
	cpu *c = (cpu*)ROUNDDOWN(read_rsp(), KSTACKSIZE);
	assert(c->magic == CPU_MAGIC);
	return c;
}

// Returns true if we're running on the bootstrap CPU.
static inline int
cpu_onboot() {
	return cpu_cur() == &cpu_boot;
}


// Set up the current CPU's private register state such as GDT and TSS.
// Assumes the cpu struct for this CPU is basically initialized
// and that we're running on the cpu's correct kernel stack.
void cpu_init(void);

// Allocate an additional cpu struct representing a non-bootstrap processor,
// and chain it onto the list of all CPUs.
cpu *cpu_alloc(void);

// Get any additional processors booted up and running.
void cpu_bootothers(void);

#endif	// ! __ASSEMBLER__

#endif // PIOS_KERN_CPU_H
#endif // LAB >= 1
