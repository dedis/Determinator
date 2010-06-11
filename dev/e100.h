#if LAB >= 5
/*
 * Intel E100 network interface device driver definitions.
 *
 * Copyright (C) 1997 Massachusetts Institute of Technology
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Primary Author: Silas Boyd-Wickizer at MIT.
 * Adapted for PIOS by Bryan Ford at Yale University.
 */

#ifndef PIOS_KERN_E100_H
#define PIOS_KERN_E100_H

struct pci_func;

extern bool e100_present;
extern uint8_t e100_irq;

int  e100_attach(struct pci_func *pcif);
int  e100_tx(void *hdr, int hlen, void *body, int blen);
void e100_intr(void);

#endif	// PIOS_KERN_E100_H
#endif  // LAB >= 5
