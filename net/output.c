#if LAB >= 6
#include "ns.h"

#define PKTMAP		0x10000000

void
output(envid_t ns_envid)
{
#if SOL >= 6
	struct jif_pkt *pkt = (struct jif_pkt *)PKTMAP;
	int32_t req;
	envid_t whom;
	int perm, r;

#endif
	binaryname = "ns_output";
#if SOL >= 6
	for (;;) {
		req = ipc_recv((int32_t *) &whom, pkt, &perm);

		if (whom != ns_envid) {
			cprintf("Invalid output sender %08x\n", whom);
			continue;
		}
		
		if (req != NSREQ_OUTPUT) {
			cprintf("Invalid output request from\n");
			continue;
		}

		if ((r = sys_net_txbuf(&pkt->jp_data[0], pkt->jp_len)))
			cprintf("Output couldn't xmit: %e\n", r);
		sys_page_unmap(0, pkt);
	}
#else
	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver
#endif
}
#endif
