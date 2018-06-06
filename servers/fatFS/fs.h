/* This is the master header for FS.  It includes some other files
 * and defines the principal constants.
 */


#define _MULTI_THREADED
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
//#include <fcntl.h>
//#include <limits.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/sysinfo.h> 
//#include <sys/mnx_stat.h>
//#include <sys/statfs.h>
#include <getopt.h>

#include <netinet/in.h>

#include <arpa/inet.h>

#include "../../kernel/minix/config.h"
#include "../../kernel/minix/limits.h"
#include "../../kernel/minix/const.h"
#include "../../kernel/minix/types.h"
#include "../../kernel/minix/dmap.h"
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
// #include "../../kernel/minix/proxy.h"
#include "../../kernel/minix/resource.h"
#include "../../kernel/minix/signal.h"
#include "../../kernel/minix/callnr.h"
#include "../../kernel/minix/syslib.h"
#include "../../kernel/minix/dir.h"
#include "../../kernel/minix/fcntl.h"
#include "../../kernel/minix/select.h"
// Este es para constantes y tipos de minix
#include "../../kernel/minix/type.h"
#include "../../kernel/minix/ioctl.h"
#include "../../kernel/minix/svrctl.h"
#include "../../kernel/minix/mollib.h"

#include "../pm/mproc.h"

#ifndef UTILITY_C
#include "../../kernel/minix/mnx_stat.h"
#include "../../kernel/minix/statfs.h"
#include "../../kernel/minix/unistd.h"
#endif // UTILITY_C

//Para lectura de configuracion de MOLFS
#include "../../kernel/minix/configfile.h"

#include "../../stub_syscall.h"

#include "../debug.h"
#include "../macros.h"

/*FatFS includes*/
// #include "ffconf.h"		/* Declarations of FatFs API */
#include "ff.h"			/* Declarations of FatFs API */
#include "diskio.h"		/* Declarations of device I/O functions */

#include "const.h"
#include "type.h"
#include "fproc.h"
#include "param.h"
#include "glo.h"
#include "inode.h"
#include "proto.h"
#include "super.h"
#include "buf.h"
#include "file.h"
// #include "select.h" //ver sys_setalarm y demas

#include "../../kernel/minix/lock.h"

#define _POSIX_SOURCE      1	/* tell headers to include POSIX stuff */
#define _MINIX             1	/* tell headers to include MINIX stuff */

#define     MAJOR2TAB(major)	dmap_tab[dmap_rev[major]]
#define     DEV2TAB(dev)		dmap_tab[dmap_rev[(dev >> MNX_MAJOR) & BYTE]]
#define 	MM2DEV(major, minor)	((major << MNX_MAJOR) | minor)
#define 	DEV2MAJOR(dev)			((dev >> MNX_MAJOR) & BYTE)
#define 	DEV2MINOR(dev)			((dev >> MNX_MINOR) & BYTE)