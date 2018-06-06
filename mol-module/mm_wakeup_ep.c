/*--------------------------------------------------------------*/
/*			mm_wakeup_ep				*/
/*--------------------------------------------------------------*/
asmlinkage long mm_wakeup_ep(int dcid, int proc_ep, int signo)
{

	struct proc *proc_ptr, *other_ptr, *sproxy_ptr;
	dc_desc_t *dc_ptr;
	int caller_nr, caller_pid, caller_ep;
	int dst_nr, dcid;
	int ret;
	struct task_struct *task_ptr;	
	struct timespec *t_ptr;
	message *mptr;

	MOLDEBUG(DBGPARAMS,"dcid=%d proc_ep=%d\n", dcid, proc_ep);
	if( DVS_NOT_INIT() ) 			
		ERROR_RETURN(EMOLDVSINIT);
	if(current_euid() != USER_ROOT) 		
		ERROR_RETURN(EMOLPRIVILEGES);
		
	task_ptr = current;
	caller_pid  = task_pid_nr(current);
	caller_tgid = current->tgid;
	MOLDEBUG(DBGPARAMS,"caller_pid=%d caller_tgid=%d\n", caller_pid, caller_tgid);
	
	if( dcid < 0 || dcid >= dvs.d_nr_dcs) 			
		ERROR_RETURN(EMOLBADDCID);
	dc_ptr 	= &dc[dcid];
	RLOCK_DC(dc_ptr);
	if( dc_ptr->dc_usr.dc_flags)  
		ERROR_RUNLOCK_DC(dc_ptr,EMOLDCNOTRUN);
	RUNLOCK_DC(dc_ptr);	

	/*------------------------------------------
	 * get the destination process number
	*------------------------------------------*/
	dst_nr = _ENDPOINT_P(proc_ep);
	if( dst_nr < (-dc_ptr->dc_usr.dc_nr_tasks)		
	 || dst_nr >= dc_ptr->dc_usr.dc_nr_procs)	
		ERROR_RETURN(EMOLRANGE)
	proc_ptr   = NBR2PTR(dc_ptr, dst_nr);
	if( proc_ptr == NULL) 				
		ERROR_RETURN(EMOLDSTDIED);
	if( caller_ep == proc_ep) 			
		ERROR_RETURN(EMOLENDPOINT);
	if( current == proc_ptr->p_task)
		ERROR_RETURN(EMOLBADPROC);
	
	/*------------------------------------------
	 * check the destination process status
	*------------------------------------------*/
	WLOCK_PROC(proc_ptr);
	MOLDEBUG(DBGPARAMS,"dst_nr=%d proc_ep=%d\n",dst_nr, proc_ptr->p_usr.p_endpoint);
	if (proc_ptr->p_usr.p_endpoint != proc_ep) 	{
		WUNLOCK_PROC(proc_ptr);
		ERROR_RETURN(EMOLENDPOINT);
	}
	if( test_bit(BIT_SLOT_FREE, &proc_ptr->p_usr.p_rts_flags)){
		WUNLOCK_PROC(proc_ptr);
		ERROR_RETURN(EMOLDSTDIED);
	}
	if( proc_ptr->p_usr.p_nodeid < 0 
		|| proc_ptr->p_usr.p_nodeid >= dvs.d_nr_nodes) 	 {
		WUNLOCK_PROC(proc_ptr);
		ERROR_RETURN(EMOLBADNODEID);
	} 
	if( ! test_bit(proc_ptr->p_usr.p_nodeid, &dc_ptr->dc_usr.dc_nodes)){ 	
		WUNLOCK_PROC(proc_ptr);
		ERROR_RETURN(EMOLNODCNODE);
	}
	if( test_bit(BIT_MIGRATE, &proc_ptr->p_usr.p_rts_flags)) {	/*destination is migrating	*/
		MOLDEBUG(GENERIC,"destination is migrating proc_ptr->p_usr.p_rts_flags=%lX\n"
			,proc_ptr->p_usr.p_rts_flags);
		WUNLOCK_PROC(proc_ptr);
		return(EMOLMIGRATE);
	}

	if( IT_IS_REMOTE(proc_ptr)) {	
		WUNLOCK_PROC(proc_ptr);
		return(EMOLRMTPROC);
	} 

	/*---------------------------------------------------------------------------------------------------*/
	/*						LOCAL  								 */
	/*---------------------------------------------------------------------------------------------------*/
	/* Check if 'dst' is blocked waiting something.   */
	if (  proc_ptr->p_usr.p_rts_flags != 0 ) {

		if (test_bit(BIT_SENDING, &proc_ptr->p_usr.p_rts_flags)){
			if( ! test_bit(BIT_RMTOPER, &proc_ptr->p_usr.p_rts_flags)){
				rp = DC_PROC(dc_ptr,(_ENDPOINT_P(proc_ptr->p_usr.p_sendto) - dc_ptr->dc_usr.dc_nr_tasks));
				uproc_ptr = &rp->p_usr;
				MOLDEBUG(INTERNAL,PROC_USR_FORMAT,PROC_USR_FIELDS(uproc_ptr));
				if( proc_ptr->p_usr.p_nr < rp->p_usr.p_nr) {
					WLOCK_PROC(rp); /* Caller LOCK is just locked */
				}else{	
					/* free the callers lock and then lock both ordered */
					WUNLOCK_PROC(proc_ptr);
					WLOCK_PROC(rp);
					WLOCK_PROC(proc_ptr);
				}
				LIST_DEL_INIT(&proc_ptr->p_link); /* remove from queue */
				WUNLOCK_PROC(rp);	
			}
			proc_ptr->p_usr.p_sendto	= NONE;
			clear_bit(BIT_SENDING, &proc_ptr->p_usr.p_rts_flags);
		}
				
		if( test_bit(BIT_RMTOPER, &proc_ptr->p_usr.p_rts_flags)){
			if(proc_ptr->p_usr.p_proxy != NONE) {
				sproxy_ptr = &proxies[proc_ptr->p_usr.p_proxy].px_sproxy;
				WLOCK_PROC(sproxy_ptr);
				LIST_DEL_INIT(&proc_ptr->p_link); /* remove from queue */
				WUNLOCK_PROC(sproxy_ptr);
			}else{
				ERROR_WUNLOCK_PROC(proc_ptr,EMOLPROCSTS);
			}
			clear_bit(BIT_RMTOPER, &proc_ptr->p_usr.p_rts_flags);
			proc_ptr->p_usr.p_proxy = NONE;
		}	

		if( test_bit(BIT_RECEIVING, &proc_ptr->p_usr.p_rts_flags)){
			clear_bit(BIT_RECEIVING, &proc_ptr->p_usr.p_rts_flags);
			proc_ptr->p_usr.p_getfrom	= NONE;
		}
		
		if(proc_ptr->p_usr.p_rts_flags == 0){
			LOCAL_PROC_UP(proc_ptr, EMOLWOKENUP); 
		}else{
			MOLDEBUG(GENERIC,"Process status is p_rts_flags=%lX\n",
				proc_ptr->p_usr.p_rts_flags);				
			set_bit(MIS_BIT_WOKENUP, &proc_ptr->p_usr.p_misc_flags))	
			ERROR_WUNLOCK_PROC(proc_ptr,EMOLPROCSTS);
		}
	} else { 
		MOLDEBUG(GENERIC,"Process is running p_rts_flags=%lX\n",
			proc_ptr->p_usr.p_rts_flags);
		set_bit(MIS_BIT_WOKENUP, &proc_ptr->p_usr.p_misc_flags))	
		ERROR_WUNLOCK_PROC(proc_ptr,EMOLPROCRUN);
	}
	WUNLOCK_PROC(proc_ptr);

	return(OK);
}
