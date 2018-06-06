//****************************************************************/
/*			MINIX OVER LINUX HYPERVISOR 	 	*/
/****************************************************************/


/*--------------------------------------------------------------*/
/*			kernel_warn2proc					*/
/* Send a message to a specified process when a process exit 	*/
/* i.e.: send an EXIT message to PM to deregister the exiting 	*/
/* process from PM which inform FS and SYSTASK			*/
/* ON ENTRY current,dc_ptr, caller_ptr, warn_ptr MUST BE WRITE  LOCKED */
/* ON EXIT  current, dc_ptr ,caller_ptr, warn_ptr WILL BE WRITE  LOCKED */
/*--------------------------------------------------------------*/
int kernel_warn2proc( dc_desc_t *dc_ptr, struct proc *caller_ptr, struct proc *warn_ptr){
	proc_usr_t *wu_ptr, *cu_ptr;
	message *m_ptr;
	struct proc *sproxy_ptr;
	int ret;
	
	
	MOLDEBUG(DBGPARAMS,"caller_ep=%d warn_ep=%d \n", 
		caller_ptr->p_usr.p_endpoint, warn_ptr->p_usr.p_endpoint);
			
	wu_ptr = &warn_ptr->p_usr;
	MOLDEBUG(DBGPARAMS, PROC_USR_FORMAT,PROC_USR_FIELDS(wu_ptr));
	
	if( IT_IS_REMOTE(warn_ptr)) {		
		/*-----------------------------------------------------------------------------------*/	
		/*					REMOTE 							*/
		/*-----------------------------------------------------------------------------------*/	
		sproxy_ptr = NODE2SPROXY(warn_ptr->p_usr.p_nodeid);
		RLOCK_PROC(sproxy_ptr);
		do {
			ret = OK;
			if( sproxy_ptr->p_usr.p_lpid == PROC_NO_PID)		
				{ret = EMOLNOPROXY ;break;}
			if( test_bit( BIT_SLOT_FREE, &sproxy_ptr->p_usr.p_rts_flags))	
				{ret = EMOLNOPROXY ;break;} 
			if( !test_bit(MIS_BIT_PROXY, &sproxy_ptr->p_usr.p_misc_flags))	
				{ret = EMOLNOPROXY;break;} 
			if( !test_bit(MIS_BIT_CONNECTED, &sproxy_ptr->p_usr.p_misc_flags))	
				{ret = EMOLNOTCONN;break;}  
		} while(0);
		RUNLOCK_PROC(sproxy_ptr);
 		if(ret) {							
			WUNLOCK_PROC2(caller_ptr, warn_ptr);
			ERROR_RETURN(ret);
		}

		clear_bit(BIT_RECEIVING, &warn_ptr->p_usr.p_rts_flags);
		if(	warn_ptr->p_usr.p_getfrom == caller_ptr->p_usr.p_endpoint){
			warn_ptr->p_usr.p_getfrom = NONE;
		}

		caller_ptr->p_rmtcmd.c_cmd 		= CMD_SEND_MSG;
		caller_ptr->p_rmtcmd.c_src 		= caller_ptr->p_usr.p_endpoint;
		caller_ptr->p_rmtcmd.c_dst 		= warn_ptr->p_usr.p_endpoint;
		caller_ptr->p_rmtcmd.c_dcid		= caller_ptr->p_usr.p_dcid;
		caller_ptr->p_rmtcmd.c_snode  	= atomic_read(&local_nodeid);
		caller_ptr->p_rmtcmd.c_dnode  	= warn_ptr->p_usr.p_nodeid;
		caller_ptr->p_rmtcmd.c_rcode  	= OK;
		caller_ptr->p_rmtcmd.c_len  	= 0;
		
		caller_ptr->p_message.m_source = caller_ptr->p_usr.p_endpoint;
		m_ptr = &caller_ptr->p_message;
		MOLDEBUG(DBGPARAMS, MSG1_FORMAT,MSG1_FIELDS(m_ptr));
		memcpy(	&caller_ptr->p_rmtcmd.c_u.cu_msg,&caller_ptr->p_message, sizeof(message));
		
		m_ptr = &caller_ptr->p_rmtcmd.c_u.cu_msg;
		MOLDEBUG(GENERIC, MSG1_FORMAT, MSG1_FIELDS(m_ptr));		
						
//		set_bit(BIT_RECEIVING, &caller_ptr->p_usr.p_rts_flags);
		caller_ptr->p_usr.p_getfrom = warn_ptr->p_usr.p_endpoint;
		set_bit(BIT_SENDING,   &caller_ptr->p_usr.p_rts_flags);
		caller_ptr->p_usr.p_sendto 	= warn_ptr->p_usr.p_endpoint;	
		set_bit(BIT_RMTOPER, &caller_ptr->p_usr.p_rts_flags);
		
		INIT_LIST_HEAD(&caller_ptr->p_link);
		
		ret = sproxy_enqueue(caller_ptr);
			
		/* wait for the SENDACK */
#define TIMEOUT_30S		30000		
		sleep_proc(caller_ptr, TIMEOUT_30S);
		ret = caller_ptr->p_rcode;
		if( ret == OK){
			caller_ptr->p_usr.p_rmtsent++;
		}else{
			if( test_bit(BIT_SENDING, &caller_ptr->p_usr.p_rts_flags)) {
				MOLDEBUG(GENERIC,"removing %d link from sender's proxy list.\n", 
					caller_ptr->p_usr.p_endpoint);
				LIST_DEL_INIT(&caller_ptr->p_link); /* remove from queue ATENCION: HAY Q PROTEGER PROXY ?? */
			}
			clear_bit(BIT_SENDING, &caller_ptr->p_usr.p_rts_flags);
			caller_ptr->p_usr.p_sendto = NONE;
			clear_bit(BIT_RMTOPER, &caller_ptr->p_usr.p_rts_flags);
			caller_ptr->p_usr.p_proxy = NONE;
		}
	}else{
		/*-----------------------------------------------------------------------------------*/	
		/*					LOCAL  							*/
		/*-----------------------------------------------------------------------------------*/	
	
		if ( (test_bit(BIT_RECEIVING, &warn_ptr->p_usr.p_rts_flags) 
			&& !test_bit(BIT_SENDING, &warn_ptr->p_usr.p_rts_flags)) &&
			(warn_ptr->p_usr.p_getfrom == ANY || warn_ptr->p_usr.p_getfrom == caller_ptr->p_usr.p_endpoint)) {
			MOLDEBUG(GENERIC,"destination is waiting. Copy the message and wakeup destination\n");

			set_bit(BIT_RECEIVING, &caller_ptr->p_usr.p_rts_flags); /* Sending part: completed, now receiving.. */
			caller_ptr->p_usr.p_getfrom = warn_ptr->p_usr.p_endpoint;
			clear_bit(BIT_RECEIVING, &warn_ptr->p_usr.p_rts_flags);
			warn_ptr->p_usr.p_getfrom = NONE;

			set_bit(MIS_BIT_NOTIFY,&warn_ptr->p_usr.p_misc_flags);
			caller_ptr->p_message.m_source = caller_ptr->p_usr.p_endpoint;
			memcpy( &warn_ptr->p_message, &caller_ptr->p_message, sizeof(message));
			m_ptr = &warn_ptr->p_message;
			MOLDEBUG(DBGPARAMS, MSG1_FORMAT,MSG1_FIELDS(m_ptr));

			if(warn_ptr->p_usr.p_rts_flags == 0){
				LOCAL_PROC_UP(warn_ptr, OK); 
			}
			
		} else { 
			MOLDEBUG(GENERIC,"destination is not waiting warn_ptr-flags=%lX. Enqueue at TAIL.\n"
				,warn_ptr->p_usr.p_rts_flags);
			/* The destination is not waiting for this message 			*/
			/* Append the caller at the TAIL of the destination senders' queue	*/
			/* blocked sending the message */
					
			caller_ptr->p_message.m_source = caller_ptr->p_usr.p_endpoint;
			set_bit(BIT_RECEIVING, &caller_ptr->p_usr.p_rts_flags);
			caller_ptr->p_usr.p_getfrom = warn_ptr->p_usr.p_endpoint;
			set_bit(BIT_SENDING,   &caller_ptr->p_usr.p_rts_flags);
			caller_ptr->p_usr.p_sendto 	= warn_ptr->p_usr.p_endpoint;	
			INIT_LIST_HEAD(&caller_ptr->p_link);
			LIST_ADD_TAIL(&caller_ptr->p_link, &warn_ptr->p_list);
		}	
	}
	
	/* WARNING do not use sleep_proc2 because the process could be signaled !! */
	caller_ptr->p_pseudosem = -1;
	WUNLOCK_PROC2(caller_ptr, warn_ptr);
	WUNLOCK_DC(dc_ptr);
	if( IT_IS_LOCAL(warn_ptr))
		WUNLOCK_TASK(current);
	// REMOTE: the SPROXY wakes up me when the message was sent.
	// LOCAL: The WARN PROC wakes up me when the message was sent.
	wait_event_interruptible(caller_ptr->p_wqhead, (caller_ptr->p_pseudosem >= 0));
	if( IT_IS_LOCAL(warn_ptr))
		WLOCK_TASK(current);
	WLOCK_DC(dc_ptr);
	WLOCK_PROC2(caller_ptr, warn_ptr); 
	cu_ptr = &caller_ptr->p_usr;
	MOLDEBUG(DBGPARAMS, PROC_USR_FORMAT,PROC_USR_FIELDS(cu_ptr));
	
	return(OK);
}

/*--------------------------------------------------------------*/
/*			mm_exit_unbind				*/
/* Called by LINUX exit() function when a process exits */
/*--------------------------------------------------------------*/
asmlinkage long mm_exit_unbind(long code)
{
	struct proc *caller_ptr, *warn_ptr;
	proc_usr_t *uproc_ptr;
	struct proc *sproxy_ptr, *rproxy_ptr;
	int rcode, px_nr, warn_nr;
	proxies_t *px_ptr;
	dc_desc_t *dc_ptr;
	dc_usr_t *dcu_ptr;

	MOLDEBUG(DBGPARAMS,"code=%d\n", code);
	
	if( DVS_NOT_INIT() ) ERROR_RETURN(EMOLDVSINIT);
	
	WLOCK_TASK(current);
	caller_ptr = (struct proc *) current->task_proc;	/* only the main thread or a binded thread could unbind */
	rcode = OK;
	uproc_ptr = &caller_ptr->p_usr;
	
	if( caller_ptr != NULL) { // bound process 
		RLOCK_PROC(caller_ptr);
		MOLDEBUG(INTERNAL,PROC_USR_FORMAT,PROC_USR_FIELDS(uproc_ptr));

			/* UNBINDING PROXIES */
		if(test_bit(MIS_BIT_PROXY, &caller_ptr->p_usr.p_misc_flags)) {
			MOLDEBUG(INTERNAL," Exiting PROXY px_nr=%d lpid=%d\n", 
				caller_ptr->p_usr.p_nr, caller_ptr->p_usr.p_lpid);
			if( caller_ptr->p_usr.p_nr >= 0 && caller_ptr->p_usr.p_nr < dvs.d_nr_nodes) {
				px_nr = caller_ptr->p_usr.p_nr;
				px_ptr = &proxies[px_nr];
				RUNLOCK_PROC(caller_ptr);
				RLOCK_PROXY(px_ptr);
				if( px_ptr->px_usr.px_flags == PROXIES_FREE)	{
					RUNLOCK_PROXY(px_ptr);
					rcode = EMOLNOPROXY;
				}else{
					sproxy_ptr = &px_ptr->px_sproxy; 
					rproxy_ptr = &px_ptr->px_rproxy; 
					RUNLOCK_PROXY(px_ptr);
				
					WLOCK_PROC(rproxy_ptr);
					WLOCK_PROC(sproxy_ptr);
					rcode = do_proxies_unbind(caller_ptr, sproxy_ptr,  rproxy_ptr);
					WUNLOCK_PROC(sproxy_ptr);
					WUNLOCK_PROC(rproxy_ptr);
				}
			} else {
				printk("WARNING: %d:%s:%u: This proxy process seems to be inconsistent p_nr=%d\n", 
				task_pid_nr(current), __FUNCTION__ ,__LINE__, caller_ptr->p_usr.p_nr);
				RUNLOCK_PROC(caller_ptr);
				rcode = EMOLPROCSTS;
			}
		} else if ( caller_ptr->p_usr.p_dcid >= 0 &&  caller_ptr->p_usr.p_dcid < dvs.d_nr_dcs) {
			MOLDEBUG(INTERNAL," Exiting endpoint=%d lpid=%d\n", 	
				caller_ptr->p_usr.p_endpoint, caller_ptr->p_usr.p_lpid);
			dc_ptr = &dc[caller_ptr->p_usr.p_dcid];
			RUNLOCK_PROC(caller_ptr);
			WLOCK_DC(dc_ptr);
			WLOCK_PROC(caller_ptr);
			if(dc_ptr->dc_usr.dc_flags) {
				printk("ERROR: %d:%s:%u: DC %d is not running but it process %d is exiting\n",
						task_pid_nr(current), __FUNCTION__ ,__LINE__,dc_ptr->dc_usr.dc_dcid, caller_ptr->p_usr.p_endpoint); 
				rcode = EMOLDCNOTRUN;		
			}else {
				MOLDEBUG(INTERNAL," endpoint=%d lpid=%d\n", 
					caller_ptr->p_usr.p_endpoint, caller_ptr->p_usr.p_lpid);
				if(caller_ptr->p_usr.p_rts_flags != SLOT_FREE){	
					/* need to send a message to another process before exit? */
					if( dc_ptr->dc_usr.dc_warn2proc != NONE){
						/* check if the destination process is in correct state */
						do {
							warn_nr = _ENDPOINT_P(dc_ptr->dc_usr.dc_warn2proc);
							if( warn_nr < (-dc_ptr->dc_usr.dc_nr_tasks)		
								|| warn_nr >= dc_ptr->dc_usr.dc_nr_procs){
									rcode = EMOLRANGE;
									ERROR_PRINT(rcode);
									break;
							}
							warn_ptr   = NBR2PTR(dc_ptr, warn_nr);
							if( warn_ptr == NULL) {	
								rcode = EMOLDSTDIED;
								ERROR_PRINT(rcode);
								break;
							}
							/* source != destination*/
							if( caller_ptr->p_usr.p_endpoint == dc_ptr->dc_usr.dc_warn2proc) {
								rcode = EMOLENDPOINT;
								ERROR_PRINT(rcode);
								break;
							}
							WUNLOCK_PROC(caller_ptr);
							WLOCK_PROC2(warn_ptr,caller_ptr);
							/* destination process is dead */
							if(warn_ptr->p_usr.p_rts_flags == SLOT_FREE){
								WUNLOCK_PROC(warn_ptr);
								rcode = EMOLNOTBIND;
								ERROR_PRINT(rcode);
								break;							
							}

							/* destination must not be migrating */
							if( test_bit(BIT_MIGRATE, &warn_ptr->p_usr.p_rts_flags))	{
								WUNLOCK_PROC(warn_ptr);
								rcode = EMOLMIGRATE;
								ERROR_PRINT(rcode);
								break;
							}
							/* build the exiting message for destination */
							dcu_ptr = &dc_ptr->dc_usr;
							MOLDEBUG(INTERNAL,DC_WARN_FORMAT, DC_WARN_FIELDS(dcu_ptr));
							/* tests if SYSTASK has sent a SIGNAL to caller */
							if( !test_bit(MIS_BIT_KILLED,&caller_ptr->p_usr.p_misc_flags)){
								/* NO, other processs has killed the caller */
								/* send a warning to PM -> SYSTASK 			*/
								set_bit(MIS_BIT_KILLED,&caller_ptr->p_usr.p_misc_flags);
								caller_ptr->p_message.m_type = dc_ptr->dc_usr.dc_warnmsg;
								caller_ptr->p_message.m1_i1 = (int) code;
								caller_ptr->p_message.m1_i2 = (-1); /* caller is exiting */
								rcode = kernel_warn2proc(dc_ptr, caller_ptr, warn_ptr);
							}
							WUNLOCK_PROC(warn_ptr);
							if(code) ERROR_PRINT((int)code);							
						}while(0);
					}
					if(caller_ptr->p_usr.p_rts_flags != SLOT_FREE){
						rcode = do_unbind(dc_ptr, caller_ptr);
					}
				}
				if(rcode == OK){
					WUNLOCK_PROC(caller_ptr);
					WUNLOCK_TASK(current);
					WUNLOCK_DC(dc_ptr);
					if (! test_bit(BIT_NO_MAP, &caller_ptr->p_usr.p_rts_flags))
						mm_put_task_struct(current);		/* decrement the reference count of the task struct */
					return(OK);
				}
			}
			WUNLOCK_PROC(caller_ptr);
			WUNLOCK_DC(dc_ptr);
		}else {
			rcode  = EMOLBADDCID;
			RUNLOCK_PROC(caller_ptr);
		}
	}else{
		MOLDEBUG(INTERNAL,"Process not bound pid=%d %s\n", 	
				task_pid_nr(current), current->comm);		
	}
	WUNLOCK_TASK(current);
	if( rcode) ERROR_RETURN(rcode);
	return(OK);
}

/*--------------------------------------------------------------*/
/*			mm_wakeup				*/
/*--------------------------------------------------------------*/
asmlinkage long mm_wakeup(int dcid, int proc_ep)
{

	struct proc *proc_ptr, *other_ptr, *sproxy_ptr;
	dc_desc_t *dc_ptr;
	int caller_pid, caller_tgid;
	int proc_nr;
	struct task_struct *task_ptr;	
	proc_usr_t  *uproc_ptr;

	MOLDEBUG(DBGPARAMS,"dcid=%d proc_ep=%d\n", dcid, proc_ep);
	if( DVS_NOT_INIT() ) ERROR_RETURN(EMOLDVSINIT);
	if(current_euid() != USER_ROOT)	ERROR_RETURN(EMOLPRIVILEGES);

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
	proc_nr = _ENDPOINT_P(proc_ep);
	if( proc_nr < (-dc_ptr->dc_usr.dc_nr_tasks)		
	 || proc_nr >= dc_ptr->dc_usr.dc_nr_procs)	
		ERROR_RETURN(EMOLRANGE)
	proc_ptr   = NBR2PTR(dc_ptr, proc_nr);
	if( proc_ptr == NULL) 				
		ERROR_RETURN(EMOLDSTDIED);
	if( current == proc_ptr->p_task)
		ERROR_RETURN(EMOLBADPROC);
	
	/*------------------------------------------
	 * check the destination process status
	*------------------------------------------*/
	WLOCK_PROC(proc_ptr);
	uproc_ptr = &proc_ptr->p_usr;
	MOLDEBUG(INTERNAL,PROC_USR_FORMAT,PROC_USR_FIELDS(uproc_ptr));

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
	if (  test_bit(BIT_RECEIVING, &proc_ptr->p_usr.p_rts_flags) 
		&& proc_ptr->p_usr.p_getfrom == ANY  ) {

		if( test_bit(BIT_RECEIVING, &proc_ptr->p_usr.p_rts_flags)){
			clear_bit(BIT_RECEIVING, &proc_ptr->p_usr.p_rts_flags);
			proc_ptr->p_usr.p_getfrom	= NONE;
		}
		
		if(proc_ptr->p_usr.p_rts_flags == 0){
			LOCAL_PROC_UP(proc_ptr, EMOLWOKENUP); 
		}else{
			MOLDEBUG(GENERIC,"Process status is p_rts_flags=%lX\n",
				proc_ptr->p_usr.p_rts_flags);				
			set_bit(MIS_BIT_WOKENUP, &proc_ptr->p_usr.p_misc_flags);	
			WUNLOCK_PROC(proc_ptr);
			return(EMOLPROCSTS);
		}
	} else { 
		MOLDEBUG(GENERIC,"Process is running p_rts_flags=%lX\n",
			proc_ptr->p_usr.p_rts_flags);
		set_bit(MIS_BIT_WOKENUP, &proc_ptr->p_usr.p_misc_flags);	
		WUNLOCK_PROC(proc_ptr);
		return(EMOLPROCRUN);
	}
	WUNLOCK_PROC(proc_ptr);

	return(OK);
}

/*----------------------------------------------------------------*/
/*			dvs_release				*/
/*----------------------------------------------------------------*/
void dvs_release(struct kref *kref)	
{
MOLDEBUG(INTERNAL,"Releasing DVS resources\n");

	/* Set the DVS as uninitilized */
	atomic_set(&local_nodeid,DVS_NO_INIT);

	remove_proc_entry("info", 	proxies_dir);
	remove_proc_entry("procs", 	proxies_dir);

   	remove_proc_entry("nodes", 	proc_dvs_dir);
	remove_proc_entry("info", 	proc_dvs_dir);
	remove_proc_entry("proxies", 	proc_dvs_dir);
	remove_proc_entry("dvs", 	NULL);

}


/*--------------------------------------------------------------*/
/*			mm_wait4bind										*/
/* IT waits for self process binding or other process unbining 	*/
/*--------------------------------------------------------------*/
asmlinkage long mm_wait4bind(int oper, int other_ep, long timeout_ms)
{
	struct proc *caller_ptr, *other_ptr;
	proc_usr_t *uproc_ptr;
	int ret;
	dc_desc_t *dc_ptr;
	int dcid;
	int caller_pid, caller_nr, caller_ep, other_nr;
	struct task_struct *task_ptr;
	
	MOLDEBUG(INTERNAL,"oper=%d other_ep=%d timeout_ms=%ld\n", 
		oper, other_ep, timeout_ms);

	if( DVS_NOT_INIT() ) 		ERROR_RETURN(EMOLDVSINIT);

	if( oper != WAIT_BIND && oper != WAIT_UNBIND) ERROR_RETURN(EMOLBADCALL);

	while(TRUE) {
		ret = check_caller(&task_ptr, &caller_ptr, &caller_pid);
		MOLDEBUG(INTERNAL,"caller_pid=%d ret=%d\n", caller_pid, ret);
		/* WARNING : task_ptr can be the "current" task or may be the thread leader */
		if( other_ep == SELF && oper == WAIT_BIND ){ /* caller itself if it is bound  */
			if(ret == OK) {		/*The process is just bound  */
				RLOCK_PROC(caller_ptr);
				ret = caller_ptr->p_usr.p_endpoint;
				RUNLOCK_PROC(caller_ptr);
				MOLDEBUG(INTERNAL,"ret=%d\n", ret);
				return(ret);
			}	
			if(ret != EMOLNOTBIND) 
				ERROR_RETURN(ret);
		}else{ /* caller is bound */
			if(ret != OK) 	ERROR_RETURN(ret);
			/*------------------------------------------
			* Checks the caller process PID
			* Gets the DCID
			* Checks if the DC is RUNNING
			*------------------------------------------*/
			RLOCK_PROC(caller_ptr);
			caller_nr   = caller_ptr->p_usr.p_nr;
			caller_ep   = caller_ptr->p_usr.p_endpoint;
			dcid		= caller_ptr->p_usr.p_dcid;
			MOLDEBUG(DBGPARAMS,"caller_nr=%d caller_ep=%d other_ep=%d \n", caller_nr, caller_ep, other_ep);
			if( caller_ep == other_ep) 	 
				ERROR_RUNLOCK_PROC(caller_ptr,EMOLENDPOINT);
			if (caller_pid != caller_ptr->p_usr.p_lpid) 	
				ERROR_RUNLOCK_PROC(caller_ptr,EMOLBADPID);
			RUNLOCK_PROC(caller_ptr);

			MOLDEBUG(INTERNAL,"dcid=%d\n", dcid);
			if( dcid < 0 || dcid >= dvs.d_nr_dcs) 			
				ERROR_RETURN(EMOLBADDCID);
			dc_ptr 	= &dc[dcid];
			RLOCK_DC(dc_ptr);
			if( dc_ptr->dc_usr.dc_flags)  
				ERROR_RUNLOCK_DC(dc_ptr,EMOLDCNOTRUN);
				
			/*------------------------------------------
			* get the OTHER process number
			*------------------------------------------*/
			other_nr = _ENDPOINT_P(other_ep);
			if( other_nr < (-dc_ptr->dc_usr.dc_nr_tasks)		
				|| other_nr >= dc_ptr->dc_usr.dc_nr_procs)	
				ERROR_RUNLOCK_DC(dc_ptr, EMOLRANGE)
			other_ptr   = NBR2PTR(dc_ptr, other_nr);
			RUNLOCK_DC(dc_ptr);	

			WLOCK_PROC2(caller_ptr,other_ptr);	
			if( !test_bit(BIT_SLOT_FREE, &other_ptr->p_usr.p_rts_flags)){
				if( oper == WAIT_BIND) {	
					if( other_ptr->p_usr.p_endpoint != other_ep){
						WUNLOCK_PROC2(caller_ptr, other_ptr);
						ERROR_RETURN(EMOLENDPOINT);  /* Other process is using the slot */
					}else{ /* the process is bound */
						WUNLOCK_PROC2(caller_ptr, other_ptr);
						return(other_ep);	
					}
				} else { /* WAIT_UNBIND */
					if( other_ptr->p_usr.p_endpoint != other_ep){
						WUNLOCK_PROC2(caller_ptr, other_ptr);
						return(OK); /* Other process is using the slot */
					}else{
						break;		/* enqueue caller's descriptor into other process unbinding queue */
					}
				}
			}else{ /* Other process slot is free */
				WUNLOCK_PROC2(caller_ptr, other_ptr);
				if( oper == WAIT_UNBIND) {
					return(OK); /* the slot  is free */
				}		
			}
		}
		
		WLOCK_TASK(current);
		if(other_ep == SELF){
			if (current->task_proc != NULL){
				caller_ptr = (struct proc *)current->task_proc;
				WUNLOCK_TASK(current);
				RLOCK_PROC(caller_ptr);
				ret = caller_ptr->p_usr.p_endpoint;
				RUNLOCK_PROC(caller_ptr);
				MOLDEBUG(INTERNAL,"ret=%d\n", ret);
				return(ret);		
			}
		}
		init_waitqueue_head(&current->task_wqh);		/* Initialize the wait queue 	*/
		WUNLOCK_TASK(current);

		/* here cames only WAIT_BIND */
		if(other_ep == SELF){
			MOLDEBUG(INTERNAL,"Self process bind waiting\n");
			if( timeout_ms < 0) {
				ret = wait_event_interruptible(current->task_wqh, 
						(current->task_proc != NULL));
			} else {
				ret = wait_event_interruptible_timeout(current->task_wqh, 
						(current->task_proc != NULL), 
						msecs_to_jiffies(timeout_ms));
			}
		}else{
			MOLDEBUG(INTERNAL,"Other process bind waiting\n");
			if( timeout_ms < 0) {
				ret = wait_event_interruptible(current->task_wqh, 
						(other_ptr->p_usr.p_rts_flags != SLOT_FREE));
			} else {
				ret = wait_event_interruptible_timeout(current->task_wqh, 
						(other_ptr->p_usr.p_rts_flags != SLOT_FREE), 
						msecs_to_jiffies(timeout_ms));
			}			
		}
		
		if (ret == 0)
			ERROR_RETURN(EMOLTIMEDOUT);		/* timeout */
			
	}

	/*here cames only WAIT4_UNBIND */
	/* insert caller into the other waiting for unbinding queue */
	MOLDEBUG(INTERNAL,"Waiting for the unbinding of endpoint=%d\n",
		other_ptr->p_usr.p_endpoint);
	INIT_LIST_HEAD(&caller_ptr->p_ulink);
	LIST_ADD_TAIL(&caller_ptr->p_ulink, &other_ptr->p_ulist);
	set_bit(BIT_WAITUNBIND, &caller_ptr->p_usr.p_rts_flags);
	caller_ptr->p_usr.p_waitunbind = other_ptr->p_usr.p_endpoint;
	WUNLOCK_PROC2(caller_ptr, other_ptr);

	sleep_proc(caller_ptr, timeout_ms);

	WLOCK_PROC2(caller_ptr,other_ptr);	
	ret = caller_ptr->p_rcode;
	if(ret != EMOLTIMEDOUT) {
		clear_bit(BIT_WAITUNBIND, &caller_ptr->p_usr.p_rts_flags);
		uproc_ptr = &caller_ptr->p_usr;
		MOLDEBUG(INTERNAL,PROC_USR_FORMAT,PROC_USR_FIELDS(uproc_ptr));
		WUNLOCK_PROC2(caller_ptr, other_ptr);
		if(ret != OK) ERROR_RETURN(ret);
		return(OK);
	}
	
	MOLDEBUG(INTERNAL,"TIMEOUT Waiting for the unbinding of endpoint=%d\n", 
		other_ptr->p_usr.p_endpoint);
	LIST_DEL_INIT(&caller_ptr->p_ulink);
	clear_bit(BIT_WAITUNBIND, &caller_ptr->p_usr.p_rts_flags);
	caller_ptr->p_usr.p_waitunbind = NONE;
	WUNLOCK_PROC2(caller_ptr, other_ptr);

	if(ret != OK) ERROR_RETURN(ret);
	return(ret);		
}

/*--------------------------------------------------------------*/
/*			mm_dvs_init				*/
/* Initialize the DVS system					*/
/* du_addr = NULL => already loaded DVS parameters 	*/ 
/* returns the local_node id if OK or negative on error	*/
/*--------------------------------------------------------------*/
asmlinkage long mm_dvs_init(int nodeid, dvs_usr_t *du_addr)
{
	int i, ret;
	dc_desc_t *dc_ptr;
	dvs_usr_t lcldvs, *d_ptr;

	if(current_euid() != USER_ROOT) ERROR_RETURN(EMOLPRIVILEGES);

	if (!try_module_get(THIS_MODULE)){
		ERROR_RETURN(EMOLNODEV);
	}
	
	if( !DVS_NOT_INIT() )   ERROR_RETURN(EMOLDVSBUSY);
	
	CHECK_NODEID(nodeid);

	WLOCK_DVS;

	/* get DVS parameters from Userspace */
	if( du_addr != NULL) {  /* NULL => already loaded DVS parameters */ 
		do {
			ret = copy_from_user(&lcldvs, du_addr, sizeof(dvs_usr_t));
			if(ret) break;
			d_ptr = &lcldvs;
			MOLDEBUG(DBGPARAMS,DVS_USR_FORMAT, DVS_USR_FIELDS(d_ptr));
			MOLDEBUG(DBGPARAMS,DVS_MAX_FORMAT, DVS_MAX_FIELDS(d_ptr));
			MOLDEBUG(DBGPARAMS,DVS_VER_FORMAT, DVS_VER_FIELDS(d_ptr));

			if(d_ptr->d_nr_dcs <= 0  || d_ptr->d_nr_dcs > NR_DCS ) {ret = EMOLBADRANGE; break;}
			if(d_ptr->d_nr_nodes <= 0 || d_ptr->d_nr_nodes > NR_NODES) {ret = EMOLBADRANGE; break;}
			if(d_ptr->d_nr_procs <= 0 || d_ptr->d_nr_procs > NR_PROCS ) {ret = EMOLBADRANGE; break;}
			if(d_ptr->d_nr_tasks <= 0 || d_ptr->d_nr_tasks > NR_TASKS ) {ret = EMOLBADRANGE; break;}
			if(d_ptr->d_nr_sysprocs <= 0 || d_ptr->d_nr_sysprocs > NR_SYS_PROCS ) {ret = EMOLBADRANGE; break;}	

			if(d_ptr->d_nr_sysprocs >= (d_ptr->d_nr_procs+ d_ptr->d_nr_tasks)) {ret = EMOLBADRANGE; break;}
			if(d_ptr->d_nr_sysprocs <= d_ptr->d_nr_tasks) {ret = EMOLBADRANGE; break;}

			if(d_ptr->d_max_copybuf > MAXCOPYBUF) {ret = EMOLBADVALUE; break;}
			if(d_ptr->d_max_copylen > MAXCOPYLEN) {ret = EMOLBADVALUE; break;}
			if(d_ptr->d_max_copylen < d_ptr->d_max_copybuf) {ret = EMOLBADVALUE; break;}

		}while(0);
		if (ret) 	{ 
			WUNLOCK_DVS;
			ERROR_RETURN(ret);
		}
		memcpy(&dvs,&lcldvs, sizeof(dvs_usr_t));
	}

	MOLDEBUG(INTERNAL,"CPU INFO: x86_cache_size=%d\n", boot_cpu_data.x86_cache_size);
	MOLDEBUG(INTERNAL,"CPU INFO: x86_cache_alignment =%d\n", boot_cpu_data.x86_cache_alignment);

	MOLDEBUG(INTERNAL,"Initializing %d DCs: dc=%p\n", dvs.d_nr_dcs, dc);
	for( i = 0;  i < dvs.d_nr_dcs; i++) {
		dc_ptr = &dc[i];
		dc_ptr->dc_usr.dc_flags = DC_FREE;
		cpumask_setall(&dc_ptr->dc_usr.dc_cpumask);
		DC_LOCK_INIT(dc_ptr);
	}

	MOLDEBUG(INTERNAL,"Initializing %d NODEs node=%p\n", dvs.d_nr_nodes, node);
	for( i = 0;  i < dvs.d_nr_nodes; i++) {
		init_node(i);
	}

	MOLDEBUG(INTERNAL,"Initializing %d PROXIES\n", dvs.d_nr_nodes);
	for( i = 0;  i < dvs.d_nr_nodes; i++) {
		init_proxies(i);
	}

	sprintf(node[nodeid].n_usr.n_name,"node%d",nodeid);
	node[nodeid].n_usr.n_flags = (NODE_ATTACHED | NODE_SCONNECTED | NODE_SCONNECTED);
	
	/*Align every struct proc with the cache line */
	sizeof_proc_aligned = boot_cpu_data.x86_cache_alignment;
	while( sizeof(struct proc) > sizeof_proc_aligned)
		sizeof_proc_aligned = (sizeof_proc_aligned << 1);
	log2_proc_size = ilog2(sizeof_proc_aligned);
	MOLDEBUG(INTERNAL,"sizeof_proc_aligned=%d log2_proc_size=%d\n",sizeof_proc_aligned, log2_proc_size);
		
	/* create the dvs dir under /sys/kernel/debug  */
	dbg_dvs =  debugfs_create_dir("dvs", NULL);
	if (dbg_dvs == NULL) {
     		printk("ERROR: %d:%s:%u: Couldn't create dvs directory under /sys/kernel/debug \n",
				task_pid_nr(current), __FUNCTION__ ,__LINE__); 
		WUNLOCK_DVS;
		ERROR_RETURN(EMOLBADDIR);
	}
	MOLDEBUG(INTERNAL,"Creating dvs directory on debugfs\n");
	
	/* create the dvs dir under /proc */
	proc_dvs_dir = proc_mkdir("dvs", NULL);
  	if (proc_dvs_dir == NULL) {
     		printk("ERROR: %d:%s:%u: Couldn't create dvs directory under /proc\n",
				task_pid_nr(current), __FUNCTION__ ,__LINE__); 
		WUNLOCK_DVS;
		ERROR_RETURN(EMOLBADDIR);
    	}else {
		/* create the nodes file under /proc/dvs */
		nodes_entry = create_proc_entry( "nodes", 0444, proc_dvs_dir);
  		if (nodes_entry == NULL) {
	     	printk("ERROR: %d:%s:%u: Couldn't create nodes under /proc/dvs\n",
				task_pid_nr(current), __FUNCTION__ ,__LINE__);
			dvs_release(NULL);
			WUNLOCK_DVS;
			ERROR_RETURN(EMOLBADFILE);
		}else{
   		 	nodes_entry->read_proc = nodes_read;
			MOLDEBUG(GENERIC,"/proc/dvs/nodes installed\n");
		}

		/* create the file info  under /proc/dvs */
		info_entry = create_proc_entry( "info", 0444, proc_dvs_dir);
		if (info_entry == NULL) {
    		printk("ERROR: %d:%s:%u: Couldn't create info into /proc/dvs\n",
				task_pid_nr(current), __FUNCTION__ ,__LINE__); 
			dvs_release(NULL);
			WUNLOCK_DVS;
			ERROR_RETURN(EMOLBADFILE);
	    }else{
			info_entry->read_proc = info_read;
			MOLDEBUG(GENERIC,"/proc/dvs/info installed\n");
		}
		
		/* create the directory proxies under /proc/dvs */
		proxies_dir = proc_mkdir("proxies", proc_dvs_dir);
		if (proxies_dir == NULL) {
   			printk("ERROR: %d:%s:%u: Couldn't create directory proxies into /proc/dvs\n",
				task_pid_nr(current), __FUNCTION__ ,__LINE__); 
			dvs_release(NULL);
			WUNLOCK_DVS;
			ERROR_RETURN(EMOLBADFILE);
		}else{
			/* create the file info  under /proc/dvs/proxies */
			proxies_info_entry = create_proc_entry( "info", 0444, proxies_dir);
			if (proxies_info_entry == NULL) {
				printk("ERROR: %d:%s:%u: Couldn't create file info into /proc/dvs/proxies\n",
					task_pid_nr(current), __FUNCTION__ ,__LINE__); 
				dvs_release(NULL);
				WUNLOCK_DVS;
				ERROR_RETURN(EMOLBADFILE);
			}else{
				proxies_info_entry->read_proc = proxies_info_read;
				MOLDEBUG(GENERIC,"/proc/dvs/proxies/info installed\n");
			}
			/* create the file procs  under /proc/dvs/proxies */
			proxies_info_procs = create_proc_entry( "procs", 0444, proxies_dir);
			if (proxies_info_procs == NULL) {
				printk("ERROR: %d:%s:%u: Couldn't create file procs into /proc/dvs/proxies\n",
					task_pid_nr(current), __FUNCTION__ ,__LINE__); 
				dvs_release(NULL);
				WUNLOCK_DVS;
				ERROR_RETURN(EMOLBADFILE);
			}else{
				proxies_info_procs->read_proc = proxies_procs_read;
				MOLDEBUG(GENERIC,"/proc/dvs/proxies/procs installed\n");
			}
			
		}
    }
		
	dvs.d_size_proc = sizeof_proc_aligned;
	MOLDEBUG(INTERNAL,"d_size_proc=%d\n", dvs.d_size_proc);
	
	kref_init(&dvs_kref);	

	MOLDEBUG(INTERNAL,"Initializing DVS. Local node ID %d\n", nodeid);
	atomic_set(&local_nodeid, nodeid);		/* LUEGO DEBE SER OBTENIDO DE LA CONFIGURACION */

	WUNLOCK_DVS;

	return(atomic_read(&local_nodeid));
}

///*----------------------------------------------------------------*/
///*			dvs_release							*/
///*----------------------------------------------------------------*/
//void dvs_release(struct kref *kref)	
//{
//MOLDEBUG(INTERNAL,"Releasing DVS resources\n");
//
//	/* Set the DVS as uninitilized */
//	atomic_set(&local_nodeid,DVS_NO_INIT);
//
//	remove_proc_entry("info", 	proxies_dir);
//	remove_proc_entry("procs", 	proxies_dir);
//
//   	remove_proc_entry("nodes", 	proc_dvs_dir);
//	remove_proc_entry("info", 	proc_dvs_dir);
//	remove_proc_entry("proxies", 	proc_dvs_dir);
//	remove_proc_entry("dvs", 	NULL);
//
//}

/*--------------------------------------------------------------*/
/*			mm_add_node				*/
/* Initializa a cluster node to be used by a DC 		*/
/*--------------------------------------------------------------*/
asmlinkage long mm_add_node(int dcid, int nodeid)
{
	cluster_node_t *n_ptr;
	dc_desc_t *dc_ptr;
	int i, ret, nodes;

	MOLDEBUG(DBGPARAMS,"dcid=%d nodeid=%d \n", dcid, nodeid);

	if(current_euid() != USER_ROOT) 		ERROR_RETURN(EMOLPRIVILEGES);

	if( DVS_NOT_INIT() )   			ERROR_RETURN(EMOLDVSINIT);

	CHECK_NODEID(nodeid);
	if(nodeid == atomic_read(&local_nodeid)) 	ERROR_RETURN(EMOLBADNODEID);
	n_ptr = &node[nodeid];
	RLOCK_NODE(n_ptr);
	if( n_ptr->n_usr.n_flags == NODE_FREE){
		RUNLOCK_NODE(n_ptr);
 		ERROR_RETURN(EMOLNODEFREE);
	}
	RUNLOCK_NODE(n_ptr);

	CHECK_DCID(dcid);
	dc_ptr 		= &dc[dcid];
	WLOCK_DC(dc_ptr);
	WLOCK_NODE(n_ptr);
	do {	
		ret = OK;	
		if( dc_ptr->dc_usr.dc_flags == DC_FREE )  	{ret = EMOLDCNOTRUN; break;}
		if(test_bit(nodeid, &dc_ptr->dc_usr.dc_nodes))	{ret = EMOLDCNODE; break;}
		if(test_bit(dcid, &n_ptr->n_usr.n_dcs)) 	{ret = EMOLDCNODE; break;}  
		/* check the number of nodes admitted by the DC*/
		nodes = 0;
		for( i = 0; i < sizeof(unsigned long int)*8; i++){
			if( test_bit(i, &dc_ptr->dc_usr.dc_nodes)){	
				nodes++;
			}	
		}
		if( nodes >=  dc_ptr->dc_usr.dc_nr_nodes)	{ret = EMOLRSCBUSY; break;} 
		set_bit(nodeid, &dc_ptr->dc_usr.dc_nodes);
		set_bit(dcid, &n_ptr->n_usr.n_dcs);
	}while(0);
	WUNLOCK_NODE(n_ptr);
	WUNLOCK_DC(dc_ptr);
	if(ret) ERROR_RETURN(ret);
	return(OK);
}


/*--------------------------------------------------------------*/
/*			mm_del_node				*/
/* Delete a cluster node from a DC 		 		*/
/*--------------------------------------------------------------*/
asmlinkage long mm_del_node(int dcid, int nodeid)
{

	cluster_node_t *n_ptr;
	dc_desc_t *dc_ptr;
	int ret;

	MOLDEBUG(DBGPARAMS,"dcid=%d nodeid=%d \n", dcid, nodeid);

	if(current_euid() != USER_ROOT) 		ERROR_RETURN(EMOLPRIVILEGES);

	if( DVS_NOT_INIT() )   			ERROR_RETURN(EMOLDVSINIT);

	CHECK_NODEID(nodeid);
	if(nodeid == atomic_read(&local_nodeid)) 	ERROR_RETURN(EMOLBADNODEID);
	n_ptr = &node[nodeid];

	RLOCK_NODE(n_ptr);
	if( n_ptr->n_usr.n_flags == NODE_FREE){
		RUNLOCK_NODE(n_ptr);
 		ERROR_RETURN(EMOLNODEFREE);
	}
	RUNLOCK_NODE(n_ptr);

	CHECK_DCID(dcid);
	dc_ptr 		= &dc[dcid];
	WLOCK_DC(dc_ptr);
	WLOCK_NODE(n_ptr);
	do {
		ret = OK;	
		if( dc_ptr->dc_usr.dc_flags) 		{ret = EMOLDCNOTRUN; break;}
		if(!test_bit(nodeid, &dc_ptr->dc_usr.dc_nodes))	{ret = EMOLDCNODE; break;}
		if(!test_bit(dcid, &n_ptr->n_usr.n_dcs))	{ret = EMOLDCNODE; break;} 
		clear_bit(nodeid, &dc_ptr->dc_usr.dc_nodes);
		clear_bit(dcid, &n_ptr->n_usr.n_dcs);
	}while(0);
	WUNLOCK_NODE(n_ptr);
	WUNLOCK_DC(dc_ptr);
	if(ret) ERROR_RETURN(ret);
	return(OK);
}


/*--------------------------------------------------------------*/
/*			mm_dc_init				*/
/* Initializa a DC and all of its processes			*/
/* returns the local_node ID					*/
/*--------------------------------------------------------------*/
asmlinkage long mm_dc_init(dc_usr_t *dcu_addr)
{
	int i, ret, cpu_id;
	long unsigned int *bm_ptr;
	dc_desc_t *dc_ptr;
	struct proc *proc_ptr;
	dc_usr_t dcu, *dcu_ptr;
	cpumask_t all_cpumask;
	cluster_node_t *n_ptr;
	int nodeid, proctab_size, order;
	
	MOLDEBUG(INTERNAL,"Current user=%d \n", current_uid());

	if(current_euid() != USER_ROOT) ERROR_RETURN(EMOLPRIVILEGES);

	if( DVS_NOT_INIT() )   ERROR_RETURN(EMOLDVSINIT);

	nodeid = atomic_read(&local_nodeid);

	/* ONLY FOR TESTING*/
	dcu_ptr = &dcu;

	cpu_id = get_cpu();
	MOLDEBUG(INTERNAL,"This process is running on cpu_id=%d \n", cpu_id);

	/* get DC parameters from Userspace */
	ret = copy_from_user(&dcu, dcu_addr, sizeof(dc_usr_t));
	if (ret) 	ERROR_RETURN(ret);
	do {
		if((dcu_ptr->dc_dcid        < 0) || (dcu_ptr->dc_dcid 	>= dvs.d_nr_dcs)) break;
		if((dcu_ptr->dc_nr_procs   <= 0) || (dcu_ptr->dc_nr_procs 	> dvs.d_nr_procs)) break;
		if((dcu_ptr->dc_nr_tasks   <  0) || (dcu_ptr->dc_nr_tasks 	> dvs.d_nr_tasks)) break;
		if((dcu_ptr->dc_nr_sysprocs < 0) || (dcu_ptr->dc_nr_sysprocs > dvs.d_nr_sysprocs)) break;
		if((dcu_ptr->dc_nr_nodes   <= 0) || (dcu_ptr->dc_nr_nodes 	> dvs.d_nr_nodes)) break;
		if(dcu_ptr->dc_nr_tasks < ((dcu_ptr->dc_nr_nodes*2)+NR_FIXED_TASKS)) 		break;
		if(dcu_ptr->dc_nr_sysprocs >= (dcu_ptr->dc_nr_procs+ dcu_ptr->dc_nr_tasks)) break;
		if(dcu_ptr->dc_nr_sysprocs <= dcu_ptr->dc_nr_tasks) 					break;
	}while(0);
	if(ret) ERROR_RETURN(EMOLBADRANGE);

	if( strlen(dcu_ptr->dc_name) >= MAXDCNAME ) ERROR_RETURN(EMOLNAMESIZE);
	
	/* checks for a repeated DC name or dcid already running */
	for( i = 0; i < dvs.d_nr_dcs; i++) {
		dc_ptr = &dc[i];
		RLOCK_DC(dc_ptr);
		if(dc_ptr->dc_usr.dc_flags == DC_RUNNING) {
			if(strcmp(dcu_ptr->dc_name, dc_ptr->dc_usr.dc_name) == 0)
				ERROR_RUNLOCK_DC(dc_ptr,EMOLRSCBUSY);	
			if( i == dcu_ptr->dc_dcid)
				ERROR_RUNLOCK_DC(dc_ptr,EMOLRSCBUSY);	
		}
		RUNLOCK_DC(dc_ptr);	
	}

	MOLDEBUG(INTERNAL,"Initializing DC=%d DCname=%s on node=%d\n",
		dcu_ptr->dc_dcid,dcu_ptr->dc_name, nodeid);
	dc_ptr = &dc[dcu_ptr->dc_dcid];

	WLOCK_DC(dc_ptr);

	/* Copy user DC configuration parameters parameters */
	dc_ptr->dc_usr.dc_dcid 		= dcu_ptr->dc_dcid;
	dc_ptr->dc_usr.dc_nr_procs 	= dcu_ptr->dc_nr_procs;
	dc_ptr->dc_usr.dc_nr_tasks 	= dcu_ptr->dc_nr_tasks;
	dc_ptr->dc_usr.dc_nr_sysprocs 	= dcu_ptr->dc_nr_sysprocs;
	dc_ptr->dc_usr.dc_nr_nodes 	= dcu_ptr->dc_nr_nodes;
	strncpy(dc_ptr->dc_usr.dc_name,dcu_ptr->dc_name,MAXDCNAME-1);
	
	dc_ptr->dc_usr.dc_flags 	= DC_RUNNING;
	dc_ptr->dc_usr.dc_nodes 	= 0;

	dc_ptr->dc_usr.dc_warn2proc 	= dcu_ptr->dc_warn2proc;
	dc_ptr->dc_usr.dc_warnmsg 	= dcu_ptr->dc_warnmsg;

	/*set the DC CPU affinity mask */
	cpumask_setall(&all_cpumask);
	if(cpumask_empty(&dcu_ptr->dc_cpumask))	{
		cpumask_setall(&dc_ptr->dc_usr.dc_cpumask);
	}else{
		/* test if the dc_cpumask is a subset of all_cpumask */
		if( cpumask_subset( &dcu_ptr->dc_cpumask, &all_cpumask)) {
			cpumask_copy(&dc_ptr->dc_usr.dc_cpumask, &dcu_ptr->dc_cpumask);
		} else {
			cpumask_setall(&dc_ptr->dc_usr.dc_cpumask);
		}
	}

	/* Set the DC NODEs bitmap */
	bm_ptr = &dc_ptr->dc_usr.dc_nodes;
	set_bit(nodeid, bm_ptr);
	
	/* Set the NODE DCs bitmap */
	n_ptr = &node[nodeid];
	WLOCK_NODE(n_ptr);
	bm_ptr = &node[nodeid].n_usr.n_dcs;
	set_bit(dc_ptr->dc_usr.dc_dcid, bm_ptr);
	WUNLOCK_NODE(n_ptr);

	/* ALLOC DYNAMIC memory for an array of pointers to structs proc */  
	proctab_size = sizeof_proc_aligned  * (dc_ptr->dc_usr.dc_nr_tasks + dc_ptr->dc_usr.dc_nr_procs);
	/* check if kalloc support the proc table size */
	for( order = 0; (PAGE_SIZE << order) < proctab_size; order++);

	MOLDEBUG(INTERNAL,"proctab_size=%d order=%d MAX_ORDER=%d\n", 
		proctab_size, order , MAX_ORDER);
		
	if( order >  (MAX_ORDER-1))
		ERROR_WUNLOCK_DC(dc_ptr, EMOLALLOCMEM);
		
//	dc_ptr->dc_proc = vmalloc( proctab_size); 
	/* It will be shared with userspace */
	dc_ptr->dc_proc = kmalloc(proctab_size, GFP_KERNEL);
	if(dc_ptr->dc_proc == NULL)	
		ERROR_WUNLOCK_DC(dc_ptr, EMOLALLOCMEM);
	dc_ptr->dc_ref_dgb = 0; /* mmap open reference count */

	/* ALLOC DYNAMIC memory for an array of pointers to structs proc pointers */  
//	dc_ptr->dc_sid2proc = vmalloc(sizeof(struct proc *) * dc_ptr->dc_usr.dc_nr_sysprocs); 
	dc_ptr->dc_sid2proc = kmalloc(sizeof(struct proc *) * dc_ptr->dc_usr.dc_nr_sysprocs, GFP_KERNEL); 
	if(dc_ptr->dc_sid2proc == NULL)	{
		kfree(dc_ptr->dc_proc);
		ERROR_WUNLOCK_DC(dc_ptr, EMOLALLOCMEM);
	}

	MOLDEBUG(INTERNAL,"Initializing DC=%d proc[] table on dc_proc=%p\n", dc_ptr->dc_usr.dc_dcid, dc_ptr->dc_proc);
	FOR_EACH_PROC(dc_ptr, i) {
		proc_ptr 		= DC_PROC(dc_ptr,i);
		strcpy(proc_ptr->p_usr.p_name, "$noname");
		init_proc_desc(proc_ptr, dc_ptr->dc_usr.dc_dcid, i);
		proc_ptr->p_pseudosem 	= 1;
		proc_ptr->p_usr.p_endpoint	= _ENDPOINT(0,i-dc_ptr->dc_usr.dc_nr_tasks);
		proc_ptr->p_usr.p_rts_flags	= SLOT_FREE;
		PROC_LOCK_INIT(proc_ptr);
	} 

	MOLDEBUG(INTERNAL,"Initializing %d element of system process vector at dc_sid2proc=%p \n", 
		dc_ptr->dc_usr.dc_nr_sysprocs,dc_ptr->dc_sid2proc);

	for (i = 0; i < (dc_ptr->dc_usr.dc_nr_sysprocs); i++) 
		dc_ptr->dc_sid2proc[i] = NULL;


	/*creates DCx directory under /sys/kernel/debug/dvs */
	dc_ptr->dc_DC_dbg = debugfs_create_dir(dc_ptr->dc_usr.dc_name, dbg_dvs);
	MOLDEBUG(INTERNAL,"Creating %s directory on debugfs\n",dc_ptr->dc_usr.dc_name);
	if (dc_ptr->dc_DC_dbg == NULL) {
		kfree(dc_ptr->dc_proc);
		vfree(dc_ptr->dc_sid2proc);
		printk("ERROR: %d:%s:%u: Couldn't create %s under /sys/kernel/debug/dvs\n",
			task_pid_nr(current), __FUNCTION__ ,__LINE__, dc_ptr->dc_usr.dc_name); 
		WUNLOCK_DC(dc_ptr);
		ERROR_RETURN(EMOLBADDIR);
	}

	/*creates "procs" file under /sys/kernel/debug/dvs/<DCname>/procs */
	dc_ptr->dc_procs_dbg = debugfs_create_file("procs", 0644, dc_ptr->dc_DC_dbg, NULL, &proc_dbg_fops);
	
	/*creates DC directory under /proc/dvs */
	dc_ptr->dc_DC_dir = proc_mkdir(dc_ptr->dc_usr.dc_name, proc_dvs_dir);
		
	/*creates DC entry under /proc/dvs/DCxx */
	ret = OK;
	do {
		dc_ptr->dc_info_entry = create_proc_entry("info", 0444, dc_ptr->dc_DC_dir);
		if (dc_ptr->dc_info_entry == NULL) {
			printk("ERROR: %d:%s:%u: Couldn't create info under /proc/dvs/%s\n",
				task_pid_nr(current), __FUNCTION__ ,__LINE__, dc_ptr->dc_usr.dc_name); 
			ret = EMOLBADDIR;
			break;
		} else {
			dc_ptr->dc_info_entry->read_proc = dc_info_read;
			dc_ptr->dc_info_entry->data = (void*) &dc_ptr->dc_usr.dc_dcid;
			MOLDEBUG(GENERIC,"/proc/dvs/%s/info installed\n", dc_ptr->dc_usr.dc_name);
		}

		/*creates DC entry under /proc/dvs/DCxx/procs */
		dc_ptr->dc_procs_entry = create_proc_entry("procs", 0444, dc_ptr->dc_DC_dir);
		if (dc_ptr->dc_procs_entry == NULL) {
			printk("ERROR: %d:%s:%u: Couldn't create procs under /proc/dvs/%s\n",
				task_pid_nr(current), __FUNCTION__ ,__LINE__, dc_ptr->dc_usr.dc_name); 
			ret = EMOLBADFILE;
			break;
		} else {
			dc_ptr->dc_procs_entry->read_proc = dc_procs_read;
			dc_ptr->dc_procs_entry->data = (void*) &dc_ptr->dc_usr.dc_dcid;
			MOLDEBUG(GENERIC,"/proc/dvs/%s/procs installed\n", dc_ptr->dc_usr.dc_name);

		}
		/*creates DC entry under /proc/dvs/DCxx/stats */
		dc_ptr->dc_stats_entry = create_proc_entry("stats", 0444, dc_ptr->dc_DC_dir);
		if (dc_ptr->dc_stats_entry == NULL) {
			printk("ERROR: %d:%s:%u: Couldn't create stats under /proc/dvs/%s\n",
				task_pid_nr(current), __FUNCTION__ ,__LINE__, dc_ptr->dc_usr.dc_name); 
			ret = EMOLBADFILE;
			break;
		} else {
			dc_ptr->dc_stats_entry->read_proc = dc_stats_read;
			dc_ptr->dc_stats_entry->data = (void*) &dc_ptr->dc_usr.dc_dcid;
			MOLDEBUG(GENERIC,"/proc/dvs/%s/stats installed\n", dc_ptr->dc_usr.dc_name);
		}
	}while(0);
	kref_init(&dc_ptr->dc_kref);	/* initializa DC reference counter = 1	*/
//	DC_INCREF(dc_ptr);					/* increment DC reference counter 	*/
	KREF_GET(&dvs_kref); 			/* increment DVS reference counter 	*/

    if(ret) {
//		kfree(dc_ptr->dc_proc);
//		vfree(dc_ptr->dc_sid2proc);
//		dc_release(&dc_ptr->dc_kref);
		DC_DECREF(dc_ptr,dc_release);
		WUNLOCK_DC(dc_ptr);
		ERROR_RETURN(ret);
	}

	dcu_ptr = &dc_ptr->dc_usr;	
	MOLDEBUG(INTERNAL,DC_CPU_FORMAT, DC_CPU_FIELDS(dcu_ptr));
	MOLDEBUG(INTERNAL,DC_WARN_FORMAT, DC_WARN_FIELDS(dcu_ptr));

	WUNLOCK_DC(dc_ptr);
	return(nodeid);
}

/*--------------------------------------------------------------*/
/*			mm_dc_dmp				*/
/* Dumps a table of all DCs into dmesg buffer			*/
/*--------------------------------------------------------------*/
asmlinkage long mm_dc_dump(void)
{
	struct  dc_struct *dc_ptr;
	int i;

	if( DVS_NOT_INIT() )   ERROR_RETURN(EMOLDVSINIT );

	printk("DCID\tFLAGS\tNAME NODESbitmap\n");
	for (i = 0; i < dvs.d_nr_dcs; i++) {
		dc_ptr = &dc[i];
		RLOCK_DC(dc_ptr);
		if( !dc_ptr->dc_usr.dc_flags)
			printk("%d\t%X\t%s\t%lX\n",
				dc_ptr->dc_usr.dc_dcid,
				dc_ptr->dc_usr.dc_flags,
				dc_ptr->dc_usr.dc_name,
				dc_ptr->dc_usr.dc_nodes);
		RUNLOCK_DC(dc_ptr);
	} 
	return(OK);
}

/*--------------------------------------------------------------*/
/*			mm_getdvsinfo				*/
/* On return: if (ret >= 0 ) return local_nodeid 		*/
/*         and the DVS configuration  parameters 		*/
/* if ret == -1, the DVS has not been initialized		*/
/* if ret < -1, a copy_to_user error has ocurred		*/
/*--------------------------------------------------------------*/
asmlinkage long mm_getdvsinfo(dvs_usr_t *dvs_usr_ptr)
{
	int rcode;
	int nodeid; 
	MOLDEBUG(DBGPARAMS,"local_nodeid=%d \n",atomic_read(&local_nodeid));
	
	if(current_euid() != USER_ROOT) ERROR_RETURN(EMOLPRIVILEGES);

	RLOCK_DVS;
	COPY_TO_USER_PROC(rcode, &dvs, dvs_usr_ptr, sizeof(dvs_usr_t));
	nodeid =atomic_read(&local_nodeid);
	RUNLOCK_DVS;
	if(rcode) ERROR_RETURN(rcode);
	return(nodeid);
}


/*--------------------------------------------------------------*/
/*			mm_getdcinfo				*/
/* Copies the DC entry to userspace				*/
/*--------------------------------------------------------------*/
asmlinkage long mm_getdcinfo(int dcid, dc_usr_t *dc_usr_ptr)
{
	int rcode;
	dc_desc_t *dc_ptr;
	struct proc *caller_ptr;
	int caller_pid;
	struct task_struct *task_ptr;	

	MOLDEBUG(DBGPARAMS,"dcid=%d \n",dcid);
	if( DVS_NOT_INIT() ) 			ERROR_RETURN(EMOLDVSINIT);
	if(current_euid() != USER_ROOT) ERROR_RETURN(EMOLPRIVILEGES);

	rcode = check_caller(&task_ptr, &caller_ptr, &caller_pid);
	if( dcid == PROC_NO_PID) {
		if(rcode) ERROR_RETURN(EMOLBADDCID);
		dcid = caller_ptr->p_usr.p_dcid;
	}

	CHECK_DCID(dcid);		/* check DC ID limits 	*/
	dc_ptr 		= &dc[dcid];
	RLOCK_DC(dc_ptr);
	COPY_TO_USER_PROC(rcode, &dc_ptr->dc_usr, dc_usr_ptr, sizeof(dc_usr_t));
	RUNLOCK_DC(dc_ptr);	
	return(rcode);
}

/*--------------------------------------------------------------*/
/*			mm_getnodeinfo				*/
/* Copies the NODE entry to userspace				*/
/*--------------------------------------------------------------*/
asmlinkage long mm_getnodeinfo(int nodeid, node_usr_t *node_usr_ptr)
{
	int rcode;
	cluster_node_t  *n_ptr;
    node_usr_t  *nu_ptr;
	struct proc *caller_ptr;
	int caller_pid;
	struct task_struct *task_ptr;	


	MOLDEBUG(DBGPARAMS,"nodeid=%d \n",nodeid);
	if(current_euid() != USER_ROOT) ERROR_RETURN(EMOLPRIVILEGES);
	if( DVS_NOT_INIT() )   		ERROR_RETURN(EMOLDVSINIT );
	
	rcode = check_caller(&task_ptr, &caller_ptr, &caller_pid);
	if( nodeid == PROC_NO_PID) {
		if( rcode) ERROR_RETURN(EMOLBADNODEID);
		nodeid = caller_ptr->p_usr.p_nodeid;
	}
	
	CHECK_NODEID(nodeid);	/* check node ID limits 	*/

	n_ptr 	= &node[nodeid];
	nu_ptr = &n_ptr->n_usr;
	MOLDEBUG(DBGPARAMS,NODE_USR_FORMAT,NODE_USR_FIELDS(nu_ptr));

	RLOCK_NODE(n_ptr);
	COPY_TO_USER_PROC(rcode, &n_ptr->n_usr, node_usr_ptr, sizeof(node_usr_t));
	RUNLOCK_NODE(n_ptr);

	return(rcode);
}


/*--------------------------------------------------------------*/
/*			mm_getprocinfo				*/
/* Copies a proc descriptor to userspace			*/
/*--------------------------------------------------------------*/
asmlinkage long mm_getprocinfo(int dcid, int p_nr, struct proc_usr *proc_usr_ptr)
{
	int rcode;
	dc_desc_t *dc_ptr;
	struct proc *proc_ptr;
	struct proc *caller_ptr;
	int caller_pid;
	struct task_struct *task_ptr;	

	MOLDEBUG(DBGPARAMS,"dcid=%d p_nr=%d\n",dcid, p_nr);
	if(current_euid() != USER_ROOT) 	ERROR_RETURN(EMOLPRIVILEGES);
	if( DVS_NOT_INIT() )   			ERROR_RETURN(EMOLDVSINIT );

	rcode = check_caller(&task_ptr, &caller_ptr, &caller_pid);
	if( dcid == PROC_NO_PID) {
		if( rcode) ERROR_RETURN(rcode);
		dcid = caller_ptr->p_usr.p_dcid;
	}
	
	CHECK_DCID(dcid);		/* check DC ID limits 	*/
	dc_ptr 		= &dc[dcid];

	RLOCK_DC(dc_ptr);	
	if( dc_ptr->dc_usr.dc_flags) 			ERROR_RUNLOCK_DC(dc_ptr,EMOLDCNOTRUN);
	if( p_nr < (-dc_ptr->dc_usr.dc_nr_tasks) 		
	 || p_nr >= dc_ptr->dc_usr.dc_nr_procs) 	ERROR_RUNLOCK_DC(dc_ptr,EMOLBADPROC);
	proc_ptr = NBR2PTR(dc_ptr,p_nr);
	RUNLOCK_DC(dc_ptr);

	RLOCK_PROC(proc_ptr);
	MOLDEBUG(INTERNAL,"lpid=%d name=%s\n", proc_ptr->p_usr.p_lpid, (char*)&proc_ptr->p_usr.p_name);
	COPY_TO_USER_PROC(rcode, &proc_ptr->p_usr, proc_usr_ptr, sizeof(struct proc_usr));
	RUNLOCK_PROC(proc_ptr);
	return(rcode);
}

/*--------------------------------------------------------------*/
/*			mm_getproxyinfo				*/
/* Copies a sproxy and rproxy proc descriptor to userspace  */
/*--------------------------------------------------------------*/
asmlinkage long mm_getproxyinfo(int px_nr,  struct proc_usr *sproc_usr_ptr, struct proc_usr *rproc_usr_ptr)
{
	int rcode;
	struct proc *sproxy_ptr, *rproxy_ptr;

	MOLDEBUG(DBGPARAMS,"px_nr=%d\n", px_nr);

	rcode = lock_sr_proxies(px_nr,  &sproxy_ptr, &rproxy_ptr);
	if(rcode != OK) ERROR_RETURN(rcode);

	COPY_TO_USER_PROC(rcode, &proxies[px_nr].px_sproxy, sproc_usr_ptr, sizeof(struct proc_usr));
	COPY_TO_USER_PROC(rcode, &proxies[px_nr].px_rproxy, rproc_usr_ptr, sizeof(struct proc_usr));

	rcode = unlock_sr_proxies(px_nr);
	if(rcode != OK) ERROR_RETURN(rcode);

	return(rcode);
}

/*--------------------------------------------------------------*/
/*			mm_proc_dmp				*/
/* Dumps a table of process fields of a DC into dmesg buffer	*/
/*--------------------------------------------------------------*/
asmlinkage long mm_proc_dump(int dcid)
{
	int i;
	struct proc *proc_ptr;
	dc_desc_t *dc_ptr;

	if( DVS_NOT_INIT() )   ERROR_RETURN(EMOLDVSINIT );

	CHECK_DCID(dcid);
	dc_ptr 		= &dc[dcid];
	
	RLOCK_DC(dc_ptr);		
	/* checks if the DC we are talking about is running */
	CHECK_IF_DC_RUN(dc_ptr);

	printk("DCID\tNR\tENDP\tLPID\tNODE\tFLAGS\tGETF\tSNDT\tWITM\tLSENT\tRSENT\n");
	FOR_EACH_PROC(dc_ptr, i) {
		proc_ptr = DC_PROC(dc_ptr,i);
		RLOCK_PROC(proc_ptr);
		if ( !test_bit(BIT_SLOT_FREE, &proc_ptr->p_usr.p_rts_flags)) {
			printk("%d\t%d\t%d\t%d\t%d\t%lX\t%d\t%d\t%d\t%ld\t%ld\n",
			proc_ptr->p_usr.p_dcid,
			proc_ptr->p_usr.p_nr,
			proc_ptr->p_usr.p_endpoint,
			proc_ptr->p_usr.p_lpid,
			proc_ptr->p_usr.p_nodeid,
			proc_ptr->p_usr.p_rts_flags,
			proc_ptr->p_usr.p_getfrom,
			proc_ptr->p_usr.p_sendto,
			proc_ptr->p_usr.p_waitmigr,
			proc_ptr->p_usr.p_lclsent,
			proc_ptr->p_usr.p_rmtsent);
		}
		RUNLOCK_PROC(proc_ptr);
	} 
	RUNLOCK_DC(dc_ptr);				/* UP DC_MUTEX*/
	return(OK);
}
/*--------------------------------------------------------------*/
/*			mm_bind				*/
/* Binds (an Initialize) a Linux process to the IPC Kernel	*/
/* Who can call bind?:						*/
/* - The main thread of a process to bind itself (mnx_bind)	*/
/* - The child thread of a process to bind itself (mnx_bind)	*/
/* - A MOL process that bind a local process (mnx_lclbind)	*/
/* - A MOL process that bind a remote process (mnx_rmtbind)	*/
/* - A MOL process that bind a local process that is a backup of*/
/*	a remote active process (mnx_bkupbind). Then, with	*/
/*	mm_migrate, the backup process can be converted into   */
/*	the primary process					*/
/* -  A MOL process that bind a local REPLICATED process (mnx_repbind)	*/
/* Local process: proc = proc number				*/
/* Remote  process: proc = endpoint 				*/
/*--------------------------------------------------------------*/
asmlinkage long mm_bind(int oper, int dcid, int pid, int endpoint, int nodeid)
{
	struct proc *proc_ptr, *rproxy_ptr, *sproxy_ptr, *leader;
	proc_usr_t *uproc_ptr;
	dc_desc_t *dc_ptr;
	int i, p_nr, rcode;
	long unsigned int *bm_ptr;
	struct task_struct *task_ptr, *leader_ptr;
	char *uname_ptr;

	if(current_euid() != USER_ROOT) ERROR_RETURN(EMOLPRIVILEGES);

	if( DVS_NOT_INIT() )   ERROR_RETURN(EMOLDVSINIT );

	MOLDEBUG(DBGPARAMS,"oper=%d dcid=%d pid=%d endpoint=%d nodeid=%d\n",oper, dcid, pid, endpoint, nodeid);

	switch(oper){
		case SELF_BIND:
		case LCL_BIND:
		case REPLICA_BIND:	
			if( nodeid != LOCALNODE) ERROR_RETURN(EMOLBADNODEID);
			nodeid = atomic_read(&local_nodeid);
			p_nr = _ENDPOINT_P(endpoint);
			break;
		case RMT_BIND:
		case BKUP_BIND:
			p_nr = _ENDPOINT_P(endpoint);
			CHECK_NODEID(nodeid);	/* check node ID limits 	*/
			if( nodeid == atomic_read(&local_nodeid)) ERROR_RETURN(EMOLBADNODEID);
			break;
		default:
			ERROR_RETURN(EMOLBINDTYPE);
	}
	
	CHECK_DCID(dcid);		/* check DC ID limits 	*/
	dc_ptr 		= &dc[dcid];
	RLOCK_DC(dc_ptr);
	do {	
		rcode = 0;
		if( dc_ptr->dc_usr.dc_flags) 			{rcode = EMOLDCNOTRUN; break;}
		if( p_nr < (-dc_ptr->dc_usr.dc_nr_tasks) 		
		 || p_nr >= dc_ptr->dc_usr.dc_nr_procs) 	{rcode = EMOLBADDCID; break;}
		bm_ptr = &dc_ptr->dc_usr.dc_nodes;
		if(!test_bit(nodeid, bm_ptr)) 			{rcode = EMOLNODCNODE; break;} 
		i = p_nr+dc_ptr->dc_usr.dc_nr_tasks;
		proc_ptr = DC_PROC(dc_ptr,i);
	}while(0);
	if(rcode) ERROR_RUNLOCK_DC(dc_ptr, rcode);

	WLOCK_PROC(proc_ptr);
	if( !test_bit(BIT_SLOT_FREE, &proc_ptr->p_usr.p_rts_flags)){
		if ( oper != BKUP_BIND && oper != REPLICA_BIND ) {
			uproc_ptr = &proc_ptr->p_usr;
			MOLDEBUG(INTERNAL,PROC_USR_FORMAT,PROC_USR_FIELDS(uproc_ptr));
			WUNLOCK_PROC(proc_ptr);
			ERROR_RUNLOCK_DC(dc_ptr,EMOLSLOTUSED);
		}
	} 

	init_proc_desc(proc_ptr, dcid, i);		/* Initialize all process' descriptor fields - PARANOID	*/

	if( oper != RMT_BIND ) {	/* LOCAL PROCESS */
		/*
		* INPUT: process PID 
		* OUTPUT: task pointer
		*/
		if( oper == SELF_BIND){
			if( pid == task_pid_nr(current)) { /* THREAD SELF BIND */
				//mol_stub_syscall5(BIND, SELF_BIND, dcid, (pid_t) syscall (SYS_gettid), endpoint, LOCALNODE)
				task_ptr = current;	
			}else{						/* MAIN THREAD BIND */
				//mol_stub_syscall5(BIND, SELF_BIND, dcid, getpid(), endpoint, LOCALNODE)
//			if(!thread_group_leader(task_ptr)) { /* The caller process is NOT the Group Leader 	*/
				if( current->tgid == pid){		/* the child thread has called bind	*/		
					lock_tasklist(); //read_lock(&tasklist_lock);
					task_ptr = pid_task(find_vpid(pid), PIDTYPE_PID);  
					if(task_ptr == NULL ) {
						unlock_tasklist(); //read_unlock(&tasklist_lock);
						WUNLOCK_PROC(proc_ptr);
						ERROR_RUNLOCK_DC(dc_ptr, EMOLBADPROC);
					}
					unlock_tasklist(); //read_unlock(&tasklist_lock);
				}else{
					WUNLOCK_PROC(proc_ptr);
					ERROR_RUNLOCK_DC(dc_ptr, EMOLBADPROC);
				}
			}
			MOLDEBUG(INTERNAL,"SELF_BIND PID=%d\n", task_pid_nr(task_ptr));
		}else{
			// mol_stub_syscall5(BIND, LCL_BIND, dcid, pid, endpoint, LOCALNODE)
			// mol_stub_syscall5(BIND, REPLICA_BIND, dcid, pid, endpoint, LOCALNODE)
			// mol_stub_syscall5(BIND, BKUP_BIND, dcid, pid, endpoint, nodeid);
			if( pid == task_pid_nr(current)) { /* THREAD SELF BIND */
				task_ptr = current;	
			}else{
				lock_tasklist(); //read_lock(&tasklist_lock);
				task_ptr = pid_task(find_vpid(pid), PIDTYPE_PID);  
				if(task_ptr == NULL ) {
					unlock_tasklist(); //read_unlock(&tasklist_lock);
					WUNLOCK_PROC(proc_ptr);
					ERROR_RUNLOCK_DC(dc_ptr, EMOLBADPROC);
				}
				unlock_tasklist(); //read_unlock(&tasklist_lock);
			}
			switch(oper) {
				case LCL_BIND:
					MOLDEBUG(INTERNAL,"LCL_BIND Linux Task PID=%d\n", task_pid_nr(task_ptr));
					break;
				case BKUP_BIND:
					MOLDEBUG(INTERNAL,"BKUP_BIND Linux Task PID=%d\n", task_pid_nr(task_ptr));	
					break;
				case REPLICA_BIND:
					MOLDEBUG(INTERNAL,"REPLICA_BIND Linux Task PID=%d\n", task_pid_nr(task_ptr));	
					break;
				default:
					ERROR_RETURN(EMOLBINDTYPE);
					break;
			}

		}
		WUNLOCK_PROC(proc_ptr);

		WLOCK_TASK(task_ptr);	
		WLOCK_PROC(proc_ptr);

		/*
		* task_ptr is LOCKED
		* proc_ptr is LOCKED
		* dc_ptr is LOCKED
		* Check that the MAIN thread is binded to the same DC !!
		*/
		if(!thread_group_leader(task_ptr)) {
			leader_ptr= task_ptr->group_leader;		/* get the leader task pointer 		*/
			WLOCK_TASK(leader_ptr);
			leader =  leader_ptr->task_proc;		/* get the leader process pointer 		*/
			if( leader == NULL)	{					/* Main thread not bound 			*/
				WUNLOCK_TASK(leader_ptr);
				WUNLOCK_PROC(proc_ptr);
				WUNLOCK_TASK(task_ptr);			
				ERROR_RUNLOCK_DC(dc_ptr, EMOLGRPLEADER);				
			}
			RLOCK_PROC(leader);
			if(leader->p_usr.p_dcid != dcid) {
				RUNLOCK_PROC(leader);	
				WUNLOCK_TASK(leader_ptr);
				WUNLOCK_PROC(proc_ptr);
				WUNLOCK_TASK(task_ptr);			
				ERROR_RUNLOCK_DC(dc_ptr, EMOLBADPROC);				
			}
			RUNLOCK_PROC(leader);	
			WUNLOCK_TASK(leader_ptr);	
		}
		
		MOLDEBUG(INTERNAL,"increment the reference count of the task struct=%d count=%d\n"
			,task_pid_nr(task_ptr),atomic_read(&task_ptr->usage)); 
		get_task_struct(task_ptr);			/* increment the reference count of the task struct */
		proc_ptr->p_task = task_ptr;		/* Set the task descriptor into the process descriptor */
		task_ptr->task_proc = (struct proc *) proc_ptr;	/* Set the  process descriptor into the task descriptor */
		strncpy((char* )&proc_ptr->p_usr.p_name, (char*)task_ptr->comm, MAXPROCNAME-1);
		proc_ptr->p_name_ptr = (char*)task_ptr->comm;
		if( thread_group_leader(task_ptr)) 
			proc_ptr->p_usr.p_misc_flags = MIS_GRPLEADER;	/* The proccess is the thread group leader 	*/	
		
		MOLDEBUG(INTERNAL,"process p_name=%s *p_name_ptr=%s\n", 
			(char*)&proc_ptr->p_usr.p_name, proc_ptr->p_name_ptr);
		
		if( oper == BKUP_BIND) {
			proc_ptr->p_usr.p_rts_flags	= REMOTE;	/* appears as if it is REMOTE		*/
			proc_ptr->p_usr.p_misc_flags	|= MIS_RMTBACKUP;/* It is a remote process' backup 	*/
			proc_ptr->p_usr.p_nodeid	= nodeid;	/* The primary process node's ID 	*/	
		}else{
			proc_ptr->p_usr.p_rts_flags	= PROC_RUNNING;	/* set to RUNNING STATE	*/
			proc_ptr->p_usr.p_nodeid	= atomic_read(&local_nodeid);
			if( oper == REPLICA_BIND)
				proc_ptr->p_usr.p_misc_flags |= MIS_REPLICATED;/* It is a replicated process 	*/
		}

		proc_ptr->p_usr.p_lpid 	= pid;			/* Update PID		*/
		if( i < dc_ptr->dc_usr.dc_nr_sysprocs) {
			proc_ptr->p_priv.s_usr.s_id = i;
		}else{
			proc_ptr->p_priv.s_usr.s_id = dc_ptr->dc_usr.dc_nr_sysprocs;
		}
		cpumask_copy(&proc_ptr->p_usr.p_cpumask, &dc_ptr->dc_usr.dc_cpumask);
		
	}else{ /* REMOTE  PROCESS */
		do {
			rcode = lock_sr_proxies(node[nodeid].n_usr.n_proxies,  &sproxy_ptr, &rproxy_ptr);
			if( rcode != OK) break;

			if ( rproxy_ptr->p_usr.p_rts_flags == SLOT_FREE || 
				 sproxy_ptr->p_usr.p_rts_flags == SLOT_FREE ){
				rcode = EMOLNOPROXY;
				break;
			}					
		}while(0);
		rcode = unlock_sr_proxies(node[nodeid].n_usr.n_proxies);
		if( rcode != OK) {
	 		WUNLOCK_PROC(proc_ptr);
			ERROR_RUNLOCK_DC(dc_ptr,EMOLNOPROXY);
		}
		
		proc_ptr->p_usr.p_rts_flags = REMOTE;			
		proc_ptr->p_usr.p_nodeid   = nodeid;
		if( i < dc_ptr->dc_usr.dc_nr_sysprocs) {
			proc_ptr->p_priv.s_usr.s_id = i;
		}else{
			proc_ptr->p_priv.s_usr.s_id = dc_ptr->dc_usr.dc_nr_sysprocs;
		}
		uname_ptr = (char *) pid; /* if LOCAL => pid=PID, if REMOTE => pid= name[] */ 
		rcode = copy_from_user(proc_ptr->p_usr.p_name,uname_ptr,MAXPROCNAME-1);
	}

	set_bit(proc_ptr->p_usr.p_nodeid, &proc_ptr->p_usr.p_nodemap);

	uproc_ptr = &proc_ptr->p_usr;
	MOLDEBUG(INTERNAL,PROC_USR_FORMAT,PROC_USR_FIELDS(uproc_ptr));
	MOLDEBUG(INTERNAL,PROC_CPU_FORMAT,PROC_CPU_FIELDS(uproc_ptr));

	endpoint = proc_ptr->p_usr.p_endpoint;
	
	/* Wake up the waiting process on mnx_wait4bind() */
	if( oper == LCL_BIND || oper == REPLICA_BIND || oper == BKUP_BIND){
		if( current != task_ptr ){
			/* Wakes up it as if has in wait4bind state  */
			if( waitqueue_active(&task_ptr->task_wqh))
				wake_up_interruptible(&task_ptr->task_wqh); 
		}
	}
		
	WUNLOCK_PROC(proc_ptr);
	if( oper != RMT_BIND)
		WUNLOCK_TASK(task_ptr);
	DC_INCREF(dc_ptr);
	RUNLOCK_DC(dc_ptr);	

	return(endpoint);
}

/*--------------------------------------------------------------*/
/*			do_autobind				*/
/* Binds (an Initialize) a REMOTE process descriptor 		*/
/* DC must be locked						*/
/* the process descriptor must be locked			*/
/*--------------------------------------------------------------*/
long do_autobind(dc_desc_t *dc_ptr, struct proc *rmt_ptr, int endpoint, int nodeid)
{
	struct proc *proc_ptr, *rproxy_ptr, *sproxy_ptr;
	int i, p_nr, rcode;
	proc_usr_t *uproc_ptr;
	long unsigned int *bm_ptr;

	MOLDEBUG(DBGPARAMS,"dcid=%d endpoint=%d nodeid=%d \n",dc_ptr->dc_usr.dc_dcid, endpoint, nodeid);

	CHECK_DCID(dc_ptr->dc_usr.dc_dcid);		/* check DC ID limits 	*/

	p_nr = _ENDPOINT_P(endpoint);
	if( p_nr < (-dc_ptr->dc_usr.dc_nr_tasks) 		
	 || p_nr >= dc_ptr->dc_usr.dc_nr_procs) 	ERROR_RETURN(EMOLBADDCID);

	if( rmt_ptr->p_usr.p_nr != p_nr)		ERROR_RETURN(EMOLBADPROC);

	CHECK_NODEID(nodeid);	/* check node ID limits 	*/

	bm_ptr = &dc_ptr->dc_usr.dc_nodes;
	if(!test_bit(nodeid, bm_ptr)) 			ERROR_RETURN(EMOLNODCNODE);
	i = p_nr+dc_ptr->dc_usr.dc_nr_tasks;
	proc_ptr = DC_PROC(dc_ptr,i);
	if(!test_bit(BIT_SLOT_FREE, &proc_ptr->p_usr.p_rts_flags))	ERROR_RETURN(EMOLSLOTUSED);

	init_proc_desc(proc_ptr, dc_ptr->dc_usr.dc_dcid, i);	/* Initialize all process descriptor fields	*/

	if( nodeid == atomic_read(&local_nodeid)) 	ERROR_RETURN(EMOLBADNODEID);

	rcode = lock_sr_proxies(node[nodeid].n_usr.n_proxies,  &sproxy_ptr, &rproxy_ptr);
	if(rcode != OK) 							ERROR_RETURN(rcode);

	/* To bind a REMOTE processes, the local proxies must be active */
	if ( rproxy_ptr->p_usr.p_rts_flags == SLOT_FREE || 
		 sproxy_ptr->p_usr.p_rts_flags == SLOT_FREE ) {
		rcode = unlock_sr_proxies(node[nodeid].n_usr.n_proxies);
		if(rcode != OK)		ERROR_RETURN(rcode);
		ERROR_RETURN(EMOLNOPROXY);
	}
	rcode = unlock_sr_proxies(node[nodeid].n_usr.n_proxies);
	if(rcode != OK)		ERROR_RETURN(rcode);


	proc_ptr->p_usr.p_rts_flags = REMOTE;		/* set to RUNNING on REMOTE host */
	proc_ptr->p_usr.p_endpoint = endpoint;
	proc_ptr->p_usr.p_nodeid   = nodeid;
	if( i < dc_ptr->dc_usr.dc_nr_sysprocs) {
		proc_ptr->p_priv.s_usr.s_id = i;
	}else{
		proc_ptr->p_priv.s_usr.s_id = dc_ptr->dc_usr.dc_nr_sysprocs;
	}

	strcpy(proc_ptr->p_usr.p_name,"remote");
	
	set_bit(proc_ptr->p_usr.p_nodeid, &proc_ptr->p_usr.p_nodemap);

	proc_ptr->p_usr.p_cpumask = dc_ptr->dc_usr.dc_cpumask;
	endpoint = proc_ptr->p_usr.p_endpoint;

	uproc_ptr = &proc_ptr->p_usr;
	MOLDEBUG(INTERNAL,PROC_USR_FORMAT,PROC_USR_FIELDS(uproc_ptr));
	MOLDEBUG(INTERNAL,PROC_CPU_FORMAT,PROC_CPU_FIELDS(uproc_ptr));
	
	DC_INCREF(dc_ptr);
	return(endpoint);
}

/*----------------------------------------------------------------*/
/*			mm_void3				*/
/*----------------------------------------------------------------*/
asmlinkage long mm_void3(void)
{
MOLDEBUG(GENERIC,"MOL_VOID3 \n");
return(OK);
}

/*----------------------------------------------------------------*/
/*			mm_void18			*/
/*----------------------------------------------------------------*/
asmlinkage long mm_void18(void)
{
MOLDEBUG(GENERIC,"MOL_VOID18 \n");
return(OK);
}

/*----------------------------------------------------------------*/
/*			do_dc_end				*/
/*DC MUTEX Must be WRITE LOCKED					*/
/* When a DC ends:						*/
/*	- Sends SIGPIPE to every LOCAL process of the DC	*/
/*	- Waits that all LOCAL processes unbind by their self 	*/
/*	*/
/*----------------------------------------------------------------*/
long do_dc_end(dc_desc_t *dc_ptr)
{
	int i, ret;
	struct proc *proc_ptr; 	
	struct task_struct *task_ptr;	
	wait_queue_head_t wqhead;

	if(dc_ptr->dc_usr.dc_flags == DC_FREE) {
		ERROR_RETURN(EMOLDCNOTRUN);
	}
	
	init_waitqueue_head(&wqhead);
	
	MOLDEBUG(INTERNAL,"Send a SIGPIPE SIGNAL to all LOCAL processes belonging to the DC:%d\n",dc_ptr->dc_usr.dc_dcid);
	
//	LOCK_ALL_PROCS(dc_ptr, tmp_ptr, i);
	FOR_EACH_PROC(dc_ptr, i) {
		proc_ptr = DC_PROC(dc_ptr,i);
		WLOCK_PROC(proc_ptr);
		if(!test_bit(BIT_SLOT_FREE, &proc_ptr->p_usr.p_rts_flags)) {
			if( IT_IS_LOCAL(proc_ptr) || test_bit(MIS_BIT_RMTBACKUP, &proc_ptr->p_usr.p_misc_flags) ) {
				task_ptr = proc_ptr->p_task;
				ret = send_sig_info(SIGKILL , SEND_SIG_NOINFO, task_ptr);
				if(ret) ERROR_PRINT(ret);
			}else{
				do_unbind(dc_ptr, proc_ptr);
			}
		}
		WUNLOCK_PROC(proc_ptr);
	}
//	UNLOCK_ALL_PROCS(dc_ptr, tmp_ptr, i);

	/* Wait for the termination of all LOCAL DC process */
	FOR_EACH_PROC(dc_ptr, i) {
		proc_ptr = DC_PROC(dc_ptr,i);
		RLOCK_PROC(proc_ptr);
		while( !test_bit(BIT_SLOT_FREE, &proc_ptr->p_usr.p_rts_flags)) {
			if( IT_IS_LOCAL(proc_ptr) || test_bit(MIS_BIT_RMTBACKUP, &proc_ptr->p_usr.p_misc_flags) ) {
				task_ptr = proc_ptr->p_task;
				mm_sys_wait4(task_pid_nr(task_ptr), (int __user *)&ret, 0, NULL);
				RUNLOCK_PROC(proc_ptr);	
				WUNLOCK_DC(dc_ptr);
				schedule();
				WLOCK_DC(dc_ptr);
				RLOCK_PROC(proc_ptr);	
			}
		}
		RUNLOCK_PROC(proc_ptr);
	}

	/* ALL DC processes has finished, now the DC can be released */
	DC_DECREF(dc_ptr, dc_release); 	/* it calls function dc_release when count=0 */

	KREF_PUT(&dvs_kref, dvs_release); /* it calls function dvs_release when count=0 !!! NEVER HERE !!! */
	return(OK);
}

/*----------------------------------------------------------------*/
/*			dc_release				*/
/* DC must be WRITE LOCKED					*/
/*----------------------------------------------------------------*/
void dc_release(struct kref *kref)	
{
	dc_desc_t *dc_ptr;
	long unsigned int *bm_ptr;
	cluster_node_t *n_ptr;
	int i;

	dc_ptr = container_of(kref, struct dc_struct, dc_kref);

	MOLDEBUG(INTERNAL,"release memory from processes of DC=%d\n", dc_ptr->dc_usr.dc_dcid);		

	/* Free process descriptors memory */
//	vfree(dc_ptr->dc_proc);
	kfree(dc_ptr->dc_proc);

	/* Free privileges process descriptors memory */
	kfree(dc_ptr->dc_sid2proc);
	
	/* Clears the DC's NODES bitmap */
	dc_ptr->dc_usr.dc_nodes = 0;

	/* Clear NODEs' DC bitmaps */
	for( i = 0;  i < dvs.d_nr_nodes; i++) {
		n_ptr = &node[i];
		WLOCK_NODE(n_ptr);
		if (n_ptr->n_usr.n_flags != NODE_FREE){
			bm_ptr = &n_ptr->n_usr.n_dcs;
			if( test_bit(dc_ptr->dc_usr.dc_dcid, bm_ptr))
				clear_bit(dc_ptr->dc_usr.dc_dcid, bm_ptr);
		}
		WUNLOCK_NODE(n_ptr);
	}

	/* remove /sys/kernel/debug/dvs/DCx/proc entry  */
	debugfs_remove(dc_ptr->dc_procs_dbg);
	/* remove /sys/kernel/debug/dvs/DCx entry  */
	debugfs_remove(dc_ptr->dc_DC_dbg);
	
	/*free the DC descriptor */
	dc_ptr->dc_usr.dc_flags = DC_FREE;

	/* remove /proc/dvs/DCx entries */
	remove_proc_entry("procs", dc_ptr->dc_DC_dir);
	remove_proc_entry("info", dc_ptr->dc_DC_dir);
	remove_proc_entry("stats", dc_ptr->dc_DC_dir);
    remove_proc_entry(dc_ptr->dc_usr.dc_name, proc_dvs_dir);
		
}

/*----------------------------------------------------------------*/
/*			mm_dc_end				*/
/*----------------------------------------------------------------*/

asmlinkage long mm_dc_end(int dcid)
{
	dc_desc_t *dc_ptr;
	int rcode = OK;
	
	if(current_euid() != USER_ROOT) ERROR_RETURN(EMOLPRIVILEGES);

	if( DVS_NOT_INIT() )   ERROR_RETURN(EMOLDVSINIT );

	MOLDEBUG(DBGPARAMS,"Ending DC=%d on node=%d\n", dcid, atomic_read(&local_nodeid));

	CHECK_DCID(dcid);
	dc_ptr 	= &dc[dcid];

	WLOCK_DC(dc_ptr);	
	if(dc_ptr->dc_usr.dc_flags != DC_FREE) {
		rcode = do_dc_end(dc_ptr);	
	}else {
		rcode = EMOLDCNOTRUN;
	}
	WUNLOCK_DC(dc_ptr);	
	if( rcode) ERROR_RETURN(rcode);
	return(OK);
}

/*--------------------------------------------------------------*/
/*			mm_getep				*/
/*--------------------------------------------------------------*/

asmlinkage long mm_getep(int pid)
{
	struct proc *caller_ptr,*proc_ptr;
	dc_desc_t *dc_ptr;
	int dcid, ret;
	struct task_struct *task_ptr;	
	int caller_pid;
	int endpoint=NONE;

	if( DVS_NOT_INIT() )   	ERROR_RETURN(EMOLDVSINIT );

	MOLDEBUG(DBGPARAMS,"pid=%d\n",pid);
	if( pid < 1 || pid > PID_MAX) 	ERROR_RETURN(EMOLBADRANGE);

	ret = check_caller(&task_ptr, &caller_ptr, &caller_pid);
	if(ret) return(ret);
	MOLDEBUG(INTERNAL,"caller_pid=%d dcid=%d\n",caller_pid, caller_ptr->p_usr.p_dcid);

	ret = OK;
	if( caller_pid != pid) { 	
		lock_tasklist(); //read_lock(&tasklist_lock);
		task_ptr= pid_task(find_vpid(pid), PIDTYPE_PID);   
		unlock_tasklist(); //read_unlock(&tasklist_lock);
		if(task_ptr == NULL) 		ERROR_RETURN(EMOLBADPID);

		MOLDEBUG(INTERNAL,"task_pid_nr(task_ptr)=%d\n",task_pid_nr(task_ptr));

		/*-----------------------------------
		 * check TASK state
		 *-----------------------------------*/

		proc_ptr = (struct proc *) task_ptr->task_proc;
		if( proc_ptr == NULL)	ERROR_RETURN(EMOLNOTBIND);

	} else {
		proc_ptr = caller_ptr; 
	}

	/*-----------------------------------
 	* check PROC state
 	*-----------------------------------*/
	RLOCK_PROC(proc_ptr);	
	do	{
		if (pid != proc_ptr->p_usr.p_lpid)
			{ret = EMOLBADPID; break;}
		if( test_bit(BIT_SLOT_FREE,&proc_ptr->p_usr.p_rts_flags))
			{ret = EMOLNOTBIND; break;}
		if(test_bit(MIS_BIT_PROXY, &proc_ptr->p_usr.p_misc_flags))
			{ret = EMOLBADPROC;break;}
	}while(0);
	if(ret) {
		RUNLOCK_PROC(proc_ptr);
		ERROR_RETURN(ret);	
	}
		
	/*-----------------------------------
	 * check DC state
	 *-----------------------------------*/
 	dcid	 = proc_ptr->p_usr.p_dcid;
   	endpoint = proc_ptr->p_usr.p_endpoint;
	RUNLOCK_PROC(proc_ptr);

	dc_ptr 	= &dc[dcid];
	MOLDEBUG(INTERNAL,"dcid=%d\n",dcid);
	RLOCK_DC(dc_ptr);
	if( dc_ptr->dc_usr.dc_flags) 
		ret = EMOLDCNOTRUN;
	RUNLOCK_DC(dc_ptr);

	if(ret) ERROR_RETURN(ret);
	MOLDEBUG(INTERNAL,"endpoint=%d\n",endpoint);
	return(endpoint);
}

/*--------------------------------------------------------------*/
/*			mm_unbind				*/
/* Unbind a  LOCAL/REMOTE  process from the DVS */
/* Who can mm_unbind a process?:				*/
/* - The main thread of a LOCAL process itself		*/
/* - The child thread of a LOCAL process itself		*/
/* - A system process that unbind a local/remote process	*/
/*--------------------------------------------------------------*/
asmlinkage long mm_unbind(int dcid, int proc_ep, long timeout_ms) 
{
	dc_desc_t *dc_ptr;
	long unsigned int *bm_ptr;
	struct proc *proc_ptr,  *caller_ptr;
	int p_nr, nodeid, other_pid;
	proc_usr_t *uproc_ptr;
	struct task_struct *task_ptr = NULL;
	int ret = OK;
	
	MOLDEBUG(DBGPARAMS,"dcid=%d proc_ep=%d\n",dcid, proc_ep);

	if(current_euid() != USER_ROOT) 		
		ERROR_RETURN(EMOLPRIVILEGES);

	if( DVS_NOT_INIT() )  ERROR_RETURN(EMOLDVSINIT );

	CHECK_DCID(dcid);	/* check DC ID limits 		*/
	
	dc_ptr 		= &dc[dcid];
	RLOCK_DC(dc_ptr);
	/* checks if the DC we are talking about is running */
	if( dc_ptr->dc_usr.dc_flags) 					
		ERROR_RUNLOCK_DC(dc_ptr,EMOLDCNOTRUN);

	p_nr = _ENDPOINT_P(proc_ep);
	if( p_nr < (-dc_ptr->dc_usr.dc_nr_tasks) 		
	 || p_nr >= dc_ptr->dc_usr.dc_nr_procs) 		
		ERROR_RUNLOCK_DC(dc_ptr,EMOLENDPOINT);
	bm_ptr = &dc_ptr->dc_usr.dc_nodes;
	
 	proc_ptr     = ENDPOINT2PTR(dc_ptr, proc_ep);
	WLOCK_TASK(current);
	RLOCK_PROC(proc_ptr);
	RUNLOCK_DC(dc_ptr);
	
	if( test_bit(BIT_SLOT_FREE, &proc_ptr->p_usr.p_rts_flags)){
		WUNLOCK_TASK(current);	
		ERROR_RUNLOCK_PROC(proc_ptr,EMOLNOTBIND);
	}
    nodeid = proc_ptr->p_usr.p_nodeid; 
	if( nodeid < 0 || nodeid >= dvs.d_nr_nodes)	{
		WUNLOCK_TASK(current);	
		ERROR_RUNLOCK_PROC(proc_ptr,EMOLBADNODEID);
	}
	if(!test_bit(nodeid, bm_ptr)) 	{
		WUNLOCK_TASK(current);	
		ERROR_RUNLOCK_PROC(proc_ptr,EMOLNODCNODE);
	}
	
	caller_ptr = current->task_proc;							/* may be NULL */
	if( IT_IS_LOCAL(proc_ptr)) {
		if( caller_ptr == proc_ptr ) {						/* Self unbind				*/
			if( task_pid_nr(current) != proc_ptr->p_usr.p_lpid) {	/* the thread is not binded 		*/
				if(current->tgid != proc_ptr->p_usr.p_lpid) {	/* the main thread is not binded too 	*/
					WUNLOCK_TASK(current);	
					ERROR_RUNLOCK_PROC(proc_ptr,EMOLNOTBIND);
				}else{
					task_ptr = caller_ptr->p_task;
					RUNLOCK_PROC(proc_ptr);
					WUNLOCK_TASK(current);
					WLOCK_TASK(task_ptr);
				}
			}else{ 
				task_ptr = caller_ptr->p_task;
				RUNLOCK_PROC(proc_ptr);
				if(task_ptr !=  current) {
					WUNLOCK_TASK(current);
					ERROR_RETURN(EMOLPROCSTS);					
				}
				/*  LOCK_TASK(task_ptr) same as LOCK_TASK(current) already locked */
			}
		}else{
			task_ptr = proc_ptr->p_task;
			RUNLOCK_PROC(proc_ptr);
			WUNLOCK_TASK(current);
			if( task_ptr == NULL)	{
				ERROR_RETURN(EMOLNOTBIND);
			}
			WLOCK_TASK(task_ptr);
		}
	}else{ /* Remote process do not have a local Linux task */
		WUNLOCK_TASK(current);
		RUNLOCK_PROC(proc_ptr);
	}

	/* LOCAL process enter here with task struct locked */
	WLOCK_DC(dc_ptr);
	WLOCK_PROC(proc_ptr);
//	set_bit(BIT_NO_MAP, &caller_ptr->p_usr.p_rts_flags);
	ret = do_unbind(dc_ptr, proc_ptr);
	/* WARNING!! from here, all proc_ptr fields were CLEARED !!! */
	if( ret ) {
		if( IT_IS_LOCAL(proc_ptr)){
			WUNLOCK_TASK(task_ptr);
			if( ret == EMOLAGAIN && caller_ptr != NULL && caller_ptr != proc_ptr ){ 
				INIT_LIST_HEAD(&caller_ptr->p_ulink);
				LIST_ADD_TAIL(&caller_ptr->p_ulink, &proc_ptr->p_ulist);
				caller_ptr->p_usr.p_waitunbind = proc_ptr->p_usr.p_endpoint;
				WUNLOCK_PROC(proc_ptr);
				WUNLOCK_DC(dc_ptr);
				set_bit(BIT_WAITUNBIND, &caller_ptr->p_usr.p_rts_flags);
				uproc_ptr = &caller_ptr->p_usr;
				MOLDEBUG(DBGPARAMS,PROC_USR_FORMAT, PROC_USR_FIELDS(uproc_ptr));
				sleep_proc(caller_ptr, timeout_ms);
				ERROR_RETURN(caller_ptr->p_rcode);
			}
		} 
		WUNLOCK_PROC(proc_ptr);
		WUNLOCK_DC(dc_ptr);
		if(ret)	ERROR_RETURN(ret);
		return(OK);
	}
	
	if( atomic_read(&local_nodeid) == nodeid ) {
		if ( caller_ptr != proc_ptr) {
			WUNLOCK_PROC(proc_ptr);	
			WUNLOCK_DC(dc_ptr);
			other_pid = task_pid_nr(task_ptr);
			WUNLOCK_TASK(task_ptr);
			/* CALLER could not be bound */
			if( caller_ptr == NULL) return(OK);
//			mm_sys_wait4(other_pid, (int __user *)&ret, 0, NULL);
//			schedule();				
// ANALIZAR si el proceso que se esta matando es HIJO del MATADOR de tal modo de hacer el  mm_sys_wait4 necesario para no quedar ZOMBIE.
			/* Waits until the target process unbinds by itself after sending it a SIGPIPE */
			do {
				ret = wait_event_interruptible(caller_ptr->p_wqhead,(other_pid != proc_ptr->p_usr.p_lpid));
				MOLDEBUG(INTERNAL,"other_pid=%d pid=%d\n",other_pid, proc_ptr->p_usr.p_lpid);
			}while(ret != 0);
			MOLDEBUG(INTERNAL,"other_pid=%d pid=%d\n",other_pid, proc_ptr->p_usr.p_lpid);
			return(OK);
		}else{
			WUNLOCK_TASK(task_ptr);
		}
	}
	
	WUNLOCK_PROC(proc_ptr);
	WUNLOCK_DC(dc_ptr);

	return(OK);
}
 
 /*--------------------------------------------------------------*/
/*		do_unbind				*/
/* Unbind a  LOCAL/REMOTE  process from the DVS 	*/
/* LOCK IN THIS ORDER					*/
/* TASK MUTEX must be LOCKED				*/
/* DC MUTEX must be WRITE LOCKED	 		*/
/* PROC MUTEX must be WRITE LOCKED			*/
/* Who can invoke do_unbind ??				*/
/* 1- the caller: calling mm_unbind			*/
/* 2- the caller: through exit() 			*/
/* 3- the caller: has received a signal ERESTARTSYS	*/
/* 4- another process that unbind a REMOTE process	*/
/* 5- another process that does dc_end or dvs_end	*/
/* Tasks made by do_unbind:				*/
/* 	- If the process is a BACKUP, convert it into a REMOTE	*/
/*	- wakeup with error all process trying to send 	*/
/*		a message to the unbinding process 	*/
/*	- delete notify messages bits sent by the proc	*/
/*	- Wakeup with error all process that are waiting*/
/*		to receive a message from the unbinding */
/*	- Removes the unbinding process descriptor from */
/*		proxy SENDER queue			*/
/*	- Removes the unbinding process descriptor from */
/*		other process sending queue			*/
/*	- Removes the unbinding process descriptor from */
/*		other process migrating queue			*/
/*--------------------------------------------------------------*/
asmlinkage long do_unbind(dc_desc_t *dc_ptr, struct proc *proc_ptr)
{
	struct proc *src_ptr, *rp, *tmp_ptr, *caller_ptr, *sproxy_ptr;
	long unsigned int *bm_ptr;
	proc_usr_t *uproc_ptr;
	int i, dcid, rcode;

#ifdef WARN_PROC
	int warn_ep;
	struct proc *warn_ptr; 
#endif /* WARN_PROC */
	
	uproc_ptr = &proc_ptr->p_usr;
	MOLDEBUG(DBGPARAMS,PROC_USR_FORMAT, PROC_USR_FIELDS(uproc_ptr));

	do {
		rcode = OK;
		if(test_bit(BIT_SLOT_FREE, &proc_ptr->p_usr.p_rts_flags)) 	
			{rcode = EMOLNOTBIND; break;}
		if(test_bit(BIT_NO_ENDPOINT, &proc_ptr->p_usr.p_rts_flags)) 
			{rcode = EMOLENDPOINT; break;}
		if(test_bit(MIS_BIT_PROXY, &proc_ptr->p_usr.p_misc_flags)) 	
			{rcode = EMOLBADPROC; break;}
		dcid = dc_ptr->dc_usr.dc_dcid;
		if(proc_ptr->p_usr.p_dcid != dcid)  						
			{rcode = EMOLBADDCID; break;}
	}while(0);
	if( rcode) ERROR_RETURN(rcode);
	
	/* Needed to test that the nodeid belongs to the DC */
	bm_ptr = &dc_ptr->dc_usr.dc_nodes;
	if(!test_bit(proc_ptr->p_usr.p_nodeid, bm_ptr)) 		ERROR_RETURN(EMOLNODCNODE);
	
	/*Gets the caller of this function 	*/
	/* Check caller's attributes 		*/
	caller_ptr = current->task_proc;
	if(caller_ptr != NULL) {
		if( caller_ptr->p_usr.p_dcid !=  dcid)				ERROR_RETURN(EMOLBADDCID);
		//      may be a signaled process 
		//		if( caller_ptr->p_usr.p_rts_flags != 0)		ERROR_RETURN(EMOLPROCSTS);
		CHECK_PID( task_pid_nr(current), caller_ptr);
		uproc_ptr = &caller_ptr->p_usr;
		MOLDEBUG(INTERNAL,"Caller "PROC_USR_FORMAT,PROC_USR_FIELDS(uproc_ptr));
	}

	if( IT_IS_LOCAL(proc_ptr)) {	
 		if( caller_ptr != proc_ptr){ /* caller is trying to unbind another local process */
			if(test_bit(BIT_ONCOPY, &proc_ptr->p_usr.p_rts_flags)){
				if(test_bit(BIT_RMTOPER, &proc_ptr->p_usr.p_rts_flags))  { /* only requester has this flag set */
					set_bit(BIT_SIGNALED, &proc_ptr->p_usr.p_rts_flags);
				}
				return(OK);
			}
			MOLDEBUG(INTERNAL,"Sending SIGPIPE to pid=%d\n", proc_ptr->p_usr.p_lpid);
			rcode = send_sig_info(SIGPIPE, SEND_SIG_NOINFO, proc_ptr->p_task);
			if(rcode) ERROR_RETURN(rcode);
			if(proc_ptr->p_usr.p_rts_flags && (BIT_SENDING | BIT_RECEIVING | BIT_WAITMIGR))  {
				LOCAL_PROC_UP(proc_ptr, EMOLSHUTDOWN);
			}
			return(EMOLAGAIN); /* It means that the caller (!= proc_ptr) must wait proc_ptr self unbind */
		}
	}else { /* REMOTE processes and REMOTE BACKUP processes (stand by local processes) */
		if (test_bit(MIS_BIT_RMTBACKUP, &proc_ptr->p_usr.p_misc_flags)) { /* it is a REMOTE BACKUP process */
			init_proc_desc(proc_ptr, proc_ptr->p_usr.p_dcid, (proc_ptr->p_usr.p_nr+dc_ptr->dc_usr.dc_nr_tasks));
			DC_DECREF(dc_ptr,dc_release);
			return(OK);
		}
	}

	/*--------------------------------------*/
	/* wakeup with error those processes	*/
	/* trying to send a message to the proc	*/
	/*--------------------------------------*/
	MOLDEBUG(INTERNAL,"wakeup with error those processes trying to send a message to the proc\n");
	LIST_FOR_EACH_ENTRY_SAFE(src_ptr, tmp_ptr, &proc_ptr->p_list, p_link) {
		if( proc_ptr->p_usr.p_nr < src_ptr->p_usr.p_nr) {
			WLOCK_PROC(src_ptr); /* Caller LOCK is just locked */
		}else{	
			/* free the callers lock and then lock both ordered */
			WUNLOCK_PROC(proc_ptr);
			WLOCK_PROC(src_ptr);
			WLOCK_PROC(proc_ptr);
		}

		LIST_DEL_INIT(&src_ptr->p_link); /* remove from queue */

		MOLDEBUG(INTERNAL,"Find process %d trying to send a message to %d\n",
			src_ptr->p_usr.p_endpoint, proc_ptr->p_usr.p_endpoint);
		clear_bit(BIT_SENDING, &src_ptr->p_usr.p_rts_flags);
		if( IT_IS_LOCAL(src_ptr)) {
			MOLDEBUG(INTERNAL,"Wakeup SENDER with error ep=%d  pid=%d\n",
				src_ptr->p_usr.p_endpoint, src_ptr->p_usr.p_lpid);	
			LOCAL_PROC_UP(src_ptr, EMOLDSTDIED);
		} else {
			/* REMOTE process descriptor are used for ACKNOWLEDGES 							 */
			send_ack_lcl2rmt(src_ptr,proc_ptr,EMOLDSTDIED);
		}
		WUNLOCK_PROC(src_ptr);
	}
	
	MOLDEBUG(GENERIC,"delete notify messages bits sent by the proc\n");
	/* ATENCION : Posible mejora.  Reducir el numero de proceso del loop a nr_sys_procs */
	FOR_EACH_PROC(dc_ptr, i) {
		rp = DC_PROC(dc_ptr,i);
		if(rp == proc_ptr)	continue;
		/* locking order compliance  */
		if( proc_ptr->p_usr.p_nr < (i - dc_ptr->dc_usr.dc_nr_tasks)) {
			WLOCK_PROC(rp); /* Caller LOCK is just locked */
		}else{	
			/* free the callers lock and then lock both ordered */
			WUNLOCK_PROC(proc_ptr);
			WLOCK_PROC(rp);
			WLOCK_PROC(proc_ptr);
		}
		if(test_bit(BIT_SLOT_FREE, &rp->p_usr.p_rts_flags)) {
			WUNLOCK_PROC(rp); 
			continue;
		}
		
		/* Remove any notify message from the process */ 		
		if( proc_ptr->p_priv.s_usr.s_id < dc_ptr->dc_usr.dc_nr_sysprocs) {
			if( get_sys_bit(rp->p_priv.s_notify_pending, proc_ptr->p_priv.s_usr.s_id)) {
				MOLDEBUG(INTERNAL,"Delete the bit %d in the notify bitmap of processes %d \n", 
					proc_ptr->p_priv.s_usr.s_id, rp->p_usr.p_endpoint);
				unset_sys_bit(rp->p_priv.s_notify_pending, proc_ptr->p_priv.s_usr.s_id);
			}
		}

		/* it another LOCAL process is waiting to receive 			*/
		/* a message from the unbound process, inform that it exits.*/ 
		if( (!test_bit(BIT_SENDING, &rp->p_usr.p_rts_flags) && test_bit(BIT_RECEIVING, &rp->p_usr.p_rts_flags) ) 
			&& (rp->p_usr.p_getfrom == proc_ptr->p_usr.p_endpoint)) {
			MOLDEBUG(INTERNAL,"Process %d is no more waiting a message from the unbinded process %d\n",
				rp->p_usr.p_endpoint, proc_ptr->p_usr.p_endpoint);
			if( IT_IS_LOCAL(rp)) {
				MOLDEBUG(INTERNAL,"Wakeup RECEIVER with error ep=%d  pid=%d\n",
					rp->p_usr.p_endpoint, rp->p_usr.p_lpid);
				LOCAL_PROC_UP(rp, EMOLSRCDIED);
			}else{		
				clear_bit(BIT_RECEIVING, &rp->p_usr.p_rts_flags);
				generic_ack_lcl2rmt(CMD_SNDREC_ACK, rp, proc_ptr, EMOLSRCDIED);
			}
		}
		WUNLOCK_PROC(rp); 
	}

	uproc_ptr = &proc_ptr->p_usr;
	MOLDEBUG(INTERNAL,PROC_USR_FORMAT,PROC_USR_FIELDS(uproc_ptr));

	/* Removes the unbinding process descriptor from proxy SENDER queue */
	if( test_bit(BIT_RMTOPER, &proc_ptr->p_usr.p_rts_flags)) {
		if(proc_ptr->p_usr.p_proxy != NONE) {
			sproxy_ptr = &proxies[proc_ptr->p_usr.p_proxy].px_sproxy;
			WLOCK_PROC(sproxy_ptr);
			LIST_DEL_INIT(&proc_ptr->p_link); /* remove from queue */
			WUNLOCK_PROC(sproxy_ptr);
		}else{
			ERROR_PRINT(EMOLPROCSTS);
		}
		clear_bit(BIT_RMTOPER, &proc_ptr->p_usr.p_rts_flags);
		proc_ptr->p_usr.p_proxy = NONE;
	}

	/* Removes the unbinding process descriptor from other process SENDING queue */
	if( test_bit(BIT_SENDING, &proc_ptr->p_usr.p_rts_flags)) {
		rp = DC_PROC(dc_ptr,(_ENDPOINT_P(proc_ptr->p_usr.p_sendto) - dc_ptr->dc_usr.dc_nr_tasks));
		uproc_ptr = &rp->p_usr;
		MOLDEBUG(INTERNAL,"sendto" PROC_USR_FORMAT,PROC_USR_FIELDS(uproc_ptr));
		if( proc_ptr->p_usr.p_nr < rp->p_usr.p_nr) {
			WLOCK_PROC(rp); /* Caller LOCK is just locked */
		}else{	
			/* free the callers lock and then lock both ordered */
			WUNLOCK_PROC(proc_ptr);
			WLOCK_PROC(rp);
			WLOCK_PROC(proc_ptr);
		}
		if( IT_IS_LOCAL(rp)) { 
			LIST_DEL_INIT(&proc_ptr->p_link); /* remove from queue */
			WUNLOCK_PROC(rp);
		}
		clear_bit(BIT_SENDING, &proc_ptr->p_usr.p_rts_flags);
		proc_ptr->p_usr.p_sendto	= NONE;
	}
	
	/* Removes the unbinding process descriptor from other process MIGRATING queue */
	if( test_bit(BIT_WAITMIGR, &proc_ptr->p_usr.p_rts_flags)) {
		rp = DC_PROC(dc_ptr,(_ENDPOINT_P(proc_ptr->p_usr.p_waitmigr) - dc_ptr->dc_usr.dc_nr_tasks));
		uproc_ptr = &rp->p_usr;
		MOLDEBUG(INTERNAL,"waitmigr " PROC_USR_FORMAT,PROC_USR_FIELDS(uproc_ptr));
		if( proc_ptr->p_usr.p_nr < rp->p_usr.p_nr) {
			WLOCK_PROC(rp); /* Caller LOCK is just locked */
		}else{	
			/* free the callers lock and then lock both ordered */
			WUNLOCK_PROC(proc_ptr);
			WLOCK_PROC(rp);
			WLOCK_PROC(proc_ptr);
		}
		LIST_DEL_INIT(&proc_ptr->p_mlink); /* remove from queue */
		WUNLOCK_PROC(rp);	
		clear_bit(BIT_WAITMIGR, &proc_ptr->p_usr.p_rts_flags);
		proc_ptr->p_usr.p_waitmigr = NONE;
	}
	
	MOLDEBUG(INTERNAL,"wakeup with error those processes waiting this process MIGRATION\n");
	LIST_FOR_EACH_ENTRY_SAFE(rp, tmp_ptr, &proc_ptr->p_mlist, p_mlink) {
		if( proc_ptr->p_usr.p_nr < rp->p_usr.p_nr) {
			WLOCK_PROC(rp); /* Caller LOCK is just locked */
		}else{	
			/* free the callers lock and then lock both ordered */
			WUNLOCK_PROC(proc_ptr);
			WLOCK_PROC(rp);
			WLOCK_PROC(proc_ptr);
		}

		LIST_DEL_INIT(&rp->p_mlink); /* remove from queue */

		MOLDEBUG(INTERNAL,"Find process %d waiting %d MIGRATION\n",
			rp->p_usr.p_endpoint, proc_ptr->p_usr.p_endpoint);
		clear_bit(BIT_WAITMIGR, &rp->p_usr.p_rts_flags);
		rp->p_usr.p_waitmigr = NONE;

		if( IT_IS_LOCAL(rp)) {
			MOLDEBUG(INTERNAL,"Wakeup waiting MIGRATION with error ep=%d pid=%d\n",
				rp->p_usr.p_endpoint, rp->p_usr.p_lpid);	
			LOCAL_PROC_UP(rp, EMOLNOTBIND);
		} else {
			/* REMOTE process descriptor are used for ACKNOWLEDGES 							 */
			send_ack_lcl2rmt(rp,proc_ptr,EMOLNOTBIND);
		}
		WUNLOCK_PROC(rp);
	}
	
	/* Removes the process descriptor waiting for UNBINDING queue */
	if( test_bit(BIT_WAITUNBIND, &proc_ptr->p_usr.p_rts_flags)) {
		rp = DC_PROC(dc_ptr,(_ENDPOINT_P(proc_ptr->p_usr.p_waitunbind) - dc_ptr->dc_usr.dc_nr_tasks));
		uproc_ptr = &rp->p_usr;
		MOLDEBUG(INTERNAL,"waitunbind " PROC_USR_FORMAT,PROC_USR_FIELDS(uproc_ptr));
		if( proc_ptr->p_usr.p_nr < rp->p_usr.p_nr) {
			WLOCK_PROC(rp); /* Caller LOCK is just locked */
		}else{	
			/* free the callers lock and then lock both ordered */
			WUNLOCK_PROC(proc_ptr);
			WLOCK_PROC(rp);
			WLOCK_PROC(proc_ptr);
		}
		LIST_DEL_INIT(&proc_ptr->p_ulink); /* remove from queue */
		WUNLOCK_PROC(rp);	
		clear_bit(BIT_WAITUNBIND, &proc_ptr->p_usr.p_rts_flags);
		proc_ptr->p_usr.p_waitunbind = NONE;
	}
	
	/* wakeup those processes waiting this process UNBINDING */
	MOLDEBUG(INTERNAL,"wakeup those processes waiting this process UNBINDING\n");
	LIST_FOR_EACH_ENTRY_SAFE(rp, tmp_ptr, &proc_ptr->p_ulist, p_ulink) {
		if( proc_ptr->p_usr.p_nr < rp->p_usr.p_nr) {
			WLOCK_PROC(rp); /* Caller LOCK is just locked */
		}else{	
			/* free the callers lock and then lock both ordered */
			WUNLOCK_PROC(proc_ptr);
			WLOCK_PROC(rp);
			WLOCK_PROC(proc_ptr);
		}

		LIST_DEL_INIT(&rp->p_ulink); /* remove from queue */

		MOLDEBUG(INTERNAL,"Find process %d waiting this process %d UNBINDING\n",
			rp->p_usr.p_endpoint, proc_ptr->p_usr.p_endpoint);

		clear_bit(BIT_WAITUNBIND, &rp->p_usr.p_rts_flags);
		rp->p_usr.p_waitunbind = NONE;
		if( IT_IS_LOCAL(rp)) {
			MOLDEBUG(INTERNAL,"Wakeup waiting for UNBINDING process ep=%d  pid=%d\n",
				rp->p_usr.p_endpoint, rp->p_usr.p_lpid);	
			LOCAL_PROC_UP(rp, OK);
		} else {
			/* REMOTE process descriptor are used for ACKNOWLEDGES 							 */
			send_ack_lcl2rmt(rp,proc_ptr,OK);
		}
		WUNLOCK_PROC(rp);
	}
		
	/*********************************************************/
	/* WARNING: Quizás falte controlar si el proceso unbind */
	/* está en medio de una MIGRACION 				*/
	/*********************************************************/
	MOLDEBUG(INTERNAL,PROC_USR_FORMAT,PROC_USR_FIELDS(uproc_ptr));
	init_proc_desc(proc_ptr, proc_ptr->p_usr.p_dcid, (proc_ptr->p_usr.p_nr+dc_ptr->dc_usr.dc_nr_tasks));
	MOLDEBUG(INTERNAL,"initialized \n");
	DC_DECREF(dc_ptr, dc_release);
	
	return(OK);
}

/*----------------------------------------------------------------*/
/*			mm_getpriv				*/
/*----------------------------------------------------------------*/

asmlinkage long mm_getpriv(int dcid, int proc_ep, priv_t *u_priv)
{
	dc_desc_t *dc_ptr;
	struct proc *proc_ptr;
	priv_usr_t *kp_ptr;
	int proc_nr;
	int ret = OK;


	if(current_euid() != USER_ROOT) ERROR_RETURN(EMOLPRIVILEGES);

	if( DVS_NOT_INIT() )   ERROR_RETURN(EMOLDVSINIT );

	CHECK_DCID(dcid);	/* check DC ID limits 		*/

	dc_ptr 		= &dc[dcid];
	RLOCK_DC(dc_ptr);
	/* checks if the DC we are talking about is running */
	if( dc_ptr->dc_usr.dc_flags) 	ERROR_RUNLOCK_DC(dc_ptr,EMOLDCNOTRUN);

	proc_nr = _ENDPOINT_P(proc_ep);
	MOLDEBUG(DBGPARAMS,"dcid=%d proc_nr=%d proc_ep=%d\n",dcid, proc_nr, proc_ep);
	if( proc_nr < (-dc_ptr->dc_usr.dc_nr_tasks) 		
	 || proc_nr >= dc_ptr->dc_usr.dc_nr_procs) ERROR_RUNLOCK_DC(dc_ptr,EMOLBADPROC);
 	proc_ptr   = NBR2PTR(dc_ptr, proc_nr);

	if( test_bit(BIT_SLOT_FREE, &proc_ptr->p_usr.p_rts_flags)) 
		ERROR_RUNLOCK_DC(dc_ptr,EMOLDSTDIED);

	RLOCK_PROC(proc_ptr);
	kp_ptr = &proc_ptr->p_priv.s_usr;
	ret = copy_to_user( u_priv, kp_ptr,  sizeof(priv_usr_t));
	MOLDEBUG(DBGPRIV,PRIV_USR_FORMAT,PRIV_USR_FIELDS(kp_ptr));

	RUNLOCK_PROC(proc_ptr);
	RUNLOCK_DC(dc_ptr);
	return(ret);
}

/*----------------------------------------------------------------*/
/*			mm_setpriv				*/
/*----------------------------------------------------------------*/

asmlinkage long mm_setpriv(int dcid, int proc_ep, priv_t *u_priv)
{
	dc_desc_t *dc_ptr;
	struct proc *proc_ptr;
	priv_usr_t *kp_ptr;
	int proc_nr;
	int ret = OK;

	if(current_euid() != USER_ROOT) ERROR_RETURN(EMOLPRIVILEGES);

	if( DVS_NOT_INIT() )   ERROR_RETURN(EMOLDVSINIT );

	CHECK_DCID(dcid);	/* check DC ID limits 		*/

	dc_ptr 		= &dc[dcid];
	RLOCK_DC(dc_ptr);
	/* checks if the DC we are talking about is running */
	if( dc_ptr->dc_usr.dc_flags) 	ERROR_RUNLOCK_DC(dc_ptr,EMOLDCNOTRUN);

	proc_nr = _ENDPOINT_P(proc_ep);
	MOLDEBUG(DBGPARAMS,"dcid=%d proc_nr=%d proc_ep=%d\n",dcid, proc_nr, proc_ep);
	if( proc_nr < (-dc_ptr->dc_usr.dc_nr_tasks) 		
	 || proc_nr >= dc_ptr->dc_usr.dc_nr_procs) ERROR_RUNLOCK_DC(dc_ptr,EMOLBADPROC);
 	proc_ptr   = NBR2PTR(dc_ptr, proc_nr);

	if( test_bit(BIT_SLOT_FREE, &proc_ptr->p_usr.p_rts_flags))
		ERROR_RUNLOCK_DC(dc_ptr,EMOLDSTDIED);

	WLOCK_PROC(proc_ptr);
	kp_ptr = &proc_ptr->p_priv.s_usr;
	ret = copy_from_user( kp_ptr, u_priv, sizeof(priv_usr_t));
	MOLDEBUG(DBGPRIV,PRIV_USR_FORMAT,PRIV_USR_FIELDS(kp_ptr));

	WUNLOCK_PROC(proc_ptr);
	RUNLOCK_DC(dc_ptr);
	return(ret);
}

/*----------------------------------------------------------------*/
/*			mm_proxies_bind			*/
/* it returns the proxies ID or ERROR				*/
/*----------------------------------------------------------------*/
asmlinkage int mm_proxies_bind(char *px_name, int px_nr, int spid, int rpid, int maxbytes)
{
	struct task_struct *stask_ptr, *rtask_ptr;	
	struct proc *sproxy_ptr, *rproxy_ptr;
	proc_usr_t  *sp_ptr, *rp_ptr;
	proxies_t *px_ptr;
	int ret;
	
	MOLDEBUG(DBGPARAMS,"px_nr=%d spid=%d, rpid=%d maxbytes=%d\n",
		px_nr, spid, rpid, maxbytes);

	if(current_euid() != USER_ROOT) ERROR_RETURN(EMOLPRIVILEGES);

	if( DVS_NOT_INIT() )   ERROR_RETURN(EMOLDVSINIT );

	CHECK_NODEID(px_nr);

	/* Verify if both PIDs are running */
	lock_tasklist(); //read_lock(&tasklist_lock);
	stask_ptr = pid_task(find_vpid(spid), PIDTYPE_PID);  
	rtask_ptr = pid_task(find_vpid(rpid), PIDTYPE_PID);  
	if( stask_ptr == NULL || rtask_ptr == NULL) {
		unlock_tasklist(); //read_unlock(&tasklist_lock);
		ERROR_RETURN(EMOLBADPID);
	}
	unlock_tasklist(); //read_unlock(&tasklist_lock);

	CHECK_PROXYID(px_nr); 
	px_ptr = &proxies[px_nr];
	WLOCK_PROXY(px_ptr);
	if( px_ptr->px_usr.px_flags != PROXIES_FREE)	{
		WUNLOCK_PROXY(px_ptr);
		ERROR_RETURN(EMOLRSCBUSY);
	}
	
	if( maxbytes > MAXCOPYBUF)	{
		WUNLOCK_PROXY(px_ptr);
		ERROR_RETURN(EMOL2BIG);
	}
	
	px_ptr->px_usr.px_maxbytes = maxbytes;
	
	sproxy_ptr = &px_ptr->px_sproxy; 
	rproxy_ptr = &px_ptr->px_rproxy; 
	WLOCK_PROC(rproxy_ptr);
	WLOCK_PROC(sproxy_ptr);
	MOLDEBUG(DBGPARAMS,"sproxy_pid=%d, rproxy_pid=%d\n", sproxy_ptr->p_usr.p_lpid, rproxy_ptr->p_usr.p_lpid);

	WLOCK_TASK(stask_ptr);
	init_proc_desc(sproxy_ptr, PROXY_NO_DC, px_nr);
	stask_ptr->task_proc = sproxy_ptr;		/* Set the  process descriptor into the task descriptor */
	sproxy_ptr->p_task = stask_ptr;			/* Set the task descriptor into the process descriptor */
	strncpy(sproxy_ptr->p_usr.p_name, stask_ptr->comm, MAXPROCNAME-1);
	sproxy_ptr->p_name_ptr = stask_ptr->comm;
	sproxy_ptr->p_usr.p_lpid = task_pid_nr(stask_ptr);
	get_task_struct(stask_ptr);			/* increment the reference count of the task struct */
	sproxy_ptr->p_usr.p_rts_flags 	= PROC_RUNNING;		/* set to RUNNING STATE	*/
	sproxy_ptr->p_usr.p_nr		 	= px_nr;		
	sproxy_ptr->p_usr.p_endpoint	= NONE;		
	sproxy_ptr->p_usr.p_nodeid		= atomic_read(&local_nodeid);	
	sproxy_ptr->p_usr.p_lpid 		= spid;			/* Update PID		*/
	sproxy_ptr->p_priv.s_usr.s_level 	= PROXY_PRIV;	
	sproxy_ptr->p_usr.p_rmtsent		= 0;
	set_bit(MIS_BIT_PROXY, &sproxy_ptr->p_usr.p_misc_flags);
	sp_ptr = &sproxy_ptr->p_usr;
	MOLDEBUG(INTERNAL, PROC_USR_FORMAT, PROC_USR_FIELDS(sp_ptr));
	WUNLOCK_TASK(stask_ptr);

	WLOCK_TASK(rtask_ptr);
	init_proc_desc(rproxy_ptr, PROXY_NO_DC, px_nr);
	rtask_ptr->task_proc = rproxy_ptr;		/* Set the  process descriptor into the task descriptor */
	rproxy_ptr->p_task = rtask_ptr;			/* Set the task descriptor into the process descriptor */
	strncpy(rproxy_ptr->p_usr.p_name, rtask_ptr->comm, MAXPROCNAME-1);
	rproxy_ptr->p_name_ptr = rtask_ptr->comm;
	rproxy_ptr->p_usr.p_lpid = task_pid_nr(rtask_ptr);
	get_task_struct(rtask_ptr);			/* increment the reference count of the task struct */
	rproxy_ptr->p_usr.p_rts_flags 	= PROC_RUNNING;		/* set to RUNNING STATE	*/
	rproxy_ptr->p_usr.p_nr		 	= px_nr;		
	rproxy_ptr->p_usr.p_endpoint	= NONE;		
	rproxy_ptr->p_usr.p_nodeid		= atomic_read(&local_nodeid);	
	rproxy_ptr->p_usr.p_lpid 		= rpid;			/* Update PID		*/
	rproxy_ptr->p_priv.s_usr.s_level	= PROXY_PRIV;	
	rproxy_ptr->p_usr.p_rmtsent		= 0;
	set_bit(MIS_BIT_PROXY, &rproxy_ptr->p_usr.p_misc_flags);
	rp_ptr = &rproxy_ptr->p_usr;
	MOLDEBUG(INTERNAL, PROC_USR_FORMAT, PROC_USR_FIELDS(rp_ptr));
	WUNLOCK_TASK(rtask_ptr);

	ret = copy_from_user(px_ptr->px_usr.px_name, px_name, MAXPROXYNAME);

	px_ptr->px_usr.px_flags 		= PROXIES_INUSE;
	KREF_GET(&dvs_kref); 			/* increment DVS reference counter 	*/

	WUNLOCK_PROC(sproxy_ptr);
	WUNLOCK_PROC(rproxy_ptr);
	WUNLOCK_PROXY(px_ptr);
	return(OK);
}

/*----------------------------------------------------------------*/
/*			kproxies_bind				*/
/* it returns the proxies ID or ERROR				*/
/* TO BE CALLED FROM KERNEL 				*/
/*----------------------------------------------------------------*/
asmlinkage int k_proxies_bind(char *px_name, int px_nr, int spid, int rpid, int maxbytes)
{
	struct task_struct *stask_ptr, *rtask_ptr;	
	struct proc *sproxy_ptr, *rproxy_ptr;
	proc_usr_t  *sp_ptr, *rp_ptr;
	proxies_t *px_ptr;
	int rcode;
		
	MOLDEBUG(DBGPARAMS,"px_name=%s px_nr=%d spid=%d, rpid=%d maxbytes=%d\n"
		, px_name, px_nr, spid, rpid, maxbytes);

	if(current_euid() != USER_ROOT) ERROR_RETURN(EMOLPRIVILEGES);

	if( DVS_NOT_INIT() )   ERROR_RETURN(EMOLDVSINIT );

	CHECK_NODEID(px_nr);

	/* Verify if both PIDs are running */
	lock_tasklist(); //read_lock(&tasklist_lock);
	stask_ptr = pid_task(find_vpid(spid), PIDTYPE_PID);  
	rtask_ptr = pid_task(find_vpid(rpid), PIDTYPE_PID);  
	if( stask_ptr == NULL || rtask_ptr == NULL) {
		unlock_tasklist(); //read_unlock(&tasklist_lock);
		ERROR_RETURN(EMOLBADPID);
	}
	unlock_tasklist(); //read_unlock(&tasklist_lock);

	CHECK_PROXYID(px_nr); 
	px_ptr = &proxies[px_nr];
	WLOCK_PROXY(px_ptr);
	if( px_ptr->px_usr.px_flags != PROXIES_FREE)	{
		WUNLOCK_PROXY(px_ptr);
		ERROR_RETURN(EMOLRSCBUSY);
	}

	if( maxbytes > MAXCOPYBUF)	{
		WUNLOCK_PROXY(px_ptr);
		ERROR_RETURN(EMOL2BIG);
	}
	
	px_ptr->px_usr.px_maxbytes = maxbytes;
	
	sproxy_ptr = &px_ptr->px_sproxy; 
	rproxy_ptr = &px_ptr->px_rproxy; 
	WLOCK_PROC(rproxy_ptr);
	WLOCK_PROC(sproxy_ptr);
	MOLDEBUG(DBGPARAMS,"sproxy_pid=%d, rproxy_pid=%d\n", sproxy_ptr->p_usr.p_lpid, rproxy_ptr->p_usr.p_lpid);

	WLOCK_TASK(stask_ptr);
	init_proc_desc(sproxy_ptr, PROXY_NO_DC, px_nr);
	stask_ptr->task_proc 			= sproxy_ptr;		/* Set the  process descriptor into the task descriptor */
	sproxy_ptr->p_task 				= stask_ptr;			/* Set the task descriptor into the process descriptor */
	strncpy(sproxy_ptr->p_usr.p_name, px_name, MAXPROCNAME-1);
	sproxy_ptr->p_name_ptr 			= stask_ptr->comm;
	sproxy_ptr->p_usr.p_lpid 		= task_pid_nr(stask_ptr);
	get_task_struct(stask_ptr);			/* increment the reference count of the task struct */
	sproxy_ptr->p_usr.p_rts_flags 	= PROC_RUNNING;		/* set to RUNNING STATE	*/
	sproxy_ptr->p_usr.p_nr		 	= px_nr;		
	sproxy_ptr->p_usr.p_endpoint	= NONE;		
	sproxy_ptr->p_usr.p_nodeid		= atomic_read(&local_nodeid);	
	sproxy_ptr->p_usr.p_lpid 		= spid;			/* Update PID		*/
	sproxy_ptr->p_priv.s_usr.s_level= PROXY_PRIV;
	set_bit(MIS_BIT_PROXY, &sproxy_ptr->p_usr.p_misc_flags);
	set_bit(MIS_BIT_KTHREAD, &sproxy_ptr->p_usr.p_misc_flags);
	sp_ptr = &sproxy_ptr->p_usr;
	MOLDEBUG(INTERNAL, PROC_USR_FORMAT, PROC_USR_FIELDS(sp_ptr));
	WUNLOCK_TASK(stask_ptr);

	rcode = OK;

	WLOCK_TASK(rtask_ptr);
	init_proc_desc(rproxy_ptr, PROXY_NO_DC, px_nr);
	rtask_ptr->task_proc = rproxy_ptr;		/* Set the  process descriptor into the task descriptor */
	rproxy_ptr->p_task = rtask_ptr;			/* Set the task descriptor into the process descriptor */
	strncpy(rproxy_ptr->p_usr.p_name, px_name, MAXPROCNAME-1);
	rproxy_ptr->p_name_ptr = rtask_ptr->comm;
	rproxy_ptr->p_usr.p_lpid = task_pid_nr(rtask_ptr);
	get_task_struct(rtask_ptr);			/* increment the reference count of the task struct */
	rproxy_ptr->p_usr.p_rts_flags 	= PROC_RUNNING;		/* set to RUNNING STATE	*/
	rproxy_ptr->p_usr.p_nr		 	= px_nr;		
	rproxy_ptr->p_usr.p_endpoint	= NONE;		
	rproxy_ptr->p_usr.p_nodeid		= atomic_read(&local_nodeid);	
	rproxy_ptr->p_usr.p_lpid 		= rpid;			/* Update PID		*/
	rproxy_ptr->p_priv.s_usr.s_level= PROXY_PRIV;	
	
	/* **************** ALLOC ALIGNED MEMORY FOR MESSAGE   **************/  
	/* needed to copy to/from kernel									*/
	rproxy_ptr->p_umsg = vmalloc( sizeof (message)); 
	if(rproxy_ptr->p_umsg == NULL)	
		rcode = EMOLALLOCMEM;
	
	set_bit(MIS_BIT_PROXY, &rproxy_ptr->p_usr.p_misc_flags);
	set_bit(MIS_BIT_KTHREAD, &rproxy_ptr->p_usr.p_misc_flags);
	rp_ptr = &rproxy_ptr->p_usr;
	MOLDEBUG(INTERNAL, PROC_USR_FORMAT, PROC_USR_FIELDS(rp_ptr));
	WUNLOCK_TASK(rtask_ptr);

	memcpy( (void *)px_ptr->px_usr.px_name, (void *) px_name, MAXPROXYNAME);

	px_ptr->px_usr.px_flags 		= PROXIES_INUSE;
	KREF_GET(&dvs_kref); 			/* increment DVS reference counter 	*/

	WUNLOCK_PROC(sproxy_ptr);
	WUNLOCK_PROC(rproxy_ptr);
	WUNLOCK_PROXY(px_ptr);

	/* faltaria liberar los recursos */
	if(rcode)
		ERROR_RETURN(rcode);

	return(OK);
}

/*----------------------------------------------------------------*/
/*			do_proxies_unbind			*/
/* The both proxies must be LOCKED		*/
/* if the unbinding process is SENDER PROXY:			*/
/*	- Flush every process descriptor enqueued to send a     */
/*		CMD to a remote node				*/
/*	- Ends every node represented by the proxy pair		*/
/* if the unbinding process is RECEIVER PROXY:			*/
/*	- Flush every process waiting to receive a message 	*/
/*		from a remote process of a node represented 	*/
/*		by the unbindind proxy				*/
/*	- Ends every node represented by the proxy pair		*/
/*----------------------------------------------------------------*/
int do_proxies_unbind(struct proc *proc_ptr, struct proc *sproxy_ptr, struct proc *rproxy_ptr)
{
	struct task_struct *task_ptr;
	
	proxies_t *px_ptr;
	cluster_node_t *n_ptr;
	int px_nr;
	int i, other_pid, ret = OK;
	wait_queue_head_t wqhead;

	if( proc_ptr == NULL) ERROR_RETURN(EMOLBADPROC);

	init_waitqueue_head(&wqhead);

	px_nr = proc_ptr->p_usr.p_nr;
	MOLDEBUG(DBGPARAMS,"px_nr=%d, pid=%d\n", px_nr, proc_ptr->p_usr.p_lpid);

	if(sproxy_ptr == proc_ptr ) {		/* the SENDER PROXY */
		for( i = 0;  i < dvs.d_nr_nodes; i++) {
			if( test_bit( i ,  &sproxy_ptr->p_usr.p_nodemap)) {
				clear_bit( i ,  &sproxy_ptr->p_usr.p_nodemap);
				clear_bit( i ,  &rproxy_ptr->p_usr.p_nodemap);
				flush_sending_procs( i, sproxy_ptr);
				n_ptr = &node[i];
				WLOCK_NODE(n_ptr);
				clear_bit(NODE_BIT_SCONNECTED, &n_ptr->n_usr.n_flags);
				if( !test_bit(NODE_BIT_RCONNECTED, &n_ptr->n_usr.n_flags)) { 
					do_node_end(&node[i]);
				}
				WUNLOCK_NODE(n_ptr);
			}
		}
		task_ptr  = sproxy_ptr->p_task;
		WLOCK_TASK(task_ptr);
		mm_put_task_struct(task_ptr);	/* decrement the reference count of the task struct */
		init_proc_desc(sproxy_ptr, PROXY_NO_DC, px_nr);	
		if( rproxy_ptr->p_task != NULL && rproxy_ptr != sproxy_ptr ){
			MOLDEBUG(INTERNAL,"sending SIGPIPE to RPROXY pid=%d\n", rproxy_ptr->p_usr.p_lpid);
			ret = send_sig_info(SIGPIPE, SEND_SIG_NOINFO, rproxy_ptr->p_task);
			other_pid = rproxy_ptr->p_usr.p_lpid;
			if(rproxy_ptr->p_usr.p_rts_flags != 0) {
				rproxy_ptr->p_usr.p_rts_flags = 0;		
				LOCAL_PROC_UP(rproxy_ptr, EMOLSHUTDOWN);
			}
			
			WUNLOCK_PROC(rproxy_ptr);
			do {
				ret = wait_event_interruptible(wqhead,(other_pid != rproxy_ptr->p_usr.p_lpid));
				MOLDEBUG(INTERNAL,"other_pid=%d pid=%d\n",other_pid, rproxy_ptr->p_usr.p_lpid);
				}while(ret != 0);
			WLOCK_PROC(rproxy_ptr);
			MOLDEBUG(INTERNAL,"other_pid=%d pid=%d\n",other_pid, rproxy_ptr->p_usr.p_lpid)
		}
		WUNLOCK_TASK(task_ptr);

	} else if (rproxy_ptr == proc_ptr) {	/* the RECEIVER PROXY */
		for( i = 0;  i < dvs.d_nr_nodes; i++) {
			if( test_bit( i ,  &rproxy_ptr->p_usr.p_nodemap)) {
				clear_bit( i ,  &sproxy_ptr->p_usr.p_nodemap);
				clear_bit( i ,  &rproxy_ptr->p_usr.p_nodemap);
				flush_receiving_procs( i, rproxy_ptr);
				n_ptr = &node[i];
				WLOCK_NODE(n_ptr);
				clear_bit(NODE_BIT_RCONNECTED, &n_ptr->n_usr.n_flags);
				if( !test_bit(NODE_BIT_SCONNECTED,  &n_ptr->n_usr.n_flags)){ 
					do_node_end(&node[i]);
				}
				WUNLOCK_NODE(n_ptr);
			}
		}
		task_ptr  = rproxy_ptr->p_task;
		WLOCK_TASK(task_ptr);
		mm_put_task_struct(task_ptr);	/* decrement the reference count of the task struct */
		init_proc_desc(rproxy_ptr, PROXY_NO_DC, px_nr);	
		
		if( sproxy_ptr->p_task != NULL && rproxy_ptr != sproxy_ptr){
			MOLDEBUG(INTERNAL,"sending SIGPIPE to SPROXY pid=%d\n", sproxy_ptr->p_usr.p_lpid);
			ret = send_sig_info(SIGPIPE, SEND_SIG_NOINFO, sproxy_ptr->p_task);
			other_pid = sproxy_ptr->p_usr.p_lpid;
			if(sproxy_ptr->p_usr.p_rts_flags != 0) {
				sproxy_ptr->p_usr.p_rts_flags = 0;		
				LOCAL_PROC_UP(sproxy_ptr, EMOLSHUTDOWN);
			}			
			WUNLOCK_PROC(sproxy_ptr);
			do {
				ret = wait_event_interruptible(wqhead,(other_pid != sproxy_ptr->p_usr.p_lpid));
				MOLDEBUG(INTERNAL,"other_pid=%d pid=%d\n",other_pid, sproxy_ptr->p_usr.p_lpid);
				}while(ret != 0);
			WLOCK_PROC(sproxy_ptr);
			MOLDEBUG(INTERNAL,"other_pid=%d pid=%d\n",other_pid, sproxy_ptr->p_usr.p_lpid)
		}
		WUNLOCK_TASK(task_ptr);

	}else {
		ERROR_RETURN(EMOLBADPID);
	}

	KREF_PUT(&dvs_kref, dvs_release); 	/* decrement DVS reference counter 	*/
	
	px_ptr = &proxies[px_nr];
	WLOCK_PROXY(px_ptr);
	if( px_ptr->px_usr.px_flags == PROXIES_FREE)	{
		WUNLOCK_PROXY(px_ptr);
		ERROR_RETURN(EMOLNOPROXY);
	}
	px_ptr->px_usr.px_flags = PROXIES_FREE;
	WUNLOCK_PROXY(px_ptr);
	
	return(OK);
}

/*----------------------------------------------------------------*/
/*			mm_proxies_unbind			*/
/*----------------------------------------------------------------*/
asmlinkage int mm_proxies_unbind(int px_nr)
{
	struct proc *sproxy_ptr, *rproxy_ptr;
	int ret;

	MOLDEBUG(DBGPARAMS,"px_nr=%d\n",px_nr);

	ret = lock_sr_proxies(px_nr,  &sproxy_ptr, &rproxy_ptr);
	if( ret != OK) ERROR_RETURN(ret);

	ret = do_proxies_unbind(sproxy_ptr,  sproxy_ptr, rproxy_ptr);

	ret = unlock_sr_proxies(px_nr);
	if( ret != OK) ERROR_RETURN(ret);

	return(OK);
}


/*----------------------------------------------------------------*/
/*			mm_proxy_conn				*/
/* Its is used by the proxies to signal that that they are 	*/
/* connected/disconnected to proxies on remote nodes		*/
/* status can be: 						*/
/* CONNECT_PROXIES or DISCONNECT_PROXIES 			*/
/*----------------------------------------------------------------*/
asmlinkage int mm_proxy_conn(int px_nr, int status)
{
	struct proc *sproxy_ptr, *rproxy_ptr;
	proc_usr_t *s_ptr, *r_ptr;
	proxies_t *px_ptr;
	cluster_node_t *n_ptr;
	int ret, rcode, i;
	
	MOLDEBUG(DBGPARAMS,"px_nr=%d, status=%d\n",px_nr, status);

	ret = lock_sr_proxies(px_nr , &sproxy_ptr,  &rproxy_ptr);
	if(ret) ERROR_RETURN(ret);
	px_ptr = &proxies[px_nr];
	
	if( sproxy_ptr->p_usr.p_rts_flags == SLOT_FREE 
	||  rproxy_ptr->p_usr.p_rts_flags == SLOT_FREE ){ 
		rcode = EMOLNOPROXY; 
	}else {	
		rcode = OK;
		switch(status){
			case DISCONNECT_SPROXY:
				clear_bit(PX_BIT_SCONNECTED, &px_ptr->px_usr.px_flags);
				clear_bit(MIS_BIT_CONNECTED, &sproxy_ptr->p_usr.p_misc_flags);
				for( i = 0;  i < dvs.d_nr_nodes; i++) {
					if( test_bit( i ,  &sproxy_ptr->p_usr.p_nodemap)) {
						flush_sending_procs( i, sproxy_ptr);
						n_ptr = &node[i];
						WLOCK_NODE(n_ptr);						
						clear_bit(NODE_BIT_SCONNECTED, &n_ptr->n_usr.n_flags);
						WUNLOCK_NODE(n_ptr);						
					}
				}
				break;
			case DISCONNECT_RPROXY:
				clear_bit(PX_BIT_RCONNECTED, &px_ptr->px_usr.px_flags);
				clear_bit(MIS_BIT_CONNECTED, &rproxy_ptr->p_usr.p_misc_flags);
				for( i = 0;  i < dvs.d_nr_nodes; i++) {
					if( test_bit( i ,  &rproxy_ptr->p_usr.p_nodemap)) {
						flush_receiving_procs( i, rproxy_ptr);
						n_ptr = &node[i];
						WLOCK_NODE(n_ptr);	
						clear_bit(NODE_BIT_RCONNECTED, &n_ptr->n_usr.n_flags);
						WUNLOCK_NODE(n_ptr);						
					}
				}
				break;
			case CONNECT_SPROXY:
				set_bit(PX_BIT_SCONNECTED, &px_ptr->px_usr.px_flags);
				set_bit(MIS_BIT_CONNECTED, &sproxy_ptr->p_usr.p_misc_flags);
				for( i = 0;  i < dvs.d_nr_nodes; i++) {
					if( test_bit( i ,  &sproxy_ptr->p_usr.p_nodemap)) {
						n_ptr = &node[i];
						WLOCK_NODE(n_ptr);
						set_bit(NODE_BIT_SCONNECTED,  &n_ptr->n_usr.n_flags);
						WUNLOCK_NODE(n_ptr);						
					}
				}
				break;
			case CONNECT_RPROXY:
				set_bit(PX_BIT_RCONNECTED, &px_ptr->px_usr.px_flags);
				set_bit(MIS_BIT_CONNECTED, &rproxy_ptr->p_usr.p_misc_flags);
				for( i = 0;  i < dvs.d_nr_nodes; i++) {
					if( test_bit( i ,  &rproxy_ptr->p_usr.p_nodemap)) {
						n_ptr = &node[i];
						WLOCK_NODE(n_ptr);
						set_bit(NODE_BIT_RCONNECTED,  &n_ptr->n_usr.n_flags);
						WUNLOCK_NODE(n_ptr);						
					}
				}
				break;
			default:
				rcode =EMOLBADVALUE;	
				break;
		}
	}

	s_ptr=&sproxy_ptr->p_usr;
	r_ptr=&rproxy_ptr->p_usr;

	ret = unlock_sr_proxies(px_nr);
	if(ret) ERROR_RETURN(ret);
	
	MOLDEBUG(INTERNAL,PROC_USR_FORMAT,PROC_USR_FIELDS(s_ptr));
	MOLDEBUG(INTERNAL,PROC_USR_FORMAT,PROC_USR_FIELDS(r_ptr));

	if(rcode) ERROR_RETURN(rcode);
	return(OK);
}

/*----------------------------------------------------------------*/
/*			k_proxy_conn					*/
/* Its is used by KERNEL proxies to signal that that they are 	*/
/* connected/disconnected to proxies on remote nodes		*/
/* status can be: 						*/
/* CONNECT_PROXIES or DISCONNECT_PROXIES 			*/
/*----------------------------------------------------------------*/
asmlinkage int k_proxy_conn(int px_nr, int status)
{
	struct proc *sproxy_ptr, *rproxy_ptr;
	proc_usr_t *s_ptr, *r_ptr;
	proxies_t *px_ptr;
	cluster_node_t *n_ptr;
	int ret, rcode, i;
	
	MOLDEBUG(DBGPARAMS,"px_nr=%d, status=%d\n",px_nr, status);

	ret = lock_sr_proxies(px_nr , &sproxy_ptr,  &rproxy_ptr);
	if(ret) ERROR_RETURN(ret);
	px_ptr = &proxies[px_nr];
	
	if( sproxy_ptr->p_usr.p_rts_flags == SLOT_FREE 
	||  rproxy_ptr->p_usr.p_rts_flags == SLOT_FREE ){ 
		rcode = EMOLNOPROXY; 
	}else {	
		rcode = OK;
		switch(status){
			case DISCONNECT_SPROXY:
				clear_bit(PX_BIT_SCONNECTED, &px_ptr->px_usr.px_flags);
				clear_bit(MIS_BIT_CONNECTED, &sproxy_ptr->p_usr.p_misc_flags);
				for( i = 0;  i < dvs.d_nr_nodes; i++) {
					if( test_bit( i ,  &sproxy_ptr->p_usr.p_nodemap)) {
						flush_sending_procs( i, sproxy_ptr);
						n_ptr = &node[i];
						WLOCK_NODE(n_ptr);						
						clear_bit(NODE_BIT_SCONNECTED, &n_ptr->n_usr.n_flags);
						WUNLOCK_NODE(n_ptr);						
					}
				}
				break;
			case DISCONNECT_RPROXY:
				clear_bit(PX_BIT_RCONNECTED, &px_ptr->px_usr.px_flags);
				clear_bit(MIS_BIT_CONNECTED, &rproxy_ptr->p_usr.p_misc_flags);
				for( i = 0;  i < dvs.d_nr_nodes; i++) {
					if( test_bit( i ,  &rproxy_ptr->p_usr.p_nodemap)) {
						flush_receiving_procs( i, rproxy_ptr);
						n_ptr = &node[i];
						WLOCK_NODE(n_ptr);	
						clear_bit(NODE_BIT_RCONNECTED, &n_ptr->n_usr.n_flags);
						WUNLOCK_NODE(n_ptr);						
					}
				}
				break;
			case CONNECT_SPROXY:
				set_bit(PX_BIT_SCONNECTED, &px_ptr->px_usr.px_flags);
				set_bit(MIS_BIT_CONNECTED, &sproxy_ptr->p_usr.p_misc_flags);
				for( i = 0;  i < dvs.d_nr_nodes; i++) {
					if( test_bit( i ,  &sproxy_ptr->p_usr.p_nodemap)) {
						n_ptr = &node[i];
						WLOCK_NODE(n_ptr);
						set_bit(NODE_BIT_SCONNECTED,  &n_ptr->n_usr.n_flags);
						WUNLOCK_NODE(n_ptr);						
					}
				}
				break;
			case CONNECT_RPROXY:
				set_bit(PX_BIT_RCONNECTED, &px_ptr->px_usr.px_flags);
				set_bit(MIS_BIT_CONNECTED, &rproxy_ptr->p_usr.p_misc_flags);
				for( i = 0;  i < dvs.d_nr_nodes; i++) {
					if( test_bit( i ,  &rproxy_ptr->p_usr.p_nodemap)) {
						n_ptr = &node[i];
						WLOCK_NODE(n_ptr);
						set_bit(NODE_BIT_RCONNECTED,  &n_ptr->n_usr.n_flags);
						WUNLOCK_NODE(n_ptr);						
					}
				}
				break;
			default:
				rcode =EMOLBADVALUE;	
				break;
		}
	}

	s_ptr=&sproxy_ptr->p_usr;
	r_ptr=&rproxy_ptr->p_usr;

	ret = unlock_sr_proxies(px_nr);
	if(ret) ERROR_RETURN(ret);
	
	MOLDEBUG(INTERNAL,PROC_USR_FORMAT,PROC_USR_FIELDS(s_ptr));
	MOLDEBUG(INTERNAL,PROC_USR_FORMAT,PROC_USR_FIELDS(r_ptr));

	if(rcode) ERROR_RETURN(rcode);
	return(OK);
}


/*----------------------------------------------------------------*/
/*			do_node_end			*/
/* when a node finishs:					*/
/*	- all remote processes of that node of all DCs 	*/
/*		are unbinded				*/
/*	- clears the bit represented the node from the 	*/
/*		every DC bitmap				*/
/*	- removes the nodename entry from the /proc/dvs*/
/*----------------------------------------------------------------*/
int do_node_end(cluster_node_t *n_ptr)
{
	int dcid, i;
	dc_desc_t *dc_ptr;
	struct proc *proc_ptr;

	MOLDEBUG(DBGPARAMS,"nodeid=%d\n",n_ptr->n_usr.n_nodeid);

	for( dcid = 0; dcid < dvs.d_nr_dcs; dcid++) {
		dc_ptr = &dc[dcid];
		WLOCK_DC(dc_ptr);
		if(dc_ptr->dc_usr.dc_flags != DC_FREE) {
			/*	UNBIND all REMOTE process on that node	*/
			FOR_EACH_PROC(dc_ptr, i) {
				proc_ptr = DC_PROC(dc_ptr,i);
				WLOCK_PROC(proc_ptr);
				if( IT_IS_REMOTE(proc_ptr) &&  (proc_ptr->p_usr.p_nodeid == n_ptr->n_usr.n_nodeid)) {
					MOLDEBUG(INTERNAL,"Unbinding remote process endpoint=%d from dc=%d node=%d\n",
						proc_ptr->p_usr.p_endpoint, dcid, n_ptr->n_usr.n_nodeid);			
					do_unbind(dc_ptr, proc_ptr);
				}		
				WUNLOCK_PROC(proc_ptr);
			}
			/* clear the bit in the node and DC bitmaps */
			clear_bit(n_ptr->n_usr.n_nodeid, &dc_ptr->dc_usr.dc_nodes);
		}
		WUNLOCK_DC(dc_ptr);	
	}
	
	clear_node(n_ptr);

	return(OK);
}

/*--------------------------------------------------------------*/
/*			mm_dvs_end				*/
/* End the DVS system						*/
/* - Unbind Proxies and delete Remote Nodes and its processes	*/
/* - End al DCs: unbind local process and remove commands	*/
/* - Unbind Proxies						*/
/*--------------------------------------------------------------*/
asmlinkage long mm_dvs_end(void)
{
	int dcid, i, rcode=OK;
	dc_desc_t *dc_ptr;
	cluster_node_t *n_ptr;
	proxies_t *px_ptr;
	struct proc *sproxy_ptr, *rproxy_ptr;


	if(current_euid() != USER_ROOT) ERROR_RETURN(EMOLPRIVILEGES);

	if( DVS_NOT_INIT() )   ERROR_RETURN(EMOLDVSINIT);
	
	MOLDEBUG(DBGPARAMS,"Ending DVS. Local node ID %d\n", atomic_read(&local_nodeid));

	WLOCK_DVS;
	
	MOLDEBUG(GENERIC,"Ending DCs \n");
	/* End all DCs 		*/
	for( dcid = 0; dcid < dvs.d_nr_dcs; dcid++) {
		dc_ptr = &dc[dcid];
		WLOCK_DC(dc_ptr);
		if(dc_ptr->dc_usr.dc_flags != DC_FREE) {
			rcode = do_dc_end(dc_ptr);
			if( rcode) ERROR_PRINT(rcode);
		}
		WUNLOCK_DC(dc_ptr);	
	}

	MOLDEBUG(GENERIC,"Ending Nodes \n");
	for (i = 0; i < dvs.d_nr_nodes; i++) {
		n_ptr = &node[i];
		if( i == atomic_read(&local_nodeid)) continue;
		WLOCK_NODE(n_ptr);
		rcode = do_node_end(n_ptr);				
		WUNLOCK_NODE(n_ptr);
	}

	MOLDEBUG(GENERIC,"Ending Proxies \n");
	for (i = 0; i < dvs.d_nr_nodes; i++) {
		px_ptr = &proxies[i];
		RLOCK_PROXY(px_ptr);	
		if( px_ptr->px_usr.px_flags != PROXIES_FREE){
			RUNLOCK_PROXY(px_ptr);	
			rcode = lock_sr_proxies(i , &sproxy_ptr,  &rproxy_ptr);
			if(rcode) break;
			rcode = do_proxies_unbind(sproxy_ptr, sproxy_ptr, rproxy_ptr);
			if(rcode) break;
			rcode = unlock_sr_proxies(i);
			if(rcode) break;
		}else {	
			RUNLOCK_PROXY(px_ptr);	
		}			
	}
	
	if(rcode) {
		WUNLOCK_DVS;
		return(rcode);
	}

	KREF_PUT(&dvs_kref, dvs_release); 	/* it calls function dvs_release when count=0 */
	WUNLOCK_DVS;
	
	module_put(THIS_MODULE);
	return(OK);
}

/*--------------------------------------------------------------*/
/*			mm_node_up				*/
/* Link a node withe a proxy pair				*/
/*--------------------------------------------------------------*/
asmlinkage long mm_node_up(char *node_name, int nodeid, int px_nr)
{
	struct proc *sproxy_ptr, *rproxy_ptr;
	cluster_node_t *n_ptr;
	proxies_t *px_ptr;
	int ret;

	MOLDEBUG(DBGPARAMS,"nodeid=%d px_nr=%d\n", nodeid, px_nr);

	if(current_euid() != USER_ROOT) ERROR_RETURN(EMOLPRIVILEGES);
	if( DVS_NOT_INIT() )   	ERROR_RETURN(EMOLDVSINIT);

	CHECK_NODEID(nodeid);
	CHECK_PROXYID(px_nr);

	ret = lock_sr_proxies(px_nr , &sproxy_ptr,  &rproxy_ptr);
	if(ret) ERROR_RETURN(ret);

	set_bit(nodeid, &sproxy_ptr->p_usr.p_nodemap);
	set_bit(nodeid, &rproxy_ptr->p_usr.p_nodemap);
	MOLDEBUG(INTERNAL,"s_map=%lX r_map=%lX\n", 
		sproxy_ptr->p_usr.p_nodemap, 
		rproxy_ptr->p_usr.p_nodemap);

	ret = unlock_sr_proxies(px_nr);
	if(ret) ERROR_RETURN(ret);

	px_ptr = &proxies[px_nr];
	RLOCK_PROXY(px_ptr);
	if( px_ptr->px_usr.px_flags == PROXIES_FREE)	{
		RUNLOCK_PROXY(px_ptr);
		ERROR_RETURN(EMOLNOPROXY);
	}
	
	n_ptr = &node[nodeid];
	WLOCK_NODE(n_ptr);
	n_ptr->n_usr.n_flags   = NODE_ATTACHED;
	n_ptr->n_usr.n_proxies = px_nr;
	if( test_bit(PX_BIT_SCONNECTED, &px_ptr->px_usr.px_flags)){
		set_bit(NODE_BIT_SCONNECTED, &n_ptr->n_usr.n_flags);
	}
	if( test_bit(PX_BIT_RCONNECTED, &px_ptr->px_usr.px_flags)){
		set_bit(NODE_BIT_RCONNECTED, &n_ptr->n_usr.n_flags);
	}
	
	ret = OK;
	if( test_bit(MIS_BIT_KTHREAD, &sproxy_ptr->p_usr.p_misc_flags)){
		memcpy((void *)n_ptr->n_usr.n_name, (void *) node_name, (MAXNODENAME-1));		
	}else{
		ret = copy_from_user(n_ptr->n_usr.n_name, node_name, (MAXNODENAME-1));
	}
	WUNLOCK_NODE(n_ptr);
	RUNLOCK_PROXY(px_ptr);

	return(ret);
}

/*--------------------------------------------------------------*/
/*			mm_node_down				*/
/* Unlink a node from a proxy pair				*/
/*--------------------------------------------------------------*/
asmlinkage long mm_node_down(int nodeid)
{
	struct proc *sproxy_ptr, *rproxy_ptr;
	cluster_node_t *n_ptr;
	int px_nr, ret, rcode;

	MOLDEBUG(DBGPARAMS,"nodeid=%d\n", nodeid);

	if(current_euid() != USER_ROOT) ERROR_RETURN(EMOLPRIVILEGES);
	if( DVS_NOT_INIT() )   	ERROR_RETURN(EMOLDVSINIT);

	CHECK_NODEID(nodeid);

	n_ptr = &node[nodeid];
	WLOCK_NODE(n_ptr);
	px_nr = n_ptr->n_usr.n_proxies;

	ret = lock_sr_proxies(px_nr , &sproxy_ptr,  &rproxy_ptr);
	if(ret) ERROR_RETURN(ret);

	if(!test_bit(nodeid, &sproxy_ptr->p_usr.p_nodemap)){
		clear_bit(nodeid, &sproxy_ptr->p_usr.p_nodemap);
		clear_bit(nodeid, &rproxy_ptr->p_usr.p_nodemap);
		/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
		/*	HACER GARBAGE COLLECTION		*/
		/* ATENCION el nodo debe seguir en estado NODE_ATTACHED */
		/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
		clear_node(n_ptr);	
		rcode = OK;	
	}else{
		rcode = EMOLBADNODEID;
	}

	ret = unlock_sr_proxies(px_nr);
	if(ret) ERROR_RETURN(ret);

	WUNLOCK_NODE(n_ptr);
	if(rcode) ERROR_RETURN(ret);
	return(OK);
}

#ifdef MOLAUTOFORK
/*--------------------------------------------------------------*/
/*			kernel_lclbind				*/
/* A parent process has forked and the PM returns de p_nr	*/
/* Now the parent must bind the child				*/
/*--------------------------------------------------------------*/
long kernel_lclbind(int dcid, int pid, int p_nr)
{
	struct proc *proc_ptr;
	dc_desc_t *dc_ptr;
	int g, i, endpoint, rcode, nodeid;
	struct task_struct *task_ptr;	

	if( DVS_NOT_INIT() )   ERROR_RETURN(EMOLDVSINIT);
	nodeid = atomic_read(&local_nodeid);
MOLDEBUG(DBGLVL1,"dcid=%d pid=%d p_nr=%d nodeid=%d\n",dcid, pid, p_nr, nodeid);

	dc_ptr 		= &dc[dcid];
	WLOCK_DC(dc_ptr);	
	if( dc_ptr->dc_usr.dc_flags) 			ERROR_WUNLOCK_DC(dc_ptr,EMOLDCNOTRUN);

	i = p_nr+dc_ptr->dc_usr.dc_nr_tasks;
	proc_ptr = DC_PROC(dc_ptr,i);
	if(!test_bit(BIT_SLOT_FREE, &proc_ptr->p_usr.p_rts_flags)) 	
		ERROR_WUNLOCK_DC(dc_ptr,EMOLRSCBUSY);

	init_proc_desc(proc_ptr, dcid, i);		/* Initialize all process descriptor fields	*/

// 	strncpy((char* )&proc_ptr->p_usr.p_name, (char*)task_ptr->comm, MAXPROCNAME-1);
	proc_ptr->p_name_ptr = (char*)task_ptr->comm;

// MOLDEBUG(DBGLVL1,"process p_name=%s *p_name_ptr=%s\n", (char*)&proc_ptr->p_usr.p_name, proc_ptr->p_name_ptr);
		
	proc_ptr->p_usr.p_rts_flags	= PROC_RUNNING;		/* set to RUNNING STATE	*/
	proc_ptr->p_usr.p_nodeid	= nodeid;	
	proc_ptr->p_usr.p_lpid 		= pid;			/* Update PID		*/
	if( i < dc_ptr->dc_usr.dc_nr_sysprocs) {
		proc_ptr->p_usr.p_endpoint = _ENDPOINT(0,proc_ptr->p_usr.p_nr);
		proc_ptr->p_priv.s_usr.s_id = i;
	}else{
		g = _ENDPOINT_G(proc_ptr->p_usr.p_endpoint);	/* Update endpoint 	*/
		if(++g >= _ENDPOINT_MAX_GENERATION)		/* increase generation */
			g = 1;					/* generation number wraparound */
		proc_ptr->p_usr.p_endpoint = _ENDPOINT(g,proc_ptr->p_usr.p_nr);
		proc_ptr->p_priv.s_usr.s_id = dc_ptr->dc_usr.dc_nr_sysprocs;
	}

	
MOLDEBUG(DBGLVL1,"i=%d p_nr=%d dcid=%d lpid=%d endpoint=%d nodeid=%d name=%s\n",
		i,
		proc_ptr->p_usr.p_nr, 
		proc_ptr->p_usr.p_dcid,
		proc_ptr->p_usr.p_lpid,
		proc_ptr->p_usr.p_endpoint,
		proc_ptr->p_usr.p_nodeid,
		proc_ptr->p_usr.p_name
		);

	endpoint = proc_ptr->p_usr.p_endpoint;
	
	DC_INCREF(dc_ptr);
	WUNLOCK_DC(dc_ptr);
	return(endpoint);
}

#endif /*MOLAUTOFORK */
