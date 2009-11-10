#if LAB >= 6
// LAB 6: Your driver code here
#if SOL >= 6
#include <inc/x86.h>
#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/error.h>
#include <kern/pci.h>
#include <kern/e100.h>
#include <kern/pmap.h>

#define E100_TX_SLOTS			64
#define E100_NULL			0xffffffff

#define	E100_CSR_SCB_COMMAND		0x02	// scb_command (1 byte)
#define	E100_CSR_SCB_GENERAL		0x04	// scb_general (4 bytes)
#define	E100_CSR_PORT			0x08	// port (4 bytes)

#define E100_PORT_SOFTWARE_RESET	0

#define E100_SCB_COMMAND_CU_START	0x10
#define E100_SCB_COMMAND_CU_RESUME	0x20

// commands
#define E100_CB_COMMAND_XMIT		0x4

// command flags
#define E100_CB_COMMAND_SF		0x0008	// simple/flexible mode
#define E100_CB_COMMAND_I		0x2000	// generate interrupt on completion
#define E100_CB_COMMAND_S		0x4000	// suspend on completion
#define E100_CB_COMMAND_EL		0x8000	// end of list

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

#define E100_SIZE_MASK			0x3fff	// mask out status/control bits

struct e100_tx_slot {
	struct e100_cb_tx tcb;
	struct e100_tbd tbd;
	struct Page *p;
};

static struct {
	uint32_t iobase;
	uint8_t irq_line;

	struct e100_tx_slot tx[E100_TX_SLOTS];
	int tx_head;
	int tx_tail;
	char tx_idle;
} the_e100;

static void udelay(unsigned int u)
{
	unsigned int i;
	for (i = 0; i < u; i++)
		inb(0x84);
}

static void
e100_scb_wait(void)
{
	int i;

	for (i = 0; i < 100000; i++) {
		if (inb(the_e100.iobase + E100_CSR_SCB_COMMAND) == 0)
			return;
	}
	
	cprintf("e100_scb_wait: timeout\n");
}

static void
e100_scb_cmd(uint8_t cmd)
{
    outb(the_e100.iobase + E100_CSR_SCB_COMMAND, cmd);
}

static void e100_tx_start(void)
{
	int i = the_e100.tx_tail % E100_TX_SLOTS;

	if (the_e100.tx_tail == the_e100.tx_head)
		panic("oops, no TCBs");

	if (the_e100.tx_idle) {
		e100_scb_wait();
		outl(the_e100.iobase + E100_CSR_SCB_GENERAL, PADDR(&the_e100.tx[i].tcb));
		e100_scb_cmd(E100_SCB_COMMAND_CU_START);
		the_e100.tx_idle = 0;
	} else {
		e100_scb_wait();
		e100_scb_cmd(E100_SCB_COMMAND_CU_RESUME);
	}
}

int e100_txbuf(struct Page *pp, unsigned int size, unsigned int offset)
{
	int i;

	if (the_e100.tx_head - the_e100.tx_tail == E100_TX_SLOTS) {
		cprintf("e100_txbuf: no space\n");
		return -E_NO_MEM;
	}

	i = the_e100.tx_head % E100_TX_SLOTS;

	the_e100.tx[i].tbd.tb_addr = page2pa(pp) + offset;
	the_e100.tx[i].tbd.tb_size = size & E100_SIZE_MASK;
	the_e100.tx[i].tcb.cb_status = 0;
	the_e100.tx[i].tcb.cb_command = E100_CB_COMMAND_XMIT |
		E100_CB_COMMAND_SF | E100_CB_COMMAND_I | E100_CB_COMMAND_S;

	pp->pp_ref++;
	the_e100.tx[i].p = pp;
	the_e100.tx_head++;
	
	e100_tx_start();

	return 0;
}

int e100_attach(struct pci_func *pcif)
{
	int i, next;

	pci_func_enable(pcif);

	the_e100.irq_line = pcif->irq_line;
	the_e100.iobase = pcif->reg_base[1];
	the_e100.tx_idle = 1;

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
