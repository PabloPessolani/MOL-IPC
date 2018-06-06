
#define _MULTI_THREADED
#define _GNU_SOURCE     
#define  MOL_USERSPACE	1
#define _MINIX

#define _XOPEN_SOURCE 600 
#define VERSION 23

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
#include <fcntl.h>
#include <malloc.h>
//#include <termios.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/sysinfo.h> 
#include <sys/stat.h>
/* According to POSIX.1-2001 */
#include <sys/select.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include "../../kernel/minix/config.h"
#include "../../kernel/minix/const.h"
#include "../../kernel/minix/types.h"
#include "../../kernel/minix/timers.h"
#include "../../kernel/minix/limits.h"
#include "../../kernel/minix/type.h"
#include "../../kernel/minix/ipc.h"
#include "../../kernel/minix/kipc.h"
#include "../../kernel/minix/syslib.h"
//#include "../../kernel/minix/u64.h"
#include "../../kernel/minix/dvs_usr.h"
#include "../../kernel/minix/dc_usr.h"
#include "../../kernel/minix/node_usr.h"
#include "../../kernel/minix/proc_usr.h"
#include "../../kernel/minix/proc_sts.h"
#include "../../kernel/minix/com.h"
//#include "../../kernel/minix/proxy_usr.h"
#include "../../kernel/minix/molerrno.h"
#include "../../kernel/minix/endpoint.h"
#include "../../kernel/minix/resource.h"
#include "../../kernel/minix/callnr.h"
#include "../../kernel/minix/ansi.h"
#include "../../kernel/minix/priv.h"
#include "../../kernel/minix/dmap.h"
#include "../../kernel/minix/termios.h"
#include "../../kernel/minix/configfile.h"
#include "../../kernel/minix/ioc_tty.h"
#include "../../kernel/minix/timers.h"
#include "../../kernel/minix/select.h"
#include "../../stub_syscall.h"
#include "../debug.h"
#include "../macros.h"

#include "./webcommon.h"

#include <getopt.h>

struct web_s {
   char *svr_name;		// web server name - from config file
   struct sockaddr_in svr_addr; // pseudo server address/port
   int   svr_ep;		// server endpoint 
   char *rootdir;		// root directory for this server
   char inbuf[WEBMAXBUF];// input buffer  
   char outbuf[WEBMAXBUF];// output buffer  
};

typedef struct web_s web_t;
#define WEB_FORMAT "svr_name=%s svr_ep=%d rootdir=%s\n"
#define WEB_FIELDS(p) p->svr_name,p->svr_ep, p->rootdir

struct sess_s {	
	int clt_ep;
	int clt_port;
	int status;
	int file_fd;
	int next;
	long filelen;
	long outtotal;
	char *pathname;
	web_t *w_ptr;
};
typedef struct sess_s sess_t;
#define SESS_FORMAT "clt_ep=%d clt_port=%d status=%X file_fd=%d next=%d filelen=%ld outtotal=%ld\n"
#define SESS_FIELDS(p) p->clt_ep,p->clt_port, p->status, p->file_fd, p->next, p->filelen, p->outtotal 

#define EXIT_CODE			1
#define NEXT_CODE			2
#define nil ((void*)0)

#define 	NR_WEBSRVS		10
#define		NR_SESSIONS		100

#define		STS_CLOSED		0	// session not used
#define		STS_WAIT4CONN	1
#define		STS_WAIT4WRITE	2	// client is connected, server is waiting for GET request
#define		STS_WAIT4READ	3	// client has requested GET, now is reading its input socket
#define		STS_FILEREAD		4	// not the first read from client

#include "glo.h"

#define MTX_LOCK(x) do{ \
		CMDDEBUG("MTX_LOCK %s \n", #x);\
		pthread_mutex_lock(&x);\
		}while(0)
			
#define MTX_UNLOCK(x) do{ \
		pthread_mutex_unlock(&x);\
		CMDDEBUG("MTX_UNLOCK %s \n", #x);\
		}while(0)	
			
#define COND_WAIT(x,y) do{ \
		CMDDEBUG("COND_WAIT %s %s\n", #x,#y );\
		pthread_cond_wait(&x, &y);\
		}while(0)	

#define COND_SIGNAL(x) do{ \
		pthread_cond_signal(&x);\
		CMDDEBUG("COND_SIGNAL %s\n", #x);\
		}while(0)	
		


