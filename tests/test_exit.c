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
	pid = getpid();

    printf("PARENT Binding process %d to DC%d with p_nr=%d\n",pid,dcid,p_nr);
    ep = mnx_bind(dcid, p_nr);

	if(pid = fork()) {
	    	printf("PARENT Sleeping until exit\n");
		sleep(5);
	}else{
		pid = getpid();
/*
	    printf("CHILD Binding process %d to DC%d with p_nr=%d\n",pid,dcid,p_nr+1);
    		ep = mnx_bind(dcid, p_nr+1);
		printf("Dump procs of DC%d\n",dcid); 
   		ret = mnx_proc_dump(dcid);
      		if( ret != OK){
			printf("mol_proc_dump error=%d\n",ret); 
			exit(1);
		}
*/
	    	printf("CHILD Sleeping waiting parent exit\n");
		sleep(1000);
	}    	
 }




