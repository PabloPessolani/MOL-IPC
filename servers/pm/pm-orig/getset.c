/* This file handles the 4 system calls that get and set uids and gids.
 * It also handles getpid(), setsid(), and getpgrp().  The code for each
 * one is so tiny that it hardly seemed worthwhile to make each a separate
 * function.
 */

#include "pm.h"
#include <sys/wait.h>
#include <minix/callnr.h>
#include <minix/com.h>
#include <minix/ipc.h>
#include <sys/resource.h>
#include <signal.h>
#include "mproc.h"
#include "param.h"

#define		MAXBUFSIZE	1432 
char	buffer[MAXBUFSIZE];

/*===========================================================================*
 *				do_getset				     *
 *===========================================================================*/
PUBLIC int do_getset()
{
/* Handle GETUID, GETGID, GETPID, GETPGRP, SETUID, SETGID, SETSID.  The four
 * GETs and SETSID return their primary results in 'r'.  GETUID, GETGID, and
 * GETPID also return secondary results (the effective IDs, or the parent
 * process ID) in 'reply_res2', which is returned to the user.
 */

  register struct mproc *rmp = mp;
  int r, proc;

  switch(call_nr) {
	case GETUID:
		r = rmp->mp_realuid;
		rmp->mp_reply.reply_res2 = rmp->mp_effuid;
		break;

	case GETGID:
		r = rmp->mp_realgid;
		rmp->mp_reply.reply_res2 = rmp->mp_effgid;
		break;

	case GETPID:
		r = mproc[who_p].mp_pid;
		rmp->mp_reply.reply_res2 = mproc[rmp->mp_parent].mp_pid;
		if(pm_isokendpt(m_in.endpt, &proc) == OK && proc >= 0)
			rmp->mp_reply.reply_res3 = mproc[proc].mp_pid;
		break;

	case SETEUID:
	case SETUID:
		if (rmp->mp_realuid != (uid_t) m_in.usr_id && 
				rmp->mp_effuid != SUPER_USER)
			return(EPERM);
		if(call_nr == SETUID) rmp->mp_realuid = (uid_t) m_in.usr_id;
		rmp->mp_effuid = (uid_t) m_in.usr_id;
		tell_fs(SETUID, who_e, rmp->mp_realuid, rmp->mp_effuid);
		r = OK;
		break;

	case SETEGID:
	case SETGID:
		if (rmp->mp_realgid != (gid_t) m_in.grp_id && 
				rmp->mp_effuid != SUPER_USER)
			return(EPERM);
		if(call_nr == SETGID) rmp->mp_realgid = (gid_t) m_in.grp_id;
		rmp->mp_effgid = (gid_t) m_in.grp_id;
		tell_fs(SETGID, who_e, rmp->mp_realgid, rmp->mp_effgid);
		r = OK;
		break;

	case SETSID:
		if (rmp->mp_procgrp == rmp->mp_pid) return(EPERM);
		rmp->mp_procgrp = rmp->mp_pid;
		tell_fs(SETSID, who_e, 0, 0);
		/* fall through */

	case GETPGRP:
		r = rmp->mp_procgrp;
		break;

	default:
		r = EINVAL;
		break;	
  }
  return(r);
}


/*===========================================================================*
 *				do_testipc				     *
 *===========================================================================*/
PUBLIC int do_testipc()
{
return(0);
}

/*===========================================================================*
 *				do_testcopy				     *
 *===========================================================================*/
PUBLIC int do_testcopy()
{
	int ret, i;
/*********	
printf("PM do_testcopy: src=%d dst=%d bytes=%d loops=%d saddr=%X daddr=%X\n", 
	m_in.m7_i1,		
	m_in.m7_i2,		
	m_in.m7_i3,		
	m_in.m7_i4,		
	m_in.m7_p1,		
	m_in.m7_p2);	
**********/

	for(i = 0; i < m_in.m7_i4; i++) { 
    		if( m_in.m7_i1 == SYSTEM)
			ret = sys_testcopy(SYSTEM, 0, who_e, m_in.m7_p2, m_in.m7_i3, m_in.m7_i4);
		else
			ret = sys_testcopy(PM_PROC_NR, &buffer, who_e, m_in.m7_p2, m_in.m7_i3, m_in.m7_i4);
	}

	return(ret);
}



