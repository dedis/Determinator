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

void net_pull(proc *p, uint32_t rr, void *pg, int pglevel);
void net_txpullrq(proc *p);
void net_rxpullrq(net_pullrq *rq);
void net_txpullrp(uint8_t rqnode, uint32_t rr, int pglev, int part, void *pg);
void net_rxpullrp(net_pullrphdr *rp, int len);
bool net_pullpte(proc *p, uint32_t *pte, int pglevel);

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
	//cprintf("net_rx len %d\n", len);
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
		if (len < sizeof(net_pullrphdr)) {
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
	//cprintf("net_tx %d+%d\n", hlen, blen);
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

	// Ship out a migrate request - net_tick() will retransmit if necessary.
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
	cprintf("net_txmigrq proc %x home %x pdir %x\n", p, p->home, p->pdir);
	net_migrq rq;
	net_ethsetup(&rq.eth, p->migrdest);
	rq.type = NET_MIGRQ;
	rq.home = p->home;
	rq.pdir = RRCONS(net_node, mem_phys(p->pdir), 0);
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

	// If the proc isn't in the AWAY state, assume it's a duplicate packet.
	// XXX not very robust - should probably have sequence numbers too.
	if (p->state != PROC_AWAY) {
		warn("net_rxmigrq: proc %x is already local");
		return net_txmigrp(srcnode, p->home);
	}

	// Copy the CPU state and pdir RR into our proc struct
	p->tf = migrq->cpu.tf;
	p->fx = migrq->cpu.fx;
	p->rrpdir = migrq->pdir;
	p->pullva = VM_USERLO;	// pull all user space from USERLO to USERHI

	// Acknowledge the migration request so the source node stops resending
	net_txmigrp(srcnode, p->home);

	// Now we need to pull over the page directory next,
	// before we can do anything else.
	// Just pull it straight into our proc's page directory;
	// XXX first free old contents of pdir
	net_pull(p, p->rrpdir, p->pdir, PGLEV_PDIR);
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

	// Retransmit process migrate requests
	proc *p;
	for (p = net_migrlist; p != NULL; p = p->migrnext) {
		cprintf("retransmit migrq for %x\n", p);
		net_txmigrq(p);
	}

	// Retransmit page pull requests
	for (p = net_pulllist; p != NULL; p = p->pullnext) {
		cprintf("retransmit pullrq for %x\n", p);
		net_txpullrq(p);
	}

	spinlock_release(&net_lock);
}

// Pull a page via a remote ref and put process p to sleep waiting for it.
void
net_pull(proc *p, uint32_t rr, void *pg, int pglevel)
{
	cprintf("net_pull: proc %x rr %x -> %x level %d\n",
		p, rr, pg, pglevel);
	uint8_t dstnode = RRNODE(rr);
	assert(dstnode > 0 && dstnode <= NET_MAXNODES && dstnode != net_node);
	assert(pglevel >= 0 && pglevel <= 2);

	spinlock_acquire(&net_lock);

	assert(p->pullnext == NULL);
	p->pullnext = net_pulllist;
	net_pulllist = p;
	p->state = PROC_PULL;
	p->pullrr = rr;
	p->pullpg = pg;
	p->pglev = pglevel;
	p->arrived = 0;		// Bitmask of page parts that have arrived

	// Ship out a pull request - net_tick() will retransmit if necessary.
	net_txpullrq(p);

	spinlock_release(&net_lock);
}

// Transmit a page pull request on behalf of some process.
void
net_txpullrq(proc *p)
{
	assert(p->state == PROC_PULL);
	assert(spinlock_holding(&net_lock));

	// Create and send a pull request
	//cprintf("net_txpullrq proc %x rr %x lev %d need %x\n",
	//	p, p->pullrr, p->pglev, p->arrived ^ 7);
	net_pullrq rq;
	net_ethsetup(&rq.eth, RRNODE(p->pullrr));
	rq.type = NET_PULLRQ;
	rq.rr = p->pullrr;
	rq.pglev = p->pglev;
	rq.need = p->arrived ^ 7;	// Need all parts that haven't arrived
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
	void *pg = mem_pi2ptr(pi);

	// OK, looks legit as far as we can tell.
	// Send back whichever of the three page parts the caller still needs.
	// (We must divide the page into parts to fit into Ethernet packets.)
	if (rq->need & 1) net_txpullrp(rqnode, rr, rq->pglev, 0, pg);
	if (rq->need & 2) net_txpullrp(rqnode, rr, rq->pglev, 1, pg);
	if (rq->need & 4) net_txpullrp(rqnode, rr, rq->pglev, 2, pg);

	// Mark this page shared with the requesting node.
	// (XXX might be necessarily only for pdir/ptab pages.)
	assert(NET_MAXNODES <= sizeof(pi->shared)*8);
	pi->shared |= 1 << (rqnode-1);
}

static const int partlen[3] = {
	NET_PULLPART0, NET_PULLPART1, NET_PULLPART2};

void
net_txpullrp(uint8_t rqnode, uint32_t rr, int pglev, int part, void *pg)
{
	// Find appropriate part of this page
	void *data = pg + NET_PULLPART*part;
	int len = partlen[part];
	assert(len <= NET_PULLPART);
	assert((len & 3) == 0);		// must contain only whole PTEs

	// If we're transmitting part of a page directory or page table,
	// then first convert all PTEs into remote references.
	// XXX it's not ideal that we just believe the requestor's word
	// about whether this is a page table or regular page;
	// would be better if we kept our own type info in struct pageinfo.
	int nrrs = len/4;
	uint32_t rrs[nrrs];
	if (pglev > 0) {
		int i;
		const uint32_t *pt = data;
		for (i = 0; i < nrrs; i++) {
			uint32_t pte = pt[i];
			if (pte & PTE_REMOTE) {	// Already remote: just copy
				rrs[i] = pte;
				continue;
			}
			if (pte & PTE_G) {	// Kernel portion of pdir
				rrs[i] = 0;
				continue;
			}
			uint32_t addr = PGADDR(pte);
			if (addr == PTE_ZERO) {	// Zero: send only perms
				rrs[i] = RR_REMOTE | (pte & RR_RW);
				continue;
			}
			pageinfo *pi = mem_phys2pi(addr);
			assert(pi > &mem_pageinfo[0]);
			assert(pi < &mem_pageinfo[mem_npage]);
			assert(pi->refcount > 0);
			if (pi->home != 0) {	// Did we originate this page?
				rrs[i] = pi->home; // No - send original RR
			} else {		// Yes - create new RR
				rrs[i] = RRCONS(net_node, addr, pte & RR_RW);
			}
		}
		data = rrs;	// Send RRs instead of original page.
	}

	// Build and send the message
	net_pullrphdr rph;
	net_ethsetup(&rph.eth, rqnode);
	rph.type = NET_PULLRP;
	rph.rr = rr;
	rph.part = part;
	net_tx(&rph, sizeof(rph), data, len);
}

void
net_rxpullrp(net_pullrphdr *rp, int len)
{
	static const int partlen[3] = {
		NET_PULLPART0, NET_PULLPART1, NET_PULLPART2};

	assert(rp->type == NET_PULLRP);

	spinlock_acquire(&net_lock);

	// Find the process waiting for this pull reply, if any.
	proc *p, **pp;
	for (pp = &net_pulllist; (p = *pp) != NULL; pp = &p->pullnext) {
		assert(p->state == PROC_PULL);
		if (p->pullrr == rp->rr)
			break;
	}
	if (p == NULL) {
		warn("net_rxpullrp: no process waiting for RR %x", rp->rr);
		return spinlock_release(&net_lock);
	}
	int part = rp->part;
	if (part < 0 || part > 2) {
		warn("net_rxpullrp: invalid part number %d", part);
		return spinlock_release(&net_lock);
	}
	if (p->arrived & (1 << rp->part)) {
		warn("net_rxpullrp: part %d already arrived", part);
		return spinlock_release(&net_lock);
	}
	int datalen = len - sizeof(*rp);
	if (datalen != partlen[rp->part]) {
		warn("net_rxpullrp: part %d wrong size %d", part, datalen);
		return spinlock_release(&net_lock);
	}

	// Fill in the appropriate part of the page.
	memcpy(p->pullpg + NET_PULLPART*part, rp->data, datalen);
	p->arrived |= 1 << rp->part;	// Mark this part arrived.
	if (p->arrived == 7)		// All three parts arrived?
		*pp = p->pullnext;	// Remove from list of waiting procs.

	spinlock_release(&net_lock);

	if (p->arrived != 7)
		return;			// Wait for remaining parts

	// If this was a page directory, reinitialize the kernel portions.
	if (p->pglev == PGLEV_PDIR) {
		uint32_t *pdir = p->pullpg;
		int i;
		for (i = 0; i < NPDENTRIES; i++) {
			if (i == PDX(VM_USERLO))	// skip user area
				i = PDX(VM_USERHI);
			pdir[i] = pmap_bootpdir[i];
		}
	}

	// Done - what else does this proc need to pull before it can run?
	// Remove/disable this code if the VM system supports pull-on-demand.
	while (p->pullva < VM_USERHI) {

		// Pull or traverse PDE to find page table.
		uint32_t *pde = &p->pdir[PDX(p->pullva)];
		if (*pde & PTE_REMOTE) {	// Need to pull remote ptab?
			if (!net_pullpte(p, pde, PGLEV_PTAB))
				return; // Wait for the pull to complete.
		}
		assert(!(*pde & PTE_REMOTE));
		if (PGADDR(*pde) == PTE_ZERO) {		// Skip empty PDEs
			p->pullva = PTADDR(p->pullva + PTSIZE);
			continue;
		}
		uint32_t *ptab = mem_ptr(PGADDR(*pde));

		// Pull or traverse PTE to find page.
		uint32_t *pte = &ptab[PTX(p->pullva)];
		if (*pte & PTE_REMOTE) {	// Need to pull remote page?
			if (!net_pullpte(p, pte, PGLEV_PAGE))
				return;	// Wait for the pull to complete.
		}
		assert(!(*pte & PTE_REMOTE));
		p->pullva += PAGESIZE;	// Page is local - move to next.
	}

	// We've pulled the proc's entire address space: it's ready to go!
	cprintf("net_rxpullrp: migration complete\n");
	proc_ready(p);
}

// See if we need to pull a page to fill a given PDE or PTE.
// Returns false if we started a pull and need to wait until it's finished,
// or true if we were able to resolve the RR immediately.
bool
net_pullpte(proc *p, uint32_t *pte, int pglevel)
{
	uint32_t rr = *pte;
	assert(rr & RR_REMOTE);

	// Don't pull zero pages - just use our own copy instead.
	if (RRADDR(rr) == 0) {
		*pte = PTE_ZERO | (rr & RR_RW);
		if (pglevel > PGLEV_PAGE || rr & SYS_READ)
			*pte |= PTE_P | PTE_U;	// make it readable
		return 1;
	}
	//cprintf("net_pullpte proc %x rr %x level %d\n", p, rr, pglevel);

	// If we already have a copy of the page, just reuse it.
	pageinfo *pi = mem_rrlookup(rr);
	if (pi != NULL) {
		cprintf("net_pullpte: already have RR %x\n", rr);
		assert(pi->home == rr);
		assert(pi->shared != 0);
		*pte = mem_pi2phys(pi) | (rr & RR_RW);
		if (pglevel > PGLEV_PAGE || rr & SYS_READ)
			*pte |= PTE_P | PTE_U;	// make it readable
		return 1;
	}

	// Allocate a page to pull into, and replace the pte with that.
	pi = mem_alloc(); assert(pi != NULL);
	*pte = mem_pi2phys(pi) | (rr & RR_RW);
	if (pglevel > PGLEV_PAGE || rr & SYS_READ)
		*pte |= PTE_P | PTE_U;	// make it readable (but read-only)

	pi->home = rr;			// remember where it came from
	pi->shared = 1 << (RRNODE(rr) - 1);	// and that it's shared
	assert(pi->shared != 0);

	net_pull(p, rr, mem_pi2ptr(pi), pglevel);	// go pull the page
	return 0;	// Now must wait for pull to complete.
}

#endif // LAB >= 5
