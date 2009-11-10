#if LAB >= 6
// LAB 6: Your driver code here
#if SOL >= 6
#include <inc/x86.h>
#include <inc/stdio.h>
#include <inc/string.h>
#include <kern/pci.h>
#include <kern/e100.h>
#include <kern/pmap.h>

#define E100_TX_SLOTS			64
#define E100_NULL			0xffffffff

#define	E100_CSR_PORT			0x08	// port (4 bytes)

#define E100_PORT_SOFTWARE_RESET	0

struct e100_cb_tx {
	volatile uint16_t cb_status;
	volatile uint16_t cb_command;
	volatile uint32_t link_addr;
	volatile uint32_t tbd_array_addr;
	volatile uint16_t byte_count;
	volatile uint8_t tx_threshold;
	volatile uint8_t tbd_number;
};

struct e100_tbd {
	volatile uint32_t tb_addr;
	volatile uint16_t tb_size;
	volatile uint16_t tb_pad;
};

struct e100_tx_slot {
	struct e100_cb_tx tcb;
	struct e100_tbd tbd;
};

static struct {
	uint32_t iobase;
	uint8_t irq_line;

	struct e100_tx_slot tx[E100_TX_SLOTS];
} the_e100;

static void udelay(unsigned int u)
{
	unsigned int i;
	for (i = 0; i < u; i++)
		inb(0x84);
}

int e100_attach(struct pci_func *pcif)
{
	int i, next;

	pci_func_enable(pcif);

	the_e100.irq_line = pcif->irq_line;
	the_e100.iobase = pcif->reg_base[1];

	// Reset the card
	outl(the_e100.iobase + E100_CSR_PORT, E100_PORT_SOFTWARE_RESET);
	udelay(10);

	// Setup TX DMA ring for CU
	for (i = 0; i < E100_TX_SLOTS; i++) {
		next = (i + 1) % E100_TX_SLOTS;
		memset(&the_e100.tx[i], 0, sizeof(the_e100.tx[i]));
		the_e100.tx[i].tcb.link_addr = PADDR(&the_e100.tx[next].tcb);
		the_e100.tx[i].tcb.tbd_array_addr = PADDR(&the_e100.tx[i].tbd);
		the_e100.tx[i].tcb.tbd_number = 1;
		the_e100.tx[i].tcb.tx_threshold = 4;
	}

	// Setup RX DMA ring for RU

	return 1;
}
#endif  // SOL >= 6
#endif  // LAB >= 6
