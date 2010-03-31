#if LAB >= 5
// Network module definitions.
// See COPYRIGHT for copyright information.
#ifndef PIOS_KERN_NET_H
#define PIOS_KERN_NET_H
#ifndef PIOS_KERNEL
# error "This is a kernel header; user programs should not #include it"
#endif

// Ethernet header
typedef struct net_ehdr {
	uint8_t		dst[6];		// Destination MAC address
	uint8_t		src[6];		// Source MAC address
	uint16_t	etype;		// Ethernet packet type
} net_ehdr;

#define NET_ETYPE_IP	0x0800		// Ethernet packet type for IPv4

#define NET_MAXPKT	1514		// Max Ethernet packet size

// Internet Protocol (IP) header - see RFC791
typedef struct net_iphdr {
	uint8_t		ver_ihl;	// Version and Internet Header Length
	uint8_t		tos;		// Type of Service
	uint16_t	len;		// Total IP packet length
	uint16_t	id;		// Packet identifier
	uint16_t	frag;		// Fragmentation flags & offset
	uint8_t		ttl;		// Time to Live
	uint8_t		proto;		// Upper-layer protocol number
	uint16_t	csum;		// IP header checksum
	uint32_t	srcip;		// Source IP address
	uint32_t	dstip;		// Destination IP address
} net_iphdr;

// User Datagram Protocol (UDP) header - see RFC768
typedef struct net_udphdr {
	uint16_t	srcport;	// Source UDP port
	uint16_t	dstport;	// Destination UDP port
	uint16_t	len;		// Datagram length
	uint16_t	csum;		// Datagram checksum (optional)
} net_udphdr;

// Dynamic Host Configuration Protocol (DHCP) header - see RFC2131
typedef struct net_dhcphdr {
	uint8_t		op;		// Message operation code
	uint8_t		htype;		// Hardware address type, 1 = Ethernet
	uint8_t		hlen;		// Hardware address length, eth = 6
	uint8_t		hops;
	uint32_t	xid;		// Transaction ID
	uint16_t	secs;		// Seconds elapsed
	uint16_t	flags;		// Flags
	uint32_t	ciaddr;		// Client's existing IP address, if any
	uint32_t	yiaddr;		// "Your" (client) assigned IP address
	uint32_t	siaddr;		// IP address of next DHCP server
	uint32_t	giaddr;		// Relay agent IP address
	uint8_t		chaddr[16];	// Client hardware address
	char		sname[64];	// Server host name
} net_dhcphdr;


void net_init(void);
void net_rx(void *ethpkt, int len);

#endif // !PIOS_KERN_NET_H
#endif // LAB >= 2
