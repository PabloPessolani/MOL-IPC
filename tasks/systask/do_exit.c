/* The kernel call implemented in this file:
 *   m_type:	SYS_EXIT
 *
 * The parameters for this kernel call are:
 *    m1_i1:	PR_ENDPT		(slot number of exiting process)
 */

#include "systask.h"

#if USE_EXIT

/*===========================================================================*
 *				do_exit					     *
 *===========================================================================*/
int do_exit(message *m_ptr)			/* pointer to request message */
{
/* Handle sys_exit. A user process has exited or a system process requests 
 * to exit. Only the PM can request other process slots to be cleared.
 * The routine to clean up a process table slot cancels outstanding timers, 
 * possibly removes the process from the message queues, and resets certain 
 * process table fields to the default values.
 */
	proc_usr_t *proc_ptr;		/* exiting process pointer */
	priv_usr_t  *priv_ptr;
	int proc_nr, proc_ep;
  	int rcode, flags;
	message sp_msg, *sp_ptr; 		/* for use with SPREAD */
	slot_t   *sp; 		/* PST from the merged partition  */

	proc_ep =  m_ptr->PR_ENDPT;
	TASKDEBUG("proc_ep=%d\n", proc_ep);

	proc_ptr = ENDPOINT2PTR(proc_ep);
	proc_nr = _ENDPOINT_P(proc_ep);
	
#ifdef ALLOC_LOCAL_TABLE 			
	rcode = mnx_getprocinfo(dc_ptr->dc_dcid, proc_nr, &proc[proc_nr+dc_ptr->dc_nr_tasks]);
	if( rcode < 0) ERROR_RETURN(rcode );
#else /* ALLOC_LOCAL_TABLE */			
	proc_ptr = (proc_usr_t *) PROC_MAPPED(proc_nr);		
	rcode = OK;
#endif /* ALLOC_LOCAL_TABLE */	
	TASKDEBUG("before " PROC_USR_FORMAT,PROC_USR_FIELDS(proc_ptr));

//	if( proc_ptr->p_rts_flags == SLOT_FREE) 
//		ERROR_RETURN(EMOLNOTBIND);
	
//	if( proc_ptr->p_nodeid != local_nodeid) 
//		ERROR_RETURN(EMOLBADNODEID);	

	/* save flags before destructive unbind */
	priv_ptr = PROC2PRIV(proc_nr);

	/*  UPDATE SLOT table */
	TASKDEBUG("dc_nr_tasks=%d proc_nr=%d dc_nr_sysprocs=%d\n",
		dc_ptr->dc_nr_tasks, proc_nr, dc_ptr->dc_nr_sysprocs);
	if( (dc_ptr->dc_nr_tasks + proc_nr)  < dc_ptr->dc_nr_sysprocs){
		sp = &slot[proc_nr+dc_ptr->dc_nr_tasks];
		if( sp->s_owner == local_nodeid) {
			rcode = mcast_exit_proc(proc_ptr);
			if( rcode < 0) 	ERROR_RETURN(rcode);
			flags = 0;
		} else {
			SET_BIT(flags, BIT_REMOTE); 
		}
		sp->s_endpoint = NONE;
		sp->s_flags = 0;
		sp->s_owner = NO_PRIMARY_MBR;
		TASKDEBUG(SLOTS_FORMAT, SLOTS_FIELDS(sp));
			TASKDEBUG("proc_nr=%d %s\n",proc_nr, proc_ptr->p_name);
		/* Inform other SYSTASKs through SPREAD the system process exiting */
	}
	
	/* Did proc_ptr be killed by a signal sent by another process? */
	if( !TEST_BIT(proc_ptr->p_misc_flags, MIS_BIT_KILLED)){ /* NO, it exits by itself */
		SET_BIT(proc_ptr->p_misc_flags, MIS_BIT_KILLED);
		TASKDEBUG("UNBIND proc_ep=%d\n",proc_ep);
		rcode = mnx_unbind(dc_ptr->dc_dcid, proc_ep);
		if( rcode < 0 && rcode != EMOLNOTBIND) 	ERROR_RETURN(rcode);
	}else{
		TASKDEBUG("NOTIFY proc_ep=%d\n",proc_ep);
		rcode = mnx_ntfy_value(PM_PROC_NR, proc_ep, PM_PROC_NR);
		if( rcode < 0) 	ERROR_RETURN(rcode);		
#define TO_WAIT4UNBIND	100 /* miliseconds */
		TASKDEBUG("mnx_wait4unbind_T\n");
		do { 
			rcode = mnx_wait4unbind_T(proc_ep, TO_WAIT4UNBIND);
			TASKDEBUG("mnx_wait4unbind_T  rcode=%d\n", rcode);
			if (rcode == EMOLTIMEDOUT) {
				TASKDEBUG("mnx_wait4unbind_T TIMEOUT\n");
				continue ;
			}else if( rcode < 0) 
				ERROR_EXIT(EXIT_FAILURE);
		} while	(rcode < OK); 
	}
	TASKDEBUG("endpoint %d unbound\n", proc_ep);
	
		
	if( !TEST_BIT(flags, BIT_REMOTE)){	/* Local Process */	
		free_slots++;
		TASKDEBUG("proc_nr=%d free_slots=%d\n",proc_nr, free_slots);
	  	reset_timer(&priv_ptr->s_alarm_timer);
	}

#ifdef ALLOC_LOCAL_TABLE 			
	rcode = mnx_getprocinfo(dc_ptr->dc_dcid, proc_nr, &proc[proc_nr+dc_ptr->dc_nr_tasks]);
	if( rcode < 0) 	ERROR_RETURN(rcode);
#else /* ALLOC_LOCAL_TABLE */			
	proc_ptr = (proc_usr_t *) PROC_MAPPED(proc_nr);	
	rcode = OK;
#endif /* ALLOC_LOCAL_TABLE */		
	TASKDEBUG("after " PROC_USR_FORMAT,PROC_USR_FIELDS(proc_ptr));

	return(OK);
}


#endif /* USE_EXIT */

