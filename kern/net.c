#if LAB >= 5

#include <kern/cpu.h>
#include <kern/spinlock.h>
#include <kern/net.h>

#include <dev/e100.h>


spinlock net_lock;

void
net_init(void)
{
	if (!cpu_onboot())
		return;

	spinlock_init(&net_lock);
}

// The e100 network interface device driver calls this
// from its interrupt handler whenever it receives a packet.
void net_rx(void *pkt, int len)
{
}

#endif // LAB >= 5
