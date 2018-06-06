/****************************************************************/
/*		MINIX OVER LINUX IPC PRIMITIVES FOR PROXIES	*/
/****************************************************************/

//asmlinkage long mm_bind(int dcid, int pid, int proc, int nodeid);

/****************************************************************/
/****************************************************************/
/*	FUNCTIONS FOR SENDER PROXY				*/
/****************************************************************/
/****************************************************************/

/*--------------------------------------------------------------*/
/*			sproxy_enqueue				*/
/* Enqueue into the sender's proxy queue  the proc_ptr   	*/
/* descriptor with the requester rqtr and remote        	*/
/* operation rmtoper 						*/
/*--------------------------------------------------------------*/
long sproxy_enqueue(struct proc *proc_ptr)
{
	struct proc *sproxy_ptr;
	proc_usr_t *p_ptr;
	cmd_t *c_ptr;
 
	p_ptr = &proc_ptr->p_usr;
	MOLDEBUG(DBGPARAMS, PROC_USR_FORMAT, PROC_USR_FIELDS(p_ptr));
	c_ptr = &proc_ptr->p_rmtcmd;
	MOLDEBUG(DBGPARAMS, CMD_FORMAT, CMD_FIELDS(c_ptr));
	
	sproxy_ptr =  NODE2SPROXY(proc_ptr->p_rmtcmd.c_dnode);
	WLOCK_PROC(sproxy_ptr);
	if( sproxy_ptr->p_usr.p_rts_flags == SLOT_FREE) ERROR_WUNLOCK_PROC(sproxy_ptr, EMOLNOPROXY);

	/* save the proxy pair number to simplify garbage collection */
	proc_ptr->p_usr.p_proxy = sproxy_ptr->p_usr.p_nr;

	/* enqueue the process descriptor at the TAIL of the sender proxy's caller_q */
	LIST_ADD_TAIL(&proc_ptr->p_link , &sproxy_ptr->p_list);
	p_ptr = &sproxy_ptr->p_usr;
	MOLDEBUG(INTERNAL, PROC_USR_FORMAT, PROC_USR_FIELDS(p_ptr));
	if( test_bit(BIT_RECEIVING, &sproxy_ptr->p_usr.p_rts_flags))  {
		clear_bit(BIT_RECEIVING,&sproxy_ptr->p_usr.p_rts_flags);
		if(sproxy_ptr->p_usr.p_rts_flags == 0) {
			LOCAL_PROC_UP(sproxy_ptr, OK);
		}
	}
	WUNLOCK_PROC(sproxy_ptr);	
	return(OK);
}


/*--------------------------------------------------------------*/
/*			mol_get2rmt				*/
/* proxy gets local (messages, notifies, errors, ups, data,etc  */
/* to send to a remote processes	 			*/
/* usr_hdr_ptr: buffer address in userspace for the header	*/
/* usr_pay_ptr: buffer address in userspace for the payload	*/
/*--------------------------------------------------------------*/
asmlinkage long mm_get2rmt(proxy_hdr_t *usr_hdr_ptr, proxy_payload_t *usr_pay_ptr, long timeout_ms)
{
	dc_desc_t *dc_ptr;
	struct proc *src_ptr, *dst_ptr, *xpp, *tmp_ptr,*sproxy_ptr;
	proxy_hdr_t *h_ptr;
	struct task_struct *task_ptr;	
	int ret, dcid;
	int sproxy_pid, px_nr;
	cluster_node_t *node_ptr;
	node_usr_t *nu_ptr;
	struct timespec *t_ptr;
	proc_usr_t *p_ptr;
	proxies_t *px_ptr;
	cmd_t *c_ptr;
	message *m_ptr;

	MOLDEBUG(GENERIC,"\n");

	if(current_euid() != USER_ROOT) ERROR_RETURN(-EPERM);

	if( DVS_NOT_INIT() )   return(EMOLDVSINIT );
	
	ret = check_caller(&task_ptr, &sproxy_ptr, &sproxy_pid);
	if(ret) 	return(ret);

	/*------------------------------------------
	 * Check the status of the proxy (caller)
         *------------------------------------------*/
	ret = OK;
	WLOCK_PROC(sproxy_ptr);
	do {
		if( !test_bit(MIS_BIT_PROXY, &sproxy_ptr->p_usr.p_misc_flags))	{ret = EMOLNOPROXY;break;} 
		if( !test_bit(MIS_BIT_CONNECTED, &sproxy_ptr->p_usr.p_misc_flags)){ret = EMOLNOTCONN;break;}  
	} while(0);
	if(ret) {
		WUNLOCK_PROC(sproxy_ptr);
		ERROR_RETURN(ret);
	}

	/*------------------------------------------
	 * Verify if SPROXY is correctectly set on proxies struct
         *------------------------------------------*/
	px_nr = sproxy_ptr->p_usr.p_nr;
	px_ptr = &proxies[px_nr];
	sproxy_ptr->p_umsg = (message *) usr_pay_ptr;
	/* required by locking order */
	WUNLOCK_PROC(sproxy_ptr);
	RLOCK_PROXY(px_ptr);
	if( sproxy_ptr != &px_ptr->px_sproxy) {
		RUNLOCK_PROXY(px_ptr);
		ERROR_RETURN(EMOLBADPROXY);
	}
	RUNLOCK_PROXY(px_ptr);

	/*------------------------------------------
	 * Get the command from the proxy queue
         *------------------------------------------*/
	RLOCK_PROC(sproxy_ptr);
	while( TRUE )	{
		MOLDEBUG(GENERIC,"LIST_FOR_EACH_ENTRY_SAFE\n");
		ret = OK;
		LIST_FOR_EACH_ENTRY_SAFE(xpp, tmp_ptr, &sproxy_ptr->p_list, p_link) {

			RUNLOCK_PROC(sproxy_ptr);
			WLOCK_PROC(xpp);
			WLOCK_PROC(sproxy_ptr);

			MOLDEBUG(GENERIC,"Found a message. p_endpoint=%d c_cmd=%d\n",
				xpp->p_usr.p_endpoint,xpp->p_rmtcmd.c_cmd);
			
			LIST_DEL_INIT(&xpp->p_link); /* remove from queue */
			
			/* A LOCAL process descriptor must have	(SENDING | ONCOPY) set	*/
			/* A REMOTE process descriptor must have (RMTOPER | ONCOPY) set	*/
			/* Those flags protect them when unlocks			*/ 
			/* RULE TO LOCK: 1st: sender, 2nd: sender proxy */

			do {
				ret = OK;
				
				p_ptr = &xpp->p_usr;
				MOLDEBUG(INTERNAL, PROC_USR_FORMAT, PROC_USR_FIELDS(p_ptr));

				/*-----------------------------------
				 * Check process descriptor status
				 *----------------------------------*/	
				c_ptr = &xpp->p_rmtcmd;
				MOLDEBUG(INTERNAL, CMD_FORMAT, CMD_FIELDS(c_ptr));

				m_ptr = &xpp->p_message;
				MOLDEBUG(INTERNAL, MSG1_FORMAT, MSG1_FIELDS(m_ptr));		
			
				node_ptr = &node[xpp->p_rmtcmd.c_dnode];
				nu_ptr = &node_ptr->n_usr;
				MOLDEBUG(INTERNAL, NODE_USR_FORMAT, NODE_USR_FIELDS(nu_ptr));

				if( test_bit(BIT_SLOT_FREE, &xpp->p_usr.p_rts_flags)) 	{ret = EMOLNOTBIND; break;}
				if(!test_bit(xpp->p_usr.p_dcid,&node_ptr->n_usr.n_dcs)) {ret = EMOLNODCNODE;break;}
				if( (xpp->p_usr.p_dcid < 0 )
				 || (xpp->p_usr.p_dcid >= dvs.d_nr_dcs))				{ret = EMOLBADDCID;break;} 	
				if( test_bit(BIT_MIGRATE, &xpp->p_usr.p_rts_flags)) 	{ret = EMOLMIGRATE; break;}
				if(!test_bit(xpp->p_rmtcmd.c_dnode,&sproxy_ptr->p_usr.p_nodemap)) {ret = EMOLNONODE;break;}
			}while(0);
			WUNLOCK_PROC(sproxy_ptr);
			dc_ptr 	= &dc[xpp->p_usr.p_dcid];
			WUNLOCK_PROC(xpp);	

			if( !ret) {	
				/*-----------------------------------
				 * Check DC status
				 *----------------------------------*/	
				RLOCK_DC(dc_ptr);
				if(dc_ptr->dc_usr.dc_flags) {
					RUNLOCK_DC(dc_ptr);
					ret = EMOLDCNOTRUN; 
				}else{
					dcid = dc_ptr->dc_usr.dc_dcid;
					RUNLOCK_DC(dc_ptr);				
					WLOCK_PROC(sproxy_ptr);
					/* The proxy takes the sender's DCID personality */
					sproxy_ptr->p_usr.p_dcid = dcid;
					WUNLOCK_PROC(sproxy_ptr);
//					DC_INCREF(dc_ptr);
				}
			} 

			WLOCK_PROC(xpp);	
			/* do not replace by "else" !! */
			if(ret) {
				/*-----------------------------------
				 * For Requests (LOCAL descriptors) 
				 * return an ACK with error
				 *----------------------------------*/
				xpp->p_usr.p_proxy = NONE;	
				if(IT_IS_LOCAL(xpp)){
					xpp->p_usr.p_rts_flags = 0;
					READY_UP_RCODE(xpp, CMD_ACKNOWLEDGE, ret);
					WUNLOCK_PROC(xpp);	
					ERROR_RETURN(ret);
				}
				/*-----------------------------------
				 * For ACKs (REMOTE descriptors) 
				 * IGNORE
				 *----------------------------------*/
				xpp->p_usr.p_rts_flags = 0;
				set_bit(BIT_REMOTE, &xpp->p_usr.p_rts_flags);
				WUNLOCK_PROC(xpp);	
				RLOCK_PROC(sproxy_ptr);
				ERROR_PRINT(ret);
				continue; /* get next message  */
			}

			h_ptr = &xpp->p_rmtcmd;
			ret = OK;

			/*
			* Verify MIGRATED destination PROCESS
			* If command is SEND, SENDREC, NOTIFY, REPLY replay this commands for the local process
			* Any ONCOPY command must be in queue because the migration checks the ONCOPY bit.
			* Any ACK command must be in queue because the MIGRATING process is blocked on a SENDREC to its SYSTASK
			*/
			if( IT_IS_LOCAL(xpp) ) {
				dst_ptr = ENDPOINT2PTR(dc_ptr, xpp->p_rmtcmd.c_dst);
				WLOCK_ORDERED2(xpp, dst_ptr);							
				if( h_ptr->c_dnode != dst_ptr->p_usr.p_nodeid ){ /* It means that the dest process has migrated */
					WUNLOCK_PROC(dst_ptr);		
					/*
					* Wakes up the local sender proc with an EMOLMIGRATE error to REPLAY the IPC.
					*/
					xpp->p_usr.p_sendto = NONE;
					xpp->p_usr.p_proxy 	= NONE;	
					xpp->p_usr.p_rts_flags = 0;
					READY_UP_RCODE(xpp, CMD_ACKNOWLEDGE, EMOLMIGRATE)
					WUNLOCK_PROC(xpp);
					RLOCK_PROC(sproxy_ptr); /* the proxy must be locked for the next loop 	*/
					continue;		/* next loop of LIST_FOR_EACH_ENTRY_SAFE 	*/	
				}else{
					WUNLOCK_PROC(dst_ptr);	
				}
			} 

			h_ptr->c_timestamp = current_kernel_time();
			t_ptr = &h_ptr->c_timestamp;
			MOLDEBUG(INTERNAL,TIME_FORMAT, TIME_FIELDS(t_ptr));

			if(IT_IS_REMOTE(xpp)){ 	/* REMOTE process descriptor used for ACKNOWLEDGES 	*/
				MOLDEBUG(DBGPROC,"REMOTE " PROC_USR_FORMAT,PROC_USR_FIELDS(p_ptr));
				switch(xpp->p_rmtcmd.c_cmd) {
					case CMD_SEND_ACK:	/* The local receiver process send a SEND ACK to the remote sender process */
						MOLDEBUG(GENERIC,"CMD_SEND_ACK\n");
						MOLDEBUG(DBGVCOPY,CMD_FORMAT,CMD_FIELDS(h_ptr));
						break;
					case CMD_SNDREC_ACK:	/* The local receiver process send a SENDREC ACK to the remote sender process  ONLY ON ERROR */
						MOLDEBUG(GENERIC,"CMD_SNDREC_ACK\n");
						MOLDEBUG(DBGVCOPY,CMD_FORMAT,CMD_FIELDS(h_ptr));
						break;
					case CMD_COPYIN_ACK:	
						MOLDEBUG(GENERIC,"CMD_COPYIN_ACK\n");
						MOLDEBUG(DBGVCOPY,CMD_FORMAT,CMD_FIELDS(h_ptr));
						MOLDEBUG(DBGVCOPY,VCOPY_FORMAT,VCOPY_FIELDS(h_ptr));
						dc_ptr 	= &dc[xpp->p_usr.p_dcid];
						src_ptr = ENDPOINT2PTR(dc_ptr, xpp->p_rmtcmd.c_u.cu_vcopy.v_dst);
						/* Other  process could be in the same slot  */
						/*!! the source of the ACK was the destination of the COPY !!*/
						WLOCK_ORDERED2(xpp, src_ptr);							
						if(src_ptr->p_usr.p_endpoint == xpp->p_rmtcmd.c_src) {
							clear_bit(BIT_ONCOPY, &src_ptr->p_usr.p_rts_flags);
						}else{
							ret = EMOLBADPROC;
						}
						WUNLOCK_PROC(src_ptr);
						break;
					case CMD_COPYOUT_DATA:	/* the remote process has requested to send local data */
						MOLDEBUG(GENERIC,"CMD_COPYOUT_DATA\n");
						MOLDEBUG(DBGVCOPY,CMD_FORMAT,CMD_FIELDS(h_ptr));
						MOLDEBUG(DBGVCOPY,VCOPY_FORMAT,VCOPY_FIELDS(h_ptr));
						/*  Copy the payload from source's user space to proxy's user space */
						dc_ptr 	= &dc[xpp->p_usr.p_dcid];
						src_ptr = ENDPOINT2PTR(dc_ptr, xpp->p_rmtcmd.c_u.cu_vcopy.v_src);
						WLOCK_ORDERED2(xpp, src_ptr);
						/* Other  process could be in the same slot  */
						/*!! the source of the DATA is the source of the COPY !!*/
						if(src_ptr->p_usr.p_endpoint == xpp->p_rmtcmd.c_src) {
							if(xpp->p_rmtcmd.c_u.cu_vcopy.v_bytes > NODE2MAXBYTES(xpp->p_rmtcmd.c_dnode))
								ret = EMOL2BIG;
							if( xpp->p_rmtcmd.c_rcode == OK){
								RLOCK_PROC(sproxy_ptr);
								if( test_bit(MIS_BIT_KTHREAD, &sproxy_ptr->p_usr.p_misc_flags))	{
									COPY_USR2KRN(ret, NONE, src_ptr, xpp->p_rmtcmd.c_u.cu_vcopy.v_saddr,
										(char*) usr_pay_ptr, xpp->p_rmtcmd.c_u.cu_vcopy.v_bytes);
								}else{
									COPY_USR2USR_PROC(ret, NONE, src_ptr, xpp->p_rmtcmd.c_u.cu_vcopy.v_saddr, 
										sproxy_ptr, (char*) usr_pay_ptr, xpp->p_rmtcmd.c_u.cu_vcopy.v_bytes);
								}
								RUNLOCK_PROC(sproxy_ptr);							
							}	
							clear_bit(BIT_ONCOPY, &src_ptr->p_usr.p_rts_flags);
						}else{
							ret = EMOLBADPROC;
						}
						WUNLOCK_PROC(src_ptr);
						break;
					case CMD_COPYIN_RQST:
						MOLDEBUG(GENERIC,"CMD_COPYIN_RQST\n");
						MOLDEBUG(DBGVCOPY,CMD_FORMAT,CMD_FIELDS(h_ptr));
						MOLDEBUG(DBGVCOPY,VCOPY_FORMAT,VCOPY_FIELDS(h_ptr));
						/*  Copy the payload from source's user space to proxy's user space */
//						dc_ptr 	= &dc[xpp->p_usr.p_dcid];
						src_ptr = ENDPOINT2PTR(dc_ptr, xpp->p_rmtcmd.c_u.cu_vcopy.v_src);
						WLOCK_ORDERED2(xpp, src_ptr);	
						/* Other  process could be in the same slot  */
						/*!! the source of the RQST is the source of the COPY !!*/
						if(src_ptr->p_usr.p_endpoint == xpp->p_rmtcmd.c_src) {
							if(xpp->p_rmtcmd.c_u.cu_vcopy.v_bytes > NODE2MAXBYTES(xpp->p_rmtcmd.c_dnode))
								ret = EMOL2BIG;
							if( xpp->p_rmtcmd.c_rcode == OK){
								RLOCK_PROC(sproxy_ptr);
								if( test_bit(MIS_BIT_KTHREAD, &sproxy_ptr->p_usr.p_misc_flags))	{
									COPY_USR2KRN(ret, NONE, src_ptr, xpp->p_rmtcmd.c_u.cu_vcopy.v_saddr,
											(char*) usr_pay_ptr, xpp->p_rmtcmd.c_u.cu_vcopy.v_bytes);
								}else{
									COPY_USR2USR_PROC(ret, NONE, src_ptr, xpp->p_rmtcmd.c_u.cu_vcopy.v_saddr, 
											sproxy_ptr, (char*) usr_pay_ptr, xpp->p_rmtcmd.c_u.cu_vcopy.v_bytes);
								RUNLOCK_PROC(sproxy_ptr);
								}
							}
							clear_bit(BIT_ONCOPY, &src_ptr->p_usr.p_rts_flags);
							clear_bit(BIT_ONCOPY, &xpp->p_usr.p_rts_flags);
						}else{
							ret = EMOLBADPROC;
						}
						WUNLOCK_PROC(src_ptr);
						break;
					case CMD_COPYLCL_ACK:	
						MOLDEBUG(GENERIC,"CMD_COPYLCL_ACK\n");
						MOLDEBUG(DBGVCOPY,CMD_FORMAT,CMD_FIELDS(h_ptr));
						MOLDEBUG(DBGVCOPY,VCOPY_FORMAT,VCOPY_FIELDS(h_ptr));
//						dc_ptr 	= &dc[xpp->p_usr.p_dcid];
						src_ptr = ENDPOINT2PTR(dc_ptr, xpp->p_rmtcmd.c_u.cu_vcopy.v_src);				
						WLOCK_ORDERED2(xpp, src_ptr);						
						/* Other  process could be in the same slot  */
						/* !!! The source of the ACK was the source of the copy  !!!*/
						if( src_ptr->p_usr.p_endpoint == xpp->p_rmtcmd.c_src) {
							clear_bit(BIT_ONCOPY, &src_ptr->p_usr.p_rts_flags);
							clear_bit(BIT_ONCOPY, &xpp->p_usr.p_rts_flags);
						}else{
							ret = EMOLBADPROC;
						}
						WUNLOCK_PROC(src_ptr);
						break;
					case CMD_COPYRMT_ACK:	
						MOLDEBUG(GENERIC,"CMD_COPYRMT_ACK\n");
						MOLDEBUG(DBGVCOPY,CMD_FORMAT,CMD_FIELDS(h_ptr));
						MOLDEBUG(DBGVCOPY,VCOPY_FORMAT,VCOPY_FIELDS(h_ptr));
//						dc_ptr 	= &dc[xpp->p_usr.p_dcid];
						src_ptr = ENDPOINT2PTR(dc_ptr, xpp->p_rmtcmd.c_u.cu_vcopy.v_src);				
						dst_ptr = ENDPOINT2PTR(dc_ptr, xpp->p_rmtcmd.c_u.cu_vcopy.v_dst);
						WUNLOCK_PROC(xpp);
						WLOCK_PROC3(xpp, src_ptr, dst_ptr);	
						/* Other  process could be in the same slot  */
						/*!!! the source of the ACK was the destination of the copy !!!*/
						if( (dst_ptr->p_usr.p_endpoint == xpp->p_rmtcmd.c_src) 
						 && (xpp->p_usr.p_endpoint == xpp->p_rmtcmd.c_u.cu_vcopy.v_rqtr)){		
							clear_bit(BIT_ONCOPY, &src_ptr->p_usr.p_rts_flags);
							clear_bit(BIT_ONCOPY, &dst_ptr->p_usr.p_rts_flags);
							clear_bit(BIT_ONCOPY, &xpp->p_usr.p_rts_flags);
						}else{
							ret = EMOLBADPROC;
						}			
						WUNLOCK_PROC2(src_ptr, dst_ptr);
						break;			
					default:
						MOLDEBUG(GENERIC,"BAD CMD=%d\n",xpp->p_rmtcmd.c_cmd);
						MOLDEBUG(DBGVCOPY,CMD_FORMAT,CMD_FIELDS(h_ptr));
						ret = EMOLINVAL;
						break;
				}
				if(ret) {
					xpp->p_usr.p_rts_flags = 0;
					set_bit(BIT_REMOTE, &xpp->p_usr.p_rts_flags);
					WUNLOCK_PROC(xpp);

					WLOCK_PROC(sproxy_ptr);
					sproxy_ptr->p_usr.p_dcid = (-1);
					WUNLOCK_PROC(sproxy_ptr);

//					DC_DECREF(dc_ptr);
					ERROR_RETURN(ret);
				}
				clear_bit(BIT_SENDING, &xpp->p_usr.p_rts_flags);
				clear_bit(BIT_RMTOPER, &xpp->p_usr.p_rts_flags);
				xpp->p_usr.p_sendto	= NONE;
			}else { 	/* the LOCAL process need to send a MINIX message to remote  */
				MOLDEBUG(DBGPROC,"LOCAL " PROC_USR_FORMAT,PROC_USR_FIELDS(p_ptr));
				clear_bit(BIT_RMTOPER, &xpp->p_usr.p_rts_flags);
				switch(xpp->p_rmtcmd.c_cmd) {
					case CMD_COPYIN_DATA:
						MOLDEBUG(GENERIC,"CMD_COPYIN_DATA\n");
						MOLDEBUG(DBGVCOPY,CMD_FORMAT,CMD_FIELDS(h_ptr));
						MOLDEBUG(DBGVCOPY,VCOPY_FORMAT,VCOPY_FIELDS(h_ptr));
						/*  Copy the payload from source's user space to proxy's user space */
//						dc_ptr 	= &dc[xpp->p_usr.p_dcid];
						src_ptr = ENDPOINT2PTR(dc_ptr, xpp->p_rmtcmd.c_u.cu_vcopy.v_src);
						/*!! the source of the REQUEST is the source of the COPY !!*/				
						if(src_ptr != xpp) { 
							WLOCK_ORDERED2(xpp, src_ptr);
							if(xpp->p_rmtcmd.c_u.cu_vcopy.v_bytes > NODE2MAXBYTES(xpp->p_rmtcmd.c_dnode))
								ret = EMOL2BIG;
							if (ret == OK) {
								RLOCK_PROC(sproxy_ptr);
								if( test_bit(MIS_BIT_KTHREAD, &sproxy_ptr->p_usr.p_misc_flags))	{
									COPY_USR2KRN(ret, NONE, src_ptr, h_ptr->c_u.cu_vcopy.v_saddr,
											(char*) usr_pay_ptr, xpp->p_rmtcmd.c_u.cu_vcopy.v_bytes);
								}else{
									COPY_USR2USR_PROC(ret, NONE, src_ptr, h_ptr->c_u.cu_vcopy.v_saddr, 
										sproxy_ptr, (char*) usr_pay_ptr, h_ptr->c_u.cu_vcopy.v_bytes);
								}
								RUNLOCK_PROC(sproxy_ptr);
							}
							WUNLOCK_PROC(src_ptr);
						} else {
							if(xpp->p_rmtcmd.c_u.cu_vcopy.v_bytes > NODE2MAXBYTES(xpp->p_rmtcmd.c_dnode))
								ret = EMOL2BIG;
							if (ret == OK) {
								RLOCK_PROC(sproxy_ptr);
								if( test_bit(MIS_BIT_KTHREAD, &sproxy_ptr->p_usr.p_misc_flags))	{
									COPY_USR2KRN(ret, NONE, src_ptr, h_ptr->c_u.cu_vcopy.v_saddr,
											(char*) usr_pay_ptr, h_ptr->c_u.cu_vcopy.v_bytes);
								}else{							
									COPY_USR2USR_PROC(ret, NONE, src_ptr, h_ptr->c_u.cu_vcopy.v_saddr, 
										sproxy_ptr, (char*) usr_pay_ptr, h_ptr->c_u.cu_vcopy.v_bytes);
								}
								RUNLOCK_PROC(sproxy_ptr);
							}
						}
						break;
					case CMD_COPYOUT_RQST:
						MOLDEBUG(GENERIC,"CMD_COPYOUT_RQST\n");
						MOLDEBUG(DBGVCOPY,VCOPY_FORMAT,VCOPY_FIELDS(h_ptr));
						break;		
					case CMD_COPYLCL_RQST:
						MOLDEBUG(GENERIC,"CMD_COPYLCL_RQST\n");
						MOLDEBUG(DBGVCOPY,VCOPY_FORMAT,VCOPY_FIELDS(h_ptr));
						break;
					case CMD_COPYRMT_RQST:
						MOLDEBUG(GENERIC,"CMD_COPYRMT_RQST\n");
						MOLDEBUG(DBGVCOPY,VCOPY_FORMAT,VCOPY_FIELDS(h_ptr));
						break;
					case CMD_SNDREC_MSG:
						MOLDEBUG(GENERIC,"CMD_SNDREC_MSG\n");
						xpp->p_usr.p_sendto = NONE;
						clear_bit(BIT_SENDING, &xpp->p_usr.p_rts_flags);
					case CMD_SEND_MSG:
						if (xpp->p_rmtcmd.c_cmd == CMD_SEND_MSG) 
							MOLDEBUG(GENERIC,"CMD_SEND_MSG\n");				
						m_ptr = &h_ptr->c_u.cu_msg;
						m_ptr->m_source = xpp->p_usr.p_endpoint;
						MOLDEBUG(GENERIC, MSG1_FORMAT, MSG1_FIELDS(m_ptr));
						break;
					case CMD_REPLY_MSG:
						MOLDEBUG(GENERIC,"CMD_REPLY_MSG\n");
						m_ptr = &h_ptr->c_u.cu_msg;
						m_ptr->m_source = xpp->p_usr.p_endpoint;
						MOLDEBUG(GENERIC, MSG1_FORMAT, MSG1_FIELDS(m_ptr));
						break;
					case CMD_NTFY_MSG:
						MOLDEBUG(GENERIC,"CMD_NTFY_MSG\n");
						xpp->p_usr.p_sendto 	= NONE;
						m_ptr = &h_ptr->c_u.cu_msg;
						m_ptr->m_source = xpp->p_usr.p_endpoint;
						MOLDEBUG(GENERIC, MSG9_FORMAT, MSG9_FIELDS(m_ptr));
						clear_bit(BIT_SENDING, &xpp->p_usr.p_rts_flags);
						clear_bit(BIT_RMTOPER, &xpp->p_usr.p_rts_flags);
						if(xpp->p_usr.p_rts_flags == 0) 			
							LOCAL_PROC_UP(xpp, ret);
						break;
					default:
						ret = EMOLINVAL;
						break;
				}
				xpp->p_usr.p_proxy = NONE;
				if(ret) {
					xpp->p_usr.p_rts_flags = 0;
					READY_UP_RCODE(xpp, CMD_ACKNOWLEDGE, ret);
					WUNLOCK_PROC(xpp);
//					DC_DECREF(dc_ptr);

					WLOCK_PROC(sproxy_ptr);
					sproxy_ptr->p_usr.p_dcid = (-1);
					WUNLOCK_PROC(sproxy_ptr);

					ERROR_RETURN(ret);
				}

			}

			MOLDEBUG(DBGCMD,HDR_FORMAT,HDR_FIELDS(h_ptr));
			/*  Copy the header to proxy */
			WLOCK_PROC(sproxy_ptr);
			if( test_bit(MIS_BIT_KTHREAD, &sproxy_ptr->p_usr.p_misc_flags))	{
				memcpy(usr_hdr_ptr, h_ptr, sizeof(proxy_hdr_t));
			}else{							
				COPY_TO_USER_PROC(ret, h_ptr, usr_hdr_ptr, sizeof(proxy_hdr_t));
			}
			xpp->p_rmtcmd.c_cmd = CMD_NONE;
			xpp->p_usr.p_proxy 	= NONE;
			WUNLOCK_PROC(xpp);

			sproxy_ptr->p_usr.p_getfrom = NONE;
			node_ptr->n_usr.n_stimestamp = current_kernel_time();
			node_ptr->n_usr.n_pxsent++;		
			
			sproxy_ptr->p_usr.p_rmtsent++;
			sproxy_ptr->p_usr.p_dcid = (-1);
			RUNLOCK_PROC(sproxy_ptr);

//			DC_DECREF(dc_ptr);
			return(OK);
		}
		
		sproxy_ptr->p_usr.p_getfrom = ANY;
		set_bit(BIT_RECEIVING, &sproxy_ptr->p_usr.p_rts_flags);
		MOLDEBUG(GENERIC,"Any message was not found.\n");
		ret = sleep_proc(sproxy_ptr, timeout_ms); 			/* SLEEP THE PROXY 	*/
		MOLDEBUG(GENERIC,"Someone wakes up the sender proxy\n");
		if(ret != OK) {
			sproxy_ptr->p_usr.p_rts_flags = PROC_RUNNING;
			RUNLOCK_PROC(sproxy_ptr);
			ERROR_RETURN(ret);
		}
	}
}	



