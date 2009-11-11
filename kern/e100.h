#if LAB >= 6
#ifndef JOS_KERN_E100_H
#define JOS_KERN_E100_H
#if SOL >= 6
struct pci_func;
struct Page;
int  e100_attach(struct pci_func *pcif);
int  e100_txbuf(struct Page *pp, unsigned int size, unsigned int offset);
void e100_intr(void);

extern uint8_t e100_irq;

#endif  // SOL >= 6
#endif	// JOS_KERN_E100_H
#endif  // LAB >= 6
