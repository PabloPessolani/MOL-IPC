/********************************************************/
/* 		UDP PROXIES				*/
/********************************************************/


#include "proxy.h"
#include "debug.h"
#include "macros.h"

#define HEADER_SIZE sizeof(proxy_hdr_t)
#define BASE_PORT      3000
            
int local_nodeid;

proxy_hdr_t 	*p_header;
proxy_payload_t *p_payload;

int rmsg_ok = 0;
int rmsg_fail = 0;
int smsg_ok = 0;
int smsg_fail = 0; 
drvs_usr_t drvs;   
struct hostent *rmthost;
proxies_usr_t px, *px_ptr;

struct sockaddr_in rmtclient_addr, rmtserver_addr;
int    rproxy_sd;
int    sproxy_sd;

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
    if ( (rproxy_sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
        ERROR_EXIT(errno);

    if( (ret = setsockopt(rproxy_sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))) < 0)
       	ERROR_EXIT(errno);

    // Bind (attach) this process to the server socket.
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port_no);
   	ret = bind(rproxy_sd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    if(ret < 0) ERROR_EXIT(errno);

	SVRDEBUG("RPROXY: is bound to port=%d socket=%d\n", port_no, rproxy_sd);

   	return(OK);
   
}


/* pr_receive_datagram: receives a datagram from remote sender */
int pr_receive_datagram(void) 
{
    int n, len, total,  received = 0;
    char *p_ptr;

   	SVRDEBUG("socket=%d\n", rproxy_sd);
   	p_ptr = (char*) p_header;
	total = sizeof(proxy_hdr_t)+sizeof(proxy_payload_t);
   	len = sizeof(struct sockaddr_in);
   	while ((n = recvfrom(rproxy_sd, p_ptr, (total-received), 0, (struct sockaddr*) &rmtclient_addr, &len )) > 0) {
        	received = received + n;
			SVRDEBUG("RPROXY: n:%d | received:%d | HEADER_SIZE:%d\n", n,received,sizeof(proxy_hdr_t));
        	if (received >= sizeof(proxy_hdr_t)) {  
				SVRDEBUG("RPROXY: " CMD_FORMAT,CMD_FIELDS(p_header));
            	return(OK);
        	} else {
				SVRDEBUG("RPROXY: Header partially received. There are %d bytes still to get\n", 
                sizeof(proxy_hdr_t) - received);
            	p_ptr += n;
        	}
		len = sizeof(struct sockaddr_in);
   	}
    
    if(n < 0) ERROR_RETURN(errno);
 	return(OK);
}

/* pr_process_message: receives header and payload if any. Then deliver the 
 * message to local */
int pr_process_message(void) {
    int rcode, payload_size;
	int pid;
	pid = getpid();

    	/* clean buffer */
    do {
//		bzero(p_header, sizeof(proxy_hdr_t));
		SVRDEBUG("RPROXY: About to receive header\n");
    	rcode = pr_receive_datagram();
    	if (rcode != 0) ERROR_RETURN(rcode);
		SVRDEBUG("RPROXY:" CMD_FORMAT,CMD_FIELDS(p_header));
   	}while(p_header->c_cmd == CMD_NONE);
    
	SVRDEBUG("RPROXY: put2lcl\n");
	rcode = mnx_put2lcl(p_header, p_payload);
    if( rcode) ERROR_RETURN(rcode);
    return(OK);
    
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

	sleep(2);
	rcode = mnx_proxy_conn(px.px_id, CONNECT_RPROXY);

    while (1){
    
    	/* Serve Forever */
		do { 
	       	/* get a complete message and process it */
       		rcode = pr_process_message();
       		if (rcode == OK) {
				SVRDEBUG("RPROXY: Message succesfully processed.\n");
				rmsg_ok++;
       		} else {
				SVRDEBUG("RPROXY: Message processing failure [%d]\n",rcode);
           		rmsg_fail++;
				if( rcode == EMOLNOTCONN) break;
       		}	
		}while(1);
    }
    
    /* never reached */
}

/* pr_init: creates socket */
void pr_init(void) 
{
   	int receiver_sd;
	char *p_buffer;

	SVRDEBUG("RPROXY: Initializing proxy receiver. PID: %d\n", getpid());
	posix_memalign( (void**) &p_buffer, getpagesize(), sizeof(proxy_hdr_t)+sizeof(proxy_payload_t));
	if (p_buffer== NULL) {
    		perror(" p_buffer malloc");
    		exit(1);
  	}

	p_header 	= (proxy_hdr_t*) 	p_buffer;
	p_payload	= (proxy_payload_t *)	(p_buffer+ sizeof(proxy_hdr_t));
	    
   	if( pr_setup_connection() == OK) {
       	pr_start_serving();
 	} else {
         	ERROR_EXIT(errno);
   	}
}

/*----------------------------------------------*/
/*      PROXY SENDER FUNCTIONS                  */
/*----------------------------------------------*/

/* ps_send_datagram: send complete header+payload to remote receiver */
int  ps_send_datagram(void ) 
{
    int sent = 0;        // how many bytes we've sent
    int bytesleft;
    int n, total;
	char *p_ptr;

	bytesleft = sizeof(proxy_hdr_t)+p_header->c_len; // how many bytes we have left to send
	total = bytesleft;

	SVRDEBUG("SPROXY: About to send a packet. Total:%d header=%d Payload=%d\n"
		,bytesleft, sizeof(proxy_hdr_t), p_header->c_len);

    p_ptr = (char *) p_header;
    while(sent < total) {
		SVRDEBUG("SPROXY: sendto socket=%d sent=%d bytesleft=%d \n", sproxy_sd, sent, bytesleft);
       	n = sendto(sproxy_sd, p_ptr, bytesleft, 0, (struct sockaddr*) &rmtserver_addr, sizeof(rmtserver_addr));
        if (n < 0) {
			if(errno == EALREADY) {
				SYSERR(errno);
				sleep(1);
				continue;
			}else{
				ERROR_RETURN(errno);
			}
		}
		SVRDEBUG("SPROXY: Sent %d bytes of payload.\n", n);
       	sent += n;
		p_ptr += n; 
       	bytesleft -= n;
    }
    return(OK);
}

/* 
 * ps_send_remote: send a message (header + payload if existing) 
 * to remote receiver
 */
int  ps_send_remote(void) 
{
   
	SVRDEBUG("SPROXY: About to send a message\n");
   
    /* send the packet */
    if ( ps_send_datagram() == OK) {
		SVRDEBUG("SPROXY:" CMD_FORMAT,CMD_FIELDS(p_header));
    }	

    return(0);
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

   	pid = getpid();
	
    while(1) {
		SVRDEBUG("SPROXY %d: Waiting a message\n", pid);

SVRDEBUG("SPROXY before get2rmt\n");
        	ret = mnx_get2rmt(p_header, p_payload);   
SVRDEBUG("SPROXY after get2rmt\n");
      
		if( ret != OK) {
			switch(ret) {
				case EMOLTIMEDOUT:
					SVRDEBUG("SPROXY: Sending HELLO \n");
					p_header->c_cmd = CMD_NONE;
					p_header->c_len = 0;
					p_header->c_rcode = 0;
					break;
				case EMOLNOTCONN:
					return(EMOLNOTCONN);
				default:
					SVRDEBUG("ERROR  mnx_get2rmt %d\n", ret);
					continue;
			}
		}

		SVRDEBUG("SPROXY: %d "HDR_FORMAT,pid, HDR_FIELDS(p_header)); 

		if(	p_header->c_cmd == CMD_SEND_MSG || 
			p_header->c_cmd == CMD_NTFY_MSG ||
			p_header->c_cmd == CMD_SNDREC_MSG ||
			p_header->c_cmd == CMD_REPLY_MSG ) {
			m_ptr = &p_payload->pay_msg;
			SVRDEBUG("SPROXY: %d "MSG1_FORMAT,pid, MSG1_FIELDS(m_ptr)); 
			}

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

    return(OK);
}

/* 
 * ps_init: creates sender socket, the connect to remote and
 * start sending messages to remote 
 */
void  ps_init(void) 
{
    int rcode = 0;
	char *p_buffer;

	SVRDEBUG("SPROXY: Initializing on PID:%d\n", getpid());
    
	posix_memalign( (void**) &p_buffer, getpagesize(), sizeof(proxy_hdr_t)+sizeof(proxy_payload_t));
	if (p_buffer== NULL) {
    		perror(" p_buffer malloc");
    		exit(1);
  	}

	p_header 	= (proxy_hdr_t*) 	p_buffer;
	p_payload	= (proxy_payload_t *)	(p_buffer+ sizeof(proxy_hdr_t));
	
    // Create server socket.
    if ( (sproxy_sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
       	ERROR_EXIT(errno)
    }

    	/* try to connect many times */
	sleep(2);

	while(1) {
		do {
			if (rcode < 0)
				SVRDEBUG("SPROXY: Could not connect to %d"
                 			" Sleeping for a while...\n", px.px_id);
			sleep(4);
			rcode = ps_connect_to_remote();
		} while (rcode != 0);

SVRDEBUG("SPROXY before CONNECT\n");
		rcode = mnx_proxy_conn(px.px_id, CONNECT_SPROXY);
		if(rcode) ERROR_EXIT(rcode);
SVRDEBUG("SPROXY after CONNECT\n");
	
		ps_start_serving();
	
		rcode = mnx_proxy_conn(px.px_id, DISCONNECT_SPROXY);
		if(rcode) ERROR_EXIT(rcode);
	
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
    drvs_usr_t *d_ptr;    

    if (argc != 3) {
       	fprintf(stderr,"Usage: %s <px_name> <px_id> \n", argv[0]);
       	exit(0);
    }

    strncpy(px.px_name,argv[1], MAXPROXYNAME);
    printf("Proxy Pair name: %s\n",px.px_name);
 
    px.px_id = atoi(argv[2]);
    printf("UDP Proxy Pair id: %d\n",px.px_id);

    local_nodeid = mnx_getdrvsinfo(&drvs);
    d_ptr=&drvs;
	SVRDEBUG(DRVS_USR_FORMAT,DRVS_USR_FIELDS(d_ptr));

    pid = getpid();
	SVRDEBUG("MAIN: pid=%d local_nodeid=%d\n", pid, local_nodeid);

    /* creates SENDER and RECEIVER Proxies as children */
    if ( (spid = fork()) == 0) ps_init();
    if ( (rpid = fork()) == 0) pr_init();

    /* register the proxies */
	/* REEMPLAZAR el sleep(2) por sincronizacion con semaforos */
	/* ver servers/init/init.c */
    sleep(2);

SVRDEBUG("SPROXY before proxies_bind\n");
    ret = mnx_proxies_bind(px.px_name, px.px_id, spid, rpid);
    if( ret < 0) ERROR_EXIT(ret);
SVRDEBUG("SPROXY after proxies_bind\n");
	
	px_ptr = &px;
	SVRDEBUG(PX_USR_FORMAT , PX_USR_FIELDS(px_ptr));
	SVRDEBUG("binded to (%d,%d)\n", spid, rpid);

	ret= mnx_node_up(px.px_name, px.px_id, px.px_id);	
	
   	wait(&status);
    wait(&status);
    exit(0);
}

