/****************************************************************/
/* 				SLOTS				*/
/* Slots algorithm routines for interDCs SYSTASKs 		*/
/****************************************************************/

#include "systask.h"
#define TRUE 1

#define 	NO_NEXT_MBR 	NO_PRIMARY_MBR
#define 	NO_SLOT_OWNER 	NO_PRIMARY_MBR


/*===========================================================================*
 *				copy_to_mrg				     
 * Copy slot table and status variables to temporal storage				     
 *===========================================================================*/
 void copy_to_mrg(void)
 {
	int i;
	for(i = 0; i < (dc_ptr->dc_nr_tasks + dc_ptr->dc_nr_procs); i++)
		slot_merge[i] = slot[i];			
	bm_waitsts_mrg 	= bm_waitsts;
	bm_init_mrg		= bm_init;
}

/*===========================================================================*
 *				copy_from_mrg				     
 * Copy slot table and status variables from temporal storage				     
 *===========================================================================*/
 void copy_from_mrg(void)
 {
	int i;

	for(i = 0; i < (dc_ptr->dc_nr_tasks + dc_ptr->dc_nr_procs); i++)
		slot[i] = slot_merge[i];			
	bm_waitsts 	= bm_waitsts_mrg;
	bm_init		= bm_init_mrg;
}
/*===========================================================================*
 *				new_rqst_slots	
* A new member without slots request a minimal count of slots 
 *===========================================================================*/
 int new_rqst_slots(int nr_slots )
 {
	int rcode  = OK;
	TASKDEBUG("nr_slots=%d\n", nr_slots);
	if( bm_donors == 0 && owned_slots == 0){ 
		FSM_state = STS_REQ_SLOTS;
		rcode = mcast_rqst_slots(nr_slots);
	}
	return(rcode);
 }
 
/*===========================================================================*
 *				primary_sts_mcast	
* The primary mcast status to the next new member
 *===========================================================================*/
 int primary_sts_mcast(void)
 {
	int rcode  = OK;
	if (primary_mbr == local_nodeid ){
		waitsts_mbr = get_next_mbr(waitsts_mbr, bm_waitsts);
		if(waitsts_mbr != NO_NEXT_MBR){
			TASKDEBUG("waitsts_mbr=%d\n", waitsts_mbr);
			rcode = mcast_status_info(waitsts_mbr, SYS_PUT_STATUS,(char*) slot);
		}
	}
	return(rcode);	
 }

/*===========================================================================*
 *				get_init_nodes				     
 * get the Primary member from bitmap					     
 *===========================================================================*/
int get_init_nodes(void)
{
	int init_nr, i;

	init_nr = 0;
	for( i=0; i < dc_ptr->dc_nr_nodes; i++ ) {
		if( TEST_BIT(bm_init, i)){
			init_nr++;
		}
	}
	
	TASKDEBUG("init_nr=%d\n", init_nr);
	return(init_nr);
}
	
/*===========================================================================*
 *				get_primary_mbr				     
 * get the Primary member from bitmap					     
 *===========================================================================*/
int get_primary_mbr(void)
{
	int i, first_mbr;
	
	first_mbr = NO_PRIMARY_MBR;  /* to test for errors */
	for( i=0; i < dc_ptr->dc_nr_nodes; i++ ) {
		if ( TEST_BIT(bm_init, i) ){
			first_mbr = i;
			break;
		}
	}
	TASKDEBUG("bm_init=%X primary_mbr=%d\n", bm_init,  first_mbr );
	return(first_mbr);
}

/*===========================================================================*
 *				get_next_mbr				     
 * get the next member from initialized node bitmap					     
 *===========================================================================*/
int get_next_mbr(int node, unsigned int bm)
{
	int i, nx_mbr;
	
	TASKDEBUG("node=%d bm=%X\n", node, bm);
	if( bm == 0) return(NO_NEXT_MBR);
	if( node == NO_NEXT_MBR) node = 0;
	assert( node >= 0 && node < dc_ptr->dc_nr_nodes);

	for(i = 0, nx_mbr = node; i < dc_ptr->dc_nr_nodes; i++) {
		nx_mbr = (nx_mbr + 1) % dc_ptr->dc_nr_nodes;
		if ( TEST_BIT(bm, nx_mbr) ){
			TASKDEBUG("bm=%X node=%d nx_mbr=%d\n", bm, node, nx_mbr);
			return(nx_mbr);
		}
	}
	TASKDEBUG("bm=%X node=%d nx_mbr=%d\n", bm, node, nx_mbr);
	return(NO_NEXT_MBR);
}

/*===========================================================================*
 *				get_nodeid				     
 * It converts the string provided by SPREAD into a node ID		      
 *===========================================================================*/
int get_nodeid(char *mbr_string)
{
	char *s_ptr;
	int nid;

	s_ptr = strchr(mbr_string, '.'); /* locate the dot character */
	if(s_ptr != NULL)
		*s_ptr = '\0';
	s_ptr = mbr_string;
	s_ptr++;	/* first character of node */
	nid = atoi(s_ptr);
	TASKDEBUG("member=%s nodeid=%d\n", mbr_string,  nid );
	return(nid);
}
	
/*===========================================================================*
 *				mcast_rqst_slots					   
 * It builds and broadcasts a message requesting slots
 *===========================================================================*/
int mcast_rqst_slots(int nr_slots)
{
	message  msg;

	TASKDEBUG("nr_slots=%d\n", nr_slots);

	/* Ignore the send, a VIEW CHANGE is ongoing */
	if( bm_waitsts != 0 )						return(OK);
	if( TEST_BIT(FSM_state,BIT_MERGING))		return(OK);
	
	/* set donors bitmap including local_nodeid  to check 	*/
	/* if this message arrives to all initialized members 		*/
	bm_donors = bm_init;
	CLR_BIT( bm_donors , local_nodeid);
	if( bm_donors == 0) 						return(OK);
	
	msg.m_source= local_nodeid;
	msg.m_type 	= SYS_REQ_SLOTS;
	msg.m_need_slots = nr_slots;
	msg.m_free_slots 	= free_slots;
	msg.m_owned_slots	= owned_slots;
	msg.m1_p1 = (char *) NULL;
	msg.m1_p2 = (char *) NULL;		
	msg.m1_p3 = (char *) NULL;
	return(SP_multicast (sysmbox, SAFE_MESS, (char *) dc_ptr->dc_name,  
			SYS_REQ_SLOTS  , sizeof(message), (char *) &msg)); 
}

/*===========================================================================*
 *				mcast_binds2rmt					*
 * LOCAL system task has joined the DC group and broadcast local proc info	*
 * to the other nodes for remote binding							*
 *===========================================================================*/
int mcast_binds2rmt(void)
{
	message *spout_ptr; 		/* for use with SPREAD */
	proc_usr_t *proc_ptr;
	int i, rcode= OK;

	TASKDEBUG("\n");
	spout_ptr = (message *)&mess_out;

	/* Broadcast local system processes to be bound by the new node */
	for( i = 0; i < dc_ptr->dc_nr_sysprocs; i++) {
#ifdef ALLOC_LOCAL_TABLE 			
		proc_ptr = &proc[i];
		rcode  = mnx_getprocinfo(dcu.dc_dcid, (i-dc_ptr->dc_nr_tasks), proc_ptr);
		if( rcode < 0) ERROR_RETURN(rcode);	
#else /* ALLOC_LOCAL_TABLE */			
		proc_ptr = (proc_usr_t *) PROC_MAPPED((i-dc_ptr->dc_nr_tasks));	
		rcode = OK;		
#endif /* ALLOC_LOCAL_TABLE */		
		if( proc_ptr->p_rts_flags != SLOT_FREE){
			if( proc_ptr->p_nodeid == local_nodeid) {
				mcast_bind_proc(proc_ptr);
			}
		}
	}
	return(rcode);
}

/*===========================================================================*
 *				mcast_exit_proc								*
 * System task inform other nodes about a terminated local system process 		*
  *===========================================================================*/
int mcast_exit_proc(	proc_usr_t *proc_ptr)
{
	message *spout_ptr; 		/* for use with SPREAD */
	int rcode = OK; 
	
	TASKDEBUG(PROC_USR_FORMAT, PROC_USR_FIELDS(proc_ptr));

	if( init_nodes == 1) return(OK);
	
	spout_ptr = (message *)&mess_out;
	pthread_mutex_lock(&sys_mutex);	
	spout_ptr->m_source 	= local_nodeid;
	spout_ptr->m_type 		= MOLEXIT;
	spout_ptr->M3_P_NR		= proc_ptr->p_nr;
	spout_ptr->M3_ENDPT	 	= proc_ptr->p_endpoint;
	rcode = SP_multicast (sysmbox, SAFE_MESS, (char *) dc_ptr->dc_name,  
				SYS_SYSCALL  , sizeof(message), (char *) spout_ptr);
	pthread_cond_wait(&sys_syscall,&sys_mutex); /* unlock, wait, and lock again sys_mutex */
	pthread_mutex_unlock(&sys_mutex);
	
	return(rcode);
}
	
/*===========================================================================*
 *				mcast_fork_proc								*
 * System task inform other nodes about a fork for a local system process 		*
  *===========================================================================*/
int mcast_fork_proc(	proc_usr_t *proc_ptr)
{
	message *spout_ptr; 		/* for use with SPREAD */
	int rcode = OK; 
	
	TASKDEBUG(PROC_USR_FORMAT, PROC_USR_FIELDS(proc_ptr));
	if( init_nodes == 1) return(OK);

	spout_ptr = (message *)&mess_out;
	pthread_mutex_lock(&sys_mutex);	
	spout_ptr->m_source 	= local_nodeid;
	spout_ptr->m_type 		= MOLFORK;
	spout_ptr->M3_P_NR		= proc_ptr->p_nr;
	spout_ptr->M3_ENDPT	 	= proc_ptr->p_endpoint;
	strncpy(spout_ptr->M3_NAME, proc_ptr->p_name, (M3_STRING-1));
	rcode = SP_multicast (sysmbox, SAFE_MESS, (char *) dc_ptr->dc_name,  
				SYS_SYSCALL  , sizeof(message), (char *) spout_ptr);
	pthread_cond_wait(&sys_syscall,&sys_mutex); /* unlock, wait, and lock again sys_mutex */
	pthread_mutex_unlock(&sys_mutex);
	
	return(rcode);
}

/*===========================================================================*
 *				mcast_bind_proc								*
 * System task inform other nodes about a bind for a local system process 		*
  *===========================================================================*/
int mcast_bind_proc(proc_usr_t *proc_ptr)
{
	message *spout_ptr; 		/* for use with SPREAD */
	int rcode = OK; 
	
	TASKDEBUG(PROC_USR_FORMAT, PROC_USR_FIELDS(proc_ptr));
	if( init_nodes == 1) return(OK);

	spout_ptr = (message *)&mess_out;
	pthread_mutex_lock(&sys_mutex);	
	spout_ptr->m_source 	= local_nodeid;
	spout_ptr->m_type 		= MOLBINDPROC;
	spout_ptr->M3_P_NR		= proc_ptr->p_nr;
	spout_ptr->M3_ENDPT	 	= proc_ptr->p_endpoint;
	strncpy(spout_ptr->M3_NAME, proc_ptr->p_name, (M3_STRING-1));
	rcode = SP_multicast (sysmbox, SAFE_MESS, (char *) dc_ptr->dc_name,  
				SYS_SYSCALL  , sizeof(message), (char *) spout_ptr);
	pthread_cond_wait(&sys_syscall,&sys_mutex); /* unlock, wait, and lock again sys_mutex */
	pthread_mutex_unlock(&sys_mutex);
	
	return(rcode);
}

/*===========================================================================*
 *				mcast_migr_proc								*
 * System task inform other nodes about a process migration to a local system process 		*
  *===========================================================================*/
int mcast_migr_proc(	proc_usr_t *proc_ptr)
{
	message *spout_ptr; 		/* for use with SPREAD */
	int rcode = OK; 
	
	TASKDEBUG(PROC_USR_FORMAT, PROC_USR_FIELDS(proc_ptr));
	if( init_nodes == 1) return(OK);

	spout_ptr = (message *)&mess_out;
	pthread_mutex_lock(&sys_mutex);	
	spout_ptr->m_source 	= local_nodeid;
	spout_ptr->m_type 		= MOLMIGRPROC;
	spout_ptr->M3_P_NR		= proc_ptr->p_nr;
	spout_ptr->M3_ENDPT		= proc_ptr->p_endpoint;
	rcode = SP_multicast (sysmbox, SAFE_MESS, (char *) dc_ptr->dc_name,  
				SYS_SYSCALL  , sizeof(message), (char *) spout_ptr);
	pthread_cond_wait(&sys_syscall,&sys_mutex); /* unlock, wait, and lock again sys_mutex */
	pthread_mutex_unlock(&sys_mutex);
	
	return(rcode);
}

/*===========================================================================*
 *				mcast_exec_proc								*
 * System task inform other nodes about a exec for a local system process 		*
  *===========================================================================*/
int mcast_exec_proc(	proc_usr_t *proc_ptr)
{
	message *spout_ptr; 		/* for use with SPREAD */
	int rcode = OK; 
	
	TASKDEBUG(PROC_USR_FORMAT, PROC_USR_FIELDS(proc_ptr));
	if( init_nodes == 1) return(OK);

	spout_ptr = (message *)&mess_out;
	pthread_mutex_lock(&sys_mutex);	
	spout_ptr->m_source 	= local_nodeid;
	spout_ptr->m_type 		= MOLEXEC;
	spout_ptr->M3_P_NR		= proc_ptr->p_nr;
	spout_ptr->M3_ENDPT	 	= proc_ptr->p_endpoint;
	strncpy(spout_ptr->M3_NAME, proc_ptr->p_name, (M3_STRING-1));
	rcode = SP_multicast (sysmbox, SAFE_MESS, (char *) dc_ptr->dc_name,  
				SYS_SYSCALL  , sizeof(message), (char *) spout_ptr);
	pthread_cond_wait(&sys_syscall,&sys_mutex); /* unlock, wait, and lock again sys_mutex */
	pthread_mutex_unlock(&sys_mutex);
	
	return(rcode);
}
	
/*===========================================================================*
 *				init_global_vars
 *===========================================================================*/
int init_global_vars()
{
	FSM_state 	= STS_NEW;	
	primary_mbr = NO_PRIMARY_MBR;
	init_nodes 	= 0;
	waitsts_mbr= 0;
	
	bm_init 	= 0;	/* which nodes are INITIALIZED 			Replicated	*/
	bm_active	= 0;	/* which nodes are ACTIVE - 			Replicated	*/
	bm_waitsts	= 0;	/* which nodes wait for SYS_PUT_STATUS 	Replicated 	*/

	bm_donors 	= 0;	/* from which nodes the local node waits for donations - LOCAL */
	bm_pending	= 0;	/* to which nodes the local node have pending donations replies with slots=0 - LOCAL */
}

/*===========================================================================*
 *				init_slots				     
 * It connects SLOTS thread to the SPREAD daemon and initilize several local
 * and replicated variables
 *===========================================================================*/
int init_slots(void)
{
	int rcode, i;
#ifdef SPREAD_VERSION
    int     mver, miver, pver;
#endif
    sp_time test_timeout;

    test_timeout.sec = SLOT_TIMEOUT_SEC;
    test_timeout.usec = SLOT_TIMEOUT_MSEC;

	TASKDEBUG("group name=%s\n", dc_ptr->dc_name);
	TASKDEBUG("user  name=%d.%d\n", local_nodeid,dc_ptr->dc_dcid);

#ifdef SPREAD_VERSION
    rcode = SP_version( &mver, &miver, &pver);
	if(!rcode)
        {
		SP_error (rcode);
	  	SYSERR("main: Illegal variables passed to SP_version()\n");
	  	ERROR_EXIT(rcode);
	}
	TASKDEBUG("Spread library version is %d.%d.%d\n", mver, miver, pver);
#else
    TASKDEBUG("Spread library version is %1.2f\n", SP_version() );
#endif
	/*------------------------------------------------------------------------------------
	* User:  it must be unique in the spread node.
	*  local_nodeid.dcid
	*--------------------------------------------------------------------------------------*/

	sprintf( Spread_name, "4803");
	sprintf( User, "%d.%d", local_nodeid, dc_ptr->dc_dcid);
	rcode = SP_connect_timeout( Spread_name, User , 0, 1, 
				&sysmbox, Private_group, test_timeout );
	if( rcode != ACCEPT_SESSION ) 	{
		SP_error (rcode);
		ERROR_EXIT(rcode);
	}
	TASKDEBUG("User %s: connected to %s with private group %s\n",
			User , Spread_name, Private_group);

	/*------------------------------------------------------------------------------------
	* Group name: dc_name
	*--------------------------------------------------------------------------------------*/
	rcode = SP_join( sysmbox, dc_ptr->dc_name);
	if( rcode){
		SP_error (rcode);
 		ERROR_EXIT(rcode);
	}

	init_global_vars();

	return(OK);
}

/*===========================================================================*
*				mcast_status_info
* Send Global status info to new member
*===========================================================================*/
int mcast_status_info(int dest, int mtype,  char *slot_ptr)
{
	message *m_ptr;
	proc_usr_t *proc_ptr;
	int i, rcode= OK;
	
	m_ptr = (message *)sts_ptr;
	m_ptr->m_source 	= local_nodeid;
	m_ptr->m_dest	 	= dest;
	m_ptr->m_type 		= mtype;
	m_ptr->m_bm_init 	= bm_init;
	m_ptr->m_bm_waitsts = bm_waitsts;
	
	for( i = 0; i < dc_ptr->dc_nr_sysprocs; i++) {
#ifdef ALLOC_LOCAL_TABLE 			
		proc_ptr = &proc[i];
		rcode  = mnx_getprocinfo(dcu.dc_dcid, (i-dc_ptr->dc_nr_tasks), proc_ptr);
		if( rcode < 0) ERROR_RETURN(rcode);	
#else /* ALLOC_LOCAL_TABLE */			
		proc_ptr = (proc_usr_t *) PROC_MAPPED((i-dc_ptr->dc_nr_tasks));	
		rcode = OK;		
#endif /* ALLOC_LOCAL_TABLE */		
		if( proc_ptr->p_rts_flags != SLOT_FREE){
			slot[i].s_owner    = proc_ptr->p_nodeid;
			slot[i].s_endpoint = proc_ptr->p_endpoint;				
		}else{
			slot[i].s_owner    = NO_PRIMARY_MBR;
			slot[i].s_endpoint = NONE;
		}
	}

	/* Build Global Status Information (message + dc_info + replicated slot table */
	memcpy( rst_ptr, 
		(char*) slot_ptr, 
		(sizeof(slot_t) * ((dcu.dc_nr_procs+dcu.dc_nr_tasks))));
		
	/* Send the Global status info to new members */
	TASKDEBUG( MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	TASKDEBUG("Send Global sts_size=%d\n", sts_size);
	rcode = SP_multicast (sysmbox, SAFE_MESS, 
				(char *) dc_ptr->dc_name,
				mtype,  
				sts_size,
				(char*)sts_ptr);
	if(rcode <0) ERROR_RETURN(rcode);
	return(rcode);
}


/*===========================================================================*
 *				sp_join											 *
 * A NEW member has joint the DC group but it is not initialized
 *===========================================================================*/
int sp_join(int new_mbr)
{
	int i, rcode;
	proc_usr_t *proc_ptr;

	TASKDEBUG("FSM_state=%X new_member=%d primary_mbr=%d active_nodes=%d bm_active=%X\n", 
		FSM_state, new_mbr, primary_mbr, active_nodes, bm_active);
	
	if( new_mbr == local_nodeid){		/* The own JOIN message	*/
		if (active_nodes == 1){ 		/* It is a LONELY member*/

			/* it is ready to start running */
			FSM_state = STS_RUNNING;

			/* it is the Primary Member 	*/
			primary_mbr=local_nodeid;

			/* add the node to the initilized nodes bitmap */
			SET_BIT(bm_init, local_nodeid);
			init_nodes = 1;

			/* allocate all slots to the member */
			owned_slots = total_slots;		
			for (i = 0; i < (dc_ptr->dc_nr_tasks + dc_ptr->dc_nr_procs); i++) {
				slot[i].s_owner = local_nodeid;
			}
	
			/* Wake up SYSTASK */
			TASKDEBUG("Wake up systask: new_mbr=%d\n", new_mbr);
			pthread_cond_signal(&sys_barrier);	/* Wakeup SYSTASK 	*/
			return(OK);
		}else{
			/* Waiting Global status info */
			SET_BIT(bm_waitsts, local_nodeid);
			FSM_state = STS_WAIT_STATUS;	
		}
	}else{ /* Other node JOINs the group	*/
		TASKDEBUG("member=%d FSM_state=%X\n", local_nodeid,FSM_state);

		/* Sets the bm_waitsts bitmap to signal which new member need to get STATUS from PRIMARY  		*/
		/* All members setup the bm_waitsts bitmap because in the future they could be the primary_mbr	*/
		/* Not initialized members also setup the bitmap because another member JOIN could arrives		*/
		/* before primary's SYS_PUT_STATUS message (and the primary don't consider in its multicasted	*/
		/* bm_waitsts bitmap (concurrent JOIN)															*/
		SET_BIT(bm_waitsts, new_mbr);

		/* concurrent JOIN: Another JOIN has been receipt by the waiting status member 	*/
		/* before it receives the STS_WAIT_STATUS from the primary_mbr					*/
		if(	FSM_state == STS_WAIT_STATUS){
			TASKDEBUG("Concurrent JOIN: local_nodeid=%d new_mbr=%d bm_waitsts=%X\n", 
				local_nodeid, new_mbr, bm_waitsts);		
			return(OK);	
		} 
	
		/* if the new member was previously considered as a member of other    */
		/* partition but really had crashed, allocate its slots to primary_mbr */
		for (i = 0; i < (dc_ptr->dc_nr_tasks + dc_ptr->dc_nr_procs); i++) {
			/* SYSTEM level processes */
			if( i < dc_ptr->dc_nr_sysprocs){
				if ( (slot[i].s_owner == new_mbr)
					&& 	(TEST_BIT(slot[i].s_flags, BIT_SLOT_PARTITION) )) {	
						TASKDEBUG("Member %d previously considered as a member of other partition\n"
							, new_mbr);
#ifdef ALLOC_LOCAL_TABLE 			
						proc_ptr = &proc[i];
						rcode  = mnx_getprocinfo(dcu.dc_dcid, (i-dc_ptr->dc_nr_tasks), proc_ptr);
						if( rcode < 0) ERROR_RETURN(rcode );
#else /* ALLOC_LOCAL_TABLE */			
						proc_ptr = (proc_usr_t *) PROC_MAPPED((i-dc_ptr->dc_nr_tasks));	
						rcode = OK;						
#endif /* ALLOC_LOCAL_TABLE */						
						rcode = unbind_rmt_sysproc(proc_ptr->p_endpoint);
						if(rcode != OK)  ERROR_RETURN(rcode);
				}
				continue;
			}
			/* USER level processes */
			if ( (slot[i].s_owner == new_mbr)
				&& 	(TEST_BIT(slot[i].s_flags, BIT_SLOT_PARTITION) )) {
				TASKDEBUG("Member %d previously considered as a member of other partition\n"
						, new_mbr);					
				if (primary_mbr == local_nodeid){
					free_slots++;
					owned_slots++;
				}
				total_slots++;
				slot[i].s_owner = primary_mbr;
				slot[i].s_flags = 0;
				TASKDEBUG("Restoring slot %d from member %d considered alive in other partition\n",
					i, new_mbr);
			}
		}
		
		/* This node has sent an SYS_REQ_SLOTS messages, but before the other nodes 		*/
		/* (and itself) could receive slots donation, a VIEW CHANGE (JOIN) has  occurred 	*/
		/* This means that the SYS_REQ_SLOTS message is not STABLE and it was discarded 	*/
		/* Therefore, if the local node hasn't got any owned slot, it must request again 	*/
		TASKDEBUG("bm_donors=%X bm_pending=%X\n", bm_donors, bm_pending);
		bm_donors=0;
		/* Another node has sent an SYS_REQ_SLOTS messages, but before this node could reply*/
		/* that it has ZERO slots to donate, a VIEW CHANGE (JOIN) has  occurred 			*/
		/* This means that the requesting node has cleared its bm_donors bitmap				*/
		bm_pending=0;
		
		/* If local node is the primary_mbr, send status info	*/
		/* to the next member waiting status info 				*/
		TASKDEBUG("member=%d FSM_state=%X\n", local_nodeid,FSM_state);
		primary_sts_mcast();
				
		/* a new member without owned slots and without donors with slots to donate */
		rcode = new_rqst_slots(min_owned_slots);
		if( rcode ) ERROR_RETURN(rcode);
	}

	return(OK);
}

/*======================================================================*
 *				sp_put_status				*
 * A new member has joined the group. 					*
 * The Primary has broadcasted Global Status information		*
 *======================================================================*/
int sp_put_status(char *spin_ptr )
{
	int rcode, i, init_mbr;
	char *bm_ptr;
	message msg, *m_ptr;
	proc_usr_t *proc_ptr;
	slot_t  *spt, *slot_tmp, *sp; 		
	
	TASKDEBUG("FSM_state=%X SP_bytes=%d \n",FSM_state, SP_bytes);

	m_ptr = (message *)spin_ptr;
	TASKDEBUG( MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	
	init_mbr = m_ptr->m_dest;
	
	if( init_mbr != local_nodeid) 	{
		/*
		*  The other members receipt the SYS_PUT_STATUS sent by Primary
		*/
		CLR_BIT(bm_waitsts, init_mbr);
		if( TEST_BIT(FSM_state, BIT_RUNNING)){
			TASKDEBUG("init_mbr=%d bm_waitsts=%d \n",init_mbr, bm_waitsts);
			primary_sts_mcast();
//		}else if (TEST_BIT(FSM_state, BIT_REQUESTING)) {
//			if (owned_slots == 0); 
//				rcode = mcast_rqst_slots(min_owned_slots);
//			return(OK);
		}else{	/* Uninitialized member */
			/* Sets the bm_init because another member SYS_PUT_STATUS arrives		*/
			/* before primary's SYS_PUT_STATUS message to local_nodeid 				*/
			/* and the primary don't consider in its multicasted SYS_PUT_STATUS		*/
			SET_BIT(bm_init, init_mbr);
			return(OK);
		}	
		
		/* Only initialized nodes came here */
		assert(TEST_BIT(bm_init, local_nodeid));

		/* The new_member must not be previously considered initialized */
		assert(!TEST_BIT(bm_init, init_mbr));
		
		/* Now, consider it as initialized */
		SET_BIT(bm_init, init_mbr);
		init_nodes++;
		
		TASKDEBUG("init_mbr=%d init_nodes=%d bm_init=%X\n", init_mbr, init_nodes, bm_init);
		/* compute the slot high water threshold	*/
		max_owned_slots	= (total_slots - (min_owned_slots*(init_nodes-1)));
		TASKDEBUG("bm_init=%X init_nodes=%d max_owned_slots=%d\n",bm_init, init_nodes, max_owned_slots );
		
		/* get remote SYSTASK  process descriptor pointer */
		proc_ptr = PROC2PTR(SYSTASK(init_mbr));

		/* is remote SYSTASK bound to the correct node ?*/
		TASKDEBUG(PROC_USR_FORMAT, PROC_USR_FIELDS(proc_ptr));
		if( proc_ptr->p_rts_flags == SLOT_FREE){
			rcode  = bind_rmt_systask(init_mbr);
			if( rcode < EMOLERRCODE) 	ERROR_RETURN(rcode);
		}
	
		/* IMPLICIT SYS_REQ_SLOTS when JOIN->PUT_STATUS  */
		rcode = sp_req_slots(init_mbr, min_owned_slots);
		return(rcode);
	}
	
	/*
	* INIT_MBR is  LOCAL NODE
	*  The init_mbr has receipt a SYS_PUT_STATUS message, therefore does not need anymore 
	*/
	if( FSM_state != STS_WAIT_STATUS){
		TASKDEBUG("RESIDUAL STS_WAIT_STATUS received\n");		
		return(OK);
	}

	/* Compare own DC descriptor against received DC descriptor */
	dcf_ptr = &dcu_primary;		
	memcpy((char *) &dcu_primary, spin_ptr+sizeof(message),  sizeof(dc_usr_t));

	TASKDEBUG( dc_USR_FORMAT, dc_USR_FIELDS(dcf_ptr));
	TASKDEBUG( dc_USR_FORMAT, dc_USR_FIELDS(dc_ptr));
	if  (dcf_ptr->dc_dcid != dc_ptr->dc_dcid)
		{rcode = EMOLBADDCID;}
	else if  (dcf_ptr->dc_nr_procs != dc_ptr->dc_nr_procs)
		{rcode = EMOLBADPROC;}
	else if  (dcf_ptr->dc_nr_tasks != dc_ptr->dc_nr_tasks)
		{rcode = EMOLINVAL;}
	else if  (dcf_ptr->dc_nr_sysprocs != dc_ptr->dc_nr_sysprocs)
		{rcode = EMOLINVAL;}
	else if  (dcf_ptr->dc_nr_nodes != dc_ptr->dc_nr_nodes)
		{rcode = EMOLDCNODE;}
	else if( strcmp(dcf_ptr->dc_name,dc_ptr->dc_name) )
		{rcode = EMOLNAMETOOLONG;}
	else
		{rcode = OK;}

	if( rcode != OK) {					/* received DC info dont match local DC info  */
		SP_disconnect (sysmbox);
		FSM_state = STS_DISCONNECTED;	/* change state  */
		ERROR_RETURN(rcode);
	}

	owned_slots = 0;
	/* bm_init considerer the bitmap sent by primary_mbr ORed by 					*/
	/* the bitmap of those nodes initialized before SYS_PUT_STATUS message arrives 	*/
	bm_init |= m_ptr->m_bm_init; 
	SET_BIT(bm_init, local_nodeid);
	init_nodes = 0; 
		
	/* Compute the count of initialized nodes */
	for(i=0 ; i < dc_ptr->dc_nr_nodes; i++){
		if( TEST_BIT(bm_init, i) )
			init_nodes++;
	}
	
	/* bm_waitsts considerer the bitmap sent by primary_mbr ORed by 				*/
	/* the bitmap of those nodes initialized before SYS_PUT_STATUS message arrives 	*/
	bm_waitsts |= m_ptr->m_bm_waitsts; 
	CLR_BIT(bm_waitsts, local_nodeid);

	TASKDEBUG("bm_init=%X init_nodes=%d\n",bm_init, init_nodes);

	slot_tmp = (slot_t  *) (spin_ptr + sizeof(message) + sizeof(dc_usr_t));

	TASKDEBUG( "Verify slot table consistency\n");
	for( i = 0; i < (dc_ptr->dc_nr_tasks + dc_ptr->dc_nr_procs);i++) {
		spt = &slot_tmp[i];
		TASKDEBUG("Verifying %i " SLOTS_FORMAT, i, SLOTS_FIELDS(spt));
		assert( i == spt->s_nr);
		
		/* BINDING POLICY  */
		if( i < dc_ptr->dc_nr_sysprocs) {
#ifdef ALLOC_LOCAL_TABLE 			
			proc_ptr = &proc[i];
			rcode  = mnx_getprocinfo(dc_ptr->dc_dcid, (i-dc_ptr->dc_nr_tasks), proc_ptr);
			if( rcode < 0) ERROR_RETURN(rcode);
#else /* ALLOC_LOCAL_TABLE */			
			proc_ptr = (proc_usr_t *) PROC_MAPPED((i-dc_ptr->dc_nr_tasks));	
			rcode = OK;			
#endif /* ALLOC_LOCAL_TABLE */	 		
			TASKDEBUG(PROC_USR_FORMAT, PROC_USR_FIELDS(proc_ptr));
			
			/* consider local endpoints */
			if( !TEST_BIT(proc_ptr->p_rts_flags, BIT_SLOT_FREE)) {
				if (proc_ptr->p_nodeid == local_nodeid){
					sp = &slot[proc_ptr->p_nr+dc_ptr->dc_nr_tasks];
					sp->s_endpoint = proc_ptr->p_endpoint;
					sp->s_flags = 0;
					sp->s_owner = local_nodeid;
					TASKDEBUG(SLOTS_FORMAT, SLOTS_FIELDS(sp));
					continue;
				}
			}
			
			if (spt->s_owner == NO_SLOT_OWNER)
				continue;
			

			TASKDEBUG("system process %i s_owner=%d s_endpoint=%d p_rts_flags=%X\n" ,
					i, spt->s_owner,  spt->s_endpoint, proc_ptr->p_rts_flags);
					
			/* local process descriptor is free, bind the remote process  */		
			if( TEST_BIT(proc_ptr->p_rts_flags, BIT_SLOT_FREE)) {
				if( spt->s_endpoint != NONE) {
					TASKDEBUG("remote binding %i " SLOTS_FORMAT, i, SLOTS_FIELDS(spt));
					rcode = mnx_rmtbind(dc_ptr->dc_dcid,"remote",spt->s_endpoint,spt->s_owner);
				}
				slot[i] = slot_tmp[i];
				continue;		
			} 

			/* WARNING, the remote endpoint could died and a new one could be 	*/
			/* started using the same slot										*/		
			if( proc_ptr->p_nodeid == spt->s_owner){
				/* same node , same endpoint */
				if( proc_ptr->p_endpoint == spt->s_endpoint){
					TASKDEBUG("matching slot %i " SLOTS_FORMAT, i, SLOTS_FIELDS(spt));
				}else{	/* same node , different endpoint */
					TASKDEBUG("unbind(%d), bind(%d), %i " SLOTS_FORMAT,
						proc_ptr->p_endpoint, spt->s_endpoint, i, SLOTS_FIELDS(spt));
					rcode = mnx_unbind(dc_ptr->dc_dcid, proc_ptr->p_endpoint);
					rcode = mnx_rmtbind(dc_ptr->dc_dcid,"remote",spt->s_endpoint,spt->s_owner);
				}
			}else{ 
				/* different  node , same endpoint */
				if( proc_ptr->p_endpoint == spt->s_endpoint){
					TASKDEBUG("migrating slot %i endpoint=%d from node %d to node %d\n",  
						i, proc_ptr->p_endpoint, proc_ptr->p_nodeid, spt->s_owner);
					rcode = mnx_migr_start(dc_ptr->dc_dcid, proc_ptr->p_endpoint);	
					rcode = mnx_migr_commit(PROC_NO_PID, dc_ptr->dc_dcid, proc_ptr->p_endpoint, spt->s_owner);
				}else{ /* different  node , different endpoint */
					TASKDEBUG("slot %i unbinding endpoint=%d from node %d to endpoint=%d node %d\n",  
						i, proc_ptr->p_endpoint, proc_ptr->p_nodeid, spt->s_endpoint, spt->s_owner);
					rcode = mnx_unbind(dc_ptr->dc_dcid, proc_ptr->p_endpoint);
					rcode = mnx_rmtbind(dc_ptr->dc_dcid,"remote",spt->s_endpoint,spt->s_owner);	
				}
			}
			slot[i] = slot_tmp[i];
			continue;
		}else{ /* User level process - shared slots */
			/* Replace only those slots without owner  */
			/* local node updates slot table with those SYS_DON_SLOTS even it is not initialized */
			/* while it is waiting for SYS_PUT_STATUS from primary_mbr */
			if( slot[i].s_owner == NO_SLOT_OWNER) 
				slot[i] = slot_tmp[i]; 
		}
	}

	TASKDEBUG("Member %d is initialize but without slots\n", init_mbr);	
	/* bound all remote SYSTASK tasks */
	for( i=0; i < dc_ptr->dc_nr_nodes; i++ ) {
		if( i == local_nodeid) continue;
		if ( TEST_BIT(bm_init, i) ){
			/* get remote SYSTASK  process descriptor pointer */
			proc_ptr = PROC2PTR(SYSTASK(i));
			/* Is the remote SYSTASK bound ? */
			if(proc_ptr->p_rts_flags == SLOT_FREE){
				rcode  = bind_rmt_systask(i);
				if( rcode > EMOLERRCODE)
						ERROR_EXIT(rcode);
			}
		}
	}
	/* The member is initialized but it hasn't got slots to start running */
	FSM_state = STS_REQ_SLOTS;
	bm_donors = bm_init;
	CLR_BIT(bm_donors, local_nodeid);
	
	/* IMPLICIT SYS_REQ_SLOTS when JOIN->PUT_STATUS  */
//	rcode = mcast_rqst_slots(min_owned_slots);
	return(rcode);
}

/*----------------------------------------------------------------------------------------------------
*			sp_req_slots
*   SYS_REQ_SLOTS: A Systask on other node has requested free slots
*----------------------------------------------------------------------------------------------------*/
int sp_req_slots(int requester, int needed_slots)
{
	int		rcode, i, j, surplus;
	int		donated_slots, don_nodes;
	proc_usr_t *proc_ptr;
	message  *spout_ptr;

	TASKDEBUG("FSM_state=%X requester=%d needed_slots=%d\n", 
		FSM_state ,requester, needed_slots);

	/* the member is not initialized yet, therefore it can't respond to this request */
	if( !TEST_BIT(FSM_state, BIT_INITIALIZED)) return(OK);

	/* Process own requests  */
	if(requester == local_nodeid) return(OK);
	
	/* Verify if the requester is initialized */
	assert(TEST_BIT(bm_init, requester));
	
	spout_ptr = (message *)&mess_out;
	/*
	* ALL other initialized members respond to a request slot message 
	* but only members with enough slots will donate
	*/
	if( bm_donors != 0 ) { /* local node is also a requester */
		/* If local node it is also a requester, then: 
		* consider the new requester as a PENDING donor which donates donated_slots = 0 
		* only if it has not replied yet. 
		* Then, do not reply to it because it has already receipt my request, but 
		* if local node is the next member of the new requester, it must ALLWAYS reply
		* (safety property)
		*/
		if( TEST_BIT(bm_donors, requester) && (get_next_mbr(requester, bm_init) != local_nodeid)){
			TASKDEBUG("concurrent request requester=%d bm_donors=%X local_nodeid=%d\n"
				,requester, bm_donors,local_nodeid);			
			CLR_BIT(bm_donors,requester);
			return(OK);
		}
		donated_slots = 0;
	}else{
		don_nodes = init_nodes-1;
		assert( don_nodes > 0);
		surplus = (free_slots - free_slots_low);
		TASKDEBUG("free_slots=%d free_slots_low=%d needed_slots=%d don_nodes=%d surplus=%d\n"
			,free_slots,free_slots_low,needed_slots,don_nodes, surplus);
		if( surplus <= 0 ) {
			donated_slots = 0;
		}else if( surplus > CEILING(needed_slots,don_nodes) ) {
			donated_slots = CEILING(needed_slots,don_nodes);
		}else {
			donated_slots = surplus;
		}
	}

	/* if the local node does not have slots to donate and have no pending donation response to requester*/
	/* it tries to reply later after receiving a donation message to the requester from other node		*/
	/* WARNING: if any node has slots to donate, the requester will hang waiting for donations which 	*/
	/* never will come. To break this issue, the requester's next member always replies (performance issue)	*/
	if( donated_slots == 0 && !TEST_BIT(bm_pending, requester)) {
		/* next member always reply */
		if( get_next_mbr(requester, bm_init) != local_nodeid){
			/* do not reply, wait until the first reply */
			SET_BIT(bm_pending, requester);
			return(OK);
		} 
	}
	
	/* Maximun number of slots that can be donated by message round - Limited by the message structure */
	if(donated_slots > SLOTS_BY_MSG) donated_slots = SLOTS_BY_MSG;
	TASKDEBUG("procs=%d donated_slots=%d requester=%d\n",
		(dc_ptr->dc_nr_tasks + dc_ptr->dc_nr_procs),donated_slots, requester);
		
	/* build the donation message */	
	spout_ptr->m_source = local_nodeid;
	spout_ptr->m_type = SYS_DON_SLOTS;
	spout_ptr->mA_dst = requester;
	spout_ptr->mA_nr = donated_slots;
	
	/* search free slots to donate and set each one in DONATING state */
	for ( j = 0 ; j < SLOTS_BY_MSG; j++) spout_ptr->mA_ia[j] = 0;
	if( donated_slots > 0) {
		/* Search free owned slots - IT CAN BE OPTIMIZED by a FREE LIST */
		for( j = 0, i = dc_ptr->dc_nr_sysprocs;
			((i < (dc_ptr->dc_nr_tasks + dc_ptr->dc_nr_procs)) && (donated_slots > 0) );
			i++) {
#ifdef ALLOC_LOCAL_TABLE 			
			proc_ptr = &proc[i];
#else /* ALLOC_LOCAL_TABLE */			
			proc_ptr = (proc_usr_t *) PROC_MAPPED(i-dc_ptr->dc_nr_tasks);						
#endif /* ALLOC_LOCAL_TABLE */			
			if( (proc_ptr->p_rts_flags == SLOT_FREE) &&
				(slot[i].s_owner == local_nodeid)) {
				donated_slots--;
				free_slots--;
				owned_slots--;
				slot[i].s_owner  = requester;
				SET_BIT(proc_ptr->p_rts_flags, BIT_NO_MAP);
				SET_BIT( slot[i].s_flags, BIT_SLOT_DONATING);
				spout_ptr->mA_ia[j++] = i;
			}
		}
	}

	TASKDEBUG(MSGA_FORMAT, MSGA_FIELDS(spout_ptr));
	/* Multicast the donation to all process to register the new owner of the slots */
	rcode = SP_multicast (sysmbox, SAFE_MESS, (char *) dc_ptr->dc_name,
			SYS_DON_SLOTS, sizeof(message), (char *) spout_ptr);
	return(rcode);
}

/*----------------------------------------------------------------------------------------------------
*				sp_don_slots
*   SYS_DON_SLOTS: A Donation Message has received
*  If it is the destination systask, it adds the new free slots to its own
* or if they are for other node register the new ownership
*----------------------------------------------------------------------------------------------------*/
int sp_don_slots(message  *spin_ptr)
{
	int rcode,  j, i, leftover, mbr;
	int		donated_slots, requester;
	proc_usr_t *proc_ptr;
	message  *spout_ptr;

	TASKDEBUG("FSM_state=%X\n", FSM_state);

	TASKDEBUG("Donation of %d slots from %d to %d\n", 
		spin_ptr->mA_nr, spin_ptr->m_source, spin_ptr->mA_dst);

	/* Is the destination an initialized member ? */
	assert(TEST_BIT(bm_init, spin_ptr->mA_dst));

	/* Is the donor an initialized member ? */
	assert(TEST_BIT(bm_init, spin_ptr->m_source));
	
	/* Change the ownership of donated slots */
	for( j = 0; j < spin_ptr->mA_nr; j++) {
		i = spin_ptr->mA_ia[j];
		slot[i].s_owner = spin_ptr->mA_dst;
		/* the donor clears donating bits */
		if( spin_ptr->m_source == local_nodeid) {
#ifdef ALLOC_LOCAL_TABLE 			
			proc_ptr = &proc[i];
#else /* ALLOC_LOCAL_TABLE */			
			proc_ptr = (proc_usr_t *) PROC_MAPPED(i-dc_ptr->dc_nr_tasks);						
#endif /* ALLOC_LOCAL_TABLE */			
			CLR_BIT(proc_ptr->p_rts_flags, BIT_NO_MAP);
			CLR_BIT(slot[i].s_flags, BIT_SLOT_DONATING);
			return(OK);
		}
	}
	
	/* not initialized nodes, return */
	if( !TEST_BIT(FSM_state, BIT_INITIALIZED)) {
		assert( spin_ptr->mA_dst != local_nodeid);	
		return(OK);
	}

	/* I am the donor */
	if( spin_ptr->m_source == local_nodeid) 
		return(OK);

	/* Are the slots for other node? */
	if( spin_ptr->mA_dst != local_nodeid) {
		rcode = OK;
		/* Are there any pending reply with ZERO slots for this destination ?*/
		if( TEST_BIT(bm_pending, spin_ptr->mA_dst)) {
			/* HERE We can re-calculate slot availability, but for now is  0 */
			donated_slots = 0;
			requester = spin_ptr->mA_dst;
			TASKDEBUG("procs=%d donated_slots=%d requester=%d\n",
				(dc_ptr->dc_nr_tasks + dc_ptr->dc_nr_procs),donated_slots, requester);
			spout_ptr->m_source = local_nodeid;
			spout_ptr->m_type = SYS_DON_SLOTS;
			spout_ptr->mA_dst = requester;
			spout_ptr->mA_nr = donated_slots;
			for ( j = 0 ; j < SLOTS_BY_MSG; j++) spout_ptr->mA_ia[j] = 0;
			rcode = SP_multicast (sysmbox, SAFE_MESS, (char *) dc_ptr->dc_name,
				SYS_DON_SLOTS, sizeof(message), (char *) spout_ptr);
			CLR_BIT(bm_pending, spin_ptr->mA_dst);
		}
		return(rcode);
	}

	/*
	* DONATION FOR LOCAL NODE 
	*/
	if( owned_slots == 0 && spin_ptr->mA_nr > 0){
		FSM_state = STS_RUNNING;
		free_slots+= spin_ptr->mA_nr;
		owned_slots+=spin_ptr->mA_nr;
		TASKDEBUG("Wake up systask init_nodes=%d bm_init=%X\n", init_nodes,bm_init);
		pthread_cond_signal(&sys_barrier);	/* Wakeup SYSTASK 		*/	
	}else{	
		free_slots+= spin_ptr->mA_nr;
		owned_slots+=spin_ptr->mA_nr;
	}
	
	CLR_BIT(bm_donors, spin_ptr->m_source);
	TASKDEBUG("free_slots=%d free_slots_low=%d\n",free_slots, free_slots_low);
	TASKDEBUG("owned_slots=%d max_owned_slots=%d bm_donors=%X\n", owned_slots, max_owned_slots,bm_donors);

	/* a new member without slots, and without donors with donated slots */
	rcode = new_rqst_slots(min_owned_slots);
	if( rcode ) ERROR_RETURN(rcode);
	
	return(OK);
}

/*===========================================================================*
 *				sp_disconnect				
 * A member has left or it has disconnected from the DC group	
 *===========================================================================*/
int sp_disconnect(int  disc_mbr)
{
	int	i, rcode, dcinfo_flag, table_flag, mbr;
	proc_usr_t *proc_ptr;

	TASKDEBUG("FSM_state=%X disc_mbr=%d active_nodes=%d\n",FSM_state, disc_mbr, active_nodes);

	if(local_nodeid == disc_mbr) {
		FSM_state = STS_DISCONNECTED;
		return(OK);
	}
	
	/* if local_nodeid is not initialized and is the only surviving node , restart all*/
	if( !TEST_BIT(FSM_state,BIT_INITIALIZED)) {
		if( active_nodes == 1) {
			init_global_vars();
			SP_disconnect (sysmbox);
			SP_join( sysmbox, dc_ptr->dc_name);
			return(OK);
		}
	}

	/* A member has disconnected during a merge: retry merging */
	if( TEST_BIT( FSM_state, BIT_MERGING)) {
		return(sp_net_merge());
	}
	
	/* if the dead node was initialized */
	if( TEST_BIT(bm_init, disc_mbr)) {
		init_nodes--;	/* decrease the number of initialized nodes */
		CLR_BIT(bm_init, disc_mbr);
	}

	/* verify if the local member is waiting a donation from the disconnected node */
	if( TEST_BIT(bm_donors, disc_mbr) ) {
		CLR_BIT(bm_donors, disc_mbr);
	}

	/* verify if the dead node was waiting SYS_PUT_STATUS message from primary_mbr */
	if( TEST_BIT(bm_waitsts, disc_mbr) ) {
		CLR_BIT(bm_waitsts, disc_mbr);
	}
	
	/* Are there any pending ZERO reply for that dead node ? */
	if( TEST_BIT(bm_pending, disc_mbr)){
		CLR_BIT(bm_pending, disc_mbr);
	}
	
	/* if the dead node was the primary_mbr, search another primary_mbr */
	if( primary_mbr == disc_mbr) {
		primary_mbr = get_primary_mbr();
		if( primary_mbr == NO_PRIMARY_MBR) {
			TASKDEBUG("primary_mbr == NO_PRIMARY_MBR\n");
			FSM_state = STS_NEW;
			init_global_vars();
			SP_disconnect (sysmbox);
			SP_join( sysmbox, dc_ptr->dc_name);
			return(OK);
		}		
	}

	TASKDEBUG("disc_mbr=%d active_nodes=%d\n",disc_mbr, active_nodes);
	TASKDEBUG("primary_mbr=%d init_nodes=%d\n",primary_mbr, init_nodes);
	TASKDEBUG("bm_active=%X bm_init=%X bm_donors=%X\n",	bm_active,bm_init,bm_donors);

	/* recalculate global variables */
	max_owned_slots	= (total_slots - (min_owned_slots*(init_nodes-1)));
	TASKDEBUG("max_owned_slots=%d\n",max_owned_slots);

	/* reallocate slots of the disconnecter member to the primary member */
	/* and unbind its endpoints								*/
	for(i = dc_ptr->dc_nr_sysprocs; 
		i < (dc_ptr->dc_nr_tasks + dc_ptr->dc_nr_procs);
		i++) {
		if( slot[i].s_owner == disc_mbr ) {
			/* an uncompleted donation */
#ifdef ALLOC_LOCAL_TABLE 			
			proc_ptr = &proc[i];
#else /* ALLOC_LOCAL_TABLE */			
			proc_ptr = (proc_usr_t *) PROC_MAPPED(i-dc_ptr->dc_nr_tasks);						
#endif /* ALLOC_LOCAL_TABLE */
			TASKDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(proc_ptr));
			if( TEST_BIT(slot[i].s_flags, BIT_SLOT_DONATING)) { 
				/* local node was the donor */
				free_slots++;
				owned_slots++;
				slot[i].s_owner = local_nodeid;
				CLR_BIT(proc_ptr->p_rts_flags, BIT_NO_MAP);
			}else {
				slot[i].s_owner = primary_mbr;
				if( primary_mbr == local_nodeid){
					/* The local node is the new primary and 
					 * has inherit disconnected members slots
					 * if it has not any slot it is blocked waiting for them.
					 * therefore, now SYSTASK must be signaled 
					 */
					if( owned_slots == 0){
						FSM_state = STS_RUNNING;
						pthread_cond_signal(&sys_barrier);
					}
					owned_slots++;
					free_slots++;
				}
			}
			if (FSM_state == STS_WAIT_STATUS) continue;

			/* DeRegister  the remote process to the kernel */
			if( !TEST_BIT(proc_ptr->p_rts_flags, BIT_SLOT_FREE)) { 
				if(proc_ptr->p_nodeid != disc_mbr ){
					printf("INCONSISTENCY!!:s_owner=%d, disc_mbr=%d, p_nodeid=%d\n",
					slot[i].s_owner, disc_mbr, proc_ptr->p_nodeid );
					ERROR_EXIT(EMOLGENERIC);
				}
				rcode = mnx_unbind(dc_ptr->dc_dcid,proc_ptr->p_endpoint);
				if(rcode != OK)  ERROR_RETURN(rcode);
			}
		}
	}

	if (FSM_state == STS_WAIT_STATUS) return(OK);

	/* unbind remote tasks & servers */
	TASKDEBUG("unbind remote tasks & servers\n");
	for(i = 0; i < dc_ptr->dc_nr_sysprocs; i++) {
#ifdef ALLOC_LOCAL_TABLE 			
		proc_ptr = &proc[i];
#else /* ALLOC_LOCAL_TABLE */			
		proc_ptr = (proc_usr_t *) PROC_MAPPED(i-dc_ptr->dc_nr_tasks);						
#endif /* ALLOC_LOCAL_TABLE */	
		if( proc_ptr->p_nodeid == disc_mbr)	{
			rcode = unbind_rmt_sysproc(proc_ptr->p_endpoint);
			if(rcode != OK)  ERROR_RETURN(rcode);
		}
	}
	
	primary_sts_mcast();

	
	/* This node has sent an SYS_REQ_SLOTS messages, but before the other nodes 	*/
	/* (and itself) could receive slots donation, a VIEW CHANGE (DISCONNECT) has  occurred 	*/
	/* This means that the SYS_REQ_SLOTS message is not STABLE and it was discarded 	*/
	/* Therefore, if the local node hasn't got any owned slot, it must request again 		*/
	TASKDEBUG("bm_donors=%X bm_pending=%X\n", bm_donors, bm_pending);
	bm_donors=0;
	/* Another node has sent an SYS_REQ_SLOTS messages, but before this node could reply*/
	/* that it has ZERO slots to donate, a VIEW CHANGE (DISCONNECT) has  occurred 			*/
	/* This means that the requesting node has cleared its bm_donors bitmap				*/
	bm_pending=0;
	
	rcode=OK;
	/* a new member without slots, and without donors with donated slots */
	rcode = new_rqst_slots(min_owned_slots);
	if( rcode ) ERROR_RETURN(rcode);

	return(rcode);
}

/*-----------------------------------------------------------------------
*				sp_net_partition
*  A network partition has occurred
*------------------------------------------------------------------------*/
int sp_net_partition(void)
{
	int i, j, mbr, rcode;
	unsigned long int	bm_tmp;
	proc_usr_t *proc_ptr;

	TASKDEBUG("FSM_state=%X\n", FSM_state);

	/* mask the old bitmaps with the mask of active members 	*/
	/* only the active nodes should be considered 			 	*/
	TASKDEBUG("bm_init=%X bm_active=%X bm_waitsts=%X \n", bm_init, bm_active, bm_waitsts);
	bm_init 	&= bm_active;
	bm_waitsts 	&= bm_active;
	
	if( FSM_state == STS_WAIT_STATUS) {
		if( bm_init == 0)
			ERROR_RETURN(EMOLNOREPLICA);
		return(OK);
	}

	/* if a network partition occurred during MERGING, ignore merging */
	if( FSM_state == STS_MERGE_STATUS) {
		FSM_state = STS_RUNNING;
	}

	/* Is this the primary_mbr partition (where the old primary_mbr is located) ? */
	if( !TEST_BIT(bm_init, primary_mbr)){ /* NO, it is on other partition */
		/* get the Primary member of this partition */
		primary_mbr = get_primary_mbr();
		if(primary_mbr != NO_PRIMARY_MBR){
			init_global_vars();
			SP_disconnect (sysmbox);
			SP_join(sysmbox, dc_ptr->dc_name);
			return(OK);
		}
	}

	/* recalculate status variables for this partition */
	total_slots = 0;
	for( i = 0; i < (dc_ptr->dc_nr_tasks + dc_ptr->dc_nr_procs);i++) {
#ifdef ALLOC_LOCAL_TABLE 			
		proc_ptr = &proc[i];
		rcode  = mnx_getprocinfo(dcu.dc_dcid, (i-dc_ptr->dc_nr_tasks), proc_ptr);
		if( rcode < 0) ERROR_RETURN(rcode);
#else /* ALLOC_LOCAL_TABLE */			
		proc_ptr = (proc_usr_t *) PROC_MAPPED((i-dc_ptr->dc_nr_tasks));	
		rcode = OK;		
#endif /* ALLOC_LOCAL_TABLE */
			
		if( i < dc_ptr->dc_nr_sysprocs) {
			if( slot[i].s_owner != local_nodeid) { 
				/* The owner is on another partition ? */
				if( !TEST_BIT(bm_init, slot[i].s_owner)) {
					SET_BIT(slot[i].s_flags, BIT_SLOT_PARTITION);
					if( TEST_BIT(proc_ptr->p_rts_flags, BIT_REMOTE))
						mnx_migr_start(dc_ptr->dc_dcid, proc_ptr->p_endpoint);
				}
			}
			continue;
		}
		/* restore own slots from uncompleted donations   */
		if ( (TEST_BIT(slot[i].s_flags, BIT_SLOT_DONATING))
//			 && (!TEST_BIT(bm_init, slot[i].s_owner))
								) { /*The destination is not on my partition */
			free_slots++;
			owned_slots++;
			slot[i].s_owner = local_nodeid;
			CLR_BIT(proc_ptr->p_rts_flags, BIT_NO_MAP);
			CLR_BIT(slot[i].s_flags, BIT_SLOT_DONATING);
			TASKDEBUG("Restoring slot %d from uncompleted donation after a network partition\n",i);
		}
		TASKDEBUG("Slot %d is owned by %d ",i, slot[i].s_owner)
		if(TEST_BIT(bm_init, slot[i].s_owner)) { /* The owner is on my partition */
			TASKDEBUG("on my partition\n");
			total_slots++;
		}else{ /* The slot belongs to a member of other partition */
			TASKDEBUG("on other partition\n");
			SET_BIT(slot[i].s_flags, BIT_SLOT_PARTITION);
		}
	}

	init_nodes = get_init_nodes();

	TASKDEBUG("primary_mbr=%d bm_init=%X bm_donors=%X bm_waitsts=%X\n",
		primary_mbr, bm_init, bm_donors, bm_waitsts);	

	/* recalculate global variables */
	max_owned_slots	= (total_slots - (min_owned_slots*(init_nodes-1)));
	TASKDEBUG("total_slots=%d max_owned_slots=%d init_nodes=%d owned_slots=%d\n",
		total_slots,max_owned_slots, init_nodes,owned_slots);

	primary_sts_mcast();

	/* Mask pending ZERO replies only to partition initialized nodes  */
//	if( bm_pending != 0) {
//		bm_pending &= bm_init;
//	}

	/* This node has sent an SYS_REQ_SLOTS messages, but before the other nodes 	*/
	/* (and itself) could receive slots donation, a VIEW CHANGE (NET PARTITION) has  occurred 	*/
	/* This means that the SYS_REQ_SLOTS message is not STABLE and it was discarded 	*/
	/* Therefore, if the local node hasn't got any owned slot, it must request again 		*/
	TASKDEBUG("bm_donors=%X bm_pending=%X\n", bm_donors, bm_pending);
	bm_donors=0;
	/* Another node has sent an SYS_REQ_SLOTS messages, but before this node could reply	*/
	/* that it has ZERO slots to donate, a VIEW CHANGE (NET PARTITION) has  occurred 	*/
	/* This means that the requesting node has cleared its bm_donors bitmap				*/
	bm_pending=0;

	/* a new member without slots, and without donors with donated slots */
	if( owned_slots == 0) {
		if( init_nodes > 1){
			rcode = new_rqst_slots(min_owned_slots);
			if( rcode ) ERROR_RETURN(rcode);
		}else{
			TASKDEBUG("Partition without slots, exiting! init_nodes=%d\n", init_nodes);
			ERROR_RETURN(EMOLNOREPLICA);
		}
	}
	
	return(OK);
}

/*----------------------------------------------------------------------------------------------------
*				sp_net_merge
*  A network merge has occurred
*  the Primary member of each merged partition
*  broadcast its current process slot table (PST)
*  only filled with the slots owned by members of their partitions
*----------------------------------------------------------------------------------------------------*/
int sp_net_merge(void)
{
	int i, rcode;
	slot_t   *slot_part, *sp; 		/* PST from the merged partition  */
	message  msg;

	TASKDEBUG("FSM_state=%X bm_init=%X\n",FSM_state, bm_init);

	/* This node has sent an SYS_REQ_SLOTS messages, but before the other nodes 	*/
	/* (and itself) could receive slots donation, a VIEW CHANGE (NET MERGE) has  occurred 	*/
	/* This means that the SYS_REQ_SLOTS message is not STABLE and it was discarded 	*/
	/* Therefore, if the local node hasn't got any owned slot, it must request again 		*/
	TASKDEBUG("bm_donors=%X bm_pending=%X\n", bm_donors, bm_pending);
	bm_donors=0;
	/* Another node has sent an SYS_REQ_SLOTS messages, but before this node could reply*/
	/* that it has ZERO slots to donate, a VIEW CHANGE (NET MERGE) has  occurred 			*/
	/* This means that the requesting node has cleared its bm_donors bitmap				*/
	bm_pending=0;
	
	SET_BIT(FSM_state, BIT_MERGING);
	
	/* copy slot table to temporal table for merging */
	copy_to_mrg();
		
	if( !TEST_BIT(FSM_state,BIT_RUNNING)) {
		return(OK);
	}
	
	/* Only executed this function the Primary members of merged partitions */
	if ( primary_mbr != local_nodeid) return(OK);
	
	/*------------------------------------
	* Alloc memory for temporal  PST
 	*------------------------------------*/
//	slot_part = (slot_t *) malloc( sizeof(slot_t)  * (dcu.dc_nr_procs+dcu.dc_nr_tasks));
	posix_memalign( (void**) &slot_part, getpagesize(), sizeof(slot_t)  * (dcu.dc_nr_procs+dcu.dc_nr_tasks));
	if(slot_part == NULL) return (EMOLNOMEM);

	/* Copy the PST to temporal PST */
	memcpy( (void*) slot_part, slot, (sizeof(slot_t) * (dcu.dc_nr_procs+dcu.dc_nr_tasks)));

	/* fill the slot table 	*/
	total_slots = 0;
	for( i = 0; i < (dcu.dc_nr_procs+dcu.dc_nr_tasks); i++) {
		/* bm_init keeps the initialized members before MERGE */
		if( !TEST_BIT(bm_init, slot[i].s_owner)) { /* the owner is not on this partition */
			sp = &slot[i];
			TASKDEBUG("NO_PRIMARY_MBR %i " SLOTS_FORMAT, i, SLOTS_FIELDS(sp));
			slot_part[i].s_owner = NO_PRIMARY_MBR; /* this partition does not own this slot*/
		}else{
			total_slots++;
		}
	}

	/* broadcast the partition PST */
	TASKDEBUG("Send SYS_MERGE_STATUS \n");
	mcast_status_info(ANY, SYS_MERGE_STATUS,(char*) slot_part);
	
    free( (void *) slot_part);

	return(OK);
}

/*===========================================================================*
 *				sp_merge_status
 * A network merge has occurred
 * All members must merge PSTs sent by primary_mbr of all partitions
 *===========================================================================*/
int sp_merge_status(int sender, char *spin_ptr, int bytes)
{
	int i, rcode, total, s_len, pending;
	slot_t   *slot_part, *sp, *spm, *spp; 		/* PST from the merged partition  */
	message  msg, *m_ptr;
	char *v_ptr, *b_ptr, *s_ptr;
	proc_usr_t *proc_ptr;

	TASKDEBUG("FSM_state=%X sender=%d\n",FSM_state, sender);

	if( !TEST_BIT(FSM_state, BIT_MERGING)) {
		init_global_vars();
		FSM_state = STS_WAIT_STATUS;
		return(OK);
	}
	
	if ( !TEST_BIT(bm_init, sender) ) { /* Primary member of other partition */
	
		m_ptr = (message*) spin_ptr;
		TASKDEBUG( MSG2_FORMAT, MSG2_FIELDS(m_ptr));
		v_ptr = spin_ptr + sizeof(message);
		total = sizeof(message) + sizeof(dc_usr_t)+ s_len;
		/* pointer to receive slot table from other partition */
		slot_part = (slot_t  *) v_ptr + sizeof(dc_usr_t);

		total_slots = 0;
		for(i = 0; i < (dc_ptr->dc_nr_tasks + dc_ptr->dc_nr_procs); i++){
#ifdef ALLOC_LOCAL_TABLE 			
			proc_ptr = &proc[i];
			rcode  = mnx_getprocinfo(dcu.dc_dcid, (i-dc_ptr->dc_nr_tasks), proc_ptr);
			if( rcode < 0) ERROR_RETURN(rcode);
#else /* ALLOC_LOCAL_TABLE */			
			proc_ptr = (proc_usr_t *) PROC_MAPPED((i-dc_ptr->dc_nr_tasks));	
			rcode = OK;			
#endif /* ALLOC_LOCAL_TABLE */

			/* skip slot not owned by the sender's partition */
			spm = &slot_merge[i];
			spp = &slot_part[i];
			if( TEST_BIT(spp->s_flags, BIT_SLOT_PARTITION)){
				TASKDEBUG("ignored %i " SLOTS_FORMAT, i, SLOTS_FIELDS(spp));
				continue;
			}
			
			/* MERGING POLICY
				1- Locally bound slots remain locally bound
				2- Remotely bound slots remain Remotely bound
					a- if remain the same endpoint  (mnx_migr_rollback)
					b- if remain the same endpoint but on other remote node (mnx_migr_commit)
					c- if the endpoint changed (mnx_unbind, mnx_rmtbind)
				3- Free slots now bound on remote node (mnx_rmtbind)
			*/	
			if( i < dc_ptr->dc_nr_sysprocs) {
				if ( TEST_BIT(bm_init,  spm->s_owner) )
					continue;
		
				/* WARNING, the remote endpoint could died and a new one could be 	*/
				/* started using the same slot										*/
				TASKDEBUG("system process %i s_owner=%d s_endpoint=%d p_rts_flags=%X\n" ,
					i, spm->s_owner,  spm->s_endpoint, proc_ptr->p_rts_flags);
				if( TEST_BIT(proc_ptr->p_rts_flags, BIT_SLOT_FREE)) {
					if( spm->s_endpoint != NONE) {
						rcode = mnx_rmtbind(dc_ptr->dc_dcid,"remote",spm->s_endpoint,spm->s_owner);
						slot_merge[i] = slot_part[i];	/* copy the slot	*/
						TASKDEBUG("remote binding %i " SLOTS_FORMAT, i, SLOTS_FIELDS(spm));
					}
					continue;		
				}
				
				if( spm->s_owner == spp->s_owner){ 
					/* same node , same endpoint */
					if( spm->s_endpoint == spp->s_endpoint){ 
						TASKDEBUG("mnx_migr_rollback %i " SLOTS_FORMAT, i, SLOTS_FIELDS(spm));
						rcode = mnx_migr_rollback(dc_ptr->dc_dcid, proc_ptr->p_endpoint);
					}else{	/* same node , different endpoint */
						TASKDEBUG("1)unbind(%d), bind(%d), %i " SLOTS_FORMAT,
							spm->s_endpoint, spp->s_endpoint, i, SLOTS_FIELDS(spm));
						rcode = mnx_unbind(dc_ptr->dc_dcid, spm->s_endpoint);
						slot_merge[i] = slot_part[i];	/* copy the slot	*/
						rcode = mnx_rmtbind(dc_ptr->dc_dcid,"remote",spm->s_endpoint,spm->s_owner);
					}
				}else{ 
					/* different  node , same endpoint */
					if( spm->s_endpoint == spp->s_endpoint){
						slot_merge[i] = slot_part[i];	/* copy the slot	*/	
						rcode = mnx_migr_commit(PROC_NO_PID, dc_ptr->dc_dcid, proc_ptr->p_endpoint, spm->s_owner);
						TASKDEBUG("mnx_migr_commit %i " SLOTS_FORMAT, i, SLOTS_FIELDS(spm));
					}else{ /* different  node , different endpoint */
						rcode = mnx_unbind(dc_ptr->dc_dcid, spm->s_endpoint);
						TASKDEBUG("2)unbind(%d), bind(%d), %i " SLOTS_FORMAT, 
						spm->s_endpoint, spm->s_endpoint, i, SLOTS_FIELDS(spm));
						slot_merge[i] = slot_part[i];	/* copy the slot	*/
						rcode = mnx_rmtbind(dc_ptr->dc_dcid,"remote",spm->s_endpoint,spm->s_owner);	
					}
				}
				CLR_BIT(spm->s_flags, BIT_SLOT_PARTITION);
			}else { /* shared slots */
				if (spp->s_owner != local_nodeid)
					slot_merge[i] = slot_part[i];	/* copy the slot	*/
				total_slots++;
				TASKDEBUG("copied %i " SLOTS_FORMAT, i, SLOTS_FIELDS(spm));
				CLR_BIT(spm->s_flags, BIT_SLOT_PARTITION);
			}
		}
		
		/* merge initilized & wait status bitmaps */
		bm_init_mrg |= m_ptr->m_bm_init;
		bm_waitsts_mrg |= m_ptr->m_bm_waitsts;

	}	
	
	/* Are there any member who do not inform about its slots ? */
	TASKDEBUG("primary_mbr=%d active=%X bm_init=%X\n",primary_mbr, bm_active, bm_init);
	pending = FALSE;
	for(i = 0; i < (dc_ptr->dc_nr_tasks + dc_ptr->dc_nr_procs); i++){
		if( TEST_BIT(spm->s_flags, BIT_SLOT_PARTITION) &&
		    TEST_BIT(bm_active,  spm->s_owner) ){
			TASKDEBUG("pending %i " SLOTS_FORMAT, i, SLOTS_FIELDS(spm));
			pending = TRUE;
		}
	}

	TASKDEBUG("pending=%d\n",pending);
	/* pending update from other partition primary member */
	if(pending == TRUE) return(OK);
	
	/* TRANSACTION FINISHED: COMMIT THE PST !!!!! */
	copy_from_mrg();

	init_nodes = get_init_nodes();
	bm_donors = 0;
	bm_pending = 0;

	/* recalculate global variables */
	max_owned_slots	= (total_slots - (min_owned_slots*(init_nodes-1)));
	TASKDEBUG("bm_init=%X max_owned_slots=%d init_nodes=%d\n",
		bm_init, max_owned_slots,init_nodes);
	
	/* elect overall primary member */
	primary_mbr = get_primary_mbr();
	assert( primary_mbr != NO_PRIMARY_MBR);
	
	if( local_nodeid == primary_mbr){
		for(i = 0; i < (dc_ptr->dc_nr_tasks + dc_ptr->dc_nr_procs); i++){
			sp = &slot[i];
			TASKDEBUG("Resulting slot %i " SLOTS_FORMAT, i, SLOTS_FIELDS(sp));
		}
		TASKDEBUG("Primary Sending SYS_PUT_STATUS bm_waitsts=%X primary_mbr=%d\n", 
			bm_waitsts, primary_mbr);
		
		primary_sts_mcast();
	}

	if( owned_slots == 0) {
		rcode = new_rqst_slots(min_owned_slots);
		if( rcode ) ERROR_RETURN(rcode);
	}else{
		FSM_state = STS_RUNNING;
	}
	
	return(OK);
}

/*===========================================================================*
 *				sp_syscall 								     	*
 * Other systask inform same SYSCALL event (bind, fork, exit)					*
 *===========================================================================*/
int sp_syscall(message  *spin_ptr)
{
	int i, ret;
	proc_usr_t *proc_ptr;	/* sysproc process pointer */
	priv_usr_t *priv_ptr;		/* sysproc privileges pointer */
	proc_usr_t *pm_ptr;
	static proc_usr_t pm_desc;

	TASKDEBUG("source=%d m_type=%d p_nr=%d p_endpoint=%d\n",
		spin_ptr->m_source,spin_ptr->m_type,spin_ptr->M3_P_NR,spin_ptr->M3_ENDPT);

	if( !TEST_BIT(FSM_state, BIT_INITIALIZED)) return(OK);

	/*Ignore owns requests  */
	if(spin_ptr->m_source == local_nodeid) {
		pthread_cond_signal(&sys_syscall);	
		return(OK);
	}	

	switch( spin_ptr->m_type) {
		/*-----------------------------------------------------------------------
		* m_source= nodeid
		* M3_P_NR : p_nr
		* M3_ENDPT :endpoint
		* M3_NAME: name
		*-----------------------------------------------------------------------*/
		case MOLBINDPROC:
		case MOLFORK:
			if(spin_ptr->m_type == MOLBINDPROC) {
				TASKDEBUG("MOLBINDPROC p_nr=%d p_endpoint=%d\n",
					spin_ptr->M3_P_NR, spin_ptr->M3_ENDPT);
				if( (spin_ptr->M3_P_NR+dc_ptr->dc_nr_tasks) >= dc_ptr->dc_nr_sysprocs) {
					TASKDEBUG("ERROR: node %d send MOLBINDPROC of p_nr=%d p_endpoint=%d\n!",
						spin_ptr->m_source,spin_ptr->M3_P_NR,spin_ptr->M3_ENDPT);
					ERROR_RETURN(EMOLBADPROC);
				}
			} else {
				TASKDEBUG("MOLFORK p_nr=%d p_endpoint=%d\n",
					spin_ptr->M3_P_NR, spin_ptr->M3_ENDPT);
				/* Verify if the node that inform the FORK is the owner of the slot */
				if( (spin_ptr->M3_P_NR+dc_ptr->dc_nr_tasks) >= dc_ptr->dc_nr_sysprocs) {
					i = (spin_ptr->M3_P_NR+dc_ptr->dc_nr_tasks);
					if( slot[i].s_owner != spin_ptr->m_source)
					TASKDEBUG("ERROR: node %d is not the owner of slot %d (owner=%d)\n!",
						spin_ptr->m_source, i, slot[i].s_owner);
					ERROR_RETURN(EMOLBADOWNER);	
				}
			}
			
			proc_ptr = PROC2PTR(spin_ptr->M3_P_NR);

			/* Check if the slot is free  */
			if( proc_ptr->p_rts_flags != SLOT_FREE ){
				if( TEST_BIT(proc_ptr->p_misc_flags, MIS_BIT_REPLICATED)){
					TASKDEBUG("REPLICATED dcid=%d name=%s p_nr=%d p_endpoint=%d nodeid=%d\n",
						dc_ptr->dc_dcid,spin_ptr->M3_NAME, spin_ptr->M3_P_NR
						,spin_ptr->M3_ENDPT, spin_ptr->m_source);					
					return(OK);
				} else if( spin_ptr->M3_ENDPT == SYSTASK(spin_ptr->m_source) ){
					/* Remote SYSTASK allready bound */
					return(OK);				
				} else{
					ERROR_RETURN(EMOLSLOTUSED);
				}
			} 
			
			/* Register  the remote process to the kernel */
			TASKDEBUG("mnx_rmtbind dcid=%d name=%s p_nr=%d p_endpoint=%d nodeid=%d\n",
				dc_ptr->dc_dcid,spin_ptr->M3_NAME, spin_ptr->M3_P_NR
				,spin_ptr->M3_ENDPT, spin_ptr->m_source);
				
			ret = mnx_rmtbind(dc_ptr->dc_dcid,spin_ptr->M3_NAME, 
				spin_ptr->M3_ENDPT, spin_ptr->m_source);
			if( ret < EMOLERRCODE) ERROR_RETURN(ret);

			/* Get info about the process from the KERNEL */
#ifdef ALLOC_LOCAL_TABLE 			
			ret = mnx_getprocinfo(dc_ptr->dc_dcid, spin_ptr->M3_P_NR, proc_ptr);
			if( ret < 0) ERROR_RETURN(ret);
#else /* ALLOC_LOCAL_TABLE */			
			proc_ptr = (proc_usr_t *) PROC_MAPPED(spin_ptr->M3_P_NR);	
			ret = OK;			
#endif /* ALLOC_LOCAL_TABLE */			
			TASKDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(proc_ptr));

			/* Get privilege information  about the process from the KERNEL */
			priv_ptr 	=  PROC2PRIV(spin_ptr->M3_P_NR);
			ret = mnx_getpriv(dc_ptr->dc_dcid, spin_ptr->M3_ENDPT, priv_ptr);
			if( ret < 0) ERROR_RETURN(ret);
			TASKDEBUG(PRIV_USR_FORMAT,PRIV_USR_FIELDS(priv_ptr));
			break;
		case MOLEXEC:
			TASKDEBUG("MOLEXEC p_nr=%d p_endpoint=%d\n",
				spin_ptr->M3_P_NR,spin_ptr->M3_ENDPT);
			/* Verify if the node that inform the EXEC is the owner of the slot */
			if( (spin_ptr->M3_P_NR+dc_ptr->dc_nr_tasks) >= dc_ptr->dc_nr_sysprocs) {
				i = (spin_ptr->M3_P_NR+dc_ptr->dc_nr_tasks);
				if( slot[i].s_owner != spin_ptr->m_source)
				TASKDEBUG("ERROR: node %d is not the owner of slot %d (owner=%d)\n!",
					spin_ptr->m_source, i, slot[i].s_owner);
				ERROR_RETURN(EMOLBADOWNER);	
			}
			proc_ptr = PROC2PTR(spin_ptr->M3_P_NR);
			/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
			/* 			SIN TERMINAR 				*/
			/* falta crear una mnx_call que permita cambiar el  	*/
			/* nombre de un proceso u otro objeto			*/
			/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
			break;
		case MOLMIGRPROC:
			/*-----------------------------------------------------------------------
			* m_source= nodeid
			* M3_P_NR - M3_P_NR: p_nr
			*-----------------------------------------------------------------------*/		
			TASKDEBUG("MOLMIGRPROC p_nr=%d p_endpoint=%d\n",
				spin_ptr->M3_P_NR,spin_ptr->M3_ENDPT);
				
			i = (spin_ptr->M3_P_NR+dc_ptr->dc_nr_tasks);
			slot[i].s_owner = spin_ptr->m_source;
			
			proc_ptr = PROC2PTR(spin_ptr->M3_P_NR);

			/* Get info about the process from the KERNEL */

#ifdef ALLOC_LOCAL_TABLE 			
			ret = mnx_getprocinfo(dc_ptr->dc_dcid, spin_ptr->M3_P_NR, proc_ptr);
			if( ret < 0) ERROR_RETURN(ret);
#else /* ALLOC_LOCAL_TABLE */			
			proc_ptr = (proc_usr_t *) PROC_MAPPED(spin_ptr->M3_P_NR);	
			ret = OK;
#endif /* ALLOC_LOCAL_TABLE */
			
			/* Check if the slot is free  */
			if( proc_ptr->p_rts_flags != SLOT_FREE ){
				if( TEST_BIT(proc_ptr->p_misc_flags, MIS_BIT_REPLICATED)){
					TASKDEBUG("REPLICATED dcid=%d name=%s p_nr=%d p_endpoint=%d nodeid=%d\n",
						dc_ptr->dc_dcid,spin_ptr->M3_NAME, spin_ptr->M3_P_NR
						,spin_ptr->M3_ENDPT, spin_ptr->m_source);					
					return(OK);
				}else{
					ERROR_RETURN(EMOLSLOTUSED);
				}
			}
			
			if( proc_ptr->p_nodeid == spin_ptr->m_source) {
				TASKDEBUG("WARNING: destination p_nodeid %d same as source node %d\n", 
					proc_ptr->p_nodeid, spin_ptr->m_source);
				return(OK);
			}
			
			TASKDEBUG("BEFORE " PROC_USR_FORMAT,PROC_USR_FIELDS(proc_ptr));
			ret = mnx_migr_start(dc_ptr->dc_dcid, proc_ptr->p_endpoint);
			if(ret != OK)  ERROR_RETURN(ret);
			
			ret = mnx_migr_commit(PROC_NO_PID, dc_ptr->dc_dcid,  proc_ptr->p_endpoint, spin_ptr->m_source);
			if(ret != OK)  ERROR_RETURN(ret);
			
			/* Get info about the process from the KERNEL */
#ifdef ALLOC_LOCAL_TABLE 			
			ret = mnx_getprocinfo(dc_ptr->dc_dcid, spin_ptr->M3_P_NR, proc_ptr);
			if( ret < 0) ERROR_RETURN(ret);
#else /* ALLOC_LOCAL_TABLE */			
			proc_ptr = (proc_usr_t *) PROC_MAPPED(spin_ptr->M3_P_NR);
			ret = OK;	
#endif /* ALLOC_LOCAL_TABLE */			
			TASKDEBUG("AFTER " PROC_USR_FORMAT,PROC_USR_FIELDS(proc_ptr));

			break;
		case MOLEXIT:
			TASKDEBUG("MOLEXIT p_nr=%d p_endpoint=%d\n",
				spin_ptr->M3_P_NR,spin_ptr->M3_ENDPT);
			i = (spin_ptr->M3_P_NR+dc_ptr->dc_nr_tasks);

			/* Verify if the node that inform the EXIT is the owner of the slot */
			if( i >= dc_ptr->dc_nr_sysprocs) {
				if( slot[i].s_owner != spin_ptr->m_source) {
					TASKDEBUG("WARNING: node %d is not the owner of slot %d (owner=%d)\n!",
						spin_ptr->m_source, i, slot[i].s_owner);
						return(OK);
				}
			}
			proc_ptr = PROC2PTR(spin_ptr->M3_P_NR);
			TASKDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(proc_ptr))

			/* Check if the slot is free  */
			if( proc_ptr->p_rts_flags == SLOT_FREE ){
				TASKDEBUG("WARNING: slot is FREE\n");
				ERROR_RETURN(EMOLPROCSTS);
			}
			
			if( TEST_BIT(proc_ptr->p_misc_flags, MIS_BIT_REPLICATED)){
				TASKDEBUG("REPLICATED dcid=%d name=%s p_nr=%d p_endpoint=%d nodeid=%d\n",
					dc_ptr->dc_dcid,spin_ptr->M3_NAME, spin_ptr->M3_P_NR
					,spin_ptr->M3_ENDPT, spin_ptr->m_source);					
				return(OK);
			}

			if( proc_ptr->p_nodeid != spin_ptr->m_source) {
				TASKDEBUG("WARNING: p_nodeid=%d \n", proc_ptr->p_nodeid);
				ERROR_RETURN(EMOLBADNODEID);
			}
						
			/* DeRegister  the remote process to the kernel */
			ret = mnx_unbind(dc_ptr->dc_dcid,spin_ptr->M3_ENDPT);
			if(ret != OK)  ERROR_RETURN(ret);

			/* Get info about the process from the KERNEL */
#ifdef ALLOC_LOCAL_TABLE 			
			ret = mnx_getprocinfo(dc_ptr->dc_dcid, spin_ptr->M3_P_NR, proc_ptr);
			if( ret < 0) ERROR_RETURN(ret);
#else /* ALLOC_LOCAL_TABLE */			
			proc_ptr = (proc_usr_t *) PROC_MAPPED(spin_ptr->M3_P_NR);	
			ret = OK;			
#endif /* ALLOC_LOCAL_TABLE */
			TASKDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(proc_ptr))
			
			break;
		default:
			ERROR_RETURN(EMOLNOSYS);
		break;
	}

#ifdef ANULADO  			
	
#ifdef ALLOC_LOCAL_TABLE 			
	pm_ptr = &pm_desc;
	ret = mnx_getprocinfo(dc_ptr->dc_dcid, PM_PROC_NR, (long int) pm_ptr);
	if( ret != OK) ERROR_RETURN(ret);
#else /* ALLOC_LOCAL_TABLE */			
	pm_ptr = (proc_usr_t *) PROC_MAPPED(PM_PROC_NR);	
	ret = OK;	
#endif /* ALLOC_LOCAL_TABLE */
	TASKDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(pm_ptr));
	
	if( pm_ptr->p_nodeid == local_nodeid){
		/* Notify PM about REMOTE server processes */
		if( (spin_ptr->M3_P_NR >= PM_PROC_NR) &&
			(spin_ptr->M3_P_NR < sizeof(update_t)) &&
			(!TEST_BIT(proc_ptr->p_misc_flags, MIS_BIT_REPLICATED))) {
			TASKDEBUG("NOTIFY PM about p_nr=%d\n", proc_ptr->p_nr);	
			mnx_ntfy_value(LOCAL_SLOTS, PM_PROC_NR, proc_ptr->p_nr);	
		}
	}
#endif /* ANULADO */

 	return(OK);
}

/*===========================================================================*
 *				slots_read_thread				     *
 *===========================================================================*/

void *slots_read_thread(void *arg)
{
	int rcode, mtype;
	int sp_pid, sp_nr, sp_ep;
	static 	char source[MAX_GROUP_NAME];
	message  sp_msg, *sp_ptr;
	proc_usr_t *proc_ptr;
	slot_t  *sp; 		

	sp_ptr =&sp_msg;

	/*------------------------------------
	 * Bind SLOT THREAD to the DC
	 *------------------------------------*/
	sp_nr = LOCAL_SLOTS;
	TASKDEBUG( "Binding to DC %d with sp_nr=%d\n", dcu.dc_dcid, sp_nr);
//	sp_ep = mnx_tbind(dcu.dc_dcid, sp_nr);
	sp_ep = mnx_replbind(dcu.dc_dcid,(pid_t) syscall (SYS_gettid),sp_nr);
	proc_ptr = PROC2PTR(sp_nr);
	if( sp_ep < EMOLERRCODE) ERROR_EXIT(sp_ep);
	TASKDEBUG( "has bound with p_endpoint=%d \n", sp_ep);

#ifdef ALLOC_LOCAL_TABLE 			
	rcode  = mnx_getprocinfo(dcu.dc_dcid, sp_nr, proc_ptr);
	if( rcode < 0) ERROR_RETURN(rcode );
#else /* ALLOC_LOCAL_TABLE */			
	proc_ptr = (proc_usr_t *) PROC_MAPPED(sp_nr);
	rcode = OK;	
#endif /* ALLOC_LOCAL_TABLE */	
	TASKDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(proc_ptr));

	/* fill slot table with local SLOT thread info */
	sp = &slot[LOCAL_SLOTS+dc_ptr->dc_nr_tasks];
	sp->s_endpoint = LOCAL_SLOTS;
	sp->s_flags = 0;
	sp->s_owner = local_nodeid;
	TASKDEBUG(SLOTS_FORMAT, SLOTS_FIELDS(sp));
	
	while(TRUE){
		slots_loop(&mtype, source);
	}
}

/*===========================================================================*
 *				slots_loop				     *
 * return : service_type
 *
 *===========================================================================*/

int slots_loop(int *mtype, char *source)
{
	char		 sender[MAX_GROUP_NAME];
    char		 target_groups[MAX_MEMBERS][MAX_GROUP_NAME];
   	int		 num_groups;

	int		 service_type;
	int16	 	mess_type;
	int		 endian_mismatch;
	int		 i,j;
	int		 ret, mbr, nr_tmp;
	unsigned long int bm_tmp;

	message  *sp_ptr;
	char  *s_ptr;

	service_type = 0;
	num_groups = -1;

replay:

	ret = SP_receive( sysmbox, &service_type, sender, 100, &num_groups, target_groups,
			&mess_type, &endian_mismatch, sizeof(mess_in), mess_in );
	if( ret < 0 ){
       	if ( (ret == GROUPS_TOO_SHORT) || (ret == BUFFER_TOO_SHORT) ) {
			service_type = DROP_RECV;
            TASKDEBUG("\n========Buffers or Groups too Short=======\n");
            ret = SP_receive( sysmbox, &service_type, sender, 
					MAX_MEMBERS, &num_groups, target_groups,
					&mess_type, &endian_mismatch, sizeof(mess_in), mess_in );
		}
	}

	if (ret < 0 ) {
		SP_error( ret );
		if( ret == EMOLAGAIN){
			ERROR_PRINT(ret);
			sleep(5);
			goto replay;
		}
		ERROR_EXIT(ret);
	}else{
		SP_bytes = ret;
	}

	TASKDEBUG("sender=%s Private_group=%s dc_name=%s service_type=%d SP_bytes=%d \n"
			,sender, Private_group, dc_ptr->dc_name, service_type, SP_bytes);

	sp_ptr = (message *) mess_in;

	pthread_mutex_lock(&sys_mutex);	/* protect global variables */
	if( Is_regular_mess( service_type ) )	{
		mess_in[ret] = 0;
		if     ( Is_unreliable_mess( service_type ) ) {TASKDEBUG("received UNRELIABLE \n ");}
		else if( Is_reliable_mess(   service_type ) ) {TASKDEBUG("received RELIABLE \n");}
		else if( Is_causal_mess(       service_type ) ) {TASKDEBUG("received CAUSAL \n");}
		else if( Is_agreed_mess(       service_type ) ) {TASKDEBUG("received AGREED \n");}
		else if( Is_safe_mess(   service_type ) || Is_fifo_mess(       service_type ) ) {
			TASKDEBUG("message from %s, of type %d, (endian %d) to %d groups (%d bytes)\n",
				sender, mess_type, endian_mismatch, num_groups, ret);

			/*----------------------------------------------------------------------------------------------------
			*   SYS_REQ_SLOTS: A Systask on other node has requested free slots
			*----------------------------------------------------------------------------------------------------*/
			if( mess_type == SYS_REQ_SLOTS ) {
				TASKDEBUG(MSG1_FORMAT, MSG1_FIELDS(sp_ptr));
				ret = sp_req_slots(sp_ptr->m_source, sp_ptr->m_need_slots);
				*mtype = mess_type;;
				/*----------------------------------------------------------------------------------------------------
				*   SYS_DON_SLOTS: A Donation Message has received
				*  If it is the destination systask, it adds the new free slots to its own
				*  If it is not the destination, it register the new slot owner
				*----------------------------------------------------------------------------------------------------*/
			}else if ( mess_type == SYS_DON_SLOTS ) {
				TASKDEBUG(MSGA_FORMAT, MSGA_FIELDS(sp_ptr));
				ret = sp_don_slots(sp_ptr);
				*mtype = mess_type;
				/*----------------------------------------------------------------------------------------------------
				*   SYS_PUT_STATUS: The first process in the group list reply with its
				*  slot descriptor table , copy it into local space
				*----------------------------------------------------------------------------------------------------*/
			}else if ( mess_type == SYS_PUT_STATUS) {
				mbr = get_nodeid(sender);
				if( !TEST_BIT(FSM_state, BIT_INITIALIZED)) {
					primary_mbr = mbr;
				}else {
					if(primary_mbr != mbr) {
						TASKDEBUG("SYS_PUT_STATUS: current primary_mbr=%d differs from new primary_mbr=%d\n"
						,primary_mbr,mbr);
					}
					if(!TEST_BIT(bm_init,mbr)){
						TASKDEBUG("SYS_PUT_STATUS: primary_mbr=%d is not in bm_init=%X\n"
						,mbr, bm_init);
					}
				}
				TASKDEBUG("SYS_PUT_STATUS: primary_mbr=%d table has %d slots\n",mbr, ret/sizeof(slot_t));
				ret = sp_put_status( (char *) &mess_in);
				*mtype = mess_type;
//			}else if ( mess_type == SYS_INITIALIZED)  {
//				TASKDEBUG("SYS_INITIALIZED dcid=%d\n",dcu_primary.dc_dcid);
//				ret = sp_initialized(sp_ptr->m_source);
//				*mtype = mess_type;
			}else if ( mess_type == SYS_MERGE_STATUS ) {
				TASKDEBUG("SYS_MERGE_STATUS dcid=%d\n",dcu_primary.dc_dcid);
				ret = sp_merge_status(get_nodeid(sender), (char *) &mess_in, ret);
				*mtype = mess_type;				
			}else if ( mess_type == SYS_SYSCALL ) {
				TASKDEBUG(MSG3_FORMAT, MSG3_FIELDS(sp_ptr));
				ret = sp_syscall(sp_ptr);
				*mtype = mess_type;
			}else {
				TASKDEBUG("Unknown message type %X\n", mess_type);
				*mtype = mess_type;
				ret = OK;
			}
		}
	}else if( Is_membership_mess( service_type ) )	{
        ret = SP_get_memb_info( mess_in, service_type, &memb_info );
        if (ret < 0) {
			TASKDEBUG("BUG: membership message does not have valid body\n");
           	SP_error( ret );
			pthread_mutex_unlock(&sys_mutex);
         	ERROR_EXIT(ret);
        }

		if  ( Is_reg_memb_mess( service_type ) ) {
			TASKDEBUG("Received REGULAR membership for group %s with %d members, where I am member %d:\n",
				sender, num_groups, mess_type );

			if( Is_caused_join_mess( service_type ) ||
				Is_caused_leave_mess( service_type ) ||
				Is_caused_disconnect_mess( service_type ) ){
				bm_tmp = 0;
				memcpy((void*) sp_members, (void *) target_groups, num_groups*MAX_GROUP_NAME);
				for( i=0; i < num_groups; i++ ){
					mbr = get_nodeid(&sp_members[i][0]);
					TASKDEBUG("\t%s:%d\n", &sp_members[i][0], mbr );
					SET_BIT(bm_tmp, mbr);
//					TASKDEBUG("grp id is %d %d %d\n",memb_info.gid.id[0], memb_info.gid.id[1], memb_info.gid.id[2] );
				}
				TASKDEBUG("num_groups=%d OLD bm_active=%X NEW bm_active=%X\n", 
					num_groups, bm_active, bm_tmp);				
			}
		}

		if( Is_caused_join_mess( service_type ) )	{
			/*----------------------------------------------------------------------------------------------------
			*   JOIN: The group has a new member
			*----------------------------------------------------------------------------------------------------*/
			TASKDEBUG("Due to the JOIN of %s\n",memb_info.changed_member );
			mbr = get_nodeid((char *)  memb_info.changed_member);
			if( !TEST_BIT(dc_ptr->dc_nodes, mbr)){
				SYSERR(EMOLNODCNODE);
				TASKDEBUG("Member %d do not belong to DC=%d\n",
					mbr, dc_ptr->dc_dcid);
			}else{
				assert( num_groups > 0 && num_groups <= dc_ptr->dc_nr_nodes);
				active_nodes = num_groups;
				bm_active = bm_tmp;
				ret = sp_join(mbr);
			}
		}else if( Is_caused_leave_mess( service_type ) 
			||  Is_caused_disconnect_mess( service_type ) ){
			/*----------------------------------------------------------------------------------------------------
			*   LEAVE or DISCONNECT:  A member has left the group
			*----------------------------------------------------------------------------------------------------*/
			TASKDEBUG("Due to the LEAVE or DISCONNECT of %s\n", memb_info.changed_member );
			mbr = get_nodeid((char *)  memb_info.changed_member);
			if( !TEST_BIT(dc_ptr->dc_nodes, mbr)){
				SYSERR(EMOLNODCNODE);
				TASKDEBUG("Member %d do not belong to DC=%d\n",
					mbr, dc_ptr->dc_dcid);
			}else{			
				assert( num_groups > 0 && num_groups <= dc_ptr->dc_nr_nodes);
				active_nodes = num_groups;
				bm_active = bm_tmp;	
				ret = sp_disconnect(mbr);
			}
		}else if( Is_caused_network_mess( service_type ) ){
			/*----------------------------------------------------------------------------------------------------
			*   NETWORK CHANGE:  A network partition or a dead deamon
			*----------------------------------------------------------------------------------------------------*/
			TASKDEBUG("Due to NETWORK change with %u VS sets\n", memb_info.num_vs_sets);
            		num_vs_sets = SP_get_vs_sets_info( mess_in, 
									&vssets[0], MAX_VSSETS, &my_vsset_index );
            if (num_vs_sets < 0) {
				TASKDEBUG("BUG: membership message has more then %d vs sets. Recompile with larger MAX_VSSETS\n", MAX_VSSETS);
				SP_error( num_vs_sets );
				pthread_mutex_unlock(&sys_mutex);
               	ERROR_EXIT( num_vs_sets );
			}
            if (num_vs_sets == 0) {
				TASKDEBUG("BUG: membership message has %d vs_sets\n", 
					num_vs_sets);
				SP_error( num_vs_sets );
				pthread_mutex_unlock(&sys_mutex);
               	ERROR_EXIT( EMOLGENERIC );
			}

			bm_tmp = 0;
			nr_tmp = 0;
            for( i = 0; i < num_vs_sets; i++ )  {
				TASKDEBUG("%s VS set %d has %u members:\n",
					(i  == my_vsset_index) ?("LOCAL") : ("OTHER"), 
					i, vssets[i].num_members );
               	ret = SP_get_vs_set_members(mess_in, &vssets[i], members, MAX_MEMBERS);
               	if (ret < 0) {
					TASKDEBUG("VS Set has more then %d members. Recompile with larger MAX_MEMBERS\n", MAX_MEMBERS);
					SP_error( ret );
					pthread_mutex_unlock(&sys_mutex);
                   	ERROR_EXIT( ret);
              	}

				/*---------------------------------------------
				* get the bitmap of current members
				--------------------------------------------- */
				for( j = 0; j < vssets[i].num_members; j++ ) {
					TASKDEBUG("\t%s\n", members[j] );
					mbr = get_nodeid(members[j]);
					if(TEST_BIT(dc_ptr->dc_nodes, mbr) ){ 
						if(!TEST_BIT(bm_tmp, mbr)) {
							SET_BIT(bm_tmp, mbr);
							nr_tmp++;
						}
					}
				}
			}
			
			TASKDEBUG("OLD bm_active=%X active_nodes=%d\n", bm_active, active_nodes);
			TASKDEBUG("NEW bm_active=%X active_nodes=%d\n", bm_tmp, nr_tmp);

			if( bm_active > bm_tmp) {	/* a NETWORK PARTITION has occurred 	*/
				TASKDEBUG("NETWORK PARTITION has occurred\n");
				active_nodes = nr_tmp; 
				bm_active = bm_tmp;
				assert( active_nodes > 0 && active_nodes <= dc_ptr->dc_nr_nodes);
				sp_net_partition();
			}else{
				if (bm_active < bm_tmp) {	/* a NETWORK MERGE has occurred 		*/
					TASKDEBUG("NETWORK MERGE has occurred\n");
					active_nodes = nr_tmp; 
					bm_active = bm_tmp;
					assert( active_nodes > 0 && active_nodes <= dc_ptr->dc_nr_nodes);
					sp_net_merge();
				}else{
					TASKDEBUG("NETWORK CHANGE with no changed members!! ");
				}
			}
		}else if( Is_transition_mess(   service_type ) ) {
			TASKDEBUG("received TRANSITIONAL membership for group %s\n", sender );
			if( Is_caused_leave_mess( service_type ) ){
				TASKDEBUG("received membership message that left group %s\n", sender );
			}else {
				TASKDEBUG("received incorrecty membership message of type 0x%x\n", service_type );
			}
		} else if ( Is_reject_mess( service_type ) )      {
			TASKDEBUG("REJECTED message from %s, of servicetype 0x%x messtype %d, (endian %d) to %d groups \n(%d bytes): %s\n",
				sender, service_type, mess_type, endian_mismatch, num_groups, ret, mess_in );
		}else {
			TASKDEBUG("received message of unknown message type 0x%x with ret %d\n", service_type, ret);
		}
	}
	pthread_mutex_unlock(&sys_mutex);
	if(ret < 0) ERROR_RETURN(ret);
	return(ret);
}

