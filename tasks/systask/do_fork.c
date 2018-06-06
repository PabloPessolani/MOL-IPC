/* The kernel call implemented in this file:
 *   m_type:	SYS_FORK
 *
 * The parameters for this kernel call are:
 *    m1_i1:	PR_LPID   child Linux PID	
 * OUTPUT:
 *    m1_i2:	PR_ENDPT child endpoint
 */
#include "systask.h"


#if USE_FORK
#define	FORK_SLEEP_SECS		1

int get_free_proc(void);

/*===========================================================================*
 *				do_fork					     *
 *===========================================================================*/
int do_fork(message *m_ptr)	
{
/* Handle sys_fork().  PR_ENDPT has forked.  The child is PR_SLOT. */
	proc_usr_t *caller_ptr;
  proc_usr_t *child_ptr;		/* child process pointer */
  priv_usr_t *cpriv_ptr;		/* child privileges pointer */
  int child_nr, child_lpid, child_ep;
  int rcode, child_gen;
  	slot_t   *sp; 		/* PST from the merged partition  */

/*!!!!!!!!!!!!!! VERIFICAR QUE EL M_SOURCE SEA PM !!!!!!!!!!!!!!!!!!!!*/
//	if( parent_ptr->p_rts_flags == SLOT_FREE) 	
//		return(EMOLNOTBIND);

  	caller_ptr = PROC2PTR(who_p);
	if(caller_ptr->p_endpoint != PM_PROC_NR) 
		ERROR_RETURN(EMOLACCES);

	TASKDEBUG("child_lpid=%d PMnodeid=%d\n",  m_ptr->PR_LPID, caller_ptr->p_nodeid);

	child_nr  = get_free_proc();
	if(child_nr < 0) ERROR_RETURN(child_nr);

	child_ptr = PROC2PTR(child_nr);
	child_lpid = m_ptr->PR_LPID;
	
	/* Get info about the child slot in KERNEL */	
#ifdef ALLOC_LOCAL_TABLE 			
	rcode = mnx_getprocinfo(dc_ptr->dc_dcid, child_nr, child_ptr);
	if( rcode < 0) ERROR_RETURN(rcode);
#else /* ALLOC_LOCAL_TABLE */			
	child_ptr = (proc_usr_t *) PROC_MAPPED(child_nr);
	rcode = OK;
#endif /* ALLOC_LOCAL_TABLE */
	TASKDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(child_ptr));
	
	/* Test if the child was binded to SYSTASK */	
	if( child_ptr->p_rts_flags != SLOT_FREE) 	return(EMOLBUSY);
	
	/* Register the process to the kernel */
	child_gen = _ENDPOINT_G(child_ptr->p_endpoint) + 1;
	child_ep = _ENDPOINT(child_gen, child_ptr->p_nr);
	TASKDEBUG("bind dcid=%d child_lpid=%d, child_nr=%d  child_ep=%d child_gen=%d\n", 
		dc_ptr->dc_dcid, child_lpid, child_nr, child_ep, child_gen);
	rcode  = mnx_lclbind(dc_ptr->dc_dcid, child_lpid, child_ep);
	TASKDEBUG("child_ep=%d rcode=%d\n", child_ep, rcode);
	if(child_ep != rcode) ERROR_RETURN(rcode);
	
	/* Get process information from kernel */
#ifdef ALLOC_LOCAL_TABLE 			
	rcode = mnx_getprocinfo(dc_ptr->dc_dcid, child_nr, child_ptr);
	if( rcode < 0) ERROR_RETURN(rcode);
#else /* ALLOC_LOCAL_TABLE */			
	child_ptr = (proc_usr_t *) PROC_MAPPED(child_nr);	
	rcode = OK;
#endif /* ALLOC_LOCAL_TABLE */
	
	TASKDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(child_ptr));

	/* Get process privileges from kernel */
	child_ep = child_ptr->p_endpoint;
	TASKDEBUG("child_ep=%d\n", child_ep);
	cpriv_ptr =  PROC2PRIV(child_nr);
	rcode = mnx_getpriv(dc_ptr->dc_dcid, child_ep, cpriv_ptr);
	if( rcode < 0) ERROR_RETURN(rcode);
	TASKDEBUG(PRIV_USR_FORMAT,PRIV_USR_FIELDS(cpriv_ptr));

	m_ptr->PR_ENDPT = child_ep;
	m_ptr->PR_SLOT  = child_nr;

	if ((child_nr+dc_ptr->dc_nr_tasks) < dc_ptr->dc_nr_sysprocs ){
		sp = &slot[child_nr+dc_ptr->dc_nr_tasks];
		sp->s_endpoint = child_ep;
		sp->s_flags = 0;
		sp->s_owner = local_nodeid;
		TASKDEBUG(SLOTS_FORMAT, SLOTS_FIELDS(sp));
		TASKDEBUG("child_nr=%d %s\n",child_nr, child_ptr->p_name);
		if( child_ptr->p_nodeid != local_nodeid) return(OK);
		rcode = mcast_fork_proc(child_ptr);
		if( rcode < 0) ERROR_RETURN(rcode );
	}else{
		free_slots--;
		TASKDEBUG("child_nr=%d free_slots=%d\n",child_nr, free_slots);
	}

  return(OK);
}

/*===========================================================================*
 *				get_free_proc									     *
 *===========================================================================*/
int get_free_proc(void)
{
	int n, rcode, retries,  leftover;
	proc_usr_t *child_ptr;		/* child process pointer */
	message msg;
	time_t	now;
  	slot_t   *sp; 		/* PST from the merged partition  */

	/*------------------------------------------------------------------------------------------
	*  If the system have not does not have enough slots 
	* Multicast SYS_SLOT_REQUEST to all nodes 
	* and return EMOLNOSPC
	*-------------------------------------------------------------------------------------------*/
	pthread_mutex_lock(&sys_mutex);

	if( free_slots < free_slots_low ) {
		if( owned_slots < max_owned_slots ) {		/* do I achieve the maximum threshold  ? */
			/*  if there pending donation requests? */
			/*  if there pending nodes waiting to receive SYS_PUT_STATUS from primary ? */
			if( bm_donors == 0 && bm_waitsts == 0) { 															
				if( CEILING(max_owned_slots, init_nodes) > owned_slots )
					leftover = CEILING(max_owned_slots,init_nodes) - owned_slots;
				else
					leftover = free_slots_low - free_slots;
				TASKDEBUG("leftover=%d free_slots=%d free_slots_low=%d bm_donors=%X\n",
					leftover, free_slots, free_slots_low, bm_donors);
				TASKDEBUG("owned_slots=%d max_owned_slots=%d\n",
					owned_slots, max_owned_slots);
				now = time(NULL); 
				if( ( now - last_rqst) > SLOTS_PAUSE) {	
					rcode = mcast_rqst_slots(leftover);
					last_rqst = now;
				}
			}
		}
	}
	
	if(free_slots == 0) {
		pthread_mutex_unlock(&sys_mutex);
		ERROR_RETURN(EMOLNOSPC);
	}

	/* Searches for the next free slot 	*/
	next_child = (dc_ptr->dc_nr_sysprocs);
	do { 
#ifdef ALLOC_LOCAL_TABLE 			
		child_ptr = &proc[next_child];
#else /* ALLOC_LOCAL_TABLE */			
		child_ptr = (proc_usr_t *) PROC_MAPPED(next_child-dc_ptr->dc_nr_tasks);						
#endif /* ALLOC_LOCAL_TABLE */		
		if( (child_ptr->p_rts_flags == SLOT_FREE) && 
		    (slot[next_child].s_owner == local_nodeid)) {
			TASKDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(child_ptr));
			TASKDEBUG("next_child=%d free_slots=%d return=%d s_owner=%d\n",
				next_child, free_slots,next_child-dc_ptr->dc_nr_tasks,
				slot[next_child].s_owner);
			pthread_mutex_unlock(&sys_mutex);
			return(next_child-dc_ptr->dc_nr_tasks);
		}
		next_child++;
	}while(next_child < (dc_ptr->dc_nr_tasks + dc_ptr->dc_nr_procs));
	
	/* NEVER HERE BUT */
	/* NEXT CODE IS ONLY TO CHECK INCONSITENCY*/
		
	TASKDEBUG("free_slots=%d\n", free_slots);
	next_child = (dc_ptr->dc_nr_sysprocs);
	do { 
#ifdef ALLOC_LOCAL_TABLE 			
		child_ptr = &proc[next_child];
#else /* ALLOC_LOCAL_TABLE */			
		child_ptr = (proc_usr_t *) PROC_MAPPED(next_child-dc_ptr->dc_nr_tasks);						
#endif /* ALLOC_LOCAL_TABLE */		
		if(slot[next_child].s_owner == local_nodeid) {
			sp = &slot[next_child];
			TASKDEBUG(SLOTS_FORMAT, SLOTS_FIELDS(sp));
			TASKDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(child_ptr));
		}
		next_child++;
	}while(next_child < (dc_ptr->dc_nr_tasks + dc_ptr->dc_nr_procs));
	pthread_mutex_unlock(&sys_mutex);

	assert(0);
	ERROR_RETURN(EMOLGENERIC);
}

#endif /* USE_FORK */

