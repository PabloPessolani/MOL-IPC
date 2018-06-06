#include <asm/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "./kernel/minix/config.h"
#include "./kernel/minix/const.h"
#include "./kernel/minix/types.h"
#include "./kernel/minix/ipc.h"
#include "./kernel/minix/kipc.h"
#include "./kernel/minix/node_usr.h"
#include "stub_syscall.h"

   
void  main ( int argc, char *argv[] )
{
	int nodeid, ret;
	struct  node_usr node_usr, *node_usr_ptr;

    	if ( argc != 2) {
 	        printf( "Usage: %s <node> \n", argv[0] );
 	        exit(1);
	    }

	nodeid = atoi(argv[1]);

	node_usr_ptr  = &node_usr;

	printf("getnodeinfo nodeid=%d\n", nodeid);
	ret = mnx_getnodeinfo(nodeid, node_usr_ptr);

	if(ret < 0) {
 	        printf( "return error=%d \n", ret );
 	        exit(1);
	}

	printf(NODE_USR_FORMAT,NODE_USR_FIELDS(node_usr_ptr));
	printf(NODE_TIME_FORMAT,NODE_TIME_FIELDS(node_usr_ptr));

 }



