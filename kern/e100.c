#if LAB >= 6
// LAB 6: Your driver code here
#if SOL >= 6
#include <inc/x86.h>
#include <kern/pci.h>
#include <kern/e100.h>

#define	E100_CSR_PORT			0x08	// port (4 bytes)

#define E100_PORT_SOFTWARE_RESET	0

static struct {
    uint32_t iobase;
    uint8_t irq_line;
} the_e100;

static void udelay(unsigned int u)
{
	unsigned int i;
	for (i = 0; i < u; i++)
		inb(0x84);
}

int e100_attach(struct pci_func *pcif)
{
	the_e100.irq_line = pcif->irq_line;
	the_e100.iobase = pcif->reg_base[1];

	pci_func_enable(pcif);

	// Reset the card
	outl(the_e100.iobase + E100_CSR_PORT, E100_PORT_SOFTWARE_RESET);
	udelay(10);

	return 0;
}
#endif  // SOL >= 6
#endif  // LAB >= 6
