/*	syslib.h - System library common definitions.	*/

#define _MULTI_THREADED

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

#include <sys/types.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/sysinfo.h> 

#include <netinet/in.h>

#include <arpa/inet.h>

#include <sys/stat.h> 


#include "../../kernel/minix/config.h"
#include "../../kernel/minix/types.h"
#include "../../kernel/minix/const.h"
#include "../../kernel/minix/timers.h"

#include "../../kernel/minix/vm_usr.h"
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
#include "../../kernel/minix/proxy.h"
#include "../../kernel/minix/resource.h"
#include "../../kernel/minix/signal.h"
#include "../../kernel/minix/callnr.h"
#include "../../kernel/minix/syslib.h"

#include "../../stub_syscall.h"

#include "../debug.h"
#include "../macros.h"


