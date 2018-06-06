#include <asm/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "stub_syscall.h"
#include "./kernel/minix/config.h"
#include "./kernel/minix/ipc.h"
#include "./kernel/minix/kipc.h"

   
void  main ( int argc, char *argv[] )
{
	int dcid, clt_pid, svr_pid, clt_ep, svr_ep, clt_nr, svr_nr, ret;
	message m;

    	if ( argc != 4) {
 	        printf( "Usage: %s <dcid>  <svr_nr> <clt_nr> \n", argv[0] );
 	        exit(1);
	    }

	dcid = atoi(argv[1]);
	if ( dcid < 0 || dcid >= NR_DCS) {
 	        printf( "Invalid dcid [0-%d]\n", NR_DCS-1 );
 	        exit(1);
	    }

	svr_nr = atoi(argv[2]);
	clt_nr = atoi(argv[3]);
	
	svr_pid = getpid();
   	svr_ep =  mnx_bind(dcid, svr_nr);
	if( svr_ep < 0 ) 
		printf("BIND ERROR svr_ep=%d\n",svr_ep);
    printf("BIND SERVER dcid=%d svr_pid=%d svr_nr=%d svr_ep=%d\n",
		dcid,
		svr_pid,
		svr_nr,
		svr_ep);

	if( (clt_pid = fork()) != 0 )	{		/* PARENT = SERVER */
		printf("Dump procs of DC%d\n",dcid); 
   		ret = mnx_proc_dump(dcid);
      		if( ret != OK){
			printf("PROCDUMP ERROR %d\n",ret); 
			exit(1);
		}

		printf("SERVER pause before RECEIVE\n");
		sleep(2);

	    	ret = mnx_receive(ANY, (long) &m);
     		if( ret != OK){
			printf("RECEIVE ERROR %d\n",ret); 
			exit(1);
		}
   		printf("RECEIVE msg: m_source=%d, m_type=%d, m1_i1=%d, m1_i2=%d, m1_i3=%d\n",
			m.m_source,
			m.m_type,
			m.m1_i1,
			m.m1_i2,
			m.m1_i3);

		m.m_type = 0xF0; /*pseudo REPLY type*/
    		ret = mnx_send(m.m_source, (long) &m);
		if( ret != 0 )
		    	printf("SEND ret=%d\n",ret);
		
	    	printf("UNBIND SERVER dcid=%d svr_pid=%d svr_nr=%d svr_ep=%d\n",
			dcid,
			svr_pid,
			svr_nr,
			svr_ep);
	    	mnx_unbind(dcid, svr_ep);

	}else{						/* SON = CLIENT		*/
		clt_pid = getpid();
    	clt_ep =mnx_bind(dcid, clt_nr);
		if( clt_ep < 0 ) 
			printf("BIND ERROR clt_ep=%d\n",clt_ep);
   		printf("BIND CLIENT dcid=%d clt_pid=%d clt_nr=%d clt_ep=%d\n",
			dcid,
			clt_pid,
			clt_nr,
			clt_ep);

	  	m.m_type= 0x0F;
		m.m1_i1 = 0x01;
		m.m1_i2 = 0x02;
		m.m1_i3 = 0x03;
   		printf("SEND msg: m_type=%d, m1_i1=%d, m1_i2=%d, m1_i3=%d\n",
			m.m_type,
			m.m1_i1,
			m.m1_i2,
			m.m1_i3);

    		ret = mnx_sendrec(svr_ep, (long) &m);
		if( ret != 0 )
		    	printf("SENDREC ret=%d\n",ret);

   		printf("REPLY msg: m_source=%d, m_type=%d, m1_i1=%d, m1_i2=%d, m1_i3=%d\n",
			m.m_source,
			m.m_type,
			m.m1_i1,
			m.m1_i2,
			m.m1_i3);

    		printf("UNBIND CLIENT dcid=%d clt_pid=%d clt_nr=%d clt_ep=%d\n",
			dcid,
			clt_pid,
			clt_nr,
			clt_ep);
	    	mnx_unbind(dcid, clt_ep);
	}

 exit(0);
 }



