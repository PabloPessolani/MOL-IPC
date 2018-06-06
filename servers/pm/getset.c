/* This file handles the 4 system calls that get and set uids and gids.
 * It also handles getpid(), setsid(), and getpgrp().  The code for each
 * one is so tiny that it hardly seemed worthwhile to make each a separate
 * function.
 */
#include "pm.h"


/*===========================================================================*
 *				do_getset				     						*
 *===========================================================================*/
int do_getset(void)
{
/* Handle MOLGETUID, MOLGETGID, MOLGETPID, MOLGETPGRP, MOLSETUID, MOLSETGID, MOLSETSID.  The four
 * MOLGETs and MOLSETSID return their primary results in 'r'.  MOLGETUID, MOLGETGID, and
 * MOLGETPID also return secondary results (the effective IDs, or the parent
 * process ID) in 'reply_res2', which is returned to the user.
 */

	mproc_t *rmp = mp;
	proc_usr_t *rkp = kp;
	int ret, proc;
	message m __attribute__((aligned(0x1000)));
	message m_ptr;
	
SVRDEBUG("call_nr=%d\n", call_nr);
  switch(call_nr) {
	case MOLGETUID:
		ret = rmp->mp_realuid;
		rmp->mp_reply.reply_res2 = rmp->mp_effuid;
		SVRDEBUG("ret=%d\n", ret);
		break;
	case MOLGETGID:
		ret = rmp->mp_realgid;
		rmp->mp_reply.reply_res2 = rmp->mp_effgid;
		SVRDEBUG("ret=%d\n", ret);
		break;
	case MOLGETPID:
		SVRDEBUG(PM_PROC_FORMAT,PM_PROC_FIELDS(rmp));
		rmp->mp_reply.reply_res2 = mproc[rmp->mp_parent].mp_pid;
		ret = pm_isokendpt(m_in.m_source, &proc);
		if( (ret == OK) && (proc >= 0 ) ) {
			rmp->mp_reply.reply_res3 = mproc[proc].mp_pid;
		} else {
			ERROR_RETURN(ret);
		}
		ret = mproc[who_p].mp_pid;
		SVRDEBUG("ret=%d\n", ret);
		break;

	case MOLSETEUID:
	case MOLSETUID:
		if (rmp->mp_realuid != (uid_t) m_in.usr_id && 
				rmp->mp_effuid != SUPER_USER)
			ERROR_RETURN(EMOLPERM);
		if(call_nr == MOLSETUID) rmp->mp_realuid = (uid_t) m_in.usr_id;
		rmp->mp_effuid = (uid_t) m_in.usr_id;
		tell_fs(MOLSETUID, who_e, rmp->mp_realuid, rmp->mp_effuid);
		ret = OK;
		SVRDEBUG("ret=%d\n", ret);
		break;

	case MOLSETEGID:
	case MOLSETGID:
		if (rmp->mp_realgid != (gid_t) m_in.grp_id && 
				rmp->mp_effuid != SUPER_USER)
			return(EMOLPERM);
		if(call_nr == MOLSETGID) rmp->mp_realgid = (gid_t) m_in.grp_id;
		rmp->mp_effgid = (gid_t) m_in.grp_id;
		tell_fs(MOLSETGID, who_e, rmp->mp_realgid, rmp->mp_effgid);
		ret = OK;
		SVRDEBUG("ret=%d\n", ret);
		break;

	case MOLSETSID:
		if (rmp->mp_procgrp == rmp->mp_pid) ERROR_RETURN(EMOLPERM);
		rmp->mp_procgrp = rmp->mp_pid;
		tell_fs(MOLSETSID, who_e, 0, 0);
		/* fall through */
	case MOLGETPGRP:
		ret = rmp->mp_procgrp;
		SVRDEBUG("ret=%d\n", ret);
		break;
	case MOLSETPNAME:
		proc = _ENDPOINT_P(m_in.M3_ENDPT);
		rkp = &kproc[proc+dc_ptr->dc_nr_tasks];
		/* Tell SYSTASK copy the kernel entry to kproc[parent_nr]  	*/
		if((ret =sys_procinfo(who_p)) != OK) 
			ERROR_EXIT(ret);
		SVRDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(rkp));

		if((ret =sys_setpname(m_in.M3_ENDPT,m_in.M3_NAME)) != OK) 
			ERROR_EXIT(ret);

		if((ret =sys_procinfo(who_p)) != OK) 
			ERROR_EXIT(ret);
		SVRDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(rkp));
		
		break;
	default:
		ERROR_RETURN(EMOLINVAL);
		break;	
  }
  return(ret);
}
