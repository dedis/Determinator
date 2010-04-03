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
proc *net_pulllist;	// List of processes currently pulling a page

#define NET_ETHERTYPE	0x9876	// Claim this ethertype for our packets


void net_txmigrq(proc *p);
void net_rxmigrq(net_migrq *migrq);
void net_txmigrp(uint8_t dstnode, uint32_t prochome);
void net_rxmigrp(net_migrp *migrp);

void gcc_noreturn net_pull(proc *p, uint32_t rr, void *pg, int pglevel);
void net_txpullrq(proc *p);
void net_rxpullrq(net_pullrq *pullrq);
void net_txpullrp(uint8_t dstnode, uint32_t prochome);
void net_rxpullrp(net_pullrp *pullrp);

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
	assert(destnode > 0 && destnode < NET_MAXNODES);
	assert(destnode != net_node);	// soliloquy isn't a virtue here

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
	if (memcmp(h->eth.src, net_mac, 5) != 0		// from a node we know?
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
	case NET_PULLRQ:
		if (len < sizeof(net_pullrq)) {
			warn("net_rx: runt pull request (%d bytes)", len);
			return;	// drop
		}
		net_rxpullrq(pkt);
		break;
	case NET_PULLRP:
		if (len < sizeof(net_pullrp)) {
			warn("net_rx: runt pull reply (%d bytes)", len);
			return;	// drop
		}
		net_rxpullrp(pkt, len);
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

	// Account for the fact that we've shared this process,
	// to make sure the remote refs it contains don't go away.
	// (In the case of a proc it won't anyway, but just for consistency.)
	net_rrshare(p, dstnode);

	// Mark the process "migrated" and put it to sleep on the migrlist
	spinlock_acquire(&net_lock);
	assert(p->state == PROC_RUN);
	assert(p->migrdest == 0);
	assert(p->migrnext == NULL);
	p->state = PROC_MIGR;
	p->migrdest = dstnode;
	p->migrnext = net_migrlist;
	net_migrlist = p;

	// Ship out a migrate request for this process
	net_txmigrq(p);

	spinlock_release(&net_lock);
	proc_sched();	// Go do something else
}

void
net_txmigrq(proc *p)
{
	assert(p->state == PROC_MIGR);
	assert(spinlock_holding(&net_lock));

	// Create and send a migrate request
	net_migrq rq;
	net_ethsetup(&rq.eth, p->migrdest);
	rq.type = NET_MIGRQ;
	rq.home = p->home;
	rq.pdir = RRCONS(net_node, mem_phys(p->pdir));
	rq.cpu.tf = p->tf;
	rq.cpu.fx = p->fx;
	net_tx(&rq, sizeof(rq), NULL, 0);
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
	p->pullva = PROC_USERLO;	// pull from USERLO to USERHI

	// Acknowledge the migration request so the source node stops resending
	net_txmigrp(srcnode, p->home);

	// Now we need to pull over the page directory next,
	// before we can do anything else.
	// Just pull it straight into our proc's page directory;
	net_pull(p, p->rrpdir, p->pdir, 2);
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
	assert(p->migrdest != 0);
	p->migrdest = 0;
	p->migrnext = NULL;
}

// Called by trap() on every timer interrupt,
// so that we can periodically retransmit lost packets.
void
net_tick()
{
	if (!cpu_onboot())
		return;		// count only one CPU's ticks

	static int tick;
	if (++tick & 63)
		return;

	spinlock_acquire(&net_lock);
	proc *p;
	for (p = net_migrlist; p != NULL; p = p->migrnext) {
		cprintf("retransmit migrq for %x\n", p);
		net_txmigrq(p);
	}
	spinlock_release(&net_lock);
}

// Pull a page via a remote ref and put this process to sleep waiting for it.
void gcc_noreturn
net_pull(proc *p, uint32_t rr, void *pg, int pglevel)
{
	assert(p == proc_cur());
	uint8_t dstnode = RRNODE(rr);
	assert(dstnode > 0 && dstnode <= NET_MAXNODES && dstnode != net_node);
	assert(pglevel >= 0 && pglevel <= 2);

	spinlock_acquire(&net_lock);

	assert(p->pullnext == NULL);
	p->pullnext = net_pulllist);
	net_pulllist = p;
	p->state = PROC_PULL;
	p->pullrr = rr;
	p->pullpg = pg;
	p->pglevel = pglevel;
	p->arrived = 0;		// Bitmask of page parts that have arrived

	// Ship out a pull request
	net_txpullrq(p);

	spinlock_release(&net_lock);
	proc_sched();	// Go do something else
}

// Transmit a page pull request on behalf of some process.
void
net_txpullrq(proc *p)
{
	assert(p->state == PROC_PULL);
	assert(spinlock_holding(&net_lock));

	// Create and send a pull request
	net_pullrq rq;
	net_ethsetup(&rq.eth, RRNODE(p->pullrr));
	rq.type = NET_PULLRQ;
	rq.rr = p->pullrr;
	net_tx(&rq, sizeof(rq), NULL, 0);
}

// Process a page pull request we've received.
void
net_rxpullrq(net_pullrq *rq)
{
	assert(rq->type == NET_PULLRQ);
	uint8_t rqnode = rq->eth.src[5];
	assert(rqnode > 0 && rqnode <= NET_MAXNODES && rqnode != net_node);

	// Validate the requested node number and page address.
	uint32_t rr = rq->rr;
	if (RRNODE(rr) != net_node) {
		warn("net_rxpullrq: pull request came to wrong node!?");
		return;
	}
	uint32_t addr = RRADDR(rr);
	pageinfo *pi = mem_phys2pi(addr);
	if (pi <= &mem_pageinfo[0] || pi >= &mem_pageinfo[mem_npage]) {
		warn("net_rxpullrq: pull request for invalid page %x", addr);
		return;
	}
	if (pi->refcount == 0) {
		warn("net_rxpullrq: pull request for free page %x", addr);
		return;
	}
	if (pi->home != 0) {
		warn("net_rxpullrq: pull request for unowned page %x", addr);
		return;
	}
	void *pg = mem_ptr(pg);
	cprintf("net_rxpullrq: %x", pg);

	// OK, looks legit as far as we can tell.
	// Send back the page contents in three parts,
	// so that each part is small enough to fit in an Ethernet packet.
	assert(NET_PULLPART0 == NET_PULLPART);
	assert(NET_PULLPART1 == NET_PULLPART);
	assert(NET_PULLPART2 <= NET_PULLPART);
	net_pullrp rph;
	net_ethsetup(&rph.eth, rqnode);
	rph.type = NET_PULLRP;
	rph.rr = rr;
	rph.part = 0;
	net_tx(&rph, sizeof(rph), pg+NET_PULLPART*0, NET_PULLPART0);
	rph.part = 1;
	net_tx(&rph, sizeof(rph), pg+NET_PULLPART*1, NET_PULLPART1);
	rph.part = 2;
	net_tx(&rph, sizeof(rph), pg+NET_PULLPART*2, NET_PULLPART2);

	// Mark this page shared with the requesting node.
	// (XXX might be necessarily only for pdir/ptab pages.)
	assert(NET_MAXNODES <= sizeof(pi->shared)*8);
	pi->shared |= 1 << (rqnode-1);
}

#if LAB >= 99
void
net_txpullrp(uint8_t dstnode, uint32_t prochome)
{
}

#endif // LAB >= 99
void
net_rxpullrp(net_pullrphdr *rp, int len)
{
	static const int partlen[3] = {
		NET_PULLPART0, NET_PULLPART1, NET_PULLPART2};

	assert(rp->type == NET_PULLRP);

	// Find the process waiting for this pull reply, if any.
	spinlock_acquire(&net_lock);
	proc *p;
	for (p = net_pulllist; p != NULL; p = p->pullnext) {
		assert(p->state == PROC_PULL);
		if (p->pullrr == rp->rr)
			break;
	}
	if (p == NULL) {
		warn("net_rxpullrp: no process waiting for RR %x", rp->rr);
		return spinlock_release(&net_lock);
	}
	if (rp->part < 0 || rp->part > 2) {
		warn("net_rxpullrp: invalid part number %d", rp->part);
		return spinlock_release(&net_lock);
	}
	if (p->arrived & (1 << rp->part)) {
		warn("net_rxpullrp: part %d already arrived", rp->part);
		return spinlock_release(&net_lock);
	}
	int datalen = len - sizeof(*rp);
	if (datalen != partlen[rp->part]) {
		warn("net_rxpullrp: part %d wrong size %d", rp->part, datalen);
		return spinlock_release(&net_lock);
	}

	// If it's part of a page directory or page table,
	// convert the sender's local PDEs/PTEs into remote refs.
	if (p->pglevel > 0) {
		uint32_t *pte = (uint32_t*)rp->data;
		XXX maybe better just to do this on the source node...
	}

	spinlock_release(&net_lock);
}


#endif // LAB >= 5
