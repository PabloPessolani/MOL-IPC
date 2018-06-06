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
	int dcid, endpoint, nodeid, ret, ep, pid, bk_ep, bk_nr;

    if ( argc != 5) {
 	        printf( "Usage: %s <dcid> <endpoint> <bkup_nr> <nodeid> \n", argv[0] );
 	       exit(1);
	   }

		
	dcid = atoi(argv[1]);
	if ( dcid < 0 || dcid >= NR_DCS) {
 	        printf( "Invalid dcid [0-%d]\n", NR_DCS-1 );
 	        exit(1);
	    }

	endpoint = atoi(argv[2]);
	bk_nr = atoi(argv[3]);
	nodeid = atoi(argv[4]);
	
	if( (pid = fork()) == 0) { /* CHILD - BACKUP */
		while(1) sleep(1);
	}
	
	bk_ep = mnx_bkupbind(dcid,pid,bk_nr,nodeid);
	if( bk_ep < 0){
		printf("mnx_bkupbind error=%d\n",bk_ep); 
		exit(1);
	}
	
	    printf("Binding BACK UP process of dcid=%d from node=%d with endpoint=%d \n",dcid,nodeid,bk_ep);

	wait(&ret);
	
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



