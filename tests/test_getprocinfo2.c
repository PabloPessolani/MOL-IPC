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
#include "./kernel/minix/proc_usr.h"
#include "stub_syscall.h"

   
void  main ( int argc, char *argv[] )
{
	int dcid, p_nr, ret, pid;
	struct proc_usr proc_usr, *proc_usr_ptr;

    	if ( argc != 3) {
 	        printf( "Usage: %s <dcid> <p_nr> \n", argv[0] );
 	        exit(1);
	    }

	dcid = atoi(argv[1]);
	p_nr = atoi(argv[2]);

	proc_usr_ptr  = &proc_usr;

	if( (pid = fork()) == 0) {
		mnx_bind(pid, p_nr);
		printf("CHILD getprocinfo dcid=%d p_nr=%d \n", dcid, p_nr);
		ret = mnx_getprocinfo(dcid, p_nr, proc_usr_ptr);
		if(ret < 0) {
 		        printf( "return error=%d \n", ret );
 	      	  exit(1);
		}
		printf("CHILD " PROC_USR_FORMAT,PROC_USR_FIELDS(proc_usr_ptr));
		
		execvp("/home/jara/mol-ipc/tests/test_getep", argv);

	} else {
		sleep(5);

		printf("PARENT getprocinfo dcid=%d p_nr=%d \n", dcid, p_nr);
		ret = mnx_getprocinfo(dcid, p_nr, proc_usr_ptr);
		if(ret < 0) {
 		        printf( "return error=%d \n", ret );
 	      	  exit(1);
		}
		printf("PARENT " PROC_USR_FORMAT,PROC_USR_FIELDS(proc_usr_ptr));
	}

 }



