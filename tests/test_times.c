#include <asm/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "stub_syscall.h"
#include "./kernel/minix/config.h"
#include "./kernel/minix/ipc.h"
#include "./kernel/minix/kipc.h"
#include "./kernel/minix/com.h"

   
void  main ( int argc, char *argv[] )
{
	int dcid, clt_pid, svr_pid, clt_ep, svr_ep, clt_nr, svr_nr, ret;
	message m, *m_ptr;

    	if ( argc != 3) {
 	        printf( "Usage: %s <dcid> <clt_nr> \n", argv[0] );
 	        exit(1);
	    }

	dcid = atoi(argv[1]);
	if ( dcid < 0 || dcid >= NR_DCS) {
 	        printf( "Invalid dcid [0-%d]\n", NR_DCS-1 );
 	        exit(1);
	    }

	clt_nr = atoi(argv[2]);
	svr_ep = svr_nr = SYSTEM;
	
	clt_pid = getpid();
	clt_ep =	mnx_bind(dcid, clt_nr);
	if( clt_ep < 0 ) 
		printf("BIND ERROR clt_ep=%d\n",clt_ep);
   	printf("BIND CLIENT dcid=%d clt_pid=%d clt_nr=%d clt_ep=%d\n",
			dcid,
			clt_pid,
			clt_nr,
			clt_ep);

	printf("CLIENT pause before SENDREC\n");
	sleep(2); 

	m.m_type= SYS_TIMES;
	m.m4_l1 = 0;
	m.m4_l2 = 0;
	m.m4_l3 = 0;
	m.m4_l4 = 0;
	m.m4_l5 = 0;
	m_ptr = &m;

   	printf("SENDREC "MSG4_FORMAT,MSG4_FIELDS(m_ptr));

    	ret = mnx_sendrec(svr_ep, (long) &m);
	if( ret != 0 )
	   	printf("SENDREC ret=%d\n",ret);

   	printf("REPLY "MSG4_FORMAT,MSG4_FIELDS(m_ptr));
 }



