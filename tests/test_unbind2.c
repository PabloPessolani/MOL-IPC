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

	if( (src_pid = fork()) != 0 )	{		/* PARENT = SENDER */

		printf("SENDER NEVER SEND and pause before UNBIND o kill it\n");
		sleep(10); 

	    	printf("UNBIND SENDER dcid=%d dst_pid=%d dst_nr=%d dst_ep=%d\n",
			dcid,
			dst_pid,
			dst_nr,
			dst_ep);
	    	mnx_unbind(dcid, dst_ep);


	}else{						/* SON = RECEIVER		*/
		src_pid = getpid();
    		src_ep =mnx_bind(dcid, src_nr);
		if( src_ep < 0 ) 
			printf("BIND ERROR src_ep=%d\n",src_ep);
   		printf("BIND RECEIVER dcid=%d src_pid=%d src_nr=%d src_ep=%d\n",
			dcid,
			src_pid,
			src_nr,
			src_ep);

    		ret = mnx_receive(dst_ep, (long) &m);
		if( ret != 0 )
		    	printf("SEND ret=%d\n",ret);

    		printf("UNBIND RECEIVER dcid=%d src_pid=%d src_nr=%d src_ep=%d\n",
			dcid,
			src_pid,
			src_nr,
			src_ep);
	    	mnx_unbind(dcid, src_ep);
	}

 exit(0);
 }



