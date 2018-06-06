/********************************************************/
/* 		MSGQ PROXIES				*/
/********************************************************/

#define  MOL_USERSPACE	1

#include "raw_proxy.h"

#define HEADER_SIZE sizeof(proxy_hdr_t)
#define BASE_PORT      3000

#define RETRY_US		2000 /* Microseconds */
#define BIND_RETRIES	3

#define	IFNAME		"eth0"

#define MTX_LOCK(x) do{ \
		SVRDEBUG("MTX_LOCK %s \n", #x);\
		pthread_mutex_lock(&x);\
		}while(0)
			
#define MTX_UNLOCK(x) do{ \
		pthread_mutex_unlock(&x);\
		SVRDEBUG("MTX_UNLOCK %s \n", #x);\
		}while(0)	
			
#define COND_WAIT(x,y) do{ \
		SVRDEBUG("COND_WAIT ENTER %s %s\n", #x,#y );\
		pthread_cond_wait(&x, &y);\
		clock_gettime(CLOCK_REALTIME, &ts);\
		SVRDEBUG("COND_WAIT EXIT %s tv_sec=%ld\n", #x,ts.tv_sec);\
		}while(0)	

#define COND_WAIT_TO(r,x,y,t) do{ \
		SVRDEBUG("COND_WAIT_TO %s %s %s\n", #r,#x,#y );\
		clock_gettime(CLOCK_REALTIME, &ts);\
		SVRDEBUG("COND_WAIT_TO before tv_sec=%ld\n", ts.tv_sec);\
		ts.tv_sec += t;\
		r = pthread_cond_timedwait(&x, &y, &ts);\
		clock_gettime(CLOCK_REALTIME, &ts);\
		SVRDEBUG("COND_WAIT_TO after tv_sec=%ld\n", ts.tv_sec);\
		}while(0)
		
#define COND_SIGNAL(x) do{ \
		pthread_cond_signal(&x);\
		clock_gettime(CLOCK_REALTIME, &ts);\
		SVRDEBUG("COND_SIGNAL %s tv_sec=%ld\n", #x,ts.tv_sec);\
		}while(0)	

int raw_mtu;
int raw_lpid;
int raw_mode;
char raw_lcl[IFNAMSIZ+1];
char raw_rmt[IFNAMSIZ+1];
uint8_t lcl_mac[ETH_ALEN];
uint8_t rmt_mac[ETH_ALEN];
static char lcl_string[3*ETH_ALEN+1];
static char rmt_string[3*ETH_ALEN+1];
struct stat file_stat;
struct sockaddr_ll lcl_sockaddr;
struct sockaddr_ll rmt_sockaddr;
struct ifreq if_idx;
struct ifreq if_mac;
struct ifreq if_mtu;
struct arpreq rmt_areq;
char *path_ptr, *read_buf;
unsigned long hdr_plus_cmd;
struct hostent *rmt_he;
int local_nodeid;


msgq_buf_t *ack_msg_ptr;
struct msqid_ds mq_in_ds;
struct msqid_ds mq_out_ds;
pthread_mutex_t raw_mtx;

int rmsg_ok = 0;
int rmsg_fail = 0;
int smsg_ok = 0;
int smsg_fail = 0; 
DVS_usr_t DVS;   
proxies_usr_t px, *px_ptr;
proxy_hdr_t 	*p_pseudo;

int raw_fd;  

struct timeval tv;

eth_frame_t 	*td_send_sframe_ptr;
eth_frame_t 	*td_send_rframe_ptr;
eth_frame_t 	*td_recev_rframe_ptr;
eth_frame_t 	*td_recev_sframe_ptr;


unsigned long 	lcl_snd_seq;	// sequence number for the next frame to send  
unsigned long 	lcl_ack_seq;	// sequence number of the last receive frame  
unsigned long 	rmt_ack_seq;	// sequence number of the last acknowledged frame: it must be (lcl_snd_seq-1)  

void choose (char *progname){
	fprintf (stderr, "%s: you must choose Client(c) or Server(s) flag\n", progname);
	exit(1);
}

void bad_file (char *progname, char *filename){
	fprintf (stderr, "%s: bad file name %s\n", progname, filename);
	exit(1);
}

void DEBUG_hdrcmd(char *funct , eth_frame_t *f_ptr)
{
	eth_hdr_t *eth_hdr_ptr;
	cmd_t *cmd_ptr;

	eth_hdr_ptr= &f_ptr->fmt.hdr;
	SVRDEBUG("%s " ETHHDR_FORMAT,funct,ETHHDR_FIELDS(eth_hdr_ptr));
	cmd_ptr = &f_ptr->fmt.cmd;
	SVRDEBUG("%s " CMD_FORMAT,funct, CMD_FIELDS(cmd_ptr));
	SVRDEBUG("%s " CMD_XFORMAT,funct, CMD_XFIELDS(cmd_ptr));
}

int send_frame(eth_frame_t 	*frame_ptr, int sock_fd, int sframe_len)
{
	int sent_bytes;
	
	set_frame_hdr(frame_ptr);
	
	frame_ptr->fmt.cmd.c_snd_seq  = lcl_snd_seq;
	frame_ptr->fmt.cmd.c_ack_seq  = lcl_ack_seq;

	SVRDEBUG("SEND lcl_snd_seq=%ld lcl_ack_seq=%ld sframe_len=%d\n", 
		lcl_snd_seq, lcl_ack_seq, sframe_len);
	sent_bytes = sendto(sock_fd, &frame_ptr->raw, sframe_len, 0x00,
					(struct sockaddr*)&rmt_sockaddr, sizeof(rmt_sockaddr));
	DEBUG_hdrcmd(__FUNCTION__, frame_ptr);
	if( sent_bytes < 0) 
		ERROR_RETURN(-errno);
	return(sent_bytes);
}
	
int arp_table()
{
    FILE *arp_fd;
static    char header[ARP_BUFFER_LEN];
static    char ipAddr[ARP_BUFFER_LEN];
static	  char hwAddr[ARP_BUFFER_LEN];
static	  char device[ARP_BUFFER_LEN];

    int count = 0;	
	arp_fd = fopen(ARP_CACHE, "r");
    if (!arp_fd) ERROR_EXIT(-errno);

    /* Ignore the first line, which contains the header */
    if (!fgets(header, sizeof(header), arp_fd))
        ERROR_EXIT(-errno);


    while (3 == fscanf(arp_fd, ARP_LINE_FORMAT, ipAddr, hwAddr, device))
    {
        printf("%03d: Mac Address of [%s] on [%s] is \"%s\"\n",
                ++count, ipAddr, device, hwAddr);
    }
    fclose(arp_fd);
    return 0;
}

static char *ethernet_mactoa(struct sockaddr *addr) {
    static char buff[256];
    unsigned char *ptr = (unsigned char *) addr->sa_data;

    sprintf(buff, "%02X:%02X:%02X:%02X:%02X:%02X",
        (ptr[0] & 0xff), (ptr[1] & 0xff), (ptr[2] & 0xff),
        (ptr[3] & 0xff), (ptr[4] & 0xff), (ptr[5] & 0xff));

    return (buff);
}

int get_arp(char *dst_ip_str)
{
    int dgram_sock;
    struct sockaddr_in *sin;
    struct in_addr ipaddr;
	
    /* Get an internet domain socket. */
    if ((dgram_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
		ERROR_EXIT(-errno);
	
	
	if (inet_aton(dst_ip_str, &ipaddr) == 0) 
		ERROR_EXIT(-errno);
	
	  /* Make the ARP request. */
    memset(&rmt_areq, 0, sizeof(rmt_areq));
    sin = (struct sockaddr_in *) &rmt_areq.arp_pa;
    sin->sin_family = AF_INET;
	
	sin->sin_addr = ipaddr;
    sin = (struct sockaddr_in *) &rmt_areq.arp_ha;
    sin->sin_family = ARPHRD_ETHER;

    strncpy(rmt_areq.arp_dev, IFNAME , 15);
    if (ioctl(dgram_sock, SIOCGARP, (caddr_t) &rmt_areq) == -1) 
		ERROR_EXIT(-errno);

    printf("Destination IP:%s MAC:%s\n",
    inet_ntoa(((struct sockaddr_in *) &rmt_areq.arp_pa)->sin_addr),
    ethernet_mactoa(&rmt_areq.arp_ha));
	return(OK);
}
	
int raw_init(char *if_name, char *hname)
{	
	int i, raw_fd;
	
	rmt_he = gethostbyname(hname);	
	printf("rmt_name=%s rmt_ip=%s\n", rmt_he->h_name, 
		inet_ntoa( *( struct in_addr*)(rmt_he->h_addr)));
	arp_table();
	get_arp(inet_ntoa( *( struct in_addr*)(rmt_he->h_addr)));
	
	// get socket for RAW
	raw_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_M3IPC));
	if (raw_fd == -1) {
		SVRDEBUG("%s\n",strerror(-errno));
		ERROR_EXIT(-errno);
	}
	
	/* Get the index of the interface to send on */
	memset(&if_idx, 0, sizeof(struct ifreq));
	strncpy(if_idx.ifr_name, if_name, strlen(if_name));
	if (ioctl(raw_fd, SIOCGIFINDEX, &if_idx) < 0)
	    perror("SIOCGIFINDEX");
		
	/* Get SOURCE interface MAC address  */
	memset(&if_mac,0,sizeof(if_mac));
	strncpy(if_mac.ifr_name, if_name, strlen(if_name));
	if (ioctl(raw_fd,SIOCGIFHWADDR,&if_mac) == -1) {
		SVRDEBUG("%s\n",strerror(-errno));
		ERROR_EXIT(-errno);
	}

	memcpy(lcl_mac, &if_mac.ifr_hwaddr.sa_data, ETH_ALEN);
	for(i = 0; i < ETH_ALEN; i++){
		sprintf(&lcl_string[i*3], "%02X:", lcl_mac[i]);
	}
	lcl_string[(ETH_ALEN*3)-1] = 0;
	SVRDEBUG("%s lcl_mac %s\n",raw_lcl, lcl_string);

	memset(&lcl_sockaddr,0,sizeof(lcl_sockaddr));
	lcl_sockaddr.sll_family=PF_PACKET;
	lcl_sockaddr.sll_protocol=htons(ETH_M3IPC); 

	SVRDEBUG("protocol:%X  ETH_P_ALL=%X\n",lcl_sockaddr.sll_protocol, htons(ETH_P_ALL));
		
	/* Get DESTINATION  interface MAC address  */
	memcpy(rmt_mac, rmt_areq.arp_ha.sa_data, ETH_ALEN);
	for(i = 0; i < ETH_ALEN; i++){
		sprintf(&rmt_string[i*3], "%02X:", rmt_mac[i]);
	}
	rmt_string[(ETH_ALEN*3)-1] = 0;
	SVRDEBUG("%s rmt_mac %s\n",raw_rmt, rmt_string);
	
	lcl_sockaddr.sll_ifindex=if_idx.ifr_ifindex;
	if (bind(raw_fd,(struct sockaddr*)&lcl_sockaddr,sizeof(lcl_sockaddr))<0){
		SVRDEBUG("%s\n",strerror(-errno));
		ERROR_EXIT(-errno);
	}
	
	tv.tv_sec = RAW_RCV_TIMEOUT;
	if(setsockopt(raw_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0){
		SVRDEBUG("%s\n",strerror(-errno));
		ERROR_EXIT(-errno);
	}
	
	pthread_mutex_init(&raw_mtx, NULL);  
	
 	hdr_plus_cmd = ((unsigned long)&td_send_sframe_ptr->fmt.pay - (unsigned long)&td_send_sframe_ptr->fmt.hdr);
	SVRDEBUG("hdr_plus_cmd=%d\n",hdr_plus_cmd);

	lcl_snd_seq = 1;	// sequence # for the next frame to send
 	lcl_ack_seq = 0;	// sequence # of the last frame received
	rmt_ack_seq = 0;	// sequence # of the last frame acknoledged by remote node 
	return(raw_fd);
}


	
void fill_socket(void)
{
	int i;

	/*RAW communication*/
	rmt_sockaddr.sll_family   = AF_PACKET;	
	
	/*we don't use a protocoll above ethernet layer  ->just use anything here*/
	rmt_sockaddr.sll_protocol = htons(ETH_M3IPC);	

	/*index of the network device */
	rmt_sockaddr.sll_ifindex  = if_idx.ifr_ifindex;

	/*address length*/
	rmt_sockaddr.sll_halen    = ETH_ALEN;
	
	/*MAC - begin*/
	for(i = 0; i < ETH_ALEN; i++){
		rmt_sockaddr.sll_addr[i]  = rmt_mac[i];		
		lcl_sockaddr.sll_addr[i]  = lcl_mac[i];		
	}
	/*MAC - end*/
	rmt_sockaddr.sll_addr[ETH_ALEN+1]  = 0x00;/*not used*/
	rmt_sockaddr.sll_addr[ETH_ALEN+2]  = 0x00;/*not used*/
	lcl_sockaddr.sll_addr[ETH_ALEN+1]  = 0x00;/*not used*/
	lcl_sockaddr.sll_addr[ETH_ALEN+2]  = 0x00;/*not used*/
}


	


void check_msgtype(int msgtype)
{
	int cmd;

  	SVRDEBUG("msgtype=%X \n", msgtype); 
	
	switch( td_send_sframe_ptr->fmt.cmd.c_cmd){
		case CMD_COPYIN_DATA:
			// header + payload
			assert(msgtype & (RAW_MQ_HDR | RAW_MQ_PAY));
			break;
		case CMD_COPYOUT_DATA:
			// header + payload
			assert(msgtype & (RAW_MQ_HDR | RAW_MQ_PAY | RAW_MQ_ACK));
			break;
		case CMD_FRAME_ACK:
			assert(msgtype == RAW_MQ_ACK);
			assert(td_send_sframe_ptr->fmt.cmd.c_cmd == CMD_FRAME_ACK);
			break;
		default:
			assert(msgtype & RAW_MQ_HDR);
			if( td_send_sframe_ptr->fmt.cmd.c_cmd & CMD_ACKNOWLEDGE){
				assert(msgtype & RAW_MQ_ACK);
				cmd = (td_send_sframe_ptr->fmt.cmd.c_cmd & ~(CMD_ACKNOWLEDGE));	
			} else {
				cmd = td_send_sframe_ptr->fmt.cmd.c_cmd;
			}
			assert( (cmd >= 0) && (cmd <= CMD_BATCHED_CMD));
			break;
	} 
}
	
void set_frame_hdr(eth_frame_t 	*frame_ptr)
{	
	ether_type_t proto;

  	SVRDEBUG("\n"); 

	// 	Destination MAC	: 
	memcpy((u8_t*)&frame_ptr->fmt.hdr.eh_dst, (u8_t*)&rmt_mac[0], ETH_ALEN);

	// 	Source MAC 
	memcpy((u8_t*)&frame_ptr->fmt.hdr.eh_src, (u8_t*)&lcl_mac[0], ETH_ALEN);
	
	//   Protocolo Field
	proto = (ether_type_t) htons(ETH_M3IPC); 
	memcpy((u8_t*)&frame_ptr->fmt.hdr.eh_proto, (u8_t*)&proto , sizeof(ether_type_t));
}

int sts_to_rproxy(int mt, int code)
{
	int rcode;
	
	SVRDEBUG("mt=%X code=%d\n", mt, code );
	//in_msg_ptr->mtype = mt; 
	//in_msg_ptr->m3.cmd.c_rcode = code; 	
	/*rcode = msgsnd(raw_mq_in , in_msg_ptr, sizeof(cmd_t), 0); 
	if( rcode < 0) {
		SVRDEBUG("msgsnd errno=%d\n",errno);
		ERROR_EXIT(-errno);
	}*/
}
	



/*----------------------------------------------*/
/*      PROXY RECEIVER FUNCTIONS               */
/*----------------------------------------------*/
                
/* pr_get_message: receives header and payload if any. Then deliver the 
 * message to local */
 
int pr_get_message(int raw_fd) {
	int total_bytes, msgtype, cmd, rcvd_bytes, rcode;
	char *data_ptr;
    int payload_size, ret, retry, bytes;
	proc_usr_t kproc, *kp_ptr;
	slot_t slot;
 	message *m_ptr;

	int sock_fd;
	struct timespec ts;
	
	sock_fd = raw_fd; 
	
	
rcv_again:		
		total_bytes = 0;
		msgtype = 0;
		data_ptr = NULL;
		// clear input message queue command 
		do 	{
		  	SVRDEBUG("Waiting on Ethernet sock_fd=%d\n",sock_fd); 		
			rcvd_bytes = recvfrom(sock_fd, &td_recev_rframe_ptr->raw, ETH_FRAME_LEN, 0, NULL, NULL);
			if( rcvd_bytes < 0){
				if( (-errno) == EMOLAGAIN){
					SVRDEBUG("rcvd_bytes=%d\n", rcvd_bytes);
					continue;
				}
				SVRDEBUG("%s\n",strerror(-errno));
				ERROR_EXIT(-errno);
			}
			SVRDEBUG("rcvd_bytes=%d\n", rcvd_bytes);
			DEBUG_hdrcmd(__FUNCTION__, td_recev_rframe_ptr);
					
			// ---------------------------------------------------------------------------	
						
			//  PAYLOAD DATA RECEIVED 
			if ( td_recev_rframe_ptr->fmt.cmd.c_flags & RAW_DATA){
				SVRDEBUG("c_len=%d\n", td_recev_rframe_ptr->fmt.cmd.c_len);

				assert (  (td_recev_rframe_ptr->fmt.cmd.c_cmd == CMD_COPYIN_DATA)
						||(td_recev_rframe_ptr->fmt.cmd.c_cmd == CMD_COPYOUT_DATA));
				assert(td_recev_rframe_ptr->fmt.cmd.c_len > 0);
				assert( data_ptr != NULL);

				assert( (total_bytes + td_recev_rframe_ptr->fmt.cmd.c_len) <= MAXCOPYBUF);
			
				memcpy((void *) data_ptr, (void *) &td_recev_rframe_ptr->fmt.pay, 
								sizeof(td_recev_rframe_ptr->fmt.cmd.c_len));
				data_ptr += td_recev_rframe_ptr->fmt.cmd.c_len;
				total_bytes += td_recev_rframe_ptr->fmt.cmd.c_len;
				SVRDEBUG("total_bytes=%d\n", total_bytes);
				
				if( td_recev_rframe_ptr->fmt.cmd.c_flags & RAW_EOB ){
					SVRDEBUG("RAW_EOB c_flags=%X\n", td_recev_rframe_ptr->fmt.cmd.c_flags);
					if( td_recev_rframe_ptr->fmt.cmd.c_u.cu_vcopy.v_bytes != total_bytes ){
						SVRDEBUG("DISCARD v_bytes=%d total_bytes=%d\n", 
								td_recev_rframe_ptr->fmt.cmd.c_u.cu_vcopy.v_bytes, total_bytes);
						msgtype = 0; // Do not enqueue 
						continue; // DISCARD SET OF FRAME 
					}
					if( td_recev_rframe_ptr->fmt.cmd.c_cmd == CMD_COPYIN_DATA) {	 
						msgtype = (RAW_MQ_HDR | RAW_MQ_PAY);
					}else if ( td_recev_rframe_ptr->fmt.cmd.c_cmd == CMD_COPYOUT_DATA) {
						msgtype = (RAW_MQ_HDR | RAW_MQ_PAY | RAW_MQ_ACK);
					}else{
						SVRDEBUG("BAD CMD cmd=%X\n", td_recev_rframe_ptr->fmt.cmd.c_cmd);
						msgtype = 0;
						continue; // discard SET OF frames
					}
					total_bytes += sizeof(cmd_t);
					SVRDEBUG("total_bytes=%d\n", total_bytes);
				}
			} else { // HEADER RECEIVED 
				//memcpy((void *) &in_msg_ptr->m3.cmd, (void *) &td_recev_rframe_ptr->fmt.cmd, sizeof(cmd_t));
				cmd = (td_recev_rframe_ptr->fmt.cmd.c_cmd & ~(CMD_ACKNOWLEDGE));	
				if( cmd > CMD_BATCHED_CMD || cmd < 0) {
					SVRDEBUG("c_cmd=%X cmd=%X\n", td_recev_rframe_ptr->fmt.cmd.c_cmd, cmd);
					ERROR_EXIT(EMOLINVAL);
				}
				switch(	td_recev_rframe_ptr->fmt.cmd.c_cmd)	{
					case CMD_COPYIN_DATA:
						SVRDEBUG("c_cmd=%X c_len=%d\n", 
							td_recev_rframe_ptr->fmt.cmd.c_cmd, td_recev_rframe_ptr->fmt.cmd.c_len);
						// header + payload
						data_ptr =(char *) &td_recev_rframe_ptr->fmt.pay;
						break;
					case CMD_COPYOUT_DATA:
						SVRDEBUG("c_cmd=%X c_len=%d\n", 
							td_recev_rframe_ptr->fmt.cmd.c_cmd, td_recev_rframe_ptr->fmt.cmd.c_len);
						// header + payload
						data_ptr = (char *) &td_recev_rframe_ptr->fmt.pay;
						break;
					default:
						SVRDEBUG("c_cmd=%X\n", td_recev_rframe_ptr->fmt.cmd.c_cmd);
						// copy HEADER 
						//memcpy(p_header, (void *) &td_recev_rframe_ptr->fmt.cmd, sizeof(cmd_t));
						//p_header= &td_recev_rframe_ptr->fmt.cmd;
						total_bytes = sizeof(cmd_t);
						if( td_recev_rframe_ptr->fmt.cmd.c_cmd & CMD_ACKNOWLEDGE){
							msgtype = (RAW_MQ_HDR | RAW_MQ_ACK);
						}else{						
							msgtype = RAW_MQ_HDR;
						}
						break;
				} 	
			}
		}while(	msgtype == 0);
	

    
	/* now we have a proxy header in the buffer. Cast it.*/
    //payload_size = p_header->c_len;
    SVRDEBUG("msgtype=%X total_bytes=%d \n", msgtype, total_bytes);
	/*cmd = (p_header->c_cmd & ~(CMD_ACKNOWLEDGE));	
	if( cmd > CMD_BATCHED_CMD || cmd < 0) {
		ERROR_EXIT(EMOLINVAL);
	}*/
	if(td_recev_rframe_ptr->fmt.cmd.c_cmd == CMD_NONE){
		SVRDEBUG("RPROXY: CMD_NONE");
		goto rcv_again;
	}
	rcode = mnx_put2lcl(&td_recev_rframe_ptr->fmt.cmd,&td_recev_rframe_ptr->fmt.pay );
	
	if( rcode < 0) {
			SVRDEBUG("msgsnd errno=%d\n",errno);
			if( (-errno) == EMOLAGAIN){ // may be nothing is waiting in the other side 
				return(OK);
			}
			ERROR_EXIT(-errno);
		}
	
    return(OK);    
}

/* pr_start_serving: accept connection from remote sender
   and loop receiving and processing messages
 */
void pr_start_serving(int raw_fd)
{
    int rcode;
	posix_memalign( (void **) &td_recev_rframe_ptr, getpagesize(), sizeof(eth_frame_t) );
	if (td_recev_rframe_ptr == NULL) {
		fprintf(stderr, "posix_memalign td_recev_rframe_ptr \n");
		ERROR_EXIT(-errno);
	}
	// memory for frame to RECEIVE
	/*posix_memalign( (void **) td_recev_rframe_ptr, getpagesize(), sizeof(eth_frame_t) );
	if (td_recev_rframe_ptr == NULL) {
		fprintf(stderr, "posix_memalign rdesc.td_recev_rframe_ptr \n");
		ERROR_EXIT(-errno);
	}*/
    while (1){
		SVRDEBUG("RPROXY: MSGQ px.px_id=%d getting command.\n",px.px_id);
		rcode = mnx_proxy_conn(px.px_id, CONNECT_RPROXY);

    	/* Serve Forever */
		do { 
	       	/* get a complete message and process it */
       		rcode = pr_get_message(raw_fd);
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
void pr_init(int raw_fd) 
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
		} if( rcode < 0) {
			SVRDEBUG("RPROXY: error\n");
			exit(EXIT_FAILURE);
		}
			
	} while	(rcode < OK);
	
   	pr_start_serving(raw_fd);
}

/*----------------------------------------------*/
/*      PROXY SENDER FUNCTIONS                  */
/*----------------------------------------------*/

/* 
 * ps_send_remote: send a message (header + payload if existing) 
 * to remote receiver
 */
int  ps_send_remote(int raw_fd, int msgtype,proxy_hdr_t *p_header, proxy_payload_t *p_payload) 
{
	int rcode, bytes, total_bytes, sock_fd,send_off, rlen,send_retries;
	int  sent_bytes, sframe_len, remain, frame_count, ret;
	
	sock_fd = raw_fd;
	SVRDEBUG("SPROXY:" CMD_FORMAT,CMD_FIELDS(p_header));

	total_bytes = ( sizeof(cmd_t)+ p_header->c_len);
	SVRDEBUG("mtype=%X total_bytes=%d\n", msgtype, total_bytes);
	
	rlen=total_bytes;
	
	
	//td_send_sframe_ptr->fmt.cmd = &out_msg_ptr->m3.cmd;
rs_resend:
	memcpy( &td_send_sframe_ptr->fmt.cmd, p_header, sizeof(cmd_t));
	// check for correct msgq type 
	check_msgtype(msgtype);

	// set protocol specific fields
	send_retries = RAW_MAX_RETRIES; 
	td_send_sframe_ptr->fmt.cmd.c_flags = 0;

	//----------------------------------------
	//		send HEADER 
	//----------------------------------------
	MTX_LOCK(raw_mtx);
		

	td_send_sframe_ptr->fmt.cmd.c_snd_seq 	= lcl_snd_seq;
	td_send_sframe_ptr->fmt.cmd.c_ack_seq 	= lcl_ack_seq;  // last received sequence # by this node 
			
	// Send the frame  with the HEADER 
	sframe_len = hdr_plus_cmd;
	sent_bytes = send_frame(td_send_sframe_ptr, sock_fd, sframe_len);
	SVRDEBUG("sframe_len=%d sent_bytes=%d\n", sframe_len, sent_bytes);
	if( sent_bytes < 0) {
		MTX_UNLOCK(raw_mtx);
		sts_to_rproxy(RAW_MQ_ERR, sent_bytes);
		sleep(RAW_WAIT4ETH);
		sts_to_rproxy(RAW_MQ_OK, OK);
		goto rs_restart;
	}

	lcl_snd_seq++;
	MTX_UNLOCK(raw_mtx);
		
	if( (td_send_sframe_ptr->fmt.cmd.c_cmd !=  CMD_COPYIN_DATA) && 
		(td_send_sframe_ptr->fmt.cmd.c_cmd !=  CMD_COPYOUT_DATA))
		goto rs_restart;
		
	//----------------------------------------
	//		send PAYLOAD  
	//----------------------------------------
	
	rlen -= ((char*) p_payload - (char*) p_header);
	SVRDEBUG("rlen=%d, %d , %d, %d\n", rlen, (char*) &td_send_sframe_ptr,(char*) p_header,(char*) p_payload );
	assert(rlen > 0);
	remain = rlen;
		
	frame_count=1;
	send_off = 0;
	do { // loop of block of data 
		send_retries = RAW_MAX_RETRIES; 
		MTX_LOCK(raw_mtx);
data_again:		
		//	sdesc.td_send_sframe_ptr->fmt.cmd.c_cmd      = CMD_PAYLOAD_DATA;
			td_send_sframe_ptr->fmt.cmd.c_flags 	|= RAW_DATA;
			SVRDEBUG("remain=%d lcl_snd_seq=%ld lcl_ack_seq=%ld send_off=%ld\n",
				remain, lcl_snd_seq, lcl_ack_seq, send_off);
			td_send_sframe_ptr->fmt.cmd.c_snd_seq  	= lcl_snd_seq;
			td_send_sframe_ptr->fmt.cmd.c_ack_seq  	= lcl_ack_seq;
			if ( remain < (ETH_FRAME_LEN - hdr_plus_cmd)) {
				td_send_sframe_ptr->fmt.cmd.c_len 		= remain;
				td_send_sframe_ptr->fmt.cmd.c_flags	|= (RAW_EOB); //| RAW_NEEDACK); 
			} else {
				td_send_sframe_ptr->fmt.cmd.c_len 	= (ETH_FRAME_LEN - hdr_plus_cmd);
				SVRDEBUG("frame_count=%d\n",frame_count);
				if( (frame_count%RAW_ACK_RATE) == 0)
					td_send_sframe_ptr->fmt.cmd.c_flags |=  RAW_NEEDACK; 
			}

			

			// send data  
			sframe_len = (hdr_plus_cmd + td_send_sframe_ptr->fmt.cmd.c_len);
			sent_bytes = send_frame(td_send_sframe_ptr, sock_fd, sframe_len);
			SVRDEBUG("sframe_len=%d sent_bytes=%d\n", sframe_len, sent_bytes);
			if( sent_bytes < 0) {
				MTX_UNLOCK(raw_mtx);
				sts_to_rproxy(RAW_MQ_ERR, sent_bytes);
				sleep(RAW_WAIT4ETH);
				sts_to_rproxy(RAW_MQ_OK, OK);
				goto rs_restart;
			}
			DEBUG_hdrcmd(__FUNCTION__,td_send_sframe_ptr);
	
			lcl_snd_seq++;
			MTX_UNLOCK(raw_mtx);
			frame_count++;				
			send_off += td_send_sframe_ptr->fmt.cmd.c_len;
			remain  -= td_send_sframe_ptr->fmt.cmd.c_len;
		}while (remain > 0); 
	
	rs_restart:
		SVRDEBUG("restart\n");
		return(OK);
}

/* 
 * ps_start_serving: gets local message and sends it to remote receiver .
 * Do this forever.
 */
int  ps_start_serving(int raw_fd)
{	
    int rcode, i;
    message *m_ptr;
    int pid,ret;
   	char *ptr; 
	proxy_hdr_t 	*p_header;
	proxy_payload_t *p_payload;
	int msgtype;

    pid = getpid();
	
	
	
	posix_memalign( (void **) &td_send_sframe_ptr, getpagesize(), sizeof(eth_frame_t) );
	if (td_send_sframe_ptr == NULL) {
		fprintf(stderr, "posix_memalign sdesc.td_send_sframe_ptr \n");
		ERROR_EXIT(-errno);
	}
	set_frame_hdr(td_send_sframe_ptr);
	
	p_header = &td_send_sframe_ptr->fmt.cmd;
	p_payload= &td_send_sframe_ptr->fmt.pay;
		
    while(1) {
	
		SVRDEBUG("SPROXY %d: Waiting a message\n", pid);
        
		rcode = mnx_get2rmt(p_header, p_payload);
		SVRDEBUG("rcode=%d\n",rcode);
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
				msgtype = RAW_MQ_HDR;
				m_ptr = &p_header->c_u.cu_msg;
				SVRDEBUG("SPROXY: " MSG1_FORMAT,  MSG1_FIELDS(m_ptr));
				break;
			case CMD_COPYIN_DATA:
			case CMD_COPYOUT_DATA:
				msgtype = (RAW_MQ_HDR | RAW_MQ_PAY);
				SVRDEBUG("SPROXY: "VCOPY_FORMAT, VCOPY_FIELDS(p_header)); 
				break;
			default:
				msgtype = RAW_MQ_HDR;
				break;
		}

		if( p_header->c_cmd  &  CMD_ACKNOWLEDGE)
			msgtype |= RAW_MQ_ACK;

		// send the message to remote 
		SVRDEBUG("SPROXY: %d "HDR_FORMAT,pid, HDR_FIELDS(p_header));
		rcode =  ps_send_remote(raw_fd,msgtype,p_header, p_payload);
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
void  ps_init(int raw_fd) 
{
    int rcode = 0;
	char *p_buffer;
	SVRDEBUG("SPROXY: Initializing on PID:%d\n", getpid());
    
	do { 
		rcode = mnx_wait4bind_T(RETRY_US);
		SVRDEBUG("SPROXY: mnx_wait4bind_T  rcode=%d ok=%d\n", rcode, OK);
		if (rcode == EMOLTIMEDOUT) {
			SVRDEBUG("SPROXY: mnx_wait4bind_T TIMEOUT\n");
			continue ;
		}else if(rcode == NONE) {
			/* proxies have not endpoint*/ 
			break;	
		} if( rcode < 0){
			//SVRDEBUG("antes while 2");
			exit(EXIT_FAILURE);
		}
			
	} while	(rcode < OK);
		
	
	/* try to connect many times */
	while(1) {
		rcode = mnx_proxy_conn(px.px_id, CONNECT_SPROXY);
		SVRDEBUG("SPROXY: mnx_proxy_conn  rcode=%d\n", rcode);
		if(rcode) ERROR_EXIT(rcode);
		
		ps_start_serving(raw_fd);
	
		rcode = mnx_proxy_conn(px.px_id, DISCONNECT_SPROXY);
		if(rcode)ERROR_EXIT(rcode);
			
	}
    /* code never reaches here */
}



/*===========================================================================*
 *				   main 				    					 *
 *===========================================================================*/
int main (int argc, char *argv[] )
{
	int spid, rpid, pid, status;
	int   ret;
	int raw_fd;
	DVS_usr_t *d_ptr; 
	
//	struct hostent *rmt_he;
		
	if(argc != 4) {
		fprintf (stderr,"usage: %s <rmt_node> <px_id> <ifname> \n", argv[0]);
		exit(1);
	}

	SVRDEBUG("%s: node:%s px_id:%s iface:%s \n", argv[0], argv[1] , argv[2], argv[3]);	
	
	raw_fd = raw_init(argv[3], argv[1]);
 	SVRDEBUG("raw_fd=%d &raw_fd=%p\n",raw_fd , &raw_fd);  
	
	fill_socket();

	strncpy(px.px_name,argv[1], MAXPROXYNAME);
    printf("MSGQ Proxy Pair name: %s\n",px.px_name);
 
    px.px_id = atoi(argv[2]);
    printf("MSGQ Proxy Pair id: %d\n",px.px_id);
	
    local_nodeid = mnx_getDVSinfo(&DVS);
    d_ptr=&DVS;
	SVRDEBUG(DVS_USR_FORMAT,DVS_USR_FIELDS(d_ptr));

	pid = getpid();
	SVRDEBUG("MAIN: pid=%d local_nodeid=%d\n", pid, local_nodeid);

	
	/* creates SENDER and RECEIVER Proxies as children */
    if ( (spid = fork()) == 0) ps_init(raw_fd);
	
    if ( (rpid = fork()) == 0) pr_init(raw_fd);
	
    /* register the proxies*/ 
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

