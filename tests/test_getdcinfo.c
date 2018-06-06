#include <asm/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#define _GNU_SOURCE
#define  __USE_GNU
#include <sched.h>
#define cpumask_t cpu_set_t
#include "./kernel/minix/config.h"
#include "./kernel/minix/const.h"
#include "./kernel/minix/types.h"
#include "./kernel/minix/ipc.h"
#include "./kernel/minix/kipc.h"
#include "./kernel/minix/dc_usr.h"
#include "stub_syscall.h"

   
void  main ( int argc, char *argv[] )
{
	int dcid, p_nr, ret;
	struct DC_usr vm, *dc_usr_ptr;

    	if ( argc != 2) {
 	        printf( "Usage: %s <dcid> \n", argv[0] );
 	        exit(1);
	    }

	dcid = atoi(argv[1]);

	dc_usr_ptr  = &vm;

	printf("getvminfo dcid=%d\n", dcid);
	ret = mnx_getvminfo(dcid, dc_usr_ptr);

	if(ret < 0) {
 	        printf( "return error=%d \n", ret );
 	        exit(1);
	}

	printf(DC_USR_FORMAT,DC_USR_FIELDS(dc_usr_ptr));

 }



