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
	int nodeid, ret;

    	if ( argc != 1) {
 	        printf( "Usage: %s\n", argv[0] );
 	        exit(1);
	}


    	printf("Ending DVS\n");
	ret = mnx_dvs_end();
	if(ret<0)
    		printf(" ERROR %d: Ending DVS\n",ret);
			
 }



