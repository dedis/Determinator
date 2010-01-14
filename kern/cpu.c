#if LAB >= 1
// Segment management required for privilege level changes:
// global descriptor table (GDT) and task state segment (TSS)
// See COPYRIGHT for copyright information.

#include <inc/assert.h>
#include <inc/string.h>

#include <kern/mem.h>
#include <kern/cpu.h>

#if LAB >= 2
#include <dev/lapic.h>
#endif


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
	[CPU_GDT_TSS >> 3] = SEGDESC_NULL,

	// 0x10 - kernel code segment
	[CPU_GDT_KCODE >> 3] = SEGDESC32(STA_X | STA_R, 0x0, 0xffffffff, 0),

	// 0x18 - kernel data segment
	[CPU_GDT_KDATA >> 3] = SEGDESC32(STA_W, 0x0, 0xffffffff, 0),

	// 0x20 - user code segment
	[CPU_GDT_UCODE >> 3] = SEGDESC32(STA_X | STA_R, 0x0, 0xffffffff, 3),

	// 0x28 - user data segment
	[CPU_GDT_UDATA >> 3] = SEGDESC32(STA_W, 0x0, 0xffffffff, 3),
};

void
cpu_init(cpu *c)
{
	assert(ROUNDDOWN(c, PAGESIZE) == c);	// must be page aligned

	// Add the new cpu to the all CPUs list
	c->next = cpu_list;
	cpu_list = c;

	// Setup a TSS for this cpu so that we get the right stack
	// when we trap to the kernel.
	c->tss.ts_esp0 = (uint32_t) cpu_kstack(c);
	c->tss.ts_ss0 = CPU_GDT_KDATA;

	// Initialize the cpu's GDT:
	// just copy the static part from gdt_proto,
	// but the TSS descriptor is different for each cpu.
	assert(sizeof(gdt_proto) == sizeof(segdesc) * CPU_GDT_NDESC);
	memmove(c->gdt, gdt_proto, sizeof(gdt_proto));
	c->gdt[CPU_GDT_TSS >> 3] = SEGDESC16(STS_T32A, (uint32_t) (&c->tss),
					sizeof(taskstate), 0);
	c->gdt[CPU_GDT_TSS >> 3].sd_s = 0;

	// Magic verification tag for stack overflow/cpu corruption checking
	c->magic = CPU_MAGIC;
}

cpu *
cpu_alloc(void)
{
	pageinfo *pi = mem_alloc();
	assert(pi != 0);	// shouldn't be out of memory just yet!

	cpu *c = (cpu*) mem_pi2ptr(pi);

	// Clear the whole page for good measure: cpu struct and kernel stack
	memset(c, 0, PAGESIZE);

	cpu_init(c);
	return c;
}

#if LAB >= 2
void
cpu_bootothers(void)
{
	extern void startup(void);
	extern uint8_t _binary_obj_boot_bootother_start[],
			_binary_obj_boot_bootother_size[];
	uint8_t *code;

	// Write bootstrap code to unused memory at 0x7000.
	code = (uint8_t*)0x7000;
	memmove(code, _binary_obj_boot_bootother_start,
		(uint32_t)_binary_obj_boot_bootother_size);

	cpu *c;
	for(c = cpu_list; c; c = c->next){
		if(c == cpu_cur())  // We''ve started already.
			continue;

		// Fill in %esp, %eip and start code on cpu.
		*(void**)(code-4) = cpu_kstack(c);
		*(void**)(code-8) = startup;
		lapic_startcpu(c->id, (uint32_t)code);

		// Wait for cpu to get through bootstrap.
		while(c->booted == 0)
			;
	}
}
#endif	// LAB >= 2

void cpu_setup()
{
	cpu *c = cpu_cur();
	assert(c->magic == CPU_MAGIC);

	// Load the GDT
	struct pseudodesc gdt_pd = {
		sizeof(c->gdt) - 1, (uint32_t) c->gdt };
	asm volatile("lgdt %0" : : "m" (gdt_pd));

	// Reload all segment registers.
	asm volatile("movw %%ax,%%gs" :: "a" (CPU_GDT_UDATA|3));
	asm volatile("movw %%ax,%%fs" :: "a" (CPU_GDT_UDATA|3));
	asm volatile("movw %%ax,%%es" :: "a" (CPU_GDT_KDATA));
	asm volatile("movw %%ax,%%ds" :: "a" (CPU_GDT_KDATA));
	asm volatile("movw %%ax,%%ss" :: "a" (CPU_GDT_KDATA));
	asm volatile("ljmp %0,$1f\n 1:\n" :: "i" (CPU_GDT_KCODE)); // reload CS

	// We don't need an LDT.
	asm volatile("lldt %%ax" :: "a" (0));

	// Load the TSS (from the GDT)
	ltr(CPU_GDT_TSS);

	// This CPU has booted - let other CPUs boot (see cpu_bootothers).
	xchg(&c->booted, 1);
}

#endif /* LAB >= 1 */
