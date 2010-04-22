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

// AMD PerfEvtSel register fields
#define PERF_EVENT_SELECT	0x000000ff	// Unit and Event Selection
#define PERF_UNIT_MASK		0x0000ff00	// Event Qualification
#define PERF_USR		0x00010000	// User mode
#define PERF_OS			0x00020000	// Operating-System Mode
#define PERF_E			0x00040000	// Edge Detect
#define PERF_PC			0x00080000	// Pin Control
#define PERF_INT		0x00100000	// Enable APIC Interrupt
#define PERF_EN			0x00400000	// Enable Counter
#define PERF_INV		0x00800000	// Invert Counter Mask
#define PERF_CNT_MASK		0xff000000	// Counter Mask

// AMD performance monitoring events
#define RetiredInstructions	0xc0		// Retired instructions event


bool pmc_avail;
void (*pmc_set)(int64_t maxcnt);
int64_t (*pmc_get)(int64_t maxcnt);

void
pmc_intelinit(void)
{
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

#if 0
	wrmsr(IA32_FIXED_CTR_CTRL, FIXED_CTR_EN_ALL);

	uint32_t v = rdmsr(IA32_FIXED_CTR_CTRL);
	cprintf("fixed ctrl %08x\n", v);

	uint64_t ctr = rdmsr(IA32_FIXED_CTR0);
	cprintf("ctr0 %016llx\n", (long long)ctr);
#endif
}

void
pmc_amdset(int64_t maxcnt)
{
	if (maxcnt == 0) {
		wrmsr(PerfEvtSel0, 0);	// Disable counter and interrupt
		return;
	}

	// Disable the counter, set it to the new value, and enable it
	wrmsr(PerfEvtSel0, RetiredInstructions | PERF_USR);
	wrmsr(PerfCtr0, -maxcnt & 0x00007fffffffffffLL);
	wrmsr(PerfEvtSel0, RetiredInstructions | PERF_USR | PERF_EN | PERF_INT);
}

int64_t
pmc_amdget(int64_t maxcnt)
{
	// Figure out how many instructions were actually executed,
	// given that we initialized the counter based on maxcnt.
	int64_t init = -maxcnt & 0x00007fffffffffffLL;
	int64_t ctr = rdpmc(0) & 0x0000ffffffffffffLL;
	if (ctr < init)
		ctr += 0x0001000000000000LL;	// must have wrapped

	// Now disable the counter again until the next pmc_set()
	wrmsr(PerfEvtSel0, RetiredInstructions | PERF_USR);

	return ctr - init;
}

void
pmc_amdinit(void)
{
	cpuinfo inf;
	cpuid(0x01, &inf);
	int family = ((inf.eax >> 8) & 0xf) + ((inf.eax >> 20) & 0xff);
	if (family < 0x0f || family > 0x10) {
		warn("pmc_amdinit: unrecognized AMD family %x", family);
		return;
	}

	pmc_avail = true;
	pmc_set = pmc_amdset;
	pmc_get = pmc_amdget;
}

void
pmc_init(void)
{
	if (!cpu_onboot())
		return;

	cprintf("pmc_init\n");

	cpuinfo inf;
	cpuid(0, &inf);
	if (memcmp(&inf.ebx, "GenuineIntel", 0) == 0)
		pmc_intelinit();
	else if (memcmp(&inf.ebx, "AuthenticAMD", 0) == 0)
		pmc_amdinit();

	if (!pmc_avail)
		cprintf("Warning: PMC support not found - "
			"instruction counting will be VERY slow!\n");
}

