#if LAB >= 5

#include <inc/string.h>

#include <kern/cpu.h>
#include <kern/spinlock.h>
#include <kern/trap.h>
#include <kern/proc.h>
#include <kern/net.h>

#include <dev/e100.h>


uint8_t net_node;	// My node number - from net_mac[5]
uint8_t net_mac[6];	// My MAC address from the Ethernet card

spinlock net_lock;
proc *net_migrlist;	// List of currently migrating processes

#define NET_ETHERTYPE	9876	// Claim this ethertype for our packets


void net_rxmigrq(net_migrq *migrq);
void net_rxmigrp(net_migrp *migrp);
void net_txmigrp(uint8_t dstnode, uint32_t prochome);

void
net_init(void)
{
	if (!cpu_onboot())
		return;

	// Ethernet card should already have been initialized
	assert(net_mac[0] != 0 && net_mac[5] != 0);
	net_node = net_mac[5];

	spinlock_init(&net_lock);
}

// Setup the Ethernet header in a packet to be sent.
static void
net_ethsetup(net_ethhdr *eth, uint8_t destnode)
{
	assert(destnode != 0 && destnode != net_node);

	memcpy(eth->dst, net_mac, 6);	eth->dst[5] = destnode;
	memcpy(eth->src, net_mac, 6);
	eth->type = htons(NET_ETHERTYPE);
}

// The e100 network interface device driver calls this
// from its interrupt handler whenever it receives a packet.
void
net_rx(void *pkt, int len)
{
	if (len < sizeof(net_hdr)) {
		warn("net_rx: runt packet (%d bytes)", len);
		return;	// drop
	}
	net_hdr *h = pkt;
	if (memcmp(h->eth.dst, net_mac, 6) != 0) {	// is it for us?
		warn("net_rx: stray packet received for someone else");
		return;	// drop
	}
	if (memcpy(h->eth.src, net_mac, 5) != 0		// from a node we know?
			|| h->eth.src[5] < 1 || h->eth.src[5] > NET_MAXNODES) {
		warn("net_rx: stray packet received from outside cluster");
		return; // drop
	}
	if (h->eth.type != htons(NET_ETHERTYPE)) {
		warn("net_rx: unrecognized ethertype %x", ntohs(h->eth.type));
		return;	// drop
	}
	switch (h->type) {
	case NET_MIGRQ:
		if (len < sizeof(net_migrq)) {
			warn("net_rx: runt migrate request (%d bytes)", len);
			return;	// drop
		}
		net_rxmigrq(pkt);
		break;
	case NET_MIGRP:
		if (len < sizeof(net_migrp)) {
			warn("net_rx: runt migrate reply (%d bytes)", len);
			return;	// drop
		}
		net_rxmigrp(pkt);
		break;
	default:
		warn("net_rx: unrecognized message type %x", h->type);
		return;
	}
}

// Just a trivial wrapper for the e100 driver's transmit function
int net_tx(void *hdr, int hlen, void *body, int blen)
{
	return e100_tx(hdr, hlen, body, blen);
}

// Whenever we send a page containing remote refs to a new node,
// we call this function to account for this sharing
// by ORing the destination node into the pageinfo's sharemask.
void
net_rrshare(void *page, uint8_t dstnode)
{
	pageinfo *pi = mem_ptr2pi(page);
	assert(pi > &mem_pageinfo[1] && pi < &mem_pageinfo[mem_npage]);
	assert(pi != mem_ptr2pi(pmap_zero));	// No remote refs to zero page!

	assert(dstnode > 0 && dstnode <= NET_MAXNODES);
	assert(NET_MAXNODES <= sizeof(pi->shared)*8);
	pi->shared |= 1 << (dstnode-1);		// XXX lock_or?
}

// Called from syscall handlers to migrate to another node if we need to.
// The 'node' argument is the node to migrate to.
void gcc_noinline
net_migrate(trapframe *tf, uint8_t dstnode)
{
	proc *p = proc_cur();
	assert(dstnode != 0 && dstnode != RRNODE(p->home));
	cprintf("proc %x migrating to node %d\n", p, dstnode);

	// Create and send a migrate request
	net_migrq rq;
	net_ethsetup(&rq.eth, dstnode);
	rq.type = NET_MIGRQ;
	rq.home = p->home;
	rq.pdir = RRCONS(net_node, mem_phys(p->pdir));
	rq.cpu.tf = p->tf;
	rq.cpu.fx = p->fx;

	// Account for the fact that we've shared this process,
	// to make sure the remote refs it contains don't go away.
	// (In the case of a proc it won't anyway, but just for consistency.)
	net_rrshare(p, dstnode);

	// Mark the process "migrated" and put it to sleep on the migrlist
	spinlock_acquire(&net_lock);
	assert(p->state == PROC_RUN);
	assert(p->migrnext == NULL);
	p->state = PROC_MIGR;
	p->migrnext = net_migrlist;
	net_migrlist = p;
	spinlock_release(&net_lock);

	// Ship out the migrate request
	net_tx(&rq, sizeof(rq), NULL, 0);

	// Go do something else
	proc_sched();
}

// This gets called by net_rx() to process a received migrq packet.
void net_rxmigrq(net_migrq *migrq)
{
	uint8_t srcnode = migrq->eth.src[5];
	assert(srcnode > 0 && srcnode <= NET_MAXNODES);

	// Do we already have a local proc corresponding to the remote one?
	pageinfo *pi = mem_rrlookup(migrq->home);
	proc *p = pi != NULL ? mem_pi2ptr(pi) : NULL;
	if (p == NULL) {
		p = proc_alloc(NULL, 0);	// Allocate new "root proc"
		p->state = PROC_AWAY;		// Pretend it's been away
		p->home = migrq->home;
		pi = mem_ptr2pi(p);
		mem_rrtrack(migrq->home, pi);	// Track for future reference
	}
	assert(p->home == migrq->home);

	// If the proc isn't in the AWAY state, assume it's a duplicate.
	// XXX not very reliable - should probably have sequence numbers too.
	if (p->state != PROC_AWAY) {
		warn("net_rxmigrq: proc %x is already local");
		return net_txmigrp(srcnode, p->home);
	}

	// Copy the CPU state and pdir RR into our proc struct
	p->tf = migrq->cpu.tf;
	p->fx = migrq->cpu.fx;
	p->rrpdir = migrq->pdir;

	// Acknowledge the migration request so the source node stops resending
	net_txmigrp(srcnode, p->home);

	panic("XXX");
}

// Transmit a migration reply to a given node, for a given proc's home RR
void
net_txmigrp(uint8_t dstnode, uint32_t prochome)
{
	net_migrp migrp;
	net_ethsetup(&migrp.eth, dstnode);
	migrp.type = NET_MIGRP;
	migrp.home = prochome;
	net_tx(&migrp, sizeof(migrp), NULL, 0);
}

// Receive a migrate reply message.
void net_rxmigrp(net_migrp *migrp)
{
	uint8_t msgsrcnode = migrp->eth.src[5];
	assert(msgsrcnode > 0 && msgsrcnode <= NET_MAXNODES);

	// Lookup and remove the process from the migrlist.
	spinlock_acquire(&net_lock);
	proc *p, **pp;
	for (pp = &net_migrlist; (p = *pp) != NULL; pp = &p->migrnext)
		if (p->home == migrp->home) {
			*pp = p->migrnext;	// remove from migrlist
			break;
		}
	spinlock_release(&net_lock);
	if (p == NULL) {
		warn("net_rxmigrp: unknown proc RR %x", migrp->home);
		return;	// drop packet
	}

	cprintf("net_rxmigrp: proc %x successfully migrated\n");
}

#endif // LAB >= 5
