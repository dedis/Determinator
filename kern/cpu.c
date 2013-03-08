#if LAB >= 1
/*
 * CPU setup and management of key protected-mode data structures,
 * such as global descriptor table (GDT) and task state segment (TSS).
 *
 * Copyright (C) 2010 Yale University.
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Primary author: Bryan Ford
 * Adapted for 64-bit PIOS by Rajat Goyal at IIT Delhi
 */

#include <inc/assert.h>
#include <inc/string.h>

#include <kern/mem.h>
#include <kern/cpu.h>
#include <kern/init.h>
#include <kern/pmap.h>

#if LAB >= 2
#include <dev/lapic.h>
#endif


cpu cpu_boot = {

	// Global descriptor table for bootstrap CPU.
	// The GDTs for other CPUs are copied from this and fixed up.
	//
	// The kernel and user segments are identical except for the DPL.
	// To load the SS register, the CPL must equal the DPL.  Thus,
	// we must duplicate the segments for the user and the kernel.
	//
	// The only descriptor that differs across CPUs is the TSS descriptor.
	//
	gdt: {
		// 0x0 - unused (always faults: for trapping NULL far pointers)
		[0] = SEGDESC_NULL,
		
		// 0x10 - 32-bit kernel code segment
		[SEG_KERN_CS_32 >> 4] = SEGDESC64(1, STA_X | STA_R, 
					0L, 0xffffffff, 0, 0),
#if SOL >= 1

		// 0x20 - 32-bit kernel data segment
		[SEG_KERN_DS_32 >> 4] = SEGDESC64(1, STA_W,
					0L, 0xffffffff, 0, 0),

		// 0x30 - 64-bit kernel code segment
		[SEG_KERN_CS_64 >> 4] = SEGDESC64(1, STA_X | STA_R,
					0L, 0xffffffff, 0, 1),

		// 0x40 - 64-bit kernel data segment
		[SEG_KERN_DS_64 >> 4] = SEGDESC64(1, STA_W,
					0L, 0xffffffff, 0, 1),

		// 0x50 - 64-bit user code segment
		[SEG_USER_CS_64 >> 4] = SEGDESC64(1, STA_X | STA_R,
					0L, 0xffffffff, 3, 1),

		// 0x60 - 64-bit user data segment
                [SEG_USER_DS_64 >> 4] = SEGDESC64(1, STA_W,
                                        0L, 0xffffffff, 3, 1),

#if LAB >= 9
		// 0x70 - 64-bit user thread local storage data segment
		[SEG_USER_GS_64 >> 4] = SEGDESC64(1, STA_W,
					0L, 0xffffffff, 3, 1),
#endif
		// 0x70 - tss, initialized in cpu_init()
		[SEG_TSS >> 4] = SEGDESC_NULL,
#endif	// SOL >= 1
	},

	magic: CPU_MAGIC
};

#if SOL >= 9
// Artificial limit on the number of CPUs the scheduler may use.
int cpu_limit = INT_MAX;

/* Static APs list */
/* 
 * The initial implementation used to make a call to mem_alloc() and 
 * then typecast the address returned to cpu *. I think that's not right.
 * First of all, mem_alloc() will return a pointer to PAGESIZE bytes of memory
 * where as kstackhi in struct cpu is 4*PAGESIZE. Second, even if I have 
 * somehow misunderstood the code, gcc_aligned fails to align kstackhi
 * to KSTACKSIZE in the old implementation.
 */
cpu cpus[NR_CPUS];

void
cpu_info()
{
	// Obtain and print some information about the CPU.
	cpuinfo inf;

	char str[12+1];
	cpuid(0x00, &inf);
	memcpy(str, &inf.ebx, 12); // str = AuthenticAMD or GenuineIntel
	str[12] = '\0';
	cpuid(0x01, &inf);

	int family = (inf.eax >> 8) & 0xf;
	int model = (inf.eax >> 3) & 0xf;
	int stepping = (inf.eax >> 3) & 0xf;
	if (family == 0xf)
		family += (inf.eax >> 20) & 0xff;
	if (family == 6 || family >= 0xf)
		model |= ((inf.eax >> 16) & 0xf) << 4;
	cprintf("CPUID: %s %x/%x/%x\n", str, family, model, stepping);
}

#define MTRRcap			0x00fe
#define MTRRphysBase0		0x0200
#define MTRRphysMask0		0x0201
#define MTRRfix64K_00000	0x0250
#define MTRRfix16K_80000	0x0258
#define MTRRfix16K_A0000	0x0259
#define MTRRfix4K_C0000		0x0268
#define MTRRdefType		0x02ff

void
cpu_setmtrr()
{
	cpuinfo inf;
	cpuid(0x01, &inf);
	if (!(inf.edx & (1 << 12))) {
		warn("cpu_setmtrr: CPU has no MTRR support?");
//		return;
	}

	cprintf("CR0: %x cap %llx deftype %llx\n",
		rcr0(), rdmsr(MTRRcap), rdmsr(MTRRdefType));
	int i;
	for (i = 0; i < 8; i++) {
		uint64_t base = rdmsr(MTRRphysBase0+2*i);
		uint64_t mask = rdmsr(MTRRphysMask0+2*i);
		if (mask & 0x800)	// only show MTRRs in use
			cprintf("MTRRvar%d: %llx %llx\n", i, base, mask);
	}
	cprintf("MTRRfix64K%d: %llx\n", 0, rdmsr(MTRRfix64K_00000));
	cprintf("MTRRfix16K%d: %llx\n", 0, rdmsr(MTRRfix16K_80000));
	cprintf("MTRRfix16K%d: %llx\n", 1, rdmsr(MTRRfix16K_A0000));
	for (i = 0; i < 8; i++)
		cprintf("MTRRfix%d: %llx\n", i, rdmsr(MTRRfix4K_C0000+i));
//	cprintf("on CPU %d\n", cpu_cur()->id);
//	if (!cpu_onboot())
//		while(1);
}
#endif

void cpu_init()
{
	cpu *c = cpu_cur();

#if SOL >= 1
#if SOL >= 9
	if (cpu_onboot()) {
		/* Initialize AP static list */
		int i;
		for (i = 0; i < NR_CPUS; i++)
			cpus[i].in_use = 0;
		cpu_info();


		// Copy the lowcode to low memory,
		// used to boot other CPUs and (optionally)
		// make callbacks to the 16-bit BIOS.
		extern uint8_t _binary_obj_boot_bootother_start[],
				_binary_obj_boot_bootother_size[];
		memmove((uint8_t*)lowcode_start,
			_binary_obj_boot_bootother_start,
			(uint64_t)_binary_obj_boot_bootother_size);

	}
#endif

	// Setup the TSS for this cpu so that we get the right stack
	// when we trap into the kernel from user mode.
	c->tss.ts_rsp[0] = (uintptr_t) c->kstackhi;
	c->tss.ts_ist[1] = (uintptr_t) c->kstackhi;
	c->tss.ts_iomb = sizeof(taskstate);

	// Initialize the non-constant part of the cpu's GDT:
	// the TSS descriptor is different for each cpu.
	c->gdt[SEG_TSS >> 4] = SEGDESC64_nogran(0, STS_T64A, (uintptr_t) (&c->tss),
					sizeof(taskstate)-1, 0, 1);

#endif	// SOL >= 1
	// Load the GDT
	struct pseudodesc gdt_pd = {
		sizeof(c->gdt) - 1, (uintptr_t) c->gdt };
	asm volatile("lgdt %0" : : "m" (gdt_pd));

	// Reload all segment registers.
	asm volatile("movw %%ax,%%gs" :: "a" (SEG_USER_DS_64|3));
	asm volatile("movw %%ax,%%fs" :: "a" (SEG_USER_DS_64|3));
	asm volatile("movw %%ax,%%es" :: "a" (SEG_KERN_DS_64));
	asm volatile("movw %%ax,%%ds" :: "a" (SEG_KERN_DS_64));
	asm volatile("movw %%ax,%%ss" :: "a" (SEG_KERN_DS_64));
	// reloading CS value can only be done with ret in long mode
	asm volatile("pushq %0\n pushq $newland\n lretq\n newland:" :: "i" (SEG_KERN_CS_64));

	// We don't need an LDT.
	asm volatile("lldt %%ax" :: "a" (0));

#if SOL >= 1
	// Load the TSS (from the GDT)
	ltr(SEG_TSS);
#endif
}

#if LAB >= 2
// Allocate an additional cpu struct representing a non-bootstrap processor.
cpu *
cpu_alloc(void)
{
	// Pointer to the cpu.next pointer of the last CPU on the list,
	// for chaining on new CPUs in cpu_alloc().  Note: static.
	static cpu **cpu_tail = &cpu_boot.next;
	cpu *c = NULL;

	int i;
	for(i=0; i < NR_CPUS; i++) {
		if(!cpus[i].in_use) {
			c = (cpu*) &cpus[i];
			break;
		}
		if(i >= NR_CPUS)
			warn("Too many CPUS on this machine, sell some..\n");
	}	

	// Clear the whole page for good measure: cpu struct and kernel stack
	memset(c, 0, sizeof(struct cpu));

	c->in_use = 1;
	// Now we need to initialize the new cpu struct
	// just to the same degree that cpu_boot was statically initialized.
	// The rest will be filled in by the CPU itself
	// when it starts up and calls cpu_init().

	// Initialize the new cpu's GDT by copying from the cpu_boot.
	// The TSS descriptor will be filled in later by cpu_init().
	assert(sizeof(c->gdt) == sizeof(segdesc) * CPU_GDT_NDESC);
	memmove(c->gdt, cpu_boot.gdt, sizeof(c->gdt));

	// Magic verification tag for stack overflow/cpu corruption checking
	c->magic = CPU_MAGIC;

	// Chain the new CPU onto the tail of the list.
	*cpu_tail = c;
	cpu_tail = &c->next;

	return c;
}

void
cpu_bootothers(void)
{
	extern uint8_t _binary_obj_boot_bootother_start[],
			_binary_obj_boot_bootother_size[];

	if (!cpu_onboot()) {
		// Just inform the boot cpu we've booted.
		xchg(&cpu_cur()->booted, 1);
		return;
	}

	// Write bootstrap code to unused memory at 0x1000 (start of 2nd page).
	uint8_t *code = (uint8_t*)lowcode_start;
	cpu *c;
	for(c = &cpu_boot; c; c = c->next){
		if(c == cpu_cur())  // We''ve started already.
			continue;

		// Fill in %rsp, %rip, location of page table and start code on cpu.
		*(void**)(code-8) = c->kstackhi;
		*(void**)(code-16) = init;
		*(void**)(code-24) = pmap_bootpmap;
		lapic_startcpu(c->id, *(uint32_t*)lowmem_bootother_vec);

		// Wait for cpu to get through bootstrap.
		while(c->booted == 0)
			;
	}
}
#endif	// LAB >= 2

#endif /* LAB >= 1 */
