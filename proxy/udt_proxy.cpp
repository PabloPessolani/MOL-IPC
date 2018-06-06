/********************************************************/
/* 		UDT PROXIES				*/
/********************************************************/

#include "proxy.h"
#include "debug.h"
#include "macros.h"
#include "stdlib.h"

#define HEADER_SIZE sizeof(proxy_hdr_t)
#define BASE_PORT      3000

#define RETRY_US		2000 /* Microseconds */
#define BIND_RETRIES	3
            
//agregamos
#ifndef WIN32
   #include <arpa/inet.h>
   #include <netdb.h>
#else
   #include <winsock2.h>
   #include <ws2tcpip.h>
#endif

#include <fstream>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <string>
#include "../../../udt/udt4/src/udt.h"

			
int local_nodeid;

proxy_hdr_t 	*p_header;
proxy_payload_t *p_payload;

int rmsg_ok = 0;
int rmsg_fail = 0;
int smsg_ok = 0;
int smsg_fail = 0; 
dvs_usr_t dvs;   
struct hostent *rmthost;
proxies_usr_t px, *px_ptr;

struct sockaddr_in rmtclient_addr, rmtserver_addr;
int    rproxy_sd;
int    sproxy_sd;

UDTSOCKET rproxy; //es un socket descriptor cuando se crea el receiver socket
UDTSOCKET recver; //es un socket descriptor que devuelve al aceptar una conexion en el receiver

UDTSOCKET sproxy; //es un socket descriptor cuando se crea el sender socket
struct addrinfo shints, *local, *peer;

struct UDTUpDown{
   UDTUpDown()
   {
      // use this function to initialize the UDT library
      UDT::startup();
   }
   ~UDTUpDown()
   {
      // use this function to release the UDT library
      UDT::cleanup();
   }
};



/*----------------------------------------------*/
/*      PROXY RECEIVER FUNCTIONS               */
/*----------------------------------------------*/
                
/* pr_setup_connection: bind and setup a listening socket 
   This socket is not accepting connections yet after
   end of this call.
 */                
int pr_setup_connection(void) 
{
    short int port_no;
	static	char service[NI_MAXSERV];

    port_no = (BASE_PORT+px.px_id);
	SVRDEBUG("RPROXY: for node %s running at port=%d\n", px.px_name, port_no);

    // Creamos server socket UDT.
	addrinfo rhints, *res;

	memset(&rhints, 0, sizeof(struct addrinfo));
	rhints.ai_flags = AI_PASSIVE;
	rhints.ai_family = AF_INET;
	rhints.ai_socktype = SOCK_STREAM;
	
	std::sprintf(service, "%d", port_no);
    SVRDEBUG("service=%s\n",service);

    if (0 != getaddrinfo(NULL, service, &rhints, &res)) {
      SVRDEBUG("illegal port number or port is busy.\n");
      ERROR_EXIT(errno);
    }

    SVRDEBUG("UDT::socket\n");	
    rproxy = UDT::socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	// Bind de UDT
    SVRDEBUG("Bind de UDT\n");	
	if (UDT::ERROR == UDT::bind(rproxy, res->ai_addr, res->ai_addrlen)) {
      SVRDEBUG("bind: %s\n",UDT::getlasterror().getErrorMessage());
      ERROR_EXIT(errno);
	}

    SVRDEBUG("freeaddrinfo\n");	
	freeaddrinfo(res);
   
    SVRDEBUG("RPROXY: is bound to port=%d service=%s\n", port_no,service);
    //se pone a escuchar el socket rproxy
	if (UDT::ERROR == UDT::listen(rproxy, 10))  {
		SVRDEBUG("listen: %s\n",UDT::getlasterror().getErrorMessage());
		ERROR_EXIT(-1);
	}
	
    return(OK);
   
}

/* pr_receive_payload: receives the header from remote sender */
int pr_receive_payload(int payload_size) 
{
    int n, len, received = 0;
    char *p_ptr;

SVRDEBUG("payload_size=%d\n",payload_size);
   	p_ptr = (char*) p_payload;
   	len = sizeof(struct sockaddr_in);
   	while (UDT::ERROR != (n = UDT::recv(recver, p_ptr, (payload_size-received), 0 ))) {
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
    int n, len, total;
	unsigned int received = 0;
    char *p_ptr;
	
	struct sockaddr* name; //name es el socket a imprimir al final
	socklen_t* namelen;
	
	/*getsockname(recver,name,namelen);
	SVRDEBUG("socket=%s\n", name->sa_data);*/
	SVRDEBUG("Receive header\n");
	
    p_ptr = (char*) p_header;
    total = sizeof(proxy_hdr_t);
    len = sizeof(struct sockaddr_in);
    while (UDT::ERROR != (n = UDT::recv(recver, p_ptr, (total-received), 0 ))) {
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
    
    //ver si son iguales los errores
    SVRDEBUG("recv: %s\n",UDT::getlasterror().getErrorMessage());

    ERROR_RETURN(errno);
}

/* pr_process_message: receives header and payload if any. Then deliver the 
 * message to local */
int pr_process_message(void) {
    int rcode, payload_size;
	message *m_ptr;

    do {
		bzero(p_header, sizeof(proxy_hdr_t));
		SVRDEBUG("RPROXY: About to receive header\n");
    	rcode = pr_receive_header();
    	if (rcode != 0) 
            ERROR_RETURN(rcode);
		SVRDEBUG("RPROXY:" CMD_FORMAT,CMD_FIELDS(p_header));
   	}while(p_header->c_cmd == CMD_NONE);
    
	/* now we have a proxy header in the buffer. Cast it.*/
    payload_size = p_header->c_len;
    
    /* payload could be zero */
    if(payload_size != 0) {
        bzero(p_payload, payload_size);
        rcode = pr_receive_payload(payload_size);
        if (rcode != 0){
			SVRDEBUG("RPROXY: No payload to receive.\n");
		}
    }else{
		switch(p_header->c_cmd){
			case CMD_SEND_MSG:
			case CMD_SNDREC_MSG:
			case CMD_REPLY_MSG:
				m_ptr = &p_header->c_u.cu_msg;
				SVRDEBUG("RPROXY: " MSG1_FORMAT,  MSG1_FIELDS(m_ptr));
				break;
			case CMD_COPYIN_DATA:
			case CMD_COPYOUT_DATA:
				SVRDEBUG("RPROXY: "VCOPY_FORMAT, VCOPY_FIELDS(p_header)); 
				break;
			default:
				break;
		}
	}
	
	SVRDEBUG("RPROXY: put2lcl\n");
	rcode = CPP_put2lcl(p_header, p_payload);
#ifdef RMT_CLIENT_BIND	
	if( rcode == OK) return (OK);
	/******************** REMOTE CLIENT BINDING ************************************/
	/* rcode: the result of the las CPP_put2lcl 	*/
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
		ret = CPP_bind(p_header->c_dcid, PROXY);
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
	ret = sys_getslot(&s_ptr, p_header->c_src);
	SVRDEBUG("RPROXY: " SLOTS_FORMAT, SLOTS_FIELDS(s_ptr));
	
	/* check if the source node match the slot owner */
	if( s_ptr->s_owner !=  p_header->c_snode){
		fprintf(stderr,"RPROXY:endpoint %d source node %d don't match slot owner %d\n"
				,p_header->c_src, s_ptr->s_owner, p_header->c_snode );
		CPP_unbind( p_header->c_dcid, PROXY);	
		ERROR_RETURN(rcode); /* !!!! return the original return code !!!! */		
	}
	
	if(rcode == EMOLNONODE || rcode == EMOLENDPOINT){
		/* free the slot first */
		ret = sys_exit(p_header->c_src);
		if(ret) 
			ERROR_PRINT(ret);
	}
	
	/* bind the remote process */
	ret = sys_bindrproc(p_header->c_src, p_header->c_snode);
	if(ret != p_header->c_src) 
		ERROR_PRINT(ret);
	CPP_unbind( p_header->c_dcid, PROXY);	
    if(ret) ERROR_RETURN(rcode); /* !!!! return the original return code !!!! */

	kp_ptr = &kproc;
	sys_getproc(kp_ptr,p_header->c_src);
	SVRDEBUG("RPROXY: " PROC_USR_FORMAT , PROC_USR_FIELDS(kp_ptr));

	if( rcode ){
		ret = CPP_put2lcl(p_header, p_payload);
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
    //int sender_addrlen;
    //char ip4[INET_ADDRSTRLEN];  // space to hold the IPv4 string
    //struct sockaddr_in sa;
    int rcode;
	
    sockaddr_storage clientaddr;
    int addrlen;
	

    while (1){
        addrlen = sizeof(clientaddr);

        SVRDEBUG("RPROXY: Waiting for connection.\n");
        if(UDT::INVALID_SOCK == (recver = UDT::accept(rproxy, (sockaddr*)&clientaddr, &addrlen))){
            SVRDEBUG("accept: %s\n",UDT::getlasterror().getErrorMessage());
            SYSERR(errno);
        }	
        //creemos que con UDT el do while no seria necesario

        char clienthost[NI_MAXHOST];
        char clientservice[NI_MAXSERV];
        getnameinfo((sockaddr *)&clientaddr, addrlen, clienthost, sizeof(clienthost), clientservice, sizeof(clientservice), NI_NUMERICHOST|NI_NUMERICSERV);
        SVRDEBUG("new connection: %s : %s\n",clienthost, clientservice);
        //cout << "new connection: " << clienthost << ":" << clientservice << endl;
        //cout << "RPROXY: Remote sender " << clienthost << "connected on sd" << clientservice << "Getting remote command." endl;
        SVRDEBUG("RPOROXY: Remote sender %s connected on sd %s Getting remote command.\n",clienthost, clientservice);

        rcode = CPP_proxy_conn(px.px_id, CONNECT_RPROXY);

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
    int rcode;

	// inicializamos UDT library
	UDT::startup();
	
	SVRDEBUG("RPROXY: Initializing proxy receiver. PID: %d\n", getpid());
    
	// Automatically start up and clean up UDT module.
	UDTUpDown _udt_;
   
	do { 
		rcode = CPP_wait4bind_T(RETRY_US);
		SVRDEBUG("RPROXY: CPP_wait4bind_T  rcode=%d\n", rcode);
		if (rcode == EMOLTIMEDOUT) {
			SVRDEBUG("RPROXY: CPP_wait4bind_T TIMEOUT\n");
			continue ;
		}else if(rcode == NONE) { /* proxies have not endpoint */
			break;	
		} if( rcode < 0) 
			exit(EXIT_FAILURE);
	} while	(rcode < OK);
	
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
    
    if( pr_setup_connection() == OK) {
        do {
                rcode = CPP_proxy_conn(px.px_id, CONNECT_RPROXY);
                if(rcode == EMOLPROXYFREE) sleep(1);
        } while(rcode == EMOLPROXYFREE);
        
        if(rcode) 
            ERROR_EXIT(rcode);
	
      	pr_start_serving();
	
        rcode = CPP_proxy_conn(px.px_id, DISCONNECT_RPROXY);
        //cerramos el socket creado en pr_setup_connection
        UDT::close(rproxy);
		
        if(rcode) 
            ERROR_EXIT(rcode);		
		
    } 
    else {
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

    p_ptr = (char *) p_header;
    while(sent < total) {
       	n = UDT::send(sproxy, p_ptr, bytesleft, 0);
        if (UDT::ERROR == n) {
			SVRDEBUG("send: %s \n",UDT::getlasterror().getErrorMessage());
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
	struct sockaddr* name; //name es el socket a imprimir al final
	socklen_t* namelen;
	
    int sent = 0;        // how many bytes we've sent
    int bytesleft;
    int n, total;
	char *p_ptr;

	if( p_header->c_len <= 0) ERROR_EXIT(EMOLINVAL);
	
	bytesleft =  p_header->c_len; // how many bytes we have left to send
	total = bytesleft;
	SVRDEBUG("SPROXY: send header=%d \n", bytesleft);

    p_ptr = (char *) p_payload;
    while(sent < total) {
       	n = UDT::send(sproxy, p_ptr, bytesleft, 0);
         if (UDT::ERROR == n) {
			SVRDEBUG("send: %s \n",UDT::getlasterror().getErrorMessage());
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
	
	/*getsockname(sproxy,name,namelen);
	SVRDEBUG("SPROXY: socket=%s sent payload=%d \n", name->sa_data, total);*/
	
	SVRDEBUG("Sent payload");
    return(OK);
}

/* 
 * ps_send_remote: send a message (header + payload if existing) 
 * to remote receiver
 */
int  ps_send_remote(void) 
{
	int rcode;

	SVRDEBUG("SPROXY:" CMD_FORMAT,CMD_FIELDS(p_header));

	/* send the header */
	rcode =  ps_send_header();
	if ( rcode != OK)  ERROR_RETURN(rcode);

	if( p_header->c_len > 0) {
		SVRDEBUG("SPROXY: send payload len=%d\n", p_header->c_len );
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
    int rcode;
    message *m_ptr;
    int pid, ret;

    pid = getpid();
	
    while(1) {
		SVRDEBUG("SPROXY %d: Waiting a message\n", pid);
        
		ret = CPP_get2rmt(p_header, p_payload);       
      
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
					SVRDEBUG("ERROR  CPP_get2rmt %d\n", ret);
					continue;
			}
		}

		SVRDEBUG("SPROXY: %d "HDR_FORMAT,pid, HDR_FIELDS(p_header)); 

		switch(p_header->c_cmd){
			case CMD_SEND_MSG:
			case CMD_SNDREC_MSG:
			case CMD_REPLY_MSG:
				m_ptr = &p_header->c_u.cu_msg;
				SVRDEBUG("SPROXY: " MSG1_FORMAT,  MSG1_FIELDS(m_ptr));
				break;
			case CMD_COPYIN_DATA:
			case CMD_COPYOUT_DATA:
				SVRDEBUG("SPROXY: "VCOPY_FORMAT, VCOPY_FIELDS(p_header)); 
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
    int port_no, rcode, i;
	static char rmt_ip[INET_ADDRSTRLEN+1];
    port_no = (BASE_PORT+local_nodeid);
	char rmt_port[NI_MAXSERV];
	SVRDEBUG("SPROXY: for node %s running at port=%d\n", px.px_name, port_no);    

	// Connect to the server client	
    /*usamos el peer y el shints en lugar de esto
	rmtserver_addr.sin_family = AF_INET;   
    rmtserver_addr.sin_port = htons(port_no);  */

    rmthost = gethostbyname(px.px_name);
	if( rmthost == NULL) ERROR_EXIT(h_errno);
	for( i =0; rmthost->h_addr_list[i] != NULL; i++) {
		SVRDEBUG("SPROXY: remote host address %i: %s\n", 
			i, inet_ntoa( *( struct in_addr*)(rmthost->h_addr_list[i])));
	}
	std::strncpy(rmt_ip, inet_ntoa( *( struct in_addr*)(rmthost->h_addr_list[0])), INET_ADDRSTRLEN );
	SVRDEBUG("SPROXY: inet_pton %s\n",rmt_ip);    

    if((inet_pton(AF_INET,rmt_ip, &rmtserver_addr.sin_addr)) <= 0)
    	ERROR_RETURN(errno);
	std::sprintf(rmt_port, "%d", port_no);
	
	SVRDEBUG("SPROXY: getaddrinfo %s:%s\n",rmt_ip, rmt_port);    
	if (0 != getaddrinfo(rmt_ip, rmt_port, &shints, &peer))  {
      SVRDEBUG("incorrect server/peer address. \n");
      return -1;
	}
	
	struct sockaddr_in *ipv4 = (struct sockaddr_in *)peer->ai_addr;
	
	//aca abajo manejamos punteros en el segundo parametro, puede fallar dijo tusam
	SVRDEBUG("SPROXY: inet_ntop ai_canonname=%s\n", peer->ai_canonname);    
    if( inet_ntop(AF_INET, &(ipv4->sin_addr),rmt_ip, INET_ADDRSTRLEN) == NULL){
    	ERROR_RETURN(errno);
	}
	SVRDEBUG("SPROXY: for node %s running at IP=%s\n", px.px_name, rmt_ip);  
	
	SVRDEBUG("SPROXY: UDT::connect\n");    
	rcode = UDT::connect(sproxy, peer->ai_addr, peer->ai_addrlen);
    if (rcode == UDT::ERROR) {
		SVRDEBUG("SPROXY: connect: %s\n", UDT::getlasterror().getErrorMessage());
		ERROR_RETURN(rcode);
	}
	
	SVRDEBUG("SPROXY: freeaddrinfo\n");    
	freeaddrinfo(peer);
    return(OK);
}

/* 
 * ps_init: creates sender socket, the connect to remote and
 * start sending messages to remote 
 */
void  ps_init(void) 
{
	static char lcl_ip[INET_ADDRSTRLEN+1];
	static char lcl_name[MAXPROXYNAME];
    int rcode = 0;

	// inicializamos UDT library
	UDT::startup();
	
	SVRDEBUG("SPROXY: Initializing on PID:%d\n", getpid());
   	UDTUpDown _udt_;

	do { 
		SVRDEBUG("entro al do");
		rcode = CPP_wait4bind_T(RETRY_US);
		SVRDEBUG("SPROXY: CPP_wait4bind_T  rcode=%d\n", rcode);
		if (rcode == EMOLTIMEDOUT) {
			SVRDEBUG("SPROXY: CPP_wait4bind_T TIMEOUT\n");
			continue ;
		}else if(rcode == NONE) { /* proxies have not endpoint */
			break;	
		} if( rcode < 0) 
			exit(EXIT_FAILURE);
	} while	(rcode < OK);
		
	posix_memalign( (void**) &p_header, getpagesize(), (sizeof(proxy_hdr_t)+ getpagesize()));
	if (p_header== NULL) {
    		perror("p_header posix_memalign");
    		exit(1);
  	}

	posix_memalign( (void**) &p_payload, getpagesize(), (sizeof(proxy_payload_t)+ getpagesize()));
	if (p_payload== NULL) {
    		perror("p_payload posix_memalign");
    		exit(1);
  	}
	
    // Create UDT server socket.
	memset(&shints, 0, sizeof(struct addrinfo));
	shints.ai_flags = AI_PASSIVE;
	shints.ai_family = AF_INET;
	shints.ai_socktype = SOCK_STREAM;
	
	std::sprintf(lcl_name, "node%d", local_nodeid);
	
    if (0 != getaddrinfo(lcl_name,  NULL, &shints, &local)) {
		ERROR_EXIT(errno);
	}
   
	sproxy = UDT::socket(local->ai_family, local->ai_socktype, local->ai_protocol);
	struct sockaddr_in *ipv4 = (struct sockaddr_in *)local->ai_addr;
	SVRDEBUG("SPROXY: lcl_name=%s lcl_ip=%s\n", 
		lcl_name,  inet_ntop(AF_INET, &(ipv4->sin_addr), lcl_ip, INET_ADDRSTRLEN));
	
    freeaddrinfo(local);

	  
    /* try to connect many times */
	while(1) {
		do {
			if (rcode != 0)
				SVRDEBUG("SPROXY: Could not connect to %d"
                    			" Sleeping for a while...\n", px.px_id);
			sleep(4);
			rcode = ps_connect_to_remote();
		} while (rcode != 0);
		
		rcode = CPP_proxy_conn(px.px_id, CONNECT_SPROXY);
		if(rcode) ERROR_EXIT(rcode);
	
		ps_start_serving();
	
		rcode = CPP_proxy_conn(px.px_id, DISCONNECT_SPROXY);
		//cerramos el socket creado en ps_connect_to_remote
		UDT::close(sproxy);
		
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



	strncpy(px.px_name,argv[1], MAXPROXYNAME);
	printf("UDT Proxy Pair name: %s\n",px.px_name);

	px.px_id = atoi(argv[2]);
	printf("Proxy Pair id: %d\n",px.px_id);

	local_nodeid = CPP_getdvsinfo(&dvs);
	d_ptr=&dvs;
	SVRDEBUG(dvs_USR_FORMAT,dvs_USR_FIELDS(d_ptr));

	pid = getpid();
	SVRDEBUG("MAIN: pid=%d local_nodeid=%d\n", pid, local_nodeid);

	/* creates SENDER and RECEIVER Proxies as children */
	if ( (spid = fork()) == 0) ps_init();
	if ( (rpid = fork()) == 0) pr_init();

	/* register the proxies MAXCOPYBUF */
	ret = CPP_proxies_bind(px.px_name, px.px_id, spid, rpid, getpagesize());
	if( ret < 0) ERROR_EXIT(ret);
	
	px_ptr = &px;
	SVRDEBUG(PX_USR_FORMAT , PX_USR_FIELDS(px_ptr));
	SVRDEBUG("binded to (%d,%d)\n", spid, rpid);

	ret= CPP_node_up(px.px_name, px.px_id, px.px_id);	
	if( ret < 0) ERROR_EXIT(ret);

	waitpid(spid, &status, 0);
	waitpid(rpid, &status, 0);
    exit(0);
	
	return(0);
}

