/* 
The kernel call implemented in this file:
 *   m_type:	SYS_BINDPROC
 *
 * The parameters for this kernel call are:
 *    PR_SLOT	 (child's process table slot)	
 *    PR_LPID   child Linux PID	
 
The kernel call implemented in this file:
 *   m_type:	SYS_UNBIND
 *
 * The parameters for this kernel call are:
 *    PR_ENDPT	

 */
#include "systask.h"

#if USE_BINDPROC

/*===========================================================================*
 *				do_bindproc				     *
 * A System process:
 *		- SERVER/TASK: is just registered to the kernel but not to SYSTASK.
 *		- REMOTE USER: it will be registered to the kernel and then to SYSTASK 
 * INPUT: M3 type message
 *   	M3_ENDPT
 *      M3_LPID (PID or NODEID)
 * 	M3_OPER must be  (minix/com.h)
 * 		SELF_BIND	0: pid, local_nodeid, self endpoint
 * 		LCL_BIND		1: pid, local_nodeid, self endpoint
 * 		RMT_BIND	2: nodeid
 * 		BKUP_BIND	3: pid, local_nodeid, self endpoint
 * 		REPLICA_BIND 4
 *     m3_ca1: Process namespace
 *
 * OUTPUT: M1 type message
 *       m_type: rcode
 *       M1_ENDPT
 *       
 *===========================================================================*/
int do_bindproc(message *m_ptr)	
{
  proc_usr_t *sysproc_ptr;		/* sysproc process pointer */
  priv_usr_t *sppriv_ptr;		/* sysproc privileges pointer */
  int sysproc_nr, sysproc_lpid, sysproc_ep, sysproc_oper, sysproc_nodeid;
  int rcode;
  message sp_msg, *sp_ptr; 		/* for use with SPREAD */
 	slot_t   *sp; 		/* PST from the merged partition  */

	TASKDEBUG(MSG3_FORMAT,  MSG3_FIELDS(m_ptr));
	sysproc_ep 		= m_ptr->M3_ENDPT;
	sysproc_nr		= _ENDPOINT_P(sysproc_ep);
	sysproc_nodeid  = sysproc_lpid = m_ptr->M3_LPID;
	sysproc_oper	= (int) m_ptr->M3_OPER; 
	TASKDEBUG("sysproc_lpid=%d, sysproc_ep=%d sysproc_nr=%d sysproc_oper=%X \n", 
		sysproc_lpid, sysproc_ep, sysproc_nr, sysproc_oper);
	if( sysproc_oper < SELF_BIND || sysproc_oper > REPLICA_BIND) 
		ERROR_RETURN(EMOLBADRANGE);
	
	CHECK_P_NR(sysproc_nr);		
	sysproc_ptr 	= PROC2PTR(sysproc_nr);
	
	/* GET process information from kernel */
#ifdef ALLOC_LOCAL_TABLE 			
	rcode = mnx_getprocinfo(dc_ptr->dc_dcid, sysproc_nr, sysproc_ptr);
	if( rcode < 0) ERROR_RETURN(rcode);
#else /* ALLOC_LOCAL_TABLE */			
	sysproc_ptr = (proc_usr_t *) PROC_MAPPED(sysproc_nr);
	rcode = OK;	
#endif /* ALLOC_LOCAL_TABLE */
	TASKDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(sysproc_ptr));

	sp = &slot[sysproc_nr+dc_ptr->dc_nr_tasks];
	TASKDEBUG(SLOTS_FORMAT, SLOTS_FIELDS(sp));
	
	if( sysproc_oper == RMT_BIND) {
		if(sp->s_owner != sysproc_nodeid) {
			TASKDEBUG("slot=%d owned by node=%d\n", 
				sysproc_nr+dc_ptr->dc_nr_tasks, sp->s_owner);
			ERROR_RETURN(EMOLBADOWNER);
		}
	} else {
		if( sp->s_owner != NO_PRIMARY_MBR){ 
			if(sp->s_owner != local_nodeid ) {
				TASKDEBUG("ERROR: slot=%d owned by node=%d\n", 
					sysproc_nr+dc_ptr->dc_nr_tasks, sp->s_owner);
				ERROR_RETURN(EMOLBADOWNER);
			}
		}
	}
	/* REMOTE USER PROCESSES */
	if( (sysproc_nr+dc_ptr->dc_nr_tasks) >= dc_ptr->dc_nr_sysprocs) {
		/* user processes can be bound only if they are REMOTE */
		if( sysproc_oper != RMT_BIND )
			ERROR_RETURN(EMOLPERM);
		/* The NODE must other than local node  */
		if( sysproc_nodeid == local_nodeid) 
			ERROR_RETURN(EMOLBADNODEID);
		if(  TEST_BIT(sysproc_ptr->p_rts_flags, BIT_SLOT_FREE)) {
			/* Any process is bound to this slot */
			TASKDEBUG("Any process is bound to endpoint=%d (new is %s)\n", 
				sysproc_ep, m_ptr->m3_ca1 );
			rcode = mnx_rmtbind(dc_ptr->dc_dcid,m_ptr->m3_ca1,
						sysproc_ep,sysproc_nodeid);
			if( rcode < 0) ERROR_RETURN(rcode);
		}else{
			/* A LOCAL process is already bound to this slot */
			if( sysproc_ptr->p_nodeid == local_nodeid)
				TASKDEBUG("A LOCAL process is already bound to this slot:%s\n",
					sysproc_ptr->p_name);
				ERROR_RETURN(EMOLSLOTUSED);
			/* A REMOTE process is already bound to this slot */			
			if(sysproc_ptr->p_endpoint == sysproc_ep){
				TASKDEBUG("A REMOTE process is already bound to this slot:%s\n",
					sysproc_ptr->p_name);
				/* The slot has same endpoint */
				if(sysproc_ptr->p_nodeid != sysproc_nodeid){
					/* the process also has different nodeid : MIGRATE IT */
					TASKDEBUG("the process also has different nodeid %d: MIGRATE IT \n",
						sysproc_ptr->p_nodeid);
					mnx_migr_start(dc_ptr->dc_dcid, sysproc_ptr->p_endpoint);
					mnx_migr_commit(PROC_NO_PID, dc_ptr->dc_dcid, 
						sysproc_ep, sysproc_nodeid);			
				}else{
					/* same node, same endpoint */
					TASKDEBUG("Remote process already bound\n");
					TASKDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(sysproc_ptr));
					m_ptr->M1_ENDPT = sysproc_ep;
					m_ptr->M1_OPER  = sysproc_oper;
				}
			} else{
				/*Another Remote process was using the slot: Unbind it before new bind */
				TASKDEBUG("Another Remote process was using the slot: Unbind it before new bind %s\n",
						sysproc_ptr->p_name);						
				mnx_unbind(dc_ptr->dc_dcid,sysproc_ptr->p_endpoint);
				rcode = mnx_rmtbind(dc_ptr->dc_dcid,basename(m_ptr->m3_ca1),
							sysproc_ep,sysproc_nodeid);
				if( rcode < 0) ERROR_RETURN(rcode);
			}
		}
#ifdef ALLOC_LOCAL_TABLE 			
		rcode = mnx_getprocinfo(dc_ptr->dc_dcid, sysproc_nr, sysproc_ptr);
		if( rcode < 0) ERROR_RETURN(rcode);
#else /* ALLOC_LOCAL_TABLE */			
		sysproc_ptr = (proc_usr_t *) PROC_MAPPED(sysproc_nr);	
		rcode = OK;
#endif /* ALLOC_LOCAL_TABLE */
		TASKDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(sysproc_ptr));
		return(OK);
	}

#ifdef ALLOC_LOCAL_TABLE 			
	rcode = mnx_getprocinfo(dc_ptr->dc_dcid, sysproc_nr, sysproc_ptr);
	if( rcode < 0) ERROR_RETURN(rcode);
#else /* ALLOC_LOCAL_TABLE */			
	sysproc_ptr = (proc_usr_t *) PROC_MAPPED(sysproc_nr);	
	rcode = OK;
#endif /* ALLOC_LOCAL_TABLE */
	TASKDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(sysproc_ptr))
	
	if(  TEST_BIT(sysproc_ptr->p_rts_flags, BIT_SLOT_FREE)) 
		ERROR_RETURN(EMOLNOTBIND);
		
	if( sysproc_ptr->p_nodeid == local_nodeid) 
		strncpy(sysproc_ptr->p_name, basename(m_ptr->M3_NAME),(M3_STRING-1));

	TASKDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(sysproc_ptr))
	
	/* Get privileges information from kernel */
	sysproc_ep = sysproc_ptr->p_endpoint;
	sppriv_ptr =  PROC2PRIV(sysproc_nr);
	rcode = mnx_getpriv(dc_ptr->dc_dcid, sysproc_ep, sppriv_ptr);
	if( rcode < 0) ERROR_RETURN(rcode);
	TASKDEBUG(PRIV_USR_FORMAT,PRIV_USR_FIELDS(sppriv_ptr));

	sp->s_endpoint = sysproc_ep;
	sp->s_flags = 0;
	sp->s_owner = sysproc_ptr->p_nodeid;
	TASKDEBUG(SLOTS_FORMAT, SLOTS_FIELDS(sp));
	
	m_ptr->M1_ENDPT = sysproc_ep;
	m_ptr->M1_OPER  = sysproc_oper;
	
	if(sysproc_ptr->p_nodeid != local_nodeid) {
		return(OK);
	}
	/* Inform other SYSTASK through SPREAD the  process binding */
	rcode = mcast_bind_proc(sysproc_ptr);
	if( rcode < 0) ERROR_RETURN(rcode);

  return(OK);
}

#endif /* USE_BINDPROC */

#if USE_SETPNAME
int do_setpname(message *m_ptr)	
{
	proc_usr_t *sysproc_ptr;		/* sysproc process pointer */
	int sysproc_nr, sysproc_ep;
	int rcode;

	TASKDEBUG(MSG3_FORMAT,  MSG3_FIELDS(m_ptr));
	sysproc_ep 		= m_ptr->M3_ENDPT;
	sysproc_nr		= _ENDPOINT_P(sysproc_ep);
	
	CHECK_P_NR(sysproc_nr);		
	sysproc_ptr 	= PROC2PTR(sysproc_nr);
	
	/* GET process information from kernel */
#ifdef ALLOC_LOCAL_TABLE 			
	rcode = mnx_getprocinfo(dc_ptr->dc_dcid, sysproc_nr, sysproc_ptr);
	if( rcode < 0) ERROR_RETURN(rcode);
#else /* ALLOC_LOCAL_TABLE */			
	sysproc_ptr = (proc_usr_t *) PROC_MAPPED(sysproc_nr);
#endif /* ALLOC_LOCAL_TABLE */
	TASKDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(sysproc_ptr));
	
	if(  TEST_BIT(sysproc_ptr->p_rts_flags, BIT_SLOT_FREE)) 
		ERROR_RETURN(EMOLNOTBIND);
	
	strncpy(sysproc_ptr->p_name,m_ptr->M3_NAME,(M3_STRING-1));
	
	return(OK);
}
#endif /* USE_SETPNAME */

#if USE_UNBIND
/*===========================================================================*
 *				do_unbind 					     *
 *===========================================================================*/
int do_unbind(message *m_ptr)			/* pointer to request message */
{

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

	if( proc_ptr->p_rts_flags == SLOT_FREE) 
		ERROR_RETURN(EMOLNOTBIND);
	
	/* save flags before destructive unbind */
	flags = proc_ptr->p_rts_flags;
	priv_ptr = PROC2PRIV(proc_nr);

	/*  UPDATE SLOT table */
	TASKDEBUG("dc_nr_tasks=%d proc_nr=%d dc_nr_sysprocs=%d\n",
		dc_ptr->dc_nr_tasks, proc_nr, dc_ptr->dc_nr_sysprocs);
	if( (dc_ptr->dc_nr_tasks + proc_nr)  < dc_ptr->dc_nr_sysprocs){
		sp = &slot[proc_nr+dc_ptr->dc_nr_tasks];
		sp->s_endpoint = NONE;
		sp->s_flags = 0;
		sp->s_owner = NO_PRIMARY_MBR;
		TASKDEBUG(SLOTS_FORMAT, SLOTS_FIELDS(sp));
			TASKDEBUG("proc_nr=%d %s\n",proc_nr, proc_ptr->p_name);
		/* Inform other SYSTASKs through SPREAD the system process exiting */
		if( proc_ptr->p_nodeid == local_nodeid) {
			rcode = mcast_exit_proc(proc_ptr);
			if( rcode < 0) 	ERROR_RETURN(rcode);
		}
	}
	
	/* Did proc_ptr be killed by a signal sent by another process? */
	if( !TEST_BIT(proc_ptr->p_misc_flags, MIS_BIT_KILLED)){ /* NO, it exits by itself */
		SET_BIT(proc_ptr->p_misc_flags, MIS_BIT_KILLED);
		TASKDEBUG("UNBIND proc_ep=%d\n",proc_ep);
		rcode = mnx_unbind(dc_ptr->dc_dcid, proc_ep);
		if( rcode < 0) 	ERROR_RETURN(rcode);
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
#endif // USE_UNBIND


