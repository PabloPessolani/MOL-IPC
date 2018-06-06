/********************************************************/
/* 		SOCAT RECEIVER PROXY				*/
/********************************************************/

#include "proxy.h"
#include "debug.h"
#include "macros.h"

#define HEADER_SIZE sizeof(proxy_hdr_t)
#define BASE_PORT      3000

#define RETRY_US		2000 /* Microseconds */
#define BIND_RETRIES	3
#define WAIT4UP			3
int local_nodeid;

proxy_hdr_t 	*p_header;
proxy_hdr_t 	*p_pseudo;
proxy_payload_t *p_payload;

int rmsg_ok = 0;
int rmsg_fail = 0;
int smsg_ok = 0;
int smsg_fail = 0; 
dvs_usr_t DVS;   
struct hostent *rmthost;
proxies_usr_t px, *px_ptr;

struct sockaddr_in rmtclient_addr, rmtserver_addr;
int    rlisten_sd, rconn_sd;
int    sproxy_sd;

FILE *log_fp;
extern int errno;

#define SOCATDBG(text, args ...) \
 do { \
     fprintf(log_fp, " %s:%s:%u:" \
             text ,__FILE__ ,__FUNCTION__ ,__LINE__, ## args); \
     fflush(log_fp);\
 }while(0);

/*----------------------------------------------*/
/*      PROXY RECEIVER FUNCTIONS               */
/*----------------------------------------------*/
                
/* pr_setup_connection: bind and setup a listening socket 
   This socket is not accepting connections yet after
   end of this call.
 */                
int pr_setup_connection(void) 
{
	SOCATDBG("RPROXY\n");
	return(OK);  
}

/* pr_receive_payloadr: receives the header from remote sender */
int pr_receive_payload(int payload_size) 
{
    int n, len, total,  received = 0;
    char *p_ptr;

   	SOCATDBG("RPROXY: payload_size=%d\n",payload_size);
   	p_ptr = (char*) p_payload;
   	len = sizeof(struct sockaddr_in);
//   	while ((n = recv(rconn_sd, p_ptr, (payload_size-received), 0 )) > 0) {
	while ((n = fread(p_ptr, 1, (payload_size-received), stdin) ) > 0) {
        received = received + n;
		SOCATDBG("RPROXY: n:%d | received:%d\n", n,received);
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

   	SOCATDBG("RPROXY: \n");
   	p_ptr = (char*) p_header;
	total = sizeof(proxy_hdr_t);
   	len = sizeof(struct sockaddr_in);
//   	while ((n = recv(rconn_sd, p_ptr, (total-received), 0 )) > 0) {
	while ((n = fread(p_ptr, 1, (total-received), stdin) ) > 0) {
		
        received = received + n;
		SOCATDBG("RPROXY: n:%d | received:%d | HEADER_SIZE:%d\n", n,received,sizeof(proxy_hdr_t));
        if (received >= sizeof(proxy_hdr_t)) {  
			SOCATDBG("RPROXY: " CMD_FORMAT,CMD_FIELDS(p_header));
        	return(OK);
        } else {
			SOCATDBG("RPROXY: Header partially received. There are %d bytes still to get\n", 
                  	sizeof(proxy_hdr_t) - received);
        	p_ptr += n;
        }
		len = sizeof(struct sockaddr_in);
   	}
    
	if( n == 0){
		if(feof(stdin)){
			SOCATDBG("RPROXY: EOF SET \n"); 
		}
		if(ferror(stdin)){
			SOCATDBG("RPROXY: ERROR SET \n"); 			
		}
		return(OK);
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

    do {
		bzero(p_header, sizeof(proxy_hdr_t));
		SOCATDBG("RPROXY: About to receive header\n");
    	rcode = pr_receive_header();
    	if (rcode != 0) ERROR_RETURN(rcode);
		SOCATDBG("RPROXY:" CMD_FORMAT,CMD_FIELDS(p_header));
   	}while(p_header->c_cmd == CMD_NONE);
    
	/* now we have a proxy header in the buffer. Cast it.*/
    payload_size = p_header->c_len;
    
    /* payload could be zero */
    if(payload_size != 0) {
        bzero(p_payload, payload_size);
        rcode = pr_receive_payload(payload_size);
        if (rcode != 0){
			SOCATDBG("RPROXY: No payload to receive.\n");
		}
    }else{
		switch(p_header->c_cmd){
			case CMD_SEND_MSG:
			case CMD_SNDREC_MSG:
			case CMD_REPLY_MSG:
				m_ptr = &p_header->c_u.cu_msg;
				SOCATDBG("RPROXY: " MSG1_FORMAT,  MSG1_FIELDS(m_ptr));
				break;
			case CMD_COPYIN_DATA:
			case CMD_COPYOUT_DATA:
				SOCATDBG("RPROXY: "VCOPY_FORMAT, VCOPY_FIELDS(p_header)); 
				break;
			default:
				break;
		}
	}
	
	SOCATDBG("RPROXY: put2lcl\n");
	rcode = mnx_put2lcl(p_header, p_payload);
#ifdef RMT_CLIENT_BIND	
	if( rcode == OK) return (OK);
	/******************** REMOTE CLIENT BINDING ************************************/
	/* rcode: the result of the las mnx_put2lcl 	*/
	/* ret: the result of the following operations	*/
	SOCATDBG("RPROXY: REMOTE CLIENT BINDING rcode=%d\n", rcode);
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

   	SOCATDBG("RPROXY: \n");

    while (1){
    	/* Serve Forever */
		do { 
	       	/* get a complete message and process it */
       		rcode = pr_process_message();
       		if (rcode == OK) {
				SOCATDBG("RPROXY: Message succesfully processed.\n");
				rmsg_ok++;
        	} else {
				SOCATDBG("RPROXY: Message processing failure [%d]\n",rcode);
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
    int receiver_sd, rcode;

	SOCATDBG("RPROXY: Initializing proxy receiver. PID: %d\n", getpid());
    
	do { 
		rcode = mnx_wait4bind_T(RETRY_US);
		SOCATDBG("RPROXY: mnx_wait4bind_T  rcode=%d\n", rcode);
		if (rcode == EMOLTIMEDOUT) {
			SOCATDBG("RPROXY: mnx_wait4bind_T TIMEOUT\n");
			continue ;
		}else if(rcode == NONE) { /* proxies have not endpoint */
			break;	
		} if( rcode < 0) 
			exit(EXIT_FAILURE);
	} while	(rcode < OK);
	
	posix_memalign( (void**) &p_header, getpagesize(), (sizeof(proxy_hdr_t)));
	if (p_header== NULL) {
    		perror(" p_header posix_memalign");
    		exit(1);
  	}

	posix_memalign( (void**) &p_payload, getpagesize(), (sizeof(proxy_payload_t)));
	if (p_payload== NULL) {
    		perror(" p_payload posix_memalign");
    		exit(1);
  	}

	posix_memalign( (void**) &p_pseudo, getpagesize(), (sizeof(proxy_hdr_t)));
	if (p_payload== NULL) {
    		perror(" p_payload posix_memalign");
    		exit(1);
  	}
	
	SOCATDBG("p_header=%p p_payload=%p diff=%d\n", 
			p_header, p_payload, ((char*) p_payload - (char*) p_header));

    if( pr_setup_connection() == OK) {
		sleep(WAIT4UP);
      	pr_start_serving();
 	} else {
       	ERROR_EXIT(errno);
    }
}

/*----------------------------------------------*/
/*		MAIN: 			*/
/*----------------------------------------------*/
void  main ( int argc, char *argv[] )
{
    int spid, rpid, pid, status;
    int ret;
    dvs_usr_t *d_ptr;    

    if (argc != 1) {
     	fprintf(stderr,"Usage: %s\n", argv[0]);
    	exit(0);
    }

	log_fp = fopen("rproxy.log", "w");
	if(log_fp == NULL){
		fprintf(stderr, "fopen rproxy.log errno=%d\n", errno);
		exit(1);
	}
	
    local_nodeid = mnx_getdvsinfo(&DVS);
    d_ptr=&DVS;
	SOCATDBG(DVS_USR_FORMAT,DVS_USR_FIELDS(d_ptr));

    pid = getpid();
	SOCATDBG("MAIN: pid=%d local_nodeid=%d\n", pid, local_nodeid);

	pr_init();
    exit(0);
}

