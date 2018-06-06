#ifndef PROC_USR_H
#define PROC_USR_H

#define MAXPROCNAME	16
#define PRIV_KERNEL	0
#define PRIV_SYSTEM	1
#define PRIV_SYSTEM	1

/* =================================*/
/* PROCESS DESCRIPTOR	             */
/* =================================*/
struct proc_usr {

  int p_nr;				/* process number				*/
  int p_endpoint;			/* process endpoint				*/
  int p_dcid;				/* process VMID					*/
  volatile unsigned long p_rts_flags;	/* process is runnable only if zero 		*/
  int p_lpid;				/* local LINUX PID 				*/
  int p_nodeid;				/* Node ID where the process PRIMARY replica is running  */
  unsigned long p_nodemap;		/* bitmap of nodes where replicas of this process exists UNTIL 32 NODES!!! */
//  node_map_t p_nodemap;		/* bitmap of nodes where replicas of this process exists */

volatile unsigned long  p_misc_flags;	/* miselaneous flags				*/

  int p_getfrom;			/* from whom does process want to receive?	*/
  int p_sendto;				/* to whom does process want to send? 		*/
  int p_waitmigr;			/* waiting the migration of a process 		*/
  int p_waitunbind; 		/*  waiting the unbinding of a process 		*/
  int p_proxy;				/* the descriptor is enqueued on a proxy 	*/
  unsigned long p_lclsent; 		/* counter of LOCAL messages sent	*/
  unsigned long p_rmtsent; 		/* counter of REMOTE messages sent	*/ 
  unsigned long p_lclcopy; 		/* counter of LOCAL copies as source	*/
  unsigned long p_rmtcopy; 		/* counter of REMOTE copies as source	*/ 

  char p_name[MAXPROCNAME]; 	

#ifdef MOL_USERSPACE
	cpu_set_t p_cpumask;
#else
	cpumask_t p_cpumask;
#endif 
  
};
typedef struct proc_usr proc_usr_t;

#define PROC_USR_FORMAT "nr=%d endp=%d dcid=%d flags=%lX misc=%lX lpid=%d nodeid=%d nodemap=%lX name=%s \n"
#define PROC_USR_FIELDS(p) p->p_nr,p->p_endpoint, p->p_dcid, p->p_rts_flags, p->p_misc_flags, \
			 p->p_lpid, p->p_nodeid, p->p_nodemap, p->p_name

#define PROC_WAIT_FORMAT "endp=%d dcid=%d flags=%lX p_getfrom=%d p_sendto=%d p_waitmigr=%ld p_waitunbind=%d p_proxy=%d\n"
#define PROC_WAIT_FIELDS(p) p->p_endpoint, p->p_dcid, p->p_rts_flags, p->p_getfrom, \
			 p->p_sendto, p->p_waitmigr, p->p_waitunbind, p->p_proxy

#define PROC_COUNT_FORMAT "nr=%d endp=%d dcid=%d p_lclsent=%ld p_rmtsent=%ld p_lclcopy=%ld p_rmtcopy=%ld \n"
#define PROC_COUNT_FIELDS(p)  p->p_nr, p->p_endpoint, p->p_dcid, p->p_lclsent, p->p_rmtsent, \
			 p->p_lclcopy, p->p_rmtcopy
			 
#define PROC_CPU_FORMAT "nr=%d endp=%d dcid=%d lpid=%d p_cpumask=%lX name=%s \n"
#define PROC_CPU_FIELDS(p) p->p_nr,p->p_endpoint, p->p_dcid, p->p_lpid, p->p_cpumask.bits[0], p->p_name
			 
#endif /* PROC_USR_H */
