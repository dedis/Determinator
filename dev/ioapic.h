#if LAB >= 2
/*
 * I/O APIC (Advanced Programmable Interrupt Controller) definitions.
 *
 * Copyright (C) 1997 Massachusetts Institute of Technology
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Derived from xv6 and FreeBSD code.
 * Adapted for PIOS by Bryan Ford at Yale University.
 */
#ifndef PIOS_DEV_IOAPIC_H
#define PIOS_DEV_IOAPIC_H
#ifndef PIOS_KERNEL
# error "This is a kernel header; user programs should not #include it"
#endif

// initialized in acpi.
extern int ismp;		// True if this is an MP-capable systemc
extern uint8_t ioapicid;	// APIC ID of system's I/O APIC
extern volatile struct ioapic *ioapic;	// Address of I/O APIC

void ioapic_init(void);

void ioapic_enable(int irq);

#endif /* !PIOS_DEV_IOAPIC_H */
#endif // LAB >= 2
