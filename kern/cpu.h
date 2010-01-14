#if LAB >= 1
// Per-CPU kernel state.
// See COPYRIGHT for copyright information.
#ifndef PIOS_KERN_SEG_H
#define PIOS_KERN_SEG_H
#ifndef PIOS_KERNEL
# error "This is a PIOS kernel header; user programs should not #include it"
#endif


// Global segment descriptor numbers used by the kernel
#define CPU_GDT_NULL	0x00	// null descriptor (required by x86 processor)
#define CPU_GDT_TSS	0x08	// task state segment
#define CPU_GDT_KCODE	0x10	// kernel text
#define CPU_GDT_KDATA	0x18	// kernel data
#define CPU_GDT_UCODE	0x20	// user text
#define CPU_GDT_UDATA	0x28	// user data
#define CPU_GDT_NDESC	6	// number of GDT entries used, including null


#ifndef __ASSEMBLER__

#include <inc/assert.h>
#include <inc/types.h>
#include <inc/x86.h>
#include <inc/mmu.h>


// Per-CPU kernel state structure.
typedef struct cpu {
	// Next in list of all CPUs (cpu_list) below
	struct cpu	*next;

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

	// Local APIC ID of this CPU, for inter-processor interrupts etc.
	uint8_t		id;

	// Flag used in cpu.c to serialize bootstrap of all CPUs
	uint32_t	booted;

	// For trap handling: recovery EIP when running sensitive code.
	void *		recovery;

	// Magic verification tag (CPU_MAGIC) to help detect corruption,
	// e.g., if the CPU's ring 0 stack overflows down onto the cpu struct.
	uint32_t	magic;
} cpu;

#define CPU_MAGIC	0x98765432	// cpu.magic should always = this

// Compute the location of a CPU's ring 0 kernel stack given a cpu struct.
// The cpu struct always begins at a page boundary,
// and the stack grows downward from the top of that same page.
#define cpu_kstack(cpu)	((void*)(cpu) + PAGESIZE)


// List of CPU structs for all CPUs.
// Set up at initialization time and then never changed;
// thus no lock required to protect this list.
cpu *cpu_list;


// Find the CPU struct representing the current CPU.
// It always resides at the bottom of the page containing the CPU's stack.
static inline cpu *
cpu_cur() {
	cpu *c = (cpu*)ROUNDDOWN(read_esp(), PAGESIZE);
	assert(c->magic == CPU_MAGIC);
	return c;
}


// Initialize a new cpu struct, and add it to the cpu_list.
// Must be provided a whole, page-aligned page, to make room for kstack.
void cpu_init(cpu *c);

// Allocate and initialize a new cpu struct, and add it to the cpu_list.
// Only called during bootup.
cpu *cpu_alloc(void);

// Get any additional processors booted up and running.
void cpu_bootothers(void);

// Set up the current CPU's private register state such as GDT and TSS.
// Assumes the cpu struct for this CPU is already initialized
// and that we're running on the cpu's correct kernel stack.
void cpu_startup(void);

#endif	// ! __ASSEMBLER__

#endif // PIOS_KERN_CPU_H
#endif // LAB >= 1
