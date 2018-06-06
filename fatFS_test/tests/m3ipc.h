#ifndef __M3IPC_HTTPD_H__
#define __M3IPC_HTTPD_H__

// #define LWIP_PROXY		1

#define _GNU_SOURCE
#define  __USE_GNU
#include <sched.h>
#define cpumask_t cpu_set_t

#include <unistd.h>
#include <sys/syscall.h>   /* For SYS_xxx definitions */
#include <pthread.h>
#include <string.h>
#include <stdio.h> /*Para las macros de SVRDEBUG, etc*/

#define _POSIX_SOURCE      1	/* tell headers to include POSIX stuff */
#define _MINIX             1	/* tell headers to include MINIX stuff */
#define _SYSTEM            1	/* tell headers that this is the kernel */

#include "/home/MoL_Module/mol-ipc/kernel/minix/config.h"
#include "/home/MoL_Module/mol-ipc/kernel/minix/const.h"
#include "/home/MoL_Module/mol-ipc/kernel/minix/types.h"
#include "/home/MoL_Module/mol-ipc/kernel/minix/timers.h"
#include "/home/MoL_Module/mol-ipc/kernel/minix/dc_usr.h"
#include "/home/MoL_Module/mol-ipc/kernel/minix/proc_usr.h"
#include "/home/MoL_Module/mol-ipc/kernel/minix/proc_sts.h"
#include "/home/MoL_Module/mol-ipc/kernel/minix/dvs_usr.h"
#include "/home/MoL_Module/mol-ipc/kernel/minix/com.h"
#include "/home/MoL_Module/mol-ipc/kernel/minix/ipc.h"
#include "/home/MoL_Module/mol-ipc/kernel/minix/kipc.h"
#include "/home/MoL_Module/mol-ipc/kernel/minix/molerrno.h"
#include "/home/MoL_Module/mol-ipc/kernel/minix/endpoint.h"
#include "/home/MoL_Module/mol-ipc/kernel/minix/ansi.h"
#include "/home/MoL_Module/mol-ipc/kernel/minix/priv.h"
#include "/home/MoL_Module/mol-ipc/kernel/minix/cmd.h"
#include "/home/MoL_Module/mol-ipc/kernel/minix/proxy_usr.h"
#include "/home/MoL_Module/mol-ipc/kernel/minix/proxy_sts.h"
#include "/home/MoL_Module/mol-ipc/kernel/minix/slots.h"
//#include "/home/MoL_Module/mol-ipc/kernel/minix/proxy.h"
#include "/home/MoL_Module/mol-ipc/kernel/minix/resource.h"
#include "/home/MoL_Module/mol-ipc/kernel/minix/signal.h"
#include "/home/MoL_Module/mol-ipc/kernel/minix/callnr.h"
#include "/home/MoL_Module/mol-ipc/kernel/minix/syslib.h"
#include "/home/MoL_Module/mol-ipc/kernel/minix/mollib.h"
#include "/home/MoL_Module/mol-ipc/kernel/minix/node_usr.h"
#include "/home/MoL_Module/mol-ipc/kernel/minix/mnx_stat.h"
#include "/home/MoL_Module/mol-ipc/kernel/minix/fcntl.h"
#include "/home/MoL_Module/mol-ipc/kernel/minix/unistd.h"

#include "/home/MoL_Module/mol-ipc/stub_syscall.h"
extern int h_errno;



#define WAIT4BIND_MS	4000

void m3ipc_httpd_init(void);
void m3ipc_proxy_init(void); 
void m3ipc_webfat_init(void); 

#if SVRDBG
 #define SVRDEBUG(text, args ...) \
 do { \
     printf(" %s:%s:%u:" \
             text ,__FILE__ ,__FUNCTION__ ,__LINE__, ## args); \
     fflush(stdout);\
 }while(0);
#else 
#define SVRDEBUG(x, args ...)
#endif 


#define ERROR_RETURN(rcode) \
 do { \
     	fprintf(stderr,"ERROR: %s:%s:%u: rcode=%d\n", __FILE__ ,__FUNCTION__ ,__LINE__,rcode); \
	fflush(stderr);\
	return(rcode);\
 }while(0)
	 
#define ERROR_EXIT(rcode) \
do { \
     	printf("ERROR: %s:%s:%u: rcode=%d\n",__FILE__ ,__FUNCTION__ ,__LINE__,rcode); \
	fflush(stderr);\
	exit(rcode); \
}while(0);

#define ERROR_PRINT(rcode) \
 do { \
     	fprintf(stderr,"ERROR: %s:%s:%u: rcode=%d\n", __FILE__ ,__FUNCTION__ ,__LINE__,rcode); \
	fflush(stderr);\
 }while(0)

 

#endif /* __M3IPC_HTTPD_H__ */
