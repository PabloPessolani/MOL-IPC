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

    	if ( argc != 4) {
 	        printf( "Usage: %s <node_name> <nodeid> <pxnr>\n", argv[0] );
 	        exit(1);
	    }

	nodeid = atoi(argv[2]);
	pxnr = atoi(argv[3]);

	printf("node_name=%s nodeid=%d pxnr=%d \n",argv[1], nodeid, pxnr);

	/* register the proxies */
	ret = mnx_node_up(argv[1], nodeid, pxnr);
	if( ret) MOLERR(ret);
	
	exit(0);
 }
