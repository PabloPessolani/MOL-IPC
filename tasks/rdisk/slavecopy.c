/****************************************************************/
/****************************************************************/
/* 				SLAVE_COPY										*/
/* SLAVE_COPY algorithm routines for intra-nodes RDISKs			*/
/****************************************************************/
#define _MULTI_THREADED
#define _GNU_SOURCE     
#define  MOL_USERSPACE	1
//#define TASKDBG		1

#include "rdisk.h"
#include <sys/syscall.h>

#define TRUE 1
#define WAIT4BIND_MS 1000


// extern struct driver *m_dtab;

// SP_message sp_ptr1; /*message to m_transfer*/	

// SP_message *sp_ptr;
// message *mnx_ptr;
// unsigned *data_ptr;

//int rcode;

/*===========================================================================*
 *				init_slavecopy				     
 * It connects REPLICATE thread to the SPREAD daemon and initilize several local 
 * and replicated  var2iables
 *===========================================================================*/
int init_slavecopy(void)
{
	int rcode;

	TASKDEBUG("Initializing SLAVE_COPY\n"); 
	return(OK);
}

/*===========================================================================*
 *				init_m3ipc					     *
 *===========================================================================*/
void init_m3ipc(void)
{
	int rcode;

		/*mail 230216*/
	
	// sc_lpid = syscall(SYS_gettid); 	
	// if( sc_lpid < 0) ERROR_EXIT(sc_lpid);
	// TASKDEBUG("SLAVE COPY SERVER %d\n", sc_lpid);

	// /* Bind SC to the kernel */
	// TASKDEBUG("Binding process lpid=%d to VM%d with p_nr=%d\n",sc_lpid,dcid,(RDISK_SLAVE+20));
	// sc_ep = mnx_bind(dcid, (RDISK_SLAVE));
	// if(sc_ep < 0) ERROR_EXIT(sc_ep);
	// TASKDEBUG("sc_ep=%d\n", sc_ep);

	// /* Register into SYSTASK (as an autofork) */
	// TASKDEBUG("Register SC into SYSTASK sc_lpid=%d\n",sc_lpid);
	// rcode = sys_bindproc((RDISK_SLAVE), sc_lpid, LCL_BIND);
	// if(rcode != sc_ep) ERROR_EXIT(rcode);

	// /* Bind SC to the PM */
	// sc_pid = mol_bindproc((RDISK_SLAVE), PROC_NO_PID, sc_lpid);

	// TASKDEBUG("SC MINIX PID=%d\n",sc_pid);
	// if( sc_pid < 0) ERROR_EXIT(sc_pid);
	
	/*mail 230216*/
	
	
	
	TASKDEBUG("Binding RDISK SLAVE=%d\n", RDISK_SLAVE);
	sc_ep = mnx_tbind(dcid, RDISK_SLAVE);
	if(sc_ep < 0) ERROR_EXIT(sc_ep);
	TASKDEBUG("sc_ep=%d\n", sc_ep);
	
	do {
		rcode = mnx_wait4bind_T(WAIT4BIND_MS);
		TASKDEBUG("SLAVE mnx_wait4bind_T  rcode=%d\n", rcode);
		if (rcode == EMOLTIMEDOUT) {
			TASKDEBUG("CLIENT mnx_wait4bind_T TIMEOUT\n");
			continue ;
		} else if ( rcode < 0)
			ERROR_EXIT(EXIT_FAILURE);
	} while	(rcode < OK);

	
	// clt_lpid = getpid();
	// do {
		// rcode = mnx_wait4bind_T(WAIT4BIND_MS);
		// TASKDEBUG("CLIENT mnx_wait4bind_T  rcode=%d\n", rcode);
		// if (rcode == EMOLTIMEDOUT) {
			// TASKDEBUG("CLIENT mnx_wait4bind_T TIMEOUT\n");
			// continue ;
		// } else if ( rcode < 0)
			// ERROR_EXIT(EXIT_FAILURE);
	// } while	(rcode < OK);

	// TASKDEBUG("Get the DVS info from SYSTASK\n");
	// rcode = sys_getkinfo(&drvs);
	// if (rcode) ERROR_EXIT(rcode);
	// drvs_ptr = &drvs;
	// TASKDEBUG(DVS_USR_FORMAT, DVS_USR_FIELDS(drvs_ptr));

	// TASKDEBUG("Get the VM info from SYSTASK\n");
	// rcode = sys_getmachine(&vmu);
	// if (rcode) ERROR_EXIT(rcode);
	// dc_ptr = &vmu;
	// TASKDEBUG(DC_USR_FORMAT, DC_USR_FIELDS(dc_ptr));

	// TASKDEBUG("Get RDISK info from SYSTASK\n");
	// rcode = sys_getproc(&rd, RDISK_PROC_NR);
	// if (rcode) ERROR_EXIT(rcode);
	// rd_ptr = &rd;
	// TASKDEBUG(PROC_USR_FORMAT, PROC_USR_FIELDS(rd_ptr));
	// if ( TEST_BIT(rd_ptr->p_rts_flags, BIT_SLOT_FREE)) {
		// fprintf(stderr, "RDISK not started\n");
		// fflush(stderr);
		// ERROR_EXIT(EMOLNOTBIND);
	// }

	// TASKDEBUG("Get Client info from SYSTASK\n");
	// rcode = sys_getproc(&clt, SELF);
	// if (rcode) ERROR_EXIT(rcode);
	// clt_ptr = &clt;
	// TASKDEBUG(PROC_USR_FORMAT, PROC_USR_FIELDS(clt_ptr));
   return(OK);
}

/*===========================================================================*
 *				slavecopy_main									     *
 *===========================================================================*/

void *slavecopy_main(void *arg)
{
	static 	char source[MAX_GROUP_NAME];
	int rcode, mtype;

	TASKDEBUG("replicate_main dcid=%d local_nodeid=%d\n"
		,dcid, local_nodeid);
	
    init_m3ipc();	

	// while(TRUE){
		// rcode = replica_loop(&mtype, source);
	// }
	return(rcode);
}

/*===========================================================================*
 *				slave_loop				    
 * return : service_type
 *===========================================================================*/

// int slave_loop(int *mtype, char *source)
// {
	// char		 sender[MAX_GROUP_NAME];
    // char		 target_groups[MAX_MEMBERS][MAX_GROUP_NAME];
   	// int		 num_groups;

	// int		 service_type;
	// int16	 mess_type;
	// int		 endian_mismatch;
	// int		 i,j;
	// int		 ret, mbr;
	// // message  *sp_ptr;
	

	// service_type = 0;
	// num_groups = -1;

	// /*si soy el primario, no puedo hacer el signal y continuar?*/
	
	// ret = SP_receive( sysmbox, &service_type, sender, 100, &num_groups, target_groups,
			// &mess_type, &endian_mismatch, sizeof(mess_in), mess_in );
	
	
	// if( ret < 0 ){
       	// if ( (ret == GROUPS_TOO_SHORT) || (ret == BUFFER_TOO_SHORT) ) {
			// service_type = DROP_RECV;
            // TASKDEBUG("\n========Buffers or Groups too Short=======\n");
            // ret = SP_receive( sysmbox, &service_type, sender, 
					// MAX_MEMBERS, &num_groups, target_groups,
					// &mess_type, &endian_mismatch, sizeof(mess_in), mess_in );
		// }
	// }

	// if (ret < 0 ) {
		// SP_error( ret );
		// ERROR_EXIT(ret);
	// }

	// TASKDEBUG("sender=%s Private_group=%s dc_name=%s service_type=%d\n"
			// ,sender, Private_group, dc_ptr->dc_name, service_type);

	// // sp_ptr = (message *) mess_in;
	// sp_ptr = (SP_message *) mess_in;
	// mnx_ptr = (message *) mess_in;
	

	
	// pthread_mutex_lock(&rd_mutex);	/* protect global variables */
	
	// if( Is_regular_mess( service_type ) )	{
		// mess_in[ret] = 0;
		// if     ( Is_unreliable_mess( service_type ) ) {TASKDEBUG("received UNRELIABLE \n ");}
		// else if( Is_reliable_mess(   service_type ) ) {TASKDEBUG("received RELIABLE \n");}
		// else if( Is_causal_mess(       service_type ) ) {TASKDEBUG("received CAUSAL \n");}
		// else if( Is_agreed_mess(       service_type ) ) {TASKDEBUG("received AGREED \n");}
		// else if( Is_safe_mess(   service_type ) || Is_fifo_mess(       service_type ) ) {
			// TASKDEBUG("message from %s, of type %d, (endian %d) to %d groups (%d bytes)\n",
				// sender, mess_type, endian_mismatch, num_groups, ret);

			// /*----------------------------------------------------------------------------------------------------
			// *   DEV_WRITE		The PRIMARY has sent a WRITE request 
			// *----------------------------------------------------------------------------------------------------*/
			// if( mess_type == DEV_WRITE ) {
				// TASKDEBUG(MSG2_FORMAT, MSG2_FIELDS(mnx_ptr));
				// ret = rep_dev_write(sp_ptr);
				// *mtype = mess_type;
			// /*----------------------------------------------------------------------------------------------------
			// *   DEV_SCATTER		The PRIMARY has sent a DEV_SCATTER request 
			// *----------------------------------------------------------------------------------------------------*/
			// }else if( mess_type == DEV_WRITE ) {
				// TASKDEBUG(MSG2_FORMAT, MSG2_FIELDS(mnx_ptr));
				// ret = rep_dev_scatter(sp_ptr);
				// *mtype = mess_type;;
			// /*----------------------------------------------------------------------------------------------------
			// *   A BACKUP member has sent a reply to the PRIMARY
			// *----------------------------------------------------------------------------------------------------*/
			// }else if ( mess_type == MOLTASK_REPLY ) {
				// TASKDEBUG(MSG2_FORMAT, MSG2_FIELDS(mnx_ptr));
				// ret = rep_task_reply(sp_ptr);
				// *mtype = mess_type;
			// /*----------------------------------------------------------------------------------------------------
			// *   DEV_OPEN		The PRIMARY has sent DEV_OPEN request 
			// *----------------------------------------------------------------------------------------------------*/
			// }else if ( mess_type == DEV_OPEN ) {
				// TASKDEBUG(MSG2_FORMAT, MSG2_FIELDS(mnx_ptr));
				// ret = rep_dev_open(sp_ptr);
				// *mtype = mess_type;
						
			// /*----------------------------------------------------------------------------------------------------
			// *   DEV_CLOSE		The PRIMARY has sent DEV_CLOSE request 
			// *----------------------------------------------------------------------------------------------------*/
			// }else if ( mess_type == DEV_CLOSE ) {
				// TASKDEBUG(MSG2_FORMAT, MSG2_FIELDS(mnx_ptr));
				// ret = rep_dev_close(sp_ptr);
				// *mtype = mess_type;
			// /*----------------------------------------------------------------------------------------------------
			// *   DEV_IOCTL		The PRIMARY has sent DEV_IOCTL request 
			// *----------------------------------------------------------------------------------------------------*/
			// }else if ( mess_type == DEV_IOCTL ) {
				// TASKDEBUG(MSG2_FORMAT, MSG2_FIELDS(mnx_ptr));
				// ret = rep_dev_ioctl(sp_ptr);
				// *mtype = mess_type;
			// /*----------------------------------------------------------------------------------------------------
			// *   CANCEL		The PRIMARY has sent CANCEL request 
			// *----------------------------------------------------------------------------------------------------*/
			// }else if ( mess_type == CANCEL ) {
				// TASKDEBUG(MSG2_FORMAT, MSG2_FIELDS(mnx_ptr));
				// ret = rep_cancel(sp_ptr);
				// *mtype = mess_type;
			// /*----------------------------------------------------------------------------------------------------
			// *   SELECT		The PRIMARY has sent SELECT request 
			// *----------------------------------------------------------------------------------------------------*/
			// }else if ( mess_type == CANCEL ) {
				// TASKDEBUG(MSG2_FORMAT, MSG2_FIELDS(mnx_ptr));
				// ret = rep_select(sp_ptr);
				// *mtype = mess_type;
			// /*----------------------------------------------------------------------------------------------------
			// *   MC_STATUS_INFO		The PRIMARY has sent MC_STATUS_INFO message 
			// *----------------------------------------------------------------------------------------------------*/
			// }else if ( mess_type == MC_STATUS_INFO ) {
				// TASKDEBUG(MSG2_FORMAT, MSG2_FIELDS(mnx_ptr));
				// ret = mc_status_info(sp_ptr);
				// *mtype = mess_type;
			// /*----------------------------------------------------------------------------------------------------
			// *   MC_SYNCHRONIZED		The new member inform that it is SYNCHRONIZED
			// *----------------------------------------------------------------------------------------------------*/
			// }else if ( mess_type == MC_SYNCHRONIZED ) {
				// TASKDEBUG(MSG2_FORMAT, MSG2_FIELDS(mnx_ptr));
				// ret = mc_synchronized(sp_ptr);
				// *mtype = mess_type;
			// } else {
				// TASKDEBUG("Unknown message type %X\n", mess_type);
				// TASKDEBUG(MSG2_FORMAT, MSG2_FIELDS(mnx_ptr));
				// *mtype = mess_type;
				// ret = OK;
			// }
		// }
	// }else if( Is_membership_mess( service_type ) )	{
        // ret = SP_get_memb_info( mess_in, service_type, &memb_info );
        // if (ret < 0) {
			// TASKDEBUG("BUG: membership message does not have valid body\n");
           	// SP_error( ret );
			// pthread_mutex_unlock(&rd_mutex);
         	// ERROR_EXIT(ret);
        // }

		// if  ( Is_reg_memb_mess( service_type ) ) {
			// TASKDEBUG("Received REGULAR membership for group %s with %d members, where I am member %d:\n",
				// sender, num_groups, mess_type );

			// if( Is_caused_join_mess( service_type ) ||
				// Is_caused_leave_mess( service_type ) ||
				// Is_caused_disconnect_mess( service_type ) ){
				// sp_nr_mbrs = num_groups;
				// memcpy((void*) sp_members, (void *) target_groups, sp_nr_mbrs*MAX_GROUP_NAME);
				// for( i=0; i < sp_nr_mbrs; i++ ){
					// TASKDEBUG("\t%s nodeid=%d\n", &sp_members[i][0],  get_nodeid(&sp_members[i][0]) );
// //					TASKDEBUG("grp id is %d %d %d\n",memb_info.gid.id[0], memb_info.gid.id[1], memb_info.gid.id[2] );
				// }
			// }
		// }

		// if( Is_caused_join_mess( service_type ) )	{
			// /*----------------------------------------------------------------------------------------------------
			// *   JOIN: The group has a new member
			// *----------------------------------------------------------------------------------------------------*/
			// TASKDEBUG("Due to the JOIN of %s service_type=%d\n", 
				// memb_info.changed_member, service_type );
			// mbr = get_nodeid((char *)  memb_info.changed_member);
			// nr_nodes = num_groups;
			// ret = sp_join(mbr);
			
		// }else if( Is_caused_leave_mess( service_type ) 
			// ||  Is_caused_disconnect_mess( service_type ) ){
			// /*----------------------------------------------------------------------------------------------------
			// *   LEAVE or DISCONNECT:  A member has left the group
			// *----------------------------------------------------------------------------------------------------*/
			// TASKDEBUG("Due to the LEAVE or DISCONNECT of %s\n", 
				// memb_info.changed_member );
			// mbr = get_nodeid((char *) memb_info.changed_member);
			// nr_nodes = num_groups;
			// ret = sp_disconnect(mbr);
		// }else if( Is_caused_network_mess( service_type ) ){
			// /*----------------------------------------------------------------------------------------------------
			// *   NETWORK CHANGE:  A network partition or a dead deamon
			// *----------------------------------------------------------------------------------------------------*/
			// TASKDEBUG("Due to NETWORK change with %u VS sets\n", memb_info.num_vs_sets);
            		// num_vs_sets = SP_get_vs_sets_info( mess_in, 
									// &vssets[0], MAX_VSSETS, &my_vsset_index );
            // if (num_vs_sets < 0) {
				// TASKDEBUG("BUG: membership message has more then %d vs sets. Recompile with larger MAX_VSSETS\n", MAX_VSSETS);
				// SP_error( num_vs_sets );
				// pthread_mutex_unlock(&rd_mutex);
               	// ERROR_EXIT( num_vs_sets );
			// }
            // if (num_vs_sets == 0) {
				// TASKDEBUG("BUG: membership message has %d vs_sets\n", 
					// num_vs_sets);
				// SP_error( num_vs_sets );
				// pthread_mutex_unlock(&rd_mutex);
               	// ERROR_EXIT( EMOLGENERIC );
			// }

			// bm_nodes = 0;
			// nr_nodes = 0;
            // for( i = 0; i < num_vs_sets; i++ )  {
				// TASKDEBUG("%s VS set %d has %u members:\n",
					// (i  == my_vsset_index) ?("LOCAL") : ("OTHER"), 
						// i, vssets[i].num_members );
               	// ret = SP_get_vs_set_members(mess_in, &vssets[i], members, MAX_MEMBERS);
               	// if (ret < 0) {
					// TASKDEBUG("VS Set has more then %d members. Recompile with larger MAX_MEMBERS\n", MAX_MEMBERS);
					// SP_error( ret );
					// pthread_mutex_unlock(&rd_mutex);
                   	// ERROR_EXIT( ret);
              	// }

				// /*---------------------------------------------
				// * get the bitmap of current members
				// --------------------------------------------- */
				// for( j = 0; j < vssets[i].num_members; j++ ) {
					// TASKDEBUG("\t%s\n", members[j] );
					// mbr = get_nodeid(members[j]);
					// if(!TEST_BIT(bm_nodes, mbr)) {
						// SET_BIT(bm_nodes, mbr);
						// nr_nodes++;
					// }
				// }
				// TASKDEBUG("old bm_sync=%X bm_nodes=%X primary_mbr=%d\n",
					// bm_sync, bm_nodes, primary_mbr);
			// }
			// if( bm_sync > bm_nodes) {		/* a NETWORK PARTITION has occurred 	*/
				// sp_net_partition();
			// }else{
				// if (bm_sync < bm_nodes) {	/* a NETWORK MERGE has occurred 		*/
					// sp_net_merge();
				// }else{
					// TASKDEBUG("NETWORK CHANGE with no changed members!! ");
				// }
			// }
		// }else if( Is_transition_mess(   service_type ) ) {
			// TASKDEBUG("received TRANSITIONAL membership for group %s\n", sender );
			// if( Is_caused_leave_mess( service_type ) ){
				// TASKDEBUG("received membership message that left group %s\n", sender );
			// }else {
				// TASKDEBUG("received incorrecty membership message of type 0x%x\n", service_type );
			// }
		// } else if ( Is_reject_mess( service_type ) )      {
			// TASKDEBUG("REJECTED message from %s, of servicetype 0x%x messtype %d, (endian %d) to %d groups \n(%d bytes): %s\n",
				// sender, service_type, mess_type, endian_mismatch, num_groups, ret, mess_in );
		// }else {
			// TASKDEBUG("received message of unknown message type 0x%x with ret %d\n", service_type, ret);
		// }
	// }
	// pthread_mutex_unlock(&rd_mutex);

	// if(ret < 0) ERROR_RETURN(ret);
	// return(ret);
// }

// /***************************************************************************/
// /* FUNCTIONS TO DEAL WITH SPREAD MESSAGES - MEMBERSHIP			*/
// /***************************************************************************/

// /*===========================================================================*
 // *				sp_join														*
 // * A NEW member has joint the VM group							*
 // *===========================================================================*/
// int sp_join( int new_mbr)
// {
	// int rcode; 
	// TASKDEBUG("new_member=%d primary_mbr=%d nr_nodes=%d\n", 
		// new_mbr, primary_mbr, nr_nodes);
	// if( nr_nodes < 0 || nr_nodes >= dc_ptr->dc_nr_nodes){
		// TASKDEBUG("nr_nodes=%d dc_ptr->dc_nr_nodes=%d\n", nr_nodes, dc_ptr->dc_nr_nodes);
		// ERROR_RETURN(EMOLINVAL);
		// }
		
	// SET_BIT(bm_nodes, new_mbr);

	// TASKDEBUG("new_member=%d primary_mbr=%d nr_nodes=%d\n", 
		// new_mbr, primary_mbr, nr_nodes);
	
	// TASKDEBUG("nr_nodes:%d\n", nr_nodes);
	// if( new_mbr == local_nodeid){		/*  My own JOIN message	 */
		// if (nr_nodes == 1){ 			/* I am a LONELY member  */
			// FSM_state 	= STS_SYNCHRONIZED;
			// synchronized = TRUE;
			// if ( ! replica_updated(local_nodeid)) {
				// TASKDEBUG("I am a BACKUP member: start the primary FIRST\n");
				// rcode = EMOLPRIMARY;
				// SP_error(rcode);
				// pthread_mutex_unlock(&rd_mutex);		
				// ERROR_EXIT(rcode);
			// }
			// SET_BIT(bm_sync, local_nodeid);
			// nr_sync = 1;
			// primary_mbr =  local_nodeid;
			// TASKDEBUG("PRIMARY_MBR=%d\n", primary_mbr);
			// send_status_info();
			// TASKDEBUG("Wake up rdisk: new_mbr=%d\n", new_mbr);
			// pthread_cond_signal(&rd_barrier);	/* Wakeup RDISK 		*/
			// return(OK);
		// }else{
			// /* Waiting that the primary_mbr will send me VM info */
			// FSM_state = STS_WAIT4PRIMARY;	
		// }
	// }else{ /* Other node JOINs the group	*/
		// if (primary_mbr == local_nodeid){ 	/* I am the first init member 	*/
			// send_status_info();
		// }
	// }

	// return(OK);
// }

// /*===========================================================================*
 // *				sp_disconnect				*
 // * A system task process has leave or disconnect from the VM group	*
 // *===========================================================================*/
// int sp_disconnect(int  disc_mbr)
// {
	// int i;
	
	// TASKDEBUG("disc_mbr=%d\n",disc_mbr);

	// CLR_BIT(bm_nodes, disc_mbr);
	// TASKDEBUG("CLR_BIT bm_nodes=%d, disc_mbr=%d\n",bm_nodes, disc_mbr);
	
	// if(local_nodeid == disc_mbr) {
		// FSM_state = STS_DISCONNECTED;
		// CLR_BIT(bm_sync, disc_mbr);
		// return(EMOLNOTCONN);
	// }

	// /* if local_nodeid is not synchronized and is the only surviving node , restart all*/
	// if(synchronized == FALSE) {
		// FSM_state = STS_NEW;
		// bm_nodes = 0;
		// bm_sync=0;
		// nr_nodes=0;
		// nr_sync=0;
		// return(sp_join(local_nodeid));
	// }

	// TASKDEBUG("bm_sync=%d, disc_mbr=%d, nr_sync=%d\n",bm_sync, disc_mbr, nr_sync);
	// /* if the dead node was synchronized */
	// if( TEST_BIT(bm_sync, disc_mbr)) {
		// nr_sync--;	/* decrease the number of synchronized nodes */
		// CLR_BIT(bm_sync, disc_mbr);
	// }
	// TASKDEBUG("primary_mbr=%d, disc_mbr=%d\n",primary_mbr, disc_mbr);
	// TASKDEBUG("bm_sync=%d, disc_mbr=%d, nr_sync=%d\n",bm_sync, disc_mbr, nr_sync);

	
	// /* if the dead node was the primary_mbr, search another primary_mbr */
	// if( primary_mbr == disc_mbr) {
		// TASKDEBUG("primary_mbr=%d, disc_mbr=%d\n",primary_mbr, disc_mbr);
		// if( bm_sync == 0) {
			// TASKDEBUG("THERE IS NO PRIMARY\n");
			// FSM_state = STS_NEW;
			// bm_acks=0;
			// return(sp_join(local_nodeid));
		// } else { /* Am I the new primary ?*/
			// for( i = 0; i < NR_NODES; i++) {
				// if(TEST_BIT(bm_nodes, i)) {
					// if( i == local_nodeid){ /* I am the new primary */
						// primary_mbr = local_nodeid;
						// // rcode = mnx_migr_start(dc_ptr->dc_dcid, MEM_PROC_NR);
						// // TASKDEBUG("mnx_migr_start rcode=%d\n",	rcode);
						// // rcode = mnx_migr_commit(m_lpid, dc_ptr->dc_dcid, MEM_PROC_NR, local_nodeid);
						// // TASKDEBUG("mnx_migr_commit rcode=%d\n",	rcode);
						// TASKDEBUG("PRIMARY_MBR=%d\n", primary_mbr);
						// send_status_info();
						// TASKDEBUG("Wake up rdisk: local_nodeid=%d\n", local_nodeid);
						// pthread_cond_signal(&primary_barrier);	/* Wakeup RDISK 		*/	
					// }
				// }	
			// }
		// }
	// }
	
	// CLR_BIT(bm_acks, disc_mbr);
	// TASKDEBUG("disc_mbr=%d nr_nodes=%d\n",	disc_mbr, nr_nodes);
	
	
	// TASKDEBUG("primary_mbr=%d nr_sync=%d\n", primary_mbr, nr_sync);
	// TASKDEBUG("bm_nodes=%X bm_sync=%X bm_acks=%X\n", bm_nodes,bm_sync,bm_acks);
	
	// return(OK);
// }

// /*----------------------------------------------------------------------------------------------------
// *				sp_net_partition
// *  A network partition has occurred
// *----------------------------------------------------------------------------------------------------*/
// int sp_net_partition(void)
// {
	// TASKDEBUG("\n");
	
// #ifdef ANULADO
	// if( synchronized == FALSE )
		// return(OK);

	// TASKDEBUG("bm_sync=%X bm_nodes=%X\n",bm_sync, bm_nodes);
	// /* mask the old init members bitmap with the mask of active members */
	// /* only the active nodes should be considered synchronized */
	// bm_sync &= bm_nodes;

	// /* is this the primary_mbr partition (where the old primary_mbr is located) ? */
	// if( TEST_BIT(bm_sync, first_act_mbr)){
		// /* primary_mbr of this partition */
		// primary_mbr = first_act_mbr;
	// }else{
		// /* get the first init member of this partition */
		// primary_mbr = NO_FIRST_INIT_MBR;
		// for( i = 0; i < num_vs_sets; i++ )  {
			// for( j = 0; j < vssets[i].num_members; j++ ) {
				// TASKDEBUG("\t%s\n", members[j] );
				// mbr = get_nodeid(members[j]);
				// if(!TEST_BIT(bm_sync, mbr)) {
					// first_act_mbr = mbr;
					// break;
				// }
			// }
			// if(first_act_mbr != NO_FIRST_INIT_MBR)
				// break;
		// }
		// if(first_act_mbr == NO_FIRST_INIT_MBR){
			// TASKDEBUG("Can't find primary_mbr of this partition %d\n",primary_mbr);
			// return(EMOLBADNODEID);
		// }
	// }

	// if( bm_acks != 0) { /* is this member waiting for donors responses ? */
		// bm_acks  &= bm_nodes;
		// if( bm_acks == 0) {
			// FSM_state = STS_SYNCHRONIZED;
			// if (FSM_state == STS_REQ_SLOTS && owned_slots > 0)
				// pthread_cond_signal(&fork_barrier);
		// }
	// }

	// total_slots = 0;
	// for( i = dc_ptr->dc_nr_sysprocs; i < (dc_ptr->dc_nr_tasks + dc_ptr->dc_nr_procs);i++) {
		// /* restore slots from uncompleted donations */
		// if( proc[i].p_rts_flags == SLOT_FREE) {
			// if ( (TEST_BIT(proc[i].p_rts_flags, BIT_DONATING)) &&
				 // (!TEST_BIT(bm_sync, slot[i].s_owner))) { /*The destination is not on my partition */
				// free_slots++;
				// owned_slots++;
				// CLR_BIT(proc[i].p_rts_flags, BIT_DONATING);
				// TASKDEBUG("Restoring slot %d from uncompleted donation after a network partition\n",i);
			// }
		// }
		// if(TEST_BIT(bm_sync, slot[i].s_owner)) { /* The owner is on my partition */
			// total_slots++;
		// }
	// }

	// nr_sync = 0;
 	// for( i =0; i < (sizeof(unsigned long int) * 8); i++){
		// if( TEST_BIT(bm_sync, i)){
			// nr_sync++;
		// }
	// }

	// TASKDEBUG("primary_mbr=%d bm_sync=%X bm_acks=%X\n",
		// primary_mbr, bm_sync, bm_acks);

	// /* recalculate global variables */
	// max_owned_slots	= (total_slots - (min_owned_slots*nr_sync));
	// TASKDEBUG("total_slots=%d max_owned_slots=%d nr_sync=%d bm_sync=%X\n",
		// total_slots,max_owned_slots, nr_sync,bm_sync);

	// if( local_nodeid == primary_mbr){ 	/* this member is the primary_mbr */
		// send_status_info();		/* broadcast partition status to waiting to initialize members */
	// }
// #endif /* ANULADO */
	// return(OK);
// }

// /*----------------------------------------------------------------------------------------------------
// *				sp_net_merge
// *  A network merge has occurred
// *  the first synchronized member of each merged partition
// *  broadcast its current process slot table (PST)
// *  only filled with the slots owned by members of their partition
// *----------------------------------------------------------------------------------------------------*/
// int sp_net_merge(void)
// {
	// TASKDEBUG("\n");
	
// #ifdef ANULADO 

	// if( synchronized == FALSE )
		// return(OK);

	// /* Only executed this function the first init members  of all partitions */
	// if ( primary_mbr != local_nodeid) 
		// return(OK);

	// /*------------------------------------
	// * Alloc memory for temporal  PST
 	// *------------------------------------*/
	// slot_part = (slot_t *) malloc( sizeof(slot_t)  * (vmu.dc_nr_procs+vmu.dc_nr_tasks));
	// if(slot_part == NULL) return (EMOLNOMEM);

	// /* Copy the PST to temporar PST */
	// memcpy( (void*) slot_part, slot, (sizeof(slot_t) * (vmu.dc_nr_procs+vmu.dc_nr_tasks)));

	// /* fill the slot table with endpoint and name				*/
	// total_slots = 0;
	// for( i = dc_ptr->dc_nr_sysprocs; i < (vmu.dc_nr_procs+vmu.dc_nr_tasks); i++) {
		// /* bm_ init keeps the synchronized members before MERGE */
		// if( !TEST_BIT(bm_sync, slot[i].s_owner)) { /* the owner is not on this partition */
			// TASKDEBUG("slot reserved %d for owner=%d\n", i, slot_part[i].s_owner);
			// slot_part[i].s_owner = NO_FIRST_INIT_MBR; /* this partition does not own this slot*/
		// }else{
			// if( proc[i].p_rts_flags != SLOT_FREE){
				// slot_part[i].s_endpoint = proc[i].p_endpoint;
				// strncpy(slot_part[i].s_name, proc[i].p_name, (MAXPROCNAME-1));
			// } else {
				// slot_part[i].s_endpoint = NONE;
			// }
			// total_slots++;
		// }
	// }

	// /* broadcast the partition PST */
	// TASKDEBUG("Send the partition's PST to all members \n");
	// rcode = SP_multicast (sysmbox, FIFO_MESS, (char *) dc_ptr->dc_name,
			// SYS_MERGE_PST,
			// (sizeof(slot_t) * ((vmu.dc_nr_procs+vmu.dc_nr_tasks)-dc_ptr->dc_nr_sysprocs)),
			// (char*) &slot_part[dc_ptr->dc_nr_sysprocs]);

    // free( (void*)slot_part);

	// if(rcode <0) ERROR_RETURN(rcode);
// #endif /* ANULADO */

	// return(OK);
// }

// /***************************************************************************/
// /* FUNCTIONS TO DEAL WITH DEVICE REPLICATION  MESSAGES 			*/
// /***************************************************************************/
// /*===========================================================================*
// *				rep_dev_scatter							     			 *
 // ============================================================================*/
// int rep_dev_scatter( message *sp_ptr){
		// TASKDEBUG(MSG2_FORMAT, MSG2_FIELDS(sp_ptr));
		// return(OK);
// }

// /*===========================================================================*
// *				rep_dev_write								     			 *
 // ============================================================================*/
// int rep_dev_write(SP_message *sp_ptr){

	// int rcode;

	// mnx_ptr = (message *) sp_ptr; /*puntero al inicio del mensaje=msj minix*/
		
	// TASKDEBUG(MSG2_FORMAT, MSG2_FIELDS(mnx_ptr));
	// TASKDEBUG("synchronized(%d) - TRUE(%d)\n", synchronized, TRUE); 
		
	// if( synchronized != TRUE) return(OK);
	
	// if ( mnx_ptr->m_source != primary_mbr){
		// TASKDEBUG("FAKE PRIMARY member=%d\n", mnx_ptr->m_source);
		// return(OK);
	// }
	
	// TASKDEBUG("bm_acks=%d, bm_sync=%d \n", bm_acks, bm_sync); 	
	// bm_acks = bm_sync;
	// CLR_BIT(bm_acks,primary_mbr);
	// TASKDEBUG("CLR_BIT -bm_acks=%d\n", bm_acks); 	
	// TASKDEBUG("primary_mbr=%d, local_nodeid= %d\n", primary_mbr, local_nodeid); 	
	// if( primary_mbr == local_nodeid){
		// TASKDEBUG("primary_mbr=%d, local_nodeid= %d\n", primary_mbr, local_nodeid); 	
		// return(OK);
		// }
	
	// /*replico acciones de driver.c antes de llamar a la función m_transfer() - idem do_rdwt()*/
	
	// /* Carry out a single read or write request. */
	// iovec_t iovec1;
	// int r, opcode;
  
	// TASKDEBUG("sp_ptr->COUNT=%u\n", mnx_ptr->COUNT);
	// /* Disk address?  Address and length of the user buffer? */
	// if (mnx_ptr->COUNT < 0) return(EINVAL);
	// /*COUNT: cantidad de  bytes reales del mensaje, sin comprimir*/ 	
  
	// /* Check the user buffer. */
	// //sys_umap(mp->IO_ENDPT, D, (vir_bytes) mp->ADDRESS, mp->COUNT, &phys_addr);
	// //if (phys_addr == 0) return(EFAULT);
  
	// /*MOL es virtual, no se realizan conversiones a direcciones*/
	// TASKDEBUG("sp_ptr->IO_ENDPT=%d - sp_ptr->ADDRESS:%p - sp_ptr->COUNT=%u\n", 
		// mnx_ptr->IO_ENDPT, (vir_bytes) mnx_ptr->ADDRESS, mnx_ptr->COUNT);
  
	// /* Prepare for I/O. */
	// if (m_prepare(mnx_ptr->DEVICE) == NIL_DEV) return(ENXIO);
	// TASKDEBUG("sp_ptr->DEVICE=%d\n", mnx_ptr->DEVICE);

	// /* Create a one element scatter/gather vector for the buffer. */
	// opcode = DEV_SCATTER; 
	// iovec1.iov_addr = (vir_bytes) mnx_ptr->ADDRESS; /*acá podría completar con los datos del puntero a los datos??*/
	// iovec1.iov_size = mnx_ptr->COUNT;	  

	// TASKDEBUG("iovec1.iov_addr= %X\n", iovec1.iov_addr);
	// TASKDEBUG("iovec1.iov_size= %d\n", iovec1.iov_size);
	// // TASKDEBUG("DATA= %s y %X\n", data_ptr, data_ptr); /*si estaría el buffer sin comprimir*/


	// // TASKDEBUG("DATA= %s\n", sp_ptr->buffer_data); /*si estaría el buffer sin comprimir*/
	
	// TASKDEBUG("File descriptor image= %d\n", devvec[mnx_ptr->DEVICE].img_p);
	// TASKDEBUG("(receive) mnx->POSITION %X\n", mnx_ptr->POSITION);	
	
	
	// // if ( (lseek(img_p, mnx_ptr->POSITION, SEEK_SET)) < 0 ){ /*ubicar, en el offset=POSITION para el fd*/
			// // rcode = errno;
			// // return(rcode);
			// // }
	
	
	// // if ( (write(img_p, sp_ptr->buffer_data, mnx_ptr->COUNT)) < 0 ){ /*write: buffer_data(recibido msj spread), COUNT bytes*/
			// // rcode = errno;
			// // return(rcode);
			// // }
	// TASKDEBUG("sp_ptr->buf.flag_buff =%d\n", sp_ptr->buf.flag_buff);

	// /*DECOMPRESS DATA BUFFER*/
	
	// if ( sp_ptr->buf.flag_buff == COMP ){
		// TASKDEBUG("DECOMPRESS DATA\n");
		// lz4_data_cd(sp_ptr->buf.buffer_data, mnx_ptr->COUNT, sp_ptr->buf.flag_buff);
		
		// // sp_ptr->buf.flag_buff = msg_lz4cd.buf.flag_buff;
		// sp_ptr->buf.flag_buff = msg_lz4cd.buf.flag_buff;
		// TASKDEBUG("sp_msg.buf.flag_buff =%d\n", sp_ptr->buf.flag_buff);
		// sp_ptr->buf.buffer_size = msg_lz4cd.buf.buffer_size;
		// TASKDEBUG("sp_msg.buf.buffer_size =%d\n", sp_ptr->buf.buffer_size);
		// memcpy(sp_ptr->buf.buffer_data, msg_lz4cd.buf.buffer_data, sp_ptr->buf.buffer_size);
		// TASKDEBUG("sp_msg.buf.buffer_data =%s\n", sp_ptr->buf.buffer_data);
					
		// TASKDEBUG("mnx_ptr->COUNT(%d) == sp_ptr->buf.buffer_size (%d)\n",mnx_ptr->COUNT,sp_ptr->buf.buffer_size);
		// if ( mnx_ptr->COUNT == sp_ptr->buf.buffer_size) {
			// TASKDEBUG("BYTES CLIENT = BYTES DECOMPRESS\n");
			// if ( (pwrite(devvec[mnx_ptr->DEVICE].img_p, sp_ptr->buf.buffer_data, sp_ptr->buf.buffer_size, mnx_ptr->POSITION)) < 0 ){ 
				// rcode = errno;
				// return(rcode);
				// }		
			// } 
		// else{
			// TASKDEBUG("ERROR. Bytes decompress not equal Bytes original\n");
			// ERROR_EXIT( EMOLPACKSIZE );
		// }	
	// }
	// else{		
		// TASKDEBUG("DATA BUFFER UNCOMPRESS\n");
		// if ( (pwrite(devvec[mnx_ptr->DEVICE].img_p, sp_ptr->buf.buffer_data, mnx_ptr->COUNT, mnx_ptr->POSITION)) < 0 ){
			// rcode = errno;
			// return(rcode);
			// }		
	// }
	// nr_optrans= mnx_ptr->m2_l2; /*nro de operación del nr_req*/
	// TASKDEBUG("nr_optrans=%d\n", nr_optrans);	
		
	// /* Transfer bytes from/to the device. */
	// r = m_transfer(mnx_ptr->IO_ENDPT, opcode, mnx_ptr->POSITION, &iovec1, 1);
	// TASKDEBUG("m_transfer = (r) %d\n", r);

	// /* Return the number of bytes transferred or an error code. */
	// return(r == OK ? (mnx_ptr->COUNT - iovec1.iov_size) : r);
	
	// /*return(rcode);*/
// }

// /*===========================================================================*
 // *				rep_task_reply								     	*
 // ===========================================================================*/
// int rep_task_reply( message *m_ptr)
// {
	// TASKDEBUG(MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	
	// if( synchronized != TRUE) return(OK);
	
	// if( !TEST_BIT(bm_sync, m_ptr->m_source)){
		// TASKDEBUG("FAKE backup member=%d (bm_sync=%X)\n", 
			// m_ptr->m_source, bm_sync );
		// return(OK);
	// }

	// CLR_BIT(bm_acks, m_ptr->m_source);

  	// if( primary_mbr == local_nodeid){
		// if(bm_acks == 0) {	/* all ACKs received */
			// TASKDEBUG("ALL ACKS received\n"); 
			// pthread_cond_signal(&update_barrier);	/* Wakeup RDISK 		*/
			// }
	// }
	
	// return(OK);
// }

// /*===========================================================================*
 // *				rep_dev_open								     	*
 // ===========================================================================*/
// int rep_dev_open( message *m_ptr)
// {
	// int rcode;

	// TASKDEBUG(MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	
	// if( synchronized != TRUE) return(OK);
	
	// if ( m_ptr->m_source != primary_mbr){
		// TASKDEBUG("FAKE PRIMARY member=%d\n", m_ptr->m_source);
		// return(OK);
	// }

	// bm_acks = bm_sync;
	// CLR_BIT(bm_acks,  primary_mbr);
		
	// if( primary_mbr == local_nodeid) return(OK);
	
	// rcode = m_do_open(m_dtab, m_ptr); // modificado pap antes era &m_dtab
	// return(rcode);
// }

// int rep_dev_close( message *sp_ptr){
		// TASKDEBUG(MSG2_FORMAT, MSG2_FIELDS(sp_ptr));
		// return(OK);
// }
// int rep_dev_ioctl( message *sp_ptr){
		// TASKDEBUG(MSG2_FORMAT, MSG2_FIELDS(sp_ptr));
		// return(OK);
// }
// int rep_cancel( message *sp_ptr){
		// TASKDEBUG(MSG2_FORMAT, MSG2_FIELDS(sp_ptr));
		// return(OK);
// }
// int rep_select( message *sp_ptr){
		// TASKDEBUG(MSG2_FORMAT, MSG2_FIELDS(sp_ptr));
		// return(OK);
// }
// /***************************************************************************/
// /* FUNCTIONS TO DEAL WITH  REPLICATION PROTOCOL  MESSAGES 		*/
// /***************************************************************************/

// /*===========================================================================*
 // *				mc_status_info								     	*
 // * The PRIMARY member sent a multicast to no sync members					*
 // ===========================================================================*/
// int mc_status_info(	message  *sp_ptr)
// {
	// int rcode;
	
	// TASKDEBUG(MSG2_FORMAT, MSG2_FIELDS(sp_ptr));

	// if( FSM_state != STS_WAIT4PRIMARY ) return (OK);

	// primary_mbr = sp_ptr->m_source;
	// TASKDEBUG("Primary_mbr=%d\n", primary_mbr);
	
	// if (nr_nodes != sp_ptr->m2_i1){
		// TASKDEBUG("Received nr_nodes=%d don't match local nr_nodes=%d\n"
			// , sp_ptr->m2_i1, nr_nodes);
		// rcode = EMOLBADVALUE;
		// SP_error(rcode);
		// pthread_mutex_unlock(&rd_mutex);
		// ERROR_EXIT(rcode);
	// }
	// nr_sync 	= sp_ptr->m2_i2;
	// bm_nodes 	= sp_ptr->m2_l1;
	// bm_sync		= sp_ptr->m2_l2;
	// FSM_state   = STS_WAIT4SYNC;
	// rcode = send_synchronized();
	// /* ESTO NO SE SI ESTA BIEN */
	// if( rcode) 
		// FSM_state   = STS_WAIT4PRIMARY;
	// return(OK);	
// }

// /*=========================================================================*
 // *				mc_synchronized							     	*
 // * A new sync member inform about it to other sync	members					*
 // ===========================================================================*/
// int mc_synchronized(  message  *sp_ptr)
// {
	// TASKDEBUG(MSG2_FORMAT, MSG2_FIELDS(sp_ptr));
	// if ( sp_ptr->m_source == local_nodeid) {
		// FSM_state = STS_SYNCHRONIZED;
		// synchronized = TRUE;
	// }
	// /* ESTO NO SE SI ESTA BIEN 	
	// * puede que un miembro recien ingresado haya recibido un mensaje 
	// *  MC_STATUS_INFO y haya cambiado los valores de bm_sync
	// * pero luego otro en el mismo estado se sincronizo y entonces
	// * bm_sync cambio y el miembro no sync no actualiza el bm_sync.
	// */
	// if( synchronized == FALSE ) return (OK);
	
	// nr_sync++;
	// SET_BIT(bm_sync, sp_ptr->m_source);
	// TASKDEBUG("New sync mbr=%d bm_sync=%X\n", 
		// sp_ptr->m_source , bm_sync);

	// pthread_cond_signal(&rd_barrier);	/* Wakeup RDISK 		*/

	// return(OK);	
// }
				
// /***************************************************************************/
// /* AUXILIARY FUNCTIONS 										*/
// /***************************************************************************/

// /*===========================================================================*
 // *				replica_updated				     					*
 // * check if the node storage is UPDATED								*
 // *===========================================================================*/
// int replica_updated(int localnodeid)
// {
	// return(TRUE);
// }

// /*===========================================================================*
 // *				get_nodeid				     *
 // * It converts a node string provided by SPREAD into a node ID
 // *===========================================================================*/
// int get_nodeid(char *mbr_string)
// {
	// char *s_ptr;
	// int nid;

	// s_ptr = strchr(&mbr_string[6], '.'); /* locate the dot character after "#RDISK" */
	// if(s_ptr != NULL)
		// *s_ptr = '\0';
	// s_ptr = &mbr_string[6];
	// nid = atoi(s_ptr);
	// TASKDEBUG("member=%s nodeid=%d\n", mbr_string,  nid );
	// return(nid);
// }

// /*===========================================================================*
// *				send_status_info
// * Send GROUP status to members
// *===========================================================================*/
// int send_status_info(void)
// {
	// int rcode;
	// message  msg;

	// if( primary_mbr != local_nodeid)
		// ERROR_RETURN(EMOLBADNODEID);
	
	// msg.m_source= local_nodeid;			/* this is the primary */
	// msg.m_type 	= MC_STATUS_INFO;
	// msg.m2_i1	= nr_nodes;
	// msg.m2_i2	= nr_sync;
	// msg.m2_l1	= bm_nodes;
	// msg.m2_l2	= bm_sync;
	
	// rcode = SP_multicast (sysmbox, SAFE_MESS, (char *) replica_name,  
			// MC_STATUS_INFO, sizeof(message), (char *) &msg); 
	// if(rcode <0) ERROR_RETURN(rcode);

	// return(rcode);
// }

// /*===========================================================================*
// *				send_synchronized
// * A new synchronized member informs to other sync members
// *===========================================================================*/
// int send_synchronized(void)
// {
	// int rcode;
	// message  msg;

	// msg.m_source= local_nodeid;			/* this is the new sync member  */
	// msg.m_type 	= MC_SYNCHRONIZED;
	
	// rcode = SP_multicast (sysmbox, SAFE_MESS, (char *) replica_name,  
			// MC_SYNCHRONIZED, sizeof(message), (char *) &msg); 
	// if(rcode <0) ERROR_RETURN(rcode);

	// return(rcode);
// }

