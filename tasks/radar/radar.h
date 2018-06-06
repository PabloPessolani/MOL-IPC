

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

#include <sys/types.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/sysinfo.h> 
#include <sys/stat.h>
#include <fcntl.h>
#include <malloc.h>

#include <netinet/in.h>
#include <arpa/inet.h>

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
#include "../../kernel/minix/molerrno.h"
#include "../../kernel/minix/endpoint.h"
#include "../../kernel/minix/resource.h"
#include "../../kernel/minix/callnr.h"
#include "../../kernel/minix/ansi.h"
#include "../../kernel/minix/priv.h"
#include "../../stub_syscall.h"

#define BUFF_SIZE		MAXCOPYBUF
#define MAX_MESSLEN     (BUFF_SIZE+1024)
#define MAX_VSSETS      10
#define MAX_MEMBERS     NR_NODES

/*Spread message: message m3-ipc, transfer data*/
typedef struct {message msg; unsigned buffer_data[BUFF_SIZE]} SP_message;	 

#include <sp.h>
#include "glo.h"

//#include "../const.h"
#include "../debug.h" 
#include "../macros.h"

/* MULTICAST MESSAGE TYPES */
#define		STS_DISCONNECTED	-1
#define 	RADAR_MULTICAST		0x80	
#define 	MC_STATUS_INFO     (RADAR_MULTICAST + 1)
#define 	MC_SYNCHRONIZED    (RADAR_MULTICAST + 2)

#define		STS_SYNCHRONIZED	0
#define	    STS_NEW				1
#define		STS_WAIT4PRIMARY	2
#define		STS_WAIT4SYNC		3
#define		STS_LEAVE			4

#define NO_PRIMARY			(-1)

#define RADAR_TIMEOUT_SEC	5
#define RADAR_TIMEOUT_MSEC	0
#define RADAR_ERROR_SPEEP	5







