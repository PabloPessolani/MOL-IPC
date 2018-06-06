#include <asm/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h> 


#include "../stub_syscall.h"
#include "../kernel/minix/config.h"
#include "../kernel/minix/ipc.h"
#include "../kernel/minix/kipc.h"


#define MOLERR(rcode) \
 do { \
     	printf("ERROR: %s:%s:%u: rcode=%d\n",__FUNCTION__ ,__LINE__,rcode); \
	exit(rcode); \
 }while(0)
 

/*----------------------------------------------*/
/*		MAIN: 			*/
/*----------------------------------------------*/
void  main ( int argc, char *argv[] )
{
	int ret,  nodeid, pxnr;

    	if ( argc != 2) {
 	        printf( "Usage: %s <nodeid>\n", argv[0] );
 	        exit(1);
	    }

	nodeid = atoi(argv[1]);

	printf("nodeid=%d \n", nodeid);

	/* deregister the node */
	ret = mnx_node_down(nodeid);
	if( ret) MOLERR(ret);
	
	exit(0);
 }
