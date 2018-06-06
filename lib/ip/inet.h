
/* This is the master header for PM.  It includes some other files
 * and defines the principal constants.
 */

#define _MULTI_THREADED
#define MOL_USERSPACE		1

#define MOL_CODE		1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <setjmp.h>
#include <pthread.h>
#include <fcntl.h>
#include <assert.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/sysinfo.h> 
#include <sys/stat.h> 

#include <netinet/in.h>       // IPPROTO_RAW, INET_ADDRSTRLEN
#include <netinet/ip.h>       // IP_MAXPACKET (which is 65535)
#include <arpa/inet.h>        // inet_pton() and inet_ntop()
#include <net/if.h>           // struct ifreq
#include <linux/if_ether.h>   // ETH_P_ARP = 0x0806
#include <linux/if_packet.h>  // struct sockaddr_ll (see man 7 packet)
#include <netinet/ip_icmp.h>  // struct icmp, ICMP_ECHO

//#include <net/ethernet.h>

#include "../../kernel/minix/config.h"
#include "../../kernel/minix/limits.h"
#include "../../kernel/minix/const.h"
#include "../../kernel/minix/types.h"
#include "../../kernel/minix/type.h"
#include "../../kernel/minix/timers.h"
#include "../../kernel/minix/dc_usr.h"
#include "../../kernel/minix/proc_usr.h"
#include "../../kernel/minix/proc_sts.h"
#include "../../kernel/minix/drvs_usr.h"
#include "../../kernel/minix/com.h"
#include "../../kernel/minix/ipc.h"
#include "../../kernel/minix/kipc.h"
#include "../../kernel/minix/molerrno.h"
#include "../../kernel/minix/endpoint.h"
#include "../../kernel/minix/ansi.h"
#include "../../kernel/minix/priv.h"
#include "../../kernel/minix/cmd.h"
#include "../../kernel/minix/proxy_usr.h"
#include "../../kernel/minix/proxy_sts.h"
#include "../../kernel/minix/resource.h"
#include "../../kernel/minix/signal.h"
#include "../../kernel/minix/callnr.h"
#include "../../kernel/minix/syslib.h"
#include "../../kernel/minix/slots.h"
#include "../../kernel/minix/ioctl.h"

#include "../../stub_syscall.h"

#include "../debug.h"
#include "../macros.h"

#include "../../kernel/minix/net/ioctl.h"
#include "../../kernel/minix/net/hton.h"
#include "../../kernel/minix/net/gen/in.h"
#include "../../kernel/minix/net/gen/ip_hdr.h"
#include "../../kernel/minix/net/gen/tcp.h"
#include "../../kernel/minix/net/gen/tcp_io.h"
#include "../../kernel/minix/net/gen/tcp_hdr.h"
#include "../../kernel/minix/net/gen/udp.h"
#include "../../kernel/minix/net/gen/udp_io.h"
#include "../../kernel/minix/net/gen/udp_hdr.h"
#include "../../kernel/minix/net/gen/icmp.h"
#include "../../kernel/minix/net/gen/icmp_hdr.h"
#include "../../kernel/minix/net/gen/ether.h"
#include "../../kernel/minix/net/gen/eth_hdr.h"
#include "../../kernel/minix/net/gen/psip_hdr.h"

#include "../../kernel/minix/net/gen/ip_io.h"
#include "../../kernel/minix/net/gen/eth_io.h"
#include "../../kernel/minix/net/gen/arp_io.h"
#include "../../kernel/minix/net/gen/psip_io.h"
#include "../../kernel/minix/net/gen/route.h"
#include "../../kernel/minix/net/gen/oneCsum.h"

#define _POSIX_SOURCE      1	/* tell headers to include POSIX stuff */
#define _MINIX             1	/* tell headers to include MINIX stuff */
#define _SYSTEM            1	/* tell headers that this is the kernel */

