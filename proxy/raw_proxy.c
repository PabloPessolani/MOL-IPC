/********************************************************/
/* 		RAW ETHERNET THREADED PROXIES		*/
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

#define MTX_TRYLOCK(r, x) do{ \
		SVRDEBUG("MTX_TRYLOCK %s\n", #x);\
		r = pthread_mutex_trylock(&x);\
		}while(0)
			
#define MTX_UNLOCK(x) do{ \
		pthread_mutex_unlock(&x);\
		SVRDEBUG("MTX_UNLOCK %s \n", #x);\
		}while(0)	
			
#define COND_WAIT(x,y, ts) do{ \
		SVRDEBUG("COND_WAIT ENTER %s %s\n", #x,#y );\
		pthread_cond_wait(&x, &y);\
		clock_gettime(CLOCK_REALTIME, &ts);\
		SVRDEBUG("COND_WAIT EXIT %s tv_sec=%ld\n", #x,ts.tv_sec);\
		}while(0)	

#define COND_WAIT_TO(r,x,y,ts,t) do{ \
		SVRDEBUG("COND_WAIT_TO %s %s %s\n", #r,#x,#y );\
		clock_gettime(CLOCK_REALTIME, &ts);\
		SVRDEBUG("COND_WAIT_TO before tv_sec=%ld\n", ts.tv_sec);\
		ts.tv_sec += t;\
		r = pthread_cond_timedwait(&x, &y, &ts);\
		clock_gettime(CLOCK_REALTIME, &ts);\
		SVRDEBUG("COND_WAIT_TO after tv_sec=%ld\n", ts.tv_sec);\
		}while(0)
		
#define COND_SIGNAL(x, ts) do{ \
		pthread_cond_signal(&x);\
		clock_gettime(CLOCK_REALTIME, &ts);\
		SVRDEBUG("COND_SIGNAL %s tv_sec=%ld\n", #x,ts.tv_sec);\
		}while(0)	

		
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

long int rmsg_ok = 0;
long int rmsg_fail = 0;
long int smsg_ok = 0;
long int smsg_fail = 0; 
dvs_usr_t dvs;   
proxies_usr_t px, *px_ptr;

struct timeval tv;
unsigned long 	lcl_snd_seq;	// sequence number for the next frame to send  
unsigned long 	lcl_ack_seq;	// sequence number of the last receive frame  
unsigned long 	rmt_ack_seq;	// sequence number of the last acknowledged frame: it must be (lcl_snd_seq-1)  
unsigned long	last_ack_send;  // for optimization 

int sender_waiting;

pthread_mutex_t main_mtx;

struct thread_desc_s {
    pthread_t 		td_thread;
	proxy_hdr_t 	*td_header;
	proxy_payload_t *td_payload;		
	eth_frame_t 	*td_sframe;
	eth_frame_t 	*td_rframe;
	
	int 			td_msg_ok;
	int 			td_msg_fail;	
	struct timespec td_ts;

	pthread_mutex_t td_mtx;    /* mutex & condition to allow main thread to
								wait for the new thread to  set its TID */
	pthread_cond_t  td_cond;   /* '' */
	pid_t           td_tid;     /* to hold new thread's TID */
	
};
typedef struct thread_desc_s thread_desc_t;
thread_desc_t rdesc;
thread_desc_t sdesc;


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
	
int raw_init( char *if_name, char *hname)
{	
	int i;
	int init_fd;
	
	sender_waiting = FALSE;
	
	rmt_he = gethostbyname(hname);	
	printf("rmt_name=%s rmt_ip=%s\n", rmt_he->h_name, 
		inet_ntoa( *( struct in_addr*)(rmt_he->h_addr)));
	arp_table();
	get_arp(inet_ntoa( *( struct in_addr*)(rmt_he->h_addr)));
	
	// get socket for RAW
	init_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_M3IPC));
	if (init_fd == -1) {
		SVRDEBUG("%s\n",strerror(-errno));
		ERROR_EXIT(-errno);
	}
	
	/* Get the index of the interface to send on */
	memset(&if_idx, 0, sizeof(struct ifreq));
	strncpy(if_idx.ifr_name, if_name, strlen(if_name));
	if (ioctl(init_fd, SIOCGIFINDEX, &if_idx) < 0)
	    perror("SIOCGIFINDEX");
		
	/* Get SOURCE interface MAC address  */
	memset(&if_mac,0,sizeof(if_mac));
	strncpy(if_mac.ifr_name, if_name, strlen(if_name));
	if (ioctl(init_fd,SIOCGIFHWADDR,&if_mac) == -1) {
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
	if (bind(init_fd,(struct sockaddr*)&lcl_sockaddr,sizeof(lcl_sockaddr))<0){
		SVRDEBUG("%s\n",strerror(-errno));
		ERROR_EXIT(-errno);
	}
	
	tv.tv_sec = RAW_RCV_TIMEOUT;
	if(setsockopt(init_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0){
		SVRDEBUG("%s\n",strerror(-errno));
		ERROR_EXIT(-errno);
	}
	
	//pthread_mutex_init(&raw_mtx, NULL);  
	
 	hdr_plus_cmd = ((unsigned long)&rdesc.td_sframe->fmt.pay - (unsigned long)&rdesc.td_sframe->fmt.hdr);
	SVRDEBUG("hdr_plus_cmd=%d\n",hdr_plus_cmd);

	lcl_snd_seq = 0;	// sequence # for the next frame to send (minus 1)
 	lcl_ack_seq = 0;	// sequence # of the last frame received
	rmt_ack_seq = 0;	// sequence # of the last frame acknoledged by remote node 
	last_ack_send = 0;   
	
	return(init_fd);
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

int send_frame(int who, int raw_fd, eth_frame_t *frame_ptr, int sframe_len, int resend)
{
	int sent_bytes;
	
	set_frame_hdr(frame_ptr);
		
	MTX_LOCK(main_mtx);

	if( who == RAW_RECEIVER) {
		if( resend == FALSE) {
			if( last_ack_send == lcl_ack_seq){
			SVRDEBUG("OPTIMIZATION who=%d lcl_snd_seq=%ld last_ack_send=%ld\n",who,  			
					lcl_snd_seq, last_ack_send);	
				MTX_UNLOCK(main_mtx);
				return(sframe_len);
			}
		}
	}
		
	lcl_snd_seq++;
	if ((frame_ptr->fmt.cmd.c_flags & RAW_RESEND) || (resend == TRUE))
		lcl_snd_seq--;

	frame_ptr->fmt.cmd.c_snd_seq  = lcl_snd_seq;
	frame_ptr->fmt.cmd.c_ack_seq  = lcl_ack_seq; 
	last_ack_send = lcl_ack_seq;
	
	SVRDEBUG("SEND lcl_snd_seq=%ld lcl_ack_seq=%ld sframe_len=%d\n", 
		lcl_snd_seq, lcl_ack_seq, sframe_len);
	MTX_UNLOCK(main_mtx);
		
	sent_bytes = sendto(raw_fd, &frame_ptr->raw, sframe_len, 0x00,
					(struct sockaddr*)&rmt_sockaddr, sizeof(rmt_sockaddr));

	DEBUG_hdrcmd(__FUNCTION__, frame_ptr);
	
	if( sent_bytes < 0) 
		ERROR_RETURN(-errno);
	return(sent_bytes);
}
	

void check_msgtype(int msgtype, eth_frame_t *frame_ptr)
{
	int cmd;

  	SVRDEBUG("msgtype=%X \n", msgtype); 
	
	switch( frame_ptr->fmt.cmd.c_cmd){
		case CMD_COPYIN_DATA:
			// header + payload
			assert(msgtype & (RAW_ETH_HDR | RAW_ETH_PAY));
			break;
		case CMD_COPYOUT_DATA:
			// header + payload
			assert(msgtype & (RAW_ETH_HDR | RAW_ETH_PAY | RAW_ETH_ACK));
			break;
		case CMD_FRAME_ACK:
			assert(msgtype == RAW_ETH_ACK);
			assert(frame_ptr->fmt.cmd.c_cmd == CMD_FRAME_ACK);
			break;
		default:
			assert(msgtype & RAW_ETH_HDR);
			if( frame_ptr->fmt.cmd.c_cmd & CMD_ACKNOWLEDGE){
				assert(msgtype & RAW_ETH_ACK);
				cmd = (frame_ptr->fmt.cmd.c_cmd & ~(CMD_ACKNOWLEDGE));	
			} else {
				cmd = frame_ptr->fmt.cmd.c_cmd;
			}
			assert( (cmd >= 0) && (cmd <= CMD_LAST_CMD));
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
                
/* pr_rcv_message: receives header and payload if any. Then deliver the 
 * message to local */
 
int pr_rcv_message(int raw_fd)
{
	int total_bytes, msgtype, cmd, rcvd_bytes, rcode;
	char *data_ptr;
	int sframe_len;
	unsigned long int sent_bytes;
    int payload_size, ret, retry, bytes;
	struct timespec ts;

rcv_again:		
		total_bytes = 0;
		msgtype = 0;
		data_ptr = NULL;
		do 	{
		  	SVRDEBUG("Waiting on Ethernet raw_fd=%d\n",raw_fd); 		
			rcvd_bytes = recvfrom(raw_fd, &rdesc.td_rframe->raw, ETH_FRAME_LEN, 0, NULL, NULL);
			if( rcvd_bytes < 0){
				if( (-errno) == EMOLAGAIN){
					SVRDEBUG("rcvd_bytes=%d\n", rcvd_bytes);
					continue;
				}
				SVRDEBUG("%s\n",strerror(-errno));
				ERROR_EXIT(-errno);
			}
			SVRDEBUG("rcvd_bytes=%d\n", rcvd_bytes);
			DEBUG_hdrcmd(__FUNCTION__, rdesc.td_rframe);
			
#ifdef WITH_ACKS
			//------------------------- sequence number checking -------------------
		
			// try to get the mutex, if it can it is because sender is waiting
			MTX_LOCK(main_mtx);
			
			// check local acknowlege sequence: resent frame  
			if( (lcl_ack_seq + 1) > rdesc.td_rframe->fmt.cmd.c_snd_seq){
				SVRDEBUG("resent frame lcl_ack_seq=%ld c_snd_seq=%ld\n",
					lcl_ack_seq, rdesc.td_rframe->fmt.cmd.c_snd_seq)
				MTX_UNLOCK(main_mtx);
				continue; // discard frame 
			}

			// check local acknowlege sequence: frame out of sequence  
			if ( (lcl_ack_seq + 1) < rdesc.td_rframe->fmt.cmd.c_snd_seq){	
				SVRDEBUG("out of sequence lcl_ack_seq=%ld c_snd_seq=%ld\n",
					lcl_ack_seq, rdesc.td_rframe->fmt.cmd.c_snd_seq)
				MTX_UNLOCK(main_mtx);
				continue;
			}
		
			// (lcl_ack_seq + 1) == td_recev_rframe_ptr->fmt.cmd.c_snd_seq
			lcl_ack_seq++;
			SVRDEBUG("correct sequence lcl_ack_seq=%ld\n", lcl_ack_seq);
			
			// send remote acknowlege sequence to sender 
			SVRDEBUG("rmt_ack_seq=%ld c_ack_seq=%ld\n", rmt_ack_seq, rdesc.td_rframe->fmt.cmd.c_ack_seq);	
			if (rmt_ack_seq < rdesc.td_rframe->fmt.cmd.c_ack_seq) {
				rmt_ack_seq = rdesc.td_rframe->fmt.cmd.c_ack_seq;
				if( sender_waiting == TRUE){
					COND_SIGNAL(sdesc.td_cond, sdesc.td_ts);
					sender_waiting = FALSE;
				}
			}
		
			if( rdesc.td_rframe->fmt.cmd.c_flags & RAW_NEEDACK){
				if( sdesc.td_sframe->fmt.cmd.c_ack_seq < lcl_ack_seq)  {
					SVRDEBUG(" send ACK \n");
					memcpy( &rdesc.td_sframe->fmt.cmd, &rdesc.td_rframe->fmt.cmd, sizeof(cmd_t)); 
					rdesc.td_sframe->fmt.cmd.c_cmd	 = CMD_FRAME_ACK;
					rdesc.td_sframe->fmt.cmd.c_flags = 0;
					rdesc.td_sframe->fmt.cmd.c_len 	 = 0;
					rdesc.td_sframe->fmt.cmd.c_rcode = 0;
					sframe_len = hdr_plus_cmd;
					MTX_UNLOCK(main_mtx);
					sent_bytes = send_frame(RAW_RECEIVER, raw_fd, rdesc.td_sframe, sframe_len, FALSE);
					if(sent_bytes < 0){
						ERROR_RETURN(sent_bytes);
					}
				} else {
					SVRDEBUG("OPTIMIZATION: send ACK avoided \n");					
				}
			}
			MTX_UNLOCK(main_mtx);
							
			if( rdesc.td_rframe->fmt.cmd.c_cmd == CMD_FRAME_ACK){
				SVRDEBUG("CMD_FRAME_ACK ignored c_snd_seq=%ld c_ack_seq=%d\n", 
					rdesc.td_rframe->fmt.cmd.c_snd_seq,
					rdesc.td_rframe->fmt.cmd.c_ack_seq
					);
				continue;
			}
			
#endif // WITH_ACKS		
					
			// ---------------------------------------------------------------------------					
			//  PAYLOAD DATA RECEIVED 
			if ( rdesc.td_rframe->fmt.cmd.c_flags & RAW_DATA){
				SVRDEBUG("c_len=%d\n", rdesc.td_rframe->fmt.cmd.c_len);

				assert (  (rdesc.td_rframe->fmt.cmd.c_cmd == CMD_COPYIN_DATA)
						||(rdesc.td_rframe->fmt.cmd.c_cmd == CMD_COPYOUT_DATA));
				assert(rdesc.td_rframe->fmt.cmd.c_len > 0);
				assert( data_ptr != NULL);

				assert( (total_bytes + rdesc.td_rframe->fmt.cmd.c_len) <= MAXCOPYBUF);
			
				memcpy((void *) data_ptr, (void *) &rdesc.td_rframe->fmt.pay, 
								sizeof(rdesc.td_rframe->fmt.cmd.c_len));
				data_ptr += rdesc.td_rframe->fmt.cmd.c_len;
				total_bytes += rdesc.td_rframe->fmt.cmd.c_len;
				SVRDEBUG("total_bytes=%d\n", total_bytes);
				
				if( rdesc.td_rframe->fmt.cmd.c_flags & RAW_EOB ){
					SVRDEBUG("RAW_EOB c_flags=%X\n", rdesc.td_rframe->fmt.cmd.c_flags);
					if( rdesc.td_rframe->fmt.cmd.c_u.cu_vcopy.v_bytes != total_bytes ){
						SVRDEBUG("DISCARD v_bytes=%d total_bytes=%d\n", 
								rdesc.td_rframe->fmt.cmd.c_u.cu_vcopy.v_bytes, total_bytes);
						msgtype = 0; // Do not enqueue 
						continue; // DISCARD SET OF FRAME 
					}
					if( rdesc.td_rframe->fmt.cmd.c_cmd == CMD_COPYIN_DATA) {	 
						msgtype = (RAW_ETH_HDR | RAW_ETH_PAY);
					}else if ( rdesc.td_rframe->fmt.cmd.c_cmd == CMD_COPYOUT_DATA) {
						msgtype = (RAW_ETH_HDR | RAW_ETH_PAY | RAW_ETH_ACK);
					}else{
						SVRDEBUG("BAD CMD cmd=%X\n", rdesc.td_rframe->fmt.cmd.c_cmd);
						msgtype = 0;
						continue; // discard SET OF frames
					}
					total_bytes += sizeof(cmd_t);
					SVRDEBUG("total_bytes=%d\n", total_bytes);
				}
			} else { // HEADER RECEIVED 
				//memcpy((void *) &in_msg_ptr->m3.cmd, (void *) &rdesc.td_rframe->fmt.cmd, sizeof(cmd_t));
				cmd = (rdesc.td_rframe->fmt.cmd.c_cmd & ~(CMD_ACKNOWLEDGE));	
				if( cmd > CMD_LAST_CMD || cmd < 0) {
					SVRDEBUG("c_cmd=%X cmd=%X\n", rdesc.td_rframe->fmt.cmd.c_cmd, cmd);
					ERROR_EXIT(EMOLINVAL);
				}
				switch(	rdesc.td_rframe->fmt.cmd.c_cmd)	{
					case CMD_COPYIN_DATA:
						SVRDEBUG("c_cmd=%X c_len=%d\n", 
							rdesc.td_rframe->fmt.cmd.c_cmd, rdesc.td_rframe->fmt.cmd.c_len);
						// header + payload
						data_ptr = (char *) rdesc.td_payload;
						break;
					case CMD_COPYOUT_DATA:
						SVRDEBUG("c_cmd=%X c_len=%d\n", 
							rdesc.td_rframe->fmt.cmd.c_cmd, rdesc.td_rframe->fmt.cmd.c_len);
						// header + payload
						data_ptr = (char *) rdesc.td_payload;
						break;
					default:
						SVRDEBUG("c_cmd=%X\n", rdesc.td_rframe->fmt.cmd.c_cmd);
						// copy HEADER 
						//memcpy(p_header, (void *) &rdesc.td_rframe->fmt.cmd, sizeof(cmd_t));
						//p_header= &rdesc.td_rframe->fmt.cmd;
						total_bytes = sizeof(cmd_t);
						if( rdesc.td_rframe->fmt.cmd.c_cmd & CMD_ACKNOWLEDGE){
							msgtype = (RAW_ETH_HDR | RAW_ETH_ACK);
						}else{						
							msgtype = RAW_ETH_HDR;
						}
					break;
				} 	
				memcpy(rdesc.td_header, &rdesc.td_rframe->fmt.cmd, sizeof(cmd_t) );
			}
		}while(	msgtype == 0);
	
 
	if(rdesc.td_rframe->fmt.cmd.c_cmd == CMD_NONE){
		SVRDEBUG("RPROXY: CMD_NONE\n");
		goto rcv_again;
	}
	rcode = mnx_put2lcl(rdesc.td_header, rdesc.td_payload);
	
	if( rcode < 0) {
			SVRDEBUG("mnx_put2lcl rcode=%d\n",rcode);
			if( (-rcode) == EMOLAGAIN){ // may be nothing is waiting in the other side 
				return(OK);
			}
			ERROR_EXIT(-rcode);
		}
	
    return(OK);    
}

/* pr_start_serving: accept connection from remote sender
   and loop receiving and processing messages
 */
void pr_start_serving(int raw_fd)
{
    int rcode;

	/* Serve Forever */
	do { 
		/* get a complete message and process it */
		rcode = pr_rcv_message(raw_fd);
		if (rcode == OK) {
			SVRDEBUG("RPROXY: Message succesfully processed.\n");
			rmsg_ok++;
		} else {
			SVRDEBUG("RPROXY: Message processing failure [%d]\n",rcode);
			rmsg_fail++;
			if( rcode == EMOLNOTCONN) break;
		}	
	}while(1);    
    /* never reached */
}

/*----------------------------------------------*/
/*      PROXY SENDER FUNCTIONS                  */
/*----------------------------------------------*/

/* 
 * ps_send_message: send a message (header + payload if existing) 
 * to remote receiver
 */
int  ps_send_message(int raw_fd,  int msgtype) 
{
	int rcode, bytes, total_bytes, send_off, send_retries,wait_retries;
	int  sent_bytes, sframe_len, remain, frame_count, ret;
	int wait_again;
	int readbytes;
	int resend;
	char *pay_ptr;
	unsigned long ack_read;
	
	SVRDEBUG("SPROXY:" CMD_FORMAT,CMD_FIELDS(sdesc.td_header));

	total_bytes = ( sizeof(cmd_t)+ sdesc.td_header->c_len);
	SVRDEBUG("mtype=%X total_bytes=%d\n", msgtype, total_bytes);
	
	ack_read=0;
	resend = FALSE;
	
rs_resend:
	memcpy( &sdesc.td_sframe->fmt.cmd, sdesc.td_header, sizeof(cmd_t));
	// check for correct msgq type 
	check_msgtype(msgtype, sdesc.td_sframe);

	// set protocol specific fields
	send_retries = RAW_MAX_RETRIES; 
	sdesc.td_sframe->fmt.cmd.c_flags = 0;

	//----------------------------------------
	//		send HEADER 
	//----------------------------------------
#ifdef WITH_ACKS			
	do {
		if( sdesc.td_sframe->fmt.cmd.c_flags & CMD_ACKNOWLEDGE )  // only commands ACKs need ethernet ACKs
			sdesc.td_sframe->fmt.cmd.c_flags |= RAW_NEEDACK;
#endif // WITH_ACKS				

		// Send the frame  with the HEADER 
		sframe_len = hdr_plus_cmd;
		sent_bytes = send_frame(RAW_SENDER, raw_fd, sdesc.td_sframe, sframe_len, FALSE);
		SVRDEBUG("HEADER sframe_len=%d sent_bytes=%d\n", sframe_len, sent_bytes);
		if( sent_bytes < 0) {
			sleep(RAW_WAIT4ETH);
			goto rs_restart;
		}
#ifdef WITH_ACKS
		// do not wait for ACK 
		if(!(sdesc.td_sframe->fmt.cmd.c_flags & RAW_NEEDACK)){
			send_retries = 0;
			continue;
		}
		
		// wait for ACK 
		wait_retries = RAW_MAX_RETRIES;
		MTX_LOCK(main_mtx);
		while  ( rmt_ack_seq < lcl_snd_seq && wait_retries > 0 ) {
			sender_waiting = TRUE;
			SVRDEBUG("rmt_ack_seq=%ld lcl_snd_seq=%ld\n",rmt_ack_seq, lcl_snd_seq);
			COND_WAIT_TO(rcode,sdesc.td_cond,main_mtx, sdesc.td_ts, RAW_WAIT4ACK);
			if( rcode ) {
				MTX_UNLOCK(main_mtx);
				return(rcode);	
			}
			if( rmt_ack_seq < lcl_snd_seq)
				wait_retries--;
		}
		SVRDEBUG("rmt_ack_seq=%ld lcl_snd_seq=%ld\n", rmt_ack_seq, lcl_snd_seq);	
		if (rmt_ack_seq == lcl_snd_seq) {
			MTX_UNLOCK(main_mtx);
			break;
		}
		MTX_UNLOCK(main_mtx);

		// the ACK has not arrived 
		SVRDEBUG("send_retries=%d\n",send_retries);
		sdesc.td_sframe->fmt.cmd.c_flags	|= RAW_RESEND;
		send_retries--;
		if(send_retries > 0) continue;
		
		// copy the header again to CANCEL 	
		resend = TRUE;
		sdesc.td_sframe->fmt.cmd.c_flags	|= (RAW_CANCEL | RAW_RESEND);
		sframe_len = hdr_plus_cmd;
		sent_bytes = send_frame(RAW_SENDER, raw_fd, sdesc.td_sframe, sframe_len, resend);
		if(sent_bytes < 0)
			ERROR_PRINT(sent_bytes);
		goto rs_resend;

	}while ( send_retries > 0);
	
#endif // WITH_ACKS
		
	if( (sdesc.td_sframe->fmt.cmd.c_cmd !=  CMD_COPYIN_DATA) && 
		(sdesc.td_sframe->fmt.cmd.c_cmd !=  CMD_COPYOUT_DATA))
		goto rs_restart;
		
	//----------------------------------------
	//		send PAYLOAD  
	//----------------------------------------
	
	remain = sdesc.td_header->c_len;
	SVRDEBUG("remain=%d\n", remain);
	assert(remain > 0);
		
	frame_count=1;
	send_off = 0;
	do { // loop for each data block 
		send_retries = RAW_MAX_RETRIES; 
data_again:		
			sdesc.td_sframe->fmt.cmd.c_flags 	|= RAW_DATA;
			SVRDEBUG("remain=%d lcl_snd_seq=%ld lcl_ack_seq=%ld send_off=%ld\n",
				remain, lcl_snd_seq, lcl_ack_seq, send_off);
	
			if ( remain < (ETH_FRAME_LEN - hdr_plus_cmd)) {
				sdesc.td_sframe->fmt.cmd.c_len 	= remain;
				sdesc.td_sframe->fmt.cmd.c_flags	|= (RAW_EOB); //| RAW_NEEDACK); 
			} else {
				sdesc.td_sframe->fmt.cmd.c_len 	= (ETH_FRAME_LEN - hdr_plus_cmd);
				SVRDEBUG("frame_count=%d\n",frame_count);
				if( (frame_count%RAW_ACK_RATE) == 0)
					sdesc.td_sframe->fmt.cmd.c_flags |=  RAW_NEEDACK; 
			}
			
			pay_ptr = (char *) sdesc.td_payload + send_off;
			memcpy( sdesc.td_sframe->fmt.pay, pay_ptr , sdesc.td_sframe->fmt.cmd.c_len);

			// send data  
			sframe_len = (hdr_plus_cmd + sdesc.td_sframe->fmt.cmd.c_len);
			sent_bytes = send_frame(RAW_SENDER, raw_fd, sdesc.td_sframe, sframe_len, FALSE);
			SVRDEBUG("PAYLOAD sframe_len=%d sent_bytes=%d\n", sframe_len, sent_bytes);
			if( sent_bytes < 0) {
				sleep(RAW_WAIT4ETH);
				goto rs_restart;
			}
			DEBUG_hdrcmd(__FUNCTION__,sdesc.td_sframe);
#ifdef WITH_ACKS
			do 	{ 
					if(!(sdesc.td_sframe->fmt.cmd.c_flags &  RAW_NEEDACK)){
						send_retries = 0;
						continue;
					}
					
					// wait for ACK 
					wait_retries = RAW_MAX_RETRIES;
					MTX_LOCK(main_mtx);
					while  ( rmt_ack_seq < lcl_snd_seq && wait_retries > 0 ) {
						sender_waiting = TRUE;
						SVRDEBUG("rmt_ack_seq=%ld lcl_snd_seq=%ld\n",rmt_ack_seq, lcl_snd_seq);
						COND_WAIT_TO(rcode,sdesc.td_cond,main_mtx,  sdesc.td_ts, RAW_WAIT4ACK);
						if( rcode ) {
							MTX_UNLOCK(main_mtx);
							return(rcode);	
						}
						if( rmt_ack_seq < lcl_snd_seq)
							wait_retries--;
					}
					SVRDEBUG("rmt_ack_seq=%ld lcl_snd_seq=%ld\n", rmt_ack_seq, lcl_snd_seq);	
					if (rmt_ack_seq == lcl_snd_seq) {
						MTX_UNLOCK(main_mtx);
						break;
					}
					MTX_UNLOCK(main_mtx);
									
					// the ACK has not arrived 
					SVRDEBUG("send_retries=%d\n",send_retries);
					sdesc.td_sframe->fmt.cmd.c_flags	|= RAW_RESEND;
					send_retries--;
					if(send_retries > 0) continue;
					
					// copy the header again to CANCEL
					resend = TRUE;
					sdesc.td_sframe->fmt.cmd.c_flags	|= (RAW_CANCEL | RAW_RESEND);
					sframe_len = hdr_plus_cmd;
					sent_bytes = send_frame(RAW_SENDER, raw_fd,  sdesc.td_sframe, sframe_len, resend);
					goto rs_resend;
			
			} while ( send_retries > 0);

#endif // WITH_ACKS			
			frame_count++;				
			send_off += sdesc.td_sframe->fmt.cmd.c_len;
			remain  -= sdesc.td_sframe->fmt.cmd.c_len;
		}while (remain > 0); 
	
	rs_restart:
		SVRDEBUG("restart\n");
		return(OK);
}

/* 
 * ps_start_serving: gets local message and sends it to remote receiver .
 * Do this forever.
 */
int  ps_start_serving(int raw_fd )
{	
    int rcode, i;
    message *m_ptr;
    int ret;
   	char *ptr; 
	int msgtype;
	
	set_frame_hdr(sdesc.td_sframe);

    while(1) {
	
		SVRDEBUG("SPROXY: Waiting a message\n");
        
		rcode = mnx_get2rmt(sdesc.td_header, sdesc.td_payload);
		SVRDEBUG("rcode=%d\n",rcode);
  		if( rcode != OK) {
			switch(rcode) {
				case EMOLTIMEDOUT:
					SVRDEBUG("SPROXY: Sending HELLO \n");
					sdesc.td_header->c_cmd = CMD_NONE;
					sdesc.td_header->c_len = 0;
					sdesc.td_header->c_rcode = 0;
					break;
				case EMOLNOTCONN:
					return(EMOLNOTCONN);
				default:
					SVRDEBUG("ERROR  mnx_get2rmt %d\n", rcode);
					continue;
			}
		}

		SVRDEBUG("SPROXY: "HDR_FORMAT, HDR_FIELDS(sdesc.td_header)); 

		switch(sdesc.td_header->c_cmd){
			case CMD_SEND_MSG:
			case CMD_SNDREC_MSG:
			case CMD_REPLY_MSG:
			case CMD_NTFY_MSG:
				msgtype = RAW_ETH_HDR;
				m_ptr = &sdesc.td_header->c_u.cu_msg;
				SVRDEBUG("SPROXY: " MSG1_FORMAT,  MSG1_FIELDS(m_ptr));
				break;
			case CMD_COPYIN_DATA:
			case CMD_COPYOUT_DATA:
				msgtype = (RAW_ETH_HDR | RAW_ETH_PAY);
				SVRDEBUG("SPROXY: "VCOPY_FORMAT, VCOPY_FIELDS(sdesc.td_header)); 
				break;
			default:
				msgtype = RAW_ETH_HDR;
				break;
		}

		if( sdesc.td_header->c_cmd  &  CMD_ACKNOWLEDGE)
			msgtype |= RAW_ETH_ACK;

		// send the message to remote 
		SVRDEBUG("SPROXY: "HDR_FORMAT,HDR_FIELDS(sdesc.td_header));
		rcode =  ps_send_message(raw_fd, msgtype);
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

/* pr_thread: creates socket */
 void *pr_thread(void *arg) 
{
    int rcode;
	int raw_fd, *s_ptr;
	
	SVRDEBUG("RPROXY: Initializing...\n");
    
	s_ptr = (int *) arg; 
	raw_fd = *s_ptr;
  	SVRDEBUG("raw_fd=%d s_ptr=%p hdr_plus_cmd=%d\n",raw_fd, s_ptr, hdr_plus_cmd); 
	
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

	posix_memalign( (void **) &rdesc.td_rframe, getpagesize(), sizeof(eth_frame_t) );
	if (rdesc.td_rframe == NULL) {
		fprintf(stderr, "posix_memalign rdesc.td_rframe \n");
		ERROR_EXIT(-errno);
	}

	posix_memalign( (void **) &rdesc.td_sframe, getpagesize(), sizeof(eth_frame_t) );
	if (rdesc.td_sframe == NULL) {
		fprintf(stderr, "posix_memalign rdesc.td_sframe \n");
		ERROR_EXIT(-errno);
	}
	
	/* try to connect many times */
	while(1) {
		rcode = mnx_proxy_conn(px.px_id, CONNECT_RPROXY);
		SVRDEBUG("RPROXY: mnx_proxy_conn  rcode=%d\n", rcode);
		if(rcode) ERROR_EXIT(rcode);
		
		pr_start_serving(raw_fd);
	
		rcode = mnx_proxy_conn(px.px_id, DISCONNECT_RPROXY);
		if(rcode)ERROR_EXIT(rcode);
	}
    /* code never reaches here */
	free(rdesc.td_header);
	free(rdesc.td_payload);
	free(rdesc.td_rframe);
	free(rdesc.td_sframe);
	
}

/* 
 * ps_thread: creates sender socket, the connect to remote and
 * start sending messages to remote 
 */
 void *ps_thread(void *arg) 
{
    int rcode;
	int sock_fd, *s_ptr;
	struct timespec ts;

	SVRDEBUG("SPROXY: Initializing...\n");
	
	s_ptr = (int *) arg; 
	sock_fd = *s_ptr;
  	SVRDEBUG("sock_fd=%d s_ptr=%p hdr_plus_cmd=%d\n",sock_fd, s_ptr, hdr_plus_cmd);
	
    
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

	posix_memalign( (void**) &sdesc.td_header, getpagesize(), (sizeof(proxy_hdr_t)));
	if (sdesc.td_header== NULL) {
    		perror(" sdesc.td_header posix_memalign");
    		exit(1);
  	}

	posix_memalign( (void**) &sdesc.td_payload, getpagesize(), (sizeof(proxy_payload_t)));
	if (sdesc.td_payload== NULL) {
    		perror(" sdesc.td_payload posix_memalign");
    		exit(1);
  	}

	posix_memalign( (void **) &sdesc.td_rframe, getpagesize(), sizeof(eth_frame_t) );
	if (sdesc.td_rframe == NULL) {
		fprintf(stderr, "posix_memalign sdesc.td_rframe \n");
		ERROR_EXIT(-errno);
	}

	posix_memalign( (void **) &sdesc.td_sframe, getpagesize(), sizeof(eth_frame_t) );
	if (sdesc.td_sframe == NULL) {
		fprintf(stderr, "posix_memalign sdesc.td_sframe \n");
		ERROR_EXIT(-errno);
	}
	
	/* try to connect many times */
	while(1) {
		rcode = mnx_proxy_conn(px.px_id, CONNECT_SPROXY);
		SVRDEBUG("SPROXY: mnx_proxy_conn  rcode=%d\n", rcode);
		if(rcode) ERROR_EXIT(rcode);
		
		ps_start_serving(sock_fd);
	
		rcode = mnx_proxy_conn(px.px_id, DISCONNECT_SPROXY);
		if(rcode)ERROR_EXIT(rcode);
	}
    /* code never reaches here */
	free(sdesc.td_header);
	free(sdesc.td_payload);
	free(sdesc.td_rframe);
	free(sdesc.td_sframe);
}

/*===========================================================================*
 *				   main 				    					 *
 *===========================================================================*/
int main (int argc, char *argv[] )
{
	int spid, rpid, pid, status;
	int   ret;
	dvs_usr_t *d_ptr; 
static 	int raw_fd;
	
//	struct hostent *rmt_he;
		
	if(argc != 4) {
		fprintf (stderr,"usage: %s <rmt_node> <px_id> <ifname> \n", argv[0]);
		exit(1);
	}

	SVRDEBUG("%s: node:%s px_id:%s iface:%s \n", argv[0], argv[1] , argv[2], argv[3]);	
	
	raw_fd = raw_init(argv[3], argv[1]);
 	SVRDEBUG("raw_fd=%d\n",raw_fd);  
	
	fill_socket();

	strncpy(px.px_name,argv[1], MAXPROXYNAME);
    printf("MSGQ Proxy Pair name: %s\n",px.px_name);
 
    px.px_id = atoi(argv[2]);
    printf("MSGQ Proxy Pair id: %d\n",px.px_id);
	
    local_nodeid = mnx_getdvsinfo(&dvs);
    d_ptr=&dvs;
	SVRDEBUG(dvs_USR_FORMAT,dvs_USR_FIELDS(d_ptr));

	pid = getpid();
	SVRDEBUG("MAIN: pid=%d local_nodeid=%d\n", pid, local_nodeid);

	pthread_mutex_init(&main_mtx, NULL);  /* init main mutex */

    /* creates SENDER and RECEIVER Proxies as Treads */
	SVRDEBUG("MAIN: pthread_create RPROXY\n");
	pthread_cond_init(&rdesc.td_cond, NULL);  /* init condition */
	pthread_mutex_init(&rdesc.td_mtx, NULL);  /* init mutex */
	pthread_mutex_lock(&rdesc.td_mtx);
	if ( (ret = pthread_create(&rdesc.td_thread, NULL, pr_thread,(void*)&raw_fd )) != 0) {
		ERROR_EXIT(ret);
	}
    pthread_cond_wait(&rdesc.td_cond, &rdesc.td_mtx);
	SVRDEBUG("MAIN: RPROXY td_tid=%d\n", rdesc.td_tid);
    pthread_mutex_unlock(&rdesc.td_mtx);

	SVRDEBUG("MAIN: pthread_create SPROXY\n");
	pthread_cond_init(&sdesc.td_cond, NULL);  /* init condition */
	pthread_mutex_init(&sdesc.td_mtx, NULL);  /* init mutex */
	pthread_mutex_lock(&sdesc.td_mtx);
	if ((ret = pthread_create(&sdesc.td_thread, NULL, ps_thread,(void*)&raw_fd )) != 0) {
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

	ret= mnx_node_up(px.px_name, px.px_id, px.px_id);	
	if (ret) {
		SVRDEBUG("mnx_node_up error=%d\n", ret);
	}
		
	pthread_join ( rdesc.td_thread, NULL );
	pthread_join ( sdesc.td_thread, NULL );
	pthread_mutex_destroy(&rdesc.td_mtx);
	pthread_cond_destroy(&rdesc.td_cond);
	pthread_mutex_destroy(&sdesc.td_mtx);
	pthread_cond_destroy(&sdesc.td_cond);
	
    exit(0);
}

