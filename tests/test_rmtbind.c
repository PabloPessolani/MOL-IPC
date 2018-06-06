#include <asm/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "./kernel/minix/config.h"
#include "./kernel/minix/ipc.h"
#include "./kernel/minix/kipc.h"
#include "../stub_syscall.h"

   
void  main ( int argc, char *argv[] )
{
	int dcid, endpoint, nodeid, ret, ep;

    	if ( argc != 4) {
 	        printf( "Usage: %s <dcid> <endpoint> <nodeid> \n", argv[0] );
 	        exit(1);
	    }

	dcid = atoi(argv[1]);
	if ( dcid < 0 || dcid >= NR_DCS) {
 	        printf( "Invalid dcid [0-%d]\n", NR_DCS-1 );
 	        exit(1);
	    }

	endpoint = atoi(argv[2]);
	nodeid = atoi(argv[3]);
	
    printf("Binding REMOTE process of dcid=%d from node=%d with endpoint=%d \n",dcid,nodeid,endpoint);
    ep = mnx_rmtbind(dcid, "SoyRemoto", endpoint, nodeid);


	
/*
	printf("Dump procs of DC%d\n",dcid); 
   	ret = mnx_proc_dump(dcid);
      	if( ret != OK){
		printf("mol_proc_dump error=%d\n",ret); 
		exit(1);
	}
*/

//    printf("Unbinding REMOTE process of dcid=%d from node=%d with endpoint=%d \n",dcid,nodeid,endpoint);
//    ret = mnx_rmtunbind(dcid, endpoint, nodeid);

 }



