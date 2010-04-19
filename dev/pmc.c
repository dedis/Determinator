// Code to detect and use performance monitoring counters

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/x86.h>

#include <kern/cpu.h>


// Intel model specific registers (MSRs) for performanc monitoring
#define IA32_FIXED_CTR0			0x309	// Counts INST_RETIRED.ANY
#define IA32_FIXED_CTR1			0x30a	// Counts CPU_CLK_UNHALTED.CORE
#define IA32_FIXED_CTR2			0x30b	// Counts CPU_CLK_UNHALTED.REF
#define IA32_PERF_CAPABILITIES		0x345
#define IA32_FIXED_CTR_CTRL		0x38d
#define IA32_PERF_GLOBAL_STATUS		0x38e
#define IA32_PERF_GLOBAL_CTRL		0x38f
#define IA32_PERF_GLOBAL_OVF_CTRL	0x390


// Bits Control bits for fixed PMCs
#define FIXED_CTR_EN		0x3	// Counter enable bits
#define FIXED_CTR_EN_NONE	0x0	// Disabled
#define FIXED_CTR_EN_KERN	0x1	// Count kernel-level events only
#define FIXED_CTR_EN_USER	0x2	// Count user-level events only
#define FIXED_CTR_EN_ALL	0x3	// Count events in all rings
#define FIXED_CTR_PMI		0x8	// Enable interrupt on overflow



// AMD MSRs for performance monitoring
#define PerfEvtSel0		0xc0010000	// Event select registers
#define PerfEvtSel1		0xc0010001
#define PerfEvtSel2		0xc0010002
#define PerfEvtSel3		0xc0010003
#define PerfCtr0		0xc0010004	// Performance counters
#define PerfCtr1		0xc0010005
#define PerfCtr2		0xc0010006
#define PerfCtr3		0xc0010007

#define RetiredInstructions	0xc0		// Retired instructions


void
pmc_init()
{
	if (!cpu_onboot())
		return;

	cprintf("pmc_init\n");

	cpuinfo inf;
	cpuid(0x0a, &inf);
	int ver = inf.eax & 0xff;
	int npmc = (inf.eax >> 8) & 0xff;
	int width = (inf.eax >> 16) & 0xff;
	int veclen = (inf.eax >> 24) & 0xff;
	int nfix = inf.edx & 0x1f;
	int fixwidth = (inf.edx >> 5) & 0xff;

	cprintf("PMC ver %d npmc %d width %d nfix %d fixwidth %d\n",
		ver, npmc, width, nfix, fixwidth);
	cprintf("raw %08x %08x %08x %08x\n",
		inf.eax, inf.ebx, inf.edx, inf.ecx);

	wrmsr(IA32_FIXED_CTR_CTRL, FIXED_CTR_EN_ALL);

	uint32_t v = rdmsr(IA32_FIXED_CTR_CTRL);
	cprintf("fixed ctrl %08x\n", v);

	uint64_t ctr = rdmsr(IA32_FIXED_CTR0);
	cprintf("ctr0 %016llx\n", (long long)ctr);
}

