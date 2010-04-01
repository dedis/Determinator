#if LAB >= 5

#include <kern/cpu.h>
#include <kern/spinlock.h>
#include <kern/trap.h>
#include <kern/proc.h>
#include <kern/net.h>

#include <dev/e100.h>


uint8_t net_node;	// My node number - from net_mac[5]
uint8_t net_mac[6];	// My MAC address from the Ethernet card

spinlock net_lock;


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

// The e100 network interface device driver calls this
// from its interrupt handler whenever it receives a packet.
void net_rx(void *pkt, int len)
{
}

// Called from syscall handlers to migrate to another node if we need to.
// The 'node' argument is the node to migrate to.
void gcc_noinline
net_migrate(trapframe *tf, uint8_t node)
{
	proc *p = proc_cur();
	assert(node != 0 && node != RRNODE(p->home));

	panic("XXX");
}

#endif // LAB >= 5
