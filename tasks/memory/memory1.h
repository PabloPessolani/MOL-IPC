/* This is the master header for memory.  It includes some other files
 * and defines the principal constants.
 */
#define _POSIX_SOURCE      1	/* tell headers to include POSIX stuff */
#define _MINIX             1	/* tell headers to include MINIX stuff */
#define _SYSTEM            1	/* tell headers that this is the kernel */

#define VERBOSE		   0    /* show messages during initialization? */

#define NR_VMS	2 /*defino para validar no encuentro el valor _NR_VMS*/

/* The following are so basic, all the *.c files get them automatically. */
#include <sys/types.h>
#include <sys/time.h>
#include <sys/time.h>


#include <minix/config.h>	/* MUST be first */
#include <minix/ansi.h>		/* MUST be second  - le agregué minix?*/

#include <minix/const.h>
#include <minix/types.h>
#include <minix/type.h>
//#include <minix/dmap.h>

#include <limits.h>
#include <errno.h>

#include <minix/syslib.h>
#include <minix/sysutil.h>

#include "const.h"
#include "type.h"
#include "proto.h"
#include "glo.h"
