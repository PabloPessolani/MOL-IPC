#include <asm/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h> 


#include "../stub_syscall.h"
#include "../kernel/minix/config.h"
#include "../kernel/minix/ipc.h"
#include "../kernel/minix/kipc.h"
#include "../kernel/minix/cmd.h"
#include "../kernel/minix/proxy_sts.h"
#include "../kernel/minix/proxy_usr.h"


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
#ifdef ANULADO
	proxy_hdr_t  header, *h_ptr;
	proxy_payload_t payload;
	message *m_ptr;
#endif /* ANULADO*/ 
	int pid, ret;

	pid = getpid();
	printf("SENDER PROXY: %d\n", pid);

#ifdef ANULADO
	while(1) {
		printf("SENDER PROXY %d: Waiting a message\n", pid	);
		ret = mnx_get2rmt(&header, &payload);
		if( ret != OK) {
			printf("ERROR  mnx_get2rmt %d\n", ret);
			continue;
		}
		h_ptr = &header;
		printf("%d "HDR_FORMAT,pid, HDR_FIELDS(h_ptr)); 

		m_ptr = &payload.pay_msg;
		printf("%d "MSG1_FORMAT,pid, MSG1_FIELDS(m_ptr));
	}
#endif /* ANULADO*/ 

	while(1) sleep(10);

	exit(0);
}

/*----------------------------------------------*/
/*		RECEIVER PROXY			*/
/*----------------------------------------------*/

void receiver_proxy(void)
{
#ifdef ANULADO
	proxy_hdr_t  header, *h_ptr;
	proxy_payload_t payload;
	message *m_ptr;
#endif /* ANULADO*/
	int ret;
	int pid;


	pid = getpid();
	printf("RECEIVER PROXY: %d\n", pid);

#ifdef ANULADO
	sleep(30);

	/* send a message - build the header */
	h_ptr = &header;
	h_ptr->c_cmd	= CMD_SEND_MSG;
	h_ptr->c_src	= 2;
	h_ptr->c_snode = 2;
	h_ptr->c_dst	= 1;
	h_ptr->c_dnode = 0;
	h_ptr->c_dcid	= 0;
	h_ptr->c_rcode= 0;
	printf("%d "HDR_FORMAT,pid, HDR_FIELDS(h_ptr)); 

	/* build a pseudo REMOTE message to send to local */
	m_ptr = &payload.pay_msg;
  	m_ptr->m_source	= 2;
  	m_ptr->m_type	= 0x01;
	m_ptr->m1_i1 	= 0x02;
	m_ptr->m1_i2 	= 0x03;
	m_ptr->m1_i3 	= 0x04;
	printf("%d "MSG1_FORMAT,pid,MSG1_FIELDS(m_ptr)); 

	ret = mnx_put2lcl(&header, &payload);
	printf("RECEIVER PROXY LOOP: %d\n", pid);
#endif /* ANULADO*/ 

	

	while(1) sleep(10);

	exit(0);
}

/*----------------------------------------------*/
/*		MAIN: 			*/
/*----------------------------------------------*/
void  main ( int argc, char *argv[] )
{
	int ret, pid,  spid, rpid, status, pxnr;

    	if ( argc != 3) {
 	        printf( "Usage: %s <px_name> <pxnr>\n", argv[0] );
 	        exit(1);
	    }

	pxnr = atoi(argv[2]);

	pid = getpid();
	printf("MAIN name=%s pxnr=%d pid=%d\n", argv[1], pxnr, pid);

	/* creates SENDER and RECEIVER Proxies as children */
	if ( (spid = fork()) == 0) sender_proxy();
	if ( (rpid = fork()) == 0) receiver_proxy();

	/* register the proxies */
	ret = mnx_proxies_bind(argv[1], pxnr, spid, rpid , MAXCOPYBUF);
	if( ret) MOLERR(ret);

	ret = mnx_proxy_conn(pxnr, CONNECT_SPROXY);
	ret = mnx_proxy_conn(pxnr, CONNECT_RPROXY);
	if( ret) MOLERR(ret);
	
	wait(&status);
	wait(&status);
	exit(0);
 }
