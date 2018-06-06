/* The kernel call implemented in this file:
 *   m_type:	SYS_TIMES
 *
 * The parameters for this kernel call are:
 *    m4_l1:	T_ENDPT		(get info for this process)	
 *    m4_l1:	T_USER_TIME		(return values ...)	
 *    m4_l2:	T_SYSTEM_TIME	
 *    m4_l5:	T_BOOT_TICKS	
 */

#include "systask.h"
#include "readproc.h"
#include "pwcache.h"
#include "procps.h"


#if USE_TIMES

extern long clockTicks;

static void stat2proc(const char* S, proc_stat_t *restrict P);

/*===========================================================================*
 *				do_times				     *
 *===========================================================================*/
int do_times(message *m_ptr)
{
/* Handle sys_times().  Retrieve the accounting information. */
	
  /* Insert the times needed by the SYS_TIMES kernel call in the message. 
   * The clock's interrupt handler may run to update the user or system time
   * while in this code, but that cannot do any harm.
   */

	struct tms t;
	int	rcode;
	int fd, bytes;
	static char sbuf[1024];
	static proc_stat_t PS;
	static char filename[80];

	/*open the /proc/PID/stat file */
	sprintf(filename, "/proc/%d/stat", getpid());
	fd = open(filename, O_RDONLY, 0);
	if( fd < 0) return (errno);

	/*read the register into the buffer*/
	bytes  = read(fd, sbuf, 1024);
	close(fd);
	if( bytes <= 0) return(errno);

	/*parse the register into the proc_stat_t */
	stat2proc( sbuf, &PS);

	TASKDEBUG("ppid=%d, pgrp=%d, session=%d, tty=%d, tpgid=%d\n",PS.ppid, PS.pgrp, PS.session, PS.tty, PS.tpgid);
	TASKDEBUG("utime=%d stime=%d\n", PS.utime,PS.stime);
	TASKDEBUG("priority=%d, nice=%d \n",PS.priority, PS.nice);

//  e_proc_nr = (m_ptr->T_ENDPT == SELF) ? m_ptr->m_source : m_ptr->T_ENDPT;
//  if(e_proc_nr != NONE && isokendpt(e_proc_nr, &proc_nr)) {
//      rp = proc_addr(proc_nr);
	m_ptr->T_USER_TIME   = PS.utime * clockTicks;
	m_ptr->T_SYSTEM_TIME = PS.stime * clockTicks;
//}
	m_ptr->T_BOOT_TICKS = get_uptime();  
	TASKDEBUG(" boot=%ld user=%d system=%d\n", m_ptr->T_BOOT_TICKS, m_ptr->T_USER_TIME,m_ptr->T_SYSTEM_TIME  );

  return(OK);
}


///////////////////////////////////////////////////////////////////////
//		stat2proc - > from procps-3.2.8
// Reads /proc/*/stat files, being careful not to trip over processes with
// names like ":-) 1 2 3 4 5 6".
static void stat2proc(const char* S, proc_stat_t *restrict P) {
    unsigned num;
    char* tmp;

    /* fill in default values for older kernels */
    P->processor = 0;
    P->rtprio = -1;
    P->sched = -1;
    P->nlwp = 0;

    S = strchr(S, '(') + 1;
    tmp = strrchr(S, ')');
    num = tmp - S;
    if(unlikely(num >= sizeof P->cmd)) num = sizeof P->cmd - 1;
    memcpy(P->cmd, S, num);
    P->cmd[num] = '\0';
    S = tmp + 2;                 // skip ") "

    num = sscanf(S,
       "%c "
       "%d %d %d %d %d "
       "%lu %lu %lu %lu %lu "
       "%Lu %Lu %Lu %Lu "  /* utime stime cutime cstime */
       "%ld %ld "
       "%d "
       "%ld "
       "%Lu "  /* start_time */
       "%lu "
       "%ld "
       "%lu %"KLF"u %"KLF"u %"KLF"u %"KLF"u %"KLF"u "
       "%*s %*s %*s %*s " /* discard, no RT signals & Linux 2.1 used hex */
       "%"KLF"u %*lu %*lu "
       "%d %d "
       "%lu %lu",
       &P->state,
       &P->ppid, &P->pgrp, &P->session, &P->tty, &P->tpgid,
       &P->flags, &P->min_flt, &P->cmin_flt, &P->maj_flt, &P->cmaj_flt,
       &P->utime, &P->stime, &P->cutime, &P->cstime,
       &P->priority, &P->nice,
       &P->nlwp,
       &P->alarm,
       &P->start_time,
       &P->vsize,
       &P->rss,
       &P->rss_rlim, &P->start_code, &P->end_code, &P->start_stack, &P->kstk_esp, &P->kstk_eip,
/*     P->signal, P->blocked, P->sigignore, P->sigcatch,   */ /* can't use */
       &P->wchan, /* &P->nswap, &P->cnswap, */  /* nswap and cnswap dead for 2.4.xx and up */
/* -- Linux 2.0.35 ends here -- */
       &P->exit_signal, &P->processor,  /* 2.2.1 ends with "exit_signal" */
/* -- Linux 2.2.8 to 2.5.17 end here -- */
       &P->rtprio, &P->sched  /* both added to 2.5.18 */
    );

    if(!P->nlwp){
      P->nlwp = 1;
    }

}

#endif /* USE_TIMES */
