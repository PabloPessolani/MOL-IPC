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
	mnx_unbind(); \
	exit(rcode); \
 }while(0)
 
int dcid, rmt_ep, src_ep, nodeid;
int lcl_nr, lcl_ep;

/*----------------------------------------------*/
/*		SENDER PROXY			*/
/*----------------------------------------------*/

void sender_proxy(void)
{
	int ret;
	proxy_hdr  header, *h_ptr;
	proxy_payload payload;
	message *m_ptr;

printf("SENDER PROXY\n");
	ret = mnx_get2rmt(&header, &payload);

h_ptr = &header;
printf(HDR_FORMAT,HDR_FIELDS(h_ptr)); 

m_ptr = &payload.pay_msg;
printf(MSG1_FORMAT,MSG1_FIELDS(m_ptr)); 

	/* unregister the proxy */
	mnx_proxy_unbind();

	sleep(10);
	exit(0);
}

/*----------------------------------------------*/
/*		RECEIVER PROXY			*/
/*----------------------------------------------*/

void receiver_proxy(void)
{
	int ret;
	proxy_hdr  header, *h_ptr;
	proxy_payload payload;
	message *m_ptr;

printf("RECEIVER PROXY\n");
	sleep(2);

	/* send a message - build the header */
	h_ptr = &header;
	h_ptr->hdr_type	= PXY_MESSAGE;
	h_ptr->hdr_src	= rmt_ep;
	h_ptr->hdr_node = nodeid;
	h_ptr->hdr_dcid	= dcid;
	h_ptr->hdr_dst	= lcl_ep;
	h_ptr->hdr_flags= 0;
	h_ptr->hdr_len	= sizeof(message);
printf(HDR_FORMAT,HDR_FIELDS(h_ptr)); 

	/* build a pseudo REMOTE message to send to local */
	m_ptr = &payload.pay_msg;
  	m_ptr->m_source	= src_ep;
  	m_ptr->m_type	= 0x01;
	m_ptr->m1_i1 	= 0x02;
	m_ptr->m1_i2 	= 0x03;
	m_ptr->m1_i3 	= 0x04;

	ret = mnx_put2lcl(&header, &payload);
	if( ret != OK) MOLERR(ret);
printf(MSG1_FORMAT,MSG1_FIELDS(m_ptr)); 


	sleep(10);

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

	nodeid = 2;				/* FIXED NODEID != 0*/
	
	pid = getpid();
    	lcl_ep  = mnx_bind(dcid, lcl_nr);
	if( lcl_ep < 0 ) MOLERR(lcl_ep);
   	printf("Binding Local process %d to DC%d with lcl_nr=%d lcl_ep=%d\n",pid,dcid,lcl_nr, lcl_nr);


	/* register the remote process */
    	src_ep = mnx_rmtbind(dcid, rmt_ep, nodeid);
    	if(src_ep != rmt_ep) {
	    	mnx_unbind();
		MOLERR(src_ep);
		}
    	printf("Binding REMOTE process of to DC%d from node=%d with rmt_ep=%d \n",dcid,nodeid,rmt_ep);

	/* creates SENDER and RECEIVER Proxies as children */
	if ( (spid = fork()) == 0) sender_proxy();
	if ( (rpid = fork()) == 0) receiver_proxy();

	/* register the proxies */
	mnx_proxy_bind(spid, rpid);

	/* receive a message from REMOTE */
	m_ptr  		= &m_in;
    	ret = mnx_receive(ANY, (long) m_ptr);
     	if( ret != OK){
		printf("RECEIVE ERROR %d\n",ret); 
		exit(1);
	}
	printf(MSG1_FORMAT,MSG1_FIELDS(m_ptr));  


	sleep(15);

    mnx_unbind();

	/* unregister the remote process */
	ret = mnx_rmtunbind(dcid, rmt_ep, nodeid);
	if( ret != OK) MOLERR(ret);

 exit(0);
 }
