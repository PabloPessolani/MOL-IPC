#include <asm/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "./kernel/minix/config.h"
#include "./kernel/minix/ipc.h"
#include "./kernel/minix/kipc.h"
#include "./kernel/minix/const.h"
#include "stub_syscall.h"
   
void  main ( int argc, char *argv[] )
{
	int dcid, pid, ret, oper, caller_nr, caller_ep, dst_node, endpoint;

    	if ( argc != 6) {
 	        printf( "Usage: %s  <oper>  <migrating_pid> <dcid> <migrating_ep> <dst_nodeid> \n", argv[0] );
			printf("\tMIGR_START=0\n\tMIGR_COMMIT=1\n\tMIGR_ROLLBACK=2\n");
 	        exit(1);
	    }

	oper     = atoi(argv[1]);
	if( oper < 0 || oper > 2) { printf("oper=%d\n",oper); exit(1);}
	pid      = atoi(argv[2]);
	dcid 	 = atoi(argv[3]);
	endpoint = atoi(argv[4]);
	dst_node = atoi(argv[5]);
	
	printf("oper=%d pid=%d dcid=%d endpoint=%d dst_nodeid=%d\n",  oper, pid, dcid, endpoint, dst_node);

	switch(oper) {
		case MIGR_START:
				ret = mnx_migr_start(dcid, endpoint);
				break;
		case MIGR_COMMIT:
				ret = mnx_migr_commit(pid, dcid, endpoint,dst_node);
				break;
		case MIGR_ROLLBACK:
				ret = mnx_migr_rollback(dcid, endpoint);
				break;
		default:
				printf("oper=%d\n",oper); 
				exit(1);
				break;
	}
	
	if(ret) printf("ERROR=%d\n", ret);
	

 }



