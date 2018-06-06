/****************************************************************/
/*			MOL REMOTE ACKNOWLEDGES				*/
/****************************************************************/


/****************************************************************/
/****************************************************************/
/*	ACKNOWLEDGES FROM LOCAL NODE TO REMOTE NODE		*/
/****************************************************************/
/****************************************************************/

/*--------------------------------------------------------------*/
/*			generic_ack_lcl2rmt			*/
/* Enqueue into the sender's proxy queue the rmt_ptr   		*/
/*--------------------------------------------------------------*/
long generic_ack_lcl2rmt(int ack, struct proc *rmt_ptr, struct proc *lcl_ptr, int rcode)
{
	/* set the RMTOPER bit to signal that this REMOTE descriptor 	*/
	/* is envolved into a remote operation 			*/
MOLDEBUG(DBGPARAMS,"ack=%d rcode=%d\n", ack, rcode);

	set_bit(BIT_RMTOPER, &rmt_ptr->p_usr.p_rts_flags);

	rmt_ptr->p_rmtcmd.c_cmd   = ack;
	rmt_ptr->p_rmtcmd.c_dcid  = rmt_ptr->p_usr.p_dcid;
	rmt_ptr->p_rmtcmd.c_src   = lcl_ptr->p_usr.p_endpoint;	/* Source endpoint		*/
	rmt_ptr->p_rmtcmd.c_dst   = rmt_ptr->p_usr.p_endpoint;	/* Destination endpoint 	*/
	rmt_ptr->p_rmtcmd.c_snode = atomic_read(&local_nodeid);	/* source node			*/
	rmt_ptr->p_rmtcmd.c_dnode = rmt_ptr->p_usr.p_nodeid;	/* destination node		*/
	rmt_ptr->p_rmtcmd.c_len   = 0;
	rmt_ptr->p_rmtcmd.c_rcode = rcode;

	return(sproxy_enqueue(rmt_ptr));
}

/*--------------------------------------------------------------*/
/*			error_lcl2rmt				*/
/* Enqueue the rmt_ptr process descriptor into the proxy sender */
/* with an ACKNOWLEDGE with an error. 				*/
/* The error header is obtained from the received header 	*/
/*--------------------------------------------------------------*/
long error_lcl2rmt( int ack, struct proc *rmt_ptr, proxy_hdr_t *h_ptr, int rcode)
{
	/* set the RMTOPER bit to signal that this REMOTE descriptor 	*/
	/* is envolved into a remote operation 		*/
MOLDEBUG(DBGPARAMS,"ack=%d rcode=%d\n", ack, rcode);

	set_bit(BIT_RMTOPER, &rmt_ptr->p_usr.p_rts_flags);

	rmt_ptr->p_rmtcmd.c_cmd   = ack;
	rmt_ptr->p_rmtcmd.c_dcid  = h_ptr->c_dcid;
	rmt_ptr->p_rmtcmd.c_src   = h_ptr->c_dst;	/* Original destination endpoint is now the Source endpoint 	*/
	rmt_ptr->p_rmtcmd.c_dst   = rmt_ptr->p_usr.p_endpoint;
	rmt_ptr->p_rmtcmd.c_snode = atomic_read(&local_nodeid);	/* source node			*/
	rmt_ptr->p_rmtcmd.c_dnode = rmt_ptr->p_usr.p_nodeid;
	rmt_ptr->p_rmtcmd.c_len   = 0;
	rmt_ptr->p_rmtcmd.c_rcode = rcode;

	return(sproxy_enqueue(rmt_ptr));
}

/*--------------------------------------------------------------*/
/*			copyin_rqst_lcl2rmt			*/ 
/* The sender sends a COPYIN_RQST to the destination 		*/
/* The remote process has the vcopy fields filled		*/
/*--------------------------------------------------------------*/
long copyin_rqst_lcl2rmt(struct proc *rmt_ptr, struct proc *lcl_ptr) 
{
	proc_usr_t *p_ptr;
	cmd_t *c_ptr;

	/* set the RMTOPER bit to signal that this REMOTE descriptor 	*/
	/* is envolved into a remote operation 		*/

	set_bit(BIT_RMTOPER, &rmt_ptr->p_usr.p_rts_flags);

	p_ptr = &rmt_ptr->p_usr;
MOLDEBUG(DBGPROC,PROC_USR_FORMAT,PROC_USR_FIELDS(p_ptr));

	rmt_ptr->p_rmtcmd.c_cmd   = CMD_COPYIN_RQST;
	rmt_ptr->p_rmtcmd.c_dcid  = lcl_ptr->p_usr.p_dcid;
	rmt_ptr->p_rmtcmd.c_src   = lcl_ptr->p_usr.p_endpoint;		/* Source endpoint		*/
	rmt_ptr->p_rmtcmd.c_dst   = rmt_ptr->p_usr.p_nodeid;		/* Destination endpoint 	*/
	rmt_ptr->p_rmtcmd.c_snode = atomic_read(&local_nodeid);		/* source node			*/
	rmt_ptr->p_rmtcmd.c_dnode = rmt_ptr->p_usr.p_nodeid;		/* destination node		*/
	rmt_ptr->p_rmtcmd.c_len   = rmt_ptr->p_rmtcmd.c_u.cu_vcopy.v_bytes;
	rmt_ptr->p_rmtcmd.c_rcode = OK;
	
	c_ptr = &rmt_ptr->p_rmtcmd;	
MOLDEBUG(DBGVCOPY, VCOPY_FORMAT,VCOPY_FIELDS(c_ptr) );

	return(sproxy_enqueue(rmt_ptr));
}

/*--------------------------------------------------------------*/
/*			copyout_data_lcl2rmt			*/
/*--------------------------------------------------------------*/
long copyout_data_lcl2rmt(struct proc *rmt_ptr, struct proc *lcl_ptr, int rcode)
{
	/* set the RMTOPER bit to signal that this REMOTE descriptor 	*/
	/* is envolved into a remote operation 			*/
	proc_usr_t *p_ptr;
	cmd_t *c_ptr;

	p_ptr = &rmt_ptr->p_usr;
MOLDEBUG(DBGPROC,PROC_USR_FORMAT,PROC_USR_FIELDS(p_ptr));

	set_bit(BIT_RMTOPER, &rmt_ptr->p_usr.p_rts_flags);

	rmt_ptr->p_rmtcmd.c_cmd   = CMD_COPYOUT_DATA;
	rmt_ptr->p_rmtcmd.c_dcid  = rmt_ptr->p_usr.p_dcid;
	rmt_ptr->p_rmtcmd.c_src   = lcl_ptr->p_usr.p_endpoint;		/* Source endpoint		*/
	rmt_ptr->p_rmtcmd.c_dst   = rmt_ptr->p_usr.p_endpoint;		/* Destination endpoint 	*/
	rmt_ptr->p_rmtcmd.c_snode = atomic_read(&local_nodeid);		/* source node			*/
	rmt_ptr->p_rmtcmd.c_dnode = rmt_ptr->p_usr.p_nodeid;		/* destination node		*/
	rmt_ptr->p_rmtcmd.c_len   = rmt_ptr->p_rmtcmd.c_u.cu_vcopy.v_bytes;
	rmt_ptr->p_rmtcmd.c_rcode = rcode;

	c_ptr = &rmt_ptr->p_rmtcmd;	
MOLDEBUG(DBGVCOPY, VCOPY_FORMAT,VCOPY_FIELDS(c_ptr) );
MOLDEBUG(INTERNAL,"rcode=%d\n", rcode);

	return(sproxy_enqueue(rmt_ptr));
}


/****************************************************************/
/****************************************************************/
/*	ACKNOWLEDGES FROM REMOTE NODE TO LOCAL NODE		*/
/****************************************************************/
/****************************************************************/

/*--------------------------------------------------------------*/
/*			send_ack_rmt2lcl			*/
/* proxy sends a SEND ACK to a local process		 	*/
/*--------------------------------------------------------------*/
asmlinkage long send_ack_rmt2lcl(struct proc *rmt_ptr, struct proc *lcl_ptr, int rcode)
{
	dc_desc_t *dc_ptr;

	dc_ptr 	= &dc[lcl_ptr->p_usr.p_dcid];

MOLDEBUG(DBGPARAMS,"dcid=%d src_ep=%d dst_ep=%d rcode=%d\n",dc_ptr->dc_usr.dc_dcid, rmt_ptr->p_usr.p_endpoint, lcl_ptr->p_usr.p_endpoint, rcode);

	/* checks if the remote source does not have pending operations */
	if( test_bit(BIT_SENDING, &rmt_ptr->p_usr.p_rts_flags))	ERROR_RETURN(EMOLPROCSTS);

	/* verify if the local destination is waiting in SENDING state */
	if(!test_bit(BIT_SENDING, &lcl_ptr->p_usr.p_rts_flags))  	ERROR_RETURN(EMOLACKDST);
	
	/* verify if the local ack destination is waiting the ack from the remote source */
	if(lcl_ptr->p_usr.p_sendto != rmt_ptr->p_usr.p_endpoint) 	ERROR_RETURN(EMOLACKSRC);

	clear_bit(BIT_SENDING, &lcl_ptr->p_usr.p_rts_flags);
	lcl_ptr->p_usr.p_sendto = NONE;

	/* Wakes up the local ack destinantion */
	if( lcl_ptr->p_usr.p_rts_flags == 0)
		READY_UP_RCODE(lcl_ptr, CMD_SEND_ACK, rcode);

	return(OK);
}


/*--------------------------------------------------------------*/
/*			generic_ack_rmt2lcl			*/
/* proxy sends an COPY ACK to a local process 			*/
/*--------------------------------------------------------------*/
long generic_ack_rmt2lcl(int ack, struct proc *rmt_ptr, struct proc *lcl_ptr, int rcode)
{
	dc_desc_t *dc_ptr;
	int ret;

	dc_ptr 	= &dc[lcl_ptr->p_usr.p_dcid];

MOLDEBUG(DBGPARAMS,"dcid=%d src_ep=%d dst_ep=%d ack=%d rcode=%d\n",
		dc_ptr->dc_usr.dc_dcid, rmt_ptr->p_usr.p_endpoint, lcl_ptr->p_usr.p_endpoint,ack, rcode);
	
	do {
		ret = OK;
		if( IT_IS_LOCAL(rmt_ptr)) 			{ret= EMOLLCLPROC; break;}

		if( lcl_ptr->p_usr.p_rts_flags == PROC_RUNNING) {ret= EMOLPROCRUN; break;}

		/*check that all envolved processes are on ONCOPY state */
		if(!test_bit(BIT_ONCOPY, &lcl_ptr->p_usr.p_rts_flags)){ret= EMOLPROCSTS; break;}
	    if(!test_bit(BIT_ONCOPY, &rmt_ptr->p_usr.p_rts_flags))  {ret= EMOLPROCSTS; break;}
	}while(0);
	if(ret == OK) {
		/* wake up the requester */
		if( lcl_ptr->p_usr.p_rts_flags == ONCOPY)
			READY_UP_RCODE(lcl_ptr, ack, rcode);
	}

	return(ret);
}


