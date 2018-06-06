/********************************************************/
/* 		TIPC  PROXIES				*/
/********************************************************/

#include "proxy.h"
#include "debug.h"
#include "macros.h"

#include <lz4frame.h>
#define LZ4_HEADER_SIZE 19
#define LZ4_FOOTER_SIZE 4
#define BLOCK_16K	(16 * 1024)

static const LZ4F_preferences_t lz4_preferences = {
	{ LZ4F_max1MB, LZ4F_blockLinked, LZ4F_noContentChecksum, LZ4F_frame, 0, { 0, 0 } },
	0,   /* compression level */
	0,   /* autoflush */
	{ 0, 0, 0, 0 },  /* reserved, must be set to 0 */
};

#define CMD_LZ4_FLAG 		0x1234

#define HEADER_SIZE sizeof(proxy_hdr_t)
#define BASE_TYPE      3000

#define RETRY_US		2000 /* Microseconds */
#define BIND_RETRIES	3

#define SERVER_TYPE  18888
#define SERVER_INST  17
           
int local_nodeid;

int rmsg_ok = 0;
int rmsg_fail = 0;
int smsg_ok = 0;
int smsg_fail = 0; 
dvs_usr_t dvs;   
struct hostent *rmthost;
proxies_usr_t px, *px_ptr;

struct sockaddr_tipc rmtclient_addr, rmtserver_addr;
int    rlisten_sd, rconn_sd;
int    sproxy_sd;

struct proxy_desc_s {
	proxy_hdr_t 	*px_header;
	proxy_hdr_t 	*px_pseudo;
	proxy_payload_t *px_payload;	/* uncompressed payload 		*/	
	char 			*px_comp_pl;	/* compressed payload 		*/ 	
	int 			px_msg_ok;
	int 			px_msg_fail;	
	LZ4F_errorCode_t px_lz4err;
	size_t			px_offset;
	size_t			px_maxCsize;		/* Maximum Compressed size */
	size_t			px_maxRsize;		/* Maximum Raw size		 */
	__attribute__((packed, aligned(4)))
	LZ4F_compressionContext_t 	px_lz4Cctx __attribute__((aligned(8))); /* Compression context */
	LZ4F_decompressionContext_t px_lz4Dctx __attribute__((aligned(8))); /* Decompression context */
};
typedef struct proxy_desc_s proxy_desc_t;

proxy_desc_t px_desc;
void init_compression( proxy_desc_t *pxd_ptr);
void stop_compression( proxy_desc_t *pxd_ptr);

int compress_payload( proxy_desc_t *pxd_ptr)
{
	size_t comp_len;

	/* size_t LZ4F_compressFrame(void* dstBuffer, size_t dstMaxSize, const void* srcBuffer, 
				size_t srcSize, const LZ4F_preferences_t* preferencesPtr);
	*/	
	comp_len = LZ4F_compressFrame(
				pxd_ptr->px_comp_pl,	pxd_ptr->px_maxCsize,
				pxd_ptr->px_payload,	pxd_ptr->px_header->c_len, 
				NULL);
				
	if (LZ4F_isError(comp_len)) {
		fprintf(stderr ,"LZ4F_compressFrame failed: error %zu", comp_len);
		ERROR_EXIT(comp_len);
	}

	SVRDEBUG("SPROXY: raw_len=%d comp_len=%d\n", pxd_ptr->px_header->c_len, comp_len);
		
	return(comp_len);
}

int decompress_payload( proxy_desc_t *pxd_ptr)
{
	LZ4F_errorCode_t lz4_rcode;
	size_t comp_len, raw_len;
	
	SVRDEBUG("RPROXY: INPUT px_maxRsize=%d px_maxCsize=%d \n", 
		pxd_ptr->px_maxRsize,pxd_ptr->px_maxCsize );

	comp_len = pxd_ptr->px_header->c_len;
	raw_len  = pxd_ptr->px_maxRsize;
	lz4_rcode = LZ4F_decompress(pxd_ptr->px_lz4Dctx,
                          pxd_ptr->px_payload, &raw_len,
                          pxd_ptr->px_comp_pl, &comp_len,
                          NULL);
						  
	if (LZ4F_isError(lz4_rcode)) {
		fprintf(stderr ,"LZ4F_decompress failed: error %zu", lz4_rcode);
		ERROR_EXIT(lz4_rcode);
	}
	
	SVRDEBUG("RPROXY: OUTPUT raw_len=%d comp_len=%d\n",	raw_len, comp_len);
	return(raw_len);					  
						  
}

void init_compression( proxy_desc_t *pxd_ptr) 
{
	size_t frame_size;

	SVRDEBUG("SPROXY\n");

	pxd_ptr->px_maxRsize = sizeof(proxy_payload_t);
	frame_size = LZ4F_compressBound(sizeof(proxy_payload_t), &lz4_preferences);
	pxd_ptr->px_maxCsize =  frame_size + LZ4_HEADER_SIZE + LZ4_FOOTER_SIZE;
	posix_memalign( (void**) &pxd_ptr->px_comp_pl, getpagesize(), pxd_ptr->px_maxCsize );
	if (pxd_ptr->px_comp_pl== NULL) {
    		fprintf(stderr, "pxd_ptr->px_comp_pl posix_memalign");
			ERROR_EXIT(errno);
  	}
}

void stop_compression( proxy_desc_t *pxd_ptr) 
{
	SVRDEBUG("SPROXY\n");
}

void init_decompression( proxy_desc_t *pxd_ptr) 
{
	LZ4F_errorCode_t lz4_rcode;

	SVRDEBUG("RPROXY\n");

	pxd_ptr->px_maxRsize = sizeof(proxy_payload_t);
	
	pxd_ptr->px_maxCsize =  pxd_ptr->px_maxRsize + LZ4_HEADER_SIZE + LZ4_FOOTER_SIZE;
	posix_memalign( (void**) &pxd_ptr->px_comp_pl, getpagesize(), pxd_ptr->px_maxCsize );
	if (pxd_ptr->px_comp_pl== NULL) {
    		fprintf(stderr, "pxd_ptr->px_comp_pl posix_memalign");
			ERROR_EXIT(errno);
  	}
	
	lz4_rcode =  LZ4F_createDecompressionContext(&pxd_ptr->px_lz4Dctx, LZ4F_VERSION);
	if (LZ4F_isError(lz4_rcode)) {
		fprintf(stderr ,"LZ4F_createDecompressionContext: error %zu", lz4_rcode);
		ERROR_EXIT(lz4_rcode);
	}
}

void stop_decompression( proxy_desc_t *pxd_ptr) 
{
	LZ4F_errorCode_t lz4_rcode;

	SVRDEBUG("RPROXY\n");

	lz4_rcode = LZ4F_freeDecompressionContext(pxd_ptr->px_lz4Dctx);
	if (LZ4F_isError(lz4_rcode)) {
		fprintf(stderr ,"LZ4F_freeDecompressionContext: error %zu", lz4_rcode);
		ERROR_EXIT(lz4_rcode);
	}
}


/*----------------------------------------------*/
/*      PROXY RECEIVER FUNCTIONS               */
/*----------------------------------------------*/
                
/* pr_setup_connection: bind and setup a listening socket 
   This socket is not accepting connections yet after
   end of this call.
 */                
int pr_setup_connection(void) 
{
    // int ret;
    int server_type;
    // int optval = 1;
	
	struct sockaddr_tipc server_addr;
	
    server_type = (BASE_TYPE + px.px_id);
	SVRDEBUG("RPROXY: for node %s running at type=%d ,inst=%d\n", px.px_name, server_type, SERVER_INST);
	
    // Create server socket.
    if ( (rlisten_sd = socket(AF_TIPC, SOCK_STREAM , 0)) < 0) 
        ERROR_EXIT(errno);

/*     if( (ret = setsockopt(rlisten_sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))) < 0)
        	ERROR_EXIT(errno); */

    // Bind (attach) this process to the server socket.
	server_addr.family = AF_TIPC;
	server_addr.addrtype = TIPC_ADDR_NAMESEQ;
	server_addr.addr.nameseq.type = server_type;
	server_addr.addr.nameseq.lower = SERVER_INST;
	server_addr.addr.nameseq.upper = SERVER_INST;
	server_addr.scope = TIPC_ZONE_SCOPE;

	if (0 != bind(rlisten_sd, (struct sockaddr *)&server_addr, sizeof(server_addr))) {
		ERROR_EXIT(errno);
	}

	SVRDEBUG("RPROXY: is bound to type=%d ,inst=%d, socket=%d\n", server_type, SERVER_INST, rlisten_sd);
	
// Turn 'rproxy_sd' to a listening socket.
	if (0 != listen(rlisten_sd, 0)) {
		ERROR_EXIT(errno);
	}

    return(OK);
   
}

/* pr_receive_payloadr: receives the header from remote sender */
int pr_receive_payload(int payload_size) 
{
    int n, len, received = 0;
    char *p_ptr;

   	SVRDEBUG("payload_size=%d\n",payload_size);
   	p_ptr = (char*) px_desc.px_payload;
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
   	p_ptr = (char*) px_desc.px_header;
	total = sizeof(proxy_hdr_t);
   	len = sizeof(struct sockaddr_in);
   	while ((n = recv(rconn_sd, p_ptr, (total-received), 0 )) > 0) {
        received = received + n;
		SVRDEBUG("RPROXY: n:%d | received:%d | HEADER_SIZE:%d\n", n,received,sizeof(proxy_hdr_t));
        if (received >= sizeof(proxy_hdr_t)) {  
			SVRDEBUG("RPROXY: " CMD_FORMAT,CMD_FIELDS(px_desc.px_header));
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
    int rcode, payload_size, raw_len;
//	proc_usr_t kproc, *kp_ptr;
//	slot_t slot, *s_ptr;
	message *m_ptr;

	SVRDEBUG("RPROXY\n");
	

    do {
		bzero(px_desc.px_header, sizeof(proxy_hdr_t));
		SVRDEBUG("RPROXY: About to receive header\n");
    	rcode = pr_receive_header();
    	if (rcode != 0) ERROR_RETURN(rcode);
		SVRDEBUG("RPROXY:" CMD_FORMAT,CMD_FIELDS(px_desc.px_header));
   	}while(px_desc.px_header->c_cmd == CMD_NONE);
    
	/* now we have a proxy header in the buffer. Cast it.*/
    payload_size = px_desc.px_header->c_len;
    
    /* payload could be zero */
    if(payload_size != 0) {
        bzero(px_desc.px_payload, payload_size);
        rcode = pr_receive_payload(payload_size);
        if (rcode != 0){
			SVRDEBUG("RPROXY: No payload to receive.\n");
		}
    }else{
		switch(px_desc.px_header->c_cmd){
			case CMD_COPYIN_DATA:
			case CMD_COPYOUT_DATA:
				SVRDEBUG("RPROXY: "VCOPY_FORMAT, VCOPY_FIELDS(px_desc.px_header)); 
				if( px_desc.px_header->c_len > 0){
					rcode = pr_receive_payload(px_desc.px_header->c_len);
					if (rcode != 0){
						fprintf(stderr, "RPROXY: No payload to receive.\n");
						ERROR_EXIT(rcode);
					}
					SVRDEBUG("RPROXY: " CMD_XFORMAT, CMD_XFIELDS(px_desc.px_header));
					if(px_desc.px_header->c_flags == CMD_LZ4_FLAG){
						raw_len = decompress_payload(&px_desc); 
						if(raw_len < 0)
							ERROR_EXIT(raw_len);
						if( raw_len != px_desc.px_header->c_u.cu_vcopy.v_bytes){
							fprintf(stderr,"raw_len=%d " VCOPY_FORMAT, raw_len, VCOPY_FIELDS(px_desc.px_header));
							ERROR_EXIT(EMOLBADVALUE);
						}
						px_desc.px_header->c_len = raw_len;
						rcode = mnx_put2lcl(px_desc.px_header, px_desc.px_payload);
					}else{ /* the payload has not been decompressed */
						rcode = mnx_put2lcl(px_desc.px_header, px_desc.px_comp_pl);		
					}
					if( rcode) ERROR_RETURN(rcode);	
					return(rcode); 
				}else{
					raw_len = 0;
				}
				break;				
			case CMD_SEND_MSG:
			case CMD_SNDREC_MSG:
			case CMD_REPLY_MSG:
				m_ptr = &px_desc.px_header->c_u.cu_msg;
				SVRDEBUG("RPROXY: " MSG1_FORMAT,  MSG1_FIELDS(m_ptr));
				break;
				break;
			default:
				break;
		}
	}
	
	SVRDEBUG("RPROXY: put2lcl\n");
	rcode = mnx_put2lcl(px_desc.px_header, px_desc.px_payload);
#ifdef RMT_CLIENT_BIND	
	if( rcode == OK) return (OK);
	/******************** REMOTE CLIENT BINDING ************************************/
	/* rcode: the result of the las mnx_put2lcl 	*/
	/* ret: the result of the following operations	*/
	SVRDEBUG("RPROXY: REMOTE CLIENT BINDING\n");
	switch(rcode){
		case EMOLNOTBIND:	/* the slot is free */	
		case EMOLENDPOINT:	/* local node registers other endpoint using the slot */
		case EMOLNONODE:	/* local node register other node for this endpoint   */
			break;
		default:
			ERROR_RETURN(rcode);
	} 
	
	/* Try to bind the PROXY to the M3-IPC kernel with  "PROXY" endpoint number */
	retry = BIND_RETRIES;
	do {
		ret = mnx_bind(px_desc.px_header->c_dcid, PROXY);
		if(ret < PROXY) {
			ERROR_PRINT(ret);
			ERROR_RETURN(rcode); /* !!!! return the original return code !!!! */
		}
		if( ret != PROXY) {
			retry--;
			usleep(RETRY_US);  /* [0,2000000] */
		}
	}while(ret != PROXY && retry > 0);
	if ( retry == 0) {
		SVRDEBUG("RPROXY: can bind after %d retries\n",BIND_RETRIES);
		ERROR_RETURN(rcode); /* !!!! return the original return code !!!! */
	}
	
    s_ptr = &slot;
	ret = sys_getslot(&s_ptr, px_desc.px_header->c_src);
	SVRDEBUG("RPROXY: " SLOTS_FORMAT, SLOTS_FIELDS(s_ptr));
	
	/* check if the source node match the slot owner */
	if( s_ptr->s_owner !=  px_desc.px_header->c_snode){
		fprintf(stderr,"RPROXY:endpoint %d source node %d don't match slot owner %d\n"
				,px_desc.px_header->c_src, s_ptr->s_owner, px_desc.px_header->c_snode );
		mnx_unbind( px_desc.px_header->c_dcid, PROXY);	
		ERROR_RETURN(rcode); /* !!!! return the original return code !!!! */		
	}
	
	if(rcode == EMOLNONODE || rcode == EMOLENDPOINT){
		/* free the slot first */
		ret = sys_exit(px_desc.px_header->c_src);
		if(ret) 
			ERROR_PRINT(ret);
	}
	
	/* bind the remote process */
	ret = sys_bindrproc(px_desc.px_header->c_src, px_desc.px_header->c_snode);
	if(ret != px_desc.px_header->c_src) 
		ERROR_PRINT(ret);
	mnx_unbind( px_desc.px_header->c_dcid, PROXY);	
    if(ret) ERROR_RETURN(rcode); /* !!!! return the original return code !!!! */

	kp_ptr = &kproc;
	sys_getproc(kp_ptr,px_desc.px_header->c_src);
	SVRDEBUG("RPROXY: " PROC_USR_FORMAT , PROC_USR_FIELDS(kp_ptr));

	if( rcode ){
		ret = mnx_put2lcl(px_desc.px_header, px_desc.px_payload);
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
	int sender_addrlen;
//    char ip4[INET_ADDRSTRLEN];  // space to hold the IPv4 string
    struct sockaddr_tipc sa;
    int rcode;

	init_decompression(&px_desc);

    while (1){
		do {
			sender_addrlen = sizeof(sa);
			SVRDEBUG("RPROXY: Waiting for connection.\n");
    		rconn_sd = accept(rlisten_sd, (struct sockaddr *) &sa, &sender_addrlen);
    		if(rconn_sd < 0) SYSERR(errno);
		}while(rconn_sd < 0);

//		SVRDEBUG("RPROXY: Remote sender connected from [%d.%d.%d] on sd [%d]. Getting remote command.\n",
//			tipc_zone(sa.addr), tipc_cluster(sa.addr), tipc_node(sa.addr), rconn_sd);
		
		rcode = mnx_proxy_conn(px.px_id, CONNECT_RPROXY);

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
				if( rcode == EMOLNOTCONN 
				 || rcode == EMOLCALLDENIED
				 || rcode == EMOLBADREQUEST) break;
			}	
		}while(1);

		rcode = mnx_proxy_conn(px.px_id, DISCONNECT_RPROXY);
		close(rconn_sd);
   	}
    
    /* never reached */
}

/* pr_init: creates socket */
void pr_init(void) 
{
    int rcode;

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
	
	posix_memalign( (void**) &px_desc.px_header, getpagesize(), (sizeof(proxy_hdr_t)));
	if (px_desc.px_header== NULL) {
    		perror(" px_desc.px_header posix_memalign");
    		exit(1);
  	}

	posix_memalign( (void**) &px_desc.px_payload, getpagesize(), (sizeof(proxy_payload_t)));
	if (px_desc.px_payload== NULL) {
    		perror(" px_desc.px_payload posix_memalign");
    		exit(1);
  	}

	posix_memalign( (void**) &px_desc.px_comp_pl, getpagesize(), (sizeof(proxy_payload_t)));
	if (px_desc.px_comp_pl== NULL) {
    		perror(" px_desc.px_comp_pl posix_memalign");
    		exit(1);
  	}
		
	posix_memalign( (void**) &px_desc.px_pseudo, getpagesize(), (sizeof(proxy_hdr_t)));
	if (px_desc.px_pseudo== NULL) {
    		perror(" px_desc.px_pseudo posix_memalign");
    		exit(1);
  	}
	
	SVRDEBUG("px_desc.px_header=%p px_desc.px_payload=%p diff=%d\n", 
			px_desc.px_header, px_desc.px_payload, ((char*) px_desc.px_payload - (char*) px_desc.px_header));
	   
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
	SVRDEBUG("SPROXY: send header=%d \n", bytesleft);

    p_ptr = (char *) px_desc.px_header;
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
int  ps_send_payload(char *pl_ptr) 
{
    int sent = 0;        // how many bytes we've sent
    int bytesleft;
    int n, total;
	// char *p_ptr;

	if( px_desc.px_header->c_len <= 0) ERROR_EXIT(EMOLINVAL);
	
	bytesleft =  px_desc.px_header->c_len; // how many bytes we have left to send
	total = bytesleft;
	SVRDEBUG("SPROXY: send header=%d \n", bytesleft);

    while(sent < total) {
        n = send(sproxy_sd, pl_ptr, bytesleft, 0);
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
		pl_ptr += n; 
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
	int rcode, comp_len;
	proxy_hdr_t *h_ptr;

	h_ptr = px_desc.px_header;
	SVRDEBUG("SPROXY:" CMD_FORMAT,CMD_FIELDS(h_ptr));
	
	if( (h_ptr->c_cmd == CMD_COPYIN_DATA) || 
		(h_ptr->c_cmd == CMD_COPYOUT_DATA)){
		if( h_ptr->c_len == 0)	
			ERROR_RETURN(EMOLPACKSIZE);

		/* compress here */
		comp_len = compress_payload(&px_desc);
		SVRDEBUG("SPROXY: c_len=%d comp_len=%d\n", h_ptr->c_len, comp_len);
		if(comp_len < h_ptr->c_len){
			/* change command header  */
			h_ptr->c_len = comp_len; 
			h_ptr->c_flags = CMD_LZ4_FLAG; 
		}else{
			h_ptr->c_flags = 0; 
		} 
		SVRDEBUG(CMD_XFORMAT, CMD_XFIELDS(h_ptr));		
	}else if( h_ptr->c_len != 0){	
		ERROR_RETURN(EMOLPACKSIZE);
	}else{
		h_ptr->c_flags = 0; 
	}
	/* send the header */
	rcode =  ps_send_header();
	if ( rcode != OK)  ERROR_RETURN(rcode);

	if( h_ptr->c_len > 0) {
		SVRDEBUG("SPROXY: send payload len=%d\n", h_ptr->c_len );
		if( h_ptr->c_flags == CMD_LZ4_FLAG) {			
			rcode =  ps_send_payload((char*)px_desc.px_comp_pl);
		}else{
			rcode =  ps_send_payload((char*)px_desc.px_payload);
		}
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
    int sm_flag;
    int rcode;
    message *m_ptr;
    int pid, ret;
   	//char *ptr; 

    pid = getpid();
	init_compression(&px_desc);

    while(1) {
		SVRDEBUG("SPROXY %d: Waiting a message\n", pid);
        
		ret = mnx_get2rmt(px_desc.px_header, px_desc.px_payload);       
      
		if( ret != OK) {
			switch(ret) {
				case EMOLTIMEDOUT:
					SVRDEBUG("SPROXY: Sending HELLO \n");
					px_desc.px_header->c_cmd = CMD_NONE;
					px_desc.px_header->c_len = 0;
					px_desc.px_header->c_rcode = 0;
					break;
				case EMOLNOTCONN:
					return(EMOLNOTCONN);
				default:
					SVRDEBUG("ERROR  mnx_get2rmt %d\n", ret);
					continue;
			}
		}

		SVRDEBUG("SPROXY: %d "HDR_FORMAT,pid, HDR_FIELDS(px_desc.px_header)); 
		sm_flag = TRUE;
		switch(px_desc.px_header->c_cmd){
			case CMD_SEND_MSG: /* reply for REMOTE CLIENT BINDING*/	
				ret = 0;
				do {
					if(px_desc.px_header->c_dcid	!= px_desc.px_header->c_dcid) {ret=1;break;}	
					if(px_desc.px_header->c_src	!= SYSTASK(local_nodeid)) {ret=2;break;}	
					if(px_desc.px_header->c_dst	!= PM_PROC_NR ) {ret=3;break;}	
					if(px_desc.px_header->c_snode	!= local_nodeid ) {ret=4;break;}	
					if(px_desc.px_header->c_dnode	!= px_desc.px_header->c_snode ) {ret=5;break;}
					m_ptr = &px_desc.px_header->c_u.cu_msg;
					SVRDEBUG("SPROXY: " MSG1_FORMAT,  MSG1_FIELDS(m_ptr))
					if(px_desc.px_header->c_u.cu_msg.m_source != SYSTASK(local_nodeid)) {ret=6;break;}	
					if((int) px_desc.px_header->c_u.cu_msg.M1_OPER != RMT_BIND) {ret=7;break;}	
				}while(0);
				if( ret == 0 ){
					SVRDEBUG("SPROXY: reply for REMOTE CLIENT BINDING - Discard it ret=%d\n",ret);
					sm_flag = FALSE;
				}else{
					SVRDEBUG("SPROXY: CMD_SEND_MSG dont match REMOTE CLIENT BINDING ret=%d\n",ret);					
				}
				break;	
			case CMD_SNDREC_MSG:
			case CMD_REPLY_MSG:
				m_ptr = &px_desc.px_header->c_u.cu_msg;
				SVRDEBUG("SPROXY: " MSG1_FORMAT,  MSG1_FIELDS(m_ptr));
				break;
			case CMD_COPYIN_DATA:
			case CMD_COPYOUT_DATA:
				SVRDEBUG("SPROXY: "VCOPY_FORMAT, VCOPY_FIELDS(px_desc.px_header)); 
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
				px_desc.px_msg_ok++;
			} else {
				px_desc.px_msg_fail++;
			}	
		}
    }
    /* never reached */
	stop_compression(&px_desc);
    exit(1);
}

void wait_for_server(__u32 name_type, __u32 name_instance, int timeout)
{
	struct sockaddr_tipc topsrv;
	struct tipc_subscr subscr;
	struct tipc_event event;

	int sd = socket(AF_TIPC, SOCK_SEQPACKET, 0);

	memset(&topsrv, 0, sizeof(topsrv));
	topsrv.family = AF_TIPC;
	topsrv.addrtype = TIPC_ADDR_NAME;
	topsrv.addr.name.name.type = TIPC_TOP_SRV;
	topsrv.addr.name.name.instance = TIPC_TOP_SRV;

	/* Connect to topology server */

	if (0 > connect(sd, (struct sockaddr *)&topsrv, sizeof(topsrv))) {
		perror("Client: failed to connect to topology server");
		exit(1);
	}

	subscr.seq.type = htonl(name_type);
	subscr.seq.lower = htonl(name_instance);
	subscr.seq.upper = htonl(name_instance);
	subscr.timeout = htonl(timeout);
	subscr.filter = htonl(TIPC_SUB_SERVICE);

	do {
		if (send(sd, &subscr, sizeof(subscr), 0) != sizeof(subscr)) {
			perror("Client: failed to send subscription");
			exit(1);
		}
		/* Now wait for the subscription to fire */

		if (recv(sd, &event, sizeof(event), 0) != sizeof(event)) {
			perror("Client: failed to receive event");
			exit(1);
		}
		if (event.event == htonl(TIPC_PUBLISHED)) {
			close(sd);
			return;
		}
		SVRDEBUG("Client: server {%u,%u} not published within %u [s]\n",
	       name_type, name_instance, timeout/1000);	     
	} while(1);
	
}

/* ps_connect_to_remote: connects to the remote receiver */
int ps_connect_to_remote(void) 
{
    int server_type, rcode;
    //char rmt_ipaddr[INET_ADDRSTRLEN+1];
	
	//int rec_num;
	//int rec_size;
	//int tot_size;
	//int sent_size;
	//int msg_size;
	
	server_type = (BASE_TYPE + local_nodeid);
	wait_for_server(server_type, SERVER_INST, 10000);

	sproxy_sd = socket(AF_TIPC, SOCK_STREAM, 0);

	rmtserver_addr.family = AF_TIPC;
	rmtserver_addr.addrtype = TIPC_ADDR_NAME;
	rmtserver_addr.addr.name.name.type = server_type;
	rmtserver_addr.addr.name.name.instance = SERVER_INST;
	rmtserver_addr.addr.name.domain = 0;

 	rcode = connect(sproxy_sd, (struct sockaddr *) &rmtserver_addr, sizeof(rmtserver_addr));
    if (rcode != 0) ERROR_RETURN(errno);
    return(OK);
}

/* 
 * ps_init: creates sender socket, the connect to remote and
 * start sending messages to remote 
 */
void  ps_init(void) 
{
    int rcode = 0;
	//char *p_buffer;

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
		
	posix_memalign( (void**) &px_desc.px_header, getpagesize(), (sizeof(proxy_hdr_t)));
	if (px_desc.px_header== NULL) {
    		perror("px_desc.px_header posix_memalign");
    		exit(1);
  	}

	posix_memalign( (void**) &px_desc.px_payload, getpagesize(), (sizeof(proxy_payload_t)));
	if (px_desc.px_payload== NULL) {
    		perror("px_desc.px_payload posix_memalign");
    		exit(1);
  	}
	
    // Create server socket.
    if ( (sproxy_sd = socket(AF_TIPC, SOCK_STREAM, 0)) < 0){
       	ERROR_EXIT(errno)
    }

    /* try to connect many times */
	while(1) {
		do {
			if (rcode < 0){
				SVRDEBUG("SPROXY: Could not connect to %d"
                    	" Sleeping for a while...\n", px.px_id);
			}
			sleep(4);
			rcode = ps_connect_to_remote();
		} while (rcode != 0);
		
		rcode = mnx_proxy_conn(px.px_id, CONNECT_SPROXY);
		if(rcode) ERROR_EXIT(rcode);
	
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
int  main ( int argc, char *argv[] )
{
	int spid, rpid, pid, status;
	int ret;
	dvs_usr_t *d_ptr;    

	if (argc != 3) {
		fprintf(stderr,"Usage: %s <px_name> <px_id> \n", argv[0]);
		exit(0);
	}

	if( (BLOCK_16K << lz4_preferences.frameInfo.blockSizeID) < MAXCOPYBUF)  {
	fprintf(stderr, "MAXCOPYBUF(%d) must be greater than (BLOCK_16K <<"
		"lz4_preferences.frameInfo.blockSizeID)(%d)\n",MAXCOPYBUF,
		(BLOCK_16K << lz4_preferences.frameInfo.blockSizeID));
	exit(1);
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

	/* creates SENDER and RECEIVER Proxies as children */
	if ( (spid = fork()) == 0) ps_init();
	if ( (rpid = fork()) == 0) pr_init();

	/* register the proxies */
	ret = mnx_proxies_bind(px.px_name, px.px_id, spid, rpid, MAXCOPYBUF);
	if( ret < 0) ERROR_EXIT(ret);
	
	px_ptr = &px;
	SVRDEBUG(PX_USR_FORMAT , PX_USR_FIELDS(px_ptr));
	SVRDEBUG("binded to (%d,%d)\n", spid, rpid);

	ret= mnx_node_up(px.px_name, px.px_id, px.px_id);	
	
   	wait(&status);
    wait(&status);
    exit(0);
}

