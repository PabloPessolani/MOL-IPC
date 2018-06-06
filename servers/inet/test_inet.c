#include "inet.h"

int local_nodeid;
u16_t _tmp;

#define WAIT4BIND_MS 1000

#define ETH_MINOR 	0
#define IP_MINOR	1
#define TCP_MINOR	2
#define UDP_MINOR	3

// Define some constants.
#define ETH_HDRLEN 14      // Ethernet header length
#define IP4_HDRLEN 20      // IPv4 header length
#define ARP_HDRLEN 28      // ARP header length
#define ARPOP_REQUEST 1    // Taken from <linux/if_arp.h>
#define LCL_TAP0_IP		"172.16.1.4"	
#define RMT_TAP0_IP		"172.16.1.9"	
#define BR0_TAP0_IP		"172.16.1.3"	
#define MASK_TAP0_IP	"255.255.255.0"	


// Define a struct for ARP header
typedef struct _arp_hdr arp_hdr;
struct _arp_hdr {
  uint16_t htype;
  uint16_t ptype;
  uint8_t hlen;
  uint8_t plen;
  uint16_t opcode;
  uint8_t sender_mac[6];
  uint8_t sender_ip[4];
  uint8_t target_mac[6];
  uint8_t target_ip[4];
};
#define ARP_FORMAT "htype=%X ptype=%X hlen=%d plen=%d opcode=%d\n"
#define ARP_FIELDS(p) 	p->htype,p->ptype, p->hlen, p->plen, p->opcode

char *mnx_inet_ntoa(mnx_ipaddr_t *in)
{
	static char b[18];
	register u8_t *p;

	p = (u8_t *)in;
	sprintf(b, "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
	return (b);
}

/*===========================================================================*
 *				main					     *
 *===========================================================================*/
void  main ( int argc, char *argv[] )
{
	int rcode, lpid;
	message m __attribute__((aligned(0x1000)));
	message *m_ptr;

	SVRDEBUG("TEST_INET\n");
	m_ptr = &m;
	
	lpid = getpid();
	SVRDEBUG("TEST_INET lpid=%d\n",lpid);

	// ************************ OPEN IP **************************
	
	SVRDEBUG("TEST OPEN\n");

	m_ptr->m_type	= NW_OPEN;
	m_ptr->DEVICE	= IP_MINOR;
	m_ptr->IO_ENDPT	= FS_PROC_NR;
	m_ptr->COUNT	= O_RDWR;

	sleep(1);
	SVRDEBUG("OPEN " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	rcode = mnx_sendrec(INET_PROC_NR, m_ptr);
	if(rcode) 
		SVRDEBUG("OPEN rcode=%d\n", rcode);
	SVRDEBUG("OPEN " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	
	
	// ************************ IOCTL SET IP CONFIG  **************************
	struct nwio_ipconf ipconf;
	int minor_ip;

	minor_ip = m_ptr->m2_i2; // IP_MINOR returned by OPEN ;
	SVRDEBUG("TEST SET IP CONFIG minor_ip=%d\n", minor_ip);
	
	ipconf.nwic_flags= (NWIC_IPADDR_SET | NWIC_NETMASK_SET |  NWIC_MTU_SET);
	ipconf.nwic_ipaddr	= inet_addr(LCL_TAP0_IP);
	ipconf.nwic_netmask = inet_addr(MASK_TAP0_IP);
	ipconf.nwic_mtu = 1490;

	m_ptr->m_type	= NW_IOCTL;
	m_ptr->DEVICE	= minor_ip;
	m_ptr->IO_ENDPT	= FS_PROC_NR;
	m_ptr->POSITION = NWIOSIPCONF;
	m_ptr->ADDRESS	= (char *) &ipconf;
	m_ptr->COUNT	= 0L;

	sleep(1);
	SVRDEBUG("NWIOSIPCONF " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	rcode = mnx_sendrec(INET_PROC_NR, m_ptr);
	if(rcode) 
		SVRDEBUG("NWIOSIPCONF rcode=%d\n", rcode);
	SVRDEBUG("NWIOSIPCONF " MSG2_FORMAT, MSG2_FIELDS(m_ptr));

	SVRDEBUG("NWIOSIPCONF ipaddr=%08lX flags=%X netmask=%08lX mtu=%d\n", 
			ipconf.nwic_ipaddr,
			ipconf.nwic_flags,
			ipconf.nwic_netmask,
			ipconf.nwic_mtu);
	SVRDEBUG("NWIOSIPCONF ipaddr=%s\n", mnx_inet_ntoa(&ipconf.nwic_ipaddr));
	SVRDEBUG("NWIOSIPCONF netmask=%s\n", mnx_inet_ntoa(&ipconf.nwic_netmask));	
	
	
	// ************************ IOCTL GET IP CONFIG  **************************

	SVRDEBUG("TEST GET IP CONFIG\n");

	ipconf.nwic_ipaddr	= 0;
	
	m_ptr->m_type	= NW_IOCTL;
	m_ptr->DEVICE	= minor_ip;
	m_ptr->IO_ENDPT	= FS_PROC_NR;
	m_ptr->POSITION = NWIOGIPCONF;
	m_ptr->ADDRESS	= (char *) &ipconf;
	m_ptr->COUNT	= 0L;

	sleep(1);
	SVRDEBUG("NWIOGIPCONF " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	rcode = mnx_sendrec(INET_PROC_NR, m_ptr);
	if(rcode) 
		SVRDEBUG("NWIOGIPCONF rcode=%d\n", rcode);
	SVRDEBUG("NWIOGIPCONF " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	SVRDEBUG("NWIOGIPCONF ipaddr=%08lX flags=%X netmask=%08lX mtu=%d\n", 
			ipconf.nwic_ipaddr,
			ipconf.nwic_flags,
			ipconf.nwic_netmask,
			ipconf.nwic_mtu);
	SVRDEBUG("NWIOGIPCONF ipaddr=%s\n", mnx_inet_ntoa(&ipconf.nwic_ipaddr));
	SVRDEBUG("NWIOGIPCONF netmask=%s\n", mnx_inet_ntoa(&ipconf.nwic_netmask));	
	
	// ************************ OPEN ETH  **************************
	
	SVRDEBUG("TEST ETH\n");

	m_ptr->m_type	= NW_OPEN;
	m_ptr->DEVICE	= ETH_MINOR;
	m_ptr->IO_ENDPT	= FS_PROC_NR;
	m_ptr->COUNT	= O_RDWR;

	sleep(1);
	SVRDEBUG("OPEN " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	rcode = mnx_sendrec(INET_PROC_NR, m_ptr);
	if(rcode) 
		SVRDEBUG("OPEN rcode=%d\n", rcode);
	SVRDEBUG("OPEN " MSG2_FORMAT, MSG2_FIELDS(m_ptr));

		// ************************ SET ETH OPT  **************************
	int minor_eth;
	static	nwio_ethopt_t 	nw_opt;
	nwio_ethopt_t  *opt_ptr;

	minor_eth = m_ptr->m2_i2; // ETH_MINOR returned by OPEN ;
	
	SVRDEBUG("SET ETH OPT minor_eth=%d\n", minor_eth);

	opt_ptr  = &nw_opt;
	opt_ptr->nweo_flags = (NWEO_COPY | NWEO_EN_LOC | NWEO_EN_BROAD
			| NWEO_REMANY | NWEO_TYPEANY | NWEO_RWDATALL);

	m_ptr->m_type	= NW_IOCTL;
	m_ptr->DEVICE	= minor_eth;
	m_ptr->IO_ENDPT	= FS_PROC_NR;
	m_ptr->POSITION = NWIOSETHOPT;
	m_ptr->ADDRESS	= (char *) &nw_opt;
	m_ptr->COUNT	= 0L;

	sleep(1);
	SVRDEBUG("NWIOSETHOPT " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	rcode = mnx_sendrec(INET_PROC_NR, m_ptr);
	if(rcode) 
		SVRDEBUG("NWIOSETHOPT rcode=%d\n", rcode);
	SVRDEBUG("NWIOSETHOPT " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	
	// ************************ IOCTL GET ETH STAT  **************************

static 	nwio_ethstat_t ethstat;
	nwio_ethstat_t *nw_ptr;

	nw_ptr = &ethstat;
	SVRDEBUG("TEST GET NWIOGETHSTAT minor_eth=%d\n", minor_eth);

	ipconf.nwic_ipaddr	= 0;
	
	m_ptr->m_type	= NW_IOCTL;
	m_ptr->DEVICE	= minor_eth;
	m_ptr->IO_ENDPT	= FS_PROC_NR;
	m_ptr->POSITION = NWIOGETHSTAT;
	m_ptr->ADDRESS	= (char *) &ethstat;
	m_ptr->COUNT	= 0L;

	sleep(1);
	SVRDEBUG("NWIOGETHSTAT " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	rcode = mnx_sendrec(INET_PROC_NR, m_ptr);
	if(rcode) 
		SVRDEBUG("NWIOGETHSTAT rcode=%d\n", rcode);
	SVRDEBUG("NWIOGETHSTAT " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	SVRDEBUG("NWIOGETHSTAT " NWES_FORMAT, NWES_FIELDS(nw_ptr));
			
	// ************************ ARP REQUEST  **************************
	char *frame_ptr;
	arp_hdr arphdr;
	int frame_length;
	arp_hdr *ah_ptr; 


	SVRDEBUG("TEST ARP minor_eth=%d\n", minor_eth);
	posix_memalign( (void **) &frame_ptr, getpagesize(), ETH_MAX_PACK_SIZE );
	if (frame_ptr == NULL) {
		fprintf(stderr, "posix_memalign\n");
	}
	
	// 	Destination MAC	: BROADCAST
	memset (&frame_ptr[0], 0xFF, 6 * sizeof (uint8_t));
	
	// 	Source MAC - It will be  overwritten by interface task
	memset (&frame_ptr[6], 0x00, 6 * sizeof (uint8_t));
	
	// Hardware type (16 bits): 1 for ethernet
	arphdr.htype = htons (1);

	// Protocol type (16 bits): 2048 for IP
	arphdr.ptype = htons (ETH_P_IP);

	// Hardware address length (8 bits): 6 bytes for MAC address
	arphdr.hlen = 6;

	// Protocol address length (8 bits): 4 bytes for IPv4 address
	arphdr.plen = 4;

	// OpCode: 1 for ARP request
	arphdr.opcode = htons (ARPOP_REQUEST);

	// Sender hardware address (48 bits): MAC address
	memcpy (&arphdr.sender_mac, &nw_ptr->nwes_addr.ea_addr, 6 * sizeof (uint8_t));

	// Sender protocol address (32 bits)
	// See getaddrinfo() resolution of src_ip.
	rcode = inet_pton(AF_INET, LCL_TAP0_IP, &arphdr.sender_ip);
	if(rcode <= 0){
		ERROR_PRINT(rcode);
	}
	
	// Target hardware address (48 bits): zero, since we don't know it yet.
	memset (&arphdr.target_mac, 0, 6 * sizeof (uint8_t));

	// Target protocol address (32 bits)
	// See getaddrinfo() resolution of target.
	rcode = inet_pton(AF_INET, BR0_TAP0_IP, &arphdr.target_ip);
	if(rcode <= 0){
		ERROR_PRINT(rcode);
	}
	
	// Fill out ethernet frame header.

	// Ethernet frame length = ethernet header (MAC + MAC + ethernet type) + ethernet data (ARP header)
	frame_length = 6 + 6 + 2 + ARP_HDRLEN;
	if( frame_length < ETH_MIN_PACK_SIZE) 
		frame_length = ETH_MIN_PACK_SIZE;
	
	
	// ARP header
	memcpy(frame_ptr + ETH_HDRLEN, &arphdr, ARP_HDRLEN * sizeof (uint8_t));
	
	// Next is ethernet type code (ETH_P_ARP for ARP).
	// http://www.iana.org/assignments/ethernet-numbers
	frame_ptr[12] = ETH_P_ARP / 256;
	frame_ptr[13] = ETH_P_ARP % 256;

	m_ptr->m_type	= NW_WRITE;
	m_ptr->DEVICE	= minor_eth;
	m_ptr->IO_ENDPT	= FS_PROC_NR;
	m_ptr->ADDRESS	= (char *) frame_ptr;
	m_ptr->COUNT	= frame_length;

	sleep(1);
	SVRDEBUG("TEST ARP " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	rcode = mnx_sendrec(INET_PROC_NR, m_ptr);
	if(rcode) 
		SVRDEBUG("TEST ARP rcode=%d\n", rcode);
	SVRDEBUG("TEST ARP " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	ah_ptr = &arphdr;
	SVRDEBUG("TEST ARP " ARP_FORMAT, ARP_FIELDS(ah_ptr));
	
	// ************************ ETH READ  **************************

	m_ptr->m_type	= NW_READ;
	m_ptr->DEVICE	= minor_eth;
	m_ptr->IO_ENDPT	= FS_PROC_NR;
	m_ptr->ADDRESS	= (char *) frame_ptr;
	m_ptr->COUNT	= ETH_MAX_PACK_SIZE;

	sleep(1);
	SVRDEBUG("ETH READ1 " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	rcode = mnx_sendrec(INET_PROC_NR, m_ptr);
	if(rcode) 
		SVRDEBUG("ETH READ1 rcode=%d\n", rcode);
	SVRDEBUG("ETH READ1 " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	ah_ptr = (arp_hdr*) (frame_ptr + ETH_HDRLEN);
	SVRDEBUG("ETH READ1 " ARP_FORMAT, ARP_FIELDS(ah_ptr));
	
		// ************************ ETH READ2  **************************

	m_ptr->m_type	= NW_READ;
	m_ptr->DEVICE	= minor_eth;
	m_ptr->IO_ENDPT	= FS_PROC_NR;
	m_ptr->ADDRESS	= (char *) frame_ptr;
	m_ptr->COUNT	= ETH_MAX_PACK_SIZE;

	sleep(1);
	SVRDEBUG("ETH READ2 " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	rcode = mnx_sendrec(INET_PROC_NR, m_ptr);
	if(rcode) 
		SVRDEBUG("ETH READ2 rcode=%d\n", rcode);
	SVRDEBUG("ETH READ2 " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	ah_ptr = (arp_hdr*) (frame_ptr + ETH_HDRLEN);
	SVRDEBUG("ETH READ2 " ARP_FORMAT, ARP_FIELDS(ah_ptr));
	
	// ************************ PING  **************************

	#define WRITE_SIZE 30
	mnx_ip_hdr_t *ip_hdr;
	icmp_hdr_t *icmp_hdr;
	nwio_ipopt_t ipopt, *ipo_ptr;
		
	SVRDEBUG("PING TEST \n");

	ipo_ptr = &ipopt;
	ipo_ptr->nwio_flags= NWIO_COPY | NWIO_PROTOSPEC;
	ipo_ptr->nwio_proto= 1;

// 	result= ioctl (fd, NWIOSIPOPT, &ipopt);
	m_ptr->m_type	= NW_IOCTL;
	m_ptr->DEVICE	= minor_ip;
	m_ptr->IO_ENDPT	= FS_PROC_NR;
	m_ptr->POSITION = NWIOSIPOPT;
	m_ptr->ADDRESS	= (char *) &ipopt;
	m_ptr->COUNT	= 0L;
	sleep(1);
	SVRDEBUG("NWIOSIPOPT " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	rcode = mnx_sendrec(INET_PROC_NR, m_ptr);
	if(rcode) 
		SVRDEBUG("NWIOSIPOPT rcode=%d\n", rcode);
	SVRDEBUG("NWIOSIPOPT " MSG2_FORMAT, MSG2_FIELDS(m_ptr))

	SVRDEBUG("NWIOSIPOPT " IPOPT_FORMAT, IPOPT_FIELDS(ipo_ptr));
	
// 	result= ioctl (fd, NWIOGIPOPT, &ipopt);
	memset(&ipopt, 0, sizeof(nwio_ipopt_t));
	
	m_ptr->m_type	= NW_IOCTL;
	m_ptr->DEVICE	= minor_ip;
	m_ptr->IO_ENDPT	= FS_PROC_NR;
	m_ptr->POSITION = NWIOGIPOPT;
	m_ptr->ADDRESS	= (char *) &ipopt;
	m_ptr->COUNT	= 0L;
	sleep(1);
	SVRDEBUG("NWIOGIPOPT " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	rcode = mnx_sendrec(INET_PROC_NR, m_ptr);
	if(rcode) 
		SVRDEBUG("NWIOGIPOPT rcode=%d\n", rcode);
	SVRDEBUG("NWIOGIPOPT " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	
	SVRDEBUG("NWIOGIPOPT " IPOPT_FORMAT, IPOPT_FIELDS(ipo_ptr));
	
		
#define PING_TEST 1	
#ifdef PING_TEST	
	ip_hdr= (mnx_ip_hdr_t *)frame_ptr;
	ip_hdr->ih_dst= inet_addr(RMT_TAP0_IP);

	icmp_hdr= (icmp_hdr_t *)(frame_ptr+20);
	icmp_hdr->ih_type= 8;
	icmp_hdr->ih_code= 0;
	icmp_hdr->ih_chksum= 0;
	icmp_hdr->ih_chksum= ~oneC_sum(0, (u16_t *)icmp_hdr,
		WRITE_SIZE-20);
		
//	result= write(fd, buffer, length);
	m_ptr->m_type	= NW_WRITE;
	m_ptr->DEVICE	= minor_ip;
	m_ptr->IO_ENDPT	= FS_PROC_NR;
	m_ptr->ADDRESS	= (char *) frame_ptr;
	m_ptr->COUNT	= WRITE_SIZE;

	SVRDEBUG("TEST PING WRITE " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	rcode = mnx_sendrec(INET_PROC_NR, m_ptr);
	if(rcode) 
		SVRDEBUG("TEST PING WRITE rcode=%d\n", rcode);
	SVRDEBUG("TEST PING WRITE " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	
	sleep(1);
	m_ptr->m_type	= NW_READ;
	m_ptr->DEVICE	= minor_ip;
	m_ptr->IO_ENDPT	= FS_PROC_NR;
	m_ptr->ADDRESS	= (char *) frame_ptr;
	m_ptr->COUNT	= ETH_MAX_PACK_SIZE;

	SVRDEBUG("TEST PING READ " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	rcode = mnx_sendrec(INET_PROC_NR, m_ptr);
	if(rcode) 
		SVRDEBUG("TEST PING rcode=%d\n", rcode);
	SVRDEBUG("TEST PING READ " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	
#endif  // PING_TEST	
	
	// **************************************************
	while(1)
		sleep(20);
	
 }


	
