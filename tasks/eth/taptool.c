#define _GNU_SOURCE     
#define _MULTI_THREADED
#define  MOL_USERSPACE	1
//#define TASKDBG			1

#include "tap.h"

#define	TAP0NAME		"tap0"
#define	TAP1NAME		"tap1"

char rbuf[ETH_FRAME_LEN];
int tap_mtu;
int tap_fd;
int tap_lpid;
int tap_mode;
char *tap_src[IFNAMSIZ+1];
char *tap_dst[IFNAMSIZ+1];
char *sframe_ptr;
char *rframe_ptr;
uint8_t src_mac[6];
uint8_t dst_mac[6];
static char src_string[3*ETH_ALEN+1];
static char dst_string[3*ETH_ALEN+1];
struct stat file_stat;
struct sockaddr_ll src_sockaddr;
struct sockaddr_ll dst_sockaddr;
struct ifreq if_idx;
struct ifreq if_mac;
struct ifreq if_mtu;


#define TAP_NONE		(-1)
#define TAP_SERVER		0
#define TAP_CLIENT		1


// Define a struct for ARP header
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
typedef struct _arp_hdr _arp_hdr_t;
#define ARP_HDRLEN 28      // ARP header length
#define ARPOP_REQUEST 1    // Taken from <linux/if_arp.h>
#define ARP_FORMAT "ARP_HDR: htype=%X ptype=%X hlen=%d plen=%d opcode=%d\n"
#define ARP_FIELDS(p) 	p->htype,p->ptype, p->hlen, p->plen, p->opcode

typedef struct _ip_hdr
{
	u8_t vers_ihl,
		tos;
	u16_t length,
		id,
		flags_fragoff;
	u8_t ttl,
		proto;
	u16_t hdr_chk;
	mnx_ipaddr_t src, dst;
} _ip_hdr_t;
#define IP4_HDRLEN 20      // IPv4 header length
#define IPHDR_FORMAT 		"IP_HDR: vers=%X tos=%X len=%X id=%X flag=%X ttl=%X proto=%X chk=%X src=%lX dst=%lX\n"
#define IPHDR_FIELDS(p) 	p->vers_ihl, p->tos, p->length, p->id, \
							p->flags_fragoff, p->ttl, p->proto, p->hdr_chk, \
							p->src, p->dst

typedef struct _eth_hdr
{
	mnx_ethaddr_t dst;
	mnx_ethaddr_t src;
	ether_type_t proto;
} _eth_hdr_t;
#define ETH_HDRLEN 14      // Ethernet header length
#define ETHHDR_FORMAT	"ETH_HDR: dst=%02X:%02X:%02X:%02X:%02X:%02X src=%02X:%02X:%02X:%02X:%02X:%02X proto=%04X\n" 
#define ETHHDR_FIELDS(p) p->dst.ea_addr[0],p->dst.ea_addr[1], p->dst.ea_addr[2], \
				p->dst.ea_addr[3], p->dst.ea_addr[4], p->dst.ea_addr[5], \
				p->src.ea_addr[0], p->src.ea_addr[1], p->src.ea_addr[2], \
				p->src.ea_addr[3], p->src.ea_addr[4], p->src.ea_addr[5], p->proto

				
typedef struct _icmp_hdr
{ 
	u8_t ih_type, ih_code;
	u16_t ih_chksum;
#ifdef FIELD_NOT_USED	
	union
	{
		u32_t ihh_unused;
		icmp_id_seq_t ihh_idseq;
		mnx_ipaddr_t ihh_gateway;
		icmp_ram_t ihh_ram;
		icmp_pp_t ihh_pp;
		icmp_mtu_t ihh_mtu;
	} ih_hun;
	union
	{
		icmp_ip_id_t ihd_ipid;
		u8_t uhd_data[1];
	} ih_dun;
#endif // FIELD_NOT_USED	
} _icmp_hdr_t;
#define ICMP_FORMAT		"ICMP_HDR: type=%X code=%X check=%X \n"
#define ICMP_FIELDS(p)	p->ih_type, p->ih_code, p->ih_chksum


/*===========================================================================*
 *                            low_level_probe                                     *
 *===========================================================================*/
int low_level_probe()
{
	int s, i;

	TASKDEBUG("%s\n", tap_src);

	s = socket(AF_PACKET,SOCK_RAW, htons(ETH_M3IPC));
	if (s == -1) ERROR_EXIT(errno);
		
	
	/* Get the index of the interface to send on */
	memset(&if_idx, 0, sizeof(struct ifreq));
	strncpy(if_idx.ifr_name, tap_src, IFNAMSIZ-1);
	if (ioctl(s, SIOCGIFINDEX, &if_idx) < 0)
	    perror("SIOCGIFINDEX");
		
	
	/* Get source interface MAC address  */
	memset(&if_mac,0,sizeof(if_mac));
	strcpy(if_mac.ifr_name,tap_src);
	if (ioctl(s,SIOCGIFHWADDR,&if_mac) == -1) {
		ERROR_EXIT(errno);
	}

	memcpy(src_mac, &if_mac.ifr_hwaddr.sa_data, 6);
	for(i = 0; i < ETH_ALEN; i++){
		sprintf(&src_string[i*3], "%02X:", src_mac[i]);
	}
	src_string[(ETH_ALEN*3)-1] = 0;
	TASKDEBUG("%s src_mac %s\n",tap_src, src_string);

	memcpy(dst_mac, &if_mac.ifr_hwaddr.sa_data, 6);
	for(i = 0; i < ETH_ALEN; i++){
		if(i == (ETH_ALEN-1)){
			if( tap_mode == TAP_CLIENT){
				dst_mac[i] = 0x00;
			} else{
				dst_mac[i] = 0x10;
			}
		}
		sprintf(&dst_string[i*3], "%02X:", dst_mac[i]);
	}
	dst_string[(ETH_ALEN*3)-1] = 0;
	TASKDEBUG("%s dst_mac %s\n",tap_dst, dst_string);
	
	/* Get source interface MTU */
//	if (ioctl(s,SIOCGIFMTU,&if_mtu) == -1) {
//		ERROR_EXIT(errno);
//	}
//	tap_mtu = if_mtu.ifr_mtu;
//	TASKDEBUG("%s MTU %d\n",tap_src, tap_mtu);
	
	close(s);
	return OK;
}

/*-----------------------------------------------------------------------------------*/
/*
 * low_level_input():
 *
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 */
/*-----------------------------------------------------------------------------------*/

static char *low_level_input(void)
{
	u16_t length;

	TASKDEBUG("tap_src=%s\n", tap_src);
	
	/* Obtain the size of the packet and put it into the "length"    variable. */
	length = read(tap_fd, rbuf, ETH_FRAME_LEN);
	TASKDEBUG("tap_src=%s Frame received: length=%d\n",tap_src, length);

	_ip_hdr_t *ip_hdr_ptr;
	_icmp_hdr_t *icmp_hdr_ptr;
	_arp_hdr_t *arp_hdr_ptr;
	_eth_hdr_t *eth_hdr_ptr;
	
	// print source and destination MAC ADDRESS
	eth_hdr_ptr = (_eth_hdr_t*)rbuf;
	TASKDEBUG(ETHHDR_FORMAT,ETHHDR_FIELDS(eth_hdr_ptr));		
	// print  IP header
	ip_hdr_ptr = (_ip_hdr_t *) (rbuf + ETH_HDRLEN);
	TASKDEBUG(IPHDR_FORMAT,IPHDR_FIELDS(ip_hdr_ptr));
	if(ip_hdr_ptr->proto == IPPROTO_ICMP){
		icmp_hdr_ptr = (_icmp_hdr_t*) (ip_hdr_ptr + IP4_HDRLEN);
		TASKDEBUG(ICMP_FORMAT,ICMP_FIELDS(icmp_hdr_ptr));
	}else{
		arp_hdr_ptr = (_arp_hdr_t *)(ip_hdr_ptr);
		if(arp_hdr_ptr->opcode == htons (ARPOP_REQUEST)){
			TASKDEBUG(ARP_FORMAT,ARP_FIELDS(arp_hdr_ptr));
		}
	}

	return(rbuf);
}

/*===========================================================================*
 *				tap_init					     *
 *===========================================================================*/
static void tap_init(void)
{
	int rcode;
	struct ifreq ifr;

	tap_lpid = getpid();
	
	TASKDEBUG("tap_lpid=%d\n", tap_lpid);

	// memory for frame to SEND
	posix_memalign( (void **) &sframe_ptr, getpagesize(), ETH_MAX_PACK_SIZE );
	if (sframe_ptr == NULL) {
		fprintf(stderr, "posix_memalign sframe_ptr \n");
		ERROR_EXIT(errno);
	}
	
	// memory for frame to RECEIVE
	posix_memalign( (void **) &rframe_ptr, getpagesize(), ETH_MAX_PACK_SIZE );
	if (rframe_ptr == NULL) {
		fprintf(stderr, "posix_memalign rframe_ptr \n");
		ERROR_EXIT(errno);
	}
	
	tap_fd = open(DEVTAP, O_RDWR);
	if( tap_fd < 0){
		ERROR_EXIT(errno);
	}
		
	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name,tap_src);
	ifr.ifr_flags = IFF_TAP|IFF_NO_PI;
	if (ioctl(tap_fd, TUNSETIFF, (void *) &ifr) < 0) {
		ERROR_EXIT(errno);
	}
	
	if ( (rcode = low_level_probe() != OK))
		ERROR_EXIT(rcode);
	
}

void usage (char *progname){
	fprintf (stderr,"usage: %s [-c|s] <filename>\n", progname);
	exit(1);
}

void choose (char *progname){
	fprintf (stderr, "%s: you must choose Client(c) or Server(s) flag\n", progname);
	exit(1);
}

void bad_file (char *progname, char *filename){
	fprintf (stderr, "%s: bad file name %s\n", progname, filename);
	exit(1);
}
	
/*===========================================================================*
 *				   main 				    					 *
 *===========================================================================*/
int main (int argc, char *argv[] )
{
	_eth_hdr_t *eth_hdr_ptr;
	ether_type_t proto; 
	int pay_len, c, rcode, i, sframe_len, sent_bytes, rcvd_bytes;
	static char payload[] = "esto es un string de pruebas";
	extern char *optarg;
	extern int optind, optopt, opterr;
		
	if(argc != 3) usage(argv[0]);

	TASKDEBUG("Starting %s\n", argv[0]);

	tap_mode = TAP_NONE;
	while ((c = getopt(argc, argv, "c:s:")) != -1) {
		switch(c) {
			case 'c':
				if(tap_mode != TAP_NONE) choose(argv[0]);
				tap_mode = TAP_CLIENT;
				break;
			case 's':
				if(tap_mode != TAP_NONE) choose(argv[0]);
				tap_mode = TAP_SERVER;
				break;
			default:
				usage(argv[0]);
		}
	}
	
	if( argv[optind] != NULL) usage(argv[0]);

	sprintf(tap_src,"tap%d", tap_mode);
	sprintf(tap_dst,"tap%d", (1-tap_mode));
	TASKDEBUG("Tap tap_src=%s tap_dst=%s tap_mode=%X\n", 
		tap_src, tap_dst, tap_mode);

	if(tap_mode == TAP_SERVER){
		rcode = stat(argv[2], &file_stat);
		if(rcode) bad_file(argv[0], argv[2]);
	}
		
	/* Initialize TAP */
	tap_init();

	// ----------- FILL THE SOCKADDR_II -------------------

	/*RAW communication*/
	dst_sockaddr.sll_family   = PF_PACKET;	
	/*we don't use a protocoll above ethernet layer  ->just use anything here*/
	dst_sockaddr.sll_protocol = htons(ETH_M3IPC);	

	/*index of the network device */
	dst_sockaddr.sll_ifindex  = if_idx.ifr_ifindex;

	/*ARP hardware identifier is ethernet*/
//		dst_sockaddr.sll_hatype   = ARPHRD_ETHER;
		
	/*target is another host*/
//		dst_sockaddr.sll_pkttype  = PACKET_OTHERHOST;

	/*address length*/
	dst_sockaddr.sll_halen    = ETH_ALEN;		
	/*MAC - begin*/
	for(i = 0; i < 6; i++){
		dst_sockaddr.sll_addr[i]  = dst_mac[i];		
	}
	/*MAC - end*/
	dst_sockaddr.sll_addr[6]  = 0x00;/*not used*/
	dst_sockaddr.sll_addr[7]  = 0x00;/*not used*/
		
	if(tap_mode == TAP_SERVER){ 
	
		// ----------- FILL THE FRAME -------------------
		// 	Destination MAC	: 
		memcpy(&sframe_ptr[0], &dst_mac[0], 6 );

		// 	Source MAC 
		memcpy(&sframe_ptr[6], &src_mac[0], 6);

		//   Protocolo Field
		proto = (ether_type_t) ETH_M3IPC; 
		memcpy(&sframe_ptr[12], &proto  , sizeof(ether_type_t));

		// Payload
		pay_len = strlen(payload);
		memcpy(&sframe_ptr[12+sizeof(ether_type_t)], payload, pay_len );

		// print source and destination MAC ADDRESS
		eth_hdr_ptr = (_eth_hdr_t*)sframe_ptr;
		TASKDEBUG(ETHHDR_FORMAT,ETHHDR_FIELDS(eth_hdr_ptr));
			
		sframe_len = (ETH_HDRLEN+pay_len);
		TASKDEBUG("sframe_len=%d pay_len=%d\n", sframe_len, pay_len);
		sent_bytes = write(tap_fd, sframe_ptr,sframe_len);
		if( sent_bytes < 0) ERROR_EXIT(errno);  
		TASKDEBUG("sent_bytes=%d\n", sent_bytes);
		
	}else{ // TAP_CLIENT 
		do {
			rcvd_bytes = recvfrom(tap_fd, rframe_ptr, ETH_FRAME_LEN, 0, NULL, NULL);
			if( rcvd_bytes < 0) ERROR_EXIT(errno);
			TASKDEBUG("rcvd_bytes=%d\n", rcvd_bytes);

			// print source and destination MAC ADDRESS
			eth_hdr_ptr = (_eth_hdr_t*)rframe_ptr;
			TASKDEBUG(ETHHDR_FORMAT,ETHHDR_FIELDS(eth_hdr_ptr));
			printf("payload:%s\n",&rframe_ptr[ETH_HDRLEN]);
		} while(1);
	}
	
	close(tap_fd);
	
	exit(0);
}

