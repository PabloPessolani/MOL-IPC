/********************************************************/
/* 		SOCAT SENDER PROXY				*/
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
dvs_usr_t dvs;   
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
	SOCATDBG("SPROXY: send header=%d \n", bytesleft);

    p_ptr = (char *) p_header;
    while(sent < total) {
		n = fwrite(p_ptr, 1, bytesleft, stdout);
//        n = send(sproxy_sd, p_ptr, bytesleft, 0);
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
	fflush(stdout);
	SOCATDBG("SPROXY: socket=%d sent header=%d \n", sproxy_sd, total);
    return(OK);
}

/* ps_send_payload: send payload to remote receiver */
int  ps_send_payload(void ) 
{
    int sent = 0;        // how many bytes we've sent
    int bytesleft;
    int n, total;
	char *p_ptr;

	if( p_header->c_len <= 0) ERROR_EXIT(EMOLINVAL);
	
	bytesleft =  p_header->c_len; // how many bytes we have left to send
	total = bytesleft;
	SOCATDBG("SPROXY: send header=%d \n", bytesleft);

    p_ptr = (char *) p_payload;
    while(sent < total) {
		n = fwrite(p_ptr, 1, bytesleft, stdout);
//        n = send(sproxy_sd, p_ptr, bytesleft, 0);
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
	fflush(stdout);
	SOCATDBG("SPROXY: socket=%d sent payload=%d \n", sproxy_sd, total);
    return(OK);
}

/* 
 * ps_send_remote: send a message (header + payload if existing) 
 * to remote receiver
 */
int  ps_send_remote(void) 
{
	int rcode;

	SOCATDBG("SPROXY:" CMD_FORMAT,CMD_FIELDS(p_header));

	/* send the header */
	rcode =  ps_send_header();
	if ( rcode != OK)  ERROR_RETURN(rcode);

	if( p_header->c_len > 0) {
		SOCATDBG("SPROXY: send payload len=%d\n", p_header->c_len );
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
    int rcode, i;
    message *m_ptr;
    int pid, ret;
   	char *ptr; 

   	SOCATDBG("SPROXY: pid=%d\n", pid);

    while(1) {
		SOCATDBG("SPROXY %d: Waiting a message\n", pid);
        
		ret = mnx_get2rmt(p_header, p_payload);       
      
		if(ret == EMOLTIMEDOUT) continue;
		
		if( ret != OK) {
			switch(ret) {
				case EMOLTIMEDOUT:
					SOCATDBG("SPROXY: Sending HELLO \n");
					p_header->c_cmd = CMD_NONE;
					p_header->c_len = 0;
					p_header->c_rcode = 0;
					break;
				case EMOLNOTCONN:
					return(EMOLNOTCONN);
				default:
					SOCATDBG("ERROR  mnx_get2rmt %d\n", ret);
					continue;
			}
		}

		SOCATDBG("SPROXY: %d "HDR_FORMAT,pid, HDR_FIELDS(p_header)); 

		switch(p_header->c_cmd){
			case CMD_SEND_MSG:
			case CMD_SNDREC_MSG:
			case CMD_REPLY_MSG:
				m_ptr = &p_header->c_u.cu_msg;
				SOCATDBG("SPROXY: " MSG1_FORMAT,  MSG1_FIELDS(m_ptr));
				break;
			case CMD_COPYIN_DATA:
			case CMD_COPYOUT_DATA:
				SOCATDBG("SPROXY: "VCOPY_FORMAT, VCOPY_FIELDS(p_header)); 
				break;
			default:
				break;
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
   	SOCATDBG("SPROXY:\n");
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

	SOCATDBG("SPROXY: Initializing on PID:%d\n", getpid());
    
	do { 
		rcode = mnx_wait4bind_T(RETRY_US);
		SOCATDBG("SPROXY: mnx_wait4bind_T  rcode=%d\n", rcode);
		if (rcode == EMOLTIMEDOUT) {
			SOCATDBG("SPROXY: mnx_wait4bind_T TIMEOUT\n");
			continue ;
		}else if(rcode == NONE) { /* proxies have not endpoint */
			break;	
		} if( rcode < 0) 
			exit(EXIT_FAILURE);
	} while	(rcode < OK);
		
	posix_memalign( (void**) &p_header, getpagesize(), (sizeof(proxy_hdr_t)));
	if (p_header== NULL) {
    		perror("p_header posix_memalign");
    		exit(1);
  	}

	posix_memalign( (void**) &p_payload, getpagesize(), (sizeof(proxy_payload_t)));
	if (p_payload== NULL) {
    		perror("p_payload posix_memalign");
    		exit(1);
  	}
	
	ps_start_serving();
	
    /* code never reaches here */
}

/*----------------------------------------------*/
/*		MAIN: 			*/
/*----------------------------------------------*/
void  main ( int argc, char *argv[] )
{
    int spid, rpid, rcode, pxid;
    int ret;
    dvs_usr_t *d_ptr;    
static	char			pxname[MAXPROXYNAME];

	
    if (argc != 4) {
     	fprintf(stderr,"Usage: %s <pxname> <pxid> <rproxy_PID>\n", argv[0]);
    	exit(0);
    }
	
	log_fp = fopen("sproxy.log", "w");
	if(log_fp == NULL){
		fprintf(stderr, "fopen sproxy.log errno=%d\n", errno);
		exit(1);
	}
	
	pxid = atoi(argv[2]);
    SOCATDBG("SOCAT Proxy Pair id: %d\n",pxid);
	
	rpid = atoi(argv[3]);
    SOCATDBG("SOCAT receiver proxy PID: %d\n",rpid); 

    spid = getpid();
    SOCATDBG("SOCAT sender proxy PID: %d\n",spid); 
	
    local_nodeid = mnx_getdvsinfo(&dvs);
    d_ptr=&dvs;
	SOCATDBG(DVS_USR_FORMAT,DVS_USR_FIELDS(d_ptr));

    /* register the proxies */
    rcode = mnx_proxies_bind(pxname, pxid, spid, rpid,  MAXCOPYBUF);
    if( rcode < 0) ERROR_EXIT(rcode);
	rcode = mnx_proxy_conn(pxid, CONNECT_SPROXY);
    if( rcode < 0) ERROR_EXIT(rcode);
	rcode = mnx_proxy_conn(pxid, CONNECT_RPROXY);
    if( rcode < 0) ERROR_EXIT(rcode);
	rcode= mnx_node_up(pxname, pxid, pxid);	
    if( rcode < 0) ERROR_EXIT(rcode);

	SOCATDBG("MAIN: pxid=%d spid=%d rpid=%d local_nodeid=%d\n", 
		pxid , spid, rpid, local_nodeid);

	ps_init();

    exit(0);
}

