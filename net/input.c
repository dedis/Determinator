#if LAB >= 6
#include "ns.h"
#if SOL >= 6
#define RX_SLOTS	64
#define PKTMAP		0x10000000

static void *rx_slot[RX_SLOTS];
static int head;
static int tail;

static void
fill_rxbuf(void)
{
	int i, r;
	void *p;

	while (head - tail < RX_SLOTS) {
		i = head % RX_SLOTS;
		p = (void *) PKTMAP + PGSIZE * i;
		if ((r = sys_page_alloc(0, p, PTE_P|PTE_W|PTE_U))) {
			cprintf("Input couldn't page_alloc: %e\n", r);
			break;
		}
		
		if ((r = sys_net_rxbuf(p, PGSIZE)))
			panic("Input couldn't rxbuf: %e", r);

		rx_slot[i] = p;
		head++;
	}
}

static void
forward_rxbuf(envid_t ns_envid)
{
	struct jif_pkt *pkt;
	int i;

	i = tail % RX_SLOTS;
	pkt = rx_slot[i];

	while (*((volatile int *)&pkt->jp_len) == 0)
		sys_yield();

	ipc_send(ns_envid, NSREQ_INPUT, pkt, PTE_P|PTE_W|PTE_U);
	rx_slot[i] = 0;
	sys_page_unmap(0, pkt);

	tail++;
}
#endif

void
input(envid_t ns_envid) {
	int req;

	binaryname = "ns_input";
#if SOL >= 6
	for (;;) {
		fill_rxbuf();
		forward_rxbuf(ns_envid);
	}
#endif

	// LAB 6: Your code here:
	// 	- read a packet from the device driver
	//	- send it to the network server
}
#endif
