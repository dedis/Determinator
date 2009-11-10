#if LAB >= 6
// LAB 6: Your driver code here
#if SOL >= 6
#include <kern/pci.h>
#include <kern/e100.h>

static struct {
    uint32_t iobase;
    uint8_t irq_line;
} the_e100;

int e100_attach(struct pci_func *pcif)
{
	the_e100.irq_line = pcif->irq_line;
	the_e100.iobase = pcif->reg_base[1];
	return 0;
}
#endif  // SOL >= 6
#endif  // LAB >= 6
