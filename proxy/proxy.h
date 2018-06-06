


#define _MULTI_THREADED
#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <signal.h>
#include <setjmp.h>
#include <pthread.h>
#include <fcntl.h>
#include <semaphore.h>
#include <netdb.h>
 
#include <sys/types.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/sysinfo.h> 
#include <sys/sem.h>
#include <sys/stat.h> 
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <sys/syscall.h>   /* For SYS_xxx definitions */
#include <sys/ioctl.h>
	   	   
#include <net/if.h>
#include <net/if_arp.h>
		   
//TIPC include
#include <linux/tipc.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <linux/if_tun.h>
//#include <netinet/ether.h>
#include <netinet/ip.h>       // IP_MAXPACKET (which is 65535)
#include <linux/if_ether.h>   // ETH_P_ARP = 0x0806
#include <linux/if_packet.h>  // struct sockaddr_ll (see man 7 packet)
#include <netinet/ip_icmp.h>  // struct icmp, ICMP_ECHO

#define _GNU_SOURCE
#define  __USE_GNU
#include <sched.h>
#define cpumask_t cpu_set_t

#include "../kernel/minix/config.h"
#include "../kernel/minix/const.h"
#include "../kernel/minix/types.h"
#include "../kernel/minix/timers.h"

#include "../kernel/minix/dc_usr.h"
#include "../kernel/minix/proc_usr.h"
#include "../kernel/minix/proc_sts.h"
#include "../kernel/minix/dvs_usr.h"
#include "../kernel/minix/com.h"
#include "../kernel/minix/ipc.h"
#include "../kernel/minix/kipc.h"
#include "../kernel/minix/molerrno.h"
#include "../kernel/minix/endpoint.h"
#include "../kernel/minix/ansi.h"
#include "../kernel/minix/priv.h"
#include "../kernel/minix/cmd.h"
#include "../kernel/minix/proxy_usr.h"
#include "../kernel/minix/proxy_sts.h"
#include "../kernel/minix/slots.h"
//#include "../kernel/minix/proxy.h"
#include "../kernel/minix/resource.h"
#include "../kernel/minix/signal.h"
#include "../kernel/minix/callnr.h"
#include "../kernel/minix/syslib.h"
#include "../kernel/minix/mollib.h"
#include "../kernel/minix/node_usr.h"
#include "../kernel/minix/net/gen/ether.h"
#include "../kernel/minix/net/gen/eth_io.h"
#include "../kernel/minix/net/gen/in.h"

#ifndef __cplusplus
#include "../stub_syscall.h"
#else
extern "C" {
#include "./stub4cpp.h"	
}
#endif

extern int h_errno;
#define _POSIX_SOURCE      1	/* tell headers to include POSIX stuff */
#define _MINIX             1	/* tell headers to include MINIX stuff */
#define _SYSTEM            1	/* tell headers that this is the kernel */

