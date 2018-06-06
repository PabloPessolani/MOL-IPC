/****************************************************************/
/*		MINIX OVER LINUX IPC MIGRATION SUPPORT 		*/
/* MIGRATION START: On REMOTE nodes, the process descriptor is signaled */
/*                                    On LOCAL old node, only the thread group leader */
/* 					can START the migration and the descriptor is	*/
/*					signaled							*/
/* MIGRATION FAILED: On REMOTE nodes, the process descriptor are reset */
/*                                    On LOCAL old node, only the thread group leader */
/* 					can FAIL the migration					*/
/*					ALL threads are reset					*/
/* MIGRATION END:  EVERY thread migrated needs a mol_migrate call	*/
/****************************************************************/

extern int send_sig_info(int, struct siginfo *, struct task_struct *);
extern struct cpuinfo_x86 boot_cpu_data;

/*------------------------------------------------------*/
/* Searchs all processes at the MIGRATION list and*/
/* clear the BIT_WAITMIGR flag			*/
/* They includes all processes that trying to send a	*/
/*  message AFTER MIGRATE_START			*/
/* and BEFORE MIGRATE_END				*/
/* ON INPUT: proc WLOCKED				*/
/* ON OUTPUT: proc WLOCKED				*/
/*------------------------------------------------------*/
void flush_migr_list( struct proc *proc_ptr)
{
	struct proc *xpp, *tmp_ptr;
	
	MOLDEBUG(GENERIC,"Searchs all processes at the MIGRATING list of ep=%d\n", proc_ptr->p_usr.p_endpoint);		
	LIST_FOR_EACH_ENTRY_SAFE(xpp, tmp_ptr, &proc_ptr->p_mlist, p_mlink) {
	
		WLOCK_ORDERED2(proc_ptr, xpp);
		LIST_DEL_INIT(&xpp->p_mlink); /* remove from queue */	
		if( xpp->p_usr.p_waitmigr == proc_ptr->p_usr.p_endpoint) {
			MOLDEBUG(GENERIC,"endpoint=%d name=%s\n", xpp->p_usr.p_endpoint,xpp->p_usr.p_name);
			clear_bit(BIT_WAITMIGR, &xpp->p_usr.p_rts_flags);
			xpp->p_usr.p_waitmigr = NONE;
			if(xpp->p_usr.p_rts_flags == 0) 
				LOCAL_PROC_UP(xpp, EMOLMIGRATE);
		}else{	
			ERROR_PRINT(EMOLENDPOINT);
		}
	 	WUNLOCK_PROC(xpp); /* proc LOCK is just locked */
	}
}

/* 
* Searchs all processes at the migrating process SENDING list
* and returns then EMOLMIGRATE to replay the IPC
* They includes all processes that trying to send a message BEFORE MIGRATE_START
* ON INPUT: proc WLOCKED				
* ON OUTPUT: proc WLOCKED				
*/
void flush_sending_list(struct proc *proc_ptr)
{
	struct proc *xpp, *tmp_ptr;
	proxy_hdr_t header, *h_ptr;

	MOLDEBUG(GENERIC,"Searchs all processes at the process sending list of ep=%d\n", proc_ptr->p_usr.p_endpoint);		
	LIST_FOR_EACH_ENTRY_SAFE(xpp, tmp_ptr, &proc_ptr->p_list, p_link) {
		WLOCK_ORDERED2(proc_ptr, xpp);
		MOLDEBUG(GENERIC,"endpoint=%d name=%s\n", xpp->p_usr.p_endpoint,xpp->p_usr.p_name);
		LIST_DEL_INIT(&xpp->p_link); /* remove from queue */	
		if( IT_IS_LOCAL(xpp)) {
			MOLDEBUG(GENERIC,"Replay IPC from endpoint=%d\n", xpp->p_usr.p_endpoint);
			clear_bit(BIT_SENDING,&xpp->p_usr.p_rts_flags);
			clear_bit(BIT_RECEIVING,&xpp->p_usr.p_rts_flags);
			xpp->p_usr.p_getfrom = NONE;
			xpp->p_usr.p_sendto  = NONE;
			if(xpp->p_usr.p_rts_flags == PROC_RUNNING) {
				LOCAL_PROC_UP(xpp, EMOLMIGRATE);
			}	
		}else{
			/*
			* Return an EMOLMIGRATE error to the sender of the message
			*/
			if( !(xpp->p_rmtcmd.c_cmd & (1 << BIT_ACKNOWLEDGE))){
				MOLDEBUG(GENERIC,"send EMOLMIGRATE remote=%d\n", xpp->p_usr.p_endpoint);
				memcpy(&header, &xpp->p_rmtcmd, sizeof(proxy_hdr_t));
				h_ptr = &header;
				/* Send an ACKNOWLEDGE to the sending process with EMOLMIGRATE error */
				error_lcl2rmt( xpp->p_rmtcmd.c_cmd | (1 << BIT_ACKNOWLEDGE), xpp, h_ptr, EMOLMIGRATE);
			}else{
				MOLDEBUG(GENERIC,"Ignore ACKs message from remote=%d\n", xpp->p_usr.p_endpoint);
			}
		}
		WUNLOCK_PROC(xpp);
	}
}
	
/*------------------------------------------------------*/
/*		migr_rollback				*/
/* MIGR_ROLLBACK:  The process migration has failed. 	*/
/* Reset the BIT_MIGRATE.				*/
/* Reset the BIT_WAITMIGR and the p_waitmigr descriptor */
/* field of all "waiting to send" processes 		*/
/*------------------------------------------------------*/
long int migr_rollback(struct proc *proc_ptr)
{

	struct task_struct *task_ptr, *thread_ptr;
	proc_usr_t *pu_ptr; 
	struct proc *p_ptr;
	int ret;
	
	pu_ptr= &proc_ptr->p_usr;
	MOLDEBUG(DBGPARAMS,PROC_USR_FORMAT, PROC_USR_FIELDS(pu_ptr));
	ret = OK;

	if( IT_IS_REMOTE(proc_ptr)) {
		do {
			if( test_bit(BIT_SLOT_FREE, &proc_ptr->p_usr.p_rts_flags))	{ret = EMOLNOTBIND; break;}
			if( test_bit(BIT_ONCOPY, &proc_ptr->p_usr.p_rts_flags))		{ret = EMOLONCOPY; break;}
			if( test_bit(BIT_RMTOPER, &proc_ptr->p_usr.p_rts_flags))	{ret = EMOLBUSY; break;}
			if(test_bit(BIT_NO_ENDPOINT, &proc_ptr->p_usr.p_rts_flags))	{ret = EMOLENDPOINT; break;}
			if(!test_bit(BIT_MIGRATE, &proc_ptr->p_usr.p_rts_flags)) 	{ret = EMOLMIGRATE; break;}
		}while(0);
		if(ret) ERROR_RETURN(ret);
		clear_bit(MIS_BIT_NEEDMIGR, &proc_ptr->p_usr.p_misc_flags);
		clear_bit(BIT_MIGRATE, &proc_ptr->p_usr.p_rts_flags);
		return(OK);
	}
	
	if(!test_bit(MIS_BIT_GRPLEADER, &proc_ptr->p_usr.p_misc_flags))	ERROR_RETURN(EMOLGRPLEADER);

	/* FIRST LOOP: Check the correct status of all process' threads */
	task_ptr = thread_ptr = proc_ptr->p_task;
	WUNLOCK_PROC(proc_ptr);
	lock_tasklist(); //read_lock(&tasklist_lock);
	do {
		p_ptr = thread_ptr->task_proc;
		if( p_ptr != NULL) {		/* Ignore not binded threads */
			RLOCK_PROC(p_ptr);
			do {
				if( test_bit(BIT_SLOT_FREE, &p_ptr->p_usr.p_rts_flags))	{ret = EMOLNOTBIND; break;}
				if( test_bit(BIT_ONCOPY, &p_ptr->p_usr.p_rts_flags))	{ret = EMOLONCOPY; break;}
				if( test_bit(BIT_RMTOPER, &p_ptr->p_usr.p_rts_flags))	{ret = EMOLBUSY; break;}
				if(test_bit(BIT_NO_ENDPOINT, &p_ptr->p_usr.p_rts_flags)){ret = EMOLENDPOINT; break;}
				if(!test_bit(BIT_MIGRATE, &p_ptr->p_usr.p_rts_flags)) 	{ret = EMOLMIGRATE; break;}
			}while(0);
			RUNLOCK_PROC(p_ptr);
			if(ret) { 
				unlock_tasklist(); //read_unlock(&tasklist_lock);
				WLOCK_PROC(proc_ptr);
				ERROR_RETURN(ret);	
			}
		}
	}while_each_thread(task_ptr, thread_ptr);
	WLOCK_PROC(proc_ptr);
	/* SECOND LOOP: Clear the MIGRATION BITS from the process' threads descriptors */
	task_ptr = thread_ptr = proc_ptr->p_task;
	WUNLOCK_PROC(proc_ptr);
	do {
		p_ptr = thread_ptr->task_proc;
		if( p_ptr != NULL) {		/* Ignore not binded threads */
			WLOCK_PROC(p_ptr);
			clear_bit(MIS_BIT_NEEDMIGR, &p_ptr->p_usr.p_misc_flags);
			clear_bit(BIT_MIGRATE, &p_ptr->p_usr.p_rts_flags);
			flush_migr_list(p_ptr);
			if(p_ptr->p_usr.p_rts_flags == PROC_RUNNING )
				LOCAL_PROC_UP(p_ptr, OK);
			WUNLOCK_PROC(p_ptr);
		}
	}while_each_thread(task_ptr, thread_ptr);
	
	unlock_tasklist(); //read_unlock(&tasklist_lock);
	WLOCK_PROC(proc_ptr);

	return(OK);
}

/*------------------------------------------------------*/
/*		migr_start				*/
/* MIGR_START:  The process migration has started. 	*/
/* The process descriptor is WRITE LOCKED		*/
/* Set the BIT_MIGRATE to change the behaviour of IPC 	*/
/* if the local node is the old node of the process 	*/
/* sets the NEED MIGRATION bit to signal the 	*/
/* IPC kernel to set the BIT_MIGRATE in the next IPC call */
/*------------------------------------------------------*/
long int migr_start(struct proc *proc_ptr)
{
	struct task_struct *task_ptr, *thread_ptr;
	struct proc *p_ptr;
	proc_usr_t *pu_ptr; 
	int ret;
	
	pu_ptr= &proc_ptr->p_usr;
	MOLDEBUG(DBGPARAMS,PROC_USR_FORMAT, PROC_USR_FIELDS(pu_ptr));
	ret = OK;

	if( IT_IS_REMOTE(proc_ptr)) {
		do {
			if( test_bit(BIT_SLOT_FREE, &proc_ptr->p_usr.p_rts_flags))	{ret = EMOLNOTBIND; break;}
			if( test_bit(BIT_ONCOPY, &proc_ptr->p_usr.p_rts_flags))		{ret = EMOLONCOPY; break;}
			if( test_bit(BIT_RMTOPER, &proc_ptr->p_usr.p_rts_flags))	{ret = EMOLBUSY; break;}
			if(test_bit(BIT_NO_ENDPOINT, &proc_ptr->p_usr.p_rts_flags))	{ret = EMOLENDPOINT; break;}
			if(test_bit(BIT_MIGRATE, &proc_ptr->p_usr.p_rts_flags)) 	{ret = EMOLMIGRATE; break;}
		}while(0);
		if(ret) ERROR_RETURN(ret);
		/* Signal the REMOTE process with the  BIT_MIGRATE */
		set_bit(BIT_MIGRATE, &proc_ptr->p_usr.p_rts_flags);
		return(OK);
	}
	
	if(!test_bit(MIS_BIT_GRPLEADER, &proc_ptr->p_usr.p_misc_flags))	ERROR_RETURN(EMOLGRPLEADER);

	/* FIRST LOOP: Check the correct status of all process' threads */
	task_ptr = thread_ptr = proc_ptr->p_task;
	WUNLOCK_PROC(proc_ptr);
	lock_tasklist(); //read_lock(&tasklist_lock);
	do {
		p_ptr = thread_ptr->task_proc;
		if( p_ptr != NULL) {		/* Ignore not binded threads */
			RLOCK_PROC(p_ptr);
			do {
				if( test_bit(BIT_SLOT_FREE, &p_ptr->p_usr.p_rts_flags))	{ret = EMOLNOTBIND; break;}
				if( test_bit(BIT_ONCOPY, &p_ptr->p_usr.p_rts_flags))		{ret = EMOLONCOPY; break;}
				if( test_bit(BIT_RMTOPER, &p_ptr->p_usr.p_rts_flags))	{ret = EMOLBUSY; break;}
				if(test_bit(BIT_NO_ENDPOINT, &p_ptr->p_usr.p_rts_flags))	{ret = EMOLENDPOINT; break;}
				if(test_bit(BIT_MIGRATE, &p_ptr->p_usr.p_rts_flags)) 	{ret = EMOLMIGRATE; break;}
			}while(0);
			RUNLOCK_PROC(p_ptr);
			if(ret) {
				unlock_tasklist(); //read_unlock(&tasklist_lock);
				WLOCK_PROC(proc_ptr);
				ERROR_RETURN(ret);
			}
		}
	}while_each_thread(task_ptr, thread_ptr);

	WLOCK_PROC(proc_ptr);
	/* SECOND LOOP: Signal all LOCAL process' threads with the  MIS_BIT_NEEDMIGR */
	task_ptr = thread_ptr = proc_ptr->p_task;
	do {
		/* Cant set BIT_MIGRATE because the process is running */
		/* Set the MIS_BIT_NEEDMIGR to set this BIT_MIGRATE in the next IPC call  */
		p_ptr = thread_ptr->task_proc;
		if( p_ptr != NULL) 		/* Ignore not binded threads */
			set_bit(MIS_BIT_NEEDMIGR, &p_ptr->p_usr.p_misc_flags);
	}while_each_thread(task_ptr, thread_ptr);
	unlock_tasklist(); //read_unlock(&tasklist_lock);
	return(OK);
}

/*------------------------------------------------------*/
/*		old_migrate				*/
/* LOCAL node is OLD node				*/
/* The process descriptor is WRITE LOCKED		*/
/* Convert the LOCAL descriptor into a REMOTE descriptor */
/* if a LOCAL process remainder is into IPC kernel 	*/
/* wakeup it with error					*/
/* ON INPUT: proc WLOCKED				*/
/* ON OUTPUT: proc WLOCKED				*/
/*------------------------------------------------------*/
long int old_migrate(struct proc *proc_ptr, int new_nodeid)
{
	struct task_struct *task_ptr, *thread_ptr; 
	struct proc *p_ptr;	
	int ret;

	MOLDEBUG(GENERIC,"new_nodeid=%d\n", new_nodeid );

	/* Verify that all THREADS of the same process has the BIT_MIGRATE flag */
	thread_ptr = proc_ptr->p_task;	
	task_ptr   = proc_ptr->p_task;
	WUNLOCK_PROC(proc_ptr);
	ret = OK;
	lock_tasklist(); //read_lock(&tasklist_lock);
	do {
		p_ptr = thread_ptr->task_proc;
		if( p_ptr != NULL) {		/* Ignore not binded threads */
			RLOCK_PROC(p_ptr);		
			if( !test_bit(BIT_MIGRATE, &p_ptr->p_usr.p_rts_flags)) ret = EMOLMIGRATE;
			RUNLOCK_PROC(p_ptr);
			if(ret) {
				unlock_tasklist(); //read_unlock(&tasklist_lock);
				WLOCK_PROC(proc_ptr);
				ERROR_RETURN(ret);
			}
		}
	}while_each_thread(task_ptr, thread_ptr);  /*!!!! DONT USE BREAK, BECAUSE HERE ARE TWO LOOPS !!!! */
	unlock_tasklist(); //read_unlock(&tasklist_lock);

	WLOCK_PROC(proc_ptr);
	if(!test_bit(MIS_BIT_GRPLEADER, &proc_ptr->p_usr.p_misc_flags)) ERROR_RETURN(EMOLGRPLEADER);
	thread_ptr = proc_ptr->p_task;	
	task_ptr   = proc_ptr->p_task;
	WUNLOCK_PROC(proc_ptr);

	lock_tasklist(); //read_lock(&tasklist_lock);
	do {
		p_ptr = thread_ptr->task_proc;
		if( p_ptr != NULL) {		/* Ignore not binded threads */
			WLOCK_PROC(p_ptr);
			if(test_bit(MIS_BIT_GRPLEADER, &p_ptr->p_usr.p_misc_flags))
				clear_bit(MIS_BIT_GRPLEADER, &p_ptr->p_usr.p_misc_flags);
			/* Convert the LOCAL descriptor into a REMOTE descriptor */
			p_ptr->p_usr.p_lpid 	= PROC_NO_PID;	/* Update PID		*/
			p_ptr->p_usr.p_nodeid 	= new_nodeid;
			thread_ptr->task_proc	= NULL;
			p_ptr->p_task 			= NULL;
			mm_put_task_struct(thread_ptr);		/* decrement the reference count of the task struct */

			/* if a LOCAL process remainder is into IPC kernel 	*/
			/* wakeup it with error					*/
			clear_bit(BIT_MIGRATE,&p_ptr->p_usr.p_rts_flags);
			if (p_ptr->p_usr.p_rts_flags != PROC_RUNNING) {	
				set_bit(BIT_REMOTE,&p_ptr->p_usr.p_rts_flags);
				LOCAL_PROC_UP(p_ptr, EMOLRMTPROC);
			}else{
				set_bit(BIT_REMOTE,&p_ptr->p_usr.p_rts_flags);
			}
			flush_sending_list(p_ptr);
			flush_migr_list(p_ptr);
			WUNLOCK_PROC(p_ptr);
		}
	}while_each_thread(task_ptr, thread_ptr);  /*!!!! DONT USE BREAK, BECAUSE HERE ARE TWO LOOPS !!!! */
	unlock_tasklist(); //read_unlock(&tasklist_lock);
	WLOCK_PROC(proc_ptr);
	return(OK);
}

/*------------------------------------------------------*/
/*		new_migrate				*/
/* LOCAL node is NEW node				*/
/*------------------------------------------------------*/
long int new_migrate(dc_desc_t *dc_ptr, struct proc *proc_ptr, int pid)
{
	struct task_struct *task_ptr;

	MOLDEBUG(GENERIC,"pid=%d\n", pid);
	
	if(! test_bit(MIS_BIT_RMTBACKUP, &proc_ptr->p_usr.p_misc_flags)) {  /* it is NOT a remote process' backup */
		/* Search for the local proc */
		lock_tasklist(); //read_lock(&tasklist_lock);
		task_ptr = pid_task(find_vpid(pid), PIDTYPE_PID);  
		if(task_ptr == NULL ) {
			unlock_tasklist(); //read_unlock(&tasklist_lock);
			ERROR_RETURN(EMOLBADPROC);
		}
		unlock_tasklist(); //read_unlock(&tasklist_lock);
		get_task_struct(task_ptr);		/* increment the reference count of the task struct */
		proc_ptr->p_task = task_ptr;		/* Set the task descriptor into the process descriptor */

		WLOCK_TASK(task_ptr);
		task_ptr->task_proc = (struct proc *) proc_ptr;	/* Set the  process descriptor into the task descriptor */
		strncpy((char* )&proc_ptr->p_usr.p_name, (char*)task_ptr->comm, MAXPROCNAME-1);
		proc_ptr->p_name_ptr = (char*)task_ptr->comm;
		WUNLOCK_TASK(task_ptr);
		proc_ptr->p_usr.p_lpid 		= pid;				/* Update PID		*/
	}else{
		if( proc_ptr->p_usr.p_lpid != pid) ERROR_RETURN(EMOLBADPID);
	}
	
	proc_ptr->p_usr.p_nodeid	= atomic_read(&local_nodeid);	
	proc_ptr->p_usr.p_rts_flags	= PROC_RUNNING;			/* set to RUNNING STATE	*/

	return(OK);
}

/*------------------------------------------------------*/
/*		migr_commit				*/
/* MIGR_START:  The process migration has started. 	*/
/* Set the BIT_MIGRATE to change the behaviour of IPC 	*/
/* ON INPUT: proc WLOCKED				*/
/* ON OUTPUT: proc WLOCKED				*/
/*------------------------------------------------------*/
long int migr_commit(dc_desc_t *dc_ptr, struct proc *proc_ptr, int pid, int new_nodeid)
{
	int ret;
	proc_usr_t *pu_ptr;
	cluster_node_t *newnode_ptr;

	MOLDEBUG(DBGPARAMS,"pid=%d local_nodeid=%d new_nodeid=%d\n",
		pid, atomic_read(&local_nodeid), new_nodeid)

	if( new_nodeid < 0 || new_nodeid >= dvs.d_nr_nodes) ERROR_RETURN(EMOLNOPROXY);
	newnode_ptr = &node[new_nodeid];
	RLOCK_NODE(newnode_ptr);
	ret = OK;
	do {
		if(newnode_ptr->n_usr.n_flags == NODE_FREE) 		{ret = EMOLNOPROXY; break;}
		if(!test_bit(new_nodeid, &dc_ptr->dc_usr.dc_nodes))	{ret = EMOLDCNODE; break;}
		if(!test_bit(dc->dc_usr.dc_dcid, &newnode_ptr->n_usr.n_dcs)) 		{ret = EMOLDCNODE; break;} 
	}while(0);
	RUNLOCK_NODE(newnode_ptr);
	if(ret) ERROR_RETURN(ret);
	
	if( atomic_read(&local_nodeid) == proc_ptr->p_usr.p_nodeid) {
		ret = old_migrate(proc_ptr, new_nodeid);
	}else if(atomic_read(&local_nodeid) == new_nodeid) {		
		ret = new_migrate(dc_ptr, proc_ptr, pid);
	}else{
		proc_ptr->p_usr.p_nodeid = new_nodeid;
		flush_migr_list(proc_ptr);
		ret = OK;
	}
	if(ret) ERROR_RETURN(ret);

	/* Set the new node where the process migrate to */
//	set_node_bit(proc_ptr->p_usr.p_nodemap, new_nodeid);
	set_bit(new_nodeid, &proc_ptr->p_usr.p_nodemap);

	/* clear  BIT_MIGRATE  */
	clear_bit(BIT_MIGRATE, &proc_ptr->p_usr.p_rts_flags);
	clear_bit(MIS_BIT_NEEDMIGR, &proc_ptr->p_usr.p_misc_flags);

	pu_ptr = &proc_ptr->p_usr;
	MOLDEBUG(INTERNAL,PROC_USR_FORMAT, PROC_USR_FIELDS(pu_ptr));
	return(OK);
}

/*--------------------------------------------------------------*/
/*			mol_migrate				*/
/* A process can only migrate when it request a service		*/
/* to its SYSTASK (sendrec operation), therefore its p_rts_flags*/
/* must have only de BIT_RECEIVING and the p_getfrom = SYSTASK  */
/* The caller of mol_migrate would be:				*/
/*	- MIGR_START: old node's SYSTASK 			*/
/*	- MIGR_COMMIT:   new node's SYSTASK 			*/
/*	- MIGR_ROLLBACK: old node's SYSTASK 			*/
/* dcid: get from caller => must be binded in the DC		*/
/* ONLY THE MAIN THREAD aka GROUP LEADER		*/
/* can be invoked in mol_migrate					*/
/* All threads in the process must be in the correct		*/
/* state to be migrated							*/
/*--------------------------------------------------------------*/
asmlinkage long mm_migrate(int oper, int pid, int dcid, int endpoint, int new_nodeid)
{
	dc_desc_t *dc_ptr;
	int ret, p_nr;
	struct proc *proc_ptr;
	struct task_struct *task_ptr;

	if(current_euid() != USER_ROOT) ERROR_RETURN(-EPERM);

	if( DVS_NOT_INIT() )   ERROR_RETURN(EMOLDVSINIT );
	
	MOLDEBUG(DBGPARAMS,"oper=%d pid=%d dcid=%d endpoint=%d new_nodeid=%d\n",
		oper, pid, dcid, endpoint, new_nodeid);

	if( oper != MIGR_START 	&& 
		oper != MIGR_COMMIT	&& 
		oper != MIGR_ROLLBACK	)		ERROR_RETURN(EMOLNOSYS);
	
	if( dcid < 0 || dcid >= dvs.d_nr_dcs) 		ERROR_RETURN(EMOLBADDCID);
	dc_ptr 	= &dc[dcid];
	RLOCK_DC(dc_ptr);

	ret = OK;  	
	p_nr = _ENDPOINT_P(endpoint);
	MOLDEBUG(GENERIC,"check endpoint=%d p_nr=%d \n", endpoint, p_nr);
	do {
		if( p_nr < (-dc_ptr->dc_usr.dc_nr_tasks)		
		 || p_nr >= dc_ptr->dc_usr.dc_nr_procs) 	{ret = EMOLRANGE; break;}
		proc_ptr   = NBR2PTR(dc_ptr, p_nr);
		if( proc_ptr == NULL) 						{ret = EMOLDSTDIED; break;}
	}while(0);
	if(ret) {
		RUNLOCK_DC(dc_ptr);
		ERROR_RETURN(ret);
	}
	
	WLOCK_PROC(proc_ptr);
	if(IT_IS_LOCAL(proc_ptr) ){
		task_ptr = proc_ptr->p_task;
		if(task_ptr == NULL) {
			WUNLOCK_PROC(proc_ptr);
			RUNLOCK_DC(dc_ptr);
			ERROR_RETURN(EMOLBADPROC);
		}
	}  	

	MOLDEBUG(GENERIC,"Operation=%d\n", oper);		
	switch(oper) {
		case MIGR_START:
			ret = migr_start(proc_ptr);
			break;
		case MIGR_ROLLBACK:
			ret = migr_rollback(proc_ptr);
			break;
		case MIGR_COMMIT:
			ret = migr_commit(dc_ptr, proc_ptr, pid, new_nodeid);
			break;
		default:
			ret = EMOLNOSYS;
			break;
	}
	
	WUNLOCK_PROC(proc_ptr);
	RUNLOCK_DC(dc_ptr);

	if(ret) ERROR_RETURN(ret);	

	return(OK);
}
	
