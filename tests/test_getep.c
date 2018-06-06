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
	int dcid, pid, endpoint, p_nr, ret;
	message m;

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

	printf("ANTES\n");
	endpoint =   mnx_getep(pid);
	printf("GETEP ERROR %d\n",endpoint);
	
    printf("Binding process %d to DC%d with p_nr=%d\n",pid,dcid,p_nr);
    mnx_bind(dcid, p_nr);

	printf("DESPUES\n");
	endpoint =   mnx_getep(pid);
	if( endpoint < 0 ) {
		printf("GETEP ERROR %d\n",endpoint);
		exit(endpoint);
	}
	printf("Process endpoint=%d\n",endpoint);

sleep(10);

	printf("Dump procs of DC%d\n",dcid); 
   	ret = mnx_proc_dump(dcid);
      	if( ret != OK){
		printf("mol_proc_dump error=%d\n",ret); 
		exit(1);
	}

//    	printf("Unbinding process %d from DC%d with endpoint=%d\n",pid,dcid,endpoint);
//    	mnx_unbind(dcid, endpoint);

 }



