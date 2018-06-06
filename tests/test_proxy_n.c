/********************************************************************/
/*				test_proxy.c								*/
/* the parent process is a local MINIX process that starts SENDER PROXY and	*/
/* RECEIVER PROXY as children. 									*/
/* The parent binds a not existing remote process, and send a message to it	 */
/* The SENDER proxy "gets" the message to send to ar pseudo remote process */
/* The RECEIVER proxy builds a pseudo SEND ACK message and send it to the  */
/* the parent process that is unblocked and resumes its execution 			 */
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
#include "./kernel/minix/molerrno.h"

#define MOLERR(rcode) \
 do { \
     	printf("ERROR: %s:%s:%u: rcode=%d\n",__FUNCTION__ ,__LINE__,rcode); \
	mnx_unbind(dcid, lcl_ep);\
	exit(rcode); \
 }while(0)
 
int dcid, rmt_ep, src_ep;
int lcl_nodeid,rmt_nodeid;
int lcl_nr, lcl_ep;

/*----------------------------------------------*/
/*		SENDER PROXY			*/
/*----------------------------------------------*/

void sender_proxy(void)
{
	int ret;
	int pid;
	proxy_hdr_t  header, *h_ptr;
	proxy_payload_t payload;
	message *m_ptr;

	pid = getpid();

printf("SENDER PROXY: %d\n",pid);
	ret = mnx_get2rmt(&header, &payload);

h_ptr = &header;
printf("%d "HDR_FORMAT,pid, HDR_FIELDS(h_ptr)); 

m_ptr = &payload.pay_msg;
printf("%d "MSG1_FORMAT,pid, MSG1_FIELDS(m_ptr)); 

	sleep(30);

	exit(0);
}

/*----------------------------------------------*/
/*		RECEIVER PROXY			*/
/*----------------------------------------------*/

void receiver_proxy(void)
{
	int ret;
	int pid;
	proxy_hdr_t  header, *h_ptr;
	proxy_payload_t payload;
	message *m_ptr;

	pid = getpid();

printf("RECEIVER PROXY: %d\n", pid);
	sleep(10);

	/* send a message - build the header */
	h_ptr = &header;
	h_ptr->hdr_type	= CMD_SEND_ACK;
	h_ptr->hdr_src	= rmt_ep;
	h_ptr->hdr_snode = rmt_nodeid;
	h_ptr->hdr_dnode = lcl_nodeid;
	h_ptr->hdr_dcid  = dcid;
	h_ptr->hdr_dst	= lcl_ep;
	h_ptr->hdr_rcode= 0;
printf("%d "HDR_FORMAT,pid, HDR_FIELDS(h_ptr)); 

//	ret = mnx_put2lcl(&header, &payload);

	sleep(30);

	/* unregister the proxy */

	exit(0);
}

/*----------------------------------------------*/
/*		MAIN: SENDER			*/
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

	lcl_nodeid = 1;				
	rmt_nodeid = 2;				
	
	pid = getpid();
    lcl_ep  = mnx_bind(dcid, lcl_nr);
	if( lcl_ep < 0 ) MOLERR(lcl_ep);
   	printf("Binding Local process %d to DC%d with lcl_nr=%d lcl_ep=%d\n",pid,dcid,lcl_nr, lcl_nr);


	/* creates SENDER and RECEIVER Proxies as children */
	if ( (spid = fork()) == 0) sender_proxy();
	if ( (rpid = fork()) == 0) receiver_proxy();

	/* register the proxies */
	mnx_proxy_bind(spid, rpid);

	mnx_add_node(dcid, rmt_nodeid);

	/* register the remote process */
    src_ep = mnx_rmtbind(dcid, rmt_ep, rmt_nodeid);
    	if(src_ep != rmt_ep) {
	    	mnx_unbind(dcid, lcl_ep);
		exit(src_ep);
		}
   	printf("Binding REMOTE process of to DC%d from node=%d with rmt_ep=%d \n",dcid,rmt_nodeid,rmt_ep);


    ret = mnx_notify(rmt_ep);
	if( ret != 0 )
	    	printf("NOTIFY ret=%d\n",ret);

    printf("Message sent to remote and ACK received!!\n");
	ret = mnx_proc_dump(dcid);
    if( ret != OK){
			printf("PROCDUMP ERROR %d\n",ret); 
			exit(1);
		}	
		
	sleep(60);

	/* deregister the remote process */
	ret = mnx_rmtunbind(dcid, rmt_ep, rmt_nodeid);
	if( ret != OK) MOLERR(ret);

	mnx_del_node(dcid, rmt_nodeid);

	mnx_proxy_unbind();

	waitpid(spid, &status, 0);
	waitpid(rpid, &status, 0);


 exit(0);
 }
