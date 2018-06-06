/* The kernel call implemented in this file:
 *   m_type:	SYS_KILLED
  * The parameters for this kernel call are:
 *    m1_i1:	PR_ENDPT		(slot number of killed process)
 */
#include "systask.h"

#if USE_KILLED


/*===========================================================================*
 *				do_killed					     *
 *===========================================================================*/
int do_killed(message *m_ptr)			/* pointer to request message */
{
/* Handle sys_killed . A user process has been killed and the PM has been advertised of this
 *  PM exited or a system process requests 
 * to exit. Only the PM can request other process slots to be cleared.
 * The routine to clean up a process table slot cancels outstanding timers, 
 * possibly removes the process from the message queues, and resets certain 
 * process table fields to the default values.
 */
  proc_usr_t *proc_ptr;		/* exiting process pointer */
  int proc_nr, proc_ep;
  int rcode;

	proc_ep =  who_e;
	TASKDEBUG("proc_ep=%d\n", proc_ep);

	proc_ptr = ENDPOINT2PTR(proc_ep);
	proc_nr = _ENDPOINT_P(proc_ep);
	if( proc_ptr->p_rts_flags == SLOT_FREE) { /* a process of this DC but not registered to SYSTASK has been killed */
		ERROR_RETURN(EMOLNOTBIND);
	}
	
	/* get the process information from kernel */
#ifdef ALLOC_LOCAL_TABLE 			
	rcode = mnx_getprocinfo(dc_ptr->dc_dcid, proc_nr, &proc[proc_nr+dc_ptr->dc_nr_tasks]);
	if( rcode < 0) 	ERROR_RETURN(rcode);
#else /* ALLOC_LOCAL_TABLE */			
	proc_ptr = (proc_usr_t *) PROC_MAPPED(proc_nr);
	rcode = OK;	
#endif /* ALLOC_LOCAL_TABLE */
	TASKDEBUG("before " PROC_USR_FORMAT,PROC_USR_FIELDS(proc_ptr));
	if( proc_ptr->p_rts_flags == SLOT_FREE) {
		ERROR_RETURN(EMOLNOTBIND); 
	}

	/* as SYS_KILLED is called by a unbinding process there is no need to unbind it in the kernel */
	/* only unbind it in SYSTASK */
	proc_ptr->p_rts_flags = SLOT_FREE;
	
  return(OK);
}


#endif /* USE_KILLED */

