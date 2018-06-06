/********************************************************/
/* 		TCP PROXIES USING THREADS				*/
/********************************************************/

#include "proxy.h"
#include "debug.h"
#include "macros.h"


#define RMT_CLIENT_BIND	1

#define HEADER_SIZE sizeof(proxy_hdr_t)
#define BASE_PORT      3000

#define RETRY_US		2000 /* Microseconds */
#define BIND_RETRIES	3
#define WAIT4SPROXY		3 	/* Seconds */
            
int local_nodeid;
dvs_usr_t dvs;   
struct hostent *rmthost;
proxies_usr_t px, *px_ptr;

struct sockaddr_in rmtclient_addr, rmtserver_addr;
int    rlisten_sd, rconn_sd;
int    sproxy_sd;

struct thread_desc_s {
    pthread_t 		td_thread;
	proxy_hdr_t 	*td_header;
	proxy_hdr_t 	*td_pseudo;
	proxy_payload_t *td_payload;
	int 			td_msg_ok;
	int 			td_msg_fail;	
	pthread_mutex_t td_mtx;    /* mutex & condition to allow main thread to
								wait for the new thread to  set its TID */
	pthread_cond_t  td_cond;   /* '' */
	pid_t           td_tid;     /* to hold new thread's TID */
};
typedef struct thread_desc_s thread_desc_t;

thread_desc_t rdesc;
thread_desc_t sdesc;

/*----------------------------------------------*/
/*      PROXY RECEIVER FUNCTIONS               */
/*----------------------------------------------*/
                
/* pr_setup_connection: bind and setup a listening socket 
   This socket is not accepting connections yet after
   end of this call.
 */                
int pr_setup_connection(void) 
{
    int ret;
    short int port_no;
    struct sockaddr_in servaddr;
    int optval = 1;

    port_no = (BASE_PORT+px.px_id);
	SVRDEBUG("RPROXY: for node %s running at port=%d\n", px.px_name, port_no);

    // Create server socket.
    if ( (rlisten_sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
        ERROR_EXIT(errno);

    if( (ret = setsockopt(rlisten_sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))) < 0)
        	ERROR_EXIT(errno);

    // Bind (attach) this process to the server socket.
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port_no);
	ret = bind(rlisten_sd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    if(ret < 0) ERROR_EXIT(errno);

	SVRDEBUG("RPROXY: is bound to port=%d socket=%d\n", port_no, rlisten_sd);

	// Turn 'rproxy_sd' to a listening socket. Listen queue size is 1.
	ret = listen(rlisten_sd, 0);
    if(ret < 0) ERROR_EXIT(errno);

    return(OK);
   
}

/* pr_receive_payloadr: receives the header from remote sender */
int pr_receive_payload(int payload_size) 
{
    int n, len, total,  received = 0;
    char *p_ptr;

   	SVRDEBUG("payload_size=%d\n",payload_size);
   	p_ptr = (char*) rdesc.td_payload;
   	len = sizeof(struct sockaddr_in);
   	while ((n = recv(rconn_sd, p_ptr, (payload_size-received), 0 )) > 0) {
        received = received + n;
		SVRDEBUG("RPROXY: n:%d | received:%d\n", n,received);
        if (received >= payload_size) return(OK);
       	p_ptr += n;
		len = sizeof(struct sockaddr_in);
   	}
    
    if(n < 0) ERROR_RETURN(errno);
 	return(OK);
}

/* pr_receive_header: receives the header from remote sender */
int pr_receive_header(void) 
{
    int n, len, total,  received = 0;
    char *p_ptr;

   	SVRDEBUG("socket=%d\n", rconn_sd);
   	p_ptr = (char*) rdesc.td_header;
	total = sizeof(proxy_hdr_t);
   	len = sizeof(struct sockaddr_in);
   	while ((n = recv(rconn_sd, p_ptr, (total-received), 0 )) > 0) {
        received = received + n;
		SVRDEBUG("RPROXY: n:%d | received:%d | HEADER_SIZE:%d\n", n,received,sizeof(proxy_hdr_t));
        if (received >= sizeof(proxy_hdr_t)) {  
			SVRDEBUG("RPROXY: " CMD_FORMAT,CMD_FIELDS(rdesc.td_header));
        	return(OK);
        } else {
			SVRDEBUG("RPROXY: Header partially received. There are %d bytes still to get\n", 
                  	sizeof(proxy_hdr_t) - received);
        	p_ptr += n;
        }
		len = sizeof(struct sockaddr_in);
   	}
    
    ERROR_RETURN(errno);
}

/* pr_process_message: receives header and payload if any. Then deliver the 
 * message to local */
int pr_process_message(void) {
    int rcode, payload_size, ret, retry;
	proc_usr_t kproc, *kp_ptr;
	slot_t slot, *s_ptr;
 	message *m_ptr;
	cmd_t *c_ptr;
	static struct timespec time_to_wait = {0, 0};	

    do {
		bzero(rdesc.td_header, sizeof(proxy_hdr_t));
		SVRDEBUG("RPROXY: About to receive header\n");
    	rcode = pr_receive_header();
    	if (rcode != 0) ERROR_RETURN(rcode);
		SVRDEBUG("RPROXY:" CMD_FORMAT,CMD_FIELDS(rdesc.td_header));
   	}while(rdesc.td_header->c_cmd == CMD_NONE);
    
	/* now we have a proxy header in the buffer. Cast it.*/
    payload_size = rdesc.td_header->c_len;
    
    /* payload could be zero */
    if(payload_size != 0) {
        bzero(rdesc.td_payload, payload_size);
        rcode = pr_receive_payload(payload_size);
        if (rcode != 0){
			SVRDEBUG("RPROXY: No payload to receive.\n");
		}
    }else{
		switch(rdesc.td_header->c_cmd){
			case CMD_SEND_MSG:
			case CMD_SNDREC_MSG:
			case CMD_REPLY_MSG:
				m_ptr = &rdesc.td_header->c_u.cu_msg;
				SVRDEBUG("RPROXY: " MSG1_FORMAT,  MSG1_FIELDS(m_ptr));
				break;
			case CMD_COPYIN_DATA:
			case CMD_COPYOUT_DATA:
				SVRDEBUG("RPROXY: "VCOPY_FORMAT, VCOPY_FIELDS(rdesc.td_header)); 
				break;
			default:
				break;
		}
	}
	
	SVRDEBUG("RPROXY: put2lcl\n");
	rcode = mnx_put2lcl(rdesc.td_header, rdesc.td_payload);
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
	
	/*Build a pseudo header  RMT_BIND request */
	rdesc.td_pseudo->c_cmd 		= CMD_SNDREC_MSG;
	rdesc.td_pseudo->c_dcid		= rdesc.td_header->c_dcid;
	rdesc.td_pseudo->c_src		= PM_PROC_NR;
	rdesc.td_pseudo->c_dst 		= SYSTASK(local_nodeid);
	rdesc.td_pseudo->c_snode 	= rdesc.td_header->c_snode;
	rdesc.td_pseudo->c_dnode 	= local_nodeid;
	rdesc.td_pseudo->c_rcode	= 0;
  	rdesc.td_pseudo->c_len		= 0;
  	rdesc.td_pseudo->c_flags	= 0;
  	rdesc.td_pseudo->c_snd_seq 	= 0;
  	rdesc.td_pseudo->c_ack_seq 	= 0;
	rdesc.td_pseudo->c_timestamp=  rdesc.td_header->c_timestamp;
		rdesc.td_pseudo->c_u.cu_msg.m_source 	= PM_PROC_NR;
		rdesc.td_pseudo->c_u.cu_msg.m_type 		= SYS_BINDPROC;
		rdesc.td_pseudo->c_u.cu_msg.M3_ENDPT 	= rdesc.td_header->c_src;
		rdesc.td_pseudo->c_u.cu_msg.M3_NODEID 	= rdesc.td_header->c_snode;
		rdesc.td_pseudo->c_u.cu_msg.M3_OPER 	= RMT_BIND;
		sprintf(rdesc.td_pseudo->c_u.cu_msg.m3_ca1,"RClient%d", rdesc.td_header->c_snode);
	
	c_ptr = rdesc.td_pseudo;
	SVRDEBUG("RPROXY: " CMD_FORMAT,  CMD_FIELDS(c_ptr));
	
	m_ptr = &rdesc.td_pseudo->c_u.cu_msg;
	SVRDEBUG("RPROXY: " MSG3_FORMAT,  MSG3_FIELDS(m_ptr));
				
	/* send PSEUDO message to local SYSTASK */
	SVRDEBUG("RPROXY: mnx_put2lcl RMT_BIND to SYSTASK\n");
	ret = mnx_put2lcl(rdesc.td_pseudo, rdesc.td_payload);
	if( ret) {
		ERROR_PRINT(ret);
		ERROR_RETURN(rcode);
	}
		
	/* Wait until SYSTASK replies through SPROXY */
	SVRDEBUG("RPROXY: Wait until SYSTASK replies through SPROXY\n");
    time_to_wait.tv_sec = time(NULL) + WAIT4SPROXY;
	pthread_mutex_lock(&rdesc.td_mtx);
    ret = pthread_cond_timedwait(&rdesc.td_cond, &rdesc.td_mtx, &time_to_wait);
    pthread_mutex_unlock(&rdesc.td_mtx);
	if( ret) {
		ERROR_PRINT(ret);
		ERROR_RETURN(rcode);
	}
	
	/*Build a pseudo header CMD_SEND_ACK to SYSTASK */
	rdesc.td_pseudo->c_cmd 		= CMD_SEND_ACK;
	rdesc.td_pseudo->c_dcid		= rdesc.td_header->c_dcid;
	rdesc.td_pseudo->c_src		= PM_PROC_NR;
	rdesc.td_pseudo->c_dst 		= SYSTASK(local_nodeid);
	rdesc.td_pseudo->c_snode 	= rdesc.td_header->c_snode;
	rdesc.td_pseudo->c_dnode 	= local_nodeid;
	rdesc.td_pseudo->c_rcode	= 0;
  	rdesc.td_pseudo->c_len		= 0;
  	rdesc.td_pseudo->c_flags	= 0;
  	rdesc.td_pseudo->c_snd_seq 	= 0;
  	rdesc.td_pseudo->c_ack_seq 	= 0;
	c_ptr = rdesc.td_pseudo;
	SVRDEBUG("RPROXY: " CMD_FORMAT,  CMD_FIELDS(c_ptr));
	
	/* send PSEUDO CMD_SEND_ACK to local SYSTASK */
	SVRDEBUG("RPROXY: mnx_put2lcl CMD_SEND_ACK to SYSTASK\n");
	ret = mnx_put2lcl(rdesc.td_pseudo, rdesc.td_payload);
	if( ret) {
		ERROR_PRINT(ret);
		ERROR_RETURN(rcode);
	}
	
	/* PUT2LCL retry after REMOTE CLIENT BINDING */
	SVRDEBUG("RPROXY: PUT2LCL retry after REMOTE CLIENT BINDING\n");
	c_ptr = rdesc.td_header;
	SVRDEBUG("RPROXY: " CMD_FORMAT,  CMD_FIELDS(c_ptr));
	m_ptr = &rdesc.td_header->c_u.cu_msg;
	SVRDEBUG("RPROXY: " MSG1_FORMAT,  MSG1_FIELDS(m_ptr));
	ret = mnx_put2lcl(rdesc.td_header, rdesc.td_payload);
	if( ret) {
		ERROR_PRINT(ret);
		ERROR_RETURN(rcode);
	}
	rcode = OK;
#else /* RMT_CLIENT_BIND	*/
    if( rcode) ERROR_RETURN(rcode);	
#endif /* RMT_CLIENT_BIND	*/
	
    return(rcode);    
}

/* pr_start_serving: accept connection from remote sender
   and loop receiving and processing messages
 */
void pr_start_serving(void)
{
	int sender_addrlen;
    char ip4[INET_ADDRSTRLEN];  // space to hold the IPv4 string
    struct sockaddr_in sa;
    int rcode;

    while (1){
		do {
			sender_addrlen = sizeof(sa);
			SVRDEBUG("RPROXY: Waiting for connection.\n");
    		rconn_sd = accept(rlisten_sd, (struct sockaddr *) &sa, &sender_addrlen);
    		if(rconn_sd < 0) SYSERR(errno);
		}while(rconn_sd < 0);

		SVRDEBUG("RPROXY: Remote sender [%s] connected on sd [%d]. Getting remote command.\n",
           		inet_ntop(AF_INET, &(sa.sin_addr), ip4, INET_ADDRSTRLEN),
           		rconn_sd);
		rcode = mnx_proxy_conn(px.px_id, CONNECT_RPROXY);

    	/* Serve Forever */
		do { 
	       	/* get a complete message and process it */
       		rcode = pr_process_message();
       		if (rcode == OK) {
				SVRDEBUG("RPROXY: Message succesfully processed.\n");
				rdesc.td_msg_ok++;
        	} else {
				SVRDEBUG("RPROXY: Message processing failure [%d]\n",rcode);
            			rdesc.td_msg_fail++;
				if( rcode == EMOLNOTCONN) break;
			}	
		}while(1);

		rcode = mnx_proxy_conn(px.px_id, DISCONNECT_RPROXY);
		close(rconn_sd);
   	}
    
    /* never reached */
}

/* pr_thread: creates socket */
 void *pr_thread(void *arg) 
{
    int receiver_sd, rcode;

	SVRDEBUG("RPROXY: Initializing...\n");
    
	/* Lock mutex... */
	pthread_mutex_lock(&rdesc.td_mtx);
	/* Get and save TID and ready flag.. */
	rdesc.td_tid = syscall(SYS_gettid);
	/* and signal main thread that we're ready */
	pthread_cond_signal(&rdesc.td_cond);
	/* ..then unlock when we're done. */
	pthread_mutex_unlock(&rdesc.td_mtx);
	
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
	
	SVRDEBUG("RPROXY: rcode=%d\n", rcode);
//	sleep(60);

	posix_memalign( (void**) &rdesc.td_header, getpagesize(), (sizeof(proxy_hdr_t)));
	if (rdesc.td_header== NULL) {
    		perror(" rdesc.td_header posix_memalign");
    		exit(1);
  	}

	posix_memalign( (void**) &rdesc.td_payload, getpagesize(), (sizeof(proxy_payload_t)));
	if (rdesc.td_payload== NULL) {
    		perror(" rdesc.td_payload posix_memalign");
    		exit(1);
  	}

	posix_memalign( (void**) &rdesc.td_pseudo, getpagesize(), (sizeof(proxy_hdr_t)));
	if (rdesc.td_pseudo== NULL) {
    		perror(" rdesc.td_pseudo posix_memalign");
    		exit(1);
  	}
	
	SVRDEBUG("rdesc.td_header=%p rdesc.td_payload=%p diff=%d\n", 
			rdesc.td_header, rdesc.td_payload, ((char*) rdesc.td_payload - (char*) rdesc.td_header));

    if( pr_setup_connection() == OK) {
      	pr_start_serving();
 	} else {
       	ERROR_EXIT(errno);
    }
}

/*----------------------------------------------*/
/*      PROXY SENDER FUNCTIONS                  */
/*----------------------------------------------*/

/* ps_send_header: send  header to remote receiver */
int  ps_send_header(void ) 
{
    int sent = 0;        // how many bytes we've sent
    int bytesleft;
    int n, total;
	char *p_ptr;

	bytesleft = sizeof(proxy_hdr_t); // how many bytes we have left to send
	total = bytesleft;
	SVRDEBUG("SPROXY: send header bytesleft=%d\n", bytesleft);

    p_ptr = (char *) sdesc.td_header;
    while(sent < total) {
        n = send(sproxy_sd, p_ptr, bytesleft, 0);
        if (n < 0) {
			if(errno == EALREADY) {
				SYSERR(errno);
				sleep(1);
				continue;
			}else{
				ERROR_RETURN(errno);
			}
		}
        sent += n;
		p_ptr += n; 
        bytesleft -= n;
    }
	SVRDEBUG("SPROXY: socket=%d sent header=%d \n", sproxy_sd, total);
    return(OK);
}

/* ps_send_payload: send payload to remote receiver */
int  ps_send_payload(void ) 
{
    int sent = 0;        // how many bytes we've sent
    int bytesleft;
    int n, total;
	char *p_ptr;

	if( sdesc.td_header->c_len <= 0) ERROR_EXIT(EMOLINVAL);
	
	bytesleft =  sdesc.td_header->c_len; // how many bytes we have left to send
	total = bytesleft;
	SVRDEBUG("SPROXY: send header=%d \n", bytesleft);

    p_ptr = (char *) sdesc.td_payload;
    while(sent < total) {
        n = send(sproxy_sd, p_ptr, bytesleft, 0);
        if (n < 0) {
			if(errno == EALREADY) {
				SYSERR(errno);
				sleep(1);
				continue;
			}else{
				ERROR_RETURN(errno);
			}
		}
        sent += n;
		p_ptr += n; 
        bytesleft -= n;
    }
	SVRDEBUG("SPROXY: socket=%d sent payload=%d \n", sproxy_sd, total);
    return(OK);
}

/* 
 * ps_send_remote: send a message (header + payload if existing) 
 * to remote receiver
 */
int  ps_send_remote(void) 
{
	int rcode;

	SVRDEBUG("SPROXY:" CMD_FORMAT,CMD_FIELDS(sdesc.td_header));

	/* send the header */
	rcode =  ps_send_header();
	if ( rcode != OK)  ERROR_RETURN(rcode);

	if( sdesc.td_header->c_len > 0) {
		SVRDEBUG("SPROXY: send payload len=%d\n", sdesc.td_header->c_len );
		rcode =  ps_send_payload();
		if ( rcode != OK)  ERROR_RETURN(rcode);
	}
	
    return(OK);
}

/* 
 * ps_start_serving: gets local message and sends it to remote receiver .
 * Do this forever.
 */
int  ps_start_serving(void)
{
    int rcode, i, sm_flag;
    message *m_ptr;
    int pid, ret;
   	char *ptr; 

    pid = getpid();
	
    while(1) {
		SVRDEBUG("SPROXY %d: Waiting a message\n", pid);
        
		ret = mnx_get2rmt(sdesc.td_header, sdesc.td_payload);       
      
		if( ret != OK) {
			switch(ret) {
				case EMOLTIMEDOUT:
					SVRDEBUG("SPROXY: Sending HELLO \n");
					sdesc.td_header->c_cmd = CMD_NONE;
					sdesc.td_header->c_len = 0;
					sdesc.td_header->c_rcode = 0;
					break;
				case EMOLNOTCONN:
					return(EMOLNOTCONN);
				default:
					SVRDEBUG("ERROR  mnx_get2rmt %d\n", ret);
					continue;
			}
		}

		SVRDEBUG("SPROXY: %d "HDR_FORMAT,pid, HDR_FIELDS(sdesc.td_header)); 
		sm_flag = TRUE;
		switch(sdesc.td_header->c_cmd){
			case CMD_SEND_MSG: /* reply for REMOTE CLIENT BINDING*/	
				ret = 0;
				pthread_mutex_lock(&rdesc.td_mtx);
				do {
					if(sdesc.td_header->c_dcid	!= rdesc.td_header->c_dcid) {ret=1;break;}	
					if(sdesc.td_header->c_src	!= SYSTASK(local_nodeid)) {ret=2;break;}	
					if(sdesc.td_header->c_dst	!= PM_PROC_NR ) {ret=3;break;}	
					if(sdesc.td_header->c_snode	!= local_nodeid ) {ret=4;break;}	
					if(sdesc.td_header->c_dnode	!= rdesc.td_header->c_snode ) {ret=5;break;}
					m_ptr = &sdesc.td_header->c_u.cu_msg;
					SVRDEBUG("SPROXY: " MSG1_FORMAT,  MSG1_FIELDS(m_ptr))
					if(sdesc.td_header->c_u.cu_msg.m_source != SYSTASK(local_nodeid)) {ret=6;break;}	
					if((int) sdesc.td_header->c_u.cu_msg.M1_OPER != RMT_BIND) {ret=7;break;}	
				}while(0);
				if( ret == 0 ){
					SVRDEBUG("SPROXY: reply for REMOTE CLIENT BINDING - Discard it ret=%d\n",ret);
					pthread_cond_signal(&rdesc.td_cond);
					sm_flag = FALSE;
				}else{
					SVRDEBUG("SPROXY: CMD_SEND_MSG dont match REMOTE CLIENT BINDING ret=%d\n",ret);					
				}
				pthread_mutex_unlock(&rdesc.td_mtx);					
				break;	
			case CMD_SNDREC_ACK: 		
			case CMD_SNDREC_MSG:
			case CMD_REPLY_MSG:
				m_ptr = &sdesc.td_header->c_u.cu_msg;
				SVRDEBUG("SPROXY: " MSG1_FORMAT,  MSG1_FIELDS(m_ptr));
				break;
			case CMD_COPYIN_DATA:
			case CMD_COPYOUT_DATA:
				SVRDEBUG("SPROXY: "VCOPY_FORMAT, VCOPY_FIELDS(sdesc.td_header)); 
				break;
			default:
				break;
		}

		/* MATCH for REMOTE CLIENT BINDING - Dont send the reply */
		if(sm_flag == FALSE){
			rcode = OK;
		}else {
			/* send the message to remote */
			rcode =  ps_send_remote();
			if (rcode == 0) {
				sdesc.td_msg_ok++;
			} else {
				sdesc.td_msg_fail++;
			}	
		}
    }

    /* never reached */
    exit(1);
}

/* ps_connect_to_remote: connects to the remote receiver */
int ps_connect_to_remote(void) 
{
    int port_no, rcode, i;
    char rmt_ipaddr[INET_ADDRSTRLEN+1];

    port_no = (BASE_PORT+local_nodeid);
	SVRDEBUG("SPROXY: for node %s running at port=%d\n", px.px_name, port_no);    

	// Connect to the server client	
    rmtserver_addr.sin_family = AF_INET;  
    rmtserver_addr.sin_port = htons(port_no);  

    rmthost = gethostbyname(px.px_name);
	if( rmthost == NULL) ERROR_EXIT(h_errno);
	for( i =0; rmthost->h_addr_list[i] != NULL; i++) {
		SVRDEBUG("SPROXY: remote host address %i: %s\n", 
			i, inet_ntoa( *( struct in_addr*)(rmthost->h_addr_list[i])));
	}

    if((inet_pton(AF_INET,inet_ntoa( *( struct in_addr*)(rmthost->h_addr_list[0])), (struct sockaddr*) &rmtserver_addr.sin_addr)) <= 0)
    	ERROR_RETURN(errno);

    inet_ntop(AF_INET, (struct sockaddr*) &rmtserver_addr.sin_addr, rmt_ipaddr, INET_ADDRSTRLEN);
	SVRDEBUG("SPROXY: for node %s running at  IP=%s\n", px.px_name, rmt_ipaddr);    

	rcode = connect(sproxy_sd, (struct sockaddr *) &rmtserver_addr, sizeof(rmtserver_addr));
    if (rcode != 0) ERROR_RETURN(errno);
    return(OK);
}

/* 
 * ps_thread: creates sender socket, the connect to remote and
 * start sending messages to remote 
 */
 void *ps_thread(void *arg) 
{
    int rcode = 0;
	char *p_buffer;

	SVRDEBUG("SPROXY: Initializing...\n");
    
	/* Lock mutex... */
	pthread_mutex_lock(&sdesc.td_mtx);
	/* Get and save TID and ready flag.. */
	sdesc.td_tid = syscall(SYS_gettid);
	/* and signal main thread that we're ready */
	pthread_cond_signal(&sdesc.td_cond);
	/* ..then unlock when we're done. */
	pthread_mutex_unlock(&sdesc.td_mtx);
	
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
		
	SVRDEBUG("SPROXY: rcode=%d\n", rcode);
//	sleep(60);
	
	posix_memalign( (void**) &sdesc.td_header, getpagesize(), (sizeof(proxy_hdr_t)));
	if (sdesc.td_header== NULL) {
    		perror("sdesc.td_header posix_memalign");
    		exit(1);
  	}

	posix_memalign( (void**) &sdesc.td_payload, getpagesize(), (sizeof(proxy_payload_t)));
	if (sdesc.td_payload== NULL) {
    		perror("sdesc.td_payload posix_memalign");
    		exit(1);
  	}
	
    // Create server socket.
    if ( (sproxy_sd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
       	ERROR_EXIT(errno)
    }
	
	/* try to connect many times */
	while(1) {
		do {
			if (rcode < 0)
				SVRDEBUG("SPROXY: Could not connect to %d"
                    			" Sleeping for a while...\n", px.px_id);
			sleep(4);
			rcode = ps_connect_to_remote();
		} while (rcode != 0);
		
		rcode = mnx_proxy_conn(px.px_id, CONNECT_SPROXY);
		if(rcode){
			ERROR_EXIT(rcode);
		} 

		ps_start_serving();
	
		rcode = mnx_proxy_conn(px.px_id, DISCONNECT_SPROXY);
		if(rcode){
			ERROR_EXIT(rcode);
		} 	
		close(sproxy_sd);
	}


    /* code never reaches here */
}

extern int errno;

/*----------------------------------------------*/
/*		MAIN: 			*/
/*----------------------------------------------*/
void  main ( int argc, char *argv[] )
{
    int spid, rpid, pid, status;
    int ret;
    dvs_usr_t *d_ptr;    

    if (argc != 3) {
     	fprintf(stderr,"Usage: %s <px_name> <px_id> \n", argv[0]);
    	exit(0);
    }

    strncpy(px.px_name,argv[1], MAXPROXYNAME);
    printf("TCP Proxy Pair name: %s\n",px.px_name);
 
    px.px_id = atoi(argv[2]);
    printf("Proxy Pair id: %d\n",px.px_id);

    local_nodeid = mnx_getdvsinfo(&dvs);
    d_ptr=&dvs;
	SVRDEBUG(dvs_USR_FORMAT,dvs_USR_FIELDS(d_ptr));

    pid = getpid();
	SVRDEBUG("MAIN: pid=%d local_nodeid=%d\n", pid, local_nodeid);

    /* creates SENDER and RECEIVER Proxies as Treads */
	SVRDEBUG("MAIN: pthread_create RPROXY\n");
	pthread_cond_init(&rdesc.td_cond, NULL);  /* init condition */
	pthread_mutex_init(&rdesc.td_mtx, NULL);  /* init mutex */
	pthread_mutex_lock(&rdesc.td_mtx);
	if (ret = pthread_create(&rdesc.td_thread, NULL, pr_thread,(void*)NULL )) {
		ERROR_EXIT(ret);
	}
    pthread_cond_wait(&rdesc.td_cond, &rdesc.td_mtx);
	SVRDEBUG("MAIN: RPROXY td_tid=%d\n", rdesc.td_tid);
    pthread_mutex_unlock(&rdesc.td_mtx);

	SVRDEBUG("MAIN: pthread_create SPROXY\n");
	pthread_cond_init(&sdesc.td_cond, NULL);  /* init condition */
	pthread_mutex_init(&sdesc.td_mtx, NULL);  /* init mutex */
	pthread_mutex_lock(&sdesc.td_mtx);
	if (ret = pthread_create(&sdesc.td_thread, NULL, ps_thread,(void*)NULL )) {
		ERROR_EXIT(ret);
	}
    pthread_cond_wait(&sdesc.td_cond, &sdesc.td_mtx);
	SVRDEBUG("MAIN: SPROXY td_tid=%d\n", sdesc.td_tid);
    pthread_mutex_unlock(&sdesc.td_mtx);
	
    /* register the proxies */
    ret = mnx_proxies_bind(px.px_name, px.px_id, sdesc.td_tid, rdesc.td_tid, MAXCOPYBUF);
    if( ret < 0) ERROR_EXIT(ret);
	
	px_ptr = &px;
	SVRDEBUG(PX_USR_FORMAT , PX_USR_FIELDS(px_ptr));
	SVRDEBUG("binded to (%d,%d)\n", sdesc.td_tid, rdesc.td_tid);
//	sleep(60);

	ret= mnx_node_up(px.px_name, px.px_id, px.px_id);	
	
	pthread_join ( rdesc.td_thread, NULL );
	pthread_join ( sdesc.td_thread, NULL );
	pthread_mutex_destroy(&rdesc.td_mtx);
	pthread_cond_destroy(&rdesc.td_cond);
	pthread_mutex_destroy(&sdesc.td_mtx);
	pthread_cond_destroy(&sdesc.td_cond);
    exit(0);
}

