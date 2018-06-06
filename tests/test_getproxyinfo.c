#include <asm/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#define _GNU_SOURCE
#define  __USE_GNU
#include <sched.h>
#define cpumask_t cpu_set_t

#include "./kernel/minix/config.h"
#include "./kernel/minix/ipc.h"
#include "./kernel/minix/kipc.h"
#include "./kernel/minix/types.h"
#include "./kernel/minix/proc_usr.h"
#include "stub_syscall.h"

   
void  main ( int argc, char *argv[] )
{
	int pxid, ret;
	struct proc_usr sproc_usr, *sproc_usr_ptr;
	struct proc_usr rproc_usr, *rproc_usr_ptr;

    	if ( argc != 2) {
 	        printf( "Usage: %s <pxid> \n", argv[0] );
 	        exit(1);
	    }

	pxid = atoi(argv[1]);

	sproc_usr_ptr  = &sproc_usr;
	rproc_usr_ptr  = &rproc_usr;

	printf("getproxyinfo pxid=%d\n", pxid);
	ret = mnx_getproxyinfo(pxid, sproc_usr_ptr, rproc_usr_ptr);

	if(ret < 0) {
 	        printf( "return error=%d \n", ret );
 	        exit(1);
	}

	printf("SPROXY: " PROC_USR_FORMAT,PROC_USR_FIELDS(sproc_usr_ptr));
	printf("RPROXY; " PROC_USR_FORMAT,PROC_USR_FIELDS(rproc_usr_ptr));

 }



