/* The kernel call implemented in this file:
 *   m_type:	SYS_NICE
 *
 * The parameters for this kernel call are:
 *    m1_i1:	PR_ENDPT   	process number to change priority
 *    m1_i2:	PR_PRIORITY	the new priority
 */

#include "systask.h"

#if USE_NICE

/*===========================================================================*
 *				  do_nice				     *
 *===========================================================================*/
int do_nice(message *m_ptr)
{
/* Change process priority or stop the process. */
  proc_usr_t *proc_ptr;		/* process pointer */
  int proc_nr, proc_ep;
  int rcode, prio;


	proc_ep =  m_ptr->PR_ENDPT;
	prio =  m_ptr->PR_PRIORITY;

	proc_nr = _ENDPOINT_P(proc_ep);
	proc_ptr = PROC2PTR(proc_nr);
	TASKDEBUG("proc_ep=%d prio=%d lpid=%d\n", proc_ep, prio, proc_ptr->p_lpid);

#ifdef ALLOC_LOCAL_TABLE 			
	rcode = mnx_getprocinfo(dc_ptr->dc_dcid, proc_nr, &proc[proc_nr+dc_ptr->dc_nr_tasks]);
	if( rcode < 0) 	ERROR_RETURN(rcode);
#else /* ALLOC_LOCAL_TABLE */			
	proc_ptr = (proc_usr_t *) PROC_MAPPED(_ENDPOINT_P(proc_nr));
	rcode = OK;	
#endif /* ALLOC_LOCAL_TABLE */
	TASKDEBUG("before " PROC_USR_FORMAT,PROC_USR_FIELDS(proc_ptr));

	if( proc_ptr->p_rts_flags == SLOT_FREE) 	ERROR_RETURN(EMOLNOTBIND); 
	if( (proc_ptr->p_nr+ dc_ptr->dc_nr_tasks)  < dc_ptr->dc_nr_sysprocs) 	ERROR_RETURN(EMOLPERM); 
	
//	if (prio == PRIO_STOP) {
//	      	/* Take process off the scheduling queues. */
//     		proc_ptr->p_rts_flags |= NO_PRIORITY;
//     		return(OK);
//  	}

	TASKDEBUG("PRIO_MIN=%d PRIO_MAX=%d\n", PRIO_MIN, PRIO_MAX);

	if( prio < PRIO_MIN || prio > PRIO_MAX)	ERROR_RETURN(EMOLINVAL);

	rcode = setpriority(PRIO_PROCESS, proc_ptr->p_lpid, prio); 
	if( rcode < 0) 	ERROR_RETURN(rcode);

#ifdef ALLOC_LOCAL_TABLE 			
	rcode = mnx_getprocinfo(dc_ptr->dc_dcid, proc_nr, &proc[proc_nr+dc_ptr->dc_nr_tasks]);
	if( rcode < 0) 	ERROR_RETURN(rcode);
#else /* ALLOC_LOCAL_TABLE */			
	proc_ptr = (proc_usr_t *) PROC_MAPPED(_ENDPOINT_P(proc_nr));
	rcode = OK;	
#endif /* ALLOC_LOCAL_TABLE */
	TASKDEBUG("after " PROC_USR_FORMAT,PROC_USR_FIELDS(proc_ptr));

	prio = getpriority(PRIO_PROCESS, proc_ptr->p_lpid);
	if( prio == (-1) && errno != 0) ERROR_RETURN(errno);

	TASKDEBUG("Priority of %d=%d\n",proc_ptr->p_lpid, prio );
	m_ptr->PR_PRIORITY=prio;

  return(OK);
}

#endif /* USE_NICE */

