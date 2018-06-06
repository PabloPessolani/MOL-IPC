#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <setjmp.h>
#include <pthread.h>
#include <sched.h>
#include <assert.h>
#include <netdb.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/sysinfo.h> 
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <malloc.h>

#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if_tun.h>
//#include <netinet/ether.h>
#include <netinet/ip.h>       // IP_MAXPACKET (which is 65535)
#include <linux/if_ether.h>   // ETH_P_ARP = 0x0806
#include <linux/if_packet.h>  // struct sockaddr_ll (see man 7 packet)
#include <netinet/ip_icmp.h>  // struct icmp, ICMP_ECHO

#include "../../kernel/minix/config.h"
#include "../../kernel/minix/const.h"
#include "../../kernel/minix/types.h"
#include "../../kernel/minix/timers.h"
#include "../../kernel/minix/type.h"
#include "../../kernel/minix/ipc.h"
#include "../../kernel/minix/kipc.h"
#include "../../kernel/minix/syslib.h"
#include "../../kernel/minix/dvs_usr.h"
#include "../../kernel/minix/dc_usr.h"
#include "../../kernel/minix/node_usr.h"
#include "../../kernel/minix/proc_usr.h"
#include "../../kernel/minix/proc_sts.h"
#include "../../kernel/minix/com.h"
#include "../../kernel/minix/cmd.h"
#include "../../kernel/minix/molerrno.h"
#include "../../kernel/minix/endpoint.h"
#include "../../kernel/minix/resource.h"
#include "../../kernel/minix/callnr.h"
#include "../../kernel/minix/ansi.h"
#include "../../kernel/minix/priv.h"
#include "../../kernel/minix/devio.h"
#include "../../kernel/minix/mollib.h"
#include "../../kernel/minix/net/gen/ether.h"
#include "../../kernel/minix/net/gen/eth_io.h"
#include "../../kernel/minix/net/gen/in.h"

//#include "../const.h"
#include "../debug.h" 
#include "../macros.h"

// #define ETH_FRAME_LEN           1518
#define Address           unsigned long

//http://stackoverflow.com/questions/3366812/linux-raw-ethernet-socket-bind-to-specific-protocol
//http://lxr.free-electrons.com/source/include/uapi/linux/if_ether.h#L49
//http://opensourceforu.com/2015/03/a-guide-to-using-raw-sockets/
#define ETH_M3IPC 		0xFD00 
#define IP_M3IPC 		0x1234


#define xstr(s) str(s)
#define str(s) #s

#define ARP_CACHE       "/proc/net/arp"
#define ARP_STRING_LEN  1023
#define ARP_BUFFER_LEN  (ARP_STRING_LEN + 1)

/* Format for fscanf() to read the 1st, 4th, and 6th space-delimited fields */
#define ARP_LINE_FORMAT "%" xstr(ARP_STRING_LEN) "s %*s %*s " \
                        "%" xstr(ARP_STRING_LEN) "s %*s " \
                        "%" xstr(ARP_STRING_LEN) "s"



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

#define PAYLOAD_LEN  (ETH_FRAME_LEN - sizeof(cmd_t) - sizeof(_eth_hdr_t))
typedef union eth_frame_u{ 
	u8_t raw[ETH_FRAME_LEN+1];
	struct {
		_eth_hdr_t	hdr;
		cmd_t		cmd;
		u8_t		pay[PAYLOAD_LEN];
	}fmt;
} eth_frame_t;

#define RAW_MAX_RETRIES		3 
#define RAW_ACK_RATE		4
#define RAW_RCV_TIMEOUT		10 

enum raw_cmd {
//        RAW_NONE      =  0,	/* NO COMMAND  					*/
		RAW_GETFILE = 1,	/* Client -> server GET file				*/
		RAW_PUTFILE = 2,		/* Client -> Server PUT file 				*/
		RAW_DATA    = 3,			/*  Frame contains data				*/
		RAW_EXIT    = 4,
  };
  
#define RAW_ACKNOWLEDGE	0x0010

// FLAGS 
enum raw_flags {
		RAW_EOF		= 1,
		RAW_EOB		= 2,		/* Client -> Server PUT file 				*/
		RAW_RESEND  = 4,
		RAW_NEEDACK = 8, 
  };

  

#define c_snd_off c_snd_seq
#define c_ack_off c_ack_seq


//http://stackoverflow.com/questions/13547721/udp-socket-set-timeout
//http://stackoverflow.com/questions/12713438/how-to-add-delay-to-sento-and-recvfrom-in-udp-client-server-in-c
//http://stackoverflow.com/questions/393276/socket-with-recv-timeout-what-is-wrong-with-this-code
//http://stackoverflow.com/questions/28244082/proper-use-of-getsockopt-and-setsockopt-for-so-rcvtimeo-and-so-sndtimeo
//http://www.linuxquestions.org/questions/programming-9/setsockopt-so_rcvtimeo-468572/
// https://cboard.cprogramming.com/networking-device-communication/128469-non-blocking-socket-timeout.html

