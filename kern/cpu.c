#if LAB >= 1
// Segment management required for privilege level changes:
// global descriptor table (GDT) and task state segment (TSS)
// See COPYRIGHT for copyright information.

#include <kern/cpu.h>


// Global descriptor table initialization prototype.
//
// The kernel and user segments are identical except for the DPL.
// To load the SS register, the CPL must equal the DPL.  Thus,
// we must duplicate the segments for the user and the kernel.
//
// The only segment descriptor that needs to be filled in later
// is the TSS descriptor, which points to a different TSS on each CPU.
//
static struct segdesc gdt_proto[] =
{
	// 0x0 - unused (always faults -- for trapping NULL far pointers)
	SEGDESC_NULL,

	// 0x08 - tss, initialized in idt_init()
	[SEG_TSS >> 3] = SEGDESC_NULL

	// 0x10 - kernel code segment
	[SEG_GDT_KCODE >> 3] = SEGDESC32(STA_X | STA_R, 0x0, 0xffffffff, 0),

	// 0x18 - kernel data segment
	[SEG_GDT_KDATA >> 3] = SEGDESC32(STA_W, 0x0, 0xffffffff, 0),

	// 0x20 - user code segment
	[SEG_GDT_UCODE >> 3] = SEGDESC32(STA_X | STA_R, 0x0, 0xffffffff, 3),

	// 0x28 - user data segment
	[SEG_GDT_UDATA >> 3] = SEGDESC32(STA_W, 0x0, 0xffffffff, 3),
};

struct pseudodesc gdt_pd = {
	sizeof(gdt) - 1, (unsigned long) gdt
};


void cpu_init()
{
	// Load the GDT
	asm volatile("lgdt gdt_pd");

	// Reload all segment registers.
	asm volatile("movw %%ax,%%gs" :: "a" (SEG_GDT_UDATA|3));
	asm volatile("movw %%ax,%%fs" :: "a" (SEG_GDT_UDATA|3));
	asm volatile("movw %%ax,%%es" :: "a" (SEG_GDT_KDATA));
	asm volatile("movw %%ax,%%ds" :: "a" (SEG_GDT_KDATA));
	asm volatile("movw %%ax,%%ss" :: "a" (SEG_GDT_KDATA));
	asm volatile("ljmp %0,$1f\n 1:\n" :: "i" (SEG_GDT_KCODE)); // reload CS
	asm volatile("lldt %%ax" :: "a" (0));

	// Setup a TSS so that we get the right stack
	// when we trap to the kernel.
	ts.ts_esp0 = KSTACKTOP;
	ts.ts_ss0 = SEG_GDT_KDATA;

	// Initialize the TSS field of the gdt.
	gdt[SEG_GDT_TSS >> 3] = SEG16(STS_T32A, (uint32_t) (&ts),
					sizeof(struct Taskstate), 0);
	gdt[SEG_GDT_TSS >> 3].sd_s = 0;

	// Load the TSS (from the GDT)
	ltr(SEG_GDT_TSS);

}

#endif /* LAB >= 1 */
