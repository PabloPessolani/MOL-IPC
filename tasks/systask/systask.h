#define _MULTI_THREADED
#define _GNU_SOURCE     
#define  MOL_USERSPACE	1

#include <stdio.h>
#include <assert.h>
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
#include <sys/syscall.h> 
#include <sys/mman.h>
#include <fcntl.h>
#include <malloc.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include "../../kernel/minix/config.h"
#include "../../kernel/minix/const.h"
#include "../../kernel/minix/types.h"
#include "../../kernel/minix/type.h"
#include "../../kernel/minix/limits.h"
#include "../../kernel/minix/timers.h"

#include "../../kernel/minix/dvs_usr.h"
#include "../../kernel/minix/dc_usr.h"
#include "../../kernel/minix/node_usr.h"
#include "../../kernel/minix/proc_usr.h"
#include "../../kernel/minix/proc_sts.h"
#include "../../kernel/minix/com.h"
#include "../../kernel/minix/ipc.h"
#include "../../kernel/minix/kipc.h"
#include "../../kernel/minix/cmd.h"
#include "../../kernel/minix/proxy_usr.h"
#include "../../kernel/minix/proxy_sts.h"
#include "../../kernel/minix/molerrno.h"
#include "../../kernel/minix/endpoint.h"
#include "../../kernel/minix/resource.h"
#include "../../kernel/minix/callnr.h"
#include "../../kernel/minix/ansi.h"
#include "../../kernel/minix/priv.h"
#include "../../kernel/minix/slots.h"
#include "../../stub_syscall.h"

#include "sp.h"

#include "../debug.h"
#include "../macros.h"

#include "config.h"
#include "system.h"
#include "table.h"
#include "proto.h"
#include "clock.h"
#include "glo.h"

#define PROC_MAPPED(p_nr) ((( p_nr + dc_ptr->dc_nr_tasks ) * dvs_ptr->d_size_proc) + kproc)



