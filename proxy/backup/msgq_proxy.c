/********************************************************/
/* 		MSGQ PROXIES				*/
/********************************************************/

#include "proxy.h"
#include "raw.h"
#include "debug.h"
#include "macros.h"

#define HEADER_SIZE sizeof(proxy_hdr_t)
#define BASE_PORT      3000

#define RETRY_US		2000 /* Microseconds */
#define BIND_RETRIES	3
            
int local_nodeid;

int rmsg_ok = 0;
int rmsg_fail = 0;
int smsg_ok = 0;
int smsg_fail = 0; 
DVS_usr_t DVS;   
proxies_usr_t px, *px_ptr;
proxy_hdr_t 	*p_pseudo;

int raw_mq_in, raw_mq_out;
msgq_buf_t *out_msg_ptr, *in_msg_ptr;
struct msqid_ds mq_in_ds;
struct msqid_ds mq_out_ds;

/*----------------------------------------------*/
/*      PROXY RECEIVER FUNCTIONS               */
/*----------------------------------------------*/
                
/* pr_get_message: receives header and payload if any. Then deliver the 
 * message to local */
int pr_get_message(void) {
    int rcode, payload_size, ret, retry, bytes, cmd;
	proc_usr_t kproc, *kp_ptr;
	slot_t slot, *s_ptr;
 	message *m_ptr;
	proxy_hdr_t 	*p_header;
	proxy_payload_t *p_payload;

	p_header = &in_msg_ptr->m3.cmd;
	p_payload= &in_msg_ptr->m3.pay;
	
	do {	
		bzero(p_header, sizeof(proxy_hdr_t));
		SVRDEBUG("Receiving reply from INPUT msgq\n");
		bytes = msgrcv(raw_mq_in, in_msg_ptr, sizeof(msgq_buf_t), 0 , 0 );
		if( bytes < 0) {
			SVRDEBUG("msgrcv errno=%d\n",errno);
			ERROR_RETURN(-errno);
		}
		SVRDEBUG("RPROXY:" CMD_FORMAT,CMD_FIELDS(p_header));
   	}while(p_header->c_cmd == CMD_NONE);
    
	/* now we have a proxy header in the buffer. Cast it.*/
    payload_size = p_header->c_len;
    
	cmd = (p_header->c_cmd & ~(CMD_ACKNOWLEDGE));	
	if( cmd > CMD_BATCHED_CMD || cmd < 0) {
		ERROR_EXIT(EMOLINVAL);
	}
	
	SVRDEBUG("RPROXY: put2lcl\n");
	rcode = mnx_put2lcl(p_header, p_payload);
#ifdef RMT_CLIENT_BIND	
	if( rcode == OK) return (OK);
	/******************** REMOTE CLIENT BINDING ************************************/
	/* rcode: the result of the las mnx_put2lcl 	*/
	/* ret: the result of the following operations	*/
	SVRDEBUG("RPROXY: REMOTE CLIENT BINDING rcode=%d\n", rcode);
	switch(rcode){
		case EMOLNOTBIND:	/* the slot is free */	
		case EMOLENDPOINT:	/* local node registers other endpoint using the slot */
		case EMOLNONODE:	/* local node register other node for this endpoint   */
			break;
		default:
			ERROR_RETURN(rcode);
	} 
	
	/*Build a pseudo header */
	p_pseudo->c_cmd 	= CMD_SNDREC_MSG;
	p_pseudo->c_dcid	= p_header->c_dcid;
	p_pseudo->c_src	 	= PM_PROC_NR;
	p_pseudo->c_dst 	= SYSTASK(localnodeid);
	p_pseudo->c_snode 	= p_header->c_snode;
	p_pseudo->c_dnode 	= local_nodeid;
	p_pseudo->c_rcode	= 0;
  	p_pseudo->c_len		= 0;
  	p_pseudo->c_flags	= 0;
  	p_pseudo->c_snd_seq = 0;
  	p_pseudo->c_ack_seq = 0;
	p_pseudo->c_timestamp=  p_header->c_timestamp;
		p_pseudo->c_u.cu_msg.m_source 	= PM_PROC_NR;
		p_pseudo->c_u.cu_msg.m_type 	= SYS_BINDPROC;
		p_pseudo->c_u.cu_msg.M3_ENDPT 	= p_header->c_src;
		p_pseudo->c_u.cu_msg.M3_NODEID 	= p_header->c_snode;
		p_pseudo->c_u.cu_msg.M3_OPER 	= RMT_BIND;
		sprintf(&p_pseudo->c_u.cu_msg.m3_ca1,"RClient%d", p_header->c_snode);
		
	/* send PSEUDO message to local SYSTASK */	
	ret = mnx_put2lcl(p_header, p_payload);
	if( ret) {
		ERROR_PRINT(ret);
		ERROR_RETURN(rcode);
	}
	
	/* PUT2LCL retry after REMOTE CLIENT BINDING */
	if( rcode ){
		ret = mnx_put2lcl(p_header, p_payload);
		if( ret) {
			ERROR_PRINT(ret);
			ERROR_RETURN(rcode);
		}
	}
#else /* RMT_CLIENT_BIND	*/
    if( rcode) ERROR_RETURN(rcode);	
#endif /* RMT_CLIENT_BIND	*/
	
    return(OK);    
}

/* pr_start_serving: accept connection from remote sender
   and loop receiving and processing messages
 */
void pr_start_serving(void)
{
    int rcode;

    while (1){
		SVRDEBUG("RPROXY: MSGQ px.px_id=%d getting command.\n",px.px_id);
		rcode = mnx_proxy_conn(px.px_id, CONNECT_RPROXY);

    	/* Serve Forever */
		do { 
	       	/* get a complete message and process it */
       		rcode = pr_get_message();
       		if (rcode == OK) {
				SVRDEBUG("RPROXY: Message succesfully processed.\n");
				rmsg_ok++;
        	} else {
				SVRDEBUG("RPROXY: Message processing failure [%d]\n",rcode);
            			rmsg_fail++;
				if( rcode == EMOLNOTCONN) break;
			}	
		}while(1);

		rcode = mnx_proxy_conn(px.px_id, DISCONNECT_RPROXY);
   	}
    
    /* never reached */
}

/* pr_init: creates socket */
void pr_init(void) 
{
    int receiver_sd, rcode;

	SVRDEBUG("RPROXY: Initializing proxy receiver. PID: %d\n", getpid());
    
	do { 
		rcode = mnx_wait4bind_T(RETRY_US);
		SVRDEBUG("RPROXY: mnx_wait4bind_T  rcode=%d\n", rcode);
		if (rcode == EMOLTIMEDOUT) {
			SVRDEBUG("RPROXY: mnx_wait4bind_T TIMEOUT\n");
			continue ;
		}else if(rcode == NONE) { /* proxies have not endpoint */
			break;	
		} if( rcode < 0) 
			exit(EXIT_FAILURE);
	} while	(rcode < OK);
	
	// memory for msgq INPUT message buffer   
	posix_memalign( (void **) &in_msg_ptr, getpagesize(), sizeof(msgq_buf_t) );
	if (in_msg_ptr == NULL) {
		fprintf(stderr, "posix_memalign in_msg_ptr \n");
		ERROR_EXIT(-errno);
	}

	// memory for msgq OUTPUT message buffer  
	posix_memalign( (void **) &out_msg_ptr, getpagesize(), sizeof(msgq_buf_t) );
	if (out_msg_ptr == NULL) {
		fprintf(stderr, "posix_memalign out_msg_ptr\n");
		ERROR_EXIT(-errno);
	}

	posix_memalign( (void**) &p_pseudo, getpagesize(), (sizeof(proxy_hdr_t)));
	if (p_pseudo== NULL) {
    		perror(" p_pseudo posix_memalign");
    		exit(1);
  	}

   	pr_start_serving();
}

/*----------------------------------------------*/
/*      PROXY SENDER FUNCTIONS                  */
/*----------------------------------------------*/

/* 
 * ps_send_remote: send a message (header + payload if existing) 
 * to remote receiver
 */
int  ps_send_remote(void) 
{
	int rcode, bytes, total_bytes;
	proxy_hdr_t 	*p_header;

	p_header = &out_msg_ptr->m3.cmd;
	SVRDEBUG("SPROXY:" CMD_FORMAT,CMD_FIELDS(p_header));

	// ENQUEUE into the SENDER QUEUE 
	total_bytes = ( sizeof(cmd_t)+ p_header->c_len);
	SVRDEBUG("mtype=%X total_bytes=%d\n", out_msg_ptr->mtype, total_bytes);
	
	rcode = msgsnd(raw_mq_out , out_msg_ptr, total_bytes , 0); 
	if( rcode < 0) {
		SVRDEBUG("msgsnd errno=%d\n",errno);
		ERROR_EXIT(-errno);
	}
	
    return(OK);
}

/* 
 * ps_start_serving: gets local message and sends it to remote receiver .
 * Do this forever.
 */
int  ps_start_serving(void)
{
    int rcode, i;
    message *m_ptr;
    int pid, ret;
   	char *ptr; 
	proxy_hdr_t 	*p_header;
	proxy_payload_t *p_payload;

    pid = getpid();
	p_header = &out_msg_ptr->m3.cmd;
	p_payload= &out_msg_ptr->m3.pay;
		
    while(1) {
		SVRDEBUG("SPROXY %d: Waiting a message\n", pid);
        
		rcode = mnx_get2rmt(p_header, p_payload);       
  		if( rcode != OK) {
			switch(rcode) {
				case EMOLTIMEDOUT:
					SVRDEBUG("SPROXY: Sending HELLO \n");
					p_header->c_cmd = CMD_NONE;
					p_header->c_len = 0;
					p_header->c_rcode = 0;
					break;
				case EMOLNOTCONN:
					return(EMOLNOTCONN);
				default:
					SVRDEBUG("ERROR  mnx_get2rmt %d\n", rcode);
					continue;
			}
		}

		SVRDEBUG("SPROXY: %d "HDR_FORMAT,pid, HDR_FIELDS(p_header)); 

		switch(p_header->c_cmd){
			case CMD_SEND_MSG:
			case CMD_SNDREC_MSG:
			case CMD_REPLY_MSG:
			case CMD_NTFY_MSG:
				out_msg_ptr->mtype = RAW_MQ_HDR;
				m_ptr = &p_header->c_u.cu_msg;
				SVRDEBUG("SPROXY: " MSG1_FORMAT,  MSG1_FIELDS(m_ptr));
				break;
			case CMD_COPYIN_DATA:
			case CMD_COPYOUT_DATA:
				out_msg_ptr->mtype = (RAW_MQ_HDR | RAW_MQ_PAY);
				SVRDEBUG("SPROXY: "VCOPY_FORMAT, VCOPY_FIELDS(p_header)); 
				break;
			default:
				out_msg_ptr->mtype = RAW_MQ_HDR;
				break;
		}

		if( p_header->c_cmd  &  CMD_ACKNOWLEDGE)
			out_msg_ptr->mtype |= RAW_MQ_ACK;

		/* send the message to remote */
		rcode =  ps_send_remote();
		if (rcode == 0) {
			smsg_ok++;
		} else {
			smsg_fail++;
		}	
    }

    /* never reached */
    exit(1);
}

/* ps_connect_to_remote: connects to the remote receiver */
/* 
 * ps_init: creates sender socket, the connect to remote and
 * start sending messages to remote 
 */
void  ps_init(void) 
{
    int rcode = 0;
	char *p_buffer;

	SVRDEBUG("SPROXY: Initializing on PID:%d\n", getpid());
    
	do { 
		rcode = mnx_wait4bind_T(RETRY_US);
		SVRDEBUG("SPROXY: mnx_wait4bind_T  rcode=%d\n", rcode);
		if (rcode == EMOLTIMEDOUT) {
			SVRDEBUG("SPROXY: mnx_wait4bind_T TIMEOUT\n");
			continue ;
		}else if(rcode == NONE) { /* proxies have not endpoint */
			break;	
		} if( rcode < 0) 
			exit(EXIT_FAILURE);
	} while	(rcode < OK);
		
	// memory for msgq INPUT message buffer   
	posix_memalign( (void **) &in_msg_ptr, getpagesize(), sizeof(msgq_buf_t) );
	if (in_msg_ptr == NULL) {
		fprintf(stderr, "posix_memalign in_msg_ptr \n");
		ERROR_EXIT(-errno);
	}

	// memory for msgq OUTPUT message buffer  
	posix_memalign( (void **) &out_msg_ptr, getpagesize(), sizeof(msgq_buf_t) );
	if (out_msg_ptr == NULL) {
		fprintf(stderr, "posix_memalign out_msg_ptr\n");
		ERROR_EXIT(-errno);
	}
	
	/* try to connect many times */
	while(1) {
		rcode = mnx_proxy_conn(px.px_id, CONNECT_SPROXY);
		if(rcode) ERROR_EXIT(rcode);
		
		ps_start_serving();
	
		rcode = mnx_proxy_conn(px.px_id, DISCONNECT_SPROXY);
		if(rcode)ERROR_EXIT(rcode);
			
	}
    /* code never reaches here */
}

extern int errno;

int mq_init(int px_id)
{
	/* receiving message queue */
	int qin_base, qout_base; 

	SVRDEBUG("px_id=%d\n",px_id);
		
	qin_base = QUEUEBASE + (px_id  * 2) + 0;
	raw_mq_in = msgget(qin_base, 0);
	if ( raw_mq_in < 0) {
		ERROR_RETURN(-errno);
	} 
	msgctl(raw_mq_in , IPC_STAT, &mq_in_ds);
	SVRDEBUG("qin_base=%d raw_mq_in msg_qbytes =%d\n",
		qin_base, mq_in_ds.msg_qbytes);

	qout_base = QUEUEBASE + (px_id * 2) + 1;
	raw_mq_out = msgget(qout_base, 0);
	if ( raw_mq_out < 0) {
		ERROR_RETURN(-errno);
	}

	msgctl(raw_mq_out , IPC_STAT, &mq_out_ds);
	SVRDEBUG("qout_base=%d raw_mq_out msg_qbytes =%d\n",
		qout_base, mq_out_ds.msg_qbytes);

	SVRDEBUG("raw_mq_in=%d raw_mq_out=%d\n",raw_mq_in, raw_mq_out);	

	return(OK);
}	


/*----------------------------------------------*/
/*		MAIN: 			*/
/*----------------------------------------------*/
void  main ( int argc, char *argv[] )
{
    int spid, rpid, pid, status;
    int ret;
    DVS_usr_t *d_ptr;    

    if (argc != 3) {
     	fprintf(stderr,"Usage: %s <px_name> <px_id>\n", argv[0]);
    	exit(0);
    }

    strncpy(px.px_name,argv[1], MAXPROXYNAME);
    printf("MSGQ Proxy Pair name: %s\n",px.px_name);
 
    px.px_id = atoi(argv[2]);
    printf("MSGQ Proxy Pair id: %d\n",px.px_id);
	
    local_nodeid = mnx_getDVSinfo(&DVS);
    d_ptr=&DVS;
	SVRDEBUG(DVS_USR_FORMAT,DVS_USR_FIELDS(d_ptr));

    pid = getpid();
	SVRDEBUG("MAIN: pid=%d local_nodeid=%d\n", pid, local_nodeid);

	ret= mq_init(px.px_id);
	if ( ret < 0) ERROR_EXIT(ret);
	
    /* creates SENDER and RECEIVER Proxies as children */
    if ( (spid = fork()) == 0) ps_init();
	
    if ( (rpid = fork()) == 0) pr_init();
	
    /* register the proxies */
    ret = mnx_proxies_bind(px.px_name, px.px_id, spid, rpid,MAXCOPYBUF);
    if( ret < 0) ERROR_EXIT(ret);
	
	px_ptr = &px;
	SVRDEBUG(PX_USR_FORMAT , PX_USR_FIELDS(px_ptr));
	SVRDEBUG("binded to (%d,%d)\n", spid, rpid);

	ret= mnx_node_up(px.px_name, px.px_id, px.px_id);	
	
   	wait(&status);
    wait(&status);
    exit(0);
}

