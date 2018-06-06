/********************************************************/
/* M3-IPC SERVER SIDE TCP PROXY (ONE PROCESS)		*/
/* This proxy acts as a Server for a TCP PROXY on MINIX3*/
/* It is used for server process residing on LINUX	*/
/* and Client processes residing on MINIX 3		*/
/********************************************************/

#include "proxy.h"
#include "debug.h"
#include "macros.h"

#define PX_NONE			0
#define	PX_TO_REMOTE		1
#define	PX_FROM_REMOTE		2

#define EMOLDONTACK EMOLDONTREPLY

#define HEADER_SIZE sizeof(proxy_hdr_t)
#define BASE_PORT      3000
            
int local_nodeid;

proxy_hdr_t 	*p_header;
proxy_payload_t *p_payload;

int rmsg_ok 	= 0;
int rmsg_fail 	= 0;
int smsg_ok 	= 0;
int smsg_fail 	= 0; 
dvs_usr_t dvs;   

struct hostent *rmthost;
proxies_usr_t px, *px_ptr;

struct sockaddr_in rmt_host_addr;
int    rhost_sd, listen_sd;
int pid, px_status;

extern int errno;


/*----------------------------------------------------
* svr_send_header: send  header to remote receiver 
----------------------------------------------------*/
int  svr_send_header(void ) 
{
	int sent = 0;        // how many bytes we've sent
	int bytesleft;
	int n, total;
	char *p_ptr;

	bytesleft = sizeof(proxy_hdr_t); // how many bytes we have left to send
	total = bytesleft;
	SVRDEBUG("SVR_PROXY: send header=%d \n", bytesleft);

    	p_ptr = (char *) p_header;
    	while(sent < total) {
		n = send(rhost_sd, p_ptr, bytesleft, 0);
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
	SVRDEBUG("SVR_PROXY: rhost_sd=%d sent header=%d \n", rhost_sd, total);
	return(OK);
}

/*----------------------------------------------------
/* svr_send_payload: send payload to remote receiver 
----------------------------------------------------*/
int  svr_send_payload(void ) 
{
	int sent = 0;        // how many bytes we've sent
	int bytesleft;
	int n, total;
	char *p_ptr;

	if( p_header->c_len <= 0) ERROR_EXIT(EMOLINVAL);
	
	bytesleft =  p_header->c_len; // how many bytes we have left to send
	total = bytesleft;
	SVRDEBUG("SVR_PROXY: send payload=%d \n", bytesleft);

	p_ptr = (char *) p_payload;
	while(sent < total) {
        	n = send(rhost_sd, p_ptr, bytesleft, 0);
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
	SVRDEBUG("SVR_PROXY: rhost_sd=%d sent payload=%d \n", rhost_sd, total);
    	return(OK);
}

/*---------------------------------------------------- 
* svr_send_rmt: send a reply message 
* (header + payload if needed)  to remote receiver
*----------------------------------------------------*/
int  svr_send_rmt(void) 
{
	int rcode;
	message *m_ptr;

	SVRDEBUG("SVR_PROXY: %d "HDR_FORMAT, pid, HDR_FIELDS(p_header)); 

	/* send the header */
	rcode =  svr_send_header();
	if ( rcode != OK) {
		smsg_fail++;
		ERROR_RETURN(rcode);
	}

	if( p_header->c_len > 0) {
		SVRDEBUG("SVR_PROXY: send payload len=%d\n", p_header->c_len );
		rcode =  svr_send_payload();
		if ( rcode != OK) {
			smsg_fail++;
			ERROR_RETURN(rcode);
		}
	}
	
	return(OK);
}


/*------------------------------------------------------
 px_receive_payload: receives the header from remote sender 
------------------------------------------------------*/
int px_receive_payload(int payload_size) 
{
	int n, len, total,  received = 0;
	char *p_ptr;

   	SVRDEBUG("payload_size=%d\n",payload_size);
   	p_ptr = (char*) p_payload;
   	len = sizeof(struct sockaddr_in);
   	while ((n = recv(rhost_sd, p_ptr, (payload_size-received), 0 )) > 0) {
        received = received + n;
		SVRDEBUG("SVR_PROXY: n:%d | received:%d\n", n,received);
        if (received >= payload_size) return(OK);
       	p_ptr += n;
		len = sizeof(struct sockaddr_in);
   	}
    
    if( n == 0)
		ERROR_RETURN(EMOLNOCONN);
		
 	ERROR_RETURN(-errno);
}

/*------------------------------------------------------
 px_receive_header: receives the header from remote sender 
------------------------------------------------------*/
int px_receive_header(void) 
{
	int n, len, total,  received = 0;
	char *p_ptr;

   	SVRDEBUG("socket=%d\n", rhost_sd);
   	p_ptr = (char*) p_header;
	total = sizeof(proxy_hdr_t);
   	len = sizeof(struct sockaddr_in);
   	while ((n = recv(rhost_sd, p_ptr, (total-received), 0 )) > 0) {
		received = received + n;
		SVRDEBUG("SVR_PROXY: n:%d | received:%d | HEADER_SIZE:%d\n", n,received,sizeof(proxy_hdr_t));
        	if (received >= sizeof(proxy_hdr_t)) {  
				SVRDEBUG("SVR_PROXY: " CMD_FORMAT,CMD_FIELDS(p_header));
        		return(OK);
        	} else {
				SVRDEBUG("SVR_PROXY: Header partially received. There are %d bytes still to get\n", 
                  		sizeof(proxy_hdr_t) - received);
        		p_ptr += n;
        	}
		len = sizeof(struct sockaddr_in);
   	}
	/* recv () returns 0 if the remote client has closed the connection */
    	if( n == 0)
		ERROR_RETURN(EMOLNOCONN);
		
 	ERROR_RETURN(-errno);
}


/*------------------------------------------------------------
px_from_remote: receives the header and payload if any of a 
REQUEST message. 
Then deliver the message to local 
------------------------------------------------------------*/
int px_from_remote(void) 
{
	int rcode, payload_size;

	bzero(p_header, sizeof(proxy_hdr_t));
	SVRDEBUG("SVR_PROXY: About to receive header\n");
   	rcode = px_receive_header();
   	if (rcode != 0) ERROR_RETURN(rcode);
	SVRDEBUG("SVR_PROXY:" CMD_FORMAT,CMD_FIELDS(p_header));
   
	/* now we have a proxy header in the buffer. Cast it.*/
	payload_size = p_header->c_len;
    
    	/* payload could be zero */
    if(payload_size != 0) {
    	bzero(p_payload, payload_size);
    	rcode = px_receive_payload(payload_size);
    	if (rcode != 0){
		SVRDEBUG("SVR_PROXY: No payload to receive.\n");
		}
    }
	
	return(OK);  
}

/*------------------------------------------------------------
 px_server_loop: accept connection from remote sender
 and loop receiving and processing messages
--------------------------------------------------------------*/
int px_server_loop(void)
{
	int sender_addrlen;
	char ip4[INET_ADDRSTRLEN];  // space to hold the IPv4 string
	struct sockaddr_in sa;
	int rcode, px_status;
	message *m_ptr;

	px_status = PX_NONE;

	sender_addrlen = sizeof(sa);
	SVRDEBUG("SVR_PROXY: Waiting for connection.\n");
	rhost_sd = accept(listen_sd, (struct sockaddr *) &sa, &sender_addrlen);
	if(rhost_sd < 0) { 
		SYSERR(-errno);
		return(EMOLGENERIC);
	}

	SVRDEBUG("SVR_PROXY: Remote sender [%s] connected on rhost_sd [%d]. Getting remote 	command.\n",
     		inet_ntop(AF_INET, &(sa.sin_addr), ip4, INET_ADDRSTRLEN),rhost_sd);

	rcode = mnx_proxy_conn(px.px_id, CONNECT_RPROXY);
	rcode = mnx_proxy_conn(px.px_id, CONNECT_SPROXY);

	/* STATE MACHINE */
	px_status = PX_FROM_REMOTE;
	do { 
		rcode = OK;			
		switch(px_status){
			case  PX_FROM_REMOTE:
				SVRDEBUG("SVR_PROXY: PX_FROM_REMOTE\n");
				rcode = px_from_remote();
				if (rcode == EMOLDONTACK) {
					rcode = OK;
					break;
				}
				if (rcode != OK ) {
					SVRDEBUG("SVR_PROXY: Request received failure [%d]\n",rcode);
					rmsg_fail++;
				} else {
					rmsg_ok++;
					if( p_header->c_cmd == CMD_NONE) break;
					switch(p_header->c_cmd) {
						case CMD_NTFY_MSG:
						case CMD_SNDREC_MSG:
							m_ptr = &p_header->c_u.cu_msg;
							SVRDEBUG("SVR_PROXY: %d "MSG1_FORMAT,pid, MSG1_FIELDS(m_ptr)); 
							break;
						case CMD_COPYIN_ACK:
						case CMD_COPYOUT_DATA:
							SVRDEBUG("SVR_PROXY: %d "VCOPY_FORMAT,pid, VCOPY_FIELDS(p_header)); 
							break;
						default:
							if (p_header->c_cmd & CMD_ACKNOWLEDGE){
								SVRDEBUG("SVR_PROXY: ACK received\n"); 
							}else{ 
								rcode =EINVAL;
							}	
							px_status=PX_FROM_REMOTE;
							break;
					}
					rcode = mnx_put2lcl(p_header, p_payload);
					if(rcode == 0) {
						if( (p_header->c_cmd != CMD_NTFY_MSG) 
						 && (p_header->c_cmd != CMD_SEND_ACK) )
							px_status=PX_TO_REMOTE;		
					}
				}
				break;
			case PX_TO_REMOTE:
				SVRDEBUG("SVR_PROXY: PX_TO_REMOTE\n");
				rcode = mnx_get2rmt(p_header, p_payload);       
  				if( rcode != OK) {
					switch(rcode) {
						case EMOLTIMEDOUT:
							SVRDEBUG("SVR_PROXY: Sending HELLO \n");
							p_header->c_cmd = CMD_NONE;
							p_header->c_len = 0;
							p_header->c_rcode = 0;
							rcode = OK;
							break;
						case EMOLNOTCONN:
							rcode = EMOLNOTCONN;
							break;
						default:
							SVRDEBUG("ERROR  mnx_get2rmt %d\n", rcode);
						continue;
					}
					if( rcode != OK) break;
				}

				switch(p_header->c_cmd) {
					case CMD_SEND_MSG:
					case CMD_NTFY_MSG:
					case CMD_REPLY_MSG:
						m_ptr = &p_header->c_u.cu_msg;
						SVRDEBUG("SVR_PROXY: %d "MSG1_FORMAT,pid, MSG1_FIELDS(m_ptr));
						break;
					case CMD_COPYIN_DATA:
					case CMD_COPYOUT_RQST:
						SVRDEBUG("SVR_PROXY: %d "VCOPY_FORMAT,pid, VCOPY_FIELDS(p_header)); 
						break;
					default:
						if (p_header->c_cmd & CMD_ACKNOWLEDGE){
							SVRDEBUG("SVR_PROXY: sending ACK \n"); 
						}else{
							rcode =EINVAL;
						}
						break;
				} 
		
				rcode = svr_send_rmt();
				if ( rcode != OK ) {
	    			smsg_fail++;
				}else{
					smsg_ok++;
					if ((p_header->c_cmd != CMD_NTFY_MSG) &&
						(p_header->c_cmd != CMD_NONE))
						px_status=PX_FROM_REMOTE;
				}
				break;
			default:
				fprintf(stderr, "SVR_PROXY: Bad proxy px_status =%d\n", px_status);
				exit(1);
		}
		if ( rcode != OK && rcode != EMOLDONTACK) break;					
	}while(TRUE);
	SYSERR(rcode);
	
	rcode = mnx_proxy_conn(px.px_id, DISCONNECT_RPROXY);
	rcode = mnx_proxy_conn(px.px_id, DISCONNECT_SPROXY);
	rcode = close(rhost_sd);

	ERROR_RETURN(rcode);
}


                
/*------------------------------------------------------
 px_init_server: bind and setup a listening socket 
   This socket is not accepting connections yet after
   end of this call.
------------------------------------------------------*/                
int px_init_server(void) 
{
	int ret;
	short int port_no;
	struct sockaddr_in servaddr;
	int optval = 1;

	port_no = (BASE_PORT+px.px_id);
	SVRDEBUG("SVR_PROXY: for node %s running at port=%d\n", px.px_name, port_no);

// Create server socket.
	if ( (listen_sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
        	ERROR_EXIT(errno);

	if( (ret = setsockopt(listen_sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))) < 0)
        	ERROR_EXIT(errno);

// Bind (attach) this process to the server socket.
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(port_no);
	ret = bind(listen_sd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    	if(ret < 0) ERROR_EXIT(errno);

	SVRDEBUG("SVR_PROXY: is bound to port=%d listen_sd=%d\n", port_no, listen_sd);

// Turn 'rproxy_sd' to a listening socket. Listen queue size is 1.
	ret = listen(listen_sd, 0);
	if(ret < 0) ERROR_EXIT(errno);

	return(OK);
}

/*----------------------------------------------*/
/*		MAIN: 			*/
/*----------------------------------------------*/
void  main ( int argc, char *argv[] )
{
	int rcode;
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

	SVRDEBUG("SVR_PROXY: Initializing proxy. PID: %d\n", pid);

	posix_memalign( (void**) &p_header, getpagesize(), (sizeof(proxy_hdr_t)+ getpagesize()));
	if (p_header== NULL) {
		perror(" p_header posix_memalign");
		exit(1);
	}

	posix_memalign( (void**) &p_payload, getpagesize(), (sizeof(proxy_payload_t)+ getpagesize()));
	if (p_payload== NULL) {
    		perror(" p_payload posix_memalign");
    		exit(1);
  	}
	
	SVRDEBUG("p_header=%p p_payload=%p diff=%d\n", 
			p_header, p_payload, ((char*) p_payload - (char*) p_header));

	rcode = px_init_server();
	if (rcode) 
		   	ERROR_EXIT(rcode);		

	rcode = mnx_proxies_bind(px.px_name, px.px_id, pid, pid,MAXCOPYBUF);
   	if( rcode < 0) ERROR_EXIT(rcode);
	px_ptr = &px;
	SVRDEBUG(PX_USR_FORMAT , PX_USR_FIELDS(px_ptr));
	SVRDEBUG("binded to (%d,%d)\n", pid, pid);
	rcode= mnx_node_up(px.px_name, px.px_id, px.px_id);
   	if( rcode < 0) ERROR_EXIT(rcode);

	do {
      	rcode = px_server_loop();
//    	rcode = mnx_proxies_unbind(px.px_id);
//		SYSERR(rcode);
	}while(TRUE);

	rcode= mnx_node_down(px.px_id);
	SYSERR(rcode);

	if (rcode) 
	   	ERROR_EXIT(rcode);

    exit(0);
}






