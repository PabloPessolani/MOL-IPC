/*For m_dtab*/
#define OPER_NAME 0
#define OPER_OPEN 1
#define OPER_NOP 2
#define OPER_IOCTL 3
#define OPER_PREPARE 4
#define OPER_TRANSF 5
#define OPER_CLEAN 6
#define OPER_GEOM 7
#define OPER_SIG 8
#define OPER_ALARM 9
#define OPER_CANC 10
#define OPER_SEL 11

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
#include "../../kernel/minix/types.h"
#include "../../kernel/minix/const.h"
#include "../../kernel/minix/timers.h"
#include "../../kernel/minix/type.h"
#include "../../kernel/minix/ipc.h"
#include "../../kernel/minix/kipc.h"
#include "../../kernel/minix/syslib.h"
//#include "../../kernel/minix/u64.h"
#include "../../kernel/minix/partition.h"

#include "../../kernel/minix/drvs_usr.h"
#include "../../kernel/minix/vm_usr.h"
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
#include "../../stub_syscall.h"


#include "glo.h"

#include "super.h" //AGREGUÉ NR_SUPERS =1
#include "../const.h"
#include "../debug.h" 
#include "../macros.h"

#include "proto.h"


#include "../libdriver/driver.h"







