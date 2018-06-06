/****************************************************************/
/* 				RADAR							*/
/* RADAR algorithm detects replicated service Primary changes 		*/
/* WARNING!!! : This service can only be run in a node without an		*/
/* instance of the monitored service 						*/
/****************************************************************/
#define _GNU_SOURCE     
#define _MULTI_THREADED
#define  MOL_USERSPACE	1
//#define TASKDBG			1
#define _TABLE
#include "radar.h"
#define TRUE 1

SP_message *sp_ptr;

/*===========================================================================*
 *				   main 				    					 *
 * This program detects node changes of endpoints (svr_ep) produced by a migration or	*
 * Primary crashes through SPREAD . It sets the remote endpoint svr_ep to the	*
* new endpoint location	.										* 
 *===========================================================================*/
//PUBLIC int main(void)
int main (int argc, char *argv[] )
{
	int rcode, mtype;
	static 	char source[MAX_GROUP_NAME];

	if ( argc != 4) {
 	    fprintf( stderr,"Usage: %s <dcid> <svr_ep> <svr_name[6]> \n", argv[0] );
 	    exit(1);
    }
	
	local_nodeid = mnx_getdvsinfo(&dvs);
	if(local_nodeid < 0 )
		ERROR_EXIT(EMOLDVSINIT);
	dvs_ptr = &dvs;
	TASKDEBUG(DVS_USR_FORMAT, DVS_USR_FIELDS(dvs_ptr));
	TASKDEBUG("local_nodeid=%d\n", local_nodeid);
	
	/* get the DC info from kernel */
	dcid = atoi(argv[1]);
	if( dcid < 0 || dcid >= NR_DCS) ERROR_EXIT(EMOLRANGE);
	rcode = mnx_getdcinfo(dcid, &dcu);
	if(rcode <0) ERROR_EXIT(rcode);
	dc_ptr = &dcu;
	
	svr_ep = atoi(argv[2]);
	if( svr_ep >  (dc_ptr->dc_nr_sysprocs - dc_ptr->dc_nr_tasks)||  
		(svr_ep < (-dc_ptr->dc_nr_tasks))){
 	    fprintf( stderr,"Usage: svr_ep=%d be lower than = %d\n", svr_ep,
			(dc_ptr->dc_nr_sysprocs - dc_ptr->dc_nr_tasks), 
			(-dc_ptr->dc_nr_tasks));
 	    exit(1);		
	}
	
	if( strlen(argv[3]) > (MAXPROCNAME-1)){
 	    fprintf( stderr,"Usage: svr_name must have less than %d chars\n", MAXPROCNAME-1);
 	    exit(1);
    }
	strncpy(svr_name, argv[3], MAXPROCNAME-1);
	
	TASKDEBUG("RADAR: dcid=%d svr_ep=%d svr_name=%s\n", dcid, svr_ep, svr_name);

	rcode = init_radar();
	if(rcode) ERROR_EXIT(rcode); 

	while(TRUE){
		rcode = radar_loop(&mtype, source);
		if(rcode) {
			sleep(RADAR_ERROR_SPEEP);
			if( rcode == EMOLNOTCONN) {
				rcode = init_radar();
			}
		}
	}
	
  return(OK);				
}

/*===========================================================================*
 *				init_radar				     
 * It connects RADAR to the SPREAD daemon and initilize several local 
 * and replicated  variables
 *===========================================================================*/
int init_radar(void)
{
	int rcode;
#ifdef SPREAD_VERSION
    int     mver, miver, pver;
#endif
    sp_time test_timeout;

    test_timeout.sec = RADAR_TIMEOUT_SEC;
    test_timeout.usec = RADAR_TIMEOUT_MSEC;

#ifdef SPREAD_VERSION
    rcode = SP_version( &mver, &miver, &pver);
	if(!rcode)
        {
		SP_error (rcode);
	  	SYSERR(rcode);
	  	ERROR_EXIT(rcode);
	}
	TASKDEBUG("Spread library version is %d.%d.%d\n", mver, miver, pver);
#else
    TASKDEBUG("Spread library version is %1.2f\n", SP_version() );
#endif
	/*------------------------------------------------------------------------------------
	* mbr_name:  it must be unique in the spread node.
	*  RADARlocal_nodeid.dcid
	*--------------------------------------------------------------------------------------*/
	sprintf( spread_group, "%s%02d", svr_name, dc_ptr->dc_dcid);
	TASKDEBUG("spread_group=%s\n", spread_group);
	sprintf( Spread_name, "4803");
	sprintf( mbr_name, "%s%0d.%0d", svr_name, local_nodeid, dc_ptr->dc_dcid);
	TASKDEBUG("mbr_name=%s\n", mbr_name);

	rcode = SP_connect_timeout( Spread_name, mbr_name , 0, 1, 
				&rdr_mbox, Private_group, test_timeout );
	if( rcode != ACCEPT_SESSION ) 	{
		SP_error (rcode);
		ERROR_EXIT(rcode);
	}
	TASKDEBUG("mbr_name %s: connected to %s with private group %s\n",
			mbr_name , Spread_name, Private_group);

	rcode = SP_join( rdr_mbox, spread_group);
	if( rcode){
		SP_error (rcode);
 		ERROR_EXIT(rcode);
	}

	FSM_state 	= STS_NEW;	
	primary_mbr = NO_PRIMARY;
	synchronized = FALSE;
	
	bm_nodes 	= 0;			/* initialize  conected  members'  bitmap */
	bm_sync   	= 0;			/* initialize  synchoronized  members'  bitmap */
	
	nr_nodes 	= 0;
	nr_sync  	= 0;
	
	return(OK);
}

/*===========================================================================*
 *				radar_loop				    
 * return : service_type
 *===========================================================================*/

int radar_loop(int *mtype, char *source)
{
	char	sender[MAX_GROUP_NAME];
	char	target_groups[MAX_MEMBERS][MAX_GROUP_NAME];
	int		num_groups;

	int		service_type;
	int16	mess_type;
	int		endian_mismatch;
	int		i,j;
	int		ret, mbr;

	service_type = 0;
	num_groups = -1;

	ret = SP_receive( rdr_mbox, &service_type, sender, 100, &num_groups, target_groups,
			&mess_type, &endian_mismatch, sizeof(mess_in), mess_in );
	
	
	if( ret < 0 ){
       	if ( (ret == GROUPS_TOO_SHORT) || (ret == BUFFER_TOO_SHORT) ) {
			service_type = DROP_RECV;
            TASKDEBUG("\n========Buffers or Groups too Short=======\n");
            ret = SP_receive( rdr_mbox, &service_type, sender, 
					MAX_MEMBERS, &num_groups, target_groups,
					&mess_type, &endian_mismatch, sizeof(mess_in), mess_in );
		}
	}

	if (ret < 0 ) {
		SP_error( ret );
		ERROR_EXIT(ret);
	}

	TASKDEBUG("sender=%s Private_group=%s dc_name=%s service_type=%d\n"
			,sender, Private_group, dc_ptr->dc_name, service_type);

	sp_ptr = (SP_message *) mess_in;
	
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
			*   MC_STATUS_INFO		The PRIMARY has sent MC_STATUS_INFO message 
			*----------------------------------------------------------------------------------------------------*/
			if ( mess_type == MC_STATUS_INFO ) {
				ret = mc_status_info(sp_ptr);
				*mtype = mess_type;
			/*----------------------------------------------------------------------------------------------------
			*   MC_SYNCHRONIZED		The new member inform that it is SYNCHRONIZED
			*----------------------------------------------------------------------------------------------------*/
			}else if ( mess_type == MC_SYNCHRONIZED ) {
				ret = mc_synchronized(sp_ptr);
				*mtype = mess_type;
			} else {
				TASKDEBUG("Ignored message type %X\n", mess_type);
				*mtype = mess_type;
				ret = OK;
			}
		}
	}else if( Is_membership_mess( service_type ) )	{
        ret = SP_get_memb_info( mess_in, service_type, &memb_info );
        if (ret < 0) {
			TASKDEBUG("BUG: membership message does not have valid body\n");
           	SP_error( ret );
         	ERROR_EXIT(ret);
        }

		if  ( Is_reg_memb_mess( service_type ) ) {
			TASKDEBUG("Received REGULAR membership for group %s with %d members, where I am member %d:\n",
				sender, num_groups, mess_type );

			if( Is_caused_join_mess( service_type ) ||
				Is_caused_leave_mess( service_type ) ||
				Is_caused_disconnect_mess( service_type ) ){
				sp_nr_mbrs = num_groups;
				memcpy((void*) sp_members, (void *) target_groups, sp_nr_mbrs*MAX_GROUP_NAME);
				for( i=0; i < sp_nr_mbrs; i++ ){
					TASKDEBUG("\t%s nodeid=%d\n", &sp_members[i][0],  get_nodeid(&sp_members[i][0]) );
//					TASKDEBUG("grp id is %d %d %d\n",memb_info.gid.id[0], memb_info.gid.id[1], memb_info.gid.id[2] );
				}
			}
		}

		if( Is_caused_join_mess( service_type ) )	{
			/*----------------------------------------------------------------------------------------------------
			*   JOIN: The group has a new member
			*----------------------------------------------------------------------------------------------------*/
			TASKDEBUG("Due to the JOIN of %s service_type=%d\n", 
				memb_info.changed_member, service_type );
			mbr = get_nodeid((char *)  memb_info.changed_member);
			nr_nodes = num_groups;
			ret = sp_join(mbr);
		}else if( Is_caused_leave_mess( service_type ) 
			||  Is_caused_disconnect_mess( service_type ) ){
			/*----------------------------------------------------------------------------------------------------
			*   LEAVE or DISCONNECT:  A member has left the group
			*----------------------------------------------------------------------------------------------------*/
			TASKDEBUG("Due to the LEAVE or DISCONNECT of %s\n", 
				memb_info.changed_member );
			mbr = get_nodeid((char *) memb_info.changed_member);
			nr_nodes = num_groups;
			ret = sp_disconnect(mbr);
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
               	ERROR_EXIT( num_vs_sets );
			}
            if (num_vs_sets == 0) {
				TASKDEBUG("BUG: membership message has %d vs_sets\n", 
					num_vs_sets);
				SP_error( num_vs_sets );
               	ERROR_EXIT( EMOLGENERIC );
			}

			bm_nodes = 0;
			nr_nodes = 0;
            for( i = 0; i < num_vs_sets; i++ )  {
				TASKDEBUG("%s VS set %d has %u members:\n",
					(i  == my_vsset_index) ?("LOCAL") : ("OTHER"), 
						i, vssets[i].num_members );
               	ret = SP_get_vs_set_members(mess_in, &vssets[i], members, MAX_MEMBERS);
               	if (ret < 0) {
					TASKDEBUG("VS Set has more then %d members. Recompile with larger MAX_MEMBERS\n", MAX_MEMBERS);
					SP_error( ret );
                   	ERROR_EXIT( ret);
              	}

				/*---------------------------------------------
				* get the bitmap of current members
				--------------------------------------------- */
				for( j = 0; j < vssets[i].num_members; j++ ) {
					TASKDEBUG("\t%s\n", members[j] );
					mbr = get_nodeid(members[j]);
					if(!TEST_BIT(bm_nodes, mbr)) {
						SET_BIT(bm_nodes, mbr);
						nr_nodes++;
					}
				}
				TASKDEBUG("old bm_sync=%X bm_nodes=%X primary_mbr=%d\n",
					bm_sync, bm_nodes, primary_mbr);
			}
			if( bm_sync > bm_nodes) {		/* a NETWORK PARTITION has occurred 	*/
				sp_net_partition();
			}else{
				if (bm_sync < bm_nodes) {	/* a NETWORK MERGE has occurred 		*/
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
	if(ret < 0) ERROR_RETURN(ret);
	return(ret);
}

/***************************************************************************/
/* FUNCTIONS TO DEAL WITH SPREAD MESSAGES - MEMBERSHIP		*/
/***************************************************************************/

/*===========================================================================*
 *				sp_join									*
 * A NEW member has joint the DC group 						*
 *===========================================================================*/
int sp_join( int new_mbr)
{
	int rcode = OK;
	TASKDEBUG("new_member=%d primary_mbr=%d nr_nodes=%d\n", 
		new_mbr, primary_mbr, nr_nodes);
	if( new_mbr == local_nodeid){
		if( nr_nodes == 1) {
			nr_nodes = 0;
			bm_nodes = 0;
			synchronized = FALSE;
			rcode = SP_disconnect(rdr_mbox);
			if(rcode) SP_error (rcode);
			rcode = EMOLNOTCONN;
		} else {
			assert( FSM_state == STS_NEW);
			FSM_state = STS_WAIT4PRIMARY;
		}
	} else { 
		if( FSM_state == STS_WAIT4PRIMARY) {
			nr_nodes = 0;
			bm_nodes = 0;
			synchronized = FALSE;
			rcode = SP_disconnect(rdr_mbox);
			if(rcode) SP_error (rcode);
			rcode = EMOLNOTCONN;			
		}
	}
	return(rcode);
}

/*===========================================================================*
 *				sp_disconnect								*
 * Primary could be the disconnected member							*
 ===========================================================================*/
int sp_disconnect(int  disc_mbr)
{
	int rcode;
	
	TASKDEBUG("disc_mbr=%d\n",disc_mbr);

	if(local_nodeid == disc_mbr) {
		ERROR_EXIT(EMOLNOTCONN);
	}

	/* If RADAR is not synchronized: ignere this message	*/
	if(FSM_state   != STS_SYNCHRONIZED) return(OK);

	CLR_BIT(bm_nodes, disc_mbr);
	TASKDEBUG("CLR_BIT bm_nodes=%d, disc_mbr=%d\n",bm_nodes, disc_mbr);
	
	TASKDEBUG("bm_sync=%d, disc_mbr=%d, nr_sync=%d\n",bm_sync, disc_mbr, nr_sync);
	/* if the dead node was synchronized */
	if( TEST_BIT(bm_sync, disc_mbr)) {
		nr_sync--;	/* decrease the number of synchronized nodes */
		CLR_BIT(bm_sync, disc_mbr);
	}
	TASKDEBUG("primary_mbr=%d, disc_mbr=%d\n",primary_mbr, disc_mbr);
	TASKDEBUG("bm_sync=%d, disc_mbr=%d, nr_sync=%d\n",bm_sync, disc_mbr, nr_sync);
	
	/* if the dead node was the primary_mbr, search another primary_mbr */
	if( primary_mbr == disc_mbr) {
		TASKDEBUG("primary_mbr=%d, disc_mbr=%d\n",primary_mbr, disc_mbr);
		if( bm_sync == 0) {
			TASKDEBUG("THERE IS NO PRIMARY:\n");
			FSM_state = STS_NEW;
			synchronized = FALSE;
			nr_nodes = 0;
			bm_nodes = 0;
			synchronized = FALSE;
			rcode = SP_disconnect(rdr_mbox);
			if(rcode) SP_error (rcode);	
			rcode = mnx_unbind(dc_ptr->dc_dcid,svr_ep);
			TASKDEBUG("mnx_unbind(%d,%d) rcode=%d\n",
					dc_ptr->dc_dcid,svr_ep,rcode);
			if( rcode)	return(rcode);
			return(EMOLNOTCONN);
		} else { 	
			FSM_state = STS_WAIT4PRIMARY;
			primary_mbr = NO_PRIMARY;
			rcode = mnx_migr_start(dc_ptr->dc_dcid, svr_ep);
			TASKDEBUG("mnx_migr_start(%d,%d) rcode=%d\n",
						dc_ptr->dc_dcid, svr_ep,rcode);
			if(rcode) return(rcode);
		}
	}
	
	TASKDEBUG("disc_mbr=%d nr_nodes=%d\n",	disc_mbr, nr_nodes);
	TASKDEBUG("primary_mbr=%d nr_sync=%d\n", primary_mbr, nr_sync);
	TASKDEBUG("bm_nodes=%X bm_sync=%X \n", bm_nodes,bm_sync);
	
	return(OK);
}
/***************************************************************************/
/* FUNCTIONS TO DEAL WITH  REPLICATION PROTOCOL  MESSAGES 		*/
/***************************************************************************/

/*===========================================================================*
 *				mc_status_info								     	*
 * The PRIMARY member sent a multicast to no sync members					*
 ===========================================================================*/
int mc_status_info(	message  *sp_ptr)
{
	int rcode;

	primary_mbr = sp_ptr->m_source;
	TASKDEBUG("mc_status_info Primary_mbr=%d\n", primary_mbr);
	
	if( FSM_state != STS_WAIT4PRIMARY ) return (OK);

	if (nr_nodes != sp_ptr->m2_i1){
		TASKDEBUG("Received nr_nodes=%d don't match local nr_nodes=%d\n"
			, sp_ptr->m2_i1, nr_nodes);
		SP_error(EMOLBADVALUE);
		ERROR_EXIT(EMOLBADVALUE);
	}
	nr_sync 	= sp_ptr->m2_i2;
	bm_nodes 	= sp_ptr->m2_l1;
	bm_sync		= sp_ptr->m2_l2;
	FSM_state   = STS_SYNCHRONIZED;
	if( primary_mbr != local_nodeid) {
		if( synchronized == FALSE){
			rcode = mnx_rmtbind(dc_ptr->dc_dcid,svr_name,svr_ep,primary_mbr);
			TASKDEBUG("mnx_rmtbind(%d,%s, %d,%d) rcode=%d\n",
						dc_ptr->dc_dcid, svr_name, svr_ep, primary_mbr,rcode);
			if( rcode == EMOLSLOTUSED) {
				rcode = mnx_unbind(dc_ptr->dc_dcid,svr_ep);
				if(rcode < 0) ERROR_EXIT(rcode);
				rcode = mnx_rmtbind(dc_ptr->dc_dcid,svr_name,svr_ep,primary_mbr);	
				if(rcode < 0) ERROR_EXIT(rcode);
			}
			if(rcode < 0) ERROR_EXIT(rcode);
			synchronized = TRUE;
		}else{
			rcode = mnx_migr_commit( (-1), dc_ptr->dc_dcid, svr_ep, primary_mbr);
			TASKDEBUG("mnx_migr_commit(-1,%d,%d,%d) rcode=%d\n",
						dc_ptr->dc_dcid, svr_ep, primary_mbr,rcode);
			if(rcode) ERROR_EXIT(rcode);
			TASKDEBUG("THE NEW primary_mbr=%d\n", primary_mbr);				
		}
	}else{
 	    fprintf( stderr,"There is a server process %d of DC=% in this node=%d !!\n",
				svr_ep,	dc_ptr->dc_dcid, local_nodeid);
 	    ERROR_EXIT(EMOLPRIMARY);		
	}

	return(OK);	
}

/*=========================================================================*
 *				mc_synchronized							     	*
 * A new sync member inform about it to other sync	members					*
 ===========================================================================*/
int mc_synchronized(  message  *sp_ptr)
{
	TASKDEBUG(MSG2_FORMAT, MSG2_FIELDS(sp_ptr));
	if( FSM_state   != STS_SYNCHRONIZED ) return (OK);
	
	nr_sync++;
	SET_BIT(bm_sync, sp_ptr->m_source);
	TASKDEBUG("New sync mbr=%d bm_sync=%X\n", 
		sp_ptr->m_source , bm_sync);
	return(OK);	
}
				
/*----------------------------------------------------------------------------------------------------
*				sp_net_partition
*  A network partition has occurred
*----------------------------------------------------------------------------------------------------*/
int sp_net_partition(void)
{
	TASKDEBUG("\n");
	
#ifdef ANULADO
	if( FSM_state   != STS_SYNCHRONIZED )
		return(OK);

	TASKDEBUG("bm_sync=%X bm_nodes=%X\n",bm_sync, bm_nodes);
	/* mask the old init members bitmap with the mask of active members */
	/* only the active nodes should be considered synchronized */
	bm_sync &= bm_nodes;

	/* is this the primary_mbr partition (where the old primary_mbr is located) ? */
	if( TEST_BIT(bm_sync, first_act_mbr)){
		/* primary_mbr of this partition */
		primary_mbr = first_act_mbr;
	}else{
		/* get the first init member of this partition */
		primary_mbr = NO_FIRST_INIT_MBR;
		for( i = 0; i < num_vs_sets; i++ )  {
			for( j = 0; j < vssets[i].num_members; j++ ) {
				TASKDEBUG("\t%s\n", members[j] );
				mbr = get_nodeid(members[j]);
				if(!TEST_BIT(bm_sync, mbr)) {
					first_act_mbr = mbr;
					break;
				}
			}
			if(first_act_mbr != NO_FIRST_INIT_MBR)
				break;
		}
		if(first_act_mbr == NO_FIRST_INIT_MBR){
			TASKDEBUG("Can't find primary_mbr of this partition %d\n",primary_mbr);
			return(EMOLBADNODEID);
		}
	}

	if( bm_acks != 0) { /* is this member waiting for donors responses ? */
		bm_acks  &= bm_nodes;
		if( bm_acks == 0) {
			FSM_state = STS_SYNCHRONIZED;
			if (FSM_state == STS_REQ_SLOTS && owned_slots > 0)
				pthread_cond_signal(&fork_barrier);
		}
	}

	total_slots = 0;
	for( i = dc_ptr->dc_nr_sysprocs; i < (dc_ptr->dc_nr_tasks + dc_ptr->dc_nr_procs);i++) {
		/* restore slots from uncompleted donations */
		if( proc[i].p_rts_flags == SLOT_FREE) {
			if ( (TEST_BIT(proc[i].p_rts_flags, BIT_DONATING)) &&
				 (!TEST_BIT(bm_sync, slot[i].s_owner))) { /*The destination is not on my partition */
				free_slots++;
				owned_slots++;
				CLR_BIT(proc[i].p_rts_flags, BIT_DONATING);
				TASKDEBUG("Restoring slot %d from uncompleted donation after a network partition\n",i);
			}
		}
		if(TEST_BIT(bm_sync, slot[i].s_owner)) { /* The owner is on my partition */
			total_slots++;
		}
	}

	nr_sync = 0;
 	for( i =0; i < (sizeof(unsigned long int) * 8); i++){
		if( TEST_BIT(bm_sync, i)){
			nr_sync++;
		}
	}

	TASKDEBUG("primary_mbr=%d bm_sync=%X bm_acks=%X\n",
		primary_mbr, bm_sync, bm_acks);

	/* recalculate global variables */
	max_owned_slots	= (total_slots - (min_owned_slots*nr_sync));
	TASKDEBUG("total_slots=%d max_owned_slots=%d nr_sync=%d bm_sync=%X\n",
		total_slots,max_owned_slots, nr_sync,bm_sync);

	if( local_nodeid == primary_mbr){ 	/* this member is the primary_mbr */
		send_status_info();		/* broadcast partition status to waiting to initialize members */
	}
#endif /* ANULADO */
	return(OK);
}

/*----------------------------------------------------------------------------------------------------
*				sp_net_merge
*  A network merge has occurred
*  the first synchronized member of each merged partition
*  broadcast its current process slot table (PST)
*  only filled with the slots owned by members of their partition
*----------------------------------------------------------------------------------------------------*/
int sp_net_merge(void)
{
	TASKDEBUG("\n");
	
#ifdef ANULADO 

	if( FSM_state   != STS_SYNCHRONIZED )
		return(OK);

	/* Only executed this function the first init members  of all partitions */
	if ( primary_mbr != local_nodeid) 
		return(OK);

	/*------------------------------------
	* Alloc memory for temporal  PST
 	*------------------------------------*/
//	slot_part = (slot_t *) malloc( sizeof(slot_t)  * (dcu.dc_nr_procs+dcu.dc_nr_tasks));
	posix_memalign( (void**) &slot_part, getpagesize(), sizeof(slot_t)  * (dcu.dc_nr_procs+dcu.dc_nr_tasks));
	if(slot_part == NULL) return (EMOLNOMEM);

	/* Copy the PST to temporar PST */
	memcpy( (void*) slot_part, slot, (sizeof(slot_t) * (dcu.dc_nr_procs+dcu.dc_nr_tasks)));

	/* fill the slot table with endpoint and name				*/
	total_slots = 0;
	for( i = dc_ptr->dc_nr_sysprocs; i < (dcu.dc_nr_procs+dcu.dc_nr_tasks); i++) {
		/* bm_ init keeps the synchronized members before MERGE */
		if( !TEST_BIT(bm_sync, slot[i].s_owner)) { /* the owner is not on this partition */
			TASKDEBUG("slot reserved %d for owner=%d\n", i, slot_part[i].s_owner);
			slot_part[i].s_owner = NO_FIRST_INIT_MBR; /* this partition does not own this slot*/
		}else{
			if( proc[i].p_rts_flags != SLOT_FREE){
				slot_part[i].s_endpoint = proc[i].p_endpoint;
				strncpy(slot_part[i].s_name, proc[i].p_name, (MAXPROCNAME-1));
			} else {
				slot_part[i].s_endpoint = NONE;
			}
			total_slots++;
		}
	}

	/* broadcast the partition PST */
	TASKDEBUG("Send the partition's PST to all members \n");
	rcode = SP_multicast (rdr_mbox, FIFO_MESS, (char *) dc_ptr->dc_name,
			SYS_MERGE_PST,
			(sizeof(slot_t) * ((dcu.dc_nr_procs+dcu.dc_nr_tasks)-dc_ptr->dc_nr_sysprocs)),
			(char*) &slot_part[dc_ptr->dc_nr_sysprocs]);

    free( (void*)slot_part);

	if(rcode <0) ERROR_RETURN(rcode);
#endif /* ANULADO */

	return(OK);
}

/***************************************************************************/
/* AUXILIARY FUNCTIONS 										*/
/***************************************************************************/

/*===========================================================================*
 *				get_nodeid				     *
 * It converts a node string provided by SPREAD into a node ID
 *===========================================================================*/
int get_nodeid(char *mbr_string)
{
	char *s_ptr;
	int nid;

	s_ptr = strchr(&mbr_string[6], '.'); /* locate the dot character after "#RDISK" */
	if(s_ptr != NULL)
		*s_ptr = '\0';
	s_ptr = &mbr_string[6];
	nid = atoi(s_ptr);
	TASKDEBUG("member=%s nodeid=%d\n", mbr_string,  nid );
	return(nid);
}

