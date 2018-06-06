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
static char dst_string[3*ETH_ALEN+1];
struct stat file_stat;
struct sockaddr_ll src_sockaddr;
struct sockaddr_ll dst_sockaddr;
struct ifreq if_idx;
struct ifreq if_mac;
struct ifreq if_mtu;
struct arpreq dst_areq;

FILE *fp;
char *path_ptr,  *write_buf;
struct hostent *rmt_he;
unsigned long hdr_plus_cmd;
int total_bytes;
int ack_off;
struct timeval tv;

void usage (char *progname){
	fprintf (stderr,"usage: %s <ifname> <svrname> <filename> \n", progname);
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

void raw_init(char *iface, char *hname)
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
	
	// memory for WRITE 
	posix_memalign( (void **) &write_buf, getpagesize(), MAXCOPYBUF );
	if (write_buf == NULL) {
		fprintf(stderr, "posix_memalign write_buf \n");
		ERROR_EXIT(errno);
	}	
	
	rmt_he = gethostbyname(hname);	
	printf("rmt_name=%s rmt_ip=%s\n", rmt_he->h_name, 
		inet_ntoa( *( struct in_addr*)(rmt_he->h_addr)));
	arp_table();
	get_arp(inet_ntoa( *( struct in_addr*)(rmt_he->h_addr)));
	
	// get socket for RAW
	raw_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_M3IPC));
	if (raw_fd == -1) {
		TASKDEBUG("%s\n",strerror(errno));
		ERROR_EXIT(errno);
	}
	
	tv.tv_sec = RAW_RCV_TIMEOUT;
	if(setsockopt(raw_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0){
		TASKDEBUG("%s\n",strerror(errno));
		ERROR_EXIT(errno);
	}
	
	/* Get the index of the interface to send on */
	memset(&if_idx, 0, sizeof(struct ifreq));
	strncpy(if_idx.ifr_name, iface, strlen(iface));
	if (ioctl(raw_fd, SIOCGIFINDEX, &if_idx) < 0)
	    perror("SIOCGIFINDEX");
		
	/* Get SOURCE  interface MAC address  */
	memset(&if_mac,0,sizeof(if_mac));
	strncpy(if_mac.ifr_name, iface, strlen(iface));
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

	/* Get DESTINATION  interface MAC address  */
	memcpy(dst_mac, dst_areq.arp_ha.sa_data, ETH_ALEN);
	for(i = 0; i < ETH_ALEN; i++){
		sprintf(&dst_string[i*3], "%02X:", dst_mac[i]);
	}
	dst_string[(ETH_ALEN*3)-1] = 0;
	TASKDEBUG("%s dst_mac %s\n",raw_dst, dst_string);
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
		dst_sockaddr.sll_addr[i]  = dst_mac[i];;		
	}
	/*MAC - end*/
	dst_sockaddr.sll_addr[ETH_ALEN+1]  = 0x00;/*not used*/
	dst_sockaddr.sll_addr[ETH_ALEN+2]  = 0x00;/*not used*/

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

void send_getfile(char *fname)
{
	int path_len, sframe_len, sent_bytes;
	ether_type_t proto; 

	path_len = strlen(fname)+1;
	
	// 	Destination MAC	: 
	memcpy((u8_t*)&sframe_ptr->fmt.hdr.dst, (u8_t*)&dst_mac[0], ETH_ALEN);

	// 	Source MAC 
	memcpy((u8_t*)&sframe_ptr->fmt.hdr.src, (u8_t*)&src_mac[0], ETH_ALEN);
	
	//   Protocolo Field
	proto = (ether_type_t) htons(ETH_M3IPC); 
	memcpy((u8_t*)&sframe_ptr->fmt.hdr.proto, (u8_t*)&proto , sizeof(ether_type_t));
	
	// FILL COMMAND 
	sframe_ptr->fmt.cmd.c_cmd 		= RAW_GETFILE;
	sframe_ptr->fmt.cmd.c_snd_off 	= 0;
	sframe_ptr->fmt.cmd.c_len 		= path_len;
	sframe_ptr->fmt.cmd.c_flags 	= RAW_NEEDACK;

	// COPY PAYLOAD (FILENAME)
	memcpy((u8_t*)sframe_ptr->fmt.pay, (u8_t*)fname,path_len);
	sframe_len = (hdr_plus_cmd + path_len);
	TASKDEBUG("hdr_plus_cmd=%ld \n", hdr_plus_cmd);
	TASKDEBUG("_eth_hdr_t=%d cmd_t=%d \n",	sizeof(_eth_hdr_t), sizeof(cmd_t));
	TASKDEBUG("sframe_len=%d path_len=%d >%s<\n",	sframe_len, path_len, sframe_ptr->fmt.pay);
	
	DEBUG_hdrcmd(sframe_ptr);

	// SEND PATH TO SERVER 
	sent_bytes = sendto(raw_fd, &sframe_ptr->raw, sframe_len, 0x00,
		   (struct sockaddr*)&dst_sockaddr, sizeof(dst_sockaddr));
	TASKDEBUG("sent_bytes=%d c_snd_off=%ld\n", 
			sent_bytes, sframe_ptr->fmt.cmd.c_snd_off);
			
	if( sent_bytes < 0)
		ERROR_EXIT(-errno);
}


int recv_getack()
{
	int rcvd_bytes;
	
	rcvd_bytes = recvfrom(raw_fd, &rframe_ptr->raw, ETH_FRAME_LEN, 0, NULL, NULL);
	if( rcvd_bytes < 0){
		TASKDEBUG("%s\n",strerror(errno));
		ERROR_RETURN(errno);
	}
	TASKDEBUG("rcvd_bytes=%d\n", rcvd_bytes);
	
	DEBUG_hdrcmd(rframe_ptr);

	if( rframe_ptr->fmt.cmd.c_cmd != (RAW_GETFILE | RAW_ACKNOWLEDGE)) {
		TASKDEBUG("c_cmd=%X\n" , rframe_ptr->fmt.cmd.c_cmd);
		ERROR_RETURN(-rframe_ptr->fmt.cmd.c_cmd);
	}
	
	if ( rframe_ptr->fmt.cmd.c_rcode != OK){
		TASKDEBUG("c_rcode=%X\n" , rframe_ptr->fmt.cmd.c_rcode);
		ERROR_RETURN(-rframe_ptr->fmt.cmd.c_rcode );		
	}
	return(OK);
}

void send_ack(int cmd, int rcode)
{
	int sframe_len, sent_bytes;
	
	sframe_ptr->fmt.cmd.c_cmd		= (RAW_DATA | RAW_ACKNOWLEDGE);;
	sframe_ptr->fmt.cmd.c_rcode 	= rcode;
	sframe_ptr->fmt.cmd.c_snd_off  	= 0;
	sframe_ptr->fmt.cmd.c_ack_off  	= ack_off;
	sframe_ptr->fmt.cmd.c_len 		= 0;
	sframe_ptr->fmt.cmd.c_flags		= 0;
	sframe_len = hdr_plus_cmd;
	TASKDEBUG("sframe_len=%d rcode=%d\n", sframe_len, rcode);
	sent_bytes = sendto(raw_fd, &sframe_ptr->raw, sframe_len, 0x00,
		  (struct sockaddr*)&dst_sockaddr, sizeof(dst_sockaddr));
	
	DEBUG_hdrcmd(sframe_ptr);
	
}

/*===========================================================================*
 *				   main 				    					 *
 *===========================================================================*/
int main (int argc, char *argv[] )
{
	int wlen, err_code, rcvd_bytes;
	int sent_frame;
	
	if(argc != 4) usage(argv[0]);

	hdr_plus_cmd = ((unsigned long)&sframe_ptr->fmt.pay - (unsigned long)&sframe_ptr->fmt.hdr);

	TASKDEBUG("Starting %s at %s connect to server %s for file %s\n", 
		argv[0], argv[1], argv[2], argv[3]);
		
	if( strlen(argv[3]) > (PAYLOAD_LEN - 2)) {
		fprintf (stderr, "filepath name too long (%d)\n", strlen(argv[3]));
		ERROR_EXIT(EMOLNAMETOOLONG);
	}
		
	path_ptr = argv[3];
	TASKDEBUG("open >%s< \n", path_ptr);
	fp = fopen(path_ptr, "w");
	if (fp == NULL){
		err_code = (-errno);
		TASKDEBUG("%s\n",strerror(errno));
		ERROR_EXIT(err_code); 
	}
	
	raw_init(argv[1], argv[2]);

	sent_frame = 0;

	fill_socket();
	
	send_getfile(path_ptr);
	
	recv_getack();
	
	ack_off = 0;
	total_bytes = 0;
	err_code = OK;
	
	// Esto es para inyectar error !!!!
	int do_error = 0;
	do 	{
		
		rcvd_bytes = recvfrom(raw_fd, &rframe_ptr->raw, ETH_FRAME_LEN, 0, NULL, NULL);
		if( rcvd_bytes < 0){
			TASKDEBUG("%s\n",strerror(errno));
			ERROR_EXIT(errno);
		}
		TASKDEBUG("rcvd_bytes=%d err_code=%d\n", rcvd_bytes, err_code);
		DEBUG_hdrcmd(rframe_ptr);

		if( err_code != OK) {
			if( rframe_ptr->fmt.cmd.c_flags & RAW_NEEDACK){
				TASKDEBUG("Sendind ERROR ACK err_code=%d\n", err_code);
				send_ack((RAW_DATA | RAW_ACKNOWLEDGE), err_code);
				err_code = OK;
			} else {
				TASKDEBUG("waiting until RAW_NEEDACK err_code=%d\n", err_code);
			}
			continue;
		}			
				
		if( rframe_ptr->fmt.cmd.c_cmd != RAW_DATA) {
			TASKDEBUG("c_cmd=%X\n" , rframe_ptr->fmt.cmd.c_cmd);
			err_code = EMOLBADCALL;
			ERROR_PRINT(err_code);
		}else if (rframe_ptr->fmt.cmd.c_rcode != OK ){
			err_code = EMOLBADVALUE;
			ERROR_PRINT(err_code);
		} if ( rframe_ptr->fmt.cmd.c_len > 0){
			memcpy(	&write_buf[ack_off], rframe_ptr->fmt.pay, rframe_ptr->fmt.cmd.c_len); 
		}
		
		if( err_code != 0){
			if( rframe_ptr->fmt.cmd.c_flags & RAW_NEEDACK){
				TASKDEBUG("err_code=%d\n", err_code);
				send_ack((RAW_DATA | RAW_ACKNOWLEDGE), err_code);
				err_code = 0;
			}	
		}else {
			if( rframe_ptr->fmt.cmd.c_flags & RAW_NEEDACK){
//#define FAULT_INYECT_1		
#ifdef FAULT_INYECT_1 
				if ( do_error == 0 && ack_off > 0 ) {
					do_error = 1;
					TASKDEBUG("FAULT INYECTION_1 r_code=%d ack_off=%d\n", err_code,ack_off);
					err_code = 1111;
					send_ack((RAW_DATA | RAW_ACKNOWLEDGE), err_code);
					err_code = 0;
					continue;
				}
#endif // FAULT_INYECT_1
				send_ack((RAW_DATA | RAW_ACKNOWLEDGE), err_code);
			}
//#define FAULT_INYECT_2		
#ifdef FAULT_INYECT_2
			else { // FAULT INYECTION
				if ( do_error == 0 && ack_off > 0 ) {
					do_error = 1;
					TASKDEBUG("FAULT INYECTION_2 err_code=%d ack_off=%d\n", err_code,ack_off);
					err_code = 2222;
					continue;
				}
			}
#endif // FAULT_INYECT_2
			ack_off += rframe_ptr->fmt.cmd.c_len;
			if( rframe_ptr->fmt.cmd.c_flags	& (RAW_EOB | RAW_EOF)){
				wlen = fwrite(write_buf, 1, ack_off , fp);
				TASKDEBUG("EOB wlen=%d\n", wlen);
				ack_off = 0;
				if(wlen < 0) {
					ERROR_EXIT(-errno);
				}
			}
		}
	
		if( rframe_ptr->fmt.cmd.c_flags	& RAW_EOF){ // EOF
			TASKDEBUG("EOF\n");
			fclose(fp);
			break;
		}
		
	}while(TRUE);
		
	return(OK);
}
