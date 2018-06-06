
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
#include <libgen.h>
 
#include <sys/types.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/sysinfo.h> 
#include <sys/stat.h> 

#include "../../kernel/minix/config.h"
#include "../../kernel/minix/limits.h"
#include "../../kernel/minix/const.h"
#include "../../kernel/minix/types.h"
#include "../../kernel/minix/type.h"
#include "../../kernel/minix/timers.h"
#include "../../kernel/minix/dc_usr.h"
#include "../../kernel/minix/proc_usr.h"
#include "../../kernel/minix/proc_sts.h"
#include "../../kernel/minix/dvs_usr.h"
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
#include "../../kernel/minix/ansi.h"
#include "../../kernel/minix/unistd.h"
//#include "../../kernel/minix/fcntl.h"
#include "../../stub_syscall.h"
#include "../debug.h"
#include "../macros.h"

#define _POSIX_SOURCE      1	/* tell headers to include POSIX stuff */
#define _MINIX             1	/* tell headers to include MINIX stuff */
#define _SYSTEM            1	/* tell headers that this is the kernel */

// REQUESTS
#define M3C_NONE	0
#define M3C_GET		1
#define M3C_PUT		2






