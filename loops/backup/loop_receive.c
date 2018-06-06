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
	int dcid, pid, p_nr, dst_ep, ret, ep, i, loops;
	message m;

    if ( argc != 4) {
 	        printf( "Usage: %s <dcid> <p_nr> <loops>\n", argv[0] );
 	        exit(1);
	    }

	dcid = atoi(argv[1]);
	if ( dcid < 0 || dcid >= NR_VMS) {
 	        printf( "Invalid dcid [0-%d]\n", NR_VMS-1 );
 	        exit(1);
	    }

	p_nr = atoi(argv[2]);
	loops  = atoi(argv[3]);

	pid = getpid();

    dst_ep = mnx_bind(dcid, p_nr);
	if( dst_ep < 0 ) {
		printf("BIND ERROR %d\n",dst_ep);
		exit(dst_ep);
	}
    printf("Binding LOCAL process %d to VM%d with p_nr=%d\n",pid,dcid,p_nr);

	for (i = 0 ; i < loops; i++) {
    	ret = mnx_receive(ANY, (long) &m);
     	if( ret != OK)
			printf("RECEIVE ERROR %d\n",ret);
	}
	
    printf("Unbinding process %d from VM%d with src_nr=%d\n",pid,dcid,p_nr);
   	mnx_unbind(dcid,dst_ep);

 }



