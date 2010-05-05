#if LAB >= 2
// Multiprocessor bootstrap.
// Search memory for MP description structures.
// http://developer.intel.com/design/pentium/datashts/24201606.pdf
// This source file adapted from xv6.

#include <inc/types.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/init.h>
#include <kern/cpu.h>
#include <kern/mp.h>

#include <dev/lapic.h>
#include <dev/ioapic.h>


int ismp;
int ncpu;
uint8_t ioapicid;
volatile struct ioapic *ioapic;


static uint8_t
sum(uint8_t * addr, int len)
{
	int i, sum;

	sum = 0;
	for (i = 0; i < len; i++)
		sum += addr[i];
	return sum;
}

//Look for an MP structure in the len bytes at addr.
static struct mp *
mpsearch1(uint8_t * addr, int len)
{
	uint8_t *e, *p;

	e = addr + len;
	for (p = addr; p < e; p += sizeof(struct mp))
		if (memcmp(p, "_MP_", 4) == 0 && sum(p, sizeof(struct mp)) == 0)
			return (struct mp *) p;
	return 0;
}

// Search for the MP Floating Pointer Structure, which according to the
// spec is in one of the following three locations:
// 1) in the first KB of the EBDA;
// 2) in the last KB of system base memory;
// 3) in the BIOS ROM between 0xE0000 and 0xFFFFF.
static struct mp *
mpsearch(void)
{
	uint8_t          *bda;
	uint32_t            p;
	struct mp      *mp;

	bda = (uint8_t *) 0x400;
	if ((p = ((bda[0x0F] << 8) | bda[0x0E]) << 4)) {
		if ((mp = mpsearch1((uint8_t *) p, 1024)))
			return mp;
	} else {
		p = ((bda[0x14] << 8) | bda[0x13]) * 1024;
		if ((mp = mpsearch1((uint8_t *) p - 1024, 1024)))
			return mp;
	}
	return mpsearch1((uint8_t *) 0xF0000, 0x10000);
}

// Search for an MP configuration table.  For now,
// don 't accept the default configurations (physaddr == 0).
// Check for correct signature, calculate the checksum and,
// if correct, check the version.
// To do: check extended table checksum.
static struct mpconf *
mpconfig(struct mp **pmp) {
	struct mpconf  *conf;
	struct mp      *mp;

	if ((mp = mpsearch()) == 0 || mp->physaddr == 0)
		return 0;
	conf = (struct mpconf *) mp->physaddr;
	if (memcmp(conf, "PCMP", 4) != 0)
		return 0;
	if (conf->version != 1 && conf->version != 4)
		return 0;
	if (sum((uint8_t *) conf, conf->length) != 0)
		return 0;
       *pmp = mp;
	return conf;
}

void
mp_init(void)
{
	uint8_t          *p, *e;
	struct mp      *mp;
	struct mpconf  *conf;
	struct mpproc  *proc;
	struct mpioapic *mpio;

	if (!cpu_onboot())	// only do once, on the boot CPU
		return;

	if ((conf = mpconfig(&mp)) == 0)
		return; // Not a multiprocessor machine - just use boot CPU.

	ismp = 1;
	lapic = (uint32_t *) conf->lapicaddr;
	for (p = (uint8_t *) (conf + 1), e = (uint8_t *) conf + conf->length;
			p < e;) {
		switch (*p) {
		case MPPROC:
			proc = (struct mpproc *) p;
			p += sizeof(struct mpproc);
			if (!(proc->flags & MPENAB))
				continue;	// processor disabled

			// Get a cpu struct and kernel stack for this CPU.
			cpu *c = (proc->flags & MPBOOT)
					? &cpu_boot : cpu_alloc();
			c->id = proc->apicid;
#if LAB >= 9
			c->num = ncpu;	// also assign sequential CPU numbers
#endif
			ncpu++;
			continue;
		case MPIOAPIC:
			mpio = (struct mpioapic *) p;
			p += sizeof(struct mpioapic);
			ioapicid = mpio->apicno;
			ioapic = (struct ioapic *) mpio->addr;
			continue;
		case MPBUS:
		case MPIOINTR:
		case MPLINTR:
			p += 8;
			continue;
		default:
			panic("mpinit: unknown config type %x\n", *p);
		}
	}
	if (mp->imcrp) {
		// Bochs doesn 't support IMCR, so this doesn' t run on Bochs.
		// But it would on real hardware.
		outb(0x22, 0x70);		// Select IMCR
		outb(0x23, inb(0x23) | 1);	// Mask external interrupts.
	}
}

#endif	// LAB >= 2
