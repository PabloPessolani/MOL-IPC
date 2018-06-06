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
	int dcid, nodeid, ret;

    	if ( argc != 3) {
 	        printf( "Usage: %s <dcid> <nodeid>\n", argv[0] );
 	        exit(1);
	    }

	dcid = atoi(argv[1]);
	if ( dcid < 0 || dcid >= NR_DCS) {
 	        printf( "Invalid dcid [0-%d]\n", NR_DCS-1 );
 	        exit(1);
	    }

	nodeid = atoi(argv[2]);
	if ( nodeid < 0 || nodeid >= NR_NODES) {
 	        printf( "Invalid nodeid [0-%d]\n", NR_NODES-1 );
 	        exit(1);
	    }

    	printf("Adding node %d to DC %d... \n", nodeid, dcid);

	ret = mnx_add_node(dcid, nodeid);
	if(ret)
	    	printf("ERROR %d Adding node %d to DC %d... \n",ret, nodeid, dcid);
 }



