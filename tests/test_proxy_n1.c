/********************************************************************/
/*				test_proxy2.c								*/
/* the parent process is a local MINIX process that starts SENDER PROXY and	*/
/* RECEIVER PROXY as children. 									*/
/* The parent binds a not existing remote process, and waits to receive a msg	 */
/* The RECEIVER proxy builds a pseudo remote message and send to parent         */
/* Once the parente receives the message it reply with a MSG_ACK msg to remote    */
/* the parent process is unblocked and resumes its execution 			      */
/********************************************************************/

#include <asm/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "stub_syscall.h"
#include "./kernel/minix/config.h"
#include "./kernel/minix/ipc.h"
#include "./kernel/minix/kipc.h"
#include "./kernel/minix/proxy.h"

#define MOLERR(rcode) \
 do { \
     	printf("ERROR: %s:%s:%u: rcode=%d\n",__FUNCTION__ ,__LINE__,rcode); \
	mnx_unbind(dcid, lcl_ep); \
	exit(rcode); \
 }while(0)
 
int dcid, rmt_ep, src_ep;
int lcl_nodeid, rmt_nodeid;
int lcl_nr, lcl_ep;

/*----------------------------------------------*/
/*		SENDER PROXY			*/
/*----------------------------------------------*/

void sender_proxy(void)
{
	int ret;
	int pid;
	proxy_hdr  header, *h_ptr;
	proxy_payload payload;
	message *m_ptr;

	pid = getpid();

	sleep (2);
	
printf("SENDER PROXY: %d\n",pid);
	ret = mnx_get2rmt(&header, &payload);

h_ptr = &header;
printf("%d "HDR_FORMAT,pid, HDR_FIELDS(h_ptr)); 

m_ptr = &payload.pay_msg;
printf("%d "MSG1_FORMAT,pid, MSG1_FIELDS(m_ptr)); 

	sleep(30);
	/* unregister the proxy */
	mnx_proxy_unbind();

	exit(0);
}

/*----------------------------------------------*/
/*		RECEIVER PROXY			*/
/*----------------------------------------------*/

void receiver_proxy(void)
{
	int ret;
	int pid;
	proxy_hdr  header, *h_ptr;
	proxy_payload payload;
	message *m_ptr;

	pid = getpid();

printf("RECEIVER PROXY: %d\n", pid);
	sleep(3);

	/* send a message - build the header */
	h_ptr = &header;
	h_ptr->hdr_type	= CMD_NOTIFY;
	h_ptr->hdr_src	= rmt_ep;
	h_ptr->hdr_snode = rmt_nodeid;
	h_ptr->hdr_dnode = lcl_nodeid;
	h_ptr->hdr_dcid	= dcid;
	h_ptr->hdr_dst	= lcl_ep;
	h_ptr->hdr_rcode= 0;
printf("%d "HDR_FORMAT,pid, HDR_FIELDS(h_ptr)); 

	ret = mnx_put2lcl(&header, &payload);

	sleep(30);

	/* unregister the proxy */
	mnx_proxy_unbind();

	exit(0);
}

/*----------------------------------------------*/
/*		MAIN: RECEIVER			*/
/*----------------------------------------------*/
void  main ( int argc, char *argv[] )
{
	int ret, pid,  spid, rpid, status;
	message m_in, m_out, *m_ptr;

    	if ( argc != 4) {
 	        printf( "Usage: %s <dcid> <lcl_nr> <rmt_ep> \n", argv[0] );
 	        exit(1);
	    }

	dcid = atoi(argv[1]);
	if ( dcid < 0 || dcid >= NR_DCS) {
 	        printf( "Invalid dcid [0-%d]\n", NR_DCS-1 );
 	        exit(1);
	    }

	lcl_nr = atoi(argv[2]);
	rmt_ep = atoi(argv[3]);

	lcl_nodeid = 0;	
	rmt_nodeid = 1;	
	
	pid = getpid();
    lcl_ep  = mnx_bind(dcid, lcl_nr);
	if( lcl_ep < 0 ) MOLERR(lcl_ep);
   	printf("Binding Local process %d to DC%d with lcl_nr=%d lcl_ep=%d\n",pid,dcid,lcl_nr, lcl_nr);


	/* creates SENDER and RECEIVER Proxies as children */
	if ( (spid = fork()) == 0) sender_proxy();
	if ( (rpid = fork()) == 0) receiver_proxy();

	/* register the proxies */
	mnx_proxy_bind(spid, rpid);

	/* register the remote process */
    src_ep = mnx_rmtbind(dcid, rmt_ep, 	rmt_nodeid);
    	if(src_ep != rmt_ep) {
	    	mnx_unbind(dcid, lcl_ep);
		MOLERR(src_ep);
		}
    printf("Binding REMOTE process of to DC%d from node=%d with rmt_ep=%d \n",dcid,rmt_nodeid,rmt_ep);
	
	sleep(10);
	
    ret = mnx_receive(ANY, (long) &m_in);
    if( ret != OK){
		printf("RECEIVE ERROR %d\n",ret); 
		exit(1);
	}
    printf("Message receive by remote and MSG ARRIVED sent!\n");
	
	sleep(5);

	/* unregister the remote process */
	ret = mnx_rmtunbind(dcid, rmt_ep, rmt_nodeid);
	if( ret != OK) MOLERR(ret);

   	mnx_unbind(dcid, lcl_ep);

 exit(0);
 }
