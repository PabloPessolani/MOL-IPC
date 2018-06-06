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
	int dcid, src_pid, dst_pid, src_ep, dst_ep, src_nr, dst_nr, ret;
	message m;

    	if ( argc != 4) {
 	        printf( "Usage: %s <dcid> <src_nr> <dst_nr> \n", argv[0] );
 	        exit(1);
	    }

	dcid = atoi(argv[1]);
	if ( dcid < 0 || dcid >= NR_DCS) {
 	        printf( "Invalid dcid [0-%d]\n", NR_DCS-1 );
 	        exit(1);
	    }

	src_nr = atoi(argv[2]);
	dst_nr = atoi(argv[3]);
	
	dst_pid = getpid();
    dst_ep = mnx_bind(dcid, dst_nr);
	if( dst_ep < 0 ) 
		printf("BIND ERROR dst_ep=%d\n",dst_ep);
   	printf("BIND DESTINATION dcid=%d dst_pid=%d dst_nr=%d dst_ep=%d\n",
		dcid,
		dst_pid,
		dst_nr,
		dst_ep);

	if( (src_pid = fork()) != 0 )	{		/* PARENT = DESTINATION */
/*
		printf("Dump procs of DC%d\n",dcid); 
   		ret = mnx_proc_dump(dcid);
      		if( ret != OK){
			printf("PROCDUMP ERROR %d\n",ret); 
			exit(1);
		}

		do {
			src_ep = mnx_getep(src_pid);
			printf("waiting process %d\n",src_pid);
			sleep(1);
			} while(src_ep < 0);

*/ 
printf("RECEIVER: receive \n");
		ret = mnx_receive(ANY , (long) &m);
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

	
		sleep(30);
	    	printf("UNBIND DESTINATION dcid=%d dst_pid=%d dst_nr=%d dst_ep=%d\n",
			dcid,
			dst_pid,
			dst_nr,
			dst_ep);
	    	mnx_unbind(dcid, dst_ep);

	}else{						/* SON = SOURCE		*/
		src_pid = getpid();
    	src_ep =mnx_bind(dcid, src_nr);
		if( src_ep < 0 ) 
			printf("BIND ERROR src_ep=%d\n",src_ep);
   		printf("BIND SOURCE dcid=%d src_pid=%d src_nr=%d src_ep=%d\n",
			dcid,
			src_pid,
			src_nr,
			src_ep);

	  	m.m_type= 0xFF;
		m.m1_i1 = 0x01;
		m.m1_i2 = 0x02;
		m.m1_i3 = 0x03;
   		printf("SEND msg: m_type=%d, m1_i1=%d, m1_i2=%d, m1_i3=%d\n",
			m.m_type,
			m.m1_i1,
			m.m1_i2,
			m.m1_i3);
		
		printf("SENDER pause before SEND\n");
		sleep(10);
		printf("SENDER: send \n");
		
    	ret = mnx_send(dst_ep, (long) &m);
		if( ret != 0 )
		    	printf("SEND ret=%d\n",ret);

		sleep(5);

		printf("UNBIND SOURCE dcid=%d src_pid=%d src_nr=%d src_ep=%d\n",
			dcid,
			src_pid,
			src_nr,
			src_ep);
	    	mnx_unbind(dcid, src_ep);
	}

 exit(0);
 }



