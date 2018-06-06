#include <asm/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "./kernel/minix/config.h"
#include "./kernel/minix/const.h"
#include "./kernel/minix/types.h"
#include "./kernel/minix/ipc.h"
#include "./kernel/minix/kipc.h"
#include "./kernel/minix/DVS_usr.h"
#include "./kernel/minix/const.h"
#include "stub_syscall.h"

   
void  main ( int argc, char *argv[] )
{
	int ret;
	dvs_usr_t  DVS, *DVS_ptr;

    	if ( argc != 1) {
 	        printf( "Usage: %s \n", argv[0] );
 	        exit(1);
	    }


	ret = mnx_getDVSinfo(&DVS);

	if(ret < (DVS_NO_INIT)) {
 	        printf( "return error=%d \n", ret );
 	        exit(1);
	}

	printf("getDVSinfo local_nodeid=%d\n", ret);
	DVS_ptr = &DVS;
	printf(DVS_USR_FORMAT,DVS_USR_FIELDS(DVS_ptr));

 }



