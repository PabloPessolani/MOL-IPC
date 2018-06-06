
/*************************************************************************************
*************************************************************************************
			COMO FUNCIONA RAWMSGQ--MSGQPROXY
El  	msgq_proxy genera 2 procesos hijos, uno para el SENDER y otro para RECEIVER
El   rawmsgq genera 2 hilos,  uno para el SENDER y otro para RECEIVER. Entre ellos necesitan
coordinar el acceso al envío de mensajes.
La bandera WITH_ACKS  habilita los ACKNOWLEDGES

Cuando el SENDER envia un mensaje utiliza la interface ethX  para enviar y si se define
que los envios deben tener ACKNOWLEDGES, debe esperar (COND_WAIT_TO) que el RECEIVER le avise 
(COND_SIGNAL) que el numero de secuencia del frame enviado ya fue recibido por el RECEIVER remoto.
Cuando el RECEIVER recibe un frame remoto que requiere ACK necesita acceder al envio de frames 
de la interface, por lo que se usa un MUTEX para realizar exclusion mutua y acceder a ella .

Mensajes Salientes:
	El msgq_proxy SENDER extrae los mensajes y bloques de datos desde el kernel utilizando mnx_get2rmt()
	y los inserta en la message queue raw_mq_out .
	El rawmsgq SENDER extrae de la cola de mensajes de envío y los envia por la interface ethX
	Si se requiere ACK, debe esperar a que el RECEIVER reciba el mensaje con el numero de seq correcto
	
Mensajes Entrantes:
	El raw RECEIVER recibe un frame y inserta en la raw_mq_in
	El msgq_proxy  RECEIVER extreae el mensaje y datos de la raw_mq_in y lo inserta en el kernel
	utilizando mnx_put2lcl()
	Si recibe un mensaje CMD_NONE lo ignora.
	
Si no hay mensajes para enviar en el kernel, el PROXY Sender da TIMEOUT al realizar mnx_get2rmt()
entonces envia un frame con comando CMD_NONE para demostrar que esta vivo.

Cuando un frame necesita ACK entonces debe enviar en c_flags el bit RAW_NEEDACK
Para indicar ultimo bloque de datos RAW_EOB 
Para indicar frame reenviado RAW_RESEND
Para indicar un frame recibido fuera de secuencia RAW_BADSEQ 	
	
EL PROTOCOLO define que todo frame que no contenga bloques de datos (es decir mensajes solamente)
DEBE tener ACK.
En cambio si se transfieren bloques de datos, se puede establecer cada cuantos frames se requiere ACK
mediante la constante 
#define RAW_ACK_RATE		1 
En este caso establece que cada 1 frame de datos debe haber un ACK. 
En definitiva con la constante en este valor, TODOS los frames deben tener ACK.
Si el ACK no se recibe a tiempo,  se reenvia el frame con flag RAW_RESEND 
			
En el protocolo se establecen reintentos.
	al enviar un frame, si no recibe el ACK entonces debe reenviar el mismo frame.
	de lo contrario, salir del loop para indicar que se cayo el remoto 
	para que el kernel local lo marque como DISCONNECTED 
	Y luego vuelva a reintentar la conexion. (no hay conexion, solo reintentos de envio)
	
*************************************************************************************/

#define _GNU_SOURCE     
#define _MULTI_THREADED
#define  MOL_USERSPACE	1

#include "rawmsgq.h"

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

int raw_mq_in, raw_mq_out;
msgq_buf_t *out_msg_ptr, *in_msg_ptr, *ack_msg_ptr;
struct msqid_ds mq_in_ds;
struct msqid_ds mq_out_ds;
pthread_mutex_t raw_mtx;

struct timeval tv;

struct thread_desc_s {
    pthread_t 		td_thread;
	pthread_cond_t  td_cond;   /* '' */
	pid_t           td_tid;     /* to hold new thread's TID */
	eth_frame_t 	*td_sframe_ptr;
	eth_frame_t 	*td_rframe_ptr;
};
typedef struct thread_desc_s thread_desc_t;

thread_desc_t rdesc;
thread_desc_t sdesc;

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
	
	// memory for msgq INPUT message buffer   (msgq--->raw)
	posix_memalign( (void **) &in_msg_ptr, getpagesize(), sizeof(msgq_buf_t) );
	if (in_msg_ptr == NULL) {
		fprintf(stderr, "posix_memalign in_msg_ptr \n");
		ERROR_EXIT(-errno);
	}

	// memory for msgq OUTPUT message buffer   (raw--->msgq)
	posix_memalign( (void **) &out_msg_ptr, getpagesize(), sizeof(msgq_buf_t) );
	if (out_msg_ptr == NULL) {
		fprintf(stderr, "posix_memalign out_msg_ptr\n");
		ERROR_EXIT(-errno);
	}

	// memory for msgq ACK message buffer   (raw--->msgq)
	posix_memalign( (void **) &ack_msg_ptr, getpagesize(), sizeof(msgq_buf_t) );
	if (ack_msg_ptr == NULL) {
		fprintf(stderr, "posix_memalign ack_msg_ptr\n");
		ERROR_EXIT(-errno);
	}
	
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
	
	pthread_cond_init(&sdesc.td_cond, NULL);
	pthread_cond_init(&rdesc.td_cond, NULL);
	pthread_mutex_init(&raw_mtx, NULL);  
	
 	hdr_plus_cmd = ((unsigned long)&sdesc.td_sframe_ptr->fmt.pay - (unsigned long)&sdesc.td_sframe_ptr->fmt.hdr);
	SVRDEBUG("hdr_plus_cmd=%d\n",hdr_plus_cmd);

	lcl_snd_seq = 1;	// sequence # for the next frame to send
 	lcl_ack_seq = 0;	// sequence # of the last frame received
	rmt_ack_seq = 0;	// sequence # of the last frame acknoledged by remote node 
	return(raw_fd);
}

void DEBUG_hdrcmd(char *funct , eth_frame_t *f_ptr)
{
	_eth_hdr_t *eth_hdr_ptr;
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
	
	frame_ptr->fmt.cmd.c_snd_seq  = lcl_snd_seq;
	frame_ptr->fmt.cmd.c_ack_seq  = lcl_ack_seq;

	SVRDEBUG("lcl_snd_seq=%ld lcl_ack_seq=%ld sframe_len=%d\n", 
		lcl_snd_seq, lcl_ack_seq, sframe_len);
	sent_bytes = sendto(sock_fd, &frame_ptr->raw, sframe_len, 0x00,
					(struct sockaddr*)&rmt_sockaddr, sizeof(rmt_sockaddr));
	DEBUG_hdrcmd(__FUNCTION__, frame_ptr);
	if( sent_bytes < 0) 
		ERROR_RETURN(-errno);
	return(sent_bytes);
}

int enqueue_ack()
{
	int rcode;
	
	SVRDEBUG("lcl_snd_seq=%d lcl_ack_seq=%d\n",lcl_snd_seq,lcl_ack_seq);

	// ENQUEUE into the SENDER QUEUE 
	memset((void*) &ack_msg_ptr->m3.cmd, 0, sizeof(cmd_t));

	ack_msg_ptr->mtype 		  		= RAW_MQ_ACK;
	ack_msg_ptr->m3.cmd.c_cmd 		= CMD_FRAME_ACK; 
	ack_msg_ptr->m3.cmd.c_snd_seq 	= lcl_snd_seq;
	ack_msg_ptr->m3.cmd.c_snd_seq 	= lcl_ack_seq;

	rcode = msgsnd(raw_mq_out , ack_msg_ptr, sizeof(cmd_t), 0); 
	if( rcode < 0) {
		SVRDEBUG("msgsnd errno=%d\n",errno);
		ERROR_RETURN(-errno);
	}
	return(OK);
}
	
int mq_init(int px_id)
{
	/* receiving message queue */
	int qin_base, qout_base; 

	SVRDEBUG("px_id=%d\n",px_id);

	qin_base = QUEUEBASE + (px_id  * 2) + 0;
	raw_mq_in = msgget(qin_base, IPC_CREAT | 0x660);
	if ( raw_mq_in < 0) {
		if ( errno != EEXIST) {
			SVRDEBUG("rerror1 %d\n",raw_mq_in);
			ERROR_RETURN(raw_mq_in);
		}
		SVRDEBUG("Queue IN base key=%d already exists\n",qin_base);
		raw_mq_in = msgget( (qin_base), 0);
		if(raw_mq_in < 0) {
			SVRDEBUG("rerror1 %d\n",raw_mq_in);
			ERROR_RETURN(raw_mq_in);
		}
		SVRDEBUG("msgget OK\n");
	} 
	msgctl(raw_mq_in , IPC_STAT, &mq_in_ds);
	SVRDEBUG("qin_base=%d before raw_mq_in msg_qbytes =%d\n",
		qin_base, mq_in_ds.msg_qbytes);
	mq_in_ds.msg_qbytes = sizeof(msgq_buf_t) * MAX_MQ_ENTRIES;
	msgctl(raw_mq_in , IPC_SET, &mq_in_ds);
	msgctl(raw_mq_in , IPC_STAT, &mq_in_ds);
	SVRDEBUG("qin_base=%d after raw_mq_in msg_qbytes =%d\n",
		qin_base, mq_in_ds.msg_qbytes);

	qout_base = QUEUEBASE + (px_id * 2) + 1;
	raw_mq_out = msgget(qout_base, IPC_CREAT | 0x660);
	if ( raw_mq_out < 0) {
		if ( errno != EEXIST) {
			SVRDEBUG("rerror2 %d\n",raw_mq_out);
			ERROR_RETURN(raw_mq_out);
		}
		SVRDEBUG("Queue OUT base key=%d already exists\n",qout_base);
		raw_mq_out = msgget( (qout_base), 0);
		if(raw_mq_out < 0) {
			SVRDEBUG("rerror2 %d\n",raw_mq_out);
			ERROR_RETURN(raw_mq_out);
		}
		SVRDEBUG("msgget OK\n");
	}

	msgctl(raw_mq_out , IPC_STAT, &mq_out_ds);
	SVRDEBUG("qout_base=%d before raw_mq_out msg_qbytes =%d\n",
		qout_base, mq_out_ds.msg_qbytes);
	mq_out_ds.msg_qbytes = sizeof(msgq_buf_t) * MAX_MQ_ENTRIES;
	msgctl(raw_mq_out , IPC_SET, &mq_out_ds);
	msgctl(raw_mq_out , IPC_STAT, &mq_out_ds);
	SVRDEBUG("qout_base=%d after raw_mq_out msg_qbytes =%d\n",
		qout_base, mq_out_ds.msg_qbytes);

	SVRDEBUG("raw_mq_in=%d raw_mq_out=%d\n",raw_mq_in, raw_mq_out);	
	return(OK);
}	


void check_msgtype(int msgtype)
{
	int cmd;

  	SVRDEBUG("msgtype=%X \n", msgtype); 
	
	switch( sdesc.td_sframe_ptr->fmt.cmd.c_cmd){
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
			assert(sdesc.td_sframe_ptr->fmt.cmd.c_cmd == CMD_FRAME_ACK);
			break;
		default:
			assert(msgtype & RAW_MQ_HDR);
			if( sdesc.td_sframe_ptr->fmt.cmd.c_cmd & CMD_ACKNOWLEDGE){
				assert(msgtype & RAW_MQ_ACK);
				cmd = (sdesc.td_sframe_ptr->fmt.cmd.c_cmd & ~(CMD_ACKNOWLEDGE));	
			} else {
				cmd = sdesc.td_sframe_ptr->fmt.cmd.c_cmd;
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
	memcpy((u8_t*)&frame_ptr->fmt.hdr.dst, (u8_t*)&rmt_mac[0], ETH_ALEN);

	// 	Source MAC 
	memcpy((u8_t*)&frame_ptr->fmt.hdr.src, (u8_t*)&lcl_mac[0], ETH_ALEN);
	
	//   Protocolo Field
	proto = (ether_type_t) htons(ETH_M3IPC); 
	memcpy((u8_t*)&frame_ptr->fmt.hdr.proto, (u8_t*)&proto , sizeof(ether_type_t));
}

int sts_to_rproxy(int mt, int code)
{
	int rcode;
	
	SVRDEBUG("mt=%X code=%d\n", mt, code );
	in_msg_ptr->mtype = mt; 
	in_msg_ptr->m3.cmd.c_rcode = code; 	
	rcode = msgsnd(raw_mq_in , in_msg_ptr, sizeof(cmd_t), 0); 
	if( rcode < 0) {
		SVRDEBUG("msgsnd errno=%d\n",errno);
		ERROR_EXIT(-errno);
	}
}
	
void *SENDER_th(void *arg) 
{
	int rlen, send_retries, msgtype, cmd, wait_again, send_off;
	int  sent_bytes, sframe_len, remain, frame_count, ret; 
	int sock_fd, *s_ptr;
	struct timespec ts;

		
	s_ptr = (int *) arg; 
	sock_fd = *s_ptr;
  	SVRDEBUG("sock_fd=%d s_ptr=%p hdr_plus_cmd=%d\n",sock_fd, s_ptr, hdr_plus_cmd); 
	
	// memory for frame to SEND
	posix_memalign( (void **) &sdesc.td_sframe_ptr, getpagesize(), sizeof(eth_frame_t) );
	if (sdesc.td_sframe_ptr == NULL) {
		fprintf(stderr, "posix_memalign sdesc.td_sframe_ptr \n");
		ERROR_EXIT(-errno);
	}

	// memory for frame to RECEIVE
	posix_memalign( (void **) &sdesc.td_rframe_ptr, getpagesize(), sizeof(eth_frame_t) );
	if (sdesc.td_rframe_ptr == NULL) {
		fprintf(stderr, "posix_memalign sdesc.td_rframe_ptr, \n");
		ERROR_EXIT(-errno);
	}
	
	set_frame_hdr(sdesc.td_sframe_ptr);

rs_restart:
  	SVRDEBUG("Waiting on output Message queue\n"); 
	while( ( rlen = msgrcv(raw_mq_out, out_msg_ptr, sizeof(msgq_buf_t), 0 , 0 )) >= 0) {

rs_resend:	
		// copy header from msgq buffer to frame buffer 
		memcpy( &sdesc.td_sframe_ptr->fmt.cmd, &out_msg_ptr->m3.cmd, sizeof(cmd_t)); 

		msgtype = out_msg_ptr->mtype;
		SVRDEBUG("rlen=%d c_cmd=%X msgtype=%X\n", rlen, sdesc.td_sframe_ptr->fmt.cmd.c_cmd, msgtype);
		// check for correct msgq type 
		check_msgtype(msgtype);

		// set protocol specific fields
		send_retries = RAW_MAX_RETRIES; 
		sdesc.td_sframe_ptr->fmt.cmd.c_flags = 0;

		//----------------------------------------
		//		send HEADER 
		//----------------------------------------
		MTX_LOCK(raw_mtx);
#ifdef WITH_ACKS			
		do {
			sdesc.td_sframe_ptr->fmt.cmd.c_flags |= RAW_NEEDACK;
#endif // WITH_ACKS			

//			if ( msgtype == RAW_MQ_ACK) { // RECEIVER thread needs to send and ACK to remote  
//				assert(sdesc.td_sframe_ptr->fmt.cmd.c_cmd == CMD_FRAME_ACK);
//				if( sdesc.td_sframe_ptr->fmt.cmd.c_snd_seq 	< lcl_snd_seq){
//					MTX_UNLOCK(raw_mtx); // discard the ACK because a previous frame was  sent
//					goto rs_restart; 	// with updated sequence numbers 
//				}	
//			}
			// set the updated sequence numbers 
			sdesc.td_sframe_ptr->fmt.cmd.c_snd_seq 	= lcl_snd_seq;
			sdesc.td_sframe_ptr->fmt.cmd.c_ack_seq 	= lcl_ack_seq;  // last received sequence # by this node 
			
			// Send the frame  with the HEADER 
			sframe_len = hdr_plus_cmd;
			sent_bytes = send_frame(sdesc.td_sframe_ptr, sock_fd, sframe_len);
			SVRDEBUG("sframe_len=%d sent_bytes=%d\n", sframe_len, sent_bytes);
			if( sent_bytes < 0) {
				MTX_UNLOCK(raw_mtx);
				sts_to_rproxy(RAW_MQ_ERR, sent_bytes);
				sleep(RAW_WAIT4ETH);
				sts_to_rproxy(RAW_MQ_OK, OK);
				goto rs_restart;
			}

#ifdef WITH_ACKS
			// do not wait for ACK 
			if(!(sdesc.td_sframe_ptr->fmt.cmd.c_flags & RAW_NEEDACK)){
				lcl_snd_seq--; // to compensate final increment
				send_retries = 0;
				continue;
			}
			
			// wait for ACK 
			wait_again = RAW_FRAMES2WAIT;
			do {
				// WAIT that the sender wakeup me when a frame has arrived 
				SVRDEBUG("rmt_ack_seq=%ld lcl_snd_seq=%ld\n",rmt_ack_seq, lcl_snd_seq);
				if( rmt_ack_seq == lcl_snd_seq){
					SVRDEBUG("ACK OK lcl_snd_seq=%ld\n",lcl_snd_seq);
					send_retries = 0;
					break;
				}
				
				COND_WAIT_TO(ret, sdesc.td_cond, raw_mtx, RAW_RCV_TIMEOUT);				
				// no ACK frame has arrived yet 
				if( ret == ETIMEDOUT){
					SVRDEBUG("wait_again=%d\n",wait_again);
					wait_again -= 5;
					if( wait_again > 0){
						continue;
					}
				} else {
					if (ret < 0 ) ERROR_EXIT(ret);
				}
			} while(wait_again > 0);
			
			if( rmt_ack_seq != lcl_snd_seq) { // compare sent sequence against frame receive ack seq 
				SVRDEBUG("rmt_ack_seq=%ld lcl_snd_seq=%ld\n", rmt_ack_seq, lcl_snd_seq);
//				SVRDEBUG("wait_again=%d\n",wait_again);
//				wait_again--;
//				if( wait_again > 0)	
//					continue;					
				SVRDEBUG("send_retries=%d\n",send_retries);
				sdesc.td_sframe_ptr->fmt.cmd.c_flags	|= RAW_RESEND;
				send_retries--;
				if(send_retries > 0) continue;
				// copy the header again to CANCEL 	
				memcpy( &sdesc.td_sframe_ptr->fmt.cmd, &out_msg_ptr->m3.cmd, sizeof(cmd_t)); 
				sdesc.td_sframe_ptr->fmt.cmd.c_flags	|= RAW_CANCEL;
				sframe_len = hdr_plus_cmd;
				sent_bytes = send_frame(sdesc.td_sframe_ptr, sock_fd, sframe_len);
				goto rs_resend;
			}else{
				SVRDEBUG("ACK OK rmt_ack_seq=%ld lcl_snd_seq=%ld\n", rmt_ack_seq, lcl_snd_seq);
				send_retries = 0;
				break;
			}		
		}while ( send_retries > 0);
#endif // WITH_ACKS
		lcl_snd_seq++;
		MTX_UNLOCK(raw_mtx);
		
		if( (sdesc.td_sframe_ptr->fmt.cmd.c_cmd !=  CMD_COPYIN_DATA) && 
			(sdesc.td_sframe_ptr->fmt.cmd.c_cmd !=  CMD_COPYOUT_DATA))
			continue;
		
		//----------------------------------------
		//		send PAYLOAD  
		//----------------------------------------
		rlen -= ((char*) &out_msg_ptr->m3.pay - (char*) &out_msg_ptr->m3.cmd);
		SVRDEBUG("rlen=%d\n", rlen);
		assert(rlen > 0);
		remain = rlen;
		
		frame_count=1;
		send_off = 0;
		do { // loop of block of data 
			send_retries = RAW_MAX_RETRIES; 
			MTX_LOCK(raw_mtx);
data_again:		
		//	sdesc.td_sframe_ptr->fmt.cmd.c_cmd      = CMD_PAYLOAD_DATA;
			sdesc.td_sframe_ptr->fmt.cmd.c_flags 	|= RAW_DATA;
			SVRDEBUG("remain=%d lcl_snd_seq=%ld lcl_ack_seq=%ld send_off=%ld\n",
				remain, lcl_snd_seq, lcl_ack_seq, send_off);
			sdesc.td_sframe_ptr->fmt.cmd.c_snd_seq  	= lcl_snd_seq;
			sdesc.td_sframe_ptr->fmt.cmd.c_ack_seq  	= lcl_ack_seq;
			if ( remain < (ETH_FRAME_LEN - hdr_plus_cmd)) {
				sdesc.td_sframe_ptr->fmt.cmd.c_len 		= remain;
				sdesc.td_sframe_ptr->fmt.cmd.c_flags	|= (RAW_EOB); //| RAW_NEEDACK); 
			} else {
				sdesc.td_sframe_ptr->fmt.cmd.c_len 	= (ETH_FRAME_LEN - hdr_plus_cmd);
				SVRDEBUG("frame_count=%d\n",frame_count);
				if( (frame_count%RAW_ACK_RATE) == 0)
					sdesc.td_sframe_ptr->fmt.cmd.c_flags |=  RAW_NEEDACK; 
			}

			memcpy(sdesc.td_sframe_ptr->fmt.pay, 
					&out_msg_ptr->m3.pay.pay_data[send_off], 
					out_msg_ptr->m3.cmd.c_len); 

			// send data  
			sframe_len = (hdr_plus_cmd + sdesc.td_sframe_ptr->fmt.cmd.c_len);
			sent_bytes = send_frame(sdesc.td_sframe_ptr, sock_fd, sframe_len);
			SVRDEBUG("sframe_len=%d sent_bytes=%d\n", sframe_len, sent_bytes);
			if( sent_bytes < 0) {
				MTX_UNLOCK(raw_mtx);
				sts_to_rproxy(RAW_MQ_ERR, sent_bytes);
				sleep(RAW_WAIT4ETH);
				sts_to_rproxy(RAW_MQ_OK, OK);
				goto rs_restart;
			}
			DEBUG_hdrcmd(__FUNCTION__,sdesc.td_sframe_ptr);

#ifdef WITH_ACKS
			if(!(sdesc.td_sframe_ptr->fmt.cmd.c_flags &  RAW_NEEDACK)){
				lcl_snd_seq--; // to compensate final increment
				send_retries = 0;
				continue;
			}
			
			do 	{  // loop of wait for acknowledge
				wait_again = RAW_FRAMES2WAIT;
				do { // loop of wait for the correct sequence acknowledge 
					// WAIT that the sender wakeup me when a frame has arrived 
					if( rmt_ack_seq == lcl_snd_seq){
						SVRDEBUG("ACK OK lcl_snd_seq=%ld\n",lcl_snd_seq);
						send_retries = 0;
						break;
					}
					
					COND_WAIT_TO(ret, sdesc.td_cond, raw_mtx, RAW_RCV_TIMEOUT);				
					// no ACK frame has arrived yet 
					if( ret == ETIMEDOUT){
						SVRDEBUG("wait_again=%d\n",wait_again);
						wait_again -= 5;
						if( wait_again > 0)
							continue;
					} else {
						if (ret < 0 ) ERROR_EXIT(ret);
					}
				} while(wait_again > 0);

				if( rmt_ack_seq != lcl_snd_seq) { 
					SVRDEBUG("rmt_ack_seq=%d lcl_snd_seq=%d%\n", rmt_ack_seq, lcl_snd_seq);
//					SVRDEBUG("wait_again=%d\n",wait_again);
//					wait_again--;
//					if( wait_again > 0)	continue;
					SVRDEBUG("send_retries=%d\n",send_retries);
					sdesc.td_sframe_ptr->fmt.cmd.c_flags	|= RAW_RESEND;
					send_retries--;
					if(send_retries > 0) continue;
					// copy the header again to CANCEL 	
					memcpy( &sdesc.td_sframe_ptr->fmt.cmd, &out_msg_ptr->m3.cmd, sizeof(cmd_t)); 
					sdesc.td_sframe_ptr->fmt.cmd.c_flags	|= RAW_CANCEL;
					sframe_len = hdr_plus_cmd;
					sent_bytes = send_frame(sdesc.td_sframe_ptr, sock_fd, sframe_len);
					goto data_again;
				}else{
					SVRDEBUG("ACK OK lcl_snd_seq=%ld\n",lcl_snd_seq);
					send_retries = 0;
					break;
				}
			} while ( send_retries > 0);

#endif // WITH_ACKS			
			lcl_snd_seq++;
			MTX_UNLOCK(raw_mtx);
			frame_count++;				
			send_off += sdesc.td_sframe_ptr->fmt.cmd.c_len;
			remain  -= sdesc.td_sframe_ptr->fmt.cmd.c_len;
		}while (remain > 0);
	} // endless loop 
	return(OK);
}

void *RECEIVER_th(void *arg) 
{
	int total_bytes, msgtype, cmd, rcvd_bytes, rcode;
	char *data_ptr;
	int sframe_len;
	unsigned long int sent_bytes;
	int sock_fd, *s_ptr;
	struct timespec ts;

	
	s_ptr = (int *) arg; 
	sock_fd = *s_ptr;
  	SVRDEBUG("sock_fd=%d s_ptr=%p hdr_plus_cmd=%d\n",sock_fd, s_ptr, hdr_plus_cmd); 
	
	// memory for frame to SEND
	posix_memalign( (void **) &rdesc.td_sframe_ptr, getpagesize(), sizeof(eth_frame_t) );
	if (rdesc.td_sframe_ptr == NULL) {
		fprintf(stderr, "posix_memalign rdesc.td_sframe_ptr \n");
		ERROR_EXIT(-errno);
	}

	// memory for frame to RECEIVE
	posix_memalign( (void **) &rdesc.td_rframe_ptr, getpagesize(), sizeof(eth_frame_t) );
	if (rdesc.td_rframe_ptr == NULL) {
		fprintf(stderr, "posix_memalign rdesc.td_rframe_ptr \n");
		ERROR_EXIT(-errno);
	}	
	
	while( TRUE) {
		memset( (void *) &in_msg_ptr->m3.cmd, 0, sizeof(cmd_t));
		memset( (void *) &in_msg_ptr->m3.pay.pay_data, 0, MAXCOPYBUF);
rcv_again:	
		total_bytes = 0;
		msgtype = 0;
		data_ptr = NULL;
		// clear input message queue command 
		do 	{
		  	SVRDEBUG("Waiting on Ethernet sock_fd=%d\n",sock_fd); 
			rcvd_bytes = recvfrom(sock_fd, &rdesc.td_rframe_ptr->raw, ETH_FRAME_LEN, 0, NULL, NULL);
			if( rcvd_bytes < 0){
				if( (-errno) == EMOLAGAIN){
					continue;
				}
				SVRDEBUG("%s\n",strerror(-errno));
				ERROR_EXIT(-errno);
			}
			SVRDEBUG("rcvd_bytes=%d\n", rcvd_bytes);
			DEBUG_hdrcmd(__FUNCTION__, rdesc.td_rframe_ptr);

#ifdef WITH_ACKS
			//------------------------- sequence number checking -------------------
			MTX_LOCK(raw_mtx);
				
			SVRDEBUG("rmt_ack_seq=%ld lcl_ack_seq=%ld\n", rmt_ack_seq, lcl_ack_seq);
			// check remote acknowlege sequence 
			if( (rmt_ack_seq + 1) == rdesc.td_rframe_ptr->fmt.cmd.c_ack_seq){ // correct sequence 
				rmt_ack_seq++; 					// update remote acknowledge 
				SVRDEBUG("correct acknowledge sequence rmt_ack_seq=%ld\n", rmt_ack_seq);
				COND_SIGNAL(sdesc.td_cond);		// notify sender 		
			}
			
			// check local acknowlege sequence: resent frame  
			if( (lcl_ack_seq + 1) > rdesc.td_rframe_ptr->fmt.cmd.c_snd_seq){
				SVRDEBUG("resent frame lcl_ack_seq=%ld\n", lcl_ack_seq);
				MTX_UNLOCK(raw_mtx);	
				continue; // discard frame 
			}
			
			if( rdesc.td_rframe_ptr->fmt.cmd.c_flags & RAW_NEEDACK){
//				rcode = enqueue_ack(sock_fd);
				SVRDEBUG(" send ACK \n");
				memcpy( &rdesc.td_sframe_ptr->fmt.cmd, &rdesc.td_rframe_ptr->fmt.cmd, sizeof(cmd_t)); 
				rdesc.td_sframe_ptr->fmt.cmd.c_cmd	= CMD_FRAME_ACK;
				rdesc.td_sframe_ptr->fmt.cmd.c_flags = 0;
				sframe_len = hdr_plus_cmd;
				sent_bytes = send_frame(rdesc.td_sframe_ptr, sock_fd, sframe_len);
			}
				
			// check local acknowlege sequence: frame out of sequence  
			if ( (lcl_ack_seq + 1) < rdesc.td_rframe_ptr->fmt.cmd.c_snd_seq){	
				SVRDEBUG("out of sequence lcl_ack_seq=%ld\n", lcl_ack_seq);
				MTX_UNLOCK(raw_mtx);	
				continue;
			}
			
			if( rdesc.td_rframe_ptr->fmt.cmd.c_cmd == CMD_FRAME_ACK){
				SVRDEBUG("CMD_FRAME_ACK ignored c_snd_seq=%ld c_ack_seq=%d\n", 
					rdesc.td_rframe_ptr->fmt.cmd.c_snd_seq,
					rdesc.td_rframe_ptr->fmt.cmd.c_ack_seq
					);
				MTX_UNLOCK(raw_mtx);
				continue;
			}
			// check local acknowlege sequence: correct sequence   
			lcl_ack_seq++;
			SVRDEBUG("correct sequence lcl_ack_seq=%ld\n", lcl_ack_seq);
			
			MTX_UNLOCK(raw_mtx);

#endif // WITH_ACKS			
			
			// ---------------------------------------------------------------------------	
						
			//  PAYLOAD DATA RECEIVED 
			if ( rdesc.td_rframe_ptr->fmt.cmd.c_flags & RAW_DATA){
				SVRDEBUG("c_len=%d\n", rdesc.td_rframe_ptr->fmt.cmd.c_len);

				assert (  (rdesc.td_rframe_ptr->fmt.cmd.c_cmd == CMD_COPYIN_DATA)
						||(rdesc.td_rframe_ptr->fmt.cmd.c_cmd == CMD_COPYOUT_DATA));
				assert(rdesc.td_rframe_ptr->fmt.cmd.c_len > 0);
				assert( data_ptr != NULL);
//		if( data_ptr == (char *) &in_msg_ptr->m3.pay){
//			total_bytes += sizeof(cmd_t);
//			SVRDEBUG("total_bytes=%d\n", total_bytes);
//		}
				assert( (total_bytes + rdesc.td_rframe_ptr->fmt.cmd.c_len) <= MAXCOPYBUF);
			
				memcpy((void *) data_ptr, (void *) &rdesc.td_rframe_ptr->fmt.pay, 
								sizeof(rdesc.td_rframe_ptr->fmt.cmd.c_len));
				data_ptr += rdesc.td_rframe_ptr->fmt.cmd.c_len;
				total_bytes += rdesc.td_rframe_ptr->fmt.cmd.c_len;
				SVRDEBUG("total_bytes=%d\n", total_bytes);
				
				if( rdesc.td_rframe_ptr->fmt.cmd.c_flags & RAW_EOB ){
					SVRDEBUG("RAW_EOB c_flags=%X\n", rdesc.td_rframe_ptr->fmt.cmd.c_flags);
					if( in_msg_ptr->m3.cmd.c_u.cu_vcopy.v_bytes != total_bytes ){
						SVRDEBUG("DISCARD v_bytes=%d total_bytes=%d\n", 
								in_msg_ptr->m3.cmd.c_u.cu_vcopy.v_bytes, total_bytes);
						msgtype = 0; // Do not enqueue 
						continue; // DISCARD SET OF FRAME 
					}
					if( in_msg_ptr->m3.cmd.c_cmd == CMD_COPYIN_DATA) {	 
						msgtype = (RAW_MQ_HDR | RAW_MQ_PAY);
					}else if ( in_msg_ptr->m3.cmd.c_cmd == CMD_COPYOUT_DATA) {
						msgtype = (RAW_MQ_HDR | RAW_MQ_PAY | RAW_MQ_ACK);
					}else{
						SVRDEBUG("BAD CMD cmd=%X\n", in_msg_ptr->m3.cmd.c_cmd);
						msgtype = 0;
						continue; // discard SET OF frames
					}
					total_bytes += sizeof(cmd_t);
					SVRDEBUG("total_bytes=%d\n", total_bytes);
				}	
			} else { // HEADER RECEIVED 
				memcpy((void *) &in_msg_ptr->m3.cmd, (void *) &rdesc.td_rframe_ptr->fmt.cmd, sizeof(cmd_t));

				cmd = (rdesc.td_rframe_ptr->fmt.cmd.c_cmd & ~(CMD_ACKNOWLEDGE));	
				if( cmd > CMD_BATCHED_CMD || cmd < 0) {
					SVRDEBUG("c_cmd=%X cmd=%X\n", rdesc.td_rframe_ptr->fmt.cmd.c_cmd, cmd);
					ERROR_EXIT(EMOLINVAL);
				}
				switch(	rdesc.td_rframe_ptr->fmt.cmd.c_cmd)	{
					case CMD_COPYIN_DATA:
						SVRDEBUG("c_cmd=%X c_len=%d\n", 
							rdesc.td_rframe_ptr->fmt.cmd.c_cmd, rdesc.td_rframe_ptr->fmt.cmd.c_len);
						// header + payload
						data_ptr = (char *) &in_msg_ptr->m3.pay;
						break;
					case CMD_COPYOUT_DATA:
						SVRDEBUG("c_cmd=%X c_len=%d\n", 
							rdesc.td_rframe_ptr->fmt.cmd.c_cmd, rdesc.td_rframe_ptr->fmt.cmd.c_len);
						// header + payload
						data_ptr = (char *) &in_msg_ptr->m3.pay;
						break;
					default:
						SVRDEBUG("c_cmd=%X\n", rdesc.td_rframe_ptr->fmt.cmd.c_cmd);
						// copy HEADER 
						memcpy((void *) &in_msg_ptr->m3.cmd, (void *) &rdesc.td_rframe_ptr->fmt.cmd, sizeof(cmd_t));
						total_bytes = sizeof(cmd_t);
						if( rdesc.td_rframe_ptr->fmt.cmd.c_cmd & CMD_ACKNOWLEDGE){
							msgtype = (RAW_MQ_HDR | RAW_MQ_ACK);
						}else{						
							msgtype = RAW_MQ_HDR;
						}
						break;
				} 	
			}
		}while(	msgtype == 0);

#ifdef WITH_ACKS
		// send ACK to remote
		if( rdesc.td_rframe_ptr->fmt.cmd.c_flags & RAW_NEEDACK){
			SVRDEBUG(" send ACK \n");
			MTX_LOCK(raw_mtx);
			memcpy( &rdesc.td_sframe_ptr->fmt.cmd, &rdesc.td_rframe_ptr->fmt.cmd, sizeof(cmd_t)); 
			rdesc.td_sframe_ptr->fmt.cmd.c_cmd	= CMD_FRAME_ACK;
			rdesc.td_sframe_ptr->fmt.cmd.c_flags = 0;
			sframe_len = hdr_plus_cmd;
			sent_bytes = send_frame(rdesc.td_sframe_ptr, sock_fd, sframe_len);
			MTX_UNLOCK(raw_mtx);
		}
#endif // WITH_ACKS		
		
		// ENQUEUE into the RECEIVER PROXY QUEUE 
		SVRDEBUG("msgtype=%X total_bytes=%d\n", msgtype, total_bytes);
		in_msg_ptr->mtype = msgtype; 
		rcode = msgsnd(raw_mq_in , in_msg_ptr, total_bytes, 0); 
		if( rcode < 0) {
			SVRDEBUG("msgsnd errno=%d\n",errno);
			if( (-errno) == EMOLAGAIN){ // may be nothing is waiting in the other side 
				continue;
			}
			ERROR_EXIT(-errno);
		}
				
	}
	return(OK);
}
  
/*===========================================================================*
 *				   main 				    					 *
 *===========================================================================*/
int main (int argc, char *argv[] )
{
	int   ret;
	int raw_fd;
//	struct hostent *rmt_he;
		
	if(argc != 4) {
		fprintf (stderr,"usage: %s <rmt_node> <px_id> <ifname> \n", argv[0]);
		exit(1);
	}

	SVRDEBUG("%s: node:%s px_id:%s iface:%s \n", argv[0], argv[1] , argv[2], argv[3]);
		
	mq_init(atoi(argv[2]));	
	
	raw_fd = raw_init(argv[3], argv[1]);
 	SVRDEBUG("raw_fd=%d &raw_fd=%p\n",raw_fd , &raw_fd);  
	
	fill_socket();

	SVRDEBUG("MAIN: pthread_create RAW RECEIVER \n");
	if (ret = pthread_create(&rdesc.td_thread, NULL, RECEIVER_th,(void*)&raw_fd )) {
		ERROR_EXIT(ret);
	}	
	
	SVRDEBUG("MAIN: pthread_create RAW SENDER \n");
	if (ret = pthread_create(&sdesc.td_thread, NULL, SENDER_th,(void*)&raw_fd )) {
		ERROR_EXIT(ret);
	}
		
  	SVRDEBUG("WAITING CHILDREN raw_fd=%d !!!\n", raw_fd);  
	pthread_join ( rdesc.td_thread, NULL );
	pthread_join ( sdesc.td_thread, NULL );
	
  	SVRDEBUG("EXITING!!!\n");  

}
