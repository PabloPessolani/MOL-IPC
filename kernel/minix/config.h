#ifndef _CONFIG_H
#define _CONFIG_H

#define MOLIPC	1
#define MOLFS	1
#define MOLPROFILING	0


/* Minix release and version numbers. */
#define OS_RELEASE "3"
#define OS_VERSION "1.2a-MOLIPC"
#define OS_DATE    "2016-03-26"

#define DVS_VERSION 4
#define DVS_SUBVER  1

#define MAXCOPYBUF	65536 
#define MAXCOPYLEN	(16 * MAXCOPYBUF)

/* This file sets configuration parameters for the MINIX kernel, FS, and PM.
 * It is divided up into two main sections.  The first section contains
 * user-settable parameters.  In the second section, various internal system

 * parameters are set based on the user-settable parameters.
 *
 * Parts of config.h have been moved to sys_config.h, which can be included
 * by other include files that wish to get at the configuration data, but
 * don't want to pollute the users namespace. Some editable values have
 * gone there.
 *
 */

/* The MACHINE (called _MINIX_MACHINE) setting can be done
 * in <minix/machine.h>.
 */
#include "sys_config.h"

#define USE_DVS_RWLOCK		0
#define USE_DVS_MUTEX		1
#define USE_DVS_SPINLOCK	2
#define USE_DVS_RWSEM	3
#define USE_DVS_RCU		4

#define USE_DC_RWLOCK		0
#define USE_DC_MUTEX		1
#define USE_DC_SPINLOCK		2
#define USE_DC_RWSEM		3
#define USE_DC_RCU			4

#define USE_PROC_RWLOCK		0
#define USE_PROC_MUTEX		1
#define USE_PROC_SPINLOCK	2
#define USE_PROC_RWSEM	3
#define USE_PROC_RCU		4

#define USE_NODE_RWLOCK		0
#define USE_NODE_MUTEX		1
#define USE_NODE_SPINLOCK	2
#define USE_NODE_RWSEM		3
#define USE_NODE_RCU		4

#define USE_PROXY_RWLOCK	0
#define USE_PROXY_MUTEX		1
#define USE_PROXY_SPINLOCK	2
#define USE_PROXY_RWSEM 	3
#define USE_PROXY_RCU		4

#define USE_TASK_RWLOCK		0
#define USE_TASK_MUTEX		1
#define USE_TASK_SPINLOCK	2
#define USE_TASK_RWSEM		3
#define USE_TASK_RCU		4

#define USE_LIST_NORMAL		0
#define USE_LIST_RCU		1

#define DEBUGALL	0xFFFFFFFF	/* Set all debug levels */

//#define LOCK_DVS_TYPE	USE_DVS_MUTEX
//#define LOCK_DC_TYPE		USE_DC_MUTEX
//#define LOCK_NODE_TYPE	USE_NODE_MUTEX
//#define LOCK_PROXY_TYPE	USE_PROXY_MUTEX

/*****************************************************/
/* WARNING: To use RCU: DVS, VM, NODE and PROXY 	*/
/* must use RCU , and PROC must use RCU or SPINLOCK */
/*****************************************************/
#define LOCK_DVS_TYPE	USE_DVS_MUTEX
#define LOCK_DC_TYPE	USE_DC_MUTEX
#define LOCK_NODE_TYPE	USE_NODE_MUTEX
#define LOCK_PROXY_TYPE	USE_PROXY_MUTEX
#define LOCK_PROC_TYPE	USE_PROC_MUTEX
#define LOCK_TASK_TYPE	USE_TASK_MUTEX
#define LIST_TYPE		USE_LIST_NORMAL

#define MACHINE      _MINIX_MACHINE

#define IBM_PC       _MACHINE_IBM_PC
#define SUN_4        _MACHINE_SUN_4
#define SUN_4_60     _MACHINE_SUN_4_60
#define ATARI        _MACHINE_ATARI
#define MACINTOSH    _MACHINE_MACINTOSH

/* Number of slots in the process table for non-kernel processes. The number
 * of system processes defines how many processes with special privileges 
 * there can be. User processes share the same properties and count for one. 
 *
 * These can be changed in sys_config.h.
 */

#define NR_FIXED_TASKS  _NR_FIXED_TASKS 
#define NR_NODES 	_NR_NODES 
#define NR_SYSTASKS	_NR_SYSTASKS
#define NR_TASKS	_NR_TASKS
#define NR_SERVERS	_NR_SERVERS
#define NR_SYS_PROCS	_NR_SYS_PROCS
#define NR_USR_PROCS    _NR_USR_PROCS
#define NR_PROCS	_NR_PROCS

#define NR_DCS 	  	_NR_DCS 

#define MAX_PROF		10 /*Maximum performance profiling entries in process descriptor */

/* The buffer cache should be made as large as you can afford. */
#if (MACHINE == IBM_PC && _WORD_SIZE == 2)
#define NR_BUFS           40	/* # blocks in the buffer cache */
#define NR_BUF_HASH       64	/* size of buf hash table; MUST BE POWER OF 2*/
#endif

#if (MACHINE == IBM_PC && _WORD_SIZE == 4)
#define NR_BUFS         1200	/* # blocks in the buffer cache */
#define NR_BUF_HASH     2048	/* size of buf hash table; MUST BE POWER OF 2*/
#endif

#if (MACHINE == SUN_4_60)
#define NR_BUFS		 512	/* # blocks in the buffer cache (<=1536) */
#define NR_BUF_HASH	 512	/* size of buf hash table; MUST BE POWER OF 2*/
#endif

#ifdef MOLIPC
#define NR_BUFS         1200	/* # blocks in the buffer cache */
#define NR_BUF_HASH     2048	/* size of buf hash table; MUST BE POWER OF 2*/
#endif
	
/* Number of controller tasks (/dev/cN device classes). */
#define NR_CTRLRS          2

/* Enable or disable the second level file system cache on the RAM disk. */
#define ENABLE_CACHE2      0

/* Enable or disable swapping processes to disk. */
#define ENABLE_SWAP	   1

/* Include or exclude an image of /dev/boot in the boot image. 
 * Please update the makefile in /usr/src/tools/ as well.
 */
#define ENABLE_BOOTDEV	   0	/* load image of /dev/boot at boot time */

/* DMA_SECTORS may be increased to speed up DMA based drivers. */
#define DMA_SECTORS        1	/* DMA buffer size (must be >= 1) */

/* Include or exclude backwards compatibility code. */
#define ENABLE_BINCOMPAT   0	/* for binaries using obsolete calls */
#define ENABLE_SRCCOMPAT   0	/* for sources using obsolete calls */

/* Which processes should receive diagnostics from the kernel and system? 
 * Directly sending it to TTY only displays the output. Sending it to the
 * log driver will cause the diagnostics to be buffered and displayed.
 * Messages are sent by src/lib/sysutil/kputc.c to these processes, in
 * the order of this array, which must be terminated by NONE. This is used
 * by drivers and servers that printf().
 * The kernel does this for its own kprintf() in kernel/utility.c, also using
 * this array, but a slightly different mechanism.
 */
#define OUTPUT_PROCS_ARRAY	{ TTY_PROC_NR, LOG_PROC_NR, NONE }

/* NR_CONS, NR_RS_LINES, and NR_PTYS determine the number of terminals the
 * system can handle.
 */
#define NR_CONS        4	/* # system consoles (1 to 8) */
#define	NR_RS_LINES	   4	/* # rs232 terminals (0 to 4) */
#define	NR_PTYS		   32	/* # pseudo terminals (0 to 64) */
#define	NR_VTTYS	   8	/* # virtual terminals (0 to 7) */

/*===========================================================================*
 *	There are no user-settable parameters after this line		     *
 *===========================================================================*/
/* Set the CHIP type based on the machine selected. The symbol CHIP is actually
 * indicative of more than just the CPU.  For example, machines for which
 * CHIP == INTEL are expected to have 8259A interrrupt controllers and the
 * other properties of IBM PC/XT/AT/386 types machines in general. */
#define INTEL             _CHIP_INTEL	/* CHIP type for PC, XT, AT, 386 and clones */
#define M68000            _CHIP_M68000	/* CHIP type for Atari, Amiga, Macintosh    */
#define SPARC             _CHIP_SPARC	/* CHIP type for SUN-4 (e.g. SPARCstation)  */

/* Set the FP_FORMAT type based on the machine selected, either hw or sw    */
#define FP_NONE	 _FP_NONE	/* no floating point support                */
#define FP_IEEE	 _FP_IEEE	/* conform IEEE floating point standard     */

/* _MINIX_CHIP is defined in sys_config.h. */
#define CHIP	_MINIX_CHIP

/* _MINIX_FP_FORMAT is defined in sys_config.h. */
#define FP_FORMAT	_MINIX_FP_FORMAT

/* _ASKDEV and _FASTLOAD are defined in sys_config.h. */
#define ASKDEV _ASKDEV
#define FASTLOAD _FASTLOAD

#endif /* _CONFIG_H */
