#include <asm/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>      // for time()
#include <string.h>
#include <syslog.h> 

#include <sys/types.h>
#include <sys/wait.h> 
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/msg.h>

#include "stub_syscall.h"
#include "./kernel/minix/config.h"
#include "./kernel/minix/ipc.h"
#include "./kernel/minix/kipc.h"
#include "./kernel/minix/cmd.h"
#include "./kernel/minix/proxy_usr.h"

#define	MAXBUF	(sizeof(msgq_buf_t) - sizeof(int))
#define QUEUEBASE	7000


typedef struct {
		int		mtype;
		proxy_hdr_t  header;
		proxy_payload_t payload;
		} msgq_buf_t;

msgq_buf_t buf_in, buf_out;

struct msqid_ds mq_snd_ds;
struct msqid_ds mq_rcv_ds;
int mq_snd, mq_rcv;

#define MOLERR(rcode) \
 do { \
     	printf("ERROR: %s:%s:%u: rcode=%d\n",__FUNCTION__ ,__LINE__,rcode); \
	exit(rcode); \
 }while(0)
 
int nodeid;

/*----------------------------------------------*/
/*		SENDER PROXY			*/
/*----------------------------------------------*/

void sender_proxy(void)
{
	proxy_hdr_t  *h_ptr;
	message *m_ptr;
	int pid, ret;

	pid = getpid();


	printf("SENDER PROXY: %d\n", pid);
	sleep(30);
	buf_out.mtype=0x0001;

	/* FIRST MESSAGE: CMD_SENDREC */
		printf("SENDER PROXY %d: Waiting a message from KERNEL\n", pid);
		ret = mnx_get2rmt(&buf_out.header, &buf_out.payload);
		if( ret != OK) {
			printf("ERROR  mnx_get2rmt %d\n", ret);
		}
		h_ptr = &buf_out.header;
		printf("%d "HDR_FORMAT,pid, HDR_FIELDS(h_ptr)); 

		m_ptr = &buf_out.payload.pay_msg;
		printf("%d "MSG1_FORMAT,pid, MSG1_FIELDS(m_ptr)); 

		ret = msgsnd(mq_snd, &buf_out, MAXBUF, 0);

	/* SECOND MESSAGE: CMD_SEND_ACK */
		printf("SENDER PROXY %d: Waiting a message from KERNEL\n", pid);
		ret = mnx_get2rmt(&buf_out.header, &buf_out.payload);
		if( ret != OK) {
			printf("ERROR  mnx_get2rmt %d\n", ret);
		}
		h_ptr = &buf_out.header;
		printf("%d "HDR_FORMAT,pid, HDR_FIELDS(h_ptr)); 

		m_ptr = &buf_out.payload.pay_msg;
		printf("%d "MSG1_FORMAT,pid, MSG1_FIELDS(m_ptr)); 

		ret = msgsnd(mq_snd, &buf_out, MAXBUF, 0);

	sleep(60);
	exit(0);
}

/*----------------------------------------------*/
/*		RECEIVER PROXY			*/
/*----------------------------------------------*/

void receiver_proxy(void)
{

	int ret;
	int pid;
	proxy_hdr_t  *h_ptr;
	message *m_ptr ;

	pid = getpid();

	printf("RECEIVER PROXY: %d\n", pid);
	sleep(5);
	buf_in.mtype=0x0001;

	/*FIRST REPLY: CMD_SEND_MSG */
		printf("RECEIVER PROXY %d: Waiting a message from MESSAGE QUEUE\n", pid	);

		ret = msgrcv(mq_rcv, &buf_in, MAXBUF, 0 , 0 );

		h_ptr = &buf_in.header;
		printf("%d "HDR_FORMAT,pid, HDR_FIELDS(h_ptr)); 

		m_ptr = &buf_in.payload.pay_msg;
		printf("%d "MSG1_FORMAT,pid, MSG1_FIELDS(m_ptr)); 

		ret = mnx_put2lcl(&buf_in.header, &buf_in.payload);

	sleep(60);
	exit(0);
}

/*----------------------------------------------*/
/*		MAIN: 			*/
/*----------------------------------------------*/
void  main ( int argc, char *argv[] )
{
	int ret, pid,  spid, rpid, status;
	proxy_hdr_t  *h_ptr;
	message *m_ptr;

    	if ( argc != 3) {
 	        printf( "Usage: %s <nodeid> <nodename>\n", argv[0] );
 	        exit(1);
	    }

	nodeid = atoi(argv[1]);
	if ( nodeid < 0 || nodeid >= NR_NODES) {
 	        printf( "Invalid nodeid [0-%d]\n", NR_NODES-1 );
 	        exit(1);
	    }

	pid = getpid();
	printf("MAIN pid = %d\n", pid);

	mq_snd = msgget(QUEUEBASE, IPC_CREAT | 0x660);
	if ( mq_snd < 0) {
		if ( errno != EEXIST) {
			printf("rerror1 %d\n",mq_snd);
			exit(1);
		}
		printf( "The queue with key=%d already exists\n",QUEUEBASE);
		mq_snd = msgget( (QUEUEBASE), 0);
		if(mq_snd < 0) {
			printf("rerror1 %d\n",mq_snd);
			exit(1);
		}
		printf("msgget OK\n");
	} 

	msgctl(mq_snd , IPC_STAT, &mq_snd_ds);
	printf("before mq_snd msg_qbytes =%d\n",mq_snd_ds.msg_qbytes);
	mq_snd_ds.msg_qbytes = MAXBUF;
	msgctl(mq_snd , IPC_SET, &mq_snd_ds);
	msgctl(mq_snd , IPC_STAT, &mq_snd_ds);
	printf("after mq_snd msg_qbytes =%d\n",mq_snd_ds.msg_qbytes);

	mq_rcv = msgget(QUEUEBASE+1, IPC_CREAT | 0x660);
	if ( mq_rcv < 0) {
		if ( errno != EEXIST) {
			printf("rerror2 %d\n",mq_rcv);
			exit(1);
		}
		printf( "The queue with key=%d already exists\n",QUEUEBASE+1);
		mq_rcv = msgget( (QUEUEBASE+1), 0);
		if(mq_rcv < 0) {
			printf("rerror2 %d\n",mq_rcv);
			exit(1);
		}
		printf("msgget OK\n");
	}

	msgctl(mq_rcv , IPC_STAT, &mq_rcv_ds);
	printf("before mq_rcv msg_qbytes =%d\n",mq_rcv_ds.msg_qbytes);
	mq_rcv_ds.msg_qbytes = MAXBUF;
	msgctl(mq_rcv , IPC_SET, &mq_rcv_ds);
	msgctl(mq_rcv , IPC_STAT, &mq_rcv_ds);
	printf("after mq_rcv msg_qbytes =%d\n",mq_rcv_ds.msg_qbytes);

	/* creates SENDER and RECEIVER Proxies as children */
	if ( (spid = fork()) == 0) sender_proxy();
	if ( (rpid = fork()) == 0) receiver_proxy();

	/* register the proxies */
	ret = mnx_proxy_bind(nodeid, argv[2], spid, rpid);
	if( ret) MOLERR(ret);
	
	buf_in.mtype=0x0001;
	buf_out.mtype=0x0001;

	/*RECEIVE CMD_SENDREC */
		ret = msgrcv(mq_snd, &buf_in, MAXBUF, 0 , 0 );

		h_ptr = &buf_in.header;
		printf("%d "HDR_FORMAT,pid, HDR_FIELDS(h_ptr)); 

		m_ptr = &buf_in.payload.pay_msg;
		printf("%d "MSG1_FORMAT,pid, MSG1_FIELDS(m_ptr)); 

	/*REPLY WITH CMD_SEND_MSG */
		/* BUILD THE REPLY HEADER */
		buf_out.header.c_cmd 	= CMD_SEND_MSG;
		buf_out.header.c_dcid 	= h_ptr->c_dcid;
		buf_out.header.c_src 	= h_ptr->c_dst;
		buf_out.header.c_dst 	= h_ptr->c_src;
		buf_out.header.c_snode	= h_ptr->c_dnode;
		buf_out.header.c_dnode  = h_ptr->c_snode;
		buf_out.header.c_len 	= h_ptr->c_len;
		buf_out.header.c_rcode	= OK;

		/* BUILD THE REPLY MESSAGE */
		buf_out.payload.pay_msg.m_source = h_ptr->c_dst;
		buf_out.payload.pay_msg.m_type   = m_ptr->m_type;
		buf_out.payload.pay_msg.m1_i1    = m_ptr->m1_i1;
		buf_out.payload.pay_msg.m1_i2    = m_ptr->m1_i2;
		buf_out.payload.pay_msg.m1_i3    = m_ptr->m1_i3;
		buf_out.payload.pay_msg.m1_p1    = 0;
		buf_out.payload.pay_msg.m1_p2    = 0;
		buf_out.payload.pay_msg.m1_p3    = 0;

		ret = msgsnd(mq_rcv, &buf_out, MAXBUF, 0); 
	
//	ret = mnx_proxy_unbind(nodeid, spid, rpid);
//	if( ret) MOLERR(ret);
	
	wait(&status);
	wait(&status);
	exit(0);
 }
