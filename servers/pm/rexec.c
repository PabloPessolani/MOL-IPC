/*
 This file handles the REXEC system call.  
  */

#include "pm.h"
#define PATH_MAX        255	/* # chars in a path name */
#define FORK_WAIT_MS 	1000

/*===========================================================================*
 *				do_rexec					     *
 * Tell remote SYSTASK to request to execute a process 	*
 * tell local SYSTASK to bind the new process as remote    *
* M7_ENDPT1  		m7_i1	the remote endpoint		*
* M7_BIND_TYPE	m7_i2	bind type 				*
* 	LCL_BIND		means bind in the remote node a local proc *	
* 	RMT_BIND	NOT VALID because any proc need to exec !!! *	
* 	BKUP_BIND	a remote process is a backup of other 	*
* 	REPLICA_BIND	a remote process is a replica of other     *
* M7_NODEID	    	m7_i3	indicates a NODEID 		*
*  M7_LEN  	  	m7_i4	len in bytes of a copy argv[]	*
* M7_ARGV_PTR	 m7_p1	pointer to a copy of argv 	*
* return: PID of the remote running process or  error	*
* Remote means in other node than PM				*
 *===========================================================================*/
int do_rexec(void)
{
	int rcode, rmt_nodeid, rmt_ep, rmt_nr, rmt_pid, bind_type, arg_len;
	char **arg_v;
  	struct mproc *rmp, *rmr;
	message *m_ptr;
static 	message m;

	rmt_nodeid 	= m_in.M7_NODEID;
	rmt_ep		= m_in.M7_ENDPT1; 
	bind_type   = m_in.M7_BIND_TYPE;
	arg_v  		= (char **) m_in.M7_ARGV_PTR;
	arg_len   	= m_in.M7_LEN;
	SVRDEBUG("rmt_ep=%d rmt_nodeid=%d bind_type=%d who_e=%d  arg_len=%d\n", 
					rmt_ep, rmt_nodeid, bind_type, who_e, arg_len );

	if( bind_type < 0 || bind_type > MAX_BIND_TYPE || bind_type == RMT_BIND)
		ERROR_RETURN(EMOLINVAL);				

	if( arg_len > MAXCOPYBUF)
		ERROR_RETURN(EMOL2BIG);
	
	/* request REMOTE SYSTASK to execute a process and LOCAL SYSTASK to bind the process*/
	rmt_ep = sys_rexec(rmt_nodeid, rmt_ep, bind_type, who_e, arg_len, arg_v);
	if(rmt_ep < 0) ERROR_RETURN(rmt_ep);

	rmt_nr = _ENDPOINT_P(rmt_ep);	
	SVRDEBUG("rmt_nr=%d rmt_ep=%d\n", rmt_nr, rmt_ep);

	rmr = &mproc[rmt_nr];
	rmp = &mproc[who_p];	/* Parent is Who makes the rexec request  */

	if( 	(bind_type == LCL_BIND) 
    	|| !(rmr->mp_flags & (IN_USE | RMT_PROC ))) {
		
		/* Create the process descriptor at PM level */
		
		/* Set up the child ; copy its 'mproc' slot from parent. */
		*rmr = *rmp;					/* copy parent's process slot to remote's */
		rmr->mp_parent = who_p;			/* record child's parent 			*/

		if((rmt_nr+dc_ptr->dc_nr_tasks) < dc_ptr->dc_nr_sysprocs) 
			rmr->mp_flags = (IN_USE | RMT_PROC | PRIV_PROC);
		else 
			rmr->mp_flags = (IN_USE | RMT_PROC );
		
		rmr->mp_child_utime 	= 0;		/* reset administration */
		rmr->mp_child_stime 	= 0;		/* reset administration */
		rmr->mp_exitstatus 		= 0;
		rmr->mp_sigstatus 		= 0;
		/* Find a free pid for the child and put it in the table. */
		rmt_pid = get_free_pid();
		SVRDEBUG("rmt_pid=%d\n", rmt_pid);
		rmr->mp_pid = rmt_pid;				/* assign pid to remote  */
	}
	
	SVRDEBUG(PM_PROC_FORMAT,PM_PROC_FIELDS(rmr));

	/* Wakeup the remote process to execvp() the filename */
	if( rmr->mp_flags & RMT_PROC){
		rcode = mnx_notify(rmt_ep);
		if(rcode < 0) ERROR_RETURN(rcode);
	}
	
	/* Tell SYSTASK to copy the kernel entry to kproc[rmt_nr]   	*/
  	if((rcode =sys_procinfo(rmt_nr)) != OK) 
		ERROR_EXIT(rcode);
	
	/* return to USER space caller the new pid of the remote process */
	return(rmt_pid);	
	
}

int do_migrproc(void){
	int rmt_nodeid;

	SVRDEBUG("rmt_nodeid=%d\n", rmt_nodeid);

}
