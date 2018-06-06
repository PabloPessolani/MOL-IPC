/* This is the master header for MOLLIB TESTS.  It includes some other files
 * and defines the principal constants.
 */

#define _MULTI_THREADED
#define _GNU_SOURCE 

#define MOL_USERSPACE	1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <setjmp.h>
#include <pthread.h>
// #include <fcntl.h>
#include <stdarg.h> 
#include <asm/ptrace.h>
//#include <utime.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/sysinfo.h> 

#include <netinet/in.h>

#include <arpa/inet.h>


#include "../stub_syscall.h"
#include "../kernel/minix/config.h"
#include "../kernel/minix/limits.h"
#include "../kernel/minix/const.h"
#include "../kernel/minix/types.h"
#include "../kernel/minix/type.h"
#include "../kernel/minix/timers.h"
#include "../kernel/minix/utime.h"
#include "../kernel/minix/dc_usr.h"
#include "../kernel/minix/proc_usr.h"
#include "../kernel/minix/proc_sts.h"
#include "../kernel/minix/drvs_usr.h"
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
#include "../kernel/minix/resource.h"
#include "../kernel/minix/mollib.h"
#include "../kernel/minix/signal.h"
#include "../kernel/minix/callnr.h"
#include "../kernel/minix/syslib.h"
#include "../kernel/minix/slots.h"
#include "../kernel/minix/ioctl.h"
#include "../kernel/minix/fcntl.h"
#include "../kernel/minix/dir.h"
#include "../kernel/minix/dirent.h"
#include "../kernel/minix/mnx_stat.h"
#include "../kernel/minix/statfs.h"
#include "../kernel/minix/unistd.h"

#include "../kernel/minix/net/ioctl.h"
#include "../kernel/minix/net/hton.h"
#include "../kernel/minix/net/gen/in.h"
#include "../kernel/minix/net/gen/ip_hdr.h"
#include "../kernel/minix/net/gen/tcp.h"
#include "../kernel/minix/net/gen/tcp_io.h"
#include "../kernel/minix/net/gen/tcp_hdr.h"
#include "../kernel/minix/net/gen/udp.h"
#include "../kernel/minix/net/gen/udp_io.h"
#include "../kernel/minix/net/gen/udp_hdr.h"
#include "../kernel/minix/net/gen/icmp.h"
#include "../kernel/minix/net/gen/icmp_hdr.h"
#include "../kernel/minix/net/gen/ether.h"
#include "../kernel/minix/net/gen/eth_hdr.h"
#include "../kernel/minix/net/gen/psip_hdr.h"
#include "../kernel/minix/net/gen/ip_io.h"
#include "../kernel/minix/net/gen/eth_io.h"
#include "../kernel/minix/net/gen/arp_io.h"
#include "../kernel/minix/net/gen/psip_io.h"
#include "../kernel/minix/net/gen/route.h"
#include "../kernel/minix/net/gen/oneCsum.h"

#include "../servers/debug.h"
#include "../servers/macros.h"
// #include "limits.h"

