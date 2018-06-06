#include <asm/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/times.h>
#include <time.h>

#include "stub_syscall.h"
#include "./kernel/minix/config.h"
#include "./kernel/minix/ipc.h"
#include "./kernel/minix/kipc.h"


   
void  main ( int argc, char *argv[] )
{
	int dcid, src_pid, dst_pid, dst_ep, src_nr, dst_nr, ret, s_id;
	message m;

    if ( argc != 3) {
 	    printf( "Usage: %s <dcid> <dst_nr> \n", argv[0] );
 	    exit(1);
	}

	dcid = atoi(argv[1]);
	if ( dcid < 0 || dcid >= NR_DCS) {
 	    printf( "Invalid dcid [0-%d]\n", NR_DCS-1 );
 	    exit(1);
	}

	dst_nr = atoi(argv[2]);
	dst_ep = dst_nr;
	
    printf("HARDWARE NOTIFY dst_ep=%d\n", dst_ep);
    ret = mnx_hdw_notify(dcid, dst_ep);
	if( ret != 0 )
	   	printf("HARDWARE NOTIFY ret=%d\n",ret);

	sleep(10);

 }



