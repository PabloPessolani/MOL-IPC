/*	this file contains the interface of the network software with rest of
	minix. Furthermore it contains the main loop of the network task.

Copyright 1995 Philip Homburg

The valid messages and their parameters are:

from FS:
 __________________________________________________________________
|		|           |         |       |          |         |
| m_type		|   DEVICE  | PROC_NR |	COUNT |	POSITION | ADDRESS |
|_______________|___________|_________|_______|__________|_________|
|		|           |         |       |          |         |
| NW_OPEN 	| minor dev | proc nr | mode  |          |         |
|_______________|___________|_________|_______|__________|_________|
|		|           |         |       |          |         |
| NW_CLOSE 	| minor dev | proc nr |       |          |         |
|_______________|___________|_________|_______|__________|_________|
|		|           |         |       |          |         |
| NW_IOCTL	| minor dev | proc nr |       |	NWIO..	 | address |
|_______________|___________|_________|_______|__________|_________|
|		|           |         |       |          |         |
| NW_READ	| minor dev | proc nr |	count |          | address |
|_______________|___________|_________|_______|__________|_________|
|		|           |         |       |          |         |
| NW_WRITE	| minor dev | proc nr |	count |          | address |
|_______________|___________|_________|_______|__________|_________|
|		|           |         |       |          |         |
| NW_CANCEL	| minor dev | proc nr |       |          |         |
|_______________|___________|_________|_______|__________|_________|

from DL_ETH:
 _______________________________________________________________________
|		|           |         |          |            |         |
| m_type	|  DL_PORT  | DL_PROC |	DL_COUNT |  DL_STAT   | DL_TIME |
|_______________|___________|_________|__________|____________|_________|
|		|           |         |          |            |         |
| DL_INIT_REPLY	| minor dev | proc nr | rd_count |  0  | stat |  time   |
|_______________|___________|_________|__________|____________|_________|
|		|           |         |          |            |         |
| DL_TASK_REPLY	| minor dev | proc nr | rd_count | err | stat |  time   |
|_______________|___________|_________|__________|____________|_________|
*/
#define _MINIX_SOURCE 1
#define _TABLE	1

#include "inet.h"

#define TRUE 1
#define WAIT4BIND_MS 1000

#define LCL_TAP0_IP		"172.16.1.4"	
#define RMT_TAP0_IP		"172.16.1.9"	
#define DST_MAC			"72:89:78:FF:88:EF"

void nw_conf(void);
void nw_init(void);
void inet_init(void);
void nw_test_arp(void);
void nw_test_icmp(void);

int main(int argc,  char *argv[])
{
	mq_t *mq;
	int rcode, ret, source;
	u8_t randbits[32];
	struct timeval tv;
	message *m_ptr;
	
	SVRDEBUG("Starting INET on p_nr=%d\n", INET_PROC_NR);

	/* Initialize INET */
	inet_init();
	
	/* Read configuration. */
	nw_conf();
	
	/* Get a random number */
	SVRDEBUG("using current time for random-number seed\n");
	ret= gettimeofday(&tv, NULL);
	if (ret == -1 )	{
		fprintf(stderr," sysutime failed: %s\n", strerror(errno));
		fflush(stderr);
		ERROR_EXIT(rcode);
	}
	memcpy(randbits, &tv, sizeof(randbits));

	init_rand256(randbits);

	/* Our new identity as a server. */
	if ((this_proc = mnx_getep(getpid())) < 0)
		ip_panic(( "unable to get own process nr\n"));
	SVRDEBUG("this_proc=%d\n", this_proc);

#ifdef ANULADO
	/*WARNING, ONLY FOR TESTS */
	while(TRUE){
    	ret = mnx_receive(ANY, (long) m_ptr);
		SVRDEBUG("" MSG1_FORMAT, MSG1_FIELDS(m_ptr));
		m_ptr->m_type = OK;
    	ret = mnx_send(m_ptr->m_source, (long) m_ptr);		
	}

	/* Register the device group. */
	device.dev= ip_dev;
	device.style= STYLE_CLONE;
	
	if (svrctl(FSSIGNON, (void *) &device) == -1) {
		printf("inet: error %d on registering ethernet devices\n",
			errno);
		pause();
	}
#endif /* ANULADO */

	nw_init();
	
// ONLY FOR TESTS 
//	nw_test_arp();
//	nw_test_icmp();
	
	while (TRUE){
		// any event to process ?
		if (ev_head){
			ev_process();
			continue;
		}
		// any timeout expired?
		if (clck_call_expire){
			clck_expire_timers();
			continue;
		}
		// alloc a message queue entry
		mq= mq_get();
		if (!mq)
			ip_panic(("out of messages"));

		// get a message and store in message queue
		SVRDEBUG("mnx_receive\n");
		rcode = mnx_receive (ANY, &mq->mq_mess);
		if (rcode<0){
			ip_panic(("unable to receive: %d", rcode));
		}
		m_ptr = &mq->mq_mess;
		SVRDEBUG(MSG2_FORMAT, MSG2_FIELDS(m_ptr));

		reset_time();
		source = mq->mq_mess.m_source;
		if (source == FS_PROC_NR){
			SVRDEBUG("message from FS_PROC_NR\n");
			sr_rec(mq);
		}else if (mq->mq_mess.m_type == SYN_ALARM){
			clck_tick(&mq->mq_mess);
			mq_free(mq);
		} else if (mq->mq_mess.m_type == PROC_EVENT){
			/* signaled */ 
			/* probably SIGTERM */
			mq_free(mq);
		} else if (mq->mq_mess.m_type & NOTIFY_MESSAGE){
			/* A driver is (re)started. */
			eth_check_drivers(&mq->mq_mess);
			mq_free(mq);
		}else{
			compare(mq->mq_mess.m_type, ==, DL_TASK_REPLY);
			eth_rec(&mq->mq_mess);
			mq_free(mq);
		}
	}
	ip_panic(("task is not allowed to terminate"));	
}

/*===========================================================================*
 *				inet_init					     *
 *===========================================================================*/
void inet_init(void)
{
  	int rcode;

	inet_lpid = getpid();

	do { 
		rcode = mnx_wait4bind_T(WAIT4BIND_MS);
		SVRDEBUG("INET: mnx_wait4bind_T  rcode=%d\n", rcode);
		if (rcode == EMOLTIMEDOUT) {
			SVRDEBUG("INET: mnx_wait4bind_T TIMEOUT\n");
			continue ;
		}else if( rcode < 0) 
			ERROR_EXIT(EXIT_FAILURE);
	} while	(rcode < OK);
	
	SVRDEBUG("Get the DVS info from SYSTASK\n");
    rcode = sys_getkinfo(&dvs);
	if(rcode) ERROR_EXIT(rcode);
	dvs_ptr = &dvs;
	SVRDEBUG(DVS_USR_FORMAT,DVS_USR_FIELDS(dvs_ptr));

	SVRDEBUG("Get the DC info from SYSTASK\n");
	rcode = sys_getmachine(&dcu);
	if(rcode) ERROR_EXIT(rcode);
	dc_ptr = &dcu;
	SVRDEBUG(DC_USR_FORMAT,DC_USR_FIELDS(dc_ptr));

	SVRDEBUG("Get ETH_PROC_NR info from SYSTASK\n");
	rcode = sys_getproc(&eth, ETH_PROC_NR);
	if(rcode) ERROR_EXIT(rcode);
	eth_ptr = &eth;
	SVRDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(eth_ptr));
	if( TEST_BIT(eth_ptr->p_rts_flags, BIT_SLOT_FREE)) {
		fprintf(stderr,"ETH task not started\n");
		fflush(stderr);		
		ERROR_EXIT(EMOLNOTBIND);
	}
	
	SVRDEBUG("Get INET_PROC_NR info from SYSTASK\n");
	rcode = sys_getproc(&inet, SELF);
	if(rcode) ERROR_EXIT(rcode);
	inet_ptr = &inet;
	SVRDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(inet_ptr));
	
	/*---------------- Allocate memory for message  ---------------*/
	posix_memalign( (void **) &m_ptr, getpagesize(), sizeof(message) );
	if (m_ptr== NULL) {
   		ERROR_EXIT(errno);
	}
	SVRDEBUG("INET m_ptr=%p\n",m_ptr);
	
	/* Fetch clock ticks */
	clockTicks = sysconf(_SC_CLK_TCK);
	if (clockTicks == -1)	ERROR_EXIT(errno);
	SVRDEBUG("clockTicks =%ld\n",clockTicks );

	if ( (boottime = mol_time(&boottime)) < 0) 
  		ERROR_EXIT(rcode);
	boottime /= clockTicks;
	SVRDEBUG("boottime =%ld\n",boottime );

	if(rcode) ERROR_EXIT(rcode);
	
}


void nw_conf(void)
{
	SVRDEBUG("\n");

	read_conf();
	eth_prep();
	arp_prep();
	psip_prep();
	ip_prep();
	tcp_prep();
	udp_prep();
}

void nw_init(void)
{
	SVRDEBUG("\n");

	mq_init();
	qp_init();
	bf_init();
	clck_init();
	sr_init();
	eth_init();
	arp_init();
	psip_init();
	ip_init();
	tcp_init();
	udp_init();
}

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

/********************************************************************/
/*			nw_test_arp								*/
/********************************************************************/
void nw_test_arp(void)
{
	message msg __attribute__((aligned(0x1000)));
	message *m_ptr;
	char *frame_ptr;
	eth_port_t *eth_port;
	arp_hdr arphdr;
	int rcode,frame_length;
	
// Define some constants.
#define ETH_HDRLEN 14      // Ethernet header length
#define IP4_HDRLEN 20      // IPv4 header length
#define ARP_HDRLEN 28      // ARP header length
#define ARPOP_REQUEST 1    // Taken from <linux/if_arp.h>

	SVRDEBUG("\n");
	posix_memalign( (void **) &frame_ptr, getpagesize(), ETH_MAX_PACK_SIZE );
	if (frame_ptr == NULL) {
		fprintf(stderr, "posix_memalign\n");
	}

// 	Destination MAC	: BROADCAST
	memset (&frame_ptr[0], 0xFF, 6 * sizeof (uint8_t));
	
// 	Source MAC - It will be  overwritten by interface task
	memset (&frame_ptr[6], 0x00, 6 * sizeof (uint8_t));
	
	eth_port= &eth_port_table[0];
	
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
	memcpy (&arphdr.sender_mac, &eth_port->etp_ethaddr, 6 * sizeof (uint8_t));

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
	rcode = inet_pton(AF_INET, RMT_TAP0_IP, &arphdr.target_ip);
	if(rcode <= 0){
		ERROR_PRINT(rcode);
	}
	
	// Fill out ethernet frame header.

	// Ethernet frame length = ethernet header (MAC + MAC + ethernet type) + ethernet data (ARP header)
	frame_length = 6 + 6 + 2 + ARP_HDRLEN;

	// ARP header
	memcpy(frame_ptr + ETH_HDRLEN, &arphdr, ARP_HDRLEN * sizeof (uint8_t));
	
	// Next is ethernet type code (ETH_P_ARP for ARP).
	// http://www.iana.org/assignments/ethernet-numbers
	frame_ptr[12] = ETH_P_ARP / 256;
	frame_ptr[13] = ETH_P_ARP % 256;

	m_ptr = &msg;
	m_ptr->m_type = DL_WRITE;
	m_ptr->DL_PORT = 0;
	m_ptr->DL_PROC = INET_PROC_NR;
	m_ptr->DL_COUNT = frame_length;
	m_ptr->DL_MODE = DL_NOMODE;
	m_ptr->DL_ADDR = frame_ptr;
	
	SVRDEBUG("request: " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	rcode = mnx_sendrec(ETH_PROC_NR, m_ptr);
	if(rcode != 0){
		ERROR_PRINT(rcode);
	}
	SVRDEBUG("reply:  " MSG2_FORMAT, MSG2_FIELDS(m_ptr));

	
}

// Allocate memory for an array of unsigned chars.
uint8_t *allocate_ustrmem(int len)
{
	void *tmp;

	if (len <= 0) {
		ERROR_EXIT(EXIT_FAILURE);
	}

	posix_memalign ( (void **) &tmp, getpagesize(),len * sizeof (uint8_t));
	if (tmp == NULL) {
		ERROR_EXIT(EXIT_FAILURE);
	}
	
	memset (tmp, 0, len * sizeof (uint8_t));
    return (tmp);
}

// Allocate memory for an array of .
char *allocate_mem (int len, int size)
{
  void *tmp;

	if (len <= 0) {
		ERROR_EXIT(EXIT_FAILURE);
	}

	posix_memalign ( (void **) &tmp, getpagesize(),(len * size));
	if (tmp == NULL) {
		ERROR_EXIT(EXIT_FAILURE);
	}
    memset (tmp, 0, len * size);
    return (tmp);
}

// Computing the internet checksum (RFC 1071).
// Note that the internet checksum does not preclude collisions.
uint16_t checksum (uint16_t *addr, int len)
{
  int count = len;
  register uint32_t sum = 0;
  uint16_t answer = 0;

  // Sum up 2-byte values until none or only one byte left.
  while (count > 1) {
    sum += *(addr++);
    count -= 2;
  }

  // Add left-over byte, if any.
  if (count > 0) {
    sum += *(uint8_t *) addr;
  }

  // Fold 32-bit sum into 16 bits; we lose information by doing this,
  // increasing the chances of a collision.
  // sum = (lower 16 bits) + (upper 16 bits shifted right 16 bits)
  while (sum >> 16) {
    sum = (sum & 0xffff) + (sum >> 16);
  }

  // Checksum is one's compliment of sum.
  answer = ~sum;

  return (answer);
}

/********************************************************************/
/*			nw_test_icmp								*/
/********************************************************************/
void nw_test_icmp(void)
{
	message msg __attribute__((aligned(0x1000)));
	message *m_ptr;
	int rcode, datalen, sd, *ip_flags;
	char *interface, *src_ip, *dst_ip;
	struct ip iphdr;
	struct icmp icmphdr;
	uint8_t *data, *packet, *frame;
	uint8_t dst_mac[6];
	uint16_t *ui_ptr;

	
	// Define some constants.
#define ETH_HDRLEN 14      // Ethernet header length
#define IP4_HDRLEN 20      // IPv4 header length
#define ICMP_HDRLEN 8         // ICMP header length for echo request, excludes data
	

	SVRDEBUG("\n");

	// Allocate memory for various arrays.
	data 	= allocate_mem (IP_MAXPACKET,sizeof (uint8_t));
	packet 	= allocate_mem (IP_MAXPACKET,sizeof (uint8_t));
	frame 	= allocate_mem (ETH_MAX_PACK_SIZE,sizeof (uint8_t));
	interface = allocate_mem (40,sizeof (char));
	src_ip 	= allocate_mem (INET_ADDRSTRLEN,sizeof (char));
	dst_ip	= allocate_mem (INET_ADDRSTRLEN,sizeof (char));
	ip_flags = allocate_mem (4,sizeof (int));
	
	// Source IPv4 address: you need to fill this out
	strcpy (src_ip, LCL_TAP0_IP);

	// Destination URL or IPv4 address: you need to fill this out
	strcpy (dst_ip, RMT_TAP0_IP);

	sscanf(DST_MAC, "%x:%x:%x:%x:%x:%x",
		&dst_mac[0],
		&dst_mac[1],
		&dst_mac[2],
		&dst_mac[3],
		&dst_mac[4],
		&dst_mac[5]);
	SVRDEBUG("dst_mac %02X:%02X:%02X:%02X:%02X:%02X \n",
		dst_mac[0],
		dst_mac[1],
		dst_mac[2],
		dst_mac[3],
		dst_mac[4],
		dst_mac[5]);	
	
	// ICMP data
	datalen = 4;
	data[0] = 'T';
	data[1] = 'e';
	data[2] = 's';
	data[3] = 't';

	// IPv4 header

	// IPv4 header length (4 bits): Number of 32-bit words in header = 5
	iphdr.ip_hl = IP4_HDRLEN / sizeof (uint32_t);

	// Internet Protocol version (4 bits): IPv4
	iphdr.ip_v = 4;

	// Type of service (8 bits)
	iphdr.ip_tos = 0;

	// Total length of datagram (16 bits): IP header + ICMP header + ICMP data
	iphdr.ip_len = htons (IP4_HDRLEN + ICMP_HDRLEN + datalen);

	// ID sequence number (16 bits): unused, since single datagram
	iphdr.ip_id = htons (0);

	// Flags, and Fragmentation offset (3, 13 bits): 0 since single datagram

	// Zero (1 bit)
	ip_flags[0] = 0;

	// Do not fragment flag (1 bit)
	ip_flags[1] = 0;

	// More fragments following flag (1 bit)
	ip_flags[2] = 0;

	// Fragmentation offset (13 bits)
	ip_flags[3] = 0;

	iphdr.ip_off = htons ((ip_flags[0] << 15)
                      + (ip_flags[1] << 14)
                      + (ip_flags[2] << 13)
                      +  ip_flags[3]);

	// Time-to-Live (8 bits): default to maximum value
	iphdr.ip_ttl = 255;

	// Transport layer protocol (8 bits): 1 for ICMP
	iphdr.ip_p = IPPROTO_ICMP;

	// Source IPv4 address (32 bits)
	if ((rcode = inet_pton (AF_INET, src_ip, &(iphdr.ip_src))) != 1) {
		ERROR_EXIT(EXIT_FAILURE);
	}

	// Destination IPv4 address (32 bits)
	if ((rcode = inet_pton (AF_INET, dst_ip, &(iphdr.ip_dst))) != 1) {
		ERROR_EXIT(EXIT_FAILURE);
	}

	// IPv4 header checksum (16 bits): set to 0 when calculating checksum
	iphdr.ip_sum = 0;
	iphdr.ip_sum = checksum ((uint16_t *) &iphdr, IP4_HDRLEN);

	// ICMP header

	// Message Type (8 bits): echo request
	icmphdr.icmp_type = ICMP_ECHO;

	// Message Code (8 bits): echo request
	icmphdr.icmp_code = 0;

	// Identifier (16 bits): usually pid of sending process - pick a number
	icmphdr.icmp_id = htons (1000);

	// Sequence Number (16 bits): starts at 0
	icmphdr.icmp_seq = htons (0);

	// ICMP header checksum (16 bits): set to 0 when calculating checksum
	icmphdr.icmp_cksum = 0;

	// Prepare packet.

	// First part is an IPv4 header.
	memcpy (packet, &iphdr, IP4_HDRLEN);

	// Next part of packet is upper layer protocol header.
	memcpy ((packet + IP4_HDRLEN), &icmphdr, ICMP_HDRLEN);

	// Finally, add the ICMP data.
	memcpy (packet + IP4_HDRLEN + ICMP_HDRLEN, data, datalen);

	// Calculate ICMP header checksum
	icmphdr.icmp_cksum = checksum ((uint16_t *) (packet + IP4_HDRLEN), ICMP_HDRLEN + datalen);
	memcpy ((packet + IP4_HDRLEN), &icmphdr, ICMP_HDRLEN);

	// Build the Ethernet Frame
	memcpy(&frame[0],dst_mac,ETH_ALEN); /* destintation MAC */
	memset(&frame[6],0x00,ETH_ALEN);	 /* source MAC will be filled by Ethernet driver */	
	ui_ptr = (uint16_t*)&frame[2*ETH_ALEN]; 
	*ui_ptr = htons(ETH_P_IP);
	memcpy(&frame[ETH_HDRLEN],packet, (IP4_HDRLEN + ICMP_HDRLEN + datalen));


	m_ptr = &msg;
	m_ptr->m_type = DL_WRITE;
	m_ptr->DL_PORT = 0;
	m_ptr->DL_PROC = INET_PROC_NR;
	m_ptr->DL_COUNT = ETH_HDRLEN + IP4_HDRLEN + ICMP_HDRLEN + datalen;
	m_ptr->DL_MODE = DL_NOMODE;
	m_ptr->DL_ADDR = frame;
	SVRDEBUG("DL_COUNT=%d  ETH_HDRLEN=%d IP4_HDRLEN=%d ICMP_HDRLEN=%d datalen=%d\n",
		m_ptr->DL_COUNT,ETH_HDRLEN,IP4_HDRLEN,ICMP_HDRLEN,datalen);
	
	SVRDEBUG("INET request: " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	rcode = mnx_sendrec(ETH_PROC_NR, m_ptr);
	
	free (data);
	free (packet);
	free (interface);
	free (src_ip);
	free (dst_ip);
	free (ip_flags);
  
	if(rcode != 0){
		ERROR_PRINT(rcode);
	}
	SVRDEBUG("INET reply:  " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	
}


void panic0(char *file, int line)
{
	fprintf(stderr,"panic0:%s:%d", file, line);
	ERROR_EXIT(NO_NUM);
}

void inet_panic(void)
{
    fprintf(stderr,"ERROR: INET PANIC\n");
	fflush(stderr);
	ERROR_EXIT(NO_NUM);
}

#if !NDEBUG
void bad_assertion(char *file, int line, char *what)
{
	
    fprintf(stderr,"assertion:%s:%d \"%s\" failed", file, line, what);
	fflush(stderr);
	ERROR_EXIT(NO_NUM);
}


void bad_compare(char *file,int line,int lhs,char *what,int rhs)
{
    fprintf(stderr,"compare (%d) %s (%d) failed", lhs, what, rhs);
	fflush(stderr);
	ERROR_EXIT(NO_NUM);
}
#endif /* !NDEBUG */

