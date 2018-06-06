/********************************************************/
/* M3-IPC CLIENT SIDE 	TCP PROXY (ONE PROCESS)		*/
/* This proxy acts as a Client for a SERVER TCP PROXY	*/
/********************************************************/

#include "proxy.h"
#include "debug.h"
#include "macros.h"

#define PX_NONE			0
#define	PX_TO_REMOTE		1
#define	PX_FROM_REMOTE		2

#define CONNECT_TIMEOUT 5 /* SECONDS */

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
int    rhost_sd;
int pid, px_status;

extern int errno;


/*----------------------------------------------------
* px_send_header: send  header to remote receiver 
----------------------------------------------------*/
int  px_send_header(void ) 
{
	int sent = 0;        // how many bytes we've sent
	int bytesleft;
	int n, total;
	char *p_ptr;

	bytesleft = sizeof(proxy_hdr_t); // how many bytes we have left to send
	total = bytesleft;
	SVRDEBUG("CLT_PROXY: send header=%d \n", bytesleft);

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
	SVRDEBUG("CLT_PROXY: socket=%d sent header=%d \n", rhost_sd, total);
	return(OK);
}

/*----------------------------------------------------
/* px_send_payload: send payload to remote receiver 
----------------------------------------------------*/
int  px_send_payload(void ) 
{
	int sent = 0;        // how many bytes we've sent
	int bytesleft;
	int n, total;
	char *p_ptr;

	if( p_header->c_len <= 0) ERROR_EXIT(EMOLINVAL);
	
	bytesleft =  p_header->c_len; // how many bytes we have left to send
	total = bytesleft;
	SVRDEBUG("CLT_PROXY: send payload=%d \n", bytesleft);

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
	SVRDEBUG("CLT_PROXY: socket=%d sent payload=%d \n", rhost_sd, total);
    	return(OK);
}
           

/*------------------------------------------------------------ 
 * px_to_remote: send a message (header + payload if existing) 
 * to remote receiver
--------------------------------------------------------------*/
int  px_to_remote(void) 
{
	int rcode;

	SVRDEBUG("CLT_PROXY:" CMD_FORMAT,CMD_FIELDS(p_header));

	/* send the header */
	rcode =  px_send_header();
	if ( rcode != OK)  ERROR_RETURN(rcode);

	if( p_header->c_len > 0) {
			SVRDEBUG("CLT_PROXY: send payload len=%d\n", p_header->c_len );
			rcode =  px_send_payload();
			if ( rcode != OK)  ERROR_RETURN(rcode);
	}
	
    return(OK);
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
		SVRDEBUG("CLT_PROXY: n:%d | received:%d | HEADER_SIZE:%d\n", n,received,sizeof(proxy_hdr_t));
        	if (received >= sizeof(proxy_hdr_t)) {  
			SVRDEBUG("CLT_PROXY: " CMD_FORMAT,CMD_FIELDS(p_header));
        		return(OK);
        	} else {
			SVRDEBUG("CLT_PROXY: Header partially received. There are %d bytes still to get\n", 
                  		sizeof(proxy_hdr_t) - received);
        		p_ptr += n;
        	}
		len = sizeof(struct sockaddr_in);
   	}
    
 	ERROR_RETURN(errno);
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
		SVRDEBUG("CLT_PROXY: n:%d | received:%d\n", n,received);
        	if (received >= payload_size) return(OK);
       		p_ptr += n;
		len = sizeof(struct sockaddr_in);
   	}
    
    	if(n < 0) ERROR_RETURN(errno);
 	return(OK);
}

/*------------------------------------------------------------
px_from_remote: receives the header and payload if any of a 
REPLY message. Then deliver the message to local 
------------------------------------------------------------*/
int px_from_remote(void) {
	int rcode, payload_size;

	bzero(p_header, sizeof(proxy_hdr_t));
	SVRDEBUG("CLT_PROXY: About to receive REPLY header\n");
	rcode = px_receive_header();
	if (rcode != 0) ERROR_RETURN(rcode);

	SVRDEBUG("CLT_PROXY:" CMD_FORMAT,CMD_FIELDS(p_header));
   	if(p_header->c_cmd == CMD_NONE) return(OK);
    
	/* now we have a proxy header in the buffer. Cast it.*/
	payload_size = p_header->c_len;
    
	/* payload could be zero */
    if(payload_size != 0) {
    		bzero(p_payload, payload_size);
    		rcode = px_receive_payload(payload_size);
    		if (rcode != 0){
			SVRDEBUG("CLT_PROXY: No payload to receive.\n");
			ERROR_RETURN(rcode);
		}
    }
	
	return(OK);  
}


/*------------------------------------------------------------
 px_connect_to_remote: connects to the remote receiver 
--------------------------------------------------------------*/
int px_connect_to_remote(void) 
{
    int port_no, rcode, i;
    char rmt_ipaddr[INET_ADDRSTRLEN+1];

    port_no = (BASE_PORT+local_nodeid);
	SVRDEBUG("SPROXY: for node %s running at port=%d\n", px.px_name, port_no);    

	// Connect to the server client	
    rmt_host_addr.sin_family = AF_INET;  
    rmt_host_addr.sin_port = htons(port_no);  

    rmthost = gethostbyname(px.px_name);
	if( rmthost == NULL) ERROR_EXIT(h_errno);
	for( i =0; rmthost->h_addr_list[i] != NULL; i++) {
		SVRDEBUG("SPROXY: remote host address %i: %s\n", 
			i, inet_ntoa( *( struct in_addr*)(rmthost->h_addr_list[i])));
	}

    if((inet_pton(AF_INET,inet_ntoa( *( struct in_addr*)(rmthost->h_addr_list[0])), (struct sockaddr*) &rmt_host_addr.sin_addr)) <= 0)
    	ERROR_RETURN(errno);

    inet_ntop(AF_INET, (struct sockaddr*) &rmt_host_addr.sin_addr, rmt_ipaddr, INET_ADDRSTRLEN);
	SVRDEBUG("SPROXY: for node %s running at  IP=%s\n", px.px_name, rmt_ipaddr);    

	rcode = connect(rhost_sd, (struct sockaddr *) &rmt_host_addr, sizeof(rmt_host_addr));
    if (rcode != 0) ERROR_RETURN(-errno);
    return(OK);
}


/*------------------------------------------------------------
 px_client_loop: start connection to remote server
 and process messages
--------------------------------------------------------------*/
int px_client_loop(void)
{
	int sender_addrlen;
	char ip4[INET_ADDRSTRLEN];  // space to hold the IPv4 string
	struct sockaddr_in sa;
	int rcode, px_status;
	message *m_ptr;
	
	do {
		rcode = px_connect_to_remote();
		if (rcode < 0) {
			SVRDEBUG("SPROXY: Could not connect to %d"
							" Sleeping for a while...\n", px.px_id);
			sleep(CONNECT_TIMEOUT);
		}
	} while (rcode != 0);
	
	
	rcode = mnx_proxy_conn(px.px_id, CONNECT_RPROXY);
	rcode = mnx_proxy_conn(px.px_id, CONNECT_SPROXY);
	
	/* STATE MACHINE */
	px_status = PX_TO_REMOTE;
	do { 
		rcode = OK;			
		switch(px_status){
			case  PX_TO_REMOTE:
				SVRDEBUG("CLT_PROXY: PX_TO_REMOTE\n");
				rcode = mnx_get2rmt(p_header, p_payload);
				SVRDEBUG("CLT_PROXY: mnx_get2rmt rcode =%d \n", rcode);
				if( rcode != OK) {
					switch(rcode) {
						case EMOLTIMEDOUT:
							SVRDEBUG("CLT_PROXY: Sending HELLO \n");
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
					
				if( rcode != OK) break;
	
				SVRDEBUG("CLT_PROXY: %d "HDR_FORMAT,pid, HDR_FIELDS(p_header)); 

				if(	p_header->c_cmd == CMD_SEND_MSG || 
					p_header->c_cmd == CMD_NTFY_MSG ||
					p_header->c_cmd == CMD_SNDREC_MSG ||
					p_header->c_cmd == CMD_REPLY_MSG ) {
					m_ptr = &p_header->c_u.cu_msg;
					SVRDEBUG("CLT_PROXY: %d "MSG1_FORMAT,pid, MSG1_FIELDS(m_ptr)); 
				} 

				/* send the message to remote */
				rcode =  px_to_remote();
				if (rcode == 0) {
					smsg_ok++;
					if( (p_header->c_cmd != CMD_NTFY_MSG) 
					 && (p_header->c_cmd != CMD_NONE)
					 && (p_header->c_cmd != CMD_SEND_ACK))
						px_status = PX_FROM_REMOTE;
				} else {
					smsg_fail++;
				}
				break;
			case  PX_FROM_REMOTE:
				SVRDEBUG("CLT_PROXY: PX_FROM_REMOTE\n");
				rcode = px_from_remote();
				if (rcode == 0) {
					rmsg_ok++;
					if( 	p_header->c_cmd == CMD_COPYIN_DATA ||
						p_header->c_cmd == CMD_COPYOUT_RQST) {
						SVRDEBUG("CLT_PROXY: %d "VCOPY_FORMAT,pid, VCOPY_FIELDS(p_header)); 
					}

					if(p_header->c_cmd != CMD_NONE){
						rcode = mnx_put2lcl(p_header, p_payload);
						if(p_header->c_cmd != CMD_NTFY_MSG )
							px_status = PX_TO_REMOTE;
					}
				} else {
					rmsg_fail++;
				}
				break;
			default:
				fprintf(stderr, "CLT_PROXY: Bad proxy px_status =%d\n", px_status);
				exit(1);
		}
		if( rcode != OK) break;					
	}while(TRUE);

	rcode = mnx_proxy_conn(px.px_id, DISCONNECT_RPROXY);
	rcode = mnx_proxy_conn(px.px_id, DISCONNECT_SPROXY);
	return(EMOLNOTCONN);
  
    /* never reached */
}


/*------------------------------------------------------
 px_client_init: bind and setup a listening socket 
   This socket is not accepting connections yet after
   end of this call.
------------------------------------------------------*/                
int px_client_init(void) 
{
	// Create socket.
	if ( (rhost_sd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		ERROR_EXIT(errno)
	}

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

	SVRDEBUG("CLT_PROXY: Initializing proxy. PID: %d\n", pid);

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

	rcode = px_client_init();
   	if( rcode < 0) ERROR_EXIT(rcode);

    rcode = mnx_proxies_bind(px.px_name, px.px_id, pid, pid, MAXCOPYBUF);
    if( rcode < 0) ERROR_EXIT(rcode);
	
	px_ptr = &px;
	SVRDEBUG(PX_USR_FORMAT , PX_USR_FIELDS(px_ptr));
	SVRDEBUG("binded to (%d,%d)\n", pid, pid);

	rcode= mnx_node_up(px.px_name, px.px_id, px.px_id);
    if( rcode < 0) ERROR_EXIT(rcode);

	px_status = PX_NONE;
	do {
		rcode= px_client_loop();
		SYSERR(rcode);
	}while(TRUE);
	
	rcode= mnx_node_down(px.px_id);
	SYSERR(rcode);

//	rcode = mnx_proxies_unbind(px.px_name);

	exit(0);
}
