#if LAB >= 6
#ifndef JOS_KERN_E100_H
#define JOS_KERN_E100_H
#if SOL >= 6
struct pci_func;
struct Page;
int e100_attach(struct pci_func *pcif);
int e100_txbuf(struct Page *pp, unsigned int size, unsigned int offset);
#endif  // SOL >= 6
#endif	// JOS_KERN_E100_H
#endif  // LAB >= 6
