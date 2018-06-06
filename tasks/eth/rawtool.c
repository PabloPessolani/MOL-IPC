#define _GNU_SOURCE     
#define _MULTI_THREADED
#define  MOL_USERSPACE	1
//#define TASKDBG			1

#include "raw.h"

#define	IFNAME		"eth0"

int raw_mtu;
int raw_fd;
int raw_lpid;
int raw_mode;
char raw_src[IFNAMSIZ+1];
char raw_dst[IFNAMSIZ+1];
char *sframe_ptr;
char *rframe_ptr;
uint8_t src_mac[ETH_ALEN];
uint8_t dst_mac[ETH_ALEN];
static char src_string[3*ETH_ALEN+1];
static char dst_string[3*ETH_ALEN+1];
struct stat file_stat;
struct sockaddr_ll src_sockaddr;
struct sockaddr_ll dst_sockaddr;
struct ifreq if_idx;
struct ifreq if_mac;
struct ifreq if_mtu;
struct arpreq dst_areq;


#define RAW_NONE		(-1)
#define RAW_SERVER		0
#define RAW_CLIENT		1


// Define a struct for ARP header
struct _arp_hdr {
  uint16_t htype;
  uint16_t ptype;
  uint8_t hlen;
  uint8_t plen;
  uint16_t opcode;
  uint8_t sender_mac[ETH_ALEN];
  uint8_t sender_ip[4];
  uint8_t target_mac[ETH_ALEN];
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
#define ETHHDR_FORMAT	"ETH_HDR: dst=%02X:%02X:%02X:%02X:%02X:%02X src=%02X:%02X:%02X:%02X:%02X:%02X proto=%02X\n" 
#define ETHHDR_FIELDS(p) (p->dst.ea_addr[0]& 0xff),(p->dst.ea_addr[1]& 0xff), (p->dst.ea_addr[2]& 0xff), \
				(p->dst.ea_addr[3]& 0xff), (p->dst.ea_addr[4]& 0xff), (p->dst.ea_addr[5]& 0xff), \
				(p->src.ea_addr[0]& 0xff), (p->src.ea_addr[1]& 0xff), (p->src.ea_addr[2]& 0xff), \
				(p->src.ea_addr[3]& 0xff), (p->src.ea_addr[4] & 0xff), (p->src.ea_addr[5]& 0xff), \
				(p->proto& 0xff)

				
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

void usage (char *progname){
	fprintf (stderr,"usage: %s [-c|s] <rmt_node> <filename>\n", progname);
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
	
int arp_table()
{
    FILE *arp_fd;
static    char header[ARP_BUFFER_LEN];
static    char ipAddr[ARP_BUFFER_LEN];
static	  char hwAddr[ARP_BUFFER_LEN];
static	  char device[ARP_BUFFER_LEN];

    int count = 0;	
	arp_fd = fopen(ARP_CACHE, "r");
    if (!arp_fd) ERROR_EXIT(errno);

    /* Ignore the first line, which contains the header */
    if (!fgets(header, sizeof(header), arp_fd))
        ERROR_EXIT(errno);


    while (3 == fscanf(arp_fd, ARP_LINE_FORMAT, ipAddr, hwAddr, device))
    {
        printf("%03d: Mac Address of [%s] on [%s] is \"%s\"\n",
                ++count, ipAddr, device, hwAddr);
    }
    fclose(arp_fd);
    return 0;
}

static char *ethernet_mactoa(struct sockaddr *addr) {
    static char buff[256];
    unsigned char *ptr = (unsigned char *) addr->sa_data;

    sprintf(buff, "%02X:%02X:%02X:%02X:%02X:%02X",
        (ptr[0] & 0xff), (ptr[1] & 0xff), (ptr[2] & 0xff),
        (ptr[3] & 0xff), (ptr[4] & 0xff), (ptr[5] & 0xff));

    return (buff);
}

int get_arp(char *dst_ip_str)
{
    int dgram_sock;
    struct sockaddr_in *sin;
    struct in_addr ipaddr;
	
    /* Get an internet domain socket. */
    if ((dgram_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
		ERROR_EXIT(errno);
	
	
	if (inet_aton(dst_ip_str, &ipaddr) == 0) 
		ERROR_EXIT(errno);
	
	  /* Make the ARP request. */
    memset(&dst_areq, 0, sizeof(dst_areq));
    sin = (struct sockaddr_in *) &dst_areq.arp_pa;
    sin->sin_family = AF_INET;
	
	sin->sin_addr = ipaddr;
    sin = (struct sockaddr_in *) &dst_areq.arp_ha;
    sin->sin_family = ARPHRD_ETHER;

    strncpy(dst_areq.arp_dev, IFNAME , 15);
    if (ioctl(dgram_sock, SIOCGARP, (caddr_t) &dst_areq) == -1) 
		ERROR_EXIT(errno);

    printf("Destination IP:%s MAC:%s\n",
    inet_ntoa(((struct sockaddr_in *) &dst_areq.arp_pa)->sin_addr),
    ethernet_mactoa(&dst_areq.arp_ha));
	
	return(OK);
}
	
/*===========================================================================*
 *				   main 				    					 *
 *===========================================================================*/
int main (int argc, char *argv[] )
{
	_eth_hdr_t *eth_hdr_ptr;
	ether_type_t proto; 
	int pay_len, c, rcode, i, sframe_len, sent_bytes, rcvd_bytes;
	struct hostent *rmt_he;
	static char payload[] = "esto es un string de pruebas";
	extern char *optarg;
	extern int optind, optopt, opterr;
		
	if(argc != 4) usage(argv[0]);

	TASKDEBUG("Starting %s\n", argv[0]);

	raw_mode = RAW_NONE;
	while ((c = getopt(argc, argv, "c:s:")) != -1) {
		switch(c) {
			case 'c':
				if(raw_mode != RAW_NONE) choose(argv[0]);
				raw_mode = RAW_CLIENT;
				break;
			case 's':
				if(raw_mode != RAW_NONE) choose(argv[0]);
				raw_mode = RAW_SERVER;
				break;
			default:
				usage(argv[0]);
		}
	}
	
	TASKDEBUG("raw_mode=%d\n", raw_mode);
	if( optind >= argc ) usage(argv[0]);

	if(raw_mode == RAW_SERVER){
		rcode = stat(argv[3], &file_stat);
		if(rcode) bad_file(argv[0], argv[3]);
	}
		
	rmt_he = gethostbyname(argv[2]);
	
	printf("rmt_name=%s rmt_ip=%s\n", rmt_he->h_name, 
		inet_ntoa( *( struct in_addr*)(rmt_he->h_addr)));

	arp_table();
	
	get_arp(inet_ntoa( *( struct in_addr*)(rmt_he->h_addr)));
		
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
	
	// get socket for RAW
	raw_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_M3IPC));
	if (raw_fd == -1) ERROR_EXIT(errno);
		
	/* Get the index of the interface to send on */
	memset(&if_idx, 0, sizeof(struct ifreq));
	strncpy(if_idx.ifr_name, IFNAME, IFNAMSIZ-1);
	if (ioctl(raw_fd, SIOCGIFINDEX, &if_idx) < 0)
	    perror("SIOCGIFINDEX");
		
	/* Get SOURCE  interface MAC address  */
	memset(&if_mac,0,sizeof(if_mac));
	strncpy(if_mac.ifr_name, IFNAME, IFNAMSIZ-1);
	if (ioctl(raw_fd,SIOCGIFHWADDR,&if_mac) == -1) {
		ERROR_EXIT(errno);
	}

	memcpy(src_mac, &if_mac.ifr_hwaddr.sa_data, ETH_ALEN);
	for(i = 0; i < ETH_ALEN; i++){
		sprintf(&src_string[i*3], "%02X:", src_mac[i]);
	}
	src_string[(ETH_ALEN*3)-1] = 0;
	TASKDEBUG("%s src_mac %s\n",raw_src, src_string);


	/* Get DESTINATION  interface MAC address  */
	memcpy(dst_mac, dst_areq.arp_ha.sa_data, ETH_ALEN);
	for(i = 0; i < ETH_ALEN; i++){
		sprintf(&dst_string[i*3], "%02X:", dst_mac[i]);
	}
	dst_string[(ETH_ALEN*3)-1] = 0;
	TASKDEBUG("%s dst_mac %s\n",raw_dst, dst_string);
	

	if(raw_mode == RAW_SERVER){ 

		// ----------- FILL THE SOCKADDR_II -------------------

		/*RAW communication*/
		dst_sockaddr.sll_family   = AF_PACKET;	
		/*we don't use a protocoll above ethernet layer  ->just use anything here*/
		dst_sockaddr.sll_protocol = htons(ETH_M3IPC);	

		/*index of the network device */
		dst_sockaddr.sll_ifindex  = if_idx.ifr_ifindex;

		/*address length*/
		dst_sockaddr.sll_halen    = ETH_ALEN;		
		/*MAC - begin*/
		for(i = 0; i < ETH_ALEN; i++){
			dst_sockaddr.sll_addr[i]  = dst_mac[i];		
		}
		/*MAC - end*/
		dst_sockaddr.sll_addr[ETH_ALEN+1]  = 0x00;/*not used*/
		dst_sockaddr.sll_addr[ETH_ALEN+2]  = 0x00;/*not used*/
				
		// ----------- FILL THE FRAME -------------------
		// 	Destination MAC	: 
		memcpy(&sframe_ptr[0], &dst_mac[0], ETH_ALEN );

		// 	Source MAC 
		memcpy(&sframe_ptr[ETH_ALEN], &src_mac[0], ETH_ALEN);

		//   Protocolo Field
		proto = (ether_type_t) ETH_M3IPC; 
		memcpy(&sframe_ptr[2*ETH_ALEN], &proto  , sizeof(ether_type_t));

		// Payload
		pay_len = strlen(payload);
		memcpy(&sframe_ptr[ETH_HDRLEN], payload, pay_len );

		// print source and destination MAC ADDRESS
		eth_hdr_ptr = (_eth_hdr_t*)sframe_ptr;
		TASKDEBUG(ETHHDR_FORMAT,ETHHDR_FIELDS(eth_hdr_ptr));
			
		sframe_len = (ETH_HDRLEN+pay_len);
		TASKDEBUG("sframe_len=%d pay_len=%d\n", sframe_len, pay_len);
//		if(write(raw_fd, sframe_ptr,sframe_len) == -1) 
//			ERROR_EXIT(errno);

		sent_bytes = sendto(raw_fd, sframe_ptr, sframe_len, 0x00,
               (struct sockaddr*)&dst_sockaddr, sizeof(dst_sockaddr));	
		if( sent_bytes < 0) ERROR_EXIT(errno);
		TASKDEBUG("sent_bytes=%d\n", sent_bytes);
		
	}else{ // RAW_CLIENT 
		/* Bind to device */
//		if (setsockopt(raw_fd, SOL_SOCKET, SO_BINDTODEVICE, IFNAME, IFNAMSIZ-1) == -1)	
//			ERROR_EXIT(errno);
	

		memset(&src_sockaddr,0,sizeof(src_sockaddr));
		src_sockaddr.sll_family=PF_PACKET;
		src_sockaddr.sll_protocol=htons(ETH_P_ALL);
		src_sockaddr.sll_ifindex=if_idx.ifr_ifindex;
		if (bind(raw_fd,(struct sockaddr*)&src_sockaddr,sizeof(src_sockaddr))<0)
			ERROR_EXIT(errno);
	
		do {
			rcvd_bytes = recvfrom(raw_fd, rframe_ptr, ETH_FRAME_LEN, 0, NULL, NULL);
			if( rcvd_bytes < 0) ERROR_EXIT(errno);
			TASKDEBUG("rcvd_bytes=%d\n", rcvd_bytes);

			// print source and destination MAC ADDRESS
			eth_hdr_ptr = (_eth_hdr_t*)rframe_ptr;
			TASKDEBUG(ETHHDR_FORMAT,ETHHDR_FIELDS(eth_hdr_ptr));
			printf("payload:%s\n",&rframe_ptr[ETH_HDRLEN]);
		} while(1);
	}
	
	close(raw_fd);
	
	exit(0);
}

