/* This table has one slot per process.  It contains all the process management
 * information for each process.  Among other things, it defines the text, data
 * and stack segments, uids and gids, and various flags.  The kernel and file
 * systems have tables that are also indexed by process, with the contents
 * of corresponding slots referring to the same process in all three.
 */

#define SYSTASK_Q	 -19	/* highest, used for kernel tasks */
#define TASK_Q		 -15	/* highest, used for kernel tasks */
#define SERVER_Q	 -10	/* highest, used for kernel tasks */
#define MAX_USER_Q  	 -5     /* highest priority for user processes */   
#define USER_Q  	  0     /* default (should correspond to nice 0) */   
#define MIN_USER_Q	  18	/* minimum priority for user processes */
#define IDLE_Q		  19    /* lowest, only IDLE process goes here */


struct mproc {

  char mp_exitstatus;		/* storage for status when process exits */
  char mp_sigstatus;		/* storage for signal # for killed procs */
  pid_t mp_pid;			/* process id */
  int mp_endpoint;		/* kernel endpoint id */

  pid_t mp_procgrp;		/* pid of process group (used for signals) */
  pid_t mp_wpid;		/* pid this process is waiting for */
  int mp_parent;		/* index of parent process */

  /* Child user and system times. Accounting done on child exit. */
  molclock_t mp_child_utime;	/* cumulative user time of children */
  molclock_t mp_child_stime;	/* cumulative sys time of children */

  /* Real and effective uids and gids. */
  uid_t mp_realuid;		/* process' real uid */
  uid_t mp_effuid;		/* process' effective uid */
  gid_t mp_realgid;		/* process' real gid */
  gid_t mp_effgid;		/* process' effective gid */

  /* Signal handling information. */
  sigset_t mp_ignore;		/* 1 means ignore the signal, 0 means don't */
  sigset_t mp_catch;		/* 1 means catch the signal, 0 means don't */
  sigset_t mp_sig2mess;		/* 1 means transform into notify message */
  sigset_t mp_sigmask;		/* signals to be blocked */
  sigset_t mp_sigmask2;		/* saved copy of mp_sigmask */
  sigset_t mp_sigpending;	/* pending signals to be handled */
  moltimer_t mp_timer;		/* watchdog timer for alarm(2) */

  unsigned mp_flags;		/* flag bits */
  message mp_reply;		/* reply message to be sent to one */

  /* Scheduling priority. */
  signed int mp_nice;		/* nice is PRIO_MIN..PRIO_MAX, standard 0. */

};
typedef struct mproc mproc_t;

#define PM_PROC_FORMAT "mp_endpoint=%d mp_pid=%d mp_parent=%d mp_flags=%X mp_nice=%d \n"
#define PM_PROC_FIELDS(p) p->mp_endpoint, p->mp_pid,p->mp_parent, p->mp_flags, p->mp_nice 

#define PM_PROC1_FORMAT "PID=%d pgrp=%d ruid=%d euid=%d rgid=%d egid=%d flags=%X\n"
#define PM_PROC1_FIELDS(mp) mp->mp_pid, mp->mp_procgrp, mp->mp_realuid,mp->mp_effuid, mp->mp_realgid,mp->mp_effgid, mp->mp_flags

/* Flag values */
#define IN_USE          0x0001	/* set when 'mproc' slot in use */
#define WAITING         0x0002	/* set by WAIT system call */
#define ZOMBIE          0x0004	/* set by EXIT, cleared by WAIT */
#define PAUSED          0x0008	/* set by PAUSE system call */
#define ALARM_ON        0x0010	/* set when SIGALRM timer started */
#define SEPARATE		0x0020	/* set if file is separate I & D space */
#define TRACED			0x0040	/* set if process is to be traced */
#define STOPPED			0x0080	/* set if process stopped for tracing */
#define SIGSUSPENDED 	0x0100	/* set by SIGSUSPEND system call */
#define REPLYPENDING 	0x0200	/* set if a reply message is pending */
#define ONSWAP	 		0x0400	/* set if data segment is swapped out */
#define SWAPIN	 		0x0800	/* set if on the "swap this in" queue */
#define DONT_SWAP      	0x1000   /* never swap out this process */
#define PRIV_PROC      	0x2000   /* system process, special privileges */
#define FORKWAIT       	0x4000
#define RMT_PROC       	0x8000	/* The process is remote */
			
#define NIL_MPROC ((struct mproc *) 0)


