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
	int dcid, src_pid, dst_pid, src_ep, dst_ep, src_nr, dst_nr, ret, s_id;
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
	dst_ep = dst_nr;
	
	src_pid = getpid();
    	src_ep =mnx_bind(dcid, src_nr);
	if( src_ep < 0 ) 
		printf("BIND ERROR src_ep=%d\n",src_ep);

    printf("NOTIFY dst_ep=%d\n", dst_ep);
    ret = mnx_notify(dst_ep);
	if( ret != 0 )
	   	printf("NOTIFY ret=%d\n",ret);

sleep(40);

 }



