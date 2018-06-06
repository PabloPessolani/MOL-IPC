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
    	int ret;
  	
	int dcid;

    	if ( argc != 2) {
 	        printf( "Usage: %s <dcid>\n", argv[0] );
 	        exit(1);
	    }

	dcid = atoi(argv[1]);
	if ( dcid < 0 || dcid >= NR_DCS) {
 	        printf( "Invalid dcid [0-%d]\n", NR_DCS-1 );
 	        exit(1);
	    }


	printf("Dump procs of DC%d\n",dcid); 
   	ret = mnx_proc_dump(dcid);
      if( ret != OK){
		printf("mol_proc_dump error=%d\n",ret); 
		exit(1);
	}
	exit(0);		
 }

