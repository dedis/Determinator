/*
 * Intel E100 network interface device driver.
 *
 * Copyright (C) 1997 Massachusetts Institute of Technology
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Primary Author: Silas Boyd-Wickizer at MIT.
 * Adapted for PIOS by Bryan Ford at Yale University.
 */
#if LAB >= 5

#include <inc/x86.h>
#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/mem.h>
#include <kern/spinlock.h>
#include <kern/net.h>

#include <dev/pic.h>
#include <dev/ioapic.h>
#include <dev/pci.h>
#include <dev/e100.h>


bool e100_present;
uint8_t e100_irq;

#define E100_TX_SLOTS			64
#define E100_RX_SLOTS			64

#define E100_NULL			0xffffffff
#define E100_SIZE_MASK			0x3fff	// mask out status/control bits

#define	E100_CSR_SCB_STATACK		0x01	// scb_statack (1 byte)
#define	E100_CSR_SCB_COMMAND		0x02	// scb_command (1 byte)
#define	E100_CSR_SCB_GENERAL		0x04	// scb_general (4 bytes)
#define	E100_CSR_PORT			0x08	// port (4 bytes)
#define E100_CSR_EEPROM			0x0e	// EEPROM control reg (1 byte)

#define E100_PORT_SOFTWARE_RESET	0

#define E100_SCB_COMMAND_CU_START	0x10
#define E100_SCB_COMMAND_CU_RESUME	0x20

#define E100_SCB_COMMAND_RU_START	1
#define E100_SCB_COMMAND_RU_RESUME	2

#define E100_SCB_STATACK_RNR		0x10
#define E100_SCB_STATACK_CNA		0x20
#define E100_SCB_STATACK_FR		0x40
#define E100_SCB_STATACK_CXTNO		0x80

// commands
#define E100_CB_COMMAND_XMIT		0x4

// command flags
#define E100_CB_COMMAND_SF		0x0008	// simple/flexible mode
#define E100_CB_COMMAND_NC		0x0010	// insert CRC and source addr
#define E100_CB_COMMAND_I		0x2000	// interrupt on completion
#define E100_CB_COMMAND_S		0x4000	// suspend on completion

#define E100_CB_STATUS_C		0x8000

#define E100_RFA_STATUS_OK		0x2000	// packet received okay
#define E100_RFA_STATUS_C		0x8000	// packet reception complete

#define E100_RFA_CONTROL_SF		0x0008	// simple/flexible memory mode
#define E100_RFA_CONTROL_S		0x4000	// suspend after reception

// EEPROM control register bits
#define E100_EESK			0x01
#define E100_EECS			0x02
#define E100_EEDI			0x04
#define E100_EEDO			0x08


struct e100_cb_tx {
	volatile uint16_t cb_status;
	volatile uint16_t cb_command;
	volatile uint32_t link_addr;
	volatile uint32_t tbd_array_addr;
	volatile uint16_t byte_count;
	volatile uint8_t tx_threshold;
	volatile uint8_t tbd_number;
};

// Transmit Buffer Descriptor (TBD)
struct e100_tbd {
	volatile uint32_t tb_addr;
	volatile uint16_t tb_size;
	volatile uint16_t tb_pad;
};

// Receive Frame Descriptor (RFD)
struct e100_rfd {
	// Fields common to all i8255x chips.
	volatile uint16_t status;
	volatile uint16_t control;
	volatile uint32_t link_addr;
	volatile uint32_t rbd_addr;
	volatile uint16_t actual;
	volatile uint16_t size;
};

// Receive Buffer Descriptor (RBD)
struct e100_rbd {
	volatile uint16_t rbd_count;
	volatile uint16_t rbd_pad0;
	volatile uint32_t rbd_link;
	volatile uint32_t rbd_buffer;
	volatile uint16_t rbd_size;
	volatile uint16_t rbd_pad1;
};

struct e100_tx_slot {
	struct e100_cb_tx tcb;	// Transmit command block
	struct e100_tbd tbd;	// Transmit buffer descriptor
	struct e100_tbd unused;	// Some cards require a second "Extended TBD"
	char buf[NET_MAXPKT];	// Buffer
};

struct e100_rx_slot {
	struct e100_rfd rfd;	// Receive frame descriptor
	char buf[NET_MAXPKT];	// Buffer - right after rfd in simplified mode
};

static struct {
	spinlock lock;
	uint32_t iobase;

	struct e100_tx_slot tx[E100_TX_SLOTS];
	int tx_head;	// Next slot we'll use to enqueue a tx packet
	int tx_tail;	// Next slot e100 should transmit and mark complete
	char tx_idle;

	struct e100_rx_slot rx[E100_RX_SLOTS];
	int rx_head;	// Next slot e100 should receive into and mark complete
	int rx_tail;	// Last slot e100 can use before it must suspend
	char rx_idle;

	int eebits;
	union {
		uint8_t mac[6];
		uint16_t mac16[3];
	};
} e100;


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

	for (i = 0; i < 100000; i++)
		if (inb(e100.iobase + E100_CSR_SCB_COMMAND) == 0)
			return;
	
	cprintf("e100_scb_wait: timeout\n");
}

static void
e100_scb_cmd(uint8_t cmd)
{
	outb(e100.iobase + E100_CSR_SCB_COMMAND, cmd);
}

static void e100_tx_start(void)
{
	assert(spinlock_holding(&e100.lock));

	int i = e100.tx_tail % E100_TX_SLOTS;

	if (e100.tx_tail == e100.tx_head)
		panic("oops, no TCBs");

	if (e100.tx_idle) {
		e100_scb_wait();
		outl(e100.iobase + E100_CSR_SCB_GENERAL, 
		     mem_phys(&e100.tx[i].tcb));
		e100_scb_cmd(E100_SCB_COMMAND_CU_START);
		e100.tx_idle = 0;
	} else {
		e100_scb_wait();
		e100_scb_cmd(E100_SCB_COMMAND_CU_RESUME);
	}
}

int e100_tx(void *hdr, int hlen, void *body, int blen)
{
	assert(hlen + blen <= NET_MAXPKT);
	int i;

	//cprintf("e100_tx: hdr %d body %d tot %d\n", hlen, blen, hlen+blen);

	spinlock_acquire(&e100.lock);

	if (e100.tx_head - e100.tx_tail == E100_TX_SLOTS) {
		warn("e100_tx: no transmit buffers");
		spinlock_release(&e100.lock);
		return 0;
	}

	i = e100.tx_head % E100_TX_SLOTS;

	// Copy the packet header and body into the transmit buffer
	memcpy(e100.tx[i].buf, hdr, hlen);
	memcpy(e100.tx[i].buf+hlen, body, blen);

	// Compute the total packet length,
	// accounting for Ethernet's 64-byte minimum.
	// XXX include the 4-byte trailing CRC.
	int len = MAX(hlen + blen, 64);

	// Set up the transmit command block
	e100.tx[i].tbd.tb_addr = mem_phys(e100.tx[i].buf);
	e100.tx[i].tbd.tb_size = len;
	e100.tx[i].tcb.cb_status = 0;
	e100.tx[i].tcb.cb_command = E100_CB_COMMAND_XMIT
		| E100_CB_COMMAND_SF | E100_CB_COMMAND_I | E100_CB_COMMAND_S;
	e100.tx_head++;

	e100_tx_start();

	spinlock_release(&e100.lock);
	return 1;
}

static void e100_rx_start(void)
{
	assert(spinlock_holding(&e100.lock));

	int i = e100.rx_head % E100_RX_SLOTS;
	if (e100.rx[i].rfd.status & E100_RFA_STATUS_C)
		return;		// We haven't finished processing this RFD.

	if (e100.rx_idle) {
		e100_scb_wait();
		outl(e100.iobase + E100_CSR_SCB_GENERAL, 
		     mem_phys(&e100.rx[i].rfd));
		e100_scb_cmd(E100_SCB_COMMAND_RU_START);
		e100.rx_idle = 0;
	} else {
		e100_scb_wait();
		e100_scb_cmd(E100_SCB_COMMAND_RU_RESUME);
	}
}

static void e100_intr_tx(void)
{
	int i;

	// Bump tx_tail past all transmit commands that have completed
	for (; e100.tx_head != e100.tx_tail; e100.tx_tail++) {
		i = e100.tx_tail % E100_TX_SLOTS;
		if (!(e100.tx[i].tcb.cb_status & E100_CB_STATUS_C))
			break;
	}
}

static void e100_intr_rx(void)
{
	assert(spinlock_holding(&e100.lock));

	int *count;
	int i;

	// Dispatch newly-filled receive buffers
	// to the kernel's network protocol stack.
	// The network stack might transmit during this upcall,
	// so we have to release and reacquire the e100.lock in the loop.
	// We use the RFD's E100_RFA_STATUS_C bit as a high-level "lock"
	// on the RFD while the received packet is being processed.
	while (1) {
		i = e100.rx_head % E100_RX_SLOTS;
		if (!(e100.rx[i].rfd.status & E100_RFA_STATUS_C))
			break;	// No more un-processed packets received

		// "Claim" this RFD by moving e100.rx_head past it,
		// while leaving the E100_RFA_STATUS_OK bit set.
		// Other CPUs might concurrently claim other RFDs
		// while we have the e100.lock released below.
		e100.rx_head++;

		// Dispatch the received packet to our network stack.
		if (e100.rx[i].rfd.status & E100_RFA_STATUS_OK) {
			spinlock_release(&e100.lock);
			int len = e100.rx[i].rfd.actual & E100_SIZE_MASK;
			net_rx(e100.rx[i].buf, len);
			spinlock_acquire(&e100.lock);
		} else
			warn("e100: packet receive error: %x",
				e100.rx[i].rfd.status);
		assert(e100.rx[i].rfd.status & E100_RFA_STATUS_C);

		// Un-claim this RFD and get it ready to be filled again.
		// Different RFDs might be un-claimed out of order
		// due to concurrency among the CPUs.
		// Mark all RFDs "suspend" until tail catches up.
		e100.rx[i].rfd.control = E100_RFA_CONTROL_S;
		e100.rx[i].rfd.status = 0;
		e100.rx[i].rfd.actual = 0;
	}

	// Now move the tail forward to the first uncompleted RFD,
	// clearing unnecessary "suspend" bits as we go.
	while (e100.rx_tail < e100.rx_head) {
		i = e100.rx_tail % E100_RX_SLOTS;
		if (e100.rx[i].rfd.status & E100_RFA_STATUS_C)
			break;	// This RFD still being processed by some CPU

		assert(e100.rx[i].rfd.control == E100_RFA_CONTROL_S);
		i = (e100.rx_tail - 1) % E100_RX_SLOTS;
		e100.rx[i].rfd.control = 0;	// Prev RFD need not suspend
		e100.rx_tail++;
	}
}

void e100_intr(void)
{
	spinlock_acquire(&e100.lock);

	int r = inb(e100.iobase + E100_CSR_SCB_STATACK);
	outb(e100.iobase + E100_CSR_SCB_STATACK, r);
	//cprintf("e100_intr status %x\n", r);

	if (r & (E100_SCB_STATACK_CXTNO | E100_SCB_STATACK_CNA)) {
		r &= ~(E100_SCB_STATACK_CXTNO | E100_SCB_STATACK_CNA);
		e100_intr_tx();
	}
	if (e100.tx_head > e100.tx_tail)
		e100_tx_start();

	if (r & E100_SCB_STATACK_RNR) {
		r &= ~E100_SCB_STATACK_RNR;
		e100.rx_idle = 1;
		cprintf("e100_intr: RNR interrupt, no RX bufs?\n");
	}

	if (r & E100_SCB_STATACK_FR) {
		r &= ~E100_SCB_STATACK_FR;
		e100_intr_rx();	// releases and re-acquires e100.lock!
	}
	e100_rx_start();

	if (r)
		warn("e100_intr: unhandled STAT/ACK %x\n", r);

	spinlock_release(&e100.lock);
}

// Clock a serial opcode/address bit out to the EEPROM.
int e100_eebit(bool bit)
{
	int eedi = bit ? E100_EEDI : 0;
	outb(e100.iobase + E100_CSR_EEPROM, E100_EECS | eedi);
	udelay(2);
	outb(e100.iobase + E100_CSR_EEPROM, E100_EECS | eedi | E100_EESK);
	udelay(2);
	outb(e100.iobase + E100_CSR_EEPROM, E100_EECS | eedi);
	udelay(2);
	return inb(e100.iobase + E100_CSR_EEPROM) & E100_EEDO;
}

#if LAB >= 99
// Clock a serial data bit in from the EEPROM.
int e100_eein(bool bit)
{
	int eedi = bit ? E100_EEDI : 0;
	outb(e100.iobase + E100_CSR_EEPROM, E100_EECS | eedi);
	outb(e100.iobase + E100_CSR_EEPROM, E100_EECS | eedi | E100_EESK);
	udelay(2);
	int eedo = inb(e100.iobase + E100_CSR_EEPROM) & E100_EEDO;
	outb(e100.iobase + E100_CSR_EEPROM, E100_EECS | eedi);
	udelay(2);
	return eedo;
}

#endif
// Read a register from the 82559's serial EEPROM.
// See Intel's 82559 EEPROM Map (AP-394) for its layout.
uint16_t e100_eeread(uint8_t addr)
{
	int i;

	outb(e100.iobase + E100_CSR_EEPROM, E100_EECS);	// activate
	e100_eebit(1); e100_eebit(1); e100_eebit(0);		// read opcode
	for (i = e100.eebits - 1; i >= 0; i--)		// address
		e100_eebit(addr & (1 << i));
	uint16_t val = 0;
	for (i = 15; i >= 0; i--)
		if (e100_eebit(0))
			val |= 1 << i;
	outb(e100.iobase + E100_CSR_EEPROM, 0);		// deactivate
	udelay(2);
	return val;
}

int e100_attach(struct pci_func *pcif)
{
	int i, next;

	pci_func_enable(pcif);

	e100_irq = pcif->irq_line;
	e100.iobase = pcif->reg_base[1];
	e100.tx_idle = 1;
	e100.rx_idle = 1;

	// Reset the card
	outl(e100.iobase + E100_CSR_PORT, E100_PORT_SOFTWARE_RESET);
	udelay(10);

	// Setup TX DMA ring for CU
	for (i = 0; i < E100_TX_SLOTS; i++) {
		next = (i + 1) % E100_TX_SLOTS;
		memset(&e100.tx[i], 0, sizeof(e100.tx[i]));
		e100.tx[i].tcb.link_addr = mem_phys(&e100.tx[next].tcb);
		e100.tx[i].tcb.tbd_array_addr = mem_phys(&e100.tx[i].tbd);
		e100.tx[i].tcb.tbd_number = 1;
		e100.tx[i].tcb.tx_threshold = 4;
	}

	// Setup RX DMA ring for RU
	for (i = 0; i < E100_RX_SLOTS; i++) {
		next = (i + 1) % E100_RX_SLOTS;
		memset(&e100.rx[i], 0, sizeof(e100.rx[i]));
		e100.rx[i].rfd.control = 0;
		e100.rx[i].rfd.status = 0;
		e100.rx[i].rfd.size = NET_MAXPKT;
		e100.rx[i].rfd.link_addr = mem_phys(&e100.rx[next].rfd);
	}
	e100.rx[E100_RX_SLOTS-1].rfd.control = E100_RFA_CONTROL_S;

	// Determine the EEPROM's size (number of address bits)
	outb(e100.iobase + E100_CSR_EEPROM, E100_EECS);	// activate
	e100_eebit(1); e100_eebit(1); e100_eebit(0);		// read opcode
	e100.eebits = 1;
	while (e100_eebit(0) != 0)
		e100.eebits++;
	outb(e100.iobase + E100_CSR_EEPROM, 0);		// deactivate
	udelay(2);
	//cprintf("e100: EEPROM size %dx16\n", 1 << e100.eebits);

	// Read the NIC's MAC address
	// (If we were diligent we would verify the EEPROM's checksum.)
	e100.mac16[0] = e100_eeread(0);
	e100.mac16[1] = e100_eeread(1);
	e100.mac16[2] = e100_eeread(2);
	cprintf("e100: MAC address");
	for (i = 0; i < 6; i++)
		cprintf("%c%02x", i ? ':' : ' ', e100.mac[i]);
	cprintf("\n");
	memcpy(net_mac, e100.mac, 6);

	// Enable network card interrupts
	pic_enable(e100_irq);
	ioapic_enable(e100_irq);

	// Start receiving packets
	spinlock_acquire(&e100.lock);
	e100_rx_start();
	spinlock_release(&e100.lock);

	e100_present = 1;
	return 1;
}

#endif  // LAB >= 5
