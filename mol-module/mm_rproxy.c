/****************************************************************/
/*		MINIX OVER LINUX IPC PRIMITIVES FOR PROXIES	*/
/****************************************************************/

/****************************************************************/
/****************************************************************/
/*	FUNCTIONS FOR RECEIVER PROXY				*/
/****************************************************************/
/****************************************************************/

/*--------------------------------------------------------------*/
/*			send_rmt2lcl				*/
/* proxy sends a remote message to a local process		 */
/* The DC mutex must be locked !!!				*/
/* src_ptr and dst_ptr must be locked 				*/
/*--------------------------------------------------------------*/
asmlinkage long send_rmt2lcl(proxy_hdr_t *usr_h_ptr, proxy_hdr_t *h_ptr, struct proc *src_ptr, struct proc *dst_ptr)
{
	message *m_ptr;
	int rcode;
	struct proc *sproxy_ptr, *rproxy_ptr;
	dc_desc_t *dc_ptr;

	/* checks if the source has another status than REMOTE  */
//	if( src_ptr->p_usr.p_rts_flags != REMOTE )  ERROR_RETURN(EMOLLCLPROC);
//	if( IT_IS_LOCAL(src_ptr) )  ERROR_RETURN(EMOLLCLPROC);
	
	sproxy_ptr = NODE2SPROXY(src_ptr->p_usr.p_nodeid);
	rproxy_ptr = NODE2RPROXY(src_ptr->p_usr.p_nodeid);
	
	dc_ptr 	= &dc[src_ptr->p_usr.p_dcid];

	MOLDEBUG(DBGPARAMS,"cmd=%d dcid=%d src_ep=%d src_flags=%lX dst_ep=%d dst_flags=%lX\n"
		,h_ptr->c_cmd, dc_ptr->dc_usr.dc_dcid, src_ptr->p_usr.p_endpoint
		,src_ptr->p_usr.p_rts_flags, dst_ptr->p_usr.p_endpoint,dst_ptr->p_usr.p_rts_flags);

	if( test_bit(MIS_BIT_KTHREAD, &rproxy_ptr->p_usr.p_misc_flags))	{
		m_ptr = rproxy_ptr->p_umsg;
	}else{
		m_ptr = &h_ptr->c_u.cu_msg;
	}	
	MOLDEBUG(DBGMESSAGE,MSG1_FORMAT,MSG1_FIELDS(m_ptr));

do {
	rcode = OK;
	/* Check if 'dst' is blocked waiting for this message.   */
	if ( (test_bit(BIT_RECEIVING, &dst_ptr->p_usr.p_rts_flags) && !test_bit(BIT_SENDING, &dst_ptr->p_usr.p_rts_flags)) &&
		(dst_ptr->p_usr.p_getfrom == ANY || dst_ptr->p_usr.p_getfrom == src_ptr->p_usr.p_endpoint)) {
		MOLDEBUG(GENERIC,"destination is waiting. p_umsg=%X\n",(unsigned int) dst_ptr->p_umsg);
		
		if( test_bit(MIS_BIT_KTHREAD, &rproxy_ptr->p_usr.p_misc_flags))	{
			COPY_KRN2USR(rcode, NONE,  (void *) rproxy_ptr->p_umsg,
				dst_ptr,(void *) dst_ptr->p_umsg, sizeof(message));			
		}else{
			COPY_USR2USR_PROC(rcode, NONE, rproxy_ptr, (void *)&usr_h_ptr->c_u.cu_msg,
				dst_ptr,(void *) dst_ptr->p_umsg, sizeof(message));
		}
		if(rcode) break;
		
		if( h_ptr->c_cmd == CMD_SNDREC_MSG) {
			src_ptr->p_usr.p_getfrom = dst_ptr->p_usr.p_endpoint;
			set_bit(BIT_RECEIVING, &src_ptr->p_usr.p_rts_flags);
		}else{
		   	src_ptr->p_rcode = OK;
			send_ack_lcl2rmt(src_ptr, dst_ptr, OK);
		}
//		dst_ptr->p_message.m_source = src_ptr->p_usr.p_endpoint;
		clear_bit(BIT_RECEIVING, &dst_ptr->p_usr.p_rts_flags);
		dst_ptr->p_usr.p_getfrom 	= NONE;
		if(dst_ptr->p_usr.p_rts_flags == 0)
			LOCAL_PROC_UP(dst_ptr, OK); /* ATENTI puede haber mas de una razon para despertar al proceso!! */
	} else { 
		MOLDEBUG(GENERIC,"destination is not waiting dst_ptr->p_usr.p_rts_flags=%lX. Enqueue at the TAIL.\n"
			,dst_ptr->p_usr.p_rts_flags);

		/* copies the message into the sender's userspace buffer and wakes up it */
		memcpy(&src_ptr->p_message, (void *)&h_ptr->c_u.cu_msg, sizeof(message));

		set_bit(BIT_SENDING, &src_ptr->p_usr.p_rts_flags);
		if( h_ptr->c_cmd == CMD_SNDREC_MSG) {
			src_ptr->p_usr.p_getfrom = dst_ptr->p_usr.p_endpoint;
			set_bit(BIT_RECEIVING, &src_ptr->p_usr.p_rts_flags);
		}
		src_ptr->p_usr.p_sendto 	= dst_ptr->p_usr.p_endpoint;
		INIT_LIST_HEAD(&src_ptr->p_link);
		LIST_ADD_TAIL(&src_ptr->p_link, &dst_ptr->p_list);
	}
} while(0);

	if(rcode) ERROR_RETURN(rcode);

	return(OK);
}
/*--------------------------------------------------------------*/
/*			notify_rmt2lcl				*/
/* proxy sends a remote notify to a local process		 */
/* MODIFICAR: El mensaje ya vino por la RED. NO HAY QUE CONSTRUIR*/
/* Sino eventualmente dejar el bitmap seteado si es q no esta	*/
/* bloqueado el receptor					*/
/* The DC mutex must be locked !!!				*/
/* src_ptr and dst_ptr must be locked 				*/
/*--------------------------------------------------------------*/
asmlinkage long notify_rmt2lcl(proxy_hdr_t *usr_h_ptr, proxy_hdr_t *h_ptr, 
								struct proc *src_ptr, struct proc *dst_ptr, 
								int update_proc)
{
	struct proc *sproxy_ptr,*rproxy_ptr;
	dc_desc_t *dc_ptr;
	message *m_ptr;
	int rcode, sid;

	/* checks if the source has another status than REMOTE  */
//	if( IT_IS_LOCAL(src_ptr) )  ERROR_RETURN(EMOLLCLPROC);

	sproxy_ptr = NODE2SPROXY(src_ptr->p_usr.p_nodeid);
	rproxy_ptr = NODE2RPROXY(src_ptr->p_usr.p_nodeid);

	dc_ptr 	= &dc[dst_ptr->p_usr.p_dcid];

	MOLDEBUG(DBGPARAMS,"dcid=%d src_ep=%d src_ptr->p_usr.p_rts_flags=%lX dst_ep=%d dst_ptr->p_usr.p_rts_flags=%lX\n"
		,dc_ptr->dc_usr.dc_dcid, src_ptr->p_usr.p_endpoint
		,src_ptr->p_usr.p_rts_flags, dst_ptr->p_usr.p_endpoint,dst_ptr->p_usr.p_rts_flags);
		
	m_ptr = &h_ptr->c_u.cu_msg;
	MOLDEBUG(DBGMESSAGE,MSG9_FORMAT,MSG9_FIELDS(m_ptr));	
		
	if( dst_ptr->p_priv.s_usr.s_id >= dc_ptr->dc_usr.dc_nr_sysprocs)
		ERROR_PRINT(EMOLPRIVILEGES);	

	MOLDEBUG(INTERNAL,"src_nr=%d\n",h_ptr->c_u.cu_msg.m_type & (~NOTIFY_MESSAGE));
	sid = _ENDPOINT_P(h_ptr->c_u.cu_msg.m_type & (~NOTIFY_MESSAGE))
						+dc_ptr->dc_usr.dc_nr_tasks;
	MOLDEBUG(INTERNAL,"sid=%d\n",sid);
	if(sid >= dc_ptr->dc_usr.dc_nr_sysprocs){
		ERROR_RETURN(EMOLPRIVILEGES);
	}
	
	if( src_ptr->p_priv.s_usr.s_id >= dc_ptr->dc_usr.dc_nr_sysprocs)
		ERROR_PRINT(EMOLPRIVILEGES);	
	
  	/* Check to see if target is blocked waiting for this message. A process 
   	* can be both sending and receiving during a SENDREC system call.
  	 */
	if ( (test_bit(BIT_RECEIVING, &dst_ptr->p_usr.p_rts_flags) && 
		!test_bit(BIT_SENDING, &dst_ptr->p_usr.p_rts_flags)) &&
      	(dst_ptr->p_usr.p_getfrom == ANY || dst_ptr->p_usr.p_getfrom == src_ptr->p_usr.p_endpoint)) {
		MOLDEBUG(GENERIC,"destination is waiting. Build and copy the message to dst userspace \n");

		if( update_proc >= 0 &&  update_proc < (sizeof(update_t)*8) ) {
			MOLDEBUG(INTERNAL,"set_bit update_proc=%d\n",update_proc);
			set_bit(update_proc, &dst_ptr->p_priv.s_updt_pending); 
		}
		
		if( test_bit(MIS_BIT_KTHREAD, &rproxy_ptr->p_usr.p_misc_flags))	{
			COPY_KRN2USR(rcode, NONE,  (void *) rproxy_ptr->p_umsg,
				dst_ptr,(void *) dst_ptr->p_umsg, sizeof(message));			
		}else{
			COPY_USR2USR_PROC(rcode, NONE, rproxy_ptr, (void *)&usr_h_ptr->c_u.cu_msg,
				dst_ptr,(void *) dst_ptr->p_umsg, sizeof(message));
		}	
		
		clear_bit(BIT_RECEIVING, &dst_ptr->p_usr.p_rts_flags);
		dst_ptr->p_usr.p_getfrom 	= NONE;
		if(dst_ptr->p_usr.p_rts_flags == 0)
			LOCAL_PROC_UP(dst_ptr, OK); /* ATENTI puede haber mas de una razon para despertar al proceso!! */
	}else{
		/* Set the NOTIFY pending bit in the destination flags */
		set_bit(MIS_BIT_NOTIFY, &dst_ptr->p_usr.p_misc_flags);

		if(get_sys_bit(dst_ptr->p_priv.s_notify_pending, sid))
			ERROR_PRINT(EMOLOVERRUN);
			
		m_ptr = &h_ptr->c_u.cu_msg;
		MOLDEBUG(DBGMESSAGE,MSG9_FORMAT,MSG9_FIELDS(m_ptr));
		
		/* Add to destination the bit map with pending notifications  */
		set_sys_bit(dst_ptr->p_priv.s_notify_pending, sid); 
		
		if( update_proc >= 0 &&  update_proc < (sizeof(update_t)*8)) {
			MOLDEBUG(INTERNAL,"set_bit update_proc=%d\n",update_proc);
			set_bit(update_proc,&dst_ptr->p_priv.s_updt_pending); 
		}
	}
	return(OK);
}

/*--------------------------------------------------------------*/
/*			copyrmt_rqst_rmt2lcl			*/
/* a Remote process (requester) has request a data copy 	*/
/* between 2 remote  processes 					*/
/* rmt_ptr = requester 						*/
/* lcl_ptr = the local process that receive the request    	*/
/*          that is the same as the source for data 		*/
/* The DC mutex must be locked !!!				*/
/* lcl_ptr and rmt_ptr must be locked 				*/
/*--------------------------------------------------------------*/
asmlinkage long copyrmt_rqst_rmt2lcl(struct proc *rproxy_ptr, struct proc *rmt_ptr, 
		struct proc *lcl_ptr, proxy_hdr_t *h_ptr, proxy_payload_t *usr_pay_ptr)
{
	int ret;
	dc_desc_t *dc_ptr;
	int dst_nr;
	struct proc *dst_ptr;

	MOLDEBUG(DBGCMD,CMD_FORMAT,CMD_FIELDS(h_ptr));
	MOLDEBUG(DBGVCOPY,VCOPY_FORMAT,VCOPY_FIELDS(h_ptr));

	dc_ptr 	= &dc[lcl_ptr->p_usr.p_dcid];
	do {
		ret = OK;
		if( h_ptr->c_u.cu_vcopy.v_bytes <= 0 
			|| h_ptr->c_u.cu_vcopy.v_bytes > MAXCOPYLEN) 
						{ret = EMOLRANGE; break;}

		/* the local process that receive the request is the same as sender of data */
		if( test_bit(BIT_ONCOPY, &lcl_ptr->p_usr.p_rts_flags))	{ret = EMOLONCOPY; break;}
		if( lcl_ptr->p_usr.p_endpoint !=  h_ptr->c_u.cu_vcopy.v_src)	{ret = EMOLBADPROC; break;}
		MOLDEBUG(INTERNAL,"SRC OK endpoint=%d\n", lcl_ptr->p_usr.p_endpoint);
	
		/*Checks Destination process */
		dst_nr = _ENDPOINT_P(h_ptr->c_u.cu_vcopy.v_dst);
		if( dst_nr < (-dc_ptr->dc_usr.dc_nr_tasks) 
   		 || dst_nr >= dc_ptr->dc_usr.dc_nr_procs) 	{ret = EMOLRANGE; break;}
		dst_ptr   = NBR2PTR(dc_ptr, dst_nr);

	}while(0);
	if(ret) ERROR_RETURN(ret);
	
	/* Set ONCOPY bits to protect the descriptors while they are UNLOCKED */
	set_bit(BIT_ONCOPY, &lcl_ptr->p_usr.p_rts_flags);
	set_bit(BIT_ONCOPY, &rmt_ptr->p_usr.p_rts_flags);
	WUNLOCK_PROC2(lcl_ptr, rmt_ptr);

	WLOCK_PROC3(lcl_ptr, rmt_ptr, dst_ptr);
	clear_bit(BIT_ONCOPY, &lcl_ptr->p_usr.p_rts_flags);
	clear_bit(BIT_ONCOPY, &rmt_ptr->p_usr.p_rts_flags);
	do {
		ret = OK;
		if(IT_IS_LOCAL(dst_ptr) ) 				{ret = EMOLLCLPROC; break;}
		if( test_bit(BIT_SLOT_FREE, &dst_ptr->p_usr.p_rts_flags))	{ret = EMOLDSTDIED; break;}
		if( dst_ptr->p_usr.p_endpoint != h_ptr->c_u.cu_vcopy.v_dst)  {ret = EMOLENDPOINT; break;}
		if( test_bit(BIT_ONCOPY, &dst_ptr->p_usr.p_rts_flags))	{ret = EMOLONCOPY; break;}
		if( test_bit(BIT_RMTOPER, &dst_ptr->p_usr.p_rts_flags))	{ret = EMOLENQUEUED; break;}
		if( test_bit(BIT_SENDING, &dst_ptr->p_usr.p_rts_flags))	{ret = EMOLENQUEUED; break;}
		MOLDEBUG(INTERNAL,"DST OK endpoint=%d\n", dst_ptr->p_usr.p_endpoint);
	}while(0);
	if(ret == OK) {
		/* copy the header to keep the vcopy data  */ 
		memcpy( &dst_ptr->p_rmtcmd.c_u.cu_vcopy, &h_ptr->c_u.cu_vcopy, sizeof(vcopy_t));
		set_bit(BIT_ONCOPY, &dst_ptr->p_usr.p_rts_flags);
		ret = copyin_rqst_lcl2rmt(dst_ptr, lcl_ptr);
	}else {
		if(ret != EMOLENQUEUED)
			ret = copyrmt_ack_lcl2rmt(rmt_ptr, lcl_ptr, ret);			
	}
	
	WUNLOCK_PROC(dst_ptr);  
	
	return(ret);
}


/*--------------------------------------------------------------*/
/*			copylcl_rqst_rmt2lcl			*/
/* a Remote process (requester) has request a data copy 	*/
/* between 2 local processes 					*/
/* rmt_ptr = requester 						*/
/* lcl_ptr = src_ptr					    	*/
/* The DC mutex must be locked !!!				*/
/* lcl_ptr and rmt_ptr must be locked 				*/
/*--------------------------------------------------------------*/
asmlinkage long copylcl_rqst_rmt2lcl(struct proc *rproxy_ptr, struct proc *rmt_ptr, 
		struct proc *lcl_ptr, proxy_hdr_t *h_ptr, proxy_payload_t *usr_pay_ptr)
{
	int ret;
	dc_desc_t *dc_ptr;
	int dst_nr;
	struct proc *dst_ptr;

	MOLDEBUG(DBGCMD,CMD_FORMAT,CMD_FIELDS(h_ptr));
	MOLDEBUG(DBGVCOPY,VCOPY_FORMAT,VCOPY_FIELDS(h_ptr));

	dc_ptr 	= &dc[lcl_ptr->p_usr.p_dcid];
	/*Checks Source process */
	do {
		ret = OK;

		if( h_ptr->c_u.cu_vcopy.v_bytes <= 0 
			|| h_ptr->c_u.cu_vcopy.v_bytes > MAXCOPYLEN) 	{ret = EMOLRANGE; break;}

		/* the local process that receive the request is the same as sender of data */
		if( test_bit(BIT_ONCOPY, &lcl_ptr->p_usr.p_rts_flags))	{ret = EMOLONCOPY; break;}
		if( lcl_ptr->p_usr.p_endpoint !=  h_ptr->c_u.cu_vcopy.v_src)	{ret = EMOLBADPROC; break;}
		MOLDEBUG(INTERNAL,"LCL OK endpoint=%d\n", lcl_ptr->p_usr.p_endpoint);

		/*Checks Destination process */
		dst_nr = _ENDPOINT_P(h_ptr->c_u.cu_vcopy.v_dst);
		if( dst_nr < (-dc_ptr->dc_usr.dc_nr_tasks) 
			|| dst_nr >= dc_ptr->dc_usr.dc_nr_procs) 	{ret = EMOLRANGE; break;}
		dst_ptr   = NBR2PTR(dc_ptr, dst_nr);
	}while(0);
	if(ret) ERROR_RETURN(ret);
	
	/* Set ONCOPY bits to protect the descriptors while they are UNLOCKED */
	set_bit(BIT_ONCOPY, &lcl_ptr->p_usr.p_rts_flags);
	set_bit(BIT_ONCOPY, &rmt_ptr->p_usr.p_rts_flags);
	WUNLOCK_PROC2(lcl_ptr, rmt_ptr);

	WLOCK_PROC3(lcl_ptr, rmt_ptr, dst_ptr);
	clear_bit(BIT_ONCOPY, &lcl_ptr->p_usr.p_rts_flags);
	clear_bit(BIT_ONCOPY, &rmt_ptr->p_usr.p_rts_flags);
	do {
		ret = OK;
		if( IT_IS_REMOTE(dst_ptr) ) 				{ret = EMOLRMTPROC; break;}
		if( test_bit(BIT_SLOT_FREE, &dst_ptr->p_usr.p_rts_flags))	{ret = EMOLDSTDIED; break;}
		if( dst_ptr->p_usr.p_endpoint != h_ptr->c_u.cu_vcopy.v_dst)  {ret = EMOLENDPOINT; break;}
		if( test_bit(BIT_ONCOPY, &dst_ptr->p_usr.p_rts_flags))	{ret = EMOLONCOPY; break;}
		if( test_bit(BIT_SENDING, &dst_ptr->p_usr.p_rts_flags))	{ret = EMOLENQUEUED; break;}
		MOLDEBUG(INTERNAL,"DST OK endpoint=%d\n", dst_ptr->p_usr.p_endpoint);
	}while(0);
	if(ret == OK) {
		COPY_USR2USR_PROC(ret, NONE, lcl_ptr, h_ptr->c_u.cu_vcopy.v_saddr,
				dst_ptr, h_ptr->c_u.cu_vcopy.v_daddr, h_ptr->c_u.cu_vcopy.v_bytes);
		memcpy( &rmt_ptr->p_rmtcmd.c_u.cu_vcopy, &h_ptr->c_u.cu_vcopy, sizeof(vcopy_t));
	}
	ret = copylcl_ack_lcl2rmt(rmt_ptr, lcl_ptr, ret);	
	WUNLOCK_PROC(dst_ptr); 
	return(ret);
}

/*--------------------------------------------------------------*/
/*			copyin_data_rmt2lcl			*/
/* proxy sends a remote data to a local process address space   */
/* The DC mutex must be locked !!!				*/
/* lcl_ptr and rmt_ptr must be locked 				*/
/*--------------------------------------------------------------*/
asmlinkage long copyin_data_rmt2lcl(struct proc *rproxy_ptr, struct proc *rmt_ptr, 
		struct proc *lcl_ptr, proxy_hdr_t *h_ptr, proxy_payload_t *usr_pay_ptr)
{
	int ret;

	MOLDEBUG(DBGCMD,CMD_FORMAT,CMD_FIELDS(h_ptr));
	MOLDEBUG(DBGVCOPY,VCOPY_FORMAT,VCOPY_FIELDS(h_ptr));

	do {
		ret = OK;
		if( h_ptr->c_u.cu_vcopy.v_bytes <= 0 
		|| h_ptr->c_u.cu_vcopy.v_bytes > NODE2MAXBYTES(h_ptr->c_snode))	{ret = EMOLRANGE ;break;}

		/* the local process that receive the request is the same as sender of data */
		if( test_bit(BIT_ONCOPY, &lcl_ptr->p_usr.p_rts_flags))	{ret = EMOLONCOPY; break;}
		if( test_bit(BIT_SENDING, &lcl_ptr->p_usr.p_rts_flags))	{ret = EMOLENQUEUED; break;}
		if( lcl_ptr->p_usr.p_endpoint !=  h_ptr->c_u.cu_vcopy.v_dst)	{ret = EMOLBADPROC; break;}
		MOLDEBUG(INTERNAL,"DST OK endpoint=%d\n", lcl_ptr->p_usr.p_endpoint);

		/* Copy Data from RPROXY to local process */
		if( test_bit(MIS_BIT_KTHREAD, &rproxy_ptr->p_usr.p_misc_flags))	{
			COPY_KRN2USR(ret, NONE,(char *)usr_pay_ptr, lcl_ptr, 
				h_ptr->c_u.cu_vcopy.v_daddr, h_ptr->c_u.cu_vcopy.v_bytes);
		}else{
			COPY_USR2USR_PROC(ret, NONE, rproxy_ptr, (char *)usr_pay_ptr,
				lcl_ptr, h_ptr->c_u.cu_vcopy.v_daddr, h_ptr->c_u.cu_vcopy.v_bytes);
		}
	}while(0);
	memcpy( &rmt_ptr->p_rmtcmd.c_u.cu_vcopy, &h_ptr->c_u.cu_vcopy, sizeof(vcopy_t));		
	ret = copyin_ack_lcl2rmt(rmt_ptr, lcl_ptr, ret);	
	return(ret);
}

/*--------------------------------------------------------------*/
/*			copyin_rqst_rmt2lcl			*/
/* proxy sends a remote data to a local process address space   */
/* The DC mutex must be locked !!!				*/
/* lcl_ptr and rmt_ptr must be locked 				*/
/*--------------------------------------------------------------*/
asmlinkage long copyin_rqst_rmt2lcl(struct proc *rproxy_ptr, struct proc *rmt_ptr, 
		struct proc *lcl_ptr, proxy_hdr_t *h_ptr, proxy_payload_t *usr_pay_ptr)
{
	int ret;
	struct proc *rqtr_ptr;
	dc_desc_t *dc_ptr;

	MOLDEBUG(DBGCMD,CMD_FORMAT,CMD_FIELDS(h_ptr));
	MOLDEBUG(DBGVCOPY,VCOPY_FORMAT,VCOPY_FIELDS(h_ptr));

	dc_ptr 	= &dc[h_ptr->c_dcid];
	rqtr_ptr = ENDPOINT2PTR(dc_ptr, h_ptr->c_u.cu_vcopy.v_rqtr);

	/* set the ONCOPY bits to protect the descriptor while unlocked */
	set_bit(BIT_ONCOPY, &lcl_ptr->p_usr.p_rts_flags);
	set_bit(BIT_ONCOPY, &rmt_ptr->p_usr.p_rts_flags);
	WUNLOCK_PROC2(rmt_ptr, lcl_ptr);

	WLOCK_PROC3(rqtr_ptr, rmt_ptr, lcl_ptr);
	clear_bit(BIT_ONCOPY, &lcl_ptr->p_usr.p_rts_flags);
	clear_bit(BIT_ONCOPY, &rmt_ptr->p_usr.p_rts_flags);
	do {
		ret = OK;
		if(IT_IS_LOCAL(rqtr_ptr))		{ret = EMOLLCLPROC; break;}
		if( h_ptr->c_u.cu_vcopy.v_bytes <= 0 
			|| h_ptr->c_u.cu_vcopy.v_bytes > NODE2MAXBYTES(h_ptr->c_snode)) 
							{ret = EMOLRANGE; break;}

		if( test_bit(BIT_ONCOPY, &rqtr_ptr->p_usr.p_rts_flags))	{ret = EMOLONCOPY; break;}
		if( test_bit(BIT_RMTOPER, &rqtr_ptr->p_usr.p_rts_flags)){ret = EMOLENQUEUED; break;}
		if( test_bit(BIT_SENDING, &rqtr_ptr->p_usr.p_rts_flags)) {ret = EMOLENQUEUED; break;}
		if( test_bit(MIS_BIT_KTHREAD, &rproxy_ptr->p_usr.p_misc_flags))	{
			COPY_KRN2USR(ret, NONE,(char *)usr_pay_ptr, 
				lcl_ptr, h_ptr->c_u.cu_vcopy.v_daddr, h_ptr->c_u.cu_vcopy.v_bytes);
		}else{
			COPY_USR2USR_PROC(ret, NONE, rproxy_ptr, (char *)usr_pay_ptr,
				lcl_ptr, h_ptr->c_u.cu_vcopy.v_daddr, h_ptr->c_u.cu_vcopy.v_bytes);	
		}
	}while(0);
	memcpy( &rqtr_ptr->p_rmtcmd.c_u.cu_vcopy, &h_ptr->c_u.cu_vcopy, sizeof(vcopy_t));
	if( ret != EMOLENQUEUED)
		copyrmt_ack_lcl2rmt(rqtr_ptr, lcl_ptr, ret);
	WUNLOCK_PROC(rqtr_ptr);
	
	return(ret);
}
/*--------------------------------------------------------------*/
/*			copyout_rqst_rmt2lcl				*/
/*  Enqueue a copyout_data into the sender proxy		*/
/*--------------------------------------------------------------*/
asmlinkage long copyout_rqst_rmt2lcl( struct proc *rproxy_ptr, struct proc *rmt_ptr, 
		struct proc *lcl_ptr, proxy_hdr_t *h_ptr, proxy_payload_t *usr_pay_ptr)
{
	int ret;

	MOLDEBUG(DBGCMD,CMD_FORMAT,CMD_FIELDS(h_ptr));
	MOLDEBUG(DBGVCOPY,VCOPY_FORMAT,VCOPY_FIELDS(h_ptr));

	do {
		ret = OK;
		if( h_ptr->c_u.cu_vcopy.v_bytes <= 0 
			|| h_ptr->c_u.cu_vcopy.v_bytes > NODE2MAXBYTES(h_ptr->c_snode)) {ret = EMOLRANGE; break;}
		if( lcl_ptr->p_usr.p_rts_flags == PROC_RUNNING)	{ret = EMOLPROCSTS; break;}

		/* the local process that receive the request is the same as sender of data */
		if( test_bit(BIT_ONCOPY, &lcl_ptr->p_usr.p_rts_flags))	{ret = EMOLONCOPY; break;}
		if( lcl_ptr->p_usr.p_endpoint !=  h_ptr->c_u.cu_vcopy.v_src)	{ret = EMOLBADPROC; break;}
	MOLDEBUG(INTERNAL,"SRC OK endpoint=%d\n", lcl_ptr->p_usr.p_endpoint);

		memcpy( &rmt_ptr->p_rmtcmd.c_u.cu_vcopy, &h_ptr->c_u.cu_vcopy, sizeof(vcopy_t));
	}while(0);
	ret = copyout_data_lcl2rmt(rmt_ptr, lcl_ptr, ret);
	return(ret);
}

/*--------------------------------------------------------------*/
/*			copyout_data_rmt2lcl			*/
/* proxy sends a remote data to a local process address space  */
/* lcl_ptr: is the requester of the copyout 			*/
/* rmt_ptr: is the sender of the data block			*/
/* dst_ptr: is de destination of the data block			*/
/*--------------------------------------------------------------*/
asmlinkage long copyout_data_rmt2lcl(struct proc *rproxy_ptr, struct proc *rmt_ptr, 
		struct proc *lcl_ptr, proxy_hdr_t *h_ptr, proxy_payload_t *usr_pay_ptr)
{
	int ret;
	struct proc *sproxy_ptr, *dst_ptr;
	dc_desc_t *dc_ptr;

	MOLDEBUG(DBGCMD,CMD_FORMAT,CMD_FIELDS(h_ptr));
	MOLDEBUG(DBGVCOPY,VCOPY_FORMAT,VCOPY_FIELDS(h_ptr));

	if( h_ptr->c_u.cu_vcopy.v_bytes <= 0 
	|| h_ptr->c_u.cu_vcopy.v_bytes > NODE2MAXBYTES(h_ptr->c_snode)) {
		ERROR_RETURN(EMOLRANGE);
	}
	
	sproxy_ptr = NODE2SPROXY(rmt_ptr->p_usr.p_nodeid);
	dc_ptr 	= &dc[lcl_ptr->p_usr.p_dcid];

	dst_ptr = ENDPOINT2PTR(dc_ptr, h_ptr->c_u.cu_vcopy.v_dst);
	if( dst_ptr->p_usr.p_endpoint != lcl_ptr->p_usr.p_endpoint) {
		WUNLOCK_PROC2(rmt_ptr, lcl_ptr);
		WLOCK_PROC3(rmt_ptr, lcl_ptr, dst_ptr);
	}
	
	do {
		ret = OK;

		if( lcl_ptr->p_usr.p_rts_flags == PROC_RUNNING) {ret= EMOLPROCRUN; break;}
		if( dst_ptr->p_usr.p_rts_flags  == PROC_RUNNING) {ret= EMOLPROCRUN; break;}

		/*check that all envolved processes are on ONCOPY state */
		if(!test_bit(BIT_ONCOPY, &lcl_ptr->p_usr.p_rts_flags))	{ret= EMOLPROCSTS; break;}
	    if(!test_bit(BIT_ONCOPY, &dst_ptr->p_usr.p_rts_flags))	{ret= EMOLPROCSTS; break;}
	    if(!test_bit(BIT_ONCOPY, &rmt_ptr->p_usr.p_rts_flags))	{ret= EMOLPROCSTS; break;}
	}while(0);
	if(ret == OK) {
		if( h_ptr->c_rcode == OK){
			if( test_bit(MIS_BIT_KTHREAD, &rproxy_ptr->p_usr.p_misc_flags))	{
				COPY_KRN2USR(ret, NONE,(char *)usr_pay_ptr, 
					dst_ptr, h_ptr->c_u.cu_vcopy.v_daddr, h_ptr->c_u.cu_vcopy.v_bytes);
			}else{
				COPY_USR2USR_PROC(ret, NONE, rproxy_ptr, (char *) usr_pay_ptr,
					dst_ptr, h_ptr->c_u.cu_vcopy.v_daddr, h_ptr->c_u.cu_vcopy.v_bytes);
			}
		}
		/* wake up the requester */
		if( lcl_ptr->p_usr.p_rts_flags == ONCOPY)
			READY_UP_RCODE(lcl_ptr, CMD_COPYOUT_DATA, h_ptr->c_rcode);
	}
	if( dst_ptr->p_usr.p_endpoint != lcl_ptr->p_usr.p_endpoint) {
		WUNLOCK_PROC(dst_ptr);
	}
	return(ret);
}



/*--------------------------------------------------------------*/
/*			mm_put2lcl				*/
/* RECEIVER proxy makes an operation to a local process requested*/
/* by a remote process						*/
/*--------------------------------------------------------------*/
asmlinkage long mm_put2lcl(proxy_hdr_t *usr_hdr_ptr, proxy_payload_t *usr_pay_ptr)
{
	dc_desc_t *dc_ptr;
	struct proc *rmt_ptr, *lcl_ptr, *rqtr_ptr, *rproxy_ptr, *sproxy_ptr;
	proxy_hdr_t *h_ptr;
	int ret, rmt_nr, lcl_nr, dcid, ack;
	long unsigned int *bm_ptr;
	int rproxy_pid;
	struct task_struct *task_ptr;	
	cluster_node_t *node_ptr;
	struct timespec *t_ptr;

	if(current_euid() != USER_ROOT) return(-EPERM);

	if( DVS_NOT_INIT() )   return(EMOLDVSINIT );

	ret = check_caller(&task_ptr, &rproxy_ptr, &rproxy_pid);
	if(ret) return(ret);
	
	ret = OK;
	WLOCK_PROC(rproxy_ptr);
	/*------------------------------------------
	  * checks the if the caller is a proxy
	 *  and it is in the correct state 
	  *------------------------------------------*/
	do {
		if( test_bit( BIT_SLOT_FREE, &rproxy_ptr->p_usr.p_rts_flags))	{ret = EMOLPROCSTS;break;} 
		if( !test_bit(MIS_BIT_PROXY, &rproxy_ptr->p_usr.p_misc_flags))	{ret = EMOLNOPROXY;break;} 
		if( !test_bit(MIS_BIT_CONNECTED, &rproxy_ptr->p_usr.p_misc_flags)){ret = EMOLNOTCONN;break;}  
	} while(0);
	if(ret) {
		WUNLOCK_PROC(rproxy_ptr);
		ERROR_RETURN(ret);
	}
	if( test_bit(MIS_BIT_KTHREAD, &rproxy_ptr->p_usr.p_misc_flags))	{
		/*copy the header */
		memcpy(rproxy_ptr->p_umsg, &usr_hdr_ptr->c_u.cu_msg, sizeof(message));	
		h_ptr = usr_hdr_ptr;
	}else{ 
		/*copy the header from user space to kernel */
		rproxy_ptr->p_umsg = (message * )usr_pay_ptr;
		ret = copy_from_user(&rproxy_ptr->p_rmtcmd, usr_hdr_ptr, sizeof(proxy_hdr_t));
		h_ptr = &rproxy_ptr->p_rmtcmd;
	}
	WUNLOCK_PROC(rproxy_ptr);
	MOLDEBUG(DBGCMD,HDR_FORMAT, HDR_FIELDS(h_ptr));

	/*------------------------------------------
 	 * Gets the DCID
     *------------------------------------------*/
	dcid		= h_ptr->c_dcid;
	MOLDEBUG(INTERNAL,"dcid=%d\n", dcid);
	if( dcid < 0 || dcid >= dvs.d_nr_dcs) 
			ERROR_RETURN(EMOLBADDCID);
	dc_ptr 	= &dc[dcid];

	ret = OK;
	lcl_nr = _ENDPOINT_P(h_ptr->c_dst);
	rmt_nr = _ENDPOINT_P(h_ptr->c_src);
	lcl_ptr = NULL;
	rmt_ptr = NULL;

	/*------------------------------------------
	 * Checks if the DC is RUNNING
	 * Locks the DC
	 * Checks local and remote process 
     *------------------------------------------*/	
	WLOCK_DC(dc_ptr);
	do {
		if( dc_ptr->dc_usr.dc_flags) 	
			{ret = EMOLDCNOTRUN; break;}

		if( rmt_nr < (-dc_ptr->dc_usr.dc_nr_tasks) 
 	 		|| rmt_nr >= dc_ptr->dc_usr.dc_nr_procs) 
			{ret = EMOLRANGE; break;}

		/*Checks Destination (LOCAL)  process */
		if( lcl_nr < (-dc_ptr->dc_usr.dc_nr_tasks) 
   		 || lcl_nr >= dc_ptr->dc_usr.dc_nr_procs) 
			{ret = EMOLRANGE; break;}
									
		/* Checks Source NODE */
		bm_ptr = &dc_ptr->dc_usr.dc_nodes;
		node_ptr = &node[h_ptr->c_snode];
		if(!test_bit(h_ptr->c_snode, bm_ptr)) 				
			{ret = EMOLNODCNODE; break;}
		if( h_ptr->c_dnode != atomic_read(&local_nodeid))	
			{ret = EMOLBADNODEID; break;}	

 	}while(0);
	if(ret)	{
		WUNLOCK_DC(dc_ptr);
		ERROR_RETURN(ret);
	}

	node_ptr->n_usr.n_rtimestamp = current_kernel_time();	
	t_ptr = &node_ptr->n_usr.n_rtimestamp;
	MOLDEBUG(INTERNAL,TIME_FORMAT, TIME_FIELDS(t_ptr));		

	rmt_ptr  = NBR2PTR(dc_ptr, rmt_nr);
	lcl_ptr  = NBR2PTR(dc_ptr, lcl_nr);

	WLOCK_PROC2(lcl_ptr, rmt_ptr);
	do{ 

#ifdef MOL_AUTOBIND 	
		if( test_bit(BIT_SLOT_FREE, &rmt_ptr->p_usr.p_rts_flags)) { /* AUTO REMOTE BIND */
			MOLDEBUG(INTERNAL,"AUTO REMOTE BIND endpoint=%d node=%d\n", h_ptr->c_src, h_ptr->c_snode);
			ret = do_autobind(dc_ptr, rmt_ptr, h_ptr->c_src, h_ptr->c_snode);
			if( ret < 0 ) {break;}
			ret = OK;		/* because mol_bind return the process endpoint */
		}

		if( IT_IS_REMOTE(rmt_ptr) 
		 && (rmt_ptr->p_usr.p_endpoint != h_ptr->c_src)) { /* AUTO - REMOTE UNBIND */
		MOLDEBUG(INTERNAL,"AUTO REMOTE UNBIND current=%d new=%d\n", rmt_ptr->p_usr.p_endpoint, h_ptr->c_src);
			ret = do_unbind(dc_ptr, rmt_ptr);
			if(ret) {break;}
		}
#else
		/* Check if the slot is free */
		if( test_bit(BIT_SLOT_FREE, &rmt_ptr->p_usr.p_rts_flags)) {
			MOLDEBUG(INTERNAL,"REMOTE EMOLNOTBIND endpoint=%d p_nr=%d\n", 
				h_ptr->c_src, rmt_ptr->p_usr.p_nr);
			ret = EMOLNOTBIND;
			break;
		}
		
		/* check if the source endpoint match local slot's endpoint (old process die, new born) */
		if( IT_IS_REMOTE(rmt_ptr) && (rmt_ptr->p_usr.p_endpoint != h_ptr->c_src)) { 
			MOLDEBUG(INTERNAL,"REMOTE EMOLENDPOINT endpoint=%d source=%d\n", 
				rmt_ptr->p_usr.p_endpoint, h_ptr->c_src);
			ret = EMOLENDPOINT;
			break;
		}

		/* check if the source node match slot node (could be migrated) */
		if( IT_IS_REMOTE(rmt_ptr) && (rmt_ptr->p_usr.p_nodeid != h_ptr->c_snode)) { 
			MOLDEBUG(INTERNAL,"REMOTE EMOLNONODE p_nodeid=%d c_snode=%d\n", 
				rmt_ptr->p_usr.p_endpoint, h_ptr->c_src);
			ret = EMOLNONODE;
			break;
		}
		
#endif  /* MOL_AUTOBIND */ 	
		
		if(IT_IS_LOCAL(rmt_ptr) && 
			!TEST_BIT(rmt_ptr->p_usr.p_misc_flags, MIS_BIT_REPLICATED))	{ret = EMOLLCLPROC; break;}
		if( test_bit(BIT_RMTOPER, &rmt_ptr->p_usr.p_rts_flags)) {ret = EMOLENQUEUED; break;}
		if( test_bit(BIT_SENDING, &rmt_ptr->p_usr.p_rts_flags)) {ret = EMOLENQUEUED; break;}
		MOLDEBUG(INTERNAL,"REMOTE source OK endpoint=%d\n", rmt_ptr->p_usr.p_endpoint);		

		if( IT_IS_REMOTE(lcl_ptr) ) 							{ret = EMOLRMTPROC; break;}
		if( test_bit(BIT_SLOT_FREE, &lcl_ptr->p_usr.p_rts_flags))	{ret = EMOLNOTBIND; break;}
		if( lcl_ptr->p_usr.p_endpoint != h_ptr->c_dst) 		{ret = EMOLENDPOINT; break;}
		if( test_bit(BIT_MIGRATE, &lcl_ptr->p_usr.p_rts_flags)) {ret = EMOLMIGRATE; break;}
		if( lcl_ptr->p_usr.p_nodeid != atomic_read(&local_nodeid)) {ret = EMOLMIGRATE; break;}

		MOLDEBUG(INTERNAL,"LOCAL destination OK endpoint=%d\n", lcl_ptr->p_usr.p_endpoint);

	}while(0);
	
	if (ret) {
		switch(ret){
			/* Acknoledge with ERROR to the REMOTE is not necessary */
			case EMOLNOTBIND:
			case EMOLNONODE: 
			case EMOLLCLPROC:
			case EMOLENQUEUED:
				WUNLOCK_DC(dc_ptr);
				WUNLOCK_PROC2(lcl_ptr, rmt_ptr);
				ERROR_RETURN(ret);
				break;
			default:
				break;
		}
		/* Acknoledge with ERROR to the REMOTE sender if it is necessary */
		ack = CMD_NONE;
		sproxy_ptr = NODE2SPROXY(h_ptr->c_snode);
		WLOCK_PROC(sproxy_ptr);
		if( test_bit(MIS_BIT_CONNECTED, &sproxy_ptr->p_usr.p_misc_flags)){
			WUNLOCK_PROC(sproxy_ptr);
			switch(h_ptr->c_cmd) {
				case CMD_SNDREC_MSG:		// ONLY ACK ON ERROR	
				case CMD_SEND_MSG:
				case CMD_REPLY_MSG:
					ack = CMD_SEND_ACK;
					break;
				case CMD_NTFY_MSG:
					ack = CMD_NONE; /* Avoid sending an ACK */
					break;
				case CMD_COPYIN_DATA:
					ack = CMD_COPYIN_ACK;
					break;
				case CMD_COPYIN_RQST:	
					/* reply to requester, not to sender */
					rqtr_ptr = ENDPOINT2PTR(dc_ptr, h_ptr->c_u.cu_vcopy.v_rqtr);
					if(IT_IS_LOCAL(rqtr_ptr)) {
						ret = EMOLLCLPROC; 
					}else{
						ack = CMD_COPYRMT_ACK; /* change the ACK type */
						error_lcl2rmt(ack, rqtr_ptr, h_ptr,  ret);
					}
					ack = CMD_NONE; /* Avoid sending an ACK */
					break;
				case CMD_COPYOUT_RQST:
					ack = CMD_COPYOUT_DATA;
					break;
				case CMD_COPYLCL_RQST:
					ack = CMD_COPYLCL_ACK;
					break;
				case CMD_COPYRMT_RQST:
					ack = CMD_COPYRMT_ACK;
					break;
				default:
					break;
			}
			
		}else{
			WUNLOCK_PROC(sproxy_ptr);
		}
		WUNLOCK_DC(dc_ptr);
		if(ack != CMD_NONE) error_lcl2rmt(ack, rmt_ptr, h_ptr,  ret);
		WUNLOCK_PROC2(lcl_ptr, rmt_ptr);

		ERROR_RETURN(ret);
	}

//	DC_INCREF(dc_ptr);
	WLOCK_PROC(rproxy_ptr);
	rproxy_ptr->p_usr.p_dcid = dc_ptr->dc_usr.dc_dcid;
	WUNLOCK_PROC(rproxy_ptr);
	RUNLOCK_DC(dc_ptr);

	switch(h_ptr->c_cmd) {
		/*----------------------------------------------------------------------------------------------*/
		/*		REMOTE REQUESTS 								*/
		/*----------------------------------------------------------------------------------------------*/
		case CMD_SNDREC_MSG:	/* The remote process has sent a message to a local process 		*/ 
			MOLDEBUG(INTERNAL,"CMD_SNDREC_MSG dcid=%d rmt_ep=%d rmt_nr=%d lcl_ep=%d lcl_nr=%d\n"
				,dcid, rmt_ptr->p_usr.p_endpoint, rmt_nr, lcl_ptr->p_usr.p_endpoint, lcl_nr);
			ret = send_rmt2lcl(usr_hdr_ptr, h_ptr, rmt_ptr, lcl_ptr);
			break;
		case CMD_SEND_MSG:	/* The remote process has sent a message to a local process 		*/ 
			MOLDEBUG(INTERNAL,"CMD_SEND_MSG dcid=%d rmt_ep=%d rmt_nr=%d lcl_ep=%d lcl_nr=%d\n"
				,dcid, rmt_ptr->p_usr.p_endpoint, rmt_nr, lcl_ptr->p_usr.p_endpoint, lcl_nr);
			ret = send_rmt2lcl(usr_hdr_ptr, h_ptr, rmt_ptr, lcl_ptr);
			break;
		case CMD_REPLY_MSG: 	/*  The remote process has sent a reply message to a local process	*/
			MOLDEBUG(INTERNAL,"CMD_REPLY_MSG dcid=%d rmt_ep=%d rmt_nr=%d lcl_ep=%d lcl_nr=%d\n"
				,dcid, rmt_ptr->p_usr.p_endpoint, rmt_nr, lcl_ptr->p_usr.p_endpoint, lcl_nr);
			ret = send_rmt2lcl(usr_hdr_ptr, h_ptr, rmt_ptr, lcl_ptr);
			break;
		case CMD_NTFY_MSG:	/* The remote process has sent a notify message to a local process 	*/
			MOLDEBUG(INTERNAL,"CMD_NTFY_MSG dcid=%d rmt_ep=%d rmt_nr=%d lcl_ep=%d lcl_nr=%d rcode=%d\n"
				,dcid, rmt_ptr->p_usr.p_endpoint, rmt_nr, lcl_ptr->p_usr.p_endpoint, lcl_nr,h_ptr->c_rcode);	
			ret = notify_rmt2lcl(usr_hdr_ptr, h_ptr, rmt_ptr, lcl_ptr, h_ptr->c_rcode);
			break;
		case CMD_COPYIN_DATA: 	/* The remote process has requested a COPYIN data into the local process space */
			MOLDEBUG(INTERNAL,"CMD_COPYIN_DATA dcid=%d rmt_ep=%d rmt_nr=%d lcl_ep=%d lcl_nr=%d\n"
				,dcid, rmt_ptr->p_usr.p_endpoint, rmt_nr, lcl_ptr->p_usr.p_endpoint, lcl_nr);
			ret = copyin_data_rmt2lcl(rproxy_ptr, rmt_ptr, lcl_ptr, h_ptr, usr_pay_ptr);
			break;
		case CMD_COPYIN_RQST: 	/* The remote process has requested a COPYIN data into the local process space */
			MOLDEBUG(INTERNAL,"CMD_COPYIN_RQST dcid=%d rmt_ep=%d rmt_nr=%d lcl_ep=%d lcl_nr=%d\n"
				,dcid, rmt_ptr->p_usr.p_endpoint, rmt_nr, lcl_ptr->p_usr.p_endpoint, lcl_nr);
			ret = copyin_rqst_rmt2lcl(rproxy_ptr, rmt_ptr, lcl_ptr, h_ptr, usr_pay_ptr);
			break;
		case CMD_COPYOUT_RQST: /* The remote process has requested to COPYOUT data from a local process space */
			MOLDEBUG(INTERNAL,"CMD_COPYOUT_RQST dcid=%d rmt_ep=%d rmt_nr=%d lcl_ep=%d lcl_nr=%d\n"
				,dcid, rmt_ptr->p_usr.p_endpoint, rmt_nr, lcl_ptr->p_usr.p_endpoint, lcl_nr);
			ret = copyout_rqst_rmt2lcl(rproxy_ptr, rmt_ptr, lcl_ptr, h_ptr, usr_pay_ptr);
			break;	
		case CMD_COPYLCL_RQST: /* The remote process has requested to COPY between 2 process on the local node  */
			MOLDEBUG(INTERNAL,"CMD_COPYLCL_RQST dcid=%d rmt_ep=%d rmt_nr=%d lcl_ep=%d lcl_nr=%d\n"
				,dcid, rmt_ptr->p_usr.p_endpoint, rmt_nr, lcl_ptr->p_usr.p_endpoint, lcl_nr);
			ret = copylcl_rqst_rmt2lcl(rproxy_ptr, rmt_ptr, lcl_ptr, h_ptr, usr_pay_ptr);
			break;	
		case CMD_COPYRMT_RQST: /* The remote process has requested to COPY between local process and other in other node  */
			MOLDEBUG(INTERNAL,"CMD_COPYRMT_RQST dcid=%d rmt_ep=%d rmt_nr=%d lcl_ep=%d lcl_nr=%d\n"
				,dcid, rmt_ptr->p_usr.p_endpoint, rmt_nr, lcl_ptr->p_usr.p_endpoint, lcl_nr);
			ret = copyrmt_rqst_rmt2lcl(rproxy_ptr, rmt_ptr, lcl_ptr, h_ptr, usr_pay_ptr);
			break;		
		/*----------------------------------------------------------------------------------------------*/
		/*		REMOTE ACKNOWLEGES/REPLIES 	 						*/
		/*----------------------------------------------------------------------------------------------*/
		case CMD_SEND_ACK: 	/* The remote process has sent a message acknowledge to local process	*/
			MOLDEBUG(INTERNAL,"CMD_SEND_ACK dcid=%d rmt_ep=%d rmt_nr=%d lcl_ep=%d lcl_nr=%d\n"
				,dcid, rmt_ptr->p_usr.p_endpoint, rmt_nr, lcl_ptr->p_usr.p_endpoint, lcl_nr);
			ret = send_ack_rmt2lcl(rmt_ptr, lcl_ptr, h_ptr->c_rcode);
			break;
		case CMD_COPYIN_ACK: 	/* The remote process has sent COPYIN acknowledge to a local process	*/
			MOLDEBUG(INTERNAL,"CMD_COPYIN_ACK dcid=%d rmt_ep=%d rmt_nr=%d lcl_ep=%d lcl_nr=%d\n"
				,dcid, rmt_ptr->p_usr.p_endpoint, rmt_nr, lcl_ptr->p_usr.p_endpoint, lcl_nr);	
			ret = copyin_ack_rmt2lcl(rmt_ptr, lcl_ptr, h_ptr->c_rcode);
			break;
		case CMD_COPYLCL_ACK: 	/* The remote process has sent COPYLCL acknowledge to a local process	*/
			MOLDEBUG(INTERNAL,"CMD_COPYLCL_ACK dcid=%d rmt_ep=%d rmt_nr=%d lcl_ep=%d lcl_nr=%d\n"
				,dcid, rmt_ptr->p_usr.p_endpoint, rmt_nr, lcl_ptr->p_usr.p_endpoint, lcl_nr);	
			ret = copylcl_ack_rmt2lcl(rmt_ptr, lcl_ptr, h_ptr->c_rcode);
			break;
		case CMD_COPYOUT_DATA: /* The remote process has sent requested DATA by a local process */
			MOLDEBUG(INTERNAL,"CMD_COPYOUT_DATA dcid=%d rmt_ep=%d rmt_nr=%d lcl_ep=%d lcl_nr=%d\n"
				,dcid, rmt_ptr->p_usr.p_endpoint, rmt_nr, lcl_ptr->p_usr.p_endpoint, lcl_nr);
			ret = copyout_data_rmt2lcl(rproxy_ptr, rmt_ptr, lcl_ptr, h_ptr, usr_pay_ptr);
			break;
		case CMD_COPYRMT_ACK: 	/* The remote process has sent COPYRMT acknowledge to a local process	*/
			MOLDEBUG(INTERNAL,"CMD_COPYRMT_ACK dcid=%d rmt_ep=%d rmt_nr=%d lcl_ep=%d lcl_nr=%d\n"
				,dcid, rmt_ptr->p_usr.p_endpoint, rmt_nr, lcl_ptr->p_usr.p_endpoint, lcl_nr);	
			ret = copyrmt_ack_rmt2lcl(rmt_ptr, lcl_ptr, h_ptr->c_rcode);
			break;
		default:
			ret = EMOLBADREQUEST;
			break;
	}

	if( ret == OK){
		rproxy_ptr->p_usr.p_lclsent++;
		node_ptr->n_usr.n_pxrcvd++;		
		if(h_ptr->c_cmd == CMD_COPYOUT_RQST || h_ptr->c_cmd == CMD_COPYRMT_RQST){
			lcl_ptr->p_usr.p_rmtcopy++;	
		}else if  (h_ptr->c_cmd == CMD_COPYLCL_RQST){
			lcl_ptr->p_usr.p_lclcopy++;				
		}
	} 	
	WUNLOCK_PROC2(lcl_ptr, rmt_ptr);

	/* If an error acknowledge says that the remote process does not exist or it is erroneous, unbind it in local node */
	if( h_ptr->c_cmd & CMD_ACKNOWLEDGE) { /* any kind of acknowledge message */
		if( h_ptr->c_rcode == EMOLRMTPROC
		 || h_ptr->c_rcode == EMOLDSTDIED
		 || h_ptr->c_rcode == EMOLENDPOINT) {
			RLOCK_DC(dc_ptr);
			WLOCK_PROC(rmt_ptr);
			do_unbind(dc_ptr, rmt_ptr);
			WUNLOCK_PROC(rmt_ptr);
			RUNLOCK_DC(dc_ptr);
			ret = h_ptr->c_rcode;
		 }	
	}  

	WLOCK_PROC(rproxy_ptr);
	rproxy_ptr->p_usr.p_dcid = (-1);
	WUNLOCK_PROC(rproxy_ptr);

//	DC_DECREF(dc_ptr);
	if(ret) ERROR_RETURN(ret);
	return(ret);
}

