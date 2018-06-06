
#define _MULTI_THREADED
#define _GNU_SOURCE     
#define  MOL_USERSPACE	1
#define _MINIX

#define _XOPEN_SOURCE 600 

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
#include "../../kernel/minix/drvs_usr.h"
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

#include <getopt.h>

/*	tty.h - Terminals	*/

/* First minor numbers for the various classes of TTY devices. */
#define CONS_MINOR	   0
#define LOG_MINOR	  15
#define RS232_MINOR	  16
#define KBD_MINOR	 127
#define KBDAUX_MINOR	 126
#define VIDEO_MINOR	 125
#define TTYPX_MINOR	 128
#define PTYPX_MINOR	 192

#define LINEWRAP	   1	/* console.c - wrap lines at column 80 */

#define TTY_IN_BYTES     256			/* tty input queue size */
#define TTY_OUT_BYTES    TTY_IN_BYTES	/* tty output queue size */

#define TAB_SIZE           8	/* distance between tab stops */
#define TAB_MASK           7	/* mask to compute a tab stop position */

#define ESC             '\33'	/* escape */

//#define O_NOCTTY       00400	/* from <fcntl.h>, or cc will choke */
#define O_NONBLOCK     04000

#define  TTY_NONE			0	/* NONE TTY 		*/
#define  TTY_PSEUDO			1	/* PSEUDO TTY 		*/
#define  TTY_TCPIP			2	/* Remote TCPIP TTY */
#define  TTY_M3IPC			3	/* M3IPC TTY 		*/

struct tty;
typedef _PROTOTYPE( int (*devfun_t), (struct tty *tp, int try_only) );
typedef _PROTOTYPE( void (*devfunarg_t), (struct tty *tp, int c) );

struct tty_conf_s {
   char* tty_name;
   int major;
   int minor;
   int type;
   char* device;
   char* server;
   int port;
   int endpoint;
 } ;
typedef struct tty_conf_s tty_conf_t;
#define TCONF_FORMAT "tty_name=%s major=%d minor=%d type=%d device=%s server=%s port=%d endpoint=%d\n"
#define TCONF_FIELDS(p) p->tty_name, p->major, p->minor, p->type, p->device, p->server ,p->port,p->endpoint

// LINUX fd_set struct is 
// #define FD_SETSIZE 64
//typedef struct fd_set {
//  unsigned int count;
//  int fd[FD_SETSIZE];
//} fd_set;

struct tty_pseudo_s {
	int master;
	int slave;
	fd_set fd_in;
	char sname[MNX_PATH_MAX];
	char in_buf[TTY_IN_BYTES];
	char in_count;
	char *in_next;
 } ;
typedef struct tty_pseudo_s tty_pseudo_t;
#define PSEUDO_FORMAT "master=%d slave=%d sname=%s\n"
#define PSEUDO_FIELDS(p) p->master, p->slave, p->sname

typedef struct tty {
  int tty_events;		/* set when TTY should inspect this line */
  int tty_index;		/* index into TTY table */
  int tty_minor;		/* device minor number */

  /* Input queue.  Typed characters are stored here until read by a program. */
  u16_t *tty_inhead;		/* pointer to place where next char goes */
  u16_t *tty_intail;		/* pointer to next char to be given to prog */
  int tty_incount;		/* # chars in the input queue */
  int tty_eotct;		/* number of "line breaks" in input queue */
  devfun_t tty_devread;		/* routine to read from low level buffers */
  devfun_t tty_icancel;		/* cancel any device input */
  int tty_min;			/* minimum requested #chars in input queue */
  moltimer_t tty_tmr;		/* the timer for this tty */

  /* Output section. */
  devfun_t tty_devwrite;	/* routine to start actual device output */
  devfunarg_t tty_echo;		/* routine to echo characters input */
  devfun_t tty_ocancel;		/* cancel any ongoing device output */
  devfun_t tty_break;		/* let the device send a break */

  /* Terminal parameters and status. */
  int tty_position;		/* current position on the screen for echoing */
  char tty_reprint;		/* 1 when echoed input messed up, else 0 */
  char tty_escaped;		/* 1 when LNEXT (^V) just seen, else 0 */
  char tty_inhibited;		/* 1 when STOP (^S) just seen (stops output) */
  int tty_pgrp;			/* slot number of controlling process */
  char tty_openct;		/* count of number of opens of this tty */

  /* Information about incomplete I/O requests is stored here. */
  char tty_inrepcode;		/* reply code, TASK_REPLY or MOLREVIVE */
  char tty_inrevived;		/* set to 1 if revive callback is pending */
  int tty_incaller;		/* process that made the call (usually FS) */
  int tty_inproc;		/* process that wants to read from tty */
  vir_bytes tty_in_vir;		/* virtual address where data is to go */
  int tty_inleft;		/* how many chars are still needed */
  int tty_incum;		/* # chars input so far */
  int tty_outrepcode;		/* reply code, TASK_REPLY or MOLREVIVE */
  int tty_outrevived;		/* set to 1 if revive callback is pending */
  int tty_outcaller;		/* process that made the call (usually FS) */
  int tty_outproc;		/* process that wants to write to tty */
  vir_bytes tty_out_vir;	/* virtual address where data comes from */
  int tty_outleft;		/* # chars yet to be output */
  int tty_outcum;		/* # chars output so far */
  int tty_iocaller;		/* process that made the call (usually FS) */
  int tty_ioproc;		/* process that wants to do an ioctl */
  int tty_ioreq;		/* ioctl request code */
  vir_bytes tty_iovir;		/* virtual address of ioctl buffer */

  /* select() data */
  int tty_select_ops;		/* which operations are interesting */
  int tty_select_proc;		/* which process wants notification */

  /* Miscellaneous. */
  devfun_t tty_ioctl;		/* set line speed, etc. at the device level */
  devfun_t tty_close;		/* tell the device that the tty is closed */
  
  void *tty_priv;		/* pointer to per device private data */
  mnx_termios_t tty_termios;	/* terminal attributes */
  mnx_winsize_t tty_winsize;	/* window size (#lines and #columns) */

  tty_conf_t	tty_cfg;		/* structure with configuration info from config file */ 
  tty_pseudo_t	tty_pseudo;		/* structure to carry Linux PSEUDO TERMINAL data */
  pthread_t		tty_out_thread;
  pthread_t		tty_in_thread;
  pthread_mutex_t tty_Imutex;
  pthread_mutex_t tty_Omutex;
  pthread_cond_t  tty_Ibarrier;	/* IN thread barrier : the tty INPUT  thread will wait on it */
  pthread_cond_t  tty_Obarrier;	/* OUT thread barrier : the tty OUTPUT thread will wait on it */
  pthread_cond_t  tty_Mbarrier; /* main  barrier  : the main thread will wait on it */
  
  u16_t tty_inbuf[TTY_IN_BYTES];/* tty input buffer */
  u8_t  tty_outbuf[TTY_OUT_BYTES];/* tty output buffer */
} tty_t;
#define TTY_FORMAT "tty_events=%X index=%d minor=%d \n"
#define TTY_FIELDS(p) p->tty_events, p->tty_index, p->tty_minor
#define TTYOUT_FORMAT "minor=%d tty_outrepcode=%d tty_outleft=%d tty_outcum=%d \n"
#define TTYOUT_FIELDS(p) p->tty_minor, p->tty_outrepcode, p->tty_outleft, p->tty_outcum 
#define TTYIN_FORMAT "minor=%d tty_incount=%d tty_eotct=%d tty_min=%d tty_inrepcode=%d tty_inleft=%d tty_incum=%d\n"
#define TTYIN_FIELDS(p) p->tty_minor, p->tty_incount, p->tty_eotct, p->tty_min, p->tty_inrepcode, p->tty_inleft, p->tty_incum 

#define TTY_NO_MINOR	(-1)	
#define TENTH_SECOND_RATE	1 // related to  clock.h INTERVAL_SECS and  INTERVAL_USECS

/* Values for the fields. */
#define NOT_ESCAPED        0	/* previous character is not LNEXT (^V) */
#define ESCAPED            1	/* previous character was LNEXT (^V) */
#define RUNNING            0	/* no STOP (^S) has been typed to stop output */
#define STOPPED            1	/* STOP (^S) has been typed to stop output */

/* Fields and flags on characters in the input queue. */
#define IN_CHAR       0x00FF	/* low 8 bits are the character itself */
#define IN_LEN        0x0F00	/* length of char if it has been echoed */
#define IN_LSHIFT          8	/* length = (c & IN_LEN) >> IN_LSHIFT */
#define IN_EOT        0x1000	/* char is a line break (^D, LF) */
#define IN_EOF        0x2000	/* char is EOF (^D), do not return to user */
#define IN_ESC        0x4000	/* escaped by LNEXT (^V), no interpretation */

/* Times and timeouts. */
#define force_timeout()	((void) (0))

/* Number of elements and limit of a buffer. */
#define buflen(buf)	(sizeof(buf) / sizeof((buf)[0]))
#define bufend(buf)	((buf) + buflen(buf))

#define EXIT_CODE			1
#define NEXT_CODE			2
#define nil ((void*)0)

#include "glo.h"

#define MTX_LOCK(x) do{ \
		TASKDEBUG("MTX_LOCK %s \n", #x);\
		pthread_mutex_lock(&x);\
		}while(0)
			
#define MTX_UNLOCK(x) do{ \
		pthread_mutex_unlock(&x);\
		TASKDEBUG("MTX_UNLOCK %s \n", #x);\
		}while(0)	
			
#define COND_WAIT(x,y) do{ \
		TASKDEBUG("COND_WAIT %s %s\n", #x,#y );\
		pthread_cond_wait(&x, &y);\
		}while(0)	

#define COND_SIGNAL(x) do{ \
		pthread_cond_signal(&x);\
		TASKDEBUG("COND_SIGNAL %s\n", #x);\
		}while(0)	
		


