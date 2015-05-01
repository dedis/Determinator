/*
 * search ACPI tables
 * partial implementation, just for finding LAPIC and IOAPIC
 */

#include <inc/types.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/init.h>
#include <kern/cpu.h>
#include <kern/mem.h>

#include <dev/acpi.h>
#include <dev/lapic.h>
#include <dev/ioapic.h>

static uint8_t
sum(uint8_t * addr, int len)
{
	int i, sum;

	sum = 0;
	for (i = 0; i < len; i++)
		sum += addr[i];
	return sum;
}

static void *
rsdpsearch1(uint8_t *p, uint8_t *e, int *rev)
{
	for ( ; p < e; p += 0x10)
		if (memcmp(p, "RSD PTR ", 8) == 0 && sum(p, 20) == 0) {
			*rev = ((struct acpi_rsdp *)p)->reserved;
			if (*rev == 0) {
				*rev = 1;
				return p;
			} else if (*rev == 2 && sum(p, sizeof(struct acpi2_rsdp)) == 0) {
				return p;
			}
		}
	return 0;
}
//Look for RSDP structure in the len bytes at addr.
static void *
rsdpsearch(int *rev)
{
	uint8_t *e, *p;

#ifdef DEBUG
	cprintf("find in EBDA\n");
#endif
	e = mem_ptr(0x400);
	uintptr_t addr = e[0xf];
	addr <<= 8;
	addr |= e[0xe];
	addr <<= 4;
	p = mem_ptr(addr);
	e = p + 0x400;	// 1KB in EBDA
	p = rsdpsearch1(p, e, rev);
	if (p) return p;

#ifdef DEBUG
	cprintf("find in mem\n");
#endif
	p = mem_ptr(0x0e0000);
	e = mem_ptr(0x100000);
	p = rsdpsearch1(p, e, rev);
	return p;
}

static bool
madt_scan(struct acpi_madt *madt)
{
	int pc_count = 0;
	int32_t length = madt->header.length;
	if (memcmp(madt, "APIC", 4) != 0 || sum(madt, length) != 0)
		return false;
	lapic = mem_ptr(madt->lapicaddr);
	cprintf("lapic addr %p\n", mem_phys(lapic));
	length -= sizeof(struct acpi_sdt_hdr) + 8;
	uint8_t *p = madt->ent;
	bool ret = false;
	while (length > 0) {
#ifdef DEBUG
		cprintf("MADT type %u length %u\n", p[0], p[1]);
#endif
		switch (p[0]) {
		case MADT_LAPIC:
			if ( p[4] ) {
				/* The first entry is always the BSP, else AP */
				cpu *c = !pc_count ? &cpu_boot : cpu_alloc();
				c->id = p[3];
				c->num = pc_count++;
				break;
			}
		case MADT_IOAPIC:
			ioapicid = ((struct acpi_madt_ioapic *)p)->ioapicid;
			ioapic = mem_ptr(((struct acpi_madt_ioapic *)p)->ioapicaddr);
			cprintf("ioapic %u addr %p\n", ioapicid,
				mem_phys(ioapic));
			ret = true;
		default:
			break;
		}
		length -= p[1];
		p += p[1];
	}
	return ret;
}

static bool
rsdt_scan(struct acpi_rsdt *rsdt)
{
	uint32_t length = rsdt->header.length;
	if (memcmp(rsdt, "RSDT", 4) != 0 || sum(rsdt, length) != 0)
		return false;
	uint32_t cnt = (length - sizeof(struct acpi_sdt_hdr)) / 4;
	uint32_t i;
	for (i = 0; i < cnt; i++) {
		bool valid = madt_scan(mem_ptr(rsdt->ent[i]));
		if (valid) return true;
	}
	return false;
}

static bool
xsdt_scan(struct acpi2_xsdt *xsdt)
{
	uint32_t length = xsdt->header.length;
	if (memcmp(xsdt, "XSDT", 4) != 0 || sum(xsdt, length) != 0)
		return false;
	uint32_t cnt = (length - sizeof(struct acpi_sdt_hdr)) / 8;
	uint32_t i;
	for (i = 0; i < cnt; i++) {
		bool valid = madt_scan(mem_ptr(xsdt->ent[i]));
		if (valid) return true;
	}
	return false;
}

void
acpi_init(void)
{
	if (!cpu_onboot())
		return;

	int rev = 0;
	void *p = rsdpsearch(&rev);
	if (!p)
		return;
#ifdef DEBUG
	cprintf("rsdp %p rev %u\n", p, rev);
#endif
	ismp = 1;
	if (rev == 1) {
		struct acpi_rsdp *rsdp = p;
		struct acpi_rsdt *rsdt = mem_ptr(rsdp->rsdtaddr);
#ifdef DEBUG
		cprintf("rsdp %p rsdt %p size %u\n", mem_phys(rsdp),
			mem_phys(rsdt), sizeof (struct acpi2_rsdp));
#endif
		rsdt_scan(rsdt);
	} else if (rev == 2) {
		struct acpi2_rsdp *rsdp = p;
#ifdef DEBUG
		cprintf("rsdp %p rev %u\n", mem_phys(rsdp), rsdp->revision);
#endif

		struct acpi_rsdt *rsdt = mem_ptr(rsdp->rsdtaddr);
		struct acpi2_xsdt *xsdt = mem_ptr(rsdp->xsdtaddr);
#ifdef DEBUG
		cprintf("rsdp %p rsdt %p xsdt %p len %u size %u\n",
			mem_phys(rsdp), mem_phys(rsdt), mem_phys(xsdt),
			rsdp->length, sizeof (struct acpi2_rsdp));
#endif
		if (rsdt_scan(rsdt))
			return;
		xsdt_scan(xsdt);
	} else {
		return;
	}
}

