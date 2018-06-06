/* The kernel call implemented in this file:
 *   m_type:	SYS_MIGRPROC
 *
 * The parameters for this kernel call are:
 *    M3_ENDPT endpoint   	
 *    M3_LPID Linux PID
 */
#include "systask.h"

#if USE_MIGRPROC

/*===========================================================================*
 *				do_migrproc				     *
 * A system process has migrate to local node (or a local backup node is now the new primary 
 * m_in.m3_i1- M3_LPID:  local Linux PID
 * m_in.m3_i2- M3_ENDPT: p_endpoint
 *===========================================================================*/
int do_migrproc(message *m_ptr)	
{
  proc_usr_t *sysproc_ptr;		/* sysproc process pointer */
  priv_usr_t *sppriv_ptr;		/* sysproc privileges pointer */
  int sysproc_nr, sysproc_lpid, sysproc_ep, sysproc_oper;
  int rcode;
  message sp_msg, *sp_ptr; 		/* for use with SPREAD */
 	slot_t   *sp; 		/* PST from the merged partition  */


	sysproc_ep 		= m_ptr->M3_SLOT;
	sysproc_lpid 	= m_ptr->M3_LPID;
	sysproc_nr = _ENDPOINT_P(sysproc_ep);
	TASKDEBUG("sysproc_lpid=%d, sysproc_ep=%d sysproc_nr=%d\n", 
		sysproc_lpid, sysproc_ep, sysproc_nr);
	
	CHECK_P_NR(sysproc_nr);		
	sysproc_ptr 	= PROC2PTR(sysproc_nr);
	if( (sysproc_nr+dc_ptr->dc_nr_tasks)  >= dc_ptr->dc_nr_sysprocs){ 
		ERROR_RETURN(EMOLPERM);
//		if( sysproc_ptr->p_rts_flags != SLOT_FREE) 	return(EMOLBUSY);
	}

	/* GET process information from kernel */
#ifdef ALLOC_LOCAL_TABLE 			
	rcode = mnx_getprocinfo(dc_ptr->dc_dcid, sysproc_nr, sysproc_ptr);
	if( rcode < 0) ERROR_RETURN(rcode);
#else /* ALLOC_LOCAL_TABLE */			
	sysproc_ptr = (proc_usr_t *) PROC_MAPPED(sysproc_nr);	
	rcode = OK;	
#endif /* ALLOC_LOCAL_TABLE */	
	TASKDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(sysproc_ptr));
	
	/* The process must be bound first, otherwise fork must be executed  */
	if( sysproc_ptr->p_rts_flags == SLOT_FREE)
			ERROR_RETURN(EMOLNOTBIND);

	if( !TEST_BIT(sysproc_ptr->p_rts_flags, BIT_REMOTE))
			ERROR_RETURN(EMOLRMTPROC);
		
	if( !TEST_BIT(sysproc_ptr->p_misc_flags, MIS_BIT_RMTBACKUP))
			ERROR_RETURN(EMOLBADPROC);
		
	if( !TEST_BIT(sysproc_ptr->p_misc_flags, MIS_BIT_REPLICATED))
			ERROR_RETURN(EMOLBADPROC);

	assert(sysproc_ptr->p_nodeid != local_nodeid);
	
	rcode = mnx_migr_start(dc_ptr->dc_dcid, sysproc_ptr->p_endpoint);
	if( rcode < 0) ERROR_RETURN(rcode);
	
	rcode = mnx_migr_commit(sysproc_lpid, dc_ptr->dc_dcid, 
					sysproc_ptr->p_endpoint, local_nodeid);
	if( rcode < 0) ERROR_RETURN(rcode);

#ifdef ALLOC_LOCAL_TABLE 			
	rcode = mnx_getprocinfo(dc_ptr->dc_dcid, sysproc_nr, sysproc_ptr);
	if( rcode < 0) ERROR_RETURN(rcode);
#else /* ALLOC_LOCAL_TABLE */			
	sysproc_ptr = (proc_usr_t *) PROC_MAPPED(sysproc_nr);	
	rcode = OK;	
#endif /* ALLOC_LOCAL_TABLE */
	
	sp = &slot[sysproc_nr+dc_ptr->dc_nr_tasks];
	sp->s_endpoint = sysproc_ep;
	sp->s_flags = 0;
	sp->s_owner = sysproc_ptr->p_nodeid;
	TASKDEBUG(SLOTS_FORMAT, SLOTS_FIELDS(sp));
	
	/* Inform other SYSTASK through SPREAD the  process migration */
	rcode = mcast_migr_proc(sysproc_ptr);
	if( rcode < 0) ERROR_RETURN(rcode);

  return(OK);
}

#endif /* USE_BINDPROC */

