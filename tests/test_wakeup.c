#include <asm/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "./kernel/minix/config.h"
#include "./kernel/minix/ipc.h"
#include "./kernel/minix/kipc.h"
#include "stub_syscall.h"
   
void  main ( int argc, char *argv[] )
{
	int dcid, pid, p_nr, ret, ep;

    	if ( argc != 3) {
 	        printf( "Usage: %s <dcid> <p_nr> \n", argv[0] );
 	        exit(1);
	    }

	dcid = atoi(argv[1]);
	if ( dcid < 0 || dcid >= NR_DCS) {
 	        printf( "Invalid dcid [0-%d]\n", NR_DCS-1 );
 	        exit(1);
	    }

	p_nr = atoi(argv[2]);

    printf("mnx_wakeup dcid=%d p_nr=%d\n",dcid,p_nr);
    ret = mnx_wakeup(dcid, p_nr);
    printf("mnx_wakeup ret=%d\n",ret);
	
	sleep(10);
/*
	printf("Dump procs of DC%d\n",dcid); 
   	ret = mnx_proc_dump(dcid);
      	if( ret != OK){
		printf("mol_proc_dump error=%d\n",ret); 
		exit(1);
	}
*/

//	mnx_unbind(dcid,ep);
 }



