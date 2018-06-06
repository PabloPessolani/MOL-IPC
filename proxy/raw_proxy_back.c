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
		

char raw_lcl[IFNAMSIZ+1];
char raw_rmt[IFNAMSIZ+1];
uint8_t lcl_mac[ETH_ALEN];
uint8_t rmt_mac[ETH_ALEN];
static char lcl_string[3*ETH_ALEN+1];
static char rmt_string[3*ETH_ALEN+1];
struct sockaddr_ll lcl_sockaddr;
struct sockaddr_ll rmt_sockaddr;
struct ifreq if_idx;
struct ifreq if_mac;
struct arpreq rmt_areq;
unsigned long hdr_plus_cmd;
struct hostent *rmt_he;
int local_nodeid;
int p[2];

long int rmsg_ok = 0;
long int rmsg_fail = 0;
long int smsg_ok = 0;
long int smsg_fail = 0; 
DVS_usr_t DVS;   
proxies_usr_t px, *px_ptr;

proxy_hdr_t 	*p_header;
proxy_payload_t *p_payload;

int raw_fd;  

eth_frame_t 	*td_send_sframe_ptr;
eth_frame_t 	*td_send_rframe_ptr;
eth_frame_t 	*td_recev_rframe_ptr;
eth_frame_t 	*td_recev_sframe_ptr;

struct timeval tv;

unsigned long 	lcl_snd_seq;	// sequence number for the next frame to send  
unsigned long 	lcl_ack_seq;	// sequence number of the last receive frame  
unsigned long 	rmt_ack_seq;	// sequence number of the last acknowledged frame: it must be (lcl_snd_seq-1)  

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
	
	//pthread_mutex_init(&raw_mtx, NULL);  
	
 	hdr_plus_cmd = ((unsigned long)&td_send_sframe_ptr->fmt.pay - (unsigned long)&td_send_sframe_ptr->fmt.hdr);
	SVRDEBUG("hdr_plus_cmd=%d\n",hdr_plus_cmd);

	lcl_snd_seq = 0;	// sequence # for the next frame to send (minus 1)
 	lcl_ack_seq = 0;	// sequence # of the last frame received
	rmt_ack_seq = 0;	// sequence # of the last frame acknoledged by remote node 
	return(raw_fd);
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

int send_frame(eth_frame_t 	*frame_ptr, int sock_fd, int sframe_len)
{
	int sent_bytes;
	
	set_frame_hdr(frame_ptr);
	
	lcl_snd_seq++;
	if (frame_ptr->fmt.cmd.c_flags & RAW_RESEND)
		lcl_snd_seq--;

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
	int sframe_len;
	unsigned long int sent_bytes;
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
			
#ifdef WITH_ACKS
			//------------------------- sequence number checking -------------------
			
			// send remote acknowlege sequence to sender 
			SVRDEBUG("rmt_ack_seq=%ld c_ack_seq=%ld\n", rmt_ack_seq, td_recev_rframe_ptr->fmt.cmd.c_ack_seq);	
			if ((rmt_ack_seq+1) == td_recev_rframe_ptr->fmt.cmd.c_ack_seq) {
				rmt_ack_seq++;
				SVRDEBUG("rmt_ack_seq=%ld lcl_ack_seq=%ld\n", rmt_ack_seq, lcl_ack_seq);
				write(p[1],&rmt_ack_seq, sizeof(rmt_ack_seq));
				SVRDEBUG("write after rmt_ack_seq=%ld\n", rmt_ack_seq);
			}
			
			// check local acknowlege sequence: resent frame  
			if( (lcl_ack_seq + 1) > td_recev_rframe_ptr->fmt.cmd.c_snd_seq){
				SVRDEBUG("resent frame lcl_ack_seq=%ld c_snd_seq=%ld\n",
					lcl_ack_seq, td_recev_rframe_ptr->fmt.cmd.c_snd_seq)
				//MTX_UNLOCK(raw_mtx);	
				continue; // discard frame 
			}
		
			// check local acknowlege sequence: frame out of sequence  
			if ( (lcl_ack_seq + 1) < td_recev_rframe_ptr->fmt.cmd.c_snd_seq){	
				SVRDEBUG("out of sequence lcl_ack_seq=%ld c_snd_seq=%ld\n",
					lcl_ack_seq, td_recev_rframe_ptr->fmt.cmd.c_snd_seq)
				//MTX_UNLOCK(raw_mtx);	
				continue;
			}
			
			// (lcl_ack_seq + 1) == td_recev_rframe_ptr->fmt.cmd.c_snd_seq
			lcl_ack_seq++;
			if( td_recev_rframe_ptr->fmt.cmd.c_flags & RAW_NEEDACK){
//				rcode = enqueue_ack(sock_fd);
				SVRDEBUG(" send ACK \n");
				memcpy( &td_recev_sframe_ptr->fmt.cmd, &td_recev_rframe_ptr->fmt.cmd, sizeof(cmd_t)); 
				td_recev_sframe_ptr->fmt.cmd.c_cmd	= CMD_FRAME_ACK;
				td_recev_sframe_ptr->fmt.cmd.c_flags = 0;
				td_recev_sframe_ptr->fmt.cmd.c_ack_seq=lcl_ack_seq;
				sframe_len = hdr_plus_cmd;
				sent_bytes = send_frame(td_recev_sframe_ptr, sock_fd, sframe_len);
			}
							
			if( td_recev_rframe_ptr->fmt.cmd.c_cmd == CMD_FRAME_ACK){
				SVRDEBUG("CMD_FRAME_ACK ignored c_snd_seq=%ld c_ack_seq=%d\n", 
					td_recev_rframe_ptr->fmt.cmd.c_snd_seq,
					td_recev_rframe_ptr->fmt.cmd.c_ack_seq
					);
				//MTX_UNLOCK(raw_mtx);
				continue;
			}
			// check local acknowlege sequence: correct sequence   
			
			SVRDEBUG("correct sequence lcl_ack_seq=%ld\n", lcl_ack_seq);
			
			//MTX_UNLOCK(raw_mtx);

#endif // WITH_ACKS		
					
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
						data_ptr = (char *) p_payload;
						break;
					case CMD_COPYOUT_DATA:
						SVRDEBUG("c_cmd=%X c_len=%d\n", 
							td_recev_rframe_ptr->fmt.cmd.c_cmd, td_recev_rframe_ptr->fmt.cmd.c_len);
						// header + payload
						data_ptr = (char *) p_payload;
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
				memcpy(p_header, &td_recev_rframe_ptr->fmt.cmd, sizeof(cmd_t) );
			}
		}while(	msgtype == 0);
	
#ifdef WITH_ACKS
		// send ACK to remote
		/*if( td_recev_rframe_ptr->fmt.cmd.c_flags & RAW_NEEDACK){
			SVRDEBUG(" send ACK \n");
			//MTX_LOCK(raw_mtx);
			memcpy( &td_recev_sframe_ptr->fmt.cmd, &td_recev_rframe_ptr->fmt.cmd, sizeof(cmd_t)); 
			td_recev_sframe_ptr->fmt.cmd.c_cmd	= CMD_FRAME_ACK;
			td_recev_sframe_ptr->fmt.cmd.c_flags = 0;
			sframe_len = hdr_plus_cmd;
			sent_bytes = send_frame(td_recev_sframe_ptr, sock_fd, sframe_len);
			//MTX_UNLOCK(raw_mtx);
		}*/
#endif // WITH_ACKS		
    
	/* now we have a proxy header in the buffer. Cast it.*/
    //payload_size = p_header->c_len;
    SVRDEBUG("msgtype=%X total_bytes=%d \n", msgtype, total_bytes);
	/*cmd = (p_header->c_cmd & ~(CMD_ACKNOWLEDGE));	
	if( cmd > CMD_BATCHED_CMD || cmd < 0) {
		ERROR_EXIT(EMOLINVAL);
	}*/
	if(td_recev_rframe_ptr->fmt.cmd.c_cmd == CMD_NONE){
		SVRDEBUG("RPROXY: CMD_NONE\n");
		goto rcv_again;
	}
	rcode = mnx_put2lcl(p_header, p_payload);
	
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
	// memory for frame to RECEIVE
	posix_memalign( (void **) &td_recev_rframe_ptr, getpagesize(), sizeof(eth_frame_t) );
	if (td_recev_rframe_ptr == NULL) {
		fprintf(stderr, "posix_memalign td_recev_rframe_ptr \n");
		ERROR_EXIT(-errno);
	}
	
	// memory for frame to SEND
	posix_memalign( (void **) &td_recev_sframe_ptr, getpagesize(), sizeof(eth_frame_t) );
	if (td_recev_rframe_ptr == NULL) {
		fprintf(stderr, "posix_memalign td_recev_sframe_ptr \n");
		ERROR_EXIT(-errno);
	}
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
	close( p[0] );
   	pr_start_serving(raw_fd);
	close(p[1]);
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
	int rcode, bytes, total_bytes, sock_fd,send_off, send_retries,wait_retries;
	int  sent_bytes, sframe_len, remain, frame_count, ret;
	int wait_again;
	int readbytes;
	char *pay_ptr;
	unsigned long ack_read;
	
	sock_fd = raw_fd;
	SVRDEBUG("SPROXY:" CMD_FORMAT,CMD_FIELDS(p_header));

	total_bytes = ( sizeof(cmd_t)+ p_header->c_len);
	SVRDEBUG("mtype=%X total_bytes=%d\n", msgtype, total_bytes);
	
	ack_read=0;
	
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
	//MTX_LOCK(raw_mtx);
#ifdef WITH_ACKS			
	do {
		td_send_sframe_ptr->fmt.cmd.c_flags |= RAW_NEEDACK;
#endif // WITH_ACKS				

	td_send_sframe_ptr->fmt.cmd.c_snd_seq 	= lcl_snd_seq;
	td_send_sframe_ptr->fmt.cmd.c_ack_seq 	= lcl_ack_seq;  // last received sequence # by this node 
	SVRDEBUG("HEADER lcl_snd_seq=%ld lcl_ack_seq=%ld\n", lcl_snd_seq, lcl_ack_seq);
		
	// Send the frame  with the HEADER 
	sframe_len = hdr_plus_cmd;
	sent_bytes = send_frame(td_send_sframe_ptr, sock_fd, sframe_len);
	SVRDEBUG("HEADER sframe_len=%d sent_bytes=%d\n", sframe_len, sent_bytes);
	if( sent_bytes < 0) {
		//MTX_UNLOCK(raw_mtx);
		sts_to_rproxy(RAW_MQ_ERR, sent_bytes);
		sleep(RAW_WAIT4ETH);
		sts_to_rproxy(RAW_MQ_OK, OK);
		goto rs_restart;
	}
#ifdef WITH_ACKS
		// do not wait for ACK 
		if(!(td_send_sframe_ptr->fmt.cmd.c_flags & RAW_NEEDACK)){
			lcl_snd_seq--; // to compensate final increment
			send_retries = 0;
			continue;
		}
		
		// wait for ACK 
		wait_retries = RAW_MAX_RETRIES;
		while  ( rmt_ack_seq < lcl_snd_seq) {
			SVRDEBUG("rmt_ack_seq=%ld lcl_snd_seq=%ld\n",rmt_ack_seq, lcl_snd_seq);

			SVRDEBUG("read before ack_read=%ld \n",ack_read );
			readbytes = read(p[0],&ack_read, sizeof(ack_read));
			SVRDEBUG("read after  ack_read=%ld rmt_ack_seq=%ld \n", ack_read,rmt_ack_seq);

			if(readbytes < 0)
				return(errno);	

			// PARANOID 
			assert( ack_read == (rmt_ack_seq+1));
			rmt_ack_seq++;
			
			if((--wait_retries) == 0)
				break;
		}
		
		SVRDEBUG("rmt_ack_seq=%ld lcl_snd_seq=%ld\n", rmt_ack_seq, lcl_snd_seq);	
		if (rmt_ack_seq == lcl_snd_seq) break;
			
		SVRDEBUG("send_retries=%d\n",send_retries);
		td_send_sframe_ptr->fmt.cmd.c_flags	|= RAW_RESEND;
		send_retries--;
		if(send_retries > 0) continue;
		// copy the header again to CANCEL 	
		//memcpy( &td_send_sframe_ptr->fmt.cmd, &out_msg_ptr->m3.cmd, sizeof(cmd_t)); 
		td_send_sframe_ptr->fmt.cmd.c_flags	|= RAW_CANCEL;
		sframe_len = hdr_plus_cmd;
		sent_bytes = send_frame(td_send_sframe_ptr, sock_fd, sframe_len);
		goto rs_resend;

	}while ( send_retries > 0);
	
#endif // WITH_ACKS
	//MTX_UNLOCK(raw_mtx);
		
	if( (td_send_sframe_ptr->fmt.cmd.c_cmd !=  CMD_COPYIN_DATA) && 
		(td_send_sframe_ptr->fmt.cmd.c_cmd !=  CMD_COPYOUT_DATA))
		goto rs_restart;
		
	//----------------------------------------
	//		send PAYLOAD  
	//----------------------------------------
	
	remain = p_header->c_len;
	SVRDEBUG("remain=%d\n", remain);
	assert(remain > 0);
		
	frame_count=1;
	send_off = 0;
	do { // loop of block of data 
		send_retries = RAW_MAX_RETRIES; 
		//MTX_LOCK(raw_mtx);
data_again:		
		//	sdesc.td_send_sframe_ptr->fmt.cmd.c_cmd      = CMD_PAYLOAD_DATA;
			td_send_sframe_ptr->fmt.cmd.c_flags 	|= RAW_DATA;
			SVRDEBUG("remain=%d lcl_snd_seq=%ld lcl_ack_seq=%ld send_off=%ld\n",
				remain, lcl_snd_seq, lcl_ack_seq, send_off);
			td_send_sframe_ptr->fmt.cmd.c_snd_seq  	= lcl_snd_seq;
			td_send_sframe_ptr->fmt.cmd.c_ack_seq  	= lcl_ack_seq;
			
	
			if ( remain < (ETH_FRAME_LEN - hdr_plus_cmd)) {
				td_send_sframe_ptr->fmt.cmd.c_len 	= remain;
				td_send_sframe_ptr->fmt.cmd.c_flags	|= (RAW_EOB); //| RAW_NEEDACK); 
			} else {
				td_send_sframe_ptr->fmt.cmd.c_len 	= (ETH_FRAME_LEN - hdr_plus_cmd);
				SVRDEBUG("frame_count=%d\n",frame_count);
				if( (frame_count%RAW_ACK_RATE) == 0)
					td_send_sframe_ptr->fmt.cmd.c_flags |=  RAW_NEEDACK; 
			}
			
			pay_ptr = (char *) p_payload + send_off;
			memcpy( td_send_sframe_ptr->fmt.pay, pay_ptr , td_send_sframe_ptr->fmt.cmd.c_len);

			// send data  
			sframe_len = (hdr_plus_cmd + td_send_sframe_ptr->fmt.cmd.c_len);
			sent_bytes = send_frame(td_send_sframe_ptr, sock_fd, sframe_len);
			SVRDEBUG("PAYLOAD sframe_len=%d sent_bytes=%d\n", sframe_len, sent_bytes);
			if( sent_bytes < 0) {
				//MTX_UNLOCK(raw_mtx);
				sts_to_rproxy(RAW_MQ_ERR, sent_bytes);
				sleep(RAW_WAIT4ETH);
				sts_to_rproxy(RAW_MQ_OK, OK);
				goto rs_restart;
			}
			DEBUG_hdrcmd(__FUNCTION__,td_send_sframe_ptr);
#ifdef WITH_ACKS
			do 	{ 
					if(!(td_send_sframe_ptr->fmt.cmd.c_flags &  RAW_NEEDACK)){
						lcl_snd_seq--; // to compensate final increment
						send_retries = 0;
						continue;
					}
							
					do{
						// loop of wait for the correct sequence acknowledge 
						// WAIT that the sender wakeup me when a frame has arrived 
						SVRDEBUG("rmt_ack_seq=%ld lcl_snd_seq=%ld\n",rmt_ack_seq, lcl_snd_seq);
						if( rmt_ack_seq == lcl_snd_seq){
							SVRDEBUG("ACK OK lcl_snd_seq=%ld\n",lcl_snd_seq);
							send_retries = 0;
							break;
						}
						
						SVRDEBUG("read before ack_read=%ld \n",ack_read );
						readbytes = read(p[0],&ack_read, sizeof(ack_read));
						SVRDEBUG("read after  ack_read=%ld rmt_ack_seq=%ld \n", ack_read,rmt_ack_seq);
						if( (ack_read+1) == rmt_ack_seq)
							rmt_ack_seq++;
						if(readbytes < 0)
							if(errno == EAGAIN ||  errno == EWOULDBLOCK)
								break;
					}while(readbytes<0);
					
					if( rmt_ack_seq != lcl_snd_seq) { 
						SVRDEBUG("rmt_ack_seq=%d lcl_snd_seq=%d%\n", rmt_ack_seq, lcl_snd_seq);					
						SVRDEBUG("send_retries=%d\n",send_retries);
						td_send_sframe_ptr->fmt.cmd.c_flags	|= RAW_RESEND;
						send_retries--;
						if(send_retries > 0) continue;
						// copy the header again to CANCEL 	
						//memcpy( &td_send_sframe_ptr->fmt.cmd, &out_msg_ptr->m3.cmd, sizeof(cmd_t)); 
						td_send_sframe_ptr->fmt.cmd.c_flags	|= RAW_CANCEL;
						sframe_len = hdr_plus_cmd;
						sent_bytes = send_frame(td_send_sframe_ptr, sock_fd, sframe_len);
						goto data_again;
					}else{
						SVRDEBUG("ACK OK lcl_snd_seq=%ld\n",lcl_snd_seq);
						send_retries = 0;
						break;
					}
			} while ( send_retries > 0);

#endif // WITH_ACKS			
			//MTX_UNLOCK(raw_mtx);
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
	int msgtype;

    pid = getpid();
	
	set_frame_hdr(td_send_sframe_ptr);
		
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
		SVRDEBUG("SPROXY: smsg_ok=%ld smsg_fail=%ld \n", smsg_ok,smsg_fail);
	}	
	/* never reached */
    exit(1);
}

/* ps_connect_to_remote: connects to the remote receiver */
/* 
 * ps_init: creates sender socket, the connect to remote and
 * start sending messages to remote 
 */
void *SENDER_th(void *arg) 
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
			exit(EXIT_FAILURE);
		}
			
	} while	(rcode < OK);
		
	posix_memalign( (void **) &td_send_sframe_ptr, getpagesize(), sizeof(eth_frame_t) );
	if (td_send_sframe_ptr == NULL) {
		fprintf(stderr, "posix_memalign sdesc.td_send_sframe_ptr \n");
		ERROR_EXIT(-errno);
	}
	
	/* try to connect many times */
	while(1) {
		rcode = mnx_proxy_conn(px.px_id, CONNECT_SPROXY);
		SVRDEBUG("SPROXY: mnx_proxy_conn  rcode=%d\n", rcode);
		if(rcode) ERROR_EXIT(rcode);
		
		close( p[1] );
		ps_start_serving(raw_fd);
	
		rcode = mnx_proxy_conn(px.px_id, DISCONNECT_SPROXY);
		if(rcode)ERROR_EXIT(rcode);
		close(p[0]);	
	}
    /* code never reaches here */
	free(td_send_sframe_ptr);
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

	SVRDEBUG("MAIN: pthread_create RAW RECEIVER \n");
	if (ret = pthread_create(&rdesc.td_thread, NULL, RECEIVER_th,(void*)&raw_fd )) {
		ERROR_EXIT(ret);
	}	
	
	SVRDEBUG("MAIN: pthread_create RAW SENDER \n");
	if (ret = pthread_create(&sdesc.td_thread, NULL, SENDER_th,(void*)&raw_fd )) {
		ERROR_EXIT(ret);
	}
		
    /* register the proxies*/ 
    ret = mnx_proxies_bind(px.px_name, px.px_id, spid, rpid,MAXCOPYBUF);
    if( ret < 0) ERROR_EXIT(ret);
	
	px_ptr = &px;
	SVRDEBUG(PX_USR_FORMAT , PX_USR_FIELDS(px_ptr));
	SVRDEBUG("binded to (%d,%d)\n", spid, rpid);

	ret= mnx_node_up(px.px_name, px.px_id, px.px_id);	
	
  	SVRDEBUG("WAITING CHILDREN raw_fd=%d !!!\n", raw_fd);  
	pthread_join ( rdesc.td_thread, NULL );
	pthread_join ( sdesc.td_thread, NULL );
	
    exit(0);
}

