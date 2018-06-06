///****************************************************************/
///*			MOL UTILITIES				*/
///****************************************************************/
//
#include "mol.h"
#include "mol-proto.h"
#include "mol-glo.h"
#include "mol-macros.h"
//
//
//asmlinkage int copy_pte_range(struct mm_struct *dst_mm, struct mm_struct *src_mm,
//		pmd_t *dst_pmd, pmd_t *src_pmd, struct dc_area_struct *dca,
//		unsigned long addr, unsigned long end);
//
//		
//proc_t *get_sproxy(int nodeid) 
//{
//
//	cluster_node_t *n_ptr;
//	proxies_t *px_ptr;
//	proc_t *sproxy_ptr;
//
//	n_ptr = &node[nodeid];
//	RLOCK_NODE(n_ptr);
//	px_ptr = &proxies[n_ptr->n_usr.n_proxies];
//	RUNLOCK_NODE(n_ptr);
//
//	RLOCK_PROXY(px_ptr);
//	sproxy_ptr = &px_ptr->px_sproxy;
//	RUNLOCK_PROXY(px_ptr);
//	
//	return(sproxy_ptr);
//} 
//		
//proc_t *get_rproxy(int nodeid) 
//{
//
//	cluster_node_t *n_ptr;
//	proxies_t *px_ptr;
//	proc_t *rproxy_ptr;
//
//	n_ptr = &node[nodeid];
//	RLOCK_NODE(n_ptr);
//	px_ptr = &proxies[n_ptr->n_usr.n_proxies];
//	RUNLOCK_NODE(n_ptr);
//
//	RLOCK_PROXY(px_ptr);
//	rproxy_ptr = &px_ptr->px_rproxy;
//	RUNLOCK_PROXY(px_ptr);
//	
//	return(rproxy_ptr);
//} 
//
///*--------------------------------------------------------------*/
///*			check_lock_caller			*/
///* Checks if the caller is a thread. Checks if it is		*/
///* binded or its main thread is binded				*/
///* ON OUTPUT: 							*/
///*   if ret==OK; caller_ptr is WRITE LOCKED			*/			
///*	*c_ptr = caller_ptr;					*/
///*	*t_ptr = task_ptr;					*/
///*	*c_pid = caller_pid;					*/
///*--------------------------------------------------------------*/
//
//int check_lock_caller(struct task_struct **t_ptr, struct proc **c_ptr, int *c_pid)
//{
//	int ret;
//	struct task_struct *task_ptr;
//	struct proc *caller_ptr;
//	int  caller_pid, caller_tgid;
//	proc_usr_t *s_ptr;
//
//	task_ptr = current;
//	caller_pid  = task_pid_nr(current);
//	caller_tgid = current->tgid;
//	MOLDEBUG(DBGPARAMS,"caller_pid=%d caller_tgid=%d\n", 
//		caller_pid, caller_tgid);
//	ret = OK;
//	do {
////		if(caller_pid == caller_tgid) {	/* task_ptr it is a MAIN thread 	*/
//		if( thread_group_leader(task_ptr)){ /* Caller is the Group Leader */
//			caller_ptr = (struct proc *) task_ptr->proc_ptr;
//			if( caller_ptr == NULL){		/* The main thread is not binded 	*/
//				task_ptr = current;			
//				ret = EMOLNOTBIND;
//				break;
//			}
//		}else {					/* Caller is NOT the Group Leader */	
//			caller_ptr = (struct proc *)current->proc_ptr;
//			if( caller_ptr == NULL){			/* The child thread is not binded */
//				task_ptr= task_ptr->group_leader;
//				if( task_ptr == NULL) {			/* the Group Leader has dead */
//					task_ptr = current;
//					ret = EMOLBADPROC;	
//					break;
//				}	
//				caller_ptr = (struct proc *) task_ptr->proc_ptr;
//				if(caller_ptr == NULL){			/* The main thread is not binded */
//					ret = EMOLNOTBIND;
//					break;
//				}else{
//					caller_pid = task_ptr->tgid;	/* Use the main thread binding	*/	
//				}
//			}
//		}
//	}while(0);
//
//	/* The DC administrator has set the need to migrate for this process */
//	if( ret)  ERROR_RETURN(ret);
//
//	WLOCK_PROC(caller_ptr);
//	
//	if( test_bit(MIS_BIT_NEEDMIGR, &caller_ptr->p_usr.p_misc_flags)){
//		s_ptr = &caller_ptr->p_usr;
//		MOLDEBUG(INTERNAL,PROC_USR_FORMAT,PROC_USR_FIELDS(s_ptr));
//		clear_bit(MIS_BIT_NEEDMIGR, &caller_ptr->p_usr.p_misc_flags);
//		set_bit(BIT_MIGRATE, &caller_ptr->p_usr.p_rts_flags);
//	}
//	
//	if (test_bit(BIT_MIGRATE, &caller_ptr->p_usr.p_rts_flags)) {
//		sleep_proc(caller_ptr, TIMEOUT_FOREVER);
//		ret = caller_ptr->p_rcode;
//		if(ret) {
//			WUNLOCK_PROC(caller_ptr);
//			ERROR_RETURN(ret);
//		}
//	}
//
//	*c_ptr = caller_ptr;
//	*t_ptr = task_ptr;
//	*c_pid = caller_pid;
//	MOLDEBUG(DBGPARAMS,"caller_pid=%d \n", caller_pid);
//
//	return(OK);
//}
//
//
//
///*----------------------------------------------------------------*/
///*			init_proc_desc				*/
///* !!! On input and output the DC mutex must be locked !!!	*/
///*----------------------------------------------------------------*/
//void init_proc_desc(struct proc *proc_ptr, int dcid, int index)
//{
//	int j;
//	DC_desc_t *dc_ptr;
//
//	MOLDEBUG(INTERNAL,"p_name=%s dcid=%d\n",proc_ptr->p_usr.p_name, dcid);
//
//	/*Suppose that the dcid has been checked before */
//	if( dcid == PROXY_NO_DC) {
//		proc_ptr->p_usr.p_nr 	= index;		/* proxies struct index */
//	}else{
//		dc_ptr = &dc[dcid];
//		proc_ptr->p_usr.p_nr 	= (index-(dc_ptr->dc_usr.dc_nr_tasks));
//		for (j=0; j< BITMAP_CHUNKS(dvs.d_nr_sysprocs); j++) {
//	      		proc_ptr->p_priv.s_notify_pending.chunk[j] = 0;	
//	      		proc_ptr->p_priv.s_usr.s_ipc_from.chunk[j] = 0;	
//	      		proc_ptr->p_priv.s_usr.s_ipc_to.chunk[j] = 0;	
//		}
//	}
//
//	proc_ptr->p_priv.s_usr.s_id	   		= 0;
//	proc_ptr->p_priv.s_usr.s_warn	   	= NONE;
//	proc_ptr->p_priv.s_usr.s_level     	= USER_PRIV;
//	proc_ptr->p_priv.s_usr.s_trap_mask 	= 0;
//	proc_ptr->p_priv.s_usr.s_call_mask 	= 0;
//
//	proc_ptr->p_priv.s_int_pending		= 0;
//	proc_ptr->p_priv.s_sig_pending		= 0;
//
//	proc_ptr->p_usr.p_dcid 				= dcid;
//	proc_ptr->p_usr.p_lpid 				= PROC_NO_PID;
//	proc_ptr->p_usr.p_rts_flags 		= SLOT_FREE;
//	proc_ptr->p_usr.p_misc_flags 		= GENERIC_PROC;
//	proc_ptr->p_usr.p_nodeid 			= (-1);			/* OLD: atomic_read(&local_nodeid); */
//	proc_ptr->p_usr.p_nodemap 			= 0;
//
//	proc_ptr->p_rcode 					= OK;	
//		
//	memset(&proc_ptr->p_message,0,sizeof(message));
//	proc_ptr->p_umsg	= NULL;
//
//	proc_ptr->p_usr.p_getfrom			= NONE;
//	proc_ptr->p_usr.p_sendto			= NONE;
//	proc_ptr->p_usr.p_waitmigr			= NONE;
//	proc_ptr->p_usr.p_proxy				= NONE;
//
//	/* Sets the process CPU affinity */
//	cpumask_setall(&proc_ptr->p_usr.p_cpumask);
//	
//	init_waitqueue_head(&proc_ptr->p_wqhead);		/* Initialize the wait queue 		*/
//	proc_ptr->p_pseudosem 				= 0;				/* pseudo semaphore			*/
//
//	
//	INIT_LIST_HEAD(&proc_ptr->p_list);
//	INIT_LIST_HEAD(&proc_ptr->p_link);
//
//	INIT_LIST_HEAD(&proc_ptr->p_mlist);
//	INIT_LIST_HEAD(&proc_ptr->p_mlink);
//	
//	proc_ptr->p_rmtcmd.c_cmd	= CMD_NONE;
//	proc_ptr->p_rmtcmd.c_dcid	= dcid;
//	proc_ptr->p_rmtcmd.c_src 	= NONE;	
//	proc_ptr->p_rmtcmd.c_dst 	= NONE;
//	proc_ptr->p_rmtcmd.c_snode 	= LOCALNODE;	
//	proc_ptr->p_rmtcmd.c_dnode	= LOCALNODE;
//	proc_ptr->p_rmtcmd.c_len	= 0;
//	proc_ptr->p_rmtcmd.c_rcode	= OK;
//
//	proc_ptr->p_rmtcmd.c_vcopy.v_src	= NONE;
//	proc_ptr->p_rmtcmd.c_vcopy.v_dst	= NONE;
//	proc_ptr->p_rmtcmd.c_vcopy.v_rqtr	= NONE;
//	proc_ptr->p_rmtcmd.c_vcopy.v_saddr	= NULL;
//	proc_ptr->p_rmtcmd.c_vcopy.v_daddr	= NULL;
//	proc_ptr->p_rmtcmd.c_vcopy.v_bytes	= 0;
//
//
//	proc_ptr->p_usr.p_lclsent		= 0;			/* counter of LOCAL sent messages	*/
//	proc_ptr->p_usr.p_rmtsent		= 0;			/* counter of REMOTE sent messages	*/
//
//	proc_ptr->p_name_ptr 		= NULL;				
//	strcpy(proc_ptr->p_usr.p_name, "$noname");
//
////	init_timer(&proc_ptr->p_timer);
//
//	proc_ptr->p_task = NULL; 
//
//#if MOLPROFILING != 0
//	for( j = 0; j < MAX_PROF; j++)
//		proc_ptr->p_profiling[j] = 0;
//#endif
//
//}
///*------------------------------------------------------*/
///*			inherit_cpu				*/
///*------------------------------------------------------*/
//void inherit_cpu(struct proc *proc_ptr)
//{
//	int cpuid, ret;
//	cpumask_t tmp_cpumask;
//	proc_usr_t *pu_ptr;	
//	
//	cpuid = get_cpu();
//	MOLDEBUG(INTERNAL, "cpuid=%d\n", cpuid );
//	cpumask_clear(&tmp_cpumask);
//	cpumask_set_cpu(cpuid, &tmp_cpumask);
//	if ( (ret = sched_setaffinity(proc_ptr->p_usr.p_lpid, &tmp_cpumask)))
//			ERROR_PRINT(ret);	
//	put_cpu();
//
//	pu_ptr = &proc_ptr->p_usr;
//	MOLDEBUG(INTERNAL, PROC_CPU_FORMAT, PROC_CPU_FIELDS(pu_ptr));
//
//}
//
///****************************************************************
// *  Copy a block of data from Userspace to Userspace	
//*****************************************************************/
//
//long copy_usr2usr(int source, struct proc *src_proc, char *src_addr, struct proc *dst_proc, char *dst_addr, int bytes)
//{
//	struct page *src_pg;
//	struct page *dst_pg;
//	void *saddr = NULL;
//	void *daddr = NULL;
//	unsigned long src_off;
//	unsigned long dst_off;
//	int 	src_npag, dst_npag;	/* number of pages the  copy implies*/
//	int ret = OK;
//	int len, slen, dlen;
//	message *m_ptr;
//
//MOLDEBUG(DBGPARAMS,"source=%d src_pid=%d dst_pid=%d bytes=%d\n", 
//		source, src_proc->p_usr.p_lpid, dst_proc->p_usr.p_lpid, bytes);
// 
//	if( bytes < 0 || bytes  > MAXCOPYLEN) ERROR_RETURN(EMOLRANGE);
//
//	src_off  = (long int) src_addr & (~PAGE_MASK);
//	dst_off  = (long int) dst_addr & (~PAGE_MASK);
//MOLDEBUG(INTERNAL,"src_off=%ld dst_off=%ld\n",src_off, dst_off);
//
///***************** SUPERFLOUS - CAN BE REMOVED ***********/
//	src_npag = (src_off+bytes+PAGE_SIZE-1)>>PAGE_SHIFT;
//	dst_npag = (dst_off+bytes+PAGE_SIZE-1)>>PAGE_SHIFT;
//MOLDEBUG(INTERNAL,"src_npag=%d dst_npag=%d\n",src_npag, dst_npag);
//
//	while( bytes > 0) {
//
//		down_read(&src_proc->p_task->mm->mmap_sem);
//		ret = get_user_pages(src_proc->p_task, src_proc->p_task->mm,
//         			(unsigned long)src_addr, 1, 1, 0, &src_pg, NULL);
//		up_read(&src_proc->p_task->mm->mmap_sem);
//		if (ret != 1) ERROR_RETURN(EMOLADDRNOTAVAIL);
//MOLDEBUG(INTERNAL,"get_user_pages SRC OK\n");
// 
//		down_read(&dst_proc->p_task->mm->mmap_sem);
//		ret = get_user_pages(dst_proc->p_task, dst_proc->p_task->mm,
//     	         	(unsigned long)dst_addr, 1, 1, 0, &dst_pg, NULL);
//		up_read(&dst_proc->p_task->mm->mmap_sem);
//		if (ret != 1) ERROR_RETURN(EMOLADDRNOTAVAIL);
//MOLDEBUG(INTERNAL,"get_user_pages DST OK\n");
//
//		saddr = kmap_atomic(src_pg, KM_USER0);
//MOLDEBUG(INTERNAL,"kmap_atomic SRC OK\n");
//		daddr = kmap_atomic(dst_pg, KM_USER0);
//MOLDEBUG(INTERNAL,"kmap_atomic DST OK\n");
//
//		slen = PAGE_SIZE-src_off;
//		dlen = PAGE_SIZE-dst_off;
//		len = min(slen,dlen);
//		len = min(len,bytes);
//
//		if( len == PAGE_SIZE) {
//	    		copy_page(daddr , saddr);
//MOLDEBUG(INTERNAL,"copy_page %d bytes\n", len);
//
//		}else{
//	    		memcpy((daddr + dst_off), (saddr + src_off), len);
//MOLDEBUG(INTERNAL,"memcpy %d bytes\n", len);
//		}
//
//		/*fill the message sender field */
//		if( (source != NONE) && (bytes == sizeof(message))) {
//MOLDEBUG(INTERNAL,"source=%d bytes=%d\n", source, bytes);
//			m_ptr = (message *) (daddr + dst_off);
//			m_ptr->m_source = source;
//		}
//
//		kunmap_atomic(saddr, KM_USER0);
//MOLDEBUG(INTERNAL,"kunmap_atomic SRC OK\n");
//		kunmap_atomic(daddr, KM_USER0);
//MOLDEBUG(INTERNAL,"kunmap_atomic DST OK\n");
//		set_page_dirty_lock(dst_pg);
//MOLDEBUG(INTERNAL,"set_page_dirty_lock OK\n");
//     	put_page(dst_pg);
//MOLDEBUG(GENERIC,"put_page DST OK\n");
//     	put_page(src_pg);
//MOLDEBUG(GENERIC,"put_page SRC OK\n");
//
//		src_addr+=len;
//		dst_addr+=len;		
//		src_off  = (long int) src_addr & (~PAGE_MASK);
//		dst_off  = (long int) dst_addr & (~PAGE_MASK);
//MOLDEBUG(INTERNAL,"src_off=%ld dst_off=%ld\n",src_off, dst_off);
//		bytes=bytes-len;
//	}
//
//return(OK);
//}
//
///****************************************************************
// *  Sleep process proc waiting for an event 
// *  On entry: process must be Write LOCKED
// *  On exit: process is Write LOCKED
// *  the result is in proc->p_rcode
//*****************************************************************/
//int sleep_proc(struct proc *proc, long timeout) 
//{
//	proc_usr_t *pu_ptr; 
//	int rcode = OK;
//	int ret = OK;
//
//	MOLDEBUG(INTERNAL,"timeout=%ld\n", timeout); 
//
//	if( timeout == TIMEOUT_NOWAIT) {
//		if( test_bit(MIS_BIT_NEEDMIGR, &proc->p_usr.p_misc_flags)){
//			clear_bit(MIS_BIT_NEEDMIGR, &proc->p_usr.p_misc_flags);
//			set_bit(BIT_MIGRATE, &proc->p_usr.p_rts_flags);
//		}
//		proc->p_rcode = EMOLAGAIN; 
//		return(EMOLAGAIN);
//	}
//	
//	MOLDEBUG(INTERNAL,"BEFORE DOWN lpid=%d p_sem=%d timeout=%ld\n",proc->p_usr.p_lpid,proc->p_pseudosem, timeout); 
//	proc->p_pseudosem = -1; 
//	WUNLOCK_PROC(proc); 	
//	MOLDEBUG(INTERNAL,"endpoint=%d flags=%lX\n",proc->p_usr.p_endpoint, proc->p_usr.p_rts_flags); 
//
//	if( timeout < 0) {
//		ret = wait_event_interruptible(proc->p_wqhead, (proc->p_pseudosem >= 0));
//	} else {
//		ret = wait_event_interruptible_timeout(proc->p_wqhead, 
//			(proc->p_pseudosem >= 0),msecs_to_jiffies(timeout));
//		if(ret > 0) ret = OK;
//		else if (ret == 0) ret = EMOLTIMEDOUT;
//		else ret = (-ERESTARTSYS);
//	}
//	MOLDEBUG(INTERNAL,"endpoint=%d flags=%lX cpuid=%d\n",proc->p_usr.p_endpoint, proc->p_usr.p_rts_flags, smp_processor_id());  
//	WLOCK_PROC(proc); 
//	
//	if( ret) {
//	MOLDEBUG(INTERNAL,"pid=%d ret=%d\n",task_pid_nr(current), ret);  
//		if(proc->p_pseudosem < 0) proc->p_pseudosem++; 
////		del_timer_sync(&proc->p_timer);
//		proc->p_rcode = ret; 
//		if( timeout < 0) {
//			while(test_bit(BIT_ONCOPY, &proc->p_usr.p_rts_flags)) {
//				WUNLOCK_PROC(proc);
//				schedule();
//				WLOCK_PROC(proc);
//			}	
//		}
//	}
//
//	if( test_bit(MIS_BIT_NEEDMIGR, &proc->p_usr.p_misc_flags)){
//		clear_bit(MIS_BIT_NEEDMIGR, &proc->p_usr.p_misc_flags);
//		set_bit(BIT_MIGRATE, &proc->p_usr.p_rts_flags);
//	}
//
//	/* reset the process CPU mask */
//	if ( (rcode = sched_setaffinity(proc->p_usr.p_lpid, &proc->p_usr.p_cpumask)))
//			ERROR_PRINT(rcode);	
//
//	pu_ptr = &proc->p_usr;
//	MOLDEBUG(INTERNAL, PROC_CPU_FORMAT, PROC_CPU_FIELDS(pu_ptr));
//	
//	MOLDEBUG(INTERNAL,"someone wakeups me: sem=%d p_rcode=%d\n",proc->p_pseudosem, proc->p_rcode);
//	return(ret);
//}
//
///****************************************************************
// *  Sleep process proc waiting for an event 
// *  On entry: processes proc and other must be Write LOCKED
// *  On exit: process is Write LOCKED
// *  the result is in proc->p_rcode
//*****************************************************************/
//int sleep_proc2(struct proc *proc, struct proc *other , long timeout)  
//{
//	proc_usr_t *pu_ptr; 
//	int ret = OK;
//	int rcode = OK;
//
//	if( timeout == TIMEOUT_NOWAIT) {
//		if( test_bit(MIS_BIT_NEEDMIGR, &proc->p_usr.p_misc_flags)){
//			clear_bit(MIS_BIT_NEEDMIGR, &proc->p_usr.p_misc_flags);
//			set_bit(BIT_MIGRATE, &proc->p_usr.p_rts_flags);
//		}
//		proc->p_rcode = EMOLAGAIN; 
//		return(EMOLAGAIN);
//	}
//	
//MOLDEBUG(INTERNAL,"BEFORE DOWN lpid=%d p_sem=%d timeout=%ld\n",proc->p_usr.p_lpid, proc->p_pseudosem, timeout); 
//	proc->p_pseudosem = -1; 
//	WUNLOCK_PROC2(proc, other); 
//MOLDEBUG(INTERNAL,"endpoint=%d flags=%lX\n",proc->p_usr.p_endpoint, proc->p_usr.p_rts_flags); 
//
//	if( timeout < 0) {
//		ret = wait_event_interruptible(proc->p_wqhead, (proc->p_pseudosem >= 0));
//	} else {
//		ret = wait_event_interruptible_timeout(proc->p_wqhead, 
//			(proc->p_pseudosem >= 0),msecs_to_jiffies(timeout));
//		if(ret > 0) ret = OK;
//		else if (ret == 0) ret = EMOLTIMEDOUT;
//		else ret = (-ERESTARTSYS);
//	}
//
//	MOLDEBUG(INTERNAL,"endpoint=%d flags=%lX cpuid=%d\n",proc->p_usr.p_endpoint, proc->p_usr.p_rts_flags, smp_processor_id());  
//	WLOCK_PROC2(proc, other); 
//	
//	if( ret) {
//MOLDEBUG(INTERNAL,"pid=%d ret=%d\n",task_pid_nr(current), ret);  
//		if(proc->p_pseudosem < 0) proc->p_pseudosem++; 
////		del_timer_sync(&proc->p_timer);
//		proc->p_rcode = ret; 
//		if( timeout < 0) {
//			while(test_bit(BIT_ONCOPY, &proc->p_usr.p_rts_flags)) {
//				WUNLOCK_PROC2(proc, other);
//				schedule();
//				WLOCK_PROC2(proc, other);
//			}	
//		}
//	}
//
//	if( test_bit(MIS_BIT_NEEDMIGR, &proc->p_usr.p_misc_flags)){
//		clear_bit(MIS_BIT_NEEDMIGR, &proc->p_usr.p_misc_flags);
//		set_bit(BIT_MIGRATE, &proc->p_usr.p_rts_flags);
//	}
//	
//	/* reset the process CPU mask */
//	if ( (rcode = sched_setaffinity(proc->p_usr.p_lpid, &proc->p_usr.p_cpumask)))
//			ERROR_PRINT(rcode);	
//			
//	pu_ptr = &proc->p_usr;
//	MOLDEBUG(INTERNAL, PROC_CPU_FORMAT, PROC_CPU_FIELDS(pu_ptr));
//	
//MOLDEBUG(INTERNAL,"someone wakeups me: sem=%d p_rcode=%d\n",proc->p_pseudosem, proc->p_rcode);
//	return(ret);
//}
//
///****************************************************************
// *  Sleep process proc waiting for an event 
// *  On entry: processes proc and other must be Write LOCKED
// *  On exit: process is Write LOCKED
// *  the result is in proc->p_rcode
//*****************************************************************/
//int sleep_proc3(struct proc *proc, struct proc *other1, struct proc *other2 , long timeout) 
//{
//	proc_usr_t *pu_ptr; 
//	int ret = OK;
//	int rcode = OK;
//
//	if( timeout == TIMEOUT_NOWAIT) {
//		if( test_bit(MIS_BIT_NEEDMIGR, &proc->p_usr.p_misc_flags)){
//			clear_bit(MIS_BIT_NEEDMIGR, &proc->p_usr.p_misc_flags);
//			set_bit(BIT_MIGRATE, &proc->p_usr.p_rts_flags);
//		}
//		proc->p_rcode = EMOLAGAIN; 
//		return(EMOLAGAIN);
//	}
//	
//	proc->p_rcode = OK; 
//MOLDEBUG(INTERNAL,"BEFORE DOWN lpid=%d p_sem=%d\n",proc->p_usr.p_lpid,proc->p_pseudosem); 
//	proc->p_pseudosem = -1; 
//	WUNLOCK_PROC3(proc, other1, other2); 
//MOLDEBUG(INTERNAL,"endpoint=%d flags=%lX\n",proc->p_usr.p_endpoint, proc->p_usr.p_rts_flags);  
//	if( timeout < 0) {
//		ret = wait_event_interruptible(proc->p_wqhead, (proc->p_pseudosem >= 0));
//	} else {
//		ret = wait_event_interruptible_timeout(proc->p_wqhead, 
//			(proc->p_pseudosem >= 0),msecs_to_jiffies(timeout));
//		if(ret > 0) ret = OK;
//		else if (ret == 0) ret = EMOLTIMEDOUT;
//		else ret = (-ERESTARTSYS);
//	}
//	MOLDEBUG(INTERNAL,"endpoint=%d flags=%lX cpuid=%d\n",proc->p_usr.p_endpoint, proc->p_usr.p_rts_flags, smp_processor_id());  
//	WLOCK_PROC3(proc, other1, other2); 
//	if( ret) {
//MOLDEBUG(INTERNAL,"pid=%d ret=%d\n",task_pid_nr(current), ret);  
//		if(proc->p_pseudosem < 0) proc->p_pseudosem = 0; 
////		del_timer_sync(&proc->p_timer);
//		proc->p_rcode = ret; 
//		while(test_bit(BIT_ONCOPY, &proc->p_usr.p_rts_flags)) {
//			WUNLOCK_PROC3(proc, other1, other2);
//			schedule();
//			WLOCK_PROC3(proc, other1, other2);
//		}
//	}
//
//	if( test_bit(MIS_BIT_NEEDMIGR, &proc->p_usr.p_misc_flags)){
//		clear_bit(MIS_BIT_NEEDMIGR, &proc->p_usr.p_misc_flags);
//		set_bit(BIT_MIGRATE, &proc->p_usr.p_rts_flags);
//	}
//	
//	/* reset the process CPU mask */
//	if ( (rcode = sched_setaffinity(proc->p_usr.p_lpid, &proc->p_usr.p_cpumask)))
//			ERROR_PRINT(rcode);	
//			
//	pu_ptr = &proc->p_usr;
//	MOLDEBUG(INTERNAL, PROC_CPU_FORMAT, PROC_CPU_FIELDS(pu_ptr));
//	
//	MOLDEBUG(INTERNAL,"someone wakeups me: sem=%d p_rcode=%d\n",proc->p_pseudosem, proc->p_rcode);
//	return(ret);
//}
//
//
///*===========================================================================*
// *				bm2ascii		 			   				     *
// *===========================================================================*/
//void bm2ascii(char *buf, unsigned long int bitmap)
//{
//	int i;
//	unsigned long int mask;
//
//	mask = 0x80000000; 
//	for( i = 0; i < BITMAP_BITS ; i++) {
//		*buf++ = (bitmap & mask)?'X':'-';
//		mask =  (mask >> 1);		
//	}
//	*buf = '\0';
//}
//
///*===========================================================================*
// *				clear_node		 		     *
// *===========================================================================*/
//void clear_node(cluster_node_t *n_ptr)
//{
//	strcpy(n_ptr->n_usr.n_name,"NONAME");
//	n_ptr->n_usr.n_flags 	= NODE_FREE;
//	n_ptr->n_usr.n_dcs   	= 0;
//	n_ptr->n_usr.n_proxies 	= NO_PROXIES;
//	n_ptr->n_usr.n_stimestamp.tv_sec = 0;
//	n_ptr->n_usr.n_stimestamp.tv_nsec = 0;
//	n_ptr->n_usr.n_rtimestamp.tv_sec = 0;
//	n_ptr->n_usr.n_rtimestamp.tv_nsec = 0;
//}
///*===========================================================================*
// *				init_node		 		     *
// *===========================================================================*/
//void init_node(int nodeid)
//{
//	cluster_node_t *n_ptr;
//
//	n_ptr = &node[nodeid];
//	n_ptr->n_usr.n_nodeid 	= nodeid;
//
//	clear_node(n_ptr);
//
//#if LOCK_NODE_TYPE == USE_NODE_RWLOCK
//	rwlock_init(&(n_ptr->n_rwlock));
//#else	/* USE_PROC_MUTEX*/
//	mutex_init(&(n_ptr->n_mutex));
//#endif
//
//}
//
///*===========================================================================*
// *				clear_proxies		 					     *
// *===========================================================================*/
//void clear_proxies(proxies_t *px_ptr)
//{
//	struct proc *sproxy_ptr, *rproxy_ptr;
//
//	px_ptr->px_usr.px_flags = PROXIES_FREE;
//	sproxy_ptr = &px_ptr->px_sproxy;
//	rproxy_ptr = &px_ptr->px_rproxy;
//	init_proc_desc(sproxy_ptr, PROXY_NO_DC, px_ptr->px_usr.px_id);
//	init_proc_desc(rproxy_ptr, PROXY_NO_DC, px_ptr->px_usr.px_id);
//	sproxy_ptr->p_usr.p_rts_flags	= SLOT_FREE;
//	rproxy_ptr->p_usr.p_rts_flags	= SLOT_FREE;
//	strcpy(px_ptr->px_usr.px_name, "NONAME");
//
//}
//
///*===========================================================================*
// *				init_proxies		 		     *
// *===========================================================================*/
//void init_proxies(int px_nr)
//{
//	proxies_t *px_ptr;
//	struct proc *sproxy_ptr, *rproxy_ptr;
//	
//	px_ptr = &proxies[px_nr];
//	px_ptr->px_usr.px_id = px_nr;
//	sproxy_ptr = &px_ptr->px_sproxy;
//	rproxy_ptr = &px_ptr->px_rproxy;
//	clear_proxies(px_ptr);
//
//	sproxy_ptr->p_pseudosem 	= 1;
//	rproxy_ptr->p_pseudosem 	= 1;
//
//#if LOCK_PROXY_TYPE == USE_PROXY_RWLOCK
//	rwlock_init(&(px_ptr->px_rwlock));
//#else	/* USE_PROC_MUTEX*/
//	mutex_init(&(px_ptr->px_mutex));
//#endif
//
//
//#if LOCK_PROC_TYPE == USE_PROC_RWLOCK
//	rwlock_init(&(sproxy_ptr->p_rwlock));
//	rwlock_init(&(rproxy_ptr->p_rwlock));
//#else	/* USE_PROC_MUTEX*/
//	mutex_init(&(sproxy_ptr->p_mutex));
//	mutex_init(&(rproxy_ptr->p_mutex));
//#endif
//
//}
//
///*--------------------------------------------------------------*/
///*			kill_unbind				*/
///* src_proc->p_task is locked 					*/
///* DC is unlocked						*/
///* src_proc & dst_proc are unlocked 				*/
///*--------------------------------------------------------------*/
//long kill_unbind(struct proc *dst_ptr, struct proc *src_ptr)
//{
//	int dst_ep, src_ep, ret = OK;
//	message m, *m_ptr;
//
//	m_ptr = &m;
//	m_ptr->m_type   = MOLEXIT;
//	dst_ep = dst_ptr->p_usr.p_endpoint;
//	src_ep = src_ptr->p_usr.p_endpoint;
//MOLDEBUG(INTERNAL,"dst_ep=%d src_ep=%d\n",dst_ep, src_ep);
////	ret = kernel_sendrec(dst_ep, m_ptr);	
//
//	if(ret) ERROR_RETURN(ret);
//	return(ret);
//}
//
///*--------------------------------------------------------------*/
///*			flush_sending_procs			*/
///* The proxy has finished or the remote node is NOT CONNECTED 	*/
///* wakeup with error all process trying to send a CMD to REMOTE */
///* sproxy_ptr must be LOCKED	 				*/
///*--------------------------------------------------------------*/
//long flush_sending_procs(int nodeid,  struct proc *sproxy_ptr)
//{
//	struct proc *src_ptr, *tmp_ptr;
//
//MOLDEBUG(INTERNAL,"SPROXY wakeup with error all process trying to send a CMD to node=%d\n",  sproxy_ptr->p_usr.p_nodeid);
//
//	list_for_each_entry_safe(src_ptr, tmp_ptr, &sproxy_ptr->p_list, p_link) {
//		/* RULE TO LOCK: 1st: sender, 2nd: sender proxy */
//		WUNLOCK_PROC(sproxy_ptr);
//		WLOCK_PROC(src_ptr); 
//		if( src_ptr->p_rmtcmd.c_dnode != nodeid) {
//			WUNLOCK_PROC(src_ptr);
//		 	continue;
//		}
//		WLOCK_PROC(sproxy_ptr);
//
//		list_del(&src_ptr->p_link); /* remove from queue */
//		src_ptr->p_usr.p_proxy = NONE;
//		clear_bit(BIT_RMTOPER, &src_ptr->p_usr.p_rts_flags);
//
//MOLDEBUG(INTERNAL,"Find process %d trying to send a CMD\n",src_ptr->p_usr.p_endpoint);
//		if( IT_IS_REMOTE(src_ptr)) {	/* Acknowledges to remote process */		
//			src_ptr->p_usr.p_rts_flags = REMOTE; 
//			src_ptr->p_usr.p_getfrom = NONE;
//			src_ptr->p_usr.p_sendto = NONE;
//		} else {
//MOLDEBUG(INTERNAL,"Wakeup SENDER with error ep=%d  pid=%d\n",src_ptr->p_usr.p_endpoint, src_ptr->p_usr.p_lpid);	
//			switch(src_ptr->p_rmtcmd.c_cmd) {
//				case CMD_SNDREC_MSG:
//					clear_bit(BIT_RECEIVING, &src_ptr->p_usr.p_rts_flags);
//					src_ptr->p_usr.p_getfrom = NONE;
//				case CMD_SEND_MSG:
//				case CMD_NTFY_MSG:
//				case CMD_REPLY_MSG:
//					clear_bit(BIT_SENDING, &src_ptr->p_usr.p_rts_flags);
//					src_ptr->p_usr.p_sendto = NONE;
//					break;
//				case CMD_COPYIN_DATA:
//				case CMD_COPYIN_RQST:
//				case CMD_COPYLCL_RQST:
//				case CMD_COPYRMT_RQST:
//					if(test_bit(BIT_ONCOPY, &src_ptr->p_usr.p_rts_flags)){
//						/* Only the requester of a VCOPY CMD must be waked up */
//						if( src_ptr->p_usr.p_endpoint == src_ptr->p_rmtcmd.c_vcopy.v_rqtr) 
//							clear_bit(BIT_ONCOPY, &src_ptr->p_usr.p_rts_flags);
//					}
//					break;
//				default:
//					WUNLOCK_PROC(src_ptr); 
//					ERROR_RETURN(EMOLBADREQUEST);			
//					break;
//				}
//			if(src_ptr->p_usr.p_rts_flags == 0) 
//				LOCAL_PROC_UP(src_ptr, EMOLNOTCONN);
//		}
//		WUNLOCK_PROC(src_ptr); 
//	}
//	return(OK);
//}
//
///*--------------------------------------------------------------*/
///*			flush_receiving_procs			*/
///* A remote node is disconnected: wake up with error all 	*/
///* processes waiting some operation from processes in that node	*/
///* rproxy_ptr must be LOCKED	 				*/
///*--------------------------------------------------------------*/
//long flush_receiving_procs(int nodeid, struct proc *rproxy_ptr)
//{	
//	int v, i;
//	DC_desc_t *dc_ptr;
//	struct proc *src_ptr, *dst_ptr, *tmp_ptr;
//	proc_usr_t *pu_ptr;
//
//MOLDEBUG(INTERNAL,"RPROXY search for process of all DCs waiting an action from a remote process with the node dead\n");
//	for( v = 0; v < dvs.d_nr_dcs; v++) {
//		dc_ptr 	= &dc[v];
//
//		WLOCK_DC(dc_ptr);
//		if( dc_ptr->dc_usr.dc_flags != 0) {
//			WUNLOCK_DC(dc_ptr);
//			continue;
//		}
//
//		WUNLOCK_PROC(rproxy_ptr);
//		LOCK_ALL_PROCS(dc_ptr, tmp_ptr, i);
//		WLOCK_PROC(rproxy_ptr);
//
//		FOR_EACH_PROC(dc_ptr, i) {
//			tmp_ptr = DC_PROC(dc_ptr,i);
//
//			if( tmp_ptr->p_usr.p_rts_flags == SLOT_FREE) 	continue;
//			if( IT_IS_REMOTE(tmp_ptr))				continue;
//			tmp_ptr->p_rcode = EMOLNOTCONN;
//				
//			/*A local process is trying to receive a message from a remote process with the node dead */
//			do {
//				if( test_bit(BIT_RECEIVING, &tmp_ptr->p_usr.p_rts_flags)) {
//					if( (tmp_ptr->p_usr.p_getfrom != ANY) 
//						&&(tmp_ptr->p_usr.p_getfrom != NONE)){
//						src_ptr = ENDPOINT2PTR(dc_ptr, tmp_ptr->p_usr.p_getfrom);
//						if( nodeid == src_ptr->p_usr.p_nodeid) {
//							pu_ptr = &tmp_ptr->p_usr;
//							MOLDEBUG(DBGPROC,"Clean receiving " PROC_USR_FORMAT, PROC_USR_FIELDS(pu_ptr));
//							clear_bit(BIT_RECEIVING, &tmp_ptr->p_usr.p_rts_flags);
//							if( tmp_ptr->p_usr.p_rts_flags == PROC_RUNNING)
//								LOCAL_PROC_UP(tmp_ptr, EMOLNOTCONN);
//						}
//					}	
//				}
//			}while(0);
//				
//			/* A local process has sent a message to a remote process and waiting for the acknowledge but the node dead  */
//			do {
//				if( test_bit(BIT_SENDING, &tmp_ptr->p_usr.p_rts_flags)) {
//					if( (tmp_ptr->p_usr.p_sendto != ANY) 
//						&&(tmp_ptr->p_usr.p_sendto != NONE)){
//						dst_ptr = ENDPOINT2PTR(dc_ptr, tmp_ptr->p_usr.p_sendto);
//						if( nodeid == dst_ptr->p_usr.p_nodeid) {
//							pu_ptr = &tmp_ptr->p_usr;
//							MOLDEBUG(DBGPROC,"Clean sending " PROC_USR_FORMAT, PROC_USR_FIELDS(pu_ptr));
//							clear_bit(BIT_SENDING, &tmp_ptr->p_usr.p_rts_flags);
//							if( tmp_ptr->p_usr.p_rts_flags == PROC_RUNNING)
//								LOCAL_PROC_UP(tmp_ptr, EMOLNOTCONN);	
//						}
//					}	
//				}
//			}while(0);
//				
//			/* A local process has sent a COPY CMD to a remote process and waiting for the acknowledge but the node dead */
//			do {
//				if( test_bit(BIT_ONCOPY, &tmp_ptr->p_usr.p_rts_flags)) {
//					if(tmp_ptr->p_usr.p_endpoint == tmp_ptr->p_rmtcmd.c_vcopy.v_rqtr) {
//						if( nodeid == tmp_ptr->p_rmtcmd.c_dnode) {
//							pu_ptr = &tmp_ptr->p_usr;
//							MOLDEBUG(DBGPROC,"Clean oncopy " PROC_USR_FORMAT, PROC_USR_FIELDS(pu_ptr));
//							clear_bit(BIT_ONCOPY, &tmp_ptr->p_usr.p_rts_flags);
//							if( tmp_ptr->p_usr.p_rts_flags == PROC_RUNNING) 
//								LOCAL_PROC_UP(tmp_ptr, EMOLNOTCONN);
//						}	
//					}	
//				}
//			}while(0);			
//		}
//
//		WUNLOCK_PROC(rproxy_ptr);
//		UNLOCK_ALL_PROCS(dc_ptr, tmp_ptr, i);
//		WUNLOCK_DC(dc_ptr);
//		WLOCK_PROC(rproxy_ptr);
//	}
//	return(OK);
//}
//
//#ifdef MOLAUTOFORK
///*--------------------------------------------------------------*/
///*			fork_bind				*/
///* the parent of a process send a MOLGETPROCNR message to PM	*/
///* the PM returns the child_nr 					*/
///* the parent bind the child to the kernel			*/
///* the parent of a process send a MOLFORK message to PM		*/
///* On return, PM and SYSTASK have registered the child 		*/
///*--------------------------------------------------------------*/
//struct proc* fork_bind(struct proc *proc_ptr, int child_lpid)
//{
//	struct proc *warn_ptr, *child_ptr;
//	int src_ep, ret, dcid, warn_ep, child_ep, child_nr, child_pid;
//	message m, *m_ptr;
//	DC_desc_t *dc_ptr;
//
//MOLDEBUG(DBGLVL1,"parent_ep=%d child_lpid=%d \n",proc_ptr->p_usr.p_endpoint, child_lpid);
//
//	dcid = proc_ptr->p_usr.p_dcid;
//MOLDEBUG(DBGLVL1,"dcid=%d\n", dcid);
//	if( dcid < 0 || dcid >= dvs.d_nr_dcs) 	return(NULL);
//	dc_ptr 	= &dc[dcid];
//	if( dc_ptr->dc_usr.dc_flags)  		return(NULL);
//
//	/* Gets the endpoint of the binder   (i.e. PM) */
//	warn_ep = proc_ptr->p_priv.s_usr.s_warn;
//	if( warn_ep == NONE || warn_ep == ANY || warn_ep == SELF) 
//						return(NULL); 
//	warn_ptr = ENDPOINT2PTR(dc_ptr,warn_ep);
//	src_ep = proc_ptr->p_usr.p_endpoint;
//MOLDEBUG(DBGLVL1,"src_ep=%d warn_ep=%d \n",src_ep, warn_ep);
//
//	/* Request to PM the next proc number */
//	m_ptr = &m;
//	m_ptr->m_type  = MOLFREEPROC;
//	ret = kernel_sendrec(warn_ep, m_ptr);	
//	if(ret) 				return(NULL);
//	child_nr = m_ptr->PR_SLOT;
//MOLDEBUG(DBGLVL1,"child_nr=%d\n",child_nr);
//	if( child_nr < 0 || child_nr > dc_ptr->dc_usr.dc_nr_procs)
//						return(NULL);
//
//	child_ep = kernel_lclbind(dcid, child_lpid, child_nr);
//	if(child_ep < 0) 			return(NULL);
//MOLDEBUG(DBGLVL1,"child_ep=%d\n",child_ep);
//
//	/* BIND the child into PM */
//	m_ptr->m_type   = MOLFORK;
//	m_ptr->PR_PID   = child_lpid;	/* LINUX PID */
//	m_ptr->PR_SLOT  = child_nr;		/* PREVIOUS assigned  child_nr */		
//	m_ptr->PR_ENDPT = NONE;
//	ret = kernel_sendrec(warn_ep, m_ptr);	
//	if(ret) 				return(NULL);
//	if( m_ptr->PR_ENDPT == NONE)		return(NULL);
//	child_pid = m_ptr->PR_PID;		/* MINIX PID */
//	child_ep  = m_ptr->PR_ENDPT;
//
//	child_ptr = ENDPOINT2PTR(dc_ptr,child_ep);
//
//MOLDEBUG(DBGLVL1,"child_pid(minix)=%d child_ep=%d \n",child_pid, child_ep);
//
//	return(child_ptr);	
//}
//
//#endif /*MOLAUTOFORK */

/*--------------------------------------------------------------*/
/*		{lock,unlock}_tasklist				*/
/* these functions let interact with tasklist_lock symbol	*/
/* from MoL Module in kernel 2.6.18 which                       */
/* does not export it anymore                                   */
/*--------------------------------------------------------------*/

void lock_tasklist(void)
{
    read_lock(&tasklist_lock);
}

void unlock_tasklist(void)
{
    read_unlock(&tasklist_lock);
}

/**
 * This routine is just to be able to access sys_wait4()
 * without exporting it.
 */
asmlinkage long mm_sys_wait4(pid_t pid, int __user *stat_addr,
                                int options, struct rusage __user *ru)
{
    return sys_wait4(pid, stat_addr, options, ru);
}

/**
 * This routine is just to be able to access _put_task_struct()
 * without exporting it.
 */
void mm_put_task_struct(struct task_struct *t) 
{
    return put_task_struct(t);
}

/**
 * This routine is just to be able to access sched_setaffinity()
 * without exporting it.
 */
long mm_sched_setaffinity(pid_t pid, const struct cpumask *in_mask)
{
    return sched_setaffinity(pid, in_mask);
}

