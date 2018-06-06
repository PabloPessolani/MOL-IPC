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
#include <sys/types.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/sysinfo.h> 
#include <sys/stat.h>
#include <sys/msg.h>
#include <sys/wait.h> 
#include <fcntl.h>
#include <syslog.h> 

#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define _GNU_SOURCE
#define  __USE_GNU
#include <sched.h>
#define cpumask_t cpu_set_t

#include "../kernel/minix/config.h"
#include "../kernel/minix/types.h"
#include "../kernel/minix/const.h"
#include "../kernel/minix/timers.h"

#include "../kernel/minix/dc_usr.h"
#include "../kernel/minix/proc_usr.h"
#include "../kernel/minix/proc_sts.h"
#include "../kernel/minix/com.h"
#include "../kernel/minix/ipc.h"
#include "../kernel/minix/kipc.h"
#include "../kernel/minix/cmd.h"
#include "../kernel/minix/proxy_sts.h"
#include "../kernel/minix/proxy_usr.h"
#include "../kernel/minix/molerrno.h"
#include "../kernel/minix/endpoint.h"
#include "../kernel/minix/resource.h"
#include "../kernel/minix/ansi.h"
#include "../kernel/minix/priv.h"
#include "../kernel/minix/DVS_usr.h"
#include "../stub_syscall.h"

