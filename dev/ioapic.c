#if LAB >= 2
/*
 * The I/O APIC manages hardware interrupts for an SMP system.
 * http://www.intel.com/design/chipsets/datashts/29056601.pdf
 * See also pic.c, which handles the old pre-SMP interrupt controller.
 *
 * Copyright (C) 1997 Massachusetts Institute of Technology
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Derived from xv6 and FreeBSD code.
 * Adapted for PIOS by Bryan Ford at Yale University.
 */

#include <inc/types.h>
#include <inc/trap.h>
#include <inc/assert.h>

#include <kern/mem.h>
#include <kern/mp.h>

#include <dev/ioapic.h>


#define IOAPIC  0xFEC00000   // Default physical address of IO APIC

#define REG_ID     0x00  // Register index: ID
#define REG_VER    0x01  // Register index: version
#define REG_TABLE  0x10  // Redirection table base

// The redirection table starts at REG_TABLE and uses
// two registers to configure each interrupt.  
// The first (low) register in a pair contains configuration bits.
// The second (high) register contains a bitmask telling which
// CPUs can serve that interrupt.
#define INT_DISABLED	0x00010000  // Interrupt disabled
#define INT_LEVEL	0x00008000  // Level-triggered (vs edge-)
#define INT_ACTIVELOW	0x00002000  // Active low (vs high)
#define INT_LOGICAL	0x00000800  // Destination is CPU id (vs APIC ID)
#define INT_DELMOD	0x00000700  // Delivery mode
#define INT_FIXED	0x00000000	// Deliver to all matching processors
#define INT_LOWEST	0x00000100	// to processor at lowest priority

// initialized in acpi.c
int ismp;
uint8_t ioapicid;
volatile struct ioapic *ioapic;

// IO APIC MMIO structure: write reg, then read or write data.
struct ioapic {
	uint32_t reg;
	uint32_t pad[3];
	uint32_t data;
};

static uint32_t
ioapic_read(int reg)
{
	ioapic->reg = reg;
	return ioapic->data;
}

static void
ioapic_write(int reg, uint32_t data)
{
	ioapic->reg = reg;
	ioapic->data = data;
}

void
ioapic_init(void)
{
	int i, id, maxintr;

	if(!ismp)
		return;

	if (ioapic == NULL)
		ioapic = mem_ptr(IOAPIC);	// assume default address

	maxintr = (ioapic_read(REG_VER) >> 16) & 0xFF;
	id = ioapic_read(REG_ID) >> 24;
	if (id == 0) {
		// I/O APIC ID not initialized yet - have to do it ourselves.
		ioapic_write(REG_ID, ioapicid << 24);
		id = ioapicid;
	}
	if (id != ioapicid)
		warn("ioapicinit: id %d != ioapicid %d", id, ioapicid);

	// Mark all interrupts edge-triggered, active high, disabled,
	// and not routed to any CPUs.
	for (i = 0; i <= maxintr; i++){
		ioapic_write(REG_TABLE+2*i, INT_DISABLED | (T_IRQ0 + i));
		ioapic_write(REG_TABLE+2*i+1, 0);
	}
}

void
ioapic_enable(int irq)
{
	if (!ismp)
		return;

	// Mark interrupt edge-triggered, active high,
	// enabled, and routed to any CPU.
	ioapic_write(REG_TABLE+2*irq,
			INT_LOGICAL | INT_LOWEST | (T_IRQ0 + irq));
	ioapic_write(REG_TABLE+2*irq+1, 0xff << 24);
}

#endif	// LAB >= 2
