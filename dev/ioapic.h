#if LAB >= 1
// I/O APIC (Advanced Programmable Interrupt Controller) definitions.
// Adapted from xv6.
// See COPYRIGHT for copyright information.
#ifndef PIOS_DEV_IOAPIC_H
#define PIOS_DEV_IOAPIC_H
#ifndef PIOS_KERNEL
# error "This is a PIOS kernel header; user programs should not #include it"
#endif


void ioapic_init(void);

void ioapic_enable(int irq, int apicid);


#endif /* !PIOS_DEV_IOAPIC_H */
#endif // LAB >= 1
