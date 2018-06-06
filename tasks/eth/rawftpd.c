#define _GNU_SOURCE     
#define _MULTI_THREADED
#define  MOL_USERSPACE	1
//#define TASKDBG			1

#include "rawftp.h"

#define	IFNAME		"eth0"

int raw_mtu;
int raw_fd;
int raw_lpid;
int raw_mode;
char raw_src[IFNAMSIZ+1];
char raw_dst[IFNAMSIZ+1];
eth_frame_t *sframe_ptr;
eth_frame_t *rframe_ptr;
uint8_t src_mac[ETH_ALEN];
uint8_t dst_mac[ETH_ALEN];
static char src_string[3*ETH_ALEN+1];
struct stat file_stat;
struct sockaddr_ll src_sockaddr;
struct sockaddr_ll dst_sockaddr;
struct ifreq if_idx;
struct ifreq if_mac;
struct ifreq if_mtu;
struct arpreq dst_areq;
char *path_ptr, *read_buf;
unsigned long hdr_plus_cmd;



FILE *fp;
struct timeval tv;


void usage (char *progname){
	fprintf (stderr,"usage: %s <ifname> \n", progname);
	exit(1);
}

void choose (char *progname){
	fprintf (stderr, "%s: you must choose Client(c) or Server(s) flag\n", progname);
	exit(1);
}

void bad_file (char *progname, char *filename){
	fprintf (stderr, "%s: bad file name %s\n", progname, filename);
	exit(1);
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
    if (!arp_fd) ERROR_EXIT(errno);

    /* Ignore the first line, which contains the header */
    if (!fgets(header, sizeof(header), arp_fd))
        ERROR_EXIT(errno);


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
		ERROR_EXIT(errno);
	
	
	if (inet_aton(dst_ip_str, &ipaddr) == 0) 
		ERROR_EXIT(errno);
	
	  /* Make the ARP request. */
    memset(&dst_areq, 0, sizeof(dst_areq));
    sin = (struct sockaddr_in *) &dst_areq.arp_pa;
    sin->sin_family = AF_INET;
	
	sin->sin_addr = ipaddr;
    sin = (struct sockaddr_in *) &dst_areq.arp_ha;
    sin->sin_family = ARPHRD_ETHER;

    strncpy(dst_areq.arp_dev, IFNAME , 15);
    if (ioctl(dgram_sock, SIOCGARP, (caddr_t) &dst_areq) == -1) 
		ERROR_EXIT(errno);

    printf("Destination IP:%s MAC:%s\n",
    inet_ntoa(((struct sockaddr_in *) &dst_areq.arp_pa)->sin_addr),
    ethernet_mactoa(&dst_areq.arp_ha));
	return(OK);
}
	
void raw_init(char *if_name)
{	
	int i;
	
	// memory for frame to SEND
	posix_memalign( (void **) &sframe_ptr, getpagesize(), sizeof(eth_frame_t) );
	if (sframe_ptr == NULL) {
		fprintf(stderr, "posix_memalign sframe_ptr \n");
		ERROR_EXIT(errno);
	}

	// memory for frame to RECEIVE
	posix_memalign( (void **) &rframe_ptr, getpagesize(), sizeof(eth_frame_t) );
	if (rframe_ptr == NULL) {
		fprintf(stderr, "posix_memalign rframe_ptr \n");
		ERROR_EXIT(errno);
	}
	
	// memory for READ 
	posix_memalign( (void **) &read_buf, getpagesize(), MAXCOPYBUF );
	if (read_buf == NULL) {
		fprintf(stderr, "posix_memalign read_buf \n");
		ERROR_EXIT(errno);
	}	
	
	// get socket for RAW
	raw_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_M3IPC));
	if (raw_fd == -1) {
		TASKDEBUG("%s\n",strerror(errno));
		ERROR_EXIT(errno);
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
		TASKDEBUG("%s\n",strerror(errno));
		ERROR_EXIT(errno);
	}

	memcpy(src_mac, &if_mac.ifr_hwaddr.sa_data, ETH_ALEN);
	for(i = 0; i < ETH_ALEN; i++){
		sprintf(&src_string[i*3], "%02X:", src_mac[i]);
	}
	src_string[(ETH_ALEN*3)-1] = 0;
	TASKDEBUG("%s src_mac %s\n",raw_src, src_string);

	memset(&src_sockaddr,0,sizeof(src_sockaddr));
	src_sockaddr.sll_family=PF_PACKET;
	src_sockaddr.sll_protocol=htons(ETH_M3IPC); 

	TASKDEBUG("protocol:%X  ETH_P_ALL=%X\n",src_sockaddr.sll_protocol, htons(ETH_P_ALL));
	
	src_sockaddr.sll_ifindex=if_idx.ifr_ifindex;
	if (bind(raw_fd,(struct sockaddr*)&src_sockaddr,sizeof(src_sockaddr))<0){
		TASKDEBUG("%s\n",strerror(errno));
		ERROR_EXIT(errno);
	}
	
	tv.tv_sec = RAW_RCV_TIMEOUT;
	if(setsockopt(raw_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0){
		TASKDEBUG("%s\n",strerror(errno));
		ERROR_EXIT(errno);
	}
	
	hdr_plus_cmd = ((unsigned long)&sframe_ptr->fmt.pay - (unsigned long)&sframe_ptr->fmt.hdr);
}

void DEBUG_hdrcmd(eth_frame_t *f_ptr)
{
	_eth_hdr_t *eth_hdr_ptr;
	cmd_t *cmd_ptr;

	eth_hdr_ptr= &f_ptr->fmt.hdr;
	TASKDEBUG(ETHHDR_FORMAT,ETHHDR_FIELDS(eth_hdr_ptr));
	cmd_ptr = &f_ptr->fmt.cmd;
	TASKDEBUG(CMD_FORMAT,CMD_FIELDS(cmd_ptr));
	TASKDEBUG(CMD_XFORMAT,CMD_XFIELDS(cmd_ptr));
}
	
void fill_socket(void)
{
	int i;

	/*RAW communication*/
	dst_sockaddr.sll_family   = AF_PACKET;	
	
	/*we don't use a protocoll above ethernet layer  ->just use anything here*/
	dst_sockaddr.sll_protocol = htons(ETH_M3IPC);	

	/*index of the network device */
	dst_sockaddr.sll_ifindex  = if_idx.ifr_ifindex;

	/*address length*/
	dst_sockaddr.sll_halen    = ETH_ALEN;
	
	/*MAC - begin*/
	for(i = 0; i < ETH_ALEN; i++){
		dst_sockaddr.sll_addr[i]  = rframe_ptr->fmt.hdr.src.ea_addr[i];		
		src_sockaddr.sll_addr[i]  = rframe_ptr->fmt.hdr.dst.ea_addr[i];		
	}
	/*MAC - end*/
	dst_sockaddr.sll_addr[ETH_ALEN+1]  = 0x00;/*not used*/
	dst_sockaddr.sll_addr[ETH_ALEN+2]  = 0x00;/*not used*/
	src_sockaddr.sll_addr[ETH_ALEN+1]  = 0x00;/*not used*/
	src_sockaddr.sll_addr[ETH_ALEN+2]  = 0x00;/*not used*/
}

int send_ack(int cmd , int rcode)
{
	ether_type_t proto; 
	int sframe_len, sent_bytes;
	
	TASKDEBUG("cmd=%d rcode=%d\n", cmd , rcode);

	// 	Destination MAC	: 
	memcpy(&sframe_ptr->fmt.hdr.dst, &rframe_ptr->fmt.hdr.src, ETH_ALEN );

	// 	Source MAC 
	memcpy(&sframe_ptr->fmt.hdr.src, &rframe_ptr->fmt.hdr.dst, ETH_ALEN );
	
	//   Protocolo Field
	proto = (ether_type_t) htons(ETH_M3IPC); 
	memcpy( &sframe_ptr->fmt.hdr.proto, &proto  , sizeof(ether_type_t));

	sframe_ptr->fmt.cmd.c_cmd		= cmd;
	sframe_ptr->fmt.cmd.c_rcode 	= rcode;
	sframe_ptr->fmt.cmd.c_flags		= 0;
	sframe_ptr->fmt.cmd.c_snd_off  	= 0;
	sframe_ptr->fmt.cmd.c_ack_off  	= 0;
	sframe_ptr->fmt.cmd.c_len 		= 0;
	sframe_len = hdr_plus_cmd;
	TASKDEBUG("sframe_len=%d rcode=%d\n", sframe_len, rcode);
	sent_bytes = sendto(raw_fd, &sframe_ptr->raw, sframe_len, 0x00,
	   (struct sockaddr*)&dst_sockaddr, sizeof(dst_sockaddr));
	 
	DEBUG_hdrcmd(sframe_ptr);
	if( sent_bytes < 0)
		return(-errno);
	return(OK);
}
	
/*===========================================================================*
 *				   main 				    					 *
 *===========================================================================*/
int main (int argc, char *argv[] )
{
	int rlen, sframe_len, err_code, remain,  frame_count;
	int total_bytes, sent_bytes, rcvd_bytes;
	int ack_off, snd_off, send_retry;
//	struct hostent *rmt_he;
		
	if(argc != 2) usage(argv[0]);

	TASKDEBUG("Starting %s at %s\n", argv[0], argv[1]);
		
	raw_init(argv[1]);
			
	do {
		ack_off = 0;
		snd_off = 0; 
		err_code = OK;
		//--------------------------------------------------------------------
		// RECEIVE FILE NAME 
		//--------------------------------------------------------------------
		rcvd_bytes = recvfrom(raw_fd, &rframe_ptr->raw, ETH_FRAME_LEN, 0, NULL, NULL);
		if( rcvd_bytes < 0){
			if( errno == EMOLAGAIN) continue;
			TASKDEBUG("%s\n",strerror(errno));
			ERROR_EXIT(errno);
		}
		TASKDEBUG("rcvd_bytes=%d\n", rcvd_bytes);

		// print receive header and cmd 
		DEBUG_hdrcmd(rframe_ptr);
		
		fill_socket();

		if( rframe_ptr->fmt.cmd.c_cmd != RAW_GETFILE) {
			err_code = EMOLBADCALL;
			TASKDEBUG("c_cmd=%d\n", rframe_ptr->fmt.cmd.c_cmd);
			send_ack((RAW_GETFILE | RAW_ACKNOWLEDGE), err_code);
			continue;
		}
		
		// try to open the requested file 
		path_ptr= (char*) &rframe_ptr->fmt.pay;
		rlen   	= rframe_ptr->fmt.cmd.c_len;
		TASKDEBUG("rlen=%d\n", rlen);
		path_ptr[rlen]  = 0;
		TASKDEBUG("open >%s< \n", path_ptr);
		fp = fopen(path_ptr, "r");
		if (fp == NULL){
			err_code = (-errno);
			TASKDEBUG("%s\n",strerror(errno));
			ERROR_PRINT(err_code); 
			send_ack((RAW_GETFILE | RAW_ACKNOWLEDGE), err_code);
			continue;
		}
		
		// send ACK 
		if( rframe_ptr->fmt.cmd.c_flags & RAW_NEEDACK) {
			err_code = send_ack((RAW_GETFILE | RAW_ACKNOWLEDGE), err_code);
			if( err_code != OK){
				ERROR_PRINT(err_code); 
				continue;
			}
		}

		//--------------------------------------------------------------------
		// FILE TRANSFER  
		//--------------------------------------------------------------------
		total_bytes = 0;
		while( (rlen = fread(read_buf, 1, MAXCOPYBUF, fp)) >= 0) {
			TASKDEBUG("MAXCOPYBUF=%d rlen=%d\n", MAXCOPYBUF, rlen);

			send_retry = 0;
			snd_off = 0;
			
			
			// fill the header 
			sframe_ptr->fmt.cmd.c_cmd		= RAW_DATA;
			sframe_ptr->fmt.cmd.c_rcode 	= OK;
			sframe_ptr->fmt.cmd.c_ack_off  	= 0;
			
			remain = rlen;
			frame_count=1;
			do {
				sframe_ptr->fmt.cmd.c_flags 	= 0;
retry:				
				sframe_ptr->fmt.cmd.c_snd_off  	= snd_off;
				if ( rlen == 0) {	
					sframe_ptr->fmt.cmd.c_flags	|= (RAW_EOF | RAW_NEEDACK); 
					sframe_ptr->fmt.cmd.c_len 	= 0;
				} else if ( remain < (ETH_FRAME_LEN - hdr_plus_cmd)) {
					sframe_ptr->fmt.cmd.c_flags	|= (RAW_EOB | RAW_NEEDACK); 
					sframe_ptr->fmt.cmd.c_len 	= remain;
				} else {
					sframe_ptr->fmt.cmd.c_len 	= (ETH_FRAME_LEN - hdr_plus_cmd);
					TASKDEBUG("frame_count=%d\n",frame_count);
					if( (frame_count%RAW_ACK_RATE) == 0)
						sframe_ptr->fmt.cmd.c_flags	|=  RAW_NEEDACK; 
				}

				memcpy(sframe_ptr->fmt.pay, &read_buf[snd_off], sframe_ptr->fmt.cmd.c_len); 
				sframe_len = (hdr_plus_cmd + sframe_ptr->fmt.cmd.c_len);

				// send data tv client 
				sent_bytes = sendto(raw_fd, &sframe_ptr->raw, sframe_len, 0x00,
				   (struct sockaddr*)&dst_sockaddr, sizeof(dst_sockaddr));	
				if( sent_bytes < 0) {
					err_code = (-errno);
					TASKDEBUG("%s\n",strerror(errno));
					ERROR_PRINT(err_code); 
					break;
				}
				frame_count++;

				DEBUG_hdrcmd(sframe_ptr);
		
				// wait for ACK 
				if( sframe_ptr->fmt.cmd.c_flags & RAW_NEEDACK) {
					rcvd_bytes = recvfrom(raw_fd, &rframe_ptr->raw, ETH_FRAME_LEN, 0, NULL, NULL);
					if( rcvd_bytes < 0) {
						if( errno == EMOLAGAIN) break;
						TASKDEBUG("%s\n",strerror(errno));
						ERROR_EXIT(errno);
					}
					TASKDEBUG("rcvd_bytes=%d\n", rcvd_bytes);
				
					DEBUG_hdrcmd(rframe_ptr);
			
					if( rframe_ptr->fmt.cmd.c_cmd != (RAW_DATA | RAW_ACKNOWLEDGE)) {
						err_code = EMOLBADCALL;
						ERROR_PRINT(err_code);
						break;
					}
						
					if( rframe_ptr->fmt.cmd.c_rcode != OK ) {
						send_retry++;
						TASKDEBUG("send_retry=%d RAW_MAX_RETRIES=%d\n", send_retry, RAW_MAX_RETRIES);
						if( send_retry == RAW_MAX_RETRIES)
							break;
						sframe_ptr->fmt.cmd.c_flags |= (RAW_RESEND | RAW_NEEDACK);
						snd_off = rframe_ptr->fmt.cmd.c_ack_seq;
						remain = rlen - snd_off;
						goto  retry;
					}
				}
				
				total_bytes += sent_bytes;
				
				if( err_code != OK)	{
					rlen = 0;
					break;
				}		
				
				snd_off += sframe_ptr->fmt.cmd.c_len;
				remain  -= sframe_ptr->fmt.cmd.c_len;
			} while (remain > 0);
			if( rlen == 0) break;
		}
		TASKDEBUG("CLOSE total_bytes=%d\n", total_bytes);
		err_code = fclose(fp);		
		if(err_code < 0) {
			TASKDEBUG("%s\n",strerror(errno));
			ERROR_PRINT(-errno); 	
		}
	} while(1);
	
}
